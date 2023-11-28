#include "ModuleRunner.h"
#include "Activity.h"
#include "ssdefs.h"
#include "runner_global.h"
#include "LockWindow.h"
#include <MessageRunner.h>
#include <Screen.h>
#include <Application.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Roster.h>
#include <File.h>
#include <Beep.h>
#if SCREENSAVER_USE_BINDER
#include <Binder.h>
#endif /* #if SCREENSAVER_USE_BINDER */
#include <Debug.h>
#include <string.h>

#include <stdio.h>

// module settings are global and unprotected since they're accessed read-only
BMessage global_settings;
const char *module_path;

ModuleRunner::ModuleRunner()
 : BApplication(module_runner_signature)
{
	memset(act, 0, sizeof(act));
	override = false;
	currentact = -1;
	crash_protection = false;
	locked = false;
	unlocking = false;
	unlocked = false;
}

void ModuleRunner::LoadPrefs()
{
	BPath		prefs_name;
	BFile		prefs_file;

	if(find_directory(B_USER_SETTINGS_DIRECTORY, &prefs_name) == B_OK &&
		prefs_name.Append(SETTINGS_FILE_NAME) == B_OK &&
		prefs_file.SetTo(prefs_name.Path(), B_READ_ONLY) == B_OK)
		global_settings.Unflatten(&prefs_file);
}

