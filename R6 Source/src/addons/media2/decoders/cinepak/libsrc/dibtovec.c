/* ex:set sw=2 ts=8 wm=0:
 * $Header: u:/rcs/cv/rcs/dibtovec.c 2.8 1995/05/09 09:23:15 bog Exp $

 * (C) Copyright 1992-1994 SuperMac Technology, Inc.
 * All rights reserved

 * This source code and any compilation or derivative thereof is the
 * sole property of SuperMac Technology, Inc. and is provided pursuant
 * to a Software License Agreement.  This code is the proprietary
 * information of SuperMac Technology and is confidential in nature.
 * Its use and dissemination by any party other than SuperMac Technology
 * is strictly limited by the confidential information provisions of the
 * Agreement referenced above.

 * $Log: dibtovec.c $
 * Revision 2.8  1995/05/09 09:23:15  bog
 * Move WINVER back into the makefile.  Sigh.
 * Revision 2.7  1995/03/14  08:24:10  bog
 * 1.  No one was looking at the remembered previous frame's smooth
 *     vectors, so there is no point in remembering them.
 * 2.  We update the previous frame's remembered detail vectors to be as
 *     refined (quantized) rather than as incoming, improving the decision
 *     about what to update in the next frame.
 * 
 * Revision 2.6  1994/10/23  17:22:32  bog
 * Try to allow big frames.
 * 
 * Revision 2.5  1994/10/20  17:38:12  bog
 * Modifications to support Sunnyvale Reference version.
 * 
 * Revision 2.4  1994/07/18  13:30:41  bog
 * Move WINVER definition from makefile to each .c file.
 * 
 * Revision 2.3  1994/05/09  17:26:06  bog
 * Black & White compression works.
 * 
 * Revision 2.2  1994/04/12  10:40:29  unknown
 * Win32-ize the DebugWriteYUV code.
 * 
 * Revision 2.1  1993/07/06  16:55:35  geoffs
 * 1st pass WIN32'izing
 * 
 * Revision 2.0  93/06/01  14:13:51  bog
 * Version 1.0 Release 1.3.0.1 of 1 June 1993.
 * 
 * Revision 1.22  93/04/21  15:47:25  bog
 * Fix up copyright and disclaimer.
 * 
 * Revision 1.21  93/03/10  15:17:29  bog
 * Enable insertion of key frames.
 * 
 * Revision 1.20  93/02/18  21:12:33  bog
 * Enable RecreateSmoothFromDetail.
 * 
 * Revision 1.19  93/02/18  14:36:00  bog
 * All rate control now in RateControl.
 * 
 * Revision 1.18  93/02/16  09:48:19  bog
 * Recreate smooth from detail on interframes.
 * 
 * Revision 1.17  93/02/16  08:24:40  bog
 * Ensure frames no larger than requested.
 * 
 * Revision 1.16  93/02/12  13:05:22  timr
 * Include ifdef-ed out code to write the currect tile in YYYYUV format to
 * a disk file.
 * 
 * Revision 1.15  93/01/27  21:44:25  geoffs
 * Forgot to cast to short before store in previous rev
 * 
 * Revision 1.14  93/01/27  21:30:18  geoffs
 * Fix up short/long overflow problem in codebook size calc
 * 
 * Revision 1.13  93/01/25  21:43:07  geoffs
 * Remove debug.
 * 
 * Revision 1.12  93/01/21  11:04:36  timr
 * Qualify "Type" with another character so it passes H2INC.
 * 
 * Revision 1.11  93/01/20  13:34:08  bog
 * Get inter codebooks and frames working.
 * 
 * Revision 1.10  93/01/20  08:15:25  geoffs
 * Fix a type in codebook size determination
 *
 * Revision 1.9  93/01/19  14:59:45  bog
 * HintedSize now unsigned long.
 *
 * Revision 1.8  93/01/19  08:48:48  bog
 * Overflow in CodeBookSize computations.
 *
 * Revision 1.7  93/01/18  21:50:52  bog
 * It must be late at night.
 *
 * Revision 1.6  93/01/18  21:49:32  bog
 * Oops.
 *
 * Revision 1.5  93/01/18  21:47:25  bog
 * Handle CodeSize and CodeBookSize.
 *
 * Revision 1.4  93/01/16  16:07:31  geoffs
 * Added code to checksum detail,smooth vectors and display
 *
 * Revision 1.3  93/01/12  17:15:17  geoffs
 * First halting steps towards running.
 *
 * Revision 1.2  93/01/11  09:47:33  bog
 * First halting steps towards integration.
 *
 * Revision 1.1  93/01/06  10:12:37  bog
 * Initial revision
 *
 *
 * CompactVideo Codec Convert DIB to Vector List
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

// #define	DEBUG_WRITEYUV	1
#ifdef	DEBUG_WRITEYUV

BYTE DebugWriteYUV = DEBUG_WRITEYUV + 0;

#endif

/**********************************************************************
 *
 * DIBToVectors()
 *
 * Convert an incoming DIB to DetailVectors, SmoothVectors, DetailList
 * and DiffList.
 *
 * Returns nz if error
 *
 **********************************************************************/

