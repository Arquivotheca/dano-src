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
 * DECBNDH.C -- function declarations and definitions needed for
 * processing the picture layer.
 *************************************************************************/

#include "datatype.h"
#include "pia_main.h"
#include "cpk_blk.h"
#include "xpardec.h"
#include "rtdec.h"
#include "decbndh.h"
#include "readbits.h"
#include "rvmptbl.h"
#include "decpl.h"
#include <string.h>	/* for memcpy proto */

#ifdef DEBUG
#include <stdio.h>
extern FILE * timefile;
extern Boo bPrintBandInfo;
#endif /* DEBUG */

extern const PBDESCRIPTOR_TYPE BlockStaticHuff[9][17];

const U32 MBSizeTabExp[4] = { 2, 4, 8, 16 };


void ZeroBand(pRTDecInst pCntx, U32 plane, U32 band) {
	register U32 c;
	register PU32 bcptr;
	register U32 zero;
	register U32 r;
	PU32 brptr;
	pRtBandSt b;

	b = &pCntx->Plane[plane].pBand[band];
	switch (band) {
		case 0:	zero = 0x00000000;	break;
		case 1:	zero = 0x00010001;	break;
		case 2:	zero = 0x00030003;	break;
		case 3:	zero = 0x00020002;	break;
	}

/*	TBD actually loop through the tiles, testing if each has */
/*	a non-NULL pointer (NULL pointers signal that the tile is not */
/*	in the decode rect. */
	brptr = (PU32)(pCntx->pFrameCurrent +
		pCntx->Plane[plane].uPlaneOffset +
		b->uBandOffset +
		b->pTile[0/*uNTilesRead*/].uTileOffset);
	for (r = 0; r < b->tBandSize.r; r++) {
		bcptr = brptr;
		for (c = 0; c < b->uPitch; c += 4) {
			*bcptr++ = zero;
		}
		brptr += (b->uPitch >> 2);
	}
}


