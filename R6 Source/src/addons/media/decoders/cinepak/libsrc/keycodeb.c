/* ex:set sw=2 ts=8 wm=0:
 * $Header: u:/rcs/cv/rcs/keycodeb.c 2.12 1995/10/02 11:54:10 bog Exp $

 * (C) Copyright 1992-1994 SuperMac Technology, Inc.
 * All rights reserved

 * This source code and any compilation or derivative thereof is the
 * sole property of SuperMac Technology, Inc. and is provided pursuant
 * to a Software License Agreement.  This code is the proprietary
 * information of SuperMac Technology and is confidential in nature.
 * Its use and dissemination by any party other than SuperMac Technology
 * is strictly limited by the confidential information provisions of the
 * Agreement referenced above.

 * $Log: keycodeb.c $
 * Revision 2.12  1995/10/02 11:54:10  bog
 * Separate CClass and VClass parameters for MatchAndRefine for
 * clarity.
 * 
 * Revision 2.11  1995/05/16  16:40:41  bog
 * What if nCodesLeft < Reserved?  It blows up.
 * 
 * Revision 2.10  1995/05/09  09:23:34  bog
 * Move WINVER back into the makefile.  Sigh.
 * 
 * Revision 2.9  1995/01/25  14:11:56  bog
 * Fix up Sharpness Classes.
 * 
 * Revision 2.8  1994/10/23  17:22:56  bog
 * Try to allow big frames.
 * 
 * Revision 2.7  1994/10/20  17:41:19  bog
 * Modifications to support Sunnyvale Reference version.
 * 
 * Revision 2.6  1994/07/18  13:30:51  bog
 * Move WINVER definition from makefile to each .c file.
 * 
 * Revision 2.5  1994/05/10  19:51:31  timr
 * Eliminate compiler warnings.
 * 
 * Revision 2.4  1994/05/01  08:07:16  unknown
 * Build the Neighbors list only in the first two rounds in
 * ConvergeCodeBook.  In subsequent rounds, just reuse the last one built.
 * 
 * Revision 2.3  1993/10/21  14:07:36  geoffs
 * Updated LOGREQUESTED stuff to include more data
 * 
 * Revision 2.2  1993/09/23  17:22:10  geoffs
 * Now correctly processing status callback during compression
 * 
 * Revision 2.1  1993/07/06  16:55:52  geoffs
 * 1st pass WIN32'izing
 * 
 * Revision 2.0  93/06/01  14:16:00  bog
 * Version 1.0 Release 1.3.0.1 of 1 June 1993.
 * 
 * Revision 1.18  93/04/21  15:57:17  bog
 * Fix up copyright and disclaimer.
 * 
 * Revision 1.17  93/02/18  21:10:53  bog
 * pC->Flags.Neighbors is now pC->Neighbors.
 * 
 * Revision 1.16  93/02/03  09:50:27  bog
 * Make NEIGHBORS a SYSTEM.INI parameter.
 * 
 * Revision 1.15  93/02/01  12:15:00  bog
 * VECTOR huge * rather than far.
 * 
 * Revision 1.14  93/01/31  13:10:59  bog
 * Add logic to refine only vectors and codebook entries that are unlocked on
 * interframes.  Peter calls it "MSEFractional".
 * 
 * Revision 1.13  93/01/28  16:25:09  bog
 * fUPDATED wasn't being set when remembering mean as first code in class.
 * 
 * Revision 1.12  93/01/27  23:03:47  bog
 * Get interframes working.
 * 
 * Revision 1.11  93/01/27  07:59:56  geoffs
 * Added guard block in low DS to check against writes with nullish ->s
 * 
 * Revision 1.10  93/01/25  21:40:54  geoffs
 * Remove debug.  Account for already-allocated assigned code.
 * 
 * Revision 1.9  93/01/22  08:45:53  bog
 * Oops.  TimR pointed out unsigned char that should have been int.
 * 
 * Revision 1.8  93/01/19  14:58:32  bog
 * Adapt to inter codebooks.
 * 
 * Revision 1.7  93/01/18  21:47:32  bog
 * Handle CodeSize and CodeBookSize.
 * 
 * Revision 1.6  93/01/18  20:16:38  bog
 * Split the bottom level stuff out so it can be used by inter codebooks.
 * 
 * Revision 1.5  93/01/17  15:53:39  bog
 * Compute distance using a squares table rather than multiplying.
 * 
 * Revision 1.4  93/01/16  16:08:28  bog
 * Compressed first frame!
 * 
 * Revision 1.3  93/01/13  16:00:41  bog
 * It compressed a frame!
 * 
 * Revision 1.2  93/01/12  17:14:02  bog
 * First halting steps towards running.
 * 
 * Revision 1.1  93/01/11  09:46:04  bog
 * Initial revision
 * 
 *
 * CompactVideo Key CodeBook Build and Refine
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
 * KeyCodeBook()
 *
 * Build a new codebook and then refine it
 *
 **********************************************************************/

