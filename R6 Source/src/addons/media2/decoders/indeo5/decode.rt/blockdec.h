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

#ifdef __BLOCKDEC_H__
#pragma message("blockdec.h: multiple inclusion")
#else /* __BLOCKDEC_H__ */
#define __BLOCKDEC_H__

#ifndef __DATATYPE_H__
#pragma message("blockdec.h requires datatype.h")
#endif

#ifdef DEBUG
static char blockdec_h_rcsid[] = "@(#) \t$Id: blockdec.h 1.17 1995/05/16 19:51:21 tw Exp $";
static char blockdec_h_vcsid[] = "@(#) \t$Workfile:   blockdec.h  $ $Revision:   1.1  $";
#endif /* DEBUG */

#ifndef FARPROC
#define PROC(x)	void (*x)()
#else /* FARPROC */
#define PROC(x) FARPROC x
#endif /* FARPROC */

typedef U8 FREQUENCY_TYPE;
typedef I8 NABSVAL_TYPE;
typedef I8 PBDESCRIPTOR_TYPE;

#define H_MAXCODELEN 13			/* longest allowed huffman code */

#define MH_BITS 10			/* <= 15, 2*average code length */

#define NZSETS 6
#define NQSETS 6
#define NQLEVS 24

/*
 *	This structure is filled in during the MacroBlock processing.
 *	Every block in a band is represented by a corresponding
 *	BlockInfoSt element.
 */
typedef struct _BlockInfoSt {
	PU8 curr;
	U32 motion_vectors;
	I32 **quant;
	U32 flags;
} BlockInfoSt, *pBlockInfoSt, *pBlkInfoSt;

/* BlockInfoSt flags values */
#define BT_CODED	0x00000001	/* coded block */
#define BT_LOG2_CODED	0		/* for shifting */
#define BT_INTRA	0x00000002	/* intra block */
#define BT_LOG2_INTRA	1		/* for shifting */
#define BT_FWD		0x00000004	/* contains forward prediction */
#define BT_LOG2_FWD		2		/* for shifting */
#define BT_BKWD		0x00000008	/* contains backward prediction */
#define BT_LOG2_BKWD	3		/* for shifting */
#define BT_4x4		0x00000010	/* this is a 4x4 block */
#define BT_LOG2_4x4		4		/* for shifting */
#define BT_PITCH	0x0000ffe0	/* pitch indicator */
#define BT_POSITION	0x03000000	/* 2 bit 0 to 3, 0-1 used for dual */
#define BT_DUAL_HI	0x01000000	/* simplify checking for now */
#define BT_LOG2_DUAL_HI	24		/* for shifting */
#define BT_QMASK	0xfc000000	/* quant mask, 6 bits biased by 32 */
#define BT_LOG2_QMASK	26		/* for shifting */
#define BT_UNUSED1	0x00ff0000	/* simplify checking for now */

/* Convenience items */
#ifndef NULL
#define NULL 0
#endif
#ifndef MAXINT
#define MAXINT ~(1<<31)
#endif
#ifndef MAXUNSIGNED
#define MAXUNSIGNED ~(0)
#endif


/* Block data decode */

#pragma pack(1)

/* Note to ME: I'm compressing the size of this and insuring that
 * we are copying it correctly, since when DRC is working correctly,
 * this will comprise a large proportion of the work on it.
 */
typedef struct {			/* Enough to specify a context */
	U8	transform_token;	/* inverse transform */
	I8	scan_token;		/* scan order */
	U8	bs_sbsize;		/* block size for b.s. scan */
	I8	quant_token;		/* quantizer set */
	U8	bs_qbsize;		/* block size for b.s. quant */
	PBDESCRIPTOR_TYPE huffman[14];	/* modified PB descriptor */
	U8	bs_scan[64];		/* the b.s. scan */
	U8	bs_qbase[2][64];	/* b.s. base */
	U8	bs_qscale[2][64];	/* b.s. scale */
	U8	PAD1;			/* to 404 bytes, 101 U32 moves */
	NABSVAL_TYPE *nabsval;		/* r/v code generator */
	FREQUENCY_TYPE *invfreq;	/* and occurence frequency */
	U8	bs_rvswaplist[(61/2+1)*4];/* r/v swap list, 32 U32 moves */
} SETCONTEXT, SetCntxSt, * pSetCntxSt;

#pragma pack()

typedef struct {
	PBDESCRIPTOR_TYPE *descriptor;	/* modified PB descriptor */
	U8	*maintable;		/* LUT indexed by maxbits bits of codes */
	U8  mainstatic[1<<H_MAXCODELEN];	/* cheap trick implementation */
	U32	maxbitsmask;	/* ((1<<maxbits)-1) */
	U8	numbits[256];	/* number of bits used up by that symbol */
	U8	minbits,maxbits;/* shortest and longest codes */
} HUFFTAB, HuffTabSt, * pHuffTabSt;

