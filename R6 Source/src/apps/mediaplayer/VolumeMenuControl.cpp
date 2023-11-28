#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <AutoLock.h>
#include <Beep.h>
#include <PopUpMenu.h>
#include <Roster.h>

#include <MediaRoster.h>
#include <ParameterWeb.h>

#include "MediaController.h"
#include "VolumeMenuControl.h"

// ********************************************************************* //

const float kLeftBarOffset = 6;
const float kRightBarOffset = 7;

const int32 kMaxHeight = 15;
const int32 kVolumeThumbWidth = 11;
const int32 kVolumeThumbHeight = 11;

const unsigned char kVolumeThumb[] = {
	0xff,0xff,0x13,0x18,0x1d,0x3f,0x1d,0x19,0x12,0xff,0xff,0xff,
	0xff,0x15,0x1d,0x1e,0x1e,0x1d,0x1e,0x1b,0x19,0x12,0xff,0xff,
	0xff,0x1e,0x1e,0x1d,0x1d,0x3f,0x1d,0x1d,0x1b,0x13,0xff,0xff,
	0x1a,0x1e,0x1d,0x1d,0x1d,0x13,0x1d,0x1d,0x1d,0x19,0x12,0xff,
	0x1e,0x1e,0x1d,0x1d,0x1d,0x3f,0x1d,0x1d,0x1d,0x19,0x11,0xff,
	0x3f,0x1d,0x1d,0x1d,0x1d,0x13,0x1d,0x1d,0x1d,0x1c,0x0e,0xff,
	0x1e,0x1e,0x1d,0x1d,0x1d,0x3f,0x1d,0x1d,0x1d,0x19,0x11,0xff,
	0x1a,0x1d,0x1d,0x1d,0x1d,0x13,0x1d,0x1d,0x1d,0x15,0xff,0xff,
	0xff,0x1a,0x19,0x1d,0x1d,0x1d,0x1d,0x1d,0x17,0x12,0xff,0xff,
	0xff,0x16,0x13,0x15,0x19,0x1c,0x19,0x15,0x12,0xff,0xff,0xff,
	0xff,0xff,0xff,0x14,0x11,0x0e,0x11,0x14,0xff,0xff,0xff,0xff
};

const int32 kVolumeCapWidth = 6;
const int32 kVolumeCapHeight = 15;

const unsigned char kRightVolumeCap [] = {
	0x19,0x1b,0xff,0xff,0xff,0xff,0xff,0xff,
	0x11,0x18,0x1a,0xff,0xff,0xff,0xff,0xff,
	0x08,0x0d,0x16,0x1a,0xff,0xff,0xff,0xff,
	0x11,0x0c,0x0f,0x17,0x1a,0xff,0xff,0xff,
	0x13,0x13,0x10,0x14,0x19,0xff,0xff,0xff,
	0x13,0x13,0x13,0x15,0x19,0x1a,0xff,0xff,
	0x13,0x13,0x13,0x16,0x1a,0x1a,0xff,0xff,
	0x13,0x13,0x13,0x15,0x1c,0x1b,0xff,0xff,
	0x13,0x13,0x13,0x19,0x1e,0x1c,0xff,0xff,
	0x13,0x13,0x15,0x1d,0x1d,0x1c,0xff,0xff,
	0x13,0x14,0x1a,0x1e,0x1c,0xff,0xff,0xff,
	0x15,0x1b,0x3f,0x1d,0xff,0xff,0xff,0xff,
	0x1d,0x1e,0x1d,0x1c,0xff,0xff,0xff,0xff,
	0x1d,0x1c,0xff,0xff,0xff,0xff,0xff,0xff,
	0x1c,0xff,0xff,0xff,0xff,0xff,0xff,0xff
};

const unsigned char kLeftVolumeCap [] = {
	0xff,0xff,0xff,0xff,0x19,0x19,0xff,0xff,
	0xff,0xff,0x1a,0x18,0x11,0x0b,0xff,0xff,
	0xff,0x19,0x15,0x0b,0x06,0x0a,0xff,0xff,
	0x1a,0x15,0x09,0x09,0xb6,0x8f,0xff,0xff,
	0x18,0x0b,0x09,0x8f,0x8f,0x8f,0xff,0xff,
	0x11,0x07,0xb6,0x8f,0x8f,0x68,0xff,0xff,
	0x0c,0x0c,0x8f,0x8f,0x68,0x68,0xff,0xff,
	0x07,0x10,0x8f,0x68,0x68,0x68,0xff,0xff,
	0x16,0x12,0x8f,0x68,0x68,0x68,0xff,0xff,
	0x1a,0x16,0x11,0x68,0x68,0x68,0xff,0xff,
	0xff,0x1a,0x18,0x68,0x68,0x68,0xff,0xff,
	0xff,0xff,0x1c,0x19,0x68,0x68,0xff,0xff,
	0xff,0xff,0xff,0x1d,0x1c,0x19,0xff,0xff,
	0xff,0xff,0xff,0xff,0x1d,0x1e,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0x1c,0xff,0xff
};

