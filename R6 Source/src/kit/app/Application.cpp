//******************************************************************************
//
//	File:			Application.cpp
//
//	Description:	BApplication class. The top level class from which
//					all Be applications must be derived.
//
//	Written by:		Peter Potrebic, Benoit Schillings
//
//	Revision history
//	JAN 96	pjp		-------- LOCKING issue --------
//			I've added a special object (_BAppServerLink_) to safely
//			access the server session, for communication with the app_server.
//			This object should be used to synchronize access by multiple
//			threads. Previously the code was just using the standard
//			BApp::Lock() method, but this could easily lead to dead locks.
//
//			Another problem is the other 'data' managed by the app object.
//			Currently this includes cursor data and the 'Main' menu. Access
//			to that data needs to be multi-thread safe. If we just used the
//			std BApp::Lock method we could have deadlock -- The app thread
//			responds to the ABOUT_REQUESTED message and opens the About
//			Panel. The app_thread has the app_object locked and it is
//			synchronously waiting for the About Panel to be dismissed. The
//			code for the About Panel could try to access the cursor data,
//			causing the AboutPanel thread to block on the app_object semaphore.
//			Deadlock! One might argue that the application thread shouldn't
//			syncronously wait for the About Panel, and that's probably true,
//			but making developers think about this is too much to ask. The
//			system can handle the problem by never trying to lock the 
//			app_object except from the app_thread itself. If any other data
//			is maintained by the app_object then another semaphore should
//			be used to provide safe access. Currently I'm just going to
//			use the same BAppServerLink semaphore, just for convience.
//
//	JUNE 94 bgs	- reworked large function for the confort of my eyes.
//	08FEB94 tdl	- Fixed PostEvent to delete the event regardless of transmission,
//			also delete shared memory if transmit failed (per Steve)
//	28JAN94 tdl	- Changed flush_task to send a 'FLSH' message to the window
//			instead of calling Flush() directly. The problem is that
//			Flush() does a window lock, and other threads may have the
//			window locked and be waiting on the application  lock,
//			(to close the widow, etc.), which causes a deadlock.
//			- Changed BApplication::ScriptEvent to make sure a window ptr
//			can't be deleted while it is in use for a script event
//			- Added locks to protect scans of the application's window list
//			(to CloseAllWindows() and CountWindows()).
//	26JAN94 tdl	Made sig for introduction message 'INTR'
//
//	Copyright 1992-95, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#ifndef _DEBUG_H
#include <Debug.h>
#endif

#include <TLS.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#ifndef _STDLIB_H
#include <stdlib.h>
#endif
#ifndef _STRING_H
#include <string.h>
#endif

#ifndef _PATH_H
#include <Path.h>
#endif
#ifndef _ALIAS_H
#include <Alias.h>
#endif
#ifndef _OS_H
#include <OS.h>
#endif
#ifndef _IMAGE_H
#include <image.h>
#endif

#ifndef _ENTRY_H
#include <Entry.h>
#endif
#ifndef _VOLUME_H
#include <Volume.h>
#endif
#ifndef _DIRECTORY_H
#include <Directory.h>
#endif
#ifndef _FILE_H
#include <File.h>
#endif
#if !defined(_RESOURCES_H)
 #include <Resources.h>
#endif /* _RESOURCES_H */
#if !defined(_AUTOLOCK_H)
 #include <Autolock.h>
#endif /* _AUTOLOCK_H */

#ifndef _MESSAGES_H
#include <messages.h>
#endif
#ifndef _TOKEN_SPACE_H
#include <token.h>
#endif
#ifndef _LIST_H
#include <List.h>
#endif
#ifndef _SESSION_H
#include <session.h>
#endif
#ifndef _CONNECTION_H
#include "connection.h"
#endif
#ifndef _APPLICATION_H
#include "Application.h"
#endif
#ifndef _WINDOW_H
#include <Window.h>
#endif
#ifndef _MENU_WINDOW_H
#include <MenuWindow.h>
#endif
#ifndef	_INTERFACE_DEFS_H
#include <InterfaceDefs.h>
#endif
#ifndef _ALERT_H
#include <Alert.h>
#endif
#ifndef _VIEW_H
#include <View.h>
#endif
#ifndef _APP_DEFS_PRIVATE_H
#include <AppDefsPrivate.h>
#endif
#ifndef _MESSAGE_H
#include <Message.h>
#endif
#ifndef _MESSAGE_UTIL_H
#include <message_util.h>
#endif
#ifndef _ROSTER_PRIVATE_H
#include <roster_private.h>
#endif
#ifndef _INTERFACE_MISC_H
#include <interface_misc.h>
#endif
#ifndef _CLIPBOARD_H
#include <Clipboard.h>
#endif
#ifndef _STORAGE_DEFS_H
#include <StorageDefs.h>
#endif
#ifndef _MENU_ITEM_H
#include <MenuItem.h>
#endif
#ifndef _POP_UP_MENU_H
#include <PopUpMenu.h>
#endif
#ifndef _MESSAGE_FILTER_H
#include <MessageFilter.h>
#endif
#ifndef _MIME_H
#include <Mime.h>
#endif
#ifndef _SHARED_FONTS_H
#include <shared_fonts.h>
#endif
#ifndef _APP_FILE_INFO_H
#include <AppFileInfo.h>
#endif
#ifndef _MESSAGE_RUNNER_H
#include <MessageRunner.h>
#endif
#include <private/storage/mime_private.h>
#include <PrivateScreen.h>
#include <PropertyInfo.h>
#include <priv_runtime.h>
#include <archive_defs.h>
#include <Cursor.h>
#include <shared_defs.h>

#ifdef REMOTE_DISPLAY
#include "TCPIPSocket.h"
#endif

#include <StreamIO.h>

/*---------------------------------------------------------------*/

// Globals (one of each per application)
BApplication	*be_app = NULL;
BMessenger		be_app_messenger;

// This will have to be changed to a synched_screen_pool or
// something for multiple-monitor DR9.
BPrivateScreen			*_the_screen_;

//+int32 BApplication::sPulseEnabledCount = 0;

BResources * BApplication::_app_resources;
BLocker BApplication::_app_resources_lock("_app_resources_lock");

BCursor cursorSystemDefault((const void*)NULL);
BCursor cursorIBeam((const void*)NULL);

const BCursor *B_CURSOR_SYSTEM_DEFAULT = &cursorSystemDefault;
const BCursor *B_CURSOR_I_BEAM = &cursorIBeam;

static BList callbacklist;

/*---------------------------------------------------------------*/
// ## TEMPORARY UNTIL WE GET DB_REFS FROM FILE OBJECT
extern "C" void heap_browser_register();

/*---------------------------------------------------------------*/

status_t _pulse_task_(void *arg);
void _run_task();

/* ---------------------------------------------------------------------
 The following class/object is used to handle any runtime initialization
 that needs to occur before main() is run. The static object below
 will get constructed before main() allowing us to set things up.
*/

class _BAppCleanup_ {
public:
				_BAppCleanup_();
				~_BAppCleanup_();

	bool		fIsBeApp;
	team_id		fTeam;
};

static BMessage	*ParseArguments();

/*---------------------------------------------------------------*/

static _BAppCleanup_	_app_cleanup_object_;

int32 send_msg_proc_TLS = -1;
static int32		main_session_created = 0;
static AppSession	*main_session = NULL;

/*---------------------------------------------------------------*/

struct _server_heap_ {
	area_id rwArea, rwClone;
	area_id roArea, roClone;
	area_id globalROArea, globalROClone;
	void *rwAddress,*roAddress,*globalROAddress;
};

/*---------------------------------------------------------------*/

extern "C" {
void	initialize_before();
void	terminate_after();
//int	_init_shared_heap_();
int		_init_roster_();
int		_init_message_();
int		_delete_message_();
}

void initialize_before(void)
{
	/* initialize the Be world */
//	_init_shared_heap_();
	send_msg_proc_TLS = tls_allocate();
	_init_roster_();
	_init_tokens_();
	_init_message_();
}

void terminate_after(void)
{
	/* tear down the Be world */
	_msg_cache_cleanup_();
	_delete_message_();
	_delete_tokens_();
}

/*---------------------------------------------------------------*/

void __set_window_decor(int32 /*theme*/)
{
	// This has been superceeded by the new decor API, but still
	// exists because the Deskbar links against it.
#if 0
	_BAppServerLink_ link;
	link.session->swrite_l(GR_SET_WINDOW_DECOR);
	link.session->swrite_l(theme);
	link.session->flush();
#endif
}

extern port_id _get_looper_port_(const BLooper *loop);

