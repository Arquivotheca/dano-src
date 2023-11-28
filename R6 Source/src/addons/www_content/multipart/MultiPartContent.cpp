#include <Bitmap.h>
#include <View.h>
#include <Debug.h>
#include <stdio.h>
#include <Message.h>
#include <Screen.h>
#include <DataIO.h>
#include <Resource.h>
#include <ResourceCache.h>
#include <Window.h>
#include "Gehnaphore.h"
#include "Content.h"
#include "Timer.h"
#include "ContentView.h"

using namespace Wagner;

class MultiPartContentInstance : public ContentInstance, public GHandler {
public:
	MultiPartContentInstance(Content*, GHandler*);
	virtual ~MultiPartContentInstance();
	virtual status_t GetSize(int32 *width, int32 *height, uint32 *outFlags);
	virtual	status_t AttachedToView(BView *view, uint32 *contentFlags);
	virtual	status_t DetachedFromView();
	virtual	status_t FrameChanged(BRect newFrame, int32 fullWidth, int32 fullHeight);
	virtual status_t HandleMessage(BMessage*);
	virtual	status_t Draw(BView *into, BRect exposed);
	void Cleanup();

	virtual	void MouseDown(BPoint where, const BMessage *event=NULL);
	virtual	void MouseUp(BPoint where, const BMessage *event=NULL);
	virtual	void MouseMoved(BPoint where, uint32 code, const BMessage *a_message, const BMessage *event=NULL);
	virtual	void KeyDown(const char *bytes, int32 numBytes, const BMessage *event=NULL);
	virtual	void KeyUp(const char *bytes, int32 numBytes, const BMessage *event=NULL);

	virtual	void Notification(BMessage *msg);
	virtual	status_t ContentNotification(BMessage *msg);


private:
	BView *fParentView;
	ContentInstance *fSlaveInstance;
	BRect fFrame;
	float fFullWidth;
	float fFullHeight;
};

class MultiPartContent : public Content {
public:
	MultiPartContent(void *handle);
	virtual ~MultiPartContent();
	virtual ssize_t Feed(const void *buffer, ssize_t bufferLen, bool done=false);
	virtual size_t GetMemoryUsage();
	virtual	void Cleanup();
	
private:
	virtual status_t CreateInstance(ContentInstance **outInstance, GHandler*, const BMessage&);
	void ParseHeader(char *data, int size);
	void HandleContentData();
	
	char fSubType[255];
	Gehnaphore fLock;
	char fSectionDelimiter[255];
	int fDelimiterLen;
	BMallocIO fData;
	Resource *fSlaveResource;
	enum State {
		kReadHeader,
		kReadBody
	} fState;
	MultiPartContentInstance *fInstance;
	friend class MultiPartContentInstance;
};


class MultiPartContentFactory : public ContentFactory {
public:
	virtual void GetIdentifiers(BMessage* into) {
		 // BE AWARE: Any changes you make to these identifiers should
		 // also be made in the 'addattr' command in the makefile.
		into->AddString(S_CONTENT_MIME_TYPES, "multipart/x-mixed-replace");
	}
	
	virtual Content* CreateContent(void* handle, const char*, const char*) {
		return new MultiPartContent(handle);
	}
};

MultiPartContentInstance::MultiPartContentInstance(Content *content, GHandler *handler)
	: 	ContentInstance(content, handler),
		fParentView(0),
		fSlaveInstance(0),
		fFullWidth(0),
		fFullHeight(0)
{
	((MultiPartContent*) content)->fInstance = this;
}

MultiPartContentInstance::~MultiPartContentInstance()
{
}

status_t MultiPartContentInstance::GetSize(int32 *width, int32 *height, uint32 *outFlags)
{
	if (fSlaveInstance)
		return fSlaveInstance->GetSize(width, height, outFlags);
		
	*width = 0;
	*height = 0;
	*outFlags = STRETCH_VERTICAL | STRETCH_HORIZONTAL;
	return B_OK;
}

