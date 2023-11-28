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

/*
 *	HIVEDEC.C -- high-level decompressor functions, called from HIVE
 *		DecodeStartup					// sets up shared memory across instances
 *		DecodeConstantInit				// sets up instance specific constants
 *		DecodeImageDimInit				// initialize the instance with image dim's
 *		DecodeQuery						?
 *		DecodeSequenceSetup				?
 *		DecodeFrame						// decode a frame
 *		DecodeSequenceEnd				?
 *		DecodeFreePrivateData			?
 *		DecodeShutdown					// frees shared memory if last instance
 *      DecodeDebugGet					?
 *      DecodeDebugSet					?
 *
 *		DecodeGetCompressedSize (only used for the QT)
 *
 *		DecodeSetAccessKey				?
 *		DecodeGetBoundingRectangle		?
 *		DecodeGetDefDecodeRect			?
 *		DecodeGetMinDecodeRect			?
 *		DecodeGetMaxDecodeRect			?
 *		DecodeGetDecodeRect
 *		DecodeSetDecodeRect
 *		DecodeIsKeyFrame				?
 *		DecodeGetDefPersData			// currently unsupported, returns ok
 *		DecodeGetPersData				// currently unsupported, returns ok
 *		DecodeSetPersData				// currently unsupported, returns ok
 *
 *		DecodeGetDefContrast			// currently unsupported, returns ok
 *		DecodeGetContrast				// currently unsupported, returns ok
 *		DecodeSetContrast				// currently unsupported, returns ok
 *		DecodeGetDefGamma				// currently unsupported, returns ok
 *		DecodeGetGamma					// currently unsupported
 *		DecodeSetGamma					// currently unsupported
 *		DecodeGetDefSaturation			// currently unsupported, returns ok
 *		DecodeGetSaturation				// currently unsupported, returns ok
 *		DecodeSetSaturation				// currently unsupported, returns ok
 *		DecodeGetDefBrightness			// currently unsupported, returns ok
 *		DecodeGetBrightness				// currently unsupported, returns ok
 *		DecodeSetBrightness				// currently unsupported, returns ok
 *
 *		DecodeGetOutputTransMask		?
 *		DecodeGetDefTransFillColor		
 *		DecodeSetTransFillColor			
 *		DecodeGetTransFillColor			
 *		DecodeSetViewRect

 *		All prototypes are in pia_dec.h
 *	For more information on the definitions of these functions and their
 *	interactions, see doc\hive\pia_1_1.doc on Cosby in the HQV directory*
 */

#ifndef C_PORT_KIT
#include <windows.h>
#endif /* C_PORT_KIT */

#include "datatype.h"
#include "pia_main.h"
#include "cpk_blk.h"
#include "xpardec.h"
#include "rtdec.h"
#include "hivedec.h"
#include "pia_dec.h"
#include "imagedef.h"
#include "decpl.h"
#include "controls.h"

#include "const.h"

#include "convtabs.h"
#ifndef C_PORT_KIT
#include "resource.h"
#endif /* C_PORT_KIT */

#ifdef DEBUG
extern U32 uBSVersion;
#undef BS_VERSION
#define BS_VERSION uBSVersion
#endif /* DEBUG */

#ifndef C_PORT_KIT
/* the instance of this driver, global variable defined in v32_drv.c */
extern HINSTANCE hDriverModule;

U32 uResourceID[2] = {	IDB_INDEO_TOO_OLD_REJECT_MSG,
						IDB_INDEO_TOO_NEW_REJECT_MSG };
static PU8 sErrorName[2] = {	"IVI50_TooOldVersionErrorBitmap",
								"IVI50_TooNewVersionErrorBitmap" };
static PU8 pBSMaskY[2] = { NULL, NULL };
static PU8 pBSMaskV[2] = { NULL, NULL };
static PU8 pBSMaskU[2] = { NULL, NULL };
#endif /* C_PORT_KIT */

#pragma data_seg(".sdata")	/* following is data shared among instances */
U32	uNInstances = 0;
U32 uBSMaskPitch[2] = { 0, 0 };
U32 uBSMaskNRows[2] = { 0, 0 };
U32 uBSMaskNCols[2] = { 0, 0 };
#pragma data_seg()	/* end shared data */

#ifndef C_PORT_KIT
#ifdef __MEANTIME__
#include "meantime.h"
extern int sum_csc_strides_seen;
#endif
#endif

#ifdef MEMDEBUG
#include <stdio.h>
FILE *fp;
#endif

#define SafeHiveGlobalFreePtr(x) if (x) HiveGlobalFreePtr(x); (x) = NULL


/*
 * DecodeStartup -- First time called
 */
PIA_RETURN_STATUS
DecodeStartup()
{
#ifndef C_PORT_KIT
	HBITMAP				hbmTmp = 0;
	LPBITMAPINFOHEADER	pInfo = 0;
	PIA_Boolean				bBitMapExists;
	U32					uYVUDelta, uSrcPitch, i;
#endif /* C_PORT_KIT */

#ifdef MEMDEBUG
	fp = fopen("/tmp/memdebug.out", "w");
#endif

#ifndef C_PORT_KIT 
	/* Load the bitmap using generized functions so that it can be 
	 * locked down in memory so that we can get a pointer
	 */
	for (i = 0; i < 2; i++) {
		if ( uNInstances == 0 /*! uBSMaskPitch[i]*/) {

			hbmTmp =
				LoadResource( hDriverModule,
					FindResource( hDriverModule,
						MAKEINTRESOURCE( uResourceID[i] ),
						RT_BITMAP
					)
				);
			if (!hbmTmp) {
				return PIA_S_OUT_OF_MEMORY;
			}
		/* If there is a bitmap, get a memory pointer to it from the Handle
		 */
			pInfo = LockResource( hbmTmp );

			uBSMaskNRows[i] = pInfo->biHeight;
			uBSMaskNCols[i] = ABS( pInfo->biWidth );

		/*	Round pitch up to the nearest multiple of 4. */
		/*	This assumes 3 bytes per pixel (RGB24) */
			uSrcPitch = ((pInfo->biBitCount*pInfo->biWidth+(3<<3))>>(2+3))<<2;
			uBSMaskPitch[i] = (uBSMaskNCols[i] + 3) & ~3;
		} /* if */

		pBSMaskY[i] = (PU8)HiveAllocSharedPtr(
			3 * uBSMaskNRows[i] * uBSMaskPitch[i],
			sErrorName[i], &bBitMapExists);
		uYVUDelta = uBSMaskNRows[i] * uBSMaskPitch[i];
		pBSMaskV[i] = pBSMaskY[i] + uYVUDelta;
		pBSMaskU[i] = pBSMaskV[i] + uYVUDelta;
		if (!pBSMaskY[i]) {
			return PIA_S_OUT_OF_MEMORY;
		}

		if (!bBitMapExists && hbmTmp && pInfo) {
			PU8					pSrcRow, pSrcCol;
			PU8					pDstRow, pDstCol;
			U32					r, c;

			pSrcRow = ((PI8)pInfo + pInfo->biSize) + uSrcPitch * pInfo->biHeight;
			if (pInfo->biBitCount == 8) {
				pSrcRow += 256*4;	/* offset the palette */
			}
			pDstRow = pBSMaskY[i];
			for (r = 0; r < uBSMaskNRows[i]; r++) {
				pSrcRow -= uSrcPitch;
				pSrcCol = pSrcRow;
				pDstCol = pDstRow;
				for (c = 0; c < uBSMaskNCols[i]; c++) {
					U32 uYVUtmp;
					if (pInfo->biBitCount == 8) {
						U32 u0RGB;
						u0RGB = *((PU32)((PU8)pInfo+pInfo->biSize) + pSrcCol[0]);
						uYVUtmp = BtoYUV[u0RGB&0xff]
							+ GtoYUV[(u0RGB>>8)&0xff]
							+ RtoYUV[(u0RGB>>16)&0xff];
					}
					else {
						uYVUtmp = BtoYUV[pSrcCol[0]]
							+ GtoYUV[pSrcCol[1]]
							+ RtoYUV[pSrcCol[2]];
					}
					pDstCol[0]			= (U8) (uYVUtmp >> 16);	/* Y */
					pDstCol[uYVUDelta]	= (U8) (uYVUtmp);		/* V */
					pDstCol[uYVUDelta*2]= (U8) (uYVUtmp >> 8);	/* U */
					pDstCol++;
					pSrcCol+=(pInfo->biBitCount>>3);
				}
				pDstRow += uBSMaskPitch[i];
			}

			FreeResource( hbmTmp );
		} /* if (!bBitMapExists && hbmTmp && pInfo) */
	} /* for i */
#endif /* C_PORT_KIT */

#ifdef C_PORT_KIT
	InitDecodeTables2();
#else /* C_PORT_KIT */
	InitDecodeTables();
#endif /* C_PORT_KIT */
	uNInstances++;

	return PIA_S_OK;
} /* DecodeStartup() */


