/************************************************************************
*                                                                       *
*               INTEL CORPORATION PROPRIETARY INFORMATION               *
*                                                                       *
*    This listing is supplied under the terms of a license agreement    *
*      with INTEL Corporation and may not be copied nor disclosed       *
*        except in accordance with the terms of that agreement.         *
*                                                                       *
*************************************************************************
*                                                                       *
*               Copyright (C) 1994-1997 Intel Corp.                       *
*                         All Rights Reserved.                          *
*                                                                       *
************************************************************************/

/* This file contains the transparency band lossless compression code */

#include <setjmp.h>

#include "datatype.h"
#include "tksys.h"
#include "errhand.h"

#include "mc.h"			/* for ivi5bs.h */
#include "hufftbls.h"
#include "qnttbls.h"

#include "ivi5bs.h"		/* for bsutil.h */
#include "mbhuftbl.h"
#include "bkhuftbl.h"

#ifdef SIMULATOR
#include "encsmdef.h"
#include "decsmdef.h"
#endif
#include "pia_main.h"
#include "bsutil.h"
#include "indeo5.h"
#include "ensyntax.h"
#include "const.h"
#include "common.h"
#include "entrans.h"

/*
 * entrans.c
 *
 * Encode the transparency plane
 *
 * Functions:
 *	EncTrans		Perform the encode of the transparency plane
 * 	EncTransOpen	Allocate and initialize the transparency plane
 *  EncTransClose	Free the resources used
 *
 */


static void BuildTransparencyBytemask(PEncTransCntxSt pTransContext,
 								 	  PU8 pu8TransBitMask, PRectSt prDirtyRect);

static void RLECompressTile(PTransTileSt pTileInfo, 
							PEncTransCntxSt pTransContext, jmp_buf jbEnv);

static void RLE_XORCompressTile(PTransTileSt pTileInfo, 
							    PEncTransCntxSt pTransContext, jmp_buf jbEnv);

extern const U32 uMaskarray[32];  /* This comes from convert.c */

/*****************************************************************/
/* Allocate and Initialize the Transparency Band Encoder Context */
/*****************************************************************/

