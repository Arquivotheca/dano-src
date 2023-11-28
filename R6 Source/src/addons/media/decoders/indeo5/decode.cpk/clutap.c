/*
 *
 *               INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *    This listing is supplied under the terms of a license agreement
 *      with INTEL Corporation and may not be copied nor disclosed
 *        except in accordance with the terms of that agreement.
 *
 *
 *
 *               Copyright (C) 1994-1997 Intel Corp.
 *                         All Rights Reserved.
 *
 */

/*************************************************************************

	clutap.c

 DESCRIPTION:
 YUV9 planar to 8 bit color look up table conversion routine (active palette). 

	Routines:                      prototypes in:
		CLUTAP()                   decproci.h

	Tabs set to 8, Pentium(r) Processor assembly code
	Tabs set to 4, C code

*************************************************************************/

#include "datatype.h"
#include "pia_main.h"
#include "yrgb8.h"

#undef NO_Y_DITHER
#if defined NO_Y_DITHER
#define YS 0
#define YB MIN_Y_DITHER
#else
#define YS 4 /*2 */
#define YB 0 /*8 */
#endif /* NO_Y_DITHER */
#undef RED_DOTS	/* place red pixels in upper left corner of 4x4 blocks */
#undef MONOCHROME
	   
#ifdef C_PORT_KIT
#define USENOASM
#else /* C_PORT_KIT */
#undef USENOASM
#endif /* C_PORT_KIT */

#ifdef USENOASM

const I32 Y_SHIFT  = 8;
const I32 UV_SHIFT = 0;

const I32 YDither00 = 0;
const I32 YDither01 = 4;
const I32 YDither02 = 1;
const I32 YDither03 = 5;
const I32 YDither10 = 6;
const I32 YDither11 = 2;
const I32 YDither12 = 7;
const I32 YDither13 = 3;
const I32 YDither20 = 1;
const I32 YDither21 = 5;
const I32 YDither22 = 0;
const I32 YDither23 = 4;
const I32 YDither30 = 7;
const I32 YDither31 = 3;
const I32 YDither32 = 6;
const I32 YDither33 = 2;

const I32 UVSubSample = 4;

/* This code needs to have transparency and local decode support added to it */
I32 C_YVU9toActivePalette(U32 nRows, U32 nCols, PU8 pY, PU8 pV, PU8 pU,
	U32 uYPitch, U32 uVUPitch, PU8 pOut, I32 iOutPitch,
	PU8 pTMask, PU8 pLMask, U32 uMStride,
	U32 uWantsFill, U32 uFillValue)
{
U32 i, j;
PU32 puTblU, puTblV; 				  /* U and V dither tables */
PU8 pu8YPlaneCursor, pu8Y;			  /* Pointers within planes by rows */
PU8 pu8UPlaneCursor, pu8VPlaneCursor; /* pu8Y is columns within a block */
PU8 pu8CLUTOutputCursor, pu8O;		  /* Same as above for output */
U8  u8U, u8V;						  /* For clipping U and V data */
U32 uUVUVUVUV;						  /* block dither info */
U32 uUVa, uUVb, uUVc, uUVd;	  		  /* dither info w/in block */
U32 uY0, uY1, uY2, uY3;		  		  /* for a row of a block */
U32 uOutputData;							 
									  
puTblU = (PU32)(gpu8ClutTables + (CLUT_TABLE_SIZE));
puTblV = puTblU + (TABLE_U_SIZE/sizeof(*puTblU));	/* V info is right after U info */

if (pTMask || pLMask)
	return C_YVU9toActivePalette_44(nRows,nCols,pY,pV,pU,
	uYPitch,uVUPitch,pOut,iOutPitch,
	pTMask,pLMask,uMStride,
	uWantsFill,uFillValue);
/* Loop through each block */
for (i = 0; i < nRows; i += UVSubSample) {

	pu8YPlaneCursor = pY + (uYPitch * i);
	pu8UPlaneCursor = pU + (uVUPitch * (i / UVSubSample));
	pu8VPlaneCursor = pV + (uVUPitch * (i / UVSubSample));
	pu8CLUTOutputCursor = pOut + (iOutPitch * i);

	for (j = 0; j < nCols; j += UVSubSample) {

		pu8Y = pu8YPlaneCursor;     /* top of y-source block */
		pu8O = pu8CLUTOutputCursor; /* top of output block */
		
        /* get chroma with dither for entire block */
		u8V = *pu8VPlaneCursor++;
		u8U = *pu8UPlaneCursor++;
		
		uUVUVUVUV = *(puTblV + u8V) | *(puTblU + u8U);
	
#if defined MONOCHROME
		uUVUVUVUV = 0x88888888;
#endif	
			
		uUVa = (uUVUVUVUV & 0xff) << UV_SHIFT;
		uUVUVUVUV >>= 8;
		uUVb = (uUVUVUVUV & 0xff) << UV_SHIFT;
		uUVUVUVUV >>= 8;
		uUVc = (uUVUVUVUV & 0xff) << UV_SHIFT;
		uUVUVUVUV >>= 8;
		uUVd = (uUVUVUVUV & 0xff) << UV_SHIFT; 

		/**** write row 0 of block */
		uY3 = (* pu8Y    + (I32)(YDither00 * YS - YB)) << Y_SHIFT;
		uY2 = (*(pu8Y+1) + (I32)(YDither01 * YS - YB)) << Y_SHIFT;
		uY1 = (*(pu8Y+2) + (I32)(YDither02 * YS - YB)) << Y_SHIFT;
		uY0 = (*(pu8Y+3) + (I32)(YDither03 * YS - YB)) << Y_SHIFT;
#if B_HOST_IS_BENDIAN
#define I1 24
#define I2 16
#define I3 8
#define I4 0
#else
#define I1 0
#define I2 8
#define I3 16
#define I4 24
#endif
		uOutputData = *(gpu8ClutTables + uY3 + uUVb) << I1;
		uOutputData |= (U32)*(gpu8ClutTables + uY2 + uUVa) << I2;
		uOutputData |= (U32)*(gpu8ClutTables + uY1 + uUVb) << I3;
		uOutputData |= (U32)*(gpu8ClutTables + uY0 + uUVa) << I4; 
		
		*((PU32)pu8O) = uOutputData;
		pu8Y += uYPitch;       /* next row of y source */
		pu8O += iOutPitch; /* next row of output */

		/**** write row 1 of block */
		uY3 = (* pu8Y    + (I32)(YDither10 * YS - YB)) << Y_SHIFT;
		uY2 = (*(pu8Y+1) + (I32)(YDither11 * YS - YB)) << Y_SHIFT;
		uY1 = (*(pu8Y+2) + (I32)(YDither12 * YS - YB)) << Y_SHIFT;
		uY0 = (*(pu8Y+3) + (I32)(YDither13 * YS - YB)) << Y_SHIFT;
		
		uOutputData = *(gpu8ClutTables + uY3 + uUVd) << I1;
		uOutputData |= (U32)*(gpu8ClutTables + uY2 + uUVc) << I2;
		uOutputData |= (U32)*(gpu8ClutTables + uY1 + uUVd) << I3;
		uOutputData |= (U32)*(gpu8ClutTables + uY0 + uUVc) << I4; 
		
		*((PU32)pu8O) = uOutputData; 
		pu8Y += uYPitch;       /* next row of y source */
		pu8O += iOutPitch; /* next row of output */
		
		/**** write row 2  of block */
		uY3 = (* pu8Y    + (I32)(YDither20 * YS - YB)) << Y_SHIFT;
		uY2 = (*(pu8Y+1) + (I32)(YDither21 * YS - YB)) << Y_SHIFT;
		uY1 = (*(pu8Y+2) + (I32)(YDither22 * YS - YB)) << Y_SHIFT;
		uY0 = (*(pu8Y+3) + (I32)(YDither23 * YS - YB)) << Y_SHIFT;
		
		uOutputData = *(gpu8ClutTables + uY3 + uUVb) << I1;
		uOutputData |= (U32)*(gpu8ClutTables + uY2 + uUVa) << I2;
		uOutputData |= (U32)*(gpu8ClutTables + uY1 + uUVb) << I3;
		uOutputData |= (U32)*(gpu8ClutTables + uY0 + uUVa) << I4;
		
		*((PU32)pu8O) = uOutputData;
		pu8Y += uYPitch;       /* next row of y source */
		pu8O += iOutPitch; /* next row of output */
		
		/**** write row 3  of block */
		uY3 = (* pu8Y    + (I32)(YDither30 * YS - YB)) << Y_SHIFT;
		uY2 = (*(pu8Y+1) + (I32)(YDither31 * YS - YB)) << Y_SHIFT;
		uY1 = (*(pu8Y+2) + (I32)(YDither32 * YS - YB)) << Y_SHIFT;
		uY0 = (*(pu8Y+3) + (I32)(YDither33 * YS - YB)) << Y_SHIFT;
		
		uOutputData = *(gpu8ClutTables + uY3 + uUVd) << I1;
		uOutputData |= (U32)*(gpu8ClutTables + uY2 + uUVc) << I2;
		uOutputData |= (U32)*(gpu8ClutTables + uY1 + uUVd) << I3;
		uOutputData |= (U32)*(gpu8ClutTables + uY0 + uUVc) << I4; 
		
		*((PU32)pu8O) = uOutputData;

        /* next block across */
		pu8YPlaneCursor += UVSubSample;
		pu8CLUTOutputCursor += UVSubSample;
		}	/* for j... blocks across row */
	}	/* for i... rows of blocks */
	return 0;
}

