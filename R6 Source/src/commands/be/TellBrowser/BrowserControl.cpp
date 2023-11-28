#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "wait_priv.h"
#include "priv_syscalls.h"
#include <kernel/OS.h>
#include <kernel/image.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Screen.h>
#include <Roster.h>
#include <InterfaceDefs.h>
#include <ResourceCache.h>
#include <StringBuffer.h>
#include <TellBrowser.h>
#include <URL.h>

#include "BrowserWindow.h"
#include "CrashWindow.h"
#include "history.h"
#include "BrowserControl.h"
#include "JapaneseControl.h"


#if DEBUG
#include <Debug.h>

#define printf _sPrintf
#endif

static const char *_resources = "RESOURCES";
static const char *_basepath = "/boot/custom/resources/";

const bool 	kDefaultAccelEnabled = true;
const int32 kDefaultMouseSpeed = 65536;
const int32 kMinMouseSpeed = 65536>>3;
const int32 kMaxMouseSpeed = 65536<<3;
const int32 kDefaultAccelFactor = 65536;
const int32 kMinAccelFactor = 0;
const int32 kMaxAccelFactor = 65536<<4;
const float kMouseSpeedScale = (kMaxMouseSpeed-kMinMouseSpeed);
const float kAccelFactorScale = (kMaxAccelFactor-kMinAccelFactor);

const int32 kDefaultDblClickSpeed = 500000;
const int32 kMinDblClickSpeed = 0;
const int32 kMaxDblClickSpeed = 1000000;

BrowserControl::BrowserControl() :
	BApplication(kTellBrowserSig),
	fLaunchBrowser(true),
	fWatchBrowser(true),
	fBrowser(-1),
	fWatchPort(-1),
	fKernelPort(-1),
	fMonitor(-1),
	fCrashed(false),
	fInitRcvd(false),
	fMode(BC_NORMAL),
	fLastCrashTime(0),
	fStatusWin(NULL),
	fCrashWin(NULL),
	fURL(NULL),
	fBrowserSig(NULL),
	fBrowserPath(NULL),
	fHistory(-1),
	fQuitWindows(true),
	fCrashCursorToken(0),
	fBusyCursorToken(0),
	fCrashQueueToken(0),
	fBusyQueueToken(0)
{
	// open the appropriate history channel
	fBrowserSig = getenv("browser_sig");
	if (fBrowserSig == NULL) {
		fBrowserSig = "application/x-vnd.Web";
	}
//	printf("fBrowserSig: %s\n", fBrowserSig);
	
	fBrowserPath = getenv("browser_path");
	if (fBrowserPath == NULL)
		fBrowserPath = "/boot/beos/apps/Wagner";
//	printf("BrowserPath: %s\n", fBrowserPath);

	fHistory = open("/dev/misc/history_cache", B_READ_WRITE);
	if (fHistory < 0) {
		printf("cannot open history_cache driver!\n");
	}

	InitCursors();
	SetBusyCursor(true);
	
	char *altq = getenv("ALTQ");
	if (altq && atoi(altq) == 0) fQuitWindows = false;

	// Retrieve the current mouse settings.  Ripped from the Mouse
	// preferences panel.
	mouse_map m;
	get_mouse_map(&m);
	fOriginalMouse.map.left = m.left;
	fOriginalMouse.map.right = m.right;
	fOriginalMouse.map.middle = m.middle;
	
	get_click_speed(&fOriginalMouse.click_speed);
	if (fOriginalMouse.click_speed < kMinDblClickSpeed)
		fOriginalMouse.click_speed = kMinDblClickSpeed;
	if (fOriginalMouse.click_speed > kMaxDblClickSpeed)
		fOriginalMouse.click_speed = kMaxDblClickSpeed;

	get_mouse_speed(&fOriginalMouse.accel.speed);
	if (fOriginalMouse.accel.speed < kMinMouseSpeed)
		fOriginalMouse.accel.speed = kMinMouseSpeed;
	if (fOriginalMouse.accel.speed > kMaxMouseSpeed)
		fOriginalMouse.accel.speed = kMaxMouseSpeed;
	
	get_mouse_acceleration(&fOriginalMouse.accel.accel_factor);
	if (fOriginalMouse.accel.accel_factor < kMinAccelFactor)
		fOriginalMouse.accel.accel_factor = kMinAccelFactor;
	if (fOriginalMouse.accel.accel_factor > kMaxAccelFactor)
		fOriginalMouse.accel.accel_factor = kMaxAccelFactor;
	
	fOriginalMouse.accel.enabled = kDefaultAccelEnabled;
	
	get_mouse_type((long*)&fOriginalMouse.type);
	
	fCurrentMouse = fOriginalMouse;
	
	InitWebSettings();
}


