/* ex:set sw=2 ts=8 wm=0:
 * $Header: u:/rcs/cv/rcs/cvcompre.c 2.36 1995/10/02 10:51:44 bog Exp $

 * (C) Copyright 1992-1994 SuperMac Technology, Inc.
 * All rights reserved

 * This source code and any compilation or derivative thereof is the
 * sole property of SuperMac Technology, Inc. and is provided pursuant
 * to a Software License Agreement.  This code is the proprietary
 * information of SuperMac Technology and is confidential in nature.
 * Its use and dissemination by any party other than SuperMac Technology
 * is strictly limited by the confidential information provisions of the
 * Agreement referenced above.

 * $Log: cvcompre.c $
 * Revision 2.36  1995/10/02 10:51:44  bog
 * Separate CClass and VClass parameters for MatchAndRefine for
 * clarity.
 * 
 * Revision 2.35  1995/05/09 09:23:08  bog
 * Move WINVER back into the makefile.  Sigh.
 *
 * Revision 2.34  1995/03/14  08:21:36  bog
 * 1.  No one was looking at the remembered previous frame's smooth
 *     vectors, so there is no point in remembering them.
 * 2.  We update the previous frame's remembered detail vectors to be as
 *     refined (quantized) rather than as incoming, improving the decision
 *     about what to update in the next frame.
 * 
 * Revision 2.33  1994/12/14  10:49:36  bog
 * Make the frame size computations all long.
 * 
 * Revision 2.32  1994/10/31  08:51:40  bog
 * Don't re-origin segment base before calling WriteTile.
 *
 * Revision 2.31  1994/10/23  17:22:23  bog
 * Try to allow big frames.
 *
 * Revision 2.30  1994/10/20  17:28:21  bog
 * Modifications to support Sunnyvale Reference version.
 *
 * Revision 2.29  1994/09/08  13:53:29  bog
 * Access to FRAME and TILE should be huge except in WriteTile.
 *
 * Revision 2.28  1994/08/16  13:22:13  timr
 * Computations in CVCompressFramesInfo could easily overflow for large data
 * rates, resulting in unexpected reductions in the produced frame rate.
 *
 * Revision 2.27  1994/07/18  13:30:36  bog
 * Move WINVER definition from makefile to each .c file.
 *
 * Revision 2.26  1994/05/02  08:20:24  bog
 * New default neighbors setup.
 *
 * Revision 2.25  1994/05/01  23:38:43  unknown
 * Move nCodes based max neighbors calculation down to MatchAndRefine().
 *
 * Revision 2.24  1994/05/01  16:15:16  bog
 * Fix default NeighborsRounds.
 *
 * Revision 2.23  1994/05/01  16:06:51  unknown
 * Build the Neighbors list only in the first two rounds in ConvergeCodeBook.
 * In subsequent rounds, just reuse the last one built.
 *
 * Revision 2.22  1994/04/30  14:43:27  bog
 * Measurement of performance improvement from skipping InsertNeighbor:
 * 15,116,516 actual inserts of 48,037,808 possible on 46 frames of bike.avi.
 *
 * Revision 2.21  1994/04/30  11:26:10  bog
 * Add nCodes basis for Neighbors range.  Seems more reasonable than nVectors.
 *
 * Revision 2.20  1994/04/06  16:33:53  bog
 * Add NeighborsRange to SYSTEM.INI stuff.  Maybe only experimental.
 *
 * Revision 2.19  1994/03/17  10:42:07  timr
 * Correct MIPS alignment faults.
 *
 * Revision 2.18  1994/03/02  16:09:20  timr
 * Fill Squares at compile time, not run time.
 *
 * Revision 2.17  1993/11/05  11:35:28  bog
 * GlobalFix/GlobalUnfix expect handle, not selector.
 *
 * Revision 2.16  1993/10/21  14:07:20  geoffs
 * Updated LOGREQUESTED stuff to include more data
 *
 * Revision 2.15  1993/10/21  09:49:57  geoffs
 * Added CD-ROM padding for compressions
 *
 * Revision 2.14  1993/10/14  16:17:57  geoffs
 * Don't need to do GlobalFix and Unfix for WIN32
 *
 * Revision 2.13  1993/10/14  16:03:54  geoffs
 * Must fix segments we access linearly during compression
 *
 * Revision 2.12  1993/10/12  17:24:28  bog
 * RGB555 is now Draw15/Expand15 and RGB565 is Draw15/Expand16.
 *
 * Revision 2.11  1993/10/06  16:37:44  timr
 * Squares table should be long for NT, short for Windows.
 *
 * Revision 2.10  1993/10/05  13:15:33  geoffs
 * Additional changes to handle keyrates of 0 for frame size generation
 *
 * Revision 2.9  1993/10/04  12:24:51  geoffs
 * Make sure Keyrate of 0 works for internal frame size generation during compression
 *
 * Revision 2.8  1993/10/03  09:11:44  geoffs
 * Slightly more correct,optimal frame size generation
 *
 * Revision 2.7  1993/10/01  12:36:21  geoffs
 * Now processing ICM_COMPRESSFRAMESINFO w/internal frame size generation
 *
 * Revision 2.6  1993/09/24  14:07:32  geoffs
 * Have an approximation of progress for compression in call to Status callback
 *
 * Revision 2.5  1993/09/23  17:21:18  geoffs
 * Now correctly processing status callback during compression
 *
 * Revision 2.4  1993/09/09  09:12:27  geoffs
 * Fix up uninitialized HintedSize in CVCompress
 *
 * Revision 2.3  1993/08/05  17:05:14  timr
 * Hack to workaround CL386 bug which overwrites the tile header.
 *
 * Revision 2.2  1993/08/04  14:58:51  timr
 * Remove a "," that CL386 cares about and CL doesn't.
 *
 * Revision 2.1  1993/07/06  08:55:30  geoffs
 * 1st pass WIN32'izing
 *
 * Revision 2.0  93/06/01  14:13:30  bog
 * Version 1.0 Release 1.3.0.1 of 1 June 1993.
 *
 * Revision 1.31  93/05/11  17:02:55  bog
 * INT 3 only if DEBUG.
 *
 * Revision 1.30  93/05/02  15:46:50  bog
 * Add LogRequested flag in system.ini [iccvid.drv] to note requested size and
 * quality.
 *
 * Revision 1.29  93/04/21  15:46:58  bog
 * Fix up copyright and disclaimer.
 *
 * Revision 1.28  93/04/15  13:35:39  geoffs
 * Don't allow HintedSizeIn to change for recursion call
 *
 * Revision 1.27  93/03/10  15:16:43  bog
 * Enable insertion of key frames.
 *
 * Revision 1.26  93/02/18  21:11:20  bog
 * pC->Flags.Neighbors is now pC->Neighbors; copying codebook context from
 * the prior tile means also copying nCodes.
 *
 * Revision 1.25  93/02/16  08:24:13  bog
 * Ensure frames no larger than requested.
 *
 * Revision 1.24  93/02/07  12:27:39  bog
 * We were getting InterCodeBook on empty codebooks.
 *
 * Revision 1.23  93/02/05  17:00:40  bog
 * Force NEIGHBORS value to something based on vector list size.
 *
 * Revision 1.22  93/02/05  16:54:23  bog
 * WIGGLE_NEIGHBOR code to get statistics.
 *
 * Revision 1.21  93/02/03  09:52:13  bog
 * Make NEIGHBORS a SYSTEM.INI parameter.
 *
 * Revision 1.20  93/01/31  13:10:43  bog
 * Add logic to refine only vectors and codebook entries that are unlocked on
 * interframes.  Peter calls it "MSEFractional".
 *
 * Revision 1.19  93/01/28  18:07:42  geoffs
 * Tile smooth,detail vectors must be allocated size of maximum tile's height
 *
 * Revision 1.18  93/01/28  16:24:32  bog
 * DEBUG check for change of value with fUPDATED off.
 *
 * Revision 1.17  93/01/27  23:03:20  bog
 * Get interframes working.
 *
 * Revision 1.16  93/01/21  11:04:27  timr
 * Qualify "Type" with another character so it passes H2INC.
 *
 * Revision 1.15  93/01/20  13:33:40  bog
 * Get inter codebooks and frames working.
 *
 * Revision 1.14  93/01/19  16:18:08  geoffs
 * Parameters to BigCopy are (src,dest,cnt), not (dest,src,cnt)
 *
 * Revision 1.13  93/01/19  13:47:18  geoffs
 * HintedSize s/b unsigned long everywhere
 *
 * Revision 1.12  93/01/19  12:23:59  geoffs
 * If not keyframe, make sure context Type == kInterFrameType
 *
 * Revision 1.11  93/01/19  09:48:55  geoffs
 * Replaced references to _fmemcpy with calls to internal BigCopy instead
 *
 * Revision 1.10  93/01/18  22:06:31  bog
 * Fix some inter problems.
 *
 * Revision 1.9  93/01/17  15:52:59  bog
 * Compute distance using a squares table rather than multiplying.
 *
 * Revision 1.8  93/01/16  16:08:03  bog
 * Compressed first frame!
 *
 * Revision 1.7  93/01/14  22:36:58  bog
 * Clean up oBits computation.  Invoke WriteTile.  Calculate FrameSize
 * correctly.
 *
 * Revision 1.6  93/01/13  16:00:24  bog
 * It compressed a frame!
 *
 * Revision 1.5  93/01/12  17:13:53  bog
 * TILEs now have VECTORs instead of YYYYUVs.
 *
 * Revision 1.4  93/01/12  10:43:59  bog
 * Tiles now hold VECTORs, not YYYYUVs.
 *
 * Revision 1.3  93/01/11  17:36:32  geoffs
 * Accommodate 2 quality parameters, CVCompressEnd now returns a void
 *
 * Revision 1.2  93/01/11  09:45:28  bog
 * First halting steps towards integration.
 *
 * Revision 1.1  93/01/06  10:12:31  bog
 * Initial revision
 *
 *
 * CompactVideo Codec Compressor Top Level
 */

