/*****************************************************************************

     File: support_p/DebugLock.h

	 Written By: Dianne Hackborn

     Copyright (c) 2001 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#if SUPPORTS_LOCK_DEBUG
#ifndef _SUPPORT_DEBUGLOCK_H
#define _SUPPORT_DEBUGLOCK_H

#include <kernel/OS.h>
#include <support/SupportDefs.h>
#include <support/StreamIO.h>
#include <support/String.h>

class BCallStack;

namespace B {
namespace Support {

static int32 LockDebugLevel();

class StaticStringIO;
struct block_links;

enum {
	/*
	 * These flags can be passed in to the lock constructor.
	 */
	
	// Set to allow the lock to be deleted by a thread that is holding it.
	LOCK_CAN_DELETE_WHILE_HELD		= 0x00000001,
	
	// Set to allow the lock to always be deleted, even if another thread holds it.
	LOCK_ANYONE_CAN_DELETE			= 0x00000002,
	
	/*
	 * These flags can be passed in to the lock constructor or Lock() function.
	 */
	
	// Set to not perform deadlock checks -- don't add this acquire to deadlock graph.
	LOCK_SKIP_DEADLOCK_CHECK		= 0x00010000,
	
	// Set to not remember for future deadlock checks that this lock is being held.
	LOCK_DO_NOT_REGISTER_HELD		= 0x00020000,
	
	// Set to allow a threads besides the current owner to perform an unlock.
	LOCK_ANYONE_CAN_UNLOCK			= 0x00040000
};

class DebugLockNode
{
public:
								DebugLockNode(	const char* type,
												const void* addr,
												const char* name,
												uint32 debug_flags = 0);
			
	virtual	void				Delete();
			
			int32				IncRefs() const;
			int32				DecRefs() const;
			
			const char*			Type() const;
			const void*			Addr() const;
			const char*			Name() const;
			uint32				GlobalFlags() const;
			
			const BCallStack*	CreationStack() const;
	
			bool				LockGraph() const;
			void				UnlockGraph() const;
			bool				AddToGraph();
			void				RegisterAsHeld();
			void				UnregisterAsHeld();
	
	virtual	int32				Contention() const;
	
	virtual	void				PrintToStream(BDataIO& io) const;
	static	void				PrintContentionToStream(BDataIO& io);
	
	// Private implementation.
	inline	const block_links*	Links() const	{ return m_links; }
	
protected:
	virtual						~DebugLockNode();
	virtual	void				PrintStacksToStream(StaticStringIO& io) const;
	virtual	void				PrintSubclassToStream(BDataIO& io) const;
	
private:
	mutable	int32				m_refs;			// instance reference count
	
	// Lock-specific information.  Does not change after creation.
			BString const		m_type;			// descriptive type of this object
			const void* const	m_addr;			// user-visible address of lock
			BString const		m_name;			// descriptive name of lock
			uint32 const		m_globalFlags;	// overall operating flags
			BCallStack* const	m_createStack;	// where the lock was created
	
	// Lock linkage information.  Protected by the global graph lock.
			block_links*		m_links;		// linkage to other locks
};

inline BDataIO& operator<<(BDataIO& io, const DebugLockNode& db)
{
	db.PrintToStream(io);
	return io;
}

class DebugLock : public DebugLockNode
{
public:
								DebugLock(	const char* type,
											const void* addr,
											const char* name,
											uint32 debug_flags = 0);
	
	virtual	void				Delete();
	virtual	int32				Contention() const;
	
			status_t			Lock(	uint32 flags = B_TIMEOUT,
										bigtime_t timeout = B_INFINITE_TIMEOUT,
										uint32 debug_flags = 0);
			status_t			Unlock();

			const BCallStack*	OwnerStack() const;
			sem_id				Semaphore() const;
			
protected:
	virtual						~DebugLock();
	virtual	void				PrintStacksToStream(StaticStringIO& io) const;
	virtual	void				PrintSubclassToStream(BDataIO& io) const;
	
private:
	// Lock-specific information.  Protected by the lock itself.
			sem_id				m_sem;			// implementation of this lock
			thread_id			m_owner;		// who currently holds the lock
			int32				m_ownerFlags;	// passed in from Lock()
			BCallStack*			m_ownerStack;	// stack of last lock accessor
			int32				m_contention;	// current waiting threads
			int32				m_maxContention;// maximum number of waiting threads
			bool				m_deleted;		// no longer valid?
};


extern int32 gLockDebugLevel;
int32 LockDebugLevelSlow();
static inline int32 LockDebugLevel() {
	if (gLockDebugLevel >= 0) return gLockDebugLevel;
	return LockDebugLevelSlow();
}

} }	// namespace B::Support

#endif	// _SUPPORT2_DEBUGLOCK_H
#endif	// SUPPORTS_LOCK_DEBUG