#else /* Use ASM code */

/* potential optimizations: */
/* - P5 pairing analysis and re-ordering - done */
/* - unroll by 2 and 4 versions */
/* - P6-specific inner loop (avoid partial register stalls) */
/* */
/* UV dither pattern: */
/* 2 3 2 3 */
/* 0 1 0 1 */
/* 2 3 2 3 */
/* 0 1 0 1 */
/* */
/* Y dither pattern: */
/* 0 4 1 5 */
/* 6 2 7 3 */
/* 1 5 0 4 */
/* 7 3 6 2 */

#define Ydither00 (0*YS-YB)
#define Ydither01 (4*YS-YB)
#define Ydither02 (1*YS-YB)
#define Ydither03 (5*YS-YB)
#define Ydither10 (6*YS-YB)
#define Ydither11 (2*YS-YB)
#define Ydither12 (7*YS-YB)
#define Ydither13 (3*YS-YB)
#define Ydither20 (1*YS-YB)
#define Ydither21 (5*YS-YB)
#define Ydither22 (0*YS-YB)
#define Ydither23 (4*YS-YB)
#define Ydither30 (7*YS-YB)
#define Ydither31 (3*YS-YB)
#define Ydither32 (6*YS-YB)
#define Ydither33 (2*YS-YB)

/* Y is in the high byte */
#define cUV	cl
#define cY	ch
#define	dUV	dl
#define	dY	dh
#define YDITHER_SCALE	256
#define TableU	CLUT_TABLE_SIZE
#define TableV	TableU + TABLE_U_SIZE

/*
 * A bit mask of "on" pels of dimension 4 pels can be used to
 * lookup a mask.
 *
 * The entries in these tables are LE/BE and LSB/MSB first specific.
 */
static unsigned long MaskOffPels[] = {
	0x00000000,
	0xFF000000,	/* 0 0 0 1 only right most pel on */
	0x00FF0000,	/* 0 0 1 0 only mid-right pel on */
	0xFFFF0000,	/* 0 0 1 1 */
	0x0000FF00,	/* 0 1 0 0 */
	0xFF00FF00,	/* 0 1 0 1 */
	0x00FFFF00,	/* 0 1 1 0 */
	0xFFFFFF00,	/* 0 1 1 1 */
	0x000000FF,
	0xFF0000FF,
	0x00FF00FF,
	0xFFFF00FF,
	0x0000FFFF,
	0xFF00FFFF,
	0x00FFFFFF,
	0xFFFFFFFF,
};

static unsigned long MaskOffFill[] = {
	0xFFFFFFFF,	/* 15 */
	0x00FFFFFF,	/* 0 1 1 1 b */
	0xFF00FFFF,
	0x0000FFFF,
	0xFFFF00FF,
	0x00FF00FF,
	0xFF0000FF,
	0x000000FF,
	0xFFFFFF00,	/* 1 1 1 0 b */
	0x00FFFF00,
	0xFF00FF00,
	0x0000FF00,
	0xFFFF0000,
	0x00FF0000,
	0xFF000000,
	0x00000000,	/* 0 0 0 0 b */
};

#define LOCALBASE esp

#define locV		DWORD PTR [LOCALBASE +  0]
#define locU		DWORD PTR [LOCALBASE +  4]
#define locOLineDelta	DWORD PTR [LOCALBASE +  8]
#define locYLineDelta	DWORD PTR [LOCALBASE + 12]
#define locCLineDelta	DWORD PTR [LOCALBASE + 16]
#define locXResetVal	DWORD PTR [LOCALBASE + 20]
#define locXLoopCtr	DWORD PTR [LOCALBASE + 24]
#define locYLoopCtr	DWORD PTR [LOCALBASE + 28]
#define locOStride	DWORD PTR [LOCALBASE + 32]
#define locIStride	DWORD PTR [LOCALBASE + 36]
#define locEDI		DWORD PTR [LOCALBASE + 40]
#define locESI		DWORD PTR [LOCALBASE + 44]
#define locMask		DWORD PTR [LOCALBASE + 48]
#define locMStride	DWORD PTR [LOCALBASE + 52]
#define locMStrideX3	DWORD PTR [LOCALBASE + 56]
#define locMLineDelta	DWORD PTR [LOCALBASE + 60]
#define locIDX		DWORD PTR [LOCALBASE + 64]
#define locEBP		DWORD PTR [LOCALBASE + 68]
#define locTMP		BYTE  PTR [LOCALBASE + 72]
#define locTMP4		DWORD PTR [LOCALBASE + 72]	/* alias */
#define locAndMask	BYTE  PTR [LOCALBASE + 76]
#define locOrMask	BYTE  PTR [LOCALBASE + 80]
#define locWantsFill	DWORD PTR [LOCALBASE + 84]
#define locFillValue	DWORD PTR [LOCALBASE + 88]
#define lF4		DWORD PTR [LOCALBASE + 88]	/* alias */
#define lF1		BYTE  PTR [LOCALBASE + 88]	/* alias */
#define locViewMask	DWORD PTR [LOCALBASE + 92]
#define MYLOCALSIZE 96

#define MSB_FIRST_BITMAPS	/* MSB first bitmaps */
#ifdef MSB_FIRST_BITMAPS
#define MASK_EVEN 0xF0
#define SHIFT_EVEN 4
#define MASK_ODD 0x0F
#define SHIFT_ODD 0
#define PEL_0 0x08
#define PEL_1 0x04
#define PEL_2 0x02
#define PEL_3 0x01
#else
#define MASK_EVEN 0x0F
#define SHIFT_EVEN 0
#define MASK_ODD 0xF0
#define SHIFT_ODD 4
#define PEL_0 0x01
#define PEL_1 0x02
#define PEL_2 0x04
#define PEL_3 0x08
#endif

#ifndef NULL
#define NULL 0
#endif 

/* Note that the row skip could be done earlier. tw 5/95 */
#define DoSelUpdate(name, rowvar, rowidx, tmp, all, hi, lo, which)__asm{\
__asm SelBegin ## name ## :						\
	__asm mov	locEBP,	ebp			/* spill */	\
	__asm  mov	ebp,	locMask					\
	__asm mov	locIDX,	esi					\
	__asm  mov	esi,	rowvar					\
	__asm mov	locTMP,	tmp					\
	__asm  nop							\
	__asm mov	tmp,	[ebp+rowidx]		/* get mask */	\
	__asm  mov	esi,	locIDX			/* unspill */	\
	__asm and	tmp,	0x0F<<which				\
	__asm  jz	AllSkip ## name					\
	__asm cmp	tmp,	0x0F<<which				\
	__asm  je	AllLit ## name					\
	__asm test	tmp,	PEL_0<<which				\
	__asm jz	SkipPelZero ## name				\
	__asm mov	[edi+0],lo 			/* pel 0 */	\
__asm SkipPelZero ## name ## :						\
	__asm test	tmp,	PEL_1<<which				\
	__asm jz	SkipPelOne ## name				\
	__asm mov	[edi+1],hi			/* pel 1 */	\
__asm SkipPelOne ## name ## :						\
	__asm shr	all,	16					\
	__asm test	tmp,	PEL_2<<which				\
	__asm jz	SkipPelTwo ## name				\
	__asm mov	[edi+2],lo			/* pel 2 */	\
__asm SkipPelTwo ## name ## :						\
	__asm test	tmp,	PEL_3<<which				\
	__asm jz	SkipPelThree ## name				\
	__asm mov	[edi+3],hi			/* pel 3 */	\
	__asm jmp	SelDone ## name					\
__asm AllLit ## name ## :						\
	__asm mov	[edi],	all			/* line 0 */	\
__asm SkipPelThree ## name ## :						\
__asm AllSkip ## name ## :						\
__asm SelDone ## name ## :						\
	__asm mov	tmp,	locTMP			/* unspill */	\
	__asm  mov	ebp,	locEBP					\
}

/* Note that the row skip could be done earlier. tw 5/95 */
#define DoSelFill(name,rowvar,rowidx,tmp,all,hi,lo,which,fill,fill4)__asm{\
__asm FillBegin ## name ## :						\
	__asm mov	locEBP,	ebp			/* spill */	\
	__asm  mov	ebp,	locMask					\
	__asm mov	locIDX,	esi					\
	__asm  mov	esi,	rowvar					\
	__asm mov	locTMP,	tmp					\
	__asm  nop							\
	__asm mov	tmp,	[ebp+rowidx]		/* get mask */	\
	__asm  mov	esi,	locIDX			/* unspill */	\
	__asm and	tmp,	0x0F<<which				\
	__asm  jz	AllFill ## name				\
	__asm cmp	tmp,	0x0F<<which				\
	__asm  je	FillAllLit ## name				\
	__asm test	tmp,	PEL_0<<which				\
	__asm jnz	NoFillPelZero ## name				\
	__asm mov	lo, fill					\