BrowserControl::~BrowserControl()
{
	fMode = BC_QUIT;
//	if (be_roster->IsRunning(fBrowserSig))
	if (fMsgr.IsValid())
		SendSimpleMessage(B_QUIT_REQUESTED);
	status_t status = B_OK;
	wait_for_thread(fMonitor, &status);
	
	BMessage *toDel = NULL;
	while ((toDel = fQ.NextMessage())) {
		fQ.RemoveMessage(toDel);
		delete toDel;
	}

	//close the appropriate history channel
	close(fHistory);

	// remove any cursors that may have been set
	SetCrashCursor(false);
	SetBusyCursor(false);
}

bool 
BrowserControl::QuitRequested()
{
	printf("BrowserControl::QuitRequested()\n");
	if (fCrashWin) fCrashWin->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void 
BrowserControl::ReadyToRun()
{
	rgb_color kBlackTransparent = {0, 0, 0, 0};
	BScreen().SetDesktopColor(kBlackTransparent);

	if (fMode == BC_QUIT) {
		PostMessage(B_QUIT_REQUESTED);
		return;
	}
	
	// create our crash window
	fCrashWin = new CrashWindow(fQuitWindows);
	status_t status = B_OK;
	if (fLaunchBrowser) {
		status = StartBrowser();
		if (status != B_ALREADY_RUNNING && status < B_OK) {
			Reboot();
			return;
		}
		// send any queued messages
		BMessage *msg = NULL;
		while ((msg = fQ.NextMessage())) {
			if (fMsgr.IsValid())
				fMsgr.SendMessage(msg);
			delete msg;
			snooze(100000);
		}
	}
}

void 
BrowserControl::ArgvReceived(int32 argc, char **argv)
{
	if (argc < 2)
		PrintHelp();
		
	for (int32 i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "--debug")) {
			fWatchBrowser = false;
		}
		else if (!strcmp(argv[i], "-goto") || !strcmp(argv[i], "-g")) {
			if (!argv[i + 1] || argv[i + 1][0] == '-') {
				printf("-goto needs argument\n");
			}
			else {
				GoTo(argv[++i]);
			}
		}
		else if (!strcmp(argv[i], "-gototop") || !strcmp(argv[i], "-gt")) {
			if (!argv[i + 1] || argv[i + 1][0] == '-') {
				printf("-gototop needs argument\n");
			}
			else {
				GoToTop(argv[++i]);
			}
		}
		else if (!strcmp(argv[i], "-rewind") || !strcmp(argv[i], "-rw")) {
			Rewind();
		}
		else if (!strcmp(argv[i], "-alert")) {
			if (!argv[i + 1] || argv[i + 1][0] == '-') {
				printf("-alert needs argument\n");
			}
			else {
				ShowAlert(argv[++i]);
			}
		}
		else if (!strcmp(argv[i], "-goforward") || !strcmp(argv[i], "-f")) {
			SendSimpleMessage(TB_GO_FORWARD);
		}
		else if (!strcmp(argv[i], "-gobackward") || !strcmp(argv[i], "-b")) {
			SendSimpleMessage(TB_GO_BACKWARD);
		}
		else if (!strcmp(argv[i], "-gohome")) {
			SendSimpleMessage(TB_GO_HOME);
		}
		else if (!strcmp(argv[i], "-gostart")) {
			SendSimpleMessage(TB_GO_START);
		}
		else if (!strcmp(argv[i], "-stop")) {
			SendSimpleMessage(TB_STOP);
		}
		else if (!strcmp(argv[i], "-reload") || !strcmp(argv[i], "-r")) {
			SendSimpleMessage(TB_RELOAD);
		}
		else if (!strcmp(argv[i], "-print")) {
			SendSimpleMessage(TB_PRINT);
		}
		else if (!strcmp(argv[i], "-launch")) {
			Launch();
		}
		else if (!strcmp(argv[i], "-quit")) {
			fMode = BC_BROWSER_DOWN;
			SendSimpleMessage(B_QUIT_REQUESTED);
		}
		else if (!strcmp(argv[i], "-upgrade")) {
			Upgrade(NULL);
		}
		else if (!strcmp(argv[i], "-startbrowser")) {
			status_t status = StartBrowser();
			if (status != B_ALREADY_RUNNING && status < B_OK)
				Reboot();
		}
		else if (!strcmp(argv[i], "-nolaunch")) {
			fLaunchBrowser = false;
			fMode = BC_BROWSER_DOWN;
		}
		else if (!strcmp(argv[i], "-q")) {
			PostMessage(B_QUIT_REQUESTED);
		}
		else if (!strcmp(argv[i], "-openstatus")) {
			if (!argv[i + 1] || argv[i + 1][0] == '-') {
				printf("-openstatus needs argument\n");
			}
			else {
				if (fMode == BC_UPGRADE)
					OpenStatus(argv[++i]);
			}
		}
		else if (!strcmp(argv[i], "-updatestatus")) {
			if (!argv[i + 1] || argv[i + 1][0] == '-') {
				printf("-updatestatus needs argument\n");
			}
			else {
				if (fMode == BC_UPGRADE)
					UpdateStatus(argv[++i]);
			}
		}
		else if (!strcmp(argv[i], "-closestatus")) {
			if (fMode == BC_UPGRADE)
				CloseStatus();
		}
			
		else {
			printf("unknown option: %s\n", argv[i]);
			PrintHelp();
		}
	}
}

