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
 * CPK_COUT.H -- function declarations and definitions needed in color out
 *              routines.  
 *************************************************************************/

#ifdef __CPK_COUT_H__
#pragma message("cpk_cout.h: multiple inclusion")
#else /* __CPK_COUT_H__ */
#define __CPK_COUT_H__

#define UDITHER00	-14
#define UDITHER01	-2
#define UDITHER02	-10
#define UDITHER03	+6
#define UDITHER10	+2
#define UDITHER11	-6
#define UDITHER12	+14
#define UDITHER13	+10
#define UDITHER20	-10
#define UDITHER21	+6
#define UDITHER22	-14
#define UDITHER23	-2
#define UDITHER30	+14
#define UDITHER31	+10
#define UDITHER32	+2
#define UDITHER33	-6

#define VDITHER00	+14
#define VDITHER01	+6
#define VDITHER02	+10
#define VDITHER03	-2
#define VDITHER10	-6
#define VDITHER11	+2
#define VDITHER12	-14
#define VDITHER13	+10
#define VDITHER20	+10
#define VDITHER21	-2
#define VDITHER22	+14
#define VDITHER23	+6
#define VDITHER30	-14
#define VDITHER31	+10
#define VDITHER32	-6
#define VDITHER33	+2

#define DD2		+2
#define DD6		+6
#define	DDA		-6
#define	DDE		-2

#if defined NO_Y_DITHER
#define YDITHER00	0
#define YDITHER01	YDITHER00
#define YDITHER02	YDITHER00
#define YDITHER03	YDITHER00
#define YDITHER10	YDITHER00
#define YDITHER11	YDITHER00
#define YDITHER12	YDITHER00
#define YDITHER13	YDITHER00
#define YDITHER20	YDITHER00
#define YDITHER21	YDITHER00
#define YDITHER22	YDITHER00
#define YDITHER23	YDITHER00
#define YDITHER30	YDITHER00
#define YDITHER31	YDITHER00
#define YDITHER32	YDITHER00
#define YDITHER33	YDITHER00
#else
#define YDITHER00	DDA
#define YDITHER01	DD6
#define YDITHER02	DDE
#define YDITHER03	DD2
#define YDITHER10	DDE
#define YDITHER11	DD2
#define YDITHER12	DDA
#define YDITHER13	DD6
#define YDITHER20	DD6
#define YDITHER21	DDA
#define YDITHER22	DD2
#define YDITHER23	DDE
#define YDITHER30	DD2
#define YDITHER31	DDE
#define YDITHER32	DD6
#define YDITHER33	DDA
#endif /* NO_Y_DITHER */

#define VKh		(255 - (96 + 64 + 16))
#define UKh		(255 - (96 + 64 + 16))
#define VUKl	(255 - (3*32))
#define YKh		(255 - (232 - 8))
#define YKl		(255 - (13*16))


I32 C_YVU9toCLUT8_C(U32 nRows, U32 nCols, PU8 pY, PU8 pV, PU8 pU,
	U32 uYPitch, U32 uVUPitch, PU8 pOut, I32 iOutPitch,
	PU8 pTMask, PU8 pLMask, U32 uTStride, U32 uLStride,
	U32 uWantsFill, U32 uFillValue, U32 uZoomX, U32 uZoomY, U32 uMoreFlags);

#if 0
void YVU9_to_RGB24_C(U32 nRows, U32 nCols, PU8 pY, PU8 pV, PU8 pU,
	U32 uYPitch, U32 uVUPitch, PU8 pOut, I32 uOutPitch,
	PU8 pTMask, PU8 pLMask, U32 uTStride, U32 uLStride,
	U32 uWantsFill, U32 uFillValue);
#endif //0

/*
 * Dither tables for the 3 planes:
 */
#define D24Y0 0
#define D24Y1 0
#define D24Y2 0
#define D24Y3 0
#define D24Y4 0
#define D24Y5 0
#define D24Y6 0
#define D24Y7 0
#define D24Y8 0
#define D24Y9 0
#define D24Ya 0
#define D24Yb 0
#define D24Yc 0
#define D24Yd 0
#define D24Ye 0
#define D24Yf 0
#define D24YMIN 0
#define D24YMAX 0

