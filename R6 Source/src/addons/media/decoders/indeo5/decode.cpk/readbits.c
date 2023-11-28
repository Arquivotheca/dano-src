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
 * READBITS.C -- function declarations and definitions needed for
 * bitstream reading of headers & data.  Includes big & little endian
 * versions of both fixed length and variable length (huffman) bit reading
 *************************************************************************/
#include "datatype.h"
#include "cpk_blk.h"
#include "readbits.h"

void (*readbitsinit2)(pbitbufst p, PU8 pntr);
U32	 (*readbits2)(pbitbufst p, U32 nbits);
U32	 (*readbits2h)(pbitbufst p, pHuffTabSt h);

void readbitsinit_le (pbitbufst p, PU8 pntr);
void readbitsinit_be (pbitbufst p, PU8 pntr);
U32 readbits_le(pbitbufst p, U32 nbits);
U32 readbits_be(pbitbufst p, U32 nbits);

#define READ4BYTES(dst, src, off) \
	(dst) = \
		*(src.bytepntr+off+0)			|	\
		(*(src.bytepntr+off+1) << 8)	|	\
		(*(src.bytepntr+off+2) << 16)	|	\
		(*(src.bytepntr+off+3) << 24);

void readbitsinit_le (pbitbufst p, PU8 pntr) {
	U32	off;
	
	off = (U32)pntr&0x3;

	p->bitsread = off<<3; /* 0, 8, 16, or 24 */
	p->p.bytepntr = pntr - off;

	p->lo = *p->p.fullpntr;
	p->hi = *(p->p.fullpntr+1);

}

void readbitsinit_be (pbitbufst p, PU8 pntr) {
	U32	off;
	
	off = (U32)pntr&0x3;

	p->bitsread = off<<3; /* 0, 8, 16, or 24 */
	p->p.bytepntr = pntr - off;

	READ4BYTES(p->lo, p->p, 0)
	READ4BYTES(p->hi, p->p, 4)
}


U32 readbits_le(pbitbufst p, U32 nbits) {
	U32 result;
	/* right justify lo part (bitsread in [0,...,31]) */
	result = p->lo >> p->bitsread;
	/* accumulate bitsread */
	p->bitsread += nbits;
	/* if overflow into hi */
	if (p->bitsread & 32) {
		/* nbits - bitsread in [1,...,32] */
		p->bitsread ^= 32;
		if (p->bitsread) {
			result |= (p->hi << (nbits-p->bitsread));
		}
		p->p.bytepntr += 4;
		p->lo = p->hi;
		p->hi = *(PU32)(p->p.bytepntr+4);
	}
	return result & ((1 << nbits) - 1 - (nbits == 32));
}

U32 readbits_be(pbitbufst p, U32 nbits) {
	U32 result;
	/* right justify lo part (bitsread in [0,...,31]) */
	result = p->lo >> p->bitsread;
	/* accumulate bitsread */
	p->bitsread += nbits;
	/* if overflow into hi */
	if (p->bitsread & 32) {
		/* nbits - bitsread in [1,...,32] */
		p->bitsread ^= 32;
		if (p->bitsread) {
			result |= (p->hi << (nbits-p->bitsread));
		}
		p->p.bytepntr += 4;
		p->lo = p->hi;
		READ4BYTES(p->hi, p->p, 4)
	}
	return result & ((1 << nbits) - 1 - (nbits == 32));
}

U32 readbitsh_le(pbitbufst p, pHuffTabSt h) {
	U32 result;
	/* right justify lo part (bitsread in [0,...,31]) */
	result = p->lo >> p->bitsread;
	/* check if enough bits to decode */
	if (32-p->bitsread < h->maxbits) {
		result |= (p->hi << (32-p->bitsread));
	}
	/* get the code */
	result = h->maintable[result&h->maxbitsmask];
	/* accumulate bitsread */
	p->bitsread += h->numbits[result];
	/* if overflow into hi */
	if (p->bitsread & 32) {
		/* nbits - bitsread in [1,...,32] */
		p->bitsread ^= 32;
		p->p.bytepntr += 4;
		p->lo = p->hi;
		p->hi = *(PU32)(p->p.bytepntr+4);
	}
	return result;
}

U32 readbitsh_be(pbitbufst p, pHuffTabSt h) {
	U32 result;
	/* right justify lo part (bitsread in [0,...,31]) */
	result = p->lo >> p->bitsread;
	/* check if enough bits to decode */
	if (32-p->bitsread < h->maxbits) {
		result |= (p->hi << (32-p->bitsread));
	}
	/* get the code */
	result = h->maintable[result&h->maxbitsmask];
	/* accumulate bitsread */
	p->bitsread += h->numbits[result];
	/* if overflow into hi */
	if (p->bitsread & 32) {
		/* nbits - bitsread in [1,...,32] */
		p->bitsread ^= 32;
		p->p.bytepntr += 4;
		p->lo = p->hi;
		READ4BYTES(p->hi, p->p, 4)
	}
	return result;
}

void readbitsstartup(void) {
	U8	bytes[4] = { 0x80, 0x00, 0x00, 0x01 };
	if (*((PU32)bytes) == 0x80000001) {
		readbitsinit2 = readbitsinit_be;;
		readbits2 = readbits_be;
		readbits2h = readbitsh_be;
	}
	else if (*((PU32)bytes) == 0x01000080) {
		readbitsinit2 = readbitsinit_le;
		readbits2 = readbits_le;
		readbits2h = readbitsh_le;
	}
/*	else {
		error();
	} */
}
