/* ex:set sw=2 ts=8 wm=0:
 * $Header: u:/rcs/cv/rcs/splitcod.c 3.8 1995/10/02 11:43:26 bog Exp $

 * (C) Copyright 1992-1994 SuperMac Technology, Inc.
 * All rights reserved

 * This source code and any compilation or derivative thereof is the
 * sole property of SuperMac Technology, Inc. and is provided pursuant
 * to a Software License Agreement.  This code is the proprietary
 * information of SuperMac Technology and is confidential in nature.
 * Its use and dissemination by any party other than SuperMac Technology
 * is strictly limited by the confidential information provisions of the
 * Agreement referenced above.

 * $Log: splitcod.c $
 * Revision 3.8  1995/10/02 11:43:26  bog
 * No need to check fINVALID on swap.  Now can't get divide fault.
 * 
 * Revision 3.7  1995/09/25 13:41:10  bog
 * Weird case where all vectors matched pHole, which was not fINVALID.
 * Caused divide error.
 * 
 * Revision 3.6  1995/05/09 09:23:41  bog
 * Move WINVER back into the makefile.  Sigh.
 *
 * Revision 3.5  1994/10/23  17:23:08  bog
 * Try to allow big frames.
 * 
 * Revision 3.4  1994/10/20  17:47:13  bog
 * Modifications to support Sunnyvale Reference version.
 * 
 * Revision 3.3  1994/07/18  13:31:00  bog
 * Move WINVER definition from makefile to each .c file.
 * 
 * Revision 3.2  1994/05/25  00:02:13  timr
 * Loop was still terminating incorrectly.
 * 
 * Revision 3.1  1994/02/23  07:13:49  timr
 * Rework FindNonIdentical inner loop; I was decremeting counter too fast.
 *
 * Revision 3.0  1993/12/10  14:49:53  timr
 * Incorporate splitasm.asm for NT C-only codec.
 *
 * Revision 2.2  1993/08/04  19:09:57  timr
 * Both compressor and decompressor now work on NT.
 *
 * Revision 2.1  1993/07/16  14:54:59  geoffs
 * Add VER.H.  (checked in by timr)
 *
 * Revision 2.0  93/06/01  14:17:26  bog
 * Version 1.0 Release 1.3.0.1 of 1 June 1993.
 *
 * Revision 1.8  93/04/21  15:59:34  bog
 * Fix up copyright and disclaimer.
 *
 * Revision 1.7  93/02/01  11:28:39  timr
 * Correct potential far/huge bug - if the first vector in a class was
 * beyond offset 65535, we would have started looking at the wrong vector.
 *
 * Revision 1.6  93/01/27  23:03:53  bog
 * Get interframes working.
 *
 * Revision 1.5  93/01/26  11:22:24  timr
 * The SplitTwoEntries return area needs to be in SS, not in the shared DS.
 *
 * Revision 1.4  93/01/26  11:16:01  timr
 * Assemblerize the inner loops into SplitASM.ASM.
 *
 * Revision 1.3  93/01/20  17:22:20  bog
 * Compression works!
 *
 * Revision 1.2  93/01/19  14:58:44  bog
 * Adapt to inter codebooks.
 *
 * Revision 1.1  93/01/18  20:17:11  bog
 * Initial revision
 *
 *
 * CompactVideo Fill out a codebook by splitting
 */

#if	defined(WINCPK)

#define	_WINDOWS
#include <stdio.h>
#include <memory.h>		// included for _fmemcpy
#include <stdlib.h>
#ifdef	NULL
#undef	NULL
#endif

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <commdlg.h>
#include <compddk.h>

#endif

#include "cv.h"
#include "cvcompre.h"

VECTOR huge * FindNonIdentical (
    VECTOR huge *, 	// Pointer to vector list
    EXTCODE _far *, 	// Pointer to old codebook entry
    BYTE, 		// Class of old entry
    BYTE		// Codebook
);

