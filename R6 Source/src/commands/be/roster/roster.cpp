/******************************************************************************

	File:			roster.cpp

	Description:	Roster dumping utility.

	Written by:		Peter Potrebic & Steve Horowitz

	Copyright 1995-96, Be Incorporated. All Rights Reserved.

*****************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <Debug.h>

#include <Application.h>
#include <Path.h>
#include <Entry.h>
#include <Roster.h>
#include <List.h>

//====================================================================

class TRosterApp : public BApplication {

public:
					TRosterApp();
virtual		void	ArgvReceived(int32 argc, char** argv);
virtual		void	ReadyToRun();

			void	Do();

			int		fArgc;
			char	**fArgv;
};

int main()
{	
	BApplication* myApp = new TRosterApp();
	myApp->Run();

	delete myApp;
	return B_NO_ERROR;
}

//--------------------------------------------------------------------

TRosterApp::TRosterApp()
		  :BApplication("application/x-vnd.Be-cmd-RSTR")
{
	fArgc = 0;
}

//--------------------------------------------------------------------

void TRosterApp::ArgvReceived(int32 argc, char** argv)
{
	int		i;

	fArgc = argc;
	fArgv = (char **) malloc((argc+1) * sizeof(void *));
	for(i=0; i<argc; i++) {
		fArgv[i] = (char *) malloc(strlen(argv[i]) + 1);
		strcpy(fArgv[i], argv[i]);
	}
	fArgv[i] = NULL;
}

void TRosterApp::Do()
{
	BList		appList;
	app_info	info;
	team_id		team;
	char		sigStr[5];
	BEntry		entry;
	BPath		path;
	const char	*name;
	
	be_roster->GetAppList(&appList);

	printf("   %32s %6s %4s %4s %5s\n",
		"path", "thread", "team", "port", "flags");
	printf("-- -------------------------------- ------ ---- ---- -----\n");

	for (int i = 0; i < appList.CountItems(); i++) {
		team = (team_id)appList.ItemAt(i);
		if (be_roster->GetRunningAppInfo(team, &info) == B_NO_ERROR) {
			name = NULL;
			entry.SetTo(&info.ref);
			entry.GetPath(&path);
			name = path.Path();
			printf("%2d %32s %6d %4d %4d %4x (%s)\n",
				   i, name ? name : "<null>", info.thread, team,
				   info.port, info.flags, info.signature);
		}
	}
}

//--------------------------------------------------------------------

void TRosterApp::ReadyToRun()
{
	Do();
	PostMessage(B_QUIT_REQUESTED);
}
