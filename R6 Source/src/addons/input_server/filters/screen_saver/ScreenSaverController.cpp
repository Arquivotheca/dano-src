#include "ScreenSaverController.h"
#if SCREENSAVER_USE_BINDER
#include "ScreenSaverControllerNode.h"
#include "command_constants.h"
#endif /* #if SCREENSAVER_USE_BINDER */
#include "ssdefs.h"
#include <Application.h>
#include <Debug.h>
#include <MessageRunner.h>
#include <FindDirectory.h>
#include <NodeMonitor.h>
#include <Roster.h>
#include <Screen.h>
#include <String.h>

#include <stdio.h>
#include <string.h>

#define BYLINE "screen_saver input_server filter"

const uint32 kWaitForBinder = 'H4CK';

ScreenSaverController::ScreenSaverController()
	: BLooper("ScreenSaver Looper"), last(system_time()), lastwake(system_time()),
		saving(false), fadetime(0), hotnow(-1), hotnever(-1),
		m_locker(new BMultiLocker(BYLINE " MultiLocker"))
#if SCREENSAVER_USE_BINDER
		, m_node(NULL), m_settings(NULL), m_listener(NULL)
#endif /* #if SCREENSAVER_USE_BINDER */
{
	BMessenger me(this);

	lastpos.Set(10, 10);	// a value that doesn't trigger any corner
	start = system_time();

	if(find_directory(B_USER_SETTINGS_DIRECTORY, &prefs_dir) != B_OK ||
		(prefs_name = prefs_dir).Append(SETTINGS_FILE_NAME) != B_OK)
	{
		prefs_dir = BPath();
		prefs_name = BPath();
	}

	// generate events every two seconds to check idleness
	idlecheck = new BMessageRunner(me, new BMessage('idle'), 1000000);

	// check launching and quitting of the module_runner
	be_roster->StartWatching(me);

	// node watching
	BEntry dir(prefs_dir.Path());
	if(dir.Exists())
	{
		node_ref dirref;
		dir.GetNodeRef(&dirref);

		watch_node(&dirref, B_WATCH_DIRECTORY, me);

		BEntry	settings(prefs_name.Path());
		if(settings.Exists())
		{
			settings.GetNodeRef(&settingsref);

			LoadPrefs();
			watch_node(&settingsref, B_WATCH_STAT, me);
		}
#if SCREENSAVER_USE_BINDER
		else {
			fadetime = 0L; // disable by default
			// send us a message to start waiting for the binder
			PostMessage(kWaitForBinder);				
		}
#endif /* #if SCREENSAVER_USE_BINDER */
	}
}

bool ScreenSaverController::QuitRequested()
{
	// shutdown gracefully
	BMessenger me(this);
	stop_watching(me);
	be_roster->StopWatching(me);
	delete idlecheck;
	delete m_locker;
	
#if SCREENSAVER_USE_BINDER
	if (m_listener) m_listener->StopListening(m_settings);
#endif /* #if SCREENSAVER_USE_BINDER */

	return true;
}