void 
BrowserControl::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		// Notified
		case 'init': {
			if (!fCrashed) {
				SetBusyCursor(false);
				// if everything is still kosher complete the boot
				if (be_roster->IsRunning(fBrowserSig)) {
					system("/boot/beos/bin/bootcomplete");
#if _DEVICE_CLIPPER
					system("/boot/beos/bin/cpq_led -sleep -off");
#endif
				}
			}
			
			fCrashWin->PostMessage(HIDE_MESSAGE);
			// remove crash cursor
			SetCrashCursor(false);			

			fInitRcvd = true;
			break;
		}
		
		case TB_OPEN_URL: {
			const char *url = NULL;
			if (msg->FindString("be:url", &url) == B_OK)
				GoTo(url);
			else if (msg->FindString("param", &url) == B_OK)
				GoTo(url);
			break;
		}
		
		case TB_OPEN_ALERT: {
			URL url;
			char hexbuf[sizeof("0x12345678")];
			int32 alertheight = 79;

			if (msg->HasInt32("alertheight")) {
				alertheight = msg->FindInt32("alertheight");
			}

			if (msg->HasString("be:url")) {
				url.SetTo(msg->FindString("be:url"), false);
			}
			else if (msg->HasString("param")) {
				url.SetTo(msg->FindString("param"), false);
			}
			else {
				url.SetTo("file://$SCRIPTS/htmlalertgen", false);

				if (msg->HasString("template")) {
					url.AddQueryParameter("template", msg->FindString("template"));
				}
				if (msg->HasString("itemplate")) {
					url.AddQueryParameter("itemplate", msg->FindString("itemplate"));
				}
				if (msg->HasString("content")) {
					url.AddQueryParameter("content", msg->FindString("content"));
				}
				if (msg->HasInt32("what")) {
					int32 dat = msg->FindInt32("what");
					sprintf(hexbuf, "0x%08lx", dat);
					url.AddQueryParameter("what", hexbuf);
				}
				for (int i=0;i<10;i++) {
					if (!msg->HasString("data", i)) {
						break;
					}
					url.AddQueryParameter("data", msg->FindString("data", i));
				}
				for (int i=0;i<10;i++) {
					char strname[sizeof("ddataN")];
					sprintf(strname, "ddata%d", i);
					if (!msg->HasString(strname)) {
						continue;
					}
					const char* origstr=msg->FindString(strname);
					char* escstr=new char[3*strlen(origstr)+1];
					char* dstchar=escstr;
					while(*origstr!='\0') {
						if (strchr("?=&%",*origstr)==NULL) { // not a magic char
							*(dstchar++)=*(origstr++);
						} else {
							sprintf(dstchar,"%%%02lx",(ulong)(*(origstr++)));
							dstchar+=3;
						}
					}
					*dstchar='\0';
					url.AddQueryParameter(strname, escstr);
					delete[] escstr;
				}
			}

			// keep track of the current message
			BMessage *toQ = DetachCurrentMessage();
			fQ.AddMessage(toQ);

			// add the message pointer to the URL (so we can find which message it was when we get the reply)
			// take off the always-present top bit to avoid signedness issues inside javascript
			sprintf(hexbuf, "0x%08lx", (uint32)toQ & 0x7fffffff);
			url.AddQueryParameter("msgid", hexbuf);

			StringBuffer sb;
			sb << url;
			ShowAlert(sb.String(), alertheight);

			break;
		}
		
		case TB_ALERT_RESPONSE: {
			printf("TB_ALERT_RESPONSE rcvd\n");
			int32 msgid = 0;
			if (msg->FindInt32("msgid", &msgid) != B_OK) {
				printf("no msgid found\n");
				return;
			}
			else printf("msgid: %08lx\n", msgid);
			
			BMessage *orig = (BMessage *)(msgid | 0x80000000);
			if (!fQ.RemoveMessage(orig)) {
				printf("cannot find message %p in fQ\n", orig);
				return;
			}
			else printf("message %p found\n", orig);
			
			BMessage toSend(*msg);
			toSend.RemoveName("msgid");
			
			if ((toSend.what=msg->FindInt32("realwhat"))==0) {
				toSend.what = TB_CMD_REPLY;
			}
			toSend.RemoveName("realwhat");

			if (msg->HasString("gotourl")) {
				GoTo(msg->FindString("gotourl"));
			}
			toSend.RemoveName("gotourl");

			toSend.AddMessage("orig_msg",orig);

			orig->SendReply(&toSend);
			delete orig;
			break;
		}
		
		case TB_GO_FORWARD:
			SendSimpleMessage(TB_GO_FORWARD);
			break;
			
		case TB_GO_BACKWARD:
			SendSimpleMessage(TB_GO_BACKWARD);
			break;
			
		case TB_GO_HOME:
			SendSimpleMessage(TB_GO_HOME);
			break;
			
		case TB_GO_START:
			SendSimpleMessage(TB_GO_START);
			break;
			
		case TB_STOP:
			SendSimpleMessage(TB_STOP);
			break;
		
		case TB_RELOAD:
			SendSimpleMessage(TB_RELOAD);
			break;
			
		case TB_PRINT:
			SendSimpleMessage(TB_PRINT);
			break;
			
		case TB_LAUNCH_BROWSER:
			Launch();
			break;
			
		case TB_QUIT_BROWSER:
			SendSimpleMessage(B_QUIT_REQUESTED);
			break;
			
		case TB_UPGRADE:
			Upgrade(msg);
			break;
		
		case TB_START_BROWSER: {
			status_t status = StartBrowser();
			if (status != B_ALREADY_RUNNING && status < B_OK)
				Reboot();
			break;
		}
		
		case TB_OPEN_STATUS: {
			const char *url;
			if ( (fMode == BC_UPGRADE)&&((msg->FindString("be:url", &url) == B_OK)||(msg->FindString("param", &url) == B_OK)) )
				OpenStatus(url);
			break;
		}
		
		case TB_UPDATE_STATUS: {
			const char *url;
			if ( (fMode == BC_UPGRADE)&&((msg->FindString("be:url", &url) == B_OK)||(msg->FindString("param", &url) == B_OK)) )
				UpdateStatus(url);
			break;
		}
		
		case TB_CLOSE_STATUS: {
			if (fMode == BC_UPGRADE)
				CloseStatus();
			break;
		}

		case TB_SET_MOUSE:{
			float value;
			if (msg->FindFloat("speed", &value) == B_OK) {
				value *= value;
				fCurrentMouse.accel.speed
					= (int32) (kMouseSpeedScale*value + kMinMouseSpeed);
				if (fCurrentMouse.accel.speed < kMinMouseSpeed)
					fCurrentMouse.accel.speed = kMinMouseSpeed;
				if (fCurrentMouse.accel.speed > kMaxMouseSpeed)
					fCurrentMouse.accel.speed = kMaxMouseSpeed;
				set_mouse_speed(fCurrentMouse.accel.speed);
			}
			if (msg->FindFloat("accel", &value) == B_OK) {
				value *= value;
				fCurrentMouse.accel.accel_factor
					= (int32) (kAccelFactorScale*value + kMinAccelFactor);
				if (fCurrentMouse.accel.accel_factor < kMinAccelFactor)
					fCurrentMouse.accel.accel_factor = kMinAccelFactor;
				if (fCurrentMouse.accel.accel_factor > kMaxAccelFactor)
					fCurrentMouse.accel.accel_factor = kMaxAccelFactor;
				set_mouse_acceleration(fCurrentMouse.accel.accel_factor);
			}
			if (msg->FindFloat("save") != 0.0) {
				if (memcmp(&fOriginalMouse, &fCurrentMouse, sizeof(fCurrentMouse)) != 0) {
					
					// Write mouse settings file.  Ripped from Mouse preferences panel.
					BPath path;
					BPoint loc(0,0);
				
					if (find_directory (B_USER_SETTINGS_DIRECTORY, &path, true) == B_OK) {
						long ref;
						
						path.Append (mouse_settings_file);
						if ((ref = creat(path.Path(), O_RDWR)) >= 0) {
							write (ref, &fCurrentMouse, sizeof (mouse_settings));
							write (ref, &loc, sizeof (BPoint));
							close(ref);
							fOriginalMouse = fCurrentMouse;
						}
					}
				}
			}
			break;
		}
		
		case TB_GET_MOUSE:{
			BMessage reply(TB_GET_MOUSE);
			const float speed = sqrtf(((float)(fCurrentMouse.accel.speed-kMinMouseSpeed))
									/ kMouseSpeedScale);
			const float accel = sqrtf(((float)(fCurrentMouse.accel.accel_factor-kMinAccelFactor))
									/ kAccelFactorScale);
			
			reply.AddFloat("speed", speed);
			reply.AddFloat("accel", accel);
			msg->SendReply(&reply);
			break;
		}
		
		case TB_SET_TIMEDATE: {
			float year, month, day, hour, minute;
			time_t now;
			struct tm new_time;
			
			if((msg->FindFloat("year", &year) == B_OK) &&
			   (msg->FindFloat("month", &month) == B_OK) &&
			   (msg->FindFloat("day", &day) == B_OK)){
				   now = time(NULL);
				   new_time = *localtime(&now);
				   
				   new_time.tm_mon = (int)month;
				   new_time.tm_mday = (int)day;
				   new_time.tm_year = (int)year - 1900;
				   
				   now = mktime(&new_time);
				   stime(&now);				
			}
			
			if((msg->FindFloat("hour", &hour) == B_OK) &&
			   (msg->FindFloat("minute", &minute) == B_OK)){
				   now = time(NULL);
				   new_time = *localtime(&now);
				   
				   new_time.tm_hour = (int)hour;
				   new_time.tm_min = (int)minute;
				   
				   now = mktime(&new_time);
				   stime(&now);
			}
			break;
		}
		
