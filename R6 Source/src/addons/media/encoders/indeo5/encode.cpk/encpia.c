/************************************************************************
*																		*
*				INTEL CORPORATION PROPRIETARY INFORMATION				*
*																		*
*	 This listing is supplied under the terms of a license agreement	*
*	   with INTEL Corporation and may not be copied nor disclosed		*
*		 except in accordance with the terms of that agreement.			*
*																		*
*************************************************************************
*																		*
*				Copyright (c) 1994-1997 Intel Corp.						*
*						  All Rights Reserved.							*
*																		*
************************************************************************/

/* ENCPIA.C 
 *
 * This file contains the implementations of the HIVE PIA Interface 
 * functions for the INDEO5 Offline Encoder.
 *
 */

/* Functions:
EncodeStartup
EncodeShutdown
EncodeConstantInit
EncodeImageDimInit
EncodeFreePrivateData
EncodeSequenceSetup
EncodeSequenceEnd
EncodeGetMaxCompressedSize
EncodeQuery
EncodeFrame

EncodeGetDefAccessKey
EncodeGetAccessKey
EncodeSetAccessKey
EncodeDebugGet
EncodeDebugSet
EncodeGetDefScaleLevel
EncodeGetScaleLevel
EncodeSetScaleLevel
EncodeSetTransColor
EncodeSetTransBitmask
EncodeGetDefViewPort
EncodeGetMinViewPort
EncodeGetViewPort
EncodeSetViewPort
EncodeGetDefTargetPlaybackPlatform
EncodeGetTargetPlaybackPlatform
EncodeSetTargetPlaybackPlatform

*/


#include <string.h>
#include <setjmp.h>

#include "datatype.h"
#include "matrix.h"

#include "tksys.h"
#include "const.h"

#include "mc.h"			/* for ivi5bs.h */
#include "hufftbls.h"
#include "qnttbls.h"

#include "ivi5bs.h"		/* for bsutil.h */
#include "mbhuftbl.h"
#include "bkhuftbl.h"

#ifdef SIMULATOR
#include "decsmdef.h"
#include "encsmdef.h"
#endif /* SIMULATOR */

#include "bsutil.h"
#include "pia_main.h"
#include "indeo5.h"
#include "ensyntax.h"
#include "encoder.h"							   
#include "enme.h"
#include "enmesrch.h"
#include "enseg.h"

#include "prefilt.h"

#include "entrans.h"
#include "context.h"
#include "enntry.h"

#include "encpia.h"		/* needs to be before pia_enc.h */

#ifdef CMD_LINE_ENC
#include "cmdlparm.h"	   /* needed for pia_enc.h */
#endif /* CMD_LINE_ENC */

#include "pia_enc.h"

#ifdef SIMULATOR
#include "encsim.h"		   
#endif /* SIMULATOR */
	
#include "common.h"
#include "errhand.h"

#define CLIP(a,lo,hi) ((a)<(lo)?(lo):((a)>(hi)?(hi):(a)))


/* External routine linkages */
void OpenPictureCompressorContext( PTR_ENC_INST			pInst,
								   PEncRtParmsSt		RtParms,
								   PEncPictureCntxSt	pPicContext,
								   jmp_buf				jbEnv);

void ClosePictureCompressorContext(PEncPictureCntxSt	pPicContext,
								   jmp_buf				jbEnv);

void CmpressPicture			 (  PTR_ENC_INST 		pInst,
									PCmpStrategyStorageSt pStrategyBuf,
								   	PEncPictureCntxSt	 pPicContext,
									PIA_Boolean			 bBRCOn,
								   	jmp_buf				 jbEnv);

void ControlBitRate				 ( PCmpStrategyStorageSt pStrategyBuf,
					 			   PTR_ENC_INST 		 pInst,
								   jmp_buf				 jbEnv);

/* Internal routine prototypes */

static void BuildRtParms(PTR_ENC_INST pInst,COLOR_FORMAT cfSrcFormat,  jmp_buf jbEnv);
static U32  SelectPitch(DimensionSt dim);
static void EncSynValTileSize(PI32 piWidth, PI32 piHeight);



static void                  ConvertFrameToPic    ( PMatrixSt pPicture, PU8 pYVU9Data );
static void					 CompressionSequencer ( PTR_ENC_INST pInst, jmp_buf jbEnv );

static PIA_RETURN_STATUS FreeEncPrivateData(PTR_ENC_INST pInst);
static PIA_RETURN_STATUS FreeEncPassDownData(PTR_ENC_INST pInst);
/***********************************************************************
 *** EncodeStartup
 *** 
 *** This function is called to start up the encoder.
 ***
 ***********************************************************************/ 

PIA_RETURN_STATUS EncodeStartup()
{

	return (PIA_S_OK);

}    /*** End of EncodeStartup() ***/


/***********************************************************************
 *** EncodeShutdown
 *** 
 *** This function is called to shut down the encoder.
 ***
 ***********************************************************************/ 

PIA_RETURN_STATUS EncodeShutdown()
{

	return (PIA_S_OK);

}    /*** End of EncodeShutdown() ***/


/***********************************************************************
 *** EncodeConstantInit
 *** 
 *** This function is called by the interface code after it has 
 *** allocated a new instance structure. 
 ***
 *** pInst -- Pointer to an encoder instance.
 ***
 ***********************************************************************/ 

PIA_RETURN_STATUS EncodeConstantInit(PTR_ENC_INST pInst)
{
	  PTR_PRIVATE_ENCODER_DATA  pEncPrivateData;
	  PIA_RETURN_STATUS prs;
	  jmp_buf					jbEnv;
	  U32						uStatus;

	/* Ensure pointer is initialized */

	if (pInst == NULL) {
		prs = PIA_S_ERROR;
		goto bail;
	}

	/* Initialize the Constant Encoder Info */

	pInst->eciInfo.uSuggestedKeyFrameRate = 15;
	pInst->eciInfo.keySuggestedKeyFrameRateKind = KEY_FIXED;
	pInst->eciInfo.uSuggestedTargetDataRate = 150000;
	pInst->eciInfo.uSuggestedAbsoluteQuality = 8500;

	pInst->eciInfo.ecscSuggestedSequenceControls = EC_FLAGS_VALID |
										  EC_ACCESS_KEY |
										  EC_TARGET_DATA_RATE;

	pInst->eciInfo.ecscSupportedSequenceControls = EC_FLAGS_VALID |
										  EC_ACCESS_KEY |
										  EC_TARGET_DATA_RATE |
										  EC_ABSOLUTE_QUALITY_SLIDER |
										  EC_VIEWPORT |
										  EC_TRANSPARENCY |
										  EC_SCALABILITY;
											
#ifdef SIMULATOR 
	pInst->eciInfo.listInputFormats.u16NumberOfAlgorithms = 2;
#else 
	pInst->eciInfo.listInputFormats.u16NumberOfAlgorithms = 1;
#endif

	pInst->eciInfo.listInputFormats.eList[0] = CF_PLANAR_YVU9_8BIT  ;
											
#ifdef SIMULATOR 
	pInst->eciInfo.listInputFormats.eList[1] = CF_PLANAR_YVU12_8BIT  ;
#endif

	pInst->eciInfo.listInputFormats.u16Tag = SUPPORTED_ALGORITHMS_TAG;

	
	pInst->eciInfo.listOutputFormats.u16NumberOfAlgorithms = 1;
	pInst->eciInfo.listOutputFormats.eList[0] = CF_IV50 ;
	pInst->eciInfo.listOutputFormats.u16Tag = SUPPORTED_ALGORITHMS_TAG;

	pInst->eciInfo.svs_list.u16NumberOfViewrectSizes = 4;
	pInst->eciInfo.svs_list.eList[0] = 0;
	pInst->eciInfo.svs_list.eList[1] = 64;
	pInst->eciInfo.svs_list.eList[2] = 128;
	pInst->eciInfo.svs_list.eList[3] = 256;

	uStatus = setjmp(jbEnv);
	if (!uStatus) { /* 0 means the environment was set */
		if (!pInst->pEncParms) {
			pInst->pEncParms = (PTR_PASS_DOWN_ENC_PARMS) 
							   SysMalloc(sizeof(PassDownEncParmsSt), jbEnv);
		}

		if (pInst->pEncParms ==  NULL) {
			prs = PIA_S_ERROR;
			goto bail;
		}

		if (pInst->pEncoderPrivate) {
			EncodeFreePrivateData(pInst);
		}
			
		pInst->pEncoderPrivate = (PTR_PRIVATE_ENCODER_DATA) 
							   SysMalloc(sizeof(EncoderPrivateData), jbEnv);
		if (pInst->pEncoderPrivate ==  NULL) {
			prs = PIA_S_ERROR;
			goto bail;
		} else {
			pEncPrivateData = pInst->pEncoderPrivate;
		}

		/* Allocate the RtParms data area */
		pEncPrivateData->pRtParms = (PEncRtParmsSt) SysMalloc(sizeof(EncRtParmsSt), jbEnv);
		if (pEncPrivateData->pRtParms == (PEncRtParmsSt) NULL) {
		   prs = PIA_S_ERROR;
		   goto bail;
		}

#ifdef SIMULATOR /* Simulation infrastructure */

		/* Allocate simulator instance struct and set tag in EncoderPrivate struct */

		pEncPrivateData->pEncSimInst = (PTR_ENC_SIM_INST) SysMalloc(sizeof(ENC_SIM_INST), jbEnv);
		pEncPrivateData->pEncSimInst->pSFile = (PTR_SIM_FILE) SysMalloc(sizeof(SIM_FILE), jbEnv);
		pEncPrivateData->pEncSimInst->pSFile->uPrintFlags = (U32) 0;  /*initialize*/
		pEncPrivateData->pEncSimInst->pSFile->bFileOpened = FALSE;   /*initialize*/
		if (pEncPrivateData->pEncSimInst == (PTR_ENC_SIM_INST) NULL) {
		   prs = PIA_S_ERROR;
		   goto bail;
		}

		pEncPrivateData->uSimDataTag = ENC_SIM_DATA_TAG;  /* Flag that this struct is valid */
#else
		pEncPrivateData->pEncSimInst = NULL;	/* make sure this pointer point nowhere */
		pEncPrivateData->uSimDataTag = 0;  /* Flag that this struct is not valid */
 
#endif /* SIMULATOR */

	} 
	else { /* Arrived through a longjmp -- Process the error */
#ifdef DEBUG
		HivePrintString("***FATAL ERROR in ");
		HivePrintString(gac8Files[(uStatus&FILE_MASK)>>FILE_OFFSET]);
		HivePrintString(", line");
		HivePrintDecInt((I32) ((uStatus&LINE_MASK)>>LINE_OFFSET));
		HivePrintString("\tError");
		HivePrintHexInt((uStatus&TYPE_MASK)>>TYPE_OFFSET);
		HivePrintString("\n");
#endif
		prs = PIA_S_INSTANCE_FATAL;		
		goto bail;
	}


	/* Init the private data area with defaults and constants */
	
	pEncPrivateData->nPicNumber = 0;
	pEncPrivateData->bFirstTime = TRUE;

	/* Insert tag indicating a partially initialized instance */

	pInst->uTag = ENC_PARTIAL_INST_TAG;

	prs = PIA_S_OK;
bail:
	return prs;

}    /*** End of EncodeConstantInit() ***/


