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
#ifdef DEBUG
static char rcsid[] = "@(#) $Id: xpardec.c 1.2 1995/06/24 15:50:00 tw Exp $";
static char vcsid[] = "@(#) $Workfile:   xpardec.c  $ $Revision:   1.2  $";
#endif /* DEBUG */

#include "datatype.h"
#include "cpk_blk.h"
#include "xpardec.h"
#include "readbits.h"

#define INIT_MAGIC 0xBEEF

I32
XparCntxSet(XparCntx *have, XparSetCntx *want){

	int i, j;

	if(have->initialized == INIT_MAGIC)
		if(PBCodeBookMatches(have->hufftab.descriptor, want->hdesc))
			return 0;

	if(have->initialized != INIT_MAGIC){
		have->hufftab.maintable = &have->hufftab.mainstatic[0];
		have->hufftab.descriptor= &have->descriptor[0];
	}

	for(j = 0, i = *want->hdesc; j <= i; j++)
		have->hufftab.descriptor[j] = want->hdesc[j];

	if(PrivateBuildPBHuffTab(&have->hufftab) == -1)
		return -1;

	have->initialized = INIT_MAGIC;

	return 0;

}

static U32 fullwidth[] = { 0, 0xFFFFFFFF };

#define BSWAP(x)	\
	(((x) >> 24) | ((x) << 24) |	\
	(((x) >> 8) & 0x0000ff00) |	(((x) << 8) & 0x00ff0000))



/* Given a pointer to a buffer sufficient to hold a dword aligned bitmap
 * of given dimension, produce a byte aligned MSB first bitmap, 1bpp, with
 * clear bits outside the view rectangle, and set bits lying on or within
 * the view rectangle.  The area if any between the decode rect and the
 * view rect is defined to have clear bits, including the decode rect itself,
 * while the area outside the decode rect, if any, is undefined.
 *
 * Return a pointer to the byte containing the bit for the upper left pel
 * of the decode rectangle.
 *
 * The rectangles are assumed to be oriented
 * -> is plus X
 *  V is plus Y
 * With the frame of reference being the picture.
 *
 * The returned pointer is NULL in case of errors.
 *
 */

#ifdef C_PORT_KIT

PU8
MakeRectMask(
	PU8 pOut,
	U32 uXres,
	U32 uYres,
	/* I32 iStride,*/
	U32 uXOriginD,
	U32 uYOriginD,
	U32 uXExtentD,
	U32 uYExtentD,
	U32 uXOriginV,
	U32 uYOriginV,
	U32 uXExtentV,
	U32 uYExtentV) {
#pragma message("move stride to parameter")
I32 iStride = ((uXres + 31) & ~31) >> 3;

register I32	r,c;
register PU8	pDst;
register U32	uLeftViewBits;
register U32	uRightViewBits;
U32	uViewDwords;
PU8	pUpLeftOfDecodeRect;
PU8	pUpLeftOfViewRect;

	if ((uXOriginD | uXExtentD) & 31) {
		return 0;
	}
	
	if (!(uYres && uYExtentD && uYExtentV)) {
		return 0;
	}

	if ((uYExtentD < uYExtentV) || (uYres < uYExtentD)) {
		return 0;
	}

	if (!(uXres && uXExtentD && uXExtentV)) {
		return 0;
	}

#define Align32(x) (((x)+31)&~31)

	if ((uXExtentD < uXExtentV) || (Align32(uXres) < uXExtentD)) {
		return 0;
	}

	pUpLeftOfDecodeRect = pOut + iStride * uYOriginD + (uXOriginD >> 3);

/*	zero the decode rect */
	pDst = pUpLeftOfDecodeRect;
	for (r = uYExtentD; r > 0; r--) {
		for (c = (uXExtentD >> 3); c > 0; c -= 4) {
			*(PU32)(pDst+c-4) = 0;
		}
		pDst += iStride;
	}

	pUpLeftOfViewRect = pOut + iStride * uYOriginV + (uXOriginV >> 3);

/*	lew: is this a bug? what if extent is 1? */
	uViewDwords = ((uXExtentV + uXOriginV - 1) >> 5) - (uXOriginV >> 5);

	uLeftViewBits = (U32)(-1) >> (uXOriginV & 31);

#if B_HOST_IS_LENDIAN
	uLeftViewBits = BSWAP(uLeftViewBits);
#endif /* B_HOST_IS_LENDIAN */

	uRightViewBits = -1 << (32 - ((uXOriginV + uXExtentV) & 31));

#if B_HOST_IS_LENDIAN
	uRightViewBits = BSWAP(uRightViewBits);
#endif /* B_HOST_IS_LENDIAN */

	if (uViewDwords > 1) {

		pDst = (PU8)(((U32)pUpLeftOfViewRect & ~3) + 4);
		for (r = uYExtentV; r > 0; r--) {
			for (c = uViewDwords-1; c > 0; c--) {
				*(PU32)(pDst+c*4-4) = -1;
			}
			pDst += iStride;
		}
	}

	pDst = (PU8)((U32)pUpLeftOfViewRect & ~3);
	if (uViewDwords) {
		for (r = uYExtentV; r > 0; r--) {
			*(PU32)(pDst) = uLeftViewBits;
			*(PU32)(pDst+uViewDwords*4) = uRightViewBits;
			pDst += iStride;
		}
	}
	else {
		for (r = uYExtentV; r > 0; r--) {
			*(PU32)(pDst) = uLeftViewBits & uRightViewBits;
			pDst += iStride;
		}
	}
	return pUpLeftOfDecodeRect;

}
#endif /* C_PORT_KIT */


