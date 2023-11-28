/* ex:set sw=2 ts=8 wm=0:
 * $Header: u:/rcs/cv/rcs/cvdecomp.c 3.23 1995/06/07 12:54:56 bog Exp $

 * (C) Copyright 1992-1995 Radius Inc.
 * All rights reserved

 * This source code and any compilation or derivative thereof is the
 * sole property of Radius Inc. and is provided pursuant to a Software
 * License Agreement.  This code is the proprietary information of
 * Radius and is confidential in nature.  Its use and dissemination by
 * any party other than Radius is strictly limited by the confidential
 * information provisions of the Agreement referenced above.

 * $Log: cvdecomp.c $
 * Revision 3.23  1995/06/07 12:54:56  bog
 * Precedence oops:  bad playback to CPLA in WIN32.
 * Revision 3.22  1995/05/09  09:23:12  bog
 * Move WINVER back into the makefile.  Sigh.
 * 
 * Revision 3.21  1995/04/29  12:51:41  bog
 * Change the parsing of incoming atoms to limit possible access to bad
 * addresses:
 * 1.  Include size of frame in the for each tile loop.
 * 2.  Include size of tile in a for each atom in tile loop, with no order
 *     dependency on atoms within the tile.
 * 
 * Revision 3.20  1995/02/22  12:38:03  bog
 * Add EndingCodeBooksFromContext so the decompression codebook tables
 * survive a destination depth change.
 * 
 * Revision 3.19  1994/10/20  17:32:52  bog
 * Modifications to support Sunnyvale Reference version.
 * 
 * Revision 3.18  1994/09/22  17:07:52  bog
 * Change ExpandXxx and DrawXxx to 4CC; add UYVY.
 * 
 * Revision 3.17  1994/09/08  13:53:39  bog
 * Oops on code cleanup.  Can't have #if/#endif inside macro invocation.
 * 
 * Revision 3.16  1994/08/30  14:07:30  bog
 * Fix up warnings from PPC compile:  unreferenced i and unreachable statement.
 * 
 * Revision 3.15  1994/08/30  09:29:26  bog
 * Fix up non-ASM, non-X86 #ifdef stuff from MOTION changes.
 * 
 * Revision 3.14  1994/08/16  13:13:30  timr
 * Fix iMotion fixup for Win32 versions.
 * 
 * Revision 3.13  1994/08/16  10:02:40  timr
 * Remove references to old patch scheme.
 * 
 * Revision 3.12  1994/07/29  15:30:57  bog
 * Weitek YUV420 works.
 * 
 * Revision 3.11  1994/07/25  16:07:24  bog
 * Change fixup mechanism to include Weitek YUV.
 * 
 * Revision 3.10  1994/05/10  19:23:20  timr
 * (bog)  Couple of problems with new YStep changing code in x86 WIN32 ASM.
 * 
 * Revision 3.9  1994/04/30  02:50:25  unknown
 * (bog)  Clean up DECOMPRESS_SET_PALETTE for C version.
 * 
 * Revision 3.8  1994/03/29  13:19:59  bog
 * Allow YStep to change between frames of a movie.
 * 
 * Revision 3.7  1994/03/24  16:55:00  timr
 * For DCI, allow the DIB YStep to change without requiring End/Begin.
 * 
 * Revision 3.6  1994/03/04  14:23:31  bog
 * Can't have #if in a macro.
 * 
 * Revision 3.5  1994/03/04  14:01:06  bog
 * CVDecompressSetPalette must copy standard table if nPalColors is zero.
 * 
 * Revision 3.4  1994/03/04  12:39:10  bog
 * Too many "address of"s.
 * 
 * Revision 3.3  1994/03/03  11:59:51  timr
 * Add code to support SET_PALETTE on non-x86 platforms.
 * 
 * Revision 3.2  1994/03/02  16:20:44  timr
 * Add support for ICM_DECOMPRESS_SET_PALETTE.
 * 
 * Revision 3.1  1994/01/13  10:19:16  geoffs
 * Must pass height down to draw*.c routines for all C version
 * 
 * Revision 3.0  1993/12/10  14:32:15  timr
 * Incorporate changes to accomodate C-only NT version of codec.
 * 
 * Revision 2.15  1993/11/29  16:04:19  geoffs
 * Pixel doubling done for 8bpp decompress
 * 
 * Revision 2.14  1993/11/05  11:34:57  bog
 * Mismatched GlobalFix/GlobalUnfix.
 * GlobalFix/GlobalUnfix expect handle, not selector.
 * 
 * Revision 2.13  1993/10/28  09:55:56  bog
 * Unfixing the wrong thing.
 * 
 * Revision 2.12  1993/10/14  16:18:03  geoffs
 * Don't need to do GlobalFix and Unfix for WIN32
 * 
 * Revision 2.11  1993/10/14  16:04:56  geoffs
 * Must fix segments we access linearly during decompression
 * 
 * Revision 2.10  1993/10/12  17:24:38  bog
 * RGB555 is now Draw15/Expand15 and RGB565 is Draw15/Expand16.
 * 
 * Revision 2.9  1993/10/11  13:03:28  geoffs
 * Must fix TEXT32 in linear space between Begin,End brackets
 * 
 * Revision 2.8  1993/07/02  16:34:39  geoffs
 * Now compiles,runs under Windows NT
 * 
 * Revision 2.7  93/06/14  11:34:57  bog
 * Remove CVFAR; we will clone for Thun acceleration.
 * 
 * Revision 2.6  93/06/13  11:21:06  bog
 * Add hooks for playback acceleration.
 * 
 * Revision 2.5  93/06/09  17:58:20  bog
 * Support 32 bit DIBs.
 * 
 * Revision 2.4  93/06/08  16:47:46  geoffs
 * GlobalFix those selectors which get aliased
 * 
 * Revision 2.3  93/06/03  12:24:53  geoffs
 * Free up code selector (from CVDecompressBegin) in CVDecompressEnd
 * 
 * Revision 2.2  93/06/02  13:27:23  bog
 * Ensure things still work if the incoming frame crosses a segment boundary.
 * 
 * Revision 2.1  93/06/01  14:48:21  bog
 * Compiled, flat decompress assembler.
 * 
 * Revision 2.0  93/06/01  14:13:42  bog
 * Version 1.0 Release 1.3.0.1 of 1 June 1993.
 * 
 * Revision 1.7  93/04/21  15:47:12  bog
 * Fix up copyright and disclaimer.
 * 
 * Revision 1.6  93/01/21  11:04:32  timr
 * Qualify "Type" with another character so it passes H2INC.
 * 
 * Revision 1.5  93/01/15  14:52:43  timr
 * Incorporate 16 bit decompressor.
 * 
 * Revision 1.4  93/01/06  11:51:47  geoffs
 * Rationalized i/f's for CVCompress... and CVDecompress...
 * 
 * Revision 1.3  92/12/21  18:43:26  timr
 * Decompressor now works split.
 * 
 * Revision 1.2  92/12/21  14:01:23  bog
 * CVDecompress stuff now compiles.
 * 
 * Revision 1.1  92/12/21  11:36:13  bog
 * Initial revision
 * 
 *
 * CompactVideo Codec decompressor top level
 */

