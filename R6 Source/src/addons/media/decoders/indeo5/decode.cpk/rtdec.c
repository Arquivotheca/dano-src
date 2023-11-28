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
 * RTDEC.C -- Main Module for decoding Indeo 5 Bitstreams.
 *************************************************************************/
/*/#define FORCE_FULL_DECODE*/
#define NEW_SCALABILITY

#ifdef DEBUG
#include <windows.h>
#endif /* DEBUG */
#include "datatype.h"
#include "pia_main.h"
#include "cpk_blk.h"
#include "xpardec.h"
#include "rtdec.h"
#include "hivedec.h"

#include "decpl.h"
#include "decbndh.h"
#include "dectile.h"
#include "gcompasm.h"
#include "readbits.h"

/* ********************************************************************* */

#ifdef DEBUG
#if 0
#define EXTERN		extern
#define ZEROINIT	
#else /* 0 */
#define EXTERN		
#define ZEROINIT	= 0
#endif /* 0 */

U32 uNumInstances = 0;
extern U32 uDemoScale ZEROINIT;
extern U32 uDemoFrame ZEROINIT;
extern Boo bFrameCheckSum ZEROINIT;
extern Boo bBandCheckSum ZEROINIT;
extern Boo bDebugScalability ZEROINIT;
extern Boo bPentimeTiming ZEROINIT;
extern Boo bPrintBandPercents ZEROINIT;
extern Boo bPrintTileInfo ZEROINIT;
extern Boo bPrintBandInfo ZEROINIT;
extern Boo bCompareCCOutput ZEROINIT;
extern Boo bForceAltLine ZEROINIT;
extern Boo bDisplayMode ZEROINIT;
extern Boo uBSVersion ZEROINIT;
static char inif[] = "ivi5.ini";
#else /* DEBUG */

#ifdef FORCE_FULL_DECODE
U32	uDemoScale = 4;
U32	uDemoFrame = 5;
#endif /* FORCE_FULL_DECODE */

#endif /* DEBUG */

/* ********************************************************************* */

#if defined(__MEANTIME__) && !defined(C_PORT_KIT)
#include "meantime.h"
#ifdef __MEANTIME_RTDEC__
#define STOPCLOCKANDSAVE(x)	STOPCLOCK				\
	if(iTimingFrame != -1)							\
		frametimes[iTimingFrame].x = elapsed;
#define STOPCLOCKANDACCUMULATE(x)	STOPCLOCK		\
	if(iTimingFrame != -1)							\
		frametimes[iTimingFrame].x += elapsed;
#define ZEROCLOCK(x)								\
	if(iTimingFrame != -1)							\
		frametimes[iTimingFrame].x = 0;

#else /* __MEANTIME_RTDEC__ */
#undef STARTCLOCK
#define STARTCLOCK
#define STOPCLOCKANDSAVE(x)
#define STOPCLOCKANDACCUMULATE(x)
#define ZEROCLOCK(x)
#endif /* __MEANTIME_RTDEC__ */
#else /* __MEANTIME__ */
#undef STARTCLOCK
#define STARTCLOCK
#define STOPCLOCKANDSAVE(x)
#define STOPCLOCKANDACCUMULATE(x)
#define ZEROCLOCK(x)
#endif /* defined(__MEANTIME__) && ! defined(C_PORT_KIT) */

/* ********************************************************************* */

#ifdef  DEBUG
#include <stdio.h>
#pragma data_seg(".sdata")	/* following is data shared among instances */
FILE * timefile = NULL;
#pragma data_seg()	/* end shared data */
/*extern FILE * timefile; */
#else
#endif

#ifdef DEBUG
__inline void FrameCheckSum(pRTDecInst pCntx);
__inline void FrameCheckSum(pRTDecInst pCntx) {
	U32 r, c, plane;
	U32 nrows, ncols;
	PU8 brptr, bcptr;
	U32 xsum = 0;

	for (plane = 0; plane < 3; plane++) {
		pRtPlaneSt p;

		p = &pCntx->Plane[plane];

		ncols = plane ? pCntx->tVUFrameSize.c : pCntx->tFrameSize.c;
		nrows = plane ? pCntx->tVUFrameSize.r : pCntx->tFrameSize.r;

		brptr = p->pOutput;
		for (r = 0; r < nrows; r++) {
			bcptr = brptr;
			for (c = 0; c < ncols; c++) {
				xsum += *bcptr++;
			}
			brptr += p->uOutputPitch;
		}
		xsum &= 0xffff;
	}
	if (xsum != pCntx->uFrameCheckSum) {
#ifndef QT_FOR_MAC
					__asm nop;
#else
					HivePrintF("Whoa\n");
#endif
	}
}
#endif

#ifdef DEBUG
static void ret_err(U32 err) {
	switch (err) {
		case 0:
			HivePrintF("Decoder: Interface supplied Image Dimensions don't match bitstream");
			break;
		case 1:
			HivePrintF("Decoder: skipped band error");
			break;
		case 2:
			HivePrintF("Decoder: dectile failed");
			break;
		case 3:
			HivePrintF("Decoder: Bad Bitstream - framesize doesn't match bytes read");
			break;
		case 4:
		case 5:
		case 6:
			HivePrintF("Decoder: Local Decode - null pointer");
			break;
	}
}
#define RET_ERR(x,y) ret_err(x); (y)

#else /* DEBUG */
#define RET_ERR(x,y) (y)
#endif /* DEBUG */

/* ********************************************************************* */

/*	Describe the 8 static PB Style MacroBlock Huffman
 *	Tables.  Each table is described by a count, followed
 *	by count suffix bit indicators for each corresponding
 *	prefix.  The last prefix group has the 0 bit ommitted,
 *	in the following form
 *		0	prefix 0
 *		10	prefix 1
 *		110	prefix 2
 *		111	prefix 3 (no 0 in prefix bits)
 */

/* This is a set of PB style descriptor tables to be used as static
   table representations.  During initialization time the PB tables
   defined below in the PBHuffTblSt arrays are converted to generic
   Huffman tables and put in the HuffTblSt arrays.  The generic
   Huffman table represention is what the encoder uses.
*/

/* Macro block PB Huffman tables */
const PBDESCRIPTOR_TYPE MBStaticHuff[8][17] = {
	{ 8, 0, 4, 5, 4, 4, 4, 6, 6,},
	{12, 0, 2, 2, 3, 3, 3, 3, 5, 3, 2, 2, 2,},
	{12, 0, 2, 3, 4, 3, 3, 3, 3, 4, 3, 2, 2,},
	{12, 0, 3, 4, 4, 3, 3, 3, 3, 3, 2, 2, 2,},
	{13, 0, 4, 4, 3, 3, 3, 3, 2, 3, 3, 2, 1, 1,},
	{ 9, 0, 4, 4, 4, 4, 3, 3, 3, 2,},
	{10, 0, 4, 4, 4, 4, 3, 3, 2, 2, 2,},
	{12, 0, 4, 4, 4, 3, 3, 2, 3, 2, 2, 2, 2,},
};