__asm NoFillPelZero ## name ## :					\
	__asm mov	[edi+0],lo 			/* pel 0 */	\
	__asm test	tmp,	PEL_1<<which				\
	__asm jnz	NoFillPelOne ## name				\
	__asm mov	hi, fill					\
__asm NoFillPelOne ## name ## :						\
	__asm mov	[edi+1],hi			/* pel 1 */	\
	__asm shr	all,	16					\
	__asm test	tmp,	PEL_2<<which				\
	__asm jnz	NoFillPelTwo ## name				\
	__asm mov	lo, fill					\
__asm NoFillPelTwo ## name ## :						\
	__asm mov	[edi+2],lo			/* pel 2 */	\
	__asm test	tmp,	PEL_3<<which				\
	__asm jnz	NoFillPelThree ## name				\
	__asm mov	hi, fill					\
__asm NoFillPelThree ## name ## :					\
	__asm mov	[edi+3],hi			/* pel 3 */	\
	__asm jmp	FillDone ## name				\
__asm AllFill ## name ## :						\
	__asm mov	all, fill4					\
__asm FillAllLit ## name ## :						\
	__asm mov	[edi],	all			/* line 0 */	\
__asm FillDone ## name ## :						\
	__asm mov	tmp,	locTMP			/* unspill */	\
	__asm  mov	ebp,	locEBP					\
}
/*
 * TMP for this macro must be one of "A", "B", "C", or "D", which will
 * be used to make both the register name "[A|B|C|D]L" for access to the
 * low byte, and the register name "E[A|B|C|D]X" for use in addressing.
 * The register is assumed cleansed, and only the low byte is modified.
 * The cleansed status is, of course, lost for the P6 by the branches.
 * The original state of the low byte is restored.
 *
 */

#define DoSelFillV(name,rowvar,rowidx,tmp,all,hi,lo,which,fill,fill4)__asm{\
__asm FillVPBegin ## name ## :						\
	__asm mov	locEBP,	ebp			/* spill */	\
	__asm  mov	ebp,	locMask					\
	__asm mov	locIDX,	esi			/* spill */	\
	__asm  mov	esi,	rowvar					\
	__asm mov	locTMP4, E ## tmp ## X				\
	__asm  xor	E ## tmp ## X , E ## tmp ## X			\
	__asm mov	tmp ## L ,[ebp+rowidx]		/* get mask */	\
	__asm  nop							\
	__asm shr	tmp ## L ,	which				\
	__asm  nop							\
	__asm and	tmp ## L , 0x0F			/* one block */	\
	__asm  nop							\
	__asm nop							\
	__asm  nop							\
	__asm mov	ebp,	MaskOffFill[ E ## tmp ## X * 4]		\
	__asm  nop							\
	__asm and	all,	MaskOffPels[ E ## tmp ## X * 4]		\
	__asm  and	ebp,	fill4					\
	__asm or	all,	ebp					\
	__asm  mov	ebp,	locViewMask				\
	__asm cmp	ebp,	0					\
	__asm  jz	FillVPNoVP ## name				\
	__asm mov	tmp ## L ,[ebp+rowidx]				\
	__asm  mov	esi,	locIDX			/* unspill */	\
	__asm and	tmp ## L ,0x0F<<which				\
	__asm  jz	AllVPOff ## name				\
	__asm cmp	tmp ## L ,0x0F<<which				\
	__asm  je	FillVPAllLit ## name				\
	__asm test	tmp ## L ,PEL_0<<which				\
	__asm jz	NoStoreVPPelZero ## name			\
	__asm mov	[edi+0],lo 			/* pel 0 */	\
__asm NoStoreVPPelZero ## name ## :					\
	__asm test	tmp ## L ,PEL_1<<which				\
	__asm jz	NoStoreVPPelOne ## name				\
	__asm mov	[edi+1],hi			/* pel 1 */	\
__asm NoStoreVPPelOne ## name ## :					\
	__asm shr	all,	16					\
	__asm test	tmp ## L ,PEL_2<<which				\
	__asm jz	NoStoreVPPelTwo ## name				\
	__asm mov	[edi+2],lo			/* pel 2 */	\
__asm NoStoreVPPelTwo ## name ## :					\
	__asm test	tmp ## L ,PEL_3<<which				\
	__asm jz	NoStoreVPPelThree ## name			\
	__asm mov	[edi+3],hi			/* pel 3 */	\
__asm NoStoreVPPelThree ## name ## :					\
	__asm jmp	FillVPDone ## name				\
__asm FillVPNoVP ## name ## :						\
	__asm mov	esi,	locIDX			/* unspill */	\
__asm FillVPAllLit ## name ## :						\
	__asm mov	[edi],	all			/* line 0 */	\
__asm AllVPOff ## name ## :						\
__asm FillVPDone ## name ## :						\
	__asm mov	E ## tmp ## X ,locTMP4		/* unspill */	\
	__asm  mov	ebp,	locEBP					\
}

#pragma warning(disable:4035) /* non void naked functions: hidden return */
#pragma warning(disable:4102) /* unreferenced labels */


/*
 * Capabilities:
 * 	CSC of arbitrary rectangles of input to equal rectangle in output
 *	nRows and nCols are recommended to be evenly divisible by 4, with
 *	'soft' failure if not (rounds down on right and bottom edges.)
 *
 * Restrictions:
 *	pY & 3 == 0	-	dword aligned
 *	pOut & 3 == 0	-	dword aligned
 *
 * Assumptions:
 *	if pTMask or pLMask is provided, the leftmost pel in the CSC rectangle
 *	corresponds with the high order bit of the first byte of the mask(s).
 *
 * Arguments:
 *	nRows, nCols, pY, pV, pU, uYPitch, uVUPitch, pOut, iOutPitch
 *		are the usual suspects
 *
 *	pTMask
 *		pointer to bits, one per pel, which if set indicate the
 *		corresponding pel is "on", and if clear indicate pel is
 *		"off".  Pels which are "off" may be either set to a single
 *		value, or may be left unmodified.
 *
 *	pLMask
 *		like pTMask, but used for cropping to handle misaligned
 *		view rectangles, synthesized in the codec, and anded with
 *		the transparency mask prior to CSC.
 *		If a 4x4 block has some or all pels turned off under pTMask
 *		anded with pLMask, and uWantsFill is non-zero, pLMask will
 *		be used to selectively light the pels.
 *
 *	uMStride
 *		number of bytes from the byte containing the transparency
 *		bit for the current pel to the byte containing the bit for
 *		the pel below.
 *
 *	uWantsFill
 *		"bool", indicates that fill should occur
 *
 *	uFillValue
 *		32 bits containing a color converter specific value, which
 *		is envisioned to be 4XCLUT8 pels, 2XRGB16 pels, or one pel
 *		of larger formats.  This is the device specific value to
 *		set the 'filled' pels to.
 *
 */

__declspec ( naked )
I32 C_YVU9toActivePalette(U32 nRows, U32 nCols, PU8 pY, PU8 pV, PU8 pU,
	U32 uYPitch, U32 uVUPitch, PU8 pOut, I32 iOutPitch,
	PU8 pTMask, PU8 pLMask, U32 uMStride,
	U32 uWantsFill, U32 uFillValue){

__asm {
	push	ebp
	mov	ebp,		esp
	push	edi
	push	esi
	push	ebx
	sub	esp,		MYLOCALSIZE

	mov	edx,	nCols		/* LOOP COUNTERS */
	shr	edx,	2
	mov	locXLoopCtr,edx		/* horizontal 4x4 blocks */
	mov	locXResetVal,edx
	mov	ecx,	nRows
	shr	ecx,	2
	mov	locYLoopCtr,ecx		/* vertical 4x4 blocks */

	mov	ebx,	uYPitch		/* LUMA */
	mov	locIStride,ebx
	shl	ebx,	2
	mov	edx,	nCols
	and	edx,	~3		/* clean off fractional blocks */
	sub	ebx,	edx
	mov	locYLineDelta,ebx	/* delta for row cross */

	mov	eax,	pV		/* CHROMA */
	mov	locV,	eax
	mov	eax,	pU
	mov	locU,	eax
	mov	eax,	uVUPitch
	mov	ebx,	nCols
	shr	ebx,	2
	sub	eax,	ebx
	mov	locCLineDelta,eax	/* delta for row cross */

	mov	ecx,	iOutPitch	/* OUT */
	mov	locOStride,ecx
	shl	ecx,	2
	mov	ebx,	nCols		/* adjust for output size */
	and	ebx,	~3		/* clean off fractional blocks */
	sub	ecx,	ebx
	mov	locOLineDelta,ecx	/* delta for row cross */

	mov	ecx,	pTMask		/* TRANSPARENCY */
	mov	edx,	pLMask
	or	edx,	ecx
	jz	NoTransparencyHandling	/* none, normal CSC */

	mov	ebx,	pLMask
	mov	locMask,edx		/* a|b */
	xor	eax,	eax
	sub	ecx,	1
	adc	eax,	0
	sub	ebx,	1
	adc	eax,	0
	sub	eax,	1
	jnb	BeginTransparency	/* only one */

	inc	ecx
	mov	locMask,ecx
	jmp	MakeAndedMask		/* t' = t & l */
FinishedAndingMasks:

BeginTransparency:
	mov	ebx,	pLMask		/* avoid writing outside viewport */
	mov	locViewMask, ebx

AllowFill:
	mov	eax,	uWantsFill
	mov	ecx,	uFillValue
	mov	ebx,	ecx
	and	ecx,	0xFFFFFF00
	jnz	CallerProvidedMany
CallerProvidedOne:
	mov	ecx,	ebx
	shl	ecx,	8
	or	ecx,	ebx
	mov	ebx,	ecx
	shl	ecx,	16
	or	ebx,	ecx
CallerProvidedMany:
	mov	locWantsFill,eax
	mov	locFillValue,ebx
	mov	eax,	uMStride
	mov	locMStride,eax
	lea	ebx,	[eax+eax*2]
	mov	locMStrideX3,ebx
	add	ebx,	eax
	mov	eax,	nCols
	add	eax,	7
	shr	eax,	3		/* bytes of transparency used in row */
	sub	ebx,	eax
	add	ebx,	1
	mov	locMLineDelta,ebx

	mov	esi,	pY
	mov	edi,	pOut
	jmp	TPrimeDitherLoop


NoTransparencyHandling:
	mov	esi,	pY
	mov	edi,	pOut

PrimeDitherLoop:
	mov	ebx,	locV			/* V input pointer */
	 mov	edx,	locU			/* U input pointer */

	mov	ebp,	DWORD PTR gpu8ClutTables[0]	/* AP table base */
	 nop

ALIGN 16

DitherLoopTop:
	/* P5: 47 clocks, no AGI penalties */
	/* ebx points to V */
	/* edx points to U */
	xor	eax,	eax
	 xor	ecx,	ecx
	
	mov	al,	BYTE PTR [ebx]		/* fetch V */
	 mov	cl,	BYTE PTR [edx]		/* fetch U */

	inc	ebx				/* advance V pointer */
	 nop
	
	mov	eax,	DWORD PTR TableV[ebp+eax*4]	/* eax = v0v0v0v0 */
	 inc	edx				/* advance U pointer */

	or	eax,	DWORD PTR TableU[ebp+ecx*4]	/* eax = vuvuvuvu (dither patterns) */
#if defined MONOCHROME
	mov	eax, 88888888h
#endif /* MONOCHROME */
 	 mov	locU,	edx			/* update U ... */

	xor	edx,	edx
	 mov	cUV,	al			/* dithered UV 3 */

	mov	cY,	BYTE PTR [esi+3]	/* Y3 */
 	 mov	dY,	BYTE PTR [esi+2]	/* Y2 */

	mov	locESI,	esi			/* save input pointer */
 	 mov	dUV,	ah			/* dithered UV 2 */

 	mov	locV,	ebx			/* update V chroma input pointer */
	 mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither03)]

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither02)]
	 mov	cY,	BYTE PTR [esi+1]	/* Y1 */

	shl	ebx,	16
	 mov	dY,	BYTE PTR [esi+0]	/* Y0 */

	add	esi,	locIStride		/* next input line (1) */
	 mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither01)]
	 
	add	esi,	locIStride		/* next input line (2) */
	 mov	locEDI,	edi			/* save output pointer */

	shr	eax,	16			/* set up next UV dither 1, 0 */
	 mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither00)]
	
	mov	cY,	BYTE PTR [esi+3]	/* Y11 */

