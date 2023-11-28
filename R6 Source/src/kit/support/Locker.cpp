/*****************************************************************************

     $Source: /net/bally/be/rcs/src/kit/support/Locker.cpp,v $

     $Revision: 1.21 $

	 Written By: Peter Potrebic

     Copyright (c) 1994-98 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#ifndef _DEBUG_H
#include <Debug.h>
#endif
#ifndef _LOCKER_H
#include <Locker.h>
#endif
#ifndef _OS_H
#include <OS.h>
#endif

#if SUPPORTS_LOCK_DEBUG
#include <support_p/DebugLock.h>
using namespace B::Support;
#endif

#include <new>

#undef _PR3_COMPATIBLE_
#define _PR3_COMPATIBLE_ 1

/*----------------------------------------------------------------*/

BLocker::BLocker()
{
	InitData(NULL, true, false);
}

/*----------------------------------------------------------------*/

BLocker::BLocker(const char *name)
{
	InitData(name, true, false);
}

/*----------------------------------------------------------------*/

BLocker::BLocker(bool benaphore_style)
{
	InitData(NULL, benaphore_style, false);
}

/*----------------------------------------------------------------*/

BLocker::BLocker(const char *name, bool benaphore_style)
{
	InitData(name, benaphore_style, false);
}

/*----------------------------------------------------------------*/

BLocker::BLocker(const char *name, bool benaphore_style, bool for_IPC)
{
	InitData(name, benaphore_style, for_IPC);
}

/*----------------------------------------------------------------*/

void BLocker::InitData(const char *name, bool benaphore, bool /*ipc*/)
{
#if SUPPORTS_LOCK_DEBUG
	if (LockDebugLevel()) benaphore = false;
#endif
	fCount = benaphore ? 0 : 1;
	fOwner = -1;
	fOwnerCount = 0;
#if !_PR3_COMPATIBLE_
	fCanUseStack = !ipc;
	fOwnerStack = 0;
#endif /* not _PR3_COMPATIBLE_ */

	if (!name) name = "some BLocker";
#if SUPPORTS_LOCK_DEBUG
	if (LockDebugLevel())
		fSem = reinterpret_cast<sem_id>(
			new(std::nothrow) DebugLock("BLocker", this, name, LOCK_CAN_DELETE_WHILE_HELD));
	else
#endif
		fSem = create_sem(benaphore ? 0 : 1, name);
}

/*----------------------------------------------------------------*/

BLocker::~BLocker()
{
#if SUPPORTS_LOCK_DEBUG
	if (LockDebugLevel())
		reinterpret_cast<DebugLock*>(fSem)->Delete();
	else
#endif
		delete_sem(fSem);
}

/*----------------------------------------------------------------*/

status_t BLocker::InitCheck() const
{
#if SUPPORTS_LOCK_DEBUG
	if (LockDebugLevel())
		return fSem ? B_OK : B_NO_MEMORY;
	else
#endif
		return fSem >= B_OK ? B_OK : fSem;
}

/*----------------------------------------------------------------*/

bool BLocker::Lock()
{
	return _Lock(B_INFINITE_TIMEOUT) == B_OK;
}

/*----------------------------------------------------------------*/

lock_status_t BLocker::LockWithStatus()
{
	return _Lock(B_INFINITE_TIMEOUT);
}

/*----------------------------------------------------------------*/

lock_status_t BLocker::LockWithTimeout(bigtime_t timeout, bool)
{
	return _Lock(timeout);
}

/*----------------------------------------------------------------*/

