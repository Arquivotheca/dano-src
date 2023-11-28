/* ++++++++++

   FILE:  AutoLock.h
   REVS:  $Revision: 1.3 $
   DATE:  Wed Jan 31 11:00:42 PST 1996

   Written By: Peter Potrebic
   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _AUTO_LOCK_H
#define _AUTO_LOCK_H

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif

template<class T>
class AutoLock
{

// exception safe locking mechanism, allocate on stack and have
// destructor unlock for you whenever the lock goes out of scope

public:
	AutoLock(T *lock, bool lockNow = true)
		// use <lockLater> for lazy locking
		:	lock(lock),
			hasLock(false)
	{
		if (lockNow)
			hasLock = lock->Lock();
	}

	AutoLock(T &lock, bool lockNow = true)
		:	lock(&lock),
			hasLock(false)
	{
		if (lockNow)
			hasLock = this->lock->Lock();
	}

	~AutoLock()
	{
		if (hasLock)
			lock->Unlock();
	}
	
	bool operator!() const
	{
		return !hasLock;
	}
	
	bool IsLocked() const
	{
		return hasLock;
	}
	
	// explicit Lock/Unlock calls are only used in special cases
	// for unlocking before lock goes out of scope and successive
	// re-locking
	// usually constructor/destructor locks/unlocks are sufficient
	void Unlock()
	{
		if (hasLock) {
			lock->Unlock();
			hasLock = false;
		}
	}
	
	bool Lock()
	{
		if (!hasLock)
			hasLock = lock->Lock();

		return hasLock;	
	}

	// convenience call used when passing the AutoLock and the locked object
	// around
	T *LockedItem() const
	{
		return lock;
	}

private:
	T *lock;
	bool hasLock;
};


//====================================================================


// THE FOLLOWING CODE IS DEPRECATED, DO NOT USE!!!!!!!!!
// THE FOLLOWING CODE IS DEPRECATED, DO NOT USE!!!!!!!!!
#ifndef _LOCKER_H
#include <Locker.h>
#endif

class BAutoLock {
public:
// THIS CODE IS DEPRECATED, DO NOT USE!!!!!!!!!
				BAutoLock(BLocker *lock);
				BAutoLock(BLocker &lock);
				~BAutoLock();
		
		bool	IsLocked();

// THIS CODE IS DEPRECATED, DO NOT USE!!!!!!!!!
private:
		BLocker	*fLock;
		bool	fLocked;
};

inline BAutoLock::BAutoLock(BLocker *lock)
{
// THIS CODE IS DEPRECATED, DO NOT USE!!!!!!!!!
	fLock = lock;
	fLocked = fLock->Lock();
}

inline BAutoLock::BAutoLock(BLocker &lock)
{
// THIS CODE IS DEPRECATED, DO NOT USE!!!!!!!!!
	fLock = &lock;
	fLocked = fLock->Lock();
}

inline BAutoLock::~BAutoLock()
{
// THIS CODE IS DEPRECATED, DO NOT USE!!!!!!!!!
	if (fLocked)
		fLock->Unlock();
}

inline bool BAutoLock::IsLocked()
{
// THIS CODE IS DEPRECATED, DO NOT USE!!!!!!!!!
	return fLocked;
}
// THE ABOVE CODE IS DEPRECATED, DO NOT USE!!!!!!!!!

#endif
