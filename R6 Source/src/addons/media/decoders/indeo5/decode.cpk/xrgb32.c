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

const I32 iYMIN = 16;

const I32 iYMAX	= 235;



const I32 iY2RGB = 76284;    /* (65536*256/(iYMAX-iYMIN+1)) */

const I32 iV2R = 89858;

const I32 iV2G = -45774;

const I32 iU2G = -22015;

const I32 iU2B = 113562;

const I32 iV2RBIAS = 176;	 /* (( iV2R*128 + 65535)/65536) */

const I32 iV2GBIAS = 90;	 /* ((-iV2G*128 + 65535)/65536) */

const I32 iU2GBIAS = 43;	 /* ((-iU2G*128 + 65535)/65536) */

const I32 iU2BBIAS = 222;	 /* (( iU2B*128 + 65535)/65536) */



/* Dithering constants */

const I32 iDITHER_V0 = -1;

const I32 iDITHER_V1 =  2;

const I32 iDITHER_V2 =  0;

const I32 iDITHER_V3 =  1;

const I32 iDITHER_V4 =  1;

const I32 iDITHER_V5 =  0;

const I32 iDITHER_V6 =  2;

const I32 iDITHER_V7 = -1;

const I32 iDITHER_V8 =  2;

const I32 iDITHER_V9 = -1;

const I32 iDITHER_Va =  1;

const I32 iDITHER_Vb =  0;

const I32 iDITHER_Vc =  0;

const I32 iDITHER_Vd =  1;

const I32 iDITHER_Ve = -1;

const I32 iDITHER_Vf =  2;



const I32 iDITHER_U0 =  0;

const I32 iDITHER_U1 =  1;

const I32 iDITHER_U2 = -1;

const I32 iDITHER_U3 =  2;

const I32 iDITHER_U4 = -1;

const I32 iDITHER_U5 =  2;

const I32 iDITHER_U6 =  0;

const I32 iDITHER_U7 =  1;

const I32 iDITHER_U8 =  1;

const I32 iDITHER_U9 =  0;

const I32 iDITHER_Ua =  2;

const I32 iDITHER_Ub = -1;

const I32 iDITHER_Uc =  2;

const I32 iDITHER_Ud = -1;

const I32 iDITHER_Ue =  1;

const I32 iDITHER_Uf =  0;





/* uBGR contains offsets to get to BGR from RGB index values; 
 * GREEN is defined for completeness 
#define RED   2-uRGB
#define GREEN 1
#define BLUE  0+uRGB



cpz - QTW
*/ 


/* build the color, clipped to unsigned 8 bits */
#define BUILD_COLOR(pu8Base, u8ColorValue, uColorIndex)	\
    if ((u8ColorValue) & 0xFFFFFF00)					\
		if ((u8ColorValue) < 0)				   			\
		    xRGBColor.u8Pel[uColorIndex] =   0;		\
		else								   			\
		    xRGBColor.u8Pel[uColorIndex] = 255;		\
    else									   			\
		xRGBColor.u8Pel[uColorIndex] = (U8)(u8ColorValue);  

#define WRITE_BUILT_PIXEL(pu8Base) *(PU32)(pu8Base) = xRGBColor.uColor;

#define RENDER_PIXEL4(uYOffset, iVDither, iUDither) {	\
	/* Form R, G, B values from Y,V,U values */			\
	iY = *(pu8Y+(uYOffset));							\
	iR=iYContribToR[(iY + iRoff + iVDither)];			\
	iG=iYContribToG[(iY + iGoff + iVDither + iUDither)];\
	iB=iYContribToB[(iY + iBoff + iUDither)];			\
														\
	/* Write out the RGB values */						\
	BUILD_COLOR(pu8Addr, iR, RED)						\
	BUILD_COLOR(pu8Addr, iG, GREEN)						\
	BUILD_COLOR(pu8Addr, iB, BLUE)						\
	xRGBColor.u8Pel[TRANS] = 0xFF;		/* pixel on */	\
	WRITE_BUILT_PIXEL(pu8Addr);							\
}

#define FILL_PIXEL4 {									\
	xRGBColor.u8Pel[RED]   = u8FillRed;					\
	xRGBColor.u8Pel[GREEN] = u8FillGreen;				\
	xRGBColor.u8Pel[BLUE]  = u8FillBlue;				\
	xRGBColor.u8Pel[TRANS] = 0x00;		/* pixel off */	\
	WRITE_BUILT_PIXEL(pu8Addr);							\
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