#if	defined(WINCPK)

#define _WINDOWS
#include <stdio.h>
#include <memory.h>             // included for _fmemcpy
#include <stdlib.h>
#ifdef  NULL
#undef  NULL
#endif

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <commdlg.h>
#if     !defined(WIN32)
#include <ver.h>
#endif
#include <compddk.h>

#endif				// defined(WINCPK)

#include "cv.h"
#include "cvcompre.h"

/**********************************************************************
 *
 * CVCompress()
 *
 * Compress a framebuffer into a CV frame
 *
 * Returns size of compressed frame or neg if error
 *
 * Assumes that the first frame after CompressBegin is key
 *
 **********************************************************************/

long CVCompress(
	CCONTEXT *pC,                 // compression context
	unsigned long oBits,          // 32 bit offset of base of input DIB
#if     !defined(WIN32)
	unsigned short sBits,         // selector to input DIB
#endif
	FRAME huge *pF,		// destination compressed frame
	char *pKeyFrame,              // in/out keyframe flag
	unsigned long HintedSizeIn,   // desired frame size; 0 if don't care
	unsigned short SQuality,      // 0..1023 spatial quality slider
	unsigned short TQuality)       // 0..1023 temporal quality slider
{
	auto unsigned long oBits0;    // offset to scanline 0
	
	auto TILECONTEXT *pT;
	
	auto TILE huge *pTile;	// -> building tile
	
	auto long FrameSize;          // size of entire compressed frame
	auto long TileSize;           // size of this compressed tile
	
	auto unsigned long HintedSize;// calc'ed hinted size
	auto unsigned long TileHintedSize;// size per tile
	auto unsigned long RecompressHintedSize;// calc'ed hinted size if recurse
	
	auto unsigned short usTmp;
	
	auto LOGREQUESTEDTILE far *pL;// useful for debug
	
#if     defined(DEBUG) || defined(DBG)
	DebugBreak();
#endif
	
	oBits0 = oBits;               // remember in case recurse
	if (*pKeyFrame)
	{
		pC->fType = kKeyFrameType;
		pC->pT[0]->tType = kKeyTileType;
	}
	else
	{
		pC->fType = kInterFrameType;
		pC->pT[0]->tType = kInterTileType;
	}
	
#if     !defined(WIN32)
	//
	// fix objects we reference in flat model so that they can't move
	// around in linear memory
	//
	GlobalFix((HGLOBAL)GlobalHandle(sBits));// incoming DIB
	GlobalFix((HGLOBAL)GlobalHandle(SELECTOROF(pC->pDiffList)));// per context
	GlobalFix((HGLOBAL)GlobalHandle(SELECTOROF(pC->pDetailList)));
	GlobalFix((HGLOBAL)GlobalHandle(SELECTOROF(pC->VBook[Detail].pVectors)));
	GlobalFix((HGLOBAL)GlobalHandle(SELECTOROF(pC->VBook[Smooth].pVectors)));
	for (pT = pC->pT[0]; pT; pT = pT->pNextTile) { // for each tile...
		GlobalFix((HGLOBAL)GlobalHandle(SELECTOROF(pT->pDetail)));
	}
#endif

	//
	// determine the size that we'll be crunching this frame to
	//
	CalcFrameSize(pC,HintedSizeIn,&HintedSize,&RecompressHintedSize,&SQuality,&TQuality);
	
	//
	// determine the allocation of the bytes of the frame among the tiles
	//
	if (HintedSize)
	{
		TileHintedSize =            // divvy up among the tiles
			((HintedSize - (sizeof(FRAME) - sizeof(TILE)) -// FRAME includes TILE
				(pC->Flags.LogRequested ? sizeof(LOGREQUESTEDTILE) : 0)
			 ) / pC->nTiles) - (sizeof(TILE) - sizeof(CODEBOOK));// TILE includes CODEBOOK
	}
	else {
		TileHintedSize = 0;         // let tiles get as big as they can
	}
	
	//
	// zero out the detail,smooth total errors
	//
	pC->VBook[Detail].TotalError = pC->VBook[Smooth].TotalError = 0;
	
	//
	// early abort pre-handling
	//
	if (
#if	defined(WINCPK)
		!setjmp(pC->envAbort)
#else
		1				// no abort in reference version
#endif
	)
	{
		VFW_START(pC);                      // starting compression
		VFW_STATUS(pC,0); pC->icssp.nMatchAndRefineCalls = 0;
		
		pTile=pC->Flags.LogRequested?((TILE*)(((char*)pF->Tile)+sizeof(LOGREQUESTEDTILE))):pF->Tile;
		pC->Difference=0;
		FrameSize=((char *)pTile)-(char *)pF;
		
		// for each tile, advancing oBits
		for(pT=pC->pT[0];pT;
			pT = pT->pNextTile
		)
		{
			// if key frame, tiles after 1st are inter
			if ((pC->fType == kKeyFrameType) && (pT->TileNum))
			{
				BigCopy(
					// base for inter tile is tile above
					pC->pT[pT->TileNum-1]->PBook[Detail].pCodes,
					pT->PBook[Detail].pCodes,
					sizeof(*pT->PBook[Detail].pCodes) << 8
				);
				pT->PBook[Detail].nCodes = pC->pT[pT->TileNum-1]->PBook[Detail].nCodes;
				BigCopy(
					pC->pT[pT->TileNum-1]->PBook[Smooth].pCodes,
					pT->PBook[Smooth].pCodes,
					sizeof(*pT->PBook[Smooth].pCodes) << 8
				);
				pT->PBook[Smooth].nCodes = pC->pT[pT->TileNum-1]->PBook[Smooth].nCodes;
			}
			TileSize = CompressTile(
				pC,                     // compression context
				pT,                     // tile context
				oBits,                  // 32 bit offset of base of input tile
#if     !defined(WIN32)
				sBits,                  // selector to input tile
#endif
				pTile,                  // where to build the compressed tile
				TileHintedSize,         // if caller gave hint at desired size
				SQuality,               // 0..1023 spatial quality slider
				TQuality                // 0..1023 temporal quality slider
			);
			if (TileSize < 0)
			{       // if some error in compress
				VFW_END(pC);            // tell VFW we are done
				return (CVCOMPRESS_ERR_ERROR);// die with error
			}
			
			// increment/offset everything...  I hate big for statements
			pTile = (TILE huge *) (((char huge *) pTile) + TileSize);
			FrameSize += TileSize;
			oBits += pC->DIBYStep * pT->Height;			
		}
		VFW_END(pC);                // end of compression
		if (pC->fType == kKeyFrameType)
		{
			pC->FramesSinceKey = 0;
		}
		else
		{
			auto long DifferenceChange;// change in temporal diff/pixel
			
			pC->Difference /= pC->nPixels;// now temporal diff per pixel
			if (pC->LastDifference == -1L)
			{
				pC->LastDifference = pC->Difference;
			}
			DifferenceChange =(pC->Difference - pC->LastDifference)*(12L + (long) pC->FramesSinceKey);
			
			pC->FramesSinceKey++;
				
			/*
			pC->LastDifference remembers the average temporal difference from
			the last interframe.  It would have been nice to remember it for
			keyframes, too, but we never compute it.  So our DifferenceChange
			computation uses what we have instead of the ideal.
			*/
			pC->LastDifference = pC->Difference;

			//
			// if this last interframe compression resulted in a difference change
			// that was too great relative to the last interframe compression,
			// we'll recompress to a natural keyframe
			//
			if (pC->bRecompressingToKey = (DifferenceChange > 4800))
			{
				*pKeyFrame = 1;         // force key frame
				FrameSize = CVCompress(
					pC,                 // compression context
					oBits0,             // 32 bit offset of base of input DIB
#if     !defined(WIN32)
					sBits,              // selector to input DIB
#endif
					pF,                 // destination compressed frame
					pKeyFrame,          // in/out keyframe flag
					RecompressHintedSize,// desired frame size; 0 if don't care
					SQuality,           // 0..1023 spatial quality slider
					TQuality            // 0..1023 temporal quality slider
				);
				pC->bRecompressingToKey = FALSE;
				goto unfix_objects_and_exit;
			}
		}

		CopySwiz4(pF->SizeType.SwizSize,(((long) pC->fType) << 24) | FrameSize);
		pF->SwizWidth = swiz2a(pC->FrameWidth);
		pF->SwizHeight = swiz2a(pC->FrameHeight);
		pF->SwizNumTiles = swiz2a(pC->nTiles);
		
		if (pC->Flags.LogRequested)
		{
			pL = (LOGREQUESTEDTILE far *) pF->Tile;
			CopySwiz4(pL->SizeType.SwizSize,(((unsigned long)kLogRequestedType)<<24L)|sizeof(LOGREQUESTEDTILE));
			CopySwiz4(pL->SwizHintedSizeReq, HintedSizeIn);
			CopySwiz4(pL->SwizHintedSizeCalc, HintedSize);
			usTmp = swiz2a(SQuality);
			pL->SwizSQuality = usTmp;
			usTmp = swiz2a(TQuality);
			pL->SwizTQuality = usTmp;
			CopySwiz4(pL->SwizDetailError, pC->VBook[Detail].TotalError);
			CopySwiz4(pL->SwizSmoothError, pC->VBook[Smooth].TotalError);
		}
		
		//
		// update # match and refine calls this compression
		//
		pC->icssp.nMaxMatchAndRefineCalls = pC->icssp.nMatchAndRefineCalls;
		
		//
		// early abort post-handling here
		//
	}
	else
	{ // early abort was detected...
		VFW_END(pC);
		FrameSize = CVCOMPRESS_ERR_ABORT;// abort return
	}
		
	//
	// update internal values if generating frame sizes internally
	//
	UpdateFrameSize(pC,FrameSize,*pKeyFrame);

unfix_objects_and_exit:

#if     !defined(WIN32)
	//
	// unfix objects we referenced in flat model so that they can now move
	// around in linear memory
	//
	GlobalUnfix((HGLOBAL)GlobalHandle(sBits));// incoming DIB
	GlobalUnfix((HGLOBAL)GlobalHandle(SELECTOROF(pC->pDiffList)));// per context
	GlobalUnfix((HGLOBAL)GlobalHandle(SELECTOROF(pC->pDetailList)));
	GlobalUnfix((HGLOBAL)GlobalHandle(SELECTOROF(pC->VBook[Detail].pVectors)));
	GlobalUnfix((HGLOBAL)GlobalHandle(SELECTOROF(pC->VBook[Smooth].pVectors)));
	for (pT = pC->pT[0]; pT; pT = pT->pNextTile) { // for each tile...
		GlobalUnfix((HGLOBAL)GlobalHandle(SELECTOROF(pT->pDetail)));
	}
#endif

	return(FrameSize);
}