void DIBToVectors(
	CCONTEXT *pC,			// compression context
	TILECONTEXT *pT,		// tile context
	unsigned long oBits,		// 32 bit offset of base of input tile
#if	!defined(WIN32)
	unsigned short sBits,		// selector to input tile
#endif
	unsigned long HintedSize,	// if caller gave hint at desired size
	unsigned short SQuality,	// 0..1023 spatial quality slider
	unsigned short TQuality)	// 0..1023 temporal quality slider
{
	/*
	  Transform tile of incoming DIB to internal YUV form.
	
	  Let w be the width of the tile rounded up to the next 0 mod 4
	  boundary, and h be the height of the tile similarly rounded.
	
	  Then the internal YUV form consists of:
	
	  1.  Y, a full size w*h bitmap of
	 Y[i] = ((4*G[i] + 2*R[i] + B[i]) + 3.5) / 7, range 0..255
	
	  2.  U, a 1/4 size (w/2)*(h/2) bitmap of U[i], range 0..255, filtered
	 with 431,1331,...,1331,134 in x and y from
	 u[i] = 128 + ((B[i] - Y[i]) / 2)
	
	  3.  V, a 1/4 size (w/2)*(h/2) bitmap of V[i], range 0..255, filtered
	 with 431,1331,...,1331,134 in x and y from
	 v[i] = 128 + ((R[i] - Y[i]) / 2)
	
	  4.  Y2, a 1/4 size (w/2)*(h/2) bitmap of Y2[i], range 0..255,
	 filtered with 431,1331,...,1331,134 from Y[i].
	
	  5.  U2, a 1/16 size (w/4)*(h/4) bitmap of U2[i], range 0..255,
	 filtered with 431,1331,...,1331,134 from U[i].
	
	  6.  V2, a 1/16 size (w/4)*(h/4) bitmap of V2[i], range 0..255,
	 filtered with 431,1331,...,1331,134 from V[i].
	
	  The 431,1331,...,1331,431 filtering means that given a scanline of
	  input i0,i1,i2,i3,i4,i5,i6,i7 we produce an output of o0,o1,o2,o3
	  where
	    o0 = 1*i0 + 3*i0 + 3*i1 + 1*i2 = 4*i0 + 3*i1 + 1*i2
	    o1 = 1*i1 + 3*i2 + 3*i3 + 1*i4
	    o2 = 1*i3 + 3*i4 + 3*i5 + 1*i6
	    o3 = 1*i5 + 3*i6 + 3*i7 + 1*i7 = 1*i5 + 3*i6 + 4*i7
	  with similar filtering done in y.
	
	  We remember #1, #2 and #3 above as an array of VECTOR elements,
	  called pC->VBook[Detail].pVectors, and #4, #5 and #6 as a similar
	  array called pC->VBook[Smooth].pVectors.
	
	  Each 2x2 patch of incoming DIB thus comprises a 6 element detail
	  vector, with 4 luminance values and 2 chroma values.  Each 4x4 patch
	  of incoming DIB comprises a 6 element smooth vector, also with 4
	  luma and 2 chroma.  Whether a given 4x4 patch is treated as 4 2x2
	  detail vectors or 1 4x4 smooth vector is decided later.
	
	  One VECTOR area is allocated per tile (for the prev tile) plus one
	  for the current tile at CompressBegin time.  They are shuffled
	  around as new tiles come in, so each tile has a prev when needed.
	 */
	DIBToYUV(			// no alloc so can't have errors
		pC,				// compression context
		pT,				// tile context
		oBits			// 32 bit offset of base of input tile
#if	!defined(WIN32)
		,sBits			// selector to input tile
#endif
	);

#ifdef	DEBUG_WRITEYUV
// Debug output this frame.

    if (DebugWriteYUV)
    {
	unsigned long T;

#ifdef	WIN32
	unsigned long writ;
	HANDLE f;

	f = CreateFile (
	    TEXT("c:\\tile.dti"), 
	    GENERIC_WRITE,
	    0,
	    NULL,
	    CREATE_ALWAYS,
	    FILE_ATTRIBUTE_NORMAL,
	    0
	);

//	_llseek (f, 0, SEEK_END);

	T = 0xbebadefa;		// Magic marker constant
	WriteFile (f, (PCHAR) &T, sizeof (T), &writ, NULL);

	// TileNum, Height, nPatches, DIBHeight
	WriteFile (f, (PCHAR) &pT->TileNum, 4 * sizeof (short), &writ, NULL);

	WriteFile (
	    f,
	    pC->VBook [Detail].pVectors,
	    pT->nPatches * 4 * sizeof (VECTOR),
	    &writ,
	    NULL
	);

	T = 0x0dd0;		// Another magic marker
	WriteFile (f, &T, 2, &writ, NULL);

	WriteFile (
	    f, 
	    pC->VBook [Smooth].pVectors,
	    pT->nPatches * sizeof (VECTOR),
	    &writ,
	    NULL
	);

	CloseHandle (f);
#else
	HFILE f;
	VECTOR _huge * V;

    // Number of VECTORS which fits in the largest power of 2 less than 65534.

#define	FSTEP	4096

	f = _lopen (TEXT("c:\\tile.dti"), OF_WRITE);
	_llseek (f, 0, SEEK_END);

	T = 0xbebadefa;		// Magic marker constant
	_lwrite (f, &T, sizeof (T));

	// TileNum, Height, nPatches, DIBHeight
	_lwrite (f, &pT->TileNum, 4 * sizeof (short));

	for (
	    T = pT->nPatches * 4, V = pC->VBook [Detail].pVectors;
	    T > FSTEP;
	    T -= FSTEP, V += FSTEP)
	{
	    _lwrite (f, V, FSTEP * sizeof (VECTOR));
	}

	if (T)
	    _lwrite (f, V, (short) T * sizeof (VECTOR));

	T = 0x0dd0;		// Another magic marker
	_lwrite (f, &T, 2);
	_lwrite (f, pC->VBook [Smooth].pVectors,
	    pT->nPatches * sizeof (VECTOR));

	_lclose (f);
#endif
	DebugWriteYUV = 0;	// only write one frame
    }
#endif

  /*
    If this is a black & white movie, clear the U and V components of
    the tile.
   */
#if !defined(NOBLACKWHITE)
	if (pC->Flags.BlackAndWhite) {
		auto VECTOR huge *pV;
		auto long i;
		
		for (pV=pC->VBook[Detail].pVectors,i=pT->nPatches<<2;i--;pV++)
		{
			pV->yuv.u = pV->yuv.v = 128;// no chroma
		}
		for (pV=pC->VBook[Smooth].pVectors,i=pT->nPatches;i--;pV++)
		{
			pV->yuv.u = pV->yuv.v = 128;// no chroma
		}
	}
#endif
	/*
	  If the tile is to be differenced from a previous tile, we compute
	  the error between the previous tile and this tile to determine which
	  4x4 patches to update.
	 */
	if (pC->fType == kInterFrameType)
	{
		/*
		  The error for each 4x4 patch is computed by taking the square of
		  the difference between an element of this tile and the
		  corresponding element of the
		  (as coded?  original?  which is better?)
		  previous tile and summing weighted:
		
		  ---------------------------
		Y:  | 3/16 | 2/16 | 2/16 | 3/16 |
		 |------+------+------+------|
		 | 2/16 | 1/16 | 1/16 | 2/16 |
		 |------+------+------+------|
		 | 2/16 | 1/16 | 1/16 | 2/16 |
		 |------+------+------+------|
		 | 3/16 | 2/16 | 2/16 | 3/16 |
		  ---------------------------
		
		  ---------------------------
		U:  |             |             |
		 |     1/4     |     1/4     |
		 |             |             |
		 |-------------+-------------|
		 |             |             |
		 |     1/4     |     1/4     |
		 |             |             |
		  ---------------------------
		
		  ---------------------------
		V:  |             |             |
		 |     1/4     |     1/4     |
		 |             |             |
		 |-------------+-------------|
		 |             |             |
		 |     1/4     |     1/4     |
		 |             |             |
		  ---------------------------
		
		  The errors, pinned at 7fff, are saved in the array DiffList, one
		  element per 4x4 patch.  The sum of the errors is remembered in
		  TileDifference.
		
		  One DiffList is allocated at CompressBegin time.
		 */
		pC->Difference += DifferenceList(pC, pT);
	}
	/*
	  To determine whether a given 4x4 patch is to be detail or smooth, we
	  compute the error we would get representing the patch as smooth
	  instead of detail.
	
	  The error for each 4x4 patch comes from the maximum of
	  the errors for the 4 2x2 subpatches.  The error for each 2x2
	  subpatch is computed by taking the square of the difference between
	  an element of the detail patch and the corresponding smooth patch
	  and summing weighted:
	
	  -------
	    Y:  | 1 | 1 |
	 |---+---|
	 | 1 | 1 |
	  -------
	
	  -------
	    U:  |       |
	 |   4   |
	 |       |
	  -------
	
	  -------
	    V:  |       |
	 |   4   |
	 |       |
	  -------
	
	  The errors, pinned at 7fff, are saved in the array DetailList, one
	  element per 4x4 patch.
	
	  One DetailList is allocated at CompressBegin time.
	 */
	DetailList(pC, pT);
	
	/*
	  We now use DetailList and DiffList (if any) to accomplish the
	  desired level of spatial and temporal quality or the desired frame
	  size.
	
	  The result is returned as booleans in DetailList and DiffList.  An
	  element in DetailList is TRUE iff the corresponding 4x4 block is a
	  detail block.  An element in DiffList is TRUE iff the corresponding
	  4x4 block is new in the current tile.
	
	  !!!
	    noticing that a tile is so different from the prev that it should
	    be key should be done in RateControl.
	  !!!
	 */
	RateControl(pC, pT, HintedSize, SQuality, TQuality);
	
	/*
	  If we are working on an interframe, we update unchanged detail
	  vectors from the previous frame and then rederive the smooth vectors
	  from the updated detail.
	
	  This makes new smooth patches blend better with what is actually on
	  the playback screen.  It also lets the compressor notice when a
	  patch on the playback screen is made obsolete by accumulating a
	  sequence of small changes.
	 */
	if (pC->fType == kInterFrameType)
	{
		auto VECTOR huge *pV;
		auto VECTOR huge *pPV;
		auto short huge *pSw;
		auto long l;
		auto int NeedRecreate;
		
		for (			// for each detail vector
		pV = pC->VBook[Detail].pVectors,// this frame
		pPV = pT->pDetail,	// previous frame
		pSw = pC->pDiffList,	// one boolean per patch
		l = pT->nPatches,	// number of patches in this tile
		NeedRecreate = 0;	// flag whether recreate needed
		l--;
		pV += 4,
		pPV += 4,
		pSw++
		)
		{
			if (!*pSw)
			{		// if not new patch
				pV[0] = pPV[0];		// update from previous frame
				pV[1] = pPV[1];
				pV[2] = pPV[2];
				pV[3] = pPV[3];
				NeedRecreate = 1;
			}
		}
		if (NeedRecreate)
		{
			RecreateSmoothFromDetail(	// no alloc so can't have errors
				pC,			// compression context
				pT			// tile context
			);
		}
	}
	/*
	  We now compress the VECTOR into pC->VBook[Detail].pVectors and
	  pC->VBook[Smooth].pVectors by tossing vectors that haven't changed
	  from the previous frame.
	
	  As a side effect of this compression, we leave a copy of the
	  uncompressed detail vectors in the tile context in case next time we
	  have an inter frame.
	 */
	GenerateVectors(pC, pT);	// no alloc so can't have errors
	
	return;
}