typedef struct {
	NABSVAL_TYPE *nabsval;		/* 'nabsval[]', with 0 terminator */
	FREQUENCY_TYPE *invfreq;	/* old style frequency info:
								 * table of rv codes, in order of
								 * frequency, highest to lowest.
								 * entry 0 is most frequent rvcode...
								 */
	U8	*runtab;		/* run length by symbol */
	I32	*valtab;		/* quantized coeff by symbol */
	U8	eob, esc;		/* indices of EOB and ESC */
} RVCODETAB;

typedef struct {
	HUFFTAB	*fulltable;	/* which huffman codebook */
	RVCODETAB *rvcodes;	/* and which r/v codebook */
	U32	*multitab;		/* multiple codebook for that context */
	U32 multistatic[1<<MH_BITS];/* cheap trick implementation */
} MULTICODETAB;

typedef struct {
	I32	setnumber;		/* static (set number 0-n) or dynamic (-1) */
	I32	*set[2][NQLEVS][64];/* inter/intra, level, position */
	I32 **level[2][NQLEVS];	/* to map from ordinal to pointer */
} DQTABLE;

typedef struct {
	I32			initialized;

	PROC(transform);
	PROC(mcaddproc);
	PROC(intra_smear);
	PROC(init_block);
	PROC(decode_low_block);
	PROC(decode_high_block);
	PROC(apply_dc);

	U32			transform_flags;
 	const U8	*transform_prescale;
	const U32	*block_initial_state;

	I32			currently_set;	/* 0 or 1 */
	SETCONTEXT	setinfo_pair[2];/*, setinfo;*/

	U8		*bs_rvswaplist;

/*/	U8		bs_mscan[64];
//	U8		bs_mqbase[2][64];
//	U8		bs_mqscale[2][64];*/

	DQTABLE		dequant;
	RVCODETAB	rvcodes;
	HUFFTAB		*hufftab;
	HUFFTAB		hufftab_static;
	MULTICODETAB	*multicode;
	MULTICODETAB	multi_static;
	I32			val[256];
	U8			run[256];
	U8			zag[64];
	U8			escape;
} BLOCKCONTEXT, BlkCntxSt, * pBlkCntxSt;

typedef struct {
	PROC(invert);
	PROC(intra_smear);
	PROC(mcaddproc);
	char name[32];
	I32	defaultzag;
	const U32 *initial;
/*/TBD: remove following element
//const U32 *prescale;*/
	U32	flags;
} TRANSFORM;

/* transform type flags */

#define TT_DUAL	0x00000001	/* U32 is 2 packed 16 bit coeff's */
#define TT_QUAD	0x00000002	/* U64 is 4 packed 16 bit coeff's */
/*/#define TT_2X32	0x00000004	/* U64 is 2 packed 32 bit coeff's */
/*/#define TT_FUTR	0x00000008	/* future expansion */
#define TT_XPOS	0x00000004	/* wants 2x2 sub-blocks transposed */
#define TT_3236	0x00000008	/* wants cache bank conflict avoidance */

#define TT_BIAS	0x00000010	/* Each coeff needs 'add coeff, bias' */
#define TT_SAME	0x00000020	/* Nonzero bias is same for all coeff's */

#define TT_4IN8	0x00000040	/* Transform is 4x4 (in 8x8 block) */
#define TT_SWAP 0x00000080	/* wants row vector r0r1 r4r5 r2r3 r6r7 */

I32 QuantToScanOrder(BLOCKCONTEXT *);				/* PROTO */
I32 BuildRVTables(RVCODETAB *);					/* PROTO */
I32 BuildPBHuffTab(pHuffTabSt);					/* PROTO */
I32 BuildParTab(HUFFTAB *, RVCODETAB *, MULTICODETAB *);	/* PROTO */
BLOCKCONTEXT *AllocContext(void);				/* PROTO */
void FreeContext(BLOCKCONTEXT *);				/* PROTO */
I32 InitDecodeTables(void);					/* PROTO */
I32 DeInitDecodeTables(void);					/* PROTO */
I32 DecodeBlockData(BLOCKCONTEXT*,SETCONTEXT*,U8*,BlockInfoSt*,I32);/* PROTO */
I32 PrivateBuildPBHuffTab(pHuffTabSt);
I32 PBCodeBookMatches(PBDESCRIPTOR_TYPE *, PBDESCRIPTOR_TYPE *);
#endif /* __BLOCKDEC_H__ */

