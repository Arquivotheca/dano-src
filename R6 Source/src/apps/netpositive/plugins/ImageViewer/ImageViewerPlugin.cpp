/*******************************************************************************
/
/	File:			ImageViewerPlugin.cpp
/
/   Description:    
/
/	Copyright 1999, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#include <View.h>
#include <Window.h>
#include <Looper.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <String.h>
#include <TranslationUtils.h>
#include <Bitmap.h>
#include <Autolock.h>
#include "../../NetPositive.h"
#include "../../NetPositivePlugins.h"
#include "../../NetPositiveStreamIO.h"

/*******************************************************************************
/ EXPORTED FUNCTION AND CLASS DEFINITIONS
*******************************************************************************/

#ifdef __GNUC
_EXPORT class ImageViewerPluginView;
#else
class _EXPORT ImageViewerPluginView;
#endif


// C functions that NetPositive plug-ins are required to export

extern "C" {
	status_t 	InitBrowserPlugin(const BMessage *browserInfo, BMessage *pluginInfo);
	void        TerminateBrowserPlugin();
};

long ParseColor(const char *valueStr);

// The class that represents the plug-in view

class ImageViewerPluginView : public BView {
public:
					ImageViewerPluginView(BMessage *data);
virtual				~ImageViewerPluginView();
static	BArchivable	*Instantiate(BMessage *data);

virtual	void		Draw(BRect updateRect);
virtual	void		AttachedToWindow();

virtual	void		MessageReceived(BMessage *message);

static	int32		ConvertBitmapThread(void *args);
static	int32		StatusThread(void *args);

private:
		BMessenger	mBrowserMessenger;
		BNetPositiveStreamIO	*mStream;
		BBitmap		*mBitmap;
		BString		mURL;
		thread_id	mServiceThread;
		thread_id	mStatusThread;
};



/*******************************************************************************
/ PRIVATE CLASSES
*******************************************************************************/


status_t InitBrowserPlugin(const BMessage *browserInfo, BMessage *pluginInfo)
{
	pluginInfo->AddString("PluginName", "ImageViewer");
	pluginInfo->AddString("PluginVersion", "1.0");
	pluginInfo->AddInt32("PluginAPISupported", 1);
	
	BMessage dataTypePlugin(B_NETPOSITIVE_DATATYPE_PLUGIN);
	dataTypePlugin.AddString("MIMEType", "image/*");

	// There is no standard filename extension for this MIME type.  Don't add one.

	dataTypePlugin.AddString("MIMEDescription", "The Image Viewer NetPositive plug-in");
	
	BMessage dataTypeArchive;
	dataTypeArchive.AddString("add_on", "application/x-vnd.Be-ImageViewerPlugin");

	dataTypeArchive.AddString("class", "ImageViewerPluginView");


	dataTypePlugin.AddMessage("ViewArchive", &dataTypeArchive);

	pluginInfo->AddMessage("DataTypePlugins", &dataTypePlugin);

	return B_OK;
}


void TerminateBrowserPlugin()
{
}


ImageViewerPluginView::ImageViewerPluginView(BMessage *data)
	// Build the BView using custom parameters.  Don't pass the BMessage on to
	// it.  The frame rect doesn't matter, because NetPositive will resize it anyway.
	: BView(BRect(0,0,0,0), "ImageViewer", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_FRAME_EVENTS)
{
	mServiceThread = -1;
	mStatusThread = -1;
	mStream = 0;
	mBitmap = 0;
}


ImageViewerPluginView::~ImageViewerPluginView()
{

	if (mServiceThread > 0)
		kill_thread(mServiceThread);
	if (mStatusThread > 0)
		kill_thread(mStatusThread);
	delete mBitmap;
	if (mStream)
		mStream->Dereference();
}


BArchivable* ImageViewerPluginView::Instantiate(BMessage *data)
{
	return new ImageViewerPluginView(data);
}


void ImageViewerPluginView::AttachedToWindow()
{
	BView::AttachedToWindow();
}