#if SUPPORT_JAPANESE
		case TB_GET_JAPANESE:
			{
				BMessage pref(TB_GET_JAPANESE);
				ReadJapaneseSettings(&pref);
				msg->SendReply(&pref);			
			}
			break;

		case TB_SET_JAPANESE:
			WriteJapaneseSettings(msg);
			break;

		case TB_ADD_TO_DICTIONARY:
			AddToJapaneseDictionary(msg);
			break;
#endif

		default:
			BApplication::MessageReceived(msg);
	}
}

int32
BrowserControl::StartMonitoring(void *tellbrowser)
{
	return ((BrowserControl *)tellbrowser)->BrowserRelaunchLoop();
}

status_t
BrowserControl::BrowserRelaunchLoop()
{
	const char * args[10];

	bool restore = false;
	bool goback = false;
	for (;;) {
		int32 count = 0;
		args[count++] = fBrowserPath.String();
		args[count++] = "-full";
		if (restore)
			args[count++] = "-relaunch";
		else
			restore = true;

		if (fURL.Length() > 0) {
			args[count++] = fURL.String();
		}
		
		args[count] = NULL;

		fInitRcvd = false;
		fBrowser = load_image(count, args, const_cast<const char **>(environ));
		if (fBrowser < 0)
			Reboot();

		resume_thread(fBrowser);
		
		int32 reason, result;
		thread_id thid;
		_kwait_for_team_(fBrowser, 0, &thid, &reason, &result);
		if (((reason == B_THREAD_EXITED) && (result != -1)) || (fMode== BC_UPGRADE))
			break;
			
		fCrashed = true;
		bigtime_t crashTime = system_time();
		if (!fInitRcvd || crashTime - fLastCrashTime < 15000000)
			goback = true;
		
		if (fCrashWin->Lock()) {
			if (fCrashWin->IsHidden()) {
				fCrashWin->Show();
				fCrashWin->Sync();
			}

			fCrashWin->Unlock();
		}

		// set crash cursor
		SetCrashCursor(true);

		fLastCrashTime = crashTime;
		
		
		// Wait until psycho killer has fully cleaned up the team to avoid a kernel panic.
		while (find_port("WAGNER_IS_ALIVE") > 0)
			snooze(250000);
		
	}

	if (fMode != BC_UPGRADE)
		PostMessage(B_QUIT_REQUESTED);

	fMonitor = -1;
	return B_OK;
}


