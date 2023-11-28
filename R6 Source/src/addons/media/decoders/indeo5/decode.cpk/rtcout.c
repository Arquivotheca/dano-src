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

/*
 *	RTCOUT.C -- high-level color out functions, called from HIVE
 *
 *		ColorOutStartup
 *		ColorOutConstantInit
 *		ColorOutImageDimInit
 *
 *		ColorOutQuery
 *
 *		ColorOutSequenceSetup
 *
 *		ColorOutFrame
 *
 *		ColorOutSequenceEnd
 *		ColorOutFreePrivateData
 *		ColorOutShutdown
 *
 * bounding rectangle
 *		ColorOutSetBoundingRectangle
 *
 * view rect
 *		ColorOutGetDefViewRect
 *		ColorOutGetViewRect
 *		ColorOutSetViewRect
 *
 * Debug
 *		ColorOutDebugGet
 *		ColorOutDebugSet
 *
 * Palette stuff
 *		ColorOutSetPaletteConfiguration in cfgpal.c
 *		ColorOutUseThisPalette
 *		ColorOutGetPalette
 *		ColorOutUseFixedPalette
 *
 * transparency mask
 *		ColorOutGetDefTransparencyKind
 *		ColorOutGetTransparencyKind
 *		ColorOutSetTransparencyKind
 *
 *		ColorOutSetForeTransMask
 *		ColorOutSetCombTransMaskPointer
 */


#include <memory.h>
#include <string.h>
#include "datatype.h"
#include "pia_main.h"
#include "rtcout.h"
#include "cpk_cout.h"
#include "pia_cout.h"
#include "cpk_blk.h"
#include "xpardec.h"
#include "rtdec.h"
#include "hivedec.h"
#include "pia_dec.h"
#include "imagedef.h"

#include "yrgb8.h"

#ifdef CMD_LINE_DEC
#include "desmhand.h"

RET_DEC_SIM DecSimHandShake(PTR_DEC_INST pDecoderInst)
{
	return BUILD_NON_SIM;
}


RET_DEC_SIM ColorOutSimHandShake(PTR_CCOUT_INST pColorOutInst)
{
	return BUILD_NON_SIM;
}
#endif /* CMD_LINE_DEC */

#ifndef C_PORT_KIT
#ifdef __MEANTIME__
#include "meantime.h"
#endif /* __MEANTIME__ */
#endif

/*extern MUTEX_HANDLE gPaletteMutex; */

/*  ConvertToRGB() -
 *	color convert the data at input per the color format indicated,
 *  placing the result at output, with the indicated output pitch.
 *
 *  Capabilities: CLUT8 and RGB24 are supported.
 *  Limitations: ZOOM & Transparency support is not currently present.
 */

#ifdef DEBUG
#include <stdio.h>
extern FILE * timefile;
#endif /* DEBUG */

Boo NeedToInitRGB24 = TRUE;

#if 1
/* Arbitrary copy for if09 support */
static void datacopy(U32 xres, U32 yres, PU8 input, U32 inputpitch,
	PU8 output, U32 outputpitch);
static void datacopy(U32 xres, U32 yres, PU8 input, U32 inputpitch,
	PU8 output, U32 outputpitch) {

	U32 r,c;
	PU8 rInPtr,cInPtr;
	PU8 rOutPtr,cOutPtr;
	rInPtr = input;
	rOutPtr = output;
	for (r = 0; r < yres; r++) {
		cInPtr = rInPtr;
		cOutPtr = rOutPtr;
		for (c = 0; c < xres; c++) {
			*cOutPtr++ = *cInPtr++;
		}
		rInPtr += inputpitch;
		rOutPtr += outputpitch;
	}
}

/* fill in skip block info for if09 */
static void doskips(PU8 output, U32 count);
static void doskips(PU8 output, U32 count) {
	while (count--) {
		*output++ = 0;
	}
}
#endif /*0 */

/* Paul's code starts here: */
/***********************************************************************
 *
 *  ColorDifference
 *
 *  Return the color difference as a U32 value.
 */
static U32 
ColorDifference(
	BGR_ENTRY bgrA, 
	BGR_ENTRY bgrB)
{
	U32 uResult = 0;
	I32 iA;
	I32 iB;
	I32 iDiff;
	
	iA = (I32) bgrA.u8R; 
	iB = (I32) bgrB.u8R;
	iDiff = iA - iB;
	uResult += (iDiff * iDiff);
	
	iA = (I32) bgrA.u8G; 
	iB = (I32) bgrB.u8G;
	iDiff = iA - iB;
	uResult += (iDiff * iDiff);
	
	iA = (I32) bgrA.u8B; 
	iB = (I32) bgrB.u8B;
	iDiff = iA - iB;
	uResult += (iDiff * iDiff);

	return uResult;
} /* end ColorDifference() */



/***********************************************************************
 *                  
 *  ClosestPaletteIndex
 *
 *  This routine returns the closest palette index to the specified color.
 *  It returns an error if called when a palette is not available.
 *
 *  'C' Porting Kit: If the output color converter will use this function,
 *  then the code needs to be modified to add a test to see if the instance 
 *  is the output color converter and call the correct function. 
 */
