//========================================================================
//	AboutBox.cpp
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include <ByteOrder.h>
#include <Roster.h>
#include <View.h>
#include <stdlib.h>
#include <string.h>

#include "AboutBox.h"
#include "VideoCoder.h"
#include "MAIFFPlayer.h"
#include "IDEMessages.h"

#include <Screen.h>
#include <Bitmap.h>
#include <Alert.h>
#include <Application.h>
#include <Resources.h>

class AboutBox;

const uint32		kAIFFType = 'aiff';

#define ADIERESIS	"\xc3\xa4"
#define EACUTE		"\xc3\xa9"
#define FRAME_RATE 16.0

// BeIDE Version for about box...
#define ABOUTVERSION "BeOS Release 5\n"

const char sAboutMessage[] = 
	"__________________________________________\n"
	"\n"
	"BeIDE" B_UTF8_TRADEMARK " by Metrowerks" B_UTF8_REGISTERED ".\n"
	ABOUTVERSION
	__DATE__ "\n"
	"\n"
	B_UTF8_COPYRIGHT " Copyright 1996-1998 Metrowerks Corporation.\n"
	"All Rights Reserved\n"
	"\n"
	"Portions "
	B_UTF8_COPYRIGHT
	" Be, Incorporated 1998-1999"
	"\n"
	"\n"
	"__________________________________________\n"
	"$Be Engineering\n"
	"\n"
	"John Dance\n"
	"\n"
	"$Engineering Team (1995-1998)\n"
	"\n"
	"Mark Anderson\n"
	"Jason Eckhardt\n"
	"Bernie Estavillo\n"
	"Andreas Hommel\n"
	"Bob Kushlis\n"
	"John McEnerney\n"
	"Burton Miller\n"
	"Fred Peterson\n"
	"Brian Stern\n"
	"Ed Swartz\n"
	"Jon W" ADIERESIS "tte\n"
	"\n"
	"_______________________\n"
	"$Head Honchos\n"
	"\n"
	"Greg Galanos\n"
	"Jean-Louis Gass" EACUTE "e\n"
	"Jean Belanger\n"
	"\n"
	"\n"
	"_______________________\n"
	"$Special thanks to:\n"
	"\n"
	"Erich Ringewald\n"
	"Hiroshi Lockheimer\n"
	"Michael Stricklin\n"
	"Ming Low\n"
	"Cyril Meurillon\n"
	"My Buddy L. Frank Turovich\n"
	"Cthulu\n"
	"\n"
	"\n"
	"_______________________\n"
	"$Cooperative Citizens\n"
	"\n"
	"Melissa Rogers\n"
	"Greg Combs\n"
	"Steve Horowitz\n"
	"Peter Potrebic\n"
	"Dominic Giampaolo\n"
	"Eric Knight\n"
	"Brad Taylor\n"
	"Bob Herold\n"
	"Al Evans\n"
	"Mike Coleman\n"
	"Mike Little\n"
	"Greg Jorgensen\n"
	"She who must be obeyed\n"
	"\n"
	"__________________________________________\n"
	"\n"
	"Styled Text Engine,\n"
	"Copyright " B_UTF8_COPYRIGHT " 1996 by Hiroshi Lockheimer\n"
	"\n"
	"__________________________________________\n"
;
	

class MemArea {
		void *					fBuffer;
		int32					fSize;
		int32					fPos;
public:
								MemArea(
									void *data,
									int32 size);
								~MemArea();

		int32					Read(
									void *buffer,
									int32 size);
		int32					Write(
									const void *buffer,
									int32 size);
};


class AboutView :
	public BView
{
		AboutBox *			fWindow;
public:
								AboutView(
									AboutBox *window,
									BRect area);
		void					Draw(
									BRect area);
};


MemArea::MemArea(
	void *data,
	int32 size)
{
	if (!fBuffer)
		throw B_BAD_VALUE;
	fBuffer = data;
	fSize = size;
	fPos = 0;
}


MemArea::~MemArea()
{
	free(fBuffer);
}


int32
MemArea::Read(
	void *buffer,
	int32 size)
{
	if (fPos >= fSize)
		return B_ERROR;
	if (size > fSize-fPos)
		size = fSize-fPos;
	memcpy(buffer, ((char *)fBuffer)+fPos, size);
	fPos += size;
	return size;
}


int32
MemArea::Write(
	const void */*data*/,
	int32 /*size*/)
{
	throw B_BAD_VALUE;
	return B_ERROR;
}



#define SCROLL_START 25
#define SCROLL_END 217



AboutView::AboutView(
	AboutBox *window,
	BRect area) : BView(area, "", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
	fWindow = window;
}


