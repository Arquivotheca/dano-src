/*******************************************************************************
/
/	File:			ReadWriteLocker.h
/
/   Description:    Multiple-reader single-writer locking class.
/
/	Copyright 2000, Be Incorporated, All Rights Reserved
/
*******************************************************************************/


#ifndef _READ_WRITE_LOCKER_H
#define _READ_WRITE_LOCKER_H

#include <OS.h>

namespace BPrivate {
struct multilocker_timing;
}

#if DEBUG
#define DEBUG_FLAG true
#else
#define DEBUG_FLAG false
#endif

class BReadWriteLocker
{
public:
								BReadWriteLocker(const char* name = "some ReadWriteLocker",
											 bool debug = DEBUG_FLAG);
		virtual					~BReadWriteLocker();
		
		status_t				InitCheck();
		
/* Locking for reading or writing */
		bool					ReadLock();
		bool					WriteLock();

/* Unlocking after reading or writing */
		bool					ReadUnlock();
		bool					WriteUnlock();

/* Does the current thread hold a write lock? */
		bool					IsWriteLocked(uint32 *stack_base = NULL, thread_id *thread = NULL) const;
/* In DEBUG mode returns whether the lock is held, otherwise always true */
		bool					IsReadLocked() const;

		enum {
			MAX_READERS = 1000000000
		};

private:
virtual	void					_ReservedReadWriteLocker1();
virtual	void					_ReservedReadWriteLocker2();
virtual	void					_ReservedReadWriteLocker3();
virtual	void					_ReservedReadWriteLocker4();
virtual	void					_ReservedReadWriteLocker5();
virtual	void					_ReservedReadWriteLocker6();
virtual	void					_ReservedReadWriteLocker7();
virtual	void					_ReservedReadWriteLocker8();
virtual	void					_ReservedReadWriteLocker9();
virtual	void					_ReservedReadWriteLocker10();
virtual	void					_ReservedReadWriteLocker11();
virtual	void					_ReservedReadWriteLocker12();
virtual	void					_ReservedReadWriteLocker13();
virtual	void					_ReservedReadWriteLocker14();
virtual	void					_ReservedReadWriteLocker15();
virtual	void					_ReservedReadWriteLocker16();

		//functions for managing the DEBUG reader array
		void					register_thread();
		void					unregister_thread();
		
		status_t				fInit;
		//readers adjust count and block on fReadSem when a writer
		//hold the lock
		int32					fReadCount;
		sem_id					fReadSem;
		//writers adjust the count and block on fWriteSem
		//when readers hold the lock
		int32					fWriteCount;
		sem_id 					fWriteSem;
		//writers must acquire fWriterLock when acquiring a write lock
		int32					fLockCount;
		sem_id					fWriterLock;
		int32					fWriterNest;
	
		thread_id				fWriterThread;
		uint32					fWriterStackBase;
				
		int32 *					fDebugArray;
		int32					fMaxThreads;
		
		BPrivate::multilocker_timing*		fTiming;
		
		int32					_reserved[12];
};

#undef DEBUG_FLAG

#endif
