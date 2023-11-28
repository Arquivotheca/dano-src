#include <Resource.h>
#include <ResourceCache.h>
#include <Protocol.h>
#include <Content.h>
#include <AppFileInfo.h>
#include <WagnerDebug.h>
#include <support/Beep.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Roster.h>
#include <GDispatcher.h>
#include <GHandler.h>
#include <fs_attr.h>
#include <NodeInfo.h>
#include <priv_syscalls.h>
#include "SmooveD.h"
#include "Wotan.h"
#include "TellBrowser.h"
#include "ContentView.h"
#include "mail/PostOffice.h"
#include "ResourceCache.h"
#include "WebBinderNode.h"
#include "PrintEnv.h"

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <math.h>

using namespace Wagner;

Gehnaphore gOpenAccess;
static bool gPageOpening = false;
static bigtime_t gLastIdle = 0;
static int gMainReturnValue = 0;
const bigtime_t kThrobberUpdateLag = 200000;
const char *kMainPage = "file://$RESOURCES/index.html";

const float kMouseScale = 128000.0;

//-------------- PPPListener ------------------------

class PPPListener: public BinderListener
{
public:
							PPPListener(const char *ifName);

private:
	virtual status_t		Overheard(binder_node node,
									  uint32 observed,
									  BString propertyName);

	binder_node				m_interfacesNode;
	BinderNode::property	m_pppProp;
	bool					m_busy;
};

PPPListener::PPPListener(const char *ifName):
	m_busy(false)
{
	//we only want B_PROPERTY_ADDED/REMOVED notifications, but due to the
	//somewhat misleading way Binder.h defines the B_*_CHANGED values
	//and the binder distributes notifications, we can't just use
	//"B_PROPERTY_ADDED | B_PROPERTY_REMOVED", as we'd end up getting
	//every type of notification message - the below explicitly excludes
	//those events we don't care about
	uint32 mask = (B_PROPERTY_ADDED | B_PROPERTY_REMOVED) &
						~(B_VALUES_CHANGED | B_NAMES_CHANGED | B_NAME_KNOWN);

	//we'll watch the list of interfaces, so we'll know when
	//a ppp interfaces is created/destroyed
	m_interfacesNode = BinderNode::Root() / "service" / "network" / "control" / "status" / "interfaces";
	StartListening(m_interfacesNode, mask, ifName);

	//get in sync with the current system state
	Overheard(m_interfacesNode, B_PROPERTY_ADDED, ifName);
}

status_t PPPListener::Overheard(binder_node node,
								uint32 observed,
								BString propertyName)
{
	if (node == m_interfacesNode) {
		m_pppProp = BinderNode::property(m_interfacesNode)[propertyName.String()];
		if (m_pppProp["type"].String() == "ppp") {
			//this is it - start observing (as above, we only want
			//B_PROPERTY_CHANGED messages, and exclude all others)
			uint32 mask = B_PROPERTY_CHANGED &
						~(B_VALUES_CHANGED | B_NAMES_CHANGED | B_NAME_KNOWN);
			StartListening(m_pppProp, mask, "linkstatus");

		} else {
			//this ain't it - do nothing (note that we don't have to
			//explicitly StopListening(), since the node we were listening
			//to was actually deleted)
			m_pppProp.Undefine();
		}
	}

	//the net_node produces tons of gratuitous notifications - only
	//change the cursor when we actually need to
	BString status = m_pppProp["linkstatus"].String();
	bool busyNow = !(m_pppProp.IsUndefined() == true ||
					 status == "connected" ||
					 status == "disconnected");

	if (busyNow != m_busy) {
		Wotan *wotan = dynamic_cast<Wotan*>(be_app);
		if (busyNow == true) {
			wotan->SetPPPCursor(true);
		} else {
			wotan->SetPPPCursor(false);
		}
		//remember new state
		m_busy = busyNow;
	}
}


