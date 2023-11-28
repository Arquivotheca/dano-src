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
/*
 * cpk_cout.c
 *	Color conversion routines
 *
 * Functions
 *	C_YVU9toCLUT8_C	- yvu9 to clut8 color conversion
 *	YVU9_to_RGB24_C	- yvu9 to rgb24 color conversion
 */

#include "datatype.h"
#include "cpk_cout.h"

#define CCF_DOING_ALT_LINE		0x01

extern PU8 clipto8bit;
extern PU8 clip;

/*packed add unsigned saturated byte */
#define AUSB(a,s) (	clip[(U8)(a)+(U8)(s)]						\
					| (clip[(U8)((a)>>8)+(U8)((s)>>8)]<<8)		\
					| (clip[(U8)((a)>>16)+(U8)((s)>>16)]<<16)	\
					| (clip[(U8)((a)>>24)+(U8)((s)>>24)]<<24)	\
					)

/*packed subtract unsigned saturated byte */
#define SUSB(a,s) (	clip[(U8)(a)-(U8)(s)]						\
					| (clip[(U8)((a)>>8)-(U8)((s)>>8)]<<8)		\
					| (clip[(U8)((a)>>16)-(U8)((s)>>16)]<<16)	\
					| (clip[(U8)((a)>>24)-(U8)((s)>>24)]<<24)	\
					)


#define MASKWRITE8(tMask, oCol, uOut1) \
	if ((tMask) & 8) {						\
		((PU8)(oCol))[0] = (U8)(uOut1);		\
	}										\
	if ((tMask) & 4) {						\
		((PU8)(oCol))[1] = (U8)(uOut1>>8);	\
	}										\
	if ((tMask) & 2) {						\
		((PU8)(oCol))[2] = (U8)(uOut1>>16);	\
	}										\
	if ((tMask) & 1) {						\
		((PU8)(oCol))[3] = (U8)(uOut1>>24);	\
	}