typedef struct {
    long	nMatches [2];
    long	Error [2];
}		S2EParms;

void
SplitTwoEntries (
    VECTOR huge *, 	// Pointer to vector list
    YYYYUV,		// Old codebook YUV
    unsigned char, 	// Code number of old entry
    YYYYUV,		// Split entry YUV
    unsigned char, 	// Code number of split entry
    unsigned char, 	// Class number of old entry
    long,		// Number of vectors in list
    LYYYYUV *, 		// Spot to accumulate error from old entry
    LYYYYUV *, 		// Spot to accumulate error from new entry
    S2EParms *		// Spot for return values
);


/**********************************************************************
 *
 * SplitCodeBook()
 *
 * Fill out a codebook by splitting the code with the maximum error
 *
 **********************************************************************/

short SplitCodeBook(
  VECTORBOOK *pVB,		// vectors and codebook to fill out
  short iFirstCode,		// index of first candidate codebook entry
  long iFirstVector,		// index of first vector in class
  unsigned char Class,		// vector class to search
  EXTCODE far * far *ppHoles,	// ->-> first hole to fill
  short nHoles,			// number of holes to fill
  unsigned char IgnFlags	// ignore entries with any of these bits set
) {
  auto EXTCODE far *pHoles;

  auto int nCodesAssigned;	// number of codes actually assigned

  auto EXTCODE far *p1stEC;	// -> first code to look at
  auto int nEC;			// number of codes to look at
  auto VECTOR huge *p1stV;	// -> first vector to look at
  auto long nV;			// number of vectors to look at

  auto unsigned char HoleCode;

  pHoles = *ppHoles;

  for (				// until holes gone or break
    p1stEC = &pVB->pECodes[iFirstCode],
      nEC = pVB->nCodes - iFirstCode,
      p1stV = &((VECTOR huge *)pVB->pVectors)[iFirstVector],
      nV = pVB->nVectors - iFirstVector,
      nCodesAssigned = 0;
    nHoles;
  ) {
    auto int i;

    auto VECTOR huge *pV;
    auto EXTCODE far *pEC;

    auto EXTCODE far *pOldC;
    auto LYYYYUV Old;		// for accumulating new mean
    auto unsigned char OldCode;
    auto LYYYYUV Hole;

    auto unsigned long MaxErrSoFar;

    auto S2EParms sparms;	// return values from SplitTwoEntries

    HoleCode = pHoles->Code;

    /*
      Find the code we're going to split.
     */
    for (
      pEC = p1stEC,
	i = nEC,
	MaxErrSoFar = 0;
      i--;
      pEC++
    ) {
      if (!(pEC->Flags & IgnFlags)) {// fINVALID or (fINVALID | fUNLOCKED)

	if (pEC->Error > MaxErrSoFar) {// if this one has more error

	  MaxErrSoFar = pEC->Error;// remember this as max
	  pOldC = pEC;
	  OldCode = pEC->Code;
	}
      }
    }
    if (MaxErrSoFar == 0) {	// fewer different vectors than codes
      break;
    }
    /*
      Find first nonidentical matching vector.
     */

    pV = FindNonIdentical (p1stV, pOldC, Class, OldCode);

    if (!pV) {// if no nonidentical matching vector
      /*
	We found no matching vector that was different from the
	codebook entry.  This is because the max error we search
	for above is one generation old.

	We just set the error to 0 and try again.
       */
      pOldC->Error = 0;

    } else {

      pHoles->yuv = pV->yuv;

      /*
	We now split vectors in this class matching OldCode
	between the code at pOldC and the code at pHoles.

	We accumulate the yuv for each match so that we can move
	the codes to the mean of the matching vectors.
       */

      SplitTwoEntries (p1stV, pOldC->yuv, OldCode, pHoles->yuv, HoleCode,
	  Class, nV, &Old, &Hole, &sparms);

      pOldC->nMatches = sparms.nMatches [0];
      pOldC->Error = sparms.Error [0];
      pHoles->nMatches = sparms.nMatches [1];
      pHoles->Error = sparms.Error [1];

      /*
	If there were no matches to the old code and pHoles points to an
	invalid code, we swap pOldC and pHole, with appropriate swaps
	elsewhere.
       */
      if (
	!pOldC->nMatches &&	// if no match to old code
	(pOldC->Flags & fUNLOCKED)// and it's unlocked
      ) {
	auto EXTCODE far *pTempC;
	auto LYYYYUV TempYuv;

	pTempC = pOldC;		// swap pOldC with pHoles
	pOldC = pHoles;
	pHoles = pTempC;
	TempYuv = Old;		// swap Old with Hole
	Old = Hole;
	Hole = TempYuv;
	OldCode = pOldC->Code;	// update OldCode, HoleCode
	HoleCode = pHoles->Code;
	pOldC->Flags = fUNLOCKED;	// code now valid
	pHoles->pHole = pOldC->pHole;	// old now into hole chain
	pHoles->Flags |= fINVALID;// holes are invalid so won't get matched
      }

      /*
	Move old and new code to centroid of matching vectors.
       */
      if (pOldC->Flags & fUNLOCKED) {// if not locked

	Old.y[0] = (Old.y[0] + (pOldC->nMatches >> 1)) / pOldC->nMatches;
	Old.y[1] = (Old.y[1] + (pOldC->nMatches >> 1)) / pOldC->nMatches;
	Old.y[2] = (Old.y[2] + (pOldC->nMatches >> 1)) / pOldC->nMatches;
	Old.y[3] = (Old.y[3] + (pOldC->nMatches >> 1)) / pOldC->nMatches;
	Old.u = (Old.u + (pOldC->nMatches >> 1)) / pOldC->nMatches;
	Old.v = (Old.v + (pOldC->nMatches >> 1)) / pOldC->nMatches;

	if (
	  (pOldC->yuv.y[0] != (unsigned char) Old.y[0]) ||
	  (pOldC->yuv.y[1] != (unsigned char) Old.y[1]) ||
	  (pOldC->yuv.y[2] != (unsigned char) Old.y[2]) ||
	  (pOldC->yuv.y[3] != (unsigned char) Old.y[3]) ||
	  (pOldC->yuv.u != (unsigned char) Old.u) ||
	  (pOldC->yuv.v != (unsigned char) Old.v)
	) {

	  pOldC->yuv.y[0] = (unsigned char) Old.y[0];
	  pOldC->yuv.y[1] = (unsigned char) Old.y[1];
	  pOldC->yuv.y[2] = (unsigned char) Old.y[2];
	  pOldC->yuv.y[3] = (unsigned char) Old.y[3];
	  pOldC->yuv.u = (unsigned char) Old.u;
	  pOldC->yuv.v = (unsigned char) Old.v;

	  pOldC->Flags |= fUPDATED;// note this code updated
	}
      }
      if (pHoles->nMatches) {	// if there were matches to new

	pHoles->yuv.y[0] = (unsigned char) (
	  (Hole.y[0] + (pHoles->nMatches >> 1)) / pHoles->nMatches
	);
	pHoles->yuv.y[1] = (unsigned char) (
	  (Hole.y[1] + (pHoles->nMatches >> 1)) / pHoles->nMatches
	);
	pHoles->yuv.y[2] = (unsigned char) (
	  (Hole.y[2] + (pHoles->nMatches >> 1)) / pHoles->nMatches
	);
	pHoles->yuv.y[3] = (unsigned char) (
	  (Hole.y[3] + (pHoles->nMatches >> 1)) / pHoles->nMatches
	);
	pHoles->yuv.u = (unsigned char) (
	  (Hole.u + (pHoles->nMatches >> 1)) / pHoles->nMatches
	);
	pHoles->yuv.v = (unsigned char) (
	  (Hole.v + (pHoles->nMatches >> 1)) / pHoles->nMatches
	);

	nCodesAssigned++;
	pHoles->Flags = fUPDATED | fUNLOCKED; // it's valid, unlocked, updated
	pHoles = pHoles->pHole;
	nHoles--;
      }
    }
  }
  *ppHoles = pHoles;
  return(nCodesAssigned);
}


