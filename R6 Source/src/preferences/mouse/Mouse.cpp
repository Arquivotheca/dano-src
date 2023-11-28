//*********************************************************************
//	
//	Mouse.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1995 Be, Inc. All Rights Reserved.
//	
//*********************************************************************

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#include <interface_misc.h>

#include <Alert.h>
#include <Button.h>
#include <FindDirectory.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <MessageFilter.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <TextControl.h>

#include <TranslationUtils.h>

#include "Mouse.h"

const int32 kDefaultMouseSpeed = 65536;
const int32 kMinMouseSpeed = 65536>>3;
const int32 kMaxMouseSpeed = 65536<<3;
const int32 kMinSpeedSlider = -3*65536;
const int32 kMaxSpeedSlider = 3*65536;
const int32 kDefaultSpeedSlider = 0;
const int32 kSpeedSliderKeyStep = 32768;
const bool 	kDefaultAccelEnabled = true;
const int32 kDefaultAccelFactor = 65536;
const int32 kMinAccelFactor = 0;
const int32 kMaxAccelFactor = 65536<<4;
const int32 kMinAccelSlider = 0;
const int32 kMaxAccelSlider = 512;
const int32 kDefaultAccelSlider = 256;
const int32 kAccelSliderKeyStep = 64;

const int32 kDefaultDblClickSpeed = 500000;
const int32 kMinDblClickSpeed = 0;
const int32 kMaxDblClickSpeed = 1000000;

const int32 msg_control_changed = 'cntl';

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

//*********************************************************************

int main()
{	
	TMouseApplication app;
	app.Run();
	return B_NO_ERROR;
}

//*********************************************************************

const int32 kWindowWidth = 397;
const int32 kWindowHeight = 231+62;

TMouseSettings 	*gMouse;

TMouseApplication::TMouseApplication()
	:BApplication("application/x-vnd.Be-MOUS")
{
	gMouse = fMouse = new TMouseSettings();

	TMouseWindow *window = new TMouseWindow(BRect(0, 0, kWindowWidth, kWindowHeight));
	window->Show();
}

TMouseApplication::~TMouseApplication()
{
	delete fMouse;
}

void
TMouseApplication::MessageReceived(BMessage* m)
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
TMouseApplication::AboutRequested()
{
	BAlert *myAlert;

	myAlert = new BAlert("", "Configure your mouse here.", "OK");
	myAlert->Go();
}

//*********************************************************************

static filter_result mousefilter
	(BMessage *message, BHandler **/*target*/, BMessageFilter *filter)
{
	uint32 what;
	if (message->what == B_MOUSE_DOWN)
		what = 'down';
	else
		what = 'msup';
		
	filter->Looper()->PostMessage(what);	
		
	return B_DISPATCH_MESSAGE;
}

