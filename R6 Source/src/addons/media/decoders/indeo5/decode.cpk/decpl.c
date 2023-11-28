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
*               Copyright (C) 1994-1997 Intel Corp.                     *
*                         All Rights Reserved.                          *
*                                                                       *
************************************************************************/
/*************************************************************************
 *                                                                       *
 *              INTEL CORPORATION PROPRIETARY INFORMATION                *
 *                                                                       *
 *      This software is supplied under the terms of a license           *
 *      agreement or nondisclosure agreement with Intel Corporation      *
 *      and may not be copied or disclosed except in accordance          *
 *      with the terms of that agreement.                                *
 *                                                                       *
 *************************************************************************/

/*************************************************************************
 * DECPL.C -- function for processing the picture layer.
 *************************************************************************/


#include "datatype.h"
#include "pia_main.h"
#include "cpk_blk.h"
#include "xpardec.h"
#include "rtdec.h"
#include "decpl.h"
#include "readbits.h"

#define NEW_ACCESS_KEY_SUPPORT


extern const PBDESCRIPTOR_TYPE BlockStaticHuff[9][17];
#ifdef DEBUG
#include <stdio.h>
extern FILE * timefile;
extern Boo bPrintTileInfo;
#endif

#define BANDDELTA 0x400

#ifdef DEBUG
#define RET_ERR(r) ivi_err(); r
void ivi_err(void) {
#ifndef QT_FOR_MAC
					__asm nop;
#else
					HivePrintF("Whoa\n");
#endif
}
#else /* DEBUG */
#define RET_ERR(r) r
#endif /* DEBUG */

/* ********************************************************************* */

const I32 XresTable[15] = { 640, 320, 160, 704, 352, 352, 176, 240,
							640, 704,  80,  88,   0,   0,   0 };
const I32 YresTable[15] = { 480, 240, 120, 480, 240, 288, 144, 180,
							240, 240, 60,   72,   0,   0,   0 };

enum	{
S8x8=0, S8x1=1, S1x8=2, N8x8=3,
S4x4=4, S4x1=5, S1x4=6, N4x4=7
}	BlockTransForms;

/* ********************************************************************* */