#if defined RED_DOTS
	mov	bl, 249
#endif /* RED_DOTS */
	 mov	[edi],	ebx			/* store line 0 */
	 
	add	edi,	locOStride		/* next output line (2) */
	 mov	dY,	BYTE PTR [esi+2]	/* Y10 */
 	
	add	edi,	locOStride		/* next output line (1) */
	 mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither23)]

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither22)]
	 mov	cY,	BYTE PTR [esi+1]	/* Y9 */

	shl	ebx,	16
	 mov	dY,	BYTE PTR [esi+0]	/* Y8 */
 	
	sub	esi,	locIStride		/* next input line (1) */
	 nop

	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither21)]
 	 mov	cUV,	al			/* dithered UV 1 */
	
	mov	cY,	BYTE PTR [esi+3]	/* Y7 */
  	 nop

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither20)]
	 mov	dY,	BYTE PTR [esi+2]	/* Y6 */

	mov	[edi],	ebx			/* store line 2 */
	 mov	dUV,	ah			/* dithered UV 0	eax avail */
	
	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither13)]
	 mov	cY,	BYTE PTR [esi+1]	/* Y5 */

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither12)]
	 mov	dY,	BYTE PTR [esi+0]	/* Y4 */

	shl	ebx,	16
	 add	esi,	locIStride		/* next input line (2) */
	 
	sub	edi,	locOStride		/* next output line (1) */
	 add	esi,	locIStride		/* next input line (3) */

	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither11)]
	 mov	eax,	locXLoopCtr		/* recover loop counter */

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither10)]
	 mov	cY,	BYTE PTR [esi+3]	/* Y15 */

	mov	[edi],	ebx			/* store line 1 */
	 mov	dY,	BYTE PTR [esi+2]	/* Y14 */

	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither33)]
	 add	edi,	locOStride		/* next output line (2) */

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither32)]
	 mov	cY,	BYTE PTR [esi+1]	/* Y13 */

	shl	ebx,	16
	 mov	dY,	BYTE PTR [esi+0]	/* Y12 */

	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither31)]
	 add	edi,	locOStride		/* next output line (3) */

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither30)]
	 mov	esi,	locESI			/* recover old input pointer */

	mov	[edi],	ebx			/* store line 3 */
	 add	esi,	4			/* next block right... */
	
	mov	edi,	locEDI			/* recover old output pointer */
	 mov	ebx,	locV			/* ++ preload V input pointer */
	
	add	edi,	4			/* next block right... */
	 dec	eax				/* done moving right? */
	
	mov	edx,	locU			/* ++ preload U input pointer */
	 mov	locXLoopCtr, eax		/* store updated counter */

	jnz	DitherLoopTop			/* do next to right */

NextLineOfBlocks:
	/* EAX and ECX are no longer known cleansed due to the
	 * misprediction of the bottom of the inner loop...
	 * Failure to cleanse here will cause partials at top
	 * of loop at first block of second and subsequent rows.
	 */
	add	esi,	locYLineDelta
	 add	ebx,	locCLineDelta
	add	edi,	locOLineDelta
	 add	edx,	locCLineDelta
	dec	locYLoopCtr
	 mov	eax,	locXResetVal
	mov	locXLoopCtr, eax
	 jnz	DitherLoopTop			/* do next row of blocks */

Bail:
	xor	eax,	eax
	add	esp,	MYLOCALSIZE
	pop	ebx
	pop	esi
	pop	edi
	pop	ebp
	ret

TPrimeDitherLoop:
	mov	ebx,	locV			/* V input pointer */
	 mov	edx,	locU			/* U input pointer */
	
	mov	ebp,	locMask			/* transparency mask pointer */
	 xor	ecx,	ecx			/* clear for ZX */

ALIGN 16

TDitherLoopTop:
BeginEvenBlock:					/* even block */
	mov	eax,	locMStride		/* stride in transparency mask */
	 mov	locEDI,	edi			/* save output pointer */
	mov	cl,	[ebp]
	 mov	edi,	locMStrideX3		/* 3 * stride in transparency */
	and	cl,	[ebp+eax]
	 and	cl,	[ebp+eax*2]
	and	cl,	[ebp+edi]
	 nop
	mov	locAndMask,cl
	 mov	cl,	[ebp]
	or	cl,	[ebp+eax]
	 or	cl,	[ebp+eax*2]
	or	cl,	[ebp+edi]
	 xor	eax,	eax
	mov	locOrMask,	cl		/* save info... */
	 mov	edi,	locEDI			/* recover output pointer */
	and	cl,	MASK_EVEN
	 jz	AllUnlitEven			/* or->0's unlit */
	mov	cl,	locAndMask
	 nop
	and	cl,	MASK_EVEN
	 nop
	cmp	cl,	MASK_EVEN
	 je	AllLitEven			/* and->1's lit */
	mov	cl,	[ebp]
	 jmp	SelectiveUpdateEvenBlock

AllLitEven:

	mov	ebp,	DWORD PTR gpu8ClutTables[0]	/* AP table base */
	 nop

	/* P5: 47 clocks, no AGI penalties */
	/* ebx points to V */
	/* edx points to U */
	xor	eax,	eax
	 xor	ecx,	ecx
	
	mov	al,	BYTE PTR [ebx]		/* fetch V */
	 mov	cl,	BYTE PTR [edx]		/* fetch U */

	inc	ebx				/* advance V pointer */
	 nop
	
	mov	eax,	DWORD PTR TableV[ebp+eax*4]	/* eax = v0v0v0v0 */
	 inc	edx				/* advance U pointer */

	or	eax,	DWORD PTR TableU[ebp+ecx*4]	/* eax = vuvuvuvu (dither patterns) */