void ScreenSaverController::MessageReceived(BMessage *msg)
{
	const char	*sig;
	const char	*name;
	node_ref	nref;
	team_id		team;
	int32		opcode;

	switch(msg->what)
	{
		case 'idle' :
			if(! saving)
			{
				int32 currentcorner = Corner();

				if(hotnever == -1 || currentcorner != hotnever)
				{
					if(fadetime && system_time() - last > fadetime * 1000000LL) {
#if SCREENSAVER_LAUNCH_BY_PATH
						entry_ref ref;
						get_ref_for_path(kModuleRunnerPath, &ref);
						be_roster->Launch(&ref);
#else
						be_roster->Launch(module_runner_signature);
#endif
					}

					if(hotnow != -1 && currentcorner == hotnow && system_time() - last > 2000000LL) {
#if SCREENSAVER_LAUNCH_BY_PATH
						entry_ref ref;
						get_ref_for_path(kModuleRunnerPath, &ref);
						be_roster->Launch(&ref);
#else
						be_roster->Launch(module_runner_signature);
#endif
					}
				}
			}
			break;

		case B_NODE_MONITOR :
			msg->FindInt32("opcode", &opcode);
			switch(opcode)
			{
				case B_ENTRY_CREATED :
				case B_ENTRY_MOVED :
					// a node was added to the directory
					if(msg->FindString("name", &name) == B_OK &&
						strcmp(name, SETTINGS_FILE_NAME) == 0)
					{
						BEntry	settings(prefs_name.Path());
						if(settings.Exists())
						{
							// settings file added, create watch
							settings.GetNodeRef(&settingsref);
							LoadPrefs();
							watch_node(&settingsref, B_WATCH_STAT, BMessenger(this));
						}
					}
					break;

				case B_ENTRY_REMOVED :
					// a node was removed from the directory
					if(msg->FindInt32("device", &nref.device) == B_OK &&
						msg->FindInt64("node", &nref.node) == B_OK &&
						nref == settingsref)
					{
						// settings file removed, kill watch
						watch_node(&settingsref, B_STOP_WATCHING, BMessenger(this));
						fadetime = 0;
					}
					break;

				case B_STAT_CHANGED :
					// the settings file stats changed: reload it
					LoadPrefs();
					break;
			}
			break;

		case B_SOME_APP_LAUNCHED :
			if(! saving &&
				msg->FindString("be:signature", &sig) == B_OK &&
				msg->FindInt32("be:team", &team) == B_OK &&
				strcasecmp(sig, module_runner_signature) == 0)
			{
				saving = true;
#if SCREENSAVER_USE_BINDER
				// notify listeners of saving's change
				BMessage msg(kNotifyListeners);
				msg.AddInt32(kEventMaskField, B_PROPERTY_CHANGED);
				msg.AddString(kPropertyNameField, kIsBlankingProp);
				BMessenger messenger(m_node);
				messenger.SendMessage(&msg);
#endif /* #if SCREENSAVER_USE_BINDER */
				blanker_team = team;
				lastwake = system_time();
			}
			break;

		case B_SOME_APP_QUIT :
			if(msg->FindString("be:signature", &sig) == B_OK &&
					msg->FindInt32("be:team", &team) == B_OK &&
					team == blanker_team &&
					strcasecmp(sig, module_runner_signature) == 0) {
				saving = false;
#if SCREENSAVER_USE_BINDER
				// notify listeners of saving's change
				BMessage msg(kNotifyListeners);
				msg.AddInt32(kEventMaskField, B_PROPERTY_CHANGED);
				msg.AddString(kPropertyNameField, kIsBlankingProp);
				BMessenger messenger(m_node);
				messenger.SendMessage(&msg);
#endif /* #if SCREENSAVER_USE_BINDER */
			}
			break;

		default :
			BLooper::MessageReceived(msg);
			break;

#if SCREENSAVER_USE_BINDER
		case kWaitForBinder: {
			status_t err;
#if DEBUG
			bigtime_t startTime = system_time();
#endif /* #if DEBUG */
	
			// wait for prefs to become available in binder
			while (true) {
				m_settings = BinderNode::Root()
					/ "service" / "screensaver" / "settings";
				if (m_settings->IsValid()) break;
				
				snooze(250000); // wait a quarter sec.
			}
#if DEBUG
			SERIAL_PRINT((BYLINE ": waited %Ld usec. for binder\n",
				system_time() - startTime));
#endif /* #if DEBUG */
			
			// prefs available -- grab them
			err = GetPrefsFromBinder();
			SERIAL_PRINT((BYLINE ": GetPrefsFromBinder() returned %s\n",
				strerror(err)));
				
			// mount our node
			m_node = new ScreenSaverControllerNode(BMessenger(this));
			BinderNode::property saverNode = BinderNode::Root() / "service"
				/ "screensaver";
			saverNode["control"] = m_node;

			// start listening for changes to prefs
			m_listener = new Listener(this);			
			
			err = m_listener->StartListening(m_settings, B_PROPERTY_CHANGED, "enabled");
			SERIAL_PRINT((BYLINE ": Listener::StartListening() returned %s\n",
				strerror(err)));
			err = m_listener->StartListening(m_settings, B_PROPERTY_CHANGED, "fade_time");
			SERIAL_PRINT((BYLINE ": Listener::StartListening() returned %s\n",
				strerror(err)));
			err = m_listener->StartListening(m_settings, B_PROPERTY_CHANGED, "no_blank");
			SERIAL_PRINT((BYLINE ": Listener::StartListening() returned %s\n",
				strerror(err)));
						
			break;
		}

		case kGetIdleTime: {
			BMessage reply(msg->what);
			bigtime_t idleTime = system_time() - last;
			
			reply.AddInt64(kIdleTimeField, idleTime);
			msg->SendReply(&reply);
			
			break;
		}
		
		case kQuitBlanker: {
			BMessage reply(msg->what);
			status_t result;

			if (saving) {
				// reset the idle timer
				last = system_time();
	
				// tell screen_blanker to quit
				BMessenger messenger(module_runner_signature, blanker_team);
				result = messenger.SendMessage(B_QUIT_REQUESTED);
			} else {
				// the blanker isn't running: return an error so ReadProperty()
				// will set the property to "false"
				result = B_ERROR;
			}
			
			reply.AddInt32(kQuitBlankerResultField, result);
			msg->SendReply(&reply);
			
			break;
		}
		
		case kGetIsBlanking: {
			BMessage reply(msg->what);
			
			reply.AddBool(kIsBlankingField, saving);
			msg->SendReply(&reply);
			
			break;
		}
			
		case kResetIdleTimer:
			last = system_time();
			
			break;
#endif /* #if SCREENSAVER_USE_BINDER */
	}
}