/**********************************************************************
 *
 * CompressTile()
 *
 * Compress a band of a DIB into a CV tile
 *
 * Returns size of compressed tile or neg if error
 *
 **********************************************************************/

long CompressTile(              // returns tile size or neg if error
	CCONTEXT *pC,                 // compression context
	TILECONTEXT *pT,              // tile context
	unsigned long oBits,          // 32 bit offset of base of input tile
#if     !defined(WIN32)
	unsigned short sBits,         // selector to input tile
#endif
	TILE huge *pTile,		// where to build the compressed tile
	unsigned long HintedSize,     // if caller gave hint at desired size
	unsigned short SQuality,      // 0..1023 spatial quality slider
	unsigned short TQuality)       // 0..1023 temporal quality slider
{
	/*
	  Convert incoming DIB to DetailVectors, SmoothVectors, DetailList and
	  DiffList.
	 */
	DIBToVectors(
		pC,                         // compression context
		pT,                         // tile context
		oBits,                      // 32 bit offset of base of input tile
#if     !defined(WIN32)
		sBits,                      // selector to input tile
#endif
		HintedSize,                 // if caller gave hint at desired size
		SQuality,                   // 0..1023 spatial quality slider
		TQuality                    // 0..1023 temporal quality slider
	);

	/*
	  build or adapt and then refine the codebooks
	 */
	CodeBook(pC, pT, &pC->VBook[Detail], &pT->PBook[Detail]);
	CodeBook(pC, pT, &pC->VBook[Smooth], &pT->PBook[Smooth]);

	/*
	  put the refined vector values into pT->pDetail so that if the next
	  frame is inter we can diff against the actual quantized prevous
	  frame rather than the incoming previous frame
	 */
	RememberQuantized(pC, pT);

	/*
	  unload context to output frame
	 */
	return(WriteTile(pC, pT, pTile));
}


