/* ex:set sw=2 ts=8 wm=0:
 * $Header: u:/rcs/cv/rcs/dib2yuv2.c 3.4 1994/10/20 17:36:17 bog Exp $

 * (C) Copyright 1992-1994 SuperMac Technology, Inc.
 * All rights reserved

 * This source code and any compilation or derivative thereof is the
 * sole property of SuperMac Technology, Inc. and is provided pursuant
 * to a Software License Agreement.  This code is the proprietary
 * information of SuperMac Technology and is confidential in nature.
 * Its use and dissemination by any party other than SuperMac Technology
 * is strictly limited by the confidential information provisions of the
 * Agreement referenced above.

 * $Log: dib2yuv2.c $
 * Revision 3.4  1994/10/20 17:36:17  bog
 * Modifications to support Sunnyvale Reference version.
 * Revision 3.3  1994/04/13  11:24:57  unknown
 * Our divide by 7 in (2R+4G+B)/7 should round instead of chop, to match the
 * ASM version.
 *
 * Revision 3.2  1994/02/23  15:08:54  timr
 * Many miscellaneous fixes: I was decrementing firstTime too many times;
 * I had an off-by-one in the UVBody loop; I was using the wrong delta between
 * U & V work scans; I was using a value and modifying it without an 
 * intermediate sequence point.  This now appears healthy.
 *
 * Revision 3.1  1994/01/13  01:57:25  geoffs
 * Refined debug stuff with all C version
 *
 * Revision 3.0  1993/12/10  14:23:54  timr
 * Initial revision, NT C-only version.
 *
 *
 * CompactVideo Codec Convert DIB to VECTOR
 */

#if	defined(WINCPK)

#include <windows.h>
#include <windowsx.h>

#include "compddk.h"

#endif

#include "cv.h"
#include "cvcompre.h"
#include "dib2yuv2.h"


#if	!defined(NOASM)
//#define	STATISTICS_ALWAYS		// for now
#define	STATISTICS			// for now
#if	(defined(DEBUG) && defined(STATISTICS)) || defined(STATISTICS_ALWAYS)
unsigned short genCodeSize = 0;		// compiled code size
#endif
#endif

#if	!defined(WIN32)
/**********************************************************************
 *
 * CopyMem()
 *
 * Copy source to destination. Move size can be no greater than 64kb
 * and must not wrap the end of source or destination segments.
 *
 * Returns a -> to the next destination location after the copy
 *
 **********************************************************************/

unsigned char far * PASCAL CopyMem(
  unsigned char far *pS,
  unsigned char far *pD,
  unsigned short len
) {
  _asm {
	push	si
	push	di
	push	ds

	lds	si,pS		// fetch -> source 
	les	di,pD		// fetch -> destination
	mov	cx,len		// # bytes to copy

	shr	cx,1		// get # whole words to copy
	rep	movsw		// move all whole words
	adc	cx,cx
	rep	movsb		// move any odd byte

	mov	word ptr pD,di	// remember next destination

	pop	ds
	pop	di
	pop	si
  }

  return (pD);			// return -> next destination
}
#else
#define	CopyMem(s,d,l)		(\
				 CopyMemory((d),(s),(l)),\
				 ((unsigned char far *) (d) + (l))\
				)
#endif

// This C implementation of DYUVFRAG is implemented as a threaded interpreter.


// If a fragment contains a jump, the instruction following the jump should
// be JUMP plus the delta from source to target.  If such an instruction is
// flowed into, it is ignored.

// Taking the jump within a fragment then becomes:
//    nextPC += instruction [nextPC] - JUMP;

// You could conceivably include multiple jump destinations by tacking on
// multiple JUMP codes.


void
CopyFragments (FRAGMENT FAR * base, int ofsTo, int ofsFrom, int count)
{
    FRAGMENT FAR * fFrom = base + ofsFrom;
    FRAGMENT FAR * fTo = base + ofsTo;

    while (count--)
        *fTo++ = *fFrom++;
}