#if	defined(WINCPK)

#define	_WINDOWS
#include <stdio.h>
#include <memory.h>		// included for _fmemcpy
#include <stdlib.h>
#include <stddef.h>
#if	defined(NULL)
#undef	NULL
#endif

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <commdlg.h>

#endif

#ifdef __BEOS__
#include <stddef.h>
#include "beos.h"
#include <OS.h>
#endif

#include "cv.h"
#include "cvdecomp.h"


extern char *buffer;
/**********************************************************************
 *
 * CVDecompress()
 *
 * Decompress incoming frame to holding DIB
 *
 * Returns nonzero if no error
 *
 **********************************************************************/

int CVDecompress(
  DCONTEXT *pD,			// decompression context
  FRAME huge *pFrame,		// frame to be decompressed
  unsigned long oBits,		// 32 bit offset of base of output DIB
#if	!defined(WIN32)
  unsigned short sBits,		// selector to output DIB
#endif
  long YStep			// bytes from (x,y) to (x,y+1) in holding DIB
) {
	auto unsigned char FrameType;	// key or inter
	auto unsigned long sFrameLeft;// bytes left in frame
	
	auto TILE huge *pTile;	// -> for traversing tiles
	auto unsigned long bTile;	// flat base addr of current tile
	auto unsigned long sTile;	// size of the tile
	
	auto int nTiles;		// count of tiles to traverse
	auto int t;			// stepping through tiles
	
	YStep = (YStep+31)&~31;
	if (pD->pInitialCodeBooks)
	{	// first frame after begin with colors?
		auto FRAME far *pInitialCodeBooks;

		/*
		  We're doing the first frame after a CVDecompressEnd and
		  CVDecompressBegin where we are probably changing the format of the
		  DIB.
		
		  We snaggled the content of the codebooks into a legal Cinepak
		  frame in EndingCodeBooksFromContext, called from CVDecompressEnd,
		  by inverse color transforming.
		
		  We remembered the InitialCodeBooks frame in pD->pInitialCodeBooks
		  in CVDecompressBegin.
		
		  Now we use the frame to recurse to build the new codebooks, then
		  free the frame.
		 */
		pInitialCodeBooks = pD->pInitialCodeBooks;
		pD->pInitialCodeBooks = 0;
		CVDecompress(
			pD,			// decompression context
			pInitialCodeBooks,	// encoded initial codebooks
			oBits,			// unused 32 bit offset of base of output DIB
#if	!defined(WIN32)
			sBits,			// selector to output DIB
#endif
			YStep			// bytes from (x,y) to (x,y+1) in holding DIB
		);
		GlobalFreePtr(pInitialCodeBooks);
	}

#if	!defined(NOASM)
	{
		auto long FixupSws;		// bit switches for fixups
		
		FixupSws = 0;			// assume no fixups needed
		
		if ((pD->FixupSws & (bYScan0 | bUScan0 | bUScan1 | bVScan0 | bVScan1)) &&
			(oBits != pD->Actual.vFixup[iYScan0])
		)
		{
			auto long dY;
			
			dY = (pD->Desired.vFixup[iYScan0] = oBits) - pD->Actual.vFixup[iYScan0];
			
			pD->Desired.vFixup[iUScan0] += dY;
			pD->Desired.vFixup[iUScan1] += dY;
			pD->Desired.vFixup[iVScan0] += dY;
			pD->Desired.vFixup[iVScan1] += dY;
			
			FixupSws |= bYScan0 | bUScan0 | bUScan1 | bVScan0 | bVScan1;
		}

		/*
		  Fix up references to YStep and its multiples if it's different
		 */
		if (YStep != (long) pD->Actual.vFixup[iYStep1])
		{
			
			pD->Desired.vFixup[iYStep7] = YStep + (
				pD->Desired.vFixup[iYStep6] = YStep + (
					pD->Desired.vFixup[iYStep5] = YStep + (
						pD->Desired.vFixup[iYStep4] = YStep + (
							pD->Desired.vFixup[iYStep3] = YStep + (
								pD->Desired.vFixup[iYStep2] = YStep + (
									pD->Desired.vFixup[iYStep1] = YStep
								)
							)
						)
					)
				)
			);
			pD->Desired.vFixup[iYDelta] = pD->Actual.vFixup[iYDelta] + (
				(pD->Desired.vFixup[iYStep1] - pD->Actual.vFixup[iYStep1]) << (pD->Scale + 1));
			FixupSws |= bYDelta |
				bYStep1 | bYStep2 | bYStep3 | bYStep4 | bYStep5 | bYStep6 | bYStep7;
		}
		if (FixupSws)
		{
			DecompressFixup(pD, pD->FixupSws & FixupSws);
		}
	}
#endif

#if	!defined(WIN32)
	//
	// fix the incoming frame in linear memory
	//
	GlobalFix((HGLOBAL)GlobalHandle(SELECTOROF(pFrame)));
#endif
	FrameType = ByteInHugeStruct(pFrame, FRAME, SizeType.sType);
	sFrameLeft = SwizSizeInHugeStruct(pFrame, FRAME, SizeType.SwizSize)- sizeof(FRAME)+ sizeof(TILE);
	nTiles = SwizWordInHugeStruct(pFrame, FRAME, SwizNumTiles);
	
	// for each tile, advancing oBits
	for (
		t = 0,
		pTile = (TILE huge *) AddrInHugeStruct(pFrame, FRAME, Tile),
#if	!defined(WIN32)
		bTile = GetSelectorBase(SELECTOROF(pTile))+(unsigned short)(unsigned long)pTile;
#else
		bTile = (unsigned long) pTile;
#endif
		(t < nTiles) && (sFrameLeft >= sizeof(SIZETYPE)) &&
		(sFrameLeft >= (sTile = SwizSizeInHugeStruct(pTile, TILE, SizeType.SwizSize)));
		bTile += sTile,pTile = (TILE huge *) (((char huge *) pTile) + sTile),sFrameLeft-=sTile)
	{
		
		auto unsigned char TileType;// key or inter
		
		TileType = ByteInHugeStruct(pTile, TILE, SizeType.sType);
		if ((TileType == kKeyTileType) || (TileType == kInterTileType)) {
			auto unsigned long sTileLeft;// bytes left in tile
			
			auto short Height;	// height of tile
			
			auto CODEBOOK huge *pCode;// -> incoming compressed codebooks
			auto unsigned long bCode;	// flat base addr of compressed codebooks
			auto unsigned long sCode;	// size of compressed codebooks

			sTileLeft = sTile - sizeof(TILE) + sizeof(CODEBOOK);
			
			/*
			Height here is native height since it comes from the original
			movie.
			*/
			Height = (SwizWordInHugeStruct(pTile, TILE, SwizRect.SwizBottom)-SwizWordInHugeStruct(pTile, TILE, SwizRect.SwizTop))*pD->Scale;
			/*
			On a key frame, the first tile has a new codebook.  Tiles below
			the first may diff from the one above.
			
			On inter frames, the codebooks for a tile may diff from the
			corresponding tile in the previous frame.
			*/
			if (t && (FrameType == kKeyFrameType) && (TileType == kInterTileType)) {
				pD->pCodeBook[t] = pD->pCodeBook[t-1];
			}
			
			// point at first atom in tile
			pCode = (CODEBOOK huge *) AddrInHugeStruct(pTile, TILE, Book);
			bCode = bTile + offsetof(TILE, Book);
			
#if	defined(NOASM)
			pD->pThisCodeBook = t + (CODEBOOKS far *) pD->oCodeBook;
			pD->YStep = YStep;
#else
			
			/*
			Fix up refs to codebook table
			*/
			pD->Desired.vFixup[iCodebook] = pD->oCodeBook + (t * sizeof(CODEBOOKS));
			printf("fixup = %08x %08x\n",pD->Desired.vFixup[iCodebook],pD->Actual.vFixup[iCodebook]);
			if (pD->Desired.vFixup[iCodebook] != pD->Actual.vFixup[iCodebook]) {
				DecompressFixup(pD, bCodebook & pD->FixupSws);
			}
#endif
			for (;			// for each atom in tile
				(sTileLeft >= sizeof(SIZETYPE)) &&
				(sTileLeft >= (sCode = SwizSizeInHugeStruct(pCode, CODEBOOK, SizeType.SwizSize)));
				pCode = (CODEBOOK huge *) (((char huge *) pCode) + sCode),bCode+=sCode,sTileLeft-=sCode)
			{
				auto unsigned char CodeType;// SizeType.sType of atom in tile
				
				CodeType = ByteInHugeStruct(pCode, CODEBOOK, SizeType.sType);
				/*
				We are looking at an atom in a tile in a frame.
				
				pCode points at the atom
				bCode is the flat base addr of the atom
				sCode is its size
				CodeType is its SizeType.sType
				
				We do the right thing for each atom, based on CodeType.
				*/

				switch (CodeType) {
				
					/*
					Detail CodeBook
					*/
					case kFullDBookType:
					case kFullDGreyBookType:
					case kPartialDBookType:
					case kPartialDGreyBookType: {
#if 0
bigtime_t t=system_time();
int32 z=0;
#endif
#if defined(__BEOS__) && !defined(NOASM)
							pD->Actual.vFixup[iCodebook]=pD->Desired.vFixup[iCodebook];	// -> where to build
#endif
#if 0
for (z=0;z<100000;z++)
#endif
	//				printf("expand, output cdebook=%08x, buffer=%08x\n",pD->Actual.vFixup[iCodebook],buffer);
	//				printf("expand=%08x, %08x\n",pD->Dispatch.pExpandDetailCodeBook,ExpandCodeBook32);
						(*pD->Dispatch.pExpandDetailCodeBook)(
#if	!defined(NOASM)
							bCode,				// -> input codebook
							pD->Actual.vFixup[iCodebook]	// -> where to build
#else
							(CODEBOOK *) bCode,		// -> input codebook
							pD->pThisCodeBook->CodeBook,	// -> where to build
							pD->pRgbLookup			// -> current RgbLookup
#endif
							);
if (0) {
	FILE *FOut=fopen("/tmp/book1.raw","wb");
	fwrite((void*)pD->pThisCodeBook,1,256*8,FOut);
	fclose(FOut);
	exit(0);
}
#if 0
t=system_time()-t;
printf("time=%g\n",t/1000000.0);
#endif
						//	printf("expanded\n");
#if 0
						if (0) //CodeType & kPartialBookBit)
						{
							int z;
							DENTRY *d=pD->Desired.vFixup[iCodebook];
							for (z=0;z<256;z++)
							{
								printf("%08x %08x %08x %08x\n",d[z].Pixel0,d[z].Pixel1,d[z].Pixel2,d[z].Pixel3);
							}
						}
#endif
						break;
					}
					
					/*
					Smooth CodeBook
					*/
					case kFullSBookType:
					case kFullSGreyBookType:
					case kPartialSBookType:
					case kPartialSGreyBookType: {
//memset(pD->pThisCodeBook->CodeBook+1,0,256*4);
						(*pD->Dispatch.pExpandSmoothCodeBook)(
#if	!defined(NOASM)
							bCode,				// -> 2nd input codebook
							pD->Actual.vFixup[iCodebook] + sizeof(DCODEBOOK)
#else
							(CODEBOOK *) bCode,		// -> 2nd input codebook
							pD->pThisCodeBook->CodeBook + 1,	// -> where to build
							pD->pRgbLookup			// -> current RgbLookup
#endif
						);
if (0) {
	FILE *FOut=fopen("/tmp/book2.raw","wb");
	fwrite((void*)pD,1,256*8,FOut);
	fclose(FOut);
	exit(0);
}
						break;
					}
					
					/*
					Key Tile codes
					*/
					case kIntraCodesType: {
//					printf("draw intra\n",pD->Actual.vFixup[iCodebook]);
#if 0
bigtime_t t=system_time();
int32 z=0;
for (z=0;z<10000;z++)
#endif
						(*pD->Dispatch.pDrawKey)(
#if	!defined(NOASM)
#if defined(__BEOS__)
							(long)&pD->Actual.vFixup,
#endif
							(bCode + offsetof(INDICES, Index)),
							oBits,		// offset for DIB bits to fill
#if	!defined(WIN32)
							sBits,		// selector for DIB bits to fill
#endif
							Height		// height of tile
#else	// C-ONLY
							pD,		// -> decompress context
							(PBYTE) (bCode + offsetof(INDICES, Index)),
							(PBYTE)oBits,	// offset for DIB bits to fill
							Height		// height of tile
#endif
						);
#if 0
t=system_time()-t;
printf("time=%g\n",t/1000000.0);
#endif
			//			printf("drawed key\n");
						break;
					}

					/*
					Inter Tile codes
					*/
					case kInterCodesType: {	// inter tile
					//printf("draw inter\n");
						(*pD->Dispatch.pDrawInter)(
#if	!defined(NOASM)
#if defined(__BEOS__)
							(long)&pD->Actual.vFixup,
#endif
							(bCode + offsetof(INDICES, Index)),
							oBits,		// offset for DIB bits to fill
#if	!defined(WIN32)
							sBits,		// selector for DIB bits to fill
#endif
							Height		// height of tile
#else	// C-ONLY
							pD,		// -> decompress context
							(PBYTE) (bCode + offsetof(INDICES, Index)),
							(PBYTE)oBits,	// offset for DIB bits to fill
							Height		// height of tile
#endif
						);
						break;
					}
					
					/*
					All Smooth Key Tile codes
					*/
					case kAllSmoothCodesType: {// all smooth key tile
						(*pD->Dispatch.pDrawSmooth)(
#if	!defined(NOASM)
#if defined(__BEOS__)
							(long)&pD->Actual.vFixup,
#endif
							(bCode + offsetof(INDICES, Index)),
							oBits,		// offset for DIB bits to fill
#if	!defined(WIN32)
							sBits,		// selector for DIB bits to fill
#endif
							Height		// height of tile
#else	// C-ONLY
							pD,		// -> decompress context
							(PBYTE) (bCode + offsetof(INDICES, Index)),
							(PBYTE)oBits,	// offset for DIB bits to fill
							Height		// height of tile
#endif
						);
						break;
					}
				}
			}
			/*
			We've processed all the atoms in the tile
			*/
			oBits += YStep * (long) Height;
			t++;			// bump if it's a tile
		}
	}

#if	!defined(WIN32)
	GlobalUnfix((HGLOBAL)GlobalHandle(SELECTOROF(pFrame)));
#endif

	return(1);
}



