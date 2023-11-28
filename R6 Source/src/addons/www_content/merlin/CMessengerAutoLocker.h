/*
	CMessengerAutoLocker.h
*/
#ifndef CMessengerAutoLocker_h
	#define CMessengerAutoLocker_h
	#include <Looper.h>
	#include <Messenger.h>

class CMessengerAutoLocker
{
	public:
								// Constructors
		inline					CMessengerAutoLocker(BMessenger messenger);
		inline					CMessengerAutoLocker(BHandler *handler);
		inline					~CMessengerAutoLocker();
								// Local methods
		inline void				Init();
		inline bool				IsLocked() const;
		inline void				Unlock();
								// Operator overloads
		inline bool				operator!() const;
	
	private:
		BMessenger				fMessenger;
		bool					fHasLock;
};

inline CMessengerAutoLocker::CMessengerAutoLocker(BMessenger messenger)
	: 	fMessenger(messenger)
{
	Init();
}

inline CMessengerAutoLocker::CMessengerAutoLocker(BHandler *handler)
	: 	fMessenger(handler)
{
	Init();
}

inline CMessengerAutoLocker::~CMessengerAutoLocker()
{
	Unlock();
}

inline void CMessengerAutoLocker::Init()
{
	fHasLock = fMessenger.IsValid() ? fMessenger.LockTarget() : false;
}

inline bool CMessengerAutoLocker::IsLocked() const
{
	return fHasLock;
}

inline void CMessengerAutoLocker::Unlock()
{
	if(fHasLock) {
		BLooper *looper;
		fMessenger.Target(&looper);
		if(looper)
			looper->Unlock();
		fHasLock = false;
	}
}

inline bool CMessengerAutoLocker::operator!() const
{
	return !fHasLock;
}

#endif
/* End of CMessengerAutoLocker.h */