/* Block tables */
const PBDESCRIPTOR_TYPE BlockStaticHuff[9][17] = {
	/* Global block Huffman table set available for all bands */
	{10, 1, 2, 3, 4, 4, 7, 5, 5, 4, 1},
	{11, 2, 3, 4, 4, 4, 7, 5, 4, 3, 3, 2,},
	{12, 2, 4, 5, 5, 5, 5, 6, 4, 4, 3, 1, 1,},
	{13, 3, 3, 4, 4, 5, 6, 6, 4, 4, 3, 2, 1, 1,},
	{11, 3, 4, 4, 5, 5, 5, 6, 5, 4, 2, 2,},
	{13, 3, 4, 5, 5, 5, 5, 6, 4, 3, 3, 2, 1, 1,},
	{13, 3, 4, 5, 5, 5, 6, 5, 4, 3, 3, 2, 1, 1,},
	/* Default Huffman table */
	{ 9, 3, 4, 4, 5, 5, 5, 6, 5, 5,},
};

static bDidDecompressBegin = FALSE;

/* ********************************************************************* */

#ifdef NEW_SCALABILITY
__inline void ScalabilityBegin(
	pScaleCntxSt	pSCntx);
#endif /* NEW_SCALABILITY */

pRTDecInst RTDecompressBegin() {
	pRTDecInst t;
	I32 i;

	readbitsstartup();

	if (!bDidDecompressBegin) {
		ComposeBegin();
		bDidDecompressBegin = TRUE;
	}

/*	Memory must be zero'd */
	t = (pRTDecInst)HiveLocalAllocPtr(sizeof(RTDecInst), TRUE);

	if (!t) {
		goto end;
	}

	t->u8FrameNumber = (U8)-1;

/*	Set up the static Macroblock Huffman tables */
	for (i = 0; i < 8; i++) {
		pHuffTabSt h;
		h = &t->MBHuffTabList[i];
		h->maintable = h->mainstatic;
		h->descriptor = (PBDESCRIPTOR_TYPE *)(&MBStaticHuff[i][0]);
		if (BuildPBHuffTab(h)) {
			HiveLocalFreePtr(t);
			t = NULL;
			goto end;
		}
	}
/*	Initialize the dynamic MB Huffman table with arbitrary but valid data */
	t->MBHuffTabList[8].descriptor = &t->MBHuffTab8Descriptor[0];
	t->MBHuffTabList[9].descriptor = &t->MBHuffTab9Descriptor[0];
	t->MBHuffTabList[10].descriptor = &t->MBHuffTab10Descriptor[0];
	t->MBHuffTabList[11].descriptor = &t->MBHuffTab11Descriptor[0];
	t->MBHuffTabList[12].descriptor = &t->MBHuffTab12Descriptor[0];

	for (i = 0; i <= MBStaticHuff[0][0]; i++) {
		t->MBHuffTabList[8].descriptor[i] =
		t->MBHuffTabList[9].descriptor[i] = 
		t->MBHuffTabList[10].descriptor[i] =
		t->MBHuffTabList[11].descriptor[i] = 
		t->MBHuffTabList[12].descriptor[i] = MBStaticHuff[0][i];
	}
	t->MBHuffTabList[8].maintable = t->MBHuffTabList[8].mainstatic;
	t->MBHuffTabList[9].maintable = t->MBHuffTabList[9].mainstatic;
	t->MBHuffTabList[10].maintable = t->MBHuffTabList[10].mainstatic;
	t->MBHuffTabList[11].maintable = t->MBHuffTabList[11].mainstatic;
	t->MBHuffTabList[12].maintable = t->MBHuffTabList[12].mainstatic;
	BuildPBHuffTab(&t->MBHuffTabList[8]);
	BuildPBHuffTab(&t->MBHuffTabList[9]);
	BuildPBHuffTab(&t->MBHuffTabList[10]);
	BuildPBHuffTab(&t->MBHuffTabList[11]);
	BuildPBHuffTab(&t->MBHuffTabList[12]);

/*	Now, init scalability info */
	ScalabilityBegin(&t->ScaleCntx);

	t->puLumaTable = NULL;
	t->puChromaTable = NULL;
	t->puBrightTable = NULL;
	t->puContrastTable = NULL;

end:
#ifdef DEBUG
#ifndef QT_FOR_MAC
	uDemoScale = GetPrivateProfileInt("scalability", "DemoScale", 0, inif);
	uDemoFrame = GetPrivateProfileInt("scalability", "DemoFrame", 0, inif);
	bFrameCheckSum = GetPrivateProfileInt("debug", "FrameChecksum", 0, inif);
	bBandCheckSum = GetPrivateProfileInt("debug", "BandChecksum", 0, inif);
	bDebugScalability = GetPrivateProfileInt("debug", "DebugScalability", 0, inif);
	bPentimeTiming = GetPrivateProfileInt("perf", "PentimeTiming", 0, inif);
	bPrintBandPercents = GetPrivateProfileInt("perf", "PrintBandPercents", 0, inif);
	bPrintTileInfo = GetPrivateProfileInt("debug", "PrintTileInfo", 0, inif);
	bPrintBandInfo = GetPrivateProfileInt("debug", "PrintBandInfo", 0, inif);
	uBSVersion = GetPrivateProfileInt("debug", "BSVersion", 2, inif);
	bCompareCCOutput = GetPrivateProfileInt("cc", "CompareCCOutput", 0, inif);
	bForceAltLine = GetPrivateProfileInt("cc", "ForceAltLine", 0, inif);
	bDisplayMode = GetPrivateProfileInt("cc", "DisplayMode", 0, inif);
	if (uDemoScale || uDemoFrame || bFrameCheckSum || bBandCheckSum
		|| bDebugScalability || bPentimeTiming || bPrintBandPercents
		|| bPrintTileInfo || bPrintBandInfo || bCompareCCOutput
		|| bForceAltLine || bDisplayMode) {

		if (!timefile) {
			timefile = fopen("c:\\time.txt","a");
		}
	}
	if (t) {
#if 0
		t->uInstNum = uNumInstances++;
#else
		uNumInstances++;
#endif /*0*/

	}
#endif
#endif /* DEBUG */

	return t;
}

/* ********************************************************************* */
/*
	Frame Buffer Management:

	After Displaying a frame:

	Current <--> Forward

	Start
	DecodePictureLayer
	DecodePictureData: Fill Current with Data
	else ComposeCurrent

	DecodeFrameEnd

 */