/**********************************************************************
 *
 * Dispatch Table
 *
 * Pointers for the various depths
 *
 **********************************************************************/

DISPATCH Dispatch[] = {
  {				// 8 bpp
#if	!defined(NOASM)
    {
      &Expand8Base,		// Expand8 base
      &Expand8Table		// Expand8 fixup table
    },
    {
      &Draw8Base,			// Draw8 base
      &Draw8Table		// Draw8 fixup table
    },
#endif
    ExpandDetailCodeBook8,
    ExpandSmoothCodeBook8,
    DrawKey8,
    DrawSmooth8,
    DrawInter8,
    DetailCodeFromDither8,
    SmoothCodeFromDither8
  },
  {				// 15 bpp:  RGB555
#if	!defined(NOASM)
    {
      &Expand15Base,
      &Expand15Table
    },
    {
      &Draw15Base,
      &Draw15Table
    },
#endif
    ExpandDetailCodeBook15,
    ExpandSmoothCodeBook15,
    DrawKey15,
    DrawSmooth15,
    DrawInter15,
    DetailCodeFromRGB555,
    SmoothCodeFromRGB555
  },
  {				// 16 bpp:  RGB565
#if	!defined(NOASM)
    {
      &Expand16Base,
      &Expand16Table
    },
    {
      &Draw15Base,		// Draw16 just uses Draw15
      &Draw15Table
    },
#endif
    ExpandDetailCodeBook16,
    ExpandSmoothCodeBook16,
    DrawKey15,
    DrawSmooth15,
    DrawInter15,
    DetailCodeFromRGB565,
    SmoothCodeFromRGB565
  },
  {				// 24 bpp
#if	!defined(NOASM)
    {
      &Expand24Base,
      &Expand24Table
    },
    {
      &Draw24Base,
      &Draw24Table
    },
#endif
    ExpandDetailCodeBook24,
    ExpandSmoothCodeBook24,
    DrawKey24,
    DrawSmooth24,
    DrawInter24,
    DetailCodeFromRGB888,
    SmoothCodeFromRGB888
  },
  {				// 32 bpp
#if	!defined(NOASM)
    {
      &Expand32Base,
      &Expand32Table
    },
    {
      &Draw32Base,
      &Draw32Table
    },
#endif
    ExpandCodeBook32,
    ExpandCodeBook32,
    DrawKey32,
    DrawSmooth32,
    DrawInter32,
    DetailCodeFromRGBa8888,
    SmoothCodeFromRGBa8888
  },
#if	(DIBS & DIBS_CPLA)
  {				// Weitek YUV420
#if	!defined(NOASM)
    {
      &ExpandCPLABase,
      &ExpandCPLATable
    },
    {
      &DrawCPLABase,
      &DrawCPLATable
    },
#endif
    ExpandDetailCodeBookCPLA,
    ExpandSmoothCodeBookCPLA,
    DrawKeyCPLA,
    DrawSmoothCPLA,
    DrawInterCPLA,
    DetailCodeFromCPLA,
    SmoothCodeFromCPLA
  },
#else	// have to put in empty values so indexing works right
	{ NULL,NULL,NULL,NULL,NULL,NULL,NULL},
#endif
#if	(DIBS & DIBS_YUY2)
  {				// various YUV422
#if	!defined(NOASM)
    {
      &ExpandYUY2Base,
      &ExpandYUY2Table
    },
    {
      &Draw15Base,		// YUY2 just uses Draw15
      &Draw15Table
    },
#endif
    ExpandDetailCodeBookYUY2,
    ExpandSmoothCodeBookYUY2,
    DrawKey15,
    DrawSmooth15,
    DrawInter15,
    DetailCodeFromYUY2,
    SmoothCodeFromYUY2
  },
#endif
#if	(DIBS & DIBS_UYVY)
  {				// Cirrus Logic 5440 YUV422
#if	!defined(NOASM)
    {
      &ExpandUYVYBase,
      &ExpandUYVYTable
    },
    {
      &Draw15Base,		// UYVY just uses Draw15
      &Draw15Table
    },
#endif
    ExpandDetailCodeBookUYVY,
    ExpandSmoothCodeBookUYVY,
    DrawKey15,
    DrawSmooth15,
    DrawInter15,
    DetailCodeFromUYVY,
    SmoothCodeFromUYVY
  },
#endif
#if	(DIBS & DIBS_CLJR)
  {				// Cirrus Logic AcuPak
#if	!defined(NOASM)
    {
      &ExpandCLJRBase,
      &ExpandCLJRTable
    },
    {
      &DrawCLJRBase,
      &DrawCLJRTable
    },
#endif
    ExpandDetailCodeBookCLJR,
    ExpandSmoothCodeBookCLJR,
    DrawKeyCLJR,
    DrawSmoothCLJR,
    DrawInterCLJR,
    DetailCodeFromCLJR,
    SmoothCodeFromCLJR
  },
#endif
  {				// 8 bpp, doubled
#if	!defined(NOASM)
    {
      &Expand8DoubledBase,
      &Expand8DoubledTable
    },
    {
      &Draw8DoubledBase,
      &Draw8DoubledTable
    },
#endif
    ExpandDetailCodeBook8Doubled,
    ExpandSmoothCodeBook8Doubled,
    DrawKey8Doubled,
    DrawSmooth8Doubled,
    DrawInter8Doubled,
    DetailCodeFromDither8Doubled,
    SmoothCodeFromDither8Doubled
  }
};