void KeyCodeBook(
  CCONTEXT *pC,			// compression context
  TILECONTEXT *pT,		// tile context
  VECTORBOOK *pVB		// current codebook
) {
  auto int i;
  auto VECTOR huge *pV;
  auto EXTCODE far *pEC;

  CHECKF0AD("KeyCodeBook start");

  /*
    First, build a new codebook.
   */
  {
    auto long nVectorsInClass[5];// for counting class membership
    auto long FirstVInClass[5];	// first member of class
    auto VECTOR huge *pFirstVInClass[5];// -> first member of class

    auto unsigned char  FirstCInClass;// index of first EXTCODE in class
    auto EXTCODE far *pFirstCInClass;// -> first EXTCODE in class

    auto int nCodesLeft;
    auto long nVectorsLeft;
    auto int Reserved;
    auto unsigned char Class;
    auto long l;

    nCodesLeft = pVB->nCodes;	// we split to fill as many as we can

    nVectorsLeft = pVB->nVectors;

    /*
      Initialize the codebook:
	fINVALID set
	pHole linked sequentially
	Code set to the index
     */
    for (pEC = pVB->pECodes, i = 0; i < 256; i++, pEC++) {// for each code
      pEC->Code = (unsigned char) i;
      pEC->Flags = fINVALID;
      pEC->pHole = (EXTCODE __based((__segment) __self) *) &pEC[1];
    }
    CHECKF0AD("KeyCodeBook: After init of codebook");

    /*
      The maximum sharpness of YYYY from the mean Y is where the Ys are
      0, 0, 255, and 255, or 65026 (0xfe02).

      We first divide the vectors into classes based on the sharpness of
      the YYYY from the mean Y:

	class       sharpness        log base 2 of max+1       mask
	  0           0..15                   4               00000f
	  1          16..127                  7               000070
	  2         128..1023                10               000380
	  3        1024..8191                13               001c00
	  4        8192..65535               16               00e000

      We divide the codebook entries among the classes in approximate
      proportion.  We remember the class in pVectors.Class.  We will
      build codebook entries separately for each class so that the
      resulting codebook is relatively balanced.
     */
#define	CLASS0MASK	0x0000000fL
#define	CLASS1MASK	0x00000070L
#define	CLASS2MASK	0x00000380L
#define	CLASS3MASK	0x00001c00L
#define	CLASS4MASK	0x0000e000L

    for (i = 5; i--;) {

      nVectorsInClass[i] = 0;	// no one in classes yet
    }
    for (pV =  pVB->pVectors, l = 0; l < nVectorsLeft; l++, pV++) {
      auto int Mean;
      auto unsigned long Sharpness;

      Mean =
	(pV->yuv.y[0] + pV->yuv.y[1] + pV->yuv.y[2] + pV->yuv.y[3] + 2) >> 2;

      Sharpness = LongSharpness(Mean, pV->yuv);

      pV->Class = Class =
	(Sharpness & CLASS4MASK) ?// if class 4
	(
	  4
	) : (			// differentiate classes 0..3
	  (Sharpness & (CLASS2MASK | CLASS3MASK)) ?
	  (3 - !(Sharpness & CLASS3MASK)) :
	  !!(Sharpness & CLASS1MASK)
	);
      /*
	bump membership; if first member of class, remember where it is
       */
      if (!nVectorsInClass[Class]++) {
	FirstVInClass[Class] = l;
	pFirstVInClass[Class] = pV;
      }
    }
    CHECKF0AD("KeyCodeBook: After class division");

    /*
      Figure out how many codes to reserve as a minimum for each class
     */
    for (Class = 5, Reserved = 0; Class--;) {

      /*
	bump by 1 if one member of class
	bump by two if more than one
       */
      if (nVectorsInClass[Class]) {
	Reserved += (nVectorsInClass[Class] > 1) + 1;
      }
    }
    if (nCodesLeft < Reserved) {
      Reserved = nCodesLeft;	// can't reserve more than we have room for
    }

    /*
      Build (by splitting) codebooks for each class
     */
    for (			// for each class
      Class = 0,
	FirstCInClass = 0,
	pFirstCInClass = pVB->pECodes;
      Class < 5;
      Class++
    ) {

      if (
	nVectorsInClass[Class] &&// if this class has some members
	nCodesLeft		// and there are codes left to allocate
      ) {
	auto int nCodesAssigned;// number of codes assigned to this class

	/*
	  remember mean as first code
	 */
	pFirstCInClass->yuv = pFirstVInClass[Class]->yuv;
	pFirstCInClass->nMatches = nVectorsInClass[Class];
	pFirstCInClass->Error = 0xffffffffL;// maximum error
	pFirstCInClass->Flags = fUNLOCKED | fUPDATED;

	nCodesAssigned = 1;	// assigned one code so far

	/*
	  assign all vectors in this class to first code
	 */
	for (
	  l = nVectorsInClass[Class],
	    pV = pFirstVInClass[Class];
	  l;
	  pV++
	) {
	  if (pV->Class == Class) {// if in this class
	    pV->Code = FirstCInClass;// assign vector to first code
	    l--;
	  }
	}
	/*
	  split codes for classes with more than one member
	 */
	if (
	  (nVectorsInClass[Class] > 1) &&// if class has more than 1 member
	  (nCodesLeft > 1)	// and there are at least two codes left
	) {
	  auto int nClassCodes;	// share of codes for this class
	  auto EXTCODE far *pHole;

	  nClassCodes = (int) (
	    ((long) nVectorsInClass[Class] * (nCodesLeft - Reserved)) /
	    nVectorsLeft
	  ) + 2 - 1;		// one already assigned

	  pHole = pFirstCInClass->pHole;

    	  CHECKF0AD("KeyCodeBook: Before SplitCodeBook");
	  nCodesAssigned = 1 + SplitCodeBook(
	    pVB,		// vectors and codebook to fill out
	    FirstCInClass,	// first code to pay attention to
	    FirstVInClass[Class],// first vector to pay attention to
	    Class,		// vector class to search
	    &pHole,		// -> holes list to fill
	    (short)nClassCodes,	// number of holes to fill
	    fINVALID		// don't split invalid codes
	  );
    	  CHECKF0AD("KeyCodeBook: After SplitCodeBook");

	  Reserved -= 2;	// this class used two reserved codes

	} else {		// exactly one member

	  pFirstCInClass->Error = 0;// no error if only one code

	  Reserved--;		// took one of the reserved codes
	}
	/*
	  account for codes assigned and vectors processed
	 */
	nCodesLeft -= nCodesAssigned;
	nVectorsLeft -= nVectorsInClass[Class];

	FirstCInClass += nCodesAssigned;// bump by codes in this class
	pFirstCInClass += nCodesAssigned;
      }
    }
  }
  /*
    Converge the codebook.
   */
  CHECKF0AD("KeyCodeBook: Before ConvergeCodeBook");

  /*
    Converge all vectors against all valid codebook entries.
   */
  pVB->TotalError += ConvergeCodeBook(pC, pVB, 0xff, 0);

  CHECKF0AD("KeyCodeBook: After ConvergeCodeBook");
}
