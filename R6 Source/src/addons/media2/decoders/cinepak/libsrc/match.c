/* ex:set sw=2 ts=8 wm=0:
 * $Header: u:/rcs/cv/rcs/match.c 3.2 1995/03/07 14:50:29 bog Exp $

 * (C) Copyright 1992-1994 SuperMac Technology, Inc.
 * All rights reserved

 * This source code and any compilation or derivative thereof is the sole
 * property of SuperMac Technology, Inc. and is provided pursuant to a
 * Software License Agreement.  This code is the proprietary information
 * of SuperMac Technology and is confidential in nature.  Its use and
 * dissemination by any party other than SuperMac Technology is strictly
 * limited by the confidential information provisions of the Agreement
 * referenced above.

 * $Log: match.c $
 * Revision 3.2  1995/03/07 14:50:29  bog
 * Fix comments.
 * Revision 3.1  1994/10/20  17:44:40  bog
 * Modifications to support Sunnyvale Reference version.
 * 
 * Revision 3.0  1993/12/10  14:24:23  timr
 * Initial revision, NT C-only version.
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


// CompactVideo Match a vector to the nearest codebook entry


//////////////////////////////////////////////////////////////////////


#define	Invalid(f)	(((f)->Flags ^ Flags) & (fINVALID | Flags))


//////////////////////////////////////////////////////////////////////


unsigned long Match(
    VECTOR FAR *pV,		// vector to match
    EXTCODE FAR *pEC,		// -> base of code table
    short nCodes,		// number of codes against which to match
    unsigned short nNeighbors,	// number of entries in the Neighbor[] table
    unsigned char Flags		// whether to look at codes that are locked
)
{
    VECTOR V;			// local copy of vector we're matching
    EXTCODE FAR * pCurr;	// current code under consideration
    EXTCODE FAR * pTry;		// possible closer code
    DWORD CurrError;		// squared distance from V to *pCurr
    DWORD BestError;		// best error found so far
    EXTCODE FAR * pBest;	// best code found so far
    BYTE Best;			// best code so far
    unsigned short iAbove, iMid, iBottom, ix;

// Match a vector to the nearest valid entry in a codebook.
// Update pV->Code.

// nCodes assumed to be > 0; SplitCodes would run first.

// Return the match error.

/**********************************************************************
 *
 * Each codebook entry has in it a list of the NEIGHBORS nearest other
 * codebook entries.  Since the incoming vector previously matched
 * pV->Code, it is likely that it is still near that same entry since
 * the only motion we've had is successive refinement.
 *
 * If the distance from the codebook entry we're looking at to its
 * nearest neighbor is more than twice the disance (four times the
 * squared distance) from the entry to the vector, we know that no other
 * code can be closer than the one we're looking at, so we have our
 * result.
 *
 * If the vector is farther from the current code than half the distance
 * to the code's nearest neighbor, we look through the nearest neighbor
 * list.  We divide the neighbors into those that are closer than twice
 * the distance from the vector to the codebook entry and those that are
 * equal in distance or farther.  If there are any nearer, we know that
 * the closest codebook entry is among that set, so we linearly search
 * for the closest entry.
 *
 * If every neighbor is less than twice as far from the codebook entry
 * as the vector is, we search through the neighbors list for a codebook
 * entry that is nearer the vector than the codebook entry.  If we find
 * one, we make it the current entry and start again at the top.
 *
 * If no neighbor in the list is nearer the vector than the current code
 * (an unusual circumstance) then we must grossly search the coodbook
 * for the the first entry nearer the vector than the current one, and
 * start again.
 *
 * If no other entry in the list is closer than the current one, then
 * the current one is obviously the nearest code.
 *
 **********************************************************************/

// C implementation details:
//
// There are four states here.  At least I think so.

// TryAgain -- look at a new EXTCODE -- neighbors table might be useful.
//    exits to TryNeighbor if closest is within neighbors table.
//    exits to Walk if closest MIGHT be within neighbors table.
//    exits to Linear if closest is definitely not within neighbors table.

// TryNeighbor -- do binary search of neighbors table.
//    returns if closest is found.
//    exits to Walk if no neighbor is .LT. twice the distance.

