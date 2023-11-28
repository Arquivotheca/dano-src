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
*               Copyright (C) 1994-1997 Intel Corp.                       *
*                         All Rights Reserved.                          *
*                                                                       *
************************************************************************/

/*
 *  Tabs set to 4
 *
 *  icconv.c 
 *
 *  DESCRIPTION:
 *  encoder input color converter module, routines included:
 *  ColorInStartup
 *  ColorInShutdown
 *  ColorInSequenceSetup
 *  ColorInSequenceEnd  
 *  ColorInQuery
 *  ColorInFrame
 *  ColorInConstantInit
 *  ColorInImageDimInit
 *  ColorInFreePrivateData
 *  ColorInDebugGet
 *  ColorInDebugSet
 *  ColorInSetTransAlphaChannel
 *  ColorInSetCalcTransColorRange
 *  ColorInTransGetDefTransColorRange
 *  ColorInGetTransColorRange
 *  ColorInSetTransColorRange
 *  ColorInGetTransMask
 *  ColorInSetTransMask
 * 	ColorInGetTransMaskSize
 *  ColorInGetRepresentativeColor
 */

#include "datatype.h"
#include "pia_main.h"
#include "icconv.h"
#include "pia_cin.h"                    /* ColorIn subsystem include file */
#include "convert.h"                    /* ColorIn subsystem include file */

/* macro to round the dimension of frame up to next multiple of 4 */                               
#define NEXT_MULT_OF_4(x) ((((U32)(x)) + 3L) & ~(0x3L)) 

#ifdef SIMULATOR 
	/* macro to round the dimension of frame up to next multiple of 2 */                               
	#define NEXT_MULT_OF_2(x) ((((U32)(x)) + 1L) & ~(0x1L)) 
#endif /* SIMULATOR */

/* Define max and min frame size we can handle */
static const U32 FrameMinWidthSupported  = 4;
static const U32 FrameMinHeightSupported = 4;

/* macro to get YVU9 internal format size */
#define YVU9_SIZE(x, y) ((x * y * 9)/8)

#ifdef SIMULATOR 
	/* macro to get YVU12 internal format size */
	#define YVU12_SIZE(x, y) ((x * y * 12)/8)
#endif /* SIMULATOR */

/* define debug enable flag */
#ifdef DEBUG
#define DEBUG_ICCONV 1
#else
#define DEBUG_ICCONV 0
#endif

/* private routine prototype */
static PIA_RETURN_STATUS CheckAndSetInfo(PTR_CCIN_INST pInst, 
										 PTR_COLORIN_INPUT_INFO pInput,
										 PTR_COLORIN_OUTPUT_INFO pOutput,
										 U32 uWidth, U32 uHeight);

/*
 * routine to setup input color converter, this routine will be called only once!
 */                       
                                                   
PIA_RETURN_STATUS ColorInStartup()
{    
    return(PIA_S_OK);                          
}                                                  
                     
/*
 * routine to shutdown input color converter, this routine will be called only once!
 * need to free any allocated memory.
 */                                                                         

PIA_RETURN_STATUS ColorInShutdown()
{   
    return(PIA_S_OK);    
}                                                  
                     

/*
 * routine to free any allocated private memory. Since we do not need any private data, just return.
 */
                      
PIA_RETURN_STATUS ColorInFreePrivateData(PTR_CCIN_INST pInst)
{   
#if DEBUG_ICCONV
	HivePrintString("\nColorInFreePrivateData");
#endif
    /* Free any memory in here for CI color converter private usage */

    if(pInst->ciscSequenceControlsUsed & CIN_TRANSPARENCY) {
		if (pInst->pColorInParms->pTransparencyMask) {
			HiveGlobalFreePtr(pInst->pColorInParms->pTransparencyMask->pu8TransBuffer);
			HiveGlobalFreePtr(pInst->pColorInParms->pTransparencyMask);
		}
	}
	HiveGlobalFreePtr(pInst->pColorInParms);
	pInst->pColorInParms = NULL;

	if (pInst->pColorInPrivate->pu8OutputData) {
		HiveGlobalFreePtr(pInst->pColorInPrivate->pu8OutputData);
	}
	if (pInst->pColorInPrivate->pu8TempBuffer) {
		HiveGlobalFreePtr(pInst->pColorInPrivate->pu8TempBuffer);
	}

	HiveGlobalFreePtr(pInst->pColorInPrivate);
	pInst->pColorInPrivate = NULL;

	return(PIA_S_OK);    
}

