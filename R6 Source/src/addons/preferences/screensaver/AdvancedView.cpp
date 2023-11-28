#include "AdvancedView.h"

#include "Settings.h"
#include "ssdefs.h"
#include "ModuleListView.h"
#include "ModuleListItem.h"
#include "ModulePreviewView.h"
#include "ModuleRoster.h"
#include "CMonitorControl.h"
#include "Sliders.h"

#include <Message.h>
#include <Rect.h>
#include <Messenger.h>
#include <CheckBox.h>
#include <TextControl.h>
#include <Roster.h>
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
#include <FilePanel.h>
#include <ScrollView.h>
#include <Message.h>
#include <Box.h>
#include <List.h>
#include <RadioButton.h>
#include <StringView.h>
#include <Alert.h>
#include <Box.h>
#include <MessageFilter.h>
#include <Screen.h>
#include <TextControl.h>
#include <Beep.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> //crypt

class LineView : public BView
{
public:
	LineView(BRect frame)
	 : BView(frame, "line", 0, B_WILL_DRAW)
	{
	}

	void AttachedToWindow()
	{
		SetViewColor(Parent()->ViewColor());
	}

	void Draw(BRect)
	{
		BRect r = Bounds();
		SetHighColor(156, 156, 156);
		StrokeLine(BPoint(r.left, 10), BPoint(r.right, 10), B_SOLID_HIGH);
		SetHighColor(156, 156, 156);
		StrokeLine(BPoint(r.left, 11), BPoint(r.right, 11), B_SOLID_HIGH);
		SetHighColor(Parent()->ViewColor());
		FillRect(BRect(r.left, 0, r.right, 9), B_SOLID_HIGH);
		FillRect(BRect(r.left, 12, r.right, r.bottom), B_SOLID_HIGH);
	}
};


const char* kNoMatchStr = "Passwords don't match. Try again.";
const char* kOKStr = "OK";
const char* kCancelStr = "Cancel";
const char* kDoneStr = "Done";
const char* kConfirmPwdStr = "Confirm password:";
const char* kPwdStr = "Password:";

#define MSG_NETRADIO			'pnet'
#define MSG_CUSTOMRADIO			'pcst'

// Examine messages before dispatched. This allows the password
// control to put up **** instead of text.
class MyFilter : public BMessageFilter
{
	BTextControl	*pass1;
	BTextControl	*pass2;
	BTextView		*real1;
	BTextView		*real2;	

public:
	MyFilter(BTextControl *p1, BTextControl *p2, BTextView *r1, BTextView *r2);
	filter_result Filter(BMessage *message, BHandler **target);
};


