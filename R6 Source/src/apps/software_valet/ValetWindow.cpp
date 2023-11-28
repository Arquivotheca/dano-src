#include <ClassInfo.h>
#include <Roster.h>
#include <string.h>
// ValetWindow.cpp

#include "ValetWindow.h"
#include "ValetApp.h"
#include "ValetVersion.h"
#include "GraphicButton.h"

#include "DownloadWindow.h"
#include "PackFilePanel.h"
//#include "PackageWindow.h"
#include "ManagerWindow.h"
#include "SettingsWindow.h"
#include "SettingsManager.h"
#include "HelpFilter.h"

#include "DoIcons.h"


#include "Util.h"
#include "MyDebug.h"

extern 		SettingsManager	*gSettings;

#define CONTEXT_HELP 0
///////////////////////////////////////////////////////////

static BPicture *BitmapToPic(BView *v, BBitmap *b, bool darken = false);

static BPicture *BitmapToPic(BView *v, BBitmap *b, bool darken)
{
	v->BeginPicture(new BPicture());
	v->DrawBitmap(b);
	if (darken) {
		v->SetDrawingMode(B_OP_SUBTRACT);
		v->SetHighColor(60,60,60,255);
		v->FillRect(b->Bounds());
		v->SetDrawingMode(B_OP_COPY);
		v->SetHighColor(0,0,0);
	}
	return v->EndPicture();
}

ValetWindow::ValetWindow()
	:	BWindow(BRect(0,0,100,100),kVersionString,B_TITLED_WINDOW,
					B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	const long	buttonWidth = P_BTN_WIDTH;
	const long	buttonHeight = P_BTN_HEIGHT;
	const long	buttonSep = 0;
	const long	buttonOffset = buttonHeight + buttonSep;
	const long	buttonCount = 5;
	Lock();

	ResizeTo(buttonWidth + 1,buttonOffset*buttonCount + buttonSep + 1 
	
#if CONTEXT_HELP	
	+ 16
#endif	
	);
	
	PositionWindow(this,0.05,0.4);
		
	////////////////////////////////
	
	BRect r = Bounds();
	
	BView *top = new BView(r,"top",B_FOLLOW_ALL,B_WILL_DRAW);
	AddChild(top);
	top->SetViewColor(64,64,64);
	
	GraphicButton::Init(top);
	
	BRect br = Bounds();
	br.InsetBy(1, buttonSep + 1);
	br.bottom = br.top + buttonHeight-1;
	br.right = br.left + buttonWidth-1;

	BView *btn = new BPictureButton(br,B_EMPTY_STRING,BitmapToPic(top,gShopGraphic),
					BitmapToPic(top,gShopGraphic,true),new BMessage(M_DO_SHOP));
					//new GraphicButton(br,"btn",NULL,
					//new BMessage(M_DO_SHOP),gShopGraphic,NULL);
	top->AddChild(btn);
#if CONTEXT_HELP
	btn->AddFilter(new HelpFilter("Go to the BeDepot website..."));
#endif

	BRect frame = btn->Bounds();
#if DEBUG
	frame.PrintToStream();
#endif	

	//// begin inserting repeated buttons
	br.OffsetBy(0,buttonOffset);
	btn = new BPictureButton(br,B_EMPTY_STRING,BitmapToPic(top,gManageGraphic),
					BitmapToPic(top,gManageGraphic,true),new BMessage(M_DO_MANAGE));
	//btn = new GraphicButton(br,"btn",NULL,
	//				new BMessage(M_DO_MANAGE),gManageGraphic,NULL);
	top->AddChild(btn);
#if CONTEXT_HELP
	btn->AddFilter(new HelpFilter("Open the software manager..."));
#endif	

	br.OffsetBy(0,buttonOffset);
	btn = new BPictureButton(br,B_EMPTY_STRING,BitmapToPic(top,gDownloadGraphic),
					BitmapToPic(top,gDownloadGraphic,true),new BMessage(M_DO_DOWNLOAD));
					
	//btn = new GraphicButton(br,"btn",NULL,
	//				new BMessage(M_DO_DOWNLOAD),gDownloadGraphic,NULL);
	top->AddChild(btn);
#if CONTEXT_HELP
	btn->AddFilter(new HelpFilter("Open the installation window..."));
#endif	
	
	br.OffsetBy(0,buttonOffset);
	btn = new BPictureButton(br,B_EMPTY_STRING,BitmapToPic(top,gInstallGraphic),
					BitmapToPic(top,gInstallGraphic,true),new BMessage(M_DO_INSTALL));
	
	//btn = new GraphicButton(br,"btn",NULL,
	//				new BMessage(M_DO_INSTALL),gInstallGraphic,NULL);
	top->AddChild(btn);
#if CONTEXT_HELP
	btn->AddFilter(new HelpFilter("Open the installation window..."));
#endif	
		
	br.OffsetBy(0,buttonOffset);
	btn = new BPictureButton(br,B_EMPTY_STRING,BitmapToPic(top,gPrefsGraphic),
					BitmapToPic(top,gPrefsGraphic,true),new BMessage(M_DO_SETTINGS));
	//btn = new GraphicButton(br,"btn",NULL,
	//				new BMessage(M_DO_SETTINGS),gPrefsGraphic,NULL);
	top->AddChild(btn);
#if CONTEXT_HELP
	btn->AddFilter(new HelpFilter("Configure SoftwareValet..."));
#endif	
	
#if CONTEXT_HELP
	br.OffsetBy(0,buttonOffset);
	br.bottom = br.top + 16;
	BStringView *sv = new HelpStringView(br,"__help_text__",B_EMPTY_STRING);
	top->AddChild(sv);
	sv->SetViewColor(250,250,170);
	sv->SetLowColor(sv->ViewColor());
#endif	
	/*
	br.OffsetBy(0,buttonOffset);
	btn = new GraphicButton(br,B_EMPTY_STRING,NULL,
					new BMessage(M_DO_TASKS),gPrefsGraphic,NULL);
	top->AddChild(btn);
	*/
	Unlock();
	// Show();
}

