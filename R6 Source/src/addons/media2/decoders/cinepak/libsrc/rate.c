/* ex:set sw=2 ts=8 wm=0:
 * $Header: u:/rcs/cv/rcs/rate.c 2.10 1995/05/09 09:23:39 bog Exp $
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
 * $Log: rate.c $
 * Revision 2.10  1995/05/09 09:23:39  bog
 * Move WINVER back into the makefile.  Sigh.
 * Revision 2.9  1995/01/21  17:41:16  bog
 * Cosmetic cleaning; "register long" instead of "register short" for SC
 * and DC in EstimateSize.
 * 
 * Revision 2.8  1994/10/23  17:23:01  bog
 * Try to allow big frames.
 * 
 * Revision 2.7  1994/10/20  17:45:56  bog
 * Modifications to support Sunnyvale Reference version.
 * 
 * Revision 2.6  1994/07/18  13:30:57  bog
 * Move WINVER definition from makefile to each .c file.
 * 
 * Revision 2.5  1994/05/09  17:26:24  bog
 * Black & White compression works.
 * 
 * Revision 2.4  1994/03/03  11:59:25  timr
 * Yank math.h; eliminated the HUGE redefinition warning.
 * 
 * Revision 2.3  1993/09/23  17:22:14  geoffs
 * Now correctly processing status callback during compression
 * 
 * Revision 2.2  1993/09/09  09:17:36  geoffs
 * Fix up divide by 0 when null HintedSize is passed in
 * 
 * Revision 2.1  1993/06/18  15:56:45  bog
 * LimitKeyFrameSize was bumping the threshold up so that DetailCount was wrong.
 * 
 * Revision 2.0  93/06/01  14:17:07  bog
 * Version 1.0 Release 1.3.0.1 of 1 June 1993.
 * 
 * Revision 1.20  93/05/17  16:18:43  bog
 * DetailCount cannot exceed nPatches.
 * 
 * Revision 1.19  93/05/02  15:47:21  bog
 * Bad parenthesis led to bad frames.
 * 
 * Revision 1.18  93/04/21  15:57:56  bog
 * Fix up copyright and disclaimer.
 * 
 * Revision 1.17  93/04/15  13:35:00  geoffs
 * Upwards-adjust DetailCount in LimitKeyFrameSize correctly
 * 
 * Revision 1.16  93/02/18  21:12:09  bog
 * DetailCount might go negative.
 * 
 * Revision 1.15  93/02/18  14:36:20  bog
 * Change to Microsoft's philosophy about quality vs. rate; consolidate
 * rate control into RateControl.
 * 
 * Revision 1.14  93/02/16  08:25:06  bog
 * Ensure frames no larger than requested.
 * 
 * Revision 1.13  93/01/28  07:48:07  geoffs
 * In DetailThreshToSQuality... more loss of precision in short arithmetic
 * problems fixed
 * 
 * Revision 1.12  93/01/27  09:42:02  timr
 * Fixed up short/long problems in EstimateSize
 * 
 * Revision 1.11  93/01/21  11:05:10  timr
 * Qualify "Type" with another character so it passes H2INC.
 * 
 * Revision 1.10  93/01/20  13:34:38  bog
 * Get inter codebooks and frames working.
 * 
 * Revision 1.9  93/01/19  16:16:57  geoffs
 * Intermediates of min,max in LimitKeyFrameSize s/b longs
 *
 * Revision 1.8  93/01/19  13:47:22  geoffs
 * HintedSize s/b unsigned long everywhere
 *
 * Revision 1.7  93/01/19  12:23:06  geoffs
 * Changed intermediates to 32 bit in Quality,isqrt functions
 *
 * Revision 1.6  93/01/19  09:48:59  geoffs
 * Replaced references to _fmemcpy with calls to internal BigCopy instead
 *
 * Revision 1.5  93/01/16  17:00:33  bog
 * Pin HintedSize at 0x7fff because VfW calls us with 0x00ffffffL for the
 * first keyframe in a movie.
 *
 * Revision 1.4  93/01/12  17:19:57  bog
 * Missed one spot in converting YYYYUV to VECTOR.
 *

 *
 * Create a detail and difference map from the quality parameters.
 */