/**********************************************************************
 *
 * CVDecompressBegin()
 *
 * Opening bracket for CompactVideo decompression
 *
 * Returns near ptr to decompression context or 0 if some error
 *
 * Allocate and initialize DCONTEXT and CODEBOOKS.
 *
 **********************************************************************/

DCONTEXT *CVDecompressBegin(
  short Width,			// 0 mod 4 width (pixels) of holding DIB
  short Height,			// 0 mod 4 height (scanlines) of holding DIB
  DIBTYPE DibType,		// type of source DIB
  short Scale,			// dest:src stretching ratio (integer only)
  FRAME far *pInitialCodeBooks	// from CVDecompressEnd:  starting codebooks
) {
  auto DCONTEXT *pD;		// decompression context

#if	defined(DEBUG)
#if	!defined(WIN32)
	_asm	int	3
#else
	DebugBreak();
#endif
#endif

	//
	//  allocate decompression context and initialize its fields...
	//
	if (pD = (DCONTEXT *) LocalAlloc(LPTR, sizeof(DCONTEXT)))
	{
		unsigned long WidthInBytes;
		// printf("DibType=%d\n",DibType);
		switch (DibType)
		{
			case Dither8:
			{
				WidthInBytes = Width;
				if (Scale != 2) {
					pD->Dispatch = Dispatch[Dither8];
				} else {
					/*
					We are pixel doubling.  Width and Height here are the
					doubled width and height, not the native.
					*/
					pD->Dispatch = Dispatch[(sizeof(Dispatch)/sizeof(Dispatch[0]))-1];
				}
				break;
			}
			case RGB555: {
				pD->Dispatch = Dispatch[RGB555];
				WidthInBytes = Width * 2;
				break;
			}
			case RGB565: {
				pD->Dispatch = Dispatch[RGB565];
				WidthInBytes = Width * 2;
				break;
			}
			case RGB888: {
				pD->Dispatch = Dispatch[RGB888];
				WidthInBytes = Width * 3;
				break;
			}
			case RGBa8888: {
				pD->Dispatch = Dispatch[RGBa8888];
				WidthInBytes = Width * 4;
				break;
			}
#if	(DIBS & DIBS_CPLA)
			case CPLA: {
				pD->Dispatch = Dispatch[CPLA];
				WidthInBytes = Width;	// Y part only
				break;
			}
#endif
#if	(DIBS & DIBS_YUY2)
			case YUY2: {
			printf("doing yuy2!\n");
				pD->Dispatch = Dispatch[YUY2];
				WidthInBytes = Width * 2;
				break;
			}
#endif
#if	(DIBS & DIBS_YUY2)
			case UYVY: {
				pD->Dispatch = Dispatch[UYVY];
				WidthInBytes = Width * 2;
				break;
			}
#endif
#if	(DIBS & DIBS_CLJR)
			case CLJR: {
				pD->Dispatch = Dispatch[CLJR];
				WidthInBytes = Width;
				break;
			}
#endif
		}
		if (			// alloc space for Codebook and code
#if	defined(NOASM)
			pD->pCodeBook = (CODEBOOKS far *) GlobalAllocPtr(
				GHND,
				(sizeof(CODEBOOKS) * kMaxTileCount)
				+ ((DibType == Dither8) ? nRgbLookup : 0))
#else
			pD->pCodeBook = (CODEBOOKS far *) GlobalAllocPtr(
				GHND,
				(sizeof(CODEBOOKS) * kMaxTileCount)
				+ ((pD->Dispatch.Expand.pTable->nBytes + 3) & ~3)
				+ pD->Dispatch.Draw.pTable->nBytes)
#endif
			)
		{
#if	!defined(WIN32)
			unsigned short CSel;
			
			GlobalFix((HGLOBAL)GlobalHandle(SELECTOROF(pD->pCodeBook)));
			if (CSel = AllocSelector(NULL))
			{
			
				if (CSel = PrestoChangoSelector(SELECTOROF(pD->pCodeBook), CSel))
				{
					auto int i;
					auto short ExpandOffset;// offset newBase - offset Base
					auto short DrawOffset;
					
					// fix the TEXT32 segment in linear memory
					GlobalFix((HGLOBAL)GlobalHandle(SELECTOROF(ExpandDetailCodeBook8)));
					
					ExpandOffset = (short)
						(((unsigned long) &pD->pCodeBook[kMaxTileCount]) -
						 ((unsigned long) pD->Dispatch.Expand.pBase));
					DrawOffset = (short)
						(((unsigned long) &pD->pCodeBook[kMaxTileCount]) +
						 ((pD->Dispatch.Expand.pTable->nBytes + 3) & ~3) -
						 (unsigned long) pD->Dispatch.Draw.pBase);
					{
						char desc[8];
						
						// transform the USE16 code descriptor into USE32
						
						_asm {
						push	es
						push	di
						mov	bx,CSel		// bx  sel for which to get descriptor
						mov	ax,ss
						mov	es,ax
						lea	di,desc		// es:di  where to put descriptor
						mov	ax,0bh		// DPMI: get descriptor
						int	031h		// get descriptor
						or	es:[di][6],040h	// make USE32
						mov	ax,0ch		// DPMI: set descriptor
						int	031h		// set descriptor
						pop	di
						pop	es
						}
					}
					pD->oCodeBook = GetSelectorBase(CSel);
					
					pD->Actual = InitialFixup;
					
					pD->Width = Width;
					pD->Height = Height;
					pD->Scale = Scale;
					
					BigCopy(		// copy Expand code to codebook area
						pD->Dispatch.Expand.pBase,
						&pD->pCodeBook[kMaxTileCount],
						pD->Dispatch.Expand.pTable->nBytes);
					BigCopy(		// copy Draw code to codebook area
						pD->Dispatch.Draw.pBase,
						((char far *) &pD->pCodeBook[kMaxTileCount]) +
						((pD->Dispatch.Expand.pTable->nBytes + 3) & ~3),
						pD->Dispatch.Draw.pTable->nBytes);
					pD->Dispatch.Expand.pBase = (char far *) MAKELP(
						SELECTOROF(pD->pCodeBook),
						OFFSETOF(pD->Dispatch.Expand.pBase) + ExpandOffset);
					pD->Dispatch.Draw.pBase = (char far *) MAKELP(
						SELECTOROF(pD->pCodeBook),
						OFFSETOF(pD->Dispatch.Draw.pBase) + DrawOffset);
					pD->Dispatch.pExpandDetailCodeBook = (PEXPANDCODEBOOK) MAKELP(
						CSel,OFFSETOF(pD->Dispatch.pExpandDetailCodeBook) + ExpandOffset);
					pD->Dispatch.pExpandSmoothCodeBook = (PEXPANDCODEBOOK) MAKELP(
						CSel,OFFSETOF(pD->Dispatch.pExpandSmoothCodeBook) + ExpandOffset);
					pD->Dispatch.pDrawKey = (PDRAWFRAME) MAKELP(
						CSel,OFFSETOF(pD->Dispatch.pDrawKey) + DrawOffset);
					pD->Dispatch.pDrawSmooth = (PDRAWFRAME) MAKELP(
						CSel,OFFSETOF(pD->Dispatch.pDrawSmooth) + DrawOffset);
					pD->Dispatch.pDrawInter = (PDRAWFRAME) MAKELP(
						CSel,OFFSETOF(pD->Dispatch.pDrawInter) + DrawOffset);
			
					/*
					pD->pCodeBook is a data pointer to
					kMaxTileCount codebooks, consisting of
					a detail codebook and
					a smooth codebook
					a copy of the Expand code, pD->Expand.pBase as data
					a copy of the Draw code, pD->Draw.pBase as data
					pD->oCodeBook is a flat offset to the same area
					procedure pointers in pD->Dispatch have use32 code selectors
					*/
			
					/*
					We must fix up references in the copied code.
					*/
					
					/*
					pD->FixupSws get a 1 in bit positions that have non zero
					nRefs
					*/
					for (
						i = nFixups,pD->FixupSws = 0;
						i--;
						pD->FixupSws <<= 1,pD->FixupSws |=pD->Dispatch.Expand.pTable->nRefs[i] || pD->Dispatch.Draw.pTable->nRefs[i]
					);
					
					/*
					First we fix up Motion refs in Draw to be Expand-relative so
					that we can subsequently treat Expand and Draw identically.
					*/
					{
						auto unsigned short nRefSave;
						
						nRefSave = pD->Dispatch.Expand.pTable->nRefs[iMotion];
						pD->Dispatch.Expand.pTable->nRefs[iMotion] = 0;
						
						pD->Desired.vFixup[iMotion] = pD->Actual.vFixup[iMotion];
						pD->Actual.vFixup[iMotion] -= DrawOffset - ExpandOffset;
						
						DecompressFixup(pD, bMotion);
						
						pD->Dispatch.Expand.pTable->nRefs[iMotion] = nRefSave;
					}
			
					/*
					YStep references will get fixed up in CVDecompress.
					YDelta starts at -WidthInBytes.
					*/
					pD->Desired.vFixup[iYStep1] =
					pD->Desired.vFixup[iYStep2] =
					pD->Desired.vFixup[iYStep3] =
					pD->Desired.vFixup[iYStep4] =
					pD->Desired.vFixup[iYStep5] =
					pD->Desired.vFixup[iYStep6] =
					pD->Desired.vFixup[iYStep7] = 0;
					pD->Desired.vFixup[iYDelta] = - (long) WidthInBytes;
					pD->Desired.vFixup[iMotion] = pD->oCodeBook + ExpandOffset;
					pD->Desired.vFixup[iWidth4] = (long) (Width / 4);
					pD->Desired.vFixup[iWidth8] = (long) (Width / 8);
					pD->Desired.vFixup[iTEXT32] =
					GetSelectorBase(SELECTOROF(ExpandDetailCodeBook8));
					pD->Desired.vFixup[iYScan0] = 0;
					pD->Desired.vFixup[iUScan0] = ((long) Width) * ((long) Height);
					pD->Desired.vFixup[iUScan1] = 
					pD->Desired.vFixup[iUScan0] + (long) (Width >> 1);
					pD->Desired.vFixup[iVScan0] =
					pD->Desired.vFixup[iUScan0] + (pD->Desired.vFixup[iUScan0] >> 2);
					pD->Desired.vFixup[iVScan1] = 
					pD->Desired.vFixup[iVScan0] + (long) (Width >> 1);
					
					DecompressFixup(pD, pD->FixupSws);
					
					pD->pInitialCodeBooks = pInitialCodeBooks;// remember initial colors
					
					return(pD);
				}
				// PrestoChangoSelector failed
				SetSelectorBase(CSel,0L);
				SetSelectorLimit(CSel,0L);
				FreeSelector(CSel);
			}
			// AllocSelector failed
			GlobalUnfix((HGLOBAL)GlobalHandle(SELECTOROF(pD->pCodeBook)));
			GlobalFreePtr(pD->pCodeBook);
			
#else	/* WIN32 */

#if	!defined(NOASM)
			auto int i;
			auto long ExpandOffset;	// offset newBase - offset Base
			auto long DrawOffset;
#endif
			
			pD->oCodeBook = (unsigned long) pD->pCodeBook;
			
			pD->Width = Width;
			pD->Height = Height;
			pD->Scale = Scale;
			
#if	defined(NOASM)
			
			pD->pRgbLookup = (unsigned char far *) &pD->pCodeBook[kMaxTileCount];
	
#else
			
			pD->Actual = InitialFixup;
			
			ExpandOffset = (long)
				(((unsigned long) &pD->pCodeBook[kMaxTileCount]) - 
				 ((unsigned long) pD->Dispatch.Expand.pBase));
			DrawOffset = (long)
				(((unsigned long) &pD->pCodeBook[kMaxTileCount]) + 
				 ((pD->Dispatch.Expand.pTable->nBytes + 3) & ~3) -
				 (unsigned long) pD->Dispatch.Draw.pBase);
			
			BigCopy(		// copy Expand code to codebook area
				pD->Dispatch.Expand.pBase,
				&pD->pCodeBook[kMaxTileCount],
				pD->Dispatch.Expand.pTable->nBytes);
			BigCopy(		// copy Draw code to codebook area
				pD->Dispatch.Draw.pBase,
				((char far *) &pD->pCodeBook[kMaxTileCount]) +
				((pD->Dispatch.Expand.pTable->nBytes + 3) & ~3),
				pD->Dispatch.Draw.pTable->nBytes);
			
			pD->Dispatch.Expand.pBase += ExpandOffset;
			pD->Dispatch.Draw.pBase += DrawOffset;
			
#ifndef __BEOS__	// this is just plain wierd...
			pD->Dispatch.pExpandDetailCodeBook = (PEXPANDCODEBOOK)(ExpandOffset + (unsigned long) pD->Dispatch.pExpandDetailCodeBook);
			pD->Dispatch.pExpandSmoothCodeBook = (PEXPANDCODEBOOK)(ExpandOffset + (unsigned long) pD->Dispatch.pExpandSmoothCodeBook);
			pD->Dispatch.pDrawKey = (PDRAWFRAME)(DrawOffset + (unsigned long) pD->Dispatch.pDrawKey);
			pD->Dispatch.pDrawSmooth = (PDRAWFRAME)(DrawOffset + (unsigned long) pD->Dispatch.pDrawSmooth);
			pD->Dispatch.pDrawInter = (PDRAWFRAME)(DrawOffset + (unsigned long) pD->Dispatch.pDrawInter);
#endif			
			/*
			pD->pCodeBook is a data pointer to
			kMaxTileCount codebooks, consisting of
			a detail codebook and
			a smooth codebook
			a copy of the Expand code, pD->Expand.pBase as data
			a copy of the Draw code, pD->Draw.pBase as data
			procedure pointers in pD->Dispatch have use32 code selectors
			*/
			
			/*
			We must fix up references in the copied code.
			*/
			
			/*
			pD->FixupSws get a 1 in bit positions that have non zero
			nRefs
			*/
			for (
				i = nFixups,pD->FixupSws = 0;
				i--;
				pD->FixupSws <<= 1,pD->FixupSws |=pD->Dispatch.Expand.pTable->nRefs[i] || pD->Dispatch.Draw.pTable->nRefs[i]
			);// printf("fixup %d=%08x\n",i,pD->Dispatch.Expand.pTable->nRefs[i]);
			//printf("pD->FixupSws=%08x\n",pD->FixupSws);
			pD->FixupSws &= ~bTEXT32;// never fix up since already ok
			
			/*
			First we fix up Motion refs in Draw to be Expand-relative so
			that we can subsequently treat Expand and Draw identically.
			*/
			{
				auto unsigned short nRefSave;
				
				nRefSave = pD->Dispatch.Expand.pTable->nRefs[iMotion];
				pD->Dispatch.Expand.pTable->nRefs[iMotion] = 0;
				
				pD->Desired.vFixup[iMotion] = pD->Actual.vFixup[iMotion];
				pD->Actual.vFixup[iMotion] -= DrawOffset - ExpandOffset;
				
				DecompressFixup(pD, bMotion);
				
				pD->Dispatch.Expand.pTable->nRefs[iMotion] = nRefSave;
			}
			
			/*
			YStep references will get fixed up in CVDecompress.
			YDelta starts at -WidthInBytes.
			*/
			pD->Desired.vFixup[iYStep1] =
			pD->Desired.vFixup[iYStep2] =
			pD->Desired.vFixup[iYStep3] =
			pD->Desired.vFixup[iYStep4] =
			pD->Desired.vFixup[iYStep5] =
			pD->Desired.vFixup[iYStep6] =
			pD->Desired.vFixup[iYStep7] = 0;
			pD->Desired.vFixup[iYDelta] = - (long) WidthInBytes;
			pD->Desired.vFixup[iMotion] = ExpandOffset;
			pD->Desired.vFixup[iWidth4] = (long) (Width / 4);
			pD->Desired.vFixup[iWidth8] = (long) (Width / 8);
			pD->Desired.vFixup[iTEXT32] = pD->Actual.vFixup[iTEXT32];
			pD->Desired.vFixup[iYScan0] = 0;
			pD->Desired.vFixup[iUScan0] = ((long) Width) * ((long) Height);
			pD->Desired.vFixup[iUScan1] = 
			pD->Desired.vFixup[iUScan0] + (Width >> 1);
			pD->Desired.vFixup[iVScan0] =
			pD->Desired.vFixup[iUScan0] + (pD->Desired.vFixup[iUScan0] >> 2);
			pD->Desired.vFixup[iVScan1] = 
			pD->Desired.vFixup[iVScan0] + (Width >> 1);
			
			DecompressFixup(pD, pD->FixupSws);
			
#endif		/* !NOASM */
			
			pD->pInitialCodeBooks = pInitialCodeBooks;// remember initial colors
			
			return(pD);
#endif		/* !WIN32 */
		}
		// GlobalAlloc failed
		LocalFree((HLOCAL) pD);
	}
	return((DCONTEXT *) 0);
}


