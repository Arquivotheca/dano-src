/* Conductor.h */

#include "NetPositive.h"
#include <app/Message.h>
#include <app/Application.h>
#include <kernel/OS.h>
#include <kernel/image.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <support/String.h>
#include <app/MessageQueue.h>


#define PRINT_HISTORY 0
#define OPERA_SIG "application/x-vnd.operasoftware-opera"
#define OPERA_PATH "/boot/apps/Opera/Opera"

const uint32 OPERA_PRINT = 'OPRN';

class Conductor : public BApplication
{
	public:
						Conductor();
						~Conductor();

		void			ArgvReceived(int32 argc, char **argv);
		static int32	BeginConcert(void *conductor);
		void			ReadyToRun();
	
	private:
		int32			Conduct();
		thread_id		fConcert;
		void			GetMessenger();
		
		bool			fConduct;
		
		BMessenger		fMsgr;		
		
		bool			fQuitting;
		BString			fStartup;
		int32			fState;

		BMessageQueue 	fQ;
		
		void			TellOpera(BMessage *msg);
		void			GoTo(char *url);
		void			GoHome();
		void			GoStart();
		void			GoForward();
		void			GoBackward();
		void			Reload();
		void			Stop();
		void			Print();

};

Conductor::Conductor() :
	BApplication("application/x-vnd.Be-Conductor"),
	fConduct(true),
	fConcert(-1),
	fQuitting(false),
	fStartup(B_EMPTY_STRING)
{
	fState = open("/dev/misc/opera-state", O_RDWR);
}


Conductor::~Conductor()
{
//	printf("TellOpera quitting\n");
	fQuitting = true;

	status_t status = B_OK;
	wait_for_thread(fConcert, &status);

#if PRINT_HISTORY
	FILE *history = fopen("/dev/misc/opera-state", "r");
	if (history)
	{
		char buf[1024];
		while((fgets(buf, 1024, history) != NULL))
			printf(buf);
		fclose(history);
	}
#endif

	close(fState);
}

void 
Conductor::ReadyToRun()
{
//	printf("TellOpera::ReadyToRun: fStartup.String(): %s\n", fStartup.String());
	// find the startup page
	if (fStartup == NULL)
	{
//		printf("go to the default page\n");
		// as we have not been told to go anywhere else, go to the default home page
		FILE *file = fopen("/boot/custom/content/startup_page", "r");
		char * startup = fStartup.LockBuffer(B_PATH_NAME_LENGTH + 1);
		fgets(startup, B_PATH_NAME_LENGTH + 1, file);
		fStartup = fStartup.UnlockBuffer();
		fclose(file);
		
		fStartup.Prepend("/boot/custom/content/");
//		printf("fStartup: %s\n", fStartup.String());
	}
//	else printf("going to %s\n", fStartup.String());
	
	if (fConduct && !fMsgr.IsValid())
	{
		fConcert = spawn_thread(BeginConcert, "Concert", B_URGENT_DISPLAY_PRIORITY, this);
		resume_thread(fConcert);
		GetMessenger();
	}
	
	// send any queued messages
	BMessage *msg = NULL;
	while ((msg = fQ.NextMessage()))
	{
		if (fMsgr.IsValid())
			fMsgr.SendMessage(msg);
		delete msg;
		snooze(1000000);
	}
}

void 
Conductor::GetMessenger()
{
	if (fMsgr.IsValid())
		return;
		
	fMsgr = BMessenger(OPERA_SIG);
	
	while (!fMsgr.IsValid())
	{
//		printf("looping to get valid messenger\n");
		snooze(100000);
		fMsgr = BMessenger(OPERA_SIG);
	}
}