#if	defined(WINCPK)

#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>

#include "compddk.h"

#endif

#include "cv.h"
#include "cvcompre.h"


unsigned short TQualityToDiffThresh(unsigned short TQuality)
{
    return(
      (short) (
	(((unsigned long) (1024 - TQuality) * (1024 - TQuality)) >> 11) + 4
      )
    );
}


unsigned short SQualityToDetailThresh(unsigned short SQuality)
{
    return (((1024 - SQuality) >> 1) + 4);
}


unsigned short Map(
  unsigned short val,		// value to map
  unsigned short ib,		// bottom of incoming range
  unsigned short it,		// top of incoming range
  unsigned short ob,		// bottom of outgoing range
  unsigned short ot		// top of outgoing range
)
/*
  Return val in [ib..it] linearly mapped into [ob..ot].
 */
{
  return(
    (unsigned short) (
      (
	(
	  ((unsigned long) (val - ib)) * ((unsigned long) (ot - ob + 1))
	) / (unsigned long) (it - ib + 1)
      ) + ob
    )
  );
}


unsigned short isqrt(unsigned long val)
{
    // Integer square root via Newton's method.

    unsigned long guess = val >> 1;
    unsigned long err = 2;

    while (guess > err)
    {
        err = val / guess;
	guess = (guess + err) >> 1;
    }

    return ((unsigned short) guess);
}


unsigned short DiffThreshToTQuality(short DiffThresh)
{
    unsigned long t = (DiffThresh - 4) * 2048;
    return (1024 - isqrt(t));
}


unsigned short DetailThreshToSQuality(short DetailThresh)
{
    return ((unsigned short)
    	    min(
	        1024,
		max(0,(1024 - ((long) DetailThresh << 1)))
	       )
	   );
}


void InsertionSort(short huge * pIn, short huge * pOut, long nMax)
/*
  Simple binary search insertion sort, descending.

  Sort nMax elements from pIn into pOut.
 */
{
    long nNow;
    long iBottom, iMid, iAbove;

    *pOut = *pIn++;		// seed with first value

    for (nNow = 1; nNow < nMax; nNow++, pIn++)
    {

    // Binary search for the first element less than new value.

	iBottom = 0;
	iAbove = nNow;

	while (iAbove > iBottom)
	{
	    iMid = (iAbove + iBottom - 1) >> 1;

	    if (*pIn <= pOut [iMid])
	      iBottom = iMid + 1;
	    else
	      iAbove = iMid;
	}

    // Insert new element in front of pOut[iBottom].

	// Copy pOut [iBottom..nNow-1] to pOut [iBottom+1..nNow]
	// then put the new guy in pOut [iBottom].

	if (iBottom < nNow)
	    BigCopy (&pOut [iBottom], &pOut [iBottom+1],
		(nNow - iBottom) * sizeof (short));

	pOut [iBottom] = *pIn;
    }
}