/*
 * routine to do per instance constant init
 */
                      
PIA_RETURN_STATUS ColorInConstantInit(PTR_CCIN_INST pInst)
{
    PIA_RETURN_STATUS prsRtn = PIA_S_OK;

#if DEBUG_ICCONV
	HivePrintString("\nColorInConstantInit");
#endif
    if(!pInst || pInst->uTag)
        prsRtn = PIA_S_ERROR;
    else 
    {
        /* init constant informations */
        pInst->ciciInfo.ciscSupportedSequenceControls = 
			CIN_SEQUENCE_CONTROLS_VALID | CIN_TRANSPARENCY; 

	pInst->ciciInfo.listInputFormats.u16NumberOfAlgorithms = 4;
	pInst->ciciInfo.listInputFormats.eList[0] = CF_CLUT_8 ;
	pInst->ciciInfo.listInputFormats.eList[1] = CF_RGB16_565 ;
	pInst->ciciInfo.listInputFormats.eList[2] = CF_BGR24 ;
	pInst->ciciInfo.listInputFormats.eList[3] = CF_XRGB32 ;
	pInst->ciciInfo.listInputFormats.u16Tag = SUPPORTED_ALGORITHMS_TAG;

#ifdef SIMULATOR 
	pInst->ciciInfo.listOutputFormats.u16NumberOfAlgorithms = 2;
#else 
	pInst->ciciInfo.listOutputFormats.u16NumberOfAlgorithms = 1;
#endif

	pInst->ciciInfo.listOutputFormats.eList[0] = CF_PLANAR_YVU9_8BIT  ;
											
#ifdef SIMULATOR 
	pInst->ciciInfo.listOutputFormats.eList[1] = CF_PLANAR_YVU12_8BIT  ;
#endif

	pInst->ciciInfo.listOutputFormats.u16Tag = SUPPORTED_ALGORITHMS_TAG;




	/* indicate CIn state is partially initialized */
	pInst->uTag = CCIN_PARTIAL_INST_TAG;

	if (!pInst->pColorInPrivate) {
		pInst->pColorInPrivate = (PTR_PRIVATE_COLORIN_DATA) 
							   HiveGlobalAllocPtr(sizeof(ColorInPrivateData), TRUE);
	}
	
	if (!pInst->pColorInParms) {
		pInst->pColorInParms = (PTR_PASS_DOWN_COLORIN_PARMS) 
							   HiveGlobalAllocPtr(sizeof(PassDownColorInParmsSt), TRUE);
	}

	pInst->pColorInPrivate->bFirstTime = TRUE;

    }
    return(prsRtn);    
}


/*
 * routine to do per instance dimension init
 */
                      
