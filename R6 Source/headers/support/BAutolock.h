/******************************************************************************
/
/	File:			Autolock.h
/
/	Description:	BAutolock is a stack-based locking mechanism.
/
/	Copyright 1993-98, Be Incorporated
/
******************************************************************************/

#ifndef	_SUPPORT_AUTOLOCK_H
#define	_SUPPORT_AUTOLOCK_H

#include <BeBuild.h>
#include <Locker.h>
#include <Looper.h>

namespace B {
namespace Support {

/*-----------------------------------------------------------------*/
/*----- BAutolock class -------------------------------------------*/

class BAutolock {
public:
						BAutolock(const lock_status_t& status);
						~BAutolock();
						
						// Deprecated constructors.
						BAutolock(BLocker *lock);
						BAutolock(BLocker &lock);
						BAutolock(BLooper *looper);
		
		bool			IsLocked();

/*----- Private or reserved ---------------*/
private:
		lock_status_t	fStatus;
		bool			_reserved;
};

/*-------------------------------------------------------------*/
/*----- inline implementations --------------------------------*/

inline BAutolock::BAutolock(const lock_status_t& status)
	:	fStatus(status), _reserved(false)
{
}

inline BAutolock::BAutolock(BLooper *looper)
	:	fStatus(looper->LockWithStatus()), _reserved(false)
{
}

inline BAutolock::BAutolock(BLocker *target)
	:	fStatus(target->LockWithStatus()), _reserved(false)
{
}

inline BAutolock::BAutolock(BLocker &target)
	:	fStatus(target.LockWithStatus()), _reserved(false)
{
}

inline BAutolock::~BAutolock()
{
	fStatus.unlock();
}

inline bool BAutolock::IsLocked()
{
	return fStatus.is_locked();
}

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

} }	// namespace B::Support

#endif /* _SUPPORT_AUTOLOCK_H */