ValetWindow::~ValetWindow()
{
}

bool TryActivate(BMessenger &mess)
{
	BLooper *loop;
	BWindow *wind;
	if (mess.IsValid()) {
		mess.Target(&loop);
		wind = cast_as(loop,BWindow);
		if (wind->Lock()) {
			wind->Activate(TRUE);
			wind->Unlock();
			return TRUE;
		}
	}
	return FALSE;
}

status_t OpenWebBrowser(const char *arg);

status_t OpenWebBrowser(const char *arg)
{
	BMimeType	mime("text/html");
	
	char browserSig[B_MIME_TYPE_LENGTH];
	mime.GetPreferredApp(browserSig);
	
	const char	*args[1];
	args[0] = arg;
	return be_roster->Launch(browserSig,1,(char **)args);
}

void ValetWindow::MessageReceived(BMessage *m)
{
	switch (m->what) {
		case M_DO_SHOP: {
			const char *serv = gSettings->data.FindString("comm/servername");
			if (serv) {
				const char *kPage = "/valet.asp";
				char *url = (char *)malloc(strlen("http://") + strlen(serv) + strlen(kPage) + 1);
				sprintf(url,"http://%s%s",serv,kPage);
				OpenWebBrowser(url);
				free(url);
			}
			else
			{
				OpenWebBrowser("http://valet.bedepot.com/valet.asp");
			}
			break;
		}
		case M_DO_INSTALL: {
			if (!TryActivate(packWindow)) {
				// packWindow = BMessenger(new PackageWindow());
				BFilePanel *p = new PackFilePanel(new BMessenger(be_app),
									NULL,
									B_REFS_RECEIVED);
				packWindow = BMessenger(p->Window());
				p->Show();
			}
			break;
		}
		case M_DO_DOWNLOAD: {
			if (!TryActivate(downWindow)) {
				downWindow = BMessenger(new DownloadWindow());
				
				((DownloadManager *)(((ValetApp *)be_app)->downloadManager))->statusWindow = downWindow;
			}
			break;
		}
		case M_DO_MANAGE: {
			if (!TryActivate(manaWindow))
				manaWindow = BMessenger(new ManagerWindow(gSettings));
			break;
		}
		case M_DO_SETTINGS: {
			if (!TryActivate(settWindow))
				settWindow = BMessenger(new SettingsWindow(gSettings));
			break;
		}
		default:
//			BWindow::MessageReceieved(m);
			break;
	}
}

bool ValetWindow::QuitRequested()
{
	// always say we can't close
	// the application will decide if it is ok to close us
	be_app->PostMessage(B_QUIT_REQUESTED);
	return false;
}
