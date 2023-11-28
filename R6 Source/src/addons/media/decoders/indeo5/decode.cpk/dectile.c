/*///////////////////////////////////////////////////////////////////////*/
/*                                                                       */
/*               INTEL CORPORATION PROPRIETARY INFORMATION               */
/*                                                                       */
/*    This listing is supplied under the terms of a license agreement    */
/*       with INTEL Corporation and may not be copied nor disclosed      */
/*          except in accordance with the terms of that agreement.       */
/*                                                                       */
/*///////////////////////////////////////////////////////////////////////*/
/*                                                                       */
/*               Copyright (c) 1996-1997 Intel Corporation.              */
/*                         All Rights Reserved.                          */
/*                                                                       */
/*///////////////////////////////////////////////////////////////////////*/
/*                                                                       */
/*    FILENAME.XXX                                                       */
/*                                                                       */
/*    DECTILE.C -- Loop through the tiles in a band, decide which ones   */
/*    to decode, and then decode their macroblock & block data layers.   */
/*///////////////////////////////////////////////////////////////////////*/
/* $Header:   I:\proj50\src\cpdecode\vcs\dectile.c_v   1.2   14 Aug 1998 09:35:44   preger  $
 */
/*///////////////////////////////////////////////////////////////////////*/

#include "datatype.h"
#include "pia_main.h"
#include "cpk_blk.h"
#include "xpardec.h"
#include "rtdec.h"
#include "dectile.h"
#include "decpl.h"
#include "cpk_mc.h"
#include "readbits.h"

#ifdef DEBUG
#include <stdio.h>
#include "tsc.h"
#endif /* DEBUG */


#ifdef DEBUG
extern FILE * timefile;
#endif /* DEBUG */


#ifdef DEBUG
extern Boo bBandCheckSum;

/*	These routines are just stubs to ease debugging and are not part of the
 *	real product.  (They are there so breakpoints can be set on them that
 *	won't go away when checksumming is turned off).
 */
void CheckSumError(U32 plane, U32 band);
void CheckSumError(U32 plane, U32 band) {
	plane;
	band;
#ifndef QT_FOR_MAC
					__asm nop;
#else
					HivePrintF("Whoa\n");
#endif
}
void TileSizeError(U32 plane, U32 band);
void TileSizeError(U32 plane, U32 band) {
	plane;
	band;
#ifndef QT_FOR_MAC
					__asm nop;
#else
					HivePrintF("Whoa\n");
#endif
}
#endif /* DEBUG */

#ifdef DEBUG
static void ret_err(U32 err) {
	switch (err) {
		case 0:
			HivePrintF("Decoder: invalid block data input");
			break;
		case 2:
			HivePrintF("Decoder: invalid block info input");
			break;
		case 3:
			HivePrintF("Decoder: bad bitstream - tile data size doesn't match bits read");
			break;
	}
}
#define RET_ERR(x,y) ret_err(x); (y)

#else /* DEBUG */
#define RET_ERR(x,y) (y)
#endif /* DEBUG */


void ZeroBand(pRTDecInst pCntx, U32 plane, U32 band);