// Walk -- do linear search of neighbors table.
//    exits to Linear once we exhaust table.
//    exits to TryAgain if we find a neighbors closer than current EXTCODE.

// Linear -- linearly search the whole EXTCODE list.
//    returns when EXTCODE list is exhausted.
//    exits to TryAgain if we find an EXTCODE closer that current.


    V = *pV;
    pCurr = &pEC [V.Code];

    if (Invalid(pCurr))
    {
	CurrError = 0x7fffffff;		// no valid entry yet
	BestError = 0x7fffffff;
        goto Linear;
    }

    // Compute distance from V to *pCurr->

    CurrError = LongDistance (V.yuv, pCurr->yuv);

    goto TryAgain;


///////////////////////////////////////////////////////////////////////
//
// State TryAgain

// Entry:  
//   pCurr = EXTCODE under consideration
//   CurrError = distance (V, pCurr)

TryAgain:
    pBest = pCurr;
    BestError = CurrError;
    Best = pCurr->Code;

///////////////////////////////////////////////////////////////////////

// If less than half the distance to Curr's nearest neighbor, we have
// found the nearest code.

///////////////////////////////////////////////////////////////////////

  // if farthest Neighbor is nearer than twice the distance from the
  // vector to Curr, then there is no guarantee that the best match is in
  // the Neighbors table.  Nerping through the Neighbor table thus
  // doesn't help us.

    if (nNeighbors == 0 || BestError > 16383)
    {
        goto Linear;
    }

    if ((BestError << 2) < pCurr->Neighbor[0].HowFar)
    {
	pV->Code = pCurr->Code;
        return BestError;
    }

    if ((BestError << 2) > pCurr->Neighbor[nNeighbors-1].HowFar)
    {
        goto Walk;		// no neighbor is close enough to hit
    }
    else
    {
	goto TryNeighbor;
    }

///////////////////////////////////////////////////////////////////////

// If we are as close as half the distance to any neighbor, our
// closest match must be among those in the list from Curr up to but
// not including that neighbor.

// We find the dividing line by bisection.

///////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////
//
// State TryNeighbor

// Entry:  
//   pCurr = EXTCODE under consideration
//   CurrError = distance (V, pCurr)
//   BestError = smallest distance encountered so far

TryNeighbor:
    iAbove = nNeighbors;
    iBottom = 0;

    while (iAbove > iBottom)
    {
        iMid = (iAbove + iBottom - 1) / 2;

	if ((CurrError << 2) >= pCurr->Neighbor [iMid].HowFar)
	{
	    iBottom = iMid + 1;
	}
	else
	{
	    iAbove = iMid;
	}
    }

    if (iAbove >= nNeighbors)
    {
	goto Walk;
    }

///////////////////////////////////////////////////////////////////////

// Either Curr or one of the first iAbove codebook entries in the
// neighbor list is the winner.

// We simply grind through, looking for the best.

// BestError and pBest were initialized above

///////////////////////////////////////////////////////////////////////

    while (iAbove--)
    {
        long tmpError;

        pTry = pCurr->Neighbor [iAbove].pWho;

	// We unroll the LongDistance macro here so we can do early outs.

	tmpError = 
	    (
	      (
	        Squares [255 + ((short) V.yuv.u) - ((short) pTry->yuv.u)] +
		Squares [255 + ((short) V.yuv.v) - ((short) pTry->yuv.v)]
	      ) << 2
	    ) - BestError;

	if (tmpError < 0 &&
	
	    (tmpError += Squares [255 + 
	        (short) V.yuv.y[0] - 
		(short) pTry->yuv.y[0]]) <= 0 &&

	    (tmpError += Squares [255 + 
		(short) V.yuv.y[1] - 
		(short) pTry->yuv.y[1]]) <= 0 &&

	    (tmpError += Squares [255 + 
		(short) V.yuv.y[2] - 
		(short) pTry->yuv.y[2]]) <= 0 &&

	    (tmpError += Squares [255 + 
		(short) V.yuv.y[3] - 
		(short) pTry->yuv.y[3]]) <= 0 &&

	    (tmpError < 0 || pTry->Code < Best)
	)

	{
	    BestError += tmpError;
	    pBest = pTry;
	    Best = pTry->Code;
	}
    }

    pV->Code = pBest->Code;
    return BestError;