/*
 * DecodeConstantInit -- Initialize the instance w/o image dims
 *	Verify the Four CC code (Returns PIA_S_BAD_COLOR_FORMAT)
 * 	Initialize the constant instance information (pInst->dciInfo)
 *  Initialize private decoder data (alloc space for tables, init booleans)
 */
PIA_RETURN_STATUS
DecodeConstantInit(	PTR_DEC_INST pInst )
{
	const I32			NPE = 256;	   /* NUMBER_PALETTE_ENTRIES */
	I32					i;
	PIA_RETURN_STATUS	prs = PIA_S_OK;

#ifdef POINTER_DEBUG
	if ((HiveGlobalPtrCheck(pInst,sizeof(DEC_INST)) != PIA_S_OK) ||
		(pInst->uTag != 0))  {
		prs = PIA_S_ERROR; /* Sanity check pointer */
		goto end;
	}
#endif /* POINTER_DEBUG */

	pInst->dciInfo.dcfcSupportedFrameControls = 
			DC_CLIPPING |
			DC_TIME_TO_DECODE |
			DC_TRANSPARENCY |
			DC_BRIGHTNESS |
			DC_SATURATION |
			DC_CONTRAST |
			DC_FRAME_FLAGS_VALID ;
	pInst->dciInfo.dcfcSuggestedFrameControls =
			DC_TIME_TO_DECODE |
			DC_FRAME_FLAGS_VALID ;
	pInst->dciInfo.dcscSupportedSequenceControls = 
			DC_USES_DIFF_FRAMES |
			DC_USES_LAST_OUTPUT_BUFFER |
			DC_ACCESS_KEY |
			DC_DONT_DROP_FRAMES	|
			DC_DONT_DROP_QUALITY |
			DC_SEQUENCE_FLAGS_VALID;
	pInst->dciInfo.dcscSuggestedSequenceControls =
			DC_USES_DIFF_FRAMES |
			DC_SEQUENCE_FLAGS_VALID;
	
	pInst->dciInfo.listInputFormats.u16NumberOfAlgorithms = 1;
	pInst->dciInfo.listInputFormats.eList[0] = CF_IV50 ;
	pInst->dciInfo.listInputFormats.u16Tag = SUPPORTED_ALGORITHMS_TAG;

#ifdef C_PORT_KIT
	pInst->dciInfo.listOutputFormats.u16NumberOfAlgorithms = 1;
	pInst->dciInfo.listOutputFormats.eList[0] = CF_PLANAR_YVU9_8BIT ;
#else /* C_PORT_KIT */
	pInst->dciInfo.listOutputFormats.u16NumberOfAlgorithms = 2;
	pInst->dciInfo.listOutputFormats.eList[0] = CF_IF09 ;
	pInst->dciInfo.listOutputFormats.eList[1] = CF_PLANAR_YVU9_8BIT ;
#endif /* C_PORT_KIT */

	pInst->dciInfo.listOutputFormats.u16Tag = SUPPORTED_ALGORITHMS_TAG;

	/* Initialize private data */
	if (pInst->pDecodePrivate) {
		SafeHiveGlobalFreePtr(pInst->pDecodePrivate->puLumaTable);
		SafeHiveGlobalFreePtr(pInst->pDecodePrivate->puChromaTable);
		SafeHiveGlobalFreePtr(pInst->pDecodePrivate->puBrightTable);
		SafeHiveGlobalFreePtr(pInst->pDecodePrivate->puContrastTable);
		SafeHiveGlobalFreePtr(pInst->pDecodePrivate);
	}
	pInst->pDecodePrivate = RTDecompressBegin();
	if (!pInst->pDecodePrivate) {
		prs = PIA_S_OUT_OF_MEMORY;
		goto end;
	}

	pInst->pDecodePrivate->bUpdateChrominance = FALSE;
	pInst->pDecodePrivate->bUpdateLuminance   = FALSE;

	pInst->pDecodePrivate->puLumaTable   = (PU8)HiveGlobalAllocPtr(NPE,TRUE);
	pInst->pDecodePrivate->puChromaTable = (PU8)HiveGlobalAllocPtr(NPE,TRUE);
	pInst->pDecodePrivate->puBrightTable = (PU8)HiveGlobalAllocPtr(NPE,TRUE);
	pInst->pDecodePrivate->puContrastTable=(PU8)HiveGlobalAllocPtr(NPE,TRUE);

	if (!pInst->pDecodePrivate->puLumaTable   ||
		!pInst->pDecodePrivate->puChromaTable ||
		!pInst->pDecodePrivate->puBrightTable ||
		!pInst->pDecodePrivate->puContrastTable) {
		prs = PIA_S_OUT_OF_MEMORY;
		goto end;
	}

	/* Since the brightness and contrast tables are interdependent,
	 * they must both be initialized */
	for (i=0; i<NPE; i++)
		pInst->pDecodePrivate->puContrastTable[i] =
			pInst->pDecodePrivate->puBrightTable[i] = (U8)i;

	pInst->uTag = DEC_PARTIAL_INST_TAG;

	pInst->pPersData = HiveLocalAllocPtr(sizeof(DECODER_PERS_DATA), FALSE);
	if (!pInst->pPersData) {
		prs = PIA_S_OUT_OF_MEMORY;
		goto end;
	}
	/* first initialize with defaults */
	DecodeGetDefPersData(pInst->pPersData);
#ifndef QT_FOR_MAC
	HiveInitDecoderPersistentData(pInst->pPersData);
#endif
	DecodeSetPersData(pInst->pPersData, pInst->pPersData);

end:
	return prs;
} /* DecodeConstantInit */

/*
 * DecodeImageDimInit -- Initialize the instance with image dim's
 *	Verify the image dimensions	(Returns PIA_S_BAD_IMAGE_DIMENSIONS)
 */
PIA_RETURN_STATUS 
DecodeImageDimInit(
	PTR_DEC_INST					pInst, 
	PTR_DECODE_FRAME_INPUT_INFO		pInput,
	PTR_DECODE_FRAME_OUTPUT_INFO	pOutput
)
{
	PIA_RETURN_STATUS prs;

#ifdef POINTER_DEBUG
	if ((HiveGlobalPtrCheck(pInst, sizeof(DEC_INST)) != PIA_S_OK) ||
		(pInst->uTag != DEC_PARTIAL_INST_TAG)) {
		prs = PIA_S_ERROR; /* Sanity check pointer */
		goto end;
	}
#endif

    /* Check that the image dimensions meet specifications */
    if ((pInst->dImageDim.w > (U32)MAX_IMAGE_WIDTH) ||
	   	(pInst->dImageDim.h > (U32)MAX_IMAGE_HEIGHT) ||
        (pInst->dImageDim.h*pInst->dImageDim.w > (U32)MAX_IMAGE_PIXELS) ||
		(!pInst->dImageDim.w) || (!pInst->dImageDim.h) ||
	   	(pInst->dImageDim.w  % IMAGE_MULTIPLE)   ||
	   	(pInst->dImageDim.h % IMAGE_MULTIPLE)) {
    	prs =  PIA_S_BAD_IMAGE_DIMENSIONS;
		goto end;
	}

	pInst->uTag = DEC_INST_TAG;
	prs = PIA_S_OK;

end:
	return prs;
} /* DecodeImageDimInit */

