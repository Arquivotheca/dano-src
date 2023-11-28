/*	$Id: DDebugApp.cpp,v 1.8 1999/05/11 21:31:03 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 04/07/98 13:39:21
*/

#include "bdb.h"
#include "DDebugApp.h"
#include "DTeamWindow.h"
#include "DMessages.h"
#include "DDwarfSymFile.h"
#include "DMainWindow.h"
#include "DAboutWindow.h"

#include <Roster.h>
#include <Screen.h>

#include <syslog.h>

ptr_t gThrowPC;

BFile gAppFile;

DDebugApp::DDebugApp()
	: BApplication("application/x-vnd.Be-debugger")
{
	try
	{
		gPrefs = new HPreferences("Debugger.settings");
		gPrefs->ReadPrefFile();

		fPanel = new BFilePanel;
		
		app_info ai;
		GetAppInfo(&ai);
		
		BEntry entry(&ai.ref);
		
		FailOSErr(gAppFile.SetTo(&entry, B_READ_ONLY));
		
		InitSelectedMap();
		
		DMainWindow::Instance();
	}
	catch (HErr& e)
	{
		e.DoError();
		exit(1);
	}
} /* DDebugApp::DDebugApp */

DDebugApp::~DDebugApp()
{
} /* DDebugApp::~DDebugApp */

bool DDebugApp::QuitRequested()
{
	DTeamWindow *tmw;
	uint32 cookie = 0;
	
	while ((tmw = DTeamWindow::GetNextTeamWindow(cookie)) != NULL)
	{
		if (tmw->Lock())
		{
			if (tmw->QuitRequested())
				tmw->Quit();
			else
			{
				tmw->Unlock();
				return false;
			}
		}
	}
	
	DMainWindow *w = DMainWindow::Instance();
	w->Lock();
	w->QuitRequested();
	w->Quit();
	
	return true;
} /* DDebugApp::QuitRequested */

void DDebugApp::ReadyToRun()
{
	if (CountWindows() == 1)
		fPanel->Show();
} /* DDebugApp::ReadyToRun */

void DDebugApp::MessageReceived(BMessage *msg)
{
	try
	{
		switch (msg->what)
		{
			case kMsgQuit:
				if (QuitRequested())
					Quit();
				break;
	
			case kMsgTeamWindowClosed:
			{
				uint32 cookie = 0;
				if (DTeamWindow::GetNextTeamWindow(cookie) == NULL)
					Quit();
				break;
			}

			case kMsgAbout:
				if (!gAboutWin)
				{
					BRect r(0, 0, 260, 125);
					BScreen screen;
					r.OffsetBy((screen.Frame().Width() - r.Width()) / 2, (screen.Frame().Height() - r.Height()) / 2);
					gAboutWin = new DAboutWindow(r);
				}
				gAboutWin->Show();
				break;

			default:
				BApplication::MessageReceived(msg);
		}
	}
	catch (HErr& e)
	{
		e.DoError();
	}
} /* DDebugApp::MessageReceived */

void DDebugApp::RefsReceived(BMessage *msg)
{
	try
	{
		entry_ref ref;
		int i = 0;
		
		while (msg->FindRef("refs", i++, &ref) == B_OK)
			new DTeamWindow(ref);
	}
	catch (HErr& e)
	{
		e.DoError();
		fPanel->Show();
	}
} /* DDebugApp::RefsReceived */