U32 DecodeBandHeader(pRTDecInst pCntx, PU8 pntr, PU32 bytesread, U32 plane, U32 band) {
	bitbufst	p;
	U32 		code;

	pRtBandSt	b;

	U32 		i;

	U32			uNPBCodes;
	U32			ret = 0;
	U32			uNTiles;
	PU16		pu16Scramble = (PU16) (pntr + 4);  /* Pointer for descrambling */
 
	b = &pCntx->Plane[plane].pBand[band];

	readbitsinit2(&p, pntr);

/*	Read Band Flags */
	code = readbits2(&p, 8);
	b->u8BandFlags = (U8) code;

/*	Read Band Empty Flag */
	if (b->u8BandFlags & BANDF_DROPPED) {
		b->uBandFlags |= BF_DROPPED;
		ret=(U32)-1;
		goto end;
	}
	if (pCntx->uFrameType == PICTYPE_K) {
		b->uBandFlags &= ~(BF_REF_DROPPED|BF_DROPPED);
	}
	else {
		b->uBandFlags &= ~BF_DROPPED;
	}

/*	Read Band Data Size */
	b->uDataSize = 0;
#define PICF_BANDDATASZ 0x80	/* from decpl.h */
    if (pCntx->u8PicFlags & PICF_BANDDATASZ) {
		code = readbits2(&p, 24);
		b->uDataSize = code;
	} else if ((plane == 0) && /* Sanity check, band data size must be */
	           (band == 0) &&  /* present in this case. */
			   (pCntx->uFrameType == PICTYPE_K) &&
			   (pCntx->u8GOPFlags & GOPF_LOCKWORD)) {
			goto end;
	}


/*	InheritTypeMV */
	if (b->u8BandFlags & BANDF_INTMV) {
		b->bInheritTypeMV = 1;
	}
	else {
		b->bInheritTypeMV = 0;
	}

/*	InheritQ */
	if (b->u8BandFlags & BANDF_INHQ) {
		b->bInheritQ = 1;
	}
	else {
		b->bInheritQ = 0;
	}

/*	InheritQ */
	if (b->u8BandFlags & BANDF_QDELTA) {
		b->bQDelta = 1;
	}
	else {
		b->bQDelta = 0;
		b->bInheritQ = 1;	/* fake inheritq to avoid reading bits */
	}


/* Descramble band data when frame lock is on and this is band 0 of */
/* Y plane of Key frame. */
 
	if ((plane == 0) && /* Sanity check, band data size must be */
	    (band == 0) &&  /* present in this case. */
		(pCntx->uFrameType == PICTYPE_K) &&
		(pCntx->u8GOPFlags & GOPF_LOCKWORD)) {

		U32 uCount, uKeyValue, uMultiplier, uScrambleLen;
		PU8 pu8Temp = (PU8) pu16Scramble;

		/* Sanity Check on byte alignment */
		if (p.bitsread & 0x7 != 0) {
			goto end;
		}
								
		if (b->uDataSize >= 68)
			uScrambleLen = 64;
		else
			uScrambleLen = ((b->uDataSize - 4) >> 1) << 1;

		uKeyValue = (0xffffffff - pCntx->uAccessKey) >> 16;

		uMultiplier = (((0x4000 + ((((pCntx->uAccessKey & 0xff) << 8) +
			                         ((pCntx->uAccessKey & 0xff00) >> 8)) >> 1))
						              / 200) * 200) + 21;
				
		/* Save scrambled data to return to bitstream after frame is decoded */
		pCntx->uScrambleLength = uScrambleLen;
		pCntx->pu8ScrambleStart = (PU8) pu16Scramble;
		memcpy(pCntx->u8ScrambledData, pCntx->pu8ScrambleStart, pCntx->uScrambleLength);
		for (uCount = 0; uCount < uScrambleLen; uCount += 2) {
			uKeyValue = ((uKeyValue * uMultiplier) + 1) % 0x10000;
			*pu16Scramble = *pu16Scramble ^ (U16)
#if B_HOST_IS_BENDIAN
									( ((uKeyValue & 0xff) << 8) | ((uKeyValue & 0xff00) >> 8) )
#else
									uKeyValue
#endif
			;
			pu16Scramble++;
		}

		/* Have to reinitialize the readbits vars to reload the */
		/* freshly descrambled bytes. */

		readbitsinit2(&p, pu8Temp);
	} else {
		pCntx->uScrambleLength = 0;
	}


/*	RV Huff Map Change List */
	if (b->u8BandFlags & BANDF_RVCHANGE) {
		U32 uNumChanges;
		uNumChanges = readbits2(&p, 8);
		if (uNumChanges > 61) {
			goto end;
		}
		b->rvswap[0] = b->SetCntx.bs_rvswaplist[0] = (U8)uNumChanges;
		for (i = 0; i < uNumChanges; i++) {
			code = readbits2(&p, 8);
			b->rvswap[1+2*i] = b->SetCntx.bs_rvswaplist[1+2*i] = (U8)code;
			code = readbits2(&p, 8);
			b->rvswap[2+2*i] = b->SetCntx.bs_rvswaplist[2+2*i] = (U8)code;
		}		
	}
	else {
		b->rvswap[0] = b->SetCntx.bs_rvswaplist[0] = 0;
	}

/*  RV Map Table
 */
    b->uRVMapTable = 0+8; /* default */
    if (b->u8BandFlags & BANDF_RVMAP) {
		code = readbits2(&p, 3);
		b->uRVMapTable = code;
    }


/*	Block Huffman Table */
	if (b->u8BandFlags & BANDF_BLOCKHUFF) {
		code = readbits2(&p, 3);
		if (code == 0x7) {	/* escape table */
		/*	Parse PB style descriptor */
			uNPBCodes = readbits2(&p, 4);
			b->SetCntx.huffman[0] = (PBDESCRIPTOR_TYPE)uNPBCodes;
			for (i = 1; i<=uNPBCodes; i++) {
				code = readbits2(&p, 4);
				b->SetCntx.huffman[i] = (PBDESCRIPTOR_TYPE)code;
			}
		}
		else {	/* use a table from 0 - 6 */
			uNPBCodes = b->SetCntx.huffman[0] = BlockStaticHuff[code][0];
			for (i = 1; i <= uNPBCodes; i++) {
				b->SetCntx.huffman[i] = BlockStaticHuff[code][i];
			}
		}
	}
	else {	/* get default */
		uNPBCodes = b->SetCntx.huffman[0] = BlockStaticHuff[7][0];
		for (i = 1; i <= uNPBCodes; i++) {
			b->SetCntx.huffman[i] = BlockStaticHuff[7][i];
		}
	}


/*	Band Checksum */
	code = readbits2(&p, 1);
	if (code) {
		code = readbits2(&p, 16);
	}
	b->uCheckSum = code;

/*	Global Quant for Band */
	code = readbits2(&p, 5);
	b->uGlobalQuant = (U8)code;


/*  Band Extensions TBD
 */

    if (b->u8BandFlags & BANDF_X10NN) {
		/* Byte Align */
		
		if (p.bitsread&0x7) {
			U32 r = 8-(p.bitsread&7);
			code = readbits2(&p, r);
		}		
        do {
            U32 uNumBytes;

			code = readbits2(&p, 8);
            uNumBytes = code;
            for (i = 0; i < uNumBytes; i++) {
				code = readbits2(&p, 8);
            }
			code = readbits2(&p, 1);
        } while (code);
    }

	uNTiles = pCntx->tNTiles.r * pCntx->tNTiles.c;

	for (i = 0; i < uNTiles; i++) {
		pRtTileSt	t;
		U32		curtilex, curtiley;

		curtilex = i % pCntx->tNTiles.c;
		curtiley = i / pCntx->tNTiles.c;

		t = &b->pTile[i];

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
	}

	b->SetCntx.nabsval = (PI8)RVTblList[b->uRVMapTable].u8NAbsVal;
	b->SetCntx.invfreq = RVTblList[b->uRVMapTable].u8FreqOrder;

	ret++;

end:

#ifdef DEBUG
	if (timefile && bPrintBandInfo) {
		fprintf(timefile,"P%dB%d TQD:%d%d%d RV:%d, GQ:%d, N/I:%d/%d\n"
			,plane,band
			,b->bInheritTypeMV,b->bInheritQ,b->bQDelta
			,b->uRVMapTable
			,b->uGlobalQuant
			,b->SetCntx.nabsval,b->SetCntx.invfreq
		);
		for (i = 0; i < b->SetCntx.bs_rvswaplist[0]; i++) {
			fprintf(timefile,"(%d, %d) ",
				b->SetCntx.bs_rvswaplist[1+2*i],
				b->SetCntx.bs_rvswaplist[2+2*i]);
		}
		if ( b->SetCntx.bs_rvswaplist[0] ) {
			fprintf(timefile,"\n");
		}
	}
#endif /* DEBUG */

	*bytesread = bytesread(pntr, &p);
	return ret;
}
