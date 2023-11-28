
#include "PDFContent.h"

#include <View.h>
#include <Alert.h>
#include <stdio.h>

#include <URL.h>
#include <ResourceCache.h>

#include "pdf_doc.h"
#include "CachingPositionIO.h"
#include "ConnectedIO.h"

#include "DocFramework.h"
#include "ToolView.h"
#include "PDFPageView.h"

#include <string.h>
#include <OS.h>
#include <TellBrowser.h>
#include <Messenger.h>
#include <Binder.h>

using namespace BPrivate;
using namespace Wagner;

#define MAX_CACHE_BITS 21
#define PREFLIGHT_MAX	128 * 1024

#include <DataIO.h>

class EmptyIO : public BPositionIO {
	public:
							EmptyIO(off_t size);
							~EmptyIO();
							
		virtual	ssize_t		ReadAt(off_t pos, void *buffer, size_t size);
		virtual	ssize_t		WriteAt(off_t pos, const void *buffer, size_t size);
	
		virtual off_t		Seek(off_t position, uint32 seek_mode);
		virtual	off_t		Position() const;
	
		virtual status_t	SetSize(off_t size);
	private:
		off_t				fSize;
		off_t				fPos;
};


PDFContentInstance::PDFContentInstance(PDFContent *content, GHandler *handler, const BMessage &) :
	ContentInstance(content, handler),
	GHandler(),
	fToolbar(NULL),
	fFramework(NULL)
{
	if (content) {
		PDFDocument *doc = content->Document();
		if (doc && doc->InitCheck() == B_OK) {
			fToolbar = new ToolView();
			LoadArt();
		}
	}
}


PDFContentInstance::~PDFContentInstance()
{
}


status_t 
PDFContentInstance::AttachedToView(BView *view, uint32 *)
{
	PDFContent *content = (PDFContent *)GetContent();
	ConnectedIO *connection = content->Connection();
	if (connection)
		connection->Acquire();

	if (!fFramework) {
		PDFDocument *doc = content->Document();
		if (doc && doc->InitCheck() == B_OK) {
			fFramework = new DocFramework(BRect(0, 0, 150, 150), "PDFFramework", fToolbar);
			PDFPageView *pv = new PDFPageView(fFramework->Bounds(), "PageView", this, doc);
			fFramework->SetDocument(pv);
			fFramework->Hide();
		}
	}
	if (fFramework)
		view->AddChild(fFramework);
	MarkDirty();
	return B_OK;
}

status_t 
PDFContentInstance::DetachedFromView()
{
	if (fFramework) {
		fFramework->RemoveSelf();
	}
	PDFContent *content = (PDFContent *)GetContent();
	ConnectedIO *connection = content->Connection();
	if (connection)
		connection->Release();
	
	return B_OK;
}

status_t 
PDFContentInstance::Draw(BView *view, BRect rect)
{
	if (!fFramework) {
		view->SetHighColor(0,0,0);
		view->FillRect(rect, B_SOLID_HIGH);
	}
	return B_OK;
}

status_t 
PDFContentInstance::FrameChanged(BRect newFrame, int32 fullWidth, int32 fullHeight)
{
	ContentInstance::FrameChanged(newFrame, fullWidth, fullHeight);
	if (fFramework) {
		fFramework->MoveTo(newFrame.LeftTop() - fFramework->Parent()->Bounds().LeftTop());
		fFramework->ResizeTo(newFrame.Width(), newFrame.Height());
	}
	return B_OK;
}

status_t 
PDFContentInstance::GetSize(int32 *x, int32 *y, uint32 *outResizeFlags)
{
	if (fFramework) {
		float fx, fy;
		fFramework->GetPreferredSize(&fx, &fy);
		*x = (int32)fx; *y = (int32)fy;
	}
	*outResizeFlags = STRETCH_HORIZONTAL | STRETCH_VERTICAL;
	return B_OK;
}

status_t 
PDFContentInstance::HandleMessage(BMessage *msg)
{
	switch(msg->what) {
		case NEW_CONTENT_INSTANCE:
		{
			//printf("NewContentInstance rcvd\n");
			if (fToolbar) {
				int32 id = -1;
				atom<ContentInstance> instance;
				msg->FindInt32("id", &id);
				msg->FindAtom("instance", instance);
				fToolbar->DeliverArt(id, instance);
			}
			break;
		}
		
		default:
			return GHandler::HandleMessage(msg);
	
	}
	return B_OK;
}