PEncTransCntxSt EncTransOpen(PTransSt *ppTransBand,
							 pPicHdrSt pPicInfoHdr,
							 PEncRtParmsSt pRtParms,
							 jmp_buf jbEnv)
{

	PEncTransCntxSt	pTransContext;  /* Newly alloced Trans Context */
	PTransSt pTransBand; 			/* Local copy of pointer */
	pTransDscrptSt pDescriptor;
	I32 iTileC, iTileR;				/* Indices to loop through tiles */
	PTransTileSt pTileInfo;			/* Pointer to current tile Info struct */
	PTransTileHdrSt pTileInfoHdr;	/* Pointer to current tile Info Header */

	/* Allocate new transparency band context structure */
	pTransContext = (PEncTransCntxSt) SysMalloc(sizeof(EncTransCntxSt), jbEnv);

#ifdef SIMULATOR
	pTransContext->pSimInst = pRtParms->pSimInst;  /* Pointer to simulator Instance */
#endif /* SIMULATOR */

	/* Initialize transparency band encoder context structure */
	pTransContext->TransHeight = pPicInfoHdr->GOPHdr.PictureHeight; 
	pTransContext->TransWidth = ((pPicInfoHdr->GOPHdr.PictureWidth + 31) / 32) * 32 ; 
	pTransContext->TileWidth = ((pPicInfoHdr->GOPHdr.TileWidth + 31) / 32) * 32;
	pTransContext->TileHeight = pPicInfoHdr->GOPHdr.TileHeight;

	pTransContext->OrigWidth = pPicInfoHdr->GOPHdr.PictureWidth;

	/* Round tiles up to nearest whole tile */
	pTransContext->NumTilesWide = 
		((pPicInfoHdr->GOPHdr.PictureWidth + pPicInfoHdr->GOPHdr.TileWidth - 1)
			 / pPicInfoHdr->GOPHdr.TileWidth); 
	pTransContext->NumTilesHigh = 
		((pPicInfoHdr->GOPHdr.PictureHeight + pPicInfoHdr->GOPHdr.TileHeight - 1)
			 / pPicInfoHdr->GOPHdr.TileHeight); 

	/* Allocate an 8bpp Trans ByteMask buffer */
	pTransContext->pu8TransByteMask = (PU8) 
		SysMalloc(pTransContext->NumTilesWide * pTransContext->TileWidth * 
		          pTransContext->NumTilesHigh * pTransContext->TileHeight,
		          jbEnv);

	/* Allocate new transparency band info structure */
	*ppTransBand = (PTransSt) SysMalloc(sizeof(TransSt), jbEnv);
	pTransBand = *ppTransBand;	/* Make local copy of pointer */

	/* Allocate the transparency descriptor structure */
	pDescriptor = pTransBand->pTransDscrptor
		= (pTransDscrptSt) SysMalloc(sizeof(TransDscrptSt), jbEnv);
	pPicInfoHdr->GOPHdr.pTransDscrpt = pDescriptor;

	/* Initialize transparency band info struct */
	pTransBand->TransHdr.ColorPlane = 3;	/* Always 11 binary for trans plane */
	pDescriptor->BitDepth = 1;		/* Always 1 bit for trans plane */

	pTransBand->TransHdr.NumDirtyRects = 1;	/* Always 1 or 0 for now */
	pTransBand->TransHdr.DirtyRect.r = 0;	/* Init with dirty rect of whole frame */
	pTransBand->TransHdr.DirtyRect.c = 0;
	pTransBand->TransHdr.DirtyRect.h = pTransContext->TransHeight;
	pTransBand->TransHdr.DirtyRect.w = pTransContext->TransWidth;

	pDescriptor->UseTransColor = pRtParms->UseTransColor;
	pDescriptor->uTransXRGB = 
				(pRtParms->TransColor.u8R<<16)
			  | (pRtParms->TransColor.u8G<<8)
			  | (pRtParms->TransColor.u8B);

	/* Note Huffman tables are not known at this time */
	BitBuffAlloc(&(pTransBand->BsTransHdr), sizeof(TransHdrSt), jbEnv);

	/* Now figure out tile sizes and number of tiles and alloc the tile info structs */
	pTransBand->NumTiles = pTransContext->NumTilesHigh * pTransContext->NumTilesWide;

	/* Allocate space for tile info structs */

	pTransBand->Tiles =	
		(PTransTileSt) SysMalloc(pTransBand->NumTiles * sizeof(TransTileSt), jbEnv);

	pTransBand->XORTiles =	
		(PTransTileSt) SysMalloc(pTransBand->NumTiles * sizeof(TransTileSt), jbEnv);

	/* Initialize the tiles in the info structures */
	for (iTileR = 0, pTileInfo = pTransBand->Tiles;
		 iTileR < pTransContext->TransHeight; iTileR += pTransContext->TileHeight) {
		for (iTileC = 0; 
			 iTileC < pTransContext->TransWidth; 
			 iTileC += pTransContext->TileWidth) {
			
			/* Calculate rectangle information */
			pTileInfo->TileRect.r = iTileR;
			pTileInfo->TileRect.c = iTileC;
			pTileInfo->TileRect.h = pTransContext->TileHeight - 
				MAX(0, iTileR + pTransContext->TileHeight - pTransContext->TransHeight);
			pTileInfo->TileRect.w = pTransContext->TileWidth - 
				MAX(0, iTileC + pTransContext->TileWidth - pTransContext->TransWidth);

			pTileInfoHdr = &pTileInfo->TransTileHdr;
			pTileInfoHdr->IsEmpty = FALSE;
			pTileInfoHdr->IsTileDataSize = TRUE;
			pTileInfoHdr->TileDataSize = 0;
			BitBuffAlloc(&(pTileInfo->BsTransTileHdr), sizeof(TransTileHdrSt), jbEnv);
			BitBuffAlloc(&(pTileInfo->BsPreHuffDat),
						 pTransContext->TileHeight*pTransContext->TileWidth,
						 jbEnv);	
			BitBuffAlloc(&(pTileInfo->BsTransTileDat),
						 pTransContext->TileHeight*pTransContext->TileWidth,
						 jbEnv);
			pTileInfo++;
		}	/* for iTileC */
	}	/* for iTileR */

	/* Initialize the tiles in the info structures */
	for (iTileR = 0, pTileInfo = pTransBand->XORTiles;
		 iTileR < pTransContext->TransHeight; iTileR += pTransContext->TileHeight) {
		for (iTileC = 0; 
			 iTileC < pTransContext->TransWidth; 
			 iTileC += pTransContext->TileWidth) {
			
			/* Calculate rectangle information */
			pTileInfo->TileRect.r = iTileR;
			pTileInfo->TileRect.c = iTileC;
			pTileInfo->TileRect.h = pTransContext->TileHeight - 
				MAX(0, iTileR + pTransContext->TileHeight - pTransContext->TransHeight);
			pTileInfo->TileRect.w = pTransContext->TileWidth - 
				MAX(0, iTileC + pTransContext->TileWidth - pTransContext->TransWidth);

			pTileInfoHdr = &pTileInfo->TransTileHdr;
			pTileInfoHdr->IsEmpty = FALSE;
			pTileInfoHdr->IsTileDataSize = TRUE;
			pTileInfoHdr->TileDataSize = 0;
			BitBuffAlloc(&(pTileInfo->BsTransTileHdr), sizeof(TransTileHdrSt), jbEnv);
			BitBuffAlloc(&(pTileInfo->BsPreHuffDat),
						 pTransContext->TileHeight*pTransContext->TileWidth,
						 jbEnv);	
			BitBuffAlloc(&(pTileInfo->BsTransTileDat),
						 pTransContext->TileHeight*pTransContext->TileWidth,
						 jbEnv);
			pTileInfo++;
		}	/* for iTileC */
	}	/* for iTileR */

	return pTransContext;
}

/**********************************************/
/* Free the Transparency Band Encoder Context */
/**********************************************/

void EncTransClose(PEncTransCntxSt pTransContext,
				   PTransSt pTransBand,
				   jmp_buf jbEnv)
{
	I32 iTile;					/* Tile Counter */
	PTransTileSt pTileInfo;		/* Pointer to current tile Info struct */

	pTileInfo = pTransBand->Tiles;
	for (iTile = 0; iTile < pTransBand->NumTiles; iTile++, pTileInfo++) {
		BitBuffFree(&(pTileInfo->BsTransTileHdr), jbEnv);
		BitBuffFree(&(pTileInfo->BsPreHuffDat), jbEnv);	
		BitBuffFree(&(pTileInfo->BsTransTileDat), jbEnv);
	}

	pTileInfo = pTransBand->XORTiles;
	for (iTile = 0; iTile < pTransBand->NumTiles; iTile++, pTileInfo++) {
		BitBuffFree(&(pTileInfo->BsTransTileHdr), jbEnv);
		BitBuffFree(&(pTileInfo->BsPreHuffDat), jbEnv);	
		BitBuffFree(&(pTileInfo->BsTransTileDat), jbEnv);
	}

	BitBuffFree(&(pTransBand->BsTransHdr), jbEnv);
	
	SysFree((PU8) pTransBand->Tiles, jbEnv);
	SysFree((PU8) pTransBand->XORTiles, jbEnv);
	SysFree((PU8) pTransBand, jbEnv);
	SysFree((PU8) pTransContext->pu8TransByteMask, jbEnv);
	SysFree((PU8) pTransContext, jbEnv);

 return;
}

/********************************/
/* Encode the Transparency Band */
/********************************/

void EncTrans(PEncTransCntxSt pTransContext,
			  PTransSt pTransBand,
			  PU8 pu8TransBitMask,
			  U8 EncodeMode,
			  jmp_buf jbEnv) {

	PTransTileSt pTileInfo;			/* Pointer to current tile Info struct */
	I32 iTileR, iTileC;

	switch (EncodeMode) {
		case ENC_MODE_TRIAL:
		case ENC_MODE_FINAL:
			/* Do nothing in these cases, compression has already been done */
		break;

		case ENC_MODE_ALL:
		case ENC_MODE_TEST_WATER:  /* Encode Transparency Band */
			/* Convert from 1 bpp to 8 bpp, calc bounding rectangle */
			BuildTransparencyBytemask(pTransContext, pu8TransBitMask, 
									  &(pTransBand->TransHdr.DirtyRect));

			/* Walk through the tiles and compress, filling in the info structs. */
			for (iTileR = 0, pTileInfo = pTransBand->Tiles;
				 iTileR < pTransContext->TransHeight; 
				 iTileR += pTransContext->TileHeight) {
				for (iTileC = 0; 
					 iTileC < pTransContext->TransWidth; 
					 iTileC += pTransContext->TileWidth) {

					RLECompressTile(pTileInfo, pTransContext, jbEnv);
					pTileInfo++;
				} /* for (iTileC) */
			} /* for (iTileR) */

			/* Walk through the tiles and compress, filling in the info structs. */
			/* Compress with XOR_RLE this time */
			for (iTileR = 0, pTileInfo = pTransBand->XORTiles;
				 iTileR < pTransContext->TransHeight; 
				 iTileR += pTransContext->TileHeight) {
				for (iTileC = 0; 
					 iTileC < pTransContext->TransWidth; 
					 iTileC += pTransContext->TileWidth) {

					RLE_XORCompressTile(pTileInfo, pTransContext, jbEnv);
					pTileInfo++;
				} /* for (iTileC) */
			} /* for (iTileR) */

			break;

		default:
			longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
		  					(__LINE__ << LINE_OFFSET) |
							(ERR_BAD_PARM << TYPE_OFFSET));
			break;

	} /* Switch EncodeMode */

	return;
}

/***************************************************************************
 *
 * BuildTransparencyByteMask
 *
 * Accepts a transparency bitmask at 1 bit per pixel in pu8TransPixels.
 * It then unpacks this information to 8 bits per pixel and puts it in
 * pContext->pu8TransparencyBitmask.
 *
 * returns nothing but will update prDirtyRect and pTransContext fields
 *
 ***************************************************************************/

static void BuildTransparencyBytemask(PEncTransCntxSt pTransContext,
 								 	  PU8 pu8TransBitMask, PRectSt prDirtyRect)
{

	I32 i,j;					/* Loop Counters */
    I32 iWidth, iHeight;		/* resolution of output frame */
    U32 uTempData=0L;			/* Hold Partial DWORDS while being built */
    U32 uMaskCnt=0L;			/* Keeps track of which bit in current DWORD */
    PU32 puBitMaskArray = (PU32) pu8TransBitMask;
	PU8 pu8ByteMaskArray = pTransContext->pu8TransByteMask;
	I32 iMinRow, iMaxRow, iMinCol, iMaxCol;	/* Holds Bounding box in progress */
	PIA_Boolean bAllTrans = TRUE;	/* Keeps track of any nontransparent pixels */

    iWidth  = pTransContext->TransWidth;
    iHeight = pTransContext->TransHeight;

	iMinRow = iHeight;   /* Init Variables to keep track of bounding box */
	iMaxRow = -1;
	iMinCol = pTransContext->OrigWidth;
	iMaxCol = -1;

	uTempData = *puBitMaskArray++;;      /* Get first DWORD of TransBitmask */

	for (i = 0; i < iHeight; i++) {
        for (j = 0; j < iWidth; j++) {
			if (uTempData & uMaskarray[uMaskCnt]) {	/* pixel opaque? */
				*pu8ByteMaskArray++ = 1;
				
				bAllTrans = FALSE;

				/* Do bounding box tests */

				iMinRow = (i < iMinRow) ? i : iMinRow;
				iMaxRow = (i > iMaxRow) ? i : iMaxRow;
				iMinCol = (j < iMinCol) ? j : iMinCol;
				iMaxCol = (j > iMaxCol) ? j : iMaxCol;

			} else {
				*pu8ByteMaskArray++ = 0;
			}
										
	       	uMaskCnt += 1;

			/* Advance to the next DWORD in Trans Bitmask*/

	       	if (uMaskCnt == 32) {
        	  	uTempData = *puBitMaskArray++;
    	       	uMaskCnt = 0;
	       	}
	    } /* for (j ... */
	} /* for (i ... */

	
	 /* Handle the exception when no nontransparent pixels were present */
	if (bAllTrans) { 
		prDirtyRect->r = 0;
		prDirtyRect->c = 0;
		prDirtyRect->h = 0;
		prDirtyRect->w = 0;
	} else {						/* Store Bounding Box Results */
	 	prDirtyRect->r = iMinRow;   /* Convert to Row, Col, Height, Width */
		prDirtyRect->c = iMinCol;
		prDirtyRect->w = iMaxCol - iMinCol + 1;
		prDirtyRect->h = iMaxRow - iMinRow + 1;
	}

	return;
}


/***************************************************************************
 *
 * RLECompressTile
 *
 * This function accepts a tile and it determines if it is empty, if not it  
 * does a RLE lossless compression on the tile data and returns the results    
 * the tile info struct.
 *
 ***************************************************************************/


static void RLECompressTile(PTransTileSt pTileInfo, PEncTransCntxSt pTransContext, jmp_buf jbEnv)

