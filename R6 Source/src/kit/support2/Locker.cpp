/*****************************************************************************

     File: support/Locker.cpp

	 Written By: George Hoffman
	 Arranged By: Dianne Hackborn

     Copyright (c) 2001 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#include <support2/Locker.h>

#include <kernel/OS.h>
#include <support2/atomic.h>
#include <support2/StringIO.h>

namespace B {
namespace Support2 {

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

} }	// namespace B::Support2

// -----------------------------------------------------------------
// -----------------------------------------------------------------
// -----------------------------------------------------------------

#if !SUPPORTS_LOCK_DEBUG

//#pragma mark -

namespace B {
namespace Support2 {

static inline void init_gehnaphore(int32* value)
{
	init_gehnaphore_real(value);
}

static inline void init_gehnaphore(int32* value, const char*)
{
	init_gehnaphore_real(value);
}

static inline void fini_gehnaphore(int32* /*value*/)
{
}

static inline void lock_gehnaphore(int32* value)
{
	lock_gehnaphore_real(value);
}

static inline void unlock_gehnaphore(int32* value)
{
	unlock_gehnaphore_real(value);
}

} }	// namespace B::Support2

// -----------------------------------------------------------------

#else

#include <support2_p/DebugLock.h>
#include <stdlib.h>
#include <new>

//#pragma mark -

namespace B {
namespace Support2 {

inline void init_gehnaphore(int32* value, const char* name = "gehnaphore")
{
	if (!LockDebugLevel()) init_gehnaphore_real(value);
	else *value = reinterpret_cast<int32>(new(std::nothrow) DebugLock("BLocker", value, name));
}

inline void fini_gehnaphore(int32* value)
{
	if (!LockDebugLevel()) ;
	else if (*value) reinterpret_cast<DebugLock*>(*value)->Delete();
}

inline void lock_gehnaphore(int32* value)
{
	if (!LockDebugLevel()) lock_gehnaphore_real(value);
	else if (*value) reinterpret_cast<DebugLock*>(*value)->Lock();
}

inline void unlock_gehnaphore(int32* value)
{
	if (!LockDebugLevel()) unlock_gehnaphore_real(value);
	else if (*value) reinterpret_cast<DebugLock*>(*value)->Unlock();
}

} }	// namespace B::Support2

#endif

// -----------------------------------------------------------------
// ---------------------------- BLocker ----------------------------
// -----------------------------------------------------------------

namespace B {
namespace Support2 {

//#pragma mark -

BLocker::BLocker()
{
	init_gehnaphore(&fLockValue);
}

BLocker::BLocker(const char* name)
{
	init_gehnaphore(&fLockValue, name);
}

BLocker::~BLocker()
{
	fini_gehnaphore(&fLockValue);
}

lock_status_t BLocker::Lock()
{
	lock_gehnaphore(&fLockValue);
	return lock_status_t((void (*)(void*))unlock_gehnaphore, &fLockValue);
}

void BLocker::Yield()
{
	if (fLockValue != 0) {
		Unlock();
		Lock();
	};
}

void BLocker::Unlock()
{
	unlock_gehnaphore(&fLockValue);
}

// -----------------------------------------------------------------
// ------------------------- BNestedLocker -------------------------
// -----------------------------------------------------------------

//#pragma mark -

BNestedLocker::BNestedLocker()
{
	init_gehnaphore(&fLockValue);
	fOwner = B_BAD_THREAD_ID;
	fOwnerCount = 0;
}

BNestedLocker::BNestedLocker(const char* name)
{
	init_gehnaphore(&fLockValue, name);
	fOwner = B_BAD_THREAD_ID;
	fOwnerCount = 0;
}

BNestedLocker::~BNestedLocker()
{
	fini_gehnaphore(&fLockValue);
}

lock_status_t BNestedLocker::Lock()
{
	thread_id me = find_thread(NULL);
	if (me == fOwner) {
		fOwnerCount++;
	} else {
		lock_gehnaphore(&fLockValue);
		fOwnerCount = 1;
		fOwner = me;
	}
	return lock_status_t((void (*)(void*))_UnlockFunc, this);
}

void BNestedLocker::Yield()
{
	if (fLockValue != 0 && fOwner == find_thread(NULL)) {
		const int32 count = fOwnerCount;
		fOwnerCount = 1;
		Unlock();
		Lock();
		fOwnerCount = count;
	}
}

void BNestedLocker::_UnlockFunc(BNestedLocker* l)
{
	if (--l->fOwnerCount) return;
	l->fOwner = B_BAD_THREAD_ID;
	unlock_gehnaphore(&l->fLockValue);
}

void BNestedLocker::Unlock()
{
	_UnlockFunc(this);
}

int32 BNestedLocker::NestingLevel() const
{
	if (fOwner == find_thread(NULL)) {
		return fOwnerCount;
	}
	return 0;
}

} }	// namespace B::Support2
