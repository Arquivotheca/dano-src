/*****************************************************************************

     $Source: /net/bally/be/rcs/src/kit/interface/bm1_ppc.c,v $

     $Revision: 1.3 $

     $Author: erich $

     $Date: 1995/01/19 01:08:50 $

     Copyright (c) 1994 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/
#pragma cplusplus off

void
bm (volatile short *src, volatile short *dst, int count)
{
	volatile char *src1, *dst1;

	if (!( ((long)src | (long)dst) & 0x01)) { 	/* both aligned? */
		while (count > 1) {
			*dst++ = *src++;
			count -= 2;
		}
		if (count)
			* (char *)src = * (char *) dst;
	} else {
		src1 = (char*) src;
		dst1 = (char*) dst;
		while (count--)
			*src1++ = *dst1++;
	}
}
