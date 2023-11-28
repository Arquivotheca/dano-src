#include "BlanketApp.h"
#include "Settings.h"
#include "SetupWindow.h"
#include "ModuleRoster.h"
#include "ssdefs.h"

#include <Message.h>
#include <string.h>
#include <stdio.h>

BlanketApp::BlanketApp()
 : BApplication("application/x-vnd.Be.screensaver")
{
	// load settings, initialize module roster
	InitSettings();
	DefaultSettings();	// initialize default values
	SaveSettings();

	// start module roster
	roster = new ModuleRoster();
	roster->Run();
}

bool BlanketApp::QuitRequested()
{
	if(roster->Lock())
		roster->Quit();

	return BApplication::QuitRequested();
}

void BlanketApp::ReadyToRun()
{
	BRect lastframe;

	BMessage *m = AcquireSettings();
	m->FindRect(kWindowFrame, &lastframe);
	ReleaseSettings();

	// build ui
	setupwin = new SetupWindow(lastframe, BMessenger(roster));
	setupwin->Show();

	BApplication::ReadyToRun();
}
