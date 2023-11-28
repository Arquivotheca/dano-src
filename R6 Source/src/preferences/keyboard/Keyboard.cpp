//******************************************************************
//	
//	Keyboard.cpp
//
//  Written by: Dominic Giampaolo
//	Based on Mouse.cpp written by: Robert Polic
//	Rehacked by Robert
//	
//
//	Copyright 1996 Be, Inc. All Rights Reserved.
//	
//******************************************************************

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <interface_misc.h>

#include <Alert.h>
#include <Button.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Screen.h>

#include "Keyboard.h"

const int32 msg_key_repeat_change = 'krpt';
const int32 msg_repeat_delay_change = 'kdly';
const int32 msg_defaults = 'dflt';
const int32 msg_revert = 'rvrt';

//******************************************************************

const int32 kKeyRepeatIconWidth = 17;
const int32 kKeyRepeatIconHeight = 18;

const unsigned char kKeyRepeatIconBits [] = {
	0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x1b,0x1c,0x1b,0x00,0x00,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x1b,0x15,0x1c,
	0x3f,0x1b,0x1c,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,
	0x1b,0x15,0x1c,0x1c,0x0f,0x1b,0x1c,0x1b,0x1c,0x00,0x00,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0x00,0x1b,0x15,0x1c,0x1c,0x00,0x15,0x1c,0x1b,0x1c,0x1b,0x1c,0x1c,0x00,
	0xff,0xff,0xff,0xff,0xff,0x00,0x1b,0x15,0x1c,0x1c,0x1b,0x00,0x00,0x00,0x1c,0x1c,
	0x1b,0x1c,0x3f,0x0f,0x00,0xff,0xff,0xff,0xff,0x00,0x3f,0x3f,0x15,0x15,0x1c,0x1b,
	0x1c,0x15,0x00,0x1c,0x1c,0x3f,0x0f,0x0f,0x00,0xff,0xff,0xff,0xff,0x00,0x3f,0x15,
	0x3f,0x3f,0x15,0x15,0x1c,0x00,0x1b,0x1c,0x3f,0x0f,0x0f,0x0f,0x00,0xff,0xff,0xff,
	0x00,0x3f,0x15,0x15,0x15,0x15,0x3f,0x3f,0x15,0x15,0x1c,0x3f,0x0f,0x0f,0x0f,0x0f,
	0x00,0xff,0xff,0xff,0x00,0x3f,0x15,0x15,0x15,0x15,0x15,0x15,0x3f,0x3f,0x3f,0x0f,
	0x0f,0x0f,0x0f,0x0f,0x00,0xff,0xff,0xff,0x00,0x3f,0x15,0x15,0x15,0x15,0x15,0x15,
	0x15,0x15,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x00,0xff,0xff,0xff,0x00,0x3f,0x15,0x15,
	0x15,0x15,0x15,0x15,0x15,0x15,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x00,0xff,0xff,0xff,
	0x00,0x3f,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x0f,0x0f,0x0f,0x0f,0x0f,0x00,
	0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x0f,0x0f,
	0x0f,0x0f,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x15,0x15,0x15,
	0x15,0x15,0x0f,0x0f,0x0f,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0x00,0x00,0x15,0x15,0x15,0x0f,0x0f,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x15,0x0f,0x00,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff
};

const int32 kRepeatDelayIconWidth = 15;
const int32 kRepeatDelayIconHeight = 15;

