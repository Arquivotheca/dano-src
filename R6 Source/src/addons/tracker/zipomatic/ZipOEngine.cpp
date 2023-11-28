
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

#include "ZipOEngine.h"

const char *kZipOptions = "-ry";
	// -r - recursive
	// -y - don't follow symlinks

ZipOEngine::ZipOEngine()
	:	state(kReady),
		statusLock("statusLock", true),
		zipThread(-1)
{
}

void 
ZipOEngine::SetOwner(BHandler *handler)
{
	BMessenger newOwner(handler, handler->Looper());
	owner = newOwner;
}


ZipOEngine::~ZipOEngine()
{
	Stop();
}

ZipOEngine::EngineState 
ZipOEngine::State() const
{
	return state;
}

void 
ZipOEngine::SetCurrentStatus(const char *text, const char *progress)
{
	BAutolock lock(statusLock);
	currentStateString = text;
	currentProgressString = progress;
}

bool 
ZipOEngine::GetCurrentStatus(BString &stateString, BString &stateProgressString)
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
ZipOEngine::SetTo(const BMessage *message)
{
	refsToZip = *message;
}

void 
ZipOEngine::Start()
{
	SetCurrentStatus("Preparing to archive", "");
	if (BuildArgLine() == B_OK) {
		thread = spawn_thread(ZipOEngine::RunBinder, "ZipOMatic",
			B_NORMAL_PRIORITY, this);
		resume_thread(thread);
	}
}

void 
ZipOEngine::Pause()
{
	DEBUG_ONLY( status_t result = )
		suspend_thread(zipThread);

	PRINT(("paused %s \n", strerror(result)));
	SetCurrentStatus("Paused", "");
	state = kPaused;
}

void 
ZipOEngine::Resume()
{
	DEBUG_ONLY( status_t result = )
		resume_thread(zipThread);

	PRINT(("resumed %s \n", strerror(result)));
	SetCurrentStatus("", "");
	state = kBusy;
}


void 
ZipOEngine::Stop()
{
	bool shouldStop = (state == kPaused);
	state = kStopped;

	if (shouldStop)
		// if we are paused here, sending a SIGNIT will crash the kernel
		send_signal(zipThread, SIGCONT);

	DEBUG_ONLY( status_t result = )
		send_signal(zipThread, SIGINT);
		// send a SIGINT to correctly exit zip. Doing a kill_thread would
		// stop it abruptly, leaving garbage behind
		
	PRINT(("stopped %s \n", strerror(result)));
	SetCurrentStatus("Stopped", "");
	state = kStopped;
}

bool 
ZipOEngine::AskToStop()
{
	return (new BAlert("",
		"Are you sure you want to stop creating this archive?",
		"Stop", "Continue"))->Go() == 0;
}


status_t 
ZipOEngine::RunBinder(void *castToThis)
{
	((ZipOEngine *)castToThis)->Run();
	return B_OK;
}

status_t 
ZipOEngine::BuildArgLine()
{
	argLine = "";
	archiveName = "";

	int32 count = 0;
	type_code type;
	refsToZip.GetInfo("refs", &type, &count);

	entry_ref ref;
	if (refsToZip.FindRef("refs", &ref) != B_OK)
		return B_ENTRY_NOT_FOUND;

	
	if (count > 1)
		archiveName = "Archive.zip";
	else
		archiveName << ref.name << ".zip";
	
	archiveName.CharacterEscape("()[]*\"\'?^\\ \t\n\r", '\\');
	argLine << "zip " << kZipOptions << " " << archiveName;
	
	for (int32 index = 0;; index++) {
		entry_ref ref;
		if (refsToZip.FindRef("refs", index, &ref) != B_OK)
			break;
		BEntry entry(&ref);
		BEntry parent;
		entry.GetParent(&parent);
		BPath path(&parent);
		archiveDir = path.Path();
			// assumes all entries are in the same directory
		BString tmp;
		tmp.CharacterEscape(ref.name, "()[]*\"\'?^\\ \t\n\r", '\\');
		argLine << " " << tmp;
	}
	PRINT(("arg line %s\n", argLine.String()));
	PRINT(("archive dir %s\n", archiveDir.String()));

	return B_OK;
}

extern char	**environ;
void 
ZipOEngine::Run()
{
	int p[2];
	FILE *file;
	
	if (pipe(p) < 0) {
		printf("failed creating pipe\n");
		return;
	}
	
	state = kBusy;
	const char *argv[4] = { "/bin/sh", "-c", argLine.String(), 0};
    if ((zipThread = fork()) == 0) {
		// child

		fflush(stdout);
		close(1);
		if (dup(p[1]) < 0)
			printf("error opening for reading\n");
			
		close(p[0]);
		close(p[1]);

		setpgid(0, 0);
		chdir(archiveDir.String());
		execve(argv[0], (char *const *)argv, environ);
	} else if (zipThread > 0) {
		close(p[1]);
		file = fdopen(p[0], "r");
		if (!file) {
			close(p[0]);
			close(p[1]);
			printf("error opening for writing\n");
		}
		
		char buffer[1024];	
		while (fgets(buffer, sizeof(buffer), file) != 0) {
			// parse the output here

			BString tmp(buffer);
			int32 start = tmp.FindFirst(": ") + 1;
			int32 end = tmp.FindLast(" (");
			BString result;
			if (start > 0 && end > start)
				tmp.CopyInto(result, start, end - start);
			else
				result = tmp;
			SetCurrentStatus("Adding: ", result.String());
		}

		if (state == kStopped)
			SetCurrentStatus("Stopped", "");
		else
			SetCurrentStatus("Archive created OK", "");


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