I32
C_YVU9toCLUT8_C(U32 nRows, U32 nCols, PU8 pY, PU8 pV, PU8 pU,
	U32 uYPitch, U32 uVUPitch, PU8 pOut, I32 iOutPitch,
	PU8 pTMask, PU8 pLMask, U32 uTStride, U32 uLStride,
	U32 uWantsFill, U32 uFillValue, U32 uZoomX, U32 uZoomY, U32 uMoreFlags) {
	U32		rr, cc;
	PU8		oRow, tCol, lCol;
	PU32	oCol;
	U32		uYPitch4, uYPitch3m4;
	I32		iOutPitch4, iOutPitch3m4;

	U32	YC =	0x01010101*(U32)YKl;
	U32 VUC =	0x01010101*(U32)VUKl;
	U32	YDC0 =	((YDITHER03+YKh)<<24)+((YDITHER02+YKh)<<16)+((YDITHER01+YKh)<<8)+(YDITHER00+YKh);
	U32 YDC1 =	((YDITHER13+YKh)<<24)+((YDITHER12+YKh)<<16)+((YDITHER11+YKh)<<8)+(YDITHER10+YKh);
	U32 YDC2 =	((YDITHER23+YKh)<<24)+((YDITHER22+YKh)<<16)+((YDITHER21+YKh)<<8)+(YDITHER20+YKh);
	U32 YDC3 =	((YDITHER33+YKh)<<24)+((YDITHER32+YKh)<<16)+((YDITHER31+YKh)<<8)+(YDITHER30+YKh);
	U32	VDC0 =	((VDITHER03+VKh)<<24)+((VDITHER02+VKh)<<16)+((VDITHER01+VKh)<<8)+(VDITHER00+VKh);
	U32	VDC1 =	((VDITHER13+VKh)<<24)+((VDITHER12+VKh)<<16)+((VDITHER11+VKh)<<8)+(VDITHER10+VKh);
	U32	VDC2 =	((VDITHER23+VKh)<<24)+((VDITHER22+VKh)<<16)+((VDITHER21+VKh)<<8)+(VDITHER20+VKh);
	U32	VDC3 =	((VDITHER33+VKh)<<24)+((VDITHER32+VKh)<<16)+((VDITHER31+VKh)<<8)+(VDITHER30+VKh);
	U32	UDC0 =	((UDITHER03+UKh)<<24)+((UDITHER02+UKh)<<16)+((UDITHER01+UKh)<<8)+(UDITHER00+UKh);
	U32	UDC1 =	((UDITHER13+UKh)<<24)+((UDITHER12+UKh)<<16)+((UDITHER11+UKh)<<8)+(UDITHER10+UKh);
	U32	UDC2 =	((UDITHER23+UKh)<<24)+((UDITHER22+UKh)<<16)+((UDITHER21+UKh)<<8)+(UDITHER20+UKh);
	U32	UDC3 =	((UDITHER33+UKh)<<24)+((UDITHER32+UKh)<<16)+((UDITHER31+UKh)<<8)+(UDITHER30+UKh);

/*	note:  this code performs 32 bit writes to memory/frame buffer, and thus will */
/*	perform better when the output is aligned on a 32 bit boundary.  no checking is */
/*	done here to test for and handle this case. */

	oRow = pOut;
	uYPitch3m4 = uYPitch * 3 - 4;
	uYPitch4 = uYPitch * 4;
	iOutPitch3m4 = iOutPitch * 3 - 4;
	iOutPitch4 = iOutPitch * 4;


	if (pTMask || pLMask) {
		U32		uTStride4, uTStride3;
		U32		uLStride4, uLStride3;
		U32		uFill1;
		uTStride3 = uTStride * 3;
		uLStride3 = uLStride * 3;
		uTStride4 = uTStride * 4;
		uLStride4 = uLStride * 4;
		if (uWantsFill) {
			uFill1 = uFillValue | (uFillValue << 8);
			uFill1 = uFill1 | (uFill1 << 16);
		}
	for (rr = 0; rr < nRows; rr+=4 ) {
		PU8	pv, pu, py;

		py = pY;
		pv = pV;
		pu = pU;
		oCol = (PU32)oRow;
		tCol = pTMask;
		lCol = pLMask;
		for (cc = 0; cc < nCols; cc+=4) {
			register U32	uU2b, uV2r;
			register U32	uOut1, uVU;
			register U8		ltShift, tMask, lMask;

			/* set up the color contribution stuff */
			/* note: interpolation may be added here */
			uV2r = *pv++;
			uV2r |= uV2r << 8;
			uV2r |= uV2r << 16;
			
			uU2b = *pu++;
			uU2b |= uU2b << 8;
			uU2b |= uU2b << 16;

			ltShift = (U8)(cc&4)^4;	/* either shift 0 or 4 bits */

		/*row 0 */
			lMask = pLMask ? (lCol[cc>>3] >> ltShift) & 0xf : 0xf;
			tMask = pTMask ? (tCol[cc>>3] >> ltShift) & lMask : lMask;
			if (tMask) {
				uVU = ((SUSB(AUSB(uV2r,VDC0),VUC)>>3)&0x1c1c1c1c) +
					((SUSB(AUSB(uU2b,UDC0),VUC)>>5)&0x07070707);
				uVU = (uVU << 4) - (uVU << 1);

				uOut1 = *(PU32)py;
				if (tMask == 0xf) {
					oCol[0] = ((SUSB(AUSB(uOut1,YDC0),YC)>>4)&0x0f0f0f0f)+uVU+0x0a0a0a0a;
				}
				else {
					uOut1 = ((SUSB(AUSB(uOut1,YDC0),YC)>>4)&0x0f0f0f0f)+uVU+0x0a0a0a0a;
					MASKWRITE8(tMask, oCol, uOut1);
					if (uWantsFill) {
						MASKWRITE8(tMask^lMask, oCol, uFill1);
					}
				}
			}
			else if (uWantsFill) {
				if (lMask == 0xf) {
					oCol[0] = uFill1;
				}
				else {
					MASKWRITE8(lMask, oCol, uFill1);
				}
			}
			(PU8)oCol += iOutPitch;
			tCol += uTStride;
			lCol += uLStride;
			py += uYPitch;
		/*row 1 */
			lMask = pLMask ? (lCol[cc>>3] >> ltShift) & 0xf : 0xf;
			tMask = pTMask ? (tCol[cc>>3] >> ltShift) & lMask : lMask;
			if (tMask) {
				uVU = ((SUSB(AUSB(uV2r,VDC1),VUC)>>3)&0x1c1c1c1c) +
					((SUSB(AUSB(uU2b,UDC1),VUC)>>5)&0x07070707);
				uVU = (uVU << 4) - (uVU << 1);

				uOut1 = *(PU32)py;
				if (tMask == 0xf) {
					oCol[0] = ((SUSB(AUSB(uOut1,YDC1),YC)>>4)&0x0f0f0f0f)+uVU+0x0a0a0a0a;
				}
				else {
					uOut1 = ((SUSB(AUSB(uOut1,YDC1),YC)>>4)&0x0f0f0f0f)+uVU+0x0a0a0a0a;
					MASKWRITE8(tMask, oCol, uOut1);
					if (uWantsFill) {
						MASKWRITE8(tMask^lMask, oCol, uFill1);
					}
				}
			}
			else if (uWantsFill) {
				if (lMask == 0xf) {
					oCol[0] = uFill1;
				}
				else {
					MASKWRITE8(lMask, oCol, uFill1);
				}
			}
			(PU8)oCol += iOutPitch;
			tCol += uTStride;
			lCol += uLStride;
			py += uYPitch;
		/*row 2 */
			lMask = pLMask ? (lCol[cc>>3] >> ltShift) & 0xf : 0xf;
			tMask = pTMask ? (tCol[cc>>3] >> ltShift) & lMask : lMask;
			if (tMask) {
				uVU = ((SUSB(AUSB(uV2r,VDC2),VUC)>>3)&0x1c1c1c1c) +
					((SUSB(AUSB(uU2b,UDC2),VUC)>>5)&0x07070707);
				uVU = (uVU << 4) - (uVU << 1);

				uOut1 = *(PU32)py;
				if (tMask == 0xf) {
					oCol[0] = ((SUSB(AUSB(uOut1,YDC2),YC)>>4)&0x0f0f0f0f)+uVU+0x0a0a0a0a;
				}
				else {
					uOut1 = ((SUSB(AUSB(uOut1,YDC2),YC)>>4)&0x0f0f0f0f)+uVU+0x0a0a0a0a;
					MASKWRITE8(tMask, oCol, uOut1);
					if (uWantsFill) {
						MASKWRITE8(tMask^lMask, oCol, uFill1);
					}
				}
			}
			else if (uWantsFill) {
				if (lMask == 0xf) {
					oCol[0] = uFill1;
				}
				else {
					MASKWRITE8(lMask, oCol, uFill1);
				}
			}
			(PU8)oCol += iOutPitch;
			tCol += uTStride;
			lCol += uLStride;
			py += uYPitch;
		/*row 3 */
			lMask = pLMask ? (lCol[cc>>3] >> ltShift) & 0xf : 0xf;
			tMask = pTMask ? (tCol[cc>>3] >> ltShift) & lMask : lMask;
			if (tMask) {
				uVU = ((SUSB(AUSB(uV2r,VDC3),VUC)>>3)&0x1c1c1c1c) +
					((SUSB(AUSB(uU2b,UDC3),VUC)>>5)&0x07070707);
				uVU = (uVU << 4) - (uVU << 1);

				uOut1 = *(PU32)py;
				if (tMask == 0xf) {
					oCol[0] = ((SUSB(AUSB(uOut1,YDC3),YC)>>4)&0x0f0f0f0f)+uVU+0x0a0a0a0a;
				}
				else {
					uOut1 = ((SUSB(AUSB(uOut1,YDC3),YC)>>4)&0x0f0f0f0f)+uVU+0x0a0a0a0a;
					MASKWRITE8(tMask, oCol, uOut1);
					if (uWantsFill) {
						MASKWRITE8(tMask^lMask, oCol, uFill1);
					}
				}
			}
			else if (uWantsFill) {
				if (lMask == 0xf) {
					oCol[0] = uFill1;
				}
				else {
					MASKWRITE8(lMask, oCol, uFill1);
				}
			}
			(PU8)oCol -= iOutPitch3m4;
			tCol -= uTStride3;
			lCol -= uLStride3;
			py -= uYPitch3m4;

		} /* for cc */
		pY += uYPitch4;
		pV += uVUPitch;
		pU += uVUPitch;
		oRow += iOutPitch4;
		if (pTMask)	pTMask += uTStride4;
		if (pLMask) pLMask += uLStride4;
	} /* for rr */
	}
	else {
	for (rr = 0; rr < nRows; rr+=4 ) {
		PU8	pv, pu, py;

		py = pY;
		pv = pV;
		pu = pU;
		oCol = (PU32)oRow;
		for (cc = 0; cc < nCols; cc+=4) {
			register U32	uU2b, uV2r;
			register U32	uOut1, uVU;

			/* set up the color contribution stuff */
			/* note: interpolation may be added here */
			uV2r = *pv++;
			uV2r |= uV2r << 8;
			uV2r |= uV2r << 16;
			
			uU2b = *pu++;
			uU2b |= uU2b << 8;
			uU2b |= uU2b << 16;

		/*row 0 */
			uVU = ((SUSB(AUSB(uV2r,VDC0),VUC)>>3)&0x1c1c1c1c) +
				((SUSB(AUSB(uU2b,UDC0),VUC)>>5)&0x07070707);
			uVU = (uVU << 4) - (uVU << 1);

			uOut1 = *(PU32)py;
			oCol[0] = ((SUSB(AUSB(uOut1,YDC0),YC)>>4)&0x0f0f0f0f)+uVU+0x0a0a0a0a;
			(PU8)oCol += iOutPitch;
			py += uYPitch;
		/*row 1 */
			uVU = ((SUSB(AUSB(uV2r,VDC1),VUC)>>3)&0x1c1c1c1c) +
				((SUSB(AUSB(uU2b,UDC1),VUC)>>5)&0x07070707);
			uVU = (uVU << 4) - (uVU << 1);

			uOut1 = *(PU32)py;
			oCol[0] = ((SUSB(AUSB(uOut1,YDC1),YC)>>4)&0x0f0f0f0f)+uVU+0x0a0a0a0a;
			(PU8)oCol += iOutPitch;
			py += uYPitch;
		/*row 2 */
			uVU = ((SUSB(AUSB(uV2r,VDC2),VUC)>>3)&0x1c1c1c1c) +
				((SUSB(AUSB(uU2b,UDC2),VUC)>>5)&0x07070707);
			uVU = (uVU << 4) - (uVU << 1);

			uOut1 = *(PU32)py;
			oCol[0] = ((SUSB(AUSB(uOut1,YDC2),YC)>>4)&0x0f0f0f0f)+uVU+0x0a0a0a0a;
			(PU8)oCol += iOutPitch;
			py += uYPitch;
		/*row 3 */
			uVU = ((SUSB(AUSB(uV2r,VDC3),VUC)>>3)&0x1c1c1c1c) +
				((SUSB(AUSB(uU2b,UDC3),VUC)>>5)&0x07070707);
			uVU = (uVU << 4) - (uVU << 1);

			uOut1 = *(PU32)py;
			oCol[0] = ((SUSB(AUSB(uOut1,YDC3),YC)>>4)&0x0f0f0f0f)+uVU+0x0a0a0a0a;
			(PU8)oCol -= iOutPitch3m4;
			py -= uYPitch3m4;

		} /* for cc */
		pY += uYPitch4;
		pV += uVUPitch;
		pU += uVUPitch;
		oRow += iOutPitch4;
	} /* for rr */
	}
	return 0;
}	/* C_YVU9toCLUT8 */


