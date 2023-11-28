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
 *               Copyright (c) 1994-1995 Intel Corporation.
 *                         All Rights Reserved.
 *
 */

/*************************************************************************
 *	Xrgb32.c				prototype in: decproci.h
 * 
 *  C_YVU9toRGB32() --
 *	Convert YVU9 planar to XRGB32.
 *  If there is a transparency or local decode mask, apply these masks and
 *  render the specified pixels.
 *
 *************************************************************************/

/* ENDIAN-NESS DEPENDENCIES:
 *   This file contains endian dependencies throughout.
 */

#define CAN_DO_XRGB32	/* cpz - QTW */

#ifdef CAN_DO_XRGB32	/* (RS) */

#include "datatype.h"
#include "pia_main.h"


/* Define constants for color conversion */

extern const I32 iYMIN;
extern const I32 iYMAX;

extern const I32 iY2RGB;    /* (65536*256/(iYMAX-iYMIN+1)) */
extern const I32 iV2R;
extern const I32 iV2G;
extern const I32 iU2G;
extern const I32 iU2B;
extern const I32 iV2RBIAS;	 /* (( iV2R*128 + 65535)/65536) */
extern const I32 iV2GBIAS;	 /* ((-iV2G*128 + 65535)/65536) */
extern const I32 iU2GBIAS;	 /* ((-iU2G*128 + 65535)/65536) */
extern const I32 iU2BBIAS;	 /* (( iU2B*128 + 65535)/65536) */


/* Dithering constants */

extern const I32 iDITHER_V0;
extern const I32 iDITHER_V1;
extern const I32 iDITHER_V2;
extern const I32 iDITHER_V3;
extern const I32 iDITHER_V4;
extern const I32 iDITHER_V5;
extern const I32 iDITHER_V6;
extern const I32 iDITHER_V7;
extern const I32 iDITHER_V8;
extern const I32 iDITHER_V9;
extern const I32 iDITHER_Va;
extern const I32 iDITHER_Vb;
extern const I32 iDITHER_Vc;
extern const I32 iDITHER_Vd;
extern const I32 iDITHER_Ve;
extern const I32 iDITHER_Vf;


extern const I32 iDITHER_U0;
extern const I32 iDITHER_U1;
extern const I32 iDITHER_U2;
extern const I32 iDITHER_U3;
extern const I32 iDITHER_U4;
extern const I32 iDITHER_U5;
extern const I32 iDITHER_U6;
extern const I32 iDITHER_U7;
extern const I32 iDITHER_U8;
extern const I32 iDITHER_U9;
extern const I32 iDITHER_Ua;
extern const I32 iDITHER_Ub;
extern const I32 iDITHER_Uc;
extern const I32 iDITHER_Ud;
extern const I32 iDITHER_Ue;
extern const I32 iDITHER_Uf;


/* uBGR contains offsets to get to BGR from RGB index values; 
 * GREEN is defined for completeness 
#define RED   2-uRGB
#define GREEN 1
#define BLUE  0+uRGB



cpz - QTW
*/ 


/* build the color, clipped to unsigned 8 bits */
#define RENDER_PIXEL4(yoff1, yoff2)												\
						{	pixel = uAndV | (pu8Y[yoff1 + yoff2] << 24);		\
							pixel |= (pu8Y[yoff1 + yoff2 + 1] << 8);			\
							*(PU32)pu8Addr = pixel;								\
						}
 
/* Used when local decode and/or transparency is on.
 * Calculate the address -- pu8Out plus the offset to this pixel
 * within the block -- and then decide whether to render the pixel,
 * fill it, or do nothing.
 */
#define HANDLE_PIXEL4(iOutRowOff, uYRowOff, iColOffset, uMaskBit, 	\
					 iVDither, iUDither) {							\
	pu8Addr = pu8Out + iOutRowOff + iColOffset;						\
	if (u8Mask & uMaskBit) 	/* Render this pixel? */				\
		RENDER_PIXEL4(uYRowOff, iVDither, iUDither)					\
	else if (bWantsFill && (!pu8LD || (u8LDMask&uMaskBit)))			\
		FILL_PIXEL4													\
}