int main()
{
	// Set this thread's signal mask to ignore SIGPIPEs. This keeps
	// this thread and all the threads that it spawns from dying
	// if there's an error during a write() or send() on a socket:
	signal(SIGPIPE, SIG_IGN);

	Wotan app;

 	// Binder tends to run us out of file descriptors a lot, so this
 	// is a temporary solution to make the machine at least usable.
 	_kset_fd_limit_(1024);
 
 	// This is a hack to determine if this team has had it's address space reclaimed.
 	// The system will run out of memory again if TellBrowser doesn't wait for this to happen
 	// before restarting Wagner (the kernel will panic too).  When performing wait_for_thread,
 	// the waiter wakes up *before* the address space is actually torn down.  Ports, on
 	// the other hand, get deleted after as_close, so checking for the existance of this port
 	// is an accurate way of determining if it is safe to restart the browser.
	create_port(1, "WAGNER_IS_ALIVE");	

	app.Run();

	resourceCache.DumpStats();
	resourceCache.Shutdown();
	return gMainReturnValue;
}

const char *kBookmarkMimeType = 			"application/x-vnd.Be-bookmark";
const char *kDocBookmarkMimeType =			"application/x-vnd.Be-doc_bookmark";



Wotan::Wotan() :
	BApplication("application/x-vnd.Web"),
	fCursorLock("WagnerCursorLock"),
	fPPPListener(NULL),
	fMainWindow(NULL),
	fBrowserWindows(0),
	fConsoleThid(0),
	fBusyCursorToken(0),
	fBusyQueueToken(0),
	fPPPQueueToken(0),
	fRandom(false),
	fFull(false),
	fNotifiedTellBrowser(false),
	fQuitWindows(true),
	fIsRelaunched(false),
	fIsRunning(false),
	fNoCache(false)
{
/*
	BMessage types;
	app_info ai;
	GetAppInfo(&ai);
	BFile f(&ai.ref,O_RDONLY);
	BAppFileInfo info(&f);
	info.GetSupportedTypes(&types);
	if (!types.FindString("types")) {
		types.AddString("types","text/html");
		types.AddString("types",kBookmarkMimeType);
		types.AddString("types",kDocBookmarkMimeType);
		info.SetSupportedTypes(&types,true);
	}
	types.PrintToStream();
*/

	BinderNode::Root()["service"]["web"]["state"]["_launch_url"] = BinderNode::property::undefined;

	char *altq = getenv("ALTQ");
	if (altq && atoi(altq) == 0) fQuitWindows = false;

	be_roster->Launch(SmooveDSignature);
	while (find_thread("bringiton") < 0) snooze(100000);

	fTimer = new Timer();

	fPPPListener = new PPPListener("ppp0");

#if INCLUDE_MAIL_CACHE
	// This will create a new PostOffice and
	// initialize itself.
	PostOffice::MailMan();
#endif

	Resource::SetNotify(Wotan::Idle, Wotan::Busy);
	InitMouseSettings();
	InitWebSettings();
	InitPrinting();
	
	new WebBinderNode();
}

void 
Wotan::ReadyToRun()
{
	InitCursor();
	fIsRunning = true;
	if (fBrowserWindows == 0) {
		//launch a default window
		if (fIsRelaunched)
			GoTo("file://$RESOURCES/recover.html");
		else	
			GoTo(kMainPage);
	}
}

void Wotan::GotoContent(const URL &url, const char *target, int32 groupID)
{
	char buf[1024];
	BMessage request('ourl');	// Notify top content that it should open some page
	url.GetString(buf,sizeof(buf));
	request.AddString("url",buf);
	if (target) request.AddString("target", target);
	if (groupID != -1) request.AddInt32("groupid", groupID);
	if (fMainWindow) fMainWindow->NotifyContent(&request);
}