#ifdef REMOTE_DISPLAY
int32 EventPiper(TCPIPSocket *socket)
{
	BMessage m;
	BApplication *app = be_app;
	int32 tag,rcv;
	int32 size;
	char buffer[4096];
	char *buf = buffer;
	int32 bufSize = 4096;
	while (socket->ReceiveWait(&tag,4) == 0) {
		size = 0;
		socket->ReceiveWait(&size,4);
		if (size > 0) {
			if (size > bufSize) {
				if (buf != buffer) free(buf);
				buf = (char*)malloc(size);
				bufSize = size;
			};
			socket->ReceiveWait(buf,size);
			if (tag == -1) {
				m.Unflatten(buf);
				app->PostMessage(&m);
			} else {
				int32 count = app->CountWindows();
				for (int32 i=0;i<count;i++) {
					int32 port = _get_looper_port_(app->WindowAt(i));
					if (port == tag) {
						write_port(port,APP_SERVER_MSG_CODE,buf,size);
					};
				};
			};
		};
	};
	delete socket;
};
#endif

void BApplication::connect_to_app_server()
{
	if (atomic_or(&main_session_created, 1) != 0) {
		// wait for someone else to create the session
		while (main_session_created != 2) sleep(20000);
		return;
	}
	
	int32	send_port,receive_port;
	message	a_message;
	char *asName = getenv("APP_SERVER_NAME");
	if (!asName) asName = "picasso";

#ifdef REMOTE_DISPLAY
	if (strncasecmp(asName,"tcp:",4) == 0) {
		/*	We're connecting to a remote app_server */
		char *p,*d,host[80];
		int16 port = 666;
		d = host;
		p = asName+4;
		while (*p && (*p != ':')) *d++ = *p++;
		*d = 0;
		if (*p == ':') port = atoi(p+1);
		
		int32 buffer[2];
		buffer[1] = Team();
		TCPIPSocket *appSocket = new TCPIPSocket();
		appSocket->Open(host,port);
		buffer[0] = RMT_APP_SOCKET;
		appSocket->Send(buffer,8);

		main_session = new AppSession(appSocket);

		TCPIPSocket *appEventSocket = new TCPIPSocket();
		appEventSocket->Open(host,port);
		buffer[0] = RMT_APP_EVENT_SOCKET;
		appEventSocket->Send(buffer,8);
		resume_thread(spawn_thread((thread_entry)EventPiper,"EventPiper",
			B_URGENT_DISPLAY_PRIORITY,appEventSocket));
	} else
#endif
	{
		get_server_port(&fServerFrom, &fServerTo, asName);
		receive_port = create_server_port("rcv");
		send_port = create_server_port("snd");
		a_message.what = GR_NEW_APP;
		a_message.parm1 = send_port;
		a_message.parm2 = receive_port;
		a_message.parm3 = fMsgPort;
		a_message.parm4 = Team();
		while (write_port(fServerTo, 0, (char*)&a_message, 64) == B_INTERRUPTED)
			;
		main_session = new AppSession(send_port, receive_port);
	
		// setup our side of the shared client/server heap
		setup_server_heaps();
	};

	// get a pointer to the server color space if possible
	get_scs();

	// setup the drag message semaphore and associated info
	fDraggedMessage = new _drag_data_;
	fDraggedMessage->serverArea = B_BAD_VALUE;
	fDraggedMessage->area = B_BAD_VALUE;
	fDraggedMessage->areaBase = 0;
	fDraggedMessage->timestamp = 0;
	fDraggedMessage->sem = create_sem(100000,"dragMessageSem");

	// init global fonts definitions and cache	
	_init_global_fonts_();
	_init_interface_kit_();
	
	// Give the app_server our icon
	{
		BBitmap *icon = new BBitmap(BRect(0,0,15,15),B_CMAP8,false);
		memset(icon->Bits(),0xFF,256);
		app_info ai; 
		BFile file; 
		BAppFileInfo afi; 
	   
		be_app->GetAppInfo(&ai); 
		file.SetTo(&ai.ref, B_READ_WRITE); 
		afi.SetTo(&file);
		afi.GetTrackerIcon(icon,B_MINI_ICON);
		
		_BAppServerLink_ link;
		link.session->swrite_l(GR_SET_MINI_ICON);
		link.session->swrite(4,&icon->fServerToken);
		link.session->flush();
		
		delete icon;
	}
	
	main_session_created = 2;
}

/*---------------------------------------------------------------*/

void 
_BAppServerLink_::Init(bool assert_session_exists)
{
	if (be_app) {
		if (!main_session) be_app->connect_to_app_server();
		if (main_session->lock()) {
			session = main_session;
		} else {
			session = NULL;
		}
	} else if (assert_session_exists) {
		debugger("You need a valid BApplication object before interacting with the app_server");
		session = NULL;
	} else {
		session = NULL;
	}
}

_BAppServerLink_::_BAppServerLink_()
{
	Init(true);
}

_BAppServerLink_::_BAppServerLink_(bool assert_session_exists)
{
	Init(assert_session_exists);
}

/*---------------------------------------------------------------*/

_BAppServerLink_::~_BAppServerLink_()
{
	if (session)
		main_session->unlock();
}

/*---------------------------------------------------------------*/
/*---------------------------------------------------------------*/

BApplication::BApplication(uint32 signature)
	: BLooper(0, -1, (char *)NULL)
{
	char mime_sig[256];
	to_new_sig(signature, mime_sig);
	InitData(mime_sig, NULL, true);
}

BApplication::BApplication(const char *mime_sig)
	: BLooper(0, -1, mime_sig)
{
	InitData(mime_sig, NULL, true);
}

BApplication::BApplication(const char *mime_sig, status_t *error)
	: BLooper(0, -1, mime_sig)
{
	InitData(mime_sig, error, true);
}

BApplication::BApplication(const char *mime_sig, status_t *error, bool connectToAppServer)
	: BLooper(0, -1, mime_sig)
{
	InitData(mime_sig, error, connectToAppServer);
}

