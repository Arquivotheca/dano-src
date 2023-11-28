//
//	NetworkingCore.cpp
//
//	russ 5/21/98
//  duncan 9/27/99
// 
//	(c) 1997-98 Be, Inc. All rights reserved

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <Application.h>
#include <File.h>
#include <Alert.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Roster.h>

#include <image.h>

#include "TimedAlert.h"
#include "Resource.h"
#include "netconfig.h"
#include "net_settings.h" 
#include "NetworkingCore.h"
#include "NetDevScan.h"

NetworkingCore::NetworkingCore()
{		
	//	build a global list of all the add ons
	fNetDevScanList = new TNetDevScanList();

	// If this file is modified let this app know.
	// This is important when gettig stuff from 
	// DHCP.
	settings.SetWatchLooper(this);
	settings.WatchSettingsFileNode();

	Run();
}

NetworkingCore::~NetworkingCore()
{
	TryToSave(true);
	delete fNetDevScanList;
}

static int
roster_quit(char *sig)
{
	app_info info;
	if (be_roster->GetAppInfo(sig, &info) < B_NO_ERROR) {
		/*
		 * Already quit
		 */
		return (1);
	}

	BMessenger app((char*)0, info.team);
	app.SendMessage(B_QUIT_REQUESTED);
	
	int32 i;
	for (i = 0; i < 10; i++) {
		if (!be_roster->IsRunning(sig)) {
			break;
		}
		sleep(1);
		app.SendMessage(B_QUIT_REQUESTED);
	}
	//
	// it's possible the net_server is wedged so bad that the above doesn't work.
	// here we get medevial on it
	//
	
	if ((i >= 10) && (info.team >= 0)) {
		kill_team(info.team);
	}	
	return (0);
}

// ************************************************************************** //

static void RestartNetworking()
{
	roster_quit("application/x-vnd.Be-dhcpc");
	roster_quit("application/x-vnd.Be-POST");		
	
	BPath script_path;
	find_directory (B_BEOS_BOOT_DIRECTORY, &script_path);
	script_path.Append ("Netscript");
	roster_quit("application/x-vnd.Be-NETS");
	sleep(5);	// let things settle down a bit
	
	char *argv[3];
	argv[0] = "/bin/sh";
	argv[1] = (char *)script_path.Path();
	argv[2] = NULL;
	
	resume_thread(load_image(2, (const char**)argv,
		 (const char**)environ));
	
	sleep(5);	// let things settle down a bit
}

static int32 LaunchThread()
{
  	int32 tid = spawn_thread((thread_entry)RestartNetworking, "network_restart",
		B_NORMAL_PRIORITY,NULL);
	resume_thread(tid);
	
	return tid;
}

void NetworkingCore::Restart()
{
	BAlert *alert=NULL;

	if (settings.Save() != kSuccess) {
	 		alert = new BAlert("Save failed", "Unable to save file", 
		   "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->Go();
	}

	//	a separate thread handles the network restarting so
	//	that the window will not be completely blocked
	//	the confirm window, waits for approximately n seconds
	//	and then frees itself		 	
	LaunchThread();

	TTimedAlert	*wind = new TTimedAlert(10000000, 0, "", "Restarting networking\nPlease wait" B_UTF8_ELLIPSIS, 0, 0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	wind->Go();		
}

bool NetworkingCore::TryToSave(bool ask)
{
	BAlert *alert;
	bool doit = false;

	if(settings.IsDirty())
	{
		if(ask)
		{
			alert = new BAlert("Save", "Save changes before quitting?", 
							   "Don't Save", "Cancel", "Save",
							   B_WIDTH_FROM_WIDEST);
			switch (alert->Go()) {
			case 0:
				doit = false;	 /* quit anyway */
				break;
			case 1:
				return false;	/* cancel */
			case 2:
				doit = true;	/* save changes */
				break;
			}
		}
		else
		{
			doit = true;
		}
		if(doit)
		{
			if(settings.Save() != kSuccess)
			{
				alert = new BAlert("Save failed", "Unable to save file", 
								   "OK", NULL, NULL, B_WIDTH_AS_USUAL,
								   B_WARNING_ALERT);
				alert->Go();
				return false;
			}
			else
			{
				 // Eventually, we should ask to reboot here
				alert = new BAlert("Save ok",
								   "Restart networking?", "Don't Restart", 
								   "Restart", NULL, B_WIDTH_FROM_WIDEST);
				if(alert->Go() == 1)
					Restart();
			}
		}
	}

	return true;
}

void NetworkingCore::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case MSG_RESTARTNETWORKING:			
			Restart();
			break;

		case MSG_SAVE :
			TryToSave(false);
			break;

		case MSG_REVERT:
		case B_NODE_MONITOR:
			// A reload was requested or
			// the settings file data has changed through DHCP (node monitoring).
			// Update the controls.
			settings.ReloadData();
			break;

		default:
			BLooper::MessageReceived(msg);
			break;
	}
}