/**********************************************************************
 *
 * Neighbors Stuff
 *
 **********************************************************************/

NEIGHBORSRANGE NeighborsRange = {
  {   0,	  0,	 0,	TEXT("Neighbors0")},
  { 128,	 32,	 4,	TEXT("Neighbors1")},
  { 256,	 64,	 8,	TEXT("Neighbors2")},
  { 512,	 96,	12,	TEXT("Neighbors3")},
  {1024,	128,	16,	TEXT("Neighbors4")},
  {2048,	160,	32,	TEXT("Neighbors5")},
  {4096,	192,	32,	TEXT("Neighbors6")},
  {8192,	224,	32,	TEXT("Neighbors7")}
};
TCHAR *szIniNeighborsMethod = TEXT("NeighborsMethod");
TCHAR *szIniNeighborsRounds = TEXT("NeighborsRounds");
extern TCHAR szIniFile[];
extern TCHAR szIniSection[];


/**********************************************************************
 *
 * CVCompressBegin()
 *
 * Allocate data structures and initialize them for CV Codec compression
 *
 * Returns near ptr to compression context or 0 if error
 *
 **********************************************************************/

CCONTEXT *CVCompressBegin(      // returns compression context or 0
  short Width,                  // width (pixels) of rect in source bitmap
  short Height,                 // height (pixels) of rect in source bitmap
  long YStep,                   // bytes from (x,y) to (x,y+1) in source DIB
  DIBTYPE DibType,              // type of source DIB
  void far *pLookup,            // -> lookup table for 8 bpp
  COMPRESSINFO far *pCI,        // -> compress frames info data
  ICSETSTATUSPROC far *pICSSP,  // -> set status proc info
  COMPRESSFLAGS Flags           // session flags
) {
  auto CCONTEXT *pC;            // -> our context
  auto short i;
  auto TILECONTEXT *pT;         // -> first tile
  auto short RemainingHeight;   // # scanlines 0 mod 4 for full frame
  auto short RemainingDIBHeight;// # scanlines for full frame
  auto short TileHeight;        // # scanlines 0 mod 4 in first tile
  auto long nPatches;           // # patches in 0 mod 4 frame

  if (!(pC = (CCONTEXT *) LocalAlloc(LPTR, sizeof(CCONTEXT)))) {
    return(0);                  // alloc failure
  }
  memset(pC,0,sizeof(CCONTEXT));
  for (i = kMaxTileCount; i--;) {// not yet allocated
    pC->pT[i] = (TILECONTEXT *) 0;
  }
  pC->pDiffList = (short huge *) 0;// not yet allocated
  pC->pDetailList = (short huge *) 0;
  pC->VBook[Detail].pECodes = (EXTCODE far *) 0;
  pC->VBook[Detail].pVectors = (VECTOR huge *) 0;
  pC->VBook[Smooth].pECodes = (EXTCODE far *) 0;
  pC->VBook[Smooth].pVectors = (VECTOR huge *) 0;

  pC->DIBYStep = YStep;         // actual ystep for DIB
  pC->DIBWidth = Width;         // actual width for DIB
  pC->DIBHeight = RemainingDIBHeight = Height;
  pC->DIBType = DibType;        // type of DIB
  pC->nPixels = ((long) Width) * (long) Height;// actual pixels in DIB

  pC->FrameWidth = (Width + 3) & ~3;// 0 mod 4 width, height for frame
  pC->FrameHeight = RemainingHeight = (Height + 3) & ~3;

  nPatches = (((long) pC->FrameWidth) * ((long) RemainingHeight)) >> 4;

  pC->nTiles =
    (short) ((nPatches + (((320L*120L)/16L)/2L)) / ((320L*120L)/16L));
  if (pC->nTiles < 1) {
    pC->nTiles = 1;
  } else {
    if (pC->nTiles > kMaxTileCount) {
      pC->nTiles = kMaxTileCount;
    }
  }
  TileHeight = (((RemainingHeight + pC->nTiles - 1) / pC->nTiles) + 3) & ~3;
  // adjust nTiles in case wide lines and rounding lost us the last tile
  pC->nTiles = (RemainingHeight + TileHeight - 1) / TileHeight;
  /*
    We will have pC->nTiles tiles.  All are of width pC->Width.  The
    first (pC->nTiles-1) are of height TileHeight.  Any remainder is in
    the last tile.  No tile is larger than the first.
   */

  //
  // perhaps determine status callback stuff
  //
  CVSetStatusProc(pC,pICSSP);
  pC->icssp.nMaxMatchAndRefineCalls = pC->nTiles * 16;// worst case at start

  //
  // perhaps determine data rate stuff
  //
  CVCompressFramesInfo(pC,pCI);

  //
  // per tile initializations and allocations
  //
  for (i = 0; i < pC->nTiles; i++) {// for each tile

    if (
      !(pC->pT[i] = pT = (TILECONTEXT *) LocalAlloc(LPTR, sizeof(TILECONTEXT)))
    ) {
      return(UnwindCompressBegin(pC));// toss all allocations & die
    }
    
    // added for BEOS...
    memset(pT,0,sizeof(TILECONTEXT));
    
    if (i) {                    // if after 1st tile
      pC->pT[i-1]->pNextTile = pT;// link to next
    }
    pT->pDetail = (VECTOR huge *) 0;// not yet allocated
    pT->PBook[Detail].pCodes = (CCODE far *) 0;
    pT->PBook[Smooth].pCodes = (CCODE far *) 0;

    pT->TileNum = i;
    pT->Height = (TileHeight < RemainingHeight) ? TileHeight : RemainingHeight;
    RemainingHeight -= pT->Height;
    pT->DIBHeight =
      (TileHeight < RemainingDIBHeight) ? TileHeight : RemainingDIBHeight;
    RemainingDIBHeight -= pT->DIBHeight;
    pT->nPatches = (((long) pC->FrameWidth) * ((long) pT->Height)) >> 4;

    if (i) {                    // tiles other than 1st are inter
      pT->tType = kInterTileType;
    }

    if (                        // allocate YYYYUV arrays for tile
      !(
        pT->pDetail = (VECTOR huge *) GlobalAllocPtr(
          GMEM_MOVEABLE,
          pC->pT[0]->nPatches * sizeof(VECTOR) * 4
        )
      ) ||
      !(
        pT->PBook[Detail].pCodes = (CCODE far *) GlobalAllocPtr(
          GMEM_MOVEABLE,
          256 * sizeof(CCODE)
        )
      ) ||
      !(
        pT->PBook[Smooth].pCodes = (CCODE far *) GlobalAllocPtr(
          GMEM_MOVEABLE,
          256 * sizeof(CCODE)
        )
      )
    ) {
      return(UnwindCompressBegin(pC));// toss all allocations & die
    }
  }
  if (                          // allocate frame arrays
    !(
      pC->pDiffList = (short huge *) GlobalAllocPtr(
        GMEM_MOVEABLE,
        pC->pT[0]->nPatches * sizeof(short)
      )
    ) ||
    !(
      pC->pDetailList = (short huge *) GlobalAllocPtr(
        GMEM_MOVEABLE,
        pC->pT[0]->nPatches * sizeof(short)
      )
    ) ||
    !(
      pC->VBook[Detail].pECodes = (EXTCODE far *) GlobalAllocPtr(
        GMEM_MOVEABLE,
        256 * sizeof(EXTCODE)
      )
    ) ||
    !(
      pC->VBook[Detail].pVectors = (VECTOR huge *) GlobalAllocPtr(
        GMEM_MOVEABLE,
        pC->pT[0]->nPatches * sizeof(VECTOR) * 4
      )
    ) ||
    !(
      pC->VBook[Smooth].pECodes = (EXTCODE far *) GlobalAllocPtr(
        GMEM_MOVEABLE,
        256 * sizeof(EXTCODE)
      )
    ) ||
    !(
      pC->VBook[Smooth].pVectors = (VECTOR huge *) GlobalAllocPtr(
        GMEM_MOVEABLE,
        pC->pT[0]->nPatches * sizeof(VECTOR)
      )
    ) ||
    !DIBToYUVBegin(pC,pLookup)
  ) {
    return(UnwindCompressBegin(pC));// toss all allocations & die
  }
  pC->VBook[Detail].Converged = 200;// detail is converged easier
  pC->VBook[Smooth].Converged = 2000;

  pC->LastDifference = -1L;     // next frame will be 1st interframe
  pC->Flags = Flags;            // remember session flags

#ifndef __BEOS__
  /*
    Initialize this movie's range table from SYSTEM.INI
   */
  for (i = sizeof(NeighborsRange)/sizeof(NeighborsRange[0]); i--;) {

    pC->NeighborsRange[i] = (int) GetPrivateProfileInt(
      szIniSection,
      NeighborsRange[i].szIniNeighbors,
      NeighborsRange[i].nNeighbors,
      szIniFile
    );
  }
  pC->NeighborsMethod = (int) GetPrivateProfileInt(
    szIniSection,
    szIniNeighborsMethod,
    1,				// based on nCodes
    szIniFile
  );
  pC->NeighborsRounds = (int) GetPrivateProfileInt(
    szIniSection,
    szIniNeighborsRounds,
    14,				// first two rounds
    szIniFile
  );
#endif
  return (pC);                  // return -> created context
}

