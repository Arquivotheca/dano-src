#include "FadeView.h"
#include "Blanket.h"
#include "Settings.h"
#include "CMonitorControl.h"
#include "Sliders.h"
#include "ssdefs.h"
#include "PasswordPanel.h"

#include <CheckBox.h>
#include <TextControl.h>
#include <Box.h>
#include <Message.h>
#include <Roster.h>
#include <Application.h>
#include <Path.h>
#include <FindDirectory.h>
#include <Beep.h>
#include <StringView.h>
#include <Slider.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <InterfaceDefs.h>
#include <Screen.h>
#include <Button.h>

#include <stdio.h>
#include <stdlib.h>

FadeView::FadeView(BRect frame, const char *name)
 : BView(frame, name, B_WILL_DRAW, 0)
{
	rgb_color	col = ui_color(B_PANEL_BACKGROUND_COLOR);

	SetViewColor(col);

	BRect r = Bounds();
	r.InsetBy(5, 5);
	BBox *back = new BBox(r, "back", B_FOLLOW_ALL, B_WILL_DRAW);
	AddChild(back);

	dofade = new BCheckBox(BRect(0, 0, 120, 20), "dolock", "Enable ScreenSaver", new BMessage('dofd'));
	back->SetLabel(dofade);

	runtext = new BStringView(BRect(40, 30, 120, 45), B_EMPTY_STRING, "Run module");
	back->AddChild(runtext);
	fade = new RunSlider(BRect(135, 32, 420, 75), "fade", "Run module", dofade,
		new BMessage('fade'));
	back->AddChild(fade);

	dodpms = new BCheckBox(BRect(20, 80, 120, 95), "dodpms", "Turn off screen", new BMessage('dodp'));
	back->AddChild(dodpms);
	dpms = new UpperSlider(BRect(135, 82, 420, 125), "dpms", "Turn off screen", dodpms,
		new BMessage('dpms'));
	back->AddChild(dpms);

	dolock = new BCheckBox(BRect(20, 130, 120, 145), "dolock", "Password lock", new BMessage('dolk'));
	back->AddChild(dolock);
	nolock = new UpperSlider(BRect(135, 132, 420, 175), "nolock", "Password lock", dolock,
		new BMessage('nolk'));
	back->AddChild(nolock);

	passbutton = new BButton(BRect(335, 176, 410, 196), "pass", "Password" B_UTF8_ELLIPSIS, new BMessage('pass'));
	back->AddChild(passbutton);

	separator = new BBox(BRect(10, 215, r.right - 20, 215), "");
	back->AddChild(separator);

	// fade corner
	fadecorner = new CMonitorControl(BRect(50, 230, 90, 270), 
							 "fademonitor", new BMessage('fadc'), -1);
	back->AddChild(fadecorner);

	BRect monCapFrame;
	BRect monCapArea;
	monCapFrame.Set(100, 240, 180, 265);
	monCapArea = monCapFrame;
	monCapArea.OffsetTo(B_ORIGIN);
	cap1 = new BTextView(monCapFrame, B_EMPTY_STRING, monCapArea, 0, B_WILL_DRAW);
	cap1->SetText("Fade now when mouse is here");			
	back->AddChild(cap1);
	cap1->SetViewColor(col);
	cap1->SetDrawingMode(B_OP_OVER);
	cap1->MakeEditable(false);
	cap1->MakeSelectable(false);
	cap1->SetWordWrap(true);

	// no fade corner
	nofadecorner = new CMonitorControl(BRect(250, 230, 290, 270), 
							 "nofademonitor", new BMessage('!fdc'), -1);
	back->AddChild(nofadecorner);

	monCapFrame.Set(300, 240, 380, 265);
	monCapArea = monCapFrame;
	monCapArea.OffsetTo(B_ORIGIN);
	cap2 = new BTextView(monCapFrame, B_EMPTY_STRING, monCapArea, 0, B_WILL_DRAW);
	cap2->SetText("Don't fade when mouse is here");	
	back->AddChild(cap2);
	cap2->SetViewColor(col);
	cap2->SetDrawingMode(B_OP_OVER);
	cap2->MakeEditable(false);
	cap2->MakeSelectable(false);
	cap2->SetWordWrap(true);	

	((RunSlider *)fade)->SetUpper(dpms, nolock);
	((UpperSlider *)dpms)->SetLower(fade, 0);
	((UpperSlider *)dpms)->SetLimit(fade);
	((UpperSlider *)nolock)->SetLower(fade, 0);
	((UpperSlider *)nolock)->SetLimit(fade);
	fade->SetDrawOnUpdate(nolock, dpms);
}

void FadeView::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case 'enab' :
		case 'dofd' :
		case 'dodp' :
		case 'dolk' :
			CheckDependencies();
			SaveState();
			break;

		case 'fade' :
		case 'dpms' :
		case 'nolk' :
			SaveState();
			break;

		case 'pass' :
			(new PasswordPanel())->Show();
			break;

		case 'fadc' :
		case '!fdc' :
			SaveState();
			break;

		default :
			BView::MessageReceived(msg);
			break;
	}
}