void 
Wotan::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case TB_OPEN_URL:
		{
			int32 groupID;
			const char *urlString = msg->FindString("be:url");
			const char *target = msg->FindString("be:frame");
			if (msg->FindInt32("be:groupid", &groupID) != B_OK) groupID = -1;
			GotoContent(URL(urlString),target,groupID);
			break;
		}
		
		case TB_OPEN_TOP:
		{
			const char *urlString = msg->FindString("be:url");
			if(!urlString)
				break;
			GoTo(urlString);
			break;
		}
		
		case TB_REWIND:
		{
			if (fMainWindow) {
				fMainWindow->Rewind(); // Clear out history
				BMessage m('rset');	// Clear subwindow & media bar, then go to first page
				fMainWindow->NotifyContent(&m);
			}
			break;
		}
		
		case TB_GO_BACKWARD:
		{
//			printf("Wotan back rcvd\n");
			BMessage m('back');
			if (fMainWindow) fMainWindow->NotifyContent(&m);
			break;
		}

		case TB_GO_FORWARD:
		{
//			printf("Wotan forward rcvd\n");
			BMessage m('forw');
			if (fMainWindow) fMainWindow->NotifyContent(&m);
			break;
		}

		case TB_GO_HOME: {
//			printf("Wotan home rcvd\n");
			if (fMainWindow)
			{
				BMessage m('home');
				fMainWindow->NotifyContent(&m);
			}
			break;
		}
			
		case TB_GO_START: {
//			printf("Wotan TB_GO_START rcvd\n");
			const char *url = NULL;
			if (msg->FindString("be:url", &url) != B_OK)
				url = kMainPage;
			GoTo(url);
		}
				
		case TB_RELOAD:
//			printf("Wotan reload rcvd\n");
			if (fMainWindow) fMainWindow->Reload();
			break;

		case TB_STOP:
//			printf("Wotan stop rcvd\n");
			if (fMainWindow) fMainWindow->StopLoading();
			break;

		
		case TB_PRINT:
//			printf("Wotan print rcvd\n");
			if (fMainWindow) fMainWindow->Print();
			break;
			
		case TB_OPEN_ALERT:
		{
//			printf("Wotan TB_OPEN_ALERT rcvd\n");
			const char * url = msg->FindString("be:url");
			if (url == NULL)
				return;
			int32 alertheight=msg->FindInt32("alertheight");
			alertheight=(alertheight<79)?79:alertheight;
			alertheight=(alertheight>383)?383:alertheight;
			BrowserWindow *alert = new BrowserWindow(BRect(0,48,799,48+alertheight), BS_ALERT, fQuitWindows);

			alert->OpenURL(Wagner::URL(url), LOAD_ON_ERROR, securityManager.GetGroupID(url));
			alert->Show();
			fBrowserWindows++;
			break;
		}
		
		case TB_SET_SETTINGS: {
			set_settings(msg);
			break;
		}
			
		case TB_GET_SETTINGS: {
			BMessage reply(TB_GET_SETTINGS);
			get_settings(msg,&reply);
			msg->SendReply(&reply);
			break;
		}

		case TB_SET_MOUSE:
		{
			float speed;
			if (msg->FindFloat("speed", &speed) == B_OK)
			{
				float scaled = kMouseScale * speed;
				float rem = scaled - floor(scaled);
				int32 newSpeed = int32(scaled);
				// suppress rounding errors
				if(rem > 0.999)
					++newSpeed;

				if(newSpeed == fMouseSettings.accel.speed)
					break;

//				printf("TB_SET_MOUSE: %f -> %ld\n", speed, newSpeed);
				set_mouse_speed(newSpeed);
				fMouseSettings.accel.speed = newSpeed;
				WriteMouseSettings();
			}
			break;
		}
		
		case TB_GET_MOUSE:
		{
			BMessage reply(TB_GET_MOUSE);
			float descaled = float(fMouseSettings.accel.speed) / kMouseScale;
//			printf("TB_GET_MOUSE: %ld -> %f, %f\n", fMouseSettings.accel.speed, descaled, floor(descaled*10));
			reply.AddFloat("speed", descaled);
			msg->SendReply(&reply);
			break;
		}
		
		
		case BW_CLOSED:
		{
//			printf("Wotan BW_CLOSED rcvd\n");
			--fBrowserWindows;
			if(fBrowserWindows < 1)
				PostMessage(B_QUIT_REQUESTED);
			break;
		}

		case 'recl': // Request a relaunch on quit
		{
			gMainReturnValue = -1;
			break;
		}

		case bmsgPageOpening:
		{
			gOpenAccess.Lock();
			bigtime_t time;
			if( msg->FindInt64("time", (int64*)&time) != B_OK ) time = gLastIdle;
			if( time < gLastIdle ) system_beep("Page Loaded");
			else gPageOpening = true;
			gOpenAccess.Unlock();
			break;
		}
		
		case 'invl':		// Invalidate the cache
			resourceCache.InvalidateAll();
			break;

		// Set proxy
		case 'sprx': {
			const char *normalServer;
			int32 normalPort;			
			if (msg->FindString("server", &normalServer) >= 0) {
				if (msg->FindInt32("port", &normalPort) < 0)
					normalPort = 80;
					
				set_proxy_server(normalServer, normalPort);
			}
					
			break;
		}