I32 iYContribToR[608];	/* [256+2*iV2RBIAS] */

I32 iYContribToG[522]; 	/* [256+2*(iV2GBIAS+iU2GBIAS)] */

I32 iYContribToB[700];	/* [256+2*iU2BBIAS] */

I32 iUContribToB[256];

I32 iUContribToG[256];

I32 iVContribToG[256];

I32 iVContribToR[256];

Boo bContribsInited = FALSE;



/*

 * Y does NOT have the same range as U and V do. See the CCIR-601 spec.

 *

 * The formulae published by the CCIR committee for

 *      Y        = 16..235

 *      U & V    = 16..240

 *      R, G & B =  0..255 are:

 * R = (1.164 * (Y - 16.)) + (-0.001 * (U - 128.)) + ( 1.596 * (V - 128.))

 * G = (1.164 * (Y - 16.)) + (-0.391 * (U - 128.)) + (-0.813 * (V - 128.))

 * B = (1.164 * (Y - 16.)) + ( 2.017 * (U - 128.)) + ( 0.001 * (V - 128.))

 *

 * The coefficients are all multiplied by 65536 to accomodate integer only

 * math.

 *

 * R = (76284 * (Y - 16.)) + (    -66 * (U - 128.)) + ( 104595 * (V - 128.))

 * G = (76284 * (Y - 16.)) + ( -25625 * (U - 128.)) + ( -53281 * (V - 128.))

 * B = (76284 * (Y - 16.)) + ( 132186 * (U - 128.)) + (     66 * (V - 128.))

 *

 * Mathematically this is equivalent to (and computationally this is nearly

 * equivalent to):

 * R = ((Y-16) + (-0.001 / 1.164 * (U-128)) + ( 1.596 / 1.164 * (V-128)))*1.164

 * G = ((Y-16) + (-0.391 / 1.164 * (U-128)) + (-0.813 / 1.164 * (V-128)))*1.164

 * B = ((Y-16) + ( 2.017 / 1.164 * (U-128)) + ( 0.001 / 1.164 * (V-128)))*1.164

 *

 * which, in integer arithmetic, and eliminating the insignificant parts, is:

 *

 * R = ((Y-16) +  		                      ( 89858 * (V - 128) >> 16)) * 1.164

 * G = ((Y-16) + (-22015 * (U - 128) >> 16) + (-45774 * (V - 128) >> 16)) * 1.164

 * B = ((Y-16) + (113562 * (U - 128) >> 16)         		          	) * 1.164

*/											  		



/* Initialize the contributions of Y,V,U to each of R, G, B; 

 * using the above equation.  

 */

void InitYVU2RGBContribs() {

	I32 i,iContrib;



	for (i = 0; i < 255 + 2 * iV2RBIAS; i++) {

		iContrib = ((i - iYMIN - iV2RBIAS) * iY2RGB) >> 16L;

		if (iContrib <   0L) iContrib =   0L;

		if (iContrib > 255L) iContrib = 255L;

		iYContribToR[i] = (U8) iContrib;

	}



	for (i = 0; i < 255 + 2 * (iV2GBIAS + iU2GBIAS); i++) {

		iContrib = (( i - iYMIN - (iV2GBIAS + iU2GBIAS)) * iY2RGB) >> 16L;

		if (iContrib <   0L) iContrib =   0L;

		if (iContrib > 255L) iContrib = 255L;

		iYContribToG[i] = (U8) iContrib;

	}



	for (i = 0; i < 255 + 2 * iU2BBIAS; i++) {

		iContrib = (( i - iYMIN - iU2BBIAS) * iY2RGB) >> 16L;

		if (iContrib <   0L) iContrib =   0L;

		if (iContrib > 255L) iContrib = 255L;

		iYContribToB[i] = (U8) iContrib;

	}

	

	for (i = 0; i < 256; i++) {

		iVContribToR[i] = (((iV2R*(i-128L))>>16L)+iV2RBIAS) & 0x1ff; /* biased V to R. */

		iVContribToG[i] = (((iV2G*(i-128L))>>16L)+iV2GBIAS) & 0x1ff; /* biased V to G. */

		iUContribToB[i] = (((iU2B*(i-128L))>>16L)+iU2BBIAS) & 0x1ff; /* biased U to B. */

		iUContribToG[i] = (((iU2G*(i-128L))>>16L)+iU2GBIAS) & 0x1ff; /* biased U to G. */

	}



}