/***********************************************************************
 *** EncodeImageDimInit
 *** 
 *** This function is called by the interface code after it has 
 *** allocated a new instance structure and filled in the image
 *** dimensions. It is always called after EncodeConstantInit. 
 ***
 *** pInst -- Pointer to an encoder instance.
 ***
 ***********************************************************************/ 

PIA_RETURN_STATUS EncodeImageDimInit(PTR_ENC_INST 	pInst, 
									 PTR_ENCODE_FRAME_INPUT_INFO 	pInput, 
									 PTR_ENCODE_FRAME_OUTPUT_INFO 	pOutput)
{
	  PTR_PRIVATE_ENCODER_DATA  pEncPrivateData;
	  DimensionSt					dLocalImageDim;
	  U32						uFullPitch, uSubPitch;
	  U32						uFullHeight, uSubHeight;
	  U32						uFullWidth, uSubWidth;
	  U32 						uVUSubsampleRate;
	  PIA_RETURN_STATUS 		prs;

	/* Ensure pointer is initialized */
	if ((pInst == NULL) || ((pInst->uTag!= ENC_PARTIAL_INST_TAG) && (pInst->uTag!= ENC_INST_TAG))) {
		prs = PIA_S_ERROR;
		goto bail;
	}

	prs = EncodeQuery(	pInst, pInput, pOutput, pInst->dImageDim.h, 
						pInst->dImageDim.w, FALSE);
	if (prs != PIA_S_OK) 
		goto bail;

	/* Update the private data areas, now that we have more valid
	   info in the Instance */

	/* Make a local de-referenced pointer to the private data, and
	   a copy of the dimension info */
	dLocalImageDim = pInst->dImageDim;
	pEncPrivateData = pInst->pEncoderPrivate;

	/* Update the global picture data areas, now that we have valid dimensions in the Instance */
	uFullHeight = dLocalImageDim.h;
	uFullWidth  = dLocalImageDim.w;
	uFullPitch  = SelectPitch(dLocalImageDim);

	if (pInput->cfSrcFormat == CF_PLANAR_YVU9_8BIT) 
		uVUSubsampleRate = 4;
#ifdef SIMULATOR 
	else if (pInput->cfSrcFormat == CF_PLANAR_YVU12_8BIT)
		uVUSubsampleRate = 2;
#endif /* SIMULATOR */
	else {
		prs = PIA_S_BAD_COLOR_FORMAT;
		goto bail;
	}

    uSubHeight = (dLocalImageDim.h /= uVUSubsampleRate);
    uSubWidth  = (dLocalImageDim.w  /= uVUSubsampleRate);
	uSubPitch  = SelectPitch(dLocalImageDim);

	/* Init global Picture-Dimensions structure */
	pEncPrivateData->PicDimensions[0].NumRows = uFullHeight;
	pEncPrivateData->PicDimensions[0].NumCols = uFullWidth;
	pEncPrivateData->PicDimensions[0].Pitch   = uFullPitch;
	pEncPrivateData->PicDimensions[0].pi16    = NULL;

	pEncPrivateData->PicDimensions[1].NumRows = uSubHeight;
	pEncPrivateData->PicDimensions[1].NumCols = uSubWidth;
	pEncPrivateData->PicDimensions[1].Pitch   = uSubPitch;
	pEncPrivateData->PicDimensions[1].pi16    = NULL;

	pEncPrivateData->PicDimensions[2].NumRows = uSubHeight;
	pEncPrivateData->PicDimensions[2].NumCols = uSubWidth;
	pEncPrivateData->PicDimensions[2].Pitch   = uSubPitch;
	pEncPrivateData->PicDimensions[2].pi16    = NULL;

	/* Calculate size of transparency bitmasks */
	pEncPrivateData->TransBMSize = uFullHeight*(((uFullWidth+31) >> 5) << 2);

	pInst->uTag = ENC_INST_TAG;

	prs = PIA_S_OK;
bail:
	return prs;

}    /*** End of EncodeImageDimInit() ***/


/***********************************************************************
 *** EncodeFreePrivateData
 *** 
 *** This function is called so an encoder instance can free its 
 *** private data including the pass down parameter structure.
 ***
 *** pInst -- Pointer to an encoder instance.
 ***
 ***********************************************************************/ 

PIA_RETURN_STATUS EncodeFreePrivateData(PTR_ENC_INST pInst)
{
	  PIA_RETURN_STATUS 		prs;
	  jmp_buf					jbEnv;
	  U32						uStatus;

	/* Ensure pointer is valid */
	if (pInst == NULL) {
		prs = PIA_S_ERROR;
		goto bail;
	}


	uStatus = setjmp(jbEnv);
	if (!uStatus) { /* 0 means the environment was set */
		/* Free everything except the Pass down data */
		if (FreeEncPrivateData(pInst)!=PIA_S_OK) {
			prs = PIA_S_ERROR;
			goto bail;
		}
		
		/* Finally, free the pass down parameters */
		if (FreeEncPassDownData(pInst)!=PIA_S_OK) {
			prs = PIA_S_ERROR;
			goto bail;
		}

	} 
	else { /* Arrived through a longjmp -- Process the error */
#ifdef DEBUG
		HivePrintString("***FATAL ERROR in ");
		HivePrintString(gac8Files[(uStatus&FILE_MASK)>>FILE_OFFSET]);
		HivePrintString(", line");
		HivePrintDecInt((I32) ((uStatus&LINE_MASK)>>LINE_OFFSET));
		HivePrintString("\tError");
		HivePrintHexInt((uStatus&TYPE_MASK)>>TYPE_OFFSET);
		HivePrintString("\n");
#endif /* DEBUG */
		prs = PIA_S_INSTANCE_FATAL;		
		goto bail;
	}

	prs = PIA_S_OK;
bail:
	return prs;

}    /*** End of EncodeFreePrivateData() ***/


/***********************************************************************
 *** EncodeSequenceSetup 
 *** 
 *** This function is called before encoding the first frame in a 
 *** sequence and before encoding the next frame if one or more
 *** compression parameters has changed.
 ***
 *** pInst -- Pointer to an encoder instance.
 ***
 ***********************************************************************/ 

