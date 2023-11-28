/* ex:set sw=2 ts=8 wm=0:
 * $Header: u:/rcs/cv/rcs/intercod.c 2.14 1995/10/16 16:54:14 bog Exp $

 * (C) Copyright 1992-1994 SuperMac Technology, Inc.
 * All rights reserved

 * This source code and any compilation or derivative thereof is the
 * sole property of SuperMac Technology, Inc. and is provided pursuant
 * to a Software License Agreement.  This code is the proprietary
 * information of SuperMac Technology and is confidential in nature.
 * Its use and dissemination by any party other than SuperMac Technology
 * is strictly limited by the confidential information provisions of the
 * Agreement referenced above.

 * $Log: intercod.c $
 * Revision 2.14  1995/10/16 16:54:14  bog
 * We put too many codebook entries into the holes list.  Try with just
 * 0 or 1 matches.  It seems to fix the original problem without introducing
 * any new ones.
 * 
 * Revision 2.13  1995/10/02 12:22:10  bog
 * Oops.  Forgot to change one of the calls to ConvergeCodeBook back.
 * 
 * Revision 2.12  1995/10/02 11:53:44  bog
 * Sometimes we end up with lots of codebook entries that have few
 * vectors matching them and thus small error.  They don't make very
 * good use of the available bandwidth.  To fix it, when we have the
 * bandwidth available we invalidate entries that have few vectors
 * matching them so that we have more split targets available.
 * 
 * Revision 2.11  1995/05/09 09:23:33  bog
 * Move WINVER back into the makefile.  Sigh.
 *
 * Revision 2.10  1994/10/23  17:22:53  bog
 * Try to allow big frames.
 * 
 * Revision 2.9  1994/10/20  17:40:40  bog
 * Modifications to support Sunnyvale Reference version.
 * 
 * Revision 2.8  1994/07/18  13:30:48  bog
 * Move WINVER definition from makefile to each .c file.
 * 
 * Revision 2.7  1994/05/10  19:51:19  timr
 * Eliminate compiler warnings.
 * 
 * Revision 2.6  1994/05/01  15:39:14  unknown
 * Move nCodes based max neighbors calculation down to MatchAndRefine().
 * 
 * Revision 2.5  1994/05/01  16:07:12  unknown
 * Build the Neighbors list only in the first two rounds in ConvergeCodeBook.
 * In subsequent rounds, just reuse the last one built.
 * 
 * Revision 2.4  1993/10/21  14:07:33  geoffs
 * Updated LOGREQUESTED stuff to include more data
 * 
 * Revision 2.3  1993/09/24  14:07:56  geoffs
 * Have an approximation of progress for compression in call to Status callback
 * 
 * Revision 2.2  1993/09/23  17:22:07  geoffs
 * Now correctly processing status callback during compression
 * 
 * Revision 2.1  1993/07/06  16:55:49  geoffs
 * 1st pass WIN32'izing
 * 
 * Revision 2.0  93/06/01  14:15:54  bog
 * Version 1.0 Release 1.3.0.1 of 1 June 1993.
 * 
 * Revision 1.11  93/04/21  15:57:12  bog
 * Fix up copyright and disclaimer.
 * 
 * Revision 1.10  93/02/18  21:10:41  bog
 * pC->Flags.Neighbors is now pC->Neighbors.
 * 
 * Revision 1.9  93/02/16  08:24:48  bog
 * Off by one in finding holes.
 * 
 * Revision 1.8  93/02/03  09:50:21  bog
 * Make NEIGHBORS a SYSTEM.INI parameter.
 * 
 * Revision 1.7  93/02/01  12:14:25  bog
 * VECTOR huge * rather than far.
 * 
 * Revision 1.6  93/01/31  13:10:52  bog
 * Add logic to refine only vectors and codebook entries that are unlocked on
 * interframes.  Peter calls it "MSEFractional".
 * 
 * Revision 1.5  93/01/27  23:03:43  bog
 * Get interframes working.
 * 
 * Revision 1.4  93/01/25  22:24:54  geoffs
 * Set fINVALID on entries in hole chain.  Turn ON fUNLOCKED, not off
 * to unlock an entry.
 * 
 * Revision 1.3  93/01/22  12:10:16  bog
 * GenerateVectors now clears Class.
 * 
 * Revision 1.2  93/01/20  13:34:31  bog
 * Get inter codebooks and frames working.
 * 
 * Revision 1.1  93/01/11  09:46:02  bog
 * Initial revision
 *
 *
 * CompactVideo Inter CodeBook Adapt and Refine
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


/**********************************************************************
 *
 * InterCodeBook()
 *
 * Adapt an old codebook and then refine it
 *
 **********************************************************************/