/* The arguments are explained in detail in clut8.c */
void YVU9_to_XRGB32(U32 uRows, U32 uCols, PU8 pY, PU8 pV, PU8 pU,
	U32 uYPitch, U32 uVUPitch, PU8 pOut, I32 iOutPitch,
	PU8 pTMask, PU8 pLMask, U32 uMStride,
	Boo bWantsFill, U32 uFillValue, COLOR_FORMAT eFormat )
{
/*    U32 uRGB;		cpz - QTW	    /* Used to adjust writes for R, B for RGB/BGR */
    U32 i,j;

	U32	RED,GREEN,BLUE,TRANS;
    I32 iY,iU,iV,iR,iG,iB;
	I32 iRoff, iGoff, iBoff;
    U32 uYRowDelta, uVURowDelta;    /* Amount to adjust ptrs every 4 rows */
	I32 iOutDelta;
	U32 u2YPitch, u3YPitch;			/* Compute multiples of pitches once */
	I32 i2OutPitch, i3OutPitch;
	PU8 pu8Y, pu8V, pu8U,pu8Out;  /* Movable pointers to pY, pV, pU, pOut */
	PU8 pu8Addr;

	union {
		U8	u8Pel[4];				/* xRGB */
		U32	uColor;
	} xRGBColor;
	
	xRGBColor.uColor = 0L;

    if (!bContribsInited) { 	  /* Initialize contributions of Y, V, U */
		InitYVU2RGBContribs();	  /* to each of R, G, and B; this is */
		bContribsInited = TRUE;	  /* only done once */
    }


	switch (eFormat) {

	  case CF_XRGB32:

		RED = 1;

		GREEN = 2;

		BLUE = 3;

		TRANS = 0;

		break;

	  case CF_XBGR32:

		RED = 3;

		GREEN = 2;

		BLUE = 1;

		TRANS = 0;

		break;

	  case CF_RGBX32:

		RED = 0;

		GREEN = 1;

		BLUE = 2;

		TRANS = 3;

		break;

	  case CF_BGRX32:

		RED = 2;

		GREEN = 1;

		BLUE = 0;

		TRANS = 3;

		break;

	  default:

		return;

	}



	/* Fill in alpha channel */

	xRGBColor.u8Pel[TRANS]  = 0xFF;						\



/* cpz - QTW */
/*    uRGB = bBGR ? 0 : 2;   /* Added for R, subtracted from 2 for B */

	uYRowDelta  = uYPitch*4 - uCols;	 /* 4 lines down, back the width */
	uVURowDelta = uVUPitch - (uCols/4);  /* of the picture */
	iOutDelta = iOutPitch*4 - (I32)(uCols * 4);
	i2OutPitch = 2*iOutPitch; i3OutPitch = 3*iOutPitch;
	u2YPitch   = 2*uYPitch;   u3YPitch   = 3*uYPitch;
	pu8Y = pY; pu8V = pV; pu8U = pU; pu8Out = pOut;

	
	if (!pTMask && !pLMask) {	/* No transp or loc dec -- fast convert */
		/* Color-convert each 4x4 block in the image */
		for (i = 0; i < uRows; i+=4, pu8Out += iOutDelta) {
			for (j = 0; j < uCols; j+=4, pu8Out += 16, pu8Y+=4, pu8V++, pu8U++) {
				iV = *pu8V;	  /* fetch V and U for this block */
				iU = *pu8U;
				/* Get contibution of U and V to R,G,B */
				iRoff = iVContribToR[iV]; 
				iGoff = iUContribToG[iU] + iVContribToG[iV];
				iBoff = iUContribToB[iU];

				/* Row  0 */
				pu8Addr = pu8Out;     RENDER_PIXEL4(0, iDITHER_V0, iDITHER_U0)
				pu8Addr = pu8Out + 4; RENDER_PIXEL4(1, iDITHER_V1, iDITHER_U1)
				pu8Addr = pu8Out + 8; RENDER_PIXEL4(2, iDITHER_V2, iDITHER_U2)
				pu8Addr = pu8Out + 12; RENDER_PIXEL4(3, iDITHER_V3, iDITHER_U3)

				/* Row  1 */
				pu8Addr = pu8Out + iOutPitch;     	 
					RENDER_PIXEL4(uYPitch, iDITHER_V4, iDITHER_U4)
				pu8Addr = pu8Out + iOutPitch + 4; 	 
					RENDER_PIXEL4(uYPitch + 1, iDITHER_V5, iDITHER_U5)
				pu8Addr = pu8Out + iOutPitch + 8; 	 
					RENDER_PIXEL4(uYPitch + 2, iDITHER_V6, iDITHER_U6)
				pu8Addr = pu8Out + iOutPitch + 12; 	 
					RENDER_PIXEL4(uYPitch + 3, iDITHER_V7, iDITHER_U7)

				/* Row  2 */
				pu8Addr = pu8Out + (i2OutPitch);     
					RENDER_PIXEL4(u2YPitch, iDITHER_V8, iDITHER_U8)
				pu8Addr = pu8Out + (i2OutPitch) + 4; 
					RENDER_PIXEL4(u2YPitch + 1, iDITHER_V9, iDITHER_U9)
				pu8Addr = pu8Out + (i2OutPitch) + 8; 
					RENDER_PIXEL4(u2YPitch + 2, iDITHER_Va, iDITHER_Ua)
				pu8Addr = pu8Out + (i2OutPitch) + 12; 
					RENDER_PIXEL4(u2YPitch + 3, iDITHER_Vb, iDITHER_Ub)

				/* Row  3 */
				pu8Addr = pu8Out + (i3OutPitch);     
					RENDER_PIXEL4(u3YPitch, iDITHER_Vc, iDITHER_Uc)
				pu8Addr = pu8Out + (i3OutPitch) + 4; 
					RENDER_PIXEL4(u3YPitch + 1, iDITHER_Vd, iDITHER_Ud)
				pu8Addr = pu8Out + (i3OutPitch) + 8; 
					RENDER_PIXEL4(u3YPitch + 2, iDITHER_Ve, iDITHER_Ue)
				pu8Addr = pu8Out + (i3OutPitch) + 12; 
					RENDER_PIXEL4(u3YPitch + 3, iDITHER_Vf, iDITHER_Uf)
			}
			pu8Y += uYRowDelta;	  /* Advance pointers to the next block of rows */
			pu8V += uVURowDelta;
			pu8U += uVURowDelta;
	    }
	} else { 				/* Transp and/or local decode */

		PU8 pu8LD, pu8T;		/* Mobile pointers within masks */
		PU32 puLD, puT;			/* 32 bit mobile mask pointers, for ANDing */
		U8  u8Mask;				/* byte of local decode  & transp mask */
		U8  u8LDMask;			/* If non-zero, process through LD Mask */
		U8  u8FillRed, u8FillGreen, u8FillBlue;  /* Fill values */
		U32 uMaskLineDelta;		/* Amount to adjust mask pointers in looping */
		U32 u2MStride, u3MStride; /* MStride multiples */
		Boo bOdd;				/* odd or even iteration of j? */
		Boo bBothValid;			/* Both Transparency and local decode */

		pu8LD = pLMask;
		if (pTMask)
			pu8T = pTMask;
		else 
			pu8T = pLMask; /* Guarantee this pointer is valid */

		u8FillRed   = (U8)(uFillValue>>16) & 0xFF;
		u8FillGreen = (U8)(uFillValue>>8 ) & 0xFF;
		u8FillBlue  = (U8) uFillValue      & 0xFF;

		uMaskLineDelta = (4*uMStride) - (uCols/8);
		u2MStride = 2*uMStride; u3MStride = 3*uMStride;

		/* AND together local decode and transparency masks into transparency
		 * buffer.  This produces a mask that specifies which pixels to 
		 * color convert.  If fill is on, still need to compare to LDMask.
		 */
		bBothValid = (pTMask && pLMask);
		if (bBothValid)	{
			puT = (PU32)pTMask;
			puLD = (PU32)pLMask;
			for (i=0; i < (uRows*uMStride)/4; i++, puT++, puLD++) 
				*puT = *puT & *puLD;
		}

		/* Color-convert each 4x4 block in the image */
		for (i = 0; i < uRows; i+=4, pu8Out += iOutDelta) {
			bOdd = 0;   /* First row is not odd */

			for (j = 0; j < uCols; j+=4, pu8Out += 16, pu8Y+=4, pu8V++, pu8U++) {
				iV = *pu8V;	  /* fetch V and U for this block */
				iU = *pu8U;
				/* Get contibution of U and V to R,G,B */
				iRoff = iVContribToR[iV]; 
				iGoff = iUContribToG[iU] + iVContribToG[iV];
				iBoff = iUContribToB[iU];

				/* The masks have the bit for the first pel at the upper end 
				 * and bit for the last pel at the lower end.  1 bit for each
				 * pel, so only need a nibble of the byte for this block of 4.
				 */
				/* Row 0 */
				u8Mask = bOdd ? *pu8T & 0xF : *pu8T >> 4;
				u8LDMask = pu8LD ? (bOdd ? *pu8LD & 0xF : *pu8LD >>4 ) : 0;
				HANDLE_PIXEL4(0,0,0,8, iDITHER_V0, iDITHER_U0);
				HANDLE_PIXEL4(0,1,4,4, iDITHER_V1, iDITHER_U1);
				HANDLE_PIXEL4(0,2,8,2, iDITHER_V2, iDITHER_U2);
				HANDLE_PIXEL4(0,3,12,1, iDITHER_V3, iDITHER_U3);

				/* Row 1 */
				u8Mask = bOdd ? *(pu8T + uMStride) & 0xF : 
							    *(pu8T + uMStride) >>4;
				u8LDMask = pu8LD ? (bOdd ? *(pu8LD + uMStride) & 0xF : 
										   *(pu8LD + uMStride)>>4 ) : 0;
				HANDLE_PIXEL4(iOutPitch, uYPitch+0,0,8, iDITHER_V4, iDITHER_U4);
				HANDLE_PIXEL4(iOutPitch, uYPitch+1,4,4, iDITHER_V5, iDITHER_U5);
				HANDLE_PIXEL4(iOutPitch, uYPitch+2,8,2, iDITHER_V6, iDITHER_U6);
				HANDLE_PIXEL4(iOutPitch, uYPitch+3,12,1, iDITHER_V7, iDITHER_U7);

				/* Row 2 */
				u8Mask = bOdd ? *(pu8T + u2MStride) & 0xF : 
							    *(pu8T + u2MStride) >>4;
				u8LDMask = pu8LD ? (bOdd ? *(pu8LD + u2MStride) & 0xF : 
										   *(pu8LD + u2MStride)>>4 ) : 0;
				HANDLE_PIXEL4(i2OutPitch, u2YPitch+0,0,8, iDITHER_V8, iDITHER_U8);
				HANDLE_PIXEL4(i2OutPitch, u2YPitch+1,4,4, iDITHER_V9, iDITHER_U9);
				HANDLE_PIXEL4(i2OutPitch, u2YPitch+2,8,2, iDITHER_Va, iDITHER_Ua);
				HANDLE_PIXEL4(i2OutPitch, u2YPitch+3,12,1, iDITHER_Vb, iDITHER_Ub);

				/* Row 3 */
				u8Mask = bOdd ? *(pu8T + u3MStride) & 0xF : 
							    *(pu8T + u3MStride) >>4;
				u8LDMask = pu8LD ? (bOdd ? *(pu8LD + u3MStride) & 0xF : 
										   *(pu8LD + u3MStride)>>4 ) : 0;
				HANDLE_PIXEL4(i3OutPitch, u3YPitch+0,0,8, iDITHER_Vc, iDITHER_Uc);
				HANDLE_PIXEL4(i3OutPitch, u3YPitch+1,4,4, iDITHER_Vd, iDITHER_Ud);
				HANDLE_PIXEL4(i3OutPitch, u3YPitch+2,8,2, iDITHER_Ve, iDITHER_Ue);
				HANDLE_PIXEL4(i3OutPitch, u3YPitch+3,12,1, iDITHER_Vf, iDITHER_Uf);

				/* 8 bits for 8 pels, or two iterations through the loop, so
				 * only update on odd iterations */
				if (bOdd) {
					if (pu8LD) pu8LD++;
					pu8T++;
				}
				bOdd = !bOdd;
			}
			pu8Y += uYRowDelta;	  /* Advance pointers to the next block of rows */
			pu8V += uVURowDelta;
			pu8U += uVURowDelta;
			if (pu8LD)

				pu8LD  += uMaskLineDelta;
			pu8T   += uMaskLineDelta;
	    }
	}
}

#endif /* CAN_DO_XRGB32 */
