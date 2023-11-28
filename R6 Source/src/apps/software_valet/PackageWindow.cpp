#include "PackageWindow.h"
#include "InstallListView.h"
#include "SettingsManager.h"
#include "LabelView.h"
#include "PackFilePanel.h"

#include "Util.h"
#include "MyDebug.h"

#include <ScrollView.h>
#include <ClassInfo.h>
#include <Button.h>
#include <Application.h>

extern SettingsManager *gSettings;


PackageWindow::PackageWindow()
	:	BWindow(BRect(0,0,300,280),"Install Packages",
					B_TITLED_WINDOW,
					0 /*NULL*/,0/*NULL*/)
{
	Lock();
	PositionWindow(this,0.4,0.4);
	
	AddChild(new PackageView(Bounds()));
	SetPreferredHandler(FindView("packageview"));
	
	Unlock();
	Show();
}

void PackageWindow::DispatchMessage(BMessage *msg, BHandler *handler)
{
	//if (msg->what == B_REFS_RECEIVED)
	//	handler = FindView("packageview");
	
	BWindow::DispatchMessage(msg,handler);
}

// shouldn't care about these windows
// we should give InstallWindow a static method
// that tries to quit ALL its windows and either succeedds or fails
bool PackageWindow::QuitRequested()
{
	return true;
}


PackageView::PackageView(BRect frame)
	:	BView(frame,"packageview",B_FOLLOW_ALL,B_WILL_DRAW)
{
}

void PackageView::AttachedToWindow()
{
	SetViewColor(light_gray_background);

	BRect r = Bounds();
	r.InsetBy(12,8);
	
	r.bottom = r.top + 42;
	
	char buf[PATH_MAX + 128];
	sprintf(buf,"These are the current packages found in the\
 folder \"%s\". Click open to begin installing the selected package.",
		gSettings->data.FindString("download/path"));
	BView *v = new LabelView(r,buf);
	AddChild(v);
	r.top = r.bottom + 10;
	r.bottom = Bounds().bottom;
	r.bottom -= 48;
	
	//r.bottom -= B_H_SCROLL_BAR_HEIGHT;
	r.right -= B_V_SCROLL_BAR_WIDTH;
	
	InstallListView *il = new InstallListView(r,this);
	
	AddChild(new BScrollView("scroller",
					il,B_FOLLOW_ALL,0,FALSE,TRUE,B_FANCY_BORDER));

	r.right += B_V_SCROLL_BAR_WIDTH;
	r.top = r.bottom + 12;
	r.bottom = r.top + 24;
	r.right -= 4;
	r.left = r.right - 80;
	BButton *btn = new BButton(r,"open","Open",new BMessage('Open'),
					B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	AddChild(btn);
	btn->SetTarget(il);
	btn->MakeDefault(TRUE);
	btn->SetEnabled(false);
	
	r.right = r.left - 20;
	r.left = r.right - 80;
	btn = new BButton(r,"other","Open Other...",new BMessage(B_REFS_RECEIVED),
					B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	AddChild(btn);
	btn->SetTarget(this);
}

void PackageView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case B_REFS_RECEIVED:
		{
			if (!msg->HasRef("refs")) {
				BFilePanel *p = new PackFilePanel(new BMessenger(be_app),
									NULL,
									B_REFS_RECEIVED);
				p->Show();
			}
			// Looper()->DetachCurrentMessage();
			BMessage *newM = new BMessage(*msg);
			msg->what = B_REFS_RECEIVED;
			be_app->PostMessage(msg);
			break;
		}
		case M_ITEMS_SELECTED:
		{
			BControl *c = cast_as(FindView("open"),BControl);
			if (c)
				c->SetEnabled(msg->FindBool("selected"));
			break;
		}
		default:
		{
			BView::MessageReceived(msg);
		}
	}
}