__inline void PreDecodeSwapFrameBuffers(pRTDecInst pCntx);
__inline void PreDecodeSwapFrameBuffers(pRTDecInst pCntx) {
	PU8 t;

	pCntx->uValidCurrent = 0;

	switch (pCntx->uFrameType) {
		case PICTYPE_K:
		/*	Double Swap for consecutive key frames */
			t = pCntx->pFrameForward;
			pCntx->pFrameForward = pCntx->pFrameCurrent;
			pCntx->pFrameCurrent = t;

			pCntx->uValidForward = pCntx->uValidBackward = FALSE;
			break;

		case PICTYPE_D:
		case PICTYPE_P2:
		/*	"2nd level" swap: B<->F */
			if (pCntx->uValidBackward) {	/* if gotap2 */
				t = pCntx->pFrameForward;
				pCntx->pFrameForward = pCntx->pFrameBackward;
				pCntx->pFrameBackward = t;
			}
			break;

		case PICTYPE_P:
			pCntx->uValidBackward = FALSE;
			break;

		case PICTYPE_R:
		case PICTYPE_V:
/*			if (/ *stepping &&* / !pCntx->uValidDisplay) */
/*				bFast = TRUE;*/	/* don't display R's in playback */
			break;

	}

} /* PreDecodeSwapFrameBuffers() */

__inline void PostDecodeSwapFrameBuffers(pRTDecInst pCntx);
__inline void PostDecodeSwapFrameBuffers(pRTDecInst pCntx) {
	PU8 t;

	switch (pCntx->uFrameType) {
		case PICTYPE_P2:
			pCntx->pFrameDisplay = pCntx->pFrameCurrent;
			pCntx->uValidDisplay = pCntx->uValidCurrent;

		/*	"2nd level" swap: B<->F */
			if (pCntx->uValidBackward) {	/* if gotap2 */
				t = pCntx->pFrameForward;
				pCntx->pFrameForward = pCntx->pFrameBackward;
				pCntx->pFrameBackward = t;
			}

			t = pCntx->pFrameCurrent;
			pCntx->pFrameCurrent = pCntx->pFrameBackward;
			pCntx->pFrameBackward = t;

			pCntx->uValidBackward = pCntx->uValidCurrent;
			pCntx->uValidCurrent = FALSE;
			break;

		case PICTYPE_K:
		case PICTYPE_P:
			pCntx->pFrameDisplay = pCntx->pFrameCurrent;
			pCntx->uValidDisplay = pCntx->uValidCurrent;

		/*	"Normal" swap: C<->F */
			t = pCntx->pFrameForward;
			pCntx->pFrameForward = pCntx->pFrameCurrent;
			pCntx->pFrameCurrent = t;

			pCntx->uValidForward = pCntx->uValidCurrent;
			pCntx->uValidCurrent = FALSE;
			break;

		case PICTYPE_D:
			pCntx->pFrameDisplay = pCntx->pFrameCurrent;
			pCntx->uValidDisplay = pCntx->uValidCurrent;

		/*	"2nd level" swap: B<->F */
			if (pCntx->uValidBackward) {	/* if gotap2 */
				t = pCntx->pFrameForward;
				pCntx->pFrameForward = pCntx->pFrameBackward;
				pCntx->pFrameBackward = t;
			}
			break;

		case PICTYPE_R:
		case PICTYPE_V:
			break;
	}

} /* PostDecodeSwapFrameBuffers() */

#define RECT_INTERSECT(R, r1, r2)											\
	(R.r) = ((r1.r) < (r2.r)) ? (r2.r) : (r1.r);							\
	(R.c) = ((r1.c) < (r2.c)) ? (r2.c) : (r1.c);							\
	(R.h) = ((r1.r+r1.h) > (r2.r+r2.h)) ? (r2.r+r2.h-R.r) : (r1.r+r1.h-R.r);\
	(R.w) = ((r1.c+r1.w) > (r2.c+r2.w)) ? (r2.c+r2.w-R.c) : (r1.c+r1.w-R.c);\
	(R.h) = ((I32)(R.h) > 0) ? (R.h) : 1;										\
	(R.w) = ((I32)(R.w) > 0) ? (R.w) : 1;

#define IS_ALIGNED(R)											\
	!(	(((R.r) | (R.h)) & 0x03) ||								\
		(((R.c) | (R.w)) & 0x1f)	)

#define IS_ALIGNED2(R,image)									\
	!(	((R.r) & 0x03) ||										\
		((R.c) & 0x1f) ||										\
		((R.h) - image.h) ||									\
		((R.w) - image.w)										\
	)

#define SET_CC_RECT(R,cc,image)									\
	cc->r = R.r & ~0x3;											\
	cc->c = R.c & ~0x1f;										\
	if (((U32)R.w == image.w) && ((U32)R.h == image.h)) {		\
		cc->w = image.w;										\
		cc->h = image.h;										\
	}															\
	else {														\
		cc->h = (R.r + R.h + 0x3) & ~0x3;						\
		cc->w = (R.c + R.w + 0x1f) & ~0x1f;						\
		cc->h -= cc->r;											\
		cc->w -= cc->c;											\
	}

/* ********************************************************************* */

#ifdef NEW_SCALABILITY

#define WEIGHTED_AVG(var, addend, num, denom) \
	{ (var) *= (num); (var) += (addend) + (num); (var) /= (denom); }

#define WEIGHTED_AVG2(var, addend, num, denom) \
	if ((var) == -1) (var) = (addend) - 1;			\
	else { (var) *= (num); (var) += (addend) + (num); (var) /= (denom); }

#define GET_TIME(start, end, target) \
	((target) ? /*100-*/(((end) - (start)) * 100 / (target)) : /*10*/0)

/*	**** */

__inline void ScalabilityBegin(
	pScaleCntxSt	pSCntx) {

	U32 i;

	pSCntx->iPercentFramesDecoded = 100;
	pSCntx->iPercentFramesDisplayed = 100;

	pSCntx->iStartClock = -1;			/* temp time keeper */
	pSCntx->iAverageDecodePercent = -1;	/* time doing band decode */
	pSCntx->iAverageDisplayPercent = -1;/* time spent in compose & cc */
	pSCntx->iAverageVoidPercent = -1;	/* time in between frames */

#define SCALABILITY_P_THRESH 75 /* if scaleleve <= then decode */
#define SCALABILITY_P2_THRESH 50 /* if scalelevel <= then decode */
#define SCALABILITY_D_THRESH 25 /* if scalelevel <= then decode */
	pSCntx->iScaleLevel = SCALABILITY_D_THRESH;
	pSCntx->uNYBands = 0;
	for (i = 0; i < MAX_Y_BANDS; i++) {
		pSCntx->iYBandPercent[i] =
		pSCntx->iVUBandPercent[i] = -1;
	}
	pSCntx->bDecode_P = TRUE;
	pSCntx->bDecode_P2 = TRUE;
	pSCntx->bDecode_D = TRUE;
} /* ScalabilityBegin */