AdvancedView::AdvancedView(BRect frame, const char *name)
 : BView(frame, name, B_WILL_DRAW, 0)
{
	rgb_color	col = ui_color(B_PANEL_BACKGROUND_COLOR);

	SetViewColor(col);
	SetLowColor(col);

	float bottom = frame.Height();
	float right = frame.Width();

	LineView	*topbar = new LineView(BRect(10, 0, right - 10, 24));
	AddChild(topbar);

	rgb_color that_blue = { 115, 120, 184, 255 };

	dodpms = new BCheckBox(BRect(10, 25, 130, 40), "dodpms", "Turn off screen after:", new BMessage('dodp'));
	AddChild(dodpms);
	timedpms = new BStringView(BRect(135, 25, 195, 40), "timedpms", "");
	AddChild(timedpms);
	timedpms->SetAlignment(B_ALIGN_RIGHT);
	dpms = new UpperSlider(BRect(200, 25, right - 10, 40), "dpms", 0, timedpms,
		new BMessage('dpms'));
	dpms->UseFillColor(true, &that_blue);
	AddChild(dpms);

	dolock = new BCheckBox(BRect(10, 60, 130, 75), "dolock", "Password lock after:", new BMessage('dolk'));
	AddChild(dolock);
	timelock = new BStringView(BRect(135, 60, 195, 75), "timelock", "");
	AddChild(timelock);
	timelock->SetAlignment(B_ALIGN_RIGHT);
	nolock = new UpperSlider(BRect(200, 60, right - 10, 75), "nolock", 0, timelock,
		new BMessage('nolk'));
	nolock->UseFillColor(true, &that_blue);
	AddChild(nolock);

//	((RunSlider *)fade)->SetUpper(dpms, nolock);
//	((UpperSlider *)dpms)->SetLower(fade, 0);
//	((UpperSlider *)dpms)->SetLimit(fade);
//	((UpperSlider *)nolock)->SetLower(fade, 0);
//	((UpperSlider *)nolock)->SetLimit(fade);
//	fade->SetDrawOnUpdate(nolock, dpms);

	BTextView *tv;

	BMessage *m = AcquireSettings();
	const char *method;
	m->FindString(kLockMethod, &method);
	int32 net = 0;
	if(strcasecmp(method, "net") == 0)
		net = 1;
	ReleaseSettings();

	const char *text = "Use custom password:";
	float w = StringWidth(text);

	netradio = new BRadioButton(BRect(28, 85, 170, 100), "net", "Use Network password", new BMessage(MSG_NETRADIO));
	AddChild(netradio);
	customradio = new BRadioButton(BRect(28, 110, 50 + w, 125), "custom", text, new BMessage(MSG_CUSTOMRADIO));
	AddChild(customradio);

	if(net)
		netradio->SetValue(1);
	else
		customradio->SetValue(1);

    m_pass1 = new BTextControl(BRect(50 + w, 110, 250, 125), "pass1", kPwdStr, "", 0);
    m_pass1->SetDivider(0);
    m_pass1->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);	
	((BTextView *)m_pass1->ChildAt(0))->SetMaxBytes(8);
	m_pass1->SetModificationMessage(new BMessage('pchg'));
    AddChild(m_pass1);
    
	m_pass2 = new BTextControl(BRect(50 + w - 90, 130, 250, 145), "pass2", kConfirmPwdStr, "", 0);
	m_pass2->SetDivider(90);
	m_pass2->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);	
	((BTextView *)m_pass2->ChildAt(0))->SetMaxBytes(8);
	m_pass2->SetModificationMessage(new BMessage('pchg'));
    AddChild(m_pass2);

	tv = (BTextView *)m_pass1->ChildAt(0);
	m_real1 = new BTextView(tv->Frame(), "real1", tv->TextRect(), 
						  tv->ResizingMode(), (tv->Flags() & ~B_NAVIGABLE));
	m_real1->SetMaxBytes(8);
	m_real1->Hide();
	AddChild(m_real1);
	
	tv = (BTextView *)m_pass2->ChildAt(0);
	m_real2 = new BTextView(tv->Frame(), "real2", tv->TextRect(), 
						  tv->ResizingMode(), (tv->Flags() & ~B_NAVIGABLE));
	m_real2->SetMaxBytes(8);
	m_real2->Hide();
	AddChild(m_real2);

	passbutton = new BButton(BRect(260, 127, 300, 147), "pass", "Set", new BMessage('pass'));
	passbutton->SetEnabled(false);
	AddChild(passbutton);

	// bottom separator
	LineView	*bottombar = new LineView(BRect(10, bottom - 60, right - 10, bottom - 40));
	AddChild(bottombar);

	// fade corner
	fadecorner = new CMonitorControl(BRect(right / 2 - 51, bottom - 40, right / 2 - 10, bottom), 
							 "fademonitor", new BMessage('fadc'), -1);
	AddChild(fadecorner);

	BRect monCapFrame;
	BRect monCapArea;
	monCapFrame.Set(10, bottom - 40, right / 2 - 60, bottom);
	monCapArea = monCapFrame;
	monCapArea.OffsetTo(B_ORIGIN);
	cap1 = new BTextView(monCapFrame, B_EMPTY_STRING, monCapArea, 0, B_WILL_DRAW);
	cap1->SetText("Invoke immediately\nif mouse is here");			
	AddChild(cap1);
	cap1->SetViewColor(col);
	cap1->SetAlignment(B_ALIGN_RIGHT);
	cap1->SetDrawingMode(B_OP_OVER);
	cap1->MakeEditable(false);
	cap1->MakeSelectable(false);
	cap1->SetWordWrap(true);

	// no fade corner
	nofadecorner = new CMonitorControl(BRect(right - 51, bottom - 40, right - 10, bottom), 
							 "nofademonitor", new BMessage('!fdc'), -1);
	AddChild(nofadecorner);

	monCapFrame.Set(right / 2 + 10, bottom - 40, right - 60, bottom);
	monCapArea = monCapFrame;
	monCapArea.OffsetTo(B_ORIGIN);
	cap2 = new BTextView(monCapFrame, B_EMPTY_STRING, monCapArea, 0, B_WILL_DRAW);
	cap2->SetText("Do not invoke\nif mouse is here");	
	AddChild(cap2);
	cap2->SetViewColor(col);
	cap2->SetAlignment(B_ALIGN_RIGHT);
	cap2->SetDrawingMode(B_OP_OVER);
	cap2->MakeEditable(false);
	cap2->MakeSelectable(false);
	cap2->SetWordWrap(true);	

	fadecorner->SetOther(nofadecorner);
	nofadecorner->SetOther(fadecorner);

	CheckDependencies();
}