#if defined MONOCHROME
	mov	eax, 88888888h
#endif /* MONOCHROME */
 	 mov	locU,	edx			/* update U ... */

	xor	edx,	edx
	 mov	cUV,	al			/* dithered UV 3 */

	mov	cY,	BYTE PTR [esi+3]	/* Y3 */
	 mov	dY,	BYTE PTR [esi+2]	/* Y2 */

	mov	locESI,	esi			/* save input pointer */
 	 mov	dUV,	ah			/* dithered UV 2 */

 	mov	locV,	ebx			/* update V chroma input pointer */
	 mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither03)]

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither02)]
	 mov	cY,	BYTE PTR [esi+1]	/* Y1 */

	shl	ebx,	16
	 mov	dY,	BYTE PTR [esi+0]	/* Y0 */

	add	esi,	locIStride		/* next input line (1) */
	 mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither01)]
	 
	add	esi,	locIStride		/* next input line (2) */
	 mov	locEDI,	edi			/* save output pointer */

	shr	eax,	16			/* set up next UV dither 1, 0 */
	 mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither00)]
	
	mov	cY,	BYTE PTR [esi+3]	/* Y11 */
	 mov	[edi],	ebx			/* store line 0 */
	 
	add	edi,	locOStride		/* next output line (2) */
	 mov	dY,	BYTE PTR [esi+2]	/* Y10 */
	
	add	edi,	locOStride		/* next output line (1) */
	 mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither23)]

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither22)]
	 mov	cY,	BYTE PTR [esi+1]	/* Y9 */

	shl	ebx,	16
	 mov	dY,	BYTE PTR [esi+0]	/* Y8 */
	
	sub	esi,	locIStride		/* next input line (1) */
	 nop

	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither21)]
 	 mov	cUV,	al			/* dithered UV 1 */
	
	mov	cY,	BYTE PTR [esi+3]	/* Y7 */
 	 nop

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither20)]
	 mov	dY,	BYTE PTR [esi+2]	/* Y6 */

	mov	[edi],	ebx			/* store line 2 */
	 mov	dUV,	ah			/* dithered UV 0	eax avail */
	
	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither13)]
	 mov	cY,	BYTE PTR [esi+1]	/* Y5 */

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither12)]
	 mov	dY,	BYTE PTR [esi+0]	/* Y4 */

	shl	ebx,	16
	 add	esi,	locIStride		/* next input line (2) */
	 
	sub	edi,	locOStride		/* next output line (1) */
	 add	esi,	locIStride		/* next input line (3) */

	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither11)]
	 mov	eax,	locXLoopCtr		/* recover loop counter */

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither10)]
	 mov	cY,	BYTE PTR [esi+3]	/* Y15 */

	mov	[edi],	ebx			/* store line 1 */
	 mov	dY,	BYTE PTR [esi+2]	/* Y14 */

	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither33)]
	 add	edi,	locOStride		/* next output line (2) */

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither32)]
	 mov	cY,	BYTE PTR [esi+1]	/* Y13 */

	shl	ebx,	16
	 mov	dY,	BYTE PTR [esi+0]	/* Y12 */

	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither31)]
	 add	edi,	locOStride		/* next output line (3) */

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither30)]
	 mov	esi,	locESI			/* recover old input pointer */

	mov	[edi],	ebx			/* store line 3 */
	 add	esi,	4			/* next block right... */
	
	mov	edi,	locEDI			/* recover old output pointer */
	 mov	ebx,	locV			/* ++ preload V input pointer */
	
	add	edi,	4			/* next block right... */
	 dec	eax				/* done moving right? */
	
	mov	edx,	locU			/* ++ preload U input pointer */
	 mov	locXLoopCtr, eax		/* store updated counter */

	jz	TNextLineOfBlocks		/* do next to right */

BeginOddBlock:
	mov	cl,	locAndMask
	 mov	ebp,	locMask
	and	cl,	MASK_ODD
	 mov	al,	locOrMask
	cmp	cl,	MASK_ODD
	 je	AllLitOdd
	and	al,	MASK_ODD
	 jz	AllUnlitOdd
	jmp	SelectiveUpdateOddBlock

AllLitOdd:

	mov	ebp,	DWORD PTR gpu8ClutTables[0]	/* AP table base */
	 nop

	/* P5: 47 clocks, no AGI penalties */
	/* ebx points to V */
	/* edx points to U */
	xor	eax,	eax
	 xor	ecx,	ecx
	
	mov	al,	BYTE PTR [ebx]		/* fetch V */
	 mov	cl,	BYTE PTR [edx]		/* fetch U */

	inc	ebx				/* advance V pointer */
	 nop
	
	mov	eax,	DWORD PTR TableV[ebp+eax*4]	/* eax = v0v0v0v0 */
	 inc	edx				/* advance U pointer */

	or	eax,	DWORD PTR TableU[ebp+ecx*4]	/* eax = vuvuvuvu (dither patterns) */
#if defined MONOCHROME
	mov	eax, 88888888h
#endif /* MONOCHROME */
 	 mov	locU,	edx			/* update U ... */

	xor	edx,	edx
	 mov	cUV,	al			/* dithered UV 3 */

	mov	cY,	BYTE PTR [esi+3]	/* Y3 */
	 mov	dY,	BYTE PTR [esi+2]	/* Y2 */

	mov	locESI,	esi			/* save input pointer */
 	 mov	dUV,	ah			/* dithered UV 2 */

 	mov	locV,	ebx			/* update V chroma input pointer */
	 mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither03)]

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither02)]
	 mov	cY,	BYTE PTR [esi+1]	/* Y1 */

	shl	ebx,	16
	 mov	dY,	BYTE PTR [esi+0]	/* Y0 */

	add	esi,	locIStride		/* next input line (1) */
	 mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither01)]
	 
	add	esi,	locIStride		/* next input line (2) */
	 mov	locEDI,	edi			/* save output pointer */

	shr	eax,	16			/* set up next UV dither 1, 0 */
	 mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither00)]
	
	mov	cY,	BYTE PTR [esi+3]	/* Y11 */
	 mov	[edi],	ebx			/* store line 0 */
	 
	add	edi,	locOStride		/* next output line (2) */
	 mov	dY,	BYTE PTR [esi+2]	/* Y10 */
	
	add	edi,	locOStride		/* next output line (1) */
	 mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither23)]

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither22)]
	 mov	cY,	BYTE PTR [esi+1]	/* Y9 */

	shl	ebx,	16
	 mov	dY,	BYTE PTR [esi+0]	/* Y8 */
	
	sub	esi,	locIStride		/* next input line (1) */
	 nop

	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither21)]
 	 mov	cUV,	al			/* dithered UV 1 */
	
	mov	cY,	BYTE PTR [esi+3]	/* Y7 */
 	 nop

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither20)]
	 mov	dY,	BYTE PTR [esi+2]	/* Y6 */

	mov	[edi],	ebx			/* store line 2 */
	 mov	dUV,	ah			/* dithered UV 0	eax avail */
	
	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither13)]
	 mov	cY,	BYTE PTR [esi+1]	/* Y5 */

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither12)]
	 mov	dY,	BYTE PTR [esi+0]	/* Y4 */

	shl	ebx,	16
	 add	esi,	locIStride		/* next input line (2) */
	 
	sub	edi,	locOStride		/* next output line (1) */
	 add	esi,	locIStride		/* next input line (3) */

	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither11)]
	 mov	eax,	locXLoopCtr		/* recover loop counter */

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither10)]
	 mov	cY,	BYTE PTR [esi+3]	/* Y15 */

	mov	[edi],	ebx			/* store line 1 */
	 mov	dY,	BYTE PTR [esi+2]	/* Y14 */

	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither33)]
	 add	edi,	locOStride		/* next output line (2) */

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither32)]
	 mov	cY,	BYTE PTR [esi+1]	/* Y13 */

	shl	ebx,	16
	 mov	dY,	BYTE PTR [esi+0]	/* Y12 */

	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither31)]
	 add	edi,	locOStride		/* next output line (3) */

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither30)]
	 mov	esi,	locESI			/* recover old input pointer */

	mov	[edi],	ebx			/* store line 3 */
	 add	esi,	4			/* next block right... */
	
	mov	edi,	locEDI			/* recover old output pointer */
	 mov	ebx,	locV			/* ++ preload V input pointer */
	
	add	edi,	4			/* next block right... */
	 dec	eax				/* done moving right? */
	
	mov	edx,	locU			/* ++ preload U input pointer */
	 mov	locXLoopCtr, eax		/* store updated counter */

	jz	TNextLineOfBlocks
	
	mov	ebp,	locMask
	 mov	edx,	locViewMask
	cmp	edx,	1
	 inc	ebp
	sbb	edx,	-1
	 mov	locMask,ebp
	mov	locViewMask,edx
	 mov	edx,	locU
	jmp	TDitherLoopTop			/* next pair to right */