U8		RGB_24_R[256+2*V2RBIAS] = { 0 };
U8		RGB_24_G[256+2*(V2GBIAS+U2GBIAS)] = { 0 };
U8		RGB_24_B[256+2*U2BBIAS] = { 0 };
U32		VU[512] = { 0 };

#define R(y,d) RGB_24_R[py[y]+uV2r+D24Y##d+D24V##d]
#define G(y,d) RGB_24_G[py[y]+uVU2g+D24Y##d+D24V##d+D24U##d]
#define B(y,d) RGB_24_B[py[y]+uU2b+D24Y##d+D24U##d]

#if B_HOST_IS_BENDIAN

#define FILL_BRGB(rgb) (((rgb) << 24)  | ((rgb) & 0xff) \
						| (((rgb) & 0xff00) << 8) | (((rgb) & 0xff0000) >> 8))
#define FILL_GBRG(rgb) (((((rgb) << 16) | ((rgb) >> 8)) & 0xff0000ff)	\
						| ((rgb) & 0xff0000) | (((rgb) & 0xff) << 8))
#define FILL_RGBR(rgb) (((((rgb) << 8) | ((rgb) >> 16)) & 0xff0000ff)	\
						| ((rgb) & 0xff00) | (((rgb) & 0xff) << 16 ))