PIA_RETURN_STATUS EncodeSequenceSetup(PTR_ENC_INST 	pInst, 
									  PTR_ENCODE_FRAME_INPUT_INFO 	pInput, 
									  PTR_ENCODE_FRAME_OUTPUT_INFO 	pOutput) 
{
	  PTR_PRIVATE_ENCODER_DATA	pEncPrivateData = pInst->pEncoderPrivate;
	  PEncPictureCntxSt			pPicContext;
	  PCmpStrategyStorageSt	    pStrategyBuf;
	  U32						uFullPitch, uSubPitch;
	  U32						uFullHeight, uSubHeight;
	  U32						uFullWidth, uSubWidth;
	  jmp_buf					jbEnv;
	  U32						uStatus;
	  PIA_RETURN_STATUS 		prs;
	  const I32 S_BUF_RING_COUNT = 2; 

	/* Ensure pointer is initialized */

	if ((pInst == NULL) || (pInst->uTag != ENC_INST_TAG)) {
		prs = PIA_S_ERROR;
		goto bail;
	}

	/* On a repeated SequenceSetup Call, we need to completely reinitialize  */
	/* all of the instance data, including reallocating all of the context   */
	/* structures and reinitializing tables, etc.  This is necessary because */
	/* parameters that can change between Sequences can radically affect the */
	/* structures allocated.                                                 */

	if (!pInst->pEncoderPrivate->bFirstTime) {

		/* First end the sequence */

		prs = EncodeSequenceEnd(pInst);
		if (prs != PIA_S_OK)
			goto bail;

		/* Then call FreeMostPrivateData to wipe out 
		 * everything except the pass down parameters 
		 */

		prs = FreeEncPrivateData(pInst);
		if (prs != PIA_S_OK)
			goto bail;

		/* Then call ConstantInit to start bringing eveything back up again */

		prs = EncodeConstantInit(pInst);
		if (prs != PIA_S_OK)
			goto bail;

		/* Finally, call ImageDimInit to complete the process of forming the instance */

		prs = EncodeImageDimInit(pInst, pInput, pOutput);
		if (prs != PIA_S_OK)
			goto bail;

		/* Now go on and do the New SequenceSetup... */
	}

		/* Initialize the simulator structure */
#ifdef SIMULATOR
		EncSimInit(pInst);
#ifdef CMD_LINE_ENC
	if (!PassEncSimParams(pInst)) {
		goto bail;
	};
#endif	  /* CMD_LINE_ENC */
#endif	  /* SIMULATOR */
	/* Make local dereferenced ptr, once we know the instance ptr is good... */
	pEncPrivateData = pInst->pEncoderPrivate;

	/* Do a query to validate all of the parms */

	prs = EncodeQuery(pInst,
					  pInput,
					  pOutput,
	       			  pInst->dImageDim.h,
					  pInst->dImageDim.w,
					  FALSE);

	if (prs != PIA_S_OK)
		goto bail;

	/* Set the sequencer's starting point */
	pEncPrivateData->uSequencerState = PIC_TYPE_K;
		
	uStatus = setjmp(jbEnv);
	if (!uStatus) { /* 0 means the environment was set */

		/* Build the RtParms structure from the encoder instance structure */
		BuildRtParms(pInst, pInput->cfSrcFormat, jbEnv);   

#ifdef SIMULATOR

		/* Now assert any simulation parms into RtParms before anything else happens */

		BuildSimRtParms(pInst);

#endif /* SIMULATOR */

		/* Init the Encoder private data that depends on the RtParms */
		pEncPrivateData->nKPeriodCntr = 0;

		/* Make a local pointer into the ring */
		pStrategyBuf = &(pEncPrivateData->StrategyBuf);

		/* PictureDimensions initialized in ImageDim Init */
		uFullHeight = pEncPrivateData->PicDimensions[0].NumRows;
		uFullWidth  = pEncPrivateData->PicDimensions[0].NumCols;
		uFullPitch  = SelectPitch(pInst->dImageDim);
	    uSubHeight = pEncPrivateData->PicDimensions[2].NumRows;
	    uSubWidth  = pEncPrivateData->PicDimensions[2].NumCols;
		uSubPitch  = SelectPitch(pInst->dImageDim);

		/* Calculate the size of our bitstream buffers */
		EncodeGetMaxCompressedSize(uFullHeight, uFullWidth, &pStrategyBuf->CSBBitStream.bufSize);

		/* Init Strategy Buffer picture dimensions */

		pStrategyBuf->CSBPicture[0].NumRows = uFullHeight;
		pStrategyBuf->CSBPicture[0].NumCols = uFullWidth;
		pStrategyBuf->CSBPicture[0].Pitch   = uFullPitch;
		MatAlloc(&(pStrategyBuf->CSBPicture[0]), jbEnv);

		pStrategyBuf->CSBReference[0].NumRows = uFullHeight;
		pStrategyBuf->CSBReference[0].NumCols = uFullWidth;
		pStrategyBuf->CSBReference[0].Pitch   = uFullPitch;
		MatAlloc(&(pStrategyBuf->CSBReference[0]), jbEnv);

		pStrategyBuf->CSBRebuiltPic[0].NumRows = uFullHeight;
		pStrategyBuf->CSBRebuiltPic[0].NumCols = uFullWidth;
		pStrategyBuf->CSBRebuiltPic[0].Pitch   = uFullPitch;
		MatAlloc(&(pStrategyBuf->CSBRebuiltPic[0]), jbEnv);

		pStrategyBuf->CSBPicture[1].NumRows = uSubHeight;
		pStrategyBuf->CSBPicture[1].NumCols = uSubWidth;
		pStrategyBuf->CSBPicture[1].Pitch   = uSubPitch;
		MatAlloc(&(pStrategyBuf->CSBPicture[1]), jbEnv);

		pStrategyBuf->CSBReference[1].NumRows = uSubHeight;
		pStrategyBuf->CSBReference[1].NumCols = uSubWidth;
		pStrategyBuf->CSBReference[1].Pitch   = uSubPitch;
		MatAlloc(&(pStrategyBuf->CSBReference[1]), jbEnv);

		pStrategyBuf->CSBRebuiltPic[1].NumRows = uSubHeight;
		pStrategyBuf->CSBRebuiltPic[1].NumCols = uSubWidth;
		pStrategyBuf->CSBRebuiltPic[1].Pitch   = uSubPitch;
		MatAlloc(&(pStrategyBuf->CSBRebuiltPic[1]), jbEnv);

		pStrategyBuf->CSBPicture[2].NumRows = uSubHeight;
		pStrategyBuf->CSBPicture[2].NumCols = uSubWidth;
		pStrategyBuf->CSBPicture[2].Pitch   = uSubPitch;
		MatAlloc(&(pStrategyBuf->CSBPicture[2]), jbEnv);
			
		pStrategyBuf->CSBReference[2].NumRows = uSubHeight;
		pStrategyBuf->CSBReference[2].NumCols = uSubWidth;
		pStrategyBuf->CSBReference[2].Pitch   = uSubPitch;
		MatAlloc(&(pStrategyBuf->CSBReference[2]), jbEnv);

		pStrategyBuf->CSBRebuiltPic[2].NumRows = uSubHeight;
		pStrategyBuf->CSBRebuiltPic[2].NumCols = uSubWidth;
		pStrategyBuf->CSBRebuiltPic[2].Pitch   = uSubPitch;
		MatAlloc(&(pStrategyBuf->CSBRebuiltPic[2]), jbEnv);

		/* Allocate the Picture Context */
		pEncPrivateData->pPicContext =
			pPicContext = (PEncPictureCntxSt) SysMalloc(sizeof(EncPictureCntxSt), jbEnv);

		/* Connect pointers in the context to their targets in the Strategy Context */
		pPicContext->pPicDimensions = &(pEncPrivateData->PicDimensions[0]);
		pPicContext->pnGlobalQuant  = &(pEncPrivateData->pRtParms->GlobalQuant);	

		OpenPictureCompressorContext(pInst,
									 pEncPrivateData->pRtParms,
									 pEncPrivateData->pPicContext,
									 jbEnv);
	} 
	else { /* Arrived through a longjmp -- Process the error */
#ifdef DEBUG
		HivePrintString("***FATAL ERROR in ");
		HivePrintString(gac8Files[(uStatus&FILE_MASK)>>FILE_OFFSET]);
		HivePrintString(", line");
		HivePrintDecInt((I32) ((uStatus&LINE_MASK)>>LINE_OFFSET));
		HivePrintString("\tError");
		HivePrintHexInt((uStatus&TYPE_MASK)>>TYPE_OFFSET);
		HivePrintString("\n");
#endif
		prs = PIA_S_INSTANCE_FATAL;		
		goto bail;
	}

	pEncPrivateData->bFirstTime = FALSE;

	/* Initialize the HIVE progress function */
	HiveEncodeProgressFunc(pInst, PCT_START, 0);

	prs = PIA_S_OK;
bail:
	return prs;

}    /*** End of EncodeSequenceSetup() ***/



/***********************************************************************
 *** EncodeSequenceEnd 
 *** 
 *** This function is called after encoding the last frame in a 
 *** sequence.  As data is freed in FreePrivateData, nothing is done here.
 ***
 *** pInst -- Pointer to an encoder instance.
 ***
 ***********************************************************************/ 

PIA_RETURN_STATUS EncodeSequenceEnd(PTR_ENC_INST pInst)
{

    PIA_RETURN_STATUS 		prs;

	/* Ensure pointer is initialized */
	if ((pInst == NULL) || (pInst->uTag!= ENC_INST_TAG)) {
		prs = PIA_S_ERROR;
		goto bail;
	}

	/* Close down the HIVE progress function */	

	HiveEncodeProgressFunc(pInst, PCT_END, 0);

	prs = PIA_S_OK;
bail:
	return prs;

}    /*** End of EncodeSequenceEnd() ***/



/***********************************************************************
 *** EncodeGetMaxCompressedSize
 *** 
 *** This function returns the maximum compressed size in bytes based
 *** on the image height and width.  This encoder will never exceed
 *** size when creating a compressed frame with these dimensions.
 ***
 *** uHeight -- Height of image to be compressed
 ***
 *** uWidth  -- Width of image to be compressed
 ***
 *** puSize  -- Pointer to the location where MaxCompressed size will
 ***			be written.
 ***
 ***********************************************************************/ 

PIA_RETURN_STATUS EncodeGetMaxCompressedSize(U32 uHeight, 
											 U32 uWidth,
											 PU32 puSize)
{

    PIA_RETURN_STATUS 		prs;

	/* Validate input to reasonable values */

	if ((uHeight < MIN_IMAGE_HEIGHT) ||
	    (uHeight > MAX_IMAGE_HEIGHT) ||
		(uWidth < MIN_IMAGE_WIDTH) ||
		(uWidth > MAX_IMAGE_WIDTH) ||
		(uWidth*uHeight > MAX_IMAGE_PIXELS) ||
		(puSize == NULL)) {
		prs = PIA_S_ERROR;
		goto bail;
	}
	
	/* Set puSize equal to the size of a YVU9 or YUV12 image
	 * depends on if YVU12 is supported,
	 * to be safe 
	 */

#ifdef SIMULATOR
	*puSize = (uHeight * uWidth * 12 * 2) / 8;
#else
	*puSize = (uHeight * uWidth * 9 * 2) / 8;
#endif /* SIMULATOR */

	prs = PIA_S_OK;
bail:
	return prs;

}    /*** End of EncodeGetMaxCompressedSize() ***/


