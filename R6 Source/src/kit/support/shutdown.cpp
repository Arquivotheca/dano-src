
//	This file contains routines to shut down and restart the 
//	media server. They can not be in libmedia, because you may 
//	want to call them in order to get at devices without using 
//	the media kit...

#include <MediaDefs.h>
#include <Roster.h>
#include <Messenger.h>
#include <Message.h>
#include <OS.h>
#include <stdio.h>
#include <signal.h>

static const char ADDON_SERVER_SIGNATURE[] = "application/x-vnd.Be.addon-host";
static const char AUDIO_SERVER_SIGNATURE[] = "application/x-vnd.Be-AUSV";
static const char MEDIA_SERVER_SIGNATURE[] = "application/x-vnd.Be.media-server";

static bool
check_error(
	status_t,
	BMessenger & msgr)
{
	team_info ti;
	if (msgr.IsValid() && !get_team_info(msgr.Team(), &ti))
	{
		fprintf(stderr, "shutdown error: %s (team %ld) is alive\n", 
			ti.args, ti.team);
		return true;
	}
	return false;
}

status_t
shutdown_media_server(
	bigtime_t wait,
	bool (*progress)(int stage, const char * message, void * cookie),
	void * cookie)
{
	bigtime_t timeout = wait;
	bigtime_t then = system_time()+wait;
	status_t err = B_OK;
	BMessage reply;
	BMessage quit(B_QUIT_REQUESTED);
	quit.AddBool("be:_user_request", true);
	
	if (wait == B_INFINITE_TIMEOUT)
	{
		timeout = 20000000;
		then = system_time() + timeout;
	}

	if (be_roster->IsRunning(MEDIA_SERVER_SIGNATURE))
	{
		BMessenger msgr(MEDIA_SERVER_SIGNATURE);
		if (progress)
		{
			(*progress)(10, "Telling media_server to quit.", cookie);
		}
		err = msgr.SendMessage(&quit, &reply, timeout);
		if(err < B_OK)
			goto killer;
		if (!reply.FindInt32("error", &err) && err < 0)
			goto killer;
	}
	if (be_roster->IsRunning(ADDON_SERVER_SIGNATURE))
	{
		BMessenger msgr(ADDON_SERVER_SIGNATURE);
		if (progress)
		{
			(*progress)(20, "Telling media_addon_server to quit.", cookie);
		}
		err = msgr.SendMessage(&quit, &reply);
		if (check_error(err, msgr))
			goto killer;
	}
	if (be_roster->IsRunning(AUDIO_SERVER_SIGNATURE))
	{
		BMessenger msgr(AUDIO_SERVER_SIGNATURE);
		if (progress)
		{
			(*progress)(30, "Telling audio_server to quit.", cookie);
		}
		err = msgr.SendMessage(&quit, &reply);
		if (check_error(err, msgr))
			goto killer;
	}
	while (true)
	{
		if (!be_roster->IsRunning(MEDIA_SERVER_SIGNATURE))
			break;
		if (progress)
		{
			(*progress)(40, "Waiting for media_server to quit.", cookie);
		}
		snooze(200000);
		if (system_time() > then)
		{
			err = B_TIMED_OUT;
			goto killer;
		}
	}
	while (true)
	{
		if (!be_roster->IsRunning(ADDON_SERVER_SIGNATURE))
			break;
		snooze(200000);
		if (progress)
		{
			(*progress)(50, "Waiting for media_addon_server to quit.", cookie);
		}
		snooze(200000);
		if (system_time() > then)
		{
			err = B_TIMED_OUT;
			goto killer;
		}
	}
	while (true)
	{
		if (!be_roster->IsRunning(AUDIO_SERVER_SIGNATURE))
			break;
		if (progress)
		{
			(*progress)(60, "Waiting for audio_server to quit.", cookie);
		}
		snooze(200000);
		if (system_time() > then)
		{
			err = B_TIMED_OUT;
			goto killer;
		}
	}
killer:
	if (wait == B_INFINITE_TIMEOUT)
	{
		if (progress)
		{
			(*progress)(70, "Cleaning Up.", cookie);
		}
		snooze(2000000);
	
		team_id team;
		team = be_roster->TeamFor(MEDIA_SERVER_SIGNATURE);
		err = send_signal(team, SIGKILL);

		team = be_roster->TeamFor(ADDON_SERVER_SIGNATURE);
		err = send_signal(team, SIGKILL);

		team = be_roster->TeamFor(AUDIO_SERVER_SIGNATURE);
		err = send_signal(team, SIGKILL);

	}
	
	if (err < B_OK)
		return err;
		
	if (progress)
	{
		(*progress)(100, "Done Shutting Down.", cookie);
	}
	snooze(1000000);
	return B_OK;
}


status_t
launch_media_server(uint32)
{
	if (be_roster->IsRunning(MEDIA_SERVER_SIGNATURE) ||
			be_roster->IsRunning(ADDON_SERVER_SIGNATURE))
	{
		return B_ALREADY_RUNNING;
	}
	status_t err = be_roster->Launch(MEDIA_SERVER_SIGNATURE);
	if (err < B_OK) return err;
	int tries = 0;
	err = B_MEDIA_SYSTEM_FAILURE;
	for (tries = 1; tries < 15; tries++)
	{
		snooze(tries * 100000);
		BMessenger msgr(ADDON_SERVER_SIGNATURE);
		if (msgr.IsValid())
		{
			//	do a turnaround to make sure it's serving messages
			BMessage foo(1);
			BMessage bar;
			msgr.SendMessage(&foo, &bar, 2000000, 2000000);
			err = B_OK;
			break;
		}
	}
	return err;
}