/*	**** */

__inline void ScalabilityUpdatePreDecode(
	pScaleCntxSt	pSCntx,
	U32				uRTFlags,
	U32				uFrameType,
	U32				uNYBands,
	U32				uValidDisplay,
	U32				uValidPredicted,
	I32				dmDecMode,
	U32				uFrameTargetTime,
	PBoo			pbDecodeThisFrame,	/* low level decode this frame? */
	PBoo			pbDisplayThisFrame,	/* compose & color convert?		*/
	PU32			uNBands2Decode);
__inline void ScalabilityUpdatePreDecode(
	pScaleCntxSt	pSCntx,
	U32				uRTFlags,
	U32				uFrameType,
	U32				uNYBands,
	U32				uValidDisplay,
	U32				uValidPredicted,
	I32				dmDecMode,
	U32				uFrameTargetTime,
	PBoo			pbDecodeThisFrame,	/* low level decode this frame? */
	PBoo			pbDisplayThisFrame,	/* compose & color convert?		*/
	PU32			uNBands2Decode) {

	Boo				bFast;

/*	EM_UNKNOWN,		*/		/* uninitialized value */
/*	EM_STEPPING,	*/		/* frame by frame mode - no dropping */
/*	EM_PREROLL,		*/		/* decode, but do not display */
/*	EM_NORMAL,		*/		/* normal - frames may be dropped */
/*	EM_HURRYUP,		*/		/* hurried - do as little work as possible */
/*	MAX_EM			*/

/*	Account for time in between frames */
	if (dmDecMode >= EM_NORMAL) {
		U32 t = HiveReadTime();
		if (pSCntx->iStartClock != -1) {
			I32 iPercentUsed;

			iPercentUsed = GET_TIME(pSCntx->iStartClock, t, uFrameTargetTime);
			WEIGHTED_AVG2(pSCntx->iAverageVoidPercent, iPercentUsed, 7, 8);
		}
		pSCntx->iStartClock = t;
	}
	else {
	/*	reset statistics */
		if (dmDecMode == EM_STEPPING) ScalabilityBegin(pSCntx);
	}

#if defined(DEBUG) || defined (FORCE_FULL_DECODE)
	if (uDemoFrame) {
		*pbDecodeThisFrame = (uFrameType < uDemoFrame); /* 1=K, 2=KP, 3=KPP2, 4=KPP2D, 5=KPP2DR */
		*pbDisplayThisFrame = *pbDecodeThisFrame;
	}
	else {
#endif /* DEBUG */

	if (uRTFlags & RT_DONT_DROP_FRAMES) {
		pSCntx->bDecode_P =
		pSCntx->bDecode_P2 =
		pSCntx->bDecode_D = TRUE;
		bFast = (dmDecMode == EM_HURRYUP) || (dmDecMode == EM_PREROLL);
	}
	else {
		pSCntx->iScaleLevel +=
#if 1
			((pSCntx->iAverageDecodePercent + pSCntx->iAverageDisplayPercent)
			* pSCntx->iPercentFramesDecoded / 90 - 90) / 8;
#else /*0*/
			((
			pSCntx->iAverageDecodePercent * pSCntx->iPercentFramesDecoded
			+ (pSCntx->iAverageDecodePercent + pSCntx->iAverageDisplayPercent)
			* pSCntx->iPercentFramesDisplayed
			)
			/ 200 - 100) / 8;
#endif /*0*/
		if (pSCntx->iScaleLevel < 0) pSCntx->iScaleLevel = 0;
		if (pSCntx->iScaleLevel > 100) pSCntx->iScaleLevel = 100;

		bFast = (dmDecMode == EM_HURRYUP) || (dmDecMode == EM_PREROLL);
		if (dmDecMode == EM_NORMAL) {
			if (pSCntx->iPercentFramesDisplayed + 10 >
				pSCntx->iPercentFramesDecoded) {

				bFast = bFast || (95 <
					(pSCntx->iAverageDecodePercent + pSCntx->iAverageDisplayPercent));
			}
		}

#if 0 /* old way */
		if (uFrameType == PICTYPE_K) {
		/* only change P & P2 frame decoding status on key frames */
			pSCntx->bDecode_P = (pSCntx->iScaleLevel <= SCALABILITY_P_THRESH);
			pSCntx->bDecode_P2 = pSCntx->bDecode_P &&
				(pSCntx->iScaleLevel <= SCALABILITY_P2_THRESH);
		}

		pSCntx->bDecode_D = pSCntx->bDecode_P && pSCntx->bDecode_P2 &&
				(pSCntx->iScaleLevel <= SCALABILITY_D_THRESH) && !bFast;
#else /* new way */
		switch (uFrameType) {
			case PICTYPE_K:
				pSCntx->bDecode_P = (pSCntx->iScaleLevel <= SCALABILITY_P_THRESH);
				/* fall thru is intentional */
			case PICTYPE_P:
				pSCntx->bDecode_P2 = pSCntx->bDecode_P &&
					(pSCntx->iScaleLevel <= SCALABILITY_P2_THRESH);
				/* fall thru is intentional */
			default:
				pSCntx->bDecode_D = pSCntx->bDecode_P && pSCntx->bDecode_P2 &&
						(pSCntx->iScaleLevel <= SCALABILITY_D_THRESH) && !bFast;
				
		}
#endif /* way */

	}

	switch (uFrameType) {
		case PICTYPE_K:
			*pbDecodeThisFrame = TRUE;
			*pbDisplayThisFrame = !bFast;
			break;
		case PICTYPE_R:
			*pbDecodeThisFrame = FALSE;
			*pbDisplayThisFrame = uValidDisplay &&
				 (!bFast || (dmDecMode == EM_STEPPING));
			break;
		case PICTYPE_V:
			*pbDecodeThisFrame = FALSE;
			*pbDisplayThisFrame = uValidDisplay && 1;
			break;
		case PICTYPE_P:
			if (uValidPredicted) {
				*pbDecodeThisFrame = uValidPredicted && pSCntx->bDecode_P;
				*pbDisplayThisFrame = !bFast && *pbDecodeThisFrame;
			}
			else {
				*pbDecodeThisFrame =
				*pbDisplayThisFrame = FALSE;
			}
			break;
		case PICTYPE_P2:
			if (uValidPredicted) {
				*pbDecodeThisFrame = uValidPredicted && pSCntx->bDecode_P2;
				*pbDisplayThisFrame = !bFast && *pbDecodeThisFrame;
			}
			else {
				*pbDecodeThisFrame =
				*pbDisplayThisFrame = FALSE;
			}
			break;
		case PICTYPE_D:
			*pbDecodeThisFrame = uValidPredicted && pSCntx->bDecode_D;
			*pbDisplayThisFrame = !bFast && *pbDecodeThisFrame;
			break;
	}

#if defined(DEBUG) || defined (FORCE_FULL_DECODE)
	}
	if (uDemoScale) {
		pSCntx->uNYBands = MIN(uDemoScale, uNYBands);
	}
	else
#endif /* DEBUG */
	if (uRTFlags & RT_DONT_DROP_QUALITY) {
		pSCntx->uNYBands = uNYBands;
	}
	else if (!pSCntx->uNYBands) {
		pSCntx->uNYBands = uNYBands;
	}
	else if (uFrameType == PICTYPE_K) {
		if (pSCntx->uNYBands <= uNYBands) {
		/* we might want to change quality */
			if ((pSCntx->uNYBands < uNYBands) &&
				(pSCntx->iVUBandPercent[0]+pSCntx->iAverageDisplayPercent)
				< 100) {
				pSCntx->uNYBands++;
			}
			if ((pSCntx->uNYBands > 1) &&
				(pSCntx->iVUBandPercent[0]+pSCntx->iAverageDisplayPercent)
				> 100 - 8) {
				pSCntx->uNYBands--;
			}
		}
		else pSCntx->uNYBands = uNYBands;
	}
	*uNBands2Decode = pSCntx->uNYBands;

#ifdef DEBUG
	if (timefile && bDebugScalability) {
		static char ftchar[8] = { 'K','P','2','D','R','V','?','X' };

		fprintf(timefile ,"%c %d^%d ", ftchar[uFrameType], *pbDecodeThisFrame, *pbDisplayThisFrame);
	}
#endif /* DEBUG */

	WEIGHTED_AVG(pSCntx->iPercentFramesDecoded, *pbDecodeThisFrame*100, 15, 16);
	WEIGHTED_AVG(pSCntx->iPercentFramesDisplayed, *pbDisplayThisFrame*100, 15, 16);

} /* ScalabilityUpdatePreDecode */

