/*******************************************************************************
/
/	BuyNowPlugin.cpp
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
#include "../../NetPositivePlugins.h"

/*******************************************************************************
/ EXPORTED FUNCTION AND CLASS DEFINITIONS
*******************************************************************************/

#ifdef __GNUC
_EXPORT class BuyNowPluginView;
#else
class _EXPORT BuyNowPluginView;
#endif


// C functions that NetPositive plug-ins are required to export

extern "C" {
	status_t	InitBrowserPlugin(const BMessage *browserInfo, BMessage *pluginInfo);
	void        TerminateBrowserPlugin();
};

long ParseColor(const char *valueStr);

// The class that represents the plug-in view

class BuyNowPluginView : public BView {
public:
					BuyNowPluginView(BMessage *data);
virtual				~BuyNowPluginView();
static	BArchivable	*Instantiate(BMessage *data);

virtual	void		Draw(BRect updateRect);
virtual	void		AttachedToWindow();
virtual void		DetachedFromWindow();
virtual	void		FrameResized(float x, float y);

virtual	void		MessageReceived(BMessage *message);

		void		Tick();
		void		RecalcSize();
static	int32		TickThreadEntry(void *args);

private:
		int32		mFrame;
		thread_id	mTickThread;
		rgb_color	mColor;
		int32		mNumber;
		BString		mParameters;
		BString		mPageURL;
		BMessenger	mPageMessenger;
};



/*******************************************************************************
/ IMPLEMENTATION
*******************************************************************************/

status_t InitBrowserPlugin(const BMessage *browserInfo, BMessage *pluginInfo)
{
	pluginInfo->AddString("PluginName", "BuyNow");
	pluginInfo->AddString("PluginVersion", "1.0");
	pluginInfo->AddInt32("PluginAPISupported", 1);
	
	BMessage dataTypePlugin(B_NETPOSITIVE_DATATYPE_PLUGIN);
	dataTypePlugin.AddString("MIMEType", "application/x-vnd.Be-BuyNowPlugin");
	//the MIMEType field is where you would put audio/x-wav or some other real
	//MIME types that you support.

	// There is no standard filename extension for this MIME type.  Don't add one.
	dataTypePlugin.AddString("MIMEDescription", "The Buy Now NetPositive plug-in");
	
	BMessage dataTypeArchive;
	dataTypeArchive.AddString("add_on", "application/x-vnd.Be-BuyNowPlugin");
	dataTypeArchive.AddString("class", "BuyNowPluginView");

	dataTypePlugin.AddMessage("ViewArchive", &dataTypeArchive);

	pluginInfo->AddMessage("DataTypePlugins", &dataTypePlugin);
	
	return B_OK;
}


void TerminateBrowserPlugin(BMessenger *pluginInstance)
{
#if 0
	if (!pluginInstance)
		return;
		
	BLooper *looper;
	pluginInstance->Target(&looper);
	looper->Lock();
	looper->Quit();
			
	delete pluginInstance;
#endif
}


BuyNowPluginView::BuyNowPluginView(BMessage *data)
	// Build the BView using custom parameters.  Don't pass the BMessage on to
	// it.  The frame rect doesn't matter, because NetPositive will resize it anyway.
	: BView(BRect(0,0,0,0), "BuyNow", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_FRAME_EVENTS)
{
	mTickThread = 0;
	mColor.red = 0;
	mColor.green = 0;
	mColor.blue = 100;
	mNumber = 1;
}


BuyNowPluginView::~BuyNowPluginView()
{
	if (mTickThread > 0)
		kill_thread(mTickThread);
}


BArchivable* BuyNowPluginView::Instantiate(BMessage *data)
{
	return new BuyNowPluginView(data);
}


void BuyNowPluginView::AttachedToWindow()
{
	BView::AttachedToWindow();
	
	SetViewColor(mColor);
	SetLowColor(mColor);
	SetHighColor(255, 255, 255);
	mFrame = 0;
	RecalcSize();
	
	mTickThread = spawn_thread(BuyNowPluginView::TickThreadEntry, "BUY NOW Tick", B_NORMAL_PRIORITY, this);
	resume_thread(mTickThread);
}

void BuyNowPluginView::DetachedFromWindow()
{
	BMessage msg(B_NETPOSITIVE_SAVE_INSTANCE_DATA);
	msg.AddString("PageURL", mPageURL.String());
	msg.AddString("PluginTag", mParameters.String());
	BMessage instanceData;
	instanceData.AddInt32("number", mNumber + 1);
	msg.AddMessage("InstanceData", &instanceData);
	mPageMessenger.SendMessage(&msg);
}