/**********************************************************************
 *
 * UnwindCompressBegin()
 *
 * Deallocate data structures when an error occurs in CVCompressBegin
 *
 * Returns 0
 *
 **********************************************************************/

CCONTEXT *UnwindCompressBegin(
  CCONTEXT *pC                  // compression context
) {
  auto TILECONTEXT *pT, *pPrev;

  for (                         // for each tile
    pT = pC->pT[0];
    pT;
    pPrev = pT,
      pT = pT->pNextTile,
      LocalFree((HLOCAL) pPrev)
  ) {
    if (pT->pDetail) {
      GlobalFreePtr(pT->pDetail);
    }
    if (pT->PBook[Detail].pCodes) {
      GlobalFreePtr(pT->PBook[Detail].pCodes);
    }
    if (pT->PBook[Smooth].pCodes) {
      GlobalFreePtr(pT->PBook[Smooth].pCodes);
    }
  }
  if (pC->pDiffList) {
    GlobalFreePtr(pC->pDiffList);
  }
  if (pC->pDetailList) {
    GlobalFreePtr(pC->pDetailList);
  }
  if (pC->VBook[Detail].pECodes) {
    GlobalFreePtr(pC->VBook[Detail].pECodes);
  }
  if (pC->VBook[Detail].pVectors) {
    GlobalFreePtr(pC->VBook[Detail].pVectors);
  }
  if (pC->VBook[Smooth].pECodes) {
    GlobalFreePtr(pC->VBook[Smooth].pECodes);
  }
  if (pC->VBook[Smooth].pVectors) {
    GlobalFreePtr(pC->VBook[Smooth].pVectors);
  }
  DIBToYUVEnd(pC);              // DIBToYUV cleanups
  LocalFree((HLOCAL) pC);

  return((CCONTEXT *) 0);
}


/**********************************************************************
 *
 * CVCompressEnd()
 *
 * Deallocate data structures initialized by CVCompressBegin and below
 *
 **********************************************************************/

void CVCompressEnd(
  CCONTEXT *pC                  // -> our context
) {
  UnwindCompressBegin(pC);      // context-related cleanups
#ifdef	PERFORMANCE_MEASURE
  {
    auto char Buf[80];

    extern long PossibleInserts, ActualInserts;

    wsprintf(Buf, "%ld actual of %ld possible", ActualInserts, PossibleInserts);
    MessageBox(0, Buf, "Huh?", MB_ICONINFORMATION | MB_OK);
  }
#endif
}


/**********************************************************************
 *
 * CodeBook()
 *
 * Build or adapt a codebook and then refine it
 *
 **********************************************************************/

void CodeBook(
  CCONTEXT *pC,                 // compression context
  TILECONTEXT *pT,              // tile context
  VECTORBOOK *pVB,              // current codebook
  CCODEBOOK *pPB                // prev codebook
) {
  auto int i;

  auto EXTCODE far *pEC;
  auto CCODE far *pCC;

  if (!pC->NeighborsMethod) {	// method 0 is based on nVectors
    /*
      Find proper nNeighbors to use for this tile
     */
      for (
	i = sizeof(NeighborsRange)/sizeof(NeighborsRange[0]);
	i-- && (pVB->nVectors < NeighborsRange[i].VRangeBottom);
      );
    pC->Neighbors = pC->NeighborsRange[i];
  }

  if (				// if we're doing a key codebook
    (pT->tType == kKeyTileType)	// if key tile
    || !pPB->nCodes		// or old codebook is empty
  ) {

    KeyCodeBook(pC, pT, pVB);   // build and refine a new codebook

  } else {

    InterCodeBook(pC, pT, pVB, pPB);// adapt and refine an old codebook
  }

  /*
    Save our codebook for the next tile or frame.
   */
  for (
    pCC = pPB->pCodes,
      pEC = pVB->pECodes,
      pPB->nCodes = 0,
      i = 0;
    i < 256;
    i++,
      pCC++,
      pEC++
  ) {
    if (pEC->Flags & fINVALID) {// if invalid code
      pCC->Age = 0xffff;
    } else {
      if (pEC->Flags & fUPDATED) {// if code changed
        pCC->yuv = pEC->yuv;
        pCC->Age = 1;
      } else {

        pCC->Age++;
      }
      pPB->nCodes = (short) (i + 1);
    }
  }
}