I32
DecodeTransparencyTile(
		pXparCntx context, pXparSetCntx proposed,
		PU8 pMask, I32 stride, PU8 pCopyOfMask, I32 c_stride,
		U32 uBMXres, U32 uPicYres, U32 uPicXres,
		U32 uXOrigin, U32 uYOrigin, U32 uXExtent, U32 uYExtent,
		PU8 pTileData, U8 bit_depth, U8 empty, U8 empty_state, U8 bxor) {


	int bitsused, total_pels, run, bitsinoutput, numstores;
	int somerun, remainingrun, lastbitsrun, middleruns, runs_parsed;

	bitbufst	p;
	pHuffTabSt	pHuff;
	U8  run_code, state, next_state, xor;
	PU8 output;
	U32 outbits32, pattern;
	PU32 aoutput, poutput;
	PU32 c_aoutput, c_poutput;

	if(context == NULL) return -1;
	if(proposed) if(XparCntxSet(context, proposed) == -1) return -1;
	if(uXOrigin & 31) return -1;
/*	if(uXExtent & 31) return -1; */
/*	strides must be multiple of 4 bytes */
	if ((stride | c_stride) & 0x3) {
		return -1;
	}
	uXExtent += 31;
	uXExtent &= ~31;

	if(bit_depth != 1) return -1;

	if((uYOrigin + uYExtent) > uPicYres){
		uYExtent = uPicYres % uYExtent;
	}

	/* Version 3.1 of bitstream spec: uXExtent is never
	 * modified, all off-image, in-tile pels are present.
	 */
	if((uXOrigin + uXExtent) > ((uPicXres + 31) & ~31)){
		uXExtent = ((uPicXres+31)&~31) % uXExtent;
	}

	total_pels = uXExtent * uYExtent;

	xor = 0;
	bitsused = 0;
	if(empty){
		state = empty_state;
	}
	else{
		readbitsinit2(&p, pTileData);
		pHuff = &context->hufftab;
		state = (U8)readbits2(&p, 1);
	}
	next_state = state ^ 1;

	output = pMask;
#if 0
	stride = ((uBMXres + 31)&~31)>>3;
#endif /* 0 */
	numstores = uXExtent>>5;
	output += uYOrigin * stride;
	output += uXOrigin >> 3;
	aoutput = (PU32) output;
	poutput = aoutput;
	outbits32 = 0;
	bitsinoutput = 0;
	stride /= 4;

	if(pCopyOfMask){
		c_aoutput = (PU32)(pCopyOfMask + (output - pMask));
		c_poutput = c_aoutput;
		c_stride /= 4;
	}
	else{
		c_aoutput = aoutput;
		c_poutput = poutput;
		c_stride = stride;
	}

	if((int)(output)&(3)) return -1;	/* error in logic */

	if(empty){
		pattern = fullwidth[state];
		while(uYExtent--){
			while(numstores--){
				*aoutput++ = pattern;
				*c_aoutput++ = pattern;
			}
			aoutput = poutput + stride;
			c_aoutput = c_poutput + c_stride;
			poutput = aoutput;
			c_poutput = c_aoutput;
			numstores = uXExtent>>5;
		}
		return 0;
	}

	runs_parsed = 0;
	while(total_pels > 0){

		run_code = (U8)readbits2h(&p, pHuff);
		runs_parsed++;

		if(run_code == 0) {
			run = 255; next_state = state;
		}
		else {
			run = run_code;
		}

		total_pels -= run;

		if(total_pels < 0)
			return -1;	/* error in data */

		if(run <= (32 - bitsinoutput)){ /* fits */
			if(run != 32){
				pattern = (state<<run)-state;
				pattern <<= (32 - bitsinoutput) - run;
			}
			else{
				pattern = fullwidth[state];
			}
			bitsinoutput += run;
			outbits32 |= pattern;
		}
		else{	/* doesn't */
			somerun = 32 - bitsinoutput;
			remainingrun = run - somerun;
			lastbitsrun = remainingrun & 31;
			middleruns = remainingrun & ~31;
			if(bitsinoutput){
				pattern = (state<<somerun)-state;
				pattern <<= (32-bitsinoutput)-somerun;
			}
			else{
				pattern = fullwidth[state];
			}
			outbits32 |= pattern;

#if B_HOST_IS_LENDIAN
			outbits32 = BSWAP(outbits32);
#endif /* B_HOST_IS_LENDIAN */

			if (xor) {
				outbits32 ^= *(aoutput - stride);
			}
			*aoutput++ = outbits32;			/* write 4 bytes */
			*c_aoutput++ = outbits32;
			numstores--;
			if(numstores == 0){
				aoutput = poutput + stride;
				c_aoutput = c_poutput + c_stride;
				poutput = aoutput;
				c_poutput = c_aoutput;
				numstores = uXExtent>>5;
				xor = bxor;
			}
			pattern = fullwidth[state];
			middleruns>>=5;
			while(middleruns--){
				if (xor) {
					*aoutput = *c_aoutput
						= pattern ^ *(aoutput - stride);
					aoutput++; c_aoutput++;
				}
				else {
					*aoutput++ = pattern;	/* write 4 bytes */
					*c_aoutput++ = pattern;
				}
				numstores--;
				if(numstores == 0){
					aoutput = poutput + stride;
					c_aoutput = c_poutput + c_stride;
					poutput = aoutput;
					c_poutput = c_aoutput;
					numstores = uXExtent>>5;
					xor = bxor;
				}
			}
			pattern = (state<<lastbitsrun)-state;
			pattern <<= 32 - lastbitsrun;
			outbits32 = pattern;
			bitsinoutput = lastbitsrun;
		}
		if(bitsinoutput == 32){ /* need to flush current buffer? */
			bitsinoutput = 0;

#if B_HOST_IS_LENDIAN
			outbits32 = BSWAP(outbits32);
#endif /* B_HOST_IS_LENDIAN */

			if (xor) {
				outbits32 ^= *(aoutput - stride);
			}
			*aoutput++ = outbits32;			/* write 4 bytes */
			*c_aoutput++ = outbits32;
			outbits32 = 0;
			numstores--;
			if(numstores == 0){
				aoutput = poutput + stride;
				c_aoutput = c_poutput + c_stride;
				poutput = aoutput;
				c_poutput = c_aoutput;
				numstores = uXExtent>>5;
				xor = bxor;
			}

		}
		state = next_state;
		next_state ^= 1;
	}

	return bytesread(pTileData, &p);

}	/* DecodeTransparencyTile */