status_t
BrowserControl::StartBrowser()
{
	if (fMonitor == -1) {
		fMode = BC_NORMAL;
		fCrashed = false;
		fInitRcvd = false;
		fMonitor = spawn_thread(StartMonitoring, "Relaunch Browser", B_NORMAL_PRIORITY, this);
		resume_thread(fMonitor);
		GetMessenger();
	}
	return B_OK;
}

void 
BrowserControl::Reboot()
{
	system("/boot/beos/bin/shutdown -r -q");
	PostMessage(B_QUIT_REQUESTED);
}


void 
BrowserControl::GetMessenger()
{
	if (fMsgr.IsValid())
		return;
		
	int32 count = 0;
	while (!(fMsgr = BMessenger(fBrowserSig)).IsValid() && count < 50) {
		count++;
		snooze(100000);
	}
	if (!fMsgr.IsValid()) printf("couldn't get valid messenger!\n");
}

void 
BrowserControl::TellBrowser(BMessage *msg)
{
//	printf("BrowserControl::TellBrowser\n");
	if (!msg) return;
	
	if (fMsgr.IsValid()) {
//		printf("fMsgr.IsValid() send message\n");
		fMsgr.SendMessage(msg);
		return;
	}
	
	if (be_roster->IsRunning(fBrowserSig)) {
		fMsgr = BMessenger(fBrowserSig);
		if (fMsgr.IsValid()) {
			fMsgr.SendMessage(msg);
			return;
		}
	}
	
	if (!fMsgr.IsValid()) {
		if (IsLaunching()) {
			// add the message to the queue
			BMessage *toQ = new BMessage(*msg);
			fQ.AddMessage(toQ);
		}
		else {
			GetMessenger();
			
			if (fMsgr.IsValid()) {
				fMsgr.SendMessage(msg);
				return;
			}
		}
	}
	printf("SHOULD NOT GET HERE! -end of BrowserControl::TellBrowser\n");
}