void BApplication::InitData(const char *mime_sig, status_t *perror, bool connectToAppServer)
{
	const char	*path;
	app_info	ainfo;
	bool		launching_roster;
	bool		found_ready_to_run = false;
	BPath		realp;
	BList		init_messages;
	entry_ref	ref;
	status_t	err;
	BMessage	*cmd_line_msg = NULL;
	int32		exit_code = 1;
	
	//Setup StdOut redirect variables
	fRedirect = NULL;
	
	// so we can know if we need to free it
	fAppName = 0;

	if (!_is_valid_app_sig_(mime_sig)) {
		printf("bad signature (%s), must begin with \"application/\" and can't conflict with existing registered mime types inside the \"application\" media type.\n",
			mime_sig);
		err = B_BAD_VALUE;
		goto die_horrible_death;
	}

	AssertLocked();

	// finish the kit initialization that is specific to BeApps.

	fInitError = B_OK;
	fInitialWorkspace = B_CURRENT_WORKSPACE;
	fPulseRunner = NULL;

	// special BApplication code - unlike other BLooper objects, BApplication
	// doesn't create a new thread - it runs in the "main" thread
	fTaskID = find_thread(NULL);

	SetNextHandler(NULL);

	// ???
	// 1) How to delete application object and/or cleanly exit.
	// 2) conflicts b/w mime_sig parameter and mime_sig in resource?

	char				mime_sig_in_rsrc[B_MIME_TYPE_LENGTH];
	uint32				flags;

	_app_cleanup_object_.fIsBeApp = true;
	_app_cleanup_object_.fTeam = Team();

	err = resolve_link(argv_save[0], &realp);
	if (err != B_NO_ERROR) {
		printf(	"Unable to find the application binary: %s (%lx).\n"
				"Don't change the current working directory before "
				"creating the BApplication.\n", argv_save[0], err);
		goto die_horrible_death;
	}
	path = realp.Path();
//+	PRINT(("resolved app path: %s --> %s (%x)\n", argv_save[0], path, err));

	if (be_app) {
		syslog(LOG_ERR, "[%s] 2 BApplication objects were created. Only one is allowed.\n", path);
		debugger("2 BApplication objects were created. Only one is allowed.");
	}


	err = get_ref_for_path(path, &ref);
	if (err != B_NO_ERROR) {
		printf("get_ref_for_path(%s) err (%lx)\n", path, err);
		goto die_horrible_death;
	}

	/*
	 Get mime_sig and flags from resource
	*/
	err = _get_sig_and_flags_(&ref, mime_sig_in_rsrc, &flags);
//+	PRINT(("_sig_and_flags_: %s, %x (tid=%d)\n",
//+		mime_sig_in_rsrc, flags, find_thread(NULL)));

	if (err != B_NO_ERROR) {
		printf("get_sig_and_flags err (%lx)\n", err);
		goto die_horrible_death;
	}

	if (mime_sig_in_rsrc[0] == 0)
		strcpy(mime_sig_in_rsrc, mime_sig);
//	else if (strcasecmp(mime_sig, mime_sig_in_rsrc) != 0) {
//		printf("Signature in rsrc doesn't match constructor arg. (%s,%s)\n",
//			mime_sig, mime_sig_in_rsrc);
//	}

	launching_roster = (strcasecmp(mime_sig_in_rsrc, ROSTER_MIME_SIG) == 0);

	if (be_roster->IsAppPreRegistered(&ref, Team(), &ainfo)) {
		// found our "uninitialized" entry in the roster

//+		SERIAL_PRINT(("(team=%d) we are a pre-registered app\n", Team()));
		long count;

		/*
		 Need to fill in the Looper port, which might have been created
		 by the Roster. If it wasn't then create it now.
		*/
		if (ainfo.port != -1)
			fMsgPort = ainfo.port;
		else {
			if (launching_roster)
				fMsgPort = create_port(100, ROSTER_PORT_NAME);
			else
				fMsgPort = create_port(100, "AppLooperPort");
		}

		// turn any argv's into a message
		cmd_line_msg = ParseArguments();
		if (cmd_line_msg) {
			cmd_line_msg->SetWhen(0);	// always place at front of queue
			init_messages.AddItem(cmd_line_msg);
		}

		// Also, get any initial messages out of port now, before the
		// port becomes 'public' (i.e. after CompleteRegistration)
		count = port_count(fMsgPort);
		if (count) {
			while (count--) {
				ASSERT(port_count(fMsgPort));
				BMessage *msg = new BMessage();
				status_t e = msg->ReadPort(fMsgPort);
				ASSERT(e == B_OK);
				if (e == B_OK) {
					init_messages.AddItem(msg);
					if (msg->what == B_READY_TO_RUN)
						break;
				} else {
					delete msg;
				}
			}
		}

		// complete registration process started by LaunchApp
		be_roster->CompleteRegistration(Team(), Thread(), fMsgPort);

	} else {
		// if app not pre-registered then add it here

//+		SERIAL_PRINT(("(team=%d) NOT pre-registered app\n", Team()));
		if (launching_roster)
			fMsgPort = create_port(100, ROSTER_PORT_NAME);
		else
			fMsgPort = create_port(100, "AppLooperPort");

		// turn any argv's into a message
		cmd_line_msg = ParseArguments();
		if (cmd_line_msg) {
			cmd_line_msg->SetWhen(0);	// always place at front of queue
			init_messages.AddItem(cmd_line_msg);
		}

		uint32	entry_token = be_roster->AddApplication(mime_sig_in_rsrc, &ref,
					flags, Team(), Thread(), fMsgPort, true);

		/*
		if entry_token is 0 then we weren't able to register (probably due to
		the fact that we are a single launch app or are already running) so just
		send activate message (and any items that were passed on command line)
		*/
		if (entry_token == 0) {
//+			SERIAL_PRINT(("defering to already running instance (%s,%s)\n",
//+				mime_sig_in_rsrc, mime_sig));

			// only try to Activate if it isn't a background app
				
			BMessenger messenger(mime_sig_in_rsrc, -1, NULL);
			if (messenger.IsValid()) {
//+				if (!(flags & B_BACKGROUND_APP)) 	// HL 8/14/98
//+					be_roster->ActivateApp(messenger.Team());
				if (cmd_line_msg) {
					messenger.SendMessage(cmd_line_msg);
				} else {
//+					PRINT(("sending Relaunched (BApp::BApp)\n"));
					messenger.SendMessage(B_SILENT_RELAUNCH);
				}
			}
			if (cmd_line_msg)
				delete cmd_line_msg;

			// force application to exit BEFORE returning from BApp::BApp()
			exit_code = 0;
			err = B_ALREADY_RUNNING;
			goto die_horrible_death;
		}
	}

	if (long c = init_messages.CountItems()) {
		// we have some initial messages, so put them in queue
		for (long i = 0; i < c; i++) {
			BMessage *m = (BMessage *) init_messages.ItemAt(i);
			ASSERT(m);

			if(m->what == B_OPEN_IN_WORKSPACE) {
				int32	w;
				m->FindInt32("be:workspace", &w);
				fInitialWorkspace = 1 << w;
				delete m;
			} else {
//+				SERIAL_PRINT(("(tid=%d) initial msg (%.4s)\n",
//+					find_thread(NULL), (char *) &(m->what)));
				if (m->what == B_READY_TO_RUN)
					found_ready_to_run = true;
				AddMessage(m);
			}
		}
	}

	if (!found_ready_to_run) {
		// now put the B_READY_TO_RUN message into the queue...
		BMessage *mmmm = new BMessage(B_READY_TO_RUN);
		mmmm->SetWhen(system_time());
		AddMessage(mmmm);
	}

	// now get things going
	message pusher;
	pusher.what = 'PUSH';
	while (write_port(fMsgPort, -1, (char *) &pusher, 64) == B_INTERRUPTED)
		;

	// get our direct message target set up, now that we have a port
	InstallLooperTarget(this);
	
	be_app = this;
	be_app_messenger = BMessenger(this);
	
	fPulseRate = 0;
	fDraggedMessage = NULL;

	// now initialize the application clipboard.
	be_clipboard = new BClipboard("system", false);

	// the Roster should be initialized by the libbe init code
	ASSERT(be_roster);

	if (!launching_roster) {

		((BRoster *) be_roster)->InitMessengers();

		// this ensures that our mime_sig is registered properly
		ASSERT(Team());
//+		be_roster->SetSignature(Team(), mime_sig);
		be_roster->SetSignature(Team(), mime_sig_in_rsrc);

		if (flags & DEFAULT_APP_FLAGS_BIT) {
			/*
			 The AppInit code didn't find a 'APPI' resource for the
			 application so it assumed that the application was 'stupid'.
			 However, now that the app has created a BApplication object
			 we know that it can handle BMessages, so it shouldn't be
			 an B_ARGV_ONLY application.
			*/
			flags &= ~(B_ARGV_ONLY | DEFAULT_APP_FLAGS_BIT);
			be_roster->SetAppFlags(Team(), flags);
		} else if (flags & B_ARGV_ONLY) {
			/*
			 There was an APPI resource and it specified B_ARGV_ONLY. Well,
			 considering that that app is constructing a BApplication
			 object it better handle messages! So drop into the
			 debugger to inform the developer of their mistake.
			*/
//+			debugger("You can't have a BApplication and be B_ARGV_ONLY");
			PRINT(("ARGV_ONLY:	%x, %x\n", flags, flags & (~B_ARGV_ONLY)));
//+			flags &= ~B_ARGV_ONLY;
			be_roster->SetAppFlags(Team(), flags);
		}
	}

	fReadyToRunCalled = false;
	
	fAppName = strdup(ref.name);
	SetName(fAppName);		// set the BHandler name

	create_app_meta_mime(path, 0,1,0);

	if (connectToAppServer)
		connect_to_app_server();

	cursorSystemDefault.m_serverToken = CURSOR_SYSTEM_DEFAULT;
	cursorIBeam.m_serverToken = CURSOR_I_BEAM;

	for(;;)
	{
		void (*callback)(void) = (void(*)(void))callbacklist.RemoveItem(int32(0));
		if(!callback)
			break;
		callback();
	}

	return;

die_horrible_death:
	// can't open a window here. No other way to tell user. Don't
	// want to drop into debugger because things can continue running.

	fInitError = err;
	if (perror) {
		*perror = err;
	} else {
		// This is the old style constructor so we have to exit.
		exit(exit_code);	
	}
}

/*---------------------------------------------------------------*/

BApplication::BApplication(BMessage *data)
	: BLooper(0, -1, (char *)NULL)
{
	const char *mime_sig;
	data->FindString("mime_sig", &mime_sig);
	InitData(mime_sig, NULL, true);

	bigtime_t	rate;
	if (data->FindInt64(S_PULSE_RATE, &rate) == B_OK) {
		SetPulseRate(rate);
	}
}

/*---------------------------------------------------------------*/

long BApplication::Archive(BMessage *data, bool deep) const
{
	status_t err;
	err = _inherited::Archive(data, deep);

	if (!err) {
		app_info	ainfo;
		GetAppInfo(&ainfo);

		data->AddString("mime_sig", ainfo.signature);

		if (fPulseRate != 0)
			data->AddInt64(S_PULSE_RATE, fPulseRate);
	}
	return err;
}

/*---------------------------------------------------------------*/

BArchivable *BApplication::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BApplication"))
		return NULL;
	return new BApplication(data);
}
	
/*-------------------------------------------------------------*/

