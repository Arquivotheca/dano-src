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
 * YRGB8.H -- function declarations and definitions needed in decompress
 *              routines (decproc.c and decproci.c).  
 *************************************************************************/

#ifdef __YRGB8_H__
#pragma message("rtcc.h: multiple inclusion")
#else /* __YRGB8_H__ */
#define __YRGB8_H__

typedef struct { I32 y,u,v; } COLOR;
typedef COLOR * PTR_COLOR;

static struct { 
	U8 uUDither, uVDither; 
} gaDither[4] = {{2, 1}, {1, 2}, {0, 3}, {3, 0}};

#define USE_844
/* 844 means 8 bits of Y, 4 bits of U and 4 bits of V (yyyyyyyyvvvvuuuu) */
/* 844 is always used -- other formats aren't supported */

#define MAX_COL 256
#define YSIZ  16
#define MAG_NUM_NEAREST  6	/* # nearest neighbors to check */
#define MIN_Y_DITHER (-16)	/* change if dither factors change in clutap.c */
#define MAX_Y_DITHER 12		/* change if dither factors change in clutap.c */

/* CLUT_TABLE_SIZE is determined by calculating the fully
 * dithered Y range, multiplying by 256 (for the U, V portion of the
 * address) and adding the TableU and TableV size. The undithered Y
 * range is [0..255], plus a dither range of [-16..12], yielding a
 * fully dithered Y range of [-16..267].
 * TableU and TableV are each 256 entries, with 4 bytes per entry, which
 * comes to 2*256*4.
 */
#define CLUT_TABLE_SIZE (256*(256+(MAX_Y_DITHER-MIN_Y_DITHER)))
#define TABLE_U_SIZE (256*4)
#define TABLE_V_SIZE (256*4)

#if 1
/* These macros were converted to use integer math to remove floating point */
/* from the codec */
#define YFROM(R, G, B) (U32)((( 16843 * R) + ( 33030 * G) + (  6423 * B) + 65536*16) /65536)
#define UFROM(R, G, B) (U32)((( -9699 * R) + (-19071 * G) + ( 28770 * B) + 65536*128)/65536)
#define VFROM(R, G, B) (U32)((( 28770 * R) + (-24117 * G) + ( -4653 * B) + 65536*128)/65536)
#else
#define YFROM(R, G, B) (int)(( 0.257 * R) + ( 0.504 * G) + ( 0.098 * B) + 16.)
#define UFROM(R, G, B) (int)((-0.148 * R) + (-0.291 * G) + ( 0.439 * B) + 128.)
#define VFROM(R, G, B) (int)(( 0.439 * R) + (-0.368 * G) + (-0.071 * B) + 128.)
#endif /* 0 */

#if defined USE_844	/* table index is yyyyyyyyvvvvuuuu */
#define TBLIDX(y,u,v) (((v)>>4<<4) + ((u)>>4) + (((y) - MIN_Y_DITHER)<<8))
#elif defined USE_448  
#define TBLIDX(y,u,v) (((u)>>4<<12) + ((v)>>4<<8)  + (y))
#else /* table index is 00vvvuuu0yyyyyyy */
ERROR - must be USE_844 of USE_448
#endif /* USE_844 */

#if !defined(RAND_MAX)
#define RAND_MAX 0x7fffffff /* bsd */
/* #define RAND_MAX 0x7fff */ /* system v */
#endif /* RAND_MAX */
#define RANDOM(x) (I32)((((I32)(x)) * (I32)rand())/(I32)RAND_MAX)
#define CLAMP8(x) (U8)((x) > 255 ? 255 : ((x) < 0 ? 0 : (x)))

extern PU8 gpu8ClutTables;
extern BGR_PALETTE gpalActivePalette;		/* The current active palette */

PIA_RETURN_STATUS ComputeDynamicClut();

/* Get aquires the configuration MUTEX, be sure to Release it. */
void ColorOutReleasePaletteConfiguration(void);
BGR_ENTRY const * ColorOutGetPaletteConfiguration(I32 *iFirst, I32 *iLast);

void InitPaletteConfiguration(void);
void DeInitPaletteConfiguration(void);

extern U8 PalTable[];  /* From cluttab.c */
extern U8 CP_PalTable[];  /* From cluttab.c */

I32 C_YVU9toCLUT8(U32 nRows, U32 nCols, PU8 pY, PU8 pV, PU8 pU,
	U32 uYPitch, U32 uVUPitch, PU8 pOut, I32 iOutPitch,
	PU8 pTMask, PU8 pLMask, U32 uMStride,
	U32 uWantsFill, U32 uFillValue, U32 uZoomX, U32 uZoomY, U32 uCCFlags);

I32 C_YVU9toConfigurablePalette(U32 nRows, U32 nCols, PU8 pY, PU8 pV, PU8 pU,
	U32 uYPitch, U32 uVUPitch, PU8 pOut, I32 iOutPitch,
	PU8 pTMask, PU8 pLMask, U32 uMStride,
	U32 uWantsFill, U32 uFillValue, U32 uZoomX, U32 uZoomY, U32 uCCFlags);

I32 C_YVU9toActivePalette(U32 nRows, U32 nCols, PU8 pY, PU8 pV, PU8 pU,
	U32 uYPitch, U32 uVUPitch, PU8 pOut, I32 iOutPitch,
	PU8 pTMask, PU8 pLMask, U32 uMStride,
	U32 uWantsFill, U32 uFillValue);

#endif /* __YRGB8_H__ */