void 
PDFContentInstance::Cleanup()
{
	ContentInstance::Cleanup();
	GHandler::Cleanup();
	if (fFramework && !fFramework->Parent())
		delete fFramework;
}

void 
PDFContentInstance::LoadArt()
{
	//printf("LoadArt()\n");
	LoadImage(iFirstPage, "PDF_firstpage_up.png");
	LoadImage(iPrevPage, "PDF_pageback_up.png");
	LoadImage(iNextPage, "PDF_pageforward_up.png");
	LoadImage(iLastPage, "PDF_lastpage_up.png");

	LoadImage(iFirstPageDown, "PDF_firstpage_down.png");
	LoadImage(iPrevPageDown, "PDF_pageback_down.png");
	LoadImage(iNextPageDown, "PDF_pageforward_down.png");
	LoadImage(iLastPageDown, "PDF_lastpage_down.png");

}

void 
PDFContentInstance::LoadImage(int32 id, const char *image)
{
	URL baseUrl("file://$RESOURCES/AppBitmaps/PDF/");
	URL url(baseUrl, image);
	(void)resourceCache.NewContentInstance(url, id, this, 0, BMessage(),
		securityManager.GetGroupID(url), 0, "");
}




//#pragma mark -

PDFContent::PDFContent(void *handle) :
	Content(handle), fPreFlight(false), fPreFlightPos(0), fFileCache(NULL), fConnection(NULL), fDoc(NULL)
{
}


PDFContent::~PDFContent()
{
	delete fDoc;
	
	// if we are using a file cache it will delete the connection
	if (fFileCache)
		delete fFileCache;
	else
		delete fConnection;
}

ssize_t 
PDFContent::Feed(const void *buffer, ssize_t bufferLen, bool done)
{
//	return B_ERROR;
//	printf("PDFContent::Feed()\n");
	status_t status = B_NO_CONTENT;
	if (fDocLock.Lock()) {
	 	if (!fDoc) {
//			printf("no document!\n");
			PDFAlert alert;
			BPositionIO *source = NULL;
			ssize_t contentLen = 0;
			bool buildDoc = false;
			bool cacheNeeded = true;
			bool connectionNeeded = true;
	
			// if not preflighting - see if we should
			if (!fPreFlight) {
//				printf("not preflighting yet\n");
				Resource *res = GetResource();
				const URL & theURL = res->GetURL();
				contentLen = res->GetContentLength();
	
				const char *scheme = theURL.GetScheme();
				if (strcmp(scheme, "file") == 0) {
					cacheNeeded = false;
					connectionNeeded = true;
				}
				else if (done) {
//					printf("done: no connection needed\n");
					connectionNeeded = false;
					fPreFlight = true;
				}
				else if (contentLen < PREFLIGHT_MAX) {
//					printf("contentLen < PREFLIGHT_MAX: no connection needed\n");
					connectionNeeded = false;
					fPreFlight = true;
				}
			
				if (connectionNeeded) {
//					printf("connection needed\n");
					// we need a connection
					fConnection = new ConnectedIO(theURL, res->GetReferrer());
					status = fConnection->Connect();
//					printf("connect status: %s (%lx)\n", strerror(status), status);
					if (status < B_OK) {
						if (status == B_NO_RANDOM_ACCESS) {
//							printf("no random access allowed: ");
							connectionNeeded = false;
							delete fConnection; fConnection = NULL;
							if (contentLen <= (1 << MAX_CACHE_BITS)) {
//								printf("preflight instead\n");
								fPreFlight = true;
							}
							else {
								//printf("report an error!");
								connectionNeeded = false;
								cacheNeeded = false;
								fPreFlight = false;
			 					source = NULL;
			 					buildDoc = false;
			 					
			 					alert.ThrowAlert(B_NO_RANDOM_ACCESS);
			 					return status;
							}
						}
					}
					else {
//						printf("connection created\n");
						contentLen = status;
					}
				
					if (fConnection) {
						scheme = fConnection->Scheme();
						if (strcmp(scheme, "file") == 0) {
	//						printf("file protocol: no cache needed\n");
							cacheNeeded = false;
							buildDoc = true;
						}
					}
				}
	
				if (cacheNeeded) {
//					printf("cache needed\n");
					if (connectionNeeded)
						source = fConnection;
					else {
//						printf("no connection needed so build EmptyIO\n");
						source = new EmptyIO(contentLen);
					}
					fFileCache = new CachingPositionIO(source, MAX_CACHE_BITS, contentLen);
					source = fFileCache;

					if (fPreFlight) {
//						printf("fill cache for preflight\n");
						fFileCache->FillCache(0, bufferLen, buffer, true);
						fPreFlightPos = bufferLen;
						status = bufferLen;
						if (done)
							buildDoc = true;
					}
					else {
//						printf("fill cache with buffer\n");
						fFileCache->FillCache(0, bufferLen, buffer);
						buildDoc = true;
					}
				}
				else
					source = fConnection;
			}
			else {
//				printf("we are preflighting!\n");
				// we are preflighting
				fFileCache->FillCache(fPreFlightPos, bufferLen, buffer, true);
				fPreFlightPos += bufferLen;
				source = fFileCache;
				status = bufferLen;
				if (done)
					buildDoc = true;
			}
	
			if (buildDoc) {
//				printf("build the document!\n");
				fDoc = new PDFDocument(source, alert);
				if (fDoc->InitCheck() != B_OK) {
					delete fDoc; fDoc = NULL;
					status = B_ERROR;
				}
				else {
					if (fDoc->IsEncrypted())
						fDoc->SetPassword("", 0);
					status = B_FINISH_STREAM;			
				}
			}
		}
		fDocLock.Unlock();
	}
	return status;
}


