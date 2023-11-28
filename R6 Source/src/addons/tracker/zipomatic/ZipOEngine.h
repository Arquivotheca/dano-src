#ifndef __ZIP_O_ENGINE__
#define __ZIP_O_ENGINE__

#include <Handler.h>
#include <Locker.h>
#include <Message.h>
#include <Messenger.h>
#include <OS.h>
#include <String.h>

const uint32 kDoneOK = 'dnok';
const uint32 kDoneError = 'dnEr';

class ZipOEngine : public BHandler {
public:
	ZipOEngine();
	virtual ~ZipOEngine();

	enum EngineState {
		kReady,
		kBusy,
		kPaused,
		kDone,
		kStopped,
		kError
	};
	
	EngineState State() const;
	void SetTo(const BMessage *);

	void Start();
	void Pause();
	void Resume();
	void Stop();

	bool AskToStop();

	void SetOwner(BHandler *);

	const char *ArchivingAs() const
		{ return archiveName.String(); }

	// following two calls protect the status string with a locker
	// to ensure data integrity
	void SetCurrentStatus(const char *, const char *);
	bool GetCurrentStatus(BString &stateString, BString &stateProgressString);
		// returns true if changed from previous
protected:
	status_t BuildArgLine();

	static status_t RunBinder(void *);
	void Run();

private:
	BMessage refsToZip;
	thread_id thread;
	thread_id zipThread;
	EngineState state;
	BString argLine;
	BString archiveDir;
	BString archiveName;
	BMessenger owner;
	BString currentProgressString;
	BString currentStateString;
	BLocker statusLock;
};


#endif
