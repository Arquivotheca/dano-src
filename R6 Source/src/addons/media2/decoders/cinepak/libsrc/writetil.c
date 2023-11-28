/* ex:set sw=2 ts=8 wm=0:
 * $Header: u:/rcs/cv/rcs/writetil.c 2.10 1995/05/09 09:23:43 bog Exp $

 * (C) Copyright 1992-1994 SuperMac Technology, Inc.
 * All rights reserved

 * This source code and any compilation or derivative thereof is the
 * sole property of SuperMac Technology, Inc. and is provided pursuant
 * to a Software License Agreement.  This code is the proprietary
 * information of SuperMac Technology and is confidential in nature.
 * Its use and dissemination by any party other than SuperMac Technology
 * is strictly limited by the confidential information provisions of the
 * Agreement referenced above.

 * $Log: writetil.c $
 * Revision 2.10  1995/05/09 09:23:43  bog
 * Move WINVER back into the makefile.  Sigh.
 * Revision 2.9  1994/12/14  10:41:59  bog
 * Round up size of INDICES to next even.
 * 
 * Revision 2.8  1994/10/23  17:23:11  bog
 * Try to allow big frames.
 * 
 * Revision 2.7  1994/10/20  17:47:58  bog
 * Modifications to support Sunnyvale Reference version.
 * 
 * Revision 2.6  1994/09/22  17:06:58  bog
 * Use union instead of #define for pO->.
 * 
 * Revision 2.5  1994/07/18  13:31:03  bog
 * Move WINVER definition from makefile to each .c file.
 * 
 * Revision 2.4  1994/05/09  17:26:28  bog
 * Black & White compression works.
 * 
 * Revision 2.3  1994/03/17  10:44:05  timr
 * Correct MIPS alignment faults.
 * 
 * Revision 2.2  1993/07/24  20:25:09  bog
 * Mac decompression cannot take key codebook in intertile.
 * 
 * Revision 2.1  19/3./7.  6.:2.:6.  geoffs
 * Remove VER.H, remove prototype for BigCopy.
 * 
 * Revision 2.0  93/06/01  14:17:35  bog
 * Version 1.0 Release 1.3.0.1 of 1 June 1993.
 * 
 * Revision 1.12  93/04/21  15:59:42  bog
 * Fix up copyright and disclaimer.
 * 
 * Revision 1.11  93/01/28  22:40:39  bog
 * YA pBitSwitches[-1] f.u.
 * 
 * Revision 1.10  93/01/28  16:26:41  bog
 * Be a bit more efficient on small frames.
 * 
 * Revision 1.9  93/01/21  15:48:30  geoffs
 * Actual size of frame written differed from calculated size
 * 
 * Revision 1.8  93/01/21  12:50:56  bog
 * Make vector pointers HUGE for simple > 65k lists.
 * 
 * Revision 1.7  93/01/21  11:05:18  timr
 * Qualify "Type" with another character so it passes H2INC.
 * 
 * Revision 1.6  93/01/20  17:22:24  bog
 * Compression works!
 * 
 * Revision 1.5  93/01/20  14:56:25  bog
 * Inter frames running.  Maybe.
 * 
 * Revision 1.4  93/01/20  13:34:51  bog
 * Get inter codebooks and frames working.
 * 
 * Revision 1.3  93/01/16  17:22:29  geoffs
 * Fixed up writing of Smooth codes
 *
 * Revision 1.2  93/01/16  16:13:00  geoffs
 * Forgot to turn on detail bit in BitSwitches; forgot to swiz4 it; rectangle
 * was backwards, too.
 *
 * Revision 1.1  93/01/14  22:37:33  bog
 * Initial revision
 *
 *
 * CompactVideo Codec Stream Generator
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
 * WriteTile()
 *
 * Write the current tile to the frame output
 *
 **********************************************************************/

