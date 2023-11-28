/******************************************************************************
/
/	File:			Autolock.h
/
/	Description:	BAutolock is a stack-based locking mechanism.
/
/	Copyright 1993-2001, Be Incorporated
/
******************************************************************************/

#ifndef	_SUPPORT2_AUTOLOCK_H
#define	_SUPPORT2_AUTOLOCK_H

#include <support2/Locker.h>

namespace B {
namespace Support2 {

/*-----------------------------------------------------------------*/
/*----- BAutolock class -------------------------------------------*/

class BAutolock {
public:
						BAutolock(const lock_status_t& status);
						~BAutolock();
		
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

} }	// namespace B::Support2

#endif /* _SUPPORT2_AUTOLOCK_H */