extern unsigned int Squares [];


unsigned int FindNearestRGB (
  LPRGBQUAD rgbqList,		// rgb list to search
  unsigned int nColors,		// number of entries in rgbq
  RGBQUAD rgbqSrc		// rgb to match
) {
  unsigned int i;
  long currError;
  long minError = 0x7fffffff;
  unsigned int minIndex = 0;

  for (i = 0; i < nColors && minError; i++, rgbqList++)
  {
    if (  // what is this, lisp?
      (
	(
	  currError = minError - 
	      (long)(DWORD)Squares [255 + rgbqList->rgbRed - rgbqSrc.rgbRed]
	) > 0
      )
      &&
      (
	(
	  currError -= 
	      (long)(DWORD)Squares [255 + rgbqList->rgbGreen - rgbqSrc.rgbGreen]
	) > 0
      )
      && 
      (
	(
	  currError -= 
	      (long)(DWORD)Squares [255 + rgbqList->rgbBlue - rgbqSrc.rgbBlue]
	) > 0
      )
    ) {
      minError -= currError;
      minIndex = i;
    }
  }
  return minIndex;
}


/**********************************************************************
 *
 * CVDecompressSetPalette()
 *
 * Inject a palette translation into the decompression context.
 * If nColors == 0, the standard translation is injected.
 *
 * Returns nonzero if no error
 *
 **********************************************************************/