TNextLineOfBlocks:
	add	esi,	locYLineDelta
	 add	edi,	locOLineDelta
	mov	locESI,	esi
	 mov	esi,	locMLineDelta
	mov	ebp,	locMask
	 mov	locEDI,	edi
	add	ebx,	locCLineDelta
	 add	edx,	locCLineDelta
	mov	edi,	locXResetVal
	 lea	ebp,	[ebp+esi]
	mov	locXLoopCtr,edi
	 mov	edi,	locViewMask
	cmp	edi,	0
	 je	TDontUpdateViewportMaskPointer
	lea	edi,	[edi+esi]
	 nop
	mov	locViewMask,edi
	 nop
TDontUpdateViewportMaskPointer:
	nop
	 mov	esi,	locYLoopCtr
	dec	esi
	 mov	edi,	locEDI
	mov	locYLoopCtr,esi
	 mov	locMask,ebp
	mov	esi,	locESI
	 jnz	TDitherLoopTop
	jmp	Bail

	/* EBX locV+mem, EDX locU+mem, EDI pDIB, ESI pLuma
	 * EAX 0 via xor, ECX 0 via xor, locXLoopCtr, locMask
	 */
AllUnlitEven:
	cmp	locWantsFill,0
	 jz	FinishAllUnlitEven
	cmp	locViewMask,0
	 jz	EasyFillAllUnlitEven
	DoSelFillV(EULZero,locMStride,0,C,eax,ah,al,SHIFT_EVEN,lF1,lF4)
	mov	locEDI,	edi
	add	edi,	locOStride
	DoSelFillV(EULOne,locMStride,esi*1,C,eax,ah,al,SHIFT_EVEN,lF1,lF4)
	add	edi,	locOStride
	DoSelFillV(EULTwo,locMStride,esi*2,C,eax,ah,al,SHIFT_EVEN,lF1,lF4)
	add	edi,	locOStride
	DoSelFillV(EULThree,locMStrideX3,esi*1,C,eax,ah,al,SHIFT_EVEN,lF1,lF4)
	mov	edi,	locEDI
	jmp	FinishAllUnlitEven

EasyFillAllUnlitEven:
	mov	eax,	locFillValue
	 mov	ecx,	locOStride
	mov	[edi],	eax	
	 nop
	mov	[edi+ecx],eax
	 nop
	mov	[edi+ecx*2],eax
	 add	edi, ecx
	nop
	 nop
	mov	[edi+ecx*2],eax
	 sub	edi,	ecx
FinishAllUnlitEven:
	inc	ebx
	 inc	edx
	mov	locV,	ebx
	 mov	ebp,	locXLoopCtr
	xor	eax,	eax
	 xor	ecx,	ecx
	add	edi,	4
	 add	esi,	4
	dec	ebp
	 mov	locU,	edx
	mov	locXLoopCtr,	ebp
	 jz	TNextLineOfBlocks
	jmp	BeginOddBlock

AllUnlitOdd:
	/* If ending row, must not advance mask pointer, as row end code
	 * will do that, if continuing in row, must advance mask pointer.
	 */
	cmp	locWantsFill,0
	 jz	FinishAllUnlitOdd
	cmp	locViewMask,0
	 jz	EasyFillAllUnlitOdd
	DoSelFillV(OULZero,locMStride,0,C,eax,ah,al,SHIFT_ODD,lF1,lF4)
	mov	locEDI,	edi
	add	edi,	locOStride
	DoSelFillV(OULOne,locMStride,esi*1,C,eax,ah,al,SHIFT_ODD,lF1,lF4)
	add	edi,	locOStride
	DoSelFillV(OULTwo,locMStride,esi*2,C,eax,ah,al,SHIFT_ODD,lF1,lF4)
	add	edi,	locOStride
	DoSelFillV(OULThree,locMStrideX3,esi*1,C,eax,ah,al,SHIFT_ODD,lF1,lF4)
	mov	edi,	locEDI
	jmp	FinishAllUnlitOdd

EasyFillAllUnlitOdd:
	mov	eax,	locFillValue
	 mov	ecx,	locOStride
	mov	[edi],	eax
	 nop
	mov	[edi+ecx],eax
	 nop
	mov	[edi+ecx*2],eax
	 add	edi,	ecx
	nop
	 nop
	mov	[edi+ecx*2],eax
	 sub	edi,	ecx
FinishAllUnlitOdd:
	inc	ebx
	 inc	edx
	mov	locV,	ebx
	 mov	locU,	edx
	mov	ebp,	locXLoopCtr
	 xor	eax,	eax
	add	edi,	4
	 xor	ecx,	ecx
	add	esi,	4
	 dec	ebp
	mov	locXLoopCtr,	ebp
	 jz	TNextLineOfBlocks
	mov	ebp,	locMask			/* preload for even block */
	 mov	edx,	locViewMask
	cmp	edx,	1
	 inc	ebp
	sbb	edx,	-1
	 mov	locMask,ebp
	mov	locViewMask,edx
	 mov	edx,	locU
	jmp	BeginEvenBlock

SelectiveUpdateEvenBlock:
	cmp	locWantsFill,0
	 jnz	FilledUpdateEvenBlock

	mov	ebp,	DWORD PTR gpu8ClutTables[0]	/* AP table base */
	 nop

	/* P5: 47 clocks, no AGI penalties */
	/* ebx points to V */
	/* edx points to U */
	xor	eax,	eax
	 xor	ecx,	ecx
	
	mov	al,	BYTE PTR [ebx]		/* fetch V */
	 mov	cl,	BYTE PTR [edx]		/* fetch U */

	inc	ebx				/* advance V pointer */
	 nop
	
	mov	eax,	DWORD PTR TableV[ebp+eax*4]	/* eax = v0v0v0v0 */
	 inc	edx				/* advance U pointer */

	or	eax,	DWORD PTR TableU[ebp+ecx*4]	/* eax = vuvuvuvu (dither patterns) */
#if defined MONOCHROME
	mov	eax, 88888888h
#endif /* MONOCHROME */
 	 mov	locU,	edx			/* update U ... */

	xor	edx,	edx
	 mov	cUV,	al			/* dithered UV 3 */

	mov	cY,	BYTE PTR [esi+3]	/* Y3 */
	 mov	dY,	BYTE PTR [esi+2]	/* Y2 */

	mov	locESI,	esi			/* save input pointer */
 	 mov	dUV,	ah			/* dithered UV 2 */

 	mov	locV,	ebx			/* update V chroma input pointer */
	 mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither03)]

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither02)]
	 mov	cY,	BYTE PTR [esi+1]	/* Y1 */

	shl	ebx,	16
	 mov	dY,	BYTE PTR [esi+0]	/* Y0 */

	add	esi,	locIStride		/* next input line (1) */
	 mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither01)]
	 
	add	esi,	locIStride		/* next input line (2) */
	 mov	locEDI,	edi			/* save output pointer */

	shr	eax,	16			/* set up next UV dither 1, 0 */
	 mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither00)]
	
	mov	cY,	BYTE PTR [esi+3]	/* Y11 */
	 /*mov	[edi],	ebx		*/	/* store line 0 */
	DoSelUpdate(EvenLineZero,locMStride,0,dY,ebx,bh,bl,SHIFT_EVEN)
	 nop
	 	 
	add	edi,	locOStride		/* next output line (2) */
	 mov	dY,	BYTE PTR [esi+2]	/* Y10 */
	
	add	edi,	locOStride		/* next output line (1) */
	 mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither23)]

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither22)]
	 mov	cY,	BYTE PTR [esi+1]	/* Y9 */

	shl	ebx,	16
	 mov	dY,	BYTE PTR [esi+0]	/* Y8 */
	

	sub	esi,	locIStride		/* next input line (1) */
	 nop

	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither21)]
 	 mov	cUV,	al			/* dithered UV 1 */
	
	mov	cY,	BYTE PTR [esi+3]	/* Y7 */
 	 nop

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither20)]
	 mov	dY,	BYTE PTR [esi+2]	/* Y6 */

	/*mov	[edi],	ebx		*/	/* store line 2 */
	DoSelUpdate(EvenLineTwo,locMStride,esi*2,dUV,ebx,bh,bl,SHIFT_EVEN)
	nop
	 mov	dUV,	ah			/* dithered UV 0	eax avail */
	
	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither13)]
	 mov	cY,	BYTE PTR [esi+1]	/* Y5 */

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither12)]
	 mov	dY,	BYTE PTR [esi+0]	/* Y4 */

	shl	ebx,	16
	 add	esi,	locIStride		/* next input line (2) */
	 
	sub	edi,	locOStride		/* next output line (1) */
	 add	esi,	locIStride		/* next input line (3) */

	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither11)]
	 mov	eax,	locXLoopCtr		/* recover loop counter */

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither10)]
	 mov	cY,	BYTE PTR [esi+3]	/* Y15 */

	/*mov	[edi],	ebx		*/	/* store line 1 */
	DoSelUpdate(EvenLineOne,locMStride,esi*1,dY,ebx,bh,bl,SHIFT_EVEN)
	nop
	 mov	dY,	BYTE PTR [esi+2]	/* Y14 */

	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither33)]
	 add	edi,	locOStride		/* next output line (2) */

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither32)]
	 mov	cY,	BYTE PTR [esi+1]	/* Y13 */

	shl	ebx,	16
	 mov	dY,	BYTE PTR [esi+0]	/* Y12 */

	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither31)]
	 add	edi,	locOStride		/* next output line (3) */

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither30)]
	 mov	esi,	locESI			/* recover old input pointer */

	/*mov	[edi],	ebx		*/	/* store line 3 */
	DoSelUpdate(EvenLineThree,locMStrideX3,esi*1,dl,ebx,bh,bl,SHIFT_EVEN)
	nop
	 add	esi,	4			/* next block right... */
	
	mov	edi,	locEDI			/* recover old output pointer */
	 mov	ebx,	locV			/* ++ preload V input pointer */
	
	add	edi,	4			/* next block right... */
	 dec	eax				/* done moving right? */
	
	mov	edx,	locU			/* ++ preload U input pointer */
	 mov	locXLoopCtr, eax		/* store updated counter */

	jz	TNextLineOfBlocks
	 jmp	BeginOddBlock

