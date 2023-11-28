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
*               Copyright (C) 1994-1998 Intel Corp.                     *
*                         All Rights Reserved.                          *
*                                                                       *
************************************************************************/

/*************************************************************************
 * READBITS.H -- function declarations and definitions needed for
 * bitstream reading of headers & data.  Includes big & little endian
 * versions of both fixed length and variable length (huffman) bit reading
 *************************************************************************/

#ifndef __READBITS_H__
#define __READBITS_H__

typedef struct {
	U32				bitsread;
	union {
		PU8			bytepntr;
		PU32		fullpntr;
	} p;
	U32	lo;
	U32	hi;
} bitbufst, *pbitbufst;


void readbitsstartup(void);
extern void (*readbitsinit2)(pbitbufst p, PU8 pntr);
extern U32 (*readbits2)(pbitbufst p, U32 nbits);
extern U32 (*readbits2h)(pbitbufst p, pHuffTabSt h);
#define bytesread(pntr, pb) \
	((pb)->p.bytepntr + (((pb)->bitsread + 7)>>3) - (PU8)pntr)

#endif /* __READBITS_H__ */