size_t 
PDFContent::GetMemoryUsage()
{
	return (3 * 1024 * 1024); // lie constructively // sizeof(this);
}

bool 
PDFContent::IsInitialized()
{
	return (fDoc && fDoc->InitCheck() == B_OK) ? true : false;
}

status_t 
PDFContent::CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage &msg)
{
	*outInstance = new PDFContentInstance(this, handler, msg);
	return B_OK;
}

PDFDocument *
PDFContent::Document()
{
	PDFDocument *doc = NULL;
	if (fDocLock.Lock()) {
		doc = fDoc;
		fDocLock.Unlock();
	}
	return doc;
}

ConnectedIO *
PDFContent::Connection()
{
	return fConnection;
}




//#pragma mark -

PDFContentFactory::PDFContentFactory()
{
}


PDFContentFactory::~PDFContentFactory()
{
}

void 
PDFContentFactory::GetIdentifiers(BMessage *into)
{
		into->AddString(S_CONTENT_MIME_TYPES, "application/pdf");
		into->AddString(S_CONTENT_EXTENSIONS, "pdf");
		into->AddString(S_CONTENT_PLUGIN_IDS, "Adobe Acrobat");
		into->AddString(S_CONTENT_PLUGIN_DESCRIPTION, "BeIA PDF Content Viewer (Adobe Acrobat v4.0)");
}

Content *
PDFContentFactory::CreateContent(void *handle, const char *mime, const char *extension)
{
	(void) mime; (void) extension;
	return new PDFContent(handle);
}

bool 
PDFContentFactory::KeepLoaded() const
{
	return false;
}

size_t 
PDFContentFactory::GetMemoryUsage() const
{
	return sizeof(this);
}

extern "C" _EXPORT ContentFactory* make_nth_content(int32 n, image_id , uint32 , ...)
{
	if( n == 0 ) return new PDFContentFactory;
	return 0;
}

//#pragma mark -


EmptyIO::EmptyIO(off_t size) :
	fSize(size), fPos(0)
{
}


EmptyIO::~EmptyIO()
{
}

ssize_t 
EmptyIO::ReadAt(off_t, void *buffer, size_t size)
{
//	printf(" EmptyIO::ReadAt ********************************\n");
	memset(buffer, 0, size);
	return B_PERMISSION_DENIED;
}

ssize_t 
EmptyIO::WriteAt(off_t, const void *, size_t)
{
//	printf(" EmptyIO::WriteAt ********************************\n");
	return B_PERMISSION_DENIED;
}

off_t 
EmptyIO::Seek(off_t position, uint32 seek_mode)
{
	if (seek_mode == SEEK_SET)
		fPos = position;
	else if (seek_mode == SEEK_END)
		fPos = fSize + position;
	else if (seek_mode == SEEK_CUR)
		fPos += position;
	//printf("EmptyIO::Seek(%Ld, %ld) returning %Ld\n", position, seek_mode, fPos);
	return fPos;
}

off_t 
EmptyIO::Position() const
{
	return fPos;
}

status_t 
EmptyIO::SetSize(off_t size)
{
	fSize = size;
	return B_OK;
}