/**********************************************************************
 *
 * RememberQuantized()
 *
 * Remember the quantized (post-CodeBook) values in the vectors for the
 * next frame.  This is closer to what is actually on the playback
 * screen.
 *
 **********************************************************************/

void RememberQuantized(
  CCONTEXT *pC,			// compression context
  TILECONTEXT *pT		// tile context
) {
  auto long i;			// count down number of patches
  auto VECTOR huge *pDV;	// current winnowed detail vectors
  auto VECTOR huge *pSV;	// current winnowed smooth vectors
  auto VECTOR huge *pPV;	// detail vectors remembered for next frame
  auto EXTCODE far *pDB;	// this frame's refined detail codebook
  auto EXTCODE far *pSB;	// this frame's refined smooth codebook
  auto short huge *pDetSw;	// TRUE if patch is detail, not smooth
  auto short huge *pDiffSw;	// TRUE if patch is new this frame

  /*
    pT->pDetail is where we hold the whole tile's detail vectors for
    temporal differencing for the next frame.

    pC->VBook[<Detail/Smooth>].pVectors is where we have this frame's
    vectors, winnowed down to the ones that correspond to patches we are
    updating this frame.

    pC->VBook[<Detail/Smooth>].pECodes is where we hold the codebook we
    have already refined this frame.

    On keyframes, pC->pDiffList is meaningless.  On interframes,
    it holds a boolean per patch that is TRUE if the patch is being
    updated this frame and has vectors in
    pC->VBook[<Detail/Smooth>].pVectors.

    pC->pDetailList holds a boolean per patch that says whether a patch
    is represented by four vectors in pC->VBook[<Detail>].pECodes or one
    vector in pC->VBook[<Smooth>].pECodes.

    We traverse the vector lists and update pT->pDetail to have the
    refined codebook yuv values rather than the incoming yuv values so
    that the next frame can decide which patches to update based on a
    closer representation of what is actually on the playback screen.
   */

  for (
    i = pT->nPatches,
      pDV = pC->VBook[Detail].pVectors,
      pSV = pC->VBook[Smooth].pVectors,
      pPV = pT->pDetail,
      pDB = pC->VBook[Detail].pECodes,
      pSB = pC->VBook[Smooth].pECodes,
      pDetSw = pC->pDetailList,
      pDiffSw = pC->pDiffList;
    i--;
    pDetSw++,
      pDiffSw++,
      pPV += 4
  ) {

    if (
      (pC->fType == kKeyFrameType) ||// if keyframe, all patches new
      (*pDiffSw)		// or if this patch was updated this frame
    ) {

      if (*pDetSw) {		// if patch is detail

	pPV[0].yuv = pDB[pDV[0].Code].yuv;
	pPV[1].yuv = pDB[pDV[1].Code].yuv;
	pPV[2].yuv = pDB[pDV[2].Code].yuv;
	pPV[3].yuv = pDB[pDV[3].Code].yuv;

	pDV += 4;

      } else {			// patch is smooth

	pPV[0].yuv.y[0]
	= pPV[0].yuv.y[1]
	= pPV[0].yuv.y[2]
	= pPV[0].yuv.y[3]
	= pSB[pSV->Code].yuv.y[0];

	pPV[1].yuv.y[0]
	= pPV[1].yuv.y[1]
	= pPV[1].yuv.y[2]
	= pPV[1].yuv.y[3]
	= pSB[pSV->Code].yuv.y[1];

	pPV[2].yuv.y[0]
	= pPV[2].yuv.y[1]
	= pPV[2].yuv.y[2]
	= pPV[2].yuv.y[3]
	= pSB[pSV->Code].yuv.y[2];

	pPV[3].yuv.y[0]
	= pPV[3].yuv.y[1]
	= pPV[3].yuv.y[2]
	= pPV[3].yuv.y[3]
	= pSB[pSV->Code].yuv.y[3];

	pPV[0].yuv.u
	= pPV[1].yuv.u
	= pPV[2].yuv.u
	= pPV[3].yuv.u
	= pSB[pSV->Code].yuv.u;

	pPV[0].yuv.v
	= pPV[1].yuv.v
	= pPV[2].yuv.v
	= pPV[3].yuv.v
	= pSB[pSV++->Code].yuv.v;
      }
    }
  }
}


/**********************************************************************
 *
 * PaddedFrameSize()
 *
 * Determine padded size to crunch current frame to
 *
 * NOTE: !!!!the padding boundary must be a power of 2!!!!
 *
 **********************************************************************/

long PaddedFrameSize(
  long FrameSize,		// input frame size
  long OverheadPerFrame,	// overhead per frame
  long PadSize			// boundary to pad to, 0 if not padding
) {
  if (PadSize) {		// will be padding...
    return (
      ((FrameSize + OverheadPerFrame + PadSize - 1) & ~(PadSize - 1))
      - OverheadPerFrame
    );
  }
  return (FrameSize);		// not padding -- just return input
}

/**********************************************************************
 *
 * CalcFrameSize()
 *
 * Determine size to crunch current frame to
 *
 **********************************************************************/

void CalcFrameSize(
  CCONTEXT *pC,                         // -> compression context
  unsigned long HintedSize,             // requested hinted size
  unsigned long far *HintedSizeOut,     // calc'ed hinted size
  unsigned long far *RecompressHintedSizeOut,// calc'ed size if recompress
  unsigned short far *SQuality,         // use quality
  unsigned short far *TQuality          // use quality
) {
  if (pC->bRecompressingToKey) { // we've recursed...
    *HintedSizeOut =
    *RecompressHintedSizeOut = HintedSize;
  }
  else if (pC->iccf.fValidICCF) { // data smoothing...

    if (!pC->iccf.InterframesLeft) { // keyframe, but not natural keyframe...
      *HintedSizeOut = PaddedFrameSize(
        pC->iccf.FrameSizes[pC->iccf.nNew].KeySize,
        pC->iccf.OverheadPerFrame,
        pC->iccf.PadSize
      );
    }
    else { // return interframe size...
      *HintedSizeOut = PaddedFrameSize(
        pC->iccf.FrameSizes[pC->iccf.nNew].InterSize,
        pC->iccf.OverheadPerFrame,
        pC->iccf.PadSize
      );
    }

    *RecompressHintedSizeOut = PaddedFrameSize(
      pC->iccf.FrameSizes[pC->iccf.nNew].KeySize,
      pC->iccf.OverheadPerFrame,
      pC->iccf.PadSize
    );
    *SQuality = *TQuality = 1023;// max the quality setting
  }
  else if (HintedSize) { // if caller supplies size hint
    *HintedSizeOut =
    *RecompressHintedSizeOut = HintedSize;
  }
  else {
    *HintedSizeOut =
    *RecompressHintedSizeOut = 0;// let it get as big as it can get
  }
}


