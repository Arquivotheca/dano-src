/* ex:set sw=2 ts=8 wm=0:
 * $Header: u:/rcs/cv/rcs/matchand.c 2.13 1995/09/30 16:14:46 bog Exp $

 * (C) Copyright 1992-1994 SuperMac Technology, Inc.
 * All rights reserved

 * This source code and any compilation or derivative thereof is the
 * sole property of SuperMac Technology, Inc. and is provided pursuant
 * to a Software License Agreement.  This code is the proprietary
 * information of SuperMac Technology and is confidential in nature.
 * Its use and dissemination by any party other than SuperMac Technology
 * is strictly limited by the confidential information provisions of the
 * Agreement referenced above.

 * $Log: matchand.c $
 * Revision 2.13  1995/09/30 16:14:46  bog
 * Separate CClass and VClass parameters for MatchAndRefine for
 * clarity.
 * 
 * Revision 2.12  1995/05/09  09:23:36  bog
 * Move WINVER back into the makefile.  Sigh.
 * 
 * Revision 2.11  1995/01/21  17:40:24  bog
 * Fix title.
 * 
 * Revision 2.10  1994/10/23  17:22:58  bog
 * Try to allow big frames.
 * 
 * Revision 2.9  1994/10/20  17:45:08  bog
 * Modifications to support Sunnyvale Reference version.
 * 
 * Revision 2.8  1994/07/18  13:30:54  bog
 * Move WINVER definition from makefile to each .c file.
 * 
 * Revision 2.7  1994/05/01  23:39:19  unknown
 * Move nCodes based max neighbors calculation down to MatchAndRefine().
 * 
 * Revision 2.6  1994/05/01  16:07:21  unknown
 * Build the Neighbors list only in the first two rounds in ConvergeCodeBook.
 * In subsequent rounds, just reuse the last one built.
 * 
 * Revision 2.5  1994/04/13  11:26:41  timr
 * Couldn't get _lopen to work, but OpenFile was OK.
 * 
 * Revision 2.4  1994/04/12  15:26:42  unknown
 * When DBG_WRITE_MATCHES is defined, write all Match results to a log file.
 * 
 * Revision 2.3  1993/10/11  13:07:17  geoffs
 * Don't ever return more than 100% for % done during VFW_STATUS
 * 
 * Revision 2.2  1993/09/24  14:07:59  geoffs
 * Have an approximation of progress for compression in call to Status callback
 * 
 * Revision 2.1  1993/07/06  16:55:56  geoffs
 * 1st pass WIN32'izing
 * 
 * Revision 2.0  93/06/01  14:16:38  bog
 * Version 1.0 Release 1.3.0.1 of 1 June 1993.
 * 
 * Revision 1.15  93/04/21  15:57:40  bog
 * Fix up copyright and disclaimer.
 * 
 * Revision 1.14  93/02/05  12:44:14  bog
 * Put check of Match() on a separate DEBUG switch.
 * 
 * Revision 1.13  93/02/03  15:35:59  bog
 * Fix DEBUG check of Match() return.
 * 
 * Revision 1.12  93/02/03  13:56:26  bog
 * Add DEBUG code to check validity of result of Match().
 * 
 * Revision 1.11  93/02/03  09:51:12  bog
 * Make NEIGHBORS a SYSTEM.INI parameter.
 * 
 * Revision 1.10  93/02/01  12:19:47  bog
 * Check matched entry a bit more carefully if DEBUG.
 * 
 * Revision 1.9  93/01/31  13:11:18  bog
 * Add logic to refine only vectors and codebook entries that are unlocked on
 * interframes.  Peter calls it "MSEFractional".
 * 
 * Revision 1.8  93/01/28  16:25:42  bog
 * DEBUG check that entry returned by Match() has fINVALID reset.
 * 
 * Revision 1.7  93/01/25  21:42:31  geoffs
 * Remove debug stuff.
 * 
 * Revision 1.6  93/01/25  14:27:06  geoffs
 * Match() needs valid Neighbor[] count.
 * 
 * Revision 1.5  93/01/21  12:50:52  bog
 * Make vector pointers HUGE for simple > 65k lists.
 * 
 * Revision 1.4  93/01/20  17:22:11  bog
 * Compression works!
 *
 * Revision 1.3  93/01/19  14:58:39  bog
 * Adapt to inter codebooks.
 *
 * Revision 1.2  93/01/18  20:17:05  bog
 * Split the bottom level stuff out so it can be used by inter codebooks.
 *
 * Revision 1.1  93/01/11  09:46:11  bog
 * Initial revision
 *
 *
 * Cinepak  
 *
 * Match each vector against the codebook and refine codebook entries
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

#if	defined(DBG_WRITE_MATCHES)

#  ifdef	WIN32

unsigned long nWrit;

typedef HANDLE	HLOGFILE;

#define	WriteLogFile(a,b,c)	WriteFile(a,b,c,&nWrit,NULL)
#define	CloseLogFile		CloseHandle

#  else

typedef HFILE	HLOGFILE;

#define	WriteLogFile		_lwrite
#define CloseLogFile		_lclose

#  endif

#endif


extern NEIGHBORSRANGE NeighborsRange;


/**********************************************************************
 *
 * MatchAndRefine()
 *
 * Refine a codebook
 *
 **********************************************************************/