PIA_RETURN_STATUS ColorInImageDimInit(
	PTR_CCIN_INST				pInst,
	PTR_COLORIN_INPUT_INFO		pInput, 
	PTR_COLORIN_OUTPUT_INFO		pOutput
)
{
    PIA_RETURN_STATUS prsRtn = PIA_S_OK;

#if DEBUG_ICCONV
	HivePrintString("\nColorInImageDimInit");
#endif
    if(!pInst || (pInst->uTag != CCIN_PARTIAL_INST_TAG) && 
    			 (pInst->uTag != CCIN_INST_TAG))
        prsRtn = PIA_S_ERROR;
	else
    {
    	/* check input frame size */
#ifdef SIMULATOR
		if (pOutput->cfOutputFormat == CF_PLANAR_YVU12_8BIT) {
	        if(pInst->dImageDim.w  < FrameMinWidthSupported || 
		       pInst->dImageDim.h < FrameMinHeightSupported || 
			   pInst->dImageDim.w  != NEXT_MULT_OF_4(pInst->dImageDim.w) ||
			   pInst->dImageDim.h != NEXT_MULT_OF_4(pInst->dImageDim.h))
	            prsRtn = PIA_S_BAD_IMAGE_DIMENSIONS;
			else
				/* indicate CIn state is fully initialized */
				pInst->uTag = CCIN_INST_TAG;

		} else {
	        if(pInst->dImageDim.w  < FrameMinWidthSupported || 
		       pInst->dImageDim.h < FrameMinHeightSupported || 
			   pInst->dImageDim.w  != NEXT_MULT_OF_4(pInst->dImageDim.w) ||
			   pInst->dImageDim.h != NEXT_MULT_OF_4(pInst->dImageDim.h))
	            prsRtn = PIA_S_BAD_IMAGE_DIMENSIONS;
			else
				/* indicate CIn state is fully initialized */
				pInst->uTag = CCIN_INST_TAG;
		}
#else
        if(pInst->dImageDim.w  < FrameMinWidthSupported || 
           pInst->dImageDim.h < FrameMinHeightSupported || 
           pInst->dImageDim.w  != NEXT_MULT_OF_4(pInst->dImageDim.w) ||
           pInst->dImageDim.h != NEXT_MULT_OF_4(pInst->dImageDim.h))
            prsRtn = PIA_S_BAD_IMAGE_DIMENSIONS;
		else
			/* indicate CIn state is fully initialized */
			pInst->uTag = CCIN_INST_TAG;

#endif /* SIMULATOR */

		pOutput->iOutputStride = pInst->dImageDim.w;
    }
    return(prsRtn);    
}
                              
/*
 * routine to do the real color convert, from here we will branch out to call routine in convert.c
 */
                              
PIA_RETURN_STATUS ColorInFrame(PTR_CCIN_INST pInst,
                               PTR_COLORIN_INPUT_INFO pInput,
                               PTR_COLORIN_OUTPUT_INFO pOutput
                               )
{
    PIA_RETURN_STATUS prsRtn = PIA_S_OK;

#if DEBUG_ICCONV

	HivePrintString("\nColorInFrame");

#endif

    if(!pInst || !pInput || !pOutput || pInput->uTag != COLORIN_INPUT_INFO_TAG) {
        prsRtn = PIA_S_ERROR;
        goto bail;
    }
    if(!pInput->pu8InputData ||  (!pOutput->pu8ExternalOutputData && !pOutput->pu8OutputData)) {
        prsRtn = PIA_S_ERROR;
        goto bail;
    }
	/* check output stride is same as width - the assumption of yvu9 color conversion routine */
	if(pOutput->iOutputStride != (I32)pInst->dImageDim.w) {
		prsRtn = PIA_S_BAD_SETUP_VALUE;
        goto bail;
	}
	/* do the actual color convert stuff */

#ifdef SIMULATOR
	if (pOutput->cfOutputFormat == CF_PLANAR_YVU12_8BIT) {
		prsRtn = ConvertToYVU12(pInst, pInput, pOutput);

		if(prsRtn == PIA_S_OK) {
		/* give back internal format size */
			pOutput->uOutputSize = 
				YVU12_SIZE(pInst->dImageDim.w, pInst->dImageDim.h);
	    	/* setup tag to indicate valid output info */
			pOutput->uTag = COLORIN_OUTPUT_INFO_TAG; 
		}
	} else {
	    prsRtn = ConvertToYVU9(pInst, pInput, pOutput);
		if(prsRtn == PIA_S_OK) {
		/* give back internal format size */
			pOutput->uOutputSize = 
				YVU9_SIZE(pInst->dImageDim.w, pInst->dImageDim.h);
	    	/* setup tag to indicate valid output info */
			pOutput->uTag = COLORIN_OUTPUT_INFO_TAG; 
		}
	}
#else
    prsRtn = ConvertToYVU9(pInst, pInput, pOutput);

	if(prsRtn == PIA_S_OK) {
		/* give back internal format size */
		pOutput->uOutputSize = 
			YVU9_SIZE(pInst->dImageDim.w, pInst->dImageDim.h);
    	/* setup tag to indicate valid output info */
    	pOutput->uTag = COLORIN_OUTPUT_INFO_TAG; 
	}

#endif /* SIMULATOR */

bail:

    return(prsRtn);    
}                               