int CVDecompressSetPalette(		// nz if no error
  DCONTEXT *pD,			// decompression context
  LPRGBQUAD rgbq,		// rgbs to insert into palette
  int nColors			// number of RGBs in palette
) {
  if (nColors) {		// if there is a user palette
    auto int i;
    auto unsigned char far *cDest;

#if	defined(NOASM)
    cDest = pD->pRgbLookup;
#else
    cDest = (unsigned char far *) pD->Dispatch.Expand.pBase;
#endif

    // For each color in RgbLookup, locate the nearest color in the incoming
    // palette and place its index into the live RgbLookup.

    for (i = 0; i < nRgbLookup; i++)
    {
      *cDest++ = FindNearestRGB (
				 rgbq, 
				 nColors, 
				 stdPAL8 [RgbLookup [i] - 10]
				);
    }
  } else {			// no user palette; use the standard one

#if	defined(NOASM)
    BigCopy(RgbLookup, pD->pRgbLookup, nRgbLookup);
#else
    BigCopy(RgbLookup, pD->Dispatch.Expand.pBase, nRgbLookup);
#endif
  }
  return 0;
}


/**********************************************************************
 *
 * CVDecompressEnd()
 *
 * Closing bracket for CompactVideo decompression
 *
 * Returns retained inverse transformed colors if requested.
 *
 * Free DCONTEXT and CODEBOOKS.
 *
 **********************************************************************/