__inline void ScalabilityUpdatePostBand(pScaleCntxSt pSCntx, I32 dmDecMode, U32 uFrameTargetTime, U32 plane, U32 band);
__inline void ScalabilityUpdatePostBand(pScaleCntxSt pSCntx, I32 dmDecMode, U32 uFrameTargetTime, U32 plane, U32 band) {

	if (dmDecMode >= EM_NORMAL) {
		U32 t = HiveReadTime();
		I32 iPercentUsed;

		iPercentUsed = GET_TIME(pSCntx->iStartClock, t, uFrameTargetTime);
		if (iPercentUsed > 125) iPercentUsed = 125;
		switch (plane) {
			case 0:
				WEIGHTED_AVG2(pSCntx->iYBandPercent[band], iPercentUsed, 7, 8);
			case 1:
				break;
			case 2:
				WEIGHTED_AVG2(pSCntx->iVUBandPercent[band], iPercentUsed, 7, 8);
				break;
		}
	} /* dmDecMode >= DM_NORMAL */
} /* ScalabilityUpdatePostBand */


__inline void ScalabilityUpdatePreDisplay(pScaleCntxSt pSCntx,
	I32 dmDecMode, U32 uFrameTargetTime);
__inline void ScalabilityUpdatePreDisplay(pScaleCntxSt pSCntx,
	I32 dmDecMode, U32 uFrameTargetTime) {

	if (dmDecMode >= EM_NORMAL) {
		U32 t = HiveReadTime();
		I32 iPercentUsed;

		iPercentUsed = GET_TIME(pSCntx->iStartClock, t, uFrameTargetTime);
		if (iPercentUsed > 125) iPercentUsed = 125;
		WEIGHTED_AVG2(pSCntx->iAverageDecodePercent, iPercentUsed, 7, 8);
		pSCntx->iStartClock = t;
	}
} /* ScalabilityUpdatePreDisplay */


__inline void ScalabilityUpdatePostDisplay(pScaleCntxSt pSCntx,
	I32 dmDecMode, U32 uFrameTargetTime);
__inline void ScalabilityUpdatePostDisplay(pScaleCntxSt pSCntx,
	I32 dmDecMode, U32 uFrameTargetTime) {
	if (dmDecMode >= EM_NORMAL) {
		U32 t = HiveReadTime();
		I32 iPercentUsed;

		iPercentUsed = GET_TIME(pSCntx->iStartClock, t, uFrameTargetTime);
		if (iPercentUsed > 125) iPercentUsed = 125;
		WEIGHTED_AVG2(pSCntx->iAverageDisplayPercent, iPercentUsed, 7, 8);
		pSCntx->iStartClock = t;

	}
#ifdef DEBUG
	if (timefile && bDebugScalability) {
		static char dmchar[5] = { 'U','S','P','N','H' };

		fprintf(timefile
			,"%c T:%8d,%3d,(%4d,%4d),%4d,%4d,%4d, {%d, %d, %d, %d} {%d} b%d\n"
			,dmchar[dmDecMode]
			,uFrameTargetTime
			,pSCntx->iScaleLevel
			,pSCntx->iPercentFramesDecoded
			,pSCntx->iPercentFramesDisplayed
			,pSCntx->iAverageDecodePercent
			,pSCntx->iAverageDisplayPercent
			,pSCntx->iAverageVoidPercent
			,pSCntx->iYBandPercent[0]
			,pSCntx->iYBandPercent[1]
			,pSCntx->iYBandPercent[2]
			,pSCntx->iYBandPercent[3]
			,pSCntx->iVUBandPercent[0]
			,pSCntx->uNYBands
			);
	}
#endif /* DEBUG */
} /* ScalabilityUpdatePostDisplay */

#endif /* NEW_SCALABILITY */

/* ********************************************************************* */

PU8
MakeRectMask(PU8 pOut, U32 uXres, U32 uYres,
	     U32 uXOriginD, U32 uYOriginD, U32 uXExtentD, U32 uYExtentD,
	     U32 uXOriginV, U32 uYOriginV, U32 uXExtentV, U32 uYExtentV);
#define LD_UNCHECKED			0
#define LD_OFF					1
#define LD_ON_BUT_NO_MASK		2
#define LD_ON_WITH_VALID_MASK	3

/* ********************************************************************* */