#if _DEVICE_CLIPPER
		// set Compaq LED
		case 'cpql': {
			const char* led;
			const char* state;
			char command[128];
			if(
				msg->FindString("led", &led) == B_OK &&
				msg->FindString("state", &state) == B_OK) {
				snprintf(command, 128, "/bin/cpq_led -%s -%s\n", led, state);
				system(command);
			}
			break;
		}
#endif

		case 'togk':		// Toggle soft keyboard
			if (fMainWindow) {
				fMainWindow->NotifyContent(msg);
				msg->SendReply('cool');
			}
			
			break;

// All of the messages below, and only those message, were previously
// forwarded on to the content notification process. This was annoying
// because any time a new message to the interface Javascript needed
// to be added, a new message code needed to be added here.  Now all
// messages that are not specifically handled above are passed on to
// the content.
 
//		case 'ourl':		// Open URL
//		case '$KEY':		// Special key message
//		case 'qNCI':		// NetServer 
//		case 'opnw':		// Open window
//		case 'clsw':		// Close window
//		case 'shwk':		// Show soft keyboard
//		case 'hidk':		// Hide soft keyboard
//		case 'shwj':		// Show japanese input method
//		case 'hidj':		// Hide japanese input method
//		case 'otop':
//		case 'back':
//		case 'forw':
		default:
			if (fMainWindow)
				fMainWindow->NotifyContent(msg);

			BApplication::MessageReceived(msg);

			break;
	}
}

#if ENABLE_LOGGING

int32 Wotan::ConsoleThread()
{
	char buf[1024];
	char argv[6][64];
	int32 argc,maxArgs=6;
	int32 bufLen;
	
	printf("\n\nWagner Console \n");
	printf("--> ");

	while (fgets(buf,1023,stdin)) {
		bufLen = strlen(buf);
		if (buf[bufLen-1] == '\n') buf[--bufLen] = 0;
		argc = sscanf(buf,"%62s %62s %62s %62s %62s %62s",argv[0],argv[1],argv[2],argv[3],argv[4],argv[5]);
		for (int32 i=argc;i<maxArgs;i++) argv[i][0] = 0;
		if (!strcmp(argv[0],"trace")) {
			int32 toTrace = atoi(argv[1]);
			int32 traceLevel = atoi(argv[2]);
			if (argc < 3) traceLevel = 1;
			printf("Setting trace level for thread %ld to %ld\n",toTrace,traceLevel);
			wdebug.SetTracing(toTrace,traceLevel);
		};
		printf("--> ");
	};
	return 0;
};

int32 Wotan::_ConsoleThread(void *app)
{
	return ((Wotan*)app)->ConsoleThread();
};

void Wotan::LaunchConsole()
{
	fConsoleThid = spawn_thread(_ConsoleThread,"Console",B_NORMAL_PRIORITY,this);
	resume_thread(fConsoleThid);
};