void ScreenSaverController::LoadPrefs()
{
	BFile		prefs_file;
	BMessage	settings;
	int32		flags;
	int32		ftime;

	if(prefs_file.SetTo(prefs_name.Path(), B_READ_ONLY) == B_OK &&
		settings.Unflatten(&prefs_file) == B_OK)
	{
		// load the time deltas, checking flags
		fadetime = 0;
		hotnow = -1;
		hotnever = -1;
		if(settings.FindInt32(kTimeFlags, &flags) == B_OK && (flags & kDoFade))
		{
			if(settings.FindInt32(kTimeFade, &ftime) == B_OK && ftime >= 30)
				fadetime = ftime;
			else
				fadetime = 0;
			settings.FindInt32(kCornerNow, &hotnow);
			settings.FindInt32(kCornerNever, &hotnever);
		}
	}
}

bool ScreenSaverController::ProcessInput(BMessage *message)
{
	last = system_time();

	// quit module runner immediately
	if(saving)
	{
		int32	mod;
		int32	chr;
		if(message->FindInt32("modifiers", &mod) == B_OK)
		{
			if((mod & (B_CONTROL_KEY | B_COMMAND_KEY)) == (B_CONTROL_KEY | B_COMMAND_KEY))
				return false;	// don't like this sequence
			else
				if(message->FindInt32("raw_char", &chr) == B_OK &&
					((mod & B_CONTROL_KEY) != 0 || (mod & B_COMMAND_KEY) != 0) && chr == 9)
						return false;	// don't like this sequence either
		}

		if(lastwake + 500000 < last)
		{
			BMessenger	m(module_runner_signature, blanker_team);
			BMessage	msg(B_QUIT_REQUESTED);

			// timeout immediatly if the target application's queue is full
			// so we don't block the input_server filter process.
			// if the target application's queue is full the user will have to
			// wiggle the mouse a little more to kill the screen saver
			m.SendMessage(&msg, (BHandler *)0, (bigtime_t)0);

			lastwake = last;
		}
	}

	// the last mouse position
	if(message->what == B_MOUSE_MOVED)
		message->FindPoint("where", &lastpos);

	return true;
}