void ModuleRunner::ReadyToRun()
{	
	LoadPrefs();

	// limit number of screen_blankers
	BList	list;
	be_roster->GetAppList(module_runner_signature, &list);
	if(list.CountItems() > (crash_protection ? 2 : 1))
	{
		if(override)
		{
			// who am I?
			thread_info info;
			get_thread_info(find_thread(0), &info);

			// who is the master? (if I'm the third
			// team then master or slave will do,
			// since the slave will forward the
			// message to the master)
			team_id		main = (team_id)list.ItemAt(0);
			if(main == info.team)
				main = (team_id)list.ItemAt(1);

			BMessenger	master(module_runner_signature, main);
			BMessage	m(B_REFS_RECEIVED);
			BEntry		e(module_path);
			entry_ref	ref;
			e.GetRef(&ref);
			m.AddRef("refs", &ref);
			master.SendMessage(&m);
		}
		PostMessage(B_QUIT_REQUESTED);
		return;
	}

	// for real: hide cursor
	be_app->HideCursor();

	if(crash_protection)
	{
		// we're all set to go in module running mode!
		act[0] = new ModuleRunActivity;
		delay[0] = 0;
	}
	else
	{
		uint32		flags;
		int32		time[5];
		const char	*modulename;

		if(! override)
		{
			module_path = 0;
			// load requested add-on
#if !SCREENSAVER_USE_BINDER
			if(global_settings.FindString(kModuleName, &modulename) == B_OK)
#else
			if (GetModuleNameFromBinder(&modulename) == B_OK)
#endif
			{
				image_id			mod;
				void				*inst;

				if(find_directory(B_USER_ADDONS_DIRECTORY, &addon) == B_OK &&
					addon.Append(kScreenSaversDir) == B_OK &&
					addon.Append(modulename) == B_OK)
				{
					const char *name = addon.Path();
					PRINT(("screen_blanker: attempting to load add-on "
						"\"%s\"\n", name));
					if((mod = load_add_on(name)) >= 0)
					{
						if(get_image_symbol(mod, "instantiate_screen_saver", B_SYMBOL_TYPE_TEXT, &inst) == B_OK)
							module_path = name;
						unload_add_on(mod);
					}
				}

				if(! module_path &&
					find_directory(B_BEOS_ADDONS_DIRECTORY, &addon) == B_OK &&
					addon.Append(kScreenSaversDir) == B_OK &&
					addon.Append(modulename) == B_OK)
				{
					const char *name = addon.Path();
					if((mod = load_add_on(name)) >= 0)
					{
						if(get_image_symbol(mod, "instantiate_screen_saver", B_SYMBOL_TYPE_TEXT, &inst) == B_OK)
							module_path = name;
						unload_add_on(mod);
					}
				}
			}
		}

		// fetch current settings
		if(global_settings.FindInt32(kTimeFlags, (int32 *)&flags) != B_OK)
			flags = 0;
	
		if(global_settings.FindInt32(kTimeFade, &time[0]) != B_OK ||
			global_settings.FindInt32(kTimeStandby, &time[1]) != B_OK ||
			global_settings.FindInt32(kTimeSuspend, &time[2]) != B_OK ||
			global_settings.FindInt32(kTimeOff, &time[3]) != B_OK)
			time[0] = time[2] = time[2] = time[3] = 0;
	
		if(testmode)
		{
			act[0] = new ModuleActivity;
			delay[0] = 0;
		}
		else
		{
			// kill off unusable flags
			uint32	caps = BScreen().DPMSCapabilites();
			uint32	mask = ((caps & B_DPMS_STAND_BY) ? kDoStandby : 0) |
					((caps & B_DPMS_SUSPEND) ? kDoSuspend : 0) |
					((caps & B_DPMS_OFF) ? kDoOff : 0);
			flags &= kDoFade | mask;

			// setup sequencer
			max = 0;
			// remove activities lasting 0 seconds
			for(int32 i = 3; i > 0; i--)
				if(time[i] == 0 && (flags & (1 << i)))
				{
					// kill first non-zero bit before the current
					for(int32 j = i - 1; j >= 0; j--)
						if(flags & (1 << j))
						{
							flags &= ~(1 << j);
							i = j;
							break;
						}
				}

			int32	junk;
			int32	*last = &junk;

			// setup fading
			if(flags & kDoFade)
			{
				act[max] = new ModuleActivity;
				*last = time[0];
				last = &delay[max];
				max++;
			}

			// setup DPMS stand by
			if(flags & kDoStandby)
			{
				act[max] = new DPMSActivity(B_DPMS_STAND_BY);
				*last = time[1];
				last = &delay[max];
				max++;
			}
	
			// setup DPMS suspend
			if(flags & kDoSuspend)
			{
				act[max] = new DPMSActivity(B_DPMS_SUSPEND);
				*last = time[2];
				last = &delay[max];
				max++;
			}
	
			// setup DPMS off
			if(flags & kDoOff)
			{
				act[max] = new DPMSActivity(B_DPMS_OFF);
				*last = time[3];
				last = &delay[max];
				max++;
			}
	
			// do something sensible if no flags are set
			if(max == 0)
			{
				act[max] = new ModuleActivity;
				*last = time[0];
				last = &delay[max];
				max++;
			}

			*last = 0;

			// set up lock event
			bool		lockenable;
			int32		lockdelay;
			if(global_settings.FindBool(kLockEnable, &lockenable) == B_OK &&
				global_settings.FindInt32(kLockDelay, &lockdelay) == B_OK &&
				lockenable)
			{
				lockdelay -= time[0];
				BMessage lock('lock');
				if(lockdelay < 1)
					PostMessage(&lock);
				else
					new BMessageRunner(BMessenger(this), &lock, lockdelay * 1000000LL, 1);
			}
		}
	}
	if(act[0])
		PostMessage('step');
	else
		be_app->PostMessage(B_QUIT_REQUESTED);
}

bool ModuleRunner::QuitRequested()
{
	// someone else is going to take care of everything
	if(crash_protection)
	{
		BList	list;
		be_roster->GetAppList(module_runner_signature, &list);
		if(list.CountItems() == 2)
		{
			// who am I?
			thread_info info;
			get_thread_info(find_thread(0), &info);

			// who is the master?
			team_id		main = (team_id)list.ItemAt(0);
			if(main == info.team)
				main = (team_id)list.ItemAt(1);

			BMessenger	master(module_runner_signature, main);
			BMessage *msg = CurrentMessage();
			if(msg->ReturnAddress().Team() != main)
			{
				master.SendMessage(msg, (BHandler *)0, 0LL);
				return false;
			}
		}

		if(currentact != -1)
			act[currentact]->Stop();

		return true;
	}

	if(unlocking)
		return unlocked;

	if(locked)
	{
		// open a full screen window
		unlocking = true;
		lockwin = new LockWindow(&unlocked);

		// make sure we re-fade
		int32	ftime;
		if(global_settings.FindInt32(kTimeFade, &ftime) == B_OK)
		{
			BMessage	refade('refd');
			new BMessageRunner(BMessenger(this), &refade, ftime * 1000000LL, 1);
		}
	}

	if(currentact != -1)
		act[currentact]->Stop();
	ShowCursor();

	return locked ? false : true;
}