/* These arrays are translation tables -- how much Y Contributes 
 * to each of R, G, B; how much U Contributes to B and G, and how
 * much V contributes to G and R.
 *
 */


/* (RS) Removed static keyword from the following entries */

extern I32 iYContribToR[608];	/* [256+2*iV2RBIAS] */
extern I32 iYContribToG[522]; 	/* [256+2*(iV2GBIAS+iU2GBIAS)] */
extern I32 iYContribToB[700];	/* [256+2*iU2BBIAS] */
extern I32 iUContribToB[256];
extern I32 iUContribToG[256];
extern I32 iVContribToG[256];
extern I32 iVContribToR[256];
extern Boo bContribsInited;


/* The arguments are explained in detail in clut8.c */
void YVU9_to_YUY2(U32 uRows, U32 uCols, PU8 pY, PU8 pV, PU8 pU,
	U32 uYPitch, U32 uVUPitch, PU8 pOut, I32 iOutPitch,
	PU8 pTMask, PU8 pLMask, U32 uMStride,
	Boo bWantsFill, U32 uFillValue, COLOR_FORMAT eFormat )
{
/*    U32 uRGB;		cpz - QTW	    /* Used to adjust writes for R, B for RGB/BGR */
    U32 i,j;

    I32 iY,iU,iV,iR,iG,iB;
	I32 iRoff, iGoff, iBoff;
    U32 uYRowDelta, uVURowDelta;    /* Amount to adjust ptrs every 4 rows */
	I32 iOutDelta;
	U32 u2YPitch, u3YPitch;			/* Compute multiples of pitches once */
	I32 i2OutPitch, i3OutPitch;
	PU8 pu8Y, pu8V, pu8U,pu8Out;  /* Movable pointers to pY, pV, pU, pOut */
	PU8 pu8Addr;

	uYRowDelta  = uYPitch*4 - uCols;	 /* 4 lines down, back the width */
	uVURowDelta = uVUPitch - (uCols/4);  /* of the picture */
	iOutDelta = iOutPitch*4 - (I32)(uCols * 2);
	i2OutPitch = 2*iOutPitch; i3OutPitch = 3*iOutPitch;
	u2YPitch   = 2*uYPitch;   u3YPitch   = 3*uYPitch;
	pu8Y = pY; pu8V = pV; pu8U = pU; pu8Out = pOut;

	/* Color-convert each 4x4 block in the image */
	for (i = 0; i < uRows; i+=4, pu8Out += iOutDelta) {
		for (j = 0; j < uCols; j+=4, pu8Out += 8, pu8Y+=4, pu8V++, pu8U++) {
			U32 uAndV;
			U32 pixel;
				iV = *pu8V;	  /* fetch V and U for this block */
			iU = *pu8U;
				uAndV = iU << 16 | iV;
			uAndV ^= 0x00800080;
				/* Row  0 */
			pu8Addr = pu8Out;     RENDER_PIXEL4(0, 0);
			pu8Addr = pu8Out + 4; RENDER_PIXEL4(2, 0);
				/* Row  1 */
			pu8Addr = pu8Out + iOutPitch;     	 
				RENDER_PIXEL4(0, uYPitch);
			pu8Addr = pu8Out + iOutPitch + 4; 	 
				RENDER_PIXEL4(2, uYPitch);
				/* Row  2 */
			pu8Addr = pu8Out + (i2OutPitch);     
				RENDER_PIXEL4(0, u2YPitch);
			pu8Addr = pu8Out + (i2OutPitch) + 4; 
				RENDER_PIXEL4(2, u2YPitch);
				/* Row  3 */
			pu8Addr = pu8Out + (i3OutPitch);     
				RENDER_PIXEL4(0, u3YPitch);
			pu8Addr = pu8Out + (i3OutPitch) + 4; 
				RENDER_PIXEL4(2, u3YPitch);
		}
		pu8Y += uYRowDelta;	  /* Advance pointers to the next block of rows */
		pu8V += uVURowDelta;
		pu8U += uVURowDelta;
    }
}

#endif /* YVU9_to_YUY2 */