lock_status_t BLocker::_Lock(bigtime_t timeout)
{
#if !_PR3_COMPATIBLE_
	thread_id		owner;
	uint32			stack = ((uint32)(&owner)) & 0xFFFFF000;
#else /* _PR3_COMPATIBLE_ */
	thread_id		owner = find_thread(NULL);
#endif /* _PR3_COMPATIBLE_ */
	lock_status_t	result(B_NO_ERROR);
	int				old;
	
#if !_PR3_COMPATIBLE_
	if ((fCanUseStack && (fOwnerStack == stack)) ||
		((owner = find_thread(NULL)) == fOwner)) {
		fOwnerStack = stack;
#else /* _PR3_COMPATIBLE_ */
	if (owner == fOwner) {
#endif /* _PR3_COMPATIBLE_ */
		fOwnerCount++;
		result.unlock_func = reinterpret_cast<void (*)(void*)>(_UnlockFunc);
		result.value.data = this;
		return result;
	}

	result.value.error = B_NO_ERROR;
	
	old = atomic_add(&fCount, 1);
	if (old >= 1) {
#if SUPPORTS_LOCK_DEBUG
		if (LockDebugLevel())
			result.value.error = reinterpret_cast<DebugLock*>(fSem)
				->Lock(B_TIMEOUT, timeout);
		else
#endif
			do {
				result.value.error = acquire_sem_etc(fSem, 1, B_TIMEOUT, timeout);
			} while (result.value.error == B_INTERRUPTED);
	}

	if (result.value.error == B_NO_ERROR) {
		fOwner = owner;
#if !_PR3_COMPATIBLE_
		fOwnerStack = stack;
#endif /* not _PR3_COMPATIBLE_ */
		fOwnerCount = 1;
		result.unlock_func = reinterpret_cast<void (*)(void*)>(_UnlockFunc);
		result.value.data = this;
	} else {
		result.unlock_func = NULL;
	}

	return result;
}

/*----------------------------------------------------------------*/

void BLocker::_UnlockFunc(BLocker* This)
{
	This->fOwnerCount--;

	if (This->fOwnerCount == 0) {
		This->fOwner = -1;
#if !_PR3_COMPATIBLE_
		This->fOwnerStack = 0;
#endif /* not _PR3_COMPATIBLE_ */
		int old = atomic_add(&This->fCount, -1);
		if (old > 1) {
#if SUPPORTS_LOCK_DEBUG
			if (LockDebugLevel())
				reinterpret_cast<DebugLock*>(This->fSem)->Unlock();
			else
#endif
				release_sem(This->fSem);
		}
	}
}

/*----------------------------------------------------------------*/

void BLocker::Unlock()
{
	_UnlockFunc(this);
}

/*----------------------------------------------------------------*/
thread_id BLocker::LockingThread() const
	{ return fOwner; }

/*----------------------------------------------------------------*/
bool BLocker::IsLocked() const
#if !_PR3_COMPATIBLE_
{
	uint32 stack;
	stack = (((uint32)(&stack)) & 0xFFFFF000);
	if (fCanUseStack && (fOwnerStack == stack)) return true;
	if (fOwner == find_thread(NULL)) {
		const_cast<BLocker*>(this)->fOwnerStack = stack;
		return true;
	};
	return false;
}
#else /* _PR3_COMPATIBLE_ */
	{ return (fOwner == find_thread(NULL)); }
#endif /* _PR3_COMPATIBLE_ */

/*----------------------------------------------------------------*/
int32 BLocker::NestingLevel() const
{
	if (IsLocked()) {
		return fOwnerCount;
	}
	return 0;
}

/*----------------------------------------------------------------*/
int32 BLocker::CountLocks() const
	{ return fOwnerCount; }

/*----------------------------------------------------------------*/
int32 BLocker::CountLockRequests() const
	{ return fCount; }

/*----------------------------------------------------------------*/
sem_id BLocker::Sem() const
	{ return fSem; }

// --------- Deprecated BLocker methods 02/2001 (Dano?) ---------

#if _R5_COMPATIBLE_
extern "C" {

	_EXPORT status_t
	#if __GNUC__
	LockWithTimeout__7BLockerx
	#elif __MWERKS__
	LockWithTimeout__7BLockerFx
	#endif
	(BLocker* This, bigtime_t timeout)
	{
		return This->LockWithTimeout(timeout);
	}
	
}
#endif