unsigned short LimitKeyFrameSize(
  CCONTEXT *pC,			// compression context
  TILECONTEXT *pT,		// tile context
  unsigned long HintedSize
)
/*
  Compute size of codes, including header and bit switches.

  Return Detail threshold.
 */
{
  auto unsigned short BookSize;	// including header and bit switches
  auto unsigned long CodeSize;	// including header and bit switches
  auto unsigned long MinFrameSize;
  auto unsigned short DetailThresh;// tentative value
  auto unsigned SizeOfCODE;

  SizeOfCODE =
#if !defined(NOBLACKWHITE)
    pC->Flags.BlackAndWhite ? 4 :
#endif
    sizeof(CODE);

  /*
    Minimum frame size is a codebook plus all smooth plus a bit
    vector.
   */
  MinFrameSize =
    sizeof(SIZETYPE) + 256 * SizeOfCODE +
    sizeof(SIZETYPE) + pT->nPatches +
    (((pT->nPatches + 31) & ~31) >> 3);

  pC->DiffCount = pT->nPatches;

  /*
    Return with the best we can do if the minimum blows the hint.
   */
  if (HintedSize < MinFrameSize) {

    // Yes.  Just return the best we can do - all smooth.
    pC->DetailCount = 0;
    return(0xffff);
  }

  /*
    The codebook should be the smaller of two full codebooks or half (a
    third if intertile in a keyframe) the available space.
   */
  BookSize = (unsigned short) min(
    2 * 256 * SizeOfCODE,	
    HintedSize / ((pT->tType == kKeyTileType) ? 2 : 3)
  );

  // How much space would be left for codes?
  CodeSize = HintedSize - BookSize;

  /*
    nPatches = DetailCount + SmoothCount
     and
    CodeSize =
      sizeof(SIZETYPE) +		// header
      SmoothCount +			// smooth patches
      4*DetailCount +			// detail patches
      ((nPatches + 31) & ~31)/8		// bit switches
     so, substituting,
    DetailCount = (
      CodeSize -
      sizeof(SIZETYPE) -
      nPatches -
      ((nPatches + 31) & ~31)/8
    ) / 3
   */
  pC->DetailCount = min(
    (
      CodeSize -
      sizeof(SIZETYPE) -
      pT->nPatches -
      (((pT->nPatches + 31) & ~31) >> 3)
    ) / 3,
    (unsigned long) pT->nPatches
  );

  if (pC->DetailCount > 0) {

    /*
      Now we know the maximum number of detail patches we can have.
    
      Sort the detail list into descending order, then look up the detail
      threshold value that generates that many detail patches.
     */
    InsertionSort(pC->pDetailList, pC->pDiffList, pT->nPatches);

    /*
      We want to ensure that we include as detail no more than
      pC->DetailCount patches.  Our selection keeps as detail anything
      greater than or equal to the threshold.  So here we make sure that
      if the selected threshold would keep more than pC->DetailCount
      patches, we adjust it up and find the new pC->DetailCount.
     */
    for (
      DetailThresh = pC->pDiffList[pC->DetailCount - 1];
      (
       (pC->DetailCount != pT->nPatches) &&
       (DetailThresh == ((unsigned short) pC->pDiffList[pC->DetailCount]))
      );
      pC->DetailCount++
    );

  } else {

    pC->DetailCount = 0;
    DetailThresh = 0xffff;
  }
  return(DetailThresh);
}


unsigned long EstimateSize(
  CCONTEXT *pC,			// compression context
  TILECONTEXT *pT,		// tile context
  unsigned short DiffThresh,
  unsigned short DetailThresh
)
/*
  Returns space that codes would take up given the thresholds.

  Updates pC->DiffCount and pC->DetailCount.
 */
{
  auto short huge *pDiff;
  auto short huge *pDetail;
  register long SC;
  register long DC;
  auto long i;

  if (pC->fType == kKeyFrameType) {// if key frame

    for (			// for each patch
      pDiff = pC->pDiffList,
	pDetail = pC->pDetailList,
	SC = 0,
	DC = 0,
	i = pT->nPatches;
      i--;
    ) {
      if (((unsigned short) *pDetail++) >= DetailThresh) {
	DC++;
      } else {
	SC++;
      }
    }
  } else {

    for (			// for each patch
      pDiff = pC->pDiffList,
	pDetail = pC->pDetailList,
	SC = 0,
	DC = 0,
	i = pT->nPatches;
      i--;
    ) {
      if (((unsigned short) *pDiff++) >= DiffThresh) {// if above pDiff threshold

	if (((unsigned short) *pDetail) >= DetailThresh) {
	  DC++;
	} else {
	  SC++;
	}
      }
      pDetail++;
    }
  }
  pC->DiffCount = SC + DC;
  pC->DetailCount = DC;

  return(
    sizeof(SIZETYPE) +
    SC +			// smooth codes
    DC*4 +			// detail codes
    (
      (
	(
	  31 +			// up to next dword
	  pT->nPatches +	// bit/patch for pDiff
	  SC + DC		// bit/diff patch for detail vs. smooth
	) & ~31
      ) >> 3
    )
  );
}