void ModuleRunner::ArgvReceived(int32 argc, char **argv)
{
	BEntry		e;

	override = true;
	module_path = 0;

	if(argc > 1 && strcmp(argv[1], "--test") == 0) {
		testmode = true;
		override = false;
		argc--;
		argv++;
	}
	if(argc > 1 && strcmp(argv[1], "--unsafe") == 0) {
		crash_protection = true;
		argc--;
		argv++;
	}
	
	if(argc > 1 && e.SetTo(argv[1]) == B_OK &&
		e.Exists() == true)
	{
		addon = argv[1];
		module_path = addon.Path();
	}

	if(! IsLaunching())
		PostMessage('load');
}

void ModuleRunner::RefsReceived(BMessage *msg)
{
	// assumes the ref points to a valid screen saver module
	// so it passes it on to the blanker
	entry_ref	ref;
	BEntry		e;

	override = true;
	module_path = 0;

	if(msg->FindBool("_testmode_", &testmode) != B_OK)
		testmode = false;

	if(msg->FindRef("refs", &ref) == B_OK &&
		e.SetTo(&ref) == B_OK &&
		e.Exists() == true)
	{
		e.GetPath(&addon);
		module_path = addon.Path();
	}

	if(! IsLaunching())
		PostMessage('load');
}

void ModuleRunner::MessageReceived(BMessage *msg)
{
	const char	*path;
	BEntry		e;
		
	switch(msg->what)
	{
		case 'lock' :
			// enable locking: we won't quit no more
			locked = true;
			break;

		case 'refd' :
			// kill the password entry window
			// this might have to be delayed to avoid flicker
			if(lockwin->Lock())
				lockwin->Quit();

			unlocking = false;
			unlocked = false;

			PostMessage('rest');
			break;

		case 'sbCP' :	// screen blanker crash protection
			crash_protection = true;
			module_path = 0;
			if(msg->FindString("module", &path) == B_OK &&
				e.SetTo(path) == B_OK &&
				e.Exists() == true)
			{
				addon = path;
				module_path = addon.Path();
			}
			break;

		case 'crsh' :
			// module crashed, restart with blank module
			addon = BPath();
			module_path = addon.Path();

			// stop previously running module
			act[currentact]->Stop();

			PostMessage('rest');
			break;

		case 'rest' :
			// hide cursor again
			be_app->HideCursor();

			// do the first step
			currentact = -1;
			act[++currentact]->Start();
			if(delay[currentact])
				new BMessageRunner(BMessenger(this), new BMessage('step'), delay[currentact] * 1000000LL, 1);
			break;

		case 'load' :
			if(! crash_protection)
			{
				// change the currently running module
				if(currentact != -1)	
					act[currentact]->Stop();
				PostMessage('rest');
			}
			break;

		case 'step' :
			if(currentact != -1)
				act[currentact]->Next();
			act[++currentact]->Start();

			if(delay[currentact])
				new BMessageRunner(BMessenger(this), new BMessage('step'), delay[currentact] * 1000000LL, 1);
			break;

		default :
			BApplication::MessageReceived(msg);
			break;
	}
}

#if SCREENSAVER_USE_BINDER
status_t ModuleRunner::GetModuleNameFromBinder(const char** moduleName)
{
	BinderNode::property currentModule = BinderNode::Root() / "service"
		/ "screensaver" / "settings" / "current_module";
		
	if (currentModule.IsUndefined()) {
		PRINT(("/service/screensaver/settings/current_module is undefined\n"));
		return B_ERROR;
	} else {
		PRINT(("current_module is %s\n", currentModule.String().String()));
		/* make this work like the non-binder version:
		** moduleName is a pointer to otherwise allocated storage
		*/
		fModuleName = currentModule.String();
		*moduleName = fModuleName.String();
	}
	
	return B_OK;
}
#endif
