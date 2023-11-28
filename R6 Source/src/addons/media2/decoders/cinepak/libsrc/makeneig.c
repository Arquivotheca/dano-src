/* ex:set sw=2 ts=8 wm=0:
 * $Header: u:/rcs/cv/rcs/makeneig.c 3.4 1994/10/20 17:44:15 bog Exp $

 * (C) Copyright 1992-1994 SuperMac Technology, Inc.
 * All rights reserved

 * This source code and any compilation or derivative thereof is the sole
 * property of SuperMac Technology, Inc. and is provided pursuant to a
 * Software License Agreement.  This code is the proprietary information
 * of SuperMac Technology and is confidential in nature.  Its use and
 * dissemination by any party other than SuperMac Technology is strictly
 * limited by the confidential information provisions of the Agreement
 * referenced above.

 * $Log: makeneig.c $
 * Revision 3.4  1994/10/20 17:44:15  bog
 * Modifications to support Sunnyvale Reference version.
 * Revision 3.3  1994/04/30  14:45:30  bog
 * Measurement of performance improvement from skipping InsertNeighbor:
 * 15,116,516 actual inserts of 48,037,808 possible on 46 frames of bike.avi.
 * 
 * Revision 3.2  1994/04/30  10:41:08  unknown
 * Avoid calling InsertNeighbor if item cannot be in list.
 *
 * Revision 3.1  1994/02/23  15:12:58  timr
 * nSlotsNewSoFar was used without being initialized in some paths.
 *
 * Revision 3.0  1993/12/10  06:24:22  timr
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


// CompactVideo Key CodeBook Build and Refine


#define	Invalid(f)	(((f)->Flags ^ Flags) & (fINVALID | Flags))

int InsertNeighbor (
    NEIGHBOR far * pNeighbors,	// 0th entry in neighbor table
    int nNow,			// number of entries now in table
    int nMax,			// maximum number of entries in table
    unsigned short dNew,	// distance of entry to insert
    EXTCODE BASED_SELF *pNew	// who that neighbor is
);


#ifdef	PERFORMANCE_MEASURE
long PossibleInserts = 0;
long ActualInserts = 0;
#endif


/**********************************************************************
 *
 * MakeNeighbors()
 *
 * Build the nearest neighbors part of the codebook extension
 *
 * Return # neighbors in neighbor tables
 *
 **********************************************************************/

short MakeNeighbors(
    EXTCODE FAR *pEC,		// extended codebook
    int nCodes,			// number of entries
    unsigned short nNeighbors,	// maximum Neighbors in list
    unsigned char Flags		// whether to look at codes that are locked
)

//  Run through the codes in pEC, building a nearest neighbors list for
//  each entry.
//
//  Each codebook entry has a list of that code's nearest neighbors.  We
//  maintain this list to speed up the "find nearest match" process.
//  There are NEIGHBORS slots in an EXTCODE for neighbors.  Each slot
//  holds the code for a neighbor and the distance to that neighbor.

