/*
	CMessengerAutoLocker.cpp
*/
#include "CMessengerAutoLocker.h"
#include <Looper.h>

/***** CMessengerAutoLocker::CMessengerAutoLocker() *****/
CMessengerAutoLocker::CMessengerAutoLocker(BMessenger messenger)
	: 	fMessenger(messenger)
{
	Init();
}

/***** CMessengerAutoLocker::CMessengerAutoLocker() *****/
CMessengerAutoLocker::CMessengerAutoLocker(BHandler *handler)
	: 	fMessenger(handler)
{
	Init();
}

/***** CMessengerAutoLocker::~CMessengerAutoLocker() *****/
CMessengerAutoLocker::~CMessengerAutoLocker()
{
	Unlock();
}

/***** CMessengerAutoLocker::Init() *****/
void CMessengerAutoLocker::Init()
{
	fHasLock = fMessenger.IsValid() ? messenger->LockTarget() : false;
}

/***** CMessengerAutoLocker::IsLocked() *****/
bool CMessengerAutoLocker::IsLocked() const
{
	return fHasLock;
}

/***** CMessengerAutoLocker::Unlock() *****/
void CMessengerAutoLocker::Unlock()
{
	if(fHasLock)
	{
		BLooper *looper;
		fMessenger.Target(&looper);
		if(looper)
			looper->Unlock();
		fHasLock = false;
	}
}

/***** CMessengerAutoLocker::operator!() *****/
bool CMessengerAutoLocker::operator!() const
{
	return !fHasLock;
}

/* End of CMessengerAutoLocker.cpp */				