static PIA_RETURN_STATUS ClosestPaletteIndex(
	PINSTANCE pInstance,
	BGR_ENTRY bgrColor,
	PU8 pu8Index)
{
	PTR_CCOUT_INST pInst;
	PIA_RETURN_STATUS prsResult = PIA_S_ERROR;
	BGR_PALETTE bgrPalette;
	I32 iClosestIndex;
	U32	uSmallestDifference;
	U32 uThisDifference;
	I32 iIndex;
	BGR_ENTRY bgraTable[MAX_NUMBER_OF_PALETTE_ENTRIES];

	/* Set the OUT parameter
	 */
	*pu8Index = 0;

	pInst = (PTR_CCOUT_INST) pInstance;
	if (pInst->uTag == CCOUT_INST_TAG) {
		bgrPalette.pbgrTable = &bgraTable[0];
		if (ColorOutGetPalette(pInst, &bgrPalette) == PIA_S_OK &&
		    bgrPalette.u16NumberOfEntries > 0) {
			/* Assume the first index is the closest.
			 */ 
			uSmallestDifference = ColorDifference(bgrColor,
									bgrPalette.pbgrTable[0]);
			iClosestIndex = 0;
			if (uSmallestDifference > 0) {
				/* The first was not a perfect match so keep looking 
				 */
				for (iIndex = 1; iIndex < bgrPalette.u16NumberOfEntries ; iIndex++) {
					uThisDifference = 
				 		ColorDifference(bgrColor, bgrPalette.pbgrTable[iIndex]);
					if (uThisDifference	< uSmallestDifference) {
						uSmallestDifference = uThisDifference;
						iClosestIndex = iIndex;
						if (uThisDifference == 0)
							break;
					}
				} 
			}
			*pu8Index = (U8) iClosestIndex;
			prsResult = PIA_S_OK;
		}
	}
	
	return prsResult;
} /* end ClosestPaletteIndex() */



PU8	gpu8ClutTables = 0;  /* C-decoder tables for active palette */
BGR_PALETTE gpalFixedPalette = { 0 };	/* The fixed palette - for DecodeGetPalette() */
BGR_PALETTE gpalConfigurablePalette = { 0 };	/* The configurable palette */
BGR_PALETTE gpalActivePalette = { 0 };	/* The current active palette */
MUTEX_HANDLE gPaletteMutex = 0;		/* for palette changes */
extern I32 EntryCount;
extern I32 CP_EntryCount;

U8 clip8bit[768];
PU8 clip = &clip8bit[256];


PIA_RETURN_STATUS ColorOutStartup( void ) {
	PIA_RETURN_STATUS	prs = PIA_S_OK;
	const I32			CTS = CLUT_TABLE_SIZE + TABLE_U_SIZE + TABLE_V_SIZE;
	const I32			NPE = 256;	   /* NUMBER_PALETTE_ENTRIES */
	PIA_Boolean				bTableExists;

	InitPaletteConfiguration();

	{
		I32 i;
		for (i = 0; i < 768; i++) {
			clip8bit[i] = i < 256 ? 0 : i > 511 ? 255 : i - 256;
		}
	}

	/*
	 * Warning: The memory allocated for these tables is sharable, identified
	 * in all instances by the text strings used in the following calls. If
	 * the content of the tables changes in any way the text strings need to also
	 * be changed to avoid attempting to share a table with an older version
	 * of the codec.
	 */
	gpu8ClutTables = (PU8)HiveAllocSharedPtr(CTS, (PU8)"IV50_ClutTables", &bTableExists);
	if ( gpu8ClutTables && !bTableExists )
		memset(gpu8ClutTables, 250 /* green index in windows std palette */,
		CLUT_TABLE_SIZE);

	gpalActivePalette.pbgrTable =	
		(PTR_BGR_ENTRY)HiveAllocSharedPtr(sizeof(BGR_ENTRY) * NPE, (PU8)"IV50_APTables",
										&bTableExists);
	if (  gpalActivePalette.pbgrTable && !bTableExists )
		memset(gpalActivePalette.pbgrTable, 0, sizeof(BGR_ENTRY) * NPE);

	gpalFixedPalette.pbgrTable =	
		(PTR_BGR_ENTRY)HiveAllocSharedPtr(sizeof(BGR_ENTRY) * NPE, (PU8)"IV50_FPTables",
										&bTableExists);
	if (  gpalFixedPalette.pbgrTable && !bTableExists )
		memset(gpalFixedPalette.pbgrTable, 0, sizeof(BGR_ENTRY) * NPE);

	gPaletteMutex = HiveCreateMutex((PU8)"Palette");
	if (!gpalActivePalette.pbgrTable || !gpalFixedPalette.pbgrTable ||
		!gpu8ClutTables || !gPaletteMutex)
		prs = PIA_S_ERROR;

	return prs;
} /* ColorOutStartup */