status_t MultiPartContentInstance::AttachedToView(BView *view, uint32 *contentFlags)
{
	fParentView = view;
	if (fSlaveInstance) {
		fSlaveInstance->AttachedToView(view, contentFlags);
		MarkDirty();
	}

	return B_OK;
}

status_t MultiPartContentInstance::DetachedFromView()
{
	fParentView = 0;
	if (fSlaveInstance)
		fSlaveInstance->DetachedFromView();

	return B_OK;
}

status_t MultiPartContentInstance::FrameChanged(BRect newFrame, int32 fullWidth,
	int32 fullHeight)
{
	fFrame = newFrame;
	fFullWidth = fullWidth;
	fFullHeight = fullHeight;
	if (fSlaveInstance)
		fSlaveInstance->FrameChanged(newFrame, fullWidth, fullHeight);

	return B_OK;
}

status_t MultiPartContentInstance::HandleMessage(BMessage *msg)
{
	if (msg->what == NEW_CONTENT_INSTANCE) {
		atom<ContentInstance> instance;
		if (msg->FindAtom("instance",instance) != B_OK)
			debugger("couldn't find instance");
			
		ContentInstance *oldInstance = fSlaveInstance;
		fSlaveInstance = instance;
		fSlaveInstance->Acquire();
		fSlaveInstance->SetID(ID());
		fSlaveInstance->SetHandler(this);
		fSlaveInstance->SetParentContent(this);

		if (oldInstance)
			fSlaveInstance->UsurpPredecessor(oldInstance);

		uint32 contentFlags = 0;
		if (fParentView && fParentView->Window()->Lock()) {
			fSlaveInstance->AttachedToView(fParentView, &contentFlags);
			fSlaveInstance->FrameChanged(fFrame, fFullWidth, fFullHeight);
			fParentView->Window()->Unlock();
		}
	} else if (fParentView) {
		if (msg->what == bmsgContentDirty) {
			BRegion tmp;
			fSlaveInstance->FetchDirty(tmp);
			for (int i = 0; i < tmp.CountRects(); i++)
				MarkDirty(&tmp.RectAt(i));			
		} else {
			msg->PrintToStream();
			fParentView->Window()->PostMessage(msg, fParentView);
		}
	}
	
	return B_OK;
}

status_t MultiPartContentInstance::Draw(BView *into, BRect exposed)
{
	if (fSlaveInstance)
		return fSlaveInstance->Draw(into, exposed);

	return B_OK;
}

void MultiPartContentInstance::MouseDown(BPoint where, const BMessage *event)
{
	if (fSlaveInstance)
		fSlaveInstance->MouseDown(where, event);
}

void MultiPartContentInstance::MouseUp(BPoint where, const BMessage *event)
{
	if (fSlaveInstance)
		fSlaveInstance->MouseUp(where, event);
}

void MultiPartContentInstance::MouseMoved(BPoint where, uint32 code, const BMessage *a_message, const BMessage *event)
{
	if (fSlaveInstance)
		fSlaveInstance->MouseMoved(where, code, a_message, event);
}

void MultiPartContentInstance::KeyDown(const char *bytes, int32 numBytes, const BMessage *event)
{
	if (fSlaveInstance)
		fSlaveInstance->KeyDown(bytes, numBytes, event);
}

void MultiPartContentInstance::KeyUp(const char *bytes, int32 numBytes, const BMessage *event)
{
	if (fSlaveInstance)
		fSlaveInstance->KeyUp(bytes, numBytes, event);
}

void MultiPartContentInstance::Notification(BMessage *msg)
{
	if (fSlaveInstance)
		fSlaveInstance->Notification(msg);
}

status_t MultiPartContentInstance::ContentNotification(BMessage *msg)
{
	if (fSlaveInstance)
		return fSlaveInstance->ContentNotification(msg);
		
	return B_OK;
}

size_t MultiPartContent::GetMemoryUsage()
{
	return 0;
}

MultiPartContent::MultiPartContent(void *handle)
	:	Content(handle),
		fState(kReadBody),
		fSlaveResource(0),
		fInstance(0)
{
	strcpy(fSectionDelimiter, "");
	strcpy(fSubType, "");
}

