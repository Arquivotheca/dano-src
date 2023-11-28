#ifndef _BYTES_H_
#define _BYTES_H_

/* File:       bytes.h

   Contains: This file contains definitions and function prototypes
             for bcopy, bzero, and bcmp.

   Written by: Drivin' Team

   Change History (most recent first):


 *********************************************************************
 * PROPRIETARY NOTICE:   The  software  information  contained
 * herein   is  the  sole  property   of  KEPS, Inc.  and   is
 * provided  to KEPS  users  under license  for use  on  their
 * designated  equipment  only. Reproduction of this matter in
 * whole or in part is forbidden without the  express  written
 * consent of KEPS, Inc.
 *
 * COPYRIGHT (c) 1992-1994 KEPS, Inc.
 * As  an unpublished  work pursuant to Title 17 of the United
 * States Code.  All rights reserved.
 *********************************************************************
 */

/* SCCSID = "@(#)bytes.h		04/07/95 @(#)bytes.h	1.7" */

#if !defined(KPNONANSIC)
#if defined(KPWIN) || defined(KPWATCOM) || defined(KPCENTERLINE)
	#define KCMS_NOBCOPY
	#define KCMS_NOBZERO
	#define KCMS_NOBCMP
	extern void	bcopy (KpGenericPtr_t, KpGenericPtr_t, KpInt32_t);
	extern void	bzero (KpGenericPtr_t, KpInt32_t);
	extern int	bcmp (KpGenericPtr_t, KpGenericPtr_t, KpInt32_t);

#elif defined(VAXC)
	extern int	bcmp ();		/* (char * b1, char * b2, int nbytes) */

	#ifdef MIN_RTL
		#define KCMS_NOBCOPY
		#define KCMS_NOBZERO
		#define KCMS_NOBCMP
		extern void	bcopy ();		/* (char * from, char * to, int nbytes) */
		extern void	bzero ();		/* (char * to, int nbytes) */

	#else
		#define bcopy(b1, b2, length)	memmove (b2, b1, length)
		#define bzero(b, length)	memset (b, 0, length)
	#endif					/* defined (MIN_RTL)  */

#elif defined (KPMAC) || defined(KPTHINK)
	#include <Types.h>
	#include <Memory.h>
	#define bcopy(b1, b2, length)	BlockMove((Ptr) b1, (Ptr) b2, (Size) length)
	#define KCMS_NOBZERO
	#define KCMS_NOBCMP
	extern void	bzero (KpGenericPtr_t, KpInt32_t);
	extern int	bcmp (KpGenericPtr_t, KpGenericPtr_t, KpInt32_t);

#else /* If not one of the special cases above */				
	#ifndef	bcopy
		#define bcopy(b1,b2,length)		memmove (b2, b1, length)
		#define bzero(b,length)			memset (b, 0, length)
		#define bcmp(b1,b2,length)		memcmp (b2, b1, length)
	#endif
#endif
#endif	/* !defined nonansi */

#endif /* _BYTES_H_ */






