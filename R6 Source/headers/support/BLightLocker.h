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

#ifndef	_SUPPORT_LIGHT_LOCKER_H
#define	_SUPPORT_LIGHT_LOCKER_H

#include <BeBuild.h>
#include <kernel/OS.h>
#include <support/String.h>

namespace B {
namespace Support {

/*-------------------------------------------------------------*/
/*----- BLightLocker class ------------------------------------*/

class BLightLocker {
public:
						BLightLocker();
						~BLightLocker();	

		lock_status_t	Lock();
		void			Yield();
		void			Unlock();

private:
		int32			fLockValue;
};

/*-------------------------------------------------------------*/
/*----- BLightNestedLocker class ------------------------------*/

class BLightNestedLocker {
public:
						BLightNestedLocker();
						BLightNestedLocker(const char *name);
						~BLightNestedLocker();	

		lock_status_t	Lock();
		void			Yield();
		void			Unlock();

		// Return the number of levels that this thread has
		// nested the lock; if the caller isn't holding the
		// lock, 0 is returned.
		int32			NestingLevel() const;
		
private:
		void			InitData(const char *name);
static	void			_UnlockFunc(BLightNestedLocker* l);

		BString			fName;
		int32			fLockValue;
		thread_id		fOwner;
		int32			fOwnerCount;
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

} }	// namespace B::Support

#endif /* _SUPPORT_LIGHT_LOCKER_H */
