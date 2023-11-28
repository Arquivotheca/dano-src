//========================================================================
//	MThread.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MTHREAD_H
#define _MTHREAD_H

#include <Locker.h>


class MThread
{
public:
 								MThread(
									const char *	inThreadName = "mthread",
									int32			inPriority = B_NORMAL_PRIORITY); 
		virtual					~MThread();

		status_t				Run();
		void					Cancel();
		bool					Cancelled();
		void					Kill();
		void					Lock();
		void					Unlock();
		thread_id				Thread();

protected:

virtual	status_t				Execute();
virtual	void					LastCall();

private:
	
		thread_id				fThread;
		bool					fCanceled;
		BLocker					fLock;


		static status_t			ThreadEntry(
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