void
AboutView::Draw(
	BRect /*area*/)
{
	DrawBitmapAsync(fWindow->fBackground, B_ORIGIN);
	if (fWindow->fStage > 2) {
		BRect area(0,SCROLL_START, fWindow->fWidth,SCROLL_END);
		DrawBitmapAsync(fWindow->fBuffer, area, area);
	}
}





AboutBox::AboutBox(
	uint32 type,
	int32 id) :
	BWindow(BRect(0,0,0,0), "About BeIDE", B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	try {

	void *			data;
	size_t 			size;
	app_info 		info;

	be_app->GetAppInfo(&info);

	{
		BFile 			appfile;
		
		if (B_NO_ERROR != appfile.SetTo(&info.ref, B_READ_ONLY))
			throw B_FILE_NOT_FOUND;
		BResources 		file(&appfile);
	
		// Save the stampsound data
		void*		stampsound = file.FindResource(kAIFFType, "stampsound", &size  );
		fSoundPlayer = nil;
	
		if (stampsound)
		{
			fStampsoundData = new char[size];
			fStampsoundDataLength = size;
			memcpy(fStampsoundData, stampsound, size);
			free(stampsound);
		}
		else
		{
			fStampsoundData = nil;
			fStampsoundDataLength = 0;
		}
		
		data = file.FindResource(type, id, &size);
	}

	if (!data) {
		throw B_FILE_NOT_FOUND;
	}

	fArea = new MemArea(data, size);
	fVideo = new VideoCoder<MemArea>(*fArea);

	int xSize, ySize;
	fVideo->GetParams(xSize, ySize);
	ResizeTo(xSize-1, ySize-1);
	fWidth = xSize-1;

	BScreen		screen(this);
	BRect		frame = screen.Frame();
	int 		left = (int) (frame.right-xSize)/2;
	left = left & ~7;
	int 		top = (int) (frame.bottom-ySize)/2;
	MoveTo(left, top);

	BRect area(0,0, xSize-1, ySize-1);
	fBackground = new BBitmap(area, B_COLOR_8_BIT, FALSE);
	fBuffer = new BBitmap(area, B_COLOR_8_BIT, TRUE);
	fBufferView = new BView(area, "", B_FOLLOW_NONE, 0);
	fBuffer->AddChild(fBufferView);
	fText = new BBitmap(area, B_COLOR_8_BIT, TRUE);
	fTextView = new BView(area, "", B_FOLLOW_NONE, 0);
	fText->AddChild(fTextView);

	fStage = 0;
	fMessage = sAboutMessage;
	fScroll = 0.0;

	fWindowView = new AboutView(this, area);
	AddChild(fWindowView);
	Lock();
	fWindowView->SetDrawingMode(B_OP_COPY);
	Unlock();

	fBuffer->Lock();
	fBufferView->SetDrawingMode(B_OP_COPY);
	fBuffer->Unlock();

	fText->Lock();

	fTextView->SetFont(be_plain_font);
	fTextView->SetFontSize(10.0);

	rgb_color textColor = { 240, 240, 240, 255 };
	rgb_color blackColor = { 0, 0, 0, 255 };
	fTextView->SetHighColor(textColor);
	fTextView->SetLowColor(blackColor);
	fTextView->SetDrawingMode(B_OP_COPY);
	fTextView->GetFontHeight(&fFontInfo);
	fText->Unlock();

	fRunning = TRUE;
	fQuitted = create_sem(0, "About Quit Sem");
	resume_thread(spawn_thread(ThreadFunc, "About Thread",
		B_DISPLAY_PRIORITY, this));

	} catch(...) {
		PostMessage(B_QUIT_REQUESTED);		// Suicide is painless
	}
}


AboutBox::~AboutBox()
{
	fRunning = false;
	delete_sem(fQuitted);
	delete fVideo;
	delete fArea;
	delete fBackground;
	delete fBuffer;
	delete fText;

	delete [] fStampsoundData;
	delete fSoundPlayer;
}


bool
AboutBox::QuitRequested()
{
	fRunning = false;		//	signal thread we're quitting
	Unlock();				//	let thread run to completion
	acquire_sem(fQuitted);
	be_app_messenger.SendMessage(BYE_BYE_BOX);
	Lock();
	return true;
}


int32
AboutBox::ThreadFunc(
	void *data)
{
	try {
		((AboutBox *)data)->DoRun();
	} catch (...) {
		BAlert *alert = new BAlert("", "An exception was thrown in AboutBox::DoRun()", "OK");
		alert->Go();
	}
	return 0;
}


