//========================================================================
//	MThread.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include <Locker.h>

#ifndef _MTHREAD_H
#define _MTHREAD_H


class MThread 
{
public:
 								MThread(
									const char *	inThreadName = "mthread",
									long			inPriority = B_NORMAL_PRIORITY); 
		virtual					~MThread();

		long					Run();
		void					Cancel();
		bool					Cancelled();
		void					Kill();
		void					Lock();
		void					Unlock();
		thread_id				Thread();

protected:

virtual	long					Execute();
virtual	void					LastCall(long);

private:
	
		thread_id				fThread;
		bool					fCanceled;
		BLocker					fLock;


		static long				ThreadEntry(
									MThread*	inObject);
};

inline void	MThread::Lock()
{
	fLock.Lock();
}
inline void MThread::Unlock()
{
	fLock.Unlock();
}
inline thread_id MThread::Thread()
{
	return fThread;
}

#endif