unsigned short AllocateEntries(
  CCONTEXT *pC,			// compression context
  TILECONTEXT *pT,		// tile context
  unsigned short nEntries	// number of codebook entries to allocate
)
/*
  Allocate codebook entries between pC->VBook[Detail].nCodes and
  pC->VBook[Smooth].nCodes in approximate proportion of pixels.
  
  Return codebook size.
 */
{
  unsigned short OverheadSize;	// header and bit switches

  OverheadSize = sizeof(SIZETYPE) * 2;// detail & smooth headers

  if (pT->tType != kKeyTileType) {// if intertile
    if (pC->DetailCount) {	// account for detail bitswitches
      OverheadSize += 256/8;
    }
    if (pC->DiffCount != pC->DetailCount) {// account for smooth bitswitches
      OverheadSize += 256/8;
    }
  } else {
    if (!pC->DetailCount) {	// if all smooth
      OverheadSize -= sizeof(SIZETYPE);// toss detailed codebook overhead
    }
  }
  if (pC->DiffCount) {		// if there are patches
    /*
      allocate codebook space in approximate proportion as pixels
     */
    pC->VBook[Detail].nCodes = (unsigned short) (
      (((unsigned long) pC->DetailCount) * (unsigned long) nEntries) /
      (unsigned long) pC->DiffCount
    );
    if (pC->VBook[Detail].nCodes > 256) {
      pC->VBook[Detail].nCodes = 256;
    }
    pC->VBook[Smooth].nCodes = nEntries - pC->VBook[Detail].nCodes;
    if (pC->VBook[Smooth].nCodes > 256) {
      pC->VBook[Smooth].nCodes = 256;
      pC->VBook[Detail].nCodes = nEntries - 256;
    }
    /*
      Pin nCodes to a minimum of 8 but no more than the number of
      vectors in the VBook.
     */
    if (pC->VBook[Detail].nCodes < 8) {// pin no less than 8
      pC->VBook[Detail].nCodes = (short) (
	(pC->DetailCount < 2) ?
	(pC->DetailCount << 2) :// DetailCount is patches here, not vectors
	8
      );
    }
    if (pC->VBook[Smooth].nCodes < 8) {
      pC->VBook[Smooth].nCodes = (short) (
	((pC->DiffCount - pC->DetailCount) < 8) ?
	(pC->DiffCount - pC->DetailCount) :
	8
      );
    }
  } else {
    pC->VBook[Detail].nCodes = 0;// no entries if no vectors
    pC->VBook[Smooth].nCodes = 0;
  }
  return(
    OverheadSize +
    (
      (
#if !defined(NOBLACKWHITE)
	pC->Flags.BlackAndWhite ? 4 :
#endif
	sizeof(CODE)
      ) *
      (pC->VBook[Detail].nCodes + pC->VBook[Smooth].nCodes)
    )
  );
}


void LimitSizeOfCodes(
  CCONTEXT *pC,			// compression context
  TILECONTEXT *pT,		// tile context
  unsigned long HintedSize,
  unsigned short huge *pDiffThresh,	// IN/OUT
  unsigned short huge *pDetailThresh	// IN/OUT
)
/*
  Adjust pDiffThresh and pDetailThresh until the size of the codes
  (including header and bitswitches) is no greater than 80% of
  HintedSize.
 */
{
  unsigned long CodeSize;
  unsigned long EstimatedSize;
  unsigned long PCOfCodeSize;
  int Bail;

  if (pC->fType == kKeyFrameType) {

    // Since this is a key frame, we do not have a difference list.
    // Thus, we can use the difference list region as a scratch area
    // for the sort in LimitKeyFrameSize.
    *pDetailThresh = LimitKeyFrameSize(
      pC,			// compression context
      pT,			// tile context
      HintedSize		// size requested
    );
    return;
  }

  // CodeBook gets 20% of the frame space.
  CodeSize = HintedSize - min(256L*2*sizeof(CODE), HintedSize / 5);

  for (Bail = 0;;) {		// until break

    EstimatedSize = EstimateSize(
      pC,			// compression context
      pT,			// tile context
      *pDiffThresh,		// prev/curr frame threshold
      *pDetailThresh		// detail/smooth threshold
    );

    PCOfCodeSize = EstimatedSize * 100L / CodeSize;

    if ((PCOfCodeSize <= 100) || (Bail == 20)) {
      break;
    }

    /*
      Increase the thresholds by about 5% and try again.
     */
    *pDiffThresh = min(
      200,
      (unsigned short) ((*pDiffThresh * PCOfCodeSize + 95/2) / 95)
    );
    *pDetailThresh = (unsigned short) (
      (*pDetailThresh * PCOfCodeSize + 95/2) / 95
    );

    Bail++;
  }
  return;
}