/**********************************************************************
 *
 * UpdateFrameSize()
 *
 * Update values when calculating frame sizes internally
 *
 **********************************************************************/

void UpdateFrameSize(
  CCONTEXT *pC,                         // -> compression context
  long FrameSize,                       // real size of generated frame
  BOOL bKeyframe                        // TRUE if current frame was key
) {
  if (pC->iccf.fValidICCF) { // doing internal frame size generation...
    auto long oldestSize;
    auto long FrameSizePadded;

    //
    // calculated padded framesize -- we use this value to calculate the
    // size of our next frame
    //
    if (pC->iccf.PadSize) { // we are padding...
      FrameSizePadded = PaddedFrameSize(
	FrameSize,
	pC->iccf.OverheadPerFrame,
	pC->iccf.PadSize
      );
    }
    else { // no padding -- just use actual frame size
      FrameSizePadded = FrameSize;
    }

    //
    // add in the bytes for the frame we just compressed -- we remember
    // the actual compressed frame size in the InterSize fields of our array
    //
    pC->iccf.FrameSizes[pC->iccf.nNew].InterSize = FrameSizePadded;
    pC->iccf.BytesInFirstCU += FrameSizePadded;
    pC->iccf.nNew = (pC->iccf.nNew + 1) % pC->iccf.FPU;

    //
    // update # interframes remaining before next keyframe
    //
    if (pC->iccf.InterframesLeft != ~0) {
      pC->iccf.InterframesLeft = (
        (bKeyframe) ? (pC->iccf.FPU - 1) : (pC->iccf.InterframesLeft - 1)
      );
    }

    //
    // remember the # bytes contribution from the earliest frame in the
    // previous CU byte total
    //
    oldestSize = pC->iccf.FrameSizes[pC->iccf.nNew].InterSize;

    //
    // finally, calculate the size of the next frame based on the # bytes
    // already generated in the first CU
    //
    // this calculation is done over a 2 CU period so that we never suffer
    // the ill effects of keyframes having dropped from existence in the
    // preceding first CU history period
    //
    pC->iccf.FrameSizes[pC->iccf.nNew].InterSize = (
      ((long) pC->iccf.ratioLower)
      * ((pC->iccf.BPU << 1) - pC->iccf.BytesInFirstCU)
    ) / (
      (((long) (pC->iccf.FPU - 1)) * (long) pC->iccf.ratioLower)
      + (long) pC->iccf.ratioUpper
    );
    pC->iccf.FrameSizes[pC->iccf.nNew].KeySize = (
       pC->iccf.FrameSizes[pC->iccf.nNew].InterSize * pC->iccf.ratioUpper
    ) / pC->iccf.ratioLower;

    //
    // once we have calculated the size of the coming frame, we can safely
    // update the previous CU period byte total, which will not yet take into
    // account the frame to be generated
    //
    pC->iccf.BytesInFirstCU -= oldestSize;
  }
}


/**********************************************************************
 *
 * CVSetStatusProc()
 *
 * Initialize -> to status callback routine
 *
 * Returns TRUE/FALSE depending
 **********************************************************************/

int CVSetStatusProc(
  CCONTEXT *pC,
  ICSETSTATUSPROC far *pICSSP
)
{
#if	defined(WINCPK)
  //
  // check to make sure that we have a valid ICSETSTATUSPROC
  //
  if (!pICSSP || !pICSSP->Status) { // must not be a valid ICSETSTATUSPROC...
    pC->icssp.Status = NULL;
  }
  else {
    //
    // struct seems to be valid -- use the values passed-in to determine
    // internal parameters
    //
    pC->icssp.lParam = pICSSP->lParam;
    pC->icssp.Status = pICSSP->Status;
  }
#endif

  return (TRUE);
}


/**********************************************************************
 *
 * CVCompressFramesInfo()
 *
 * Initialize internal parameters to guide next compression sequence
 *
 * Returns TRUE/FALSE depending
 **********************************************************************/

int CVCompressFramesInfo(
  CCONTEXT *pC,
  LPCOMPRESSINFO pCI
) {
  //
  // check to make sure that we have a valid ICCOMPRESSFRAMES
  //
  if (
    !pCI ||
    (!pCI->dwRate && !pCI->dwScale)
  ) { // must not be a valid ICCOMPRESSFRAMES...
    pC->iccf.fValidICCF = FALSE;
  }
  else {
    //
    // info struct seems to be valid -- use the values passed-in to determine
    // internal parameters
    //

    //
    // In the attempt to smooth the sizes of frames according to the requested
    // data rate, we define a crunching unit (CU from here on) as a sequence
    // of frames from a keyframe to the next keyframe. It needs to be noted
    // that a natural keyframe may be injected by our compression mechanism
    // at any time during a CU, exclusive of the base keyframe rate. The
    // following definitions are in order.
    //
    //  BPS = bytes / second requested
    //  FPS = frame rate / second
    //
    //  FPU = frames / CU
    //  BPU = bytes / CU = (BPS / FPS) * FPU
    //
    // We calculate the above values, save them in our context, initialize
    // current state, and let CalcFrameSize do the rest
    //

    if (
      ((pC->iccf.FPU = (unsigned short) pCI->lKeyRate) <= MAX_FRAME_HISTORY)
    ) {
      auto int i;
      auto long IdealInterframeSize;
      auto long IdealKeyframeSize;

      //
      // if a 0 keyframe rate was requested, we will set the crunch unit
      // to be equal to our default keyframe rate
      //
      if (!pC->iccf.FPU) {
        pC->iccf.FPU = DFLT_KEYFRAMERATE;
        pC->iccf.InterframesLeft = ~0;
      } else {
        pC->iccf.InterframesLeft = 0;
      }

      //
      // calculate the # of video bytes available per crunch unit
      //
      pC->iccf.BPU =
        muldiv32(pCI->dwScale,pCI->lDataRate,pCI->dwRate) * pC->iccf.FPU;

      //
      // remember a padding value if we need to pad
      //
      pC->iccf.PadSize =
        (pCI->dwFlags & ICCOMPRESSFRAMES_PADDING) ? pCI->dwPadding : 0;
      pC->iccf.OverheadPerFrame = pCI->dwOverheadPerFrame;

      //
      // calculate the ideal allocation of bytes to each interframe in the
      // crunch unit
      //
      // derived as follows:
      //
      //        KS = size in bytes of an ideal keyframe
      //        IS = size in bytes of an ideal interframe
      //
      //        ratioUpper = upper part of KS/IS ratio
      //        ratioLower = lower part of KS/IS ratio
      //        NOTE: The above two numbers s/b small integers so as to not
      //              overflow the calculations
      //
      //        FPU and BPU as defined above
      //
      //        We have two equations describing our situation:
      //
      //                KS = (ratioUpper * IS) / ratioLower
      //                BPU = ((FPU - 1) * IS) + KS
      //
      //        Solving through substitution:
      //
      //                IS =            (ratioLower * BPU)
      //                     ---------------------------------------
      //                     (((FPU - 1) * ratioLower) + ratioUpper)
      //
      //                KS = (ratioUpper * IS)
      //                     -----------------
      //                        ratioLower
      //

      //
      // latch keyframe to interframe size ratio
      //
      pC->iccf.ratioUpper = pCI->ratioUpper;
      pC->iccf.ratioLower = pCI->ratioLower;

      //
      // calculate ideal interframe size if all frames were generated at
      // the ideal size
      //
      IdealInterframeSize = (
	((long) pC->iccf.ratioLower) * pC->iccf.BPU
      ) / (
	((((long) pC->iccf.FPU) - 1) * (long) pC->iccf.ratioLower)
	+ (long) pC->iccf.ratioUpper
      );

      IdealKeyframeSize = (
	IdealInterframeSize * pC->iccf.ratioUpper
      ) / pC->iccf.ratioLower;

      //
      // initialize slots of previous CU frames
      //
      // the oldest frame will be set to be a keyframe, all others are
      // interframes
      //
      for (i = 1; i < pC->iccf.FPU; i++) {
        pC->iccf.FrameSizes[i].InterSize = IdealInterframeSize;
      }

      //
      // 1st frame will be a keyframe
      //
      pC->iccf.nNew = 0;
      pC->iccf.FrameSizes[0].InterSize =
      pC->iccf.FrameSizes[0].KeySize = IdealKeyframeSize;
      pC->iccf.BytesInFirstCU = IdealInterframeSize * (pC->iccf.FPU - 1);

      pC->iccf.fValidICCF = TRUE;
    }
  }
  return(pC->iccf.fValidICCF);
}