TSliderMenuItem::TSliderMenuItem(int32 min, int32 max,
	BMessage *message, BMessage* modificationMessage, 
	char shortcut, uint32 modifiers)
	: BMenuItem("Volume", message, shortcut, modifiers),
		fValue(0), fMinValue(min), fMaxValue(max),
		fModificationMessage(modificationMessage)
{
	InitObject();			//	init most of the bitmaps

	fFloor = -60.0;			//	default audio range
	fCeiling = 18.0;		//	volume will be percentage of range
}

TSliderMenuItem::~TSliderMenuItem()
{
	StopThread();			//	mouse tracking thread
	KillObject();			//	delete the bitmaps
}


void
TSliderMenuItem::InitObject()
{	
	fLocation.x = fLocation.y = 0;
	fXOffset = 5;		//	just a default value for a reasonable edge buffer
	fYOffset = 0;		//	buffer not needed as a BMenuItem is lame
	fThreadID = -1;
	fOffScreenBits = 0;
	fOffScreenView = 0;

	fLeftCapBits = new BBitmap(BRect(0,0,kVolumeCapWidth-1, kVolumeCapHeight-1),
		B_COLOR_8_BIT, false, false);
	if (fLeftCapBits)
		fLeftCapBits->SetBits(kLeftVolumeCap, fLeftCapBits->BitsLength(), 0,
			B_COLOR_8_BIT);
			
	fRightCapBits = new BBitmap(BRect(0,0,kVolumeCapWidth-1, kVolumeCapHeight-1),
		B_COLOR_8_BIT, false, false);
	if (fRightCapBits)
		fRightCapBits->SetBits(kRightVolumeCap, fRightCapBits->BitsLength(), 0,
			B_COLOR_8_BIT);
			
	fThumbBits= new BBitmap(BRect(0,0,kVolumeThumbWidth-1, kVolumeThumbHeight-1),
		B_COLOR_8_BIT, false, false);
	if (fThumbBits)
		fThumbBits->SetBits(kVolumeThumb, fThumbBits->BitsLength(), 0,
			B_COLOR_8_BIT);
	
	fInitd = false;
	//	don't init the offscreen bitmap here, the size is not
	//	correct Frame() that is

	fFirstTime = true;
	fInitialValue = 0;
	fShowPink = false;
}

void
TSliderMenuItem::KillObject()
{
	delete fOffScreenBits;
	delete fThumbBits;
	delete fLeftCapBits;
	delete fRightCapBits;
}

void
TSliderMenuItem::GetContentSize(float *w, float *h)
{
	//	frame is based on volume range
	//	entire bounds is based on frame plus ends plus gutters
	*w = (abs((int32)fFloor) + abs((int32)fCeiling)) * 2;
	*w += kLeftBarOffset + kRightBarOffset;
	*h = kMaxHeight;
}

//	handles an invoke in the same way that a BControl does
status_t
TSliderMenuItem::Invoke(BMessage *msg)
{
//printf("invoke\n");
	StopThread();		

	if (!msg)
		msg = Message();
	if (!msg)
		return B_BAD_VALUE;

	BMessage clone(*msg);

	clone.AddInt64("when", system_time());
	clone.AddPointer("source", this);
	clone.AddInt32("be:value", fValue);
	clone.AddFloat("be:position", Position());
	status_t err = BInvoker::Invoke(&clone);

	return err;
}

status_t
TSliderMenuItem::SendModificationMessage()
{
	if (!fModificationMessage)
		return B_OK;

	BMessage clone(*fModificationMessage);

	clone.AddInt64("when", system_time());
	clone.AddPointer("source", this);
	clone.AddInt32("be:value",fValue);
	clone.AddFloat("be:position", Position());
	
	return Messenger().SendMessage(&clone, HandlerForReply(), Timeout());
}

