/*******************************************************************************
/
/	BigThingyPlugin.cpp
/
/	Copyright 1999, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#include <Application.h>
#include <Screen.h>
#include <Bitmap.h>
#include <stdlib.h>
#include <View.h>
#include <Window.h>
#include <Looper.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <String.h>
#include "../../NetPositivePlugins.h"

#define NUM_LEVELS 500

/*******************************************************************************
/ EXPORTED FUNCTION AND CLASS DEFINITIONS
*******************************************************************************/

#ifdef __GNUC
_EXPORT class SysInfoView;
#else
class _EXPORT SysInfoView;
#endif

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

// C functions that NetPositive plug-ins are required to export

extern "C" {
	status_t	InitBrowserPlugin(const BMessage *browserInfo, BMessage *pluginInfo);
	void        TerminateBrowserPlugin();
};

status_t InitBrowserPlugin(const BMessage *browserInfo, BMessage *pluginInfo)
{
	pluginInfo->AddString("PluginName", "BigThingy");
	pluginInfo->AddString("PluginVersion", "1.0");
	pluginInfo->AddInt32("PluginAPISupported", 1);
	
	BMessage dataTypePlugin(B_NETPOSITIVE_DATATYPE_PLUGIN);
	dataTypePlugin.AddString("MIMEType", "application/x-vnd.Be-BigThingyPlugin");
	// There is no standard filename extension for this MIME type.  Don't add one.
	dataTypePlugin.AddString("MIMEDescription", "The Big Thingy NetPositive plug-in");
	
	BMessage dataTypeArchive;
	dataTypeArchive.AddString("add_on", "application/x-vnd.Be-BigThingyPlugin");
	dataTypeArchive.AddString("class", "SysInfoView");

	dataTypePlugin.AddMessage("ViewArchive", &dataTypeArchive);

	pluginInfo->AddMessage("DataTypePlugins", &dataTypePlugin);
	
	return B_OK;
}

void TerminateBrowserPlugin(BMessenger *pluginInstance)
{
}


class SysInfoView : public BView {
public:
					SysInfoView(BMessage *data);
	BArchivable* 	Instantiate(BMessage *data);
	void 			MessageReceived(BMessage *message);
	
	virtual void	Draw(BRect updateRect);
	virtual void	Pulse();
	virtual void	AttachedToWindow();
	void			DoDraw();
	
private:
	bigtime_t		mLastUpdateTime;
	bigtime_t		mCurrUpdateTime;

	bigtime_t		mLastActiveCPUTime;
	bigtime_t		mCurrActiveCPUTime;
	float			mLastActiveCPUBar;
	int32			mLastUsedPages;
	int32			mCurrUsedPages;
	float			mLastUsedPagesBar;
	int32			mTotalPages;
	int32			mLastPageFaults;
	int32			mCurrPageFaults;
	float			mLastPageFaultsBar;
	
	BRect			mFrame;
	float			mBarHeight;
	rgb_color		bgcolor;
	float			cpu_levels[NUM_LEVELS];
	int				cpu_index;
	float			page_levels[NUM_LEVELS];
	int 			page_index;
	float			mem_levels[NUM_LEVELS];
	int 			mem_index;
};

SysInfoView::SysInfoView(BMessage *data)
	: BView(BRect(0,0,0,0), "Uptime", B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED | B_FRAME_EVENTS), mLastUpdateTime(0),
	mLastActiveCPUTime(0), mCurrActiveCPUTime(0), mLastUsedPages(0), mCurrUsedPages(0),
	mTotalPages(0), mLastPageFaults(0), mCurrPageFaults(0)
{
	bgcolor.red = 0;
	bgcolor.green = 0;
	bgcolor.blue = 0;
	bgcolor.alpha = 0;
	for(int i=0; i<NUM_LEVELS; ++i){
		cpu_levels[i] = 0;
		page_levels[i] = 0;
		mem_levels[i] = 0;
	}
	cpu_index = 0;
	page_index = 0;
	mem_index = 0;
}

BArchivable* SysInfoView::Instantiate(BMessage *data)
{
	return new SysInfoView(data);
}

void SysInfoView::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case B_NETPOSITIVE_INIT_INSTANCE: {
			// Parse out the tag attributes.
			BMessage params;
			message->FindMessage("Parameters", &params);
			int32 i = 0;
			const char *attrName;
			const char *attrValue;
			while ((attrName = params.FindString("Attribute", i)) != NULL) {
				// Deal with the BGCOLOR attribute.
				if (strcasecmp(attrName, "BGCOLOR") == 0 &&
					(attrValue = params.FindString("Value", i)) != 0) {
					long color = ParseColor(attrValue);
					bgcolor.red = (color & 0xff0000) >> 16;
					bgcolor.green = (color & 0xff00) >> 8;
					bgcolor.blue = color & 0xff;
					break;
				}
				i++;
			}
		}
	}
}