BApplication::~BApplication()
{
	if (fPulseRunner != NULL) {
		debugger("Must call Quit and wait for Run() to return before deleting app\n");
	}

	Lock();

	if (main_session)
		quit_all_windows(true);

	be_app = NULL;

	if (main_session) {
		_delete_menu_bitmaps_();	// do this before closing session.
		_fini_interface_kit_();

		main_session->lock();
		main_session->swrite_l(GR_CLOSEAPP);
		main_session->sclose();
		delete(main_session);
		main_session = NULL;

		delete_port(fServerFrom);

		/* free global font stuff */
		_clean_global_fonts_();
	
		// delete the dragmessage sem and the area, if it's still around
		delete_sem(fDraggedMessage->sem);
		if (fDraggedMessage->area != B_BAD_VALUE)
			delete_area(fDraggedMessage->area);
		delete fDraggedMessage;
	
		// delete the shared memory area clones
		if (fServerHeap) {
			delete_area(fServerHeap->rwClone);
			delete_area(fServerHeap->roClone);
			delete_area(fServerHeap->globalROClone);
			delete fServerHeap;
			fServerHeap = NULL;
		};
	}

	if (be_clipboard) {
		delete be_clipboard;
		be_clipboard = NULL;
	}

	if (fAppName) free(const_cast<char *>(fAppName));

	// don't want the BLooper destructor to think that Run() was called.
	fRunCalled = false;
	
	//Cleanup StdOut Redirect -- fRedirect is cleaned up
	//inside hms_B_PIPESTDOUT_RESET()
	if(fRedirect != NULL)
		hmsg_B_PIPESTDOUT_RESET(NULL);
}

/*-------------------------------------------------------------*/

status_t BApplication::GetAppInfo(app_info* info) const
{
	return(be_roster->GetRunningAppInfo(Team(), info));
}

/*-------------------------------------------------------------*/

status_t BApplication::InitCheck() const
{
	return fInitError;
}

/*-------------------------------------------------------------*/

bool BApplication::IsLaunching() const
{
	return !fReadyToRunCalled;
}

/*-------------------------------------------------------------*/

thread_id BApplication::Run()
{
	if (InitCheck() != B_OK)
		return InitCheck();

	if (fRunCalled) {
		debugger("BApplication::Run was already called. Can only be called once.");
		return -1;
	}

	fRunCalled = true;

	/*
	Special code for BApplication. Unlike other BLooper objects, BApplication
	doesn't create a new thread. It runs in the "main" thread. This is done
	by overriding the BLooper::Run() method and directly calling the
	task_looper() method.
	*/
	task_looper();

	if (fPulseRunner) {
		// this will kill the pulse runner
		SetPulseRate(0);
	}

	return fTaskID;
}

/*-------------------------------------------------------------*/

void BApplication::Quit()
{
	if (!IsLocked()) {
		printf("ERROR - you must Lock the application object before"
			" calling Quit(), team=%li, looper=%s\n",
			Team(), Name() ? Name() : "no-name");
	}

	if (fRunCalled) {
		/*
		 Run() was called so let's set the flag to quit.
		 For BApplication a synchcronous close doesn't make sense, so simply
		 set the flag and return.
		*/

		fTerminating = true;

		// The application might be blocked on the read_port(). Need to
		// wake it up...
		(void) PostMessage((ulong) 0);	// OK to ignore error
	} else {
		/*
		 Run() hasn't been called to make Quit() equivalent to delete.
		*/
		delete this;
	}
}

/*-------------------------------------------------------------*/

void BApplication::AboutRequested()
{
	thread_info tinfo;
	get_thread_info(Thread(), &tinfo);
	BAlert *a = new BAlert("_about_", tinfo.name, "OK");
	a->Go(NULL);
}

/*-------------------------------------------------------------*/

void BApplication::Pulse()
{
}

/*-------------------------------------------------------------*/

void BApplication::ArgvReceived(int32, char **)
{
}

/*-------------------------------------------------------------*/

void BApplication::ReadyToRun()
{
}

/*-------------------------------------------------------------*/

// Need to rename this function when Hiroshi is done with App.h...
void	BApplication::get_scs()
{	
	_the_screen_ = new BPrivateScreen;
}

/*-------------------------------------------------------------*/

void *	BApplication::rw_offs_to_ptr(uint32 offset)
{
	if (offset == 0xFFFFFFFF) return NULL;
	return ((uint8*)fServerHeap->rwAddress) + offset;
}

void *	BApplication::ro_offs_to_ptr(uint32 offset)
{
	if (offset == 0xFFFFFFFF) return NULL;
	return ((uint8*)fServerHeap->roAddress) + offset;
}

void *	BApplication::global_ro_offs_to_ptr(uint32 offset)
{
	if (offset == 0xFFFFFFFF) return NULL;
	return ((uint8*)fServerHeap->globalROAddress) + offset;
}

void	BApplication::setup_server_heaps()
{
	_BAppServerLink_ link;
	area_id rwArea,roArea,globalROArea;
	void *addr;

	link.session->swrite_l(GR_GET_SERVER_HEAP_AREA);
	link.session->flush();
	link.session->sread(4,&rwArea);
	link.session->sread(4,&roArea);
	link.session->sread(4,&globalROArea);

	if (rwArea < 0) {
		fServerHeap = NULL;
	} else {
		fServerHeap = new _server_heap_;

		addr = (void*)0xD0000000;
		fServerHeap->rwArea = rwArea;
		fServerHeap->rwClone =
			clone_area("rw_server_area",&addr,B_BASE_ADDRESS,
				B_READ_AREA|B_WRITE_AREA,rwArea);
		fServerHeap->rwAddress = addr;

		addr = (void*)0xDE000000;
		fServerHeap->roArea = roArea;
		fServerHeap->roClone =
			clone_area("ro_server_area",&addr,B_BASE_ADDRESS,
				B_READ_AREA|B_WRITE_AREA,roArea);
		fServerHeap->roAddress = addr;

		addr = (void*)0xDF000000;
		fServerHeap->globalROArea = globalROArea;
		fServerHeap->globalROClone =
			clone_area("global_ro_server_area",&addr,B_BASE_ADDRESS,
				B_READ_AREA,globalROArea);
		fServerHeap->globalROAddress = addr;
	};
}

/*-------------------------------------------------------------*/

void	BApplication::BeginRectTracking(BRect r, bool trackWhole)
{
	_BAppServerLink_ link;
	link.session->swrite_l(GR_SET_RECT_TRACK);
	if (trackWhole)
		link.session->swrite_l(1);
	else
		link.session->swrite_l(2);
	link.session->swrite_rect(&r);
	link.session->flush();
}

/*-------------------------------------------------------------*/

void	BApplication::EndRectTracking()
{
	BRect r;

	_BAppServerLink_ link;
	link.session->swrite_l(GR_SET_RECT_TRACK);
	link.session->swrite_l(0);		// turns off rect tracking
	link.session->swrite_rect(&r);
	link.session->flush();
}

/*-------------------------------------------------------------*/

void	BApplication::send_drag(BMessage *aMessage, int32 vs_token,
	BPoint offset, int32 bitmap_token, drawing_mode dragMode, BMessenger reply_to)
{
	_BAppServerLink_ link;
	uint32 dummy;
	/*
	  Dropping a message is different from SendMessage in that it
	  really a 'different' message in the drop case. The data is
	  compressed and re-hydrated into a new message when the drop
	  occurs. For this reason setting the 'sending-app' field is
	  a little clumsier.
	*/
	int16 mode = dragMode;
	aMessage->AddMessenger("_reply_messenger_", reply_to);
	link.session->swrite_l(GR_SET_DRAG_BM);
	link.session->swrite_l(vs_token);
	link.session->swrite_l((long) offset.x);
	link.session->swrite_l((long) offset.y);
	link.session->swrite_l(bitmap_token);
	link.session->swrite(2,&mode);
	write_drag(link.session, aMessage);
	link.session->flush();
	/*	We need to wait for an ack here because we may be (i.e. are)
		deleting the bitmap, and it may be a window, in which
		case the close message is sent over the window's own
		private port, in which case we have a race condition.
		This prevents that.  How often will we be dragging
		bitmaps, anyway? */
	link.session->sread(4,&dummy);
	aMessage->RemoveName("_reply_messenger_");
}

/*-------------------------------------------------------------*/

void	BApplication::send_drag(BMessage *aMessage, int32 vs_token,
	BPoint offset, BRect drag_rect, BMessenger reply_to)
{
	_BAppServerLink_ link;
	aMessage->AddMessenger("_reply_messenger_", reply_to);
	link.session->swrite_l(GR_SET_DRAG);
	link.session->swrite_l(vs_token);
	link.session->swrite_l((long) offset.x);
	link.session->swrite_l((long) offset.y);
	link.session->swrite_rect(&drag_rect);
	write_drag(link.session, aMessage);
	link.session->flush();
	aMessage->RemoveName("_reply_messenger_");
}

