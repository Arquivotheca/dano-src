/*****************************************************************************\
 * File:		fc_lock.h
 * Description:	Kernel module implementing flow-control lock
 *
 * Copyright 2001, Be Incorporated, All Rights Reserved.
 *
 * Author:		Chris Liscio <liscio@be.com>,
 *				based on fc_lock.h by Joe Kloss <joek@be.com>
\*****************************************************************************/

#ifndef _FC_LOCK_H_
#define _FC_LOCK_H_

#include <OS.h>
#include <KernelExport.h>
#include <module.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
	fc_lock implements a flow-control lock well-suited for network driver
	flow control.
	
	You can NOT call fc_wait during an interrupt handler as it waits on
	a semaphore; however, calling fc_signal during an interrupt handler
	is often encouraged. :)
	
	Another thing to keep in mind is that fc_locks cannot be shared
	between threads.  Be warned.

  	fc_locks behave a lot like counting semaphores and therefore are set
  	up and used quite the same:
  	
	create_fc_lock
		Creates a new fc_lock structure.  Assign a count and name as you
		would a new semaphore.
	
	delete_fc_lock
		Deletes the fc_lock you created.
	
	fc_signal
		Signal (and release) any held fc_locks.  The count and sem_flags
		are to be treated as you would in a semaphore situation.
	
	fc_wait
		Wait for a fc_lock to become available.  The timeout works just as
		on a semaphore with the B_CAN_INTERRUPT and B_TIMEOUT flags.
	
*/

#define B_FC_LOCK_MODULE_NAME "generic/fc_lock/v1"

typedef struct fc_lock {
	int32			count;
	spinlock		slock;
	int32			waiting;
	sem_id			sem;
} fc_lock;

typedef struct {
	module_info		minfo;
	status_t		( *create_fc_lock )( struct fc_lock *fc, int32 count, const char *name );
	void			( *delete_fc_lock )( struct fc_lock *fc );
	bool			( *fc_signal )( struct fc_lock *fc, int32 count, int32 sem_flags );
	status_t		( *fc_wait )( struct fc_lock *fc, bigtime_t timeout );
} fc_lock_module_info;

#ifdef __cplusplus
}
#endif

#endif // _FC_LOCK_H_