SelectiveUpdateOddBlock:
	cmp	locWantsFill,0
	 jnz	FilledUpdateOddBlock

	mov	ebp,	DWORD PTR gpu8ClutTables[0]	/* AP table base */
	 nop

	/* P5: 47 clocks, no AGI penalties */
	/* ebx points to V */
	/* edx points to U */
	xor	eax,	eax
	 xor	ecx,	ecx
	
	mov	al,	BYTE PTR [ebx]		/* fetch V */
	 mov	cl,	BYTE PTR [edx]		/* fetch U */

	inc	ebx				/* advance V pointer */
	 nop
	
	mov	eax,	DWORD PTR TableV[ebp+eax*4]	/* eax = v0v0v0v0 */
	 inc	edx				/* advance U pointer */

	or	eax,	DWORD PTR TableU[ebp+ecx*4]	/* eax = vuvuvuvu (dither patterns) */
#if defined MONOCHROME
	mov	eax, 88888888h
#endif /* MONOCHROME */
 	 mov	locU,	edx			/* update U ... */

	xor	edx,	edx
	 mov	cUV,	al			/* dithered UV 3 */

	mov	cY,	BYTE PTR [esi+3]	/* Y3 */
	 mov	dY,	BYTE PTR [esi+2]	/* Y2 */

	mov	locESI,	esi			/* save input pointer */
 	 mov	dUV,	ah			/* dithered UV 2 */

 	mov	locV,	ebx			/* update V chroma input pointer */
	 mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither03)]

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither02)]
	 mov	cY,	BYTE PTR [esi+1]	/* Y1 */

	shl	ebx,	16
	 mov	dY,	BYTE PTR [esi+0]	/* Y0 */

	add	esi,	locIStride		/* next input line (1) */
	 mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither01)]
	 
	add	esi,	locIStride		/* next input line (2) */
	 mov	locEDI,	edi			/* save output pointer */

	shr	eax,	16			/* set up next UV dither 1, 0 */
	 mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither00)]
	
	mov	cY,	BYTE PTR [esi+3]	/* Y11 */
	 /*mov	[edi],	ebx		*/	/* store line 0 */
	DoSelUpdate(OddLineZero,locMStride,0,dY,ebx,bh,bl,SHIFT_ODD)
	 
	add	edi,	locOStride		/* next output line (2) */
	 mov	dY,	BYTE PTR [esi+2]	/* Y10 */
	
	add	edi,	locOStride		/* next output line (1) */
	 mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither23)]

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither22)]
	 mov	cY,	BYTE PTR [esi+1]	/* Y9 */

	shl	ebx,	16
	 mov	dY,	BYTE PTR [esi+0]	/* Y8 */
	
	sub	esi,	locIStride		/* next input line (1) */
	 nop

	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither21)]
 	 mov	cUV,	al			/* dithered UV 1 */
	
	mov	cY,	BYTE PTR [esi+3]	/* Y7 */
 	 nop

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither20)]
	 mov	dY,	BYTE PTR [esi+2]	/* Y6 */

	/*mov	[edi],	ebx		*/	/* store line 2 */
	DoSelUpdate(OddLineTwo,locMStride,esi*2,dUV,ebx,bh,bl,SHIFT_ODD)
	 mov	dUV,	ah			/* dithered UV 0	eax avail */
	
	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither13)]
	 mov	cY,	BYTE PTR [esi+1]	/* Y5 */

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither12)]
	 mov	dY,	BYTE PTR [esi+0]	/* Y4 */

	shl	ebx,	16
	 add	esi,	locIStride		/* next input line (2) */
	 
	sub	edi,	locOStride		/* next output line (1) */
	 add	esi,	locIStride		/* next input line (3) */

	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither11)]
	 mov	eax,	locXLoopCtr		/* recover loop counter */

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither10)]
	 mov	cY,	BYTE PTR [esi+3]	/* Y15 */

	/*mov	[edi],	ebx		*/	/* store line 1 */
	DoSelUpdate(OddLineOne,locMStride,esi*1,dY,ebx,bh,bl,SHIFT_ODD)
	 mov	dY,	BYTE PTR [esi+2]	/* Y14 */

	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither33)]
	 add	edi,	locOStride		/* next output line (2) */

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither32)]
	 mov	cY,	BYTE PTR [esi+1]	/* Y13 */

	shl	ebx,	16
	 mov	dY,	BYTE PTR [esi+0]	/* Y12 */

	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither31)]
	 add	edi,	locOStride		/* next output line (3) */

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither30)]
	 mov	esi,	locESI			/* recover old input pointer */

	/*mov	[edi],	ebx		*/	/* store line 3 */
	DoSelUpdate(OddLineThree,locMStrideX3,esi*1,dl,ebx,bh,bl,SHIFT_ODD)
	 add	esi,	4			/* next block right... */
	
	mov	edi,	locEDI			/* recover old output pointer */
	 mov	ebx,	locV			/* ++ preload V input pointer */
	
	add	edi,	4			/* next block right... */
	 dec	eax				/* done moving right? */
	
	mov	edx,	locU			/* ++ preload U input pointer */
	 mov	locXLoopCtr, eax		/* store updated counter */

	jz	TNextLineOfBlocks
	 nop

	mov	ebp,	locMask
	 mov	edx,	locViewMask
	cmp	edx,	1
	 inc	ebp
	sbb	edx,	-1
	 mov	locMask,ebp
	mov	locViewMask,edx
	 mov	edx,	locU
	jmp	TDitherLoopTop			/* next pair to right */

MakeAndedMask:
	mov	esi,	pLMask
	mov	edi,	pTMask
	mov	edx,	nRows
AndRowLoopTop:
	mov	ecx,	nCols
	add	ecx,	31
	shr	ecx,	5
AndColLoopTop:
	mov	eax,	[esi+ecx*4-4]
	mov	ebx,	[edi+ecx*4-4]
	and	ebx,	eax
	mov	[edi+ecx*4-4],ebx
	dec	ecx
	jnz	AndColLoopTop
	add	esi,	uMStride
	add	edi,	uMStride
	dec	edx
	jnz	AndRowLoopTop
	jmp	FinishedAndingMasks

FilledUpdateEvenBlock:

	mov	ebp,	DWORD PTR gpu8ClutTables[0]	/* AP table base */
	 nop

	/* P5: 47 clocks, no AGI penalties */
	/* ebx points to V */
	/* edx points to U */
	xor	eax,	eax
	 xor	ecx,	ecx
	
	mov	al,	BYTE PTR [ebx]		/* fetch V */
	 mov	cl,	BYTE PTR [edx]		/* fetch U */

	inc	ebx				/* advance V pointer */
	 nop
	
	mov	eax,	DWORD PTR TableV[ebp+eax*4]	/* eax = v0v0v0v0 */
	 inc	edx				/* advance U pointer */

	or	eax,	DWORD PTR TableU[ebp+ecx*4]	/* eax = vuvuvuvu (dither patterns) */
#if defined MONOCHROME
	mov	eax, 88888888h