void CalcMap(
    short huge * List,
    unsigned short Thresh,
    long Count
)
{
    for (; Count--; List++)
    {
	*List = (((unsigned short) *List) >= Thresh);
    }
}


void CalcDetailMap(
    short huge * DetailList,
    short huge * DiffMap,
    unsigned short DetailThresh,
    long Count
)
{
  for (; Count--; DiffMap++, DetailList++)
  {
    *DetailList = *DiffMap && (((unsigned short) *DetailList) >= DetailThresh);
  }
}


void RateControl(
  CCONTEXT *pC,			// compression context
  TILECONTEXT *pT,		// tile context
  unsigned long HintedSize,	// if caller gave hint at desired size
  unsigned short SQuality,	// 0..1023 quality slider
  unsigned short TQuality	// 0..1023 quality slider
)
/*
  We use HintedSize, SQuality and TQuality to determine how deep the
  compression ought to be.  We control it by:
    a)  thresholding pC->pDiffList on interframes, changing the number
	of patdches updated this frame
    b)  thresholding pC->pDetailList, changing the split between detail
	and smooth patches
    c)  adjusting the amount of space allocated to updating the detail
	and smooth codebooks.

  The result of thresholding pC->pDiffList is the booleans we leave in
  the list.  Similarly, we leave booleans in pC->pDetailList reflecting
  its thresholding.  We leave in pC->VBook[Detail].nCodes and
  pC->VBook[Detail].nUpdateCodes the allocation of space for detail
  codebook entries this frame, and in pC->VBook[Smooth].nCodes and
  pC->VBook[Smooth].nUpdateCodes the smooth.

  We first see what size we would get if we adjusted our compression
  solely from TQuality and SQuality.  If the resulting size is no larger
  than HintedSize, then that is the compression we perform.

  If the size as derived from the quality settings is larger than
  HintedSize, then we compress as necessary to accomplish that size.
 */
{
  auto unsigned short DiffThresh;
  auto unsigned short DetailThresh;

  auto unsigned short nEntries;

  auto unsigned short Quality;

  auto unsigned long CodeSize;
  auto unsigned short CodeBookSize;

  DiffThresh = TQualityToDiffThresh(TQuality);
  DetailThresh = SQualityToDetailThresh(SQuality);

  CodeSize = EstimateSize(	// compute code size based on quality
    pC,				// compression context
    pT,				// tile context
    DiffThresh,			// prev/curr frame threshold
    DetailThresh		// detail/smooth threshold
  );
  /*
    The number of codebook entries is figured from the quality.  (We use
    the average of TQuality and SQuality.)

    Quality in [0..511] is mapped linearly to [64..380] entries if a key
    frame and [16..366] if inter.  Quality in [512..1023] is mapped
    linearly to [381..512] if key and [367..512] if inter.
   */
  Quality = (TQuality + SQuality + 1) >> 1;

  nEntries = (
    pC->fType == kKeyFrameType
  ) ? (
    (Quality < 512) ?
    Map(Quality, 0, 511, 64, 380) :
    Map(Quality, 512, 1023, 381, 512)
  ) : (
    (Quality < 512) ?
    Map(Quality, 0, 511, 16, 366) :
    Map(Quality, 512, 1023, 367, 512)
  );

  CodeBookSize = AllocateEntries(
    pC,				// compression context
    pT,				// tile context
    nEntries			// codebook entries to allocate
  );
  /*
    CodeSize now holds what the size of the codes portion of the tile
    would be if we were to compress based on TQuality and SQuality, and
    CodeBookSize holds the size of the codebooks.
   */
  if (HintedSize && (HintedSize < (CodeSize + CodeBookSize))) {
    auto int SizeOfCODE;

    SizeOfCODE =
#if !defined(NOBLACKWHITE)
      pC->Flags.BlackAndWhite ? 4 :
#endif
      sizeof(CODE);

    /*
      Compressing to TQuality and SQuality would not produce a small
      enough frame.

      We instead compress to match the frame size.
     */
    DiffThresh = TQualityToDiffThresh(1023);// max
    DetailThresh = SQualityToDetailThresh(1023);

    LimitSizeOfCodes(
      pC,			// compression context
      pT,			// tile context
      HintedSize,		// size requested
      &DiffThresh,		// IN/OUT:  prev/curr threshold
      &DetailThresh		// IN/OUT:  detail/smooth threshold
    );

    CodeSize =
      ((unsigned long) pC->DiffCount) +
      ((unsigned long) pC->DetailCount) * 3 +
      sizeof(SIZETYPE);

    if (pC->fType == kKeyFrameType) {

      if (pC->DetailCount) {	// if not all smooth

	CodeSize += ((pT->nPatches + 31) & ~31) >> 3;// count BitSwitches
      }
    } else {
      CodeSize += (		// count BitSwitches
	(
	  pT->nPatches +	// difference bits
	  pC->DiffCount +	// detail bits
	  31
	) & ~31
      ) >> 3;
    }
    /*
      CodeSize is the number of bytes that will be taken up by the codes.

      Now we compute the size of the codebooks.
     */
    CodeBookSize = sizeof(SIZETYPE) * 2;// detail & smooth headers

    if (pT->tType != kKeyTileType) {// if intertile
      if (pC->DetailCount) {	// account for detail bitswitches
	CodeBookSize += 256/8;
      }
      if (pC->DiffCount != pC->DetailCount) {// account for smooth bitswitches
	CodeBookSize += 256/8;
      }
    } else {
      if (!pC->DetailCount) {	// if all smooth
	CodeBookSize -= sizeof(SIZETYPE);// toss detailed codebook overhead
      }
    }
    /*
      CodeBookSize now holds bytes in header and bitswitches.
     */
    if (HintedSize <= (32*SizeOfCODE + CodeSize + CodeBookSize)) {
      nEntries = 32;		// minimum of 32 codebook entries
    } else {
      if (HintedSize >= (256*2*SizeOfCODE + CodeSize + CodeBookSize)) {
	nEntries = 256*2;	// maximum of 256 codebook entries
      } else {
	nEntries =
	  (
	    (unsigned short) (HintedSize - CodeSize - CodeBookSize)
	  ) / SizeOfCODE;
      }
    }
    if ((pT->tType == kKeyTileType) && (nEntries < 64)) {
      nEntries = 64;		// at least 64 entries in key codebooks
    }
    CodeBookSize = AllocateEntries(
      pC,			// compression context
      pT,			// tile context
      nEntries			// codebook entries to allocate
    );
  }
  pC->VBook[Detail].nUpdateCodes = pC->VBook[Detail].nCodes;
  pC->VBook[Smooth].nUpdateCodes = pC->VBook[Smooth].nCodes;

  /*
    Turn the thresholds into booleans.
   */
  if (pC->fType == kKeyFrameType) {

    CalcMap (pC->pDetailList, DetailThresh, pT->nPatches);

  } else {

    CalcMap(pC->pDiffList, DiffThresh, pT->nPatches);
    CalcDetailMap(pC->pDetailList, pC->pDiffList, DetailThresh, pT->nPatches);
  }
}