/*
 * Provided with a pointer to the bitstream, where the first byte is the
 * byte aligned beginning of the transparency plane, parse the plane.
 * If the Skip flag is set, simply space over the tiles, without actually
 * decoding them (as possible).
 *
 * pXparMask and optionally pXparMaskCopy are assumed PU8, pointing
 *  to storage for ((((xres+31)>>3)&3)*yres) bytes of bitmap storage.
 *
 * pDoDecode is assumed to be an array of byte of dimension number of tiles,
 *  with 0 indicating the corresponding tile should not be decoded.
 */
I32
DecodeTransparencyPlane(pXparCntx pXCntx, pXparSetCntx pXSCntx,
						PIA_Boolean bTranspDataSize,
						PPointSt ptFrameSize, PPointSt ptTileSize,
						PPointSt ptNTiles, PU8 pDoDecode, PU8 pBS,
						PU32 bytesread, PIA_Boolean Skip, PU8 pXparMask,
						U32	uXparMaskPitch, PU8 pXparMaskCopy,
						U32 uXparMaskCopyPitch,
						PPIA_Boolean pbDidXparDec,
						PPIA_Boolean pbDirtyUpdate, PRectSt rDirty){

	/* Data for 'readbits' macro */

	bitbufst	p;

	U32 bits_i_want;
	U32 total_bits_read = 0;
	U32 bm_xres, uPicYres, uPicXres;
	PIA_Boolean bUseXORComp;
	PU8 pStartInput;

	U32 uXparDataSize = 0;
	U32 code;

	I32 iNDirtyRects;
	I32 i, minX, minY, maxX, maxY, leftX, rightX, topY, bottomY;
	I32 width, height, tried_to_update_mask, numtiles;

#ifdef __MEANTIME_RTDEC__
	STARTCLOCK
#endif

	pStartInput = pBS;

	*pbDidXparDec = 0;
	*pbDirtyUpdate = 0;

	tried_to_update_mask = 0;


	readbitsinit2(&p, pBS);

#if 1
/*	Transparency Data Size */
	if (bTranspDataSize) {
		uXparDataSize = readbits2(&p, 24);
	}
/*	# Dirty Rectangles */
	iNDirtyRects = readbits2(&p, 3);
	if (iNDirtyRects) {
		minX = minY = 65536;
		maxX = maxY = 0;
		for (i = 0; i < iNDirtyRects; i++) {
			leftX = readbits2(&p, 16);
			topY = readbits2(&p, 16);
			width = readbits2(&p, 16);
			height = readbits2(&p, 16);
			rightX = leftX + width - 1;
			bottomY = topY + height - 1;
			minX = MIN(minX, leftX);
			minY = MIN(minY, topY);
			maxX = MAX(maxX, rightX);
			maxY = MAX(maxY, bottomY);
		}
		/* Add clip to framesize to avoid caller doing unfortunate things? */
		rDirty->r = minY;
		rDirty->c = minX;
		rDirty->h = maxY - minY + 1;
		rDirty->w = maxX - minX + 1;
		*pbDirtyUpdate = 1;
	}
/*	Use XOR compression */
	bUseXORComp = readbits2(&p, 1);
/*	Transparency Huffman Table */
	code = readbits2(&p, 1);
	if (code) {
		code = readbits2(&p, 4);
		pXSCntx->hdesc[0] = (U8) code;

		for(i = 1; i <= pXSCntx->hdesc[0]; i++){
			code = readbits2(&p, 4);
			pXSCntx->hdesc[i] = (U8) code;
		}
	}
#endif

	pBS += bytesread(pBS, &p);
	readbitsinit2(&p, pBS);
	numtiles = ptNTiles->r * ptNTiles->c;
	bm_xres = (ptFrameSize->c+31)&~31;
	uPicYres = ptFrameSize->r;
	uPicXres = ptFrameSize->c;

	if(Skip){/* forward space tiles to end of plane. */
		int last_empty;
		while(numtiles--){
			bits_i_want = readbits2(&p, 1);
			if(bits_i_want){ /* empty */
				bits_i_want = readbits2(&p, 1);
				last_empty = 1;
			}
			else{	/* non-empty */
				last_empty = 0;
				bits_i_want = readbits2(&p, 1);
				if(bits_i_want){
					bits_i_want = readbits2(&p, 8);
					if(bits_i_want == 255){
						bits_i_want = readbits2(&p, 24);
					}
					pBS += bits_i_want;
				}
				else{	/* non-empty, no tile data size */
					U32 x;
					pBS += bytesread(pBS, &p);
					x = DecodeTransparencyTile(pXCntx, pXSCntx, pXparMask,
											uXparMaskPitch, NULL, 0,
											bm_xres, uPicYres, uPicXres,
											0, 0, ptTileSize->c, ptTileSize->r,
											pBS, 1, 0, 0, (U8)bUseXORComp);

					if(x == -1){
						return -1;	/* error parsing tile when missing size */
					}
					else{
						pBS += x;
					}
				}
				readbitsinit2(&p, pBS);
			}
		}
		if(last_empty){
			pBS += bytesread(pBS, &p);
		}
	}
	else {					/* decode tiles needed */
		int tilenumber, row, col, last_empty, x;

		tilenumber = 0;

		tried_to_update_mask = 1;

		for(row = 0; row < (int)ptNTiles->r; row++){
			for(col = 0; col < (int)ptNTiles->c; col++){
				bits_i_want = readbits2(&p, 1);
				if(pDoDecode[tilenumber]){
					if(bits_i_want){				/* want tile, empty */
						last_empty = 1;
						bits_i_want = readbits2(&p, 1);
						if(DecodeTransparencyTile(pXCntx, pXSCntx, pXparMask,
							uXparMaskPitch, pXparMaskCopy, uXparMaskCopyPitch,
							bm_xres, uPicYres, uPicXres,
							/* tiles are multiples of 32 in size, or there
							 * is just one tile.
							 */
							col * ptTileSize->c, row * ptTileSize->r,
							ptTileSize->c,ptTileSize->r,NULL,1,1,
							(U8)bits_i_want, (U8)bUseXORComp))
								return -1;	/* error on empty tile? */
					}
					else{							/* want tile, non empty */
						last_empty = 0;
						bits_i_want = readbits2(&p, 1);
						if(bits_i_want) {			/* size present */
							bits_i_want = readbits2(&p, 8);
							if(bits_i_want == 255){
								bits_i_want = readbits2(&p, 24);
							}
							x = DecodeTransparencyTile(pXCntx,pXSCntx,pXparMask,
								uXparMaskPitch, pXparMaskCopy, uXparMaskCopyPitch,
								bm_xres, uPicYres, uPicXres,
								col * ptTileSize->c, row * ptTileSize->r,
								ptTileSize->c, ptTileSize->r,
								pBS + bytesread(pBS, &p), 1, 0, 0, (U8)bUseXORComp);
							if(x == -1) {
								return -1;	/* error in needed tile */
							}
							if((U32)(x+bytesread(pBS, &p)) != (bits_i_want)) {
								return -1;			/* wrong size parsed? */
							}
							pBS += bits_i_want;
						}
						else{						/* no size present */
							pBS += bytesread(pBS, &p);
							x = DecodeTransparencyTile(pXCntx, pXSCntx,
									pXparMask, uXparMaskPitch, pXparMaskCopy,
									uXparMaskCopyPitch, bm_xres,
									uPicYres, uPicXres,
									col * ptTileSize->c,
									row * ptTileSize->r,
									ptTileSize->c, ptTileSize->r,
									pBS, 1, 0, 0, (U8)bUseXORComp);
						
							if(x == -1) {
								return -1;	/* error in needed tile */
							}
							pBS += x;
						}

						readbitsinit2(&p, pBS);
					} /* empty? */
				}
				else{		/* ...tile not wanted... */
					if(bits_i_want){				/* empty, skip */
						last_empty = 1;
						bits_i_want = readbits2(&p, 1);
					}
					else{							/* non-empty, skip */
						last_empty = 0;
						bits_i_want = readbits2(&p, 1);
						if(bits_i_want){			/* size present */
							bits_i_want = readbits2(&p, 8);
							if(bits_i_want == 255){
								bits_i_want = readbits2(&p, 24);
							}
							pBS += bits_i_want;
						}
						else{						/* no size present */

							pBS += bytesread(pBS, &p);
							x = DecodeTransparencyTile(pXCntx, pXSCntx,
									pXparMask, uXparMaskPitch, pXparMaskCopy,
									uXparMaskCopyPitch, bm_xres,
									uPicYres, uPicXres,
									col * ptTileSize->c,
									row * ptTileSize->r,
									ptTileSize->c, ptTileSize->r,
									pBS, 1, 0, 0, (U8)bUseXORComp);
							if(x == -1) {
								return -1;	/* error in needed tile */
							}
							pBS += x;
						}
						readbitsinit2(&p, pBS);
					}
				} /* wanted? */
				tilenumber++;
			} /* for(col) */
		} /* for(row) */
		/* adjust bitstream pointer, based on last_empty... */
		if(last_empty){
			pBS += bytesread(pBS, &p);
		}
	}
	*bytesread = pBS - pStartInput;
	*pbDidXparDec = tried_to_update_mask;

#ifdef __MEANTIME_RTDEC__
	STOPCLOCK
	if(iTimingFrame != -1) frametimes[iTimingFrame].xpardec += elapsed;
#endif

	return 0;
} /* DecodeTransparencyPlane */

