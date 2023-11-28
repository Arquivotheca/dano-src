//  PasswordPanel.h
//
//	brad modified by russ 5/21/98
// 
//	(c) 1997-98 Be, Inc. All rights reserved


#include <stdio.h>
#include <string.h>
#include <unistd.h> //crypt

#include <RadioButton.h>
#include <StringView.h>
#include <Alert.h>
#include <Box.h>
#include <Button.h>
#include <MessageFilter.h>
#include <Screen.h>
#include <TextControl.h>
#include <Beep.h>

#include "Settings.h"
#include "ssdefs.h"
#include "PasswordPanel.h"

const char* kNoMatchStr = "Passwords don't match. Try again.";
const char* kEmptyPwdStr = "If you don't want to set the password uncheck the password option in the main window.";
const char* kOKStr = "OK";
const char* kCancelStr = "Cancel";
const char* kDoneStr = "Done";
const char* kConfirmPwdStr = "Confirm password:";
const char* kPwdStr = "Password:";

const int32 kPasswordPanelWidth = 280;
const int32 kPasswordPanelHeight = 149;

#define MSG_DONE				'done'
#define MSG_PASSWORD_TEXT		'ptxt'
#define MSG_CONFIRM_PSWD_TEXT	'conf'
#define MSG_CANCEL				'canc'
#define MSG_NETRADIO			'pnet'
#define MSG_CUSTOMRADIO			'pcst'

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

PasswordPanel::PasswordPanel()
	:BWindow(BRect(0,0,kPasswordPanelWidth, kPasswordPanelHeight),
		"Server Login",  B_MODAL_WINDOW,
		 B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_CLOSABLE)
{
    CreatePasswordUI();

	CenterWindowOnScreen(this);
	Show();	

	Lock();
    m_pass1->MakeFocus(true);
	Unlock();
}

PasswordPanel::~PasswordPanel()
{
}

void 
PasswordPanel::MessageReceived(BMessage *msg) 
{
	status_t status;

	switch (msg->what) {
		case MSG_PASSWORD_TEXT:
		case MSG_CONFIRM_PSWD_TEXT:
			break;
		case MSG_CANCEL:
			PostMessage(B_QUIT_REQUESTED);
			break;

		case MSG_NETRADIO :
		case MSG_CUSTOMRADIO :
			Dependencies();
			break;

		case MSG_DONE:
			status = netradio->Value() ? B_OK : check_password();
			if (status >= B_NO_ERROR) {
				BMessage *m = AcquireSettings();
				m->ReplaceString(kLockMethod, netradio->Value() ? "net" : "custom");
				if(netradio->Value())
					m->ReplaceString(kLockPassword, "");
				else
				{
					SetPassword(m_pass1->Text());
					m->ReplaceString(kLockPassword, m_Password);
				}
				ReleaseSettings();
				SaveSettings();
				PostMessage(B_QUIT_REQUESTED);
			}
			m_pass1->MakeFocus(TRUE);
			break;
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

int
PasswordPanel::check_password(void)
{
	BAlert *alert;

	if (strcmp(m_pass1->Text(), m_pass2->Text()) != 0) {
		alert = new BAlert("", kNoMatchStr, kOKStr);
		alert->Go();
		return (B_ERROR);
	}

#if 0
	if (*m_real1->Text() == 0) {
		alert = new BAlert("", kEmptyPwdStr, kOKStr);
		alert->Go();
		return (B_ERROR);
	}
#endif

	return (B_NO_ERROR);
}

void 
PasswordPanel::CreatePasswordUI()
{
	BMessage *m = AcquireSettings();
	const char *method;
	m->FindString(kLockMethod, &method);
	int32 net = 0;
	if(strcasecmp(method, "net") == 0)
		net = 1;
	ReleaseSettings();


	BRect rect(Bounds());
	rect.InsetBy(-1.0, -1.0);
	m_background = new BBox(rect);
	m_background->SetFont(be_plain_font);
	AddChild(m_background);

	netradio = new BRadioButton(BRect(15, 10, 250, 25), "net", "Use Network password", new BMessage(MSG_NETRADIO));
	m_background->AddChild(netradio);
	custombox = new BBox(BRect(10, 30, Bounds().Width() - 10, 105), "custombox");
	customradio = new BRadioButton(BRect(0, 0, 250, 25), "custom", "Use custom password", new BMessage(MSG_CUSTOMRADIO));
	custombox->SetLabel(customradio);
	m_background->AddChild(custombox);

	if(net)
		netradio->SetValue(1);
	else
		customradio->SetValue(1);

    rect.left = 5;
    rect.right = custombox->Bounds().Width() - 11;
    rect.top = 22; rect.bottom = 13;
    m_pass1 = new BTextControl(rect, "", kPwdStr, "", new BMessage(MSG_PASSWORD_TEXT));
    m_pass1->SetDivider(m_background->StringWidth(kConfirmPwdStr)+10);
    m_pass1->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);	
	((BTextView *)m_pass1->ChildAt(0))->SetMaxBytes(8);
	((BTextView *)m_pass1->ChildAt(0))->HideTyping(true);
    custombox->AddChild(m_pass1);
    
	rect.top = m_pass1->Frame().bottom + 5;
	rect.bottom = rect.top + m_pass1->Frame().Height();
	m_pass2 = new BTextControl(rect, "", kConfirmPwdStr, "", new BMessage(MSG_CONFIRM_PSWD_TEXT));
	m_pass2->SetDivider(m_background->StringWidth(kConfirmPwdStr)+10);
	m_pass2->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);	
	((BTextView *)m_pass2->ChildAt(0))->SetMaxBytes(8);
	((BTextView *)m_pass2->ChildAt(0))->HideTyping(true);
    custombox->AddChild(m_pass2);

	rect = Bounds();
	rect.Set(rect.right - 170, rect.bottom - 30, rect.right - 95, rect.bottom - 10);	
    m_cancelbut = new BButton(rect, "", kCancelStr, new BMessage(MSG_CANCEL));
	m_background->AddChild(m_cancelbut);

	rect = Bounds();
	rect.Set(rect.right - 85, rect.bottom - 30, rect.right - 10, rect.bottom - 10);	
	m_okbut = new BButton(rect, "", kDoneStr, new BMessage(MSG_DONE));
	m_okbut->MakeDefault(true);
	m_background->AddChild(m_okbut);

	Dependencies();
}

void PasswordPanel::Dependencies()
{
	bool ena = netradio->Value() == 0 ? true : false;
	m_pass1->SetEnabled(ena);
	m_pass2->SetEnabled(ena);
}

static char
ranchar(void)
{
	static const char saltmap[] = 
		"abcdefghijklmnopqrstuvwxyz"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789/.";

	return (saltmap[(unsigned)fmod(system_time(), 64.0)]);
}

void
PasswordPanel::SetPassword(const char *pass)
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