const unsigned char kRepeatDelayIconBits [] = {
	0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0x00,0x00,0x3f,0x3f,0x3f,0x3f,0x3f,0x00,0x00,0xff,0xff,0xff,0xff,
	0xff,0xff,0x00,0x3f,0x3f,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x00,0xff,0xff,0xff,
	0xff,0x00,0x3f,0x17,0x17,0x10,0x10,0x10,0x10,0x10,0x17,0x17,0x17,0x00,0xff,0xff,
	0xff,0x00,0x3f,0x17,0x10,0x3f,0x3f,0x3f,0x3f,0x3f,0x10,0x17,0x17,0x00,0xff,0xff,
	0x00,0x3f,0x17,0x10,0x3f,0x3f,0x3f,0x00,0x3f,0x3f,0x3f,0x10,0x17,0x10,0x00,0xff,
	0x00,0x3f,0x17,0x10,0x3f,0x3f,0x3f,0x00,0x3f,0x3f,0x3f,0x10,0x17,0x10,0x00,0xff,
	0x00,0x3f,0x17,0x10,0x3f,0x3f,0x3f,0x00,0x00,0x3f,0x3f,0x10,0x17,0x10,0x00,0xff,
	0x00,0x3f,0x17,0x10,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x10,0x17,0x10,0x00,0xff,
	0x00,0x3f,0x17,0x10,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x10,0x17,0x10,0x00,0xff,
	0xff,0x00,0x17,0x17,0x10,0x3f,0x3f,0x3f,0x3f,0x3f,0x10,0x17,0x10,0x00,0xff,0xff,
	0xff,0x00,0x17,0x17,0x17,0x10,0x10,0x10,0x10,0x10,0x17,0x17,0x10,0x00,0xff,0xff,
	0xff,0xff,0x00,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x10,0x10,0x00,0xff,0xff,0xff,
	0xff,0xff,0xff,0x00,0x00,0x10,0x10,0x10,0x10,0x10,0x00,0x00,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff
};

//******************************************************************

static float
FontHeight(BView* target, bool full)
{
	font_height finfo;		
	target->GetFontHeight(&finfo);
	float h = finfo.ascent + finfo.descent;

	if (full)
		h += finfo.leading;
	
	return h;
}

//******************************************************************

int main()
{	
	TKeyboardPrefApp app;

	app.Run();

	return B_NO_ERROR;
}

//******************************************************************

const int32 kWindowWidth = 229;
const int32 kWindowHeight = 221;

TKeyboardPrefApp::TKeyboardPrefApp()
		  :BApplication("application/x-vnd.Be-KYBD")
{
	fWind = new TKeyboardWindow(BRect(0,0,kWindowWidth,kWindowHeight));
	fWind->Show();
}

void
TKeyboardPrefApp::MessageReceived(BMessage* m)
{
	switch(m->what) {
		case B_ABOUT_REQUESTED:
			AboutRequested();
			break;
		default:
			BApplication::MessageReceived(m);
			break;
	}
}

void
TKeyboardPrefApp::AboutRequested()
{
	(new BAlert("", "Configure your keyboard here.", "OK"))->Go();
}

//******************************************************************

TKeyboardWindow::TKeyboardWindow(BRect rect)
	:BWindow(rect, "Keyboard",
	B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	GetPrefs();
	BuildParts();
	CanRevert(false);
	
	AddShortcut('I', B_COMMAND_KEY, new BMessage(B_ABOUT_REQUESTED));
	AddShortcut('R', B_COMMAND_KEY, new BMessage(msg_revert));
	AddShortcut('D', B_COMMAND_KEY, new BMessage(msg_defaults));
}

void
TKeyboardWindow::MessageReceived(BMessage* theMessage)
{
	int32 val=0;
	
	switch(theMessage->what) {
		case B_ABOUT_REQUESTED:
			be_app->MessageReceived(theMessage);
			break;
			
		case msg_key_repeat_change:
			val = fKeyRepeatSlider->Value();
			set_key_repeat_rate(val);
			//
			//	the settings for this are kind of odd,
			//	try to set, get it and reset it
			//	* to make this a 'sticky' slider, uncomment the following
			//
			//get_key_repeat_rate(&val2);
			//set_key_repeat_rate(val);
			//fKeyRepeatSlider->SetValue(val);
			fCurrentSettings.key_repeat_rate = val;

			CanRevert(true);
			break;
			
		case msg_repeat_delay_change:
		{
			long long lval = fDelaySlider->Value();
			int32 temp = lval / 250000;
			if (lval > ((temp * 250000) + 125000))  lval = (temp+1) * 250000;
			else lval = temp * 250000;
			set_key_repeat_delay(lval);
			get_key_repeat_delay(&lval);
			fDelaySlider->SetValue(lval);
			fCurrentSettings.key_repeat_delay = lval;

			CanRevert(true);
			break;
		}
			
		case msg_revert:
			Revert();
			CanRevert(false);
			break;

		case msg_defaults:
			SetDefaults();
			CanRevert(true);
			break;
			
		default:
			BWindow::MessageReceived(theMessage);
			break;
	}
}