/*----------------------------------------------------------------*/

void	BApplication::write_drag(AppSession *the_session, BMessage *msg)
{
	message_write_context context;
	
	status_t result = msg->start_writing(&context);
	if (result == B_OK) {
		the_session->swrite_l(context.size);
		the_session->swrite(context.size, (void*)context.data);
	}
	
	msg->finish_writing(&context);
}

/*-------------------------------------------------------------*/

extern const unsigned char B_HAND_CURSOR[] =	 {
	16,	/*cursor size, valid values are 16 and 32*/
	1,	/*1 bit per pixel (only value right now) */
	2,	/*hot spot vertical			 */
	2,	/*hot spot horizontal			 */

/* data */
0x0,0x0,0x0,0x0,0x38,0x0,0x24,0x0,0x24,0x0,0x13,0xe0,0x12,0x5c,0x9,
0x2a,0x8,0x1,0x3c,0x1,0x4c,0x1,0x42,0x1,0x30,0x1,0xc,0x1,0x2,0x0,
0x1,0x0,

/* mask */
0x0,0x0,0x0,0x0,0x38,0x0,0x3c,0x0,0x3c,0x0,0x1f,0xe0,0x1f,0xfc,0xf,
0xfe,0xf,0xff,0x3f,0xff,0x7f,0xff,0x7f,0xff,0x3f,0xff,0xf,0xff,0x3,0xfe,
0x1,0xf8
};

extern const unsigned char B_I_BEAM_CURSOR[] =	 {
	16,	/*cursor size, valid values are 16 and 32*/
	1,	/*1 bit per pixel (only value right now) */
	5,	/*hot spot vertical				 */
	8,	/*hot spot horizontal			 */

/* data */
0x6,0xc0,0x3,0x80,0x1,0x0,0x1,0x0,0x1,0x0,0x1,0x0,0x1,0x0,0x1,
0x0,0x1,0x0,0x1,0x0,0x1,0x0,0x1,0x0,0x1,0x0,0x1,0x0,0x3,0x80,
0x6,0xc0,

/* mask */
0xf,0xc0,0x7,0x80,0x3,0x0,0x3,0x0,0x3,0x0,0x3,0x0,0x3,0x0,0x3,
0x0,0x3,0x0,0x3,0x0,0x3,0x0,0x3,0x0,0x3,0x0,0x3,0x0,0x7,0x80,
0xf,0xc0};

/*-------------------------------------------------------------*/

void	BApplication::ShowCursor()
{
	_BAppServerLink_ link;
	link.session->swrite_l(GR_SHOW_CURSOR);
	link.session->flush();
}

/*-------------------------------------------------------------*/

void	BApplication::HideCursor()
{
	_BAppServerLink_ link;
	link.session->swrite_l(GR_HIDE_CURSOR);
	link.session->flush();
}

/*-------------------------------------------------------------*/

void	BApplication::ObscureCursor()
{
	_BAppServerLink_ link;
	link.session->swrite_l(GR_OBSCURE_CURSOR);
	link.session->flush();
}

/*-------------------------------------------------------------*/

void BApplication::SetCursor(const BCursor *cursor, bool sync)
{
	_BAppServerLink_ link;
	link.session->swrite_l(GR_SET_CURSOR);
	link.session->swrite_l(cursor->m_serverToken);
	if (sync) link.session->full_sync();
}

void BApplication::SetCursor(const void *cursorData)
{
	BCursor cursor(cursorData);
	SetCursor(&cursor);
}

/*-------------------------------------------------------------*/

bool BApplication::IsCursorHidden() const
{
	bool		r;

	_BAppServerLink_ link;
	link.session->swrite_l(GR_IS_CURSOR_HIDDEN);
	link.session->flush();
	link.session->sread(sizeof(r), &r);

	return r;
}

#if 0
/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

class TMMKeyDownFilter : public BMessageFilter {
public:
						TMMKeyDownFilter();

virtual	filter_result	Filter(BMessage *msg, BHandler **target);
		bool			DoSwitch() { return fSwitchMenus; } ;
private:
		bool			fSwitchMenus;
};

//------------------------------------------------------------

TMMKeyDownFilter::TMMKeyDownFilter()
	: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, B_KEY_DOWN)
{
	fSwitchMenus = false;
}

//------------------------------------------------------------

filter_result TMMKeyDownFilter::Filter(BMessage *msg, BHandler **target)
{
	ulong	ch;
	msg->FindInt32("char", (long*) &ch);
	if ((ch == B_LEFT_ARROW) || (ch == B_RIGHT_ARROW)) {
		// only interested in LEFT or RIGHT arrow
		// change this into the B_ESCAPE character so that the main menu is
		// closed
//+		PRINT(("	(main_menu) replace LEFT w/ ESCAPE\n"));
		msg->ReplaceInt32("char", B_ESCAPE);
		fSwitchMenus = true;
	}

	return B_DISPATCH_MESSAGE;
}
#endif

/*---------------------------------------------------------------*/

void	BApplication::SetPulseRate(bigtime_t rate)
{
	/*
	 This function could be called from any thread.
	*/

	if (rate <= 0) {
		rate = 0;
	} else if (rate < 100000) {
		rate = 100000;
	} else {
		rate = (rate / 50000) * 50000;
	}

	if (Lock()) {
		fPulseRate = rate;
		ASSERT(rate >= 0);
		if (rate == 0) {
			if (fPulseRunner) {
				delete fPulseRunner;
				fPulseRunner = NULL;
			}
		} else if (!fPulseRunner) {
			BMessenger	me(this);
			BMessage	msg(B_PULSE);
			fPulseRunner = new BMessageRunner(me, &msg, rate, -1);
		} else {
			fPulseRunner->SetInterval(rate);
		}
		Unlock();
	}
}

bool BApplication::window_quit_loop(bool quitFilePanels, bool forceQuit)
{
	for (int32 index = 0;;) {
		BWindow *window = WindowAt(index);
		if (!window)
			break;
		
		if (!quitFilePanels && window->IsFilePanel()) {
			// skip over file panels - they would respond that they do not want
			// to get quit and should be deleted instead by the application
			index++;
			continue;
		}

		// if the Lock() fails it means that there was a race condition
		// on that window and it was just closed by some other means.
		if (window->Lock()) {
			BMessenger	w_mess(window, window);
//+			PRINT(("QuitRequested on \"%s\"\n", window->Title()));
			bool ok = forceQuit
				|| window->QuitRequested()
				|| (quitFilePanels && window->IsFilePanel());

			/*
			 To prevent window thread from being blocked, one trick that
			 app developers can use is to Unlock the window before bring
			 up an Alert inside of BWindow::QuitRequested. And then relock
			 after the alert completes. A subtle issue here is that the window
			 might really die in this case (e.g. it receives another msg
			 that forces it to quit on the spot). So, let's make sure that
			 the window still exists, and that it is really locked by this
			 thread, the application main thread.
			 We'll do this by trying to lock the window again. If the lock
			 works then we're OK. We just have to unlock twice to unwind
			 the lock stack.
			*/
			if (w_mess.LockTarget()) {
				// we're OK. So unlock once here
//+				PRINT(("second lock verified\n"));
				window->Unlock();
			} else {
				// oops, window is no longer a valid window ptr!
//+				PRINT(("disaster averted\n"));
				continue;
			}

			if (ok)
				window->Quit();
			else {
				// the request was denied so abort the CloseAll
				window->Unlock();
				return false;
			}
		}
	}
	return true;
}

/*----------------------------------------------------------------*/

bool BApplication::quit_all_windows(bool forceQuit)
{
	ASSERT(LockingThread() == find_thread(NULL));

	/*
	 Unlock the app thread so help prevent deadlocks.
	 ??? Is this needed anymore.  There has to be a better way.
	*/
	Unlock();
	bool result = window_quit_loop(false, forceQuit);
		// first time around leave file panels alone, they need
		// to get quit after every other window because they might
		// be needed for saving
	if (result)
		result = window_quit_loop(true, forceQuit);
			// this time only file panels should be around, quit
			// them too

	// ??? Lock the app thread again.
	Lock();

	return result;
}

/*----------------------------------------------------------------*/

int32	BApplication::CountWindows() const
{
	return count_windows(false);
}

/*----------------------------------------------------------------*/