void SysInfoView::Draw(BRect updateRect)
{
	mLastActiveCPUBar = 0.0;
	mLastUsedPagesBar = 0.0;
	mLastPageFaultsBar = 0.0;
	SetHighColor(bgcolor);
	FillRect(Bounds());
	DoDraw();
	BView::Draw(updateRect);
}

void SysInfoView::DoDraw()
{
	BRect rect(Bounds());
	float width = rect.Width();
	float height = rect.top - rect.bottom;
	float bottom = rect.bottom;
	float levelWidth = (float)width / (float)NUM_LEVELS;
	BRect levelRect(Bounds());
	
	int l_index = 0;
//-----------
	BBitmap *phaseBitmap = new BBitmap(Bounds(), B_RGB_32_BIT, true);
	BView *phaseView = new BView(phaseBitmap->Bounds(),"phaseView",B_FOLLOW_ALL,B_WILL_DRAW);
	phaseBitmap->AddChild(phaseView);
	phaseBitmap->Lock();
	phaseView->SetDrawingMode(B_OP_OVER);

	//clear the rectangle
	phaseView->SetHighColor(bgcolor);
	phaseView->FillRect(rect);
	
	//draw memory meter
	phaseView->SetHighColor(0, 0, 255);
	mem_levels[mem_index] = bottom + ((float)mCurrUsedPages) / ((float)mTotalPages) * height;
	l_index = mem_index;
	int draw_index = 0;
	do {
		levelRect.top = mem_levels[l_index];
		levelRect.bottom = levelRect.top + 1;
		levelRect.left = levelWidth * draw_index++;
		levelRect.right = levelRect.left + levelWidth;
		if (mem_levels[l_index] > 0.0)
			phaseView->FillRect(levelRect);
		l_index = l_index - 1;
		if(l_index < 0) l_index = NUM_LEVELS - 1;
	} while (l_index != mem_index);
	mem_index = (mem_index + 1) % NUM_LEVELS;	

	//draw cpu 
	phaseView->SetHighColor(255, 0, 0);
	cpu_levels[cpu_index] = bottom + ((float)(mCurrActiveCPUTime - mLastActiveCPUTime)) / ((float)(mCurrUpdateTime - mLastUpdateTime)) * height;
	l_index = cpu_index;
	draw_index = 0;
	do {
		levelRect.top = cpu_levels[l_index];
		levelRect.bottom = levelRect.top + 1;
		levelRect.left = levelWidth * draw_index++;
		levelRect.right = levelRect.left + levelWidth;
		if (cpu_levels[l_index] > 0.0)
			phaseView->FillRect(levelRect);
		l_index = l_index - 1;
		if(l_index < 0) l_index = NUM_LEVELS - 1;
	} while (l_index != cpu_index);
	cpu_index = (cpu_index + 1) % NUM_LEVELS;	

	//draw paging
	phaseView->SetHighColor(0, 255, 0);
	page_levels[page_index] = bottom + ((float)(mCurrPageFaults - mLastPageFaults)) * (height / 50.0);
	l_index = page_index;
	draw_index = 0;
	do {
		levelRect.top = page_levels[l_index];
		levelRect.bottom = levelRect.top + 1;
		levelRect.left = levelWidth * draw_index++;
		levelRect.right = levelRect.left + levelWidth;
		if (page_levels[l_index] > 0.0)
			phaseView->FillRect(levelRect);
		l_index = l_index - 1;
		if(l_index < 0) l_index = NUM_LEVELS - 1;
	} while (l_index != page_index);
	page_index = (page_index + 1) % NUM_LEVELS;	

	//prep for next draw
	mLastActiveCPUTime = mCurrActiveCPUTime;
	mLastUsedPages = mCurrUsedPages;
	mLastPageFaults = mCurrPageFaults;
	mLastUpdateTime = mCurrUpdateTime;
	
	phaseView->Sync();
	phaseBitmap->Unlock();

	DrawBitmap(phaseBitmap, BPoint(0,0));
	delete phaseBitmap;
}

void SysInfoView::AttachedToWindow()
{
	rgb_color	c = bgcolor;

	SetDrawingMode(B_OP_OVER);

	SetViewColor(c);
	SetLowColor(c);
	Invalidate();

	mFrame = Bounds();
	mBarHeight = mFrame.Height() / 3.0;
	Pulse();
	Invalidate();
}

void SysInfoView::Pulse()
{
	mCurrUpdateTime = system_time();
	system_info sysinfo;
	get_system_info(&sysinfo);
	mCurrActiveCPUTime = sysinfo.cpu_infos[0].active_time;
	mCurrUsedPages = sysinfo.used_pages;
	mCurrPageFaults = sysinfo.page_faults;
	mTotalPages = sysinfo.max_pages;
	if (mLastPageFaults == 0)
		mLastPageFaults = mCurrPageFaults;
		
		
		
	if (mCurrActiveCPUTime != mLastActiveCPUTime ||
		mCurrUsedPages     != mLastUsedPages ||
		mCurrPageFaults    != mLastPageFaults)
			DoDraw();
}

// Since this plug-in is built as an executable, include a dummy main()
int main(int argc, void** argv)
{
	return 0;
}