#define BRGB(x,y,z,w) B(1,y) | (R(0,x)<<8) | (G(0,x)<<16) | (B(0,x)<<24)
#define GBRG(x,y,z,w) G(2,z) | (B(2,z)<<8) | (R(1,y)<<16) | (G(1,y)<<24)
#define RGBR(x,y,z,w) R(3,w) | (G(3,w)<<8) | (B(3,w)<<16) | (R(2,z)<<24)

#define MASKWRITE24(tMask, oCol, uOut1, uOut2, uOut3) \
	if ((tMask) & 8) {							\
		((PU8)(oCol))[0] = (U8)((uOut1)>>24);	\
		((PU8)(oCol))[1] = (U8)((uOut1)>>16);	\
		((PU8)(oCol))[2] = (U8)((uOut1)>>8);	\
	}											\
	if ((tMask) & 4) {							\
		((PU8)(oCol))[3] = (U8)(uOut1);			\
		((PU8)(oCol))[4] = (U8)((uOut2)>>24);	\
		((PU8)(oCol))[5] = (U8)((uOut2)>>16);	\
	}											\
	if ((tMask) & 2) {							\
		((PU8)(oCol))[6] = (U8)((uOut2)>>8);	\
		((PU8)(oCol))[7] = (U8)(uOut2);			\
		((PU8)(oCol))[8] = (U8)((uOut3)>>24);	\
	}											\
	if ((tMask) & 1) {							\
		((PU8)(oCol))[9] = (U8)((uOut3)>>16);	\
		((PU8)(oCol))[10] = (U8)((uOut3)>>8);	\
		((PU8)(oCol))[11] = (U8)(uOut3);		\
	}