void
ExecuteFragments (
    DIBTOYUVPRIVATE FAR * pP, 
    FRAGMENT FAR * instruction
)
{
	DIBTOYUVPRIVATE dp = *pP;

// This data was originally in DIBTOYUVPRIVATE.

//  unsigned short tileHeight;		// # scans in current tile

	unsigned char hzSwizzle;		// used for nerp'ing through VECTOR

//  BYTE FAR * oBits;			// 32-bit offset of tile data

//  BYTE FAR * oDetail;			// 32-bit offset of detail list
	BYTE FAR * oDetail_0Mod4;		// current 0 mod 4 detail ->
#define oDetail_0Mod2	oDetail_0Mod4
	BYTE FAR * oDetail_1Mod4;		// current 1 mod 4 detail ->
#define oDetail_1Mod2	oDetail_1Mod4
	BYTE FAR * oDetail_2Mod4;		// current 2 mod 4 detail ->
	BYTE FAR * oDetail_3Mod4;		// current 3 mod 4 detail ->
	
//  BYTE FAR * oSmooth;			// 32-bit offset of smooth list
	BYTE FAR * oSmooth_0Mod2;		// current 0 mod 2 smooth ->
	BYTE FAR * oSmooth_1Mod2;		// current 1 mod 2 smooth ->
	BYTE FAR * oSmoothUV;			// 32-bit offset of smooth U2 list
	
	BYTE FAR * oInterU2Work;		// working 32-bit offset of U2
	BYTE FAR * oInterV2Work;		// working 32-bit offset of V2
	
	BYTE FAR * oWork;			// 32-bit offset of work buffer
	BYTE FAR * oWS0Current;		// 32-bit offset of current workscan 0
	BYTE FAR * oWS0Effective;		// 32-bit offset of effective workscan 0
	BYTE FAR * oWS1Current;		// 32-bit offset of current workscan 1
	BYTE FAR * oWS2Current;		// 32-bit offset of current workscan 2
	BYTE FAR * oWS3Current;		// 32-bit offset of current workscan 3
	BYTE FAR * oWS3Effective;		// 32-bit offset of effective workscan 3
	
	unsigned long srcWidth;		// actual width in pixels of input
	unsigned long cntHLoop;		// loop counter for horizontal filters
	
	unsigned long cntVLoop;		// loop counter for vertical filter
	
	unsigned long firstTime;		// first time flag for code
	
	unsigned long workYStep4;		// 4 ysteps

//  RGBA FAR * lookUp8;			// 8 bit palettized lookup table

//  unsigned char divBy7[256 * 7];	// the divide by 7 table

	RGB	r;
	YUV	p;
	
	BYTE FAR * rRGB;
	BYTE FAR * riUV;
	BYTE FAR * roUV;
	
	BYTE FAR * rDest;
	BYTE FAR * rDetail;
	
	BYTE FAR * rWorkY;
	BYTE FAR * rWorkU;
	BYTE FAR * rWorkV;
	
	BYTE FAR * rWork0;
	BYTE FAR * rWork1;
	BYTE FAR * rWork2;
	BYTE FAR * rWork3;
	
	BYTE FAR * tmp;
	
	WORD Y2lo;
	WORD Y2hi;
	WORD U2lo;
	WORD U2hi;
	WORD V2lo;
	WORD V2hi;
	
	int stop = 0;
	int k;
	unsigned long UVOffset;
	
	int nextPC = 0;
	
	workYStep4 = 4 * dp.cd.workYStep;
	
	while (!stop)
	{
	//	printf("instruction=%d %02x\n",instruction[nextPC],instruction[nextPC]);
		switch (instruction [nextPC++])
		{
			case cEntry:
				break;

			// Fragments for performing horizontal 1331 filtering.
			
			// Incoming:
			//   rRGB -> 8, 16, 24 or 32 bit DIB
			
			// Outgoing:
			//   rDetail -> detail list
			//   rWorkY -> Y 1331
			//   rWorkU -> U 1331
			//   rWorkV -> V 1331

		case H1331Init:
			cntVLoop = dp.tileHeight >> 1;
			
			oInterU2Work = dp.cd.oInterU2;
			oInterV2Work = dp.cd.oInterV2;
			oWork = dp.cd.oWork;
			
			oDetail_0Mod4 = dp.oDetail;
			oDetail_1Mod4 = oDetail_0Mod4 + 2;	// skip YY
			oDetail_2Mod4 = oDetail_1Mod4 + 14;	// skip YYUV YYYYUV
			oDetail_3Mod4 = oDetail_2Mod4 + 2;	// skip YY
			
			oSmooth_0Mod2 = dp.oSmooth;
			oSmooth_1Mod2 = dp.oSmooth + 2;
			
			oWS1Current = oWork;
			oWS2Current = oWork + dp.cd.workYStep;
			oWS3Current = oWS2Current + dp.cd.workYStep;
			oWS0Current = oWS3Current + dp.cd.workYStep;
			
			firstTime = 1;
			break;
		
		case H1331Start:
			rRGB = dp.oBits;			// current input scanline
			if (dp.srcHeight > 0)
			{
				dp.srcHeight--;
				dp.oBits += dp.cd.DIBYStep;	// point to next scanline
			}
			
			rWorkY = oWork;			// current work areas
			rWorkU = rWorkY + workYStep4;
			rWorkV = rWorkU + workYStep4;
			oWork += dp.cd.workYStep;		// point to next work area
			if (oWork >= (dp.cd.oWork + workYStep4))	// handle overflow
			{
				oWork = dp.cd.oWork;
			}
			
			hzSwizzle = 1;			// every other time through loop
			rDetail = oDetail_0Mod4;	// next scan in detail list
			srcWidth = dp.cd.DIBYStep - 1;
			cntHLoop = dp.cd.workYStep;	// save for loop counter;
			break;
		
		case H1331End:
			//	    case H134:
			// Add last component in again.
			
			*rWorkY = (BYTE) ((Y2lo + Y2hi + 3) >> 3);
			*rWorkV = (BYTE) ((V2lo + V2hi + 3) >> 3);
			*rWorkU = (BYTE) ((U2lo + U2hi + 3) >> 3);
			
			// HERE ENDS H134
			oDetail_0Mod4 = oDetail_1Mod4;
			oDetail_1Mod4 = oDetail_2Mod4;
			oDetail_2Mod4 = oDetail_3Mod4;
			oDetail_3Mod4 = rDetail;
			rDetail = oDetail_0Mod4;
			break;
			
		case H1331FetchPixel8:
			r = *(RGB FAR *) (dp.lookUp8 + *rRGB);
			
			if (srcWidth--)
			{
				rRGB++;
			}
			break;
		
		case H1331FetchPixel15:
			// rRGB -> GGGBBBBB 0RRRRRRGG
			
			r.R = (rRGB [1] & 0x7c) << 1;
			r.G = (((rRGB [1] & 0x03) << 6) | ((rRGB [0] & 0xe0) >> 2));
			r.B = (rRGB [0] & 0x3f) << 3;
			
			if (srcWidth--)
			{
				rRGB += 2;
			}
			break;
		
		case H1331FetchPixel16:
			// rRGB -> GGGBBBBB RRRRRRGGG
			
			r.R = (rRGB [1] & 0xf8);
			r.G = (((rRGB [1] & 0x07) << 5) | ((rRGB [0] & 0xe0) >> 2));
			r.B = (rRGB [0] & 0x3f) << 3;
			
			if (srcWidth--)
			{
				rRGB += 2;
			}
			break;
		
		case H1331FetchPixel24:
			r = *(RGB FAR *) rRGB;
			if (srcWidth--)
			{
				rRGB += 3;
			}
			break;
		
		case H1331FetchPixel32:
			r = *(RGB FAR *) rRGB;
		//	printf("fetched, %02x %02x %02x\n",r.R,r.G,r.B);
			if (srcWidth--)
			{
				rRGB += 4;
			}
			break;
		
		case H1331ToYUV:
	//	printf("RGBto YUV: %02x %02x %02x\n",r.R,r.G,r.B);
			k = ((r.G * 2 + r.R) * 2 + r.B);	// 2R+4G+B
			p.Y = (k + 3 + (k & 1)) / 7;		// round(k/7)
			p.U = (unsigned char) (128 + ((int) r.B - (int) p.Y) / 2);
			p.V = (unsigned char) (128 + ((int) r.R - (int) p.Y) / 2);
			break;
		
		case H1331StoreY0:
			rDetail[0] = p.Y;
			break;
		
		case H1331StoreY1:
			rDetail[1] = p.Y;
			break;
		
		case H431:
			Y2lo = p.Y << 2;
			V2lo = p.V << 2;
			U2lo = p.U << 2;
			nextPC += instruction[nextPC] - JUMP;
			break;
		
		case H1331B:
			*rWorkY++ = (BYTE) ((Y2hi + p.Y + 3) >> 3);	// add in 133(1)
			Y2lo += 3 * p.Y;		// add in 1(3)31
			
			*rWorkU++ = (BYTE) ((U2hi + p.U + 3) >> 3);	// add in 133(1)
			U2lo += 3 * p.U;		// add in 1(3)31
			
			*rWorkV++ = (BYTE) ((V2hi + p.V + 3) >> 3);	// add in 133(1)
			V2lo += 3 * p.V;		// add in 1(3)31
			
			break;
		
		case H1331C:
			Y2hi = (Y2lo + 3 * p.Y);	// add in 13(3)1
			Y2lo = p.Y;			// add in (1)331
			
			U2hi = (U2lo + 3 * p.U);	// add in 13(3)1
			U2lo = p.U;			// add in (1)331
			
			V2hi = (V2lo + 3 * p.V);	// add in 13(3)1
			V2lo = p.V;			// add in (1)331
			
			break;
		
		case H1331Loop:
			rDetail += sizeof (VECTOR);
			if ((hzSwizzle ^= 1) & 1)
			{
				rDetail += 2 * sizeof (VECTOR);
			}
			
			if (--cntHLoop)
			{
				nextPC += instruction[nextPC] - JUMP;
			}
			break;

		// Fragments for performing vertical 1331 filtering for Y2,U2,V2.
		
		// Incoming:
		//    rWork0 -> top scan of a set of 4 in rWork
		//    rWork1 -> next scan
		//    rWork2 -> next scan
		//    rWork3 -> bottom scan of a set of 4 in rWork
		
		// Outgoing:
		//    rDest -> result detail list

		case V1331EarlyOut:
			if (cntVLoop == 1)
			{
				nextPC += instruction[nextPC] - JUMP;
			}
			break;
		
		case V1331FetchY2Dest:
			rDest = oSmooth_0Mod2;
			rWork0 = oWS0Current;
			rWork1 = oWS1Current;
			rWork2 = oWS2Current;
			rWork3 = oWS3Current;
			if (firstTime)
			{
				firstTime = 0;
				rWork0 = rWork1;	// setup for 431
			}
			oWS0Effective = rWork0;
			
			if (cntVLoop == 1)
			{
				rWork3 = rWork2;	// setup for 134
			}
			oWS3Effective = rWork3;
			cntHLoop = dp.cd.workYStep;
			break;
		
		case V1331FetchU2Dest:
			rDest = oInterU2Work;
			rWork0 = oWS0Effective + workYStep4;
			rWork1 = oWS1Current + workYStep4;
			rWork2 = oWS2Current + workYStep4;
			rWork3 = oWS3Effective + workYStep4;
			cntHLoop = dp.cd.workYStep;
			break;
		
		case V1331FetchV2Dest:
			rDest = oInterV2Work;
			rWork0 = oWS0Effective + 2 * workYStep4;
			rWork1 = oWS1Current + 2 * workYStep4;
			rWork2 = oWS2Current + 2 * workYStep4;
			rWork3 = oWS3Effective + 2 * workYStep4;
			cntHLoop = dp.cd.workYStep;
			break;
		
		case V1331Body:
#ifdef	DEBUG
		case V1331Body_Y2:
#endif
			*rDest = (*rWork0++ + (*rWork1++ + *rWork2++) * 3 + *rWork3++ + 3) >> 3;
			break;
		
		case V1331IncY2Dest:
			// !!! This assumes that oWork always starts word aligned.
			rDest += ((unsigned long)rDest) & 1 ? 7 : 1;
			break;
		
		case V1331IncU2V2Dest:
			rDest++;
			break;
		
		case V1331Loop:
			if (--cntHLoop)
			{
				nextPC += instruction[nextPC] - JUMP;
			}
			break;
		
		case V1331StoreY2Dest:
			oSmooth_0Mod2 = oSmooth_1Mod2;
			oSmooth_1Mod2 = rDest;
			rDest = oSmooth_0Mod2;
			break;
		
		case V1331StoreU2Dest:
			oInterU2Work = rDest;
			break;
		
		case V1331StoreV2Dest:
			oInterV2Work = rDest;
			break;
		
		case HVLoop:
			if (--cntVLoop)
			{
				nextPC += instruction[nextPC] - JUMP;
			}
			break;

		// Fragments for performing filtering for U2,V2 from the U,V filtered pixels
		// now residing in intermediate buffers.  We use a filter similar to the 
		// original one to produce a twice filtered product.

		case U2V2HInit:
			cntVLoop = dp.tileHeight >> 2;
			oInterU2Work = dp.cd.oInterU2;
			UVOffset = dp.cd.oInterV2 - dp.cd.oInterU2;
			oWork = dp.cd.oWork;
			
			oDetail_0Mod2 = dp.oDetail+4;	// -> UV for 0 mod 2 scans
			oDetail_1Mod2 = dp.oDetail+20;	// -> UV for 1 mod 2 scans
			oSmoothUV = dp.oSmooth + 4;	// -> first smooth UV
			
			oWS1Current = oWork;
			oWS2Current = oWS1Current + dp.cd.workYStep;
			oWS3Current = oWS2Current + dp.cd.workYStep;
			oWS0Current = oWS3Current + dp.cd.workYStep;
			
			firstTime = 1;
			break;
		
		case U2V2HStart:
			riUV = oInterU2Work;	// -> incoming
			roUV = oDetail_0Mod2;	// -> outgoing
			
			rWorkU = oWork;
			rWorkV = oWork + workYStep4;
			
			oWork += dp.cd.workYStep;		// point to next work area
			if (oWork >= (dp.cd.oWork + workYStep4))	// handle overflow
			{
				oWork = dp.cd.oWork;
			}
			cntHLoop = dp.cd.workYStep >> 1;
			break;
		
		case U2V2HEnd:
//	    case U2V2H134:
			*rWorkV = (BYTE) ((V2lo + V2hi + 3) >> 3);
			*rWorkU = (BYTE) ((U2lo + U2hi + 3) >> 3);
			// Here Ends U2V2H134
			oInterU2Work = riUV;
			oDetail_0Mod2 = oDetail_1Mod2;
			oDetail_1Mod2 = roUV;
			roUV = oDetail_0Mod2;
			break;
		
		case U2V2H431:
			U2lo = *riUV;
			V2lo = riUV [UVOffset];
			riUV++;
			*roUV++ = (BYTE) U2lo;
			*roUV++ = (BYTE) V2lo;
			roUV += 8 - 2;
			U2lo <<= 2;			// set (4)31
			V2lo <<= 2;			// set (4)31
			nextPC += instruction[nextPC] - JUMP;
			break;
		
		case U2V2H1331B:
			k = *riUV;
			*roUV++ = k;
			*rWorkU++ = (BYTE) ((U2hi + k + 3) >> 3);	// set 133(1)
			U2lo += k * 3;			// add in 1(3)31
			
			k = riUV [UVOffset];
			*roUV++ = k;
			*rWorkV++ = (BYTE) ((V2hi + k + 3) >> 3);	// set 133(1)
			V2lo += k * 3;			// add in 1(3)31
			
			riUV ++;
			roUV += 8 - 2;
			break;
		
		case U2V2H1331C:
			k = *riUV;
			*roUV++ = k;
			U2hi = U2lo + k * 3;		// add in 13(3)1
			U2lo = k;			// add in (1)331
			
			k = riUV [UVOffset];
			*roUV++ = k;
			V2hi = V2lo + k * 3;		// add in 13(3)1
			V2lo = k;			// add in (1)331
			
			riUV ++;
			roUV += 24 - 2;		// point to next detail list spot
			break;
		
		case U2V2HLoop:
			if (--cntHLoop)
			{
				nextPC += instruction[nextPC] - JUMP;
			}
			break;
		
		case U2V2VBody:
			rDest = oSmoothUV;		// fetch -> smooth U's
			rWork0 = oWS0Current;	// load -> to each of 4 scans
			rWork1 = oWS1Current;
			rWork2 = oWS2Current;
			rWork3 = oWS3Current;
			
			if (firstTime)			// first time, need 431
			{
				firstTime = 0;
				rWork0 = rWork1;
			}
			oWS0Effective = rWork0;
			
			if (cntVLoop == 1)		// last scan, use 134
			{
				rWork3 = rWork2;
			}
			oWS3Effective = rWork3;
			cntHLoop = dp.cd.workYStep >> 1;
			
			while (cntHLoop--)
			{
				// Compute U2
				*rDest++ = (*rWork0 + (*rWork1 + *rWork2) * 3 + *rWork3 + 3) >> 3;

				// Compute V2
				*rDest++ = (rWork0[workYStep4]+(rWork1[workYStep4]+rWork2[workYStep4])*3+rWork3[workYStep4]+3)>>3;
				
				rDest += 8 - 2;
				
				rWork0++;
				rWork1++;
				rWork2++;
				rWork3++;
			}
			oSmoothUV = rDest;		// -> next spot in smooth list
			
			tmp = oWS2Current;		// swizzle ->s for next time
			oWS2Current = oWS0Current;
			oWS0Current = tmp;
			tmp = oWS3Current;		// swizzle ->s for next time
			oWS3Current = oWS1Current;
			oWS1Current = tmp;
			break;
		
		case cExit:
			stop = 1;			// could just return
			break;
		
		// Fragments to generate smooth vector Ys from detail vector Ys.
	
	    case rY_H1331Init:
			cntVLoop = dp.tileHeight >> 1;	// # out scans is 1/2 input
			oWork = dp.cd.oWork;
			oDetail_0Mod4 = dp.oDetail;	// 0 mod 4 scan in detail list
			oDetail_1Mod4 = dp.oDetail + 2;	// 1 mod 4 scan
			oDetail_2Mod4 = dp.oDetail + 16;	// 2 mod 4 scan
			oDetail_3Mod4 = dp.oDetail + 18;	// 3 mod 4 scan
	
			oSmooth_0Mod2 = dp.oSmooth;	// 0 mod 2 scan in smooth list
			oSmooth_1Mod2 = dp.oSmooth + 2;	// 1 mod 2 scan
	
			oWS1Current = oWork;
			oWS2Current = oWS1Current + dp.cd.workYStep;
			oWS3Current = oWS2Current + dp.cd.workYStep;
			oWS0Current = oWS3Current + dp.cd.workYStep;
	
			firstTime = 1;
			break;

		case rY_H1331Start:
			rWorkY = oWork;
			
			oWork += dp.cd.workYStep;		// point to next work area
			if (oWork >= (dp.cd.oWork + workYStep4))	// handle overflow
			{
				oWork = dp.cd.oWork;
			}
			
			hzSwizzle = 1;		// every other time through loop
			rDetail = oDetail_0Mod4;	// -> next scan in detail list
			
			cntHLoop = dp.cd.workYStep;
			break;
		
		case rY_H1331End:
//	    case rY_H134:
			*rWorkY = (BYTE) ((Y2lo + Y2hi + 3) >> 3);
			// Here Ends rY_H134
			oDetail_0Mod4 = oDetail_1Mod4;
			oDetail_1Mod4 = oDetail_2Mod4;
			oDetail_2Mod4 = oDetail_3Mod4;
			oDetail_3Mod4 = rDetail;
			rDetail = oDetail_0Mod4;
			break;
		
		case rY_H431:
			Y2lo = *rDetail << 2;	// set (4)31
			nextPC += instruction [nextPC] - JUMP;
			break;
		
		case rY_H1331B:
			*rWorkY++ = (BYTE) ((Y2hi + *rDetail + 3) >> 3);// add in 133(1)
			Y2lo += *rDetail * 3;			// add in 1(3)31
			break;
		
		case rY_H1331C:
			Y2hi = Y2lo + 3 * rDetail [1];		// add in 13(3)1
			Y2lo = rDetail [1];			// add in (1)331
			break;
		
		case rY_V1331Body:
			rDest = oSmooth_0Mod2;		// fetch -> smooth Y's
			rWork0 = oWS0Current;		// load ->s to each of 4 scans
			rWork1 = oWS1Current;
			rWork2 = oWS2Current;
			rWork3 = oWS3Current;
			
			if (firstTime)
			{
				firstTime = 0;
				rWork0 = rWork1;		// first time, dup first scan
			}
			if (cntVLoop == 1)
			{
				rWork3 = rWork2;		// last time, dup last scan
			}
				
			cntHLoop = dp.cd.workYStep;		// save pixels for loop count
				
			while (cntHLoop--)
			{
				// Compute Y2
				*rDest = (*rWork0++ + (*rWork1++ + *rWork2++) * 3 + *rWork3++ + 3)>>3;
				
				rDest += ((unsigned long)rDest) & 1 ? 7 : 1;
			}
			
			oSmooth_0Mod2 = oSmooth_1Mod2;	// swizzle smooth pointers
			oSmooth_1Mod2 = rDest;
			rDest = oSmooth_0Mod2;
			
			tmp = oWS0Current;		// swizzle workspace pointers
			oWS0Current = oWS2Current;
			oWS2Current = tmp;
			
			tmp = oWS1Current;
			oWS1Current = oWS3Current;
			oWS3Current = tmp;
			
			if (!--cntVLoop)
			{
				nextPC += instruction [nextPC] - JUMP;
			}
			break;
		
		// Code fragments to generate smooth vector UVs from detail vector UVs.

	    case rUV_H1331Init:
			cntVLoop = dp.tileHeight >> 1;	// # out scans is 1/2 input
			oWork = dp.cd.oWork;
			oDetail_0Mod2 = dp.oDetail + 4;	// -> U,V in first detail vector
			oDetail_1Mod2 = dp.oDetail + 20;	// skip UV YYYYUV YYYY
	
			oSmooth_0Mod2 = dp.oSmooth + 4;	// -> U,V in first smooth vector
	
			oWS1Current = oWork;
			oWS2Current = oWS1Current + dp.cd.workYStep;
			oWS3Current = oWS2Current + dp.cd.workYStep;
			oWS0Current = oWS3Current + dp.cd.workYStep;
	
			firstTime = 1;
			break;

	    case rUV_H1331Start:
			rWorkU = oWork;
			rWorkV = oWork + workYStep4;
	
			oWork += dp.cd.workYStep;	// point to next work area
			if (oWork >= (dp.cd.oWork + workYStep4))	// handle overflow
			{
				oWork = dp.cd.oWork;
			}
	
			hzSwizzle = 1;		// every other time through loop
			rDetail = oDetail_0Mod2;	// -> next scan in detail list
	
			cntHLoop = dp.cd.workYStep >> 1;
			break;

	    case rUV_H1331End:
//	    case rUV_H134:
			*rWorkU = (BYTE) ((U2lo + U2hi + 3) >> 3);
			*rWorkV = (BYTE) ((V2lo + V2hi + 3) >> 3);
			// Here Ends rUV_H134
			oDetail_0Mod2 = oDetail_1Mod2;
			oDetail_1Mod2 = rDetail;
			rDetail = oDetail_0Mod2;
			break;

	    case rUV_H431:
			U2lo = *rDetail << 2;		// add in (4)31
			V2lo = rDetail[1] << 2;		// add in (4)31
			nextPC += instruction [nextPC] - JUMP;	// jump into loop
			break;

	    case rUV_H1331B:
			*rWorkU++ = (BYTE) ((U2hi + *rDetail + 3) >> 3);// add in 133(1)
			U2lo += *rDetail++ * 3;			// addin 1(3)31
			*rWorkV++ = (BYTE) ((V2hi + *rDetail + 3) >> 3);// add in 133(1)
			V2lo += *rDetail * 3;			// addin 1(3)31
			
			rDetail += sizeof(VECTOR) - 1;		// skip to next UV
			if ((hzSwizzle ^= 1) & 1)		// every other time...
			{
				rDetail += 2 * sizeof(VECTOR);	// skip two vectors
			}

			break;

	    case rUV_H1331C:
			U2hi = U2lo + 3 * *rDetail;		// add in 13(3)1
			U2lo = *rDetail;			// add in (1)331
			V2hi = V2lo + 3 * rDetail[1];		// add in 13(3)1
			V2lo = rDetail[1];			// add in (1)331
			break;

	    case rUV_V1331Body:
			rDest = oSmooth_0Mod2;		// fetch -> smooth UV's
			rWork0 = oWS0Current;		// load ->s to each of 4 scans
			rWork1 = oWS1Current;
			rWork2 = oWS2Current;
			rWork3 = oWS3Current;
			
			if (firstTime)
			{
				firstTime = 0;
				rWork0 = rWork1;		// first time, dup first scan
			}
			if (cntVLoop == 1)
			{
				rWork3 = rWork2;		// last time, dup last scan
			}
			
			cntHLoop = dp.cd.workYStep >> 1;	// save pixels for loop count
			
			while (cntHLoop--)
			{
				// Compute U2
				*rDest++ = (*rWork0 + (*rWork1 + *rWork2) * 3 + *rWork3 + 3) >> 3;
				
				// Compute V2
				*rDest++ = (rWork0[workYStep4]+(rWork1[workYStep4]+rWork2[workYStep4])*3+rWork3[workYStep4]+3)>>3;
				
				rDest += 8 - 2;
				
				rWork0++;
				rWork1++;
				rWork2++;
				rWork3++;
			}	    
			
			oSmooth_0Mod2 = rDest;
			
			tmp = oWS0Current;		// swizzle workspace pointers
			oWS0Current = oWS2Current;
			oWS2Current = tmp;
			
			tmp = oWS1Current;
			oWS1Current = oWS3Current;
			oWS3Current = tmp;
			
			if (!cntVLoop--)
			{
				nextPC += instruction [nextPC] - JUMP;
			}
			break;

	    default:
			// This is probably a jump.
			if ((instruction [nextPC-1] & 0x8000) == 0)
			{
				// NOT A JUMP -- Illegal instruction.
#if	defined(NOASM) || defined(__BEOS__)
				DebugBreak ();
#else
		    _asm int 3
		    // I ought to issue a general protection fault...
#endif
			}
			break;
		}
	}
}