PIA_RETURN_STATUS ColorOutConstantInit( PTR_CCOUT_INST pInst ) {
	PIA_RETURN_STATUS	prs = PIA_S_OK;

#ifdef POINTER_DEBUG
	if ((HiveGlobalPtrCheck(pInst,sizeof(CCOUT_INST)) != PIA_S_OK) ||
		(pInst->uTag != 0))  {
		prs = PIA_S_ERROR; /* Sanity check pointer */
		goto end;
	}
#endif
	pInst->cociInfo.cofcSupportedFrameControls = 
		COUT_GET_PALETTE | 
		COUT_USE_THIS_PALETTE |
		COUT_USE_FIXED_PALETTE |
		COUT_RESIZING |
		COUT_BOUNDING_RECT |
		COUT_TRANSPARENCY_COLOR |
		COUT_TRANSPARENCY_STREAM_MASK |
		COUT_TRANSPARENCY_FORE_MASK |
		COUT_FRAME_FLAGS_VALID;

	pInst->cociInfo.cofcSuggestedFrameControls = 	 /*TODO:LLYANG : ? */
		COUT_GET_PALETTE | 
		COUT_USE_THIS_PALETTE |
		COUT_USE_FIXED_PALETTE |
		COUT_FRAME_FLAGS_VALID;

	pInst->cociInfo.coscSupportedSequenceControls = 
		COUT_TOP_DOWN_OUTPUT |
		COUT_BOTTOM_UP_OUTPUT |
		COUT_TRANSPARENCY_KIND |
		COUT_ALT_LINE |
		COUT_SEQUENCE_FLAGS_VALID ;

	pInst->cociInfo.coscSuggestedSequenceControls = 	 /*TODO: LLYANG ? */
		COUT_TOP_DOWN_OUTPUT |
		COUT_TRANSPARENCY_KIND |
		COUT_SEQUENCE_FLAGS_VALID ;

	pInst->cociInfo.listInputFormats.u16NumberOfAlgorithms = 1;
	pInst->cociInfo.listInputFormats.eList[0] = CF_PLANAR_YVU9_8BIT ;
	pInst->cociInfo.listInputFormats.u16Tag = SUPPORTED_ALGORITHMS_TAG;

#if 0
	pInst->cociInfo.listOutputFormats.u16NumberOfAlgorithms = 3;/*11; */
	pInst->cociInfo.listOutputFormats.eList[0]	= CF_CLUT_8;
	pInst->cociInfo.listOutputFormats.eList[1]	= CF_RGB24;
	pInst->cociInfo.listOutputFormats.eList[2]	= CF_XRGB32;  
#else
	pInst->cociInfo.listOutputFormats.u16NumberOfAlgorithms = 4;
	pInst->cociInfo.listOutputFormats.eList[0]	= CF_CLUT_8;
	pInst->cociInfo.listOutputFormats.eList[1]	= CF_RGB24;
	pInst->cociInfo.listOutputFormats.eList[2]	= CF_XRGB32;  
	pInst->cociInfo.listOutputFormats.eList[3]	= CF_BGRX32;
#endif
	pInst->cociInfo.listOutputFormats.u16Tag = SUPPORTED_ALGORITHMS_TAG;

	/* Initialize private data */
	if (pInst->pColorOutPrivate) {
		HiveGlobalFreePtr(pInst->pColorOutPrivate);
	}
	pInst->pColorOutPrivate =
		HiveGlobalAllocPtr(sizeof(PRIVATE_COLOROUT_DATA),TRUE);
	if (!pInst->pColorOutPrivate) {
		prs = PIA_S_OUT_OF_MEMORY;
		goto end;
	}

	pInst->pColorOutPrivate->bPalette = FALSE;
	pInst->pColorOutPrivate->bPalTablesBuilt = FALSE;
	pInst->pColorOutPrivate->nPaletteType = PT_FIXED;
	pInst->pColorOutPrivate->bPaletteSwitch = FALSE;
	pInst->pColorOutPrivate->bNeedsUpdate = TRUE;

	pInst->uTag = CCOUT_PARTIAL_INST_TAG;
end:
	return prs;
}


PIA_RETURN_STATUS ColorOutImageDimInit(
	PTR_CCOUT_INST pInst, 
	PTR_COLOROUT_FRAME_INPUT_INFO pInput,
	PTR_COLOROUT_FRAME_OUTPUT_INFO pOutput
) {
	PIA_RETURN_STATUS prs = PIA_S_OK;

#ifdef POINTER_DEBUG
    if ((HiveGlobalPtrCheck(pInst, sizeof(CCOUT_INST)) != PIA_S_OK) ||
        (pInst->uTag != CCOUT_PARTIAL_INST_TAG)) {
        prs = PIA_S_ERROR; /* Sanity check pointer */
        goto end;
    }
#endif

    /* Check that the image dimensions meet specifications */
    if ((pInput->dInputDim.w  > (U32)MAX_IMAGE_WIDTH) ||
        (pInput->dInputDim.h > (U32)MAX_IMAGE_HEIGHT) ||
        (pInput->dInputDim.h*pInput->dInputDim.w > (U32)MAX_IMAGE_PIXELS) ||
        (!pInput->dInputDim.w) || (!pInput->dInputDim.h) ||
        (pInput->dInputDim.w  % IMAGE_MULTIPLE)   ||
        (pInput->dInputDim.h % IMAGE_MULTIPLE)) {
        prs =  PIA_S_BAD_IMAGE_DIMENSIONS;
        goto end;
    }

    pInst->uTag = CCOUT_INST_TAG;

end:
	return prs;
}


/*
 * ColorOutQuery --
 *  Verify the controls are OK (PIA_S_BAD_CONTROL_VALUE,
 *                              PIA_S_UNSUPPORTED_CONTROL)
 *  Verify that the output destination is appropriate (PIA_S_BAD_DESTINATION)
 *  Verify that the resizing amount is valid (PIA_S_BAD_CONTROL_VALUE)
 *  Verify the color representation for the output (PIA_S_BAD_COLOR_FORMAT)
 *  Set a time estimate
 */
