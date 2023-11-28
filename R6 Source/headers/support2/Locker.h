/******************************************************************************
/
/	File:			support/BLightLocker.h
/
/	Description:	BLightLocker:		a non-semaphare based class that is not
/										nestable.
/					BLightNestedLocker:	a non-semaphore based class that is
/										nestable.
/
/	Copyright 2001, Be Incorporated
/
******************************************************************************/

#ifndef	_SUPPORT2_LOCKER_H
#define	_SUPPORT2_LOCKER_H

#include <support2/SupportDefs.h>

namespace B {
namespace Support2 {

/*-------------------------------------------------------------*/
/*----- BLocker class ------------------------------------*/

class BLocker {
public:
						BLocker();
						BLocker(const char* name);
						~BLocker();	

		lock_status_t	Lock();
		void			Yield();
		void			Unlock();

private:
		int32			fLockValue;
};

/*-------------------------------------------------------------*/
/*----- BNestedLocker class ------------------------------*/

class BNestedLocker {
public:
						BNestedLocker();
						BNestedLocker(const char* name);
						~BNestedLocker();	

		lock_status_t	Lock();
		void			Yield();
		void			Unlock();

		// Return the number of levels that this thread has
		// nested the lock; if the caller isn't holding the
		// lock, 0 is returned.
		int32			NestingLevel() const;
		
private:
static	void			_UnlockFunc(BNestedLocker* l);

		int32			fLockValue;
		int32			fOwner;
		int32			fOwnerCount;
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

} }	// namespace B::Support2

#endif /* _SUPPORT2_LOCKER_H */