void 
BrowserControl::SendSimpleMessage(uint32 what)
{
	BMessage msg(what);
	TellBrowser(&msg);
}

void 
BrowserControl::GoTo(const char *url, const char * frame)
{
	if (!url)
		return;
		
	if (be_roster->IsRunning(fBrowserSig)) {
		BMessage goTo(TB_OPEN_URL);
		goTo.AddString("be:url", url);
		if (frame)
			goTo.AddString("be:frame", frame);

		GroupID customContentGroupID = securityManager.RegisterGroup("custom_content");
		goTo.AddInt32("be:groupid", customContentGroupID);
		TellBrowser(&goTo);
	}
	else if (IsLaunching()) {
		fURL = url;
	}
}


void 
BrowserControl::GoToTop(const char *url)
{
	printf("BrowserControl::GoToTop(%s)\n", url);
	if (!url) return;
	
	if (be_roster->IsRunning(fBrowserSig)) {
//		BMessage goTo('otop');
//		goTo.AddString("url", url);
		BMessage goTo(TB_OPEN_TOP);
		goTo.AddString("be:url", url);
		TellBrowser(&goTo);
	}
	else if (IsLaunching()) {
		fURL = url;
	}
}

void 
BrowserControl::Rewind()
{
	printf("BrowserControl::Rewind()\n");
	
	if (be_roster->IsRunning(fBrowserSig)) {
		BMessage goTo(TB_REWIND);
		TellBrowser(&goTo);
	}
}