/*
 * routine to answer some questions, try our best.
 */
                              
PIA_RETURN_STATUS ColorInQuery(PTR_CCIN_INST pInst, 
							   PTR_COLORIN_INPUT_INFO pInput,
							   PTR_COLORIN_OUTPUT_INFO pOutput,
							   U32 uHeight, 
                               U32 uWidth, 
							   PIA_Boolean bSpeedOverQuality)
{
    PIA_RETURN_STATUS prsRtn = PIA_S_OK;
    
#if DEBUG_ICCONV
	HivePrintString("\nColorInQuery");
	if(!pInst)
		return PIA_S_ERROR;
#endif

    if(pInput->cfSrcFormat == CF_QUERY) {
        if(bSpeedOverQuality) 
            pInput->cfSrcFormat = CF_XRGB32;
        else
            pInput->cfSrcFormat = CF_XRGB32;
    }

    /* since we only support YVU9 planar now, so return YVU9 planar regardless */
    if(pOutput->cfOutputFormat == CF_QUERY) 
         pOutput->cfOutputFormat = CF_PLANAR_YVU9_8BIT;
	prsRtn = CheckAndSetInfo(pInst, pInput, pOutput, uWidth, uHeight);

    return(prsRtn);    
}                               


/*
 * routine to do the setup for a sequence frames.
 */
                              
PIA_RETURN_STATUS ColorInSequenceSetup(PTR_CCIN_INST pInst,
									   PTR_COLORIN_INPUT_INFO pInput,
									   PTR_COLORIN_OUTPUT_INFO pOutput)
{
/*	PTR_PRIVATE_COLORIN_DATA	pPrivate = pInst->pColorInPrivate; */
	PTR_PASS_DOWN_COLORIN_PARMS  pParms= pInst->pColorInParms;
	U32 uMaxInternalFormatSize;
    PIA_RETURN_STATUS prsRtn = PIA_S_OK;
#if DEBUG_ICCONV
	HivePrintString("\nColorInSequenceSetup");
	if(!pInst)
		return PIA_S_ERROR;
#endif
    
	/* On a repeated SequenceSetup Call, we need to completely reinitialize  */
	/* all of the instance data, including reallocating all of the context   */
	/* structures and reinitializing tables, etc.  This is necessary because */
	/* parameters that can change between Sequences can radically affect the */
	/* structures allocated.                                                 */

	if (!pInst->pColorInPrivate->bFirstTime) {

		/* First end the sequence */

		prsRtn = ColorInSequenceEnd(pInst);
		if (prsRtn != PIA_S_OK)
			goto bail;

		/* Free the private data */
		if (pInst->pColorInPrivate->pu8OutputData) {
			HiveGlobalFreePtr(pInst->pColorInPrivate->pu8OutputData);
		}
		if (pInst->pColorInPrivate->pu8TempBuffer) {
			HiveGlobalFreePtr(pInst->pColorInPrivate->pu8TempBuffer);
		}
		HiveGlobalFreePtr(pInst->pColorInPrivate);
		pInst->pColorInPrivate = NULL;

		/* Added 5/9/97 Steve Wood
		 * To ensure ColorInConstantInit() would succeed.
		 * Maybe this should be in ColorInSequenceEnd()?
		 */
		pInst->uTag = NULL;

		/* Then call ConstantInit to start bringing eveything back up again */

		prsRtn = ColorInConstantInit(pInst);
		if (prsRtn != PIA_S_OK)
			goto bail;

		/* Finally, call ImageDimInit to complete the process of forming the instance */

		prsRtn = ColorInImageDimInit(pInst, pInput, pOutput);
		if (prsRtn != PIA_S_OK)
			goto bail;

		/* Now go on and do the New SequenceSetup... */
	}

	prsRtn=CheckAndSetInfo(pInst, pInput, pOutput, pInst->dImageDim.w, pInst->dImageDim.h);


	/* allocate the private tempBuffer */
	pInst->pColorInPrivate->pu8TempBuffer = 
		HiveGlobalAllocPtr(pInst->pColorInPrivate->uMaxTempSize, TRUE);

	/* allocate the private TransMask */
    if(pInst->ciscSequenceControlsUsed & CIN_TRANSPARENCY) {
		pParms->pTransparencyMask = 	
			HiveGlobalAllocPtr(sizeof(TRANSPARENCY_MASK), TRUE);
		pParms->pTransparencyMask->u16Tag = TRANSPARENCY_MASK_TAG;
		pParms->pTransparencyMask->cfPixelSize = CF_GRAY_1;
		pParms->pTransparencyMask->uStride = (((pInst->dImageDim.w+31) >> 5) << 2);
		pParms->pTransparencyMask->uMaskSize = pInst->dImageDim.h*pParms->pTransparencyMask->uStride;
		pParms->pTransparencyMask->pu8TransBuffer = 
			HiveGlobalAllocPtr(pParms->pTransparencyMask->uMaskSize, TRUE);
	}  else {
		pParms->pTransparencyMask = NULL;
	}

	if (!pOutput->pu8ExternalOutputData) {
		/* allocate the output data buffer for Color Input module */
		prsRtn = ColorInGetMaxOutputBuffSize(pOutput->cfOutputFormat,
										pInst->dImageDim.h, pInst->dImageDim.w,
										&uMaxInternalFormatSize);
		if (prsRtn != PIA_S_OK) 
			goto bail;

		pInst->pColorInPrivate->pu8OutputData = HiveGlobalAllocPtr(uMaxInternalFormatSize, TRUE);
		pOutput->pu8OutputData = pInst->pColorInPrivate->pu8OutputData;
		if (pInst->pColorInPrivate->pu8OutputData == NULL)
			prsRtn = PIA_S_OUT_OF_MEMORY;
	}

	pInst->pColorInPrivate->bFirstTime = FALSE;

bail:
	return(prsRtn);
}                               

