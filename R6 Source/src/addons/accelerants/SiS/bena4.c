/* :ts=8 bk=0
 *
 * bena4.c:	"Benaphore" library routines, with some generalizations...
 *
 * $Id:$
 *
 * Benoit Schillings					9???.??
 *  Modified and isolated by Leo L. Schwab		9806.02
 */
#include "bena4.h"


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
 * the b4_FastLock field to 1 rather than zero.  This allows clients to
 * statically declare them without any special adornment, allowing this code
 * to detect uninitialized Bena4s which might get fed to disposeBena4().
 * This of course presumes the client won't call unlockBena4() more times
 * than lockBena4()...
 */
status_t
initBena4 (struct Bena4 *b4, const char *name)
{
	if (b4) {
		/*
		 * Create semaphore in "pre-locked" state.  First call to
		 * acquire_sem() will block; parallel client must call
		 * release_sem() to unblock first client.
		 */
		if ((b4->b4_Sema4 = create_sem (0, name)) >= 0)
			b4->b4_FastLock = 1;
		return (b4->b4_Sema4 < 0 ?  b4->b4_Sema4 :  0);
	} else
		return (B_ERROR);
}

status_t
initOwnedBena4 (struct Bena4 *b4, const char *name, team_id owner)
{
	status_t	retval;

	if (!(retval = initBena4 (b4, name)))
		retval = set_sem_owner (b4->b4_Sema4, owner);
	return (retval);
}

void
disposeBena4 (struct Bena4 *b4)
{
	if (b4  &&  b4->b4_FastLock) {
		if (b4->b4_Sema4 >= 0) {
			delete_sem (b4->b4_Sema4);
			b4->b4_Sema4 = -1;
		}
		b4->b4_FastLock = 0;	/*  Uninitialized  */
	}
}


void
lockBena4 (struct Bena4 *b4)
{
	int32	old;

	old = atomic_add (&b4->b4_FastLock, 1);
	if (old >= 2) {
		acquire_sem (b4->b4_Sema4);
	}
}

void
unlockBena4 (struct Bena4 *b4)
{
	int32	old;

	old = atomic_add (&b4->b4_FastLock, -1);
	if (old > 2) {
		release_sem (b4->b4_Sema4);
	}
}