/***********************************************************************
 *** EncodeQuery
 *** 
 *** This function is called to query if an encode operation is
 *** possible.  The encoder read from the input fields in pInst and 
 *** writes to the output fields in that struct.                 
 ***
 *** pInst 			   -- Pointer to an encoder instance.
 ***
 *** bSpeedOverQuality -- Indicates whether to prefer speed over quality
 *** 					  during the query.  If true, the encoder will
 ***					  set the query field to the choice that is 
 ***					  fastest.  If false, the encoder will set the 
 ***					  query field to the choice that is the best
 ***					  quality.
 ***
 ***********************************************************************/ 

PIA_RETURN_STATUS EncodeQuery(PTR_ENC_INST 					pInst, 
							  PTR_ENCODE_FRAME_INPUT_INFO 	pInput, 
							  PTR_ENCODE_FRAME_OUTPUT_INFO 	pOutput,
							  U32 uHeight,
							  U32 uWidth,
							  PIA_Boolean bSpeedOverQuality)
{
	PTR_PRIVATE_ENCODER_DATA	pPrivateInst = pInst->pEncoderPrivate;
	SUPPORTED_ALGORITHMS listFormats;
	PIA_Boolean bValidFormat;
	I32 i;
    PIA_RETURN_STATUS 		prs;
	const U32 MAX_KEYFRAME_RATE    = 65535;
	const U32 MAX_TARGET_DATA_RATE = 4000000;

	/* Validate parms */

	if ((pInst == NULL) ||
		((bSpeedOverQuality != TRUE) && (bSpeedOverQuality != FALSE))) {
		prs = PIA_S_ERROR;
		goto bail;
	}

	/* Validate inst tag */

	if ((pInst->uTag != ENC_INST_TAG) && (pInst->uTag != ENC_PARTIAL_INST_TAG)) {
		prs = PIA_S_ERROR;
		goto bail;
	}

	/* Deal with Color Format Queries */

	if (pInput->cfSrcFormat == CF_QUERY) {
		pInput->cfSrcFormat = CF_PLANAR_YVU9_8BIT;
	}

	/* Validate color format: both input and output */

	listFormats = pInst->eciInfo.listInputFormats;
	if (listFormats.u16Tag == SUPPORTED_ALGORITHMS_TAG) {
		bValidFormat = FALSE;
		for (i=0;i<listFormats.u16NumberOfAlgorithms; i++) {
			if (pInput->cfSrcFormat == listFormats.eList[i] ) {
				bValidFormat = TRUE;
				break;
			}
		}
		if (!bValidFormat) {
			prs = PIA_S_BAD_COLOR_FORMAT;
			goto bail;
		}
	} else {
		prs = PIA_S_ERROR;
		goto bail;
	}

	listFormats = pInst->eciInfo.listOutputFormats;
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
			goto bail;
		}
	} else {
		prs = PIA_S_ERROR;
		goto bail;
	}


	/* Validate Image Height and Width, Total number of Pixels */

	if ((uHeight > MAX_IMAGE_HEIGHT) || (uWidth > MAX_IMAGE_WIDTH) ||
		(uHeight < MIN_IMAGE_HEIGHT) || (uWidth < MIN_IMAGE_WIDTH) ||
		(uHeight*uWidth > MAX_IMAGE_PIXELS)) {
		prs = PIA_S_BAD_IMAGE_DIMENSIONS;  
		goto bail;
	}

	/* Validate Controls used against supported controls */

	if (pInst->ecscSequenceControlsUsed & ~(pInst->eciInfo.ecscSupportedSequenceControls)) {
		prs = PIA_S_UNSUPPORTED_CONTROL;
		goto bail;
	}

	/* Now, one by one, validate the values of the controls that are set */

	pInst->ecscBadSequenceControls = EC_FLAGS_VALID;  /* Init Bad Controls Flag */

	if (!(pInst->ecscSequenceControlsUsed & EC_FLAGS_VALID)) {  /* Valid bit must be set */
		prs = PIA_S_ERROR;
		goto bail;
	}

	if (pInst->keyKeyFrameRateKind != KEY_NONE) {  /* Check KeyFrame Rate */
		if (pInst->uKeyFrameRate > MAX_KEYFRAME_RATE) {
			prs = PIA_S_ERROR;
			goto bail;
		}
	}

	if (pInst->ecscSequenceControlsUsed & EC_TARGET_DATA_RATE) {  
		/* Check Bitrate Setting, clip it to MAX_TARGET_DATA_RATE = 4000000 */
		if (pInst->uTargetDataRate > MAX_TARGET_DATA_RATE)
			pInst->uTargetDataRate = MAX_TARGET_DATA_RATE;
	
	}


	if (pInst->ecscSequenceControlsUsed & EC_VIEWPORT) {  /* Check ViewPort Dims */
		if ((pInst->pEncParms->dViewportDim.w < 1) ||
			(pInst->pEncParms->dViewportDim.w > MAX_IMAGE_WIDTH) ||
			(pInst->pEncParms->dViewportDim.h < 1) ||
			(pInst->pEncParms->dViewportDim.h > MAX_IMAGE_HEIGHT) ||
			(pInst->pEncParms->dViewportDim.w*pInst->pEncParms->dViewportDim.h > MAX_IMAGE_PIXELS)) {
			pInst->ecscBadSequenceControls |= EC_VIEWPORT;
		}
	}

	if ((!(pInst->ecscSequenceControlsUsed & EC_TARGET_DATA_RATE)) &&
		(!(pInst->ecscSequenceControlsUsed & EC_ABSOLUTE_QUALITY_SLIDER))) {
		pInst->ecscBadSequenceControls |= EC_TARGET_DATA_RATE;
		pInst->ecscBadSequenceControls |= EC_ABSOLUTE_QUALITY_SLIDER;
	}

	/* Now, check ecfBadControlValues to see if anything didn't check out */

	if (pInst->ecscBadSequenceControls & ~EC_FLAGS_VALID) {
		prs = PIA_S_BAD_CONTROL_VALUE;
		goto bail;
	}

	prs = PIA_S_OK;
bail:
	return prs;

}    /*** End of EncodeQuery() ***/


/***********************************************************************
 *** EncodeFrame
 *** 
 *** Calling sequence parameters:
 ***
 *** pInst   -- Pointer to an encoder instance.
 ***
 *** pInput	 -- Pointer to an encode frame input info struct.  All
 ***			   fields, including the tag field, are set on entry.
 ***
 *** pOutput --	Pointer to an encode frame output info struct.  All
 ***			   fields, including the tag field, are set prior to return.
 *** 
 *** This function is called to compress an individual frame. It MUST
 *** return an encoded frame for each input frame.  Because look-ahead
 *** and BiDi prediction involve working with frames other than the
 *** current input frame, this process maintains two working areas:
 ***
 *** (1) the Strategy Buffers, which hold previously encoded pictures,
 ***     their interesting statistics and encoding parameters, and their
 ***     compressed bitstreams, until the latter are used to create
 ***     a frame to be returned as output from EncodeFrame(); and
 ***
 *** (2) the ReturnedFrame, in which the returned frame is assembled,
 ***     perhaps combining one or more frames from the Strategy Buffers.       
 ***
 *** The data flow in EncodeFrame() is as follows:
 ***
 *** (1) the incoming data is converted from YVU9 format to our internal
 ***     bit-plane representation (called a "picture"), which is stored
 ***     in the current strategy buffer (part of a ring of SBufs).
 ***
 *** (2) a sequencer selects the type of bitstream-frame to compress
 ***     this data into, selects the correct reference image(s), and
 ***     initializes the compressor for the desired frame type.
 ***
 *** NOTE: Step 3 below is not yet implemented, and the reference pictures are
 ***       maintained by the low level EncBand() routines.
 ***
 *** (3) the sequencer invokes the bitrate controller, if requested,
 ***     to set up the global quantization that will provide the best
 ***     quality consistent with the available output bandwidth;  This
 ***     usually involves some trial compressions of the input picture.
 ***
 *** (4) the sequencer then invokes the compressor with the desired
 ***     global quantizer, which results in the construction of a
 ***     compressed bitstream; the sequencer assembles all the relevant
 ***     information, and stores bitstream, parameters, and
 ***     statistics in a selected SBuf, for use in constructing a later
 ***     version of ReturnedSBuf.
 ***
 ***
 *** NOTE: Step 5 below is not yet implemented, and no adjustment is made to the G-Q,
 ***       nor is there any re-compression.
 ***
 *** (5) the sequencer looks at the returned bitstream, if look-ahead is
 ***     enabled, and adjusts the global quantizers stored with the
 ***     prior pictures for possible re-compression. The results of each
 ***     re-compression replace the previous copy of bitstream, statistics,
 ***     and reconstituted image in the same Strategy Buffer.
 ***
 *** (6) the sequencer then selects the appropriate Strategy Buffer to
 ***     return, and composes its bitstream accordingly.
 ***
 *** (7) EncodeFrame() copies the Returned SBuf's data into the PIA
 ***     output buffer prior to returning.
 ***
 ***********************************************************************/ 