void ImageViewerPluginView::Draw(BRect udpateRect)
{
	// Make sure we draw the whole frame contents.  NetPositive may use
	// B_DRAW_ON_CHILDREN in the future.
	
	BRect bounds = Bounds();
	
	FillRect(bounds, B_SOLID_LOW);
	
	if (mBitmap) {
		BRect bitmapBounds = mBitmap->Bounds();
		DrawBitmapAsync(mBitmap,
			BPoint((bounds.Width() - bitmapBounds.Width()) / 2, (bounds.Height() - bitmapBounds.Height()) / 2));
	}

}

void ImageViewerPluginView::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case B_NETPOSITIVE_INIT_INSTANCE: {
			// Parse out the tag attributes.
			BMessage params;
			message->FindMessenger("BrowserMessenger", &mBrowserMessenger);
			message->FindMessage("Parameters", &params);
			int32 i = 0;
			const char *attrName;
			const char *attrValue;
			while ((attrName = params.FindString("Attribute", i)) != NULL) {
				// Deal with the BGCOLOR attribute.
				if (strcasecmp(attrName, "SRC") == 0 &&
					(attrValue = params.FindString("Value", i)) != 0) {
						mURL = attrValue;
						mServiceThread = spawn_thread(ConvertBitmapThread, "Image Viewer conversion", B_NORMAL_PRIORITY, this);

						resume_thread(mServiceThread);
				}
				i++;
			}
			break;
		}
	}
}

int32 ImageViewerPluginView::StatusThread(void *args)
{
	ImageViewerPluginView *view = (ImageViewerPluginView *)args;
	
	if (!view)
		return 0;
		
	BNetPositiveStreamIO *stream = view->mStream;

	BString msg;
	
	BMessage status(B_NETPOSITIVE_STATUS_MESSAGE);
	status.AddString("Message", "");

	while (stream->GetError() == B_NO_ERROR && (stream->ContentLength() == 0 || stream->AmountWritten() < stream->ContentLength())) {
		msg = "";
		msg << "Loading image " << view->mURL << "  " << stream->AmountWritten() <<
			  " of " << stream->ContentLength() << " bytes";

		status.ReplaceString("Message", msg.String());
		view->mBrowserMessenger.SendMessage(&status);
		
		snooze(500000);
	}
		
	msg = "Done loading image ";
	msg += view->mURL.String();
	status.ReplaceString("Message", msg.String());
	view->mBrowserMessenger.SendMessage(&status);

	stream->Dereference();
		
	view->mStream = 0;
	view->mStatusThread = -1;

	return 0;
}

int32 ImageViewerPluginView::ConvertBitmapThread(void *args)
{
	ImageViewerPluginView *view = (ImageViewerPluginView *)args;
	
	if (!view)
		return 0;
		
	BMessage requestStream(B_NETPOSITIVE_OPEN_URL);
	requestStream.AddString("url", view->mURL.String());
	requestStream.AddBool("ReturnStream", true);

	BMessage reply;
	view->mBrowserMessenger.SendMessage(&requestStream, &reply);
	BMessage streamArchive;
	(BNetPositiveStreamIO *)reply.FindPointer("Stream", (void **)&view->mStream);
	if (view->mStream) {
		BNetPositiveStreamIO *stream = view->mStream;
		
		stream->Reference();
		
		view->mStatusThread = spawn_thread(StatusThread, "Image Viewer Status Thread", B_LOW_PRIORITY, args);
		resume_thread(view->mStatusThread);
		
		view->mBitmap = BTranslationUtils::GetBitmap(stream);

		stream->Dereference();

		// There's a race condition here if the view kills this thread and then tries to delete the stream.  It
		// may not get deleted.
			
		view->mServiceThread = -1;
		
		BAutolock lock(view->Window());
		if (view->Window())
			view->Invalidate();
	}
	
	return 0;
}


// Since this plug-in is built as an executable, include a dummy main()

int main(int argc, void** argv)
{
	return 0;
}