int32	BApplication::count_windows(bool incl_menus) const
{
	int32	num_windows = 0;
	BList	list(10);

	if (!BLooper::sLooperListLock.Lock()) 
		return 0;

	BLooper::GetLooperList(&list);

	uint32 count = list.CountItems();

	// count all BWindows, repecting the 'incl_menus' flag.
	// always skip bitmap windows.
	for (uint32 i = 0; i < count; i++) {
		BLooper	*loop = (BLooper *) list.ItemAt(i);
		BWindow *w = dynamic_cast<BWindow *>(loop);
		if (w && !w->fOffscreen) {
			if (incl_menus || dynamic_cast<BMenuWindow*>(w) == NULL) {
				num_windows++;
			}
		}
	}
	BLooper::sLooperListLock.Unlock();

	return num_windows;
}

/*----------------------------------------------------------------*/

BWindow	*BApplication::WindowAt(int32 index) const
{
	return window_at(index, false);
}

/*----------------------------------------------------------------*/

status_t	BApplication::get_window_list(BList *list, bool incl_menus) const
{
	uint32	count;
	BWindow	*w = NULL;
	BList	loop_list(10);

	if (!BLooper::sLooperListLock.Lock()) {
		return B_ERROR;
	}

	BLooper::GetLooperList(&loop_list);

	count = loop_list.CountItems();

	// index into all BWindows, repecting the 'incl_menus' flag.
	// always skip bitmap windows.
	for (uint32 i = 0; i < count; i++) {
		BLooper	*loop = (BLooper *) loop_list.ItemAt(i);
		w = dynamic_cast<BWindow *>(loop);
		if (w && !w->fOffscreen) {
			if (incl_menus || dynamic_cast<BMenuWindow*>(w) == NULL) {
				list->AddItem(w);
			}
		}
	}

	BLooper::sLooperListLock.Unlock();
	return B_OK;
}

/*----------------------------------------------------------------*/

int32	BApplication::CountLoopers() const
{
	uint32	count;

	if (!BLooper::sLooperListLock.Lock()) {
		return -1;
	}

	count = BLooper::sLooperCount;

	BLooper::sLooperListLock.Unlock();

	return count;
}

/*----------------------------------------------------------------*/

BLooper	*BApplication::LooperAt(int32 index) const
{
	if (index < 0)
		return NULL;
	
	if (!BLooper::sLooperListLock.Lock()) {
		return NULL;
	}

	BList	list(10);
	BLooper::GetLooperList(&list);
	BLooper	*loop = (BLooper *) list.ItemAt(index);

	BLooper::sLooperListLock.Unlock();

	return loop;
}

/*----------------------------------------------------------------*/

BWindow	*BApplication::window_at(uint32 index, bool incl_menus) const
{
	uint32	count;
	uint32	windows = 0;
	BWindow	*w = NULL;
	BWindow	*window = NULL;
	BList	list(10);

	if (!BLooper::sLooperListLock.Lock()) {
		return NULL;
	}

	// ??? not very effiecient. Every time WindowAt(i) is called
	// a new BList is created.

	BLooper::GetLooperList(&list);

	count = list.CountItems();

	// index into all BWindows, repecting the 'incl_menus' flag.
	// always skip bitmap windows.
	for (uint32 i = 0; i < count; i++) {
		BLooper	*loop = (BLooper *) list.ItemAt(i);
		w = dynamic_cast<BWindow *>(loop);
		if (w && !w->fOffscreen) {
			if (incl_menus || dynamic_cast<BMenuWindow*>(w) == NULL) {
				if (windows++ == index) {
					window = w;
					break;
				}
			}
		}
	}
	BLooper::sLooperListLock.Unlock();
	
	return window;
}

/*---------------------------------------------------------------*/

bool BApplication::QuitRequested()
{
	return quit_all_windows(false);
}

/*---------------------------------------------------------------*/

void BApplication::do_argv(BMessage *msg)
{
	const char	*arg;
	char		**argv;
	long		argc;
	long		i;

	msg->FindInt32("argc", &argc);

	ASSERT(argc > 1);

	argv = (char **) malloc((argc + 1) * sizeof(char *));

	for (i = 0; i < argc; i++) {
		msg->FindString("argv", i, &arg);
		argv[i] = (char *) malloc(strlen(arg) + 1);
		strcpy(argv[i], arg);
	}
	argv[i] = NULL;

	ArgvReceived(argc, argv);

	for (i = 0; i < argc; i++) {
		free(argv[i]);
	}
	free(argv);
}

/*---------------------------------------------------------------*/

void	BApplication::DispatchMessage(BMessage *msg, BHandler *handler)
{
	ASSERT(IsLocked());

//	if (msg->what != B_PULSE)
//		PRINT(("Dispatching: %4s\n", &(msg->what)));

#if 0
	if (msg->what == B_REFS_RECEIVED ||
		msg->what == B_ARGV_RECEIVED ||
		msg->what == B_READY_TO_RUN ||
		msg->what == B_SILENT_RELAUNCH)
		BSer << "Dispatching: " << *msg << endl;
#endif

	switch (msg->what) {
		case _PING_: {
			break;
		}
		case B_QUIT_REQUESTED:
			{
			if (handler == this) {
				if (QuitRequested()) {
					Quit();
				} else if (msg->FindBool("_shutdown_")) {
					BMessage	m(B_CANCEL);
					BMessage	reply;
					_send_to_roster_(&m, &reply, false);
					msg->SendReply(&m);
				}
			} else {
				handler->MessageReceived(msg);
			}
			break;
			}
		case B_ABOUT_REQUESTED:
			if (handler == this) {
				AboutRequested();
			} else {
				handler->MessageReceived(msg);
			}
			break;
		case B_READY_TO_RUN:
			if (handler == this) {
				if (!fReadyToRunCalled)
					ReadyToRun();
				fReadyToRunCalled = true;
			} else {
				handler->MessageReceived(msg);
			}
			break;
		case B_ARGV_RECEIVED:
			if (handler == this) {
				do_argv(msg);
			} else {
				handler->MessageReceived(msg);
			}
			break;
		case B_PULSE:
			if (handler == this) {
				Pulse();
			} else {
				handler->MessageReceived(msg);
			}
			break;
		case B_APP_ACTIVATED:
			if (handler == this) {
				bool active;
				msg->FindBool("active", &active);
				AppActivated(active);
			} else {
				handler->MessageReceived(msg);
			}
			break;
		case B_REFS_RECEIVED:

			for (int32 index = 0; ; index++) {
				// populate the recent documents/folders list
				entry_ref ref;
				if (msg->FindRef("refs", index, &ref) != B_OK)
					break;
				BEntry entry(&ref, true);
				if (entry.InitCheck() == B_OK) {
					if (entry.IsDirectory())
						BRoster().AddToRecentFolders(&ref);
					else {

						// resolve the path
						BEntry entry(&ref, true);
						BNode node(&entry);
						
						// get the type
						char mimeString[B_MIME_TYPE_LENGTH];
						BNodeInfo info(&node);

						if (info.GetType(mimeString) != B_OK
							|| strcasecmp(mimeString, B_APP_MIME_TYPE) != 0) 
							// don't add apps to recent documents - this filters
							// out apps launched by Tracker
							BRoster().AddToRecentDocuments(&ref);
					}
				}
			}

			if (handler == this) {
				RefsReceived(msg);
			} else {
				handler->MessageReceived(msg);
			}
			break;
		case B_UI_SETTINGS_CHANGED:
			if (handler == this) {
				BMessage settings;
				BMessage names;
				msg->FindMessage("be:settings", &settings);
				msg->FindMessage("be:names", &names);
				if (!settings.IsEmpty() || !names.IsEmpty()) {
					cache_ui_settings(settings, &names);
					UISettingsChanged(&settings, 0);
					
					// Distribute message to all windows.
					BList	win_list(10);
					get_window_list(&win_list, true);
					const int32 count = win_list.CountItems();
	
					for (int32 i = 0; i < count; i++) {
						BLooper	*loop = (BLooper *) win_list.ItemAt(i);
						BMessenger(loop).SendMessage(*msg, BMessenger(), B_TIMEOUT, 0);
					}
				}
			} else {
				handler->MessageReceived(msg);
			}
			break;
		case _SHOW_DRAG_HANDLES_:
			bool b;
			if (msg->FindBool("visible", &b) == B_OK) {
				_toggle_handles_(b);
			}
			break;
		case _DISPOSE_DRAG_:
		{	/* Get rid of the dragmessage area */
			bigtime_t when;
			area_id theArea = msg->FindInt32("_msg_data_");
			msg->FindInt64("when",&when);
			if ((theArea == fDraggedMessage->serverArea) &&
				(fDraggedMessage->timestamp < when)) {
				/*	We want to avoid any potential deadlock with MouseMoved
					waiting for the app thread to do something, so we'll
					just forget about it if we can't delete the area pretty
					quickly.  The worst that'll happen is that the area
					stays around until the user moves the mouse through one
					of our windows. */
				if (acquire_sem_etc(fDraggedMessage->sem,100000,B_TIMEOUT,50000) == B_OK) {
					if ((theArea == fDraggedMessage->serverArea) &&
						(fDraggedMessage->timestamp < when)) {
						delete_area(fDraggedMessage->area);
						fDraggedMessage->area = B_BAD_VALUE;
						fDraggedMessage->serverArea = B_BAD_VALUE;
						fDraggedMessage->timestamp = when;
					};
					release_sem_etc(fDraggedMessage->sem,100000,B_DO_NOT_RESCHEDULE);
				};
			};
			break;
		}
		default:
			{
			_inherited::DispatchMessage(msg, handler);
			break;
			}
	}
}

