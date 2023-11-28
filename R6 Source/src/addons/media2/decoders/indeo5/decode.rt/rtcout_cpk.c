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

/*
 * ColorOutQuery --
 *  Verify the controls are OK (PIA_S_BAD_CONTROL_VALUE,
 *                              PIA_S_UNSUPPORTED_CONTROL)
 *  Verify that the output destination is appropriate (PIA_S_BAD_DESTINATION)
 *  Verify that the resizing amount is valid (PIA_S_BAD_CONTROL_VALUE)
 *  Verify the color representation for the output (PIA_S_BAD_COLOR_FORMAT)
 *  Set a time estimate
 */

#if 0
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

printf("ColorOutQuery 1\n");

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

printf("ColorOutQuery 2\n");
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

printf("ColorOutQuery 3\n");
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


printf("ColorOutQuery 4\n");
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

printf("ColorOutQuery 5\n");
    listFormats = pInst->cociInfo.listOutputFormats;
    if (listFormats.u16Tag == SUPPORTED_ALGORITHMS_TAG) {
        bValidFormat = FALSE;
			printf("listFormats.u16NumberOfAlgorithms = %d\n", listFormats.u16NumberOfAlgorithms);
        for (i=0;i<listFormats.u16NumberOfAlgorithms; i++) {
			printf("pOutput->cfOutputFormat %d, listFormats.eList[i] %d\n",
				pOutput->cfOutputFormat,
				listFormats.eList[i]);
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
printf("ColorOutQuery 6\n");

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
printf("ColorOutQuery done\n");

end:
printf("ColorOutQuery ret %d\n", prs);
	return prs;
}	/* ColorOutQuery */

#endif

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

	pInst->cociInfo.listOutputFormats.u16NumberOfAlgorithms = 3;/*11; */
	pInst->cociInfo.listOutputFormats.u16NumberOfAlgorithms = 6;
	pInst->cociInfo.listOutputFormats.eList[0]	= CF_CLUT_8;
	pInst->cociInfo.listOutputFormats.eList[1]	= CF_RGB24;
	pInst->cociInfo.listOutputFormats.eList[2]	= CF_XRGB32;  
	pInst->cociInfo.listOutputFormats.eList[3]	= CF_BGRX32;
	pInst->cociInfo.listOutputFormats.eList[4]	= CF_XRGB16_1555;
	pInst->cociInfo.listOutputFormats.eList[5]	= CF_RGB16_565;
	
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