#else /* B_HOST_IS_BENDIAN */

#define FILL_BRGB(rgb) (((rgb) << 24) | ((rgb) & 0x00ffffff))
#define FILL_GBRG(rgb) (((rgb) << 16) | (((rgb) & 0x00ffff00) >> 8))
#define FILL_RGBR(rgb) (((rgb) << 8) | (((rgb) & 0x00ff0000) >> 16))

#define BRGB(x,y,z,w) (B(1,y)<<24) | (R(0,x)<<16) | (G(0,x)<<8) | B(0,x)
#define GBRG(x,y,z,w) (G(2,z)<<24) | (B(2,z)<<16) | (R(1,y)<<8) | G(1,y)
#define RGBR(x,y,z,w) (R(3,w)<<24) | (G(3,w)<<16) | (B(3,w)<<8) | R(2,z)

#define MASKWRITE24(tMask, oCol, uOut1, uOut2, uOut3) \
	if ((tMask) & 8) {						\
		((PU8)(oCol))[0] = (U8)(uOut1);		\
		((PU8)(oCol))[1] = (U8)(uOut1>>8);	\
		((PU8)(oCol))[2] = (U8)(uOut1>>16); \
	}										\
	if ((tMask) & 4) {						\
		((PU8)(oCol))[3] = (U8)(uOut1>>24);	\
		((PU8)(oCol))[4] = (U8)(uOut2);		\
		((PU8)(oCol))[5] = (U8)(uOut2>>8);	\
	}										\
	if ((tMask) & 2) {						\
		((PU8)(oCol))[6] = (U8)(uOut2>>16);	\
		((PU8)(oCol))[7] = (U8)(uOut2>>24);	\
		((PU8)(oCol))[8] = (U8)(uOut3);		\
	}										\
	if ((tMask) & 1) {						\
		((PU8)(oCol))[9] = (U8)(uOut3>>8);	\
		((PU8)(oCol))[10] = (U8)(uOut3>>16);\
		((PU8)(oCol))[11] = (U8)(uOut3>>24);\
	}

#endif /* B_HOST_IS_BENDIAN */


