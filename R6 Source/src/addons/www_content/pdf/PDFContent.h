#ifndef _PDF_CONTENT_H_
#define _PDF_CONTENT_H_

#include <Content.h>
#include <Protocol.h>
using namespace Wagner;

class PDFContentFactory : public ContentFactory
{
	public:
									PDFContentFactory();
									~PDFContentFactory();
	virtual void 					GetIdentifiers(BMessage* into);
	virtual Content* 				CreateContent(void* handle, const char* mime, const char* extension);
	virtual bool 					KeepLoaded() const;
	virtual size_t 					GetMemoryUsage() const;
};

namespace BPrivate {
	class PDFDocument;
};

using namespace BPrivate;

#include <Locker.h>
#include <OS.h>
class CachingPositionIO;
class ConnectedIO;

class PDFContent : public Content
{
	public:
									PDFContent(void *handle);
									~PDFContent();
		virtual	ssize_t				Feed(const void *buffer, ssize_t bufferLen, bool done=false);
		virtual	size_t				GetMemoryUsage();
		virtual bool				IsInitialized();

		PDFDocument *				Document();
		ConnectedIO *				Connection();
	private:									
		virtual status_t			CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage &msg);
		bool						fPreFlight;
		off_t						fPreFlightPos;
		CachingPositionIO *			fFileCache;
		ConnectedIO *				fConnection;
		PDFDocument *				fDoc;
		BLocker						fDocLock;
};


class DocFramework;
class PDFPageView;
class ToolView;
class PDFContentInstance : public ContentInstance, public GHandler
{
	public:
									PDFContentInstance(PDFContent *content, GHandler *handler, const BMessage &msg);
	protected:
									~PDFContentInstance();
	public:
		virtual	status_t			AttachedToView(BView *view, uint32 *contentFlags);
		virtual	status_t			DetachedFromView();
		virtual	status_t			Draw(BView *into, BRect exposed);
		virtual	status_t			FrameChanged(BRect newFrame, int32 fullWidth, int32 fullHeight);
		virtual	status_t			GetSize(int32 *x, int32 *y, uint32 *outResizeFlags);
		virtual status_t			HandleMessage(BMessage *message);
		virtual	void				Cleanup();
	private:
		void						LoadArt();
		void						LoadImage(int32 id, const char *image);
		ToolView *					fToolbar;
		DocFramework*				fFramework;
};


#endif