void BuyNowPluginView::RecalcSize()
{
	BFont f, nf;
	font_height fh;
	float s;
	BRect r = Bounds();
			
	GetFont(&f);
	nf = f;
	nf.GetHeight(&fh);
	s = nf.Size();
	
	while ((fh.ascent + fh.descent + fh.leading) < (r.bottom - 10)/2) {
		s++;
		nf.SetSize(s);
		nf.GetHeight(&fh);
	}

	SetFont(&nf);
}

void BuyNowPluginView::FrameResized(float x, float y)
{
	BView::FrameResized(x, y);
	RecalcSize();
}

int32 BuyNowPluginView::TickThreadEntry(void *args)
{
	BuyNowPluginView *view = (BuyNowPluginView *)args;
	while(view && view->Window()) {
		view->Window()->Lock();
		view->Invalidate();
		view->mFrame++;
		view->Window()->Unlock();
		snooze(500000);
	}
	return 0;
}



void BuyNowPluginView::Tick()
{
	mFrame++;
	Invalidate();
}


void BuyNowPluginView::Draw(BRect udpateRect)
{
	// Make sure we draw the whole frame contents.  NetPositive may use
	// B_DRAW_ON_CHILDREN in the future.
	
	FillRect(Bounds(), B_SOLID_LOW);
	if (mFrame % 2 == 0) {
		return;
	}
	
	BRect r = Bounds();
	float w;
	font_height fh;
	GetFontHeight(&fh);
	float leftover = r.Height() - (fh.ascent + fh.leading) * 2;

	w = StringWidth("BUY");
	MovePenTo((r.right - w) / 2, leftover / 4 + fh.ascent + fh.leading);
	DrawString("BUY");
	
	char str[16];
	sprintf(str, "%ld", mNumber);
	w = StringWidth(str);
	MovePenTo((r.right - w) / 2, leftover * 3 / 4 + (fh.ascent*2) + (fh.leading*2));
	DrawString(str);
}

void BuyNowPluginView::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case B_NETPOSITIVE_INIT_INSTANCE: {
			// Parse out the tag attributes.
			BMessage params;
			message->FindMessage("Parameters", &params);
			mParameters = message->FindString("ParameterString");
			mPageURL = message->FindString("URL");
			message->FindMessenger("BrowserMessenger", &mPageMessenger);
			int32 i = 0;
			const char *attrName;
			const char *attrValue;
			while ((attrName = params.FindString("Attribute", i)) != NULL) {
				// Deal with the BGCOLOR attribute.
				if (strcasecmp(attrName, "BGCOLOR") == 0 &&
					(attrValue = params.FindString("Value", i)) != 0) {
					long color = ParseColor(attrValue);
					mColor.red = (color & 0xff0000) >> 16;
					mColor.green = (color & 0xff00) >> 8;
					mColor.blue = color & 0xff;
				}
				i++;
			}
			
			BMessage instanceData;
			message->FindMessage("InstanceData", &instanceData);
			if (instanceData.FindInt32("number", &mNumber) != B_OK)
				mNumber = 1;
			break;
		}
	}
}


long ParseColor(const char *valueStr)
{
	// A surprising number of sites use the letter 'O' instead of the number
	// zero in color strings.  May the fleas of a thousand camels infest the pubic
	// hair of the site authors responsible.

	BString mungedValueStr(valueStr);
	mungedValueStr.ReplaceAll('O','0');
	mungedValueStr.RemoveAll("\"");

	long value;
	int32 count = mungedValueStr.Length();
		
	if (mungedValueStr[0] == '#') {
		if (sscanf(mungedValueStr.String(),"#%lX",&value) == 1)
			return value;
		
		long r,g,b;
		if (sscanf(mungedValueStr.String(),"#%lX %lX %lX",&r,&g,&b) == 3) {		// Metrowerks does this
			value = (r << 16) | (g << 8) | b;						// Big/Little
			return value;
		}
		return -1;
	}

//	Color lacks leading #

	if (count == 6 || count == 8) {
		if (sscanf(mungedValueStr.String(),"%lX",&value) == 1)
			return value;
	}
	return -1;
}	

// Since this plug-in is built as an executable, include a dummy main()

int main(int argc, void** argv)
{
	return 0;
}