void YVU9_to_RGB24_C(U32 nRows, U32 nCols, PU8 pY, PU8 pV, PU8 pU,
	U32 uYPitch, U32 uVUPitch, PU8 pOut, I32 uOutPitch,
	PU8 pTMask, PU8 pLMask, U32 uTStride, U32 uLStride,
	U32 uWantsFill, U32 uFillValue) {

	U32		r, c;
	PU8		oRow, tCol, lCol;
	PU32	oCol;
	U32		uYPitch4, uYPitch3m4;
	U32		uOutPitch4, uOutPitch3m12;

/*	note:  this code performs 32 bit writes to memory/frame buffer, and thus will */
/*	perform better when the output is aligned on a 32 bit boundary.  no checking is */
/*	done here to test for and handle this case. */

	oRow = pOut;
	uYPitch3m4 = uYPitch * 3 - 4;
	uYPitch4 = uYPitch * 4;
	uOutPitch3m12 = uOutPitch * 3 - 12;
	uOutPitch4 = uOutPitch * 4;

	if (pTMask || pLMask) {
		U32		uTStride4, uTStride3;
		U32		uLStride4, uLStride3;
		U32		uFill1, uFill2, uFill3;
		uTStride3 = uTStride * 3;
		uLStride3 = uLStride * 3;
		uTStride4 = uTStride * 4;
		uLStride4 = uLStride * 4;
		if (uWantsFill) {
			uFill1 = FILL_BRGB(uFillValue);
			uFill2 = FILL_GBRG(uFillValue);
			uFill3 = FILL_RGBR(uFillValue);
		}
	/* loop for transparency or local decode */
	for (r = 0; r < nRows; r+=4 ) {
		PU8	pv, pu, py;

		py = pY;
		pv = pV;
		pu = pU;
		oCol = (PU32)oRow;
		tCol = pTMask;
		lCol = pLMask;
		for (c = 0; c < nCols; c+=4) {
			register U32	uU2b, uV2r, uVU2g;
			register U32	uOut1, uOut2, uOut3;
			register U8		ltShift, tMask, lMask;

			/* set up the color contribution stuff */
			uV2r = *pv++;
			uV2r = VU[uV2r*2];
			
			uU2b = *pu++;
			uU2b = VU[uU2b*2+1];

			uVU2g = (uV2r + uU2b) >> 9;
			uV2r &= 0x1ff;
			uU2b &= 0x1ff;

			ltShift = (U8)(c&4)^4;	/* either shift 0 or 4 bits */

		/*row 0 */
			lMask = pLMask ? (lCol[c>>3] >> ltShift) & 0xf : 0xf;
			tMask = pTMask ? (tCol[c>>3] >> ltShift) & lMask : lMask;
			if (tMask) {

				uOut1 = BRGB(0,1,2,3);
				uOut2 = GBRG(0,1,2,3);
				uOut3 = RGBR(0,1,2,3);

				if (tMask == 0xf) {
					oCol[0] = uOut1; oCol[1] = uOut2; oCol[2] = uOut3;
				}
				else {
					MASKWRITE24(tMask, oCol, uOut1, uOut2, uOut3);
					if (uWantsFill) {
						MASKWRITE24(tMask^lMask, oCol, uFill1, uFill2, uFill3);
					}
				}
			}
			else if (uWantsFill) {
				if (lMask == 0xf) {
					oCol[0] = uFill1; oCol[1] = uFill2; oCol[2] = uFill3;
				}
				else {
					MASKWRITE24(lMask, oCol, uFill1, uFill2, uFill3);
				}
			}
			(PU8)oCol += uOutPitch;
			tCol += uTStride;
			lCol += uLStride;
			py += uYPitch;
		/*row 1 */
			lMask = pLMask ? (lCol[c>>3] >> ltShift) & 0xf : 0xf;
			tMask = pTMask ? (tCol[c>>3] >> ltShift) & lMask : lMask;
			if (tMask) {
				uOut1 = BRGB(4,5,6,7);
				uOut2 = GBRG(4,5,6,7);
				uOut3 = RGBR(4,5,6,7);

				if (tMask == 0xf) {
					oCol[0] = uOut1; oCol[1] = uOut2; oCol[2] = uOut3;
				}
				else {
					MASKWRITE24(tMask, oCol, uOut1, uOut2, uOut3);
					if (uWantsFill) {
						MASKWRITE24(tMask^lMask, oCol, uFill1, uFill2, uFill3);
					}
				}
			}
			else if (uWantsFill) {
				if (lMask == 0xf) {
					oCol[0] = uFill1; oCol[1] = uFill2; oCol[2] = uFill3;
				}
				else {
					MASKWRITE24(lMask, oCol, uFill1, uFill2, uFill3);
				}
			}
			(PU8)oCol += uOutPitch;
			tCol += uTStride;
			lCol += uLStride;
			py += uYPitch;
		/*row 2 */
			lMask = pLMask ? (lCol[c>>3] >> ltShift) & 0xf : 0xf;
			tMask = pTMask ? (tCol[c>>3] >> ltShift) & lMask : lMask;
			if (tMask) {
				uOut1 = BRGB(8,9,a,b);
				uOut2 = GBRG(8,9,a,b);
				uOut3 = RGBR(8,9,a,b);

				if (tMask == 0xf) {
					oCol[0] = uOut1; oCol[1] = uOut2; oCol[2] = uOut3;
				}
				else {
					MASKWRITE24(tMask, oCol, uOut1, uOut2, uOut3);
					if (uWantsFill) {
						MASKWRITE24(tMask^lMask, oCol, uFill1, uFill2, uFill3);
					}
				}
			}
			else if (uWantsFill) {
				if (lMask == 0xf) {
					oCol[0] = uFill1; oCol[1] = uFill2; oCol[2] = uFill3;
				}
				else {
					MASKWRITE24(lMask, oCol, uFill1, uFill2, uFill3);
				}
			}
			(PU8)oCol += uOutPitch;
			tCol += uTStride;
			lCol += uLStride;
			py += uYPitch;
		/*row 3 */
			lMask = pLMask ? (lCol[c>>3] >> ltShift) & 0xf : 0xf;
			tMask = pTMask ? (tCol[c>>3] >> ltShift) & lMask : lMask;
			if (tMask) {
				uOut1 = BRGB(c,d,e,f);
				uOut2 = GBRG(c,d,e,f);
				uOut3 = RGBR(c,d,e,f);

				if (tMask == 0xf) {
					oCol[0] = uOut1; oCol[1] = uOut2; oCol[2] = uOut3;
				}
				else {
					MASKWRITE24(tMask, oCol, uOut1, uOut2, uOut3);
					if (uWantsFill) {
						MASKWRITE24(tMask^lMask, oCol, uFill1, uFill2, uFill3);
					}
				}
			}
			else if (uWantsFill) {
				if (lMask == 0xf) {
					oCol[0] = uFill1; oCol[1] = uFill2; oCol[2] = uFill3;
				}
				else {
					MASKWRITE24(lMask, oCol, uFill1, uFill2, uFill3);
				}
			}
			(PU8)oCol -= uOutPitch3m12;
			tCol -= uTStride3;
			lCol -= uLStride3;
			py -= uYPitch3m4;

		} /* for c */
		pY += uYPitch4;
		pV += uVUPitch;
		pU += uVUPitch;
		oRow += uOutPitch4;
		if (pTMask)	pTMask += uTStride4;
		if (pLMask) pLMask += uLStride4;
	} /* for r */
	}
	else {
	/* normal - non-transparent loop */
	for (r = 0; r < nRows; r+=4 ) {
		PU8	pv, pu, py;

		py = pY;
		pv = pV;
		pu = pU;
		oCol = (PU32)oRow;
		for (c = 0; c < nCols; c+=4) {
			register U32	uU2b, uV2r, uVU2g;
			register U32	uOut1, uOut2, uOut3;

			/* set up the color contribution stuff */
			uV2r = *pv++;
			uV2r = VU[uV2r*2];
			
			uU2b = *pu++;
			uU2b = VU[uU2b*2+1];

			uVU2g = (uV2r + uU2b) >> 9;
			uV2r &= 0x1ff;
			uU2b &= 0x1ff;

		/*row 0 */
			uOut1 = BRGB(0,1,2,3);
			uOut2 = GBRG(0,1,2,3);
			uOut3 = RGBR(0,1,2,3);
			oCol[0] = uOut1; oCol[1] = uOut2; oCol[2] = uOut3;
			(PU8)oCol += uOutPitch;
			py += uYPitch;
		/*row 1 */
			uOut1 = BRGB(4,5,6,7);
			uOut2 = GBRG(4,5,6,7);
			uOut3 = RGBR(4,5,6,7);
			oCol[0] = uOut1; oCol[1] = uOut2; oCol[2] = uOut3;
			(PU8)oCol += uOutPitch;
			py += uYPitch;
		/*row 2 */
			uOut1 = BRGB(8,9,a,b);
			uOut2 = GBRG(8,9,a,b);
			uOut3 = RGBR(8,9,a,b);
			oCol[0] = uOut1; oCol[1] = uOut2; oCol[2] = uOut3;
			(PU8)oCol += uOutPitch;
			py += uYPitch;
		/*row 3 */
			uOut1 = BRGB(c,d,e,f);
			uOut2 = GBRG(c,d,e,f);
			uOut3 = RGBR(c,d,e,f);
			oCol[0] = uOut1; oCol[1] = uOut2; oCol[2] = uOut3;
			(PU8)oCol -= uOutPitch3m12;
			py -= uYPitch3m4;

		} /* for c */
		pY += uYPitch4;
		pV += uVUPitch;
		pU += uVUPitch;
		oRow += uOutPitch4;
	} /* for r */
	} /* else */

} /* YVU9_to_RGB24_C */


