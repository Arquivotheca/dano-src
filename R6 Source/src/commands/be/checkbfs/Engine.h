#ifndef __ENGINE__
#define __ENGINE__

#include <Handler.h>
#include <Locker.h>
#include <Message.h>
#include <Messenger.h>
#include <OS.h>
#include <String.h>

const uint32 kDoneOK = 'dnok';
const uint32 kDoneError = 'dnEr';

class Engine : public BHandler {
public:
	Engine();
	virtual ~Engine();

	enum EngineState {
		kReady,
		kBusy,
		kPaused,
		kDone,
		kStopped,
		kError
	};
	
	EngineState State() const;

	void Start(const char *volume);
	void Pause();
	void Resume();
	void Stop();

	bool AskToStop();

	void SetOwner(BHandler *);

	// following two calls protect the status string with a locker
	// to ensure data integrity
	void SetCurrentStatus(const char *, const char *);
	bool GetCurrentStatus(BString &stateString, BString &stateProgressString);
		// returns true if changed from previous
	
protected:
	static status_t RunBinder(void *);
	void Run();

private:
	thread_id thread;
	thread_id toolThread;
	EngineState state;

	BString argLine;

	BMessenger owner;
	BString currentProgressString;
	BString currentStateString;
	BLocker statusLock;
};


#endif