bool
TKeyboardWindow::QuitRequested()
{
	SetPrefs();
	be_app->PostMessage(B_QUIT_REQUESTED);

	return true;
}

const int32 kSliderWidth = 174;
void
TKeyboardWindow::BuildParts()
{
	BRect r(Bounds());
	//
	//	add a view to contain all children of the window
	//
	r.InsetBy(-1, -1);
	fBG = new BBox(r);
	AddChild(fBG);
	
	r.Set(12, 13, fBG->Bounds().Width()-12, fBG->Bounds().Height()-45);
	fBox = new TBox(r);
	fBG->AddChild(fBox);
	//
	//	add the key repeat slider
	//
	get_key_repeat_rate(&(fCurrentSettings.key_repeat_rate));
	fOriginalSettings.key_repeat_rate = fCurrentSettings.key_repeat_rate;
	r.Set(10,7,kSliderWidth-1,30);
	fKeyRepeatSlider = new BSlider(r, "key repeat slider", "Key repeat rate",
		new BMessage(msg_key_repeat_change), 20, 300);
	fKeyRepeatSlider->SetLimitLabels("Slow","Fast");
	fKeyRepeatSlider->SetKeyIncrementValue(10);
	fKeyRepeatSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fKeyRepeatSlider->SetHashMarkCount(5);
	fKeyRepeatSlider->SetValue(fCurrentSettings.key_repeat_rate);
	fBox->AddChild(fKeyRepeatSlider);

	int32 x = (int32) (fKeyRepeatSlider->Frame().right + 7);
	int32 y = (int32) (fKeyRepeatSlider->Frame().top  + 7 + 
		(kKeyRepeatIconHeight/2));
	BRect iconRect(x, y, x+kKeyRepeatIconWidth-1, y+kKeyRepeatIconHeight-1);
	TIconThing* delayIcon = new TIconThing(iconRect, kKeyRepeatIconBits);
	fBox->AddChild(delayIcon);
	//
	//	add the delay slider
	//
	get_key_repeat_delay(&(fCurrentSettings.key_repeat_delay));
	fOriginalSettings.key_repeat_delay = fCurrentSettings.key_repeat_delay;
	r.top = r.bottom + 37;
	r.bottom = r.top + 24;
	fDelaySlider = new BSlider(r, "delay slider", "Delay until key repeat",
		new BMessage(msg_repeat_delay_change), 250000, 1000000);

	fDelaySlider->SetLimitLabels("Short","Long");
	fDelaySlider->SetKeyIncrementValue(250000);
	fDelaySlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fDelaySlider->SetHashMarkCount(4);
	fDelaySlider->SetValue(fCurrentSettings.key_repeat_delay);
	fBox->AddChild(fDelaySlider);
	
	x = (int32)(fDelaySlider->Frame().right + 8);
	y = (int32)(fDelaySlider->Frame().top  + 11 + (kRepeatDelayIconHeight/2));
	iconRect.Set(x, y, x+kRepeatDelayIconWidth-1, y+kRepeatDelayIconHeight-1);
	TIconThing* repeatIcon = new TIconThing(iconRect, kRepeatDelayIconBits);
	fBox->AddChild(repeatIcon);

	float b = fDelaySlider->Frame().bottom;
	fBox->SetLineLocation(BPoint(11, b+6),
		BPoint(fBox->Bounds().Width()-11, b+6));
		
	//
	//	add the sample text field
	//
	r.left = 8; r.right = fBox->Bounds().Width()-11;
	r.top = b + 18;
	r.bottom = r.top + 20;
	fTextControl = new BTextControl(r, "", NULL, "", NULL);
	fTextControl->SetFont(be_bold_font);
	fTextControl->SetFontSize(14);
	fTextControl->SetText("Typing test area");
	fTextControl->SetAlignment(B_ALIGN_CENTER, B_ALIGN_CENTER);
	fBox->AddChild(fTextControl);

	r.top = fBox->Frame().bottom + 1;
	r.bottom = Bounds().Height()+1;
	r.left = 1; r.right = Bounds().Width()+1;
	fBtnBar = new TButtonBar(r, true, true, BB_BORDER_NONE);
	fBG->AddChild(fBtnBar);
}