/*---------------------------------------------------------------*/

void	BApplication::MessageReceived(BMessage *msg)
{
	bool		call_inherited = true;
	status_t	err;
	switch (msg->what) {
		case B_COUNT_PROPERTIES:
		case B_GET_PROPERTY:
		case B_SET_PROPERTY: {
			BMessage	specifier;
			int32		form;
			const char	*prop;
			int32		cur;
			err = msg->GetCurrentSpecifier(&cur, &specifier, &form, &prop);
			if (!err)
				call_inherited = ScriptReceived(msg, cur, &specifier,
					form, prop);
			break;
		}

		case B_SILENT_RELAUNCH: {
//+			PRINT(("B_RELAUNCHED\n"));
			call_inherited = false;
			be_roster->ActivateApp(Team());
			break;
		}
		
		case B_PIPESTDOUT_REQUESTED:
			hmsg_B_PIPESTDOUT_REQUESTED(msg);
			break;
		
		case B_PIPESTDOUT_RESET:
			hmsg_B_PIPESTDOUT_RESET(msg);
			break;
	}

	if (call_inherited)
		_inherited::MessageReceived(msg);
}

/*-------------------------------------------------------------*/

bool BApplication::ScriptReceived(BMessage *cmd, int32,
	BMessage *, int32 , const char *property)
{
	bool handled = false;
	BMessage reply(B_REPLY);
//+	PRINT(("BApp::ScriptReceived: cmd->what=%.4s, form=%d, property=%s\n",
//+		(char*) &(cmd->what), form, property ? property : "null"));
	switch (cmd->what) {
		case B_GET_PROPERTY:
			{
				bool just_windows = false;
				if (strcmp(property, "Name") == 0) {
					thread_info tinfo;
					get_thread_info(Thread(), &tinfo);
					reply.AddString("result", tinfo.name);
					handled = true;
				} else if (strcmp(property, "Loopers") == 0 ||
					(just_windows = strcmp(property, "Windows") == 0) == true) {
					BList list(10);
					if (BLooper::sLooperListLock.Lock()) {
						GetLooperList(&list);
						for (int32 i = 0;; i++) {
							BLooper *looper = static_cast<BLooper *>(list.ItemAt(i));
							if (!looper)
								break;
								
							if (just_windows && (dynamic_cast<BWindow *>(looper) == NULL)) 
								continue;
	
							reply.AddMessenger("result", BMessenger(looper));
						}
						BLooper::sLooperListLock.Unlock();
					}
					handled = true;
				}
				break;
			}
		case B_SET_PROPERTY: 
			break;

		case B_COUNT_PROPERTIES: 
			if (strcmp(property, "Window") == 0) {
				reply.AddInt32("result", CountWindows());
				handled = true;
			} else if (strcmp(property, "Looper") == 0) {
				reply.AddInt32("result", CountLoopers());
				handled = true;
			break;
		}
	}

	if (handled)
		cmd->SendReply(&reply);

	return !handled;
}

/*---------------------------------------------------------------*/

void	BApplication::AppActivated(bool)
{
}

/*---------------------------------------------------------------*/

void	BApplication::RefsReceived(BMessage *)
{
}

/*---------------------------------------------------------------*/

_BAppCleanup_::_BAppCleanup_()
{
	fIsBeApp = false;
}

_BAppCleanup_::~_BAppCleanup_()
{
	if (!fIsBeApp)
		return;

	if (be_roster)
		be_roster->RemoveApp(fTeam);

}

BMessage *ParseArguments()
{
	BMessage*		msg;
	int				i;

	if (argv_save[1] == NULL)
		return NULL;

	msg = new BMessage(B_ARGV_RECEIVED);

	i = 0;
	while (argv_save[i]) {
		msg->AddString("argv", argv_save[i]);
		i++;
	}
	ASSERT(i > 1);

	msg->AddInt32("argc", i);

	char cwd[PATH_MAX + 1];
	if (getcwd(cwd, PATH_MAX) != NULL)
		msg->AddString("cwd", cwd);

	return msg;
}

/*---------------------------------------------------------------*/

uint32 BApplication::InitialWorkspace()
{
	return fInitialWorkspace;
}

/*---------------------------------------------------------------*/

status_t _get_sig_and_flags_(const entry_ref *ref, char *mime_sig,
	uint32 *flags)
{
	/*
	 Get mime_sig and flags from resource
	*/

	BFile		file;
	status_t	err;

	// default settings in case we have no resources
	mime_sig[0] = 0;
	*flags = DEFAULT_APP_FLAGS;

	BEntry entry(ref);

	err = file.SetTo(&entry, O_RDONLY);

	BAppFileInfo app(&file);

	err = app.GetSignature(mime_sig);
//+	if (!err)
//+		app.SetSignature(mime_sig, ref);
	app.GetAppFlags(flags);

	// not finding the resources is NOT an error.
	return B_OK;
}

/*-------------------------------------------------------------*/

#if _SUPPORTS_FEATURE_SCRIPTING

// exp merging note-
// due to major exp-rel differences, pretty much the easiest way to merge these
// in the main tree is to redo them from scratch in the exp branch

enum {
	_NO_TAG_ = -1,
	_WINDOW_IDX_ = 0,
	_WINDOW_NAME_,
	_LOOPER_IDX_,
	_LOOPER_THREAD_,
	_LOOPER_NAME_,
	_FOR_ME_
};