PIA_RETURN_STATUS EncodeFrame(PTR_ENC_INST 					pInst, 
							  PTR_ENCODE_FRAME_INPUT_INFO 	pInput, 
							  PTR_ENCODE_FRAME_OUTPUT_INFO 	pOutput)
{
	  PTR_PRIVATE_ENCODER_DATA  pEncPrivateData;
	  PCmpStrategyStorageSt		pStrategyBuf;
 	  jmp_buf 					jbEnv;   /* For setjmp and longjmp error handling */
	  U32 						uStatus; /* Result of setjmp */
      PIA_RETURN_STATUS 		prs;
	  PEncRtParmsSt				pParms;
	  const U32 MAX_ABSOLUTE_QUALITY = 10000;

	/* Validate parms */

	if ( (pInst == NULL)  || (pInst->uTag != ENC_INST_TAG) ||
		 (pInput == NULL) || (pInput->uTag != ENCODE_FRAME_INPUT_INFO_TAG) ||
		 (pOutput == NULL) || (pInput->pu8UncompressedData == NULL)) {

		prs = PIA_S_ERROR;
		goto bail;
	}

	if (pInst->ecscSequenceControlsUsed & EC_ABSOLUTE_QUALITY_SLIDER) {  /* Check Absolute Quality */
		if (pInput->uAbsoluteQuality > MAX_ABSOLUTE_QUALITY) {
			prs = PIA_S_ERROR;
			goto bail;
		}
	}

	pParms = pInst->pEncoderPrivate->pRtParms;
	if (pInst->ecscSequenceControlsUsed & EC_ABSOLUTE_QUALITY_SLIDER)
		pParms->GlobalQuant = 23 - (U32) (pInput->uAbsoluteQuality * 23/10000);

	/* Make local pointer to the Encode-Frame Context (actually the Instance Private Data) */
	pEncPrivateData = pInst->pEncoderPrivate;
	/* and to the current strategy buffer */
	pStrategyBuf = &(pEncPrivateData->StrategyBuf);

	/* Compressing frame #... */
	pEncPrivateData->nPicNumber = pInput->uFrameNumber;

	/* Copy YVU9 input, which is contiguous, to 3 expanded buffers in the current SBuf.
	 * At the same time, scale each value by -128 (0x80) to center the ranges on 0.
	 * NOTE: This code assumes that the picture dimensions are already set up.
	 */
	ConvertFrameToPic(&(pStrategyBuf->CSBPicture[0]), pInput->pu8UncompressedData);

	/* Set the frame type here, initially.  If the caller requests that this be
	 * a key frame, this value will be "K", and will override any other selection
	 * in the sequencer.  Otherwise, it will be "P", and will usually be overridden
	 * in the sequencer.
	 */
	if (pInst->ecfcFrameControlsUsed & EC_FORCE_KEY_FRAME) {
		pEncPrivateData->uSequencerState = PIC_TYPE_K;
		pInst->ecfcFrameControlsUsed &= ~EC_FORCE_KEY_FRAME; /* turn it off for next frame */
	}

	/* If transparency support is active, copy Transparency BM from pInput to pCurSBuf. */

	if (pInst->ecscSequenceControlsUsed & EC_TRANSPARENCY ) {
		if (pInst->pEncParms->pTransparencyMask != NULL)	{
			if (pInst->pEncParms->pTransparencyMask->pu8TransBuffer!=NULL) {
				pStrategyBuf->pTransBM = pInst->pEncParms->pTransparencyMask->pu8TransBuffer;
			}
		}
	}


	/* Hook up the pointer to the overhead size */
	pEncPrivateData->pPicContext->pnFrameAudioOverhead = &(pInput->uOverheadSize);

	/* Hook up the pointer for the output bitstream */

	pStrategyBuf->CSBBitStream.bufPtr = pOutput->pu8CompressedData;

	/* Invoke the sequencer to select the frame type and perform the compression.
	 * Note: The Sbuf to compress is indicated in the instance data. It usually
	 *       is not the one returned.
	 */

	uStatus = setjmp(jbEnv);
	if (!uStatus) { /* 0 means the environment was set */
		CompressionSequencer(pInst, jbEnv);
	}
	else {	/* Arrived through a longjmp -- Process the error */
#ifdef DEBUG
		HivePrintString("***FATAL ERROR in ");
		HivePrintString(gac8Files[(uStatus&FILE_MASK)>>FILE_OFFSET]);
		HivePrintString(", line");
		HivePrintDecInt((I32) ((uStatus&LINE_MASK)>>LINE_OFFSET));
		HivePrintString("\tError");
		HivePrintHexInt((uStatus&TYPE_MASK)>>TYPE_OFFSET);
		HivePrintString("\n");
#endif
		prs = PIA_S_INSTANCE_FATAL;		
		goto bail;
	}

	pOutput->uFrameSize  = pStrategyBuf->CSBBitStream.bufSize;
	pOutput->bIsKeyFrame = (pStrategyBuf->nFrameType == PIC_TYPE_K);
	pOutput->uTag = ENCODE_FRAME_OUTPUT_INFO_TAG;

	/* Copy YVU9 input, which is contiguous, to 3 expanded buffers in the SBuf.
	 * This is the frame that will be used for temporal filtering by the next frame.
	 * At the same time, scale each value by -128 (0x80) to center the ranges on 0.
	 * NOTE: This code assumes that the picture dimensions are already set up.
	 */
	ConvertFrameToPic(&(pStrategyBuf->CSBReference[0]), pInput->pu8UncompressedData);


	prs = PIA_S_OK;
bail:
	return prs;

}    /*** End of EncodeFrame() ***/


/****************************************************************************
 *** BuildRtParms
 *** 
 *** This function is called to build the local RtParms data structure
 *** out of information supplied by the framework in the instance structure.
 ***
 *** pInst 			   -- Pointer to an encoder instance.
 ***
 *** Note: The RtParms structure is located in the instance's private data.
 ***
 *** Note: Most of this routine comes from ENSYNTAX.C, with a def from IOI.H
 ***
 ****************************************************************************/ 