void
TKeyboardWindow::GetPrefs()
{
	BPath path;
	
	if (find_directory (B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		long ref;
		BPoint loc;
				
		path.Append (kb_settings_file);

		if ((ref = open(path.Path(), O_RDWR)) >= 0) {
			lseek (ref, sizeof (kb_settings), SEEK_SET);
			read(ref, &loc, sizeof(BPoint));
			close(ref);
			
			if (BScreen(B_MAIN_SCREEN_ID).Frame().Contains(loc)) {
				MoveTo(loc);
				return;
			}
		}
	}
	
	BRect	screenFrame = (BScreen(B_MAIN_SCREEN_ID).Frame());
	BPoint 	pt;
	pt.x = screenFrame.Width()/2 - Bounds().Width()/2;
	pt.y = screenFrame.Height()/2 - Bounds().Height()/2;

	if (screenFrame.Contains(pt))
		MoveTo(pt);
}

void
TKeyboardWindow::SetPrefs()
{
	BPath path;
	BPoint loc = Frame().LeftTop();

	if (find_directory (B_USER_SETTINGS_DIRECTORY, &path, true) == B_OK) {
		long ref;
		path.Append (kb_settings_file);
		if ((ref = creat(path.Path(), O_RDWR)) >= 0) {
			write (ref, &fCurrentSettings, sizeof (kb_settings));
			write (ref, &loc, sizeof (BPoint));
			close(ref);
		}
	}
}

void
TKeyboardWindow::SetDefaults()
{
	fKeyRepeatSlider->SetValue(200);
	fDelaySlider->SetValue(250000);
	
	set_key_repeat_rate(200);
	fCurrentSettings.key_repeat_rate = 200;

	set_key_repeat_delay(250000);
	fCurrentSettings.key_repeat_delay = 250000;
}

void
TKeyboardWindow::Revert()
{
	fKeyRepeatSlider->SetValue(fOriginalSettings.key_repeat_rate);
	fDelaySlider->SetValue(fOriginalSettings.key_repeat_delay);

	set_key_repeat_rate(fOriginalSettings.key_repeat_rate);
	set_key_repeat_delay(fOriginalSettings.key_repeat_delay);

	fCurrentSettings.key_repeat_rate = fOriginalSettings.key_repeat_rate;
	fCurrentSettings.key_repeat_delay = fOriginalSettings.key_repeat_delay;
}

void
TKeyboardWindow::CanRevert(bool state)
{
	fBtnBar->CanRevert(state);
}

//*********************************************************************

const rgb_color kWhite = { 255, 255, 255, 255};

TBox::TBox(BRect frame)
	: BBox(frame)
{
	fStart.x=fStart.y=fEnd.x=fEnd.y = -1;
}

void
TBox::Draw(BRect r)
{
	BBox::Draw(r);
	
	PushState();
	SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_3_TINT));
	StrokeLine(fStart, fEnd);
	SetHighColor(kWhite);
	StrokeLine(BPoint(fStart.x, fStart.y+1), BPoint(fEnd.x, fEnd.y+1));
	PopState();
}

void
TBox::SetLineLocation(BPoint start, BPoint end)
{
	fStart = start;
	fEnd = end;
}

//*********************************************************************

const int32 kButtonXLoc = 10;
const int32 kButtonWidth = 75;							// same as B_WIDTH_AS_USUAL

TButtonBar::TButtonBar(BRect frame, bool defaultsBtn, bool revertBtn,
	bb_border_type borderType)
	: BView(frame, "button bar", B_FOLLOW_LEFT|B_FOLLOW_TOP, B_WILL_DRAW),
	fBorderType(borderType),
	fHasDefaultsBtn(defaultsBtn), fHasRevertBtn(revertBtn),
	fHasOtherBtn(false)
{
	BRect r;
	
	r.bottom = Bounds().Height()-12;
	r.top = r.bottom - (FontHeight(this, true) + 10);
	r.left = kButtonXLoc;
	if (fHasDefaultsBtn) {
		r.right = r.left + kButtonWidth;
		fDefaultsBtn = new BButton(r, "defaults", "Defaults",
			new BMessage(msg_defaults));
		AddChild(fDefaultsBtn);
	} else
		fDefaultsBtn=NULL;
	
	if (fHasRevertBtn) {
		if (fHasDefaultsBtn)
			r.left = fDefaultsBtn->Frame().right + 7;
		else
			r.left = kButtonXLoc;
		r.right = r.left + kButtonWidth;
		fRevertBtn = new BButton(r, "revert", "Revert",
			new BMessage(msg_revert));
		AddChild(fRevertBtn);
	} else
		fRevertBtn=NULL;
		
	fOtherBtn=NULL;
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


TButtonBar::~TButtonBar()
{
}

void 
TButtonBar::Draw(BRect)
{
	PushState();	
	if (fHasOtherBtn) {
		BPoint top, bottom;
		
		top.x = fOtherBtn->Frame().right + 10;
		top.y = fOtherBtn->Frame().top;
		bottom.x = top.x;
		bottom.y = fOtherBtn->Frame().bottom;
		
		SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_3_TINT));
		SetLowColor(ViewColor());
		StrokeLine(top, bottom);
		SetHighColor(kWhite);
		top.x++;
		bottom.x++;
		StrokeLine(top, bottom);
	}
	PopState();
}

void 
TButtonBar::AddButton(const char* title, BMessage* m)
{
	BRect r;

	r.bottom = Bounds().Height() - 10;
	r.top = r.bottom - (FontHeight(this, true) + 10);
	r.left = kButtonXLoc;
	r.right = r.left + StringWidth(title) + 20;
	fOtherBtn = new BButton(r, "other", title, m);
	
	int32 w = (int32)(22+fOtherBtn->Bounds().Width());
	if (fHasDefaultsBtn) {
		RemoveChild(fDefaultsBtn);
		if (fHasRevertBtn)
			RemoveChild(fRevertBtn);
		AddChild(fOtherBtn) /*, fDefaultsBtn)*/;
		AddChild(fDefaultsBtn);
		fDefaultsBtn->MoveBy(w, 0);
		if (fHasRevertBtn) {
			AddChild(fRevertBtn);
			fRevertBtn->MoveBy(w,0);
		}
	} else if (fHasRevertBtn) {
		RemoveChild(fRevertBtn);
		AddChild(fOtherBtn) /*, fRevertBtn)*/;
		AddChild(fRevertBtn);
		fRevertBtn->MoveBy(w,0);
	} else
		AddChild(fOtherBtn);
		
	fHasOtherBtn = (fOtherBtn != NULL);
}

void
TButtonBar::CanRevert(bool state)
{
	fRevertBtn->SetEnabled(state);
}

//*********************************************************************

TIconThing::TIconThing(BRect frame, const uchar* bits)
	: BView(frame, "icon thing", B_FOLLOW_NONE, B_WILL_DRAW)
{
	fBits = new BBitmap(Bounds(), B_COLOR_8_BIT);
	fBits->SetBits((char*)bits, fBits->BitsLength(), 0, B_COLOR_8_BIT);
}

TIconThing::~TIconThing()
{
	delete fBits;
}

void
TIconThing::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());
	else
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

void
TIconThing::Draw(BRect)
{
	SetDrawingMode(B_OP_OVER);
	DrawBitmap(fBits, Bounds());
}