int8 ScreenSaverController::Corner()
{
	int8	corner = -1;
#define CX 3
#define CY 3

	// this hack is needed to let the input_server initialize and
	// position the cursor at the center of the screen, if we don't
	// do this and the corner is the upper-left the screen will blank
	// on boot
	if(system_time() - start < 5000000)
		return -1;

	BPoint	pos = lastpos;
	BRect	f = BScreen().Frame();

	if(pos.x < f.left + CX && pos.y < f.top + CY)
		corner = 0;
	else
	{
		if(pos.x > f.right - CX && pos.y < f.top + CY)
			corner = 1;
		else
		{
			if(pos.x > f.right - CX && pos.y > f.bottom - CY)
				corner = 2;
			else
			{
				if(pos.x < f.left + CX && pos.y > f.bottom - CY)
					corner = 3;
			}
		}
	}

	return corner;
}

#if SCREENSAVER_USE_BINDER
status_t ScreenSaverController::GetPrefsFromBinder(void)
{
	SERIAL_PRINT((BYLINE ": %s\n", __PRETTY_FUNCTION__));

	BinderNode::property parent = BinderNode::Root() / "service" / "screensaver"
		/ "settings";
	
	BinderNode::property enabled = parent / "enabled";
	BinderNode::property fadetimePref = parent / "fade_time";
	BinderNode::property noblank = parent / "no_blank";

	if (enabled.IsUndefined() || fadetimePref.IsUndefined()
			|| noblank.IsUndefined()) {
		SERIAL_PRINT((BYLINE ": some properties are undefined\n"));
		return B_ERROR;
	}
	
	fadetime = (enabled.String() == "true" && noblank.String() == "false") ?
		static_cast<int32>(fadetimePref.Number()) : 0L;
	SERIAL_PRINT((BYLINE ": fadetime got set to %ld\n", fadetime));
	
	return B_OK;
}

Listener::Listener(ScreenSaverController* controller)
	: m_controller(controller)
{
	SERIAL_PRINT((BYLINE ": %s()\n", __PRETTY_FUNCTION__));
}

Listener::~Listener(void)
{
}

status_t Listener::Overheard(binder_node node,
	uint32 observed, BString propertyName)
{
	SERIAL_PRINT((BYLINE ": %s(%s)\n", __PRETTY_FUNCTION__,
		propertyName.String()));

	status_t err = B_OK;

	if (observed & B_PROPERTY_CHANGED) {
		if (propertyName == "no_blank" || propertyName == "enabled") {
			BMessenger messenger(m_controller);
		
			// reset idle timer
			SERIAL_PRINT((BYLINE ": resetting idle time in ObserverCallback()\n"));
			messenger.SendMessage(new BMessage(kResetIdleTimer));
			
			// if screen_blanker is running, and no_blank has been set to "true"
			// or enabled has been set to "false", quit screen_blanker
			BMessage msg(kGetIsBlanking);
			bool isBlanking;
			
			err = messenger.SendMessage(&msg, &msg);
			if (err == B_OK && msg.what != B_NO_REPLY) {
				err = msg.FindBool(kIsBlankingField, &isBlanking);
				if (err == B_OK && isBlanking) {
					BinderNode::property prop =
						BinderNode::property::property(node)[propertyName.String()];
					// [Ed.: whew, that's ugly!]
					if ((propertyName == "no_blank" && prop.String() == "true")
							|| (propertyName == "enabled" && prop.String() == "false")) {
						// tell screen_blanker to quit
						SERIAL_PRINT((BYLINE ": quitting screen_blanker in "
							"ObserverCallback()\n"));
						messenger.SendMessage(new BMessage(kQuitBlanker));
					}
				}
			}	
		}
		
		// some property we're interested in has changed
		err = m_controller->GetPrefsFromBinder();
	}
	
	return err;
}
#endif /* #if SCREENSAVER_USE_BINDER */
