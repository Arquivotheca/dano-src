/* :ts=8 bk=0
 *
 * bena4.c:	"Benaphore" library routines, with some generalizations...
 *
 * $Id:$
 *
 * Benoit Schillings					9???.??
 *  Modified and isolated by Leo L. Schwab		9806.02
 */
#ifndef	_BENA4_H
#define	_BENA4_H

#ifndef	_OS_H
#include <kernel/OS.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


typedef struct Bena4 {
	int32		b4_FastLock;	/*  Quick lock + sem created flag  */
	sem_id		b4_Sema4;
} Bena4;


extern status_t	BInitBena4 (struct Bena4 *b4, const char *name);
extern status_t	BInitOwnedBena4 (struct Bena4 *b4,
				 const char *name,
				 team_id owner);
extern void	BDisposeBena4 (struct Bena4 *b4);
extern status_t	BLockBena4 (struct Bena4 *b4);
extern void	BUnlockBena4 (struct Bena4 *b4);


#ifdef __cplusplus
}
#endif

#endif	/*  _BENA4_H  */
