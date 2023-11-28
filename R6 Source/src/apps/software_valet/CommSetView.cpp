// CommSetView.cpp

#include "SettingsWindow.h"
#include "SettingsManager.h"
#include "Util.h"
#include "LabelView.h"

#include <stdlib.h>
#include <Resources.h>
#include <Application.h>
#include <Path.h>
#include <FindDirectory.h>
#include <TextControl.h>
#include <StringView.h>
#include <Button.h>
#include <CheckBox.h>
#include <string.h>
#include <Roster.h>
#include <Alert.h>

#include "MyDebug.h"

static const char *kCommSetViewHelpString =
	"SoftwareValet uses HTTP to communicate with BeDepot.  These settings "
	"configure SoftwareValet to communicate with BeDepot using an HTTP proxy.";
	
status_t CheckLaunchListener(bool = false);

/////////////////////// Communication Settings /////////////////////

CommSetView::CommSetView(BRect fr)
	:	ResizeView(fr,"Communication",B_FOLLOW_ALL, 0 /*B_WILL_DRAW | B_PULSE_NEEDED*/)
{
	SetViewColor(light_gray_background);
	fDaemonUp = false;
}

void CommSetView::AttachedToWindow()
{
	BView::AttachedToWindow();

	BRect bounds(Bounds());
	bounds.InsetBy(5,12);
	BRect r(bounds);
	
	r.bottom = r.top + 16;
	r.right = r.left + 240;
	BTextControl *tc = new BTextControl(r,"proxy","Use Proxy Server:",
							gSettings->data.FindString("comm/proxy"),
							new BMessage(S_SET_PROXY));
	AddChild(tc);
	tc->SetDivider(tc->Divider() - 20);
	tc->SetTarget(this);
	
	r.left = r.right + 8;
	r.right = r.left + 80;
	tc = new BTextControl(r,"proxyport","Port:",
						B_EMPTY_STRING,
						new BMessage(S_SET_PROXYPORT));
	AddChild(tc);
	tc->SetTarget(this);

	char pt[12];
	sprintf(pt,"%ld",gSettings->data.FindInt32("comm/proxyport"));
	tc->SetText(pt);	

	r.top = r.bottom + 12;
	r.bottom = r.top + 42;
	r.left = bounds.left;
	r.right = bounds.right;
	
	LabelView *lv = new LabelView(r, kCommSetViewHelpString);
	AddChild(lv);
	
	///////// Listener start/restart

	// every second
//	Window()->SetPulseRate(1000*1000);
//
//	r.OffsetBy(0,32);
//	r.left = Bounds().left + 5;
//	r.right = Bounds().right - 5;
//	
//	r.bottom = r.top + 14;
//
//	BStringView *sv = new BStringView(r,"listenerstat",B_EMPTY_STRING);
//	AddChild(sv);
//	sv->SetViewColor(ViewColor());
//	
//	r.left += 20;
//	r.top = r.bottom + 8;
//	r.bottom = r.top + 20;
//	r.right = r.left + 90;
//	BButton *btn = new BButton(r,"stopbtn",
//				"Stop",new BMessage(LISTENER_STOP));
//	AddChild(btn);
//	btn->SetFont(be_plain_font);
//	btn->SetTarget(this);
//	
//	BRect br = r;
//	br.OffsetBy(100,0);
//	btn = new BButton(br,"restart",
//				"Start",new BMessage(LISTENER_RESTART));
//	AddChild(btn);
//	btn->SetFont(be_plain_font);
//	btn->SetTarget(this);
//	
//	////////////////////
//	
//	r.top = r.bottom + 10;
//	r.bottom = r.top + 14;
//	r.right = r.left + Bounds().right - 5;
//	
//	BCheckBox *cb = new BCheckBox(r,"listenerstart",
//							"Launch SoftwareValet Transciever at boot",
//							new BMessage(LISTENER_AUTO));
//	AddChild(cb);
//	cb->SetValue(gSettings->data.FindBool("comm/listenerauto"));
//	cb->SetFont(be_plain_font);
//	cb->SetTarget(this);
//	
//	////////////////////
	winHeight = r.bottom + 50;
	
//	ListenerStatus(true);
}

//void CommSetView::Draw(BRect r)
//{
//	BView::Draw(r);
//	BRect b = Bounds();
//	BRect f = FindView("listenerstat")->Frame();
//	DrawHSeparator(b.left + 4,b.right - 4 ,f.top - 4,this);
	
	//f = FindView("proxy")->Frame();
	//DrawHSeparator(b.left + 4,b.right - 4 ,f.top - 8,this);
