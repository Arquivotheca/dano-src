#include "ConfirmWindow.h"

#include <Bitmap.h>
#include <Path.h>
#include <FindDirectory.h>
#include <File.h>
#include <Resources.h>
#include <stdlib.h>
#include <Button.h>
#include <ClassInfo.h>
#include <Debug.h>
#include <CheckBox.h>
#include <Screen.h>
#include <TextView.h>

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

static void
CenterWindowOnScreen(BWindow* w)
{
	BRect	screenFrame = (BScreen(B_MAIN_SCREEN_ID).Frame());
	BPoint 	pt;
	pt.x = screenFrame.Width()/2 - w->Bounds().Width()/2;
	pt.y = screenFrame.Height()/2 - w->Bounds().Height()/2;

	if (screenFrame.Contains(pt))
		w->MoveTo(pt);
}

TAlertView::TAlertView(BRect b, alert_type type)
	: BView(b, "", B_FOLLOW_ALL, B_WILL_DRAW),
		fMsgType(type)
{
	GetIcon();
}

TAlertView::~TAlertView()
{
	if (fIconBits)
		delete fIconBits;
}

void
TAlertView::AttachedToWindow()
{
	if (Parent()) {
		rgb_color c = Parent()->ViewColor();
		SetViewColor(c);
		SetLowColor(c);
	} else {
		SetViewColor(216, 216, 216, 255);
		SetLowColor(216, 216, 216, 255);
	}
}

#define SHOW_COLOR 0	// this idea is not done yet, an example of a colored alert as per tim
void
TAlertView::Draw(BRect u)
{
	BView::Draw(u);
	BRect r(Bounds());

#if SHOW_COLOR
	//	gray border
	SetHighColor(80, 80, 80);
	StrokeRect(r);
	
	// 	lt yellow border
	r.InsetBy(1,1);
	SetHighColor(255,255,102);
	StrokeRect(r);
	
	// dk yellow border
	r.InsetBy(-1,-1);
	r.right--; r.bottom--;
	SetHighColor(255,152,0);
	StrokeRect(r);
	
	// fill of yellow
	r.InsetBy(1,1);
	r.top++;
	r.left++;
	SetHighColor(255,203,0,255);
	FillRect(r);
	
	//	lt yellow frame
	r.right -= 2; r.left += 2;
	r.top += 16; r.bottom -= 2;
	SetHighColor(255,255,102,255);
	StrokeLine(r.LeftBottom(), r.RightBottom());
	StrokeLine(r.RightBottom(), r.RightTop());
	
	SetHighColor(255,152,0,255);
	StrokeLine(r.LeftBottom(), r.LeftTop());
	StrokeLine(r.LeftTop(), r.RightTop());
	
	// gray border
	SetHighColor(152, 152, 152, 255);
	r.InsetBy(1,1);
	StrokeRect(r);
	
	r.InsetBy(1,1);
	SetHighColor(184,184,184,255);
	StrokeLine(r.LeftBottom(), r.RightBottom());
	StrokeLine(r.RightBottom(), r.RightTop());
	
	SetHighColor(255,255,255,255);
	StrokeLine(r.LeftBottom(), r.LeftTop());
	StrokeLine(r.LeftTop(), r.RightTop());
	
	// interior
	r.InsetBy(1,1);
	BRect tempr(r);
	tempr.right = tempr.left + 40;
	SetHighColor(200, 200, 200, 255);
	SetLowColor(ViewColor());
	FillRect(tempr);
	
	tempr = r;
	tempr.left = r.left+41;
	SetHighColor(216,216,216, 255);
	FillRect(tempr);

	//	draw icon
	r.Set(30, 38, 61, 69);
	SetDrawingMode(B_OP_OVER);
	DrawBitmap(fIconBits, BRect(0,0,31,31), r);
	SetDrawingMode(B_OP_COPY);
	
	// window title
	SetHighColor(0,0,0,255);
	MovePenTo(15, 13);
	SetFont(be_bold_font);
	DrawString("Warning");
#else	
	//	draw gray box
	r.right = r.left + 30;
	SetHighColor(184, 184, 184, 255);
	SetLowColor(ViewColor());
	FillRect(r);

	//	draw icon
	r.Set(18, 6, 49, 37);
	SetDrawingMode(B_OP_OVER);
	DrawBitmap(fIconBits, BRect(0,0,31,31), r);
	SetDrawingMode(B_OP_COPY);
#endif
}

void
TAlertView::GetIcon()
{
	if (fMsgType != B_EMPTY_ALERT) {
		BPath path;
		if (find_directory (B_BEOS_SERVERS_DIRECTORY, &path) == B_OK) {
			// why the hell is this code copied 13+ x times in all
			// the preferences???
			path.Append ("app_server");
			BFile		file(path.Path(), O_RDONLY);
			BResources	rfile;

			if (rfile.SetTo(&file) == B_NO_ERROR) {
				size_t	size;
				char	*name = "";
				switch(fMsgType) {
					case B_INFO_ALERT:		name = "info"; break;
					case B_IDEA_ALERT:		name = "idea"; break;
					case B_WARNING_ALERT:	name = "warn"; break;
					case B_STOP_ALERT:		name = "stop"; break;
					default:
						TRESPASS();
				}
				void *data = rfile.FindResource('ICON', name, &size);

				if (data) {
					fIconBits = new BBitmap(BRect(0,0,31,31), B_COLOR_8_BIT);
					fIconBits->SetBits(data, size, 0, B_COLOR_8_BIT);
					free(data);
				}
			} 
		}
		if (!fIconBits) {
			// couldn't find icon so make this an B_EMPTY_ALERT
			fMsgType = B_EMPTY_ALERT;
		}
	} else
		fIconBits = NULL;
}