void
ModifyContrastOrSaturation(U16 iDelta, PU8 puTable);

void YVU9_to_RGB24_Init(U32 mode, U32 uCCFlags);
void YVU9_to_RGB24_Init(U32 mode, U32 uCCFlags) {
	PU8		PRValue;
	PU8		PGValue;
	PU8		PBValue;
	PU32	PVUContrib;


	I32 i, ii;
	U32		uAdjustment;

	U8		BSCTable[256];

	if (uCCFlags & CCF_DOING_ALT_LINE)
		uAdjustment = 10;
	else
		uAdjustment =  0;

	PRValue      = RGB_24_R;
	PGValue      = RGB_24_G;
	PBValue      = RGB_24_B;
	PVUContrib   = VU;

	for (i = 0; i < 256; i++) {
		if (i + uAdjustment > 255)
			BSCTable[i] = 255;
		else
			BSCTable[i] = i + (U8)uAdjustment;
	}

	ModifyContrastOrSaturation((U16)(uAdjustment*uAdjustment+256), &BSCTable[0] );

	for (ii = 0; ii < 256; ii++) {
		U32	t1, t2;

		t1 = (((V2R*(ii-128L))>>16L)+V2RBIAS) & 0x1ff; /* biased V to R. */
		t2 = (((V2G*(ii-128L))>>16L)+V2GBIAS) & 0x1ff; /* biased V to G. */
		*PVUContrib++ = (t2 << 9) | t1;

		t1 = (((U2B*(ii-128L))>>16L)+U2BBIAS) & 0x1ff; /* biased U to B. */
		t2 = (((U2G*(ii-128L))>>16L)+U2GBIAS) & 0x1ff; /* biased U to G. */
		*PVUContrib++ = (t2 << 9) | t1;
	}

	for (i = 0; i < 255 + 2 * V2RBIAS; i++) {
		ii = (I32)(((I32)(i - YMIN - V2RBIAS)) * Y2RGB) >> 16L;
		if (ii <   0L) ii =   0L;
		if (ii > 255L) ii = 255L;
		PRValue[i] = BSCTable[ii];
	}

	for (i = 0; i < 255 + 2 * (V2GBIAS + U2GBIAS); i++) {
		ii = (I32)(((I32) i - YMIN - (V2GBIAS + U2GBIAS)) * Y2RGB) >> 16L;
		if (ii <   0L) ii =   0L;
		if (ii > 255L) ii = 255L;
		PGValue[i] = BSCTable[ii];
	}

	for (i = 0; i < 255 + 2 * U2BBIAS; i++) {
		ii = (I32)(((I32) i - YMIN - U2BBIAS) * Y2RGB) >> 16L;
		if (ii <   0L) ii =   0L;
		if (ii > 255L) ii = 255L;
		PBValue[i] = BSCTable[ii];
	}
} /* YVU9_to_RGB24_Z2_Alt_Init */