void
AboutBox::DoRun()
{
	while (fRunning) {
		switch (fStage) {
		case 0:			//	animation
			AnimateFrame();
			break;
		case 1:			//	transition effect
			Transition();
			break;
		case 2:			//	scrolling text
			Scroll();
			break;
		case 3:
			fScroll = 0.0;
			fStage = 2;
			break;
		}
	}
	release_sem(fQuitted);
}


void
AboutBox::AnimateFrame()
{
	bigtime_t then = (bigtime_t) (system_time() + 1000000/FRAME_RATE);	//	frame rate
	if (B_NO_ERROR != fVideo->GetFrame((uchar *)fBackground->Bits(),
			fBackground->BytesPerRow())) {
		fStage++;
		return;
	}

	if (!Lock())
		return;

	fWindowView->Sync();
	fWindowView->DrawBitmapAsync(fBackground, B_ORIGIN);
	fWindowView->Flush();

	Unlock();

	bigtime_t now = then-system_time();
	if (now > 0)
		snooze(now);
}


void
AboutBox::Transition()
{
	PlayStampsound();	//	play a sound
	fStage++;
}

void
AboutBox::PlayStampsound()
{
	if (fStampsoundData != nil)
	{
		fSoundPlayer = new MAIFFPlayer(fStampsoundData, fStampsoundDataLength);		
		fSoundPlayer->Play();
	}
}


void
AboutBox::Scroll()
{
	bigtime_t then = system_time()+(1000000/50); // frame rate

	fBuffer->Lock();
	fText->Lock();

	BRect area(0,SCROLL_START, fWidth,SCROLL_END);
	fBufferView->SetDrawingMode(B_OP_COPY);
	fBufferView->DrawBitmapAsync(fBackground, area, area);
	fTextView->FillRect(area, B_SOLID_LOW);
	fTextView->SetDrawingMode(B_OP_OVER);	// for anti-aliasing
	fBufferView->Sync();

	float base = area.bottom+fFontInfo.ascent+fFontInfo.leading-fScroll;
	float feed = fFontInfo.descent+fFontInfo.descent+fFontInfo.leading;
	if (feed < 11.0) feed = 11.0;
	const char *ptr = fMessage;
	const char *end;
	int rPos = (int) Bounds().right;
	float	halfLineHeight = (fFontInfo.ascent+fFontInfo.descent+fFontInfo.leading)/2.0;
	const rgb_color		headingColor = { 255, 203, 51, 255 };

	while (ptr && *ptr) {

		end = strchr(ptr, '\n');
		if (!end) end = ptr+strlen(ptr);

		if (base > area.top-feed) {
			float x = (fWidth-fTextView->StringWidth(ptr, end-ptr))/2.0;
			
			switch (*ptr)
			{
				case '_':
					fTextView->StrokeLine(BPoint(0, base-halfLineHeight), BPoint(rPos,base-halfLineHeight));				
					break;

				case '$':
					rgb_color	highColor = fTextView->HighColor();
					fTextView->SetHighColor(headingColor);
					ptr++;
					fTextView->DrawString(ptr, end-ptr, BPoint(x,base));
					fTextView->SetHighColor(highColor);
					break;
				
				default:
					fTextView->DrawString(ptr, end-ptr, BPoint(x,base));
					break;
			}
		}

		base += feed;
		if (base > area.bottom+feed)
			break;
		if (!*end)
			break;
		ptr = end+1;
	}
	fTextView->SetDrawingMode(B_OP_MIN);
	{
		for (int ix=0; ix<5; ix++) {
			rgb_color col;
			col.red = col.green = col.blue = 40*ix+50;
			col.alpha = 255;
			fTextView->SetHighColor(col);
			fTextView->StrokeLine(BPoint(0,SCROLL_START+ix), BPoint(fWidth,SCROLL_START+ix));
			fTextView->StrokeLine(BPoint(0,SCROLL_END-ix), BPoint(fWidth,SCROLL_END-ix));
		}
		rgb_color whiteColor = { 240, 240, 240, 255 };
		fTextView->SetHighColor(whiteColor);
	}
	fTextView->SetDrawingMode(B_OP_COPY);
	fTextView->Sync();

	fBufferView->SetDrawingMode(B_OP_MAX);
	fBufferView->DrawBitmap(fText, area, area);
	fBufferView->Sync();

	fText->Unlock();
	fBuffer->Unlock();

	if (!Lock())
		return;
	fWindowView->DrawBitmapAsync(fBuffer, area, area);
	fWindowView->Sync();
	Unlock();

	fScroll++;
	if (base < area.top-feed)
		fStage++;

	bigtime_t now = then-system_time();
	if (now > 0)
		snooze(now);
}