long WriteTile(
  CCONTEXT *pC,			// compression context
  TILECONTEXT *pT,		// tile context
  TILE huge *pTile		// where to build the compressed tile
) {
  auto int i;
  auto long l;
  auto CCODEBOOKINDEX CBIndex;

  auto int BitsUsed;
  auto unsigned long BitSwitches=0;
  auto unsigned long huge *pBitSwitches;

  auto union {
    void huge *pvoid;		// -> output tile
    CODEBOOK huge *pCBook;
    CODE huge *pYuv;
    unsigned char huge *pCode;
    unsigned long huge *pulong;
    INDICES huge *pIndices;
  } U;

  auto INDICES huge *pIndices;	// remember the beginning of the codes

  /*
    We remember the tile's rectangle at the beginning of the tile.
   */
  pTile->SwizRect.SwizTop = pTile->SwizRect.SwizLeft = 0;
  pTile->SwizRect.SwizRight = swiz2a(pC->FrameWidth);
  pTile->SwizRect.SwizBottom = swiz2a(pT->Height);

  /*
    We put out
      - the detail codebook,
      - the smooth codebook, then
      - the codes.
   */

  U.pCBook = &pTile->Book[0];	// detail codebook goes here

  for (CBIndex = Detail; CBIndex <= Smooth; CBIndex++) {
    auto int LastValidAndUpdated;// last valid codebook entry
    auto int nValid;		// number of valid codebook entries
    auto int nUpdated;		// number of updated codebook entries

    auto EXTCODE far *pEC;

    /*
      Count valid and updated codebook entries.
     */
    for (
      pEC = pC->VBook[CBIndex].pECodes,
	LastValidAndUpdated = -1,
	nValid = 0,
	nUpdated = 0,
	i = 0;
      i < pC->VBook[CBIndex].nCodes;
      i++, pEC++
    ) {
      if (!(pEC->Flags & fINVALID)) {// if valid codebook entry

	nValid++;

	if (pEC->Flags & fUPDATED) {

	  LastValidAndUpdated = i;
	  nUpdated++;
	}
      }
    }

    /*
      A full codebook would contain LastValidAndUpdated+1 codebook
      entries.  A partial codebook contains 32 bytes of bit switches and
      nUpdated codebook entries.

      The Mac decompressor checks consistency of the TileType against
      the CodeBookType.  We want our byte stream to be playable on the
      Mac.  So rather than deciding whether to ship a full or partial
      codebook by the size of the resulting chunk, we ship a full
      codebook if the tile is full, and a partial one if the tile is
      partial.
     */
    if (pT->tType == kKeyTileType) {

      static unsigned long FullBookType[2] = {
	((unsigned long) kFullDBookType) << 24,
	((unsigned long) kFullSBookType) << 24
      };

#if !defined(NOBLACKWHITE)
      CopySwiz4(
	U.pCBook->SizeType.SwizSize,
	FullBookType[CBIndex] | (
	  pC->Flags.BlackAndWhite ?
	  (
	    (((long) kGreyBookBit) << 24L) |
	    ((4 * (unsigned long) (LastValidAndUpdated + 1)) + 4)
	  ) : (
	    (sizeof(CODE) * (unsigned long) (LastValidAndUpdated + 1)) + 4
	  )
	)
      );
#else
      CopySwiz4(
	U.pCBook->SizeType.SwizSize,
	FullBookType[CBIndex] | (
	  (sizeof(CODE) * (unsigned long) (LastValidAndUpdated + 1)) + 4
	)
      );
#endif

      for (			// for each codebook entry
	i = LastValidAndUpdated + 1,
	  U.pYuv = &U.pCBook->Code[0],
	  pEC = pC->VBook[CBIndex].pECodes;
	i--;
	U.pYuv++, pEC++
      ) {
	U.pYuv->Y[0] = pEC->yuv.y[0];
	U.pYuv->Y[1] = pEC->yuv.y[1];
	U.pYuv->Y[2] = pEC->yuv.y[2];
	U.pYuv->Y[3] = pEC->yuv.y[3];
#if !defined(NOBLACKWHITE)
	if (pC->Flags.BlackAndWhite) {
	  U.pYuv = (CODE huge *) (((char huge *) U.pYuv) - 2);
	} else
#endif
	{
	  U.pYuv->U = pEC->yuv.u ^ 0x80;// u,v in stream is -128..127
	  U.pYuv->V = pEC->yuv.v ^ 0x80;
	}
      }
    } else {			// only partial codebooks go out
      auto unsigned long huge *pSwizSize;
      auto int EntriesWritten;

      static unsigned long PartialBookType[2] = {
	((unsigned long) kPartialDBookType) << 24,
	((unsigned long) kPartialSBookType) << 24
      };

      for (			// for each possible codebook entry
	i = 0,
	  BitsUsed = 0,
	  EntriesWritten = 0,
	  pSwizSize = &U.pCBook->SizeType.SwizSize,
	  pBitSwitches = (unsigned long huge *) &U.pCBook->Code[0],
	  U.pYuv = (CODE huge *) &pBitSwitches[1],
	  pEC = pC->VBook[CBIndex].pECodes;
	i < 256;
	i++, pEC++
      ) {

	BitSwitches <<= 1;	// make room for new bit

	/*
	  If this code is new this tile
	 */
	if (
	  !(pEC->Flags & fINVALID) &&// if this code valid
	  (pEC->Flags & fUPDATED)// and code changed this frame
	) {
	  U.pYuv->Y[0] = pEC->yuv.y[0];
	  U.pYuv->Y[1] = pEC->yuv.y[1];
	  U.pYuv->Y[2] = pEC->yuv.y[2];
	  U.pYuv->Y[3] = pEC->yuv.y[3];
#if !defined(NOBLACKWHITE)
	  if (pC->Flags.BlackAndWhite) {
	    U.pYuv = (CODE huge *) (((char huge *) U.pYuv) - 2);
	  } else
#endif
	  {
	    U.pYuv->U = pEC->yuv.u ^ 0x80;// u,v in stream is -128..127
	    U.pYuv->V = pEC->yuv.v ^ 0x80;
	  }
	  BitSwitches |= 1;	// note this code included
	  U.pYuv++;
	  EntriesWritten++;
	}
	/*
	  If we have filled up the bit switch dword, dump it back where
	  we were and make room for another.
	 */
	if (++BitsUsed == 32) {
	  CopySwiz4 (*pBitSwitches, BitSwitches);
	  pBitSwitches = U.pulong;
	  U.pYuv = (CODE huge *) &pBitSwitches[1];
	  BitsUsed = 0;
	}
      }
#if !defined(NOBLACKWHITE)
      CopySwiz4 (
	*pSwizSize,
	PartialBookType[CBIndex] | (
	  pC->Flags.BlackAndWhite ?
	  (
	    (((long) kGreyBookBit) << 24L) |
	    ((4 * (unsigned long) EntriesWritten) + 32 + 4)
	  ) : (
	    (sizeof(CODE) * (unsigned long) EntriesWritten) + 32 + 4
	  )
	)
      );
#else
      CopySwiz4 (
	*pSwizSize,
	PartialBookType[CBIndex] | (
	  (sizeof(CODE) * (unsigned long) EntriesWritten) + 32 + 4
	)
      );
#endif
      /*
	Unmake room for the last BitSwitches.
       */
      U.pvoid = (void huge *) (&U.pulong[-1]);
    }
  }
  /*
    Tiles in key frames that have no detail patches go out as all
    smooth.  If they have detail patches, they go out as key.

    Inter frame tiles go out as inter.
   */
  pIndices = U.pIndices;
  U.pCode = pIndices->Index;

  if (pC->fType == kKeyFrameType) {// full codes go out

    if (!pC->DetailCount) {	// if all smooth
      auto VECTOR huge *pV;

      pIndices->SizeType.sType = kAllSmoothCodesType;

      for (			// write out the smooth codes
	l = pT->nPatches,
	  pV = pC->VBook[Smooth].pVectors;
	l--;
	*U.pCode++ = pV++->Code
      );

    } else {
      auto VECTOR huge *pDetail;
      auto VECTOR huge *pSmooth;
      auto short huge *pDetailSwitch;

      /*
	One byte per smooth patch, 4 bytes per detail patch, and enough
	dwords to hold a bitswitch per patch.  Plus 4 bytes for the
	SizeType.
       */
      pIndices->SizeType.sType = kIntraCodesType;

      for (			// write out the codes
	l = pT->nPatches,
	  BitsUsed = 0,
	  pBitSwitches = U.pulong,
	  U.pCode = (void huge *) &pBitSwitches[1],
	  pDetail = pC->VBook[Detail].pVectors,
	  pSmooth = pC->VBook[Smooth].pVectors,
	  pDetailSwitch = pC->pDetailList;
	l--;
      ) {

	BitSwitches <<= 1;	// make room for new switch

	if (*pDetailSwitch++) {	// if this patch is detail

	  U.pCode[0] = pDetail[0].Code;
	  U.pCode[1] = pDetail[1].Code;
	  U.pCode[2] = pDetail[2].Code;
	  U.pCode[3] = pDetail[3].Code;

	  U.pCode += 4;
	  pDetail += 4;

	  BitSwitches |= 1;	// mark as detail

	} else {

	  *U.pCode++ = pSmooth++->Code;
	}
	if (++BitsUsed == 32) {
	  CopySwiz4 (*pBitSwitches, BitSwitches);
	  pBitSwitches = U.pulong;
	  U.pCode = (unsigned char huge *) &pBitSwitches[1];
	  BitsUsed = 0;
	}
      }
      if (BitsUsed) {		// if BitSwitches still dangling
	BitSwitches <<= 32 - BitsUsed;
	CopySwiz4 (*pBitSwitches, BitSwitches);
      } else {			// unmake room for switches
	U.pCode = (unsigned char huge *) pBitSwitches;
      }
    }
  } else {			// only partial codes go out
    auto VECTOR huge *pDetail;
    auto VECTOR huge *pSmooth;
    auto short huge *pDetailSwitch;
    auto short huge *pDiffSwitch;

    /*
      Four bytes per detail patch, one byte per smooth patch, and enough
      dwords to hold a diff bitswitch per patch and a detail/smooth
      bitswitch per updated patch.  Plus 4 bytes for the SizeType.
     */
    pIndices->SizeType.sType = kInterCodesType;

    for (			// for each possible patch
      l = 0,
	BitsUsed = 0,
	pBitSwitches = U.pulong,
	U.pCode = (void huge *) &pBitSwitches[1],
	pDetail = pC->VBook[Detail].pVectors,
	pSmooth = pC->VBook[Smooth].pVectors,
	pDetailSwitch = pC->pDetailList,
	pDiffSwitch = pC->pDiffList;
      l < pT->nPatches;
      l++, pDiffSwitch++, pDetailSwitch++
    ) {

      BitSwitches <<= 1;	// make room for diff switch

      if (*pDiffSwitch) {	// if this patch is updated

	BitSwitches |= 1;	// mark patch as present

	if (++BitsUsed == 32) {
	  CopySwiz4 (*pBitSwitches, BitSwitches);
	  pBitSwitches = U.pulong;
	  U.pCode = (unsigned char huge *) &pBitSwitches[1];
	  BitsUsed = 0;
	}
	BitSwitches <<= 1;	// make room for detail switch

	if (*pDetailSwitch) {	// if this patch is detail

	  U.pCode[0] = pDetail[0].Code;
	  U.pCode[1] = pDetail[1].Code;
	  U.pCode[2] = pDetail[2].Code;
	  U.pCode[3] = pDetail[3].Code;

	  U.pCode += 4;
	  pDetail += 4;

	  BitSwitches |= 1;	// mark as detail

	} else {

	  *U.pCode++ = pSmooth++->Code;
	}
      }
      if (++BitsUsed == 32) {
	CopySwiz4 (*pBitSwitches, BitSwitches);
	pBitSwitches = U.pulong;
	U.pCode = (unsigned char huge *) &pBitSwitches[1];
	BitsUsed = 0;
      }
    }
    if (BitsUsed) {		// if BitSwitches still dangling
      BitSwitches <<= 32 - BitsUsed;
      CopySwiz4 (*pBitSwitches, BitSwitches);
    } else {			// unmake room for switches
      U.pCode = (unsigned char huge *) pBitSwitches;
    }
  }
  BitSwitches = (
    ((unsigned char huge *) U.pvoid) - ((unsigned char huge *) pIndices) + 1
  ) & ~1L;
  CopySwiz4(
    pIndices->SizeType.SwizSize,
    (((unsigned long) pIndices->SizeType.sType) << 24) | BitSwitches
  );

  (unsigned char huge *) U.pvoid =
    ((unsigned char huge *) pIndices) + BitSwitches;

  BitSwitches =
    ((unsigned char huge *) U.pvoid) - ((unsigned char huge *) pTile);
  CopySwiz4 (
    pTile->SizeType.SwizSize,
    (((unsigned long) pT->tType) << 24) | BitSwitches
  );

  return(BitSwitches);
}