PIA_RETURN_STATUS RTDecompress(
	pRTDecInst		pCntx,					/* decoder instance */
	PU8				pInput,					/* input bitstream */
	U32				uInputLen,				/* input bitstream length */
	DimensionSt		dImageDim,				/* frame size */

	U32				uFrameStartTime,		/* timer at start of frame */
	U32				uFrameTargetTime,		/* allocated time per frame */
	U32				uFrameScale,			/* scale of time units to get ms */

	PU8				pTransMaskForground,	/* if outside world has xparancy */
											/* & wants to share, here it is */
											/* (null means no mask) */
	PU32			puTransPitchForground,	/* pitch of forground mask */
	PU8				pTransMaskExternal,		/* if xparancy & it's wanted, put */
											/* it here, for outside world */
											/* (null means none requested) */
	PU32			puTransPitchExternal,	/* pitch of external mask */
	PPIA_Boolean		pbValidTransparency,	/* did we do what was requested? */


	PRectSt			prColorConvert,			/* How much to color convert */
	U32				uRTFlags,				/* run-time flags (rtdec.h) */
	COLOR_FORMAT	cfOutputFormat,			/* current output format */
	I32				dmDecMode				/* current decode mode */
) {
	PIA_RETURN_STATUS	ret;
	PU8					pBS = pInput;
	U32					bytesRead = 0;
	U32					plane, band;
	RectSt				rViewRectCopy;
	RectSt				rDecodeRectCopy;
	U32					skippedbands;

/*	Scalability hooks */
	Boo					bDecodeBand;
	Boo					bDecodeThisFrame;	/* low level decode? */
	Boo					bOutputThisFrame;	/* compose & color convert? */
	U32					uNBands2Decode;		/* compose & color convert? */

	U32					uLDStatus = LD_UNCHECKED;
	U32					uMockFrame;
	PU8					pTransMaskExternalLocal;

	rViewRectCopy = pCntx->rViewRect;
	rDecodeRectCopy = pCntx->rDecodeRect;

	if (uRTFlags & RT_REDRAW_LAST_FRAME) {
		uMockFrame = PIC_START_CODE | (PICTYPE_R << 5);
		pBS = (PU8)&uMockFrame;
	}
	else /*if (!bFast)*/ {
		pCntx->uFlags &= ~(PL_FLAG_DID_DECODE|PL_FLAG_DID_DISPLAY);
	}

	pInput = pBS;

STARTCLOCK /* decpl */

	ret = DecodePictureLayer(pCntx, pBS, &bytesRead, rDecodeRectCopy, rViewRectCopy);

#define NEW_KEY_SUPPORT
#ifdef NEW_KEY_SUPPORT
	if (ret == PIA_S_KEY_FAILURE) {
		pCntx->uFrameType = PICTYPE_V;
		pCntx->uFlags &= ~PL_CODE_TRANSP;
		if (!pCntx->uValidDisplay) {
			pCntx->uValidDisplay = TRUE;
			pCntx->pFrameDisplay = pCntx->pFrameCurrent;
		}
		ret = PIA_S_OK;
	}
	else
#endif /* NEW_KEY_SUPPORT */
	if (ret != PIA_S_OK) {
		goto end;
	}
	PreDecodeSwapFrameBuffers(pCntx);


#ifdef NEW_SCALABILITY
	ScalabilityUpdatePreDecode(&pCntx->ScaleCntx,
		uRTFlags,
		pCntx->uFrameType,
		pCntx->Plane[0].uNBands,
		pCntx->uValidDisplay,
		pCntx->uValidForward,
		dmDecMode,
		uFrameTargetTime,		/* allocated time per frame */
		&bDecodeThisFrame,
		&bOutputThisFrame,
		&uNBands2Decode
	);
#endif /* NEW_SCALABILITY */


/*	Color Convert Rectangle is view rect, rounded to multiples of 4 vertically,
 *	and multiples of 32 horizontally
 */
	prColorConvert->r = rViewRectCopy.r & ~0x3;
	prColorConvert->c = rViewRectCopy.c & ~0x1f;
	prColorConvert->h = (rViewRectCopy.r + rViewRectCopy.h + 0x3) & ~0x3;
	prColorConvert->w = (rViewRectCopy.c + rViewRectCopy.w + 0x1f) & ~0x1f;
	prColorConvert->h -= prColorConvert->r;
	prColorConvert->w -= prColorConvert->c;

#pragma message("move ld status logic to hivedec")
/*	If View Rect is not entire image, then local decode is on... */
	if ( rViewRectCopy.r | rViewRectCopy.c
		| (rViewRectCopy.h - dImageDim.h)
		| (rViewRectCopy.w - dImageDim.w) ) {
		uLDStatus = LD_ON_BUT_NO_MASK;
	}
	else {
		uLDStatus = LD_OFF;
	}
	pCntx->uMaskStride = ((pCntx->tFrameSize.c + 31) & ~31) / 8;
	if (puTransPitchExternal && !*puTransPitchExternal) {
		*puTransPitchExternal = pCntx->uMaskStride;
	}
	if (puTransPitchForground && !*puTransPitchForground) {
		*puTransPitchForground = pCntx->uMaskStride;
	}


STOPCLOCKANDSAVE(decpl)

	if ((pCntx->tFrameSize.r - dImageDim.h)
		| (pCntx->tFrameSize.c - dImageDim.w)) {
		RET_ERR(0, ret = PIA_S_BAD_IMAGE_DIMENSIONS);
		goto end;
	}

	if (bDecodeThisFrame) { /* is there a real bitstream to decode? */
		pCntx->uFlags |= PL_FLAG_DID_DECODE;
		pCntx->pTranspMask = pCntx->pTranspMaskCurrent;

		pBS += bytesRead;

		ZEROCLOCK(decbndh)
		ZEROCLOCK(dectile)

		skippedbands = 0;
		for (plane = 0; plane < 3; plane++) {

			for (band = 0; band < pCntx->Plane[plane].uNBands; band++) {
				STARTCLOCK /* decbndh */
				if (!skippedbands) {
					skippedbands =
						DecodeBandHeader(pCntx, pBS, &bytesRead, plane, band);
				}
				if (skippedbands == 0) {
					RET_ERR(1, ret = PIA_S_ERROR);
					goto end;
				}
				else if (skippedbands == (U32)-1) {
					ZeroBand(pCntx, plane, band);
				}
				else if (--skippedbands) {
					ZeroBand(pCntx, plane, band);
					continue;
				}
				STOPCLOCKANDACCUMULATE(decbndh)
				pBS += bytesRead;


			/*	Determine if we should decode this band */
				bDecodeBand = (band < uNBands2Decode) &&
					!(pCntx->Plane[plane].pBand[band].uBandFlags &
						(BF_REF_DROPPED|BF_DROPPED));

			/*	if we didn't decode the band, set the ref dropped flag if this */
			/*	is a reference frame */
				if (!bDecodeBand) {
					if (pCntx->uFrameType != PICTYPE_D) {
						pCntx->Plane[plane].pBand[band].uBandFlags |=
							BF_REF_DROPPED;
					}
				}

#ifdef DEBUG
				if (timefile && bPrintBandPercents) {
					if (!bDecodeBand) {
						fprintf(timefile,"plane %d: band %d dropped\n", plane, band);
					}
					else {
						fprintf(timefile,"unsupported %% remaining\n");
					}
				}
#endif /* DEBUG */

				STARTCLOCK /* dectile */
				if (skippedbands == (U32)-1) {
					skippedbands = 0;
				}
				else {
					if (!bDecodeBand && (band < uNBands2Decode)) {
						ZeroBand(pCntx, plane, band);
					}
					if (DecodeTileInfo(pCntx, pBS, &bytesRead, plane, band,
						bDecodeBand) == FALSE) {
						RET_ERR(2, ret = PIA_S_ERROR);
						goto end;
					}
					else {
						pBS += bytesRead;
					}
				}
				STOPCLOCKANDACCUMULATE(dectile)

#ifdef NEW_SCALABILITY
				ScalabilityUpdatePostBand(&pCntx->ScaleCntx,
					dmDecMode, uFrameTargetTime, plane, band);
#endif /* NEW_SCALABILITY */

				/* Now replace scrambled bits if they were changed du to access key */
				if (pCntx->uScrambleLength) {
					memcpy(pCntx->pu8ScrambleStart, pCntx->u8ScrambledData, pCntx->uScrambleLength);
					pCntx->uScrambleLength = 0;
				}

			} /* for band */

		} /* for plane */

		if(pCntx->uFlags & PL_CODE_TRANSP) {	/* if transp present in bs */
			if (bOutputThisFrame
				&& pTransMaskExternal && !pTransMaskForground) {
				if (pbValidTransparency) {
					*pbValidTransparency = FALSE;/* was transparency returned */
				}
				pTransMaskExternalLocal = pTransMaskExternal;
			}
			else {
				pTransMaskExternalLocal = NULL;
			}

			pCntx->bValidBoundingRect = TRUE;
			if (DecodeTransparencyPlane(&pCntx->XCntx, &pCntx->XSCntx,
					pCntx->u8PicFlags & PICF_DATASIZE,
					&pCntx->tFrameSize,
					&pCntx->tTileSize,
					&pCntx->tNTiles, pCntx->vu8DecodeTiles,
					pBS, &bytesRead, FALSE,
					pCntx->pTranspMask,
					pCntx->uMaskStride,
					pTransMaskExternalLocal,
					*puTransPitchExternal,
					&pCntx->bDidXparDec,
					&pCntx->bDirtyUpdate, &pCntx->rBoundRect)) {
			/*	There was an error in the transparency decode ... */
				pCntx->pTranspMask = NULL;
				bytesRead = 0;
				pCntx->bValidBoundingRect = FALSE;
			}

			pBS += bytesRead;
		}
		else {
			pCntx->pTranspMask = NULL;
			pCntx->bDidXparDec = FALSE;
			pCntx->bDirtyUpdate = FALSE;
			pCntx->bValidBoundingRect = FALSE;
		}

	/*	Account for padding the b frame
	 */
		pBS += 8 - (U32)pInput;
		pBS = (PU8)((U32)pBS & ~3);

		if (pCntx->uDataSize) {
			if ((U32)(pBS) != pCntx->uDataSize) {
				RET_ERR(3, ret = PIA_S_ERROR); /* wrong frame size */
				goto end;
			}
		}
		else {
			pCntx->uDataSize = (U32)pBS;
		}
		pBS += (U32)pInput;

	} /* if (bDecodeThisFrame) */
	else { /* if (!bDecodeThisFrame) */
		pBS += pCntx->uDataSize;
	} /* if (!bDecodeThisFrame) */

	/*	Transparency Mask Stuff */
	if (bDecodeThisFrame &&	(pCntx->uFlags & PL_CODE_TRANSP)) {

		/*	Return Transparency Mask if not yet done */
		if (pCntx->bDidXparDec) {
			PU8 pDstRow, pSrcRow;
			PU8 pDstCol, pSrcCol;
			U32 r, c;

			pCntx->pTranspMask = pCntx->pTranspMaskCurrent;
			if (pbValidTransparency) {
				*pbValidTransparency = TRUE;
			}
			if (pTransMaskForground) {
			/*	andnot pTransMask with pTransMaskExternal */
				pDstRow = pCntx->pTranspMask;
				pSrcRow = pTransMaskForground;
				for (r = 0; r < pCntx->tFrameSize.r; r++) {
					pDstCol = pDstRow;
					pSrcCol = pSrcRow;
					for (c = 0; c < pCntx->uMaskStride; c++) {
						*pDstCol++ &= ~(*pSrcCol++);
					}
					pDstRow += pCntx->uMaskStride;
					pSrcRow += *puTransPitchForground;
				}
			}

			if ((pCntx->uFrameType == PICTYPE_R) || pTransMaskForground) {
				if (bOutputThisFrame && pTransMaskExternal) {
				/*	copy pTransMask to pTransMaskExternal */
					pDstRow = pTransMaskExternal;
					pSrcRow = pCntx->pTranspMask;
					for (r = 0; r < pCntx->tFrameSize.r; r++) {
						pDstCol = pDstRow;
						pSrcCol = pSrcRow;
						for (c = 0; c < pCntx->uMaskStride; c++) {
							*pDstCol++ = *pSrcCol++;
						}
						pDstRow += *puTransPitchExternal;
						pSrcRow += pCntx->uMaskStride;
					}
				}
			}

			if (/*!pCntx->uHoldCount && */
				((uLDStatus == LD_ON_BUT_NO_MASK) ||
					 (uLDStatus == LD_OFF))) {
		/*	We have local decode - create the mask */
				RectSt r;
#pragma message ("tbd: optimize xpar local decode w/o fill")
#if 0
				if (pCntx->htkTransparencyKind == TK_SELECTIVE_UPDATE) {
					RECT_INTERSECT(r, pCntx->rBoundRect, rViewRectCopy);
				}
				else
#endif /*0*/
				{
					r = rViewRectCopy;

				}
				SET_CC_RECT(r,prColorConvert,dImageDim);

				if (IS_ALIGNED(r)
					|| IS_ALIGNED2(r,dImageDim)	/* new */
						) {
					pCntx->pTranspMask += r.r*pCntx->uMaskStride +
						r.c/8;
				}
				else {
					if (!pCntx->pLDMask) {
						pCntx->pLDMask = MakeRectMask(pCntx->pLDMaskStorage,
							(pCntx->tFrameSize.c+31)&~31,
							pCntx->tFrameSize.r,
							prColorConvert->c, prColorConvert->r,
							prColorConvert->w, prColorConvert->h,
							r.c, r.r,
							r.w, r.h);
					}
					if (pCntx->pLDMask) {
						pCntx->pTranspMask +=
							(pCntx->pLDMask - pCntx->pLDMaskStorage);
					}
					else {
						RET_ERR(4, ret = PIA_S_ERROR);
						goto end;
					}
				}

				uLDStatus = LD_ON_WITH_VALID_MASK;
			}
		}
		else {
			if (pbValidTransparency) {
				*pbValidTransparency = FALSE;
			}
			pCntx->pTranspMask = NULL;
		}
	}
	else {
		if (pTransMaskForground) {
			pCntx->pTranspMask = pTransMaskForground;
			pCntx->uMaskStride = *puTransPitchForground;

		/*	if we have local decode - then create the mask */
			if ((uLDStatus == LD_ON_BUT_NO_MASK) ||
					 (uLDStatus == LD_OFF)) {
				RectSt r;
				r = rViewRectCopy;

				SET_CC_RECT(r,prColorConvert,dImageDim);

				if (IS_ALIGNED(r)
					|| IS_ALIGNED2(r,dImageDim)	/* new */
					) {
					pCntx->pTranspMask += r.r*pCntx->uMaskStride +
						r.c/8;
				}
				else {
					if (!pCntx->pLDMask) {
						pCntx->pLDMask = MakeRectMask(pCntx->pLDMaskStorage,
							(pCntx->tFrameSize.c+31)&~31,
							pCntx->tFrameSize.r,
							prColorConvert->c, prColorConvert->r,
							prColorConvert->w, prColorConvert->h,
							r.c, r.r,
							r.w, r.h);
					}
					if (pCntx->pLDMask) {
						pCntx->pTranspMask +=
							(pCntx->pLDMask - pCntx->pLDMaskStorage);
					}
					else {
						RET_ERR(5, ret = PIA_S_ERROR);
						goto end;
					}
				}

				uLDStatus = LD_ON_WITH_VALID_MASK;
			}
		}
		if (pbValidTransparency) {
			*pbValidTransparency = FALSE;
		}
	}

	/*	if transparency is off, we still haven't dealt with local
	 *	decode - do it now.
	 */
	if (uLDStatus == LD_ON_BUT_NO_MASK) {
	/*	We have local decode - create the mask */
		SET_CC_RECT(rViewRectCopy,prColorConvert,dImageDim);
		if (!(IS_ALIGNED(rViewRectCopy)
					|| IS_ALIGNED2(rViewRectCopy,dImageDim)	/* new */
				)) {
			if (!pCntx->pLDMask) {
				if (!(pCntx->pLDMask = MakeRectMask(pCntx->pLDMaskStorage,
				(pCntx->tFrameSize.c+31)&~31,
				pCntx->tFrameSize.r,
				prColorConvert->c, prColorConvert->r,
				prColorConvert->w, prColorConvert->h,
				rViewRectCopy.c, rViewRectCopy.r,
				rViewRectCopy.w, rViewRectCopy.h))) {
					RET_ERR(6, ret = PIA_S_ERROR);
					goto end;
				}
			}
		}
		uLDStatus = LD_ON_WITH_VALID_MASK;
	}
	else if (uLDStatus == LD_OFF) {
		SET_CC_RECT(rViewRectCopy,prColorConvert,dImageDim);
	}

#ifdef NEW_SCALABILITY
	pCntx->uValidCurrent = uNBands2Decode;
	ScalabilityUpdatePreDisplay(&pCntx->ScaleCntx, dmDecMode, uFrameTargetTime);
#endif /* NEW_SCALABILITY */

	PostDecodeSwapFrameBuffers(pCntx);

STARTCLOCK /* compose */

/*	Determine if we should compose the bands */
	if (!(bOutputThisFrame && pCntx->uValidDisplay)) {
		pCntx->uFlags &= ~PL_FLAG_DID_DISPLAY;
		ret = PIA_S_DONT_DRAW;
	} /* if (bFast) */
	else { /* if (!bFast) */
	/*	Do the Global Composition
	 */
		if (!(pCntx->uFlags & PL_FLAG_DID_DISPLAY)) {
			if ((pCntx->uFlags & PL_CODE_TRANSP)) {
				RectSt r;
			/*	Intersect pCntx->rBoundRect with ViewRect */
				RECT_INTERSECT(r, pCntx->rBoundRect, rViewRectCopy);
				Compose(pCntx, r, uNBands2Decode);
			} /* if transparency */
			else { /* if not transparency */
				Compose(pCntx, rViewRectCopy, uNBands2Decode);
			} /* if not transparency */
			pCntx->uFlags |= PL_FLAG_DID_DISPLAY;

#ifdef DEBUG
			if ((bFrameCheckSum && (pCntx->uFrameCheckSum != (U32)-1))) {
				FrameCheckSum(pCntx);
			}
#endif /* DEBUG */

		} /* if not if09 */

	} /* if (!bFast) */

STOPCLOCKANDSAVE(compose)

end:

	return ret;
}	/* RTDecompress() */