void AdvancedView::MessageReceived(BMessage *msg)
{
	status_t status;

	switch(msg->what)
	{
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

		case 'pchg' :
			// password changed
			passbutton->SetEnabled(true);
			break;

		case MSG_NETRADIO :
		case MSG_CUSTOMRADIO :
			CheckDependencies();
			SaveState();
			break;

		case 'pass' :
			status = netradio->Value() ? B_OK : check_password();
			if(status >= B_NO_ERROR)
			{
				BMessage *m = AcquireSettings();
				if(netradio->Value())
					m->ReplaceString(kLockPassword, "");
				else
				{
					SetPassword(m_real1->Text());
					m->ReplaceString(kLockPassword, m_Password);
				}
				ReleaseSettings();
				SaveSettings();
				passbutton->SetEnabled(false);
			}
			m_pass1->MakeFocus(TRUE);
			break;

		default :
			BView::MessageReceived(msg);
			break;
	}
}

void AdvancedView::AllAttached(void)
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
		dodpms->SetValue((flags & (kDoStandby | kDoSuspend | kDoOff)) ? 1 : 0);

	int32	t = 0, t1;
	m->FindInt32(kTimeFade, &t);
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

void AdvancedView::CheckDependencies()
{
	bool	dpmsena = (BScreen().DPMSCapabilites() & (B_DPMS_STAND_BY | B_DPMS_SUSPEND | B_DPMS_OFF)) ? true : false;
	if(! dpmsena)
		dodpms->SetValue(0);

	dodpms->SetEnabled(dpmsena && masterenable);

	bool	dpmstimeena = dodpms->Value() && masterenable;
	dpms->SetEnabled(dpmstimeena);
	timedpms->SetDrawingMode(dpmstimeena ? B_OP_OVER : B_OP_BLEND);
	timedpms->Invalidate();

	dolock->SetEnabled(masterenable);
	bool	lockena = dolock->Value() && masterenable;
	nolock->SetEnabled(lockena);
	timelock->SetDrawingMode(lockena ? B_OP_OVER : B_OP_BLEND);
	timelock->Invalidate();

	netradio->SetEnabled(lockena);
	customradio->SetEnabled(lockena);

	if(lockena && customradio->Value())
	{
		m_pass1->SetEnabled(true);
		m_pass2->SetEnabled(true);
	}
	else
	{
		m_pass1->SetEnabled(false);
		m_pass2->SetEnabled(false);
		m_pass1->SetModificationMessage(0);
		m_pass1->SetText("");
		m_pass1->SetModificationMessage(new BMessage('pchg'));
		m_real1->SetText("");
		m_pass2->SetModificationMessage(0);
		m_pass2->SetText("");
		m_pass2->SetModificationMessage(new BMessage('pchg'));
		m_real2->SetText("");
		passbutton->SetEnabled(false);
	}

	fadecorner->SetEnabled(masterenable);
	nofadecorner->SetEnabled(masterenable);
	cap1->SetDrawingMode(masterenable ? B_OP_OVER : B_OP_BLEND);
	cap1->Invalidate();
	cap2->SetDrawingMode(masterenable ? B_OP_OVER : B_OP_BLEND);
	cap2->Invalidate();
}

void AdvancedView::SaveState()
{
	BMessage *m = AcquireSettings();

	int32 f;
	int32 t;
	if(m->FindInt32(kTimeFlags, &f) == B_OK &&
		m->FindInt32(kTimeFade, &t) == B_OK)
	{
		int32	mask = (f & ~(kDoStandby | kDoSuspend | kDoOff)) | (dodpms->Value() ? (kDoStandby | kDoSuspend | kDoOff) : 0);
		int32	timediff = (f & kDoFade) ? dpms->Time() - t : dpms->Time();

		if(timediff < 0)
		{
			m->ReplaceInt32(kTimeFade, dpms->Time());
			timediff = 0;
		}

		// to simplify the UI if standby happens after 10 minutes then
		// suspend happens after another 10 minutes and off after 10 more.
		m->ReplaceInt32(kTimeFlags, mask);
		m->ReplaceInt32(kTimeStandby, timediff);
		m->ReplaceInt32(kTimeSuspend, timediff);
		m->ReplaceInt32(kTimeOff, timediff);
	}

	// password method
	m->ReplaceString(kLockMethod, netradio->Value() ? "net" : "custom");

	// corner settings
	m->ReplaceInt32(kCornerNow, fadecorner->Value());
	m->ReplaceInt32(kCornerNever, nofadecorner->Value());

	// lock settings
	m->ReplaceBool(kLockEnable, dolock->Value() ? true : false);
	m->ReplaceInt32(kLockDelay, nolock->Time());

	ReleaseSettings();

	// save to disk
	SaveSettings();
}