#endif

void 
Wotan::ArgvReceived(int32 argc, char **argv)
{
	BPoint windowPos(50, 50);
	if (argc > 1)
	{
		int abuse = 0;
		const char *url_file = NULL;
		const char *save_urls = NULL;
		int loop_urls = false;

		for (int ix = 1; ix < argc; ix++)
		{
//			printf("argv[ix]: %s\n", argv[ix]);
			if (strncasecmp(argv[ix], "-abuse", strlen("-abuse")) == 0)
			{
				int alen = strlen("-abuse");
				switch(argv[ix][alen]) {
				case '\0':
					abuse = 1;
					break;
				case '=':
					abuse = atoi(argv[ix]+alen+1);
					if(abuse <= 0) {
						abuse = 1;
					}
					break;
				default:
					// old behavior
					GoTo(argv[ix]);
				}
			}
			else if (strcasecmp(argv[ix], "-no_cache") == 0)
			{
				fNoCache = true;
			}
			else if (ix < argc-1 && strcasecmp(argv[ix], "-url_file") == 0)
			{
				ix++;
				url_file = argv[ix];
				if(!abuse) abuse = 1;
			}
			else if (ix < argc-1 && strcasecmp(argv[ix], "-save_urls") == 0)
			{
				ix++;
				save_urls = argv[ix];
				if(!abuse) abuse = 1;
			}
			else if (strcasecmp(argv[ix], "-loop_urls") == 0)
			{
				loop_urls = true;
			}
			else if (strcasecmp(argv[ix], "-nofull") == 0)
			{
				fFull = false;			
			}
			else if (strcasecmp(argv[ix], "-full") == 0)
			{
				fFull = true;			
			}
			else if	(strcasecmp(argv[ix], "-random") == 0)
			{
				fRandom = true;
			}
			else if (strcasecmp(argv[ix], "-noquitwindows") == 0)
			{
				fQuitWindows = false;
			}
#if ENABLE_LOGGING
			else if (strcasecmp(argv[ix], "-console") == 0)
			{
				LaunchConsole();
			}
#endif
			else if (strcasecmp(argv[ix], "-relaunch") == 0)
			{
				fIsRelaunched = true;
			}
			else if (strcasecmp(argv[ix], "-help") == 0 || strcasecmp(argv[ix], "--help") == 0)
			{
				fprintf(stderr, "Usage: %s [<options>] [URL]\n"
				                "	options:\n"
				                "		-[no]full\n"
				                "		-random\n"
				                "		-noquitwindows\n"
				                "		-console\n"
				                "		-help\n"
				                "\n"
				                "		-abuse[=<n>]       Run in abuse mode with <n> windows\n"
				                "		-url_file <file>   Use the URLs in <file> for abuse\n"
				                "		-save_urls <file>  Append the abused URLs to <file>\n"
				                "		-loop_urls         Loop over the URLs in the url_file\n"
				                "\n"
				                "		      -url_file and -save_urls imply -abuse\n",
								argv[0]);
				exit(0);
			}
			else
				GoTo(argv[ix]);
		}

		if(abuse) {
			printf("\e[41m ABUSE %d \e[0m\n", abuse);
			for (int i = 0; i < abuse; i++) {
				BrowserWindow::abuse_t *at;
				BrowserWindow *window = new BrowserWindow(BRect(windowPos.x, windowPos.y,
					windowPos.x + 799, windowPos.y + 599), BS_NORMAL);
				window->Show();

				at = (BrowserWindow::abuse_t*)malloc(sizeof(BrowserWindow::abuse_t));
				at->window = window;
				at->num_windows = abuse;
				at->url_file = url_file;
				at->save_urls = save_urls;
				at->loop_urls = loop_urls;

				window->StartAbuse(at);
				windowPos.x += 250;
				if (windowPos.x + 250 >= 1280) {
					windowPos.x = 50;
					windowPos.y += 300;
					if (windowPos.y + 300 >= 768)
						windowPos.y = 50;
				}
				fBrowserWindows++;
			}
		}
	}
}