/**********************************************************************
 *
 * CompileFragments()
 *
 * Compile optimized code for DIB to VECTOR conversion
 *
 **********************************************************************/


int PASCAL CompileFragments(
  DIBTOYUVAREA far *pP,			// -> private context
  short Width,				// width of the maximum size tile
  short Height,				// height of the maximum size tile
  short DIBWidth,			// constant width of incoming DIB
  long DIBYStep,			// bytes from (x,y) to (x,y+1) in DIB
  DIBTYPE DibType,			// type of source DIB
  P8LOOKUP far *pLookup			// -> lookup if DibType == Dither8
) {
  FRAGMENT FAR * code;			// compiled code
  FRAGMENT pPF;			// pixel fetching fragment
  unsigned short idx;			// current index into code
  unsigned short startPC;
  unsigned short storedPC0;
  unsigned short storedPC1;
  unsigned short LP1;

  // set up initial data in the COMPILEDATA structure
				      // work scan width in bytes
  pP->dp.cd.workYStep = (unsigned short) (Width >> 1);
//  pP->dp.cd.oPrivate = &pP->dp;
  pP->dp.cd.oInterU2 = pP->rest;
  pP->dp.cd.oInterV2 = pP->dp.cd.oInterU2 + (pP->dp.cd.workYStep * (Height >> 1));
  pP->dp.cd.oWork = pP->dp.cd.oInterV2 + (pP->dp.cd.workYStep * (Height >> 1));
  pP->dp.cd.DIBWidth = (unsigned short) DIBWidth;
  pP->dp.cd.DIBYStep = DIBYStep;

  // set up a data pointer to the start of the area where code is to be built
  code = pP->f;
  memset(code,0,CODEMAXSIZE);

  // load the appropriate portions of the fragment code segment to the
  // private data area we have allocated
//  CopyMem(pF.pP->divBy7,pP->divBy7,sizeof(pP->divBy7));

  // set up -> pixel fetching code depending on incoming pixel depth
  switch (DibType) {
    case Dither8: {
      pPF = H1331FetchPixel8;
      pP->dp.lookUp8 = (RGBA FAR *) pLookup->palPalEntry;
      break;
    }
    case RGB555: {
      pPF = H1331FetchPixel15;
      break;
    }
    case RGB565: {
      pPF = H1331FetchPixel16;
      break;
    }
    case RGB888: {
      pPF = H1331FetchPixel24;
      break;
    }
    case RGBa8888: {
      pPF = H1331FetchPixel32;
      break;
    }
  }

  /****************************************************************************
				Begin Compilation
  ****************************************************************************/

  startPC = idx = 0;
  pP->dp.ofsDIBToYUV = idx;		// save offset of code

  code [idx++] = cEntry;		// entry code

#if 0
  startPC = pC;			// for generated code size checking

  pC = LoadFragment(pF.pF,pC,&cd,&cEntry);		// entry code
#endif

  /****************************************************************************
     Start compilation of inner looping construct which processes each scan
     of input pixels.

     Code will be generated that has the following logic
   
	entry code (push registers, load work registers, etc...)

   	fetch pixel (and convert to RGB24 if necessary)
   	convert RGB24 to YUV
   	store Y0 to detail list
   	process to begin a 4:3:1 filter sequence
   	jump to loop1
   
     loop0:
   
   	fetch pixel (and convert to RGB24 if necessary)
   	convert RGB24 to YUV
   	store Y0 to detail list
   	process to continue a 1:3:3:1 filter sequence
   
     loop1:
   
   	fetch pixel (and convert to RGB24 if necessary)
   	convert RGB24 to YUV
   	store Y1 to detail list
   	process to continue a 1:3:3:1 filter sequence
   
   	bump detail list -> to next Y pair
   	drop count of pixel pairs
   	jmp to loop0 if count != 0
   
   	process to end a 1:3:4 filter sequence

	exit code (pop registers, etc...)
  ****************************************************************************/
  {
    unsigned short startH1331;		// -> start of hfilter code
    unsigned short sizeH1331;		// size of hfilter code

    code [idx++] = H1331Init;		// one-time inits

    startH1331 = idx;			// remember start of hfilter code
    code [idx++] = H1331Start;		// top-loop inits

    code [idx++] = pPF;			// pixel fetch
    code [idx++] = H1331ToYUV;		// convert to YUV
    code [idx++] = H1331StoreY0;	// store Y0 to detail
    code [idx++] = H431;		// start 431 filter
    LP1 = idx++;			// record forward jump

    storedPC0 = idx;			// save -> start of loop0

    code [idx++] = pPF;			// pixel fetch		
    code [idx++] = H1331ToYUV;		// convert to YUV
    code [idx++] = H1331StoreY0;	// store Y0 to detail
    code [idx++] = H1331B;		// 1331B filter

    code [LP1] += JUMP + idx - LP1;	// patch forward jump

    code [idx++] = pPF;			// pixel fetch		
    code [idx++] = H1331ToYUV;		// convert to YUV
    code [idx++] = H1331StoreY1;	// store Y1 to detail
    code [idx++] = H1331C;		// 1331C filter

    code [idx++] = H1331Loop;		// end of loop code
    code [idx] = JUMP + storedPC0 - idx;	// jump destination
    idx++;

    code [idx++] = H1331End;		// 134 + cleanups

    sizeH1331 = idx - startH1331;	// size of hfilter code;

    // we need to duplicate the above code for 2nd and 3rd prefetches of scans
    // into the work buffers

    storedPC1 = idx;			// remember pc: will be loop top

    CopyFragments (code, idx, startH1331, sizeH1331);	// 2nd line prefetch
    idx += sizeH1331;

    code [idx++] = V1331EarlyOut;	// vert 134 early out
    LP1 = idx++;			// record forward jump

    CopyFragments (code, idx, startH1331, sizeH1331);	// 3rd line prefetch
    idx += sizeH1331;

    code [LP1] = JUMP + idx - LP1;	// patch forward jump
  }


  /****************************************************************************
     Compile code which vertically filters the Y,U,V data into Y2,U,V forms

     Code will be generated that has the following logic
   
	load -> Y2 smooth list (start with even pixel ->)

	load work buffer registers
     loop0:
        fetch 4 vertically aligned pixels of Y from work scans and calculate
	    a + 3b + 3c + d
	store result to smooth Y2 list
	horizontally increment pointers to work scans

	increment -> next smooth Y2 pixel

	drop pixel count
   	jmp to loop0 if count != 0

	store -> next smooth Y2 pixel for next iteration
   

	load -> U2 intermediate

	load work buffer registers
     loop1:
        fetch 4 vertically aligned pixels of U2 from work scans and calculate
	    a + 3b + 3c + d
	store result to U2 intermediate
	horizontally increment pointers to work scans

	increment -> next U2 intermediate pixel

	drop pixel count
   	jmp to loop1 if count != 0

	store -> next U2 intermediate pixel for next iteration
   

	load -> V2 intermediate

	load work buffer registers
     loop2:
        fetch 4 vertically aligned pixels of V2 from work scans and calculate
	    a + 3b + 3c + d
	store result to V2 intermediate
	horizontally increment pointers to work scans

	increment -> next V2 intermediate pixel

	drop pixel count
   	jmp to loop2 if count != 0

	store -> next V2 intermediate pixel for next iteration
  ****************************************************************************/

  code [idx++] = V1331FetchY2Dest;	// Y2 -> fetch
  storedPC0 = idx;
  code [idx++] = V1331Body;		// inner loop body
  code [idx++] = V1331IncY2Dest;	// update Y2 dest ->
  code [idx++] = V1331Loop;		// loop back to top
  code [idx] = JUMP + storedPC0 - idx;
  idx++;
  code [idx++] = V1331StoreY2Dest;

  code [idx++] = V1331FetchU2Dest;	// U2 -> fetch
  storedPC0 = idx;
  code [idx++] = V1331Body;		// inner loop body
  code [idx++] = V1331IncU2V2Dest;	// update U2 dest ->
  code [idx++] = V1331Loop;		// loop back to top
  code [idx] = JUMP + storedPC0 - idx;
  idx++;
  code [idx++] = V1331StoreU2Dest;

  code [idx++] = V1331FetchV2Dest;	// V2 -> fetch
  storedPC0 = idx;
  code [idx++] = V1331Body;		// inner loop body
  code [idx++] = V1331IncU2V2Dest;	// update V2 dest ->
  code [idx++] = V1331Loop;		// loop back to top
  code [idx] = JUMP + storedPC0 - idx;
  idx++;
  code [idx++] = V1331StoreV2Dest;

  /****************************************************************************
   Load fragment which jumps back to top of horizontal,vertical filter code
  ****************************************************************************/

  code [idx++] = HVLoop;		// loop to top...
  code [idx] = JUMP + storedPC1 - idx;
  idx++;

  /****************************************************************************
   At this point all scans of the original input have been processed:

   	- original Y components exist in the detail VECTOR list
	- filtered Y components exist in the smooth VECTOR list
	- filtered U,V components are being held in the quarter-size
	  U,V intermediate buffers

   We now treat the U,V intermediate buffers as though they contained
   the original incoming pixels. We have to handle one less component since
   the filtered Y components have already been processed.

   Load fragments which process the U,V intermediates producing U2,V2 pixels
   in the smooth list as well as U,V pixels in the detail list
  ****************************************************************************/

  /****************************************************************************
     Start compilation of inner looping construct which processes each scan
     of input pixels.

     Code will be generated that has the following logic
   
	entry code (push registers, load work registers, etc...)

   	fetch once filtered U,V and bump
   	store U,V to detail VECTOR list and bump
   	process to begin a 4:3:1 filter sequence
   	jump to loop1
   
     loop0:
   
   	fetch once filtered U,V and bump
   	store U,V to detail VECTOR list and bump
   	process to continue a 1:3:3:1 filter sequence
   
     loop1:
   
   	fetch once filtered U,V and bump
   	store odd U,V to detail VECTOR list and bump
   	process to continue a 1:3:3:1 filter sequence
   
   	drop count of pixels
   	jmp to loop0 if count != 0
   
   	process to end a 1:3:4 filter sequence

	exit code (cleanups, etc...)
  ****************************************************************************/

  {
    unsigned short startU2V2H;		// -> start of hfilter code
    unsigned short sizeU2V2H;		// size of hfilter code

    code [idx++] = U2V2HInit;		// one-time inits
    startU2V2H = idx;			// remember start of hfilter code

    code [idx++] = U2V2HStart;		// top-loop inits
    code [idx++] = U2V2H431;		// start 431 filter
    LP1 = idx++;			// record forward jump

    storedPC0 = idx;			// save -> loop 0
    code [idx++] = U2V2H1331B;		// 1331B filter
    code [LP1] = JUMP + idx - LP1;	// fixup jump
    code [idx++] = U2V2H1331C;		// 1331C filter
    code [idx++] = U2V2HLoop;		// end of loop code
    code [idx] = JUMP + storedPC0 - idx;
    idx++;
    code [idx++] = U2V2HEnd;		// 134 + cleanups

    sizeU2V2H = idx - startU2V2H;	// size of hfilter code

    // we need to duplicate the above code for 2nd and 3rd prefetches of scans
    // into the work buffers

    storedPC1 = idx;			// remember pc: will be loop top

    CopyFragments (code, idx, startU2V2H, sizeU2V2H);	// 2nd line prefetch
    idx += sizeU2V2H;

    code [idx++] = V1331EarlyOut;	// vert 134 early out
    LP1 = idx++;			// record forward jump;

    CopyFragments (code, idx, startU2V2H, sizeU2V2H);	// 3rd line prefetch
    idx += sizeU2V2H;

    code [LP1] = JUMP + idx - LP1;	// fixup forward jump

    // insert code which does vertical filtering on the contents of the
    // work buffers

    code [idx++] = U2V2VBody;		// vertical code

    // finally loop back up to horizontal filtering code from the
    // intermediate buffers into the work buffers.

    code [idx++] = HVLoop;		// loop to PC1
    code [idx] = JUMP + storedPC1 - idx;
    idx++;
  }

  /****************************************************************************
   Load fragment which pops used registers and returns from compiled code
  ****************************************************************************/

  code [idx++] = cExit;


  /****************************************************************************
   ****************************************************************************
	Begin compilation of code to recreate smooth vectors from the
	detail vectors
   ****************************************************************************
   ****************************************************************************/

  // remember -> to this function

  pP->dp.ofsRecreate = idx;

  code [idx++] = cEntry;		// entry code

  /****************************************************************************
     Start compilation of inner looping construct which processes each scan
     of detail Y's.

     Code will be generated that has the following logic
   
	entry code (push registers, load work registers, etc...)

   	fetch detail Y
   	process to begin a 4:3:1 filter sequence
   	jump to loop1
   
     loop0:
   
   	fetch detail Y
   	process to continue a 1:3:3:1 filter sequence
   
     loop1:
   
   	fetch detail Y
   	process to continue a 1:3:3:1 filter sequence
   
   	bump detail list -> to next Y pair
   	drop count of pixel pairs
   	jmp to loop0 if count != 0
   
   	process to end a 1:3:4 filter sequence

	exit code (pop registers, etc...)
  ****************************************************************************/
  {
    unsigned short startH1331;		// -> start of hfilter code
    unsigned short sizeH1331;		// size of hfilter code

    code [idx++] = rY_H1331Init;	// one-time inits

    startH1331 = idx;			// remember start of hfilter code
    code [idx++] = rY_H1331Start;	// top-loop inits

    code [idx++] = rY_H431;		// start is 431 filter
    LP1 = idx++;

    storedPC0 = idx;			// save -> loop0

    code [idx++] = rY_H1331B;		// 1331B filter
    code [LP1] = JUMP + idx - LP1;	// fixup jump into loop

    code [idx++] = rY_H1331C;		// 1331C filter

    code [idx++] = H1331Loop;		// end of loop code
    code [idx] = JUMP + storedPC0 - idx;
    idx++;

    code [idx++] = rY_H1331End;	// 134 + cleanups

    sizeH1331 = idx - startH1331;	// size of hfilter code

    // we need to duplicate the above code for 2nd and 3rd prefetches of scans
    // into the work buffers

    storedPC1 = idx;			// remember pc: will be loop top

    CopyFragments (code, idx, startH1331, sizeH1331);	// 2nd line prefetch
    idx += sizeH1331;

    code [idx++] = V1331EarlyOut;	// vert 134 early out
    LP1 = idx++;			// record forward jump

    CopyFragments (code, idx, startH1331, sizeH1331);	// 3rd line prefetch
    idx += sizeH1331;

    code [LP1] = JUMP + idx - LP1;	// fix early jump out

  }

  /****************************************************************************
     Compile code which vertically filters the Y data into Y2 forms

     Code will be generated that has the following logic
   
	load -> Y2 smooth list (start with even pixel ->)

	load work buffer registers
     loop0:
        fetch 4 vertically aligned pixels of Y from work scans and calculate
	    a + 3b + 3c + d
	store result to smooth Y2 list
	horizontally increment pointers to work scans

	increment -> next smooth Y2 pixel

	drop pixel count
   	jmp to loop0 if count != 0

	store -> next smooth Y2 pixel for next iteration
  ****************************************************************************/

  code [idx++] = rY_V1331Body;		// inner loop body
  code [idx] = JUMP + storedPC1 - idx;
  idx++;

  /****************************************************************************
     Start compilation of inner looping construct which processes each scan
     of once-filtered U's and V's.

     Code will be generated that has the following logic
   
	entry code (push registers, load work registers, etc...)

   	fetch once-filtered U,V from detail list
   	process to begin a 4:3:1 filter sequence
   	jump to loop1
   
     loop0:
   
   	fetch once-filtered U,V from detail list
   	process to continue a 1:3:3:1 filter sequence
   
     loop1:
   
   	fetch once-filtered U,V from detail list
   	process to continue a 1:3:3:1 filter sequence
   
   	bump detail list -> to next U,V pair
   	drop count of pixel pairs
   	jmp to loop0 if count != 0
   
   	process to end a 1:3:4 filter sequence

	exit code (pop registers, etc...)
  ****************************************************************************/
  {
    unsigned short startH1331;		// -> start of hfilter code
    unsigned short sizeH1331;		// size of hfilter code

    code [idx++] = rUV_H1331Init;	// one-time inits

    startH1331 = idx;			// remember start of hfilter code
    code [idx++] = rUV_H1331Start;	// top-loop inits

    code [idx++] = rUV_H431;		// start is 431 filter
    LP1 = idx++;			// record forward jump

    storedPC0 = idx;			// save -> loop0

    code [idx++] = rUV_H1331B;		// 1331B filter

    code [LP1] = JUMP + idx - LP1;	// fixup jump into loop
    code [idx++] = rUV_H1331C;		// 1331C filter

    code [idx++] = H1331Loop;		// end of loop code
    code [idx] = JUMP + storedPC0 - idx;
    idx++;

    code [idx++] = rUV_H1331End;	// 134 + cleanups

    sizeH1331 = idx - startH1331;		// size of hfilter code

    // we need to duplicate the above code for 2nd and 3rd prefetches of scans
    // into the work buffers

    storedPC1 = idx;			// remember pc: will be loop top

    CopyFragments (code, idx, startH1331, sizeH1331);	// 2nd line prefetch
    idx += sizeH1331;

    code [idx++] = V1331EarlyOut;	// vert 134 early out
    LP1 = idx++;			// record forward jump

    CopyFragments (code, idx, startH1331, sizeH1331);	// 3rd line prefetch
    idx += sizeH1331;

    code [LP1] = JUMP + idx - LP1;	// fix early jump out
  }

  /****************************************************************************
     Compile code which vertically filters the U,V data into twice-filtered
     forms.

     Code will be generated that has the following logic:
   
	load -> UV smooth list (start with even pixel ->)

	load work buffer registers
     loop0:
        fetch 4 vertically aligned pixels of U,V from work scans and calculate
	    a + 3b + 3c + d
	store result to smooth U,V list
	horizontally increment pointers to work scans

	increment -> next smooth U,V pixel

	drop pixel count
   	jmp to loop0 if count != 0

	store -> next smooth U,V pixel for next iteration
  ****************************************************************************/

  code [idx++] = rUV_V1331Body;		// inner loop body
  code [idx] = JUMP + storedPC1 - idx;
  idx++;

  /****************************************************************************
   Load fragment which pops used registers and returns from compiled code
  ****************************************************************************/

  code [idx++] = cExit;

  /****************************************************************************
				End Compilation
  ****************************************************************************/

#if	!defined(NOASM)
#if	((defined(DEBUG) || defined(DBG)) && defined(STATISTICS)) || defined(STATISTICS_ALWAYS)
  if (((unsigned short) (idx - startPC)) > genCodeSize) {
    genCodeSize = (idx - startPC);// latch larger codesize...
  }
  {
   TCHAR foo[128];
#if	!defined(WIN32)
   wsprintf(foo,"Code Size: %d, Code Selector: 0x%x\n", genCodeSize,
   	pP->pCode.parts.selector);
#else
   wsprintf(foo,TEXT("Code Size: %d, Code Offset: 0x%lx\n"),(long) genCodeSize,
   	(unsigned long) code);
#endif
   MessageBox(NULL,foo,TEXT("DIBToYUV"),MB_OK);
  }
#endif
#endif

  // return success if our compiled code did not exceed the amount of
  // space that we had presumed would be needed for it

  return (( (idx - startPC)) <= CODEMAXSIZE);
}