#ifndef C_PORT_KIT
static void BSPlaneCopy(PU8 pSrc,
	PU8 pDst, U32 uDstPitch, U32 uNDstRow, U32 uNDstCol, U32 uSub, U32 i);
static void BSPlaneCopy(PU8 pSrc,
	PU8 pDst, U32 uDstPitch, U32 uNDstRow, U32 uNDstCol, U32 uSub, U32 i) {

	U32	r,c, r2, c2;
	PU8	pSrcRow, pSrcCol;
	PU8	pDstRow, pDstCol;

	pDstRow = pDst;
	for (r = 0, r2 = uBSMaskNRows[i]; r < uNDstRow; r++, r2+=uSub) {
		if (r2 >= uBSMaskNRows[i]) {
			r2 -= uBSMaskNRows[i];
			pSrcRow = pSrc + (r2 * uBSMaskPitch[i]);
		}
		pDstCol = pDstRow;
		for (c = 0, c2 = uBSMaskNCols[i]; c < uNDstCol; c++, c2+=uSub) {
			if (c2 >= uBSMaskNCols[i]) {
				c2 -= uBSMaskNCols[i];
				pSrcCol = pSrcRow + c2;
			}
			*pDstCol = *pSrcCol;
			pSrcCol += uSub;
			pDstCol++;
		} /* c */
		pSrcRow += uBSMaskPitch[i] * uSub;
		pDstRow += uDstPitch;
	} /* r */
} /* BSPlaneCopy */

static void FillYVUWithBSMask(PU8 pYDst, PU8 pVDst, PU8 pUDst,
	U32 uYDstPitch, U32 uVUDstPitch,
	U32 uNDstRow, U32 uNDstCol, Boo bYVU12, U32 i
);
static void FillYVUWithBSMask(PU8 pYDst, PU8 pVDst, PU8 pUDst,
	U32 uYDstPitch, U32 uVUDstPitch,
	U32 uNDstRow, U32 uNDstCol, Boo bYVU12, U32 i
) {
	U32	uNVUDstRow;
	U32	uNVUDstCol;
	U32	uVUSub;

#ifdef DEBUG
	HivePrintF("FillYVU... %d y=%x v=%x u=%x p=%x r=%x c=%x\n",
		i,pBSMaskY[i],pBSMaskV[i],pBSMaskU[i],
		uBSMaskPitch[i],uBSMaskNRows[i],uBSMaskNCols[i]
	);
#endif /* DEBUG */

	/* abort upon invalid input */
	if (!((U32)pYDst && (U32)pBSMaskY[i])) {
		return;
	}

	if (bYVU12) {
		uNVUDstRow = (uNDstRow + 1) / 2;
		uNVUDstCol = (uNDstCol + 1) / 2;
		uVUSub = 2;
	}
	else { /* good ol' yvu9 */
		uNVUDstRow = (uNDstRow + 3) / 4;
		uNVUDstCol = (uNDstCol + 3) / 4;
		uVUSub = 4;
	}

	BSPlaneCopy(pBSMaskY[i], pYDst, uYDstPitch, uNDstRow, uNDstCol, 1, i);
	BSPlaneCopy(pBSMaskV[i], pVDst, uVUDstPitch, uNVUDstRow,
		uNVUDstCol, uVUSub, i);
	BSPlaneCopy(pBSMaskU[i], pUDst, uVUDstPitch, uNVUDstRow,
		uNVUDstCol, uVUSub, i);

} /* FillYVUWithBSMask */
#endif /* C_PORT_KIT */

/*
 * DecodeFrame -- Decode a frame
 *	Prepare parameters for RTDecompress
 *	Call RTDecompress to do the actual decompression
 *  Set the output parameters
 *	Adjust brightness, contrast, and saturation if necessary
 *	Perform color convert
 *	RTDecompress returns	PIA_S_DONT_DRAW if time runs out while decompressing
 *				 		PIA_S_BAD_COMPRESSED_DATA if the bitstream is corrupted
 *				 		PIA_S_MISSING_KEY_FRAME if a keyframe is missing
 *				 		PIA_S_UNSUPPORTED_FUNCTION
 *				 		PIA_S_OUT_OF_MEMORY
 */