bool Wotan::RefToURL(entry_ref& ref, URL& url)
{
	bool result = false;
	
	BEntry entry(&ref, true);
	if (entry.IsFile()) {	
		// See if the ref is a bookmark.  If so, then get its URL
		// and open it up.
		BNode node(&ref);
		attr_info inf;
		char fileType[B_MIME_TYPE_LENGTH];
		BNodeInfo info(&node);
		if (info.GetType(fileType) == B_OK) {
			if (strcasecmp(fileType, kBookmarkMimeType) == 0 || strcasecmp(fileType, kDocBookmarkMimeType) == 0 ||
			    strcasecmp(fileType, "application/x-person") == 0) {
				if ((node.GetAttrInfo("META:url", &inf) == B_NO_ERROR && inf.type == B_STRING_TYPE && inf.size > 0) ||
					(node.GetAttrInfo("url", &inf) == B_NO_ERROR && inf.type == B_STRING_TYPE && inf.size > 0)) {
					char *str = (char *)malloc(inf.size + 1);
					if (node.ReadAttr("META:url", B_STRING_TYPE, 0, str, inf.size) == inf.size ||
						node.ReadAttr("url", B_STRING_TYPE, 0, str, inf.size) == inf.size) {
						str[inf.size] = 0;
						url.SetTo(str);
						result = true;
					}
					free(str);			
				}
				return result;
			}
		}
		url.SetTo(ref);
		return true;
	}
	return result;
}

void Wotan::RefsReceived(BMessage *message)
{
	entry_ref ref;
	if (message->FindRef("refs", 0, &ref) == B_OK) {
		URL url;
		if (RefToURL(ref,url)) {
			if (fIsRunning)
				GotoContent(url,NULL,securityManager.RegisterGroup("custom_content"));
			else {
				char buf[1024];
				url.GetString(buf,sizeof(buf));
				BinderNode::Root()["service"]["web"]["state"]["_launch_url"] = buf;
			}
		}
	}
}

bool Wotan::QuitRequested()
{
#if INCLUDE_MAIL_CACHE
	PostOffice::GoPostal();
#endif

#if ((PRINTING_FOR_DESKTOP == 0) || (B_BEOS_VERSION >= B_BEOS_VERSION_DANO))
	// Cancel the current printjob synchronousely if any
	TPrintTools::CancelAllJobs(true);
#endif

	return true;
}

atom<ContentInstance> Wotan::GetTopContentInstance()
{
	if (!fMainWindow) return NULL;
	fMainWindow->Lock();
	atom<ContentInstance> c = fMainWindow->GetContentInstance();
	fMainWindow->Unlock();
	return c;
}

ContentView *Wotan::GetContentView()
{
	if (!fMainWindow) return NULL;
	return fMainWindow->GetContentView();
}

status_t Wotan::GoTo(ContentInstance *instance)
{
	if (!fMainWindow) {
		BRect rect(0,0,799,599);
		BPoint windowPos(50, 50);
		rect.OffsetTo(windowPos);
		fMainWindow = new BrowserWindow(rect, (fFull) ? BS_FULL_SCREEN : BS_NORMAL, fQuitWindows);
		fBrowserWindows++;
		fMainWindow->Show();
	}

	return fMainWindow->SetContent(instance);
}

void Wotan::GoTo(const char *url, int32 window)
{
	(void)window;	// squash warning

	if (!url)
		return;
		
	GoTo(URL(url));
}

void Wotan::GoTo(const URL &url)
{
	if (!fMainWindow) {
 		BRect rect(0,0,799,599);
 		BPoint windowPos(50, 50);
		rect.OffsetTo(windowPos);
		fMainWindow = new BrowserWindow(rect, (fFull) ? BS_FULL_SCREEN : BS_NORMAL, fQuitWindows);
		fBrowserWindows++;
		fMainWindow->Show();
	}

	fMainWindow->OpenURL(url, LOAD_ON_ERROR, securityManager.GetGroupID(url));
}

