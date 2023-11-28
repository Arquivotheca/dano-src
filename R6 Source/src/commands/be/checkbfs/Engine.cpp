
#include <Alert.h>
#include <Autolock.h>
#include <Entry.h>
#include <Debug.h>
#include <Handler.h>
#include <Looper.h>
#include <Path.h>

#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "Engine.h"

Engine::Engine()
	:	state(kReady),
		statusLock("statusLock", true)
{
}

void 
Engine::SetOwner(BHandler *handler)
{
	BMessenger newOwner(handler, handler->Looper());
	owner = newOwner;
}


Engine::~Engine()
{
	Stop();
}

Engine::EngineState 
Engine::State() const
{
	return state;
}

void 
Engine::SetCurrentStatus(const char *text, const char *progress)
{
	BAutolock lock(statusLock);
	currentStateString = text;
	currentProgressString = progress;
}

bool 
Engine::GetCurrentStatus(BString &stateString, BString &stateProgressString)
{
	BAutolock lock(statusLock);
	if (currentStateString == stateString
		&& currentProgressString == stateProgressString)
		return false;
	
	stateString = currentStateString;
	stateProgressString = currentProgressString;
	return true;
}

void 
Engine::Start(const char *volume)
{
	SetCurrentStatus("Starting", "");
	argLine = "/bin/chkbfs ";
	argLine << '/' << volume;
	
	printf("argLine %s\n", argLine.String());

	thread = spawn_thread(Engine::RunBinder, "CheckBFS",
		B_NORMAL_PRIORITY, this);
	resume_thread(thread);
}

void 
Engine::Pause()
{
	DEBUG_ONLY( status_t result = )
		suspend_thread(toolThread);

	PRINT(("paused %s \n", strerror(result)));
	SetCurrentStatus("Paused", "");
	state = kPaused;
}

void 
Engine::Resume()
{
	DEBUG_ONLY( status_t result = )
		resume_thread(toolThread);

	PRINT(("resumed %s \n", strerror(result)));
	SetCurrentStatus("", "");
	state = kBusy;
}


void 
Engine::Stop()
{
	bool shouldStop = (state == kPaused);
	state = kStopped;

	if (shouldStop)
		// if we are paused here, sending a SIGNIT will crash the kernel
		send_signal(toolThread, SIGCONT);

	DEBUG_ONLY( status_t result = )
		send_signal(toolThread, SIGINT);
		// send a SIGINT to correctly exit zip. Doing a kill_thread would
		// stop it abruptly, leaving garbage behind
		
	PRINT(("stopped %s \n", strerror(result)));
	SetCurrentStatus("Stopped", "");
	state = kStopped;
}

bool 
Engine::AskToStop()
{
	return (new BAlert("",
		"Are you sure you want to stop running CheckBFS?",
		"Stop", "Continue", NULL, B_WIDTH_AS_USUAL,
		B_EMPTY_ALERT))->Go() == 0;
}


status_t 
Engine::RunBinder(void *castToThis)
{
	((Engine *)castToThis)->Run();
	return B_OK;
}

extern char	**environ;

void 
Engine::Run()
{
	int p[2];
	FILE *file;
	
	if (pipe(p) < 0) {
		printf("failed creating pipe\n");
		return;
	}
	
	state = kBusy;
	const char *argv[4] = { "/bin/sh", "-c", argLine.String(), 0};
    if ((toolThread = fork()) == 0) {
		// child

		fflush(stdout);
		close(1);
		if (dup(p[1]) < 0)
			printf("error opening for reading\n");
			
		close(p[0]);
		close(p[1]);

		setpgid(0, 0);
//		chdir(archiveDir.String());
		execve(argv[0], (char *const *)argv, environ);
	} else if (toolThread > 0) {
		close(p[1]);
		file = fdopen(p[0], "r");
		if (!file) {
			close(p[0]);
			close(p[1]);
			printf("error opening for writing\n");
		}
		
		BString buffer;
		for (;;) {
			int ch = fgetc(file);
			if (ch == EOF)
				break;

			if (ch < ' ') {
				printf("%s\n", buffer.String());
				SetCurrentStatus("", buffer.String());
				buffer = "";
			} else
				buffer << (char)ch;
		}
			
		if (state == kStopped)
			SetCurrentStatus("Stopped", "");
		else
			SetCurrentStatus("CheckBFS completed OK", "");


	} else {
		close(p[0]);
		close(p[1]);
		printf("error forking\n");
		state = kError;
		return;
	}

	state = kDone;
	owner.SendMessage(kDoneOK);
}