TMouseWindow::TMouseWindow(BRect rect)
	:BWindow(rect, "Mouse",
	B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
{
	AddCommonFilter(new BMessageFilter(B_MOUSE_DOWN, mousefilter));
	AddCommonFilter(new BMessageFilter(B_MOUSE_UP, mousefilter));
	GetPrefs();
	
	BRect r(Bounds());
	r.InsetBy(-1, -1);
	fBG = new BBox(r);
	AddChild(fBG);
	
	r.Set(12, 12, fBG->Bounds().Width()-12, fBG->Bounds().Height()-45);
	fMouseView = new TMouseView(r);	
	fBG->AddChild(fMouseView);

	r.top = fMouseView->Frame().bottom + 1;
	r.bottom = Bounds().Height()+1;
	r.left = 1; r.right = Bounds().Width()+1;
	fBtnBar = new TButtonBar(r, true, true, BB_BORDER_NONE);
	fBG->AddChild(fBtnBar);
	
	SetPulseRate(100000);
	CanRevert(false);
	
	AddShortcut('I', B_COMMAND_KEY, new BMessage(B_ABOUT_REQUESTED));
	AddShortcut('R', B_COMMAND_KEY, new BMessage(msg_revert));
	AddShortcut('D', B_COMMAND_KEY, new BMessage(msg_defaults));
}

void
TMouseWindow::MessageReceived(BMessage* theMessage)
{
	int32 		val=0;
	mouse_type	types[3] = {MOUSE_1_BUTTON, MOUSE_2_BUTTON, MOUSE_3_BUTTON};
	BSlider*	s=NULL;

	switch(theMessage->what) {
		//	pass all MouseUp and MouseDown messages to the
		//	mouse view for button image updates
		case 'down':
			fMouseView->MouseDown(BPoint(0,0));
			break;
		case 'msup':
			fMouseView->MouseUp(BPoint(0,0));
			break;
			
		case B_ABOUT_REQUESTED:
			be_app->MessageReceived(theMessage);
			break;
			
		case MOUSE1:
		case MOUSE2:
		case MOUSE3:
			(fMouseView->fMouseType)->SetType(types [theMessage->what - MOUSE1]);
			CanRevert(true);
			break;

		case CLICK_CHANGE:
			s = fMouseView->fSliderBox->DoubleClickSpeedSlider();
			if (s) {
				val = s->Value();
				gMouse->SetClickSpeed(kMaxDblClickSpeed - val);
				CanRevert(true);
			}
			break;

		case msg_speed_change:
			s = fMouseView->fSliderBox->MouseSpeedSlider();
			if (s) {
				val = s->Value();
				val = int32(65536 * exp (val/65536.*log(2)));
				gMouse->SetSpeedFactor(val);
				CanRevert(true);
			}
			break;
			
		case TRACK_CHANGE:
			s = fMouseView->fSliderBox->MouseAccelerationSlider();
			if (s) {
				val = s->Value();
				val *= val;
				gMouse->SetAccelFactor(val);
				CanRevert(true);
			}
			break;

		case 'ffm ':
			{
				int8 val = -1;
				theMessage->FindInt8("ffm", &val);
				
				mode_mouse mode = (mode_mouse)val;
				if (val == 2)	mode = B_WARP_MOUSE;
				else if (val == 3)	mode = B_INSTANT_WARP_MOUSE;
				
				gMouse->SetMouseMode(mode);
			}
			CanRevert(true);
			break;

		case msg_defaults:
			SetDefaults();
			CanRevert(true);
			break;
		case msg_revert:
			Revert();
			CanRevert(false);
			break;
			
		case msg_control_changed:
			CanRevert(true);
			break;

		default:
			BWindow::MessageReceived(theMessage);
			break;
	}
}

bool
TMouseWindow::QuitRequested()
{
	SetPrefs();
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void
TMouseWindow::GetPrefs()
{
	BPath path;
	
	if (find_directory (B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		long ref;
		BPoint loc;
				
		path.Append (mouse_settings_file);
		if ((ref = open(path.Path(), O_RDWR)) >= 0) {
			
			lseek (ref, sizeof (mouse_settings), SEEK_SET);
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
	else
		MoveTo( 25, 25);
}

void
TMouseWindow::SetPrefs()
{
	BPath path;
	BPoint loc = Frame().LeftTop();

	if (find_directory (B_USER_SETTINGS_DIRECTORY, &path, true) == B_OK) {
		long ref;
		
		path.Append (mouse_settings_file);
		if ((ref = creat(path.Path(), O_RDWR)) >= 0) {
			mouse_settings settings = gMouse->Settings();
			write (ref, &settings, sizeof (mouse_settings));
			write (ref, &loc, sizeof (BPoint));
			close(ref);
			
		}
	}
}

void
TMouseWindow::SetDefaults()
{
	gMouse->SetDefaults();
	fMouseView->SetDefaults();
}

void
TMouseWindow::Revert()
{
	gMouse->Revert();
	fMouseView->Revert();
}

void
TMouseWindow::CanRevert(bool state)
{
	fBtnBar->CanRevert(state);
}

//***************************************************************************************


TMouseView::TMouseView(BRect rect)
	: TBox(rect)
{
	SetFont(be_plain_font);
	SetLineCount(3);
	
	BRect r, lr;
	
	r.Set(8, 8, 147, 134+62);
	fMouseType = new TMouseType(r);
	AddChild(fMouseType);
	
	r.left = fMouseType->Frame().Width() + 27;
	r.right = Bounds().Width() - 8;
	fSliderBox = new TSliderBox(r);
	AddChild(fSliderBox);

	r.Set( 9, Bounds().bottom - 30, 145, Bounds().bottom - 12);
	BTextControl* clickTest = new BTextControl(r, "", NULL, "", NULL);
	clickTest->SetFont(be_bold_font);
	clickTest->SetFontSize(14);
	clickTest->SetText("Double-click test area");
	clickTest->SetAlignment(B_ALIGN_CENTER, B_ALIGN_CENTER);
	BTextView* tv = clickTest->TextView();
	if (tv)								//	workaround for bug #13870
		tv->MakeResizable(false);
	AddChild(clickTest);

	// ffm
	BPopUpMenu* ffmMenu = new BPopUpMenu("ffm");
	{
		BMessage* msg = new BMessage('ffm ');
		msg->AddInt8("ffm", 0);
		BMenuItem* mitem = new BMenuItem("Disabled", msg);
		ffmMenu->AddItem(mitem);
		if (gMouse->MouseMode() == B_NORMAL_MOUSE)	mitem->SetMarked(true);
		msg = new BMessage('ffm ');
		msg->AddInt8("ffm", 1);
		mitem = new BMenuItem("Enabled", msg);
		ffmMenu->AddItem(mitem);
		if (gMouse->MouseMode() == B_FOCUS_FOLLOWS_MOUSE)	mitem->SetMarked(true);
		msg = new BMessage('ffm ');
		msg->AddInt8("ffm", 2);
		mitem = new BMenuItem("Warping", msg);
		ffmMenu->AddItem(mitem);
		if (gMouse->MouseMode() == B_WARP_MOUSE)	mitem->SetMarked(true);
		msg = new BMessage('ffm ');
		msg->AddInt8("ffm", 3);
		mitem = new BMenuItem("Instant Warping", msg);
		ffmMenu->AddItem(mitem);
		if (gMouse->MouseMode() == B_INSTANT_WARP_MOUSE)	mitem->SetMarked(true);
	}

	r.left = fSliderBox->Frame().left - 5;
	r.right = r.left + StringWidth("Focus follows mouse")
		+ StringWidth("Instant Warping") + 40;

	fFFMBtn = new BMenuField(r, "", "Focus follows mouse:", ffmMenu, false);
	fFFMBtn->SetFontSize(10);
	fFFMBtn->SetAlignment(B_ALIGN_RIGHT);
	(fFFMBtn->MenuBar())->SetFontSize(10);
	(fFFMBtn->Menu())->SetFontSize(10);
	fFFMBtn->SetDivider(StringWidth("Focus follows mouse:") + 10);
	AddChild(fFFMBtn);
	
	// left h line
	lr.Set(fMouseType->Frame().left, Bounds().bottom - 40,
		fMouseType->Frame().right+2, Bounds().bottom - 39);
	SetLineLocation(0, lr, true);
	
	//' right h line
	lr.Set(fSliderBox->Frame().left-2, Bounds().bottom - 40,
		fSliderBox->Frame().right, Bounds().bottom - 39);
	SetLineLocation(1, lr, true);	

	// middle v line
	lr.Set(fMouseType->Frame().right + 9, fMouseType->Frame().top,
		fMouseType->Frame().right + 10, clickTest->Frame().bottom + 3);
	SetLineLocation(2, lr, false);
}

TMouseView::~TMouseView()
{
}

void TMouseView::MouseDown(BPoint thePoint)
{
	//
	//	get any mousedown in the view and pass it to the mouse picture
	//	to simulate a button press
	//
	if (fMouseType)
		fMouseType->MouseDown(thePoint);
}

void TMouseView::MouseUp(BPoint thePoint)
{
	//
	//	get any mouseup in the view and pass it to the mouse picture
	//	to simulate a button press
	//
	if (fMouseType)
		fMouseType->MouseUp(thePoint);
}

void
TMouseView::SetDefaults()
{
	fMouseType->SetDefaults();
	fSliderBox->SetDefaults();
	
	mode_mouse mode = gMouse->MouseMode();
	
	int32 index;
	if (mode == B_INSTANT_WARP_MOUSE) index = 3;
	else if (mode == B_WARP_MOUSE) index = 2;
	else if (mode == B_FOCUS_FOLLOWS_MOUSE) index = 1;
	else index = 0;
	
	BMenu* menu = fFFMBtn->Menu();
	if (menu){
		BMenuItem* mitem = menu->ItemAt(index);
		if (mitem)
			mitem->SetMarked(true);
	}
}

void
TMouseView::Revert()
{
	fMouseType->Revert();
	fSliderBox->Revert();
	
	mode_mouse mode = gMouse->MouseMode();
	
	int32 index;
	if (mode == B_INSTANT_WARP_MOUSE) index = 3;
	else if (mode == B_WARP_MOUSE) index = 2;
	else if (mode == B_FOCUS_FOLLOWS_MOUSE) index = 1;
	else index = 0;
	
	BMenu* menu = fFFMBtn->Menu();
	if (menu){
		BMenuItem* mitem = menu->ItemAt(index);
		if (mitem)
			mitem->SetMarked(true);
	}
}

//****************************************************************************************

TMouseType::TMouseType(BRect frame)
	: BBox(frame, "mouse type", B_FOLLOW_NONE, B_WILL_DRAW | B_PULSE_NEEDED,
		B_NO_BORDER)
{
	fState[0] = fState[1] = fState[2] = MOUSE_UP;
	
	BuildMousePicture();
	BuildMouseTypeMenu();
	
}

TMouseType::~TMouseType()
{
	long i;
	for (i = 0; i < 3; i++)		delete fMouse[i];
	for (i = 0; i < 2; i++)		delete fM1Buttons[i];
	for (i = 0; i < 4; i++)		delete fM2Buttons[i];
	for (i = 0; i < 6; i++)		delete fM3Buttons[i];
	delete fButtonMenu;
}

void
TMouseType::AttachedToWindow()
{
	BBox::AttachedToWindow();
	SetDrawingMode(B_OP_ALPHA);
	SetDoubleBuffering(B_UPDATE_INVALIDATED);
}

void
TMouseType::BuildMousePicture()
{
	//
	//		create the mouse btn pop up menu
	//
	BMenuItem *mitem;
	fButtonMenu = new BPopUpMenu("Buttons");
	fButtonMenu->AddItem(mitem = new BMenuItem("1", new BMessage(BUT1)));
	fButtonMenu->AddItem(mitem = new BMenuItem("2", new BMessage(BUT2)));
	fButtonMenu->AddItem(mitem = new BMenuItem("3", new BMessage(BUT3)));

	// get the icones
	const uint32 type = 'PNG ';
	fMouse[0] = BTranslationUtils::GetBitmap(type, "mouse1");
	fMouse[1] = BTranslationUtils::GetBitmap(type, "mouse2");
	fMouse[2] = BTranslationUtils::GetBitmap(type, "mouse3");
	fM1Buttons[0] = BTranslationUtils::GetBitmap(type, "m1b1up");
	fM1Buttons[1] = BTranslationUtils::GetBitmap(type, "m1b1down");
	fM2Buttons[0] = BTranslationUtils::GetBitmap(type, "m2b1up");
	fM2Buttons[1] = BTranslationUtils::GetBitmap(type, "m2b1down");
	fM2Buttons[2] = BTranslationUtils::GetBitmap(type, "m2b2up");
	fM2Buttons[3] = BTranslationUtils::GetBitmap(type, "m2b2down");
	fM3Buttons[0] = BTranslationUtils::GetBitmap(type, "m3b1up");
	fM3Buttons[1] = BTranslationUtils::GetBitmap(type, "m3b1down");
	fM3Buttons[2] = BTranslationUtils::GetBitmap(type, "m3b2up");
	fM3Buttons[3] = BTranslationUtils::GetBitmap(type, "m3b2down");
	fM3Buttons[4] = BTranslationUtils::GetBitmap(type, "m3b3up");
	fM3Buttons[5] = BTranslationUtils::GetBitmap(type, "m3b3down");
}

void
TMouseType::BuildMouseTypeMenu()
{
	BMenuItem	*mitem;
	//
	//		build the mouse type menu and menu field button
	//
	fTypeMenu = new BPopUpMenu("Type");
	fTypeMenu->AddItem(mitem = new BMenuItem("1-Button", new BMessage(MOUSE1)));
	if (gMouse->MouseType() == MOUSE_1_BUTTON)
		mitem->SetMarked(TRUE);
	fTypeMenu->AddItem(mitem = new BMenuItem("2-Button", new BMessage(MOUSE2)));
	if (gMouse->MouseType() == MOUSE_2_BUTTON)
		mitem->SetMarked(TRUE);
	fTypeMenu->AddItem(mitem = new BMenuItem("3-Button", new BMessage(MOUSE3)));
	if (gMouse->MouseType() == MOUSE_3_BUTTON)
		mitem->SetMarked(TRUE);
		
	BRect rect(0, 0, 140, 20);
	BMenuField *mc = new BMenuField(rect, "type", "Mouse type", fTypeMenu);
	mc->SetFont(be_plain_font);
	mc->MenuBar()->SetFont(be_plain_font);
	mc->SetAlignment(B_ALIGN_RIGHT);
	AddChild(mc);
}

void
TMouseType::Draw(BRect where)
{
	BBox::Draw(where); // to draw the border or not
	BRect	dr;

	dr.Set(MOUSE_LEFT, MOUSE_TOP, MOUSE_LEFT + MOUSE_WIDTH, MOUSE_TOP + MOUSE_HEIGHT);
	if (dr.Intersects(where))
		DrawMouse();
}

void
TMouseType::DrawMouse()
{
	BRect	sr;
	BRect	dr;

	sr.Set(0, 0, MOUSE_WIDTH, MOUSE_HEIGHT);
	dr.Set(MOUSE_LEFT, MOUSE_TOP,
		   MOUSE_LEFT + MOUSE_WIDTH, MOUSE_TOP + MOUSE_HEIGHT);
	DrawBitmapAsync(fMouse[gMouse->MouseType() - 1], sr, dr);
	DrawMouseButton(B_PRIMARY_MOUSE_BUTTON >> 1);
	DrawMouseButton(B_SECONDARY_MOUSE_BUTTON >> 1);
	DrawMouseButton(B_TERTIARY_MOUSE_BUTTON >> 1);
}

void
TMouseType::DrawMouseButton(short button)
{
	char	string[2];
	float	width;
	BRect	sr;
	BRect	dr;

	switch (gMouse->MouseType()) {
		case 1:
			sr.Set(0, 0, M1_WIDTH, M1_HEIGHT);
			break;
		case 2:
			sr.Set(0, 0, M2_WIDTH, M2_HEIGHT);
			break;
		case 3:
			sr.Set(0, 0, M3_WIDTH, M3_HEIGHT);
			break;
	}

	string[1] = 0;
	SetHighColor(0, 0, 0);
	SetFont(be_plain_font);
	SetDrawingMode(B_OP_ALPHA);
	SetHighColor(32, 32, 32);

	if ((gMouse->MouseMap()).left == (1 << button)) {
		switch (gMouse->MouseType()) {
			case 1:
				dr.Set(MOUSE_LEFT + M1B1_H, MOUSE_TOP + M1B1_V,
					   MOUSE_LEFT + M1B1_H + M1_WIDTH, MOUSE_TOP + M1B1_V + M1_HEIGHT);
				DrawBitmapAsync(fM1Buttons[0 + fState[button]], sr, dr);
				break;
			case 2:
				dr.Set(MOUSE_LEFT + M2B1_H, MOUSE_TOP + M2B1_V,
					   MOUSE_LEFT + M2B1_H + M2_WIDTH, MOUSE_TOP + M2B1_V + M2_HEIGHT);
				DrawBitmapAsync(fM2Buttons[0 + fState[button]], sr, dr);
				break;
			case 3:
				dr.Set(MOUSE_LEFT + M3B1_H, MOUSE_TOP + M3B1_V,
					   MOUSE_LEFT + M3B1_H + M3_WIDTH, MOUSE_TOP + M3B1_V + M3_HEIGHT);
				DrawBitmapAsync(fM3Buttons[0 + fState[button]], sr, dr);
				break;
		}
		string[0] = (char)button + 1 + '0';
		width = StringWidth(string);
		MovePenTo(dr.left + ((dr.Width() - width) / 2),
				  MOUSE_TOP + MOUSE_BUTTON_TEXT_V);
		DrawString(string);
	}
	if ((gMouse->MouseMap()).right == (1 << button)) {
		switch (gMouse->MouseType()) {
			case 1:
				return;
			case 2:
				dr.Set(MOUSE_LEFT + M2B2_H, MOUSE_TOP + M2B2_V,
					   MOUSE_LEFT + M2B2_H + M2_WIDTH, MOUSE_TOP + M2B2_V + M2_HEIGHT);
				DrawBitmapAsync(fM2Buttons[2 + fState[button]], sr, dr);
				break;
			case 3:
				dr.Set(MOUSE_LEFT + M3B2_H, MOUSE_TOP + M3B2_V,
					   MOUSE_LEFT + M3B2_H + M3_WIDTH, MOUSE_TOP + M3B2_V + M3_HEIGHT);
				DrawBitmapAsync(fM3Buttons[2 + fState[button]], sr, dr);
				break;
		}
		string[0] = (char)button + 1 + '0';
		width = StringWidth(string);
		MovePenTo(dr.left + ((dr.Width() - width) / 2),
				  MOUSE_TOP + MOUSE_BUTTON_TEXT_V);
		DrawString(string);
	}
	if ((gMouse->MouseMap()).middle == (1 << button)) {
		switch (gMouse->MouseType()) {
			case 1:
			case 2:
				return;
			case 3:
				dr.Set(MOUSE_LEFT + M3B3_H, MOUSE_TOP + M3B3_V,
					   MOUSE_LEFT + M3B3_H + M3_WIDTH, MOUSE_TOP + M3B3_V + M3_HEIGHT);
				DrawBitmapAsync(fM3Buttons[4 + fState[button]], sr, dr);
				break;
		}
		string[0] = (char)button + 1 + '0';
		if (string[0] == '4')
			string[0] = '3';
		width = StringWidth(string);
		MovePenTo(dr.left + ((dr.Width() - width) / 2),
				  MOUSE_TOP + MOUSE_BUTTON_TEXT_V);
		DrawString(string);
	}
}

void
TMouseType::MouseDown(BPoint thePoint)
{
	int			button;
	int			current = 0;
	int			item = 0;
	BMenuItem	*mitem;
	BPoint		p;
	BRect		r;

	//	determine if one of the buttons was hit
	switch (gMouse->MouseType()) {
		case MOUSE_1_BUTTON:
			r.Set(MOUSE_LEFT + M1B1_H, MOUSE_TOP + M1B1_V,
				  MOUSE_LEFT + M1B1_H + M1_WIDTH, MOUSE_TOP + M1B1_V + M1_HEIGHT);
			if (r.Contains(thePoint)) {
				item = BUT1;
				current = (gMouse->MouseMap()).left;
				if (current == B_TERTIARY_MOUSE_BUTTON)
					current = 3;
			}
			break;
		case MOUSE_2_BUTTON:
			r.Set(MOUSE_LEFT + M2B1_H, MOUSE_TOP + M2B1_V,
				  MOUSE_LEFT + M2B1_H + M2_WIDTH, MOUSE_TOP + M2B1_V + M2_HEIGHT);
			if (r.Contains(thePoint)) {
				item = BUT1;
				current = (gMouse->MouseMap()).left;
				if (current == B_TERTIARY_MOUSE_BUTTON)
					current = 3;
				break;
			}
			r.Set(MOUSE_LEFT + M2B2_H, MOUSE_TOP + M2B2_V,
				  MOUSE_LEFT + M2B2_H + M2_WIDTH, MOUSE_TOP + M2B2_V + M2_HEIGHT);
			if (r.Contains(thePoint)) {
				item = BUT2;
				current = (gMouse->MouseMap()).right;
				if (current == B_TERTIARY_MOUSE_BUTTON)
					current = 3;
			}
			break;
		case MOUSE_3_BUTTON:
			r.Set(MOUSE_LEFT + M3B1_H, MOUSE_TOP + M3B1_V,
				  MOUSE_LEFT + M3B1_H + M3_WIDTH, MOUSE_TOP + M3B1_V + M3_HEIGHT);
			if (r.Contains(thePoint)) {
				item = BUT1;
				current = (gMouse->MouseMap()).left;
				if (current == B_TERTIARY_MOUSE_BUTTON)
					current = 3;
				break;
			}
			r.Set(MOUSE_LEFT + M3B2_H, MOUSE_TOP + M3B2_V,
				  MOUSE_LEFT + M3B2_H + M2_WIDTH, MOUSE_TOP + M3B2_V + M2_HEIGHT);
			if (r.Contains(thePoint)) {
				item = BUT2;
				current = (gMouse->MouseMap()).right;
				if (current == B_TERTIARY_MOUSE_BUTTON)
					current = 3;
				break;
			}
			r.Set(MOUSE_LEFT + M3B3_H, MOUSE_TOP + M3B3_V,
				  MOUSE_LEFT + M3B3_H + M2_WIDTH, MOUSE_TOP + M3B3_V + M2_HEIGHT);
			if (r.Contains(thePoint)) {
				item = BUT3;
				current = (gMouse->MouseMap()).middle;
				if (current == B_TERTIARY_MOUSE_BUTTON)
					current = 3;
			}
			break;
	}

	if (item) {
		//	if a click was detected in a 'button' then
		//	show the menu of buttons
		mitem = fButtonMenu->FindItem((current - 1) + BUT1);
		mitem->SetMarked(TRUE);
		p = thePoint;
		ConvertToScreen(&p);
		mitem = fButtonMenu->Go(p, TRUE);
		if (mitem) {
			button = (mitem->Command() - BUT1) + 1;
			if (button == 3)
				button = B_TERTIARY_MOUSE_BUTTON;
			switch (item) {
				case BUT1:
					gMouse->SetLeftMouse(button);
					Window()->PostMessage(msg_control_changed);
					break;
				case BUT2:
					gMouse->SetRightMouse(button);
					Window()->PostMessage(msg_control_changed);
					break;
				case BUT3:
					gMouse->SetMiddleMouse(button);
					Window()->PostMessage(msg_control_changed);
					break;
			}
			Invalidate();
			Window()->UpdateIfNeeded();
		}
	} else {
		//	draw a button down
		ulong buttons;
		BPoint loc;
	
		GetMouse(&loc, &buttons, FALSE);
		if ((buttons & B_PRIMARY_MOUSE_BUTTON) && 
			(fState[0] == MOUSE_UP)) {
			fState[0] = MOUSE_DOWN;
			Invalidate();
		}
		if ((buttons & B_SECONDARY_MOUSE_BUTTON) && 
			(fState[1] == MOUSE_UP)) {
			fState[1] = MOUSE_DOWN;
			Invalidate();
		}
		if ((buttons & B_TERTIARY_MOUSE_BUTTON) && 
			(fState[2] == MOUSE_UP)) {
			fState[2] = MOUSE_DOWN;
			Invalidate();
		}
		Window()->UpdateIfNeeded();
	}
}

void
TMouseType::MouseUp(BPoint /*pt*/)
{
	ulong		buttons;
	BPoint		loc;

	//	draw a button up
	GetMouse(&loc, &buttons, FALSE);
	if (!(buttons & B_PRIMARY_MOUSE_BUTTON) &&
		 (fState[0] == MOUSE_DOWN)) {
		fState[0] = MOUSE_UP;
		Invalidate();
	}
	if (!(buttons & B_SECONDARY_MOUSE_BUTTON) &&
		 (fState[1] == MOUSE_DOWN)) {
		fState[1] = MOUSE_UP;
		Invalidate();
	}
	if (!(buttons & B_TERTIARY_MOUSE_BUTTON) &&
		 (fState[2] == MOUSE_DOWN)) {
		fState[2] = MOUSE_UP;
		Invalidate();
	}
	Window()->UpdateIfNeeded();
}

void
TMouseType::SetDefaults()
{
	//
	//	the global gMouse->SetDefaults should be called first
	//	SetType handles mouse picture update
	//
	SetType(gMouse->MouseType());
}

void
TMouseType::Revert()
{
	SetType(gMouse->MouseType());
}

void
TMouseType::SetType(mouse_type type)
{
	gMouse->SetMouseType(type);
	Invalidate();
	BMenuItem *mitem;
	if (gMouse->MouseType() == MOUSE_1_BUTTON) {
		mitem = fTypeMenu->FindItem("1-Button");
		mitem->SetMarked(TRUE);
	}
	if (gMouse->MouseType() == MOUSE_2_BUTTON) {
		mitem = fTypeMenu->FindItem("2-Button");
		mitem->SetMarked(TRUE);
	}
	if (gMouse->MouseType() == MOUSE_3_BUTTON) {
		mitem = fTypeMenu->FindItem("3-Button");
		mitem->SetMarked(TRUE);
	}
}

void
TMouseType::Pulse()
{
	ulong		buttons;
	BPoint		loc;

	//	watch for buttons down and mirror them
	GetMouse(&loc, &buttons, FALSE);
	if ((buttons & B_PRIMARY_MOUSE_BUTTON) && 
		(fState[0] == MOUSE_UP)) {
		fState[0] = MOUSE_DOWN;
		Invalidate();
	}
	if (!(buttons & B_PRIMARY_MOUSE_BUTTON) &&
		 (fState[0] == MOUSE_DOWN)) {
		fState[0] = MOUSE_UP;
		Invalidate();
	}
	if ((buttons & B_SECONDARY_MOUSE_BUTTON) && 
		(fState[1] == MOUSE_UP)) {
		fState[1] = MOUSE_DOWN;
		Invalidate();
	}
	if (!(buttons & B_SECONDARY_MOUSE_BUTTON) &&
		 (fState[1] == MOUSE_DOWN)) {
		fState[1] = MOUSE_UP;
		Invalidate();
	}
	if ((buttons & B_TERTIARY_MOUSE_BUTTON) && 
		(fState[2] == MOUSE_UP)) {
		fState[2] = MOUSE_DOWN;
		Invalidate();
	}
	if (!(buttons & B_TERTIARY_MOUSE_BUTTON) &&
		 (fState[2] == MOUSE_DOWN)) {
		fState[2] = MOUSE_UP;
		Invalidate();
	}
	Window()->UpdateIfNeeded();
}

//****************************************************************************************

class TIconThing : public BView {
public:
					TIconThing(BRect frame, BBitmap* bits);
					~TIconThing();
		void		AttachedToWindow();
		void		Draw(BRect);
private:
		BBitmap*	fBits;
};

TSliderBox::TSliderBox(BRect frame)
	: BBox(frame, "sliders", B_FOLLOW_NONE,
		B_WILL_DRAW | B_PULSE_NEEDED,
		B_NO_BORDER)
{
	BRect r;

	//	Double Click speed control
	fInitialClickSpeed = gMouse->ClickSpeed();
	r.Set(0,3,Bounds().Width()-kMouseDoubleClickIconWidth-25,20);
	fDoubleClickSlider = new BSlider(r, "click", "Double-click speed",
		new BMessage(CLICK_CHANGE), kMinDblClickSpeed, kMaxDblClickSpeed);
	fDoubleClickSlider->SetLimitLabels("Slow","Fast");
	fDoubleClickSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fDoubleClickSlider->SetHashMarkCount(5);
	fDoubleClickSlider->SetKeyIncrementValue(10000);
	fDoubleClickSlider->SetValue(kMaxDblClickSpeed - fInitialClickSpeed);
	AddChild(fDoubleClickSlider);

	// Get the icones
	const uint32 type = 'PNG ';
	BBitmap *kMouseDoubleClickIconBits = BTranslationUtils::GetBitmap(type, "kMouseDoubleClickIconBits");
	BBitmap *kMouseTrackingIconBits = BTranslationUtils::GetBitmap(type, "kMouseTrackingIconBits");
	BBitmap *kMouseAccelerationIconBits = BTranslationUtils::GetBitmap(type, "kMouseAccelerationIconBits");
	
	//	Hand icon
	int32 x = (int32)(fDoubleClickSlider->Frame().right + 16);
	int32 y = (int32)(fDoubleClickSlider->Frame().top + 5);
	BRect iconRect(x, y, x+kMouseDoubleClickIconWidth-1, y+kMouseDoubleClickIconHeight-1);
	TIconThing* dblClickIcon = new TIconThing(iconRect, kMouseDoubleClickIconBits);
	AddChild(dblClickIcon);

	int32 slider_height = (int32)(fDoubleClickSlider->Bounds().Height());

	//	Mouse Speed control	
	fInitialMouseSpeed = gMouse->SpeedFactor();
	r.top = fDoubleClickSlider->Frame().bottom + 15;
	r.bottom = r.top + slider_height;
	fMouseSpeedSlider = new BSlider(r, "mouse speed", "Mouse Speed",
		new BMessage(msg_speed_change),
		kMinSpeedSlider,kMaxSpeedSlider);
	fMouseSpeedSlider->SetLimitLabels("Slow","Fast");
	fMouseSpeedSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fMouseSpeedSlider->SetHashMarkCount(7);
	fMouseSpeedSlider->SetKeyIncrementValue(kSpeedSliderKeyStep);
	fMouseSpeedSlider->SetValue(65536*(log(fInitialMouseSpeed/65536.)/log(2)));
	AddChild(fMouseSpeedSlider);

	x = (int32)(fMouseSpeedSlider->Frame().right + 5);
	y = (int32)(fMouseSpeedSlider->Frame().top  + 4 + (kMouseTrackingIconHeight/2));
	iconRect.Set(x, y, x+kMouseTrackingIconWidth-1, y+kMouseTrackingIconHeight-1);
	TIconThing* speedIcon = new TIconThing(iconRect, kMouseTrackingIconBits);
	AddChild(speedIcon);

	//	Mouse Acceleration control	
	fInitialMouseAcceleration = gMouse->AccelFactor();
	r.bottom = Bounds().Height() - 3;
	r.top = r.bottom - slider_height;
	r.top = fMouseSpeedSlider->Frame().bottom + 15;
	r.bottom = r.top + slider_height;
	fMouseAccelerationSlider = new BSlider(r, "mouse accel", "Mouse Acceleration",
		new BMessage(TRACK_CHANGE),
		kMinAccelSlider, kMaxAccelSlider);
	fMouseAccelerationSlider->SetLimitLabels("Slow","Fast");
	fMouseAccelerationSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fMouseAccelerationSlider->SetHashMarkCount(5);
	fMouseAccelerationSlider->SetKeyIncrementValue(kAccelSliderKeyStep);
	fMouseAccelerationSlider->SetValue(sqrt(fInitialMouseAcceleration));
	AddChild(fMouseAccelerationSlider);

	x = (int32)(fMouseAccelerationSlider->Frame().right + 5);
	y = (int32)(fMouseAccelerationSlider->Frame().top  + 4 + (kMouseAccelerationIconWidth/2));
	iconRect.Set(x, y, x+kMouseTrackingIconWidth-1, y+kMouseAccelerationIconHeight-1);
	TIconThing* accelIcon = new TIconThing(iconRect, kMouseAccelerationIconBits);
	AddChild(accelIcon);
}
				
TSliderBox::~TSliderBox()
{
}
				
void
TSliderBox::Draw(BRect bounds)
{
	BBox::Draw(bounds);
}

void
TSliderBox::AttachedToWindow()
{
	BBox::AttachedToWindow();
}

void
TSliderBox::SetDefaults()
{
	fDoubleClickSlider->SetValue(kMaxDblClickSpeed - kDefaultDblClickSpeed);
	fMouseAccelerationSlider->SetValue(kDefaultAccelSlider);
	fMouseSpeedSlider->SetValue(kDefaultSpeedSlider);
}

void
TSliderBox::Revert()
{
	fDoubleClickSlider->SetValue(kMaxDblClickSpeed - fInitialClickSpeed);
	fMouseAccelerationSlider->SetValue(sqrt(fInitialMouseAcceleration));
	fMouseSpeedSlider->SetValue(65536*(log(fInitialMouseSpeed/65536.)/log(2)));
}

BSlider*
TSliderBox::DoubleClickSpeedSlider()
{
	return fDoubleClickSlider;
}

BSlider*
TSliderBox::MouseAccelerationSlider()
{
	return fMouseAccelerationSlider;
}

BSlider*
TSliderBox::MouseSpeedSlider()
{
	return fMouseSpeedSlider;
}

//****************************************************************************************

static void
DumpMouseSettings(mouse_settings m)
{
	printf("**\n");
	printf("  mouse type: %d\n",m.type);
	printf("  mouse map: left   - %ld\n",m.map.left);
	printf("             right  - %ld\n",m.map.right);
	printf("             middle - %ld\n",m.map.middle);
	printf("  mouse acceleration: enabled      - %d\n",m.accel.enabled);
	printf("                      accel factor - %ld\n",m.accel.accel_factor);
	printf("                      speed        - %ld\n",m.accel.speed);
	printf("  click speed: %Li\n",m.click_speed);
	printf("**\n");
}

TMouseSettings::TMouseSettings()
{
	GetCurrent();
//	DumpMouseSettings(fMouseMap);
}

TMouseSettings::~TMouseSettings()
{
	SetCurrent();
}

void
TMouseSettings::GetCurrent()
{
	mouse_map m;
	get_mouse_map(&m);
	fMouseMap.map.left = m.left;
	fMouseMap.map.right = m.right;
	fMouseMap.map.middle = m.middle;
	
	get_click_speed(&fMouseMap.click_speed);
	if (fMouseMap.click_speed < kMinDblClickSpeed)
		fMouseMap.click_speed = kMinDblClickSpeed;
	if (fMouseMap.click_speed > kMaxDblClickSpeed)
		fMouseMap.click_speed = kMaxDblClickSpeed;

	get_mouse_speed(&fMouseMap.accel.speed);
	if (fMouseMap.accel.speed < kMinMouseSpeed)
		fMouseMap.accel.speed = kMinMouseSpeed;
	if (fMouseMap.accel.speed > kMaxMouseSpeed)
		fMouseMap.accel.speed = kMaxMouseSpeed;
	
	get_mouse_acceleration(&fMouseMap.accel.accel_factor);
	if (fMouseMap.accel.accel_factor < kMinAccelFactor)
		fMouseMap.accel.accel_factor = kMinAccelFactor;
	if (fMouseMap.accel.accel_factor > kMaxAccelFactor)
		fMouseMap.accel.accel_factor = kMaxAccelFactor;
	
	fMouseMap.accel.enabled = kDefaultAccelEnabled;
	
	get_mouse_type((long*)&fMouseMap.type);
	
	fOriginal = fMouseMap;
	
	fFFMState = fFFMInitialState = mouse_mode();
}

void
TMouseSettings::SetCurrent()
{
	mouse_map m;
	
	set_mouse_type((int32)MouseType());

	m.left = fMouseMap.map.left;
	m.right = fMouseMap.map.right;
	m.middle = fMouseMap.map.middle;	
	set_mouse_map(&m);
		
	set_click_speed(ClickSpeed());
	
	set_mouse_acceleration(AccelFactor());
	set_mouse_speed(SpeedFactor());
	
	set_mouse_mode(fFFMState);
}

mouse_type 
TMouseSettings::MouseType()
{
	return fMouseMap.type;
}

void 
TMouseSettings::SetMouseType(mouse_type m)
{
	fMouseMap.type = m;
	set_mouse_type((int32)m);
}

map_mouse 
TMouseSettings::MouseMap()
{
	return fMouseMap.map;
}

void
TMouseSettings::UpdateMouseMap()
{
	mouse_map m2;
	
	m2.left = fMouseMap.map.left;
	m2.right = fMouseMap.map.right;
	m2.middle = fMouseMap.map.middle;
	
	set_mouse_map(&m2);
}

void 
TMouseSettings::SetMouseMap(map_mouse m)
{
	fMouseMap.map = m;

	UpdateMouseMap();
}

void
TMouseSettings::SetMouseMap(int32 l, int32 m, int32 r)
{
	fMouseMap.map.left = l;
	fMouseMap.map.middle = m;
	fMouseMap.map.right = r;
	UpdateMouseMap();
}

void
TMouseSettings::SetLeftMouse(int32 m)
{
	fMouseMap.map.left = m;
	UpdateMouseMap();
}

void
TMouseSettings::SetMiddleMouse(int32 m)
{
	fMouseMap.map.middle = m;
	UpdateMouseMap();
}

void
TMouseSettings::SetRightMouse(int32 m)
{
	fMouseMap.map.right = m;
	UpdateMouseMap();
}

//

mouse_accel 
TMouseSettings::MouseAcceleration()
{
	return fMouseMap.accel;
}

bool 
TMouseSettings::AccelerationEnabled()
{
	return fMouseMap.accel.enabled;
}

void
TMouseSettings::SetAccelEnabled(bool e)
{
	fMouseMap.accel.enabled = e;
}

//

int32 
TMouseSettings::AccelFactor()
{
	return fMouseMap.accel.accel_factor;
}

void
TMouseSettings::SetAccelFactor(int32 a)
{
	fMouseMap.accel.accel_factor = a;
	set_mouse_acceleration(fMouseMap.accel.accel_factor);
}

//

int32 
TMouseSettings::SpeedFactor()
{
	return fMouseMap.accel.speed;
}

void
TMouseSettings::SetSpeedFactor(int32 s)
{
	fMouseMap.accel.speed = s;
	set_mouse_speed(fMouseMap.accel.speed);
}

//

void 
TMouseSettings::SetMouseAcceleration(mouse_accel m)
{
	fMouseMap.accel = m;
}

void 
TMouseSettings::SetMouseAcceleration(bool e, int32 a, int32 s)
{
	fMouseMap.accel.enabled = e;
	fMouseMap.accel.accel_factor = a;
	fMouseMap.accel.speed = s;
}

//

bigtime_t 
TMouseSettings::ClickSpeed()
{
	return fMouseMap.click_speed;
}

void 
TMouseSettings::SetClickSpeed(bigtime_t t)
{
	fMouseMap.click_speed = t;
	set_click_speed(fMouseMap.click_speed);
}

//

bool
TMouseSettings::FFMIsOn()
{
	return fFFMState;
}

void
TMouseSettings::SetFFM(bool s)
{
	if (s)
		fFFMState = B_FOCUS_FOLLOWS_MOUSE;
	else
		fFFMState = B_NORMAL_MOUSE;
	set_focus_follows_mouse(fFFMState);
}

void
TMouseSettings::SetMouseMode(mode_mouse which)
{
	fFFMState = which;
	set_mouse_mode(fFFMState);
}

mode_mouse
TMouseSettings::MouseMode()
{
	return fFFMState;
}

//

void
TMouseSettings::SetDefaults()
{
	SetMouseType(MOUSE_3_BUTTON);
	SetMouseMap(B_PRIMARY_MOUSE_BUTTON, B_TERTIARY_MOUSE_BUTTON,
		B_SECONDARY_MOUSE_BUTTON);		
	SetMouseAcceleration(kDefaultAccelEnabled,kDefaultAccelFactor,kDefaultMouseSpeed);
	SetClickSpeed(kMaxDblClickSpeed - kDefaultDblClickSpeed);

	SetFFM(false);
	
	SetCurrent();
}

void
TMouseSettings::Revert()
{
	SetMouseType(fOriginal.type);
	SetMouseMap(fOriginal.map);		
	SetMouseAcceleration(fOriginal.accel.enabled,fOriginal.accel.accel_factor,fOriginal.accel.speed);
	SetClickSpeed(fOriginal.click_speed);
	
	SetMouseMode(fFFMInitialState);
	
	SetCurrent();	
}

//*********************************************************************

const rgb_color kWhite = { 255, 255, 255, 255};

TBox::TBox(BRect frame)
	: BBox(frame)
{
	fLineCount=0;
}

void
TBox::Draw(BRect updateRect)
{
	BRect r;
	bool o;
	BBox::Draw(updateRect);
	
	PushState();
	
	for (int32 index=0 ; index<fLineCount ; index++) {
		r = fLineArray[index].frame;
		o = fLineArray[index].orientation;
		
		SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_3_TINT));
		if (o)
			StrokeLine(r.LeftTop(), r.RightTop());
		else
			StrokeLine(r.LeftTop(), r.LeftBottom());
			
		SetHighColor(kWhite);
		if (o)
			StrokeLine(r.LeftBottom(), r.RightBottom());
		else
			StrokeLine(r.RightTop(), r.RightBottom());
	}
	
	PopState();
}

void
TBox::SetLineCount(int32 c)
{
	fLineCount = c;
}

void
TBox::SetLineLocation(int32 index, BRect lineFrame, bool orientation)
{
	if (index < 0 && index >= 256)
		return;
		
	fLineArray[index].frame = lineFrame;
	fLineArray[index].orientation = orientation;
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
TButtonBar::Draw(BRect r)
{
	BView::Draw(r);
	
	PushState();
	
	if (fHasOtherBtn) {
		BPoint top, bottom;
		
		top.x = fOtherBtn->Frame().right + 7;
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

TIconThing::TIconThing(BRect frame, BBitmap* bits)
	: BView(frame, "icon thing", B_FOLLOW_NONE, B_WILL_DRAW)
{
	fBits = bits;
}

TIconThing::~TIconThing()
{
	delete fBits;
}

void
TIconThing::AttachedToWindow()
{
	if (Parent())	SetViewColor(Parent()->ViewColor());
	else			SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetDrawingMode(B_OP_OVER);
}

void
TIconThing::Draw(BRect)
{
	DrawBitmapAsync(fBits, Bounds());
}