unsigned int Squares[511] = {           // squares of -255..255
  65025,  64516,  64009,  63504,  63001,  62500,  62001,  61504,
  61009,  60516,  60025,  59536,  59049,  58564,  58081,  57600,
  57121,  56644,  56169,  55696,  55225,  54756,  54289,  53824,
  53361,  52900,  52441,  51984,  51529,  51076,  50625,  50176,
  49729,  49284,  48841,  48400,  47961,  47524,  47089,  46656,
  46225,  45796,  45369,  44944,  44521,  44100,  43681,  43264,
  42849,  42436,  42025,  41616,  41209,  40804,  40401,  40000,
  39601,  39204,  38809,  38416,  38025,  37636,  37249,  36864,
  36481,  36100,  35721,  35344,  34969,  34596,  34225,  33856,
  33489,  33124,  32761,  32400,  32041,  31684,  31329,  30976,
  30625,  30276,  29929,  29584,  29241,  28900,  28561,  28224,
  27889,  27556,  27225,  26896,  26569,  26244,  25921,  25600,
  25281,  24964,  24649,  24336,  24025,  23716,  23409,  23104,
  22801,  22500,  22201,  21904,  21609,  21316,  21025,  20736,
  20449,  20164,  19881,  19600,  19321,  19044,  18769,  18496,
  18225,  17956,  17689,  17424,  17161,  16900,  16641,  16384,
  16129,  15876,  15625,  15376,  15129,  14884,  14641,  14400,
  14161,  13924,  13689,  13456,  13225,  12996,  12769,  12544,
  12321,  12100,  11881,  11664,  11449,  11236,  11025,  10816,
  10609,  10404,  10201,  10000,   9801,   9604,   9409,   9216,
   9025,   8836,   8649,   8464,   8281,   8100,   7921,   7744,
   7569,   7396,   7225,   7056,   6889,   6724,   6561,   6400,
   6241,   6084,   5929,   5776,   5625,   5476,   5329,   5184,
   5041,   4900,   4761,   4624,   4489,   4356,   4225,   4096,
   3969,   3844,   3721,   3600,   3481,   3364,   3249,   3136,
   3025,   2916,   2809,   2704,   2601,   2500,   2401,   2304,
   2209,   2116,   2025,   1936,   1849,   1764,   1681,   1600,
   1521,   1444,   1369,   1296,   1225,   1156,   1089,   1024,
    961,    900,    841,    784,    729,    676,    625,    576,
    529,    484,    441,    400,    361,    324,    289,    256,
    225,    196,    169,    144,    121,    100,     81,     64,
     49,     36,     25,     16,      9,      4,      1,      0,
      1,      4,      9,     16,     25,     36,     49,     64,
     81,    100,    121,    144,    169,    196,    225,    256,
    289,    324,    361,    400,    441,    484,    529,    576,
    625,    676,    729,    784,    841,    900,    961,   1024,
   1089,   1156,   1225,   1296,   1369,   1444,   1521,   1600,
   1681,   1764,   1849,   1936,   2025,   2116,   2209,   2304,
   2401,   2500,   2601,   2704,   2809,   2916,   3025,   3136,
   3249,   3364,   3481,   3600,   3721,   3844,   3969,   4096,
   4225,   4356,   4489,   4624,   4761,   4900,   5041,   5184,
   5329,   5476,   5625,   5776,   5929,   6084,   6241,   6400,
   6561,   6724,   6889,   7056,   7225,   7396,   7569,   7744,
   7921,   8100,   8281,   8464,   8649,   8836,   9025,   9216,
   9409,   9604,   9801,  10000,  10201,  10404,  10609,  10816,
  11025,  11236,  11449,  11664,  11881,  12100,  12321,  12544,
  12769,  12996,  13225,  13456,  13689,  13924,  14161,  14400,
  14641,  14884,  15129,  15376,  15625,  15876,  16129,  16384,
  16641,  16900,  17161,  17424,  17689,  17956,  18225,  18496,
  18769,  19044,  19321,  19600,  19881,  20164,  20449,  20736,
  21025,  21316,  21609,  21904,  22201,  22500,  22801,  23104,
  23409,  23716,  24025,  24336,  24649,  24964,  25281,  25600,
  25921,  26244,  26569,  26896,  27225,  27556,  27889,  28224,
  28561,  28900,  29241,  29584,  29929,  30276,  30625,  30976,
  31329,  31684,  32041,  32400,  32761,  33124,  33489,  33856,
  34225,  34596,  34969,  35344,  35721,  36100,  36481,  36864,
  37249,  37636,  38025,  38416,  38809,  39204,  39601,  40000,
  40401,  40804,  41209,  41616,  42025,  42436,  42849,  43264,
  43681,  44100,  44521,  44944,  45369,  45796,  46225,  46656,
  47089,  47524,  47961,  48400,  48841,  49284,  49729,  50176,
  50625,  51076,  51529,  51984,  52441,  52900,  53361,  53824,
  54289,  54756,  55225,  55696,  56169,  56644,  57121,  57600,
  58081,  58564,  59049,  59536,  60025,  60516,  61009,  61504,
  62001,  62500,  63001,  63504,  64009,  64516,  65025,
};