/**********************************************************************
 *
 * DIBToYUVBegin()
 *
 * Initializations for ensuing calls to DIBToYUV()
 *
 **********************************************************************/

int DIBToYUVBegin(
  CCONTEXT *pC,				// compression context
  void far *pLookup			// -> 8 bpp lookup table
) {
  DIBTOYUVAREA FAR * pP;
  unsigned long sizePrivate;		// size of private area

  // allocate space that we'll need for the following:
  //
  //	data for compiled code	--> size is sizeof(DIBTOYUVPRIVATE)
  //	compiled code		--> reserve CODEMAXSIZE bytes
  //	YUV intermediates	--> max size dependent on width,height
  //				    of the max size tile
  //	work buffers		--> room for 4 Y2, 4 U, 4 V scans

  sizePrivate = sizeof(DIBTOYUVAREA) + 
	        (unsigned long) ( // 2 quarter-size spots for U,V
		 		 (unsigned long) (pC->FrameWidth >> 1) *
		 		 (unsigned) pC->pT[0]->Height
				) +
		(pC->FrameWidth * 6);	// space for 4 half-width scans of YUV
  if (pP = (DIBTOYUVAREA far *) GlobalAllocPtr (
						 GMEM_MOVEABLE,
						 sizePrivate
						)
     ) {

#if	!defined(WIN32)
    //
    // make sure the private area cannot move in linear memory (this we must
    // do since we are going to create a code alias for this memory)
    //
    GlobalFix(SELECTOROF(pP));
#endif

    // compile the code based on input parameters
    if (!CompileFragments(
    		          pP,		// -> private context
		          pC->FrameWidth,// width of the maximum width tile
		          pC->pT[0]->Height,// height of the maximum size tile
		          pC->DIBWidth,	// width of incoming DIB
		          pC->DIBYStep,	// bytes from (x,y) to (x,y+1) in DIB
		          pC->DIBType,	// type of incoming DIB
		          pLookup	// -> lookup table for 8 bpp
		         )
       ) { // some error during compilation...
      goto fail_begin_1;
    }

    // remember -> to our private context in the global compression context
    pC->pDIBToYUVPrivate = pP;

#if	!defined(NOASM)
#if	defined(DEBUG) || defined(DBG)
  pP->dp.bitsYStep = pC->DIBYStep;
#if	!defined(WIN32)
  pP->dp.privBase = GetSelectorBase(SELECTOROF(pP.pP)) +
  	sizeof(DIBTOYUVPRIVATE) + CODEMAXSIZE;
#else
  pP->dp.privBase = (unsigned long) &pP->rest;
#endif
  pP->dp.privLimit = pP->dp.privBase + (sizePrivate - CODEMAXSIZE -
   	sizeof(DIBTOYUVPRIVATE)) - 1;
#endif
#endif
  }

  return (1);				// success

  //
  // We arrive here for all failure conditions from this function
  //

fail_begin_1:

#if	!defined(WIN32)
  // failure: clean up memory allocated for private use

  GlobalUnfix(SELECTOROF(pP));
#endif
  GlobalFreePtr(pP);

  return (0);				// failure
}