PIA_RETURN_STATUS ColorOutQuery(
	PTR_CCOUT_INST pInst, 
	PTR_COLOROUT_FRAME_INPUT_INFO pInput,
	PTR_COLOROUT_FRAME_OUTPUT_INFO pOutput,
	U32 uHeight,   
	U32 uWidth,
	PIA_Boolean bSpeedOverQuality
) {
	PIA_RETURN_STATUS		prs = PIA_S_OK;
    SUPPORTED_ALGORITHMS	listFormats;
    PIA_Boolean					bValidFormat;
    I32						i;

#ifdef POINTER_DEBUG
    if ((HiveGlobalPtrCheck(pInst, sizeof(CCOUT_INST)) != PIA_S_OK) ||
        ((pInst->uTag != CCOUT_PARTIAL_INST_TAG) &&
         (pInst->uTag != CCOUT_INST_TAG))) {
        prs = PIA_S_ERROR;                       /* sanity check */
        goto end;
    }
#endif

    pInst->cofcBadFrameControls = COUT_FRAME_FLAGS_VALID;
    pInst->coscBadSequenceControls = COUT_SEQUENCE_FLAGS_VALID;

    /* Make sure unsupported controls aren't set and the flags are valid */
    if (!(pInst->cofcFrameControlsUsed & COUT_FRAME_FLAGS_VALID))  {
        prs = PIA_S_BAD_CONTROL_VALUE;
        goto end;
    }
    if (!(pInst->coscSequenceControlsUsed & COUT_SEQUENCE_FLAGS_VALID))  {
        prs = PIA_S_BAD_CONTROL_VALUE;
        goto end;
    }

    if (pInst->coscSequenceControlsUsed & ~(pInst->cociInfo.coscSupportedSequenceControls)) {
        prs = PIA_S_UNSUPPORTED_CONTROL;
        goto end;
    }

    if (pInst->cofcFrameControlsUsed & ~(pInst->cociInfo.cofcSupportedFrameControls)) {
        prs = PIA_S_UNSUPPORTED_CONTROL;
        goto end;
    }

    if (pInst->cofcBadFrameControls & ~COUT_FRAME_FLAGS_VALID) {
        prs = PIA_S_UNSUPPORTED_CONTROL;
        goto end;
    }

    if (pInst->coscBadSequenceControls & ~COUT_SEQUENCE_FLAGS_VALID) {
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
    if (pOutput->cfOutputFormat == CF_QUERY)    {
        if (bSpeedOverQuality)
            pOutput->cfOutputFormat = CF_CLUT_8;
        else
            pOutput->cfOutputFormat = CF_RGB24;
    }


    listFormats = pInst->cociInfo.listInputFormats;
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

    listFormats = pInst->cociInfo.listOutputFormats;
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

    if (pInst->cofcFrameControlsUsed & COUT_RESIZING) {
        /* Check if Zooming by 2 */
        if ((pInst->dImageDim.w == pInput->dInputDim.w) &&
            (pInst->dImageDim.h == pInput->dInputDim.h) ) {
            pInst->pColorOutPrivate->uZoomX = ZOOM_NONE;  /* zoom by 1 */
            pInst->pColorOutPrivate->uZoomY = ZOOM_NONE;  /* zoom by 1 */
        }
		else if ((pInst->dImageDim.w == (pInput->dInputDim.w << 1)) &&
            (pInst->dImageDim.h == (pInput->dInputDim.h << 1)) ) {
            pInst->pColorOutPrivate->uZoomX = ZOOM_X2;  /* zoom by 2 */
            pInst->pColorOutPrivate->uZoomY = ZOOM_X2;  /* zoom by 2 */
        }
		else {
#if 1
		    pInst->cofcBadFrameControls |= COUT_RESIZING;
	        prs = PIA_S_BAD_CONTROL_VALUE;
			goto end;
#else
#pragma message("temp code till pInput assignment is fixed")
		pInput->dInputDim = pInst->dImageDim;
#endif
		}
    }
	else if ( pInst->dImageDim.w == pInput->dInputDim.w &&
                pInst->dImageDim.h == pInput->dInputDim.h) {
		pInst->pColorOutPrivate->uZoomX = ZOOM_NONE;  /* zoom by 1/2 this value */
		pInst->pColorOutPrivate->uZoomY = ZOOM_NONE;  /* initialize to no zoom */
    }
	else {
        prs = PIA_S_ERROR;  /* Should catch in Query */
        goto end;
	}

end:
	return prs;
}	/* ColorOutQuery */

PIA_RETURN_STATUS ColorOutSequenceSetup(
	PTR_CCOUT_INST pInst, 
	PTR_COLOROUT_FRAME_INPUT_INFO pInput,
	PTR_COLOROUT_FRAME_OUTPUT_INFO pOutput
) {
	PIA_RETURN_STATUS prs = PIA_S_OK;

#ifdef POINTER_DEBUG
	if ((HiveGlobalPtrCheck(pInst, sizeof(COUT_INST)) != PIA_S_OK) ||
		(pInst->uTag != CCOUT_INST_TAG)) {
		prs = PIA_S_ERROR;				/* sanity check */
		goto end;
	}
#endif

	if ((prs = ColorOutQuery(pInst, pInput, pOutput,
								pInput->dInputDim.h,
								pInput->dInputDim.w, TRUE)) != PIA_S_OK) {
		goto end;  /* DecodeQuery not called first */
	}

	pInst->pColorOutPrivate->bAltlineBlackFill = TRUE;

	/* If a palette has been set, and the palette tables not built,
	 * build the tables */
	if ( pInst->pColorOutPrivate->bPalette &&
		!pInst->pColorOutPrivate->bPalTablesBuilt &&
		(pInst->pColorOutPrivate->nPaletteType == PT_ACTIVE)) {
		prs = HiveBeginCriticalSection(gPaletteMutex,5000); /* 5 seconds */
		if (prs != PIA_S_OK)
			goto end;
		prs = ComputeDynamicClut();
		if (prs != PIA_S_OK)
			goto end;
		pInst->pColorOutPrivate->bPalTablesBuilt = TRUE;
		prs = HiveEndCriticalSection(gPaletteMutex);
		if (prs != PIA_S_OK)
			goto end;
	}

end:
	return prs;
} /* ColorOutSequenceSetup */


PIA_RETURN_STATUS ColorOutFrame(
	PTR_CCOUT_INST pInst, 
	PTR_COLOROUT_FRAME_INPUT_INFO pInput,
	PTR_COLOROUT_FRAME_OUTPUT_INFO pOutput
)
{
	U32					uZoomX, uZoomY, uWidth, uHeight;
	Boo					bTransFill;
	U32					uTransColor;
	PIA_RETURN_STATUS	prs = PIA_S_OK;
	U32					uCCFlags;
	PU8					pOutputBase;

	pRTCOutInst pCntx;

	/* static */ U32 uLastTransColor = 0x80000000; /* illegal value */
	/* static */ U32 uConvertedTrans = 0;

	/* These parms are set in CDecompress */
	pOutput->uTag = COLOROUT_FRAME_OUTPUT_INFO_TAG;

	pCntx = pInst->pColorOutPrivate;

	uZoomX = pCntx->uZoomX;
	uZoomY = pCntx->uZoomY;

 /* Fill if transparency AND fill set */
	bTransFill = (pInput->pioInputData.pu8TransMask &&
				 (pCntx->htkTransparencyKind == TK_IGNORE));


#define BGR_TO_U32(BGR) (							\
	((BGR).u8Reserved << 24) | ((BGR).u8R << 16) |	\
	((BGR).u8G << 8) | (BGR).u8B					\
	)

	uTransColor = BGR_TO_U32(pInput->pioInputData.bgrXFill);

	if (pInst->coscSequenceControlsUsed & COUT_ALT_LINE) {
		uCCFlags = 1;
		if (pCntx->bAltlineBlackFill) {
			uCCFlags |= 2;
			pCntx->bAltlineBlackFill = FALSE;
		}
	}
	else {
		uCCFlags = 0;
	}


	uWidth  = pInst->dImageDim.w;
	uHeight = pInst->dImageDim.h;
	uWidth  = pInput->pioInputData.rCCRect.w;
	uHeight = pInput->pioInputData.rCCRect.h;

/*	if (!bViewRectOrigin) */
	{
		U32 uOutputByteWidth;

		switch (pOutput->cfOutputFormat) {
			case CF_XBGR32:
			case CF_RGBX32:
			case CF_BGRX32:
			case CF_XRGB32:
				uOutputByteWidth = 4;	break;
			case CF_RGB24:
			case CF_BGR24:
				uOutputByteWidth = 3;	break;
			case CF_YUY2:
				uOutputByteWidth = 2;   break;
			default:
				uOutputByteWidth = 1;	break;
		}

		if (uZoomX == 4) {
			uOutputByteWidth *= 2;
		}
		pOutputBase =
			pOutput->pu8OutputData +
			pInput->pioInputData.rCCRect.c * uOutputByteWidth +
			pInput->pioInputData.rCCRect.r * pOutput->iOutputStride;
	}

	switch (pOutput->cfOutputFormat) {
		case CF_XBGR32:
		case CF_RGBX32:
		case CF_BGRX32:
		case CF_XRGB32:
			if(pInput->pioInputData.pu8TransMask && bTransFill)
				uConvertedTrans = uTransColor;

            if((uZoomX==2)&&(uZoomY==2))	/* Zoom by 1 */
                YVU9_to_XRGB32(uHeight, uWidth,
							pInput->pioInputData.pu8Y,
							pInput->pioInputData.pu8V,
							pInput->pioInputData.pu8U,
							pInput->pioInputData.uYPitch,
							pInput->pioInputData.uVUPitch,
							pOutputBase,
							pOutput->iOutputStride,
							pInput->pioInputData.pu8TransMask,
							pInput->pioInputData.pu8LDMask,
							pInput->pioInputData.uTransPitch,
							bTransFill,
							uConvertedTrans,
							pOutput->cfOutputFormat);
			else

				prs = PIA_S_DONT_DRAW;

			break;
		case CF_CLUT_8:
			if (pInput->pioInputData.pu8TransMask && bTransFill) { 
				/* Transparency is on and we're in fill mode --
				 * Need to compute fill color */
				/* Is the initial state of uLastTransColor illegal? */

				if (uTransColor != uLastTransColor) { 
					BGR_ENTRY bgr;
					U8 uIndex;
					bgr.u8R = (U8) ((uTransColor&0x00FF0000) >> 16);
					bgr.u8G = (U8) ((uTransColor&0x0000FF00) >> 8);
					bgr.u8B = (U8) ((uTransColor&0x000000FF));
					prs = ClosestPaletteIndex(pInst, bgr, &uIndex);
					if (prs != PIA_S_OK)
						goto end;
					uLastTransColor = uTransColor; /* Update color */
					uConvertedTrans = (U32) uIndex;
				}

				/* As a temporary fix until View Rects with fill and 
				 * transparency are worked out, do a prefill before 
				 * color conversion and never ask for fill
				 */
			}

			if ((uZoomX != 4) || (uZoomY != 4)) {
				/* clear Alt Line flags if present */
				uCCFlags &= ~(CCF_DOING_ALT_LINE | CCF_REFRESH_ALT_LINES);
				/* was: uCCFlags = 0; */
			}

			switch (pCntx->nPaletteType) {
				case PT_FIXED:
					if( (uZoomX == 2 && uZoomY == 2)||
						(uZoomX == 4 && uZoomY == 4) ){
						if((
#ifdef __MEANTIME_DECPROCI__
							frametimes[iTimingFrame].more_bogus =
#endif /*  __MEANTIME_DECPROCI__ */
							C_YVU9toCLUT8_C
							(uHeight, uWidth,
							pInput->pioInputData.pu8Y,
							pInput->pioInputData.pu8V,
							pInput->pioInputData.pu8U,
							pInput->pioInputData.uYPitch,
							pInput->pioInputData.uVUPitch,
							pOutputBase,
							pOutput->iOutputStride,
							pInput->pioInputData.pu8TransMask,
							pInput->pioInputData.pu8LDMask,
							pInput->pioInputData.uTransPitch,
							pInput->pioInputData.uTransPitch,
							bTransFill,
							uConvertedTrans,
							uZoomX, uZoomY, uCCFlags)))
							 prs = PIA_S_DONT_DRAW;
					}
					else
						prs = PIA_S_DONT_DRAW;
					break;

				case PT_ACTIVE:
					if (uZoomX == 2 && uZoomY == 2) { /* Zoom by 1 */
						C_YVU9toActivePalette
							(uHeight, uWidth,
							pInput->pioInputData.pu8Y,
							pInput->pioInputData.pu8V,
							pInput->pioInputData.pu8U,
							pInput->pioInputData.uYPitch,
							pInput->pioInputData.uVUPitch,
							pOutputBase,
							pOutput->iOutputStride,
							pInput->pioInputData.pu8TransMask,
							pInput->pioInputData.pu8LDMask,
							pInput->pioInputData.uTransPitch,
							bTransFill,
							uConvertedTrans);
					}
					else
						prs = PIA_S_DONT_DRAW;
					break;

			} /* switch nPaletteType */
			break; 

		case CF_RGB24:
			if (NeedToInitRGB24) {
				NeedToInitRGB24 = FALSE;
				YVU9_to_RGB24_Init(pOutput->cfOutputFormat, uCCFlags);
			}
			if (pInput->pioInputData.pu8TransMask && bTransFill) { 
				uConvertedTrans = uTransColor;
			}

            YVU9_to_RGB24_C(uHeight, uWidth,
				pInput->pioInputData.pu8Y,
				pInput->pioInputData.pu8V,
				pInput->pioInputData.pu8U,
				pInput->pioInputData.uYPitch,
				pInput->pioInputData.uVUPitch,
				pOutputBase,
				pOutput->iOutputStride,
				pInput->pioInputData.pu8TransMask,
				pInput->pioInputData.pu8LDMask,
				pInput->pioInputData.uTransPitch,
				pInput->pioInputData.uTransPitch,
				bTransFill,
				uConvertedTrans);
			break;

		case CF_YUY2:
			if (pInput->pioInputData.pu8TransMask && bTransFill) { 
				uConvertedTrans = uTransColor;
			}

			if (uZoomX == 2 && uZoomY == 2) { /* Zoom by 1 */
                YVU9_to_YUY2(uHeight, uWidth,
				pInput->pioInputData.pu8Y,
				pInput->pioInputData.pu8V,
				pInput->pioInputData.pu8U,
				pInput->pioInputData.uYPitch,
				pInput->pioInputData.uVUPitch,
				pOutputBase,
				pOutput->iOutputStride,
				pInput->pioInputData.pu8TransMask,
				pInput->pioInputData.pu8LDMask,
				pInput->pioInputData.uTransPitch,
                        bTransFill,
                        uConvertedTrans,
						CF_YUY2);
            } else {
				prs = PIA_S_BAD_COLOR_FORMAT;
			}
			break;

		default:
			prs = PIA_S_ERROR;
			break;
	 
	}

end:
	pInput->pioInputData.uCCFlags = uCCFlags;

	return prs;
} /* ColorOutFrame */