PIA_RETURN_STATUS ColorInGetMaxOutputBuffSize(
	COLOR_FORMAT    cfFormat,
	U32				uH, 
	U32				uW,
	PU32			puSize
)
{
#ifdef SIMULATOR 
	if (cfFormat  == CF_PLANAR_YVU12_8BIT) {
	    *puSize = YVU12_SIZE(uW, uH);
	} else {
	    *puSize = YVU9_SIZE(uW, uH);
	}

#else
    *puSize = YVU9_SIZE(uW, uH);
#endif /* SIMULATOR */

	return(PIA_S_OK);
}

/*
 * routine to clean up the setup for a sequence frames.
 */
                              
PIA_RETURN_STATUS ColorInSequenceEnd(PTR_CCIN_INST pInst)
{
#if DEBUG_ICCONV
	HivePrintString("\nColorInSequenceEnd");
	if(!pInst)
		return PIA_S_ERROR;
#endif
    pInst += 0;
    return(PIA_S_OK);    
}

static PIA_RETURN_STATUS CheckAndSetInfo(PTR_CCIN_INST pInst, 
										 PTR_COLORIN_INPUT_INFO pInput,
										 PTR_COLORIN_OUTPUT_INFO pOutput,
										 U32 uWidth, U32 uHeight)
{
/*	PTR_PRIVATE_COLORIN_DATA	pPrivate = pInst->pColorInPrivate; */
    PIA_RETURN_STATUS prsRtn = PIA_S_OK;
    U32 uH, uW;                                                
	SUPPORTED_ALGORITHMS listFormats;
	PIA_Boolean bValidFormat;
	I32 i;

#ifdef SIMULATOR 
   	/* check input frame size */
	if (pOutput->cfOutputFormat == CF_PLANAR_YVU12_8BIT) {
		if (uWidth  < FrameMinWidthSupported || 
			uHeight < FrameMinHeightSupported || 
			uWidth  != NEXT_MULT_OF_2(uWidth) ||
			uHeight != NEXT_MULT_OF_2(uHeight))
		{
    		prsRtn = PIA_S_BAD_IMAGE_DIMENSIONS;
			goto bail;
		}
	} else {
		if (uWidth  < FrameMinWidthSupported || 
			uHeight < FrameMinHeightSupported || 
			uWidth  != NEXT_MULT_OF_4(uWidth) ||
		 	uHeight != NEXT_MULT_OF_4(uHeight))
		{ 
			prsRtn = PIA_S_BAD_IMAGE_DIMENSIONS;
			goto bail;
		}
	}
#else
   	/* check input frame size */
    if(uWidth  < FrameMinWidthSupported || 
       uHeight < FrameMinHeightSupported || 
       uWidth  != NEXT_MULT_OF_4(uWidth) ||
       uHeight != NEXT_MULT_OF_4(uHeight))
	{
    	prsRtn = PIA_S_BAD_IMAGE_DIMENSIONS;
        goto bail;
	}
#endif /* SIMULATOR */

	if (pInst->ciscSequenceControlsUsed & ~(pInst->ciciInfo.ciscSupportedSequenceControls)) {
        prsRtn = PIA_S_UNSUPPORTED_CONTROL;
        goto bail;
	}

	/* Validate color format: both input and output */

	listFormats = pInst->ciciInfo.listInputFormats;
	if (listFormats.u16Tag == SUPPORTED_ALGORITHMS_TAG) {
		bValidFormat = FALSE;
		for (i=0;i<listFormats.u16NumberOfAlgorithms; i++) {
			if (pInput->cfSrcFormat == listFormats.eList[i] ) {
				bValidFormat = TRUE;
				break;
			}
		}
		if (!bValidFormat) {
			prsRtn = PIA_S_BAD_COLOR_FORMAT;
			goto bail;
		}
	} else {
		prsRtn = PIA_S_ERROR;
		goto bail;
	}

	listFormats = pInst->ciciInfo.listOutputFormats;
	if (listFormats.u16Tag == SUPPORTED_ALGORITHMS_TAG) {
		bValidFormat = FALSE;
		for (i=0;i<listFormats.u16NumberOfAlgorithms; i++) {
			if (pOutput->cfOutputFormat == listFormats.eList[i] ) {
				bValidFormat = TRUE;
				break;
			}
		}
		if (!bValidFormat) {
			prsRtn = PIA_S_BAD_COLOR_FORMAT;
			goto bail;
		}
	} else {
		prsRtn = PIA_S_ERROR;
		goto bail;
	}
	
    uW = pInst->dImageDim.w;
    uH = pInst->dImageDim.h;

    /* x*y for each non subsampled U, and V plane. */
	pInst->pColorInPrivate->uMaxTempSize = uW * uH * 2;

	/* Add another byte per pixel for transparency scratch space */

	if (pInst->ciscSequenceControlsUsed & CIN_TRANSPARENCY)
		pInst->pColorInPrivate->uMaxTempSize += uW * uH;

	/* allocate the memory for TempBuffer if not yet */
    pInst->pColorInPrivate->uPrevYAvg = 128;

bail:
 	return(prsRtn);
}