/* ********************************************************************* */
/*	This function is called after color conversion has taken place so that
 *	we can determine how long the compose & CC took for future reference.
 */
PIA_RETURN_STATUS RTDecompressFrameEnd(
	pRTDecInst pCntx,
	I32 dmDecMode,
	U32 uFrameTargetTime
) {
	PIA_RETURN_STATUS	prs = PIA_S_OK;

#ifdef NEW_SCALABILITY
	ScalabilityUpdatePostDisplay(&pCntx->ScaleCntx, dmDecMode, uFrameTargetTime);
#endif /* NEW_SCALABILITY */

	return prs;
}

/* ********************************************************************* */

PIA_RETURN_STATUS RTDecompressEnd(pRTDecInst pCntx) {
#define SafeFreePtr(x) if (x) HiveLocalFreePtr(x); (x) = NULL

	if (pCntx) {
		U32 p, b;		/* plane, band indices */
/*	Free the Frame level structures */
		SafeFreePtr(pCntx->YVUOutputStorage);
		SafeFreePtr(pCntx->pBlockInfo);
		SafeFreePtr(pCntx->pBlockInfoBand0);
		SafeFreePtr(pCntx->pPlane3Storage);
		SafeFreePtr(pCntx->pTranspMaskCurrent);
		SafeFreePtr(pCntx->pLDMaskStorage);
		SafeFreePtr(pCntx->vu8DecodeTiles);

		for (p = 0; p < 3; p++) {
			for (b = 0; b < pCntx->Plane[p].uNBands; b++) {
				if (p < 2) {
				/*	don't free plane 2's tiles, since they are
				 *	shared with plane 1
				 */
					SafeFreePtr(pCntx->Plane[p].pBand[b].pTile);
				}
			}
			SafeFreePtr(pCntx->Plane[p].pBand);
		}
		HiveLocalFreePtr(pCntx);
	}

#ifdef DEBUG
	if (timefile) {
		fclose(timefile);
		timefile = NULL;
	}
#endif /* DEBUG */

	return PIA_S_OK;

#undef SafeFreePtr
}	/* RTDecompressEnd() */