static void BuildRtParms(PTR_ENC_INST pInst, COLOR_FORMAT cfSrcFormat, jmp_buf jbEnv) {

  PEncRtParmsSt		pParms;
  I32				iCnt;
  I32 i;
  U32 GlobalQuantThreshold[16] = {1, 5000, 10000, 20000, 30000, 40000, 50000, 75000, 
								  100000, 200000, 300000, 400000, 500000, 750000, 
								  1000000, 5000000};

	pParms = pInst->pEncoderPrivate->pRtParms;

	if (cfSrcFormat==CF_PLANAR_YVU9_8BIT) {
		pParms-> bYVU12 = FALSE;
	} else	if (cfSrcFormat==CF_PLANAR_YVU12_8BIT) {
		pParms-> bYVU12 =	TRUE;
	}

	/* Setup vars for transparency */

	if (pInst->ecscSequenceControlsUsed & EC_TRANSPARENCY ) 
		pParms->Transparency = TRUE;
	else
		pParms->Transparency = FALSE;

	if (pInst->pEncParms->bUseTransColor) {
		pParms->UseTransColor = TRUE; 
		pParms->TransColor = pInst->pEncParms->bgrTransparencyColor;	 	
	}
	else 
		pParms->UseTransColor = FALSE; 

	pParms->UseRVChange = 1;	  

	if ((pInst->keyKeyFrameRateKind == KEY_NONE) ||
		(pInst->uKeyFrameRate == 0)) {                         
		pParms->PeriodK = 0x0FFFFFFF; 
	} else {
		pParms->PeriodK = pInst->uKeyFrameRate;  
	}
	
	pParms->HTMb = -1;
	pParms->HTBlock = -1;
	pParms->FRVTbl = -1;

	pParms->MaxBudget = 153600;
	pParms->TargetBudget = 76800;
	pParms->DeltaTA = 0.00020;

	/* Init BRC params */

	if (pInst->ecscSequenceControlsUsed & EC_TARGET_DATA_RATE) {
		pParms->KByteRate = pInst->uTargetDataRate / 1024;
		pParms->BRCOn = TRUE;
		pParms->FrameTime = pInst->uFrameTime;
		pParms->FrameScale = pInst->uFrameScale;
	}
	else {
		pParms->BRCOn = FALSE;
		pParms->FrameTime = pInst->uFrameTime;
		pParms->FrameScale = pInst->uFrameScale;
	}

	pParms->KByteRatio = 10;
	pParms->PByteRatio = 8;
	pParms->P2ByteRatio = 8;
	pParms->DByteRatio = 6;

	pParms->MaxBuffer = 32000;

	pParms->BRCReactPos = 16; /* 1/16 on a scale of 256 */
	pParms->BRCReactNeg = 128; /* 1.0 on a scale of 256 */

	/* Initialize GlobalQuant. The initial GlobalQuant can vary between
	 * 2 and 16 according to the specified bit rate.
	 * (this will be overridden on a per frame basis if using the quality control.)
	 */
	
	pParms->GlobalQuant = 1;
	i = 15;
	while (pInst->uTargetDataRate < GlobalQuantThreshold[i]) {
		i--;
		pParms->GlobalQuant++;
	}
    pParms->GlobalQuant = CLIP(pParms->GlobalQuant, 2, 16);
	
	/* end init BRC params */


	if (pInst->ecscSequenceControlsUsed & EC_VIEWPORT) {  /* Set the  tile size */

		pParms->iaTileSize[0] = pInst->pEncParms->dViewportDim.w;
		pParms->iaTileSize[1] = pInst->pEncParms->dViewportDim.h;

		/* Now check, if the tile size is over or within some close tolerance (64) */
		/* of the image size in each dimension, then don't do tiling. */

		if (((U32) pParms->iaTileSize[0] + 64 >= pInst->dImageDim.w) &&
			((U32) pParms->iaTileSize[1] + 64 >= pInst->dImageDim.h)) {

			pParms->iaTileSize[0] = pParms->iaTileSize[1] = 0;
		}

	}	else {		 /* no tiling */
		pParms->iaTileSize[0] = pParms->iaTileSize[1] = 0;
	}
	

	EncSynValTileSize(&pParms->iaTileSize[0], &pParms->iaTileSize[1]);

	/*	Having read all the rt parms from input, set up all inferred rtparms.
		These are:
			 YLevels, YNumBands, YSubDiv, YMBSize, YBlockSize,YXfrm, YQuant
			 VULevels, VUNumBands, VUSubDiv, VUMBSize, VUBlockSize, VUXfrm, VUQuant 
	 */


	if (pInst->ecscSequenceControlsUsed & EC_SCALABILITY) {
		pParms->YLevels = 1;
	} else {
		pParms->YLevels = 0;
	}

	/* Choose a motion estimation method based upon the scalability is on or off */
	pParms->MeMeasure = ME_MAD;
	pParms->MeResolution = HALF_PEL_RES;
#ifndef SIMULATOR	 /* Default */
	if (pParms->YLevels==0) {/* Non-SC: 4-step logorithm search */
		pParms->MeSearch[0] = ME_FULL;
		pParms->MeInitSs = 64;	
	}	else if (pParms->YLevels==1) { /* SC case: 2-step logorithm search */
		pParms->MeSearch[0] = ME_FULL;
		pParms->MeInitSs = 16;
	}
#endif
	pParms->MeSearch[1] = 2;

	/* Set the search range to 8,8. */
	pParms->MeRange = 8;


			/* set up plane decomposition levels, numbands, subdiv, inheritance */
	pParms->YNumBands = 1;
	for(iCnt=0;iCnt<pParms->YLevels;iCnt++) {
		pParms->YSubDiv[iCnt] = SUBDIVQUAD;
		pParms->YNumBands += 3;
	}
	for(iCnt=pParms->YLevels;iCnt<pParms->YNumBands+pParms->YLevels;iCnt++) {
		pParms->YSubDiv[iCnt] = SUBDIVLEAF;
	}
	pParms->YSubDiv[pParms->YNumBands+pParms->YLevels] = SUBDIVEND;

	pParms->VULevels = 0;
	pParms->VUNumBands = 1;
	pParms->VUSubDiv[0] = SUBDIVLEAF;
	pParms->VUSubDiv[1] = SUBDIVEND;
	pParms->McUseBand0 = 1;
	pParms->QuantUseBand0 = 1;
	pParms->UseVarQuant = 1;
	
	/* set up U and V plane MacroBlocksize/Blocksize/Xfrm/QuantTable */
#ifdef SIMULATOR
			/* If YVU12, choose different block sizes and transforms */
			if ((pInst->pEncoderPrivate->PicDimensions[0].NumRows / 
				pInst->pEncoderPrivate->PicDimensions[2].NumRows) == 2) {

				for (i=0;i<pParms->VUNumBands; i++) {
							pParms->VUMBSize[i] = 8;
  							pParms->VUBlockSize[i] = 8;
							pParms->VUXfrm[i] = Default_VU12_Xfrm[i]; 
							pParms->VUQuant[i] = Default_VU12_Quant[pParms->VULevels][i]; 
				}							
			} else {  /* YVU9 */
				switch(pParms->YLevels) {
					case 0:  /* 0 level decomposition */
										pParms->VUMBSize[0] = 4;
  										pParms->VUBlockSize[0] = 4;
										pParms->VUXfrm[0] = Default_VU9_Xfrm[0]; 
										pParms->VUQuant[0] = Default_VU9_Quant[pParms->VULevels][0]; 
						break;
					case 1:  /* 1 level decomposition */
										pParms->VUMBSize[0] = 8;
  										pParms->VUBlockSize[0] = 4;
										pParms->VUXfrm[0] = Default_VU9_Xfrm[0]; 
										pParms->VUQuant[0] = Default_VU9_Quant[pParms->VULevels][0]; 
						break;
				}
			} 
#else		/* YVU9 only */
	switch(pParms->YLevels) {
		case 0:  /* 0 level decomposition */
							pParms->VUMBSize[0] = 4;
  							pParms->VUBlockSize[0] = 4;
							pParms->VUXfrm[0] = Default_VU9_Xfrm[0]; 
							pParms->VUQuant[0] = Default_VU9_Quant[pParms->VULevels][0]; 
			break;
		case 1:  /* 1 level decomposition */
							pParms->VUMBSize[0] = 8;
  							pParms->VUBlockSize[0] = 4;
							pParms->VUXfrm[0] = Default_VU9_Xfrm[0]; 
							pParms->VUQuant[0] = Default_VU9_Quant[pParms->VULevels][0]; 
			break;
	}
#endif /* SIMULATOR */

		/* Select the frame type pattern based upon scalability */
		/* and frame rate. */

		if (pInst->ecscSequenceControlsUsed & EC_SCALABILITY) {
			/* Scalable clips will have different frame type patterns */
			/* depending on 1 factor: frame rate. */
			/* This factor is used to determine the number of levels */
			/* of frame scalability the is appropriate for the situation. */

			if (1000000 / pParms->FrameTime < 10) {
				/* For frame rates less than 10, no frame scalability */
				/* is necessary. */
				pParms->PeriodP = 1;	/* This gives KPPPPPPPP... */
				pParms->PeriodP2 = 999999999;
			} else if (1000000 / pParms->FrameTime < 20) {
				/* For frame rates less than 20, use 1 level of scalability */
				pParms->PeriodP = 3;	/* This gives KP2P2PP2P2P... */
				pParms->PeriodP2 = 1;
			} else {
				/* For frame rates greater than or equal to 20, use */
				/* 2 levels of scalability */
				pParms->PeriodP = 6;	/* This gives KDP2DP2DP... */
				pParms->PeriodP2 = 2;
			}

		} else {
			/* Nonscalable clips will have different frame type patterns */
			/* depending on 2 factors: frame size, and frame rate. */
			/* These factors are used to determine whether D frames */
			/* should be present for performance reasons on lower end */
			/* systems.  D frames help performance on lower end systems */
			/* by giving the decoder the option to drop frames. */

			if (pInst->dImageDim.w * pInst->dImageDim.h < 45000) {
				/* If the image size is less than 45000 pixels (slightly */
				/* larger than 240x180), then never use D frames, at any */
				/* frame rate. */
				pParms->PeriodP = 1;	/* This gives KPPPPPPPP... */
				pParms->PeriodP2 = 999999999;

			} else {
				/* If the image size is greater than 45000 pixels (slightly */
				/* larger than 240x180), then use D frames, depending on the */
				/* frame rate. */
				if (1000000 / pParms->FrameTime < 10) {
					/* For frame rates less than 10, do not use D frames */
					pParms->PeriodP = 1;	/* This gives KPPPPPPPP... */
					pParms->PeriodP2 = 999999999;
				} else {
					/* For frame rates greater than or equal to 10, use D frames */
					pParms->PeriodP = 2;	/* This gives KDPDPDPDP... */
					pParms->PeriodP2 = 999999999;
				}
			}
		}


	/* set up band xfrm and quant table sets for Y: use default for now */
	for (i=0;i<pParms->YNumBands; i++) {
		pParms->YXfrm[i] = Default_Y_Xfrm[i];
		pParms->YQuant[i] = Default_Y_Quant[pParms->YLevels][i];
	}

		/* set up Y band macroblock sizes, block sizes */
	switch(pParms->YLevels) {
		case 0:  /* 0 level decomposition */
			pParms->YMBSize[0] = 16;
			pParms->YBlockSize[0] = 8;
			break;
		case 1:  /* 1 level decomposition */
			for (i=0;i<4;i++) /* 4 bands */ {
				pParms->YMBSize[i] = 16;
				pParms->YBlockSize[i] = 8;
			}
			break;
	}
			
		/* SIMULATOR option, set constant block size if selected */
#ifdef SIMULATOR
	if(pInst->pEncoderPrivate->pEncSimInst->SimParms.bBlockSizeSet) {
				/* This option may require no_inheritance from band zero */
		switch(pParms->YLevels) {
			case 1:
					/* set block size */
					pParms->YBlockSize[3] = pInst->pEncoderPrivate->pEncSimInst->SimParms.iBlockSize;
					pParms->YBlockSize[2] = pInst->pEncoderPrivate->pEncSimInst->SimParms.iBlockSize;
					pParms->YBlockSize[1] = pInst->pEncoderPrivate->pEncSimInst->SimParms.iBlockSize;
					/* fall through */
			case 0:
			default:
				/* set block size */
					pParms->YBlockSize[0] = pInst->pEncoderPrivate->pEncSimInst->SimParms.iBlockSize;
					/* fall through and done */
		}	/* end of switch */
	}  /* end of if */
#endif /* #ifdef SIMULATOR */		

			/* SIMULATOR option, set constant macro block size if selected */
#ifdef SIMULATOR
	if(pInst->pEncoderPrivate->pEncSimInst->SimParms.bMacroBlockSizeSet) {
				/* This option may require no_inheritance from band zero */
		switch(pParms->YLevels) {
			case 1:
				pParms->YMBSize[3] = pInst->pEncoderPrivate->pEncSimInst->SimParms.iMacroBlockSize;
				pParms->YMBSize[2] = pInst->pEncoderPrivate->pEncSimInst->SimParms.iMacroBlockSize;
				pParms->YMBSize[1] = pInst->pEncoderPrivate->pEncSimInst->SimParms.iMacroBlockSize;
				/* fall through */
			case 0:
			default:
				pParms->YMBSize[0] = pInst->pEncoderPrivate->pEncSimInst->SimParms.iMacroBlockSize;
					/* fall through and done */
		} /* end of switch */
	} /* end of if */
#endif /* #ifdef SIMULATOR */		
			
			/* set Motion estimation resolution */
#ifdef SIMULATOR 
	if(pInst->pEncoderPrivate->pEncSimInst->SimParms.iMERes == 2) {
		pParms->MeResolution = HALF_PEL_RES;
	} else {
		pParms->MeResolution = INTEGER_PEL_RES;
	}
			
#endif /* #ifdef SIMULATOR */		

	/*	End set up all inferred rtparms */

#ifdef SIMULATOR /* no inheritance */
	/* if using the no inheritance simulator option */
	if(pInst->pEncoderPrivate->pEncSimInst->SimParms.bNoInherit) {
		/* then force no inheritance */
		pParms->McUseBand0 = 0;
	}
#endif /* SIMULATOR */

}    /*** End of BuildRtParms() ***/