/**********************************************************************
 *
 * DIBToYUV()
 *
 * Make the intermediate YUV bitmaps from the incoming RGB DIB
 *
 **********************************************************************/

void DIBToYUV(
	CCONTEXT *pC,				// compression context
	TILECONTEXT *pT,			// current tile context
	unsigned long oBits			// 32 bit offset of base of input tile
#if	!defined(WIN32)
	,unsigned short sBits			// selector to input tile
#endif
)
{
	DIBTOYUVAREA far *pP;			// -> our private context
	
	/*
	  Transform tile of incoming DIB to internal YUV form.
	
	  Let w be the width of the tile rounded up to the next 0 mod 4
	  boundary, and h be the height of the tile similarly rounded.
	
	  Then the internal YUV form consists of:
	
	  1.  Y, a full size w*h bitmap of
	 Y[i] = ((4*G[i] + 2*R[i] + B[i]) + 3.5) / 7, range 0..255
	
	  2.  U, a 1/4 size (w/2)*(h/2) bitmap of U[i], range 0..255, filtered
	 with 431,1331,...,1331,134 in x and y from
	 u[i] = 128 + ((B[i] - Y[i]) / 2)
	
	  3.  V, a 1/4 size (w/2)*(h/2) bitmap of V[i], range 0..255, filtered
	 with 431,1331,...,1331,134 in x and y from
	 v[i] = 128 + ((R[i] - Y[i]) / 2)
	
	  4.  Y2, a 1/4 size (w/2)*(h/2) bitmap of Y2[i], range 0..255,
	 filtered with 431,1331,...,1331,134 from Y[i].
	
	  5.  U2, a 1/16 size (w/4)*(h/4) bitmap of U2[i], range 0..255,
	 filtered with 431,1331,...,1331,134 from U[i].
	
	  6.  V2, a 1/16 size (w/4)*(h/4) bitmap of V2[i], range 0..255,
	 filtered with 431,1331,...,1331,134 from V[i].
	
	  The 431,1331,...,1331,431 filtering means that given a scanline of
	  input i0,i1,i2,i3,i4,i5,i6,i7 we produce an output of o0,o1,o2,o3
	  where
	    o0 = 1*i0 + 3*i0 + 3*i1 + 1*i2 = 4*i0 + 3*i1 + 1*i2
	    o1 = 1*i1 + 3*i2 + 3*i3 + 1*i4
	    o2 = 1*i3 + 3*i4 + 3*i5 + 1*i6
	    o3 = 1*i5 + 3*i6 + 3*i7 + 1*i7 = 1*i5 + 3*i6 + 4*i7
	  with similar filtering done in y.
	
	  We remember #1, #2 and #3 above as an array of VECTOR elements,
	  called pC->VBook[Detail].pVectors, and #4, #5 and #6 as a similar
	  array called pC->VBook[Smooth].pVectors.
	
	  Each 2x2 patch of incoming DIB thus comprises a 6 element detail
	  vector, with 4 luminance values and 2 chroma values.  Each 4x4 patch
	  of incoming DIB comprises a 6 element smooth vector, also with 4
	  luma and 2 chroma.  Whether a given 4x4 patch is treated as 4 2x2
	  detail vectors or 1 4x4 smooth vector is decided later.
	
	  !!!
	    One VECTOR area is allocated per tile (for the prev tile) plus one
	    for the current tile at CompressBegin time.  They are shuffled
	    around as new tiles come in, so each tile has a prev when needed.
	  !!!
	 */
	
	/*
	  !!!
	    note that the internal form is an array of VECTOR but could be
	    built as separated arrays of Y then U then V as long as they are
	    shuffled before return
	  !!!
	 */
	
	// get a local -> to the private context
	pP = pC->pDIBToYUVPrivate;
	
	//
	// load up private data area with parameters needed for compiled code
	//
	
	pP->dp.tileHeight = pT->Height;		// 0 mod 4 height of this tile
	pP->dp.srcHeight = (unsigned) pT->DIBHeight - 1;// real height of tile - 1
	
						// flat -> incoming pixels
#if	!defined(WIN32)
	pP->dp.oBits = MAKELP (sBits, oBits);
#else
	pP->dp.oBits = (BYTE FAR *) oBits;
#endif
#if	!defined(NOASM) && (defined(DEBUG) || defined(DBG))
  if (pP->dp.bitsYStep < 0) {
    pP->dp.bitsLimit = (DWORD) pP->dp.oBits - pP->dp.bitsYStep - 1;
    pP->dp.bitsBase = (DWORD) pP->dp.oBits + (pP->dp.srcHeight * pP->dp.bitsYStep);
  }
  else {
    pP->dp.bitsBase = (DWORD) pP->dp.oBits;
    pP->dp.bitsLimit = (DWORD) pP->dp.oBits + ((pP->dp.srcHeight + 1) * pP->dp.bitsYStep) - 1;
  }
#endif

	// ->s to detail VECTOR parts
	pP->dp.oDetail = (LPBYTE) pC->VBook[Detail].pVectors;
#if	!defined(NOASM) && (defined(DEBUG) || defined(DBG))
  pP->dp.detailBase = (DWORD) pP->dp.oDetail;
  pP->dp.detailLimit = (DWORD) pP->dp.detailBase + (
  				      (
				       (unsigned long) pT->nPatches *
				       sizeof(VECTOR)
				      ) << 2
				     ) - 1;
#endif

	// flat ->s to smooth VECTOR parts
	pP->dp.oSmooth = (LPBYTE) pC->VBook[Smooth].pVectors;
#if	!defined(NOASM) && (defined(DEBUG) || defined(DBG))
  pP->dp.smoothBase = (DWORD) pP->dp.oSmooth;
  pP->dp.smoothLimit = (DWORD) pP->dp.smoothBase + (pT->nPatches * sizeof(VECTOR)) - 1;
#endif

	/*
	  Invoke the bottom-level do'er
	
	  All additional runtime parameters are initialized in the entry code of
	  the called code
	*/
	
	ExecuteFragments(&pP->dp,pP->f + pP->dp.ofsDIBToYUV);

	return;
}


