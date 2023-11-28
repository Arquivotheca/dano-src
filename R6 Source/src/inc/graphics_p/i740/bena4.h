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


typedef struct Bena4 {
	sem_id	b4_Sema4;
	int32	b4_FastLock;
} Bena4;


status_t	initBena4 (struct Bena4 *b4, const char *name);
status_t	initOwnedBena4 (struct Bena4 *b4,
				const char *name,
				team_id owner);
void		disposeBena4 (struct Bena4 *b4);
void		lockBena4 (struct Bena4 *b4);
void		unlockBena4 (struct Bena4 *b4);


#endif	/*  _BENA4_H  */