static property_info prop_list[] = {
	{"Window",
		{},
		{B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER},
		NULL, _WINDOW_IDX_,
		{},
		{},
		{}
	},
	{"Window",
		{},
		{B_NAME_SPECIFIER},
		NULL, _WINDOW_NAME_,
		{},
		{},
		{}
	},
	{"Looper",
		{},
		{B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER},
		NULL, _LOOPER_IDX_,
		{},
		{},
		{}
	},
	{"Looper",
		{},
		{B_ID_SPECIFIER},
		NULL, _LOOPER_THREAD_,
		{},
		{},
		{}
	},
	{"Looper",
		{},
		{B_NAME_SPECIFIER},
		NULL, _LOOPER_NAME_,
		{},
		{},
		{}
	},
	{"Name",
		{B_GET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		NULL, _FOR_ME_,
		{B_STRING_TYPE},
		{},
		{}
	},
	{"Window",
		{B_COUNT_PROPERTIES},
		{B_DIRECT_SPECIFIER},
		NULL, _FOR_ME_,
		{B_INT32_TYPE},
		{},
		{}
	},
	{"Loopers",
		{B_GET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		NULL, _FOR_ME_,
		{B_MESSENGER_TYPE},
		{},
		{}
	},
	{"Windows",
		{B_GET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		NULL, _FOR_ME_,
		{B_MESSENGER_TYPE},
		{},
		{}
	},
	{"Looper",
		{B_COUNT_PROPERTIES},
		{B_DIRECT_SPECIFIER},
		NULL, _FOR_ME_,
		{B_INT32_TYPE},
		{},
		{}
	},
	{NULL,
		{},
		{},
		NULL, 0,
		{},
		{},
		{}
	}
};

#endif

/*-------------------------------------------------------------*/

BHandler *BApplication::ResolveSpecifier(BMessage *_SCRIPTING_ONLY(msg), int32 _SCRIPTING_ONLY(index),
	BMessage *_SCRIPTING_ONLY(spec), int32 _SCRIPTING_ONLY(form), const char *_SCRIPTING_ONLY(property))
{
#if _SUPPORTS_FEATURE_SCRIPTING
	status_t	err = B_OK;
	BHandler	*target = this;
	BMessage	error_msg(B_MESSAGE_NOT_UNDERSTOOD);
	int32		match;
	int32		tag = _NO_TAG_;


	BPropertyInfo	pi(prop_list);

	match = pi.FindMatch(msg, index, spec, form, property, &tag);
	
	if ((tag == _WINDOW_IDX_) || (tag == _LOOPER_IDX_)) {
		int32	index = spec->FindInt32("index");
		if (form == B_REVERSE_INDEX_SPECIFIER) {
			int32 cnt = (tag==_WINDOW_IDX_) ? CountWindows() : CountLoopers();
			index = cnt - index;
		}
		BLooper	*loop = (tag==_WINDOW_IDX_) ? WindowAt(index) : LooperAt(index);
		if (loop) {
			msg->PopSpecifier();
			err = loop->PostMessage(msg);
		} else {
			err = B_BAD_INDEX;
			error_msg.AddString("message", "window/looper index out of range");
		}

		target = NULL;
	} else if ((tag == _WINDOW_NAME_) || (tag == _LOOPER_NAME_)) {
		BWindow		*wd = NULL;
		BLooper		*loop = NULL;
		BLooper		*looper = NULL;
		bool		wd_only = (tag == _WINDOW_NAME_);
		int			i;
		const char	*name = spec->FindString(B_PROPERTY_NAME_ENTRY);
		if (!name) {
			err = B_BAD_SCRIPT_SYNTAX;
			goto error;
		}
		
		/*
		 There's a subtle race condition/deadlock to avoid. Can't own the
		 sLooperListLock and then call BWindow::Title or BHandler::Name. The
		 Title/Name can change at any moment, but this code can't call
		 BLooper::Lock to safely call Title() or Name(). So, the code
		 will not keep ownership of sLooperListLock.
		*/
		
		uint32	cached_id;

		do {
			cached_id = BLooper::sLooperID;
			/*
			 Because we don't (and can't) own sLooperListLock the list of
			 windows can change during this loop. That's why we'll repeat
			 the loop if we didn't find a match AND if a new looper was created
			 since we began.
			*/
			i = 0;
			while (1) {
				
				if (wd_only)
					loop = wd = WindowAt(i++);
				else
					loop = LooperAt(i++);

				if (!loop || !loop->Lock()) {
					break;
				}

				if (wd_only) {
					const char *t = wd->Title();
					if (t && (strcmp(t, name) == 0)) {
						looper = wd;
						loop->Unlock();
						break;
					}
				} else  {
					const char *n = loop->Name();
					if (n && (strcmp(n, name) == 0)) {
						looper = loop;
						loop->Unlock();
						break;
					}
				}

				loop->Unlock();
			}
		} while (!looper && (cached_id != BLooper::sLooperID));

		if (looper) {
			msg->PopSpecifier();
			err = looper->PostMessage(msg);
		} else  {
			err = B_NAME_NOT_FOUND;
		}

		target = NULL;
	} else if (tag == _LOOPER_THREAD_) {
		int32	tid = spec->FindInt32("id");
		BLooper	*loop = LooperForThread(tid);
		if (loop) {
			msg->PopSpecifier();
			err = loop->PostMessage(msg);
		} else {
			err = B_BAD_THREAD_ID;
		}
	} else if (tag == _NO_TAG_) {
		target = _inherited::ResolveSpecifier(msg,index,spec,form,property);
	} else {
		ASSERT(tag == _FOR_ME_);
		target = this;
	}

error:
	if (err) {
		error_msg.AddInt32("error", err);
		msg->SendReply(&error_msg);
		target = NULL;
	}

	return target;
#else
	return NULL;
#endif
}

/*-------------------------------------------------------------*/

BApplication::BApplication(const BApplication &)
	:	BLooper()
	{}
BApplication &BApplication::operator=(const BApplication &) { return *this; }

/*---------------------------------------------------------------*/

status_t	BApplication::GetSupportedSuites(BMessage *_SCRIPTING_ONLY(data))
{
#if _SUPPORTS_FEATURE_SCRIPTING
	data->AddString("suites", "suite/vnd.Be-application");
	BPropertyInfo	pi(prop_list);
	data->AddFlat("messages", &pi);
	return _inherited::GetSupportedSuites(data);
#else
	return B_UNSUPPORTED;
#endif
}

/*----------------------------------------------------------------*/

status_t BApplication::UISettingsChanged(const BMessage*, uint32)
{
	return B_OK;
}

/*----------------------------------------------------------------*/

status_t BApplication::Perform(perform_code d, void *arg)
{
	return _inherited::Perform(d, arg);
}

/* ---------------------------------------------------------------- */

BResources * BApplication::AppResources()
{
	BAutolock lock(_app_resources_lock);
	if (_app_resources != NULL) {
		return _app_resources;
	}
	entry_ref ref;
	bool ok = false;
	if (be_app) {
		app_info info;
		if (!be_app->GetAppInfo(&info)) {
			ref = info.ref;
			ok = true;
		}
	}
	if (!ok) {
		image_info info;
		thread_info tinfo;
		get_thread_info(find_thread(NULL), &tinfo);
		int32 cookie = 0;
		while (!ok && !get_next_image_info(tinfo.team, &cookie, &info)) {
			if (info.type == B_APP_IMAGE) {
				ok = !get_ref_for_path(info.name, &ref);
			}
		}
	}
	if (!ok) {
		return NULL;
	}
	_app_resources = new BResources;
	BFile file(&ref, O_RDONLY);
	if (file.InitCheck() || _app_resources->SetTo(&file, false)) {
		BString name(ref.name);
		name += ".rsrc";
		ref.set_name(name.String());
		if (file.SetTo(&ref, O_RDONLY) || _app_resources->SetTo(&file, false)) {
			delete _app_resources;
			_app_resources = NULL;
			return NULL;
		}
	}
	return _app_resources;
}

status_t BApplication::AddInitializer(void (*callback)(void))
{
	if(be_app)
	{
		callback();
		return B_OK;
	}
	
	if(callbacklist.AddItem((void*)callback))
		return B_OK;
	return B_ERROR;
}

status_t BApplication::RemoveInitializer(void (*callback)(void))
{
	if(callbacklist.RemoveItem((void*)callback))
		return B_OK;
	return B_ERROR;
}

void BApplication::hmsg_B_PIPESTDOUT_REQUESTED(BMessage *msg)
{
	BString pipename;
	char *buffer;
	if(msg->FindString("pipename", &pipename) != B_OK)
	{
		pipename.SetTo("stdout.XXXXXX");
		buffer = pipename.LockBuffer(0);
		buffer = mktemp(buffer);
		pipename.UnlockBuffer();
	}
	
	BString pipepath("/pipe/");
	pipepath << pipename;
		
	int pipeout = open(pipepath.String(), O_WRONLY | O_CREAT, 
		0x777);
	
	int readpipe = open(pipepath.String(), O_RDONLY);
	
	if((pipeout > -1))
	{
		if(fRedirect == NULL)
		{
			fRedirect = new redirect_struct;
			fRedirect->oldstdout = -1;
			fRedirect->readfd = -1;
		}
		
		printf("StdOut Redirected to %s\n\n", pipepath.String());
		fflush(stdout);
		
		BMessage rm(B_PIPESTDOUT_ACKNOWLEDGE);
		rm.AddString("pipename", pipename);
		BMessage reply;
		msg->SendReply(&rm, &reply, B_INFINITE_TIMEOUT, 2000);
		
		if(fRedirect->oldstdout == -1) fRedirect->oldstdout = dup(STDOUT_FILENO);
		
		fRedirect->readfd = readpipe;
		
		close(STDOUT_FILENO);
										
		dup2(pipeout, STDOUT_FILENO);
		close(pipeout);
		
		stdout = fdopen(STDOUT_FILENO, "w");				
		
		if(setvbuf(stdout, NULL, _IOLBF, 0) != 0)
			debugger("setvbuf error...");
		
		fflush(stdout);
		
		printf("Start output on %s\n\n", pipepath.String());
	}
}

void BApplication::hmsg_B_PIPESTDOUT_RESET(BMessage *msg)
{
	if((fRedirect != NULL) && (fRedirect->oldstdout > -1))
	{
		printf("Reseting StdOut...\n\n");
		
		close(fRedirect->readfd);
		close(STDOUT_FILENO);
		
		dup2(fRedirect->oldstdout, STDOUT_FILENO);
		
		delete fRedirect;
		fRedirect = NULL;
		
		printf("StdOut Reset...\n\n");
	}
}

/* ---------------------------------------------------------------- */

#if _R5_COMPATIBLE_
extern "C" {

	_EXPORT status_t
	#if __GNUC__
	_ReservedApplication1__12BApplication
	#elif __MWERKS__
	_ReservedApplication1__12BApplicationFv
	#endif
	(BApplication* This, const BMessage* changes, uint32 flags)
	{
		return This->BApplication::UISettingsChanged(changes, flags);
	}
}
#endif

void BApplication::_ReservedApplication2() {}
void BApplication::_ReservedApplication3() {}
void BApplication::_ReservedApplication4() {}
void BApplication::_ReservedApplication5() {}
void BApplication::_ReservedApplication6() {}
void BApplication::_ReservedApplication7() {}
void BApplication::_ReservedApplication8() {}

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/