void
TSliderMenuItem::Draw()
{
	if (!fInitd) {
		//	create the offscreen bitmap here so that the size
		//	matches the display size, yes, this is lame
		//	frame != contentsize
		fInitd = true;
		BRect offscreenRect(Frame());
		offscreenRect.OffsetTo(0,0);	
		fOffScreenView = new BView(offscreenRect, "", B_FOLLOW_ALL, B_WILL_DRAW);
		fOffScreenBits = new BBitmap(offscreenRect, B_COLOR_8_BIT, true);
		
		if (fOffScreenBits && fOffScreenView)
			fOffScreenBits->AddChild(fOffScreenView);
			
		fYOffset = (Frame().Height() - kMaxHeight) / 2;
		InitBarFrames();		
	}
	if (fFirstTime) {
		//	this will get set each time the parent menu is selected
		//	by calling SetInitialValue
		fFirstTime = false;				
		SetValue(fInitialValue);		//	this, by default, sets the thumb position
										//	to whatever fInitialValue is								
	}
	
	if (!fOffScreenBits)
		return;

	AutoLock<BBitmap> lock(fOffScreenBits);
	if (!lock)
		return;

	BMenu* parent = Menu();		

	fOffScreenView->SetDrawingMode(B_OP_OVER);
	fOffScreenView->SetHighColor(parent->ViewColor());
	fOffScreenView->FillRect(fOffScreenView->Bounds());
	
	DrawBar(fOffScreenView);
	DrawHashMarks(fOffScreenView);
	//DrawText(fOffScreenView);
	DrawThumb(fOffScreenView);
	
	//	don't call inherited Draw unless you want DrawContent
	//	to also be called, remember this for Deskbar D&D
	//BMenuItem::Draw();
	
	fOffScreenView->Sync();

	parent->DrawBitmap(fOffScreenBits, Frame().LeftTop());
}

void
TSliderMenuItem::DrawContent()
{
	BMenuItem::DrawContent();
}

#include "DrawingTidbits.h"		// for color conversion routines

const rgb_color kDarkGreen = {102, 152, 102, 255};
const rgb_color kFillGreen = {171, 221, 161, 255};

const rgb_color kWhiteColor = {255,255,255,255};
const rgb_color kLtLtGrayColor = {224,224,224,255};
const rgb_color kLtGrayColor = {184,184,184,255};
const rgb_color kMedGrayColor = {128,128,128,255};
const rgb_color kBlackColor = {0,0,0,255};

const rgb_color kUnusedColor = {153,153,153,255};

void 
TSliderMenuItem::DrawBar(BView* parent)
{
	BView* target = parent;
	
	rgb_color white = (IsEnabled()) ? kWhiteColor : ShiftColor(kWhiteColor, 0.5);
	rgb_color ltgray = (IsEnabled()) ? kLtGrayColor : ShiftColor(kLtGrayColor, 0.5);
	rgb_color ltltgray = (IsEnabled()) ? kLtLtGrayColor : ShiftColor(kLtLtGrayColor, 0.5);
	rgb_color black = (IsEnabled()) ? kBlackColor : ShiftColor(kBlackColor, 0.5);
	rgb_color dkgreen = (IsEnabled()) ? kDarkGreen : ShiftColor(kDarkGreen, 0.5);
	rgb_color fillgreen = (IsEnabled()) ? kFillGreen : ShiftColor(kFillGreen, 0.5);
	rgb_color unusedgrey = (IsEnabled()) ? kUnusedColor : ShiftColor(kUnusedColor, 0.5);
	
	if (fLocation.x < fBarFrame.left)
		fLocation.x = fBarFrame.left;
	else if (fLocation.x > fBarFrame.right)
		fLocation.x = fBarFrame.right;
		
	fRightBarFrame.left = fLeftBarFrame.right = fLocation.x;

	bool showPink = false;
	if (fShowPink && fLeftBarFrame.right >= fMidBarFrame.left) {
		fMidBarFrame.right = fLeftBarFrame.right;
		fLeftBarFrame.right = fMidBarFrame.left-1;

		target->SetHighColor(255, 128, 128);
		target->FillRect(fMidBarFrame, B_SOLID_HIGH);
		
		showPink = true;
	}
	
	target->SetHighColor(fillgreen);
	target->FillRect(fLeftBarFrame);
	
	target->SetHighColor(unusedgrey);
	target->FillRect(fRightBarFrame);
	
	target->BeginLineArray(showPink ? 8 : 6);
	
	//	shading for the green region
	target->AddLine(BPoint(fLeftBarFrame.left, fLeftBarFrame.top),
		BPoint(fLeftBarFrame.right, fLeftBarFrame.top), dkgreen);	
	target->AddLine(BPoint(fLeftBarFrame.left, fLeftBarFrame.top+1),
		BPoint(fLeftBarFrame.right, fLeftBarFrame.top+1), dkgreen);	

	if (showPink) {
		//	add some shading to the pink region
		rgb_color pink;
		pink.red = 150; pink.green = 102; pink.blue = 102;
		target->AddLine(BPoint(fMidBarFrame.left, fMidBarFrame.top),
			BPoint(fMidBarFrame.right, fMidBarFrame.top), pink);	
		target->AddLine(BPoint(fMidBarFrame.left, fMidBarFrame.top+1),
			BPoint(fMidBarFrame.right, fMidBarFrame.top+1), pink);	
	}

	target->AddLine(BPoint(fBarFrame.left, fBarFrame.top),
		BPoint(fBarFrame.right, fBarFrame.top),ltgray);
	target->AddLine(BPoint(fBarFrame.left, fBarFrame.top+1),
		BPoint(fBarFrame.right, fBarFrame.top+1),black);
		
	target->AddLine(BPoint(fBarFrame.left, fBarFrame.bottom-1),
		BPoint(fBarFrame.right, fBarFrame.bottom-1),white);
	target->AddLine(BPoint(fBarFrame.left, fBarFrame.bottom),
		BPoint(fBarFrame.right, fBarFrame.bottom),ltltgray);

	target->EndLineArray();

	target->SetDrawingMode(B_OP_OVER);
	target->DrawBitmap(fLeftCapBits, BPoint(fLeftBarFrame.left - 6,fYOffset));
	target->DrawBitmap(fRightCapBits, BPoint(fRightBarFrame.right,fYOffset));
}