unsigned long MatchAndRefine(
  CCONTEXT *pC,			// current compression context
  VECTORBOOK *pVB,		// current codebook
  int RecomputeNeighbors,	// 2:  recalc max neighbors
  				// 1:  recompute neighbors list
				// 0:  use what's already computed
  unsigned char VClass,		// vector class to match; 0xff means any
  unsigned char CClass		// any bit on here must be on in EC Flags
) {
  auto EXTCODE far *pEC;
  auto VECTOR huge *pV;
  auto int nEC;
  auto long nV;
  auto int nCodes;		// number of codes that are targets of Match()
  auto unsigned long Error;

#ifdef	DBG_WRITE_MATCHES
  HLOGFILE	hMatchLog;

#ifdef	WIN32
  hMatchLog = CreateFile (
      TEXT("c:\\match.log"),
      GENERIC_WRITE,
      0,
      NULL,
      OPEN_ALWAYS,
      FILE_ATTRIBUTE_NORMAL,
      0
  );

  SetFilePointer (hMatchLog, 0, NULL, FILE_END);	// seek to end
#else
  OFSTRUCT ofs;

  if ( 
    (
      (
	hMatchLog = OpenFile (TEXT("c:\\match.log"), &ofs, OF_WRITE)
      ) == HFILE_ERROR
    )
    &&
    (
      (
	hMatchLog = OpenFile (TEXT("c:\\match.log"), &ofs, OF_WRITE | OF_CREATE)
      ) == HFILE_ERROR
    )
  ) 
    _asm int 3;

  _llseek (hMatchLog, 0, SEEK_END);
#endif

  {
    unsigned long stats [3];

    stats [0] = 0x0dd0adf0;			// include a marker
    stats [1] = sizeof(VECTOR);
    stats [2] = sizeof(EXTCODE) - NEIGHBORS * sizeof(NEIGHBOR);
    WriteLogFile (hMatchLog, stats, sizeof (stats));
  }

#endif

  /*
    Clear accumulation area.
   */
  for (pEC = pVB->pECodes, nEC = pVB->nCodes, nCodes = 0; nEC--; pEC++) {
    pEC->nMatches = 0;
    pEC->Error = 0;
    pEC->lyuv.y[0] = 0;
    pEC->lyuv.y[1] = 0;
    pEC->lyuv.y[2] = 0;
    pEC->lyuv.y[3] = 0;
    pEC->lyuv.u = 0;
    pEC->lyuv.v = 0;
    /*
      Don't count as targets for Match() codes that are
	invalid, or
	locked, if CClass came in non-zero
     */
    if (!((pEC->Flags ^ CClass) & (fINVALID | CClass))) {
      nCodes++;
    }
  }
    
  if (RecomputeNeighbors) {	// if allowed to rebuild the Neighbors list

    if (
      (RecomputeNeighbors == 2) &&// if recalculate max neighbors
      (pC->NeighborsMethod == 1)// based on nCodes
    ) {
      for (
	nEC = sizeof(NeighborsRange)/sizeof(NeighborsRange[0]);
	nEC-- && (nCodes < NeighborsRange[nEC].CRangeBottom);
      );

      pC->Neighbors = pC->NeighborsRange[nEC];
    }

    pC->CurrNeighbors =

      pC->Neighbors ?

      MakeNeighbors(		// build neighbors lists
	pVB->pECodes,
	pVB->nCodes,
	pC->Neighbors,		// no more than this many Neighbors
	CClass			// ignore entries with this off
      ) :

      0;			// no neighbor list this time
  }

  /*
    For each vector we find the best matching code, using our nearest
    neighbors table.  We remember the error and the number of matching
    vectors in the codebook entry and accumulate the vector's components
    in the codebook entry.

    Then for each codebook entry we adjust the components to be the
    average of all the matching vectors.

    We return the total error for all vectors for all matches.

    !!!!
      Note that the error returned is that before codebook entry
      adjustment.
    !!!!
   */

  /*
    Find match and accumulate.
   */
  for (nV = pVB->nVectors, pV = pVB->pVectors; nV--; pV++) {

    if (
      (VClass == 0xff) ||	// if match any vector class
      (VClass == pV->Class)	// or this vector is in the class
    ) {

#ifdef	CHECK_MATCH
      auto unsigned char OldCode;

      OldCode = pV->Code;
#endif

      /*
	Find nearest codebook entry.
       */
      Error = Match(pV, pVB->pECodes, pVB->nCodes, pC->CurrNeighbors, CClass);

      pEC = &pVB->pECodes[pV->Code];

#ifdef	DBG_WRITE_MATCHES

// Write the matched vector and the matching code (minus neighbor table).

      WriteLogFile (
        hMatchLog, 
	pV, 
	sizeof(VECTOR)
      );

      WriteLogFile (
	hMatchLog, 
	pEC, 
	sizeof(EXTCODE) - NEIGHBORS * sizeof(NEIGHBOR)
      );

#endif

#ifdef	CHECK_MATCH
      {
	auto int i;
	auto unsigned long BestErr, TryErr;
	auto unsigned int Best;

	if ((pEC->Flags ^ CClass) & (fINVALID | CClass)) {
	  DebugBreak();
	}
	for (i = 0, Best = 0xffff, BestErr = 0xffffffffL; i < 256; i++) {
	  if (
	    !((pEC->Flags ^ CClass) & (fINVALID | CClass)) &&
	    ((TryErr = LongDistance(pVB->pECodes[i].yuv, pV->yuv)) < BestErr)
	  ) {
	    Best = i;
	    BestErr = TryErr;
	  }
	}
	if (Best != pV->Code) {
	  DebugBreak();
	}
      }
#endif

      pEC->nMatches++;
      pEC->Error += Error;
      pEC->lyuv.y[0] += pV->yuv.y[0];
      pEC->lyuv.y[1] += pV->yuv.y[1];
      pEC->lyuv.y[2] += pV->yuv.y[2];
      pEC->lyuv.y[3] += pV->yuv.y[3];
      pEC->lyuv.u += pV->yuv.u;
      pEC->lyuv.v += pV->yuv.v;
    }
  }

  /*
    Move all unlocked codes to the centroid of their matching vectors.
    Accumulate total error.
   */
  for (pEC = pVB->pECodes, nEC = pVB->nCodes, Error = 0; nEC--; pEC++) {

    if (
      !(pEC->Flags & fINVALID) &&
      pEC->nMatches
    ) {

      Error += pEC->Error;

      if (pEC->Flags & fUNLOCKED) {

	pEC->lyuv.y[0] =
	  (pEC->lyuv.y[0] + (pEC->nMatches >> 1)) / pEC->nMatches;
	pEC->lyuv.y[1] =
	  (pEC->lyuv.y[1] + (pEC->nMatches >> 1)) / pEC->nMatches;
	pEC->lyuv.y[2] =
	  (pEC->lyuv.y[2] + (pEC->nMatches >> 1)) / pEC->nMatches;
	pEC->lyuv.y[3] =
	  (pEC->lyuv.y[3] + (pEC->nMatches >> 1)) / pEC->nMatches;
	pEC->lyuv.u =
	  (pEC->lyuv.u + (pEC->nMatches >> 1)) / pEC->nMatches;
	pEC->lyuv.v =
	  (pEC->lyuv.v + (pEC->nMatches >> 1)) / pEC->nMatches;

	if (
	  (pEC->yuv.y[0] != (unsigned char) pEC->lyuv.y[0]) ||
	  (pEC->yuv.y[1] != (unsigned char) pEC->lyuv.y[1]) ||
	  (pEC->yuv.y[2] != (unsigned char) pEC->lyuv.y[2]) ||
	  (pEC->yuv.y[3] != (unsigned char) pEC->lyuv.y[3]) ||
	  (pEC->yuv.u != (unsigned char) pEC->lyuv.u) ||
	  (pEC->yuv.v != (unsigned char) pEC->lyuv.v)
	) {

	  pEC->yuv.y[0] = (unsigned char) pEC->lyuv.y[0];
	  pEC->yuv.y[1] = (unsigned char) pEC->lyuv.y[1];
	  pEC->yuv.y[2] = (unsigned char) pEC->lyuv.y[2];
	  pEC->yuv.y[3] = (unsigned char) pEC->lyuv.y[3];
	  pEC->yuv.u = (unsigned char) pEC->lyuv.u;
	  pEC->yuv.v = (unsigned char) pEC->lyuv.v;

	  pEC->Flags |= fUPDATED;// note this code updated
	}
      }
    }
  }

  //
  // update status for VFW
  //
  pC->icssp.nMatchAndRefineCalls++;
  VFW_STATUS(
    pC,
    min(
	100,
	(
	 (pC->icssp.nMatchAndRefineCalls * 100) /
	 pC->icssp.nMaxMatchAndRefineCalls
	)
    )
  );

#ifdef	DBG_WRITE_MATCHES
  CloseLogFile (hMatchLog);
#endif

  return(Error);		// return total error for all vectors
}