PIA_RETURN_STATUS 
DecodeFrame(
	PTR_DEC_INST					pInst, 
	PTR_DECODE_FRAME_INPUT_INFO		pInput,
	PTR_DECODE_FRAME_OUTPUT_INFO	pOutput
)
{
	PIA_RETURN_STATUS prs;
	pRTDecInst pCntx = pInst->pDecodePrivate;
	PIA_Boolean bTransparency = FALSE;  /* Transparency bitmask present */
	PU8 pu8OutputTransparency;	/* Pointer to requested xpar mask (0 if not wanted) */

	U32 uWidth, uHeight;		/* Of area to color convert */
	PU8 pY, pV, pU;				/* Start of Y, V, and U Planes for col conv */
	U32 uYPitch, uVUPitch;		/* Pitch of Y and V=U Planes for col conv */

#ifdef __MEANTIME__
	SummarizeTimes(998,pInput->rDecodeRect.uWidth,pInput->rDecodeRect.uHeight);
	if(iTimingFrame != -1){
		STARTCLOCK
		frametimes[iTimingFrame].start_lo = startlow;
		frametimes[iTimingFrame].start_hi = starthigh;
		STARTCLOCK
	}
#endif

#ifdef POINTER_DEBUG
	if ((HiveGlobalPtrCheck(pInst,   sizeof(DEC_INST))   != PIA_S_OK) ||
		(HiveGlobalPtrCheck(pInput,  sizeof(DECODE_FRAME_INPUT_INFO))  != PIA_S_OK) ||
		(HiveGlobalPtrCheck(pOutput, sizeof(DECODE_FRAME_OUTPUT_INFO)) != PIA_S_OK)) {
		prs = PIA_S_ERROR;
		goto end;
	}
	if ((pInst->uTag  != DEC_INST_TAG) ||
		(pInput->uTag != DECODE_FRAME_INPUT_INFO_TAG) ) {
		prs = PIA_S_ERROR; /* verify input OK */
		goto end;
	}
#endif

	if (pInput->cfInputFormat == CF_IV50) {
		U32 uRTFlags = 0;
		U32 uCCFlags = 0;

		/* Ask for transparency only when it was requested AND the control is active */
		if ( pOutput->bOutputTransparencyBitmask &&
				(pInst->dcfcFrameControlsUsed & DC_TRANSPARENCY)) {
			pu8OutputTransparency = pOutput->pioOutputData.pu8ExpTransMask;
		}
		else {
			pu8OutputTransparency = NULL;
		}


		/* bUpdate frame means the frame is already partially  
		 * decoded... it needs to be composed and color converted.
		 */
		if (pInput->bUpdateFrame) {
			uRTFlags |= RT_REDRAW_LAST_FRAME;
		}
		
		/* Limit Scalabity to only quality droppage or only frame droppage
		 *	if requested
		 */
		if (pInst->dcscSequenceControlsUsed & DC_DONT_DROP_FRAMES) {
			uRTFlags |= RT_DONT_DROP_FRAMES;
		}
		if (pInst->dcscSequenceControlsUsed & DC_DONT_DROP_QUALITY) {
			uRTFlags |= RT_DONT_DROP_QUALITY;
		}


#ifdef __MEANTIME_HIVEDEC__
		STOPCLOCK
		if(iTimingFrame != -1)
			frametimes[iTimingFrame].pregetsize = elapsed;
		STARTCLOCK
#endif /* __MEANTIME_HIVEDEC__ */

#ifdef __MEANTIME_HIVEDEC__
		STOPCLOCK
		if(iTimingFrame != -1)
			frametimes[iTimingFrame].getsize = elapsed;
		STARTCLOCK
#endif /* __MEANTIME_HIVEDEC__ */

		/* Decompress the frame */
		/* It's OK to pass in the View and Decode Rectangles without checking
		 * about clipping being on or not, because if clipping is off they are
		 * passed in with the image dimensions, which is what is wanted here.
		 */

		prs = RTDecompress(pCntx
			,pInput->pu8CompressedData
			,pInput->uSizeCompressedData
			,pInst->dImageDim

			,pInput->uStartTime
			,pInst->uMaxFrameDuration
			,pInst->peiEnvironment->uTimerScale

			,pOutput->pioOutputData.pu8FGTransMask
			,&pOutput->pioOutputData.uFGTransPitch

			,pu8OutputTransparency
			,&pOutput->pioOutputData.uExpTransPitch
			,&bTransparency

			,&pOutput->pioOutputData.rCCRect
			,uRTFlags
			,0
			,pInst->eMode
		);

#ifdef __MEANTIME_HIVEDEC__
		STOPCLOCK
		if(iTimingFrame != -1)
			frametimes[iTimingFrame].rtdecompress = elapsed;
		STARTCLOCK
#endif /* __MEANTIME_HIVEDEC__ */

		if (prs != PIA_S_OK) {
			goto end;
		}

		pY = pInst->pDecodePrivate->Plane[0].pOutput;
		pV = pInst->pDecodePrivate->Plane[1].pOutput;
		pU = pInst->pDecodePrivate->Plane[2].pOutput;
		uYPitch = pOutput->pioOutputData.uYPitch =
			pInst->pDecodePrivate->Plane[0].uOutputPitch;
		uVUPitch = pOutput->pioOutputData.uVUPitch =
			pInst->pDecodePrivate->Plane[1].uOutputPitch;

		pOutput->pioOutputData.pu8TransMask = pCntx->pTranspMask;
		pOutput->pioOutputData.uTransPitch = pCntx->uMaskStride;
		pOutput->pioOutputData.uMaskSize = pCntx->uMaskSize;

		pOutput->pioOutputData.pu8LDMask = pCntx->pLDMask;
#define BGR_UNDEFINED 0xff000000
#define BGR_TO_U32(BGR) ( ((BGR).u8Reserved << 24) | ((BGR).u8R << 16) | ((BGR).u8G << 8) | (BGR).u8B )
		if (BGR_TO_U32(pOutput->pioOutputData.bgrXFill) == BGR_UNDEFINED) {
			pOutput->pioOutputData.bgrXFill = pCntx->uTransColor;
		}
		
		/* These parms are set in Decompress */
		pOutput->uTag = DECODE_FRAME_OUTPUT_INFO_TAG;

		pInst->pDecodePrivate->bValidTransBitMask = bTransparency;


	/*	Brightness, Saturation, & Contrast Controls */
		if (pCntx->uFrameType != PICTYPE_R ) {
			/* If the brightness, saturation, or contrast controls have been
		 	* adjusted, apply the adjustment to the image */


#pragma message("move this to rtcout.c")
			if (pInst->pDecodePrivate->bUpdateLuminance) {
				DecodeUpdateLuma(pY + pCntx->rDecodeRect.c +
					pCntx->rDecodeRect.r*uYPitch,
					pCntx->rDecodeRect.w, pCntx->rDecodeRect.h,
					uYPitch, pInst->pDecodePrivate->puLumaTable);
			}

			if (pInst->pDecodePrivate->bUpdateChrominance) {
				RectSt r;
				r = pCntx->rDecodeRect;
				/* obtain decode rect of vu planes */
				r.h = ((r.r + r.h + 3) >> 2); (r.r >>= 2); r.h -= r.r;
				r.w = ((r.c + r.w + 3) >> 2); (r.c >>= 2); r.w -= r.c;
				DecodeUpdateChroma(pV + r.c + r.r*uVUPitch,
					r.w, r.h, uVUPitch,
					pInst->pDecodePrivate->puChromaTable);
				DecodeUpdateChroma(pU + r.c + r.r*uVUPitch,
					r.w, r.h, uVUPitch,
					pInst->pDecodePrivate->puChromaTable);
			}
		}


		/* If View Rect is entire picture, *or* if the view rect's dimensions
		 * are such that it could be the entire picture, don't do local decode.
		 * Adjust picture dimensions accordingly. */
	
		if (pCntx->rViewRect.w  == pInst->dImageDim.w &&
			pCntx->rViewRect.h == pInst->dImageDim.h) {
			uWidth  = pInst->dImageDim.w;
			uHeight = pInst->dImageDim.h; 
		} else if ((pCntx->rViewRect.c & 0x1f) ||
				   (pCntx->rViewRect.r & 0x3) ||
			 	   (pCntx->rViewRect.w & 0x1f) ||
			 	   (pCntx->rViewRect.h & 0x3)) {
		/* Need to process rColorConvertRect; set pointers up */
			U32 c, r;
			c = pOutput->pioOutputData.rCCRect.c;
			r = pOutput->pioOutputData.rCCRect.r;
			uWidth  = pOutput->pioOutputData.rCCRect.w;
			uHeight = pOutput->pioOutputData.rCCRect.h;
			pY += uYPitch*r + c;
			pV += uVUPitch*(r>>2) + (c>>2);
			pU += uVUPitch*(r>>2) + (c>>2);
		} else {
			/* View Rect is situated perfectly to decode ONLY view rect...
			 * set up width, height, Y, V, and U pointers accordingly
			 */
			pCntx->pLDMask = NULL;
			uWidth  = pCntx->rViewRect.w;
			uHeight = pCntx->rViewRect.h;

			pY += (uYPitch * pCntx->rViewRect.r) + pCntx->rViewRect.c;
			pV += (uVUPitch * (pCntx->rViewRect.r>>2))
				+ (pCntx->rViewRect.c>>2);
			pU += (uVUPitch * (pCntx->rViewRect.r>>2))
				+ (pCntx->rViewRect.c>>2);
		}

		pOutput->pioOutputData.pu8Y = pY;
		pOutput->pioOutputData.pu8V = pV;
		pOutput->pioOutputData.pu8U = pU;

#ifdef DEBUG
/*	decoder will only decode bitstreams with the following bitstream
 *	version.
 */
		if ((U32)pCntx->u8BSVersion != BS_VERSION) {
			HivePrintF("BS Ver = %d (expected %d)\n",
				(U32)pCntx->u8BSVersion,BS_VERSION);
		}
#endif /* DEBUG */

/*	if alpha or beta use this logic */
		if (pCntx->u8BSVersion < BS_VERSION) {
		/*	This bitstream is too old, and will be rejected by this codec */
#ifdef C_PORT_KIT
#pragma message("splash indeo bitstream skew message here")
#else /* C_PORT_KIT */
			FillYVUWithBSMask(pY, pV, pU, uYPitch, uVUPitch,
			pCntx->rViewRect.h, pCntx->rViewRect.w,
			pCntx->u8GOPFlags & GOPF_YVU12, 0);
#endif /* C_PORT_KIT */
		}

		if (pCntx->u8BSVersion > BS_VERSION) {
		/*	This bitstream is too new, and cannot be decoded by this codec */
#ifdef C_PORT_KIT
#pragma message("splash indeo bitstream skew message here")
#else /* C_PORT_KIT */
			FillYVUWithBSMask(pY, pV, pU, uYPitch, uVUPitch,
			FillYVUWithBSMask(pY, pV, pU, uYPitch, uVUPitch,
			pCntx->rViewRect.h, pCntx->rViewRect.w,
			pCntx->u8GOPFlags & GOPF_YVU12, 1);
#endif /* C_PORT_KIT */
		}

		if (pInst->eMode != EM_PREROLL) {
			switch (pOutput->cfOutputFormat) {

#ifndef C_PORT_KIT
			case  CF_IF09:	/*IF09 output is requested , no further color output is necessary */
				if (!(pCntx->u8GOPFlags & GOPF_YVU12)) {  /* if input Bitstream is in YVU9 */
					/*pY = pInst->pDecodePrivate->Plane[0].pOutput; */
					/*pV = pInst->pDecodePrivate->Plane[1].pOutput; */
					/*pU = pInst->pDecodePrivate->Plane[2].pOutput; */
#ifdef __MEANTIME_HIVEDEC__
					STOPCLOCK
					if(iTimingFrame != -1)
						frametimes[iTimingFrame].precsc = elapsed;
#endif /* __MEANTIME_HIVEDEC__ */
#ifdef __MEANTIME__
					sum_csc_strides_seen += pInput->iStride;
#endif /* __MEANTIME__ */
#ifdef __MEANTIME_HIVEDEC__
					STARTCLOCK
#endif /* __MEANTIME_HIVEDEC__ */
					ComposeIF09(pInst->pDecodePrivate,
						pOutput->pu8IF09BaseAddress, uYPitch);

					if (pCntx->u8BSVersion < BS_VERSION) {
/*	This bitstream is too old, and will be rejected by this codec */
						pY = pOutput->pu8IF09BaseAddress;
						pV = pY + uYPitch * pCntx->Plane[0].rDecoded.h;
						pU = pV + uVUPitch * (pCntx->Plane[0].rDecoded.h/4);
						FillYVUWithBSMask(pY, pV, pU,	uYPitch, uVUPitch,
							pCntx->rViewRect.h, pCntx->rViewRect.w,	FALSE, 0);
					}

					if (pCntx->u8BSVersion > BS_VERSION) {
/*	This bitstream is too new, and cannot be decoded by this codec */
						pY = pOutput->pu8IF09BaseAddress;
						pV = pY + uYPitch * pCntx->Plane[0].rDecoded.h;
						pU = pV + uVUPitch * (pCntx->Plane[0].rDecoded.h/4);
						FillYVUWithBSMask(pY, pV, pU, uYPitch, uVUPitch,
							pCntx->rViewRect.h, pCntx->rViewRect.w,	FALSE, 1);
					}

#ifdef __MEANTIME_HIVEDEC__
					STOPCLOCK
					if(iTimingFrame != -1){
						frametimes[iTimingFrame].csc = elapsed;
						STARTCLOCK
						frametimes[iTimingFrame].finish_lo = startlow;
						frametimes[iTimingFrame].finish_hi = starthigh;
					}
#endif /* __MEANTIME_HIVEDEC__ */

				} else { /* if input BS is in YVU12, then we can't get into YVU9 */
					prs = 	PIA_S_BAD_COLOR_FORMAT;
					goto end;
				}
				break;
#endif /* C_PORT_KIT */

			case CF_PLANAR_YVU9_8BIT: /* YVU9 is requested */
						/* it is already YVU9 now,  if that matches to the bitstream.
						 * otherwise, the format is invalid 
						 */
				if (pCntx->u8GOPFlags & GOPF_YVU12)	{
					prs = 	PIA_S_BAD_COLOR_FORMAT;
					goto end;
				}
				break;
			default: 
				prs = 	PIA_S_BAD_COLOR_FORMAT;
				goto end;
				break;
			}
			
		}


#ifdef __MEANTIME__
		STOPCLOCK
		if(iTimingFrame != -1)
			frametimes[iTimingFrame].precsc = elapsed;
		sum_csc_strides_seen += pInput->iInputStride;

		STARTCLOCK
#endif

#ifdef MEMDEBUG
	fprintf(fp, "Frame %d \n", pCntx->uFrameNumber);
#endif
/* LEW_PERS */
	/*	update frame types decoded in persistant data */
		if (	(pCntx->uFrameType == PICTYPE_P) ||
				(pCntx->uFrameType == PICTYPE_P2) ||
				(pCntx->uFrameType == PICTYPE_D)
		) {
			pInst->pPersData->bDeltaFrames = TRUE;
		}
		pInst->pPersData->bFrameDecoded = TRUE;

	}

end:

#ifdef __MEANTIME__
	STOPCLOCK
	if(iTimingFrame != -1)
		frametimes[iTimingFrame].rtdecompress = elapsed;
	STARTCLOCK
#endif

	return prs;
} /* DecodeFrame */

/*
 * DecodeFrameComplete -- Update Scalability statistics after
 * Color Conversion has been performed.
 */
PIA_RETURN_STATUS 
DecodeFrameComplete(PTR_DEC_INST pInst)
{
#if 1
	return RTDecompressFrameEnd(pInst->pDecodePrivate
		,pInst->eMode
		,pInst->uMaxFrameDuration
		);
#else	/*0*/
	return PIA_S_OK;
#endif /*0*/
}

/*
 * DecodeFreePrivateData -- Free private data and close the instance
 *	Free the context, the tables, and the private data structure
 */
PIA_RETURN_STATUS
DecodeFreePrivateData(PTR_DEC_INST pInst)
{
	PIA_RETURN_STATUS prs;

#ifdef POINTER_DEBUG
	if ((HiveGlobalPtrCheck(pInst, sizeof(DEC_INST)) != PIA_S_OK) ||
		((pInst->uTag != DEC_INST_TAG) &&
		 (pInst->uTag != DEC_PARTIAL_INST_TAG))) {
		prs = PIA_S_ERROR; 				   /* sanity check */
		goto end;
	}
#endif

	if (pInst->pDecodePrivate == NULL) {
		prs = PIA_S_ERROR;
		goto end;
	}

	SafeHiveGlobalFreePtr(pInst->pDecodePrivate->puLumaTable);
	SafeHiveGlobalFreePtr(pInst->pDecodePrivate->puChromaTable);
	SafeHiveGlobalFreePtr(pInst->pDecodePrivate->puBrightTable);
	SafeHiveGlobalFreePtr(pInst->pDecodePrivate->puContrastTable);

	if (pInst->pDecodePrivate != NULL) {
		prs = RTDecompressEnd(pInst->pDecodePrivate); 
		pInst->pDecodePrivate = NULL; 
		if (prs != PIA_S_OK)
			goto end;
	}

	if (pInst->pPersData) {
		prs = HiveLocalFreePtr(pInst->pPersData);
		pInst->pPersData = NULL;
		if (prs != PIA_S_OK)
			goto end;
	}

	prs = PIA_S_OK;
end:
	return prs;
} /* DecodeFreePrivateData */


/*
 * DecodeGetCompressedSize -- Number of bytes of compressed data
 */
PIA_RETURN_STATUS
DecodeGetCompressedSize(COLOR_FORMAT	cfInputFormat,
						PU8				pu8CompressedData,
						PU32			puSize)
{
	PIA_RETURN_STATUS	prs;

#ifdef POINTER_DEBUG	   	/* sanity check */
	if (HiveGlobalPtrCheck(pu8CompressedData, sizeof(U8)) != PIA_S_OK) {
		prs = PIA_S_ERROR;
		goto end;
	}
#endif

	if (cfInputFormat == CF_IV50) {
		U32 gopsize = 0;
		U8 picflags;

		if (!(pu8CompressedData[0]&0xe0)) {	/* if keyframe */
			U8 gopflags;

			gopflags = pu8CompressedData[2];
			if (gopflags & GOPF_HAS_SIZE) {
				gopsize = *(PU32)(&pu8CompressedData[3]) & 0xffff;
			}
			else {
				*puSize = 0;
				prs = PIA_S_ERROR;
				goto end;
			}
		}

		picflags = pu8CompressedData[2+gopsize];
		if (picflags & PICF_DATASIZE) {
			*puSize = *(PU32)(&pu8CompressedData[3+gopsize]) & 0xffffff;
		}
		else {
			*puSize = 0;
			prs = PIA_S_ERROR;
			goto end;
		}
	} else {
		prs = PIA_S_BAD_COLOR_FORMAT;
		goto end;
	}

	prs = PIA_S_OK;

end:
	return prs;
} /* DecodeGetCompressedSize */





/*
 * DecodeShutdown -- Last time called, all instances closing
 */
PIA_RETURN_STATUS
DecodeShutdown()
{
	PIA_RETURN_STATUS	prs = PIA_S_OK;
#ifndef C_PORT_KIT
	U32					i;
#endif /* C_PORT_KIT */

#ifdef C_PORT_KIT
	DeInitDecodeTables2();
#else /* C_PORT_KIT */
	DeInitDecodeTables();
#endif /* C_PORT_KIT */

#ifndef C_PORT_KIT
	for (i = 0; i < 2; i++) {
		if (pBSMaskY[i]) {
			prs = HiveFreeSharedPtr(pBSMaskY[i]);
			pBSMaskY[i] = NULL;
			if (prs != PIA_S_OK) {
				goto bail;
			}
		}
	} /* for i */
#endif /* C_PORT_KIT */

	if (uNInstances) uNInstances--;

#ifdef MEMDEBUG
	fclose(fp);
#endif

#ifndef C_PORT_KIT
bail:
#endif /* C_PORT_KIT */
	return prs;
} /* DecodeShutDown */

/**************************************************************************
 *
 *  DecodeDebugGet 
 *
 *  Special debug entry point needed by the test group.  The data pointed
 *  to by vfpVoidPtr is defined by an include file that is shared by the
 *  test application and the debugger.
 */
PIA_RETURN_STATUS 
DecodeDebugGet(
	PTR_DEC_INST pInst,
	void FAR * vfpVoidPtr)
{
	/* Return any diagnostic information you want here */

	return PIA_S_TBD;
}

/**************************************************************************
 *
 *  DecodeDebugSet 
 *
 *  Special debug entry point needed by the test group.	The data pointed
 *  to by vfpVoidPtr is defined by an include file that is shared by the
 *  test application and the debugger.
 */
PIA_RETURN_STATUS 
DecodeDebugSet(
	PTR_DEC_INST pInst,
	void FAR * vfpVoidPtr)
{
	return PIA_S_TBD;
}


/*
 * DecodeIsKeyFrame
 *	sets pbIsKeyFrame to 1 if the frame is a key frame, to 0 if not.
 *  This information is stored in  the Bitstream 
 */
PIA_RETURN_STATUS	
DecodeIsKeyFrame(
	PTR_DEC_INST pInst, 
	PU8 pu8CompressedData, 
	PPIA_Boolean pbIsKeyFrame
)
{
	PIA_RETURN_STATUS prs;

#ifdef POINTER_DEBUG
	if ((HiveGlobalPtrCheck(pInst, sizeof(DEC_INST)) != PIA_S_OK) ||
		(HiveGlobalPtrCheck(pu8CompressedData, sizeof(U8)) != PIA_S_OK) ||
		(HiveGlobalPtrCheck(pbIsKeyFrame, sizeof(U32)) != PIA_S_OK) ||
		(pInst->uTag != DEC_INST_TAG) ) {
		prs = PIA_S_ERROR; /* sanity check */
		goto end;
	}
#endif

    *pbIsKeyFrame = (!(pu8CompressedData[0]&0xe0));  /* 0 is K */

	prs = PIA_S_OK;
#ifdef POINTER_DEBUG
	end:
#endif
	return prs;
}


/*
 * DecodeQuery -- Can the decoder do this situation?
 *	Verify the controls are OK (PIA_S_BAD_CONTROL_VALUE,
 *								PIA_S_UNSUPPORTED_CONTROL)
 *	Verify that the output destination is appropriate (PIA_S_BAD_DESTINATION)
 *	Verify that the resizing amount is valid (PIA_S_BAD_CONTROL_VALUE)
 *	Verify the color representation for the output (PIA_S_BAD_COLOR_FORMAT)
 *	Set a time estimate
 */
PIA_RETURN_STATUS 
DecodeQuery(
	PTR_DEC_INST					pInst, 
	PTR_DECODE_FRAME_INPUT_INFO		pInput,
	PTR_DECODE_FRAME_OUTPUT_INFO	pOutput,
	U32								uHeight,
	U32								uWidth,
	PIA_Boolean							bSpeedOverQuality
)
{
	PIA_RETURN_STATUS		prs = PIA_S_OK;
	I32						i;
	SUPPORTED_ALGORITHMS	listFormats;
	PIA_Boolean					bValidFormat;

#ifdef POINTER_DEBUG
	if ((HiveGlobalPtrCheck(pInst, sizeof(DEC_INST)) != PIA_S_OK) ||
		((pInst->uTag != DEC_PARTIAL_INST_TAG) &&
		 (pInst->uTag != DEC_INST_TAG))) {
		prs = PIA_S_ERROR;						 /* sanity check */
		goto end;
	}
#endif

	pInst->dcfcBadFrameControls = DC_FRAME_FLAGS_VALID;
	pInst->dcscBadSequenceControls = DC_SEQUENCE_FLAGS_VALID;

	/* Make sure unsupported controls aren't set and the flags are valid */
	if (!(pInst->dcfcFrameControlsUsed & DC_FRAME_FLAGS_VALID))  {
		prs = PIA_S_BAD_CONTROL_VALUE;
		goto end;
	}
	if (!(pInst->dcscSequenceControlsUsed & DC_SEQUENCE_FLAGS_VALID))  {
		prs = PIA_S_BAD_CONTROL_VALUE;
		goto end;
	}

	if (pInst->dcscSequenceControlsUsed & ~(pInst->dciInfo.dcscSupportedSequenceControls)) {
		prs = PIA_S_UNSUPPORTED_CONTROL;
		goto end;
	}

	if (pInst->dcfcFrameControlsUsed & ~(pInst->dciInfo.dcfcSupportedFrameControls)) {
		prs = PIA_S_UNSUPPORTED_CONTROL;
		goto end;
	}

	/* Confirm destination OK -- only accept BUFFER */
	if (pOutput->odDestination == DEST_QUERY)
		pOutput->odDestination = DEST_BUFFER;
	else if (pOutput->odDestination != DEST_BUFFER) {
		prs = PIA_S_BAD_DESTINATION;
		goto end;
	}

	/* Check Color Format: both input and output */
	if (pOutput->cfOutputFormat == CF_QUERY)	{
		if (bSpeedOverQuality)
			pOutput->cfOutputFormat = CF_PLANAR_YVU9_8BIT;
		else
		/* ? CF_PLANAR_YVU12_8BIT */
			pOutput->cfOutputFormat = CF_PLANAR_YVU9_8BIT;
	}


	listFormats = pInst->dciInfo.listInputFormats;
	if (listFormats.u16Tag == SUPPORTED_ALGORITHMS_TAG) {
		bValidFormat = FALSE;
		for (i=0;i<listFormats.u16NumberOfAlgorithms; i++) {
			if (pInput->cfInputFormat == listFormats.eList[i] ) {
				bValidFormat = TRUE;
				break;
			}
		}
		if (!bValidFormat) {
			prs = PIA_S_BAD_COLOR_FORMAT;
			goto end;
		}
	} else {
		prs = PIA_S_ERROR;
		goto end;
	}

	listFormats = pInst->dciInfo.listOutputFormats;
	if (listFormats.u16Tag == SUPPORTED_ALGORITHMS_TAG) {
		bValidFormat = FALSE;
		for (i=0;i<listFormats.u16NumberOfAlgorithms; i++) {
			if (pOutput->cfOutputFormat == listFormats.eList[i] ) {
				bValidFormat = TRUE;
				break;
			}
		}
		if (!bValidFormat) {
			prs = PIA_S_BAD_COLOR_FORMAT;
			goto end;
		}
	} else {
		prs = PIA_S_ERROR;
		goto end;
	}

end:
	return prs;
} /* DecodeQuery */


/*
 * DecodeSequenceSetup -- called before a change in decompression inputs
 *	Verify DecodeQuery is OK  (any DecodeQuery results)
 * 	Determine if zooming or not
 *	If a palette has been set and the CLUT tables not built, build them
 *			(PIA_S_OUT_OF_MEMORY if not able to alloc temp storage)
 */
PIA_RETURN_STATUS 
DecodeSequenceSetup(
	PTR_DEC_INST					pInst, 
	PTR_DECODE_FRAME_INPUT_INFO		pInput,
	PTR_DECODE_FRAME_OUTPUT_INFO	pOutput
)
{
	PIA_RETURN_STATUS prs = PIA_S_OK;
	U32 uSize;

#ifdef POINTER_DEBUG
	if ((HiveGlobalPtrCheck(pInst, sizeof(DEC_INST)) != PIA_S_OK) ||
		(pInst->uTag != DEC_INST_TAG)) {
		prs = PIA_S_ERROR; 				   /* sanity check */
		goto end;
	}
#endif

	if ((prs = DecodeQuery(pInst,
						   pInput,
						   pOutput,
						   pInst->dImageDim.h,
						   pInst->dImageDim.w, 
						   TRUE)) != PIA_S_OK)
		goto end;  /* DecodeQuery not called first */

	/* allocate decoder output buffer */
	switch (pOutput->cfOutputFormat) {
#ifndef C_PORT_KIT
	case CF_IF09:
#endif /* C_PORT_KIT */
	case  CF_PLANAR_YVU9_8BIT:
		uSize = pInst->dImageDim.h * pInst->dImageDim.w * 2;  /* really just need 9/8 */
		break;
#ifdef SIMULATOR
	case CF_PLANAR_YVU12_8BIT:
		uSize = pInst->dImageDim.h * pInst->dImageDim.w * 2;  /* really just need 12/8 */
		break;
#endif
	}

 	{	/* prealloc & preinitialize decoder video data buffers, using
		 * persistent decoder data.
		 */
		PTR_DECODER_PERS_DATA pPersData = pInst->pPersData;
		PointSt	tFrameSize;
		PointSt tTileSize;
		RectSt	rDecodeRect;
		PointSt	tUVSubsample;
		U32		uNYBands;

		/* at this point the frame size is known. 
		 */
		tFrameSize.c = pInst->dImageDim.w;
		tFrameSize.r = pInst->dImageDim.h;
		
		/* if tile size in persistent data is 0, or larger than the
		 * frame size, use frame size.
		 */
		if (	(pPersData->uTileWidth == 0) ||
				(pPersData->uTileWidth > tFrameSize.c) ||
				(pPersData->uTileHeight == 0) ||
				(pPersData->uTileHeight > tFrameSize.r)) {
			tTileSize = tFrameSize;
		}
		else {
			tTileSize.c = pPersData->uTileWidth;
			tTileSize.r = pPersData->uTileHeight;
		}

		/* decode rect; entire frame for now */
		rDecodeRect.c = 0;
		rDecodeRect.r = 0;
		rDecodeRect.w = tFrameSize.c;
		rDecodeRect.h = tFrameSize.r;
		tUVSubsample.c = 4;				/* YVU9 */
		tUVSubsample.r = 4;

		/* scalability off, 1 Y band; on, 4 Y bands */
		uNYBands = (pPersData->uNLevels == PD_SCALE0) ? 1 : 4;

		/* ignore return status which is only not OK when alloc failed; this
		 * is not fatal as allocs will be reattempted during first frame
		 */

		prs = DecBufferSetup( pInst->pDecodePrivate, tFrameSize, tTileSize,
			uNYBands, rDecodeRect, tUVSubsample, pPersData->bTransparency,
			FALSE, pPersData->bDeltaFrames );

		/* set persistent data frame types to FALSE. They will be updated to
		 * TRUE if the frame types occur in this sequence. Also mark data as
		 * used for init so that a request to set the data can be rejected.
		 */
		pPersData->bDeltaFrames = FALSE;
		pPersData->bPDUsed = TRUE;

	}

	end:
		return prs;
}


/*
 * DecodeSequenceEnd -- Ending this sequence
 */
PIA_RETURN_STATUS
DecodeSequenceEnd(PTR_DEC_INST pInst)
{
	return PIA_S_OK;
}


/* access key */

PIA_RETURN_STATUS
DecodeSetAccessKey( PTR_DEC_INST pInst, U32 uAccessKey )
{
	PIA_RETURN_STATUS prs;
	pInst->pDecodePrivate->uAccessKey = uAccessKey;
	pInst->pDecodePrivate->bUseAccessKey =
		pInst->dcscSequenceControlsUsed & DC_ACCESS_KEY;

	prs = PIA_S_OK;
	return prs;
}

PIA_RETURN_STATUS
DecodeGetAccessKeyStatus( PTR_DEC_INST pInst, PPIA_Boolean pbAccessKeyViolation )
{
	PIA_RETURN_STATUS prs;
	*pbAccessKeyViolation = pInst->pDecodePrivate->bKeyFailure;
	prs = PIA_S_OK;
	return prs;
}

/* bounding rectangle */
PIA_RETURN_STATUS	
DecodeGetBoundingRectangle( PTR_DEC_INST pInst, PRectSt pRect )
{
	PIA_RETURN_STATUS prs = PIA_S_OK;

	if (pInst->pDecodePrivate->bValidBoundingRect) {
		*pRect = pInst->pDecodePrivate->rBoundRect;
	} else {
		*pRect = pInst->pDecodePrivate->rViewRect;
		prs = PIA_S_ERROR;
	}

	return prs;
}

/* decode rectangle */
PIA_RETURN_STATUS	
DecodeGetDefDecodeRect( PTR_DEC_INST pInst, PRectSt pDecodeRect )
{
	PIA_RETURN_STATUS prs = PIA_S_OK;

	pDecodeRect->r = pDecodeRect->c = 0;
	pDecodeRect->h = pInst->pDecodePrivate->tFrameSize.r;
	pDecodeRect->w = pInst->pDecodePrivate->tFrameSize.c;

	return prs;
}


PIA_RETURN_STATUS	
DecodeGetDecodeRect( PTR_DEC_INST pInst, PRectSt pDecodeRect )
{
	PIA_RETURN_STATUS prs = PIA_S_OK;

	*pDecodeRect = pInst->pDecodePrivate->rDecodeRect;

	return prs;
}

PIA_RETURN_STATUS	
DecodeSetDecodeRect( PTR_DEC_INST pInst, RectSt rDecodeRect )
{
	PIA_RETURN_STATUS prs;
	pInst->pDecodePrivate->rDecodeRect = rDecodeRect;

	prs = PIA_S_OK;
	return prs;
}


/*
 * DecodeGetDefaultPersData -- return default persistent data values
 */
PIA_RETURN_STATUS	
DecodeGetDefPersData(
	PTR_DECODER_PERS_DATA pPersData
)
{
	PIA_RETURN_STATUS prs = PIA_S_OK;

/* LEW_PERS */
	pPersData->uNLevels = PD_SCALE0;
/*	pPersData->bBidir = FALSE; */
	pPersData->bDeltaFrames = TRUE;
	pPersData->bTransparency = FALSE;
	pPersData->uTileWidth = 0;
	pPersData->uTileHeight = 0;

	return prs;
}


/*
 * DecodeGetPersData
 *  Return updated persistent data, retrieved from decode context
 *  except for presence of P, P2, and D; these booleans are
 *  updated in the instance persistent data if those frame types
 *  are decoded. Returns error if no frames have been decoded.
 */
PIA_RETURN_STATUS	
DecodeGetPersData(
	PTR_DEC_INST pInst,
	PTR_DECODER_PERS_DATA pPersData
)
{
	PIA_RETURN_STATUS	prs = PIA_S_OK;

	if (pInst->pPersData && pInst->pPersData->bFrameDecoded) {
		pPersData->uNLevels = pInst->pDecodePrivate->Plane[0].uNBands == 1 ? PD_SCALE0 : PD_SCALE1;
		pPersData->bTransparency = (pInst->pDecodePrivate->uFlags & PL_CODE_TRANSP) != 0;
		pPersData->bAccessKeyInBS = (pInst->pDecodePrivate->u8GOPFlags & GOPF_LOCKWORD);
		pPersData->uTileWidth = pInst->pDecodePrivate->tTileSize.c;
		pPersData->uTileHeight = pInst->pDecodePrivate->tTileSize.r;
		pPersData->bPDUsed = pInst->pPersData->bPDUsed;
		pPersData->bDeltaFrames = pInst->pPersData->bDeltaFrames;
		pPersData->bFrameDecoded = pInst->pPersData->bFrameDecoded;
	}
	else {
		prs = PIA_S_ERROR;
	}

	return prs;
}


/*
 * DecodeSetPersData
 *  Transfer the contents of the input persistent data structure
 *  to the persistent data in the decoder instance, checking each
 *  value for validity before doing so. Return error if the decoder
 *  has already been initialized (bPDUsed true).
 */
PIA_RETURN_STATUS	
DecodeSetPersData(
	PTR_DECODER_PERS_DATA pInputPersData,
	PTR_DECODER_PERS_DATA pInstPersData
)
{
	PIA_RETURN_STATUS prs = PIA_S_OK;

	if ( pInstPersData->bPDUsed ) {
		prs = PIA_S_ERROR;
	}
	else {
		if ((pInputPersData->uNLevels == PD_SCALE0) ||
			 (pInputPersData->uNLevels == PD_SCALE1))
			pInstPersData->uNLevels = pInputPersData->uNLevels;

		/* no validity checks on booleans */
/*		pInstPersData->bBidir = pInputPersData->bBidir; */
		pInstPersData->bDeltaFrames = pInputPersData->bDeltaFrames;
		pInstPersData->bTransparency = pInputPersData->bTransparency;

		/* tile width & height must be either zero (use pic size); or
		 * an exact multiple; and less than max. Both must be valid to
		 * use either.
		 */
#define PD_TILESIZE_MAX 256
#define PD_TILESIZE_MULT 64
		if (	(pInputPersData->uTileWidth == 0) &&
				(pInputPersData->uTileHeight == 0)) {
			pInstPersData->uTileWidth = pInputPersData->uTileWidth;
			pInstPersData->uTileHeight = pInputPersData->uTileHeight;
		}
		else if ((pInputPersData->uTileWidth != 0) &&
				(pInputPersData->uTileWidth <= PD_TILESIZE_MAX) &&
				((pInputPersData->uTileWidth % PD_TILESIZE_MULT) == 0)) {
		/*	width ok, check height */
			if ((pInputPersData->uTileHeight != 0) &&
				(pInputPersData->uTileHeight <= PD_TILESIZE_MAX) &&
				((pInputPersData->uTileHeight % PD_TILESIZE_MULT) == 0)) {
			/*	both width & height ok; copy to instance */
				pInstPersData->uTileWidth = pInputPersData->uTileWidth;
				pInstPersData->uTileHeight = pInputPersData->uTileHeight;
			}
			else {
			/*	height error, mark in input struct */
				pInputPersData->uTileHeight = 0xffffffff;
				prs = PIA_S_BAD_CONTROL_VALUE;
			}
		}
		else {
			/* width error, mark in input struct */
			pInputPersData->uTileWidth = 0xffffffff;
			prs = PIA_S_BAD_CONTROL_VALUE;
		}
	}

	return prs;
}



/* Real time effects */
PIA_RETURN_STATUS	
DecodeGetDefContrast( PTR_DEC_INST pInst, PI32 piContrast)
{
	PIA_RETURN_STATUS prs;
	prs = PIA_S_OK;
	return prs;
}

PIA_RETURN_STATUS	
DecodeGetContrast( PTR_DEC_INST pInst, PI32 piContrast)
{
	PIA_RETURN_STATUS prs;
	prs = PIA_S_OK;
	return prs;
}

PIA_RETURN_STATUS	
DecodeSetContrast( PTR_DEC_INST pInst, I32 iContrast )
{
	PIA_RETURN_STATUS prs;
	if (iContrast)
		prs = DecodeChangeContrast(pInst, iContrast);
 	else
	 	prs = DecodeResetContrast(pInst);
	return prs;
}


PIA_RETURN_STATUS	
DecodeGetDefGamma( PTR_DEC_INST pInst, PI32 piGamma)
{
	PIA_RETURN_STATUS prs;
	prs = PIA_S_OK;
	return prs;
}

PIA_RETURN_STATUS	
DecodeGetGamma( PTR_DEC_INST pInst, PI32 piGamma)
{
	PIA_RETURN_STATUS prs;
	prs = PIA_S_UNSUPPORTED_FUNCTION;
	return prs;
}

PIA_RETURN_STATUS	
DecodeSetGamma( PTR_DEC_INST pInst, I32 iGamma)
{
	PIA_RETURN_STATUS prs;
	prs = PIA_S_UNSUPPORTED_FUNCTION;
	return prs;
}


PIA_RETURN_STATUS	
DecodeGetDefSaturation( PTR_DEC_INST pInst, PI32 piSat )
{
	PIA_RETURN_STATUS prs;
	prs = PIA_S_OK;
	return prs;
}

PIA_RETURN_STATUS	
DecodeGetSaturation( PTR_DEC_INST pInst, PI32  piSat)
{
	PIA_RETURN_STATUS prs;
	prs = PIA_S_OK;
	return prs;
}

PIA_RETURN_STATUS	
DecodeSetSaturation( PTR_DEC_INST pInst, I32 iSat )
{
	PIA_RETURN_STATUS prs;
	if (iSat)
		prs = DecodeChangeSaturation(pInst, iSat);
	else
	 	prs = DecodeResetSaturation(pInst);
	return prs;
}


PIA_RETURN_STATUS	
DecodeGetDefBrightness( PTR_DEC_INST pInst, PI32 piBrightness)
{
	PIA_RETURN_STATUS prs;
	prs = PIA_S_OK;
	return prs;
}

PIA_RETURN_STATUS
DecodeGetBrightness( PTR_DEC_INST pInst, PI32 piBrightness)
{
	PIA_RETURN_STATUS prs;
	prs = PIA_S_OK;
	return prs;
}

PIA_RETURN_STATUS
DecodeSetBrightness( PTR_DEC_INST pInst, I32 iBrightness)
{
	PIA_RETURN_STATUS prs;
	if (iBrightness)
		prs = DecodeChangeBrightness(pInst, iBrightness);
	else
		prs = DecodeResetBrightness(pInst);
	return prs;
}

/* view rectangle */

PIA_RETURN_STATUS
DecodeSetViewRect( PTR_DEC_INST pInst, RectSt ViewRect)
{
	PIA_RETURN_STATUS prs;
	if ((ViewRect.r != pInst->pDecodePrivate->rViewRect.r) ||
		(ViewRect.c != pInst->pDecodePrivate->rViewRect.c) ||
		(ViewRect.h != pInst->pDecodePrivate->rViewRect.h) ||
		(ViewRect.w != pInst->pDecodePrivate->rViewRect.w)) {
	/* reset local decode mask upon view rect change */
		pInst->pDecodePrivate->pLDMask = NULL;
	}
	pInst->pDecodePrivate->rViewRect = ViewRect;
	prs = PIA_S_OK;
	return prs;
}

/* transparency fill color */

PIA_RETURN_STATUS
DecodeGetDefTransFillColor( PTR_DECODE_FRAME_OUTPUT_INFO pOutput,
	PTR_BGR_ENTRY puTransFillColor)
{
	PIA_RETURN_STATUS prs;
	/* default xpar color */
	*puTransFillColor = pOutput->pioOutputData.bgrXFill;
	prs = PIA_S_OK;
	return prs;
}

DecodeSetTransFillColor( PTR_DECODE_FRAME_OUTPUT_INFO pOutput,
	BGR_ENTRY uTransFillColor)
{
	PIA_RETURN_STATUS prs;
	pOutput->pioOutputData.bgrXFill = uTransFillColor;
	prs = PIA_S_OK;
	return prs;
}

DecodeGetTransFillColor( PTR_DECODE_FRAME_OUTPUT_INFO pOutput,
	PTR_BGR_ENTRY puTransFillColor)
{
	PIA_RETURN_STATUS prs;
	/* current xpar color */
	*puTransFillColor = pOutput->pioOutputData.bgrXFill;
	prs = PIA_S_OK;
	return prs;
}

PIA_RETURN_STATUS
DecodeSetTransparencyKind( PTR_DEC_INST pInst, TRANSPARENCY_KIND tk)
{
	PIA_RETURN_STATUS prs = PIA_S_OK;
	pInst->pDecodePrivate->htkTransparencyKind = tk;
	return prs;
}