/***********************************************************************
 *** SelectPitch
 *** 
 *** This function is called to expand the image planar dimensions for 
 *** memory allocation.  It embodies the convention concerning image row
 *** and column edges, which it expands to the size of an integral number
 *** of macroblocks (which are themselves assumed to be square of 16x16 at max.).
 ***
 *** NOTE: The pitch returned is the column-pitch (size in entries of
 ***       the expanded column). The row pitch, without which you can't
 ***       calculate the actual memory allocation, is NOT returned!
 ***
 *** NOTE: The column pitch is "entries", NOT "bytes" or "words", and
 ***       so is independent of the actual pixel size. Note that this
 ***       pitch is re-calculated in MatAlloc (MATRIX.C), prior to
 ***       being used, and with an assumed padding to 8's, not 16's!
 ***
 ***********************************************************************/ 

static U32 SelectPitch(DimensionSt dim)
{	 
U8 const MACROBLOCK_SIZE=16;

	  U32	uExpandedNumberOfRows, uExpandedNumberOfCols;

	uExpandedNumberOfRows = (dim.h + MACROBLOCK_SIZE - 1) & ~(MACROBLOCK_SIZE-1);
	uExpandedNumberOfCols = (dim.w + MACROBLOCK_SIZE - 1) & ~(MACROBLOCK_SIZE-1);
	return (uExpandedNumberOfCols);

}    /*** End of SelectPitch() ***/



/***********************************************************************
 *** EncSynValTileSize
 *** 
 *** This function is called to validate the tile sizes in the RtParms 
 ***
 ***********************************************************************/ 
static void EncSynValTileSize(PI32 piWidth, PI32 piHeight)
{
	U32 iW;	/* local copies */
	U32 iH;	/* local copies */
	/*
	 * - height and width must be either 64, or 128 or 256.
	 * - tile size need not be an integral size of the picture size, i.e.,
	 *   tiles can drop off the right and/or the bottom of the picture.
	 * - tile size <= picture size
	 * - valid choices for height and width are:
	 *		no tile (or 1 tile == picture size)
	 *  	use width of picture
	 *  	use height of picture
	 *  	some numeric value
	 */
	iW = (U32) *piWidth;
	iH = (U32) *piHeight;

	iW = (iW + 32) & (~0x3f);	/* make the nearest multiple of 64 */
	iH = (iH + 32) & (~0x3f);	/* make the nearest multiple of 64 */

	if (iW!=iH) {  /* width and height must be the same */
		iW = iH = (iW<iH)?  iW : iH;  /* take the smaller one */
	}

	if (iW>256) {  /* max 256 */
		iW = iH = 256;
	}


	*piWidth = (I32) iW;
	*piHeight = (I32) iH;
	/*
	 * iaTileSize contains valid tile size dimensions at this point
	 *
	 * Values of 0 indicate to use the picture size (not known at this time)
	 * for the corresponding dimension.
	 */
	return;

}    /*** End of EncSynValTileSize() ***/



/***************************************************************************
 *** ConvertFrameToPic
 *** 
 *** This function copies a frame of YVU9 input, which is contiguous,
 *** to 3 expanded buffers in some MatrixSt selected by pPicture.
 *** At the same time, it scales each value by -128 (0x80) to center the
 *** ranges of pixel values on 0.
 ***
 *** No input checking is performed.
 ***
 *** NOTE: This code assumes that the Picture dimensions are already set up.
 ***
 ***************************************************************************/ 

static void  ConvertFrameToPic( PMatrixSt pPicture, PU8 pYVU9Data ) {

		NaturalUnsigned uPlane;
		NaturalUnsigned uRow, uCol;
		PI16		    pIndeo5Input;
		NaturalUnsigned uNumCols, uPitch;

	for (uPlane=0;uPlane<3;uPlane++) {
		uPitch = pPicture[uPlane].Pitch;
		uNumCols = pPicture[uPlane].NumCols;
#ifdef SIMULATOR

		/* TODO  Sort this out...  The code below swaps the U and V
		 planes because they are swapped in I420 files from what
		 they are in YVU9 files.  The input color convertors 
		 currently produce the same backward format as I420 and
		 that is undone here.  The output color convertors in 
		 the C Decoder reswap the UV data so that when written 
		 back to I420, it is once again as expected.
		
		 IVI and YVU9 are Y V U, I420 is Y U V
		
		 Currently the code works like this:
		
		 RGB -> YVU9 -> IVI (YVU) -> YVU9 (YVU) -> RGB
		 RGB -> YUV12 -> IVI (YVU) -> YVU12 -> I420 (YUV)
		 I420 (YUV) -> YUV12 -> IVI (YVU) -> YVU12 -> I420 (YUV)
		
		 Clearly this needs to be sorted out.  Will there be a 
		 YVU12 codec? */
		 		
		if (((pPicture[0].NumRows / pPicture[2].NumRows) == 2) &&
			(uPlane != 0)) {
			if (uPlane == 1)
				pIndeo5Input = pPicture[2].pi16;
			else 
				pIndeo5Input = pPicture[1].pi16;
		} else {
			pIndeo5Input = pPicture[uPlane].pi16;
		}
#else 	
		pIndeo5Input = pPicture[uPlane].pi16;
#endif /* SIMULATOR */

		for (uRow=0;uRow<(unsigned int)(pPicture[uPlane].NumRows);uRow++) {
			for (uCol=0; uCol< uNumCols; uCol++) {
				*pIndeo5Input++ = (I16)(*pYVU9Data++ - 128);
			}
			pIndeo5Input += uPitch - uNumCols;
		}
	}

}    /*** End of ConvertFrameToPic() ***/




/***************************************************************************************
 *
 *	CompressionSequencer
 *
 *    The sequencer chooses the frame type(s) to encode, handles buffering of the
 *    intermediate- and output data.
 *
 *    It is designed to be re-entrant, and uses recursion whenever the current frame
 *    type to be created changes from what was predicted on the immediately preceding
 *    call. User-requested key frames, described below, are an example of one such
 *    situation.
 *
 *    The frame-type sequence re-starts with each K, requested or cyclic.
 *
 ***************************************************************************************/

static void CompressionSequencer(PTR_ENC_INST pInst, jmp_buf jbEnv) 
{

	PCmpStrategyStorageSt	  pStrategyBuf;
	PTR_PRIVATE_ENCODER_DATA  pEncPrivateData = pInst->pEncoderPrivate;
	PEncRtParmsSt			  pParms = pInst->pEncoderPrivate->pRtParms;
	I32					      iPlane;

	/* Make a local ptr to the Strategy Buffer, */
	pStrategyBuf = &(pEncPrivateData->StrategyBuf);

	/* Do temporal filtering unless this is first frame */
	if (pEncPrivateData->nPicNumber != 0) {
#ifdef SIMULATOR 
		for (iPlane = 0; iPlane < 3; iPlane++) {
			if (EncSimTempPrefilt(pInst->pEncoderPrivate->pEncSimInst)) {
				TempPreFilter(pStrategyBuf->CSBPicture[iPlane], pStrategyBuf->CSBReference[iPlane], iPlane);
			}
		}
#else
		for (iPlane = 0; iPlane < 3; iPlane++) {
			TempPreFilter(pStrategyBuf->CSBPicture[iPlane], pStrategyBuf->CSBReference[iPlane], iPlane);
		}
#endif /* SIMULATOR */

	}

	pStrategyBuf->nFrameType = pEncPrivateData->uSequencerState;

	if (pEncPrivateData->uSequencerState == PIC_TYPE_K)
		pEncPrivateData->nKPeriodCntr = 0;		/* Reset with each K frame */ 

	/* If the BRC is enabled, use it to select the Global Quantizer. */
	if (pParms->BRCOn == TRUE)	{
		ControlBitRate(pStrategyBuf, pInst, jbEnv);
	}

	/* Compress the frame */
	CmpressPicture(pInst, pStrategyBuf, pEncPrivateData->pPicContext, pParms->BRCOn, jbEnv);

	/* Update the frame counters, and use it to choose the next state */
	pEncPrivateData->nKPeriodCntr++;	

	/* Update the frame counters, and use them to choose the next state */
	if (pEncPrivateData->nKPeriodCntr % pParms->PeriodK ) {
		if (pEncPrivateData->nKPeriodCntr % pParms->PeriodP ) {
			if (pEncPrivateData->nKPeriodCntr % pParms->PeriodP2 ) {
				pEncPrivateData->uSequencerState = PIC_TYPE_D;
			} else {
				pEncPrivateData->uSequencerState = PIC_TYPE_P2;
			}
		} else {
			pEncPrivateData->uSequencerState = PIC_TYPE_P;
		}
	} else {
		pEncPrivateData->uSequencerState = PIC_TYPE_K;
	}

#ifdef SIMULATOR /* diagnostic */
	EncDumpFrameType(pStrategyBuf->nFrameType, pEncPrivateData->pPicContext->pSimInst);
#endif /* SIMULATOR */

	return;

}    /*** End of CompressionSequencer() ***/


/* access key */
U32 const Def_AccessKey = 0;
PIA_RETURN_STATUS EncodeGetDefAccessKey( PTR_ENC_INST pInst, 
										PU32 puAccessKey )
{
	if (pInst->ecscSequenceControlsUsed  & EC_ACCESS_KEY) {
		*puAccessKey = Def_AccessKey;
	}
		return(PIA_S_OK);

}

PIA_RETURN_STATUS EncodeGetAccessKey( PTR_ENC_INST pInst, 
									 PU32 puAccessKey )
{

	if (pInst->ecscSequenceControlsUsed  & EC_ACCESS_KEY) {
		*puAccessKey = pInst->pEncParms->uAccessKey;
	}
	return(PIA_S_OK);	

}