void 
Conductor::ArgvReceived(int32 argc, char **argv)
{
	for (int32 i = 1; i < argc; i++)
	{
//		printf("argv[i]: %s\n", argv[i]);
		if (!strcmp(argv[i], "-goto")) {
			if (!argv[i + 1])
				printf("-goto needs argument\n");
			else {
				GoTo(argv[++i]);
			}
		}
		else if (!strcmp(argv[i], "-goforward"))
			GoForward();
		else if (!strcmp(argv[i], "-gobackward"))
			GoBackward();
		else if (!strcmp(argv[i], "-print"))
			Print();
		else if (!strcmp(argv[i], "-gohome"))
			GoHome();
		else if (!strcmp(argv[i], "-gostart"))
			GoStart();
		else if (!strcmp(argv[i], "-reload"))
			Reload();
		else if (!strcmp(argv[i], "-nolaunch"))
			fConduct = false;
		else
			printf("unknown argument: %s\n", argv[i]);
	}
}

int32 
Conductor::BeginConcert(void *conductor)
{
	return ((Conductor *)conductor)->Conduct();
}

int32
Conductor::Conduct()
{
	status_t signal = B_ERROR;
	thread_id opera = -1;
	
	char *args[3];
	args[0] = OPERA_PATH;
	args[1] = fStartup.String();
	args[2] = NULL;			
	int32 arg_count = 2;
	
	
	while (!fQuitting && signal != B_OK)
	{
		opera = load_image(arg_count, args, environ);
		if (opera < 0)
			break;			

		wait_for_thread(opera, &signal);
//		printf("signal: %d\n", signal);
		if (signal != B_OK)
		{
			#if PRINT_HISTORY
				FILE *history = fopen("/dev/misc/opera-state", "r");
				if (history)
				{
					char buf[1024];
					while((fgets(buf, 1024, history) != NULL))
						printf(buf);
				
					fseek(history, 0L, SEEK_SET);
					fclose(history);
				}
			#endif
//			printf("bad signal (%d): relaunching Opera\n", signal);
			args[1] = NULL;
			arg_count = 1;
		}
//		else printf("signal okay - let's quit\n");
	}
	
	if (!fQuitting)
		PostMessage(B_QUIT_REQUESTED);

	return 0;
}

void 
Conductor::TellOpera(BMessage *msg)
{
	if (IsLaunching())
	{
		BMessage *toQ = new BMessage(*msg);
		fQ.AddMessage(toQ);
	}
	else
	{
		if (!fMsgr.IsValid())
			GetMessenger();
		fMsgr.SendMessage(msg);
	}
}

void 
Conductor::GoTo(char *url)
{
//	printf("Conductor GoTo: %s IsLaunching: %d\n", url, IsLaunching());
	if (IsLaunching())
	{
		fStartup = url;	
	}
	else {
		BMessage goTo(B_NETPOSITIVE_OPEN_URL);
		goTo.AddString("be:url", url);
		TellOpera(&goTo);
	}
}

void 
Conductor::GoStart()
{
//	printf("Conductor GoStart\n");
	BMessage goTo(B_NETPOSITIVE_OPEN_URL);
	goTo.AddString("be:url", fStartup.String());
	TellOpera(&goTo);
}


void 
Conductor::GoForward()
{
//	printf("Conductor GoForward\n");
	BMessage forward(B_NETPOSITIVE_FORWARD);
	TellOpera(&forward);
}

void 
Conductor::GoBackward()
{
//	printf("Conductor GoBackward\n");
	BMessage backward(B_NETPOSITIVE_BACK);
	TellOpera(&backward);
}

void 
Conductor::Reload()
{
//	printf("Conductor Reload\n");
	BMessage backward(B_NETPOSITIVE_RELOAD);
	TellOpera(&backward);
}

void 
Conductor::Stop()
{
//	printf("Conductor Stop\n");
	BMessage backward(B_NETPOSITIVE_STOP);
	TellOpera(&backward);
}

void 
Conductor::GoHome()
{
//	printf("Conductor GoHome\n");
	BMessage backward(B_NETPOSITIVE_HOME);
	TellOpera(&backward);
}

void 
Conductor::Print()
{
//	printf("Conductor Print\n");
	BMessage print(OPERA_PRINT);
	TellOpera(&print);
}

int main()
{
	Conductor app;
	app.Run();
	return 0;
}