{

	PU8		pu8Cur;		/* Pointer to current trans pel in BM */
	I32		iStride;	/* Stride in trans BM */
	I32		i, j;   	/* Looping Variables */
	U32		uSym, uRun;	/* Keep track of current Symbol and Run length */
	PIA_Boolean bStillEmpty;	/* Keeps track if all pixels are the same value */

	bStillEmpty = TRUE;		 /* Initialize Empty Detection */

	for (i=0; i<256; i++)				/* Clear the Histogram Array */
		pTileInfo->RunHistogram[i] = 0;

	pTileInfo->NumRuns = 0;				/* Clean Run Counter */

	iStride = pTransContext->TransWidth;
	pu8Cur = pTransContext->pu8TransByteMask + (pTileInfo->TileRect.r * iStride) + pTileInfo->TileRect.c;


	/* Loop through all pels and build runs */
	uSym = *pu8Cur;
	uRun = 0;

	/* Initial state of the tile which becomes the fill value in the case of an
	   empty tile.
	 */
	pTileInfo->TransTileHdr.InitialState = uSym;
    BitBuffInit(&(pTileInfo->BsPreHuffDat), jbEnv);

	for (i=0; i < (I32)pTileInfo->TileRect.h; i++) {
		for (j=0; j < (I32)pTileInfo->TileRect.w; j++) {
			if (uSym == *pu8Cur) {	/* If this pel is the same as the previous... */
				uRun++;				/* Increment the length of the run */
				if (uRun == 256) {	/* If the run is now 256 (max length + 1) */
					/* Write out a run of 0, which is a run of 255, 
					 * followed by a run of the same pel value (no flip) */
					BitBuffWrite(&(pTileInfo->BsPreHuffDat), 
								HuffCodeIndic, IndicatorLen, jbEnv);
					BitBuffWrite(&(pTileInfo->BsPreHuffDat), 0, HuffLen, jbEnv);

					pTileInfo->RunHistogram[0]++;
					pTileInfo->NumRuns++;
					uRun = 1;   /* This pel is included in the new run */
				}	
			} else {		/* else this is the end of the previous run */
				bStillEmpty = FALSE;
		 		uSym = *pu8Cur;
				BitBuffWrite(&(pTileInfo->BsPreHuffDat), 
							HuffCodeIndic, IndicatorLen, jbEnv);
				BitBuffWrite(&(pTileInfo->BsPreHuffDat), uRun, HuffLen, jbEnv);

				pTileInfo->RunHistogram[uRun]++;
				pTileInfo->NumRuns++;
				uRun = 1;  /* This pel is included in the new run */
			}
			pu8Cur++;
		}
		pu8Cur += iStride - pTileInfo->TileRect.w;  /* Get to start of next line in tile */
	}

	/* Write out the last run at the end of the tile */
	BitBuffWrite(&(pTileInfo->BsPreHuffDat), HuffCodeIndic, IndicatorLen, jbEnv);
	BitBuffWrite(&(pTileInfo->BsPreHuffDat), uRun, HuffLen, jbEnv);
	pTileInfo->RunHistogram[uRun]++;
	pTileInfo->NumRuns++;
	pTileInfo->TransTileHdr.IsEmpty = bStillEmpty;

    BitBuffFlush(&(pTileInfo->BsPreHuffDat));

	return;
}

/***************************************************************************
 *
 * RLE_XORCompressTile
 *
 * This function accepts a tile and it determines if it is empty, if not it  
 * does a RLE_XOR lossless compression on the tile data and returns the results    
 * the tile info struct.
 *
 ***************************************************************************/


static void RLE_XORCompressTile(PTransTileSt pTileInfo, PEncTransCntxSt pTransContext, jmp_buf jbEnv)

