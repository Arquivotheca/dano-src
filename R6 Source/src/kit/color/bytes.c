/*
 *
 * file: bytes.c
 *
 * SCCSID = @(#)bytes.c	1.5    10/28/94
 *
 * PROPRIETARY NOTICE :   The  software  information  contained herein  is
 * the  sole  property  of  KEPS INC.  and   is provided  to KEPS users  under
 * license for use  on  their designated  equipment  only.  Reproduction of
 * this matter in whole or in part is forbidden without  the  express  written
 * consent of KEPS INC.
 * 
 * COPYRIGHT (c) 1992-1994 KEPS INC. As  an  unpublished  work pursuant to 
 * Title 17 of the United States Code.  All rights reserved.
 */


/*
 * byte manipulation routines for various architectures.
 */
#include "kcms_sys.h"
#include "bytes.h"


#ifdef KCMS_NOBCOPY
/*
 * bcopy() copies len bytes from b1 to b2.  Overlapping buffers
 * are handled correctly.
 */
void
bcopy (KpGenericPtr_t b1, KpGenericPtr_t b2, KpInt32_t len)
{
	KpUInt8_t *b1_8, *b2_8;
	
	b1_8 = (KpUInt8_t *) b1;
	b2_8 = (KpUInt8_t *) b2;
	
	if (b1_8 > b2_8) {
		/* start at beginning and work towards the end */
		while (--len >= 0) {
			*b2_8++ = *b1_8++;
		}

	} else if (b1 < b2) {
		/* start at end and work towards the beginning */
		b1_8 += len;
		b2_8 += len;
		while (--len >= 0) {
			*(--b2_8) = *(--b1_8);
		}

	} else {
		/* b1 == b2, copy not necessary */
	}
}

#endif					/* KCMS_NOBCOPY */

#ifdef KCMS_NOBZERO
/*
 * bzero() sets len byte to 0 starting at b
 */
void
bzero (KpGenericPtr_t b, KpInt32_t len)
{
	KpUInt8_t *b8;
	
	b8 = (KpUInt8_t *)b;
	while (--len >= 0) {
		*b8++ = 0;
	}
}

#endif					/* KCMS_NOBZERO */

#ifdef KCMS_NOBCMP
/*
 * compares two byte arrays.  Returns 0 if they are the same, 1 otherwise.
 */
bcmp (KpGenericPtr_t b1, KpGenericPtr_t b2, KpInt32_t len)
{
	KpUInt8_t *b1_8, *b2_8;
	
	b1_8 = (KpUInt8_t *) b1;
	b2_8 = (KpUInt8_t *) b2;
	
	while (--len >= 0)
		if (*b1_8++ != *b2_8++)
			return (1);

	return (0);
}

#endif					/* KCMS_NOBCMP */