{
    int nSlotsNewSoFar = 0;	// number of slots built in new

    if (!nNeighbors)		// if we aren't building neighbors, exit
    {
        return 0;
    }

// Skip any invalid codes at the beginning

    while (nCodes && Invalid(pEC))
    {
        nCodes--;
	pEC++;
    }

    if (nCodes)			// if there are some valid codes
    {
	unsigned short iNew;	// new code to consider
	unsigned short iValidNew;// counting valid codes
	EXTCODE far *pNew;	// -> new code to consider
	unsigned char iPrev;	// already built code we're looking at
	EXTCODE far *pPrev;	// -> already built code we're looking at

	for (
	    iNew = 1,
	      iValidNew = 1,
	      pNew = pEC + 1;
	    iNew < (unsigned) nCodes;
	    iNew++,
	      pNew++
	) {
	    int PrevToCheck;	// counting down already built codes
	    int nSlotsSearch;	// number of slots to search in old
	    int nSlotsNew;	// number of slots to build in new

	    if (!Invalid(pNew))	 // look only at valid codes
	    {

//	  We cannot build more than iValidNew slots in the new guy nor
//	  search more than iValidNew-1 slots in the previous guys.

		nSlotsNew = (iValidNew <= nNeighbors) 
			? iValidNew 
			: nNeighbors;
		nSlotsSearch = (iValidNew <= nNeighbors) 
			? (iValidNew - 1) 
			: nNeighbors;

		for (
		    iPrev = 0,
		      pPrev = pEC,
		      PrevToCheck = iNew,
		      nSlotsNewSoFar = 0;
		    PrevToCheck--;
		    iPrev++,
		      pPrev++
		) {
		    unsigned long LDist;	// distance between pPrev-> and pNew->
		    unsigned short Distance;	// pinned at ffff

		    if (!Invalid (pPrev)) // look only at valid codes
		    {

			LDist = LongDistance (pPrev->yuv, pNew->yuv);
			Distance = (LDist > 0x0000ffffL) 
				? 0xffff 
				: (unsigned short) LDist;

//	    Check and insert new in previous neighbor list

#ifdef	PERFORMANCE_MEASURE
			PossibleInserts += 2;
#endif

			if (
			    (nSlotsSearch < nSlotsNew) ||
			    (Distance < pPrev->Neighbor[nSlotsSearch - 1].HowFar)
			) {
#ifdef	PERFORMANCE_MEASURE
			    ActualInserts++;
#endif
			    InsertNeighbor (
				pPrev->Neighbor,
				nSlotsSearch,
				nSlotsNew,
				Distance,
				pNew
			    );
			}

//	    Check and insert previous in new neighbor list

			if (
			    (nSlotsNewSoFar < nSlotsNew) ||
			    (Distance < pNew->Neighbor[nSlotsNewSoFar - 1].HowFar)
			) {
#ifdef	PERFORMANCE_MEASURE
			    ActualInserts++;
#endif
			    nSlotsNewSoFar += InsertNeighbor(
				pNew->Neighbor,
				nSlotsNewSoFar,
				nSlotsNew,
				Distance,
				pPrev
			    );
			}
		    }
		}
		iValidNew++;		/* note another valid entry */
	    }
	}
    }

    return nSlotsNewSoFar;
}



/**********************************************************************
 *
 * InsertNeighbor()
 *
 * Looks up dNew in the table of size n at pN.  If dNew is less than any
 * entry in the table, a hole is dug and dNew and iNew are inserted.
 *
 * Returns 1 iff the table grew.
 *
 **********************************************************************/

int InsertNeighbor (
    NEIGHBOR far *pNeighbors,	// 0th entry in neighbor table
    int nNow,			// number of entries now in table
    int nMax,			// maximum number of entries in table
    unsigned short dNew,	// distance of entry to insert
    EXTCODE BASED_SELF *pNew	// EXTCODE for insert
) {
  int iBottom, iMid, iAbove;	// bisection indices

// We might insert at iBottom but not before.  We will insert below iAbove.

// We find the insertion spot via bisection.

  for (
    iBottom = 0,
      iAbove = nNow;
    iAbove > iBottom;
  ) {
    iMid = (iAbove + iBottom - 1) >> 1;

    if (dNew >= pNeighbors[iMid].HowFar) {/* equal inserts after */

      iBottom = iMid + 1;

    } else {

      iAbove = iMid;
    }
  }

// We insert in front of pNeighbors[iBottom].

  if (iBottom < nMax) { /* if not trying to insert beyond end of table */

    iAbove = (nNow < nMax) ? nNow : (nMax - 1);

//      Copy pNeighbors[iBottom..iAbove-1] to
//      pNeighbors[iBottom+1..iAbove] then put the new guy in
//      pNeighbors[iBottom].

    for (
      pNeighbors += iAbove,
	iMid = iAbove - iBottom;
      iMid--;
      pNeighbors--,
	pNeighbors[1] = pNeighbors[0]
    );

    pNeighbors[0].HowFar = dNew;
    pNeighbors[0].pWho = pNew;
//printf("adding Neighbor %08x to %08x\n",pNew,&pNeighbors->pWho);
    return(nMax > nNow);

  }
  return(0);

}
