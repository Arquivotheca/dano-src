/* ex:set sw=2 ts=8 wm=0:
 * $Header: u:/rcs/cv/rcs/generate.c 3.3 1995/03/14 08:24:13 bog Exp $

 * (C) Copyright 1992-1994 SuperMac Technology, Inc.
 * All rights reserved

 * This source code and any compilation or derivative thereof is the sole
 * property of SuperMac Technology, Inc. and is provided pursuant to a
 * Software License Agreement.  This code is the proprietary information
 * of SuperMac Technology and is confidential in nature.  Its use and
 * dissemination by any party other than SuperMac Technology is strictly
 * limited by the confidential information provisions of the Agreement
 * referenced above.

 * $Log: generate.c $
 * Revision 3.3  1995/03/14 08:24:13  bog
 * 1.  No one was looking at the remembered previous frame's smooth
 *     vectors, so there is no point in remembering them.
 * 2.  We update the previous frame's remembered detail vectors to be as
 *     refined (quantized) rather than as incoming, improving the decision
 *     about what to update in the next frame.
 * Revision 3.2  1994/10/23  17:22:50  bog
 * Try to allow big frames.
 * 
 * Revision 3.1  1994/10/20  17:40:28  bog
 * Modifications to support Sunnyvale Reference version.
 * 
 * Revision 3.0  1993/12/10  14:24:21  timr
 * Initial revision, NT C-only version.
 *
 
 * CompactVideo Codec Toss unused vectors
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

#endif

#include "cv.h"
#include "cvcompre.h"

void
GenerateVectors (
   CCONTEXT	*pC,
   TILECONTEXT	*pT
)
{
    long nPatches;
    BYTE FrameType;
    VECTOR huge * pDetailIn;
    VECTOR huge * pDetailOut;
    VECTOR huge * pSmoothIn;
    VECTOR huge * pSmoothOut;
    WORD HUGE * DetailMap;
    WORD HUGE * DiffMap;
    long p;

// Swap the pointers in pC->VBook [Detail].pVectors and pT->Detail.

    pDetailIn = pC->VBook [Detail].pVectors;
    pDetailOut = pT->pDetail;
    pT->pDetail = pDetailIn;
    pC->VBook [Detail].pVectors = pDetailOut;

// Swap the pointers in pC->VBook [Smooth].pVectors and pT->Smooth.

    pSmoothIn = pSmoothOut = pC->VBook [Smooth].pVectors;

// Tuck static data in the stack frame.

    FrameType = pC->fType;
    nPatches = pT->nPatches;
    DetailMap = pC->pDetailList;
    DiffMap = pC->pDiffList;

// Set vector counts into pC->VBook [x].nVectors.

    pC->VBook [Detail].nVectors = pC->DetailCount * 4;
    pC->VBook [Smooth].nVectors = pC->DiffCount - pC->DetailCount;

// Note that DetailMap[k] is always zero if DiffMap[k] is zero.

    for (p = 0; p < nPatches; p++, pDetailIn+= 4, pSmoothIn++)
    {
        if (DetailMap [p] != 0)
	{
	    // Copy 4 vectors from DetailIn to DetailOut.

	    pDetailOut [0] = pDetailIn [0];
	    pDetailOut [0].Class = 0;
	    pDetailOut [1] = pDetailIn [1];
	    pDetailOut [1].Class = 0;
	    pDetailOut [2] = pDetailIn [2];
	    pDetailOut [2].Class = 0;
	    pDetailOut [3] = pDetailIn [3];
	    pDetailOut [3].Class = 0;

	    pDetailOut += 4;
	}
	else if (FrameType == kKeyFrameType || DiffMap [p] != 0)
	{
	    // Copy 1 vector from SmoothIn to SmoothOut.

	    pSmoothOut [0] = *pSmoothIn;
	    pSmoothOut [0].Class = 0;

	    pSmoothOut++;
	}
    }
}