const char* poofstr1 = "WARNING: This is a very high resolution. ";
const char* poofstr2 = "You risk damaging your display and even starting a fire if you ";
const char* poofstr3 = "select a resolution your display isn't designed to support. ";
const char* poofstr4 = "Read the owner's guide that came with your display ";
const char* poofstr5 = "to make sure it supports this very high resolution.";

const float kAlertWidth = 350;
const float kAlertHeight = 240;

#if SHOW_COLOR
TPoofAlert::TPoofAlert(alert_type type)
	: BWindow(BRect(0,0,kAlertWidth, kAlertHeight), "Poof",
		B_NO_BORDER_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_NOT_CLOSABLE | B_NOT_RESIZABLE | B_NOT_MINIMIZABLE),
			fAlertType(type)
#else
TPoofAlert::TPoofAlert(alert_type type)
	: BWindow(BRect(0,0,kAlertWidth, kAlertHeight), "Poof",
		B_MODAL_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_NOT_CLOSABLE | B_NOT_RESIZABLE | B_NOT_MINIMIZABLE),
			fAlertType(type)
#endif
{
	fSkip = false;
	BRect r(Bounds());

	fBG = new TAlertView(r, fAlertType);
	fBG->SetFont(be_plain_font);
	AddChild(fBG);
	
	float fh = FontHeight(fBG, true);
	
	r.left = r.left + 55;
	r.right -= 10;
	r.top += 6;
	r.bottom = r.top + 4 + (fh * 5) + 4;
	BRect r2(r);
	r2.OffsetTo(0, 0);
	fMsgFld = new BTextView(r, "msg", r2, B_FOLLOW_NONE, B_WILL_DRAW);
	fMsgFld->MakeEditable(false);
	fMsgFld->MakeSelectable(false);
	fMsgFld->SetViewColor(fBG->ViewColor());
	fBG->AddChild(fMsgFld);
	fMsgFld->SetText(poofstr1);
	fMsgFld->Insert(strlen(poofstr1), poofstr2, strlen(poofstr2));
	fMsgFld->Insert(fMsgFld->TextLength(), poofstr3, strlen(poofstr3));
	fMsgFld->Insert(fMsgFld->TextLength(), poofstr4, strlen(poofstr4));
	fMsgFld->Insert(fMsgFld->TextLength(), poofstr5, strlen(poofstr5));

	r.right = Bounds().Width() - 10;
	r.left = r.right - 75;
	r.top = fMsgFld->Frame().bottom + 14;
	r.bottom = r.top + 20;	
	fCancelBtn = new BButton(r, "Cancel", "Cancel", new BMessage('exit'));
	fBG->AddChild(fCancelBtn);
	
	r.right = r.left - 10;
	r.left = r.right - 75;
	fPoofBtn = new BButton(r, "Poof!", "Poof!", new BMessage('poof'));
	fBG->AddChild(fPoofBtn);
	
	r.top += 4; r.bottom += 4;
	r.left = 55;
	r.right = r.left + fBG->StringWidth("Dont' show again") + 24;
	fBypassWarning = new BCheckBox(r, "bypass", "Don't show again", new BMessage('skip'));
	fBG->AddChild(fBypassWarning);
	
	ResizeTo(kAlertWidth, fPoofBtn->Frame().bottom + 10);
	
	CenterWindowOnScreen(this);
	SetDefaultButton(fCancelBtn);
}

TPoofAlert::~TPoofAlert()
{
}

void TPoofAlert::MessageReceived(BMessage *m)
{
	switch(m->what)
	{
		case 'skip':
			fSkip = fBypassWarning->Value();
			break;
		case 'exit':
			fAlertVal = 1;
			delete_sem(fAlertSem);
			fAlertSem = -1;
			break;
		case 'poof':
			fAlertVal = 0;
			delete_sem(fAlertSem);
			fAlertSem = -1;
			break;
	}
}

int32 TPoofAlert::Go()
{
	long		value;
	thread_id	this_tid = find_thread(NULL);
	BLooper		*loop;
	BWindow		*wind = NULL;

	fAlertSem = create_sem(0, "AlertSem");
	loop = BLooper::LooperForThread(this_tid);
	if (loop)
		wind = cast_as(loop, BWindow);

	Show();

	long err;

	if (wind) {
		// A window is being blocked. We'll keep the window updated
		// by calling UpdateIfNeeded.
		while (1) {
			while ((err = acquire_sem_etc(fAlertSem, 1, B_TIMEOUT, 50000)) == B_INTERRUPTED)
				;
			if (err == B_BAD_SEM_ID)
				break;
			wind->UpdateIfNeeded();
		}
	} else {
		do {
			err = acquire_sem(fAlertSem);
		} while (err == B_INTERRUPTED);
	}

	// synchronous call to close the alert window. Remember that this will
	// 'delete' the object that we're in. That's why the return value is
	// saved on the stack.
	value = fAlertVal;

	if (Lock()) {
		Quit();
	}

	return value;
}
