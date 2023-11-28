#include <stdio.h>
#include <OS.h>
#include "Gehnaphore.h"
#include "atomic.h"

static inline void init_gehnaphore_real(int32* value)
{
	*value = -1;
}

static inline void lock_gehnaphore_real(int32* value)
{
	int32 chainedThread=-1,myThread=find_thread(NULL),msg=-1;
	bool success = false;
	
	while (!success) {
		while (!success && (chainedThread == -1))
			success = cmpxchg32(value,&chainedThread,0);
		while (!success && (chainedThread != -1))
			success = cmpxchg32(value,&chainedThread,myThread);
	}

	do {
		if (chainedThread != -1)
			while ( (msg=receive_data(NULL,NULL,0)) == B_INTERRUPTED ) ;
		if ((chainedThread > 0) && (chainedThread != msg))
			while ( send_data(chainedThread,msg,NULL,0) == B_INTERRUPTED ) ;
	} while ((chainedThread > 0) && (chainedThread != msg));
}

static inline void unlock_gehnaphore_real(int32* value)
{
	int32 chainedThread=0,myThread=find_thread(NULL);
	bool success;
	
	success = cmpxchg32(value,&chainedThread,-1);
	if (!success && (chainedThread == myThread))
		success = cmpxchg32(value,&chainedThread,-1);
	
	if (!success) {
		while ( send_data(chainedThread,myThread,NULL,0) == B_INTERRUPTED ) ;
	}
}

#if !SUPPORTS_LOCK_DEBUG

//#pragma mark -

void init_gehnaphore(int32* value)
{
	init_gehnaphore_real(value);
}

void init_gehnaphore(int32* value, const char*)
{
	init_gehnaphore_real(value);
}

void fini_gehnaphore(int32* /*value*/)
{
}

void lock_gehnaphore(int32* value)
{
	lock_gehnaphore_real(value);
}

void unlock_gehnaphore(int32* value)
{
	unlock_gehnaphore_real(value);
}

// -----------------------------------------------------------------

#else

#include <support_p/DebugLock.h>
#include <stdlib.h>
#include <new>

using namespace B::Support;

// #pragma mark -

void init_gehnaphore(int32* value)
{
	if (!LockDebugLevel()) init_gehnaphore_real(value);
	else *value = reinterpret_cast<int32>(new(std::nothrow) DebugLock("Gehnaphore", value, "gehnaphore"));
}

void init_gehnaphore(int32* value, const char* name)
{
	if (!LockDebugLevel()) init_gehnaphore_real(value);
	else *value = reinterpret_cast<int32>(new(std::nothrow) DebugLock("Gehnaphore", value, name));
}

inline void fini_gehnaphore(int32* value)
{
	if (!LockDebugLevel()) ;
	else if (*value) reinterpret_cast<DebugLock*>(*value)->Delete();
}

void lock_gehnaphore(int32* value)
{
	if (!LockDebugLevel()) lock_gehnaphore_real(value);
	else if (*value) reinterpret_cast<DebugLock*>(*value)->Lock();
}

void unlock_gehnaphore(int32* value)
{
	if (!LockDebugLevel()) unlock_gehnaphore_real(value);
	else if (*value) reinterpret_cast<DebugLock*>(*value)->Unlock();
}

#endif

/***************************************************/
// #pragma mark -

Gehnaphore::Gehnaphore()
{
	init_gehnaphore(&m_lockValue);
}

Gehnaphore::Gehnaphore(const char *name)
{
	init_gehnaphore(&m_lockValue, name);
}

Gehnaphore::~Gehnaphore()
{
	fini_gehnaphore(&m_lockValue);
}

void Gehnaphore::Lock()
{
	lock_gehnaphore(&m_lockValue);
}

void Gehnaphore::Unlock()
{
	unlock_gehnaphore(&m_lockValue);
}

void Gehnaphore::Yield()
{
	if (m_lockValue != 0) {
		Unlock();
		Lock();
	};
};

/***************************************************/

// #pragma mark -
NestedGehnaphore::NestedGehnaphore()
{
	m_nesting = 0;
	m_owner = B_BAD_THREAD_ID;
}

NestedGehnaphore::~NestedGehnaphore()
{
}

void NestedGehnaphore::Lock()
{
	thread_id me = find_thread(NULL);
	if (me == m_owner) {
		m_nesting++;
		return;
	};

	Gehnaphore::Lock();
	
	m_nesting = 0;
	m_owner = me;
};

void NestedGehnaphore::Unlock()
{
	if (m_nesting--) return;
	m_owner = B_BAD_THREAD_ID;
	Gehnaphore::Unlock();
};

void NestedGehnaphore::Yield()
{
	int32 count = 0;
	thread_id me = find_thread(NULL);
	if (m_lockValue != 0) {
		while (me == m_owner) {
			Unlock();
			count++;
		};
		while (count--) Lock();
	};
};