FRAME far *CVDecompressEnd(	// nz if no error
  DCONTEXT *pD,			// decompression context
  BOOL ReturnEndingCodeBooks,	// TRUE if we are to return pEndingCodeBooks
  LPRGBQUAD lpRgbQ		// user palette for pEndingCodeBooks if 8 bit
) {
  auto FRAME far *pEndingCodeBooks
  = ReturnEndingCodeBooks
  ? EndingCodeBooksFromContext(pD, lpRgbQ)// inverse translate codebooks
  : 0;

#if	!defined(WIN32)
  auto unsigned short CSel;
  
  GlobalUnfix((HGLOBAL)GlobalHandle(SELECTOROF(pD->pCodeBook)));

  CSel = SELECTOROF(pD->Dispatch.pDrawKey);

  // free allocated selector for code compilation
  GlobalUnfix((HGLOBAL)GlobalHandle(SELECTOROF(ExpandDetailCodeBook8)));
  SetSelectorBase(CSel,0L);
  SetSelectorLimit(CSel,0L);
  FreeSelector(CSel);
#endif

  GlobalFreePtr(pD->pCodeBook);	//!!!!!!!!!!!!!!!!!!!!!!

  LocalFree((HLOCAL) pD);	// free decompression context
  return(pEndingCodeBooks);
}