PIA_RETURN_STATUS DecodePictureLayer(pRTDecInst pCntx, PU8 pntr, PU32 bytes,
	RectSt rDecodeRect, RectSt rViewRect) {

	bitbufst	pb;
	U32		code;

	U32		i;
	U32		plane;

	PointSt	tTemp;

	U32		uYVUOutputSize;

	Boo		bNewFrameSize;
	Boo		bNewBanding;
	Boo		bNewTileSize;
	Boo		bNewDecodeRect;

	U32		uNTiles;
	U32		uTmpTileSize;
	U32		uBsAccessKey;
	Boo		bIsAccessKeyInBs;

	U32		uNBands[2] = { 0, 0 };
	U32		uRunningBandOffset;
	PIA_RETURN_STATUS	ret = PIA_S_OK;

	bNewFrameSize = bNewTileSize = FALSE;

/*	Save the view rect
 */
	pCntx->rViewRect.h = rViewRect.h;
	pCntx->rViewRect.w = rViewRect.w;
	pCntx->rViewRect.r = rViewRect.r;
	pCntx->rViewRect.c = rViewRect.c;

	readbitsinit2(&pb, pntr);

	code = readbits2(&pb, 5);
	if (code != PIC_START_CODE) {
		RET_ERR(return) PIA_S_BAD_COMPRESSED_DATA;
	}

/*	Picture Type
 */
	code = readbits2(&pb, 3);
	if (code >= PICTYPE_RESERVED) {
		RET_ERR(return) PIA_S_BAD_COMPRESSED_DATA;
	}
	pCntx->uFrameType = code;


/*	Read Picture Number
 */
	code = readbits2(&pb, 8);
	if (pCntx->uFrameType && (pCntx->u8FrameNumber == code)) {
	/*	This fixed the media player last frame bug: if same non-key
	 *	frame is passed to codec, pretend it's a repeat
	 */
		pCntx->uFrameType = PICTYPE_R;
	}
	pCntx->u8FrameNumber = (U8)code;

	pCntx->uFlags &= ~(PL_CODE_EXPRMNTL|PL_CODE_TRANSP);
	if (pCntx->uFrameType != PICTYPE_K) {
		if (pCntx->u8GOPFlags & GOPF_XPAR) {
			pCntx->uFlags |= PL_CODE_TRANSP;
		}
		if (pCntx->u8GOPFlags & GOPF_EXPERMNTL) {
			pCntx->uFlags |= PL_CODE_EXPRMNTL;
			RET_ERR(return) PIA_S_UNSUPPORTED_FUNCTION;
		}

		if (pCntx->uFrameType >= PICTYPE_R) {
			goto end;
		}
		goto decodepictureheader;
	}

/*	Procede to read GOP Header */
	code = readbits2(&pb, 8);
	pCntx->u8GOPFlags = (U8)code;

/*	Transparency, Experimental
 */
	if (pCntx->u8GOPFlags & GOPF_XPAR) {
		pCntx->uFlags |= PL_CODE_TRANSP;
	}
	if (pCntx->u8GOPFlags & GOPF_EXPERMNTL) {
		pCntx->uFlags |= PL_CODE_EXPRMNTL;
	}

/*	GOP Header Size 
 */
	if (pCntx->u8GOPFlags & GOPF_HAS_SIZE) {
	/*	read the size flag - lew: i don't think this has any value
	 */
		code = readbits2(&pb, 16);
	}

/*	Access Key
 */
	if (pCntx->u8GOPFlags & GOPF_LOCKWORD) {
	/*	Key frame Locking enabled:  get code */
		if (pCntx->bKeyFailure = !pCntx->bUseAccessKey) {
#ifdef NEW_ACCESS_KEY_SUPPORT
			ret = PIA_S_KEY_FAILURE;
#else /* NEW_ACCESS_KEY_SUPPORT */
			RET_ERR(return) PIA_S_KEY_FAILURE;
#endif /* NEW_ACCESS_KEY_SUPPORT */
		}
		uBsAccessKey = readbits2(&pb, 16);
		code = readbits2(&pb, 16);
		uBsAccessKey |= code << 16;
	}

/*	Obtain Tile Size Indicator 
 */
	uTmpTileSize = 0;
	if (pCntx->u8GOPFlags & GOPF_TILESIZE) {
	/*	00	64x64
	 *	01	128x128
	 *	10	256x256
	 *	11	Reserved
	 */
		code = readbits2(&pb, 2);
		uTmpTileSize = 1 + code;
	}

/*	Y, then VU subsampling
 */
	code = readbits2(&pb, 3);
/*  make sure ylevels & ulevels are a valid combination:
 *			nbands  band
 *  code	vu  y	descriptors
 *  000		1		1
 *  001	 	1	4	5
 *  010		1	7	8
 *  011		1	10  reserved
 *  100		4	1	reserved
 *  101		4	4	reserved
 *  110		4	7	11
 *  111		4	10  reserved
 */

	if ((code > 2) && (code != 6)) {
		RET_ERR(return) PIA_S_ERROR;	/*	invalid combination */
	}

/*	Y
 */
	uNBands[0] = 1 + (code & 3) * 3;
	bNewBanding = (pCntx->Plane[0].uNBands != uNBands[0]);

/*	V & U
 */
	uNBands[1] = 1 + (code >> 2) * 3;
	bNewBanding |= (pCntx->Plane[1].uNBands != uNBands[1]);

/*  Picture Size
 */
#define PICSIZE_ESCAPE 0xf
	code = readbits2(&pb, 4);
	if (code == PICSIZE_ESCAPE) {
		code = readbits2(&pb, 26);
		tTemp.c = code >> 13;
		tTemp.r = code & 0x1fff;
	}
	else {
		tTemp.c = XresTable[code];
		tTemp.r = YresTable[code];
	}

/*	if frame size has changed, then prepare to update memory allocation
 */
	if ((pCntx->tFrameSize.r != tTemp.r)
		|| (pCntx->tFrameSize.c != tTemp.c)) {

		bNewFrameSize = TRUE;
		pCntx->tFrameSize = tTemp;
	}

	if (pCntx->u8GOPFlags & GOPF_YVU12) {
		tTemp.c = (pCntx->tFrameSize.c + 1) >> 1;
		tTemp.r = (pCntx->tFrameSize.r + 1) >> 1;
	}
	else {
		tTemp.c = (pCntx->tFrameSize.c + 3) >> 2;
		tTemp.r = (pCntx->tFrameSize.r + 3) >> 2;
	}

	if ((pCntx->tVUFrameSize.r != tTemp.r)
		|| (pCntx->tVUFrameSize.c != tTemp.c)) {

		bNewFrameSize = TRUE;
		pCntx->tVUFrameSize = tTemp;
	}

/*	Process Tile Size Indicator 
 */
	if (uTmpTileSize) {
		tTemp.r = 32 << uTmpTileSize;
		tTemp.c = tTemp.r;

		if (tTemp.r > pCntx->tFrameSize.r) {
			tTemp.r = pCntx->tFrameSize.r;
			pCntx->tNTiles.r = 1;
		}
		else {
			pCntx->tNTiles.r = pCntx->tFrameSize.r + tTemp.r - 1;
			pCntx->tNTiles.r /= tTemp.r;
		}
		if (tTemp.c > pCntx->tFrameSize.c) {
			tTemp.c = pCntx->tFrameSize.c;
			pCntx->tNTiles.c = 1;
		}
		else {
			pCntx->tNTiles.c = pCntx->tFrameSize.c + tTemp.c - 1;
			pCntx->tNTiles.c /= tTemp.c;
		}
	}
	else {
		tTemp = pCntx->tFrameSize;
		pCntx->tNTiles.r = pCntx->tNTiles.c = 1;
	}

	if ((pCntx->tTileSize.r != tTemp.r)
		|| (pCntx->tTileSize.c != tTemp.c)) {

		bNewTileSize = TRUE;
		pCntx->tTileSize = tTemp;
	}

/*	Compute the number of tiles here
 */
	uNTiles = pCntx->tNTiles.r * pCntx->tNTiles.c;

/*	Determine the decoded portions of the planes.
 */
	pCntx->Plane[0].rDecoded.r = 0;
	pCntx->Plane[0].rDecoded.c = 0;
	pCntx->Plane[0].rDecoded.h = pCntx->tFrameSize.r;
	pCntx->Plane[0].rDecoded.w = pCntx->tFrameSize.c;

	pCntx->Plane[1].rDecoded.r = 0;
	pCntx->Plane[1].rDecoded.c = 0;
	pCntx->Plane[1].rDecoded.h = pCntx->tVUFrameSize.r;
	pCntx->Plane[1].rDecoded.w = pCntx->tVUFrameSize.c;
	pCntx->Plane[2].rDecoded = pCntx->Plane[1].rDecoded;

/*	Determine the visible portions of the planes.
 */
	pCntx->Plane[0].rVisible = rViewRect;

	pCntx->Plane[1].rVisible.r = rViewRect.r
		* pCntx->tVUFrameSize.r / pCntx->tFrameSize.r;
	pCntx->Plane[1].rVisible.c = rViewRect.c
		* pCntx->tVUFrameSize.c / pCntx->tFrameSize.c;
	pCntx->Plane[1].rVisible.h = rViewRect.h
		* pCntx->tVUFrameSize.r / pCntx->tFrameSize.r;
	pCntx->Plane[1].rVisible.w = rViewRect.w
		* pCntx->tVUFrameSize.c / pCntx->tFrameSize.c;
	pCntx->Plane[2].rVisible = pCntx->Plane[1].rVisible;

	pCntx->Plane[0].uOutputPitch = (pCntx->Plane[0].rDecoded.w + 0x1f) & ~0x1f;
	pCntx->Plane[1].uOutputPitch = (pCntx->Plane[1].rDecoded.w + 0x1f) & ~0x1f;
	pCntx->Plane[2].uOutputPitch = (pCntx->Plane[2].rDecoded.w + 0x1f) & ~0x1f;

	uYVUOutputSize =
		pCntx->Plane[0].rDecoded.h * pCntx->Plane[0].uOutputPitch +
		pCntx->Plane[1].rDecoded.h * pCntx->Plane[1].uOutputPitch +
		pCntx->Plane[2].rDecoded.h * pCntx->Plane[2].uOutputPitch;


	uRunningBandOffset = 0;


	for (plane = 0; plane < 2; plane++)	{
		const U8 XDefault[2][4] = {
			{ S8x8, S8x1, S1x8, N8x8 },
			{ S4x4, S4x1, S1x4, N4x4 }
		};

		pRtPlaneSt p;
		p = &pCntx->Plane[plane];

		if (bNewFrameSize || bNewBanding || bNewTileSize) {
			p->uPlaneOffset = uRunningBandOffset;
			if (!plane) {
				if (pCntx->pBlockInfo) {
					HiveLocalFreePtr(pCntx->pBlockInfo);
				}
				if (pCntx->pBlockInfoBand0) {
					HiveLocalFreePtr(pCntx->pBlockInfoBand0);
				}
			/*	can make this smaller if move the tile loop lower & correctly */
			/*	compute max band size, and double check this, of course */
				i = ((p->rDecoded.h * (p->rDecoded.w+0xf) >> 4) + uNTiles - 1)
					/ uNTiles;
				pCntx->pBlockInfo = HiveLocalAllocPtr(i*sizeof(BlockInfoSt), TRUE);
				if (!pCntx->pBlockInfo) {
					RET_ERR(return) PIA_S_OUT_OF_MEMORY;
				}
			}	/* !plane */

			if (p->pBand) {
				pRtBandSt b;
				for (i = 0; i < p->uNBands; i++) {
					b = &p->pBand[i];
					if (b->pTile) {
						HiveLocalFreePtr(b->pTile);
					}
				}
				HiveLocalFreePtr(p->pBand);
			}
			p->uNBands = uNBands[plane];
			p->pBand = HiveLocalAllocPtr(p->uNBands*sizeof(RtBandSt), 0);
			if (!p->pBand) {
				RET_ERR(return) PIA_S_OUT_OF_MEMORY;
			}
		}

	/*	Initialize the band structures 
	 */
		for (i = 0; i < p->uNBands; i++) {
			pRtBandSt	b;
			U32			uTile;
			U32			uMaxBlockInfoInBand;
			const U8	x2z[8]  = { 0, 1, 2,  2,  3,  4,  5,  5 };
			const U32	MBSizeTab[4] = { 16, 8, 8, 4 };
			const U32	BlockSizeTab[4] = { 8, 8, 4, 4 };

			b = &p->pBand[i];

		/*	b->SetCntx; */
			b->BlkCntx[0].initialized = 0;	/* key frame block context */
			b->BlkCntx[1].initialized = 0;	/* pee frame block context */
			b->BlkCntx[2].initialized = 0;	/* p2 frame block context */
			b->BlkCntx[3].initialized = 0;	/* dee frame block context */

			code = readbits2(&pb, 6);
			b->uMVRes = code & 1;	  /* 0 => int, 1 => half pel */
			b->uMBSize = MBSizeTab[(code >> 1) & 0x3];  /* 16/8, 8/8, 8/4, 4/4*/
			b->uBlockSize = BlockSizeTab[(code >> 1) & 0x3];  /* 16/8, 8/8, 8/4, 4/4 */

		/*	Transform */
			b->SetCntx.transform_token = (I8)XDefault[plane][i];
			if (code>>3) {
				b->SetCntx.transform_token = (I8)XDefault[plane][code>>4];
				code = readbits2(&pb, 2);
			}
			else code >>= 4;

			if (code) {
				/* explicit scan & quant not yet supported */
				RET_ERR(return) PIA_S_UNSUPPORTED_FUNCTION;
			}

		/*	Scan Order */
			b->SetCntx.scan_token = (I8)x2z[b->SetCntx.transform_token];

		/*	Quantization */
			if (plane) {
				if (p->uNBands > 1) {
					b->SetCntx.quant_token = 6+(U8)i/*&3+(U8)i>>2*/;
				}
				else {
					b->SetCntx.quant_token = 5;
				}
			}
			else {
				if (p->uNBands > 1) {
					b->SetCntx.quant_token = 1+(U8)i/*&3+(U8)i>>2*/;
				}
				else {
					b->SetCntx.quant_token = 0;
				}
			}
/*			b->SetCntx.quant_token = 5*(!!plane)+(p->uNBands>1)+(U8)i&3+(U8)i>>2; */

			if (bNewFrameSize || bNewBanding || bNewTileSize) {
				b->uBandOffset = uRunningBandOffset - p->uPlaneOffset;

			/*	Tiles */
				if (p->uNBands == 1) {
					b->tBandSize.r = p->rDecoded.h;
					b->tBandSize.c = p->rDecoded.w;
				}
				else if (p->uNBands == 4) {
					b->tBandSize.r = p->rDecoded.h >> 1;
					b->tBandSize.c = p->rDecoded.w >> 1;
				}
				else if (p->uNBands == 7) {
					if (i < 4) {
						b->tBandSize.r = p->rDecoded.h >> 2;
						b->tBandSize.c = p->rDecoded.w >> 2;
					}
					else {
						b->tBandSize.r = p->rDecoded.h >> 1;
						b->tBandSize.c = p->rDecoded.w >> 1;
					}
				}

				b->tTileSize.r = pCntx->tTileSize.r /
					(pCntx->tFrameSize.r / b->tBandSize.r);
				b->tTileSize.c = pCntx->tTileSize.c /
					(pCntx->tFrameSize.c / b->tBandSize.c);

				b->tTileSize.c <<= 1;
				b->tBandSize.c <<= 1;
			/*	Round up Pitch to a multiple of 32 - a requirement of our dual
			 *	interleaved block band storage.
			 */
				b->uPitch = (b->tBandSize.c + 0x1f) & ~0x1f;

				if (!(i||plane)) {
				/*	Allocate enough storage for worst case - 4x4 blocks, leaving
				 *	7 extra blockinfo elements for padding - assumed to be there
				 *	by the block decode
				 */
					uMaxBlockInfoInBand = uNTiles * 7 +
						(b->tBandSize.r * (b->tBandSize.c+0x1f) >> (4 + 1));
					pCntx->pBlockInfoBand0 = HiveLocalAllocPtr((uMaxBlockInfoInBand)
						* sizeof(BlockInfoSt), TRUE);
					if (!pCntx->pBlockInfoBand0) {
						RET_ERR(return) PIA_S_OUT_OF_MEMORY;
					}
				}

				b->pTile = HiveLocalAllocPtr(sizeof(RtTileSt) * uNTiles, 0);
				if (!b->pTile) {
					RET_ERR(return) PIA_S_OUT_OF_MEMORY;
				}
				for (uTile = 0; uTile < uNTiles; uTile++) {
					pRtTileSt	t;
					U32			curtilex, curtiley;

					curtilex = uTile % pCntx->tNTiles.c;
					curtiley = uTile / pCntx->tNTiles.c;

					t = &b->pTile[uTile];
#if 0 /* LEW TEST */
					if (curtiley + 1 == pCntx->tNTiles.r) {
						t->tNTileMBs.r =
							((b->tBandSize.r - curtiley*b->tTileSize.r) + b->uMBSize - 1)
							/ b->uMBSize;
					}
					else {
						t->tNTileMBs.r = (b->tTileSize.r + b->uMBSize - 1)
							/ b->uMBSize;
					}

					if (curtilex + 1 == pCntx->tNTiles.c) {
						t->tNTileMBs.c =
							(((b->tBandSize.c - curtilex*b->tTileSize.c)>>1) + b->uMBSize - 1)
							/ b->uMBSize;
					}
					else {
						t->tNTileMBs.c = ((b->tTileSize.c>>1) + b->uMBSize - 1)
							/ b->uMBSize;
					}

					t->tNTileBlocks = t->tNTileMBs;
					if (b->uMBSize != b->uBlockSize) {
						t->tNTileBlocks.r <<= 1;
						t->tNTileBlocks.c <<= 1;
					}
#endif /* LEW TEST */

				/*	double check this... */
					t->pBlockInfo = (i||plane) ? pCntx->pBlockInfo
						: &pCntx->pBlockInfoBand0[uTile*uMaxBlockInfoInBand/uNTiles];
					t->uTileOffset = curtilex * b->tTileSize.c
						+ curtiley * b->tTileSize.r * b->uPitch;



				}
				uRunningBandOffset += b->uPitch
					* ((b->tBandSize.r + 0xf) & ~0xf) + BANDDELTA;

			/*	Reset the band dropping info for the plane */
				b->uBandFlags = 0;

			}	/* if (bNewFrameSize || bNewBanding || bNewTileSize) */
		}	/* for i */
	}	/* for p */

	if (bNewFrameSize || bNewBanding || bNewTileSize) {
		if (pCntx->Plane[2].pBand) {
			HiveLocalFreePtr(pCntx->Plane[2].pBand);
		}
		pCntx->Plane[2] = pCntx->Plane[1];
		pCntx->Plane[2].uPlaneOffset = uRunningBandOffset;
		pCntx->Plane[2].pBand = HiveLocalAllocPtr(pCntx->Plane[1].uNBands*sizeof(RtBandSt), 0);
		if (!pCntx->Plane[2].pBand) {
			RET_ERR(return) PIA_S_OUT_OF_MEMORY;
		}
		uRunningBandOffset += pCntx->Plane[2].uPlaneOffset;
		uRunningBandOffset -= pCntx->Plane[1].uPlaneOffset;
	}	/* if new banding */
	for (i = 0; i < pCntx->Plane[1].uNBands; i++) {
		pCntx->Plane[2].pBand[i] = pCntx->Plane[1].pBand[i];
	}


	if (pCntx->u8GOPFlags & GOPF_XPAR) {
		code = readbits2(&pb, 4);
	/* pixel depth */
		if (code & 0x7) {
			RET_ERR(return) PIA_S_UNSUPPORTED_FUNCTION;
		}
	/* transparency color */
		if (code >> 3) {
			code = readbits2(&pb, 24);
			pCntx->uTransColor.u8Reserved = 0;
			pCntx->uTransColor.u8B = (U8)(code>>16);
			pCntx->uTransColor.u8G = (U8)(code>>8);
			pCntx->uTransColor.u8R = (U8)(code);
		}
	/* transparency color */
	}

	if (bNewFrameSize) {
		if (pCntx->YVUOutputStorage) {
			HiveLocalFreePtr(pCntx->YVUOutputStorage);
		}
#if B_HOST_IS_BENDIAN
		pCntx->YVUOutputStorage = HiveLocalAllocPtr(uYVUOutputSize+31, 1);
#else /* B_HOST_IS_BENDIAN */
		pCntx->YVUOutputStorage = HiveLocalAllocPtr(uYVUOutputSize+31, 0);
#endif /* B_HOST_IS_BENDIAN */
		if (!pCntx->YVUOutputStorage) {
			RET_ERR(return) PIA_S_OUT_OF_MEMORY;
		}

		pCntx->YVUAlignedOutput =
			(PU8)(((U32)pCntx->YVUOutputStorage+0x1f)&~0x1f);
	}

#define CLEAR_BANDS

	if (bNewBanding || bNewFrameSize || bNewTileSize) {
		PU32 pCur, pFwd, pBkwd;
#ifdef CLEAR_BANDS
		U32 i, uZero = 0x0;
#endif /* CLEAR_BANDS */

		pCntx->Plane[0].pOutput = pCntx->YVUAlignedOutput;
		pCntx->Plane[1].pOutput = pCntx->Plane[0].pOutput +
			pCntx->Plane[0].rDecoded.h * pCntx->Plane[0].uOutputPitch;
		pCntx->Plane[2].pOutput = pCntx->Plane[1].pOutput +
			pCntx->Plane[1].rDecoded.h * pCntx->Plane[1].uOutputPitch;

		if (pCntx->pPlane3Storage) {
			HiveLocalFreePtr(pCntx->pPlane3Storage);
		}
		uRunningBandOffset += 3616 - 2048;	/* for better l1 cache alignment */
	/*	Reset the Band Storage array, leaving some slop to ensure 32 byte
	 *	alignment
	 */
#ifdef CLEAR_BANDS
		pCntx->pPlane3Storage = HiveLocalAllocPtr(31+uRunningBandOffset*3, 0);
#else /* CLEAR_BANDS */
		pCntx->pPlane3Storage = HiveLocalAllocPtr(31+uRunningBandOffset*3, TRUE);
#endif /* CLEAR_BANDS */
		if (!pCntx->pPlane3Storage) {
			RET_ERR(return) PIA_S_OUT_OF_MEMORY;
		}
		pCntx->pFrameCurrent = (PU8)(((U32)pCntx->pPlane3Storage+0x1f)&~0x1f);
		pCntx->pFrameForward = pCntx->pFrameCurrent + uRunningBandOffset;
		pCntx->pFrameBackward = pCntx->pFrameForward + uRunningBandOffset;
/*		pCntx->uBSize = uRunningBandOffset;	*/ /* size of one buffer */
		pCur = (PU32) pCntx->pFrameCurrent;
		pFwd = (PU32) pCntx->pFrameForward;
		pBkwd = (PU32) pCntx->pFrameBackward;
#ifdef CLEAR_BANDS
		for (i = 0; i < uRunningBandOffset; i+=4) {
			*pCur = uZero;
			*pFwd = *pCur;
			*pBkwd = *pFwd++;
			*pCur++ = *pBkwd++;
		}
#endif /* CLEAR_BANDS */
	}

/*	(Re)Allocate pCntx->vu8DecodeTiles - vector of bytes that determine whether
 *	tiles should be decoded or not.
 */

	if (bNewTileSize) {
		if (pCntx->vu8DecodeTiles) {
			HiveLocalFreePtr(pCntx->vu8DecodeTiles);
		}
		pCntx->vu8DecodeTiles = HiveLocalAllocPtr(uNTiles, 0);
		if (!pCntx->vu8DecodeTiles) {
			RET_ERR(return) PIA_S_OUT_OF_MEMORY;
		}
	}

/*	Handle Re-sizing of Decode Rectangle
 */
	bNewDecodeRect = (rDecodeRect.r != pCntx->rDecodeRect.r)
		|| (rDecodeRect.c != pCntx->rDecodeRect.c)
		|| (rDecodeRect.h != pCntx->rDecodeRect.h)
		|| (rDecodeRect.w != pCntx->rDecodeRect.w);
	if (bNewDecodeRect || bNewTileSize) {
		U32 r,c;
		U32 rCurTile,cCurTile;
		Boo bTileIntersectsRow;
	/*	store possibly new decode rect */
		pCntx->rDecodeRect = rDecodeRect;

	/*	(re?)compute intersection of tiles with decode rect
	 */
	 	rCurTile = i = 0;
	 	for (r = 0; r < pCntx->tNTiles.r; r++) {
			bTileIntersectsRow = (rCurTile < rDecodeRect.r + rDecodeRect.h) &&
				(rCurTile+(I32)pCntx->tTileSize.r > rDecodeRect.r);
			cCurTile = 0;
			for (c = 0; c < pCntx->tNTiles.c; c++) {
				pCntx->vu8DecodeTiles[i++] = bTileIntersectsRow &&
					(cCurTile < rDecodeRect.c + rDecodeRect.w) &&
					(cCurTile+(I32)pCntx->tTileSize.c > rDecodeRect.c);
				cCurTile += pCntx->tTileSize.c;
			}
			rCurTile += pCntx->tTileSize.r;
		}
#ifdef DEBUG
		if (timefile) {
			fprintf(timefile,"Tiles:");
			for (i = 0; i < uNTiles; i++) {
				fprintf(timefile,"%2d",pCntx->vu8DecodeTiles[i]);
			}
			fprintf(timefile,"\n");
		}
#endif /* DEBUG */
	}

/*  Version
 */
	if (pb.bitsread&0x7) {
		U32 r = 8-(pb.bitsread&7);
		code = readbits2(&pb, r);
	}

	code = readbits2(&pb, 24);
{
U32 platformid;
U32 buildnumber;
U32 encoderid;
U32 chainbit;
U32 toolindex;

	platformid = code & 0xff;
	buildnumber = (U8)(code >> 8);
	encoderid = (code >> 16) & 0x7;
	pCntx->u8BSVersion = (U8)((code >> 19) & 0xf);
	chainbit = code >> 23;
	while (chainbit) {
		code = readbits2(&pb, 16);
		toolindex = code & 0x7f;
		buildnumber = (code >> 7) & 0xff;
		chainbit = code >> 15;
	}
}

decodepictureheader:

	bIsAccessKeyInBs = pCntx->u8GOPFlags & GOPF_LOCKWORD;

	if (pb.bitsread&0x7) {
		U32 r = 8-(pb.bitsread&7);
		code = readbits2(&pb, r);
	}

/*	Picture Flags */
	code = readbits2(&pb, 8);
	pCntx->u8PicFlags = (U8)code;

/*  DataSize
 */
	pCntx->uDataSize = 0;
	if (pCntx->u8PicFlags & PICF_DATASIZE) {
		code = readbits2(&pb, 24);
		pCntx->uDataSize = code;
	}

/*	Picture Level Iheritance */
	if (pCntx->u8PicFlags & PICF_INHER_TMV) {
		pCntx->bInheritTypeMV = TRUE;
	}
	else pCntx->bInheritTypeMV = FALSE;

	if (pCntx->u8PicFlags & PICF_INHER_Q) {
		pCntx->bInheritQ = TRUE;
	}
	else pCntx->bInheritQ = FALSE;

/*  Picture Checksum
 */
	pCntx->uFrameCheckSum = 0;
	if (pCntx->u8PicFlags & PICF_XSUM) {
		code = readbits2(&pb, 16);
		pCntx->uFrameCheckSum = code;
	}

	if (bIsAccessKeyInBs) {
		U32 hash;
	/*	Key frame Locking enabled:  get code */

		if (!pCntx->uFrameType && !pCntx->bKeyFailure) {
			hash = (((pCntx->uFrameCheckSum) + (pCntx->uAccessKey & 0xff00) + 1) *
				((pCntx->uAccessKey >> 16) + (((pCntx->uAccessKey & 0xff) << 8) + 1)));
			hash = (hash / ((hash % ((pCntx->uAccessKey & 0xff) + 1)) + 1));
			hash = (hash * ((hash % ((pCntx->uFrameCheckSum & 0xff00) + 1)) + 1));

			pCntx->bKeyFailure = (uBsAccessKey != hash);
		}
		if (pCntx->bKeyFailure) {
#ifdef NEW_ACCESS_KEY_SUPPORT
			ret = PIA_S_KEY_FAILURE;
#else /* NEW_ACCESS_KEY_SUPPORT */
			RET_ERR(return) PIA_S_KEY_FAILURE;
#endif /* NEW_ACCESS_KEY_SUPPORT */
		}

	}

/*  Picture Extensions
 */
	if (pCntx->u8PicFlags & PICF_X10NN) {
		U32 uNXBytes;
		do {
			uNXBytes = readbits2(&pb, 8);
			for (i = 0; i < uNXBytes; i++) {
				/* tbd store extensions here */
				/* code = */ readbits2(&pb, 8);
			}
		} while (uNXBytes);
	}

/*	Read MacroBlock Huffman Table Index
 */
	if (pCntx->u8PicFlags & PICF_MBHUFF) {
		code = readbits2(&pb, 3);
		if (code != 0x7) {
			pCntx->MBHuffTab = &pCntx->MBHuffTabList[code];
		}
		else {	/* escaped table */
			Boo					bNewPBTable;
			U32					uNPBCodes;
			PBDESCRIPTOR_TYPE	TempPBTable[17];
			HUFFTAB				*h;

			pCntx->MBHuffTab = &pCntx->MBHuffTabList[8+pCntx->uFrameType];

		/*	Parse PB style descriptor */
			uNPBCodes = readbits2(&pb, 4);
			TempPBTable[0] = (PBDESCRIPTOR_TYPE)uNPBCodes;
			for (i = 1; i<=uNPBCodes; i++) {
				code = readbits2(&pb, 4);
				TempPBTable[i] = (PBDESCRIPTOR_TYPE)code;
			}

			bNewPBTable = FALSE;
			h = &pCntx->MBHuffTabList[8+pCntx->uFrameType];
			for (i = 0; i <= (U32)TempPBTable[0]; i++) {
				if (TempPBTable[i] != h->descriptor[i]) {
					bNewPBTable = TRUE;
				}
				h->descriptor[i] = TempPBTable[i];
			}
			if (bNewPBTable) {
				BuildPBHuffTab(h);
			}
		}
	}
	else {	/* use default */
		pCntx->MBHuffTab = &pCntx->MBHuffTabList[7];
	}

/*  Picture Clamping Hints
 */
	code = readbits2(&pb, 3);
	pCntx->uDecoderHints = code;

end:

	if ((pCntx->uFlags & PL_CODE_TRANSP) ||
		(rViewRect.r | rViewRect.c | (rViewRect.h - pCntx->tFrameSize.r) |
		(rViewRect.w - pCntx->tFrameSize.c))) {
		/*	probably want to make a general routine to perform this
		 *	type of operation.
		 */
		U32 uMaskSize;

		uNTiles = pCntx->tNTiles.r * pCntx->tNTiles.c;
		uMaskSize = uNTiles * pCntx->tTileSize.r
			* ((pCntx->tTileSize.c + 0x1f) & ~0x1f) >> 3;

		if ((uMaskSize > pCntx->uMaskSize)
/*			|| ((uMaskSize << 1) < pCntx->uMaskSize) */	) {
			if (pCntx->pTranspMaskCurrent)
				HiveLocalFreePtr(pCntx->pTranspMaskCurrent);

			if (pCntx->pLDMaskStorage)
				HiveLocalFreePtr(pCntx->pLDMaskStorage);
			pCntx->pTranspMaskCurrent = HiveLocalAllocPtr(uMaskSize, 0);
			if (!pCntx->pTranspMaskCurrent) {
				RET_ERR(return) PIA_S_OUT_OF_MEMORY;
			}

			pCntx->pLDMaskStorage = HiveLocalAllocPtr(uMaskSize, 0);
			if (!pCntx->pLDMaskStorage) {
				RET_ERR(return) PIA_S_OUT_OF_MEMORY;
			}
			pCntx->uMaskSize = uMaskSize;
/*	LEW: error checking - need to see if any of these pointers are null 
	Also, should make sure these are aligned & consolodate the allocation
 */
		}
	}

	*bytes = bytesread(pntr, &pb);
	return ret;
} /* DecodePictureLayer */












