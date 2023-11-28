//
// BThread
//
//   An object wrapper around a kernel thread.  To use BThread, create a
//   BRunnable subclass that does work inside of it's Run() method, and
//   create an instance of BThread with an instance of your BRunnable.  Then
//   call Resume() on this BThread instance.
//
// by Nathan Schrenk (nschrenk@be.com)

#ifndef THREAD_H_
#define THREAD_H_

#include <Locker.h>
#include <map>


class BRunnable {
public:
	virtual status_t Run() = 0;
};


class BThread {
public:
				BThread(BRunnable *runner, const char *name = B_EMPTY_STRING,
					int32 priority = B_NORMAL_PRIORITY);
				~BThread();
				
	status_t	InitCheck(); // returns B_OK, B_NO_MORE_THREADS, or B_NO_MEMORY
//	status_t	Start();
	status_t	Kill();
	status_t	Suspend();
	status_t	Resume();
	status_t	Wait();
	
//	bigtime_t	EstimatedMaxSchedulingLatency();
	thread_id	ThreadId();
	int32		Priority();
	status_t	SetPriority(int32 priority); // returns the previous priority or
											 // B_BAD_THREAD_ID
	const char*	Name();		// returns the thread's name, or NULL if there is a problem
	status_t	SetName(const char *name);
	static BThread*	CurrentThread();	// returns the BThread object associated with the
										// running thread
	
private:
	static int32	Run(void *castToBThread);

	thread_id	fId;
	BRunnable	*fRunner;
	bool		fExited;
	status_t	fExitValue;
	// static state for keeping track of BThreads
	static BLocker	sLock;
	static map<thread_id, BThread *>	sThreadMap;
};

#endif // THREAD_H_