#if	!defined(NOASM)

/**********************************************************************
 *
 * DecompressFixup()
 *
 * Fix up copied code pieces in CodeBook area
 *
 **********************************************************************/

void DecompressFixup(
  DCONTEXT *pD,			// decompress context
  unsigned long BitSw		// bitswitches of items to fix
)
{
	auto IFIXUP iFixup;
	auto unsigned short *pERef, *pDRef;// expand and draw ref lists
	
	for (				// for each fixup type
		iFixup = 0,
		pERef = pD->Dispatch.Expand.pTable->oRef,
		pDRef = pD->Dispatch.Draw.pTable->oRef;
		BitSw;
		iFixup++,
		BitSw >>= 1
		)
	{
		
		if (BitSw & 1)
		{
			auto int nOffset;		// for counting down refs
			auto long Delta;		// delta from previous value
			
			Delta = pD->Desired.vFixup[iFixup] - pD->Actual.vFixup[iFixup];
			pD->Actual.vFixup[iFixup] = pD->Desired.vFixup[iFixup];
			
			for (nOffset = pD->Dispatch.Expand.pTable->nRefs[iFixup]; nOffset--;)
			{
				*(long far *) (&pD->Dispatch.Expand.pBase[*pERef++]) += Delta;
			}
			
			for (nOffset = pD->Dispatch.Draw.pTable->nRefs[iFixup]; nOffset--;)
			{
				*(long far *) (&pD->Dispatch.Draw.pBase[*pDRef++]) += Delta;
			}
			
		} else {
		
		// skip fixup on this type
		pERef += pD->Dispatch.Expand.pTable->nRefs[iFixup];
		pDRef += pD->Dispatch.Draw.pTable->nRefs[iFixup];
		}
	}
}
#endif
