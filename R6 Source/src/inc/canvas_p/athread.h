#ifndef ATHREAD_H
#define ATHREAD_H

#include <OS.h>

/*
	Interface: BRunnable
	
	A BRunnable is an object that does the actual work
	of a thread.  It is handed to a BThread's constructor which
	then takes responsibility for deleting it.
	
	The BThread object will call the Prepare() method of the BRunnable.
	If it returns 'true', then it will then call the Run() method.  If it
	returns 'false', then the Run() method will not be called and the thread
	will exit.

	A BThread Object can call the Quit() method if it wants to give the
	runnable a chance to shut down reasonably.  This is a better method than
	simply terminating the thread if at all possible.
*/

class BRunnable
{
public:
	virtual 	~BRunnable();
	
	virtual bool		Prepare();
	virtual status_t	Run()=0;
	virtual status_t	Quit();
	
protected:
private:
};

/*
	Interface: BThread
	
	A thread object that implements threading type stuff.  You pass
	a BRunnable in the constructor and the thread object will
	call the ::Run() method of the thread handler.

	Using this mechanism, you don't have to inherit from BThread
	to get anything done.  You simply inherit from BRunnable instead.
*/

class BThread
{
public:


			BThread(BRunnable *, const char *name, const bool autoStart=true, const long aPriority = B_NORMAL_PRIORITY);
	virtual	~BThread();
			
	status_t	GetInfo(thread_info *info);
	status_t	Terminate();					// Terminate the thread immediately
	status_t	Resume();						// Resume a suspended thread
	status_t	Suspend();						// Suspend a running thread
	status_t	Quit();							// Give the thread a chance to quit reasonably
	
	status_t	Rename(const char *newName);
	status_t	SetPriority(long newPriority);
	status_t	WaitForExit (long *thread_return_value);
	
	thread_id	ThreadID() {return fThreadID;};
	
	bool	IsRunning(){return fIsRunning;};
	
	// This class method is called within the constructor.
	// is the routine the thread actually runs.  It is passed
	// the handler as the only argument, and it will call the
	// ::Run() method of the handler.
	static	status_t	CallThreadHandler(void *data);
	
protected:
	BRunnable	*fHandler;
	bool		fIsRunning;
	thread_id	fThreadID;
	
private:

};

//=================================================
//=================================================


#endif