void MultiPartContent::Cleanup()
{
	GehnaphoreAutoLock l(fLock);
	Content::Cleanup();
}

void MultiPartContentInstance::Cleanup()
{
	ContentInstance::Cleanup();
	if (fSlaveInstance)
		fSlaveInstance->Release();

	GHandler::Cleanup();
}

MultiPartContent::~MultiPartContent()
{
	delete fSlaveResource;
}

ssize_t MultiPartContent::Feed(const void *d, ssize_t count, bool done)
{
	if (fSectionDelimiter[0] == '\0') {
		GetResource()->SetCachePolicy(CC_NO_CACHE);
		
		const char *c = GetResource()->GetContentType();
		while (*c && *c != '=')
			c++;
			
		if (*c == '=')
			c++;
			
		strcpy(fSectionDelimiter, "--");
		strcat(fSectionDelimiter, c);
		fDelimiterLen = strlen(fSectionDelimiter);
		PRINT(("MultiPartContent: part delimiter is \"%s\"\n", fSectionDelimiter));
	}

	switch (fState) {
		case kReadHeader: {
			int lfcount = 0;
			for (int index = 0; index < count; index++) {
				char c = ((char*)d)[index];
				if (c == '\r')
					continue;
					
				if (c == '\n')
					lfcount++;
				else
					lfcount = 0;
					
				if (lfcount >= 2) {
					ParseHeader((char*) d, index);
					fState = kReadBody;
					return index + 1;
				}
			}
			
			return done ? count : 0;
		}

		case kReadBody:
			if (done) {
				fData.Write(d, count);
				if (fData.BufferLength() > 0)
					HandleContentData();
				
				return count;
			} else {
				int matchLevel = 0;
				for (int index = 0; index < count; index++) {
					char c = ((char*)d)[index];
					if (c == fSectionDelimiter[matchLevel])
						matchLevel++;
					else
						matchLevel = 0;
						
					if (matchLevel == fDelimiterLen) {
						if (index > fDelimiterLen)
							fData.Write(d, index - fDelimiterLen);
							
						if (fData.BufferLength() > 0)
							HandleContentData();
		
						// Start over
						matchLevel = 0;
						fState = kReadHeader;
						strcpy(fSubType, "");
						return index + 1;
					}
				}
			
				if (matchLevel > 0)
					return 0;
			}
	
			break;
	}

	fData.Write(d, count);
	return count;
}

status_t MultiPartContent::CreateInstance(ContentInstance **outInstance, GHandler *handler,
	const BMessage&)
{
	GehnaphoreAutoLock l(fLock);
	*outInstance = new MultiPartContentInstance(this, handler);
	return B_OK;
}

void MultiPartContent::ParseHeader(char *data, int size)
{
	const char *kType = "content-type: ";
	int len = strlen(kType);	
	for (const char *c = (const char*) data; c; c++) {
		if (strncasecmp(c, kType, len) == 0) {
			c += len;
			char *d = fSubType;
			while (*c && *c != '\n' && *c != '\r')
				*d++ = *c++;

			*d = 0;				
			return;
		}
	}
}

void MultiPartContent::HandleContentData()
{
	if (strlen(fSubType) == 0)
		return;	// skip

	fSlaveResource = new Resource(GetResource()->GetURL());
	fSlaveResource->SetState(kStateLoading);
	fSlaveResource->AcquireReference();
	BMessage userData;
	fSlaveResource->CreateContentInstance(fInstance, 0, userData, 0,
		fSlaveResource->GetURL());

	fSlaveResource->SetContentType(fSubType);
	
	if (!fInstance)
		debugger("oops");
	
	fData.Seek(0, SEEK_SET);
	fSlaveResource->ReadFromStream(&fData, fData.BufferLength());
	fData.Seek(0, SEEK_SET);
	fData.SetSize(0);
}

extern "C" _EXPORT ContentFactory* make_nth_content(int32 n, image_id you,
	uint32 flags, ...)
{
	if (n == 0) return new MultiPartContentFactory;
	return 0;
}