void Wotan::Idle()
{
	gOpenAccess.Lock();
	if( gPageOpening ) system_beep("Page Loaded");
	gPageOpening = false;
	gLastIdle = system_time();
	gOpenAccess.Unlock();
	
	Wotan *wotan = dynamic_cast<Wotan*>(be_app);
	if (!wotan->fNotifiedTellBrowser)
		wotan->fTimer->Start(NotifyTellBrowser, 0, 1000000, 1);
	else {
		BMessage msg('idle');
		wotan->fTimer->Start(UpdateThrobber, &msg, kThrobberUpdateLag, 1);
	}
}

void Wotan::NotifyTellBrowser(const BMessage*)
{
	dynamic_cast<Wotan*>(be_app)->fNotifiedTellBrowser = true;
	BMessenger(kTellBrowserSig).SendMessage('init');
}

void Wotan::Busy()
{
	Wotan *wotan = dynamic_cast<Wotan*>(be_app);
	if (wotan->fNotifiedTellBrowser) {
		BMessage msg('busy');
		wotan->fTimer->Start(UpdateThrobber, &msg, kThrobberUpdateLag, 1);
	}
}

void Wotan::UpdateThrobber(const BMessage *msg)
{
	Wotan *wotan = dynamic_cast<Wotan*>(be_app);
	if (msg->what == 'idle') {
		wotan->SetBusyCursor(false);		
	} else if (msg->what == 'busy') {
		wotan->SetBusyCursor(true);
	}
	
	BMessage msgWrapper('notc');
	msgWrapper.AddMessage("msg", msg);
	dynamic_cast<Wotan*>(be_app)->fMainWindow->PostMessage(&msgWrapper);
}

void 
Wotan::InitMouseSettings()
{
	// fetch current mouse settings from the input server
	mouse_map m;
	get_mouse_map(&m);
	fMouseSettings.map.left = m.left;
	fMouseSettings.map.right = m.right;
	fMouseSettings.map.middle = m.middle;

	get_click_speed(&fMouseSettings.click_speed);
	get_mouse_speed(&fMouseSettings.accel.speed);
	get_mouse_acceleration(&fMouseSettings.accel.accel_factor);
	get_mouse_type((long*)&fMouseSettings.type);
}

void 
Wotan::WriteMouseSettings()
{
	// write mouse settings to file
	BPath path;
	if(find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) == B_OK)
	{
		long ref;
		path.Append(mouse_settings_file);
		if((ref = creat(path.Path(), O_RDWR)) >= 0)
		{
			write(ref, &fMouseSettings, sizeof(fMouseSettings));
			close(ref);
		}
	}
}

void
Wotan::InitCursor()
{
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
Wotan::SetBusyCursor(bool busy)
{
	fCursorLock.Lock();
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
	fCursorLock.Unlock();	
}

void
Wotan::SetPPPCursor(bool connecting)
{
	fCursorLock.Lock();
	if (connecting) {
		// set cursor state to busy if it's not already (and we were able to load the busy cursor)
		if ((fPPPQueueToken == 0) && (fBusyCursorToken != 0)) {
			cursorManager.SetCursor(fBusyCursorToken, 2 /* priority */, &fPPPQueueToken);
		}
	} else {
		// remove busy cursor state if it is set
		if (fPPPQueueToken != 0) {
			cursorManager.RemoveCursor(fPPPQueueToken);
			fPPPQueueToken = 0;
		}
	}
	fCursorLock.Unlock();	
}


void 
Wotan::InitPrinting()
{ // Make sure that all pritners are free
#if ((PRINTING_FOR_DESKTOP == 0) || (B_BEOS_VERSION >= B_BEOS_VERSION_DANO))
	// Cancel the current printjob synchronousely if any
	TPrintTools::FreeAllPrinters();
#endif
}