PIA_RETURN_STATUS ColorOutSequenceEnd( PTR_CCOUT_INST pInst ) {
	PIA_RETURN_STATUS prs = PIA_S_OK;
	return prs;
} /* ColorOutSequenceEnd */

PIA_RETURN_STATUS ColorOutFreePrivateData( PTR_CCOUT_INST pInst ) {
	PIA_RETURN_STATUS prs = PIA_S_OK;

	if (pInst->pColorOutPrivate != NULL)
		HiveGlobalFreePtr(pInst->pColorOutPrivate);

	return prs;
} /* ColorOutFreePrivateData */

PIA_RETURN_STATUS ColorOutShutdown( void ) {
	PIA_RETURN_STATUS prs = PIA_S_OK;

	DeInitPaletteConfiguration();

	if (gpalActivePalette.pbgrTable != NULL)
		HiveFreeSharedPtr(gpalActivePalette.pbgrTable);
	if (gpalFixedPalette.pbgrTable != NULL)
		HiveFreeSharedPtr(gpalFixedPalette.pbgrTable);
	if (gpu8ClutTables != NULL)
		HiveFreeSharedPtr(gpu8ClutTables);

	return HiveFreeMutex(gPaletteMutex);

	return prs;
} /* ColorOutShutdown */


/* bounding rectangle */
PIA_RETURN_STATUS ColorOutSetBoundingRectangle(
	PTR_CCOUT_INST pInst,
	RectSt rBound ) {
	PIA_RETURN_STATUS prs = PIA_S_OK;

	pInst->pColorOutPrivate->rColorConvert = rBound;

	return prs;
} /* ColorOutSetBoundingRectangle */