#endif /* MONOCHROME */
 	 mov	locU,	edx			/* update U ... */

	xor	edx,	edx
	 mov	cUV,	al			/* dithered UV 3 */

	mov	cY,	BYTE PTR [esi+3]	/* Y3 */
	 mov	dY,	BYTE PTR [esi+2]	/* Y2 */

	mov	locESI,	esi			/* save input pointer */
 	 mov	dUV,	ah			/* dithered UV 2 */

 	mov	locV,	ebx			/* update V chroma input pointer */
	 mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither03)]

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither02)]
	 mov	cY,	BYTE PTR [esi+1]	/* Y1 */

	shl	ebx,	16
	 mov	dY,	BYTE PTR [esi+0]	/* Y0 */

	add	esi,	locIStride		/* next input line (1) */
	 mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither01)]
	 
	add	esi,	locIStride		/* next input line (2) */
	 mov	locEDI,	edi			/* save output pointer */

	shr	eax,	16			/* set up next UV dither 1, 0 */
	 mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither00)]
	
	mov	cY,	BYTE PTR [esi+3]	/* Y11 */
	 /*mov	[edi],	ebx		*/	/* store line 0 */
	DoSelFillV(EvenLineZero,locMStride,0,D,ebx,bh,bl,SHIFT_EVEN,lF1,lF4)
	 nop
	  
	add	edi,	locOStride		/* next output line (2) */
	 mov	dY,	BYTE PTR [esi+2]	/* Y10 */
	
	add	edi,	locOStride		/* next output line (1) */
	 mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither23)]

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither22)]
	 mov	cY,	BYTE PTR [esi+1]	/* Y9 */

	shl	ebx,	16
	 mov	dY,	BYTE PTR [esi+0]	/* Y8 */
	
	sub	esi,	locIStride		/* next input line (1) */
	 nop

	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither21)]
 	 mov	cUV,	al			/* dithered UV 1 */
	
	mov	cY,	BYTE PTR [esi+3]	/* Y7 */
 	 nop

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither20)]
	 mov	dY,	BYTE PTR [esi+2]	/* Y6 */

	/*mov	[edi],	ebx		*/	/* store line 2 */
	DoSelFillV(EvenLineTwo,locMStride,esi*2,D,ebx,bh,bl,SHIFT_EVEN,lF1,lF4)
	
	nop
	 mov	dUV,	ah			/* dithered UV 0	eax avail */
	
	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither13)]
	 mov	cY,	BYTE PTR [esi+1]	/* Y5 */

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither12)]
	 mov	dY,	BYTE PTR [esi+0]	/* Y4 */

	shl	ebx,	16
	 add	esi,	locIStride		/* next input line (2) */
	 
	sub	edi,	locOStride		/* next output line (1) */
	 add	esi,	locIStride		/* next input line (3) */

	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither11)]
	 mov	eax,	locXLoopCtr		/* recover loop counter */

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither10)]
	 mov	cY,	BYTE PTR [esi+3]	/* Y15 */

	/*mov	[edi],	ebx		*/	/* store line 1 */
	DoSelFillV(EvenLineOne,locMStride,esi*1,D,ebx,bh,bl,SHIFT_EVEN,lF1,lF4)
	nop
	 mov	dY,	BYTE PTR [esi+2]	/* Y14 */

	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither33)]
	 add	edi,	locOStride		/* next output line (2) */

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither32)]
	 mov	cY,	BYTE PTR [esi+1]	/* Y13 */

	shl	ebx,	16
	 mov	dY,	BYTE PTR [esi+0]	/* Y12 */

	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither31)]
	 add	edi,	locOStride		/* next output line (3) */

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither30)]
	 mov	esi,	locESI			/* recover old input pointer */

	/*mov	[edi],	ebx		*/	/* store line 3 */
	DoSelFillV(EvenLineThree,locMStrideX3,esi*1,D,ebx,bh,bl,SHIFT_EVEN,lF1,lF4)
	nop
	 add	esi,	4			/* next block right... */
	
	mov	edi,	locEDI			/* recover old output pointer */
	 mov	ebx,	locV			/* ++ preload V input pointer */
	
	add	edi,	4			/* next block right... */
	 dec	eax				/* done moving right? */
	
	mov	edx,	locU			/* ++ preload U input pointer */
	 mov	locXLoopCtr, eax		/* store updated counter */

	jz	TNextLineOfBlocks
	 jmp	BeginOddBlock

FilledUpdateOddBlock:

	mov	ebp,	DWORD PTR gpu8ClutTables[0]	/* AP table base */
	 nop

	/* P5: 47 clocks, no AGI penalties */
	/* ebx points to V */
	/* edx points to U */
	xor	eax,	eax
	 xor	ecx,	ecx
	
	mov	al,	BYTE PTR [ebx]		/* fetch V */
	 mov	cl,	BYTE PTR [edx]		/* fetch U */

	inc	ebx				/* advance V pointer */
	 nop
	
	mov	eax,	DWORD PTR TableV[ebp+eax*4]	/* eax = v0v0v0v0 */
	 inc	edx				/* advance U pointer */

	or	eax,	DWORD PTR TableU[ebp+ecx*4]	/* eax = vuvuvuvu (dither patterns) */
#if defined MONOCHROME
	mov	eax, 88888888h
#endif /* MONOCHROME */
 	 mov	locU,	edx			/* update U ... */

	xor	edx,	edx
	 mov	cUV,	al			/* dithered UV 3 */

	mov	cY,	BYTE PTR [esi+3]	/* Y3 */
	 mov	dY,	BYTE PTR [esi+2]	/* Y2 */

	mov	locESI,	esi			/* save input pointer */
 	 mov	dUV,	ah			/* dithered UV 2 */

 	mov	locV,	ebx			/* update V chroma input pointer */
	 mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither03)]

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither02)]
	 mov	cY,	BYTE PTR [esi+1]	/* Y1 */

	shl	ebx,	16
	 mov	dY,	BYTE PTR [esi+0]	/* Y0 */

	add	esi,	locIStride		/* next input line (1) */
	 mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither01)]
	 
	add	esi,	locIStride		/* next input line (2) */
	 mov	locEDI,	edi			/* save output pointer */

	shr	eax,	16			/* set up next UV dither 1, 0 */
	 mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither00)]
	
	mov	cY,	BYTE PTR [esi+3]	/* Y11 */
	 /*mov	[edi],	ebx		*/	/* store line 0 */
	DoSelFillV(OddLineZero,locMStride,0,D,ebx,bh,bl,SHIFT_ODD,lF1,lF4)
	 nop
	  
	add	edi,	locOStride		/* next output line (2) */
	 mov	dY,	BYTE PTR [esi+2]	/* Y10 */
	
	add	edi,	locOStride		/* next output line (1) */
	 mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither23)]

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither22)]
	 mov	cY,	BYTE PTR [esi+1]	/* Y9 */

	shl	ebx,	16
	 mov	dY,	BYTE PTR [esi+0]	/* Y8 */
	
	sub	esi,	locIStride		/* next input line (1) */
	 nop

	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither21)]
 	 mov	cUV,	al			/* dithered UV 1 */
	
	mov	cY,	BYTE PTR [esi+3]	/* Y7 */
 	 nop

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither20)]
	 mov	dY,	BYTE PTR [esi+2]	/* Y6 */

	/*mov	[edi],	ebx		*/	/* store line 2 */
	DoSelFillV(OddLineTwo,locMStride,esi*2,D,ebx,bh,bl,SHIFT_ODD,lF1,lF4)
	nop
	 mov	dUV,	ah			/* dithered UV 0	eax avail */
	
	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither13)]
	 mov	cY,	BYTE PTR [esi+1]	/* Y5 */

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither12)]
	 mov	dY,	BYTE PTR [esi+0]	/* Y4 */

	shl	ebx,	16
	 add	esi,	locIStride		/* next input line (2) */
	 
	sub	edi,	locOStride		/* next output line (1) */
	 add	esi,	locIStride		/* next input line (3) */

	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither11)]
	 mov	eax,	locXLoopCtr		/* recover loop counter */

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither10)]
	 mov	cY,	BYTE PTR [esi+3]	/* Y15 */

	/*mov	[edi],	ebx		*/	/* store line 1 */
	DoSelFillV(OddLineOne,locMStride,esi*1,D,ebx,bh,bl,SHIFT_ODD,lF1,lF4)
	nop
	 mov	dY,	BYTE PTR [esi+2]	/* Y14 */

	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither33)]
	 add	edi,	locOStride		/* next output line (2) */

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither32)]
	 mov	cY,	BYTE PTR [esi+1]	/* Y13 */

	shl	ebx,	16
	 mov	dY,	BYTE PTR [esi+0]	/* Y12 */

	mov	bh,	BYTE PTR [ebp+ecx+YDITHER_SCALE*(Ydither31)]
	 add	edi,	locOStride		/* next output line (3) */

	mov	bl,	BYTE PTR [ebp+edx+YDITHER_SCALE*(Ydither30)]
	 mov	esi,	locESI			/* recover old input pointer */

	/*mov	[edi],	ebx		*/	/* store line 3 */
	DoSelFillV(OddLineThree,locMStrideX3,esi*1,D,ebx,bh,bl,SHIFT_ODD,lF1,lF4)
	nop
	 add	esi,	4			/* next block right... */
	
	mov	edi,	locEDI			/* recover old output pointer */
	 mov	ebx,	locV			/* ++ preload V input pointer */
	
	add	edi,	4			/* next block right... */
	 dec	eax				/* done moving right? */
	
	mov	edx,	locU			/* ++ preload U input pointer */
	 mov	locXLoopCtr, eax		/* store updated counter */

	jz	TNextLineOfBlocks
	 nop

	mov	ebp,	locMask
	 mov	edx,	locViewMask
	cmp	edx,	1
	 inc	ebp
	sbb	edx,	-1
	 mov	locMask,ebp
	mov	locViewMask,edx
	 mov	edx,	locU
	jmp	TDitherLoopTop			/* next pair to right */

} /* end asm block */
} /* end function */
#endif /* Use no ASM */