/*
 * DecBufferSetup
 *
 * Allocates and initializes context and data buffers as DecodePictureLayer,
 * except uses input arguments instead of picture header. Purpose is to do the
 * allocation prior to the first frame to speed decoding of the first frame.
 * When the actual first frame differs from what is set up by this function,
 * DecodePictureLayer will reinitialize. Because the purpose of this function
 * is preallocation, it does nothing if allocs have previously been done.
 *
 * When a memory alloc fails in this function, the context information which
 * DecodePictureLayer checks is reset to ensure DecodePictureLayer attempts
 * to reinitialize.
 *
 * Notes:
 * 1. uNYBands must be 1 or 4
 * 2. assumes one band per UV plane
 *.
 */

PIA_RETURN_STATUS DecBufferSetup(
pRTDecInst pCntx,
PointSt tFrameSize,
PointSt tTileSize,
U32 uNYBands,
/*	U32 uNVUBands, */
RectSt rDecodeRect,
PointSt tUVSubsample,
Boo bTransparency,
Boo bBFrames,
Boo bDeltaFrames
/*	Boo bYVU12 */
) {

	U32		i;
	U32		uYVUOutputSize;
	U32		uNTiles;
	U32		uNBands[2];
	U32		uRunningBandOffset;
	Boo		bYVU12 = FALSE;
	U32		plane;
	PIA_RETURN_STATUS	ret = PIA_S_OK;

	/* input arg check */
	if ( uNYBands != 1 && uNYBands != 4 ) {
		return PIA_S_ERROR;
	}

	/* do nothing if allocs previously done */
	if ( pCntx->pPlane3Storage ) {
		return PIA_S_OK;
	}

	uNBands[0] = uNYBands;
	uNBands[1] = 1 /* uNVUBands */;

/*	Frame Size
 */
	pCntx->tFrameSize = tFrameSize;

/*	VU size
 */
	pCntx->tVUFrameSize.c = (pCntx->tFrameSize.c + tUVSubsample.c - 1) / tUVSubsample.c;
	pCntx->tVUFrameSize.r = (pCntx->tFrameSize.r + tUVSubsample.r - 1) / tUVSubsample.r;


/*	Tile Size
 */

	pCntx->tTileSize = tTileSize;

/*	Compute the number of tiles here
 */
	pCntx->tNTiles.c = pCntx->tFrameSize.c + pCntx->tTileSize.c - 1;
	pCntx->tNTiles.r = pCntx->tFrameSize.r + pCntx->tTileSize.r - 1;
	pCntx->tNTiles.c /= pCntx->tTileSize.c;
	pCntx->tNTiles.r /= pCntx->tTileSize.r;
	uNTiles = pCntx->tNTiles.r * pCntx->tNTiles.c;


/*	Determine the decoded portions of the planes.
 */
	pCntx->Plane[0].rDecoded.r = 0;
	pCntx->Plane[0].rDecoded.c = 0;
	pCntx->Plane[0].rDecoded.h = pCntx->tFrameSize.r;
	pCntx->Plane[0].rDecoded.w = pCntx->tFrameSize.c;

	pCntx->Plane[1].rDecoded.r = 0;
	pCntx->Plane[1].rDecoded.c = 0;
	pCntx->Plane[1].rDecoded.h = pCntx->tVUFrameSize.r;
	pCntx->Plane[1].rDecoded.w = pCntx->tVUFrameSize.c;
	pCntx->Plane[2].rDecoded = pCntx->Plane[1].rDecoded;

	pCntx->Plane[0].uOutputPitch = (pCntx->Plane[0].rDecoded.w + 0x1f) & ~0x1f;
	pCntx->Plane[1].uOutputPitch = (pCntx->Plane[1].rDecoded.w + 0x1f) & ~0x1f;
	pCntx->Plane[2].uOutputPitch = (pCntx->Plane[2].rDecoded.w + 0x1f) & ~0x1f;

	uYVUOutputSize =
		pCntx->Plane[0].rDecoded.h * pCntx->Plane[0].uOutputPitch +
		pCntx->Plane[1].rDecoded.h * pCntx->Plane[1].uOutputPitch +
		pCntx->Plane[2].rDecoded.h * pCntx->Plane[2].uOutputPitch;


	uRunningBandOffset = 0;


	for (plane = 0; plane < 2; plane++)	{
		const U8 XDefault[2][4] = {
			{ S8x8, S8x1, S1x8, N8x8 },
			{ S4x4, S4x1, S1x4, N4x4 }
		};

		pRtPlaneSt p;
		p = &pCntx->Plane[plane];

		/*if (bNewFrameSize || bNewBanding || bNewTileSize) */
		{
			p->uPlaneOffset = uRunningBandOffset;
			if (!plane) {
				if (pCntx->pBlockInfo) {
					HiveLocalFreePtr(pCntx->pBlockInfo);
				}
				if (pCntx->pBlockInfoBand0) {
					HiveLocalFreePtr(pCntx->pBlockInfoBand0);
				}
			/*	can make this smaller if move the tile loop lower & correctly */
			/*	compute max band size, and double check this, of course */
				i = ((p->rDecoded.h * (p->rDecoded.w+0xf) >> 4) + uNTiles - 1)
					/ uNTiles;
				pCntx->pBlockInfo = HiveLocalAllocPtr(i*sizeof(BlockInfoSt), TRUE);
				if (!pCntx->pBlockInfo) {
					goto error_end;/*RET_ERR(return) PIA_S_OUT_OF_MEMORY; */
				}
			}	/* !plane */

			if (p->pBand) {
				pRtBandSt b;
				for (i = 0; i < p->uNBands; i++) {
					b = &p->pBand[i];
					if (b->pTile) {
						HiveLocalFreePtr(b->pTile);
					}
				}
				HiveLocalFreePtr(p->pBand);
			}
			p->uNBands = uNBands[plane];
			p->pBand = HiveLocalAllocPtr(p->uNBands*sizeof(RtBandSt), 0);
			if (!p->pBand) {
				goto error_end;/*RET_ERR(return) PIA_S_OUT_OF_MEMORY; */
			}
		}

	/*	Initialize the band structures 
	 */
		for (i = 0; i < p->uNBands; i++) {
			pRtBandSt	b;
			U32			uTile;
			U32			uMaxBlockInfoInBand;
			const U8	x2z[8]  = { 0, 1, 2,  2,  3,  4,  5,  5 };
			const U32	MBSizeTab[4] = { 16, 8, 8, 4 };
			const U32	BlockSizeTab[4] = { 8, 8, 4, 4 };

			b = &p->pBand[i];

			/*if (bNewFrameSize || bNewBanding || bNewTileSize)*/ {
				b->uBandOffset = uRunningBandOffset - p->uPlaneOffset;

			/*	Tiles */
				if (p->uNBands == 1) {
					b->tBandSize.r = p->rDecoded.h;
					b->tBandSize.c = p->rDecoded.w;
				}
				else if (p->uNBands == 4) {
					b->tBandSize.r = p->rDecoded.h >> 1;
					b->tBandSize.c = p->rDecoded.w >> 1;
				}
				else if (p->uNBands == 7) {
					if (i < 4) {
						b->tBandSize.r = p->rDecoded.h >> 2;
						b->tBandSize.c = p->rDecoded.w >> 2;
					}
					else {
						b->tBandSize.r = p->rDecoded.h >> 1;
						b->tBandSize.c = p->rDecoded.w >> 1;
					}
				}

				b->tTileSize.r = pCntx->tTileSize.r /
					(pCntx->tFrameSize.r / b->tBandSize.r);
				b->tTileSize.c = pCntx->tTileSize.c /
					(pCntx->tFrameSize.c / b->tBandSize.c);

				b->tTileSize.c <<= 1;
				b->tBandSize.c <<= 1;
			/*	Round up Pitch to a multiple of 32 - a requirement of our dual
			 *	interleaved block band storage.
			 */
				b->uPitch = (b->tBandSize.c + 0x1f) & ~0x1f;

				if (!(i||plane)) {
				/*	Allocate enough storage for worst case - 4x4 blocks, leaving
				 *	7 extra blockinfo elements for padding - assumed to be there
				 *	by the block decode
				 */
					uMaxBlockInfoInBand = uNTiles * 7 +
						(b->tBandSize.r * (b->tBandSize.c+0x1f) >> (4 + 1));
					pCntx->pBlockInfoBand0 = HiveLocalAllocPtr((uMaxBlockInfoInBand)
						* sizeof(BlockInfoSt), TRUE);
					if (!pCntx->pBlockInfoBand0) {
						goto error_end;/*RET_ERR(return) PIA_S_OUT_OF_MEMORY; */
					}
				}

				b->pTile = HiveLocalAllocPtr(sizeof(RtTileSt) * uNTiles, 0);
				if (!b->pTile) {
					goto error_end;/*RET_ERR(return) PIA_S_OUT_OF_MEMORY; */
				}
				for (uTile = 0; uTile < uNTiles; uTile++) {
					pRtTileSt	t;
					U32		curtilex, curtiley;

					curtilex = uTile % pCntx->tNTiles.c;
					curtiley = uTile / pCntx->tNTiles.c;

					t = &b->pTile[uTile];
#if 0 /* LEW TEST */
					t->tNTileMBs.r = t->tNTileMBs.c = 0;
					t->tNTileBlocks.r = t->tNTileBlocks.c = 0;
#endif /* LEW TEST */
				/*	double check this... */
					t->pBlockInfo = (i||plane) ? pCntx->pBlockInfo
						: &pCntx->pBlockInfoBand0[uTile*uMaxBlockInfoInBand/uNTiles];
					t->uTileOffset = curtilex * b->tTileSize.c
						+ curtiley * b->tTileSize.r * b->uPitch;
				}
				uRunningBandOffset += b->uPitch
					* ((b->tBandSize.r + 0xf) & ~0xf) + BANDDELTA;

			/*	Reset the band dropping info for the plane */
				b->uBandFlags = 0;

			}	/* if (bNewFrameSize || bNewBanding || bNewTileSize) */
		}	/* for i */
	}	/* for p */

	/*if (bNewFrameSize || bNewBanding || bNewTileSize)*/ {
		if (pCntx->Plane[2].pBand) {
			HiveLocalFreePtr(pCntx->Plane[2].pBand);
		}
		pCntx->Plane[2] = pCntx->Plane[1];
		pCntx->Plane[2].uPlaneOffset = uRunningBandOffset;
		pCntx->Plane[2].pBand = HiveLocalAllocPtr(pCntx->Plane[1].uNBands*sizeof(RtBandSt), 0);
		if (!pCntx->Plane[2].pBand) {
			goto error_end;/*RET_ERR(return) PIA_S_OUT_OF_MEMORY; */
		}
		uRunningBandOffset += pCntx->Plane[2].uPlaneOffset;
		uRunningBandOffset -= pCntx->Plane[1].uPlaneOffset;
	}	/* if new banding */
	for (i = 0; i < pCntx->Plane[1].uNBands; i++) {
		pCntx->Plane[2].pBand[i] = pCntx->Plane[1].pBand[i];
	}



	/*if (bNewFrameSize)*/ {
		if (pCntx->YVUOutputStorage) {
			HiveLocalFreePtr(pCntx->YVUOutputStorage);
		}
		pCntx->YVUOutputStorage = HiveLocalAllocPtr(uYVUOutputSize+31, 1);
		if (!pCntx->YVUOutputStorage) {
			goto error_end;/*RET_ERR(return) PIA_S_OUT_OF_MEMORY; */
		}
		pCntx->YVUAlignedOutput =
			(PU8)(((U32)pCntx->YVUOutputStorage+0x1f)&~0x1f);
	}

#define CLEAR_BANDS

	/* if (bNewBanding || bNewFrameSize || bNewTileSize) */ {
		PU32 pCur, pFwd, pBkwd;
#ifdef CLEAR_BANDS
		U32 i, uZero = 0x0;
#endif /* CLEAR_BANDS */

		pCntx->Plane[0].pOutput = pCntx->YVUAlignedOutput;
		pCntx->Plane[1].pOutput = pCntx->Plane[0].pOutput +
			pCntx->Plane[0].rDecoded.h * pCntx->Plane[0].uOutputPitch;
		pCntx->Plane[2].pOutput = pCntx->Plane[1].pOutput +
			pCntx->Plane[1].rDecoded.h * pCntx->Plane[1].uOutputPitch;

		if (pCntx->pPlane3Storage) {
			HiveLocalFreePtr(pCntx->pPlane3Storage);
		}
		uRunningBandOffset += 3616 - 2048;	/* for better l1 cache alignment */
	/*	Reset the Band Storage array, leaving some slop to ensure 32 byte
	 *	alignment
	 */
#ifdef CLEAR_BANDS
		pCntx->pPlane3Storage = HiveLocalAllocPtr(31+uRunningBandOffset*3, 0);
#else /* CLEAR_BANDS */
		pCntx->pPlane3Storage = HiveLocalAllocPtr(31+uRunningBandOffset*3, TRUE);
#endif /* CLEAR_BANDS */
		if (!pCntx->pPlane3Storage) {
			goto error_end;/*RET_ERR(return) PIA_S_OUT_OF_MEMORY; */
		}
		pCntx->pFrameCurrent = (PU8)(((U32)pCntx->pPlane3Storage+0x1f)&~0x1f);
		pCntx->pFrameForward = pCntx->pFrameCurrent + uRunningBandOffset;
		pCntx->pFrameBackward = pCntx->pFrameForward + uRunningBandOffset;
/*		pCntx->uBSize = uRunningBandOffset;	*/ /* size of one buffer */
		pCur = (PU32) pCntx->pFrameCurrent;
		pFwd = (PU32) pCntx->pFrameForward;
		pBkwd = (PU32) pCntx->pFrameBackward;
#ifdef CLEAR_BANDS
		for (i = 0; i < uRunningBandOffset; i+=4) {
			*pCur = uZero;
			*pFwd = *pCur;
			*pBkwd = *pFwd++;
			*pCur++ = *pBkwd++;
		}
#endif /* CLEAR_BANDS */
	}

/*	(Re)Allocate pCntx->vu8DecodeTiles - vector of bytes that determine whether
 *	tiles should be decoded or not.
 */

	/* if (bNewTileSize) */ {
		if (pCntx->vu8DecodeTiles) {
			HiveLocalFreePtr(pCntx->vu8DecodeTiles);
		}
		pCntx->vu8DecodeTiles = HiveLocalAllocPtr(uNTiles, 0);
		if (!pCntx->vu8DecodeTiles) {
			goto error_end;/*RET_ERR(return) PIA_S_OUT_OF_MEMORY; */
		}
	}

#if 0
/*	Handle Re-sizing of Decode Rectangle
 */
	bNewDecodeRect = (rDecodeRect.r != pCntx->rDecodeRect.r)
		|| (rDecodeRect.c != pCntx->rDecodeRect.c)
		|| (rDecodeRect.h != pCntx->rDecodeRect.h)
		|| (rDecodeRect.w != pCntx->rDecodeRect.w);
#endif /*0 */
	/*if (bNewDecodeRect || bNewTileSize)*/ {
		U32 r,c;
		U32 rCurTile,cCurTile;
		Boo bTileIntersectsRow;
	/*	store possibly new decode rect */
		pCntx->rDecodeRect = rDecodeRect;

	/*	(re?)compute intersection of tiles with decode rect
	 */
	 	rCurTile = i = 0;
	 	for (r = 0; r < pCntx->tNTiles.r; r++) {
			bTileIntersectsRow = (rCurTile < rDecodeRect.r + rDecodeRect.h) &&
				(rCurTile+(I32)pCntx->tTileSize.r > rDecodeRect.r);
			cCurTile = 0;
			for (c = 0; c < pCntx->tNTiles.c; c++) {
				pCntx->vu8DecodeTiles[i++] = bTileIntersectsRow &&
					(cCurTile < rDecodeRect.c + rDecodeRect.w) &&
					(cCurTile+(I32)pCntx->tTileSize.c > rDecodeRect.c);
				cCurTile += pCntx->tTileSize.c;
			}
			rCurTile += pCntx->tTileSize.r;
		}
	}

/*end: */

	if ( bTransparency /*(pCntx->uFlags & PL_CODE_TRANSP) ||
		(rViewRect.r | rViewRect.c | (rViewRect.h - pCntx->tFrameSize.r) |
		(rViewRect.w - pCntx->tFrameSize.c))*/ ) {
		/*	probably want to make a general routine to perform this
		 *	type of operation.
		 */
		U32 uMaskSize;

		uNTiles = pCntx->tNTiles.r * pCntx->tNTiles.c;
		uMaskSize = uNTiles * pCntx->tTileSize.r
			* ((pCntx->tTileSize.c + 0x1f) & ~0x1f) >> 3;

		if ((uMaskSize > pCntx->uMaskSize)
/*			|| ((uMaskSize << 1) < pCntx->uMaskSize) */	) {
			if (pCntx->pTranspMaskCurrent)
				HiveLocalFreePtr(pCntx->pTranspMaskCurrent);

			if (pCntx->pLDMaskStorage)
				HiveLocalFreePtr(pCntx->pLDMaskStorage);
			pCntx->pTranspMaskCurrent = HiveLocalAllocPtr(uMaskSize, 0);
			if (!pCntx->pTranspMaskCurrent) {
				goto error_end;/*RET_ERR(return) PIA_S_OUT_OF_MEMORY; */
			}

			pCntx->pLDMaskStorage = HiveLocalAllocPtr(uMaskSize, 0);
			if (!pCntx->pLDMaskStorage) {
				goto error_end;/*RET_ERR(return) PIA_S_OUT_OF_MEMORY; */
			}
			pCntx->uMaskSize = uMaskSize;
/*	LEW: error checking - need to see if any of these pointers are null 
	Also, should make sure these are aligned & consolodate the allocation
 */
		}
	}

	return ret;


error_end:
	/* mem alloc failure; reset context for later DecodePictureLayer attempt */
	pCntx->tFrameSize.c = 0;
	pCntx->tFrameSize.r = 0;
	pCntx->tTileSize.c = 0;
	pCntx->tTileSize.r = 0;
	pCntx->rDecodeRect.w = 0;
	pCntx->rDecodeRect.h = 0;
	return PIA_S_OUT_OF_MEMORY;

} /* DecodePictureLayer */