/* Debug */
PIA_RETURN_STATUS ColorOutDebugGet(
	PTR_CCOUT_INST pInst,
	PVOID_GLOBAL vgpVoidPtr ) {
	PIA_RETURN_STATUS prs = PIA_S_OK;
/*#pragma message("tbd") */
	return prs;
} /* ColorOutDebugGet */

PIA_RETURN_STATUS ColorOutDebugSet(
	PTR_CCOUT_INST pInst,
	PVOID_GLOBAL vgpVoidPtr ) {
	PIA_RETURN_STATUS prs = PIA_S_OK;
/*#pragma message("tbd") */
	return prs;
} /* ColorOutDebugSet */


PIA_RETURN_STATUS ColorOutUseThisPalette(
	PTR_CCOUT_INST pInst, 
	PTR_BGR_PALETTE ppalNewPalette,
	PPIA_Boolean pbFixedPaletteDetected) {
	I32					i,j;
	PIA_RETURN_STATUS	prs = PIA_S_OK;
	Boo					bSame = FALSE;

#ifdef POINTER_DEBUG
  	if ((pInst == NULL) || (ppalNewPalette == NULL) ||
		((pInst->uTag != CCOUT_INST_TAG)  &&
		 (pInst->uTag != CCOUT_PARTIAL_INST_TAG)) ||
 		(ppalNewPalette->u16Tag != BGR_PALETTE_TAG) ) {
		prs =  PIA_S_ERROR; /* sanity check */
		goto end;
	}
#endif

	prs = HiveBeginCriticalSection(gPaletteMutex,5000); /* 5 seconds */
	if (prs != PIA_S_OK)
		goto end;

	/* Loop over each non-system entry to see if it isn't PalTable */
	*pbFixedPaletteDetected = TRUE;
	for (i=SKIP_ENTRIES; i < EntryCount + SKIP_ENTRIES; i++) {
		j = (i-SKIP_ENTRIES) * 4;
		if ((ppalNewPalette->pbgrTable[i].u8R !=  PalTable[j++]) ||
			(ppalNewPalette->pbgrTable[i].u8G !=  PalTable[j++]) ||
			(ppalNewPalette->pbgrTable[i].u8B !=  PalTable[j++]) ) {
			*pbFixedPaletteDetected = FALSE;
			break; 			/* No need to continue checking values */
		}
	}

#ifdef FORCE_ACTIVE_PALETTE
	*pbFixedPaletteDetected = FALSE;
#endif

	if (*pbFixedPaletteDetected == TRUE) {
		pInst->pColorOutPrivate->nPaletteType = PT_FIXED;
	} else {
		I32 iInt, iFirst, iLast;
		BGR_ENTRY const *pPalette;

		*pbFixedPaletteDetected = TRUE;

		if((pPalette = ColorOutGetPaletteConfiguration(&iFirst,&iLast)) != NULL){
			for(iInt = iFirst; iInt <= iLast; iInt++){
				if(
			(*((U32 *)(ppalNewPalette->pbgrTable+iInt))&0x0FFF)!=
			(*((U32 *)(pPalette+iInt))&0x0FFF)){
					*pbFixedPaletteDetected = FALSE;
					break;
				}
			}
			if(*pbFixedPaletteDetected)
				pInst->pColorOutPrivate->nPaletteType = PT_CONFIGURABLE;
			ColorOutReleasePaletteConfiguration();
		}
		else
			*pbFixedPaletteDetected = FALSE;

	}
	
	if (*pbFixedPaletteDetected == FALSE) {

		/* turn Active Palette on */
		pInst->pColorOutPrivate->nPaletteType = PT_ACTIVE;

		/* If the input palette is the same as the last one, save work.. */
		if (pInst->pColorOutPrivate->bPalette) {
			bSame = TRUE;
			for (i=0; i<PALETTE_SIZE; i++) {
				if ((ppalNewPalette->pbgrTable[i].u8R !=
						gpalActivePalette.pbgrTable[i].u8R) ||
					(ppalNewPalette->pbgrTable[i].u8G !=
						gpalActivePalette.pbgrTable[i].u8G) ||
					(ppalNewPalette->pbgrTable[i].u8B != 
						gpalActivePalette.pbgrTable[i].u8B) ) {
					bSame = FALSE;
					break; 			/* No need to continue checking values */
				}
			}
		}

		if (!bSame) {
		/* Copy the passed-in palette to a local copy */
			memcpy( gpalActivePalette.pbgrTable,
					ppalNewPalette->pbgrTable,
					PALETTE_SIZE * 4); /* B, G, R, and reserved */

			gpalActivePalette.u16NumberOfEntries = ppalNewPalette->u16NumberOfEntries;
			gpalActivePalette.u16Tag = BGR_PALETTE_TAG;
		}
	}

	prs = HiveEndCriticalSection(gPaletteMutex);
	if (prs != PIA_S_OK)
		goto end;
	pInst->pColorOutPrivate->bPalette = TRUE;
#ifdef FORCE_ACTIVE_PALETTE
	pInst->pColorOutPrivate->bPalTablesBuilt = *pbFixedPaletteDetected;
#else
	pInst->pColorOutPrivate->bPalTablesBuilt = *pbFixedPaletteDetected || bSame;
#endif
	prs = PIA_S_OK;

end:

	return prs;
} /* ColorOutUseThisPalette */