void AdvancedView::AttachedToWindow()
{
	// load value of checkbox in the other panel
	BMessage *m = AcquireSettings();
	masterenable = false;
	int32 f;
	if(m->FindInt32(kTimeFlags, &f) == B_OK && (f & kDoFade))
		masterenable = true;
	ReleaseSettings();
	
	dodpms->SetTarget(this);
	dpms->SetTarget(this);
	dolock->SetTarget(this);
	nolock->SetTarget(this);
	fadecorner->SetTarget(this);
	nofadecorner->SetTarget(this);
	passbutton->SetTarget(this);
	netradio->SetTarget(this);
	customradio->SetTarget(this);
	m_pass1->SetTarget(this);
	m_pass2->SetTarget(this);

	filter = new MyFilter(m_pass1, m_pass2, m_real1, m_real2);
	Window()->AddCommonFilter(filter);
}

void AdvancedView::DetachedFromWindow()
{
	// remove message filter
	Window()->RemoveCommonFilter(filter);
}

int AdvancedView::check_password(void)
{
	BAlert *alert;

	if (strcmp(m_real1->Text(), m_real2->Text()) != 0) {
		alert = new BAlert("Password entry error", kNoMatchStr, kOKStr);
		alert->Go();
		return B_ERROR;
	}

	return B_NO_ERROR;
}

char AdvancedView::ranchar(void)
{
	static const char saltmap[] = 
		"abcdefghijklmnopqrstuvwxyz"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789/.";

	return (saltmap[(unsigned)fmod(system_time(), 64.0)]);
}

void AdvancedView::SetPassword(const char *pass)
{
	char salt[3];
	
	if (*pass == 0) {
		*m_Password = 0;
	} else {
		salt[0] = ranchar();
		salt[1] = ranchar();
		salt[2] = 0;
		strcpy(m_Password, crypt(pass, salt));
	}
	return;
}

///////////////////////////////////////////////////////////////

// scan through text for UTF-8 char positions
static void find_utf8_boundaries(const char *text, long *boundary)
{
	long curr = 0;
	const char *p = text;
	boundary[curr++] = 0;
	while(*p)
	{
		if(*p++ & 0x80)
			while((*p++ & 0xe0) == 0x80)
				;
		boundary[curr++] = p - text;
	}
}

MyFilter::MyFilter(BTextControl *p1, BTextControl *p2, BTextView *r1, BTextView *r2)
 : BMessageFilter(B_KEY_DOWN), pass1(p1), pass2(p2), real1(r1), real2(r2)
{
}

filter_result MyFilter::Filter(BMessage *msg, BHandler **)
{ 
	const char *str;
	char buf[5];
	if(msg->FindString("bytes", &str) != B_NO_ERROR) 
		return B_DISPATCH_MESSAGE;
	strcpy(buf, str);

	BView *focus = ((BWindow *)Looper())->CurrentFocus();
	BTextView *pass = 0;
	BTextView *real = 0;

	if(strcmp(str, "\t") == 0 ||
		strcmp(str, "\n") == 0 ||
		strcmp(str, "\r") == 0 ||
		(strlen(str) == 1 && str[0] < ' '))
		return B_DISPATCH_MESSAGE;

	if(focus == pass1->ChildAt(0))
	{
		real = real1;
		pass = (BTextView *)pass1->ChildAt(0);
	}
	else
		if(focus == pass2->ChildAt(0))
		{
			real = real2;
			pass = (BTextView *)pass2->ChildAt(0);
		}

	if(real)
	{
		if(strlen(str) > 1)
		{
			beep();
			return B_SKIP_MESSAGE;
		}

		msg->ReplaceString("bytes", "*");
		msg->RemoveName("byte");
		msg->AddInt8("byte", '*');
		long start;
		long end;
		pass->GetSelection(&start, &end);

		// selecting into the real string is pesky because
		// it might have multibyte chars in it
		const char *text = real->Text();
		long *boundary = new long [strlen(text) + 1];
		find_utf8_boundaries(text, boundary);
		real->Select(boundary[start], boundary[end]);
		delete [] boundary;
		real->KeyDown(buf, strlen(buf));
	}

	return B_DISPATCH_MESSAGE;
}