{

	PU8		pu8Cur;		/* Pointer to current trans pel in BM */
	PU8		pu8Prev;    /* Pointer to previous line trans pel in BM */
	I32		iStride;	/* Stride in trans BM */
	I32		i, j;   	/* Looping Variables */
	U32		uSym, uRun;	/* Keep track of current Symbol and Run length */
	PIA_Boolean bStillEmpty;	/* Keeps track if all pixels are the same value */

	bStillEmpty = TRUE;		 /* Initialize Empty Detection */

	for (i=0; i<256; i++)				/* Clear the Histogram Array */
		pTileInfo->RunHistogram[i] = 0;

	pTileInfo->NumRuns = 0;				/* Clean Run Counter */

	iStride = pTransContext->TransWidth;
	pu8Cur = pTransContext->pu8TransByteMask + (pTileInfo->TileRect.r * iStride) + pTileInfo->TileRect.c;
	pu8Prev = pTransContext->pu8TransByteMask + (pTileInfo->TileRect.r * iStride) + pTileInfo->TileRect.c;

	/* Loop through all pels in first line and build runs */
	uSym = *pu8Cur;
	uRun = 0;

	/* Initial state of the tile which becomes the fill value in the case of an
	   empty tile.
	 */
	pTileInfo->TransTileHdr.InitialState = uSym;
    BitBuffInit(&(pTileInfo->BsPreHuffDat), jbEnv);

	for (j=0; j < (I32)pTileInfo->TileRect.w; j++) {
		if (uSym == *pu8Cur) {	/* If this pel is the same as the previous... */
			uRun++;				/* Increment the length of the run */
			if (uRun == 256) {	/* If the run is now 256 (max length + 1) */
				/* Write out a run of 0, which is a run of 255, 
				 * followed by a run of the same pel value (no flip) */
				BitBuffWrite(&(pTileInfo->BsPreHuffDat), 
							HuffCodeIndic, IndicatorLen, jbEnv);
				BitBuffWrite(&(pTileInfo->BsPreHuffDat), 0, HuffLen, jbEnv);

				pTileInfo->RunHistogram[0]++;
				pTileInfo->NumRuns++;
				uRun = 1;   /* This pel is included in the new run */
			}	
		} else {		/* else this is the end of the previous run */
			bStillEmpty = FALSE;
	 		uSym = *pu8Cur;
			BitBuffWrite(&(pTileInfo->BsPreHuffDat), 
						HuffCodeIndic, IndicatorLen, jbEnv);
			BitBuffWrite(&(pTileInfo->BsPreHuffDat), uRun, HuffLen, jbEnv);

			pTileInfo->RunHistogram[uRun]++;
			pTileInfo->NumRuns++;
			uRun = 1;  /* This pel is included in the new run */
		}
		pu8Cur++;
	}

	pu8Cur += iStride - pTileInfo->TileRect.w;  /* Get to start of next line in tile */

	/* Now, begin coding all remaining lines of the image as line diffs */

	/* Loop through all remaining pels and build runs */

	for (i=0; i < (I32)pTileInfo->TileRect.h - 1; i++) {
		for (j=0; j < (I32)pTileInfo->TileRect.w; j++) {
			if (uSym == *pu8Cur ^ *pu8Prev) {	/* If this pel is the same as the previous... */
				uRun++;				/* Increment the length of the run */
				if (uRun == 256) {	/* If the run is now 256 (max length + 1) */
					/* Write out a run of 0, which is a run of 255, 
					 * followed by a run of the same pel value (no flip) */
					BitBuffWrite(&(pTileInfo->BsPreHuffDat), 
								HuffCodeIndic, IndicatorLen, jbEnv);
					BitBuffWrite(&(pTileInfo->BsPreHuffDat), 0, HuffLen, jbEnv);

					pTileInfo->RunHistogram[0]++;
					pTileInfo->NumRuns++;
					uRun = 1;   /* This pel is included in the new run */
				}	
			} else {		/* else this is the end of the previous run */
				bStillEmpty = FALSE;
		 		uSym = *pu8Cur ^ *pu8Prev;
				BitBuffWrite(&(pTileInfo->BsPreHuffDat), 
							HuffCodeIndic, IndicatorLen, jbEnv);
				BitBuffWrite(&(pTileInfo->BsPreHuffDat), uRun, HuffLen, jbEnv);

				pTileInfo->RunHistogram[uRun]++;
				pTileInfo->NumRuns++;
				uRun = 1;  /* This pel is included in the new run */
			}
			pu8Cur++;
			pu8Prev++;
		}
		pu8Cur += iStride - pTileInfo->TileRect.w;  /* Get to start of next line in tile */
		pu8Prev += iStride - pTileInfo->TileRect.w;  /* Get to start of next line in tile */
	}

	/* Write out the last run at the end of the tile */
	BitBuffWrite(&(pTileInfo->BsPreHuffDat), HuffCodeIndic, IndicatorLen, jbEnv);
	BitBuffWrite(&(pTileInfo->BsPreHuffDat), uRun, HuffLen, jbEnv);
	pTileInfo->RunHistogram[uRun]++;
	pTileInfo->NumRuns++;
	pTileInfo->TransTileHdr.IsEmpty = bStillEmpty;

    BitBuffFlush(&(pTileInfo->BsPreHuffDat));

	return;
}