bool NestedGehnaphore::IsLocked()
{
	return (m_owner == find_thread(NULL));
}

/***************************************************/

void RWGehnaphore::ReadLock()
{
	rw_data oldData,newData;
	int32 myThread=find_thread(NULL);
	
	oldData.writerActive =
	oldData.writerWaitingOn = 
	oldData.readerThread =
	oldData.readerCount = 0;

	newData.writerActive =
	newData.writerWaitingOn =
	newData.readerThread = 0;
	newData.readerCount = 1;

	while (!cmpxchg64((int64*)&m_extraValue,(int64*)&oldData,*((int64*)&newData))) {
		if (oldData.writerActive) {
			/*	A writer has claimed the lock.  Queue ourselves up to
				get rescheduled when he leaves. */
			newData.readerThread = myThread;
		} else {
			newData.readerThread = oldData.readerThread;
		};
		newData.writerActive = oldData.writerActive;
		newData.writerWaitingOn = oldData.writerWaitingOn;
		newData.readerCount = oldData.readerCount+1;
	};
/*
	printf("ReadLock(%d): %d,%d,%d,%d ---> %d,%d,%d,%d\n",find_thread(NULL),
		oldData.readerThread,oldData.readerCount,oldData.writerActive,oldData.writerWaitingOn,
		newData.readerThread,newData.readerCount,newData.writerActive,newData.writerWaitingOn);
*/
	if (oldData.writerActive) {
		receive_data(NULL,NULL,0);
		if (oldData.readerThread) send_data(oldData.readerThread,0,NULL,0);
	};
};

void RWGehnaphore::ReadUnlock()
{
	rw_data oldData,newData;
	
	oldData.writerActive =
	oldData.writerWaitingOn = 
	oldData.readerThread = 0;
	oldData.readerCount = 1;

	newData.writerActive =
	newData.writerWaitingOn =
	newData.readerThread =
	newData.readerCount = 0;

	while (!cmpxchg64((int64*)&m_extraValue,(int64*)&oldData,*((int64*)&newData))) {
		newData = oldData;
		if (oldData.writerActive)
			newData.writerWaitingOn -= 1;
		else
			newData.readerCount -= 1;
	}
/*
	printf("ReadUnlock(%d): %d,%d,%d,%d ---> %d,%d,%d,%d\n",find_thread(NULL),
		oldData.readerThread,oldData.readerCount,oldData.writerActive,oldData.writerWaitingOn,
		newData.readerThread,newData.readerCount,newData.writerActive,newData.writerWaitingOn);
*/
	if (oldData.writerActive && (oldData.writerWaitingOn == 1))
		send_data(m_writerThread,0,NULL,0);
};

void RWGehnaphore::WriteLock()
{
	rw_data oldData,newData;

	m_write.Lock();
	m_writerThread = find_thread(NULL);

	oldData.writerActive =
	oldData.writerWaitingOn = 
	oldData.readerThread =
	oldData.readerCount = 0;

	newData.writerActive = 1;
	newData.writerWaitingOn =
	newData.readerThread = 
	newData.readerCount = 0;

	while (!cmpxchg64((int64*)&m_extraValue,(int64*)&oldData,*((int64*)&newData))) {
		newData.writerWaitingOn = oldData.readerCount;
	}
/*
	printf("WriteLock(%d): %d,%d,%d,%d ---> %d,%d,%d,%d\n",find_thread(NULL),
		oldData.readerThread,oldData.readerCount,oldData.writerActive,oldData.writerWaitingOn,
		newData.readerThread,newData.readerCount,newData.writerActive,newData.writerWaitingOn);
*/
	if (oldData.readerCount) receive_data(NULL,NULL,0);
};

void RWGehnaphore::WriteUnlock()
{
	rw_data oldData,newData;
	
	oldData.writerActive = 1;
	oldData.writerWaitingOn = 
	oldData.readerThread =
	oldData.readerCount = 0;

	newData.writerActive =
	newData.writerWaitingOn =
	newData.readerThread = 
	newData.readerCount = 0;

	while (!cmpxchg64((int64*)&m_extraValue,(int64*)&oldData,*((int64*)&newData))) {
		newData.readerCount = oldData.readerCount;
	};
/*
	printf("WriteUnlock(%d): %d,%d,%d,%d ---> %d,%d,%d,%d\n",find_thread(NULL),
		oldData.readerThread,oldData.readerCount,oldData.writerActive,oldData.writerWaitingOn,
		newData.readerThread,newData.readerCount,newData.writerActive,newData.writerWaitingOn);
*/
	if (oldData.readerThread) send_data(oldData.readerThread,0,NULL,0);

	m_writerThread = 0;
	m_write.Unlock();
};