//}

//void CommSetView::ListenerStatus(bool running)
//{
//	if (running == fDaemonUp)
//		return;
//		
//	BStringView *v = cast_as(FindView("listenerstat"),BStringView);
//	ASSERT(v);
//	char buf[80] = "SoftwareValet Transciever is currently: ";
//	if (running) {
//		strcat(buf,"Running");
//	}
//	else
//		strcat(buf,"Not Running");
//	v->SetText(buf);
//	
//	fDaemonUp = running;
//}

//void CommSetView::Pulse()
//{
//	BView::Pulse();
////	ListenerStatus(be_roster->IsRunning(kListenerSig));
//}

//status_t	RunScript(const char *script, const char *arg1, const char *arg2);
//void		SetTransceiverAuto(bool autoStart);

void CommSetView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
#if 0
		case LISTENER_STOP: {
			bool kill;		
			key_info	inf;
			get_key_info(&inf);
			kill = (inf.modifiers & B_CONTROL_KEY || 
					inf.modifiers & B_OPTION_KEY);
					
			BAlert	*a = new BAlert(B_EMPTY_STRING,"SoftwareValet Transceiver \
must be running to to perform scheduled update checking. Are you sure you want to stop it?","No","Yes");	
 			PositionWindow(a,0.5,0.4);
 			int result = a->Go();
 			if (result) {				
				if (kill) {
					team_id id = be_roster->TeamFor(kListenerSig);
					PRINT(("killing team %d\n",id));
					kill_team(id);
				}
				else {
					BMessenger lm(kListenerSig);
					lm.SendMessage(B_QUIT_REQUESTED);
				}
			}
			break;
		}
		case LISTENER_RESTART: {
			CheckLaunchListener(true);
			break;
		}
		case LISTENER_AUTO: {
			bool autoStart = GetBoolControlValue(msg);
			SetTransceiverAuto(autoStart);
			gSettings->data.ReplaceBool("comm/listenerauto",autoStart);
			break;
		}
#endif
		case S_SET_PROXYPORT: {
			BTextControl *v;
			msg->FindPointer("source",(void **)&v);
			if (v)			
				gSettings->data.ReplaceInt32("comm/proxyport",atol(v->Text()));
			break;
		}
		case S_SET_PROXY: {
			BTextControl *v;
			msg->FindPointer("source",(void **)&v);
			if (v)			
				gSettings->data.ReplaceString("comm/proxy",v->Text());
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}

#if 0
void SetTransceiverAuto(bool autoStart)
{
	entry_ref	ref;
	if (be_roster->FindApp(kListenerSig,&ref) < B_OK) {
		doError("Error getting Transciever application info. \
		Could not setup UserBootscript. Make sure SoftwareValet Transceiver is properly installed.");
		return;
	}
	BPath	p1;
	BPath	p2;
	BEntry	ent(&ref);
	ent.GetPath(&p1);
	
	find_directory(B_USER_BOOT_DIRECTORY,&p2,false);
	
	RunScript("clean.sh",p1.Path(),p2.Path());
	
	if (autoStart)
	{
		RunScript("setup.sh",p1.Path(),p2.Path());					
	}
}

status_t RunScript(const char *script, const char *arg1, const char *arg2)
{
	status_t	err;
	BPath		p;
	find_directory(B_COMMON_TEMP_DIRECTORY,&p,true);
	char buf[40];
	sprintf(buf,"setupscript%d.sh",find_thread(0));
	p.Append(buf);
	
	BFile		tmpFile(p.Path(),O_RDWR | O_CREAT);
	err = tmpFile.InitCheck();
	if (err < B_NO_ERROR)
		return err;
		
	app_info	info;
	be_app->GetAppInfo(&info);
	BFile		appFile(&info.ref,O_RDONLY);
	BResources	appRes(&appFile);
	
	size_t		sz;
	void *data = appRes.FindResource('Scpt', script, &sz);
	if (!data)
		return -1;
		
	tmpFile.Write(data,sz);
	free(data);
	
	char	*cmd = (char *)malloc(strlen(p.Path()) + 2 + strlen(arg1) + 3 + strlen(arg2) + 2);
	sprintf(cmd,"%s \"%s\" \"%s\"",p.Path(),arg1,arg2);

	// MGM DEBUG!  REMOVE ME!
	printf("running %s\n", cmd);
	
	PRINT(("running %s\n",cmd));
		
	system(cmd);
	free(cmd);
	
	tmpFile.Unset();
	BEntry	ent(p.Path());
	ent.Remove();
	
	return B_OK;	
}
#endif