/* COUT_GET_PALETTE */
PIA_RETURN_STATUS ColorOutGetPalette(
	PTR_CCOUT_INST pInst, 
	PTR_BGR_PALETTE ppalCurrentPalette) {
	PIA_RETURN_STATUS prs = PIA_S_OK;

#ifdef POINTER_DEBUG
	if ((HiveGlobalPtrCheck(pInst, sizeof(DEC_INST)) != PIA_S_OK) ||
		(HiveGlobalPtrCheck(ppalCurrentPalette, sizeof(BGR_PALETTE)) != PIA_S_OK)) {
		prs = PIA_S_ERROR;
		goto end;
	}
	if ((pInst->uTag != DEC_INST_TAG) &&
		(pInst->uTag != DEC_PARTIAL_INST_TAG)) {
		prs = PIA_S_ERROR;
		goto end;
	}
#endif

	if (!pInst->pColorOutPrivate->bPalette) {
		prs = PIA_S_ERROR; /* UseThisPalette or UseFixedPalette not called */
		goto end;
	}

	switch (pInst->pColorOutPrivate->nPaletteType) {
	case PT_FIXED:
		ppalCurrentPalette->u16Tag = gpalFixedPalette.u16Tag;
		ppalCurrentPalette->u16NumberOfEntries = gpalFixedPalette.u16NumberOfEntries;

		memcpy( ppalCurrentPalette->pbgrTable,
				gpalFixedPalette.pbgrTable,
				PALETTE_SIZE * 4); /* B, G, R, and reserved for each entry */
		break;
	case PT_CONFIGURABLE:
		{
			I32 iInt, iFirst, iLast;
			BGR_ENTRY const *pPalette;

			if((pPalette=ColorOutGetPaletteConfiguration(&iFirst,&iLast))!=NULL){
				ppalCurrentPalette->u16Tag = BGR_PALETTE_TAG;
				ppalCurrentPalette->u16NumberOfEntries = iLast - iFirst + 1;
				for(iInt = iFirst; iInt <= iLast; iInt++)
					ppalCurrentPalette->pbgrTable[iInt] = pPalette[iInt];
				ColorOutReleasePaletteConfiguration();
			}
			else
				return PIA_S_ERROR;
		}
		break;
	case PT_ACTIVE:
		ppalCurrentPalette->u16Tag = gpalActivePalette.u16Tag;
		ppalCurrentPalette->u16NumberOfEntries = gpalActivePalette.u16NumberOfEntries;

		memcpy( ppalCurrentPalette->pbgrTable,
				gpalActivePalette.pbgrTable,
				PALETTE_SIZE * 4); /* B, G, R, and reserved for each entry */
		break;
	} /* switch */

end:
	return prs;
} /* ColorOutGetPalette */