/**********************************************************************
 *
 * RecreateSmoothFromDetail()
 *
 * Recreate the smooth vector list from the detail vector list
 *
 **********************************************************************/

void RecreateSmoothFromDetail(
  CCONTEXT *pC,				// compression context
  TILECONTEXT *pT			// current tile context
) {
  DIBTOYUVAREA far *pP;			// -> our private context

  // get a local -> to the private context
  pP = pC->pDIBToYUVPrivate;

  //
  // load up private data area with parameters needed for compiled code
  //

  pP->dp.tileHeight = pT->Height;	// 0 mod 4 height of this tile

  					// ->s to detail VECTOR parts
  pP->dp.oDetail = (LPBYTE) pC->VBook[Detail].pVectors;
#if	!defined(NOASM) && (defined(DEBUG) || defined(DBG))
  pP->dp.detailBase = (DWORD) pP->dp.oDetail;
  pP->dp.detailLimit = (DWORD) pP->dp.detailBase + (
  				      (
				       (unsigned long) pT->nPatches *
				       sizeof(VECTOR)
				      ) << 2
				     ) - 1;
#endif

  					// ->s to smooth VECTOR parts
  pP->dp.oSmooth = (LPBYTE) pC->VBook[Smooth].pVectors;
#if	!defined(NOASM) && (defined(DEBUG) || defined(DBG))
  pP->dp.smoothBase = (DWORD) pP->dp.oSmooth;
  pP->dp.smoothLimit = (DWORD) pP->dp.smoothBase + (pT->nPatches * sizeof(VECTOR)) - 1;
#endif

  /*
    Invoke the bottom-level do'er

    All additional runtime parameters are initialized in the entry code of
    the called code
  */
#if	!defined(NOASM) && (defined(DEBUG) || defined(DBG))
  {
    unsigned long css = 0L;
    int i;
    VECTOR far *pv;
    TCHAR msg[64];
    
    for (
         pv = pC->VBook[Smooth].pVectors,
	   i = ((pC->FrameWidth >> 2) * (pT->Height >> 2));
	 i--;
	 pv++
        ) {
      css += (unsigned short) pv->yuv.y[0] + pv->yuv.y[1] + pv->yuv.y[2] +
      	     pv->yuv.y[3] + pv->yuv.u + pv->yuv.v;
    }
    wsprintf(msg,TEXT("Smooth checksum before = %lX"),css);
    MessageBox(NULL,msg,TEXT("RecreateSmoothFromDetail"),MB_OK);
  }
#endif
  ExecuteFragments (
      &pP->dp,
      pP->f + pP->dp.ofsRecreate
  );
#if	!defined(NOASM) && (defined(DEBUG) || defined(DBG))
  {
    unsigned long css = 0L;
    int i;
    VECTOR far *pv;
    TCHAR msg[64];
    
    for (
         pv = pC->VBook[Smooth].pVectors,
	   i = ((pC->FrameWidth >> 2) * (pT->Height >> 2));
	 i--;
	 pv++
        ) {
      css += (unsigned short) pv->yuv.y[0] + pv->yuv.y[1] + pv->yuv.y[2] +
      	     pv->yuv.y[3] + pv->yuv.u + pv->yuv.v;
    }
    wsprintf(msg,TEXT("Smooth checksum after = %lX"),css);
    MessageBox(NULL,msg,TEXT("RecreateSmoothFromDetail"),MB_OK);
  }
#endif

  return;
}


/**********************************************************************
 *
 * DIBToYUVEnd()
 *
 * Clean up after previous call to DIBToYUVBegin()
 *
 **********************************************************************/

void DIBToYUVEnd(
  CCONTEXT *pC				// compression context
) {
  DIBTOYUVAREA far *pP;		// -> our private context

  if (pP = pC->pDIBToYUVPrivate) { // private context was allocated...

    // free up space allocated to the private context
#if	!defined(WIN32)
    GlobalUnfix(SELECTOROF(pP));
#endif
    GlobalFreePtr(pP);

    pC->pDIBToYUVPrivate = (void far *) 0;// flag uninitialized
  }
}
