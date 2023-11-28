#include "LockWindow.h"
#include "runner_global.h"
#include "ssdefs.h"
#include <Application.h>
#include <Window.h>
#include <Box.h>
#include <TextControl.h>
#include <TextView.h>
#include <Button.h>
#include <MessageFilter.h>
#include <Screen.h>
#include <Beep.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

class BackView : public BView
{
public:
	BackView(BRect frame, const char *name, uint32 resizingMode, uint32 flags)
	 : BView(frame, name, resizingMode, flags | B_FRAME_EVENTS)
	{
	}

	void FrameResized(float, float)
	{
		BView	*v = ChildAt(0);
		if(v)
		{
			BRect bounds = v->Frame();
			float offsx = (Bounds().Width() - bounds.Width()) / 2;
			float offsy = (Bounds().Height() - bounds.Height()) / 2;
			v->MoveTo(offsx, offsy);
		}
	}
};

LockWindow::LockWindow(bool *unlocked)
 : BWindow(BScreen().Frame(), "lockwindow",
    B_BORDERED_WINDOW_LOOK, B_MODAL_ALL_WINDOW_FEEL,
	B_NOT_MOVABLE | B_NOT_H_RESIZABLE | B_NOT_RESIZABLE |
	B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE |
	B_WILL_ACCEPT_FIRST_CLICK,
	B_ALL_WORKSPACES)
{
	unlock = unlocked;

	BRect	r = Bounds();
	BView *black = new BackView(r, "", B_FOLLOW_ALL, 0);
	AddChild(black);
	black->SetViewColor(0, 0, 0, 255);

	float offsx = (r.Width() - 270) / 2;
	float offsy = (r.Height() - 120) / 2;
	r.Set(offsx, offsy, offsx + 270, offsy + 120);

	BView *gray = new BView(r, "", 0, 0);
	gray->SetViewColor(216, 216, 216, 255);
	black->AddChild(gray);

	BBox *back = new BBox(BRect(10, 10, 260, 110));
	back->SetLabel("Unlock screen saver");
	back->SetBorder(B_FANCY_BORDER);
	back->SetFont(be_plain_font);
	gray->AddChild(back);
	back->SetViewColor(216, 216, 216, 255);
	back->SetLowColor(255, 255, 255, 255);
	back->SetHighColor(0, 0, 0, 255);

	const char *conf = "Enter password:";

    pass = new BTextControl(BRect(10, 20, 235, 35), "", conf, "", new BMessage('pass'));
    pass->SetDivider(back->StringWidth(conf)+10);
    pass->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);	
	((BTextView *)pass->ChildAt(0))->SetMaxBytes(8);
	((BTextView *)pass->ChildAt(0))->HideTyping(true);
    back->AddChild(pass);

	BRect rect = back->Bounds();
	rect.Set(rect.right - 95, rect.bottom - 40, rect.right - 20, rect.bottom - 20);	
	ok = new BButton(rect, "", "Unlock", new BMessage('doit'));
	ok->MakeDefault(true);
	back->AddChild(ok);

	// fetch password
	const char *method;
	global_settings.FindString(kLockMethod, &method);
	net = 0;
	if(strcasecmp(method, "net") == 0)
		net = 1;

	Show();	
	Activate();

	Lock();
    pass->MakeFocus(true);
	Unlock();
}

void LockWindow::ScreenChanged(BRect frame, color_space)
{
	MoveTo(frame.LeftTop());
	ResizeTo(frame.Width(), frame.Height());
}

void LockWindow::MessageReceived(BMessage *msg)
{

	switch(msg->what)
	{
		case 'pass' :
			break;

		case 'doit' :
			{
			struct passwd *pw;
			bool ok = false;
			if(net)
			{
				if((pw = getpwuid(getuid())) &&
					strcmp(pw->pw_passwd, crypt(pass->Text(),pw->pw_passwd)) == 0)
					ok = true;
			}
			else
			{
				const char *s;
				if(global_settings.FindString(kLockPassword, &s) == B_OK &&
					strcmp(s, crypt(pass->Text(), s)) == 0)
					ok = true;
			}

			if(ok)
			{
				*unlock = true;
				be_app->PostMessage(B_QUIT_REQUESTED);
			}
			else
			{
				beep();
				pass->SetText("");
			}
			}
			break;

		default :
			BWindow::MessageReceived(msg);
			break;
	}
}
