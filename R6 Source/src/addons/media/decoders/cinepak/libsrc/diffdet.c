/* ex:set sw=2 ts=8 wm=0:
 * $Header: u:/rcs/cv/rcs/diffdet.c 3.4 1994/10/23 17:22:43 bog Exp $
 *
 * (C) Copyright 1992-1994 SuperMac Technology, Inc.
 * All rights reserved

 * This source code and any compilation or derivative thereof is the
 * sole property of SuperMac Technology, Inc. and is provided pursuant
 * to a Software License Agreement.  This code is the proprietary
 * information of SuperMac Technology and is confidential in nature.
 * Its use and dissemination by any party other than SuperMac Technology
 * is strictly limited by the confidential information provisions of the
 * Agreement referenced above.
 *
 * $Log: diffdet.c $
 * Revision 3.4  1994/10/23 17:22:43  bog
 * Try to allow big frames.
 * Revision 3.3  1994/10/20  17:39:09  bog
 * Modifications to support Sunnyvale Reference version.
 * 
 * Revision 3.2  1994/02/23  15:11:26  timr
 * Need to use the Squares wrapper (Sqr) throughout.
 *
 * Revision 3.1  1993/12/13  02:38:18  geoffs
 * Initialize runningSum to 0, was uninitialized before
 *
 * Revision 3.0  1993/12/10  14:23:59  timr
 * Initial revision, NT C-only version.
 *
 * Revision 2.0  93/06/01  14:14:11  bog
 * Version 1.0 Release 1.3.0.1 of 1 June 1993.
 * 
 * Revision 1.5  93/04/21  15:47:55  bog
 * Fix up copyright and disclaimer.
 * 
 * Revision 1.4  93/01/20  15:56:53  timr
 * Do detail & difference list computations for the whole list of patches,
 * not line by line.  Don't be stupid.
 * 
 * Revision 1.3  93/01/12  17:19:39  bog
 * Missed one spot in converting YYYYUV to VECTOR.
 * 

 *
 * Functions to compute the difference list for a pair of tiles, and
 * the detail list for a given tile.
 */

#if	defined(WINCPK)

#define	_WINDOWS
#include <memory.h>		// included for _fmemcpy
#include <stdlib.h>
#ifdef	NULL
#undef	NULL
#endif

#include <windows.h>
#include <windowsx.h>
#include <compddk.h>

#include "w16_32.h"

#endif

#include "cv.h"
#include "cvcompre.h"



#define	DIFFLIMIT	0x7fff


#if 0
#define	Sqr(k)	((unsigned long)(k))*(unsigned long)(k))
#else
#define	Sqr(k)	(Squares[255+(k)])
#endif


unsigned long
DifferenceList (CCONTEXT * pC, TILECONTEXT * pT)
{
    VECTOR HUGE * pOld = pT->pDetail;
    VECTOR HUGE * pNew = pC->VBook [Detail].pVectors;
    short HUGE * pDiffList = pC->pDiffList;
    long wCount = pT->nPatches;
    unsigned long sum;
    unsigned long sum3;
    unsigned long runningSum;

// Byte offsets of the bytes in the patches are:

//      Y          U       V
//  0  1  8  9	 4 12	 5 13
//  2  3 10 11
// 16 17 24 25   20 28   21 29
// 18 19 26 27

    for (runningSum = 0; wCount--; )
    {
    	// Add in 3x values.

        sum3  = Sqr (pOld[0].yuv.y[0] - pNew[0].yuv.y[0]);
        sum3 += Sqr (pOld[1].yuv.y[1] - pNew[1].yuv.y[1]);
        sum3 += Sqr (pOld[2].yuv.y[2] - pNew[2].yuv.y[2]);
        sum3 += Sqr (pOld[3].yuv.y[3] - pNew[3].yuv.y[3]);

	// Add in 2x values.

        sum  = Sqr (pOld[0].yuv.y[1] - pNew[0].yuv.y[1]);
        sum += Sqr (pOld[0].yuv.y[2] - pNew[0].yuv.y[2]);
        sum += Sqr (pOld[1].yuv.y[0] - pNew[1].yuv.y[0]);
        sum += Sqr (pOld[1].yuv.y[3] - pNew[1].yuv.y[3]);
        sum += Sqr (pOld[2].yuv.y[0] - pNew[2].yuv.y[0]);
        sum += Sqr (pOld[2].yuv.y[3] - pNew[2].yuv.y[3]);
        sum += Sqr (pOld[3].yuv.y[1] - pNew[3].yuv.y[1]);
        sum += Sqr (pOld[3].yuv.y[2] - pNew[3].yuv.y[2]);

	sum = sum3 + 2 * (sum + sum3);

	// Add in 1x values.

        sum += Sqr (pOld[0].yuv.y[3] - pNew[0].yuv.y[3]);
        sum += Sqr (pOld[1].yuv.y[2] - pNew[1].yuv.y[2]);
        sum += Sqr (pOld[2].yuv.y[1] - pNew[2].yuv.y[1]);
        sum += Sqr (pOld[3].yuv.y[0] - pNew[3].yuv.y[0]);

	sum >>= 2;		// sumY / 4

	sum += Sqr (pOld[0].yuv.u - pNew[0].yuv.u);
	sum += Sqr (pOld[1].yuv.u - pNew[1].yuv.u);
	sum += Sqr (pOld[2].yuv.u - pNew[2].yuv.u);
	sum += Sqr (pOld[3].yuv.u - pNew[3].yuv.u);
	sum += Sqr (pOld[0].yuv.v - pNew[0].yuv.v);
	sum += Sqr (pOld[1].yuv.v - pNew[1].yuv.v);
	sum += Sqr (pOld[2].yuv.v - pNew[2].yuv.v);
	sum += Sqr (pOld[3].yuv.v - pNew[3].yuv.v);

	sum >>= 2;		// sumY/16 + sumU/4 + sumV/4

	if (sum > DIFFLIMIT)
	    sum = DIFFLIMIT;

	*pDiffList++ = (short) sum;
	runningSum += sum;

	pOld += 4;
	pNew += 4;
    }

    return runningSum;
}

void
DetailList (CCONTEXT * pC, TILECONTEXT * pT)
{
    VECTOR HUGE * pDetail = pC->VBook [Detail].pVectors;
    VECTOR HUGE * pSmooth = pC->VBook [Smooth].pVectors;
    short HUGE * pDetailList = pC->pDetailList;
    long wCount = pT->nPatches;
    int i;

    while (wCount--)
    {
        unsigned long maxErr = 0;

	for (i = 0; i < 4; i++)
	{
	    register unsigned short mean = (
		    pDetail->yuv.y[0] + 
		    pDetail->yuv.y[1] +
		    pDetail->yuv.y[2] + 
		    pDetail->yuv.y[3]
		   ) >> 2;

	    register unsigned long sumsq = 
	        Sqr (pDetail->yuv.y[0] - mean) +
	        Sqr (pDetail->yuv.y[1] - mean) +
	        Sqr (pDetail->yuv.y[2] - mean) +
	        Sqr (pDetail->yuv.y[3] - mean) +
		(Sqr (pDetail->yuv.u - pSmooth->yuv.u) << 2) +
		(Sqr (pDetail->yuv.v - pSmooth->yuv.v) << 2);

	    if (sumsq > DIFFLIMIT)
	        sumsq = DIFFLIMIT;

	    if (sumsq > maxErr)
	        maxErr = sumsq;

	    pDetail ++;
	}

	*pDetailList++ = (short) maxErr;
	pSmooth++;
    }
}