void FadeView::LoadState(void)
{
	// fetch current settings
	BMessage *m = AcquireSettings();

	int32	now;
	int32	never;
	if(m->FindInt32(kCornerNow, &now) == B_OK)
		fadecorner->SetValue(now);
	if(m->FindInt32(kCornerNever, &never) == B_OK)
		nofadecorner->SetValue(never);

	int32	flags;
	if(m->FindInt32(kTimeFlags, &flags) == B_OK)
	{
		dofade->SetValue(flags & kDoFade ? 1 : 0);
		dodpms->SetValue((flags & (kDoStandby | kDoSuspend | kDoOff)) ? 1 : 0);
	}

	int32	t = 0, t1;
	if(m->FindInt32(kTimeFade, &t) == B_OK)
		fade->SetTime(t);
	if(m->FindInt32(kTimeStandby, &t1) == B_OK)
		dpms->SetTime(t + t1);

	bool	lock;
	if(m->FindBool(kLockEnable, &lock) == B_OK)
		dolock->SetValue(lock);

	if(m->FindInt32(kLockDelay, &t) == B_OK)
		nolock->SetTime(t);

	CheckDependencies();

	ReleaseSettings();
}

void FadeView::CheckDependencies()
{
	bool	dpmsena = (BScreen().DPMSCapabilites() & (B_DPMS_STAND_BY | B_DPMS_SUSPEND | B_DPMS_OFF)) ? true : false;
	if(! dpmsena)
		dodpms->SetValue(0);

	if(dofade->Value())
	{
		fade->SetEnabled(true);
		dodpms->SetEnabled(dpmsena);
		dpms->SetEnabled(dodpms->Value());
		dolock->SetEnabled(true);
		nolock->SetEnabled(dolock->Value());
		fadecorner->SetEnabled(true);
		nofadecorner->SetEnabled(true);
		cap1->SetDrawingMode(B_OP_OVER);
		cap1->Invalidate();
		cap2->SetDrawingMode(B_OP_OVER);
		cap2->Invalidate();
		runtext->SetDrawingMode(B_OP_OVER);
		runtext->Invalidate();
		passbutton->SetEnabled(dolock->Value());
		separator->SetDrawingMode(B_OP_OVER);
		separator->Invalidate();
	}
	else
	{
		// disable everything
		fade->SetEnabled(false);
		dodpms->SetEnabled(false);
		dpms->SetEnabled(false);
		dolock->SetEnabled(false);
		nolock->SetEnabled(false);
		fadecorner->SetEnabled(false);
		nofadecorner->SetEnabled(false);
		passbutton->SetEnabled(false);
		cap1->SetDrawingMode(B_OP_BLEND);
		cap1->Invalidate();
		cap2->SetDrawingMode(B_OP_BLEND);
		cap2->Invalidate();
		runtext->SetDrawingMode(B_OP_BLEND);
		runtext->Invalidate();
		separator->SetDrawingMode(B_OP_BLEND);
		separator->Invalidate();
	}
	Window()->UpdateIfNeeded();
}

void FadeView::SaveState(bool write)
{
	int32	mask = (dofade->Value() ? kDoFade : 0) | (dodpms->Value() ? (kDoStandby | kDoSuspend | kDoOff) : 0);
	int32	timediff = dofade->Value() ? dpms->Time() - fade->Time() : dpms->Time();

	BMessage *m = AcquireSettings();

	// to simplify the UI if standby happens after 10 minutes then
	// suspend happens after another 10 minutes and off after 10 more.
	m->ReplaceInt32(kTimeFlags, mask);
	m->ReplaceInt32(kTimeFade, fade->Time());
	m->ReplaceInt32(kTimeStandby, timediff);
	m->ReplaceInt32(kTimeSuspend, timediff);
	m->ReplaceInt32(kTimeOff, timediff);

	// corner settings
	m->ReplaceInt32(kCornerNow, fadecorner->Value());
	m->ReplaceInt32(kCornerNever, nofadecorner->Value());

	// lock settings
	m->ReplaceBool(kLockEnable, dolock->Value() ? true : false);
	m->ReplaceInt32(kLockDelay, nolock->Time());

	ReleaseSettings();

	// save to disk
	if(write)
		SaveSettings();
}

void FadeView::AttachedToWindow()
{
	dofade->SetTarget(this);
	fade->SetTarget(this);
	dodpms->SetTarget(this);
	dpms->SetTarget(this);
	dolock->SetTarget(this);
	nolock->SetTarget(this);
	fadecorner->SetTarget(this);
	nofadecorner->SetTarget(this);
	passbutton->SetTarget(this);
}

void FadeView::AllAttached()
{
	LoadState();
}