/* Debug */
PIA_RETURN_STATUS ColorInDebugGet( PTR_CCIN_INST pInst, PVOID_GLOBAL vgpVoidPtr )
{
	return (PIA_S_UNSUPPORTED_FUNCTION);
}

PIA_RETURN_STATUS ColorInDebugSet( PTR_CCIN_INST pInst, PVOID_GLOBAL vgpVoidPtr )
{
	return (PIA_S_UNSUPPORTED_FUNCTION);
}

/* Transparency input - alpha channel */
PIA_RETURN_STATUS ColorInSetTransAlphaChannel( PTR_CCIN_INST pInst )
{
	PTR_PASS_DOWN_COLORIN_PARMS  pParms= pInst->pColorInParms;
	PIA_RETURN_STATUS prtRnt = PIA_S_OK;

	if (pInst->ciscSequenceControlsUsed & CIN_TRANSPARENCY) {
		pParms->bAlphaChannel = TRUE;
	} else {
		prtRnt = PIA_S_ERROR;
	}
	return(prtRnt);
}

/* Transparency Color analysis: HIVE called this only for first frame */
PIA_RETURN_STATUS ColorInSetCalcTransColorRange( PTR_CCIN_INST pInst )
{
	PTR_PASS_DOWN_COLORIN_PARMS  pParms= pInst->pColorInParms;
	PIA_RETURN_STATUS prtRnt = PIA_S_OK;

	if (pInst->ciscSequenceControlsUsed & CIN_TRANSPARENCY) {
		pParms->bBuildColorRange = TRUE;
		pParms->bBuildBitmask = TRUE;
	} else {
		prtRnt = PIA_S_ERROR;
	}
	return(prtRnt);
}