void InterCodeBook(
  CCONTEXT *pC,			// compression context
  TILECONTEXT *pT,		// tile context
  VECTORBOOK *pVB,		// current codebook
  CCODEBOOK *pPB		// prev codebook
) {
  auto int nCodes;		// new codebook size
  auto int i;

  auto int nSplit;		// number of codes we have split to

  auto EXTCODE far *pEC;
  auto CCODE far *pCC;

  auto EXTCODE far *pHole;	// for list of candidates to split to
  auto int nHoles;
  auto EXTCODE far *pOldestHole;

  /*
    Figure out the new codebook size.

    As it came from the previous tile, it had pPB->nCodes in it.

    We can grow it by pVB->nUpdateCodes, but not by more than the number
    of vectors in this tile, and at can't end up any larger than 256.
   */
  nCodes = min(
    256,
    pPB->nCodes + (short) min(pVB->nVectors, (long) pVB->nUpdateCodes)
  );
  if (nCodes <= pVB->nUpdateCodes) {
    /*
      The bandwidth constraint leaves room to completely replace the
      codebook.  Treat it as Key.
     */
    KeyCodeBook(pC, pT, pVB);
    return;
  }

  /*
    Bring the previous codebook back.
   */
  for (
    pCC = pPB->pCodes,
      pEC = pVB->pECodes,
      i = 0;
    i < 256;
    pCC++,
      pEC++,
      i++
  ) {
    if (pCC->Age == 0xffff) {	// if invalid code
      pEC->Flags = fINVALID;
      pEC->nMatches = 0;
    } else {
      pEC->yuv = pCC->yuv;
      pEC->Code = (unsigned char) i;
      pEC->Flags = 0;		// valid, locked, not updated
    }
  }
  /*
    We do the first MatchAndRefine using the previous codebook's nCodes
    because that'as as far as we have to search.
   */
  pVB->nCodes = pPB->nCodes;

  /*
    pVB->pECodes is the extended codebook we have brought back from a
    previous tile, either in the previous frame or from the previous
    tile in this keyframe.

    If in the previous tile a given entry had a color value in it, it is
    marked not fINVALID, not fUNLOCKED, and not fUPDATED.  If there was
    no valid color value in the entry, probably because the entry never
    has had a vector mapping to it, it is marked fINVALID.
   */

  /*
    Match this tile's vectors against valid entries in the codebook.
   */
  MatchAndRefine(pC, pVB, 2, 0xff, 0);// any any match, recompute neighbors

  /*
    Since we are going to fill out the codebook, we must now bound our
    searches by the potential new codebook size, not the old.
   */
  pVB->nCodes = nCodes;

  /*
    Our bandwidth constraints allow us to put out pVB->nUpdateCodes for
    this tile.  We want to find that many entries in the codebook to be
    targets of our SplitCodes process, where the entry with maximum
    error has its vectors split between the original entry and a new
    one.  The idea is that the result will be smaller overall error.

    We build our target entries in a linked list starting at pHole:
      - entries with fINVALID set, then
      - entries with nMatches == 0, then
      - entries with nMatches == 1

    We stop the list when it gets as big as nUpdateCodes.
   */

  /*
    First we gather codes with fINVALID -- those that have never had a
    color value.
   */
  for (				// for each codebook entry
    pEC = pVB->pECodes,
      i = nCodes,
      pHole = 0,
      nHoles = 0;
    i-- && (nHoles < pVB->nUpdateCodes);
    pEC++
  ) {
    if (pEC->Flags & fINVALID) {

      if (!nHoles++) {		// if first link

	pOldestHole = pEC;	// remember it as oldest

      } else {

	pHole->pHole = pEC;	// prev points at this
      }
      pHole = pEC;		// this is now last hole
    }
  }
  {
    auto int nMatches;
    auto int bNeedRematch;	// nz if we must re-Match some vectors

    /*
      We have pVB->nUpdateCodes slots available in the output stream to
      take codebook entries, and we have found nHoles of them.

      If we still have bandwidth remaining, we free up codebook entries
      from the previous codebook that have no matching vectors, then
      those that have exactly one matching vector.

      We set bNeedRematch to nz iff we invalidate some entry that has
      matching vectors.
     */
    for (
      nMatches = 0,		// start with unmatched
	bNeedRematch = 0;
      (nMatches <= 1) && (nHoles < pVB->nUpdateCodes);
      nMatches++		// work up in number of vectors matched
    ) {
      for (			// for each codebook entry
	pEC = pVB->pECodes,
	  pCC = pPB->pCodes,
	  i = nCodes;
	i--;
	pEC++,
	  pCC++
      ) {
	if (
	  !(pEC->Flags & fINVALID) &&// if valid codebook entry
	  (pEC->nMatches == nMatches)// and it has matches of interest
	) {
	  /*
	    If we are putting an entry into our pHoles list that has
	    matching vectors, we will remap those vectors later,
	    using fINVALID to find them.
	   */
	  bNeedRematch |= nMatches;

	  if (!nHoles++) {	// if first link

	    pOldestHole = pEC;// remember it as oldest

	  } else {

	    pHole->pHole = pEC;// prev points at this
	  }
	  pHole = pEC;	// this is now last hole

	  pEC->Flags = fINVALID;// mark it as no longer valid

	  if (nHoles == pVB->nUpdateCodes) {// if we've got what we wanted
	    break;
	  }
	}
      }
    }
    /*
      The set of entries now has two groups:

	1.  Those that have vectors matching them and are not candidates
	    for replacement.  Yet, anyway.  They are not fINVALID, not
	    fUNLOCKED and not fUPDATED.

	2.  Those that are candidates for replacement.  They are tagged
	    fINVALID.

      If there are any entries in the second group (those that are
      fINVALID) that have matching vectors, bInvalidated has been set.

      We remap their matching vectors to entries in the first group.
     */
    if (bNeedRematch) {		// if vectors in any invalidated entries
      auto long nV;
      auto VECTOR huge *pV;

      for (
	nV = pVB->nVectors,
	  pV = pVB->pVectors;
	nV--;
	pV++
      ) {
	if (pVB->pECodes[pV->Code].Flags & fINVALID) {
	  auto unsigned long Error;

	  /*
	    pV is a vector that matches an entry that we have
	    invalidated.  We now remap it to a valid, locked entry.
	   */
	  Error = Match(pV, pVB->pECodes, nCodes, pC->CurrNeighbors, 0);

	  pEC = &pVB->pECodes[pV->Code];

	  pEC->nMatches++;
	  pEC->Error += Error;
	}
      }
    }
  }
  /*
    All vectors now match entries that are not fINVALID, not fUNLOCKED
    and not fUPDATED.

    pHole is now a chain of up to nUpdateCodes fINVALID entries that are
    candidates for being split destinations.
   */

  /*
    Split locked entries with highest error to our hole list.
   */
  nSplit = SplitCodeBook(
    pVB,			// vectors and codebook to fill out
    0,				// first code to pay attention to
    0,				// first vector to pay attention to
    0,				// vector class to search
    &pOldestHole,		// ->-> holes list to fill
    (short) nHoles,		// number of holes to fill
    fINVALID | fUNLOCKED	// don't split invalid or unlocked codes
  );

  /*
    Split locked or unlocked entries with highest error to our hole list.
   */
  nSplit += SplitCodeBook(
    pVB,			// vectors and codebook to fill out
    0,				// first code to pay attention to
    0,				// first vector to pay attention to
    0,				// vector class to search
    &pOldestHole,		// ->-> holes list to fill
    (short) (nHoles - nSplit),	// number of holes to fill
    fINVALID			// don't split invalid codes
  );
  /*
    We have now split codebook entries to nSplit new entries.

    If nUpdateCodes is more than nSplit, we can unlock more codes during
    refinement.
   */
  while (nSplit++ < pVB->nUpdateCodes) {
    auto unsigned long MaxError;
    auto EXTCODE far *pMax;

    /*
      find code with maximum error
     */
    for (
      MaxError = 0,
	pEC = pVB->pECodes,
	i = nCodes;
      i--;
      pEC++
    ) {
      if (!(pEC->Flags & (fINVALID | fUNLOCKED))) {// if valid & locked

	if (pEC->Error >= MaxError) {// remember if largest
	  MaxError = pEC->Error;
	  pMax = pEC;
	}
      }
    }
    if (!MaxError) {		// if no code with error found
      break;
    }
    pMax->Flags |= fUNLOCKED;	// unlock the code with max error
  }
  /*
    Now let the unlocked codebook entries refine.

    pC->Flags.FractionInterCodeBook is a SYSTEM.INI flag.  By default,
    it is set.

    If it is set, we first converge vectors matching unlocked codes
    against those unlocked codes and then, as a last pass, we let all
    vectors match against all the valid codes.

    If it is not set, we simply converge all vectors against all valid
    codes.

    The idea is that we get better performance with little quality loss
    if we do the former.
   */
  if (pC->Flags.FractionInterCodeBook) {// if refine just the subset
    auto long nV;
    auto VECTOR huge *pV;
    auto unsigned long ThisErr;

    for (nV = pVB->nVectors, pV = pVB->pVectors; nV--; pV++) {

      pV->Class = pVB->pECodes[pV->Code].Flags & fUNLOCKED;
    }
    /*
      Converge the vectors that match unlocked entries against those
      unlocked entries.
     */
    if (ThisErr = ConvergeCodeBook(pC, pVB, fUNLOCKED, fUNLOCKED)) {

      // last pass let all vectors move
      /*
	On last pass, match all vectors against all valid codebook
	entries.
       */
      ThisErr = MatchAndRefine(pC, pVB, 2, 0xff, 0);
    }

    pVB->TotalError += ThisErr;

  } else { // refine the whole thing...

    pVB->TotalError += ConvergeCodeBook(pC, pVB, 0xff, 0);
  }
}