#ifdef	NOASM

//
//  SplitCodeBook()
//
//  Fill out a codebook by splitting the code with the maximum error
//

VECTOR huge * FindNonIdentical (
    VECTOR huge * lpV,
    EXTCODE FAR * lpOldC,
    BYTE class,
    BYTE oldcode
)
{
    int counter = lpOldC->nMatches;

// Find the first nonidentical matching vector.

// We return upon finding the first vector which:
//  - is in the correct class
//  - is matched to the current codebook entry,
//  - is not identical to the current YUV.

    while (counter)
    {
        if (lpV->Code == oldcode &&
	    lpV->Class == class)
	{
	    counter--;
	    if (
		lpV->yuv.y[0] != lpOldC->yuv.y[0] ||
		lpV->yuv.y[1] != lpOldC->yuv.y[1] ||
		lpV->yuv.y[2] != lpOldC->yuv.y[2] ||
		lpV->yuv.y[3] != lpOldC->yuv.y[3] ||
		lpV->yuv.u != lpOldC->yuv.u ||
		lpV->yuv.v != lpOldC->yuv.v
	    )
	        return lpV;
	}
        lpV++;
    }

    return NULL;
}


void
SplitTwoEntries (
    VECTOR huge *	pV, 	// Pointer to vector list
    YYYYUV		yOldC,	// Old codebook YUV
    unsigned char	Code,	// Code number of old entry
    YYYYUV		yHole,	// Split entry YUV
    unsigned char	HoleCode, 	// Code number of split entry
    unsigned char	Class, 	// Class number of old entry
    long		count,	// Number of vectors in list
    LYYYYUV *		Old, 	// Spot to accumulate error from old entry
    LYYYYUV *		Hole, 	// Spot to accumulate error from new entry
    S2EParms *		rtnval	// Spot for return values
)
{

// Clear Old and Hole.

    memset ((void *) Old, 0, sizeof (LYYYYUV));
    memset ((void *) Hole, 0, sizeof (LYYYYUV));
    memset ((void *) rtnval, 0, sizeof (S2EParms));

// We now split vectors in this class matching OldCode between the code at
// pOldC and the code at pHoles.  We accumulate the yuv for each match so
// that we can move the codes to the mean of the matching vectors.

    while (count--)
    {
        if (pV->Class == Class && pV->Code == Code)
	{
	    DWORD OldDist;
	    DWORD HoleDist;

// Compute distance from pV to pOldC.

	    OldDist = LongDistance (pV->yuv, yOldC);

// Compute distance from pV to pHole.

	    HoleDist = LongDistance (pV->yuv, yHole);

// Add this vector into whichever side was closer.

	    if (OldDist > HoleDist)
	    {
		rtnval->nMatches [1]++;
		rtnval->Error [1] += HoleDist;

		Hole->y[0] += yHole.y[0];
		Hole->y[1] += yHole.y[1];
		Hole->y[2] += yHole.y[2];
		Hole->y[3] += yHole.y[3];
		Hole->u    += yHole.u;
		Hole->v    += yHole.v;

		// Attach to new codebook entry.

		pV->Code = HoleCode;
	    }
	    else
	    {
		rtnval->nMatches [0]++;
		rtnval->Error [0] += OldDist;

		Old->y[0] += yOldC.y[0];
		Old->y[1] += yOldC.y[1];
		Old->y[2] += yOldC.y[2];
		Old->y[3] += yOldC.y[3];
		Old->u    += yOldC.u;
		Old->v    += yOldC.v;
	    }
	}

	pV++;
    }
}

#endif
