/* :ts=8 bk=0
 *
 * bena4.c:	"Benaphore" library routines, with some generalizations...
 *
 * $Id:$
 *
 * Benoit Schillings					9???.??
 *  Modified and isolated by Leo L. Schwab		9806.02
 */
#include <dinky/bena4.h>


//**************************************************************************
//  This protection is done by an atomic lock (very fast) and a semaphore
//  only when necessary (much slower), to put a thread in waiting state.
//  These two levels protection allow very good performance. The
//  Be-implementation is currently called "Benaphore", as an extension of
//  semaphore, first used by Benoit Schillings.
//**************************************************************************


/*
 * Client must supply storage for Bena4 structure.  (I thought about having
 * this routine do the allocation, but it seemed overkill...)
 *
 * The routine has been modified from the "classic" source to initialize
 * the b4_FastLock field to -1 rather than zero.  This allows clients to
 * statically declare them without any special adornment, and forces them
 * to safely receive an error when attempting to lock an uninitialized Bena4.
 * It also greatly aids in safely destroying a Bena4 (deadlocks are still
 * possible, but only under pathological circumstances).
 */
status_t
BInitBena4 (struct Bena4 *b4, const char *name)
{
	if (b4) {
		/*
		 * Create semaphore in "pre-locked" state.  First call to
		 * acquire_sem() will block; parallel client must call
		 * release_sem() to unblock first client.
		 */
		if ((b4->b4_Sema4 = create_sem (0, name)) >= 0) {
			/*  Enable fast lock  */
			atomic_or (&b4->b4_FastLock, -1);
		}
		return (b4->b4_Sema4 < 0 ?  b4->b4_Sema4 :  B_OK);
	} else
		return (B_ERROR);
}

status_t
BInitOwnedBena4 (struct Bena4 *b4, const char *name, team_id owner)
{
	status_t	retval;

	if (!(retval = BInitBena4 (b4, name)))
		retval = set_sem_owner (b4->b4_Sema4, owner);
	return (retval);
}

void
BDisposeBena4 (struct Bena4 *b4)
{
	if (b4) {
		/*
		 * Force other clients to block on semaphore.
		 */
		if (BLockBena4 (b4) < 0)
			/*  Got any better ideas?  */
			return;
		
		/*
		 * Other clients will now return an error.  We leave
		 * b4_FastLock untouched, as it will force future clients
		 * to attempt to acquire a non-existent semaphore, which
		 * safely generates an error.
		 */
		delete_sem (b4->b4_Sema4);
		b4->b4_Sema4 = -1;
	}
}


status_t
BLockBena4 (struct Bena4 *b4)
{
	status_t	err;

	if (!b4)
		return B_ERROR;

	if (atomic_add (&b4->b4_FastLock, 1) >= 0) {
		/*  Nope, gotta do it the hard way.  */
		do
			err = acquire_sem (b4->b4_Sema4);
		while (err == B_INTERRUPTED);
	} else
		err = B_OK;

	if (err < 0)
		/*  Lock failed; back out the count.  */
		atomic_add (&b4->b4_FastLock, -1);

	return err;
}

void
BUnlockBena4 (struct Bena4 *b4)
{
	if (b4)
		if (atomic_add (&b4->b4_FastLock, -1) > 0)
			release_sem (b4->b4_Sema4);
}