void
TSliderMenuItem::DrawHashMarks(BView* target)
{
	BRect frame(fBarFrame);
	
	if (frame.Width() <= 6)
		return;

	int32 count = (int32)(frame.Width() / 4);
	BPoint pt1, pt2;
	
	pt1.x = pt2.x = frame.left + 4;
	pt1.y = (frame.Height() / 2) + 1;
	pt2.y = pt1.y + 1;
	
	rgb_color hashColor;

	target->BeginLineArray(count * 2);
	while(true) {
		if (pt1.x <= fLocation.x) {
			target->SetLowColor(kFillGreen);
			hashColor = kDarkGreen;
		} else {
			target->SetLowColor(kUnusedColor);
			hashColor = kMedGrayColor;
		}
		target->AddLine(pt1, pt2, hashColor);

		pt1.x += 5;
		pt2.x += 5;
		if (pt1.x > frame.right - 4)
			break;
	}
	target->EndLineArray();
}

void
TSliderMenuItem::DrawText(BView* target)
{
	font_height fh;
	be_plain_font->GetHeight(&fh);
	const char * str = "Volume";

	BRect c = fBarFrame;
	float sw = be_plain_font->StringWidth(str);
	BPoint sp(floor((c.Width()-sw)/2)+c.left,
			floor((c.Height()-(fh.leading+fh.descent+fh.ascent))/2)+
			fh.leading+fh.ascent+c.top);

	target->SetLowColor(128, 255, 128);
	target->SetHighColor(64, 128, 64);
	target->DrawString(str, sp);
}

void 
TSliderMenuItem::DrawThumb(BView* target)
{
	if (fLocation.x < fBarFrame.left)
		fLocation.x = fBarFrame.left;
	else if (fLocation.x > fBarFrame.right)
		fLocation.x = fBarFrame.right;
		
	fThumbFrame.Set(fLocation.x - kVolumeThumbWidth/2 - 1, 2,
		fLocation.x + kVolumeThumbWidth/2 - 1, 2 + kVolumeThumbHeight - 1);
	
	fThumbFrame.top += fYOffset; fThumbFrame.bottom += fYOffset;
	//	fixed height, just move it

	target->SetDrawingMode(B_OP_OVER);
	target->DrawBitmap(fThumbBits, fThumbFrame);
}

void
TSliderMenuItem::InitBarFrames()
{
	fBarFrame.Set(kLeftBarOffset, 0, Frame().Width() - kRightBarOffset, kMaxHeight-1);
	fBarFrame.top += fYOffset; fBarFrame.bottom += fYOffset;
	//	fixed height, just move it
	fBarFrame.left += fXOffset; fBarFrame.right -= fXOffset;
	//	variable width, inset it

	fLeftBarFrame = fBarFrame;
	fLeftBarFrame.top += 2;
	fLeftBarFrame.bottom -= 2;
	//	right of leftbarframe is based on location
	
	fMidBarFrame = fLeftBarFrame;
	float floor = abs((int32)fFloor);
	float ceiling = abs((int32)fCeiling);
	float perc = (floor / (floor + ceiling));
	fMidBarFrame.left += perc * fBarFrame.Width();
	fRightBarFrame = fLeftBarFrame;
	//	left of rightbarframe is based on location
}

