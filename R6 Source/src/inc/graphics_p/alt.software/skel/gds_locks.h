///////////////////////////////////////////////////////////////////////////////
// Locks Header
//
///////////////////////////////////////////////////////////////////////////////
//    These macros implement benaphore locks. These were originally inline
// functions, but I doubt that these would be properly inlined if compiled in
// one file and linked to another. Macros will always generate immediate code,
// though they're a bit messier. This is important, because locking will be
// performed in speed-sensitive areas.


///////////////////////////////////////////////////////////////////////////////
// Type Definitions ///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// A benaphore is a combination of an atomic lock and a semaphore. The
// semaphore is acquired if the atomic lock is not available. This is much
// faster than using a semaphore alone, as in the majority of cases, the lock
// will be free, and the expensive semaphore acquisition won't have to be
// performed.

typedef struct
{
  sem_id sem;
  long lock;
} gds_benaphore; // We don't want to conflict with other definitions of "benaphore".



///////////////////////////////////////////////////////////////////////////////
// Helper Functions ///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// These are implemented as macros for the time being. For speed, these should
// either be macros or inline functions.


// Benaphore initialization macro.

// __inline void init_gds_benaphore(gds_benaphore *the_benaphore,
//  char *semaphore_name)

// No pointer checking.
// The old version of this routine made sure that the semaphore value was negative
// (unused) before initializing the benaphore. This version just ignores that, as
// in practice we're often passed benaphores filled with random garbage.
// NOTE: This macro isn't thread-safe! Make sure that it isn't called by multiple
// threads simultaneously for the same benaphore.

#define INIT_GDS_BENAPHORE(the_benaphore, semaphore_name);              \
  (the_benaphore)->lock = 0;                                            \
  (the_benaphore)->sem = create_sem(1, semaphore_name);


// Benaphore de-initialization macro.

// __inline void dispose_gds_benaphore(gds_benaphore *the_benaphore)

// No pointer checking.
// NOTE: This macro isn't thread-safe! Make sure that it isn't called by multiple
// threads simultaneously for the same benaphore.

#define DISPOSE_GDS_BENAPHORE(the_benaphore);                           \
  if ((the_benaphore)->sem >= 0)                                        \
    delete_sem((the_benaphore)->sem);                                   \
  (the_benaphore)->sem = -1;                                            \
  (the_benaphore)->lock = 0;


// The acquisition and release macros _are_ thread-safe.

// Benaphore acquisition macro.

// __inline void acquire_gds_benaphore(gds_benaphore *the_benaphore)

// No pointer checking!.
// Increment the lock counter. If it was >= 1 before we incremented it, it was
// already in use, so acquire the semaphore.

#define ACQUIRE_GDS_BENAPHORE(the_benaphore);                      \
  if (atomic_add(&((the_benaphore)->lock), 1) >= 1)                     \
    acquire_sem((the_benaphore)->sem);


// Benaphore release macro.

// __inline void release_gds_benaphore(gds_benaphore *the_benaphore)

// No pointer checking!.
// Decrement the lock counter. If it was > 1 before we decremented it, then
// another thread is also using it, so release the semaphore to allow that
// thread to run.

#define RELEASE_GDS_BENAPHORE(the_benaphore);                      \
  if (atomic_add(&((the_benaphore)->lock), -1) > 1)                     \
    release_sem((the_benaphore)->sem);


///////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