PIA_RETURN_STATUS EncodeSetAccessKey( PTR_ENC_INST pInst, 
									 U32 uAccessKey )
{
	if (pInst->ecscSequenceControlsUsed  & EC_ACCESS_KEY) {
		pInst->pEncParms->uAccessKey = uAccessKey;
	}
	return(PIA_S_OK);	

}

/* Debug */
PIA_RETURN_STATUS EncodeDebugGet( PTR_ENC_INST pInst, 
								 PVOID_GLOBAL vgpVoidPtr )
{
	return(PIA_S_UNSUPPORTED_FUNCTION);	
}

PIA_RETURN_STATUS EncodeDebugSet( PTR_ENC_INST pInst, 
								 PVOID_GLOBAL vgpVoidPtr )
{
	return(PIA_S_UNSUPPORTED_FUNCTION);	
}


/* scalability */
PIA_RETURN_STATUS EncodeGetDefScaleLevel( PTR_ENC_INST pInst, PU32 puScaleLevel )
{
	if (pInst->ecscSequenceControlsUsed & EC_SCALABILITY) {
		*puScaleLevel = 1;
	} else {
		*puScaleLevel = 0;
	}
	return(PIA_S_OK);	
}

PIA_RETURN_STATUS EncodeGetScaleLevel( PTR_ENC_INST pInst, PU32 puScaleLevel )

{
	*puScaleLevel = pInst->pEncParms->uScaleLevel;
	return(PIA_S_OK);	
}

PIA_RETURN_STATUS EncodeSetScaleLevel( PTR_ENC_INST pInst, U32 uScaleLevel  )
{
	pInst->pEncParms->uScaleLevel = uScaleLevel;
	return(PIA_S_OK);	
}

/* transparnecy */
PIA_RETURN_STATUS EncodeSetTransColor( PTR_ENC_INST pInst, BGR_ENTRY bgrColor )
{
	pInst->pEncParms->bUseTransColor = TRUE;
	pInst->pEncParms->bgrTransparencyColor = bgrColor;
	return(PIA_S_OK);

}

/* transparency mask */
PIA_RETURN_STATUS EncodeSetTransBitmask( PTR_ENC_INST pInst, PTR_TRANSPARENCY_MASK pTransMask )
{
	pInst->pEncParms->pTransparencyMask = pTransMask;
	return(PIA_S_OK);
}

/* view port for local decode */
PIA_RETURN_STATUS EncodeGetDefViewPort( PTR_ENC_INST pInst, 
										PDimensionSt pdViewPort )
{
	pdViewPort->w = pdViewPort->h =0;
	return(PIA_S_OK);
}

PIA_RETURN_STATUS EncodeGetMinViewPort( PTR_ENC_INST pInst, 
										PDimensionSt pdViewPort )
{
	pdViewPort->w = pdViewPort->h =0;
	return(PIA_S_OK);
}


PIA_RETURN_STATUS EncodeGetViewPort( PTR_ENC_INST pInst, 
										   PDimensionSt pdViewPort )
{
	PTR_PRIVATE_ENCODER_DATA pEnc=pInst->pEncoderPrivate;
	*pdViewPort = pInst->pEncParms->dViewportDim;
	return(PIA_S_OK);
}

PIA_RETURN_STATUS EncodeSetViewPort( PTR_ENC_INST pInst, 
										   DimensionSt dViewPort )
{
	PTR_PRIVATE_ENCODER_DATA pEnc=pInst->pEncoderPrivate;
	pInst->pEncParms->dViewportDim = dViewPort ;
	return(PIA_S_OK);
}

/* target playforms */
PIA_RETURN_STATUS 
EncodeGetDefTargetPlaybackPlatform(PTR_ENC_INST pInst,PU32 puTargetPlatform)
{
	return(PIA_S_UNSUPPORTED_CONTROL);
}

PIA_RETURN_STATUS 
EncodeGetTargetPlaybackPlatform(PTR_ENC_INST pInst,PU32 puTargetPlatform)
{
	return(PIA_S_UNSUPPORTED_CONTROL);
}

PIA_RETURN_STATUS 
EncodeSetTargetPlaybackPlatform(PTR_ENC_INST pInst,U32 uTargetPlatform)
{
	return(PIA_S_UNSUPPORTED_CONTROL);
}
 

/***********************************************************************
 *** FreeEncPrivateData
 *** 
 *** This function is called so an encoder instance can free its 
 *** private data but not the pass down parameters from HIVE.
 ***
 *** pInst -- Pointer to an encoder instance.
 ***
 ***********************************************************************/ 

static PIA_RETURN_STATUS FreeEncPrivateData(PTR_ENC_INST pInst)
{
	  I32						iPlane;
	  PTR_PRIVATE_ENCODER_DATA  pEncPrivateData;
	  PCmpStrategyStorageSt		pStrategyBuf;
	  PIA_RETURN_STATUS 		prs;
	  jmp_buf					jbEnv;
	  U32						uStatus;

	/* Ensure pointer is valid */
	if (pInst == NULL) {
		prs = PIA_S_ERROR;
		goto bail;
	}

	/* Make a local de-referenced pointer to the private data */
	if (pEncPrivateData = pInst->pEncoderPrivate) {

		uStatus = setjmp(jbEnv);
		if (!uStatus) { /* 0 means the environment was set */
			/* Walk down EncoderPrivateData and free all embedded memory */

			if (pEncPrivateData->pRtParms)
				SysFree((PU8)pEncPrivateData->pRtParms, jbEnv);
			/* Free everything in the Compress Picture Context, then the context */
			if (pEncPrivateData->pPicContext) {
				ClosePictureCompressorContext(pEncPrivateData->pPicContext, jbEnv);
				SysFree((PU8)pEncPrivateData->pPicContext, jbEnv);
			}

#ifdef SIMULATOR /* Simulation infrastructure */
			/* Free up the simulator only data */

			SimIOClose(pEncPrivateData->pEncSimInst->pSFile);
		
			SysFree((PU8) pEncPrivateData->pEncSimInst->pSFile, jbEnv);
			SysFree((PU8) pEncPrivateData->pEncSimInst, jbEnv);

#endif /* SIMULATOR */

			/* Free all the picture structures embedded in the SBufs, one by one */
			pStrategyBuf = &(pEncPrivateData->StrategyBuf);
 			for (iPlane = 0; iPlane < 3; iPlane++) {
				if (pStrategyBuf->CSBPicture[iPlane].pi16)
					SysFree((PU8) pStrategyBuf->CSBPicture[iPlane].pi16, jbEnv);
				if (pStrategyBuf->CSBRebuiltPic[iPlane].pi16)
					SysFree((PU8) pStrategyBuf->CSBRebuiltPic[iPlane].pi16, jbEnv);
				if (pStrategyBuf->CSBReference[iPlane].pi16)
					SysFree((PU8) pStrategyBuf->CSBReference[iPlane].pi16, jbEnv);
			}

			/* Finally, free the private data area itself... */
			if (pEncPrivateData)
				SysFree((PU8)pEncPrivateData, jbEnv);
			pInst->pEncoderPrivate = 0;
		}
		else { /* Arrived through a longjmp -- Process the error */
#ifdef DEBUG
			HivePrintString("***FATAL ERROR in ");
			HivePrintString(gac8Files[(uStatus&FILE_MASK)>>FILE_OFFSET]);
			HivePrintString(", line");
			HivePrintDecInt((I32) ((uStatus&LINE_MASK)>>LINE_OFFSET));
			HivePrintString("\tError");
			HivePrintHexInt((uStatus&TYPE_MASK)>>TYPE_OFFSET);
			HivePrintString("\n");
#endif /* DEBUG */
			prs = PIA_S_INSTANCE_FATAL;		
			goto bail;
		}
	}

	prs = PIA_S_OK;
bail:
	return prs;

}    /*** End of FreeEncPrivateData() ***/


/***********************************************************************
 *** FreePassDownData
 *** 
 *** This function is called so an encoder instance can free  
 *** the pass down parameters from HIVE.
 ***
 *** pInst -- Pointer to an encoder instance.
 ***
 ***********************************************************************/ 

static PIA_RETURN_STATUS FreeEncPassDownData(PTR_ENC_INST pInst)
{
	  PIA_RETURN_STATUS 		prs;
	  PTR_PASS_DOWN_ENC_PARMS   pEncParms;
	  jmp_buf					jbEnv;
	  U32						uStatus;

	/* Ensure pointer is valid */
	if (pInst == NULL) {
		prs = PIA_S_ERROR;
		goto bail;
	}

	/* Make a local de-referenced pointer to the private data */
	pEncParms = pInst->pEncParms;

	uStatus = setjmp(jbEnv);
	if (!uStatus) { /* 0 means the environment was set */
		/* free all embedded memory */

		/* Finally, free the  data area itself... */
		if (pEncParms) {
			SysFree((PU8)pEncParms, jbEnv);
			pInst->pEncParms = 0;
		}
	} 
	else { /* Arrived through a longjmp -- Process the error */
#ifdef DEBUG
		HivePrintString("***FATAL ERROR in ");
		HivePrintString(gac8Files[(uStatus&FILE_MASK)>>FILE_OFFSET]);
		HivePrintString(", line");
		HivePrintDecInt((I32) ((uStatus&LINE_MASK)>>LINE_OFFSET));
		HivePrintString("\tError");
		HivePrintHexInt((uStatus&TYPE_MASK)>>TYPE_OFFSET);
		HivePrintString("\n");
#endif /* DEBUG */
		prs = PIA_S_INSTANCE_FATAL;		
		goto bail;
	}

	prs = PIA_S_OK;
bail:
	return prs;

}    /*** End of FreeEncPassDownData() ***/