/* COUT_USE_FIXED_PALETTE Palette stuff */
PIA_RETURN_STATUS ColorOutUseFixedPalette(
	PTR_CCOUT_INST pInst, 
	PTR_BGR_PALETTE ppalCurrentPalette) {
	I32					i, j;
	PIA_RETURN_STATUS	prs = PIA_S_OK;

#ifdef FORCE_ACTIVE_PALETTE
	Boo bFixedPalette;
#endif

#ifdef POINTER_DEBUG
  	if ((pInst == NULL) || (ppalCurrentPalette == NULL)  ||
		((pInst->uTag != CCOUT_INST_TAG)  &&
		 (pInst->uTag != CCPIT_PARTIAL_INST_TAG)) ||
		(ppalCurrentPalette->u16Tag != BGR_PALETTE_TAG) ) {
		prs = PIA_S_ERROR; /* sanity check */
		goto end;
	}
#endif

	prs = HiveBeginCriticalSection(gPaletteMutex,5000); /* 5 seconds */
	if (prs != PIA_S_OK)
		goto end;

	/* Copy the passed-in palette to a local copy */
	memcpy( gpalFixedPalette.pbgrTable,
			ppalCurrentPalette->pbgrTable,
			PALETTE_SIZE * 4); /* B, G, R, and reserved */

	gpalFixedPalette.u16NumberOfEntries = ppalCurrentPalette->u16NumberOfEntries;

	/* Copy fixed palette (PalTable from cluttab.c) to middle 236 entries */
	for (i=SKIP_ENTRIES, j= 0; i < EntryCount + SKIP_ENTRIES; i++) {
		/* Assign to pInst (BGR) out of order since PalTable is RGB */
		gpalFixedPalette.pbgrTable[i].u8R = PalTable[j++];
		gpalFixedPalette.pbgrTable[i].u8G = PalTable[j++];
		gpalFixedPalette.pbgrTable[i].u8B = PalTable[j++];
		gpalFixedPalette.pbgrTable[i].u8Reserved = PalTable[j++];
	}

#ifdef FORCE_ACTIVE_PALETTE
	prs = HiveEndCriticalSection(gPaletteMutex);
	if (prs != PIA_S_OK)
		goto end;
	ColorOutUseThisPalette(pInst, &gpalFixedPalette, &bFixedPalette);
#else /* Normal operation */
	gpalFixedPalette.u16Tag = BGR_PALETTE_TAG;
	prs = HiveEndCriticalSection(gPaletteMutex);
	if (prs != PIA_S_OK)
		goto end;
	pInst->pColorOutPrivate->nPaletteType = PT_FIXED;	/* Using PalTable */
	pInst->pColorOutPrivate->bPalette = TRUE;			/* The palette's valid */
	pInst->pColorOutPrivate->bPalTablesBuilt = TRUE;	/* Don't build for fixed */
#endif

end:
	return prs;
} /* ColorOutUseFixedPalette */

/* transparency mask */
PIA_RETURN_STATUS	ColorOutGetDefTransparencyKind(
	PTR_CCOUT_INST pInst,
	PTRANSPARENCY_KIND ptkTk) {
	PIA_RETURN_STATUS prs = PIA_S_OK;
	*ptkTk = TK_IGNORE;	/* default is to fill background */
	return prs;
}

PIA_RETURN_STATUS	ColorOutGetTransparencyKind(
	PTR_CCOUT_INST pInst,
	PTRANSPARENCY_KIND ptkTk) {
	PIA_RETURN_STATUS prs = PIA_S_OK;
	*ptkTk = pInst->pColorOutPrivate->htkTransparencyKind;
	return prs;
}

PIA_RETURN_STATUS	ColorOutSetTransparencyKind(
	PTR_CCOUT_INST pInst,
	TRANSPARENCY_KIND tkTk) {
	PIA_RETURN_STATUS prs = PIA_S_OK;
	pInst->pColorOutPrivate->htkTransparencyKind = tkTk;
	return prs;
}

#if 0
PIA_RETURN_STATUS	ColorOutSetForeTransMask(
	PTR_CCOUT_INST pInst,
	PTR_TRANSPARENCY_MASK ptmTm) {
	PIA_RETURN_STATUS prs = PIA_S_OK;
#pragma message("tbd")
	return prs;
}

PIA_RETURN_STATUS	ColorOutSetCombTransMaskPointer(
	PTR_CCOUT_INST pInst,
	PTR_TRANSPARENCY_MASK ptmTM, PPIA_Boolean pbX) {
	PIA_RETURN_STATUS prs = PIA_S_OK;
#pragma message("tbd")
	return prs;
}
#endif /*0 */

PIA_RETURN_STATUS	ColorOutGetDefViewRect(
	PTR_CCOUT_INST pInst, PRectSt pViewRect ) {
	PIA_RETURN_STATUS prs = PIA_S_OK;
	pViewRect->r = 0;
	pViewRect->c = 0;
	pViewRect->h = pInst->dImageDim.h;
	pViewRect->w = pInst->dImageDim.w;
 	return prs;
}

PIA_RETURN_STATUS	ColorOutGetViewRect(
	PTR_CCOUT_INST pInst, PRectSt pViewRect ) {
	PIA_RETURN_STATUS prs = PIA_S_OK;
	*pViewRect = pInst->pColorOutPrivate->rViewRect;
	return prs;
}

PIA_RETURN_STATUS	ColorOutSetViewRect(
	PTR_CCOUT_INST pInst, RectSt rViewRect ) {
	PIA_RETURN_STATUS prs = PIA_S_OK;
	pInst->pColorOutPrivate->rViewRect = rViewRect;
	return prs;
}