void 
BrowserControl::Launch()
{
	fLaunchBrowser = true;
	status_t status = StartBrowser();
	if (status != B_ALREADY_RUNNING && status < B_OK)
		Reboot();
}

void 
BrowserControl::Upgrade(BMessage */*msg*/)
{
	fMode = BC_UPGRADE;
	SendSimpleMessage(B_QUIT_REQUESTED);
}

void 
BrowserControl::ShowAlert(const char *url,int32 alertheight)
{
	if (!url) return;
	if (fMode != BC_UPGRADE) {
		BMessage alert(TB_OPEN_ALERT);
		alert.AddString("be:url", url);
		alert.AddInt32("alertheight",alertheight);
		TellBrowser(&alert);
	}
	else {
		// we will be displaying this alert
		BrowserWindow *alert = new BrowserWindow(BScreen().Frame(), BS_ALERT, fQuitWindows);
		if (alert) {
			alert->Show();
			alert->OpenURL(Wagner::URL(url), LOAD_ON_ERROR, securityManager.GetGroupID(Wagner::URL(url)));
		}
	}
}

void 
BrowserControl::SetEnv(BMessage *msg)
{
	// set the environment variables
	int ix = 0;
	const char *name = NULL;
	char *temp;
	int32 count = 0;
	type_code found;
	
	while (msg->GetInfo(B_STRING_TYPE, ix, &name, &found, &count) == B_OK)
	{
		const char *env = NULL;
		if (msg->FindString(name, &env) != B_OK)
			continue;
			
		temp = (char*) malloc(strlen(name) + strlen(env) + 2);
		if(temp != NULL){
			sprintf(temp, "%s=%s", name, env);
			printf("putenv(%s)\n", temp);
			putenv(temp);
			free(temp);
		}
		
		if(!strcmp(name,"LANGUAGE")){
			temp = (char*) malloc(strlen(_resources) + strlen(_basepath) + strlen(env) + 2);
			if(temp != NULL){
				sprintf(temp,"%s=%s%s",_resources,_basepath,env);
				printf("putenv(%s)\n",temp);
				putenv(temp);
				free(temp);
			}
		}
		ix++;
	}
	
	if (be_roster->IsRunning(fBrowserSig)) {
		BMessage setEnv(*msg);
		TellBrowser(msg);
	}
}


