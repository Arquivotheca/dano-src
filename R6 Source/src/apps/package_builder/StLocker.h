//	StLocker.h

#ifndef _STLOCKER_H
#define _STLOCKER_H

template<class T>
class StLocker
{
		T*						fLock;
		bool					fHasLock;
public:
		StLocker(T* lock) : fLock(lock)
		{
			fHasLock = fLock->Lock();
		}
		~StLocker()
		{
			if (fHasLock)
				fLock->Unlock();
		}
		bool operator!()
		{
			return !fHasLock;
		}
};

typedef StLocker<BLocker> StBLock;

#endif