U32 DecodeTileInfo(
	pRTDecInst	pCntx,
	PU8			InPtr,
	PU32		bytes,
	U32			plane,
	U32			band,
	Boo			bDecodeBand) {

	bitbufst	p;
	U32			code;			/* temporary for reading codes from bs */

	PU8			InPtrSave;

	U32			uNTilesRead;
	U32			uTileDataSize;
	U32			uNTiles;
	pRtBandSt	b;
	Boo			bBandDecodeIsInitialized = FALSE;

	InPtrSave = InPtr;

	b = &pCntx->Plane[plane].pBand[band];
	uNTiles = pCntx->tNTiles.r * pCntx->tNTiles.c;
	
	uNTilesRead = 0;

	readbitsinit2(&p, InPtr);

	while (uNTilesRead < uNTiles) {
		pRtTileSt	t;
		Boo			bDecTile;	/* do we decode this tile or not? */
		U32			uNBlocks;
		U32			uNMBlocks;

		t = &b->pTile[uNTilesRead];

		uNMBlocks = t->tNTileMBs.c * t->tNTileMBs.r;
		uNBlocks = t->tNTileBlocks.c * t->tNTileBlocks.r;

		bDecTile = bDecodeBand && pCntx->vu8DecodeTiles[uNTilesRead];

		code = readbits2(&p, 1);
		if (code) {
		/*	We have an empty tile - if delta frame, then copy predicted
		 *	tile to current tile.  If InheritTypeMV is on, then use the
		 *	inherited motion vectors.
		 */

			if (!b->bQDelta && !plane && !band) {
			/*	support for no variable quant */
				U32	quantf, i;

				quantf = ((NQLEVS + b->uGlobalQuant) << BT_LOG2_QMASK)
					| BT_FWD | b->uPitch;
				for (i = 0; i < uNBlocks; i++) {
					t->pBlockInfo[i].motion_vectors = 0x80808080;
					t->pBlockInfo[i].flags = quantf;
					if (i&1) t->pBlockInfo[i].flags |= BT_DUAL_HI;
				}
			}



			if (bDecTile && (pCntx->uFrameType > PICTYPE_K)) {
				U32		r, c, i;
				U32		nr, nc, offset;

				PU32	brCurPtr, bcCurPtr;
				PU32	brPredPtr, bcPredPtr;

				offset = pCntx->Plane[plane].uPlaneOffset
					+ b->uBandOffset + t->uTileOffset;

				brCurPtr = (PU32)(pCntx->pFrameCurrent + offset);

				nr = t->tNTileBlocks.r;
				nc = t->tNTileBlocks.c;

				i = 0;
				if (b->uMBSize == b->uBlockSize) {
					for (r = 0; r < nr; r++) {
						bcCurPtr = brCurPtr;
						for (c = 0; c < nc; c++) {
							BlockInfoSt		tb;

							tb = pCntx->Plane[0].pBand[0].pTile[uNTilesRead].pBlockInfo[i];
							t->pBlockInfo[i].flags = b->uPitch | ((c&1)<<BT_LOG2_DUAL_HI) | BT_FWD;
							t->pBlockInfo[i].motion_vectors = tb.motion_vectors;
							t->pBlockInfo[i++].curr = (PU8)bcCurPtr;

							bcCurPtr += b->uBlockSize/2;
						}
						brCurPtr += b->uPitch*b->uBlockSize/4;
					}
				}
				else {	/* 4 blocks per macroblock */
					for (r = 0; r < nr; r+=2) {
						bcCurPtr = brCurPtr;
						for (c = 0; c < nc; c+=2) {
							BlockInfoSt		tb;

							tb = pCntx->Plane[0].pBand[0].pTile[uNTilesRead].pBlockInfo[i];
							t->pBlockInfo[i].flags = b->uPitch | BT_FWD;
							t->pBlockInfo[i].motion_vectors = tb.motion_vectors;
							t->pBlockInfo[i++].curr = (PU8)bcCurPtr;

							tb = pCntx->Plane[0].pBand[0].pTile[uNTilesRead].pBlockInfo[i];
							t->pBlockInfo[i].flags = b->uPitch | BT_DUAL_HI | BT_FWD;
							t->pBlockInfo[i].motion_vectors = tb.motion_vectors;
							t->pBlockInfo[i++].curr = (PU8)bcCurPtr + b->uBlockSize*2;

							tb = pCntx->Plane[0].pBand[0].pTile[uNTilesRead].pBlockInfo[i];
							t->pBlockInfo[i].flags = b->uPitch | BT_FWD;
							t->pBlockInfo[i].motion_vectors = tb.motion_vectors;
							t->pBlockInfo[i++].curr = (PU8)bcCurPtr + b->uBlockSize*(b->uPitch);

							tb = pCntx->Plane[0].pBand[0].pTile[uNTilesRead].pBlockInfo[i];
							t->pBlockInfo[i].flags = b->uPitch | BT_DUAL_HI | BT_FWD;
							t->pBlockInfo[i].motion_vectors = tb.motion_vectors;
							t->pBlockInfo[i++].curr = (PU8)bcCurPtr + b->uBlockSize*(b->uPitch+2);

							bcCurPtr += b->uBlockSize; /* 2 blocks * 2 bytes per elemsnt / 4 */
						}
						brCurPtr += b->uPitch*b->uBlockSize/2; /* = *2 blocks / 4 bytes_per_dword */
					}
				}

				if (b->bInheritTypeMV) {
					U32		i;

					if (b->uMBSize*2 == pCntx->Plane[0].pBand[0].uMBSize) {
						U32 tt;
						for (i = 0; i < uNBlocks; i++) {
							tt = t->pBlockInfo[i].motion_vectors;
							t->pBlockInfo[i].motion_vectors = SCALE_2(tt);
						}
					}

					if (b->uMBSize*4 == pCntx->Plane[0].pBand[0].uMBSize) {
						U32 tt;
						for (i = 0; i < uNBlocks; i++) {
							tt = t->pBlockInfo[i].motion_vectors;
							t->pBlockInfo[i].motion_vectors = SCALE_4(tt);
						}
					}

					AddMotionToDelta_C(pCntx->uFrameType,
						t->pBlockInfo, uNBlocks, b->uBlockSize, b->uMVRes,
						(PU32)(pCntx->pFrameCurrent +
							pCntx->Plane[plane].uPlaneOffset +
							b->uBandOffset + t->uTileOffset),
						(PU32)(pCntx->pFrameForward +
							pCntx->Plane[plane].uPlaneOffset +
							b->uBandOffset + t->uTileOffset),
						(PU32)(pCntx->pFrameBackward +
							pCntx->Plane[plane].uPlaneOffset +
							b->uBandOffset + t->uTileOffset));
				}	/* bInheritTypeMV */
				else {

					brCurPtr = (PU32)(pCntx->pFrameCurrent + offset);
					brPredPtr = (PU32)(pCntx->pFrameForward + offset);

					nc = t->tNTileMBs.c;
					nc *= b->uMBSize << (2 - 1);
					nr = t->tNTileMBs.r * b->uMBSize;
					for (r = 0; r < nr; r++) {
						bcCurPtr = brCurPtr;
						bcPredPtr = brPredPtr;
						for (c = 0; c < nc; c += 4) {
							*bcCurPtr++ = *bcPredPtr++;
						}
						brCurPtr += (b->uPitch >> 2);
						brPredPtr += (b->uPitch >> 2);
					}
				}


			}
			uNTilesRead++;
			continue;
		}	/* if empty tile */

	/*	Read Tile Datasize field */
		code = readbits2(&p, 1);
		if (code) {
			code = readbits2(&p, 8);
			if (code == 255) {
				code = readbits2(&p, 24);
			}
		}
		uTileDataSize = code;

	/*	we must decode a tile if the datasize is not present */
		if (bDecTile || !uTileDataSize) {
			U32		sumBytesRead;

			sumBytesRead = bytesread(InPtr, &p);

		/*	only initialize 1st decoded tile */
			if (!bBandDecodeIsInitialized) {

				CheckSetContext2(&b->BlkCntx[pCntx->uFrameType], &b->SetCntx);
				bBandDecodeIsInitialized = TRUE;
			}	/* bBandDecodeIsInitialized */


			{
				pRtBandSt pYBand0;
				pYBand0 = &pCntx->Plane[0].pBand[0];

			code = DecodeBlockInfo_C(
				pCntx->uFrameType,	
				pCntx->MBHuffTab,

				t->pBlockInfo,
				pYBand0->pTile[uNTilesRead].pBlockInfo,
				pCntx->pFrameCurrent
					+ pCntx->Plane[plane].uPlaneOffset
					+ b->uBandOffset
					+ t->uTileOffset,
				t->tNTileMBs,
				b->uMBSize,
				b->uBlockSize,
				b->uPitch,
				b->uMVRes,
				b->bInheritTypeMV,	/* Inheritance: Type & MV */
				b->bInheritQ,		/* Inheritance: Quant */
				b->bQDelta,			/* Quant Delta Used ? */
				b->uGlobalQuant,
				pYBand0->uGlobalQuant,
				!(band|plane) && pCntx->bInheritQ,
				pYBand0->uMBSize,
				&b->BlkCntx[pCntx->uFrameType].dequant,
				pYBand0->uBlockSize != pYBand0->uMBSize,
				InPtr + sumBytesRead);
			}
			if (!code) {
				RET_ERR(1, 0); return FALSE;
			}
			sumBytesRead += code;

			code = DecodeBlockData_C(&b->BlkCntx[pCntx->uFrameType],
				0, InPtr + sumBytesRead, t->pBlockInfo, uNBlocks,
				&b->rvswap[0]);
			sumBytesRead += code;

			if (pCntx->uFrameType) { /* if delta frame */
				AddMotionToDelta_C(pCntx->uFrameType, t->pBlockInfo, uNBlocks,
				b->uBlockSize, b->uMVRes,
				(PU32)(pCntx->pFrameCurrent + b->uBandOffset + t->uTileOffset),
				(PU32)(pCntx->pFrameForward + b->uBandOffset + t->uTileOffset),
				(PU32)(pCntx->pFrameBackward + b->uBandOffset + t->uTileOffset));
			}

			if (sumBytesRead != uTileDataSize) {
				if (uTileDataSize) {
					RET_ERR(2, 0); return FALSE;
				}
			}
			InPtr += uTileDataSize;
		}
		else {
			InPtr += uTileDataSize;
		}
	/*	if inherit, then increment source tile buffer
	 */
		uNTilesRead++;
		readbitsinit2(&p, InPtr);
	}

#ifdef DEBUG
/*	Debug code to calculate & verify checksums */
	if (/*bBandCheckSum && bDecodeBand &&*/ (b->uCheckSum != (U32)(-1))) {
		U32 r, c;
		PU32 brptr, bcptr;
		U32 tt, xsum;
		xsum=0;

		brptr = (PU32)(pCntx->pFrameCurrent + pCntx->Plane[plane].uPlaneOffset
			+ b->uBandOffset);

		if (b->uBlockSize == 4) {
			for (r = 0; r < b->tBandSize.r; r++) {
				bcptr = brptr;
				for (c = 0; c < b->tBandSize.c; c += 4) {
					tt = *bcptr++;
					xsum += ((tt & 0xffff) - 0x4000);
					if (c+2 < b->tBandSize.c) {
						xsum += ((tt >> 16) - 0x4000);
					}
				}
				brptr += (b->uPitch >> 2);
			}
		}
		else {
			for (r = 0; r < b->tBandSize.r; r++) {
				bcptr = brptr;
				for (c = 0; c < b->tBandSize.c; c += 4) {
					tt = *bcptr++;
					xsum += ((tt & 0xffff) - 0x4000);
					if (c+2 < b->tBandSize.c) {
						xsum += ((tt >> 16) - 0x4000);
					}
				}
				brptr += (b->uPitch >> 2);
			}
		}
		xsum &= 0xffff;
		{	U32 t;
			t = b->uCheckSum;
			if (xsum != b->uCheckSum) {
				CheckSumError(plane, band);
				if (timefile)
					fprintf(timefile,
						"XSum Error - f%d, p%d, b%d, obs=%4x, exp=%4x\n",
						pCntx->u8FrameNumber, plane, band, xsum, b->uCheckSum);
			}
		}
	}
#endif /* DEBUG */

	*bytes = bytesread(InPtrSave, &p);

	return TRUE;
}
