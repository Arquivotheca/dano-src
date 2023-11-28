#include "ssdefs.h"
#include "runner_global.h"
#include "Activity.h"
#include "BlankWindow.h"
#include <Application.h>
#include <Messenger.h>
#include <Roster.h>
#include <Screen.h>
#include <View.h>

#include <stdio.h>
#include <string.h>

BWindow *DPMSActivity::black = 0;

DPMSActivity::DPMSActivity(uint32 state)
 : st(state)
{
}

void DPMSActivity::Start()
{
	BScreen	scr;
	scr.SetDPMS(st);
	if(! black)
	{
		black = new BWindow(scr.Frame(), "", B_NO_BORDER_WINDOW_LOOK, B_MODAL_ALL_WINDOW_FEEL, 0, B_ALL_WORKSPACES);
		BView *v = new BView(scr.Frame(), "", 0, 0);
		black->AddChild(v);
		v->SetViewColor(0, 0, 0);
		black->Show();
	}
}

void DPMSActivity::Next()
{
}

void DPMSActivity::Stop()
{
	BScreen().SetDPMS(B_DPMS_ON);
	if(black && black->Lock())
	{
		black->Quit();
		black = 0;
	}
}

ModuleActivity::ModuleActivity()
{
}

// a hackish way of finding add-on information, based on
// an old post to BeDevTalk by Jon Wätte (no less)
static char	globalvar = 1;

bool find_addon_path(char *buf);
bool find_addon_path(char *buf)
{
	const char	*addr = &globalvar;
	image_info	info;
	int32		cookie = 0;

	// Get the image_info for every image in this team.
	while(get_next_image_info(0, &cookie, &info) == B_OK)
	{
		if (((char *)info.data <= addr) &&
			(((char *)info.data+info.data_size) > addr))
		{
			strcpy(buf, info.name);
			return true;
		}
	}

	return false;
}

class DeathDetector : public BLooper
{
	team_id		team;
public:
	DeathDetector(team_id t) : BLooper("premature death"), team(t)
	{
		Run();
		BMessenger me(this);
		be_roster->StartWatching(me);
	}

	bool QuitRequested()
	{
		BMessenger me(this);
		be_roster->StopWatching(me);
		return true;
	}

	void MessageReceived(BMessage *msg)
	{
		team_id	some_team;

		switch(msg->what)
		{
			case B_SOME_APP_QUIT :
				// YOWZA! The real module runner died!
				if(msg->FindInt32("be:team", &some_team) == B_OK &&
					team == some_team)
					be_app->PostMessage('crsh');
				break;

			default :
				BLooper::MessageReceived(msg);
				break;
		}
	}
};

void ModuleActivity::Start()
{
	BMessage	init('sbCP');	// screen blanker crash protection

	// module to run
	if(module_path)
		init.AddString("module", module_path);

	// try launching myself
	char path[MAXPATHLEN];
	if(find_addon_path(path))
	{
		BEntry e(path);
		entry_ref ref;
		e.GetRef(&ref);
		be_roster->Launch(&ref, &init, &team);
	}
	else
		// didn't find myself, run through signature
		be_roster->Launch(module_runner_signature, &init, &team);

	dd = new DeathDetector(team);
}

void ModuleActivity::Stop()
{
	if(dd->Lock())
		dd->Quit();
	BMessenger	runner(module_runner_signature, team);
	runner.SendMessage(B_QUIT_REQUESTED);
}

ModuleRunActivity::ModuleRunActivity()
{
}

void ModuleRunActivity::Start()
{
	win = new BlankWindow();
	win->Run();
}

void ModuleRunActivity::Stop()
{
	win->PostMessage(B_QUIT_REQUESTED);
}