#if 0

#define D24V0 -1
#define D24V1 2
#define D24V2 0
#define D24V3 1
#define D24V4 1
#define D24V5 0
#define D24V6 2
#define D24V7 -1
#define D24V8 2
#define D24V9 -1
#define D24Va 1
#define D24Vb 0
#define D24Vc 0
#define D24Vd 1
#define D24Ve -1
#define D24Vf 2
#define D24VMIN -1
#define D24VMAX 2

#define D24U0 0
#define D24U1 1
#define D24U2 -1
#define D24U3 2
#define D24U4 -1
#define D24U5 2
#define D24U6 0
#define D24U7 1
#define D24U8 1
#define D24U9 0
#define D24Ua 2
#define D24Ub -1
#define D24Uc 2
#define D24Ud -1
#define D24Ue 1
#define D24Uf 0
#define D24UMIN -1
#define D24UMAX 2

#else /* 0 */

#define D24V2 -1
#define D24V3 2
#define D24V0 0
#define D24V1 1
#define D24V6 1
#define D24V7 0
#define D24V4 2
#define D24V5 -1
#define D24Va 2
#define D24Vb -1
#define D24V8 1
#define D24V9 0
#define D24Ve 0
#define D24Vf 1
#define D24Vc -1
#define D24Vd 2
#define D24VMIN -1
#define D24VMAX 2

#define D24U2 0
#define D24U3 1
#define D24U0 -1
#define D24U1 2
#define D24U6 -1
#define D24U7 2
#define D24U4 0
#define D24U5 1
#define D24Ua 1
#define D24Ub 0
#define D24U8 2
#define D24U9 -1
#define D24Ue 2
#define D24Uf -1
#define D24Uc 1
#define D24Ud 0
#define D24UMIN -1
#define D24UMAX 2

#endif /* 0 */
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
 * R = ((Y-16) + (-0.001 / 1.164 * (U-128)) + ( 1.596 * 1.164 * (V-128)))*1.164
 * G = ((Y-16) + (-0.391 / 1.164 * (U-128)) + (-0.813 * 1.164 * (V-128)))*1.164
 * B = ((Y-16) + ( 2.017 / 1.164 * (U-128)) + ( 0.001 * 1.164 * (V-128)))*1.164
 *
 * which, in integer arithmetic, and eliminating the insignificant parts, is:
 *
 * R = ((Y-16) +                        ( 89858 * (V - 128))) * 1.164
 * G = ((Y-16) + (-22015 * (U - 128)) + (-45774 * (V - 128))) * 1.164
 * B = ((Y-16) + (113562 * (U - 128))                       ) * 1.164
*/

#define YMIN	16
#define YMAX	235
/*#define VUMIN	16 */
/*#define VUMAX	240 */

#define Y2RGB	 (65536*256/(YMAX-YMIN))
#define V2R		 89858
#define	V2G		-45774
#define	U2G		-22015
#define U2B		113562
#define V2RBIAS	(176 + (D24VMAX)-(D24VMIN))	/* (U32)(( V2R*128 + 65535)/65536)*/
#define	V2GBIAS	(90 + (D24VMAX)-(D24VMIN))	/* (U32)((-V2G*128 + 65535)/65536)*/
#define	U2GBIAS	(43 + (D24UMAX)-(D24UMIN))	/* (U32)((-U2G*128 + 65535)/65536)*/
#define U2BBIAS	(222 + (D24UMAX)-(D24UMIN))	/* (U32)(( U2B*128 + 65535)/65536)*/

#define mac_abs(n) ((n) < 0 ? -(n) : (n))


void YVU9_to_RGB24_Init(U32 mode, U32 uCCFlags);

void YVU9_to_RGB24_C(U32 nRows, U32 nCols, PU8 pY, PU8 pV, PU8 pU,
	U32 uYPitch, U32 uVUPitch, PU8 pOut, I32 uOutPitch,
	PU8 pTMask, PU8 pLMask, U32 uTStride, U32 uLStride,
	U32 uWantsFill, U32 uFillValue);

#endif /* __CPK_COUT_H__ */