//	sent when the menuitem has the mouse in it
void
TSliderMenuItem::Highlight(bool on)
{
	if (on) {
		StartThread();
	} else {
		StopThread();
	}
	
	BMenuItem::Highlight(on);
}

//	mouse tracking thread
static int32
Mickey(TSliderMenuItem* w)
{
	return w->MouseWatcher();
}

int32
TSliderMenuItem::StartThread()
{
	if (fThreadID == -1) {
		fLooping = true;
	  	fThreadID = spawn_thread((thread_entry)Mickey, "mickey_thread",
			B_NORMAL_PRIORITY, this);
		resume_thread(fThreadID);
	}
	return fThreadID;
}

void
TSliderMenuItem::StopThread()
{
	fLooping = false;
	if (fThreadID > -1) {
		kill_thread(fThreadID);
		fThreadID = -1;		
	}
}

status_t
TSliderMenuItem::MouseWatcher()
{
	BPoint tempLocation;
	tempLocation.x = -1; tempLocation.y = -1;
	while(fLooping) {
		if (IsSelected()) {
			uint32 buttons;
			BMenu* parent = Menu();
			if (parent) {
				parent->LockLooper();
				parent->GetMouse(&tempLocation, &buttons);
				if (!buttons) {
					//	if the button goes up, exit this thread
					parent->UnlockLooper();
					StopThread();
					break;
				}
				if (fLocation != tempLocation ) {
					//	update if necessary
					SetValue(ValueForPoint(tempLocation));
					Draw();
					SendModificationMessage();
				}
				parent->UnlockLooper();
			}
			snooze(10000);
		} else
			break;
	}
	return B_OK;
}

void
TSliderMenuItem::SetValue(int32 v)
{
	// check the bounds
	if (v < fMinValue) v = fMinValue;
	if (v > fMaxValue) v = fMaxValue;

	// offset to actual min and max
	float min = v - fMinValue;
	float max = fMaxValue - fMinValue;
	float val = (min / max) * (fBarFrame.right - fBarFrame.left);
	val = ceil(val);

	//	set the new location
	fLocation.x = val + fBarFrame.left;
	fLocation.y = 0;

	fValue = v;
}

void
TSliderMenuItem::SetInitialValue(int32 v)
{
	fInitialValue = v;
	fFirstTime = true;
}

int32
TSliderMenuItem::Value() const
{
	return fValue;
}

int32
TSliderMenuItem::ValueForPoint(BPoint p) const
{
	float position = p.x;
	float min = fBarFrame.left;
	float max = fBarFrame.right;
	
	// check the bounds	
	if (position < min) position = min;
	if (position > max) position = max;
	
	//	offset to 0 based	
	position = position - min;
	max = max - min;
	
	//	get a value and compensate
	float v = position/max * (fMaxValue-fMinValue);
	float c = ceil(v);
	float f = floor(v);
	int32 val = (int32)((v < (f + 0.50)) ? f : c);
	val = val + (int32)fMinValue;

	return val;
}

void
TSliderMenuItem::SetPosition(float p)
{
	if (p < 0) p = 0;
	if (p > 1.0) p = 1.0;
	
	int32 v = (int32)((fMaxValue - fMinValue) * p);
	
	SetValue(v);
}

float
TSliderMenuItem::Position() const
{
	float v = Value();
	return v / fMaxValue;
}

// ********************************************************************* //

TVolumeMenu::TVolumeMenu(const char *title, TSliderMenuItem* submenu,
	MediaController* controller)
	: BMenu(title),
		fSubmenu(submenu), fController(controller)
{
	AddItem(submenu);
}

TVolumeMenu::~TVolumeMenu()
{
}

void
TVolumeMenu::AttachedToWindow()
{
	if (fSubmenu) {
		int32 num = (int32)((fSubmenu->MaxValue() - fSubmenu->MinValue()) * GetVolume());
		fSubmenu->SetInitialValue(num);
	}
	
	BMenu::AttachedToWindow();
}

float
TVolumeMenu::GetVolume()
{
	if (fController)
		return fController->Volume();

	return 0.0;
}
