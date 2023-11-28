// ex:set sw=2 ts=8 wm=0:
// $Header: u:/rcs/cv/rcs/diffdetc.c 2.4 1995/05/09 09:23:20 bog Exp $
/*
 * (C) Copyright 1992-1993 SuperMac Technology, Inc.
 * All rights reserved

 * This source code and any compilation or derivative thereof is the
 * sole property of SuperMac Technology, Inc. and is provided pursuant
 * to a Software License Agreement.  This code is the proprietary
 * information of SuperMac Technology and is confidential in nature.
 * Its use and dissemination by any party other than SuperMac Technology
 * is strictly limited by the confidential information provisions of the
 * Agreement referenced above.
 */
// $Log: diffdetc.c $
// Revision 2.4  1995/05/09 09:23:20  bog
// Move WINVER back into the makefile.  Sigh.
// Revision 2.3  1994/10/23  17:22:45  bog
// Try to allow big frames.
// 
// Revision 2.2  1994/07/18  13:30:46  bog
// Move WINVER definition from makefile to each .c file.
// 
// Revision 2.1  1993/09/23  17:21:29  geoffs
// Now correctly processing status callback during compression
// 
// Revision 2.0  1993/06/01  14:14:11  bog
// Version 1.0 Release 1.3.0.1 of 1 June 1993.
// 
// Revision 1.5  93/04/21  15:47:55  bog
// Fix up copyright and disclaimer.
// 
// Revision 1.4  93/01/20  15:56:53  timr
// Do detail & difference list computations for the whole list of patches,
// not line by line.  Don't be stupid.
// 
// Revision 1.3  93/01/12  17:19:39  bog
// Missed one spot in converting YYYYUV to VECTOR.
// 

//
// Functions to compute the difference list for a pair of tiles, and
// the detail list for a given tile.
//

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>

#include "compddk.h"
#include "cv.h"
#include "cvcompre.h"


// Declare the assembly helpers.

unsigned long
YUVDifferenceInnerLoop (VECTOR huge *, VECTOR huge *, short huge *, long);

void
YUVDetailInnerLoop (VECTOR huge *, VECTOR huge *, short huge *, long);


unsigned long
DifferenceList (CCONTEXT * pC, TILECONTEXT * pT)
{
    return
        YUVDifferenceInnerLoop (
	    pT->pDetail,
	    pC->VBook [Detail].pVectors,
	    pC->pDiffList,
	    pT->nPatches
	);
}


void
DetailList (CCONTEXT * pC, TILECONTEXT * pT)
{
    YUVDetailInnerLoop (
	pC->VBook [Detail].pVectors,
	pC->VBook [Smooth].pVectors,
	pC->pDetailList,
	pT->nPatches
    );
}
