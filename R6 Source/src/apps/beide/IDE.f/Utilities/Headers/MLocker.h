//========================================================================
//	MLocker.h
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	An auto-locker class to be used for locking BLockers and BLoopers (can actually be
//	used for any class that has the same interface as BLocker, like BClipboard).
/*	Use like:
	MLocker<BLocker>	lock(aBLocker);
	MLocker<BLooper>	lock(aBLooperPtr);
*/

#ifndef _MLOCKER_H
#define _MLOCKER_H

template <class T>
class MLocker
{
public:
								MLocker(
									T&	inLockable)
									: fLockable(inLockable)
								{
									fLocked = inLockable.Lock();
								}
								MLocker(
									T*	inLockable)
									: fLockable(*inLockable)
								{
									fLocked = inLockable->Lock();
								}
								~MLocker()
								{
									if (fLocked)
										fLockable.Unlock();
								}
		bool					IsLocked()
								{
									return fLocked;
								}

private:

	T&			fLockable;
	bool		fLocked;
};

#endif