/* Transparency input - user supplied color range */
PIA_RETURN_STATUS ColorInTransGetDefTransColorRange( PTR_CCIN_INST pInst, 
													PTR_COLOR_RANGE pcrRange )
{
	PIA_RETURN_STATUS prtRnt = PIA_S_OK;
	return(prtRnt);
}

PIA_RETURN_STATUS ColorInGetTransColorRange(PTR_CCIN_INST pInst, 
											PTR_COLOR_RANGE pcrRange )
{
	PTR_PASS_DOWN_COLORIN_PARMS  pParms= pInst->pColorInParms;
	PIA_RETURN_STATUS prtRnt = PIA_S_OK;

	if (pInst->ciscSequenceControlsUsed & CIN_TRANSPARENCY) {
		*pcrRange = pParms->crTransColorRange;
	} else {
		prtRnt = PIA_S_ERROR;
	}
	return(prtRnt);
}


PIA_RETURN_STATUS ColorInSetTransColorRange( PTR_CCIN_INST pInst, COLOR_RANGE crRange )
{
	PTR_PASS_DOWN_COLORIN_PARMS  pParms= pInst->pColorInParms;
	PIA_RETURN_STATUS prtRnt = PIA_S_OK;

	if (pInst->ciscSequenceControlsUsed & CIN_TRANSPARENCY) {
		pParms->crTransColorRange = crRange ;
	} else {
		prtRnt = PIA_S_ERROR;
	}
	return(prtRnt);
}


PIA_RETURN_STATUS ColorInGetTransMask(PTR_CCIN_INST pInst, 
									  PTR_PTR_TRANSPARENCY_MASK ppTransMask)
{
	PTR_PASS_DOWN_COLORIN_PARMS  pParms= pInst->pColorInParms;
	PIA_RETURN_STATUS prtRnt = PIA_S_OK;

	if (pInst->ciscSequenceControlsUsed & CIN_TRANSPARENCY) {
		*ppTransMask = pParms->pTransparencyMask;
	} else {
		prtRnt = PIA_S_ERROR;
	}
	return(prtRnt);
}


PIA_RETURN_STATUS ColorInSetTransMask(PTR_CCIN_INST pInst, 
									  PTR_TRANSPARENCY_MASK pTransMask)
{
	PTR_PASS_DOWN_COLORIN_PARMS  pParms= pInst->pColorInParms;
	PIA_RETURN_STATUS prtRnt = PIA_S_OK;

	if (pInst->ciscSequenceControlsUsed & CIN_TRANSPARENCY) {
		pParms->pTransparencyMask = pTransMask;
	} else {
		prtRnt = PIA_S_ERROR;
	}
	return(prtRnt);
}

PIA_RETURN_STATUS ColorInGetTransMaskSize( PTR_CCIN_INST pInst, PU32 uSize )
{
	PTR_PASS_DOWN_COLORIN_PARMS  pParms= pInst->pColorInParms;
	PIA_RETURN_STATUS prtRnt = PIA_S_OK;

	if (pInst->ciscSequenceControlsUsed & CIN_TRANSPARENCY) {
		*uSize = pParms->pTransparencyMask->uMaskSize;
	} else {
		prtRnt = PIA_S_ERROR;
	}
	return(prtRnt);
}

/* transparency output - representative color */
PIA_RETURN_STATUS ColorInGetRepresentativeColor(PTR_CCIN_INST pInst, 
												PTR_BGR_ENTRY pbgrColor )
{
	PTR_PASS_DOWN_COLORIN_PARMS  pParms= pInst->pColorInParms;
	PIA_RETURN_STATUS prtRnt = PIA_S_OK;

	if (pInst->ciscSequenceControlsUsed & CIN_TRANSPARENCY) {
		*pbgrColor = pParms->bgrRepresentativeColor;
	} else {
		prtRnt = PIA_S_ERROR;
	}
	return(prtRnt);
}