///////////////////////////////////////////////////////////////////////
//
// State Walk
//
// Entry:
//    pCurr = EXTCODE under consideration
//    BestError = setup


///////////////////////////////////////////////////////////////////////

// There might be some codebook entry not in the neighbor list that
// is closer to the vector than any in the neighbor list.  If it
// turns out that the vector is closer to some neighbor than to
// Curr, we just start over again with that one being Curr.
// Otherwise, we must linearly search.


// Walk through the Neighbors table until we either fall off the end
// or find a neighbor nearer than Curr.

///////////////////////////////////////////////////////////////////////


Walk:
    for (
        ix = 0;
	ix < nNeighbors; 
	ix++)
    {
        long tmpError;

        pTry = pCurr->Neighbor [ix].pWho;

	// We unroll the LongDistance macro here so we can do early outs.

	tmpError = 
	    (
	      (
	        Squares [255 + ((short) V.yuv.u) - ((short) pTry->yuv.u)] +
		Squares [255 + ((short) V.yuv.v) - ((short) pTry->yuv.v)]
	      ) << 2
	    ) - BestError;

	if (tmpError < 0 &&
	
	    (tmpError += Squares [255 + 
	        (short) V.yuv.y[0] - 
		(short) pTry->yuv.y[0]]) < 0 &&

	    (tmpError += Squares [255 + 
		(short) V.yuv.y[1] - 
		(short) pTry->yuv.y[1]]) < 0 &&

	    (tmpError += Squares [255 + 
		(short) V.yuv.y[2] - 
		(short) pTry->yuv.y[2]]) < 0 &&

	    (tmpError += Squares [255 + 
		(short) V.yuv.y[3] - 
		(short) pTry->yuv.y[3]]) < 0
	)
	{
	    // The code at pTry is closer than pCurr.  Make it pCurr and
	    // start again.

	    CurrError = tmpError + BestError;
	    pCurr = pTry;
	    goto TryAgain;
	}
    }

///////////////////////////////////////////////////////////////////////
//
// State Linear
//
// Entry:
//    pCurr = EXTCODE under consideration
//    pBest = best choice from neighbor list
//    BestError = distance from V to pBest



///////////////////////////////////////////////////////////////////////

// Continue our linear search from wherever we last left off.

// We break out of our search if we run off the end of the codebook
// (in which case Curr is our best match) or if we find a code that
// is closer to our vector than Curr, in which case we switch to it
// and start again.

///////////////////////////////////////////////////////////////////////


Linear:

    while (nCodes--)
    {
        long tmpError;

        pTry = pEC++;

	if (!Invalid (pTry) && pTry->Code != pCurr->Code)
	{
	    // We unroll the LongDistance macro here so we can do early outs.

	    tmpError = 
		(
		  (
		    Squares [255 + ((short) V.yuv.u) - ((short) pTry->yuv.u)] +
		    Squares [255 + ((short) V.yuv.v) - ((short) pTry->yuv.v)]
		  ) << 2
		) - BestError;

	    if (tmpError < 0 &&
	
		(tmpError += Squares [255 + 
		    (short) V.yuv.y[0] - 
		    (short) pTry->yuv.y[0]]) <= 0 &&

		(tmpError += Squares [255 + 
		    (short) V.yuv.y[1] - 
		    (short) pTry->yuv.y[1]]) <= 0 &&

		(tmpError += Squares [255 + 
		    (short) V.yuv.y[2] - 
		    (short) pTry->yuv.y[2]]) <= 0 &&

		(tmpError += Squares [255 + 
		    (short) V.yuv.y[3] - 
		    (short) pTry->yuv.y[3]]) <= 0 &&
	
		(tmpError < 0 || pTry->Code < Best)
	    )
	    {
		// The code at pTry is closer than pCurr.  Make it pCurr and
		// start again.

		CurrError = tmpError + BestError;
		pCurr = pTry;
		goto TryAgain;
	    }
	}
    }

    // We finished out search.  pCurr has the best match.

    pV->Code = pCurr->Code;
    return CurrError;
}