void 
BrowserControl::OpenStatus(const char *url)
{
	if (fMode != BC_UPGRADE)
		return;
	if (!fStatusWin) {
		fStatusWin = new BrowserWindow(BScreen().Frame(), BS_FULL_SCREEN, fQuitWindows);
		fStatusWin->Show();
	}
	UpdateStatus(url);
}

void 
BrowserControl::UpdateStatus(const char *url)
{
	if (fMode != BC_UPGRADE || !fStatusWin)
		return;
	fStatusWin->OpenURL(url, LOAD_ON_ERROR, securityManager.GetGroupID(url));
}

void 
BrowserControl::CloseStatus()
{
	if (fMode != BC_UPGRADE || !fStatusWin)
		return;
	fStatusWin->PostMessage(B_QUIT_REQUESTED);
	fStatusWin = NULL;
}

void
BrowserControl::InitCursors()
{
	if (fCrashCursorToken == 0) {
		BCursorManager::cursor_data data;
		data.name = "busy_crash_";
		// XXX: hotspotX and hotspotY are reversed to workaround app_server bug
		data.hotspotX = 1;
		data.hotspotY = 6;
		cursorManager.GetCursorToken(&data, &fCrashCursorToken);
	}

	if (fBusyCursorToken == 0) {
		BCursorManager::cursor_data data;
		data.name = "busy_web_";
		// XXX: hotspotX and hotspotY are reversed to workaround app_server bug
		data.hotspotX = 1;
		data.hotspotY = 6;
		cursorManager.GetCursorToken(&data, &fBusyCursorToken);
	}
}

void
BrowserControl::SetCrashCursor(bool crash)
{
	if (crash) {
		// set cursor state to crash if it's not already (and we were able to load the crash cursor)
		if ((fCrashQueueToken == 0) && (fCrashCursorToken != 0)) {
			cursorManager.SetCursor(fCrashCursorToken, 2 /* priority */, &fCrashQueueToken);
		}
	} else {
		// remove crash cursor state if it is set
		if (fCrashQueueToken != 0) {
			cursorManager.RemoveCursor(fCrashQueueToken);
			fCrashQueueToken = 0;
		}
	}
}

void
BrowserControl::SetBusyCursor(bool busy)
{
	if (busy) {
		// set cursor state to busy if it's not already (and we were able to load the busy cursor)
		if ((fBusyQueueToken == 0) && (fBusyCursorToken != 0)) {
			cursorManager.SetCursor(fBusyCursorToken, 2 /* priority */, &fBusyQueueToken);
		}
	} else {
		// remove busy cursor state if it is set
		if (fBusyQueueToken != 0) {
			cursorManager.RemoveCursor(fBusyQueueToken);
			fBusyQueueToken = 0;
		}
	}
}

void 
BrowserControl::PrintHelp()
{
	printf("TellBrowser [options]:\n"
			"\t--debug (allow debugger when crashed)\n"
			"\t-g | -goto URL (open URL as normal page)\n"
			"\t-gt | -gototop URL (open URL as top-level page)\n"
			"\t-alert URL (open URL as alert)\n"
			"\t-f | -goforward\n"
			"\t-b | -gobackward\n"
			"\t-gohome (go to home page)\n"
			"\t-gostart (go to start page)\n"
			"\t-stop\n"
			"\t-r | -reload\n"
			"\t-print\n"
			"\t-launch\n"
			"\t-quit (quits browser)\n"
			"\t-upgrade\n"
			"\t-nolaunch (don't launch the browser)\n"
			"\t-q (quits TellBrowser)\n"
// set/get volume isn't done from the command line because we have snd_vol for that.
		);
	if (IsLaunching()) {
		fMode = BC_QUIT;
		exit (0);
	}
}

int main()
{
	if (getenv("LANGUAGE") == 0){
		putenv("RESOURCES=/boot/custom/resources/en");
		putenv("LANGUAGE=en");
	}
	BrowserControl app;
	app.Run();
	resourceCache.Shutdown();
	return 0;
}