void DDebugApp::ArgvReceived(int32 argc, char **argv)
{
	try
	{
		if (argc > 2 && strcmp(argv[1], "-t") == 0)
		{
			// debug a running team:  bdb -t team_id
			char* dontCare = NULL;
			team_id id = strtoul(argv[2], &dontCare, 10);
			new DTeamWindow(id);
		}
		else if (argc > 3 && strcmp(argv[1], "-p") == 0)
		{
			// debug_server handoff:  bdb -p debug_port team_id
			char* dontCare = NULL;
			port_id port = strtoul(argv[2], &dontCare, 10);
			team_id id = strtoul(argv[3], &dontCare, 10);
			new DTeamWindow(id, port);
		}
		else if (!strcmp(argv[1], "-r") && (argc > 2))
		{
			// attach to a remote proxy:  bdb -r hostname [port]
			new DTeamWindow(argv[2], (argc > 3) ? atoi(argv[3]) : 5038);
		}
		else if (argc > 1)
		{
			// arguments are an app to run, and its args
			entry_ref ref;

			bool appFound = false;
			if (get_ref_for_path(argv[1], &ref) == B_OK)
			{
				BEntry tempEntry(&ref);
				if (tempEntry.Exists()) 
				{
					new DTeamWindow(ref, argc - 1, argv + 1);
					appFound = true;
				}
			}	

			if (appFound == false)
			{
				THROW(("Could not find %s", argv[1]));
			}
		}
	}
	catch (HErr& e)
	{
		// if something failed, such as trying to connect to the remote debug target
		// in the -r version, or inability to find the specified app to run in the no-flags
		// version, let the user know gracefully what happened.
		e.DoError();
	}
} // DDebugApp::ArgvReceived

#if DEBUG
void PrintFreeFDs()
{
	vector<int> fds;
	int fd;
	
	fd = open("/tmp/piep", O_CREAT | O_RDWR);
	
	while (fd > 0)
	{
		fds.push_back(fd);
		fd = dup(fd);
	}
	
//	printf("Number of file descriptors: %d\n", fds.size());
//	fflush(stdout);
	syslog(LOG_DEBUG, "Number of file descriptors: %d\n", fds.size());

	for (vector<int>::iterator i = fds.begin(); i != fds.end(); i++)
		close(*i);
} // PrintFreeFDs

long fd_stats(void */*p*/)
{
	while (true)
	{
		PrintFreeFDs();
		snooze(500000);
	}
}
#endif

extern "C" int _kset_fd_limit_(int num);

int
main (int argc, char** argv)
{
	if ((argc > 1) &&
		(!strcmp(argv[1], "-h") || !strcmp(argv[1], "-help") || !strcmp(argv[1], "--help")))
	{
		// Help text, then exit
		printf("Local usage summary:\n");
		printf("\tbdb\t\t\t: launch bdb alone\n");
		printf("\tbdb program [args...]\t: run 'program' under bdb\n");
		printf("\tbdb -t team_id\t\t: attach bdb to the given running team\n");
		printf("\nRemote usage summary:\n");
		printf("\tbdb -r remote-host [ip port]\t: attach to remote 'bdbproxy'\n");
		printf("\nThe location of the local copy of the remote machine's install tree is\n"
			"specified by the BDB_REMOTE_BIN_ROOT environment variable.  The IP\n"
			"port argument is optional; if omitted, it defaults to 5038, the default\n"
			"bdbproxy listen port.\n");
		printf("\nBy default, bdb will use TCP when communicating with bdbproxy.  To\n"
			"communicate with older builds that use UDP, set the BDB_USE_UDP\n"
			"environment variable to '1', 'true', or 'TRUE'.\n\n");
		return 1;
	}

	// would be nice if LOG_SERIAL would do what it suggested 
	openlog_team ("bdb", LOG_SERIAL, 0);
	
	// To debug file descriptor problems, uncomment following line...
	//	resume_thread(spawn_thread(fd_stats, "fd stats", B_NORMAL_PRIORITY, NULL));
	_kset_fd_limit_(1024);
	
	// DDebugApp needs to be around for the catch so that we can properly
	// display the alert.  (If DDebugApp::DDebugApp has a problem, it is
	// caught internally)
	DDebugApp app;
	try
	{
		app.Run();
		
		gPrefs->WritePrefFile();
	}
	catch (HErr& e)
	{
		e.DoError();
	}
	
	closelog();
	
	return 0;
} /* main */
