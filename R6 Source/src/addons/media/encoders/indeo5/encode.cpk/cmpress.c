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
 * cmpress.c
 *
 * Upper level of the encoder -- produce a compressed bitstream for
 * a single frame.
 *
 * OpenPictureCompressorContext		Allocate the Picture Context
 * ClosePictureCompressorContext	Free the picture context
 * CmpressPicture					Compress a single frame 
 * ControlBitRate					Prep work for CmpressPicture
 */

#include	<setjmp.h>
#include	<string.h>
#include <stdio.h>

#include	"datatype.h"	
#include	"matrix.h"
#include	"tksys.h"
#include	"const.h"

#include 	"mc.h"				/* for ivi5bs.h */
#include 	"hufftbls.h"		
#include 	"qnttbls.h"
#include 	"ivi5bs.h"			/* for bsutil.h */
#include 	"mbhuftbl.h"
#include 	"bkhuftbl.h"

#ifdef SIMULATOR
#include	"encsmdef.h"
#include	"decsmdef.h"
#endif	  /* SIMULATOR */
#include	"bsutil.h"

#include	"pia_main.h"
#include	"indeo5.h"
#ifdef CMD_LINE_ENC
#include "CmdLParm.h"  
#endif	/*CMD_LINE_ENC*/

#include 	"ensyntax.h"
#include	"encoder.h"
#include	"enme.h"
#include	"enmesrch.h"
#include	"enseg.h"
#include    "context.h"
#include 	"entrans.h"
#include	"enntry.h"
#include	"encpia.h"
#include	"pia_enc.h"			
#include	"enntryns.h"
#include	"enwav.h"
#include	"enbs.h"
#include    "errhand.h"
#include 	"common.h"

#ifdef SIMULATOR
#include    "encsim.h"
#endif	 /* SIMULATOR */

const Dbl dFrame1DRC = 1.0E10;	/* DRC for first frame */
const Dbl dInitDRC = 50.0;		/* Initial DRC for after first frame */
const Dbl dMinDRC = 20.0;		/* Smallest dDRC value allowed */
const I32 iMaxCycleBudget =	1000000; 	/* Maximum cycle budget */
const I32 iMinCycleBudget = -1000000;	/* Minimum cycle budget */


#define CLIP(a,lo,hi) ((a)<(lo)?(lo):((a)>(hi)?(hi):(a)))

/* Public routine prototypes */
void OpenPictureCompressorContext( PTR_ENC_INST			pInst,
								   PEncRtParmsSt		pRtParms,
								   PEncPictureCntxSt	pPicContext,
								   jmp_buf 				jbEnv);

void ClosePictureCompressorContext(PEncPictureCntxSt	pPicContext,
								   jmp_buf				jbEnv);

void CmpressPicture			 ( 	    PTR_ENC_INST 	      pInst,
									PCmpStrategyStorageSt pStrategyBuf,
								    PEncPictureCntxSt	  pPicContext,
									PIA_Boolean		      bBRCOn,
								    jmp_buf				  jbEnv);

void ControlBitRate				 ( PCmpStrategyStorageSt pStrategyBuf,
					 			   PTR_ENC_INST 		 pInst,
								   jmp_buf				 jbEnv);


/* Internal routine prototypes */

static void CopyBitBuffs (PU8 pu8Dest, PPicBSInfoSt pBsPicInfo, jmp_buf jbEnv);
static void CalcBitBuffSize(PPicBSInfoSt pBsPicInfo, 
							PU32 puTotalSize, 
							PU32 puNonDbgSize, 
							jmp_buf jbEnv);
static void AddPictureSize (U32 uDataSize, pBitBuffSt pBsPicHdr, jmp_buf jbEnv);
static void SetTileSizes (PEncRtParmsSt pRtParms, pGOPHdrSt pGOPHdr);

#ifdef SIMULATOR 
	I32 aiTypeDelta[5] = {0,0,0,0,0};
#else
	const I32 aiTypeDelta[5] = {0,0,0,0,0};
#endif /* SIMULATOR */

/* ---------------------- COMPRESS PICTURE -------------------------*/
/*
 * This routine is called to actually produce a compressed bit
 * stream for a single frame.  Its inputs are (1) a strategy buffer with
 * a raw frame to be compressed, (2) some parms that control the type
 * of output frame, specify some frame-order information that goes in
 * the bitstream header, and that are used for compressor tuning, and
 * (3) an array of target sizes for the encoding of each color plane.
 * These parms (2) and (3) are all in the picture context.
 *
 * This routine is context-free. It can be called to speculatively
 * compress a frame for analysis as many times as needed, and will
 * return the same bitstream each time, given the same input setup.
 *
 * It returns a BitStrmBuf structure (just a length + a buffer ptr)
 * embedded in the Strategy Buffer.  The buffer is NOT dynamically
 * allocated at each compression! (Which means that it only needs
 * to be free'd when the encoder shuts down for the night...)
 *
 */ 


void CmpressPicture(  PTR_ENC_INST pInst,
					  PCmpStrategyStorageSt	pStrategyBuf,
					  PEncPictureCntxSt		pPicContext,
					  PIA_Boolean			bBRCOn,
					  jmp_buf				jbEnv )

{
	PPicBSInfoSt   	pBsPicInfo  = &pPicContext->FrameBSInfo;
	pPicHdrSt      	pPicInfoHdr = &pBsPicInfo->Hdr;
	PTR_PRIVATE_ENCODER_DATA  pEncPrivateData = pInst->pEncoderPrivate;
	pBitStrmBuf    	newBsPtr;
	I32            	iPlane;
	I32			 	iCkSum;
	I32				iFrameType;
	U32			 	uSize, uNonDbgSize;		/* size of bitstream picture info */
 	Dbl dError;
	PIA_Boolean bDoDRC;			/* Set to true if DRC should be done */
	const PIA_Boolean bLastBSFormat = TRUE;  /* Final bitstream format */
	U8  u8MaxQuant = 23;
	U8  u8MaxColorQuant = 23;

	pPicInfoHdr->PictureType = (U8) pStrategyBuf->nFrameType;
	
	/*
	 * The following function takes an increasing sequence of numbers, starting
	 * with 0 (at each key frame), and produces the following sequence of numbers:
	 * 0, 1, 2, ... , 254, 255, 1, 2, 3, ... , 254, 255, 1, 2 
	 * This is used for the bitstream frame number, which is 0 for Key frames,
	 * and increments in an 8 bit field for all other frame types, wrapping
	 * to the value 1 (NOT 0, because only key frames are zero) when the 8 bit
	 * field overflows
	 */

	pPicInfoHdr->PictureNum  = ((pInst->pEncoderPrivate->nKPeriodCntr - 1) % 255) + 1;
	
	pPicInfoHdr->RefPictureNum = 0;  /* Default case */

	if (pPicInfoHdr->PictureType == PIC_TYPE_P) {
		pPicInfoHdr->RefPictureNum = pPicContext->EncPlaneCntx[0]->EncBandCntx[0]->PicRefNum[1];  
	} else if ((pPicInfoHdr->PictureType == PIC_TYPE_P2) ||
			   (pPicInfoHdr->PictureType == PIC_TYPE_D)) {
		pPicInfoHdr->RefPictureNum = pPicContext->EncPlaneCntx[0]->EncBandCntx[0]->PicRefNum[0];  
	}

	if (pPicInfoHdr->RefPictureNum == 255) 
		pPicInfoHdr->RefPictureNum = 0;  /* Handle wrap around properly in subsequent calculations */

	newBsPtr = &(pStrategyBuf->CSBBitStream);

	if (pPicInfoHdr->PictureType != PIC_TYPE_R) {
#ifdef SIMULATOR
		pBsPicInfo->pSimInst = pEncPrivateData->pEncSimInst;
		u8MaxQuant = (U8) EncSimMaxQuant(pPicContext->pSimInst);
		u8MaxColorQuant = (U8) EncSimMaxColorQuant(pPicContext->pSimInst);
		EncSimFrameQuantBias(pPicContext->pSimInst, aiTypeDelta);
#endif /* SIMULATOR */

		pPicInfoHdr->GlobalQuant = (U8)(* pPicContext->pnGlobalQuant);
		/* determine inheritance at picture level if BRC is not on.
		 * Pass this if the BRC is on because it was already determined in the 
		 * initial encoding stage of the BRC and it should not be changed during 
		 * the BRC cycle.
		 */
		if (!bBRCOn)   {
			if ((pPicInfoHdr->GlobalQuant<=6) || (pPicInfoHdr->GlobalQuant>=17)	) { 
				/* At high or low end, we don't do adaptive quant and so inheritance is 
				 * turned off at picture level 
				 */
				pPicInfoHdr->PicInheritQ = FALSE;
			}  else {
				pPicInfoHdr->PicInheritQ = TRUE;
			}
		}

		iCkSum = 0;
		for (iPlane = 0; iPlane < 3; iPlane++) {
			pBsPicInfo->Planes[iPlane].GlobalQuant = 
						(U8)(* pPicContext->pnGlobalQuant
						+ aiTypeDelta[pPicContext->FrameBSInfo.Hdr.PictureType]);

			/* Decrease the quantization for the color planes. */
			if(iPlane != COLOR_Y) {
				pBsPicInfo->Planes[iPlane].GlobalQuant = 
								CLIP(pBsPicInfo->Planes[iPlane].GlobalQuant, 0, u8MaxColorQuant);
			}

			pBsPicInfo->Planes[iPlane].GlobalQuant = 
							CLIP(pBsPicInfo->Planes[iPlane].GlobalQuant, 0, u8MaxQuant );

			EncPlane( pInst, &pBsPicInfo->Hdr, 
					  &pBsPicInfo->Planes[iPlane],
					  pPicContext->EncPlaneCntx[iPlane], 
					  pStrategyBuf->CSBPicture[iPlane], 
					  pStrategyBuf->CSBRebuiltPic[iPlane],
					  (U8)(pPicContext->BRCOn ? ENC_MODE_FINAL : ENC_MODE_ALL),
					  jbEnv );
		
			/*	Encode the decoder hints (initialized to FALSE and off) */
			switch (iPlane) {
				case COLOR_Y:
					pPicInfoHdr->YClamping = pBsPicInfo->Planes[iPlane].Clamped;
					break;
				case COLOR_U:
					pPicInfoHdr->UClamping = pBsPicInfo->Planes[iPlane].Clamped;
					break;
				case COLOR_V:
					pPicInfoHdr->VClamping = pBsPicInfo->Planes[iPlane].Clamped;
					break;
				}
			iCkSum += MatCksum(pStrategyBuf->CSBRebuiltPic[iPlane]);
		} /* end for each color plane */

		pPicInfoHdr->PicChecksum = (U16) iCkSum;

		/* Do Transparency Band Encoding */
		if (pPicContext->pEncTransCntx != NULL)
			EncTrans(pPicContext->pEncTransCntx,
					 pBsPicInfo->pTransBand,
					 pStrategyBuf->pTransBM,
					 (U8)(pPicContext->BRCOn ? ENC_MODE_FINAL : ENC_MODE_ALL),
					 jbEnv);
	} /* End of if (!PIC_TYPE_R) */

	/* Do decode rate control if this frame type has previous context info */
	iFrameType = PicDRCType(pBsPicInfo->Hdr.PictureType, jbEnv);
	bDoDRC = 
	  (pBsPicInfo->PicDRCCntx[iFrameType].MBHuffTbl.StaticTblNum != NoLastTbl);

	/* Use all of the information gathered in the info structures
   	   to create the bitstream. */
	EncodeBitstream(pBsPicInfo, 
					(U8)(pPicContext->BRCOn ? ENC_MODE_FINAL : ENC_MODE_ALL),
					jbEnv);

	/* Compact the bit buffers for the bitstream:
	 * calculate the total size of the bitstream and 
	 * gather the picture bit buffers into the StrategyBuffer's 
	 * bitstream buffer.  
	 */
#ifdef SIMULATOR  /* simulator option (/BANDSIZE): bit stream size break down, besides total picture size */
	EncSimCalcBitBuffSize(pBsPicInfo, pInst, 0,FALSE, &uSize, &uNonDbgSize, jbEnv); /* FALSE means not the ned of video clip */
#else /* non-simulator:	only calculate the total picture size, no break down needed */
	CalcBitBuffSize(pBsPicInfo, &uSize, &uNonDbgSize, jbEnv);
#endif	   /* SIMULATOR */

	if (pBsPicInfo->Hdr.PictureType != PIC_TYPE_R && pBsPicInfo->Hdr.DataSizeFlag) {
		AddPictureSize(uSize, pBsPicInfo->BsPicHdr, jbEnv);
	}
	CopyBitBuffs(newBsPtr->bufPtr, pBsPicInfo, jbEnv);
	
	newBsPtr->bufSize = uSize;

	pPicContext->GlobalByteBankFullness += 
					uNonDbgSize + *(pPicContext->pnFrameAudioOverhead);

	/* Do not allow GBBF to get too negative */
	if (pPicContext->GlobalByteBankFullness < -pPicContext->MaxBuffer) {
		pPicContext->GlobalByteBankFullness = -pPicContext->MaxBuffer;
	}

	/* Indicates the first frame which we get for free */
	if(pBsPicInfo->PicDRC.dDRC == dFrame1DRC)  {
		pBsPicInfo->PicDRC.dDRC = dInitDRC;
		pBsPicInfo->PicDRC.CycleBudget = pBsPicInfo->PicDRC.TargetBudget;
	} else if(bDoDRC) {    						/* Decode rate control */
		/* Add the difference between the target and actual cycles to the
		   cycle budget.  The CycleBudget value is only used for
		   diagnostics.
		 */
		pBsPicInfo->PicDRC.CycleBudget += pBsPicInfo->PicDRC.TargetBudget -
			pBsPicInfo->PicDRC.LastCycles;

		/* Clip the Cycle budget to reasonable range */
		if(pBsPicInfo->PicDRC.CycleBudget < iMinCycleBudget) {
			pBsPicInfo->PicDRC.CycleBudget = iMinCycleBudget;
		} else if(pBsPicInfo->PicDRC.CycleBudget > iMaxCycleBudget) {
			pBsPicInfo->PicDRC.CycleBudget = iMaxCycleBudget;
		}

		/* Error between target cycles and actual cycles */
		dError = pBsPicInfo->PicDRC.LastCycles - pBsPicInfo->PicDRC.TargetBudget;

		/* Modify the dDRC depending on the error */
		pBsPicInfo->PicDRC.dDRC = pBsPicInfo->PicDRC.dDRC +
			pBsPicInfo->PicDRC.DeltaTA * dError;

		/* Don't allow the dDRC value to go below dMinDRC */
		if(pBsPicInfo->PicDRC.dDRC < dMinDRC) {
			pBsPicInfo->PicDRC.dDRC = dMinDRC;
		}
	}

	pBsPicInfo->PicDRC.LastCycles = 0;
	return;

}    /*** End of CmpressPicture() ***/


/*************************************************************************************
 *
 *	OpenPictureCompressorContext
 *
 *	The caller has allocated and zero'd the picture context, and has initialized any
 *  pointers to its own information that will be useful in compressing pictures.
 *  Legacy runtime parameters are lumped together in the RtParms structure, and this
 *	routine initializes any of its information that depends on them.  It also will
 *	allocate and initialize any embedded contexts, once its own info is set up.
 *
 ************************************************************************************/

void OpenPictureCompressorContext( PTR_ENC_INST			pInst,
								   PEncRtParmsSt		pRtParms,
								   PEncPictureCntxSt	pPicContext,
								   jmp_buf 				jbEnv	)
{
	PPicBSInfoSt	pBsPicInfo;	/* Bitstream Picture header info */
	I32				iPlane;     /* Plane Counter */
	I32				iCnt;   	/* Temp Counter */
	EncConstInfoSt	Cinfo;
	U32 uPNumer, uDNumer, uDenom;  /* for BRC */
	U32 uKNumer, uP2Numer;
	U32 uBytesPerFrame;
	U32 uPPerGop=0, uDPerGop=0;	/* Frames of type X per GOP */
	U32 uP2PerGop = 0;
	pPicHdrSt		pPicInfoHdr;
	pGOPHdrSt		pGOPHdr;
	pBandSt			pBand0;  /* point to  band 0 of Y */

#ifdef SIMULATOR
	pPicContext->pSimInst = pRtParms->pSimInst;  /* Pointer to simulator Instance */
#endif /* SIMULATOR */

	/* initialize BRC parameters */
	pPicContext->BRCOn = pRtParms->BRCOn;
	pPicContext->MaxBuffer = pRtParms->MaxBuffer;

	pPicContext->KByteRate  = pRtParms->KByteRate; 
	pPicContext->KByteRatio = pRtParms->KByteRatio;
	pPicContext->PByteRatio = pRtParms->PByteRatio;
	pPicContext->P2ByteRatio = pRtParms->P2ByteRatio;
	pPicContext->BRCReactPos = pRtParms->BRCReactPos;
	pPicContext->BRCReactNeg = pRtParms->BRCReactNeg;

	/* Set up how many Ps, P2s, and Ds in each Gop */
	if ((pRtParms->PeriodP >= pRtParms->PeriodK) ||
	    (pRtParms->PeriodP == 0)) {
			uPPerGop = 0;
	} else {
		if (pRtParms->PeriodK % pRtParms->PeriodP)
			uPPerGop = (pRtParms->PeriodK/pRtParms->PeriodP);
		else
			uPPerGop = (pRtParms->PeriodK/pRtParms->PeriodP) - 1;
	}

	if ((pRtParms->PeriodP2 >= pRtParms->PeriodK) ||
	    (pRtParms->PeriodP2 == 0)) {
			uP2PerGop = 0;
	} else {
		if (pRtParms->PeriodK%pRtParms->PeriodP2)
			uP2PerGop = (pRtParms->PeriodK/pRtParms->PeriodP2) - uPPerGop;
		else
			uP2PerGop = (pRtParms->PeriodK/pRtParms->PeriodP2) - uPPerGop - 1;
	}

	uDPerGop = pRtParms->PeriodK - uPPerGop - uP2PerGop - 1;	

	pPicContext->P2PerGop = uP2PerGop;
	pPicContext->P2LeftPerGop = uP2PerGop;

	pPicContext->PPerGop = uPPerGop;
	pPicContext->DPerGop = uDPerGop;

	pPicContext->PLeftPerGop = uPPerGop;
	pPicContext->DLeftPerGop = uDPerGop;

	uKNumer = pRtParms->KByteRatio;
	uPNumer = pRtParms->PByteRatio * uPPerGop;
	uDNumer = pRtParms->DByteRatio * uDPerGop;   
 	uP2Numer = pRtParms->P2ByteRatio * uP2PerGop;   
	uDenom = uKNumer + uPNumer + uDNumer + uP2Numer;  

	pPicContext->GlobalByteBankFullness = 0;
    uBytesPerFrame = (U32)(((double)pRtParms->KByteRate * 1024*pRtParms->FrameTime)/1000000);
	pPicContext->BytesPerFrame = uBytesPerFrame;
	pPicContext->BytesPerK = (U32)(((float)pRtParms->PeriodK * 
									(float)uBytesPerFrame) / 
								    (float)uDenom * 
								    (float)uKNumer);
	if (uPPerGop > 0) {
		pPicContext->BytesPerP = (U32) (((float)pRtParms->PeriodK * 
										 (float)uBytesPerFrame) / 
										((float)uDenom*(float)uPPerGop) * 
										 (float)uPNumer);
	} else  /* no Ps in this seq */
		pPicContext->BytesPerP = 0;

	if (uP2PerGop > 0) {
		pPicContext->BytesPerP2 = (U32) (((float)pRtParms->PeriodK * 
										 (float)uBytesPerFrame) / 
										((float)uDenom*(float)uP2PerGop) * 
										 (float)uP2Numer);
	} else  /* no P2s in this seq */
		pPicContext->BytesPerP2 = 0;

	if (uDPerGop > 0) {
		pPicContext->BytesPerD = (U32) (((float)pRtParms->PeriodK * 
										 (float)uBytesPerFrame) / 
										((float)uDenom*(float)uDPerGop) * 
										 (float)uDNumer);
	} else  /* no Ds in this seq */
		pPicContext->BytesPerD = 0;

	pBsPicInfo = &pPicContext->FrameBSInfo;

	pPicInfoHdr = &pBsPicInfo->Hdr;
	
	pPicInfoHdr->PicStartCode = START_CODE;
	pGOPHdr= &(pPicInfoHdr->GOPHdr);
	
	/* set the GOPHdr info */
	pGOPHdr->GOPHdrSizeFlag = TRUE;  /* Always encode the Header Size */
#ifdef SIMULATOR
	if (pRtParms->bYVU12 ) {
		pGOPHdr->IsYVU12 = TRUE;
	} else { 
		pGOPHdr->IsYVU12 = FALSE;
	}
	if (pRtParms->bExpBS ) {
		pGOPHdr->IsExpBS = TRUE;
	} else { 
		pGOPHdr->IsExpBS = FALSE;
	}
#else 
		pGOPHdr->IsYVU12 = FALSE;  /* YVU9 is default */
		pGOPHdr->IsExpBS = FALSE;  /* No Experimental BS for non-simulator build  */
#endif
	if (pGOPHdr->IsYVU12) {
		pGOPHdr->VUSubVertical = pGOPHdr->VUSubHorizontal = 2;
	} else {
		pGOPHdr->VUSubVertical = pGOPHdr->VUSubHorizontal = 4;
	}

	pGOPHdr->Transparency = pRtParms->Transparency;
	pGOPHdr->SkipBitBand = FALSE;	/* always FALSE because SkipBit Band is not currently supported */

	if (pInst->ecscSequenceControlsUsed & EC_ACCESS_KEY) {
		pGOPHdr->UseKeyFrameLock = TRUE;
		pGOPHdr->AccessKey = pInst->pEncParms->uAccessKey; 
	}
	else {
		pGOPHdr->UseKeyFrameLock = FALSE;
	}		

	pGOPHdr->PictureWidth  = (I16) pPicContext->pPicDimensions[COLOR_Y].NumCols;
	pGOPHdr->PictureHeight = (I16) pPicContext->pPicDimensions[COLOR_Y].NumRows;
	SetTileSizes(pRtParms, pGOPHdr);

	/* Tiling */
	if ( (pGOPHdr->PictureWidth==pGOPHdr->TileWidth) &&
		 (pGOPHdr->PictureHeight==pGOPHdr->TileHeight) ) {
		pGOPHdr->Tiling = FALSE;
	}  else {
		pGOPHdr->Tiling = TRUE;
	}

	/* Padding: TODO :LLYANG 
	 * always padded for now. Maybe FALSE if low datarate 
	 */
	pGOPHdr->Padding = TRUE; 

	/* Y and VU Decomp Levels */
	pGOPHdr->YDecompLevel = (U8) pRtParms->YLevels;
	pGOPHdr->VUDecompLevel = (U8) pRtParms->VULevels;

	/* Version */
	pGOPHdr->Version.PlatformId = (U8)pInst->peiEnvironment->uPlatformId; 
	pGOPHdr->Version.BuildNum = (U8)pInst->peiEnvironment->uHiveBuildNumber;
	pGOPHdr->Version.EncoderId = OFFLINE_ENCODER;
	pGOPHdr->Version.BSVersion = BS_VERSION;
	pGOPHdr->Version.ChainBit = FALSE; /* no tool yet */
	pGOPHdr->Version.pToolStamp = NULL;

	/* set the persistent picture Header info */
	pPicInfoHdr->DataSizeFlag = TRUE;  /* always true for now unless being overwritten later*/
	pPicInfoHdr->BandDataSizeFlag = TRUE;  /* always true for now unless being overwritten later*/
	pPicInfoHdr->SideBitStream = FALSE; /* always FALSE unless being overwritten */
	pPicInfoHdr->PicInheritTypeMV = pRtParms->McUseBand0;
	pPicInfoHdr->UseChecksum = TRUE;  /* default ON so that Acces Key may be used if user wants to */
	pPicInfoHdr->UsePicExt = FALSE;
	pBsPicInfo->HuffInfo.FHTMB = pRtParms->HTMb;

	pBsPicInfo->PicDRC.bShowDRC = pRtParms->bShowDRC;
	pBsPicInfo->PicDRC.MaxBudget = pRtParms->MaxBudget;
	pBsPicInfo->PicDRC.TargetBudget = pRtParms->TargetBudget;
	pBsPicInfo->PicDRC.dDRC = dFrame1DRC;
	pBsPicInfo->PicDRC.DeltaTA = pRtParms->DeltaTA;
	pBsPicInfo->PicDRC.CycleBudget = pRtParms->TargetBudget;
	
	for (iCnt = 0; iCnt < NUM_PIC_DRC_TYPES; iCnt++) {
		pBsPicInfo->PicDRCCntx[iCnt].MBHuffTbl.StaticTblNum = NoLastTbl;
	}

	pBsPicInfo->NumPlanes = 3;

	/* allocate the bi buffers for StartCode, GOPHdr and PicHdr */
	pBsPicInfo->BsPicHdr = (pBitBuffSt) SysMalloc(sizeof(BitBuffSt),jbEnv);	
	BitBuffAlloc(pBsPicInfo->BsPicHdr, sizeof(PicHdrSt), jbEnv); 

	pBsPicInfo->BsGOPHdr = (pBitBuffSt) SysMalloc(sizeof(BitBuffSt),jbEnv);	
	BitBuffAlloc(pBsPicInfo->BsGOPHdr, sizeof(GOPHdrSt), jbEnv); 

	pBsPicInfo->BsStartCode = (pBitBuffSt) SysMalloc(sizeof(BitBuffSt),jbEnv);
	BitBuffAlloc(pBsPicInfo->BsStartCode, 2, jbEnv); 	/* 2 bytes for StartCode */

	/* Initialize Cinfo to all 0's */
	memset(&Cinfo, 0, sizeof(Cinfo)); 

	/* Pass down the picture size for tile size calculation	*/
	Cinfo.iNumCols = pPicInfoHdr->GOPHdr.PictureWidth;
	Cinfo.iNumRows = pPicInfoHdr->GOPHdr.PictureHeight;
	pBsPicInfo->Planes = (pPlaneSt) SysMalloc(sizeof(PlaneSt)*pBsPicInfo->NumPlanes, jbEnv);

	for (iPlane = 0; iPlane < pBsPicInfo->NumPlanes; iPlane++) { /* init for 3 planes */
		Cinfo.Pic = pPicContext->pPicDimensions[iPlane];
		Cinfo.Color = iPlane;
		pPicContext->EncPlaneCntx[iPlane] = EncPlaneOpen(&pBsPicInfo->Planes[iPlane], 
														 pPicInfoHdr,
														 &Cinfo, pRtParms, &pBand0, jbEnv);
		if (iPlane==0) { /* Aftre Y plane: pBand0 is already available */
			pBand0 = (pBandSt) pPicContext->EncPlaneCntx[0]->EncBandCntx[0]->Band0Info;
		}
   	}
	
	pPicInfoHdr->YSubDiv  = pBsPicInfo->Planes[COLOR_Y].SubDiv;
	pPicInfoHdr->VUSubDiv = pBsPicInfo->Planes[COLOR_V].SubDiv;


	/* Initialize transparency context if used */
	if (pRtParms->Transparency)	{
		pPicContext->pEncTransCntx = EncTransOpen(&pBsPicInfo->pTransBand,
												  pPicInfoHdr,
												  pRtParms, jbEnv);
	} else {
		pPicContext->pEncTransCntx = NULL;
	}
	     /* From PB style static tables to Huffman static tables */
	InitStaticEncodeTables(pBsPicInfo->HuffInfo.MBHuffTbls, 
						   pBsPicInfo->HuffInfo.BlkHuffTbls,
						   &pBsPicInfo->HuffInfo.DefMBHuffTbl,
						   &pBsPicInfo->HuffInfo.DefBlkHuffTbl,
					       jbEnv);
		/* Get the static and default RunVal mapping table array */
	InitStaticRVMapTables(&(pBsPicInfo->HuffInfo.RVMapTbls[0]),
						  &pBsPicInfo->HuffInfo.DefRVMapTbl,
						  jbEnv);

	return;

}    /*** End of OpenPictureCompressorContext() ***/



/* ------------------------  CLOSE  ------------------------------*/
/* End the sequence; free any allocated storage
*/
void ClosePictureCompressorContext(PEncPictureCntxSt pPicContext, 
								   jmp_buf           jbEnv)
{
	I32				iPlane;
	PPicBSInfoSt	pBsPicInfo = &pPicContext->FrameBSInfo;
	
	for (iPlane = 0; iPlane < 3; iPlane++) {
		EncPlaneClose(  pPicContext->EncPlaneCntx[iPlane], 
						&pBsPicInfo->Planes[iPlane], jbEnv);
	}
	
	if (pPicContext->pEncTransCntx != NULL)
		EncTransClose(	pPicContext->pEncTransCntx, 
						pBsPicInfo->pTransBand, jbEnv);
	BitBuffFree(pBsPicInfo->BsPicHdr, jbEnv);

	SysFree((PU8)pBsPicInfo->Planes,jbEnv);  
	SysFree((PU8)pBsPicInfo->BsPicHdr, jbEnv);  

}


/*========================================================*/
/* Determine the lowest quantization level that can be used and still stay 
 * within the bitrate.  This includes calls to EncPlane and EncodeBitstream.
 * When the ideal quantization is determined, then call CmpressPicture to
 * "do it for real."
 */
 
void ControlBitRate( PCmpStrategyStorageSt	pStrategyBuf,
					 PTR_ENC_INST 			pInst,
					 jmp_buf				jbEnv )
{
	PEncPictureCntxSt pPicContext = pInst->pEncoderPrivate->pPicContext;
	PPicBSInfoSt	  pBsPicInfo  = &pPicContext->FrameBSInfo;
	I32 iPlane;
	I32 iBytesForThisFrame;
	I32 iBufSize;
	I32 iDelta;
	I32 iReactPos = pPicContext->BRCReactPos;
	I32 iReactNeg = pPicContext->BRCReactNeg;
	I32 iByteDelta;
	const PIA_Boolean bIntBSFormat = FALSE;  /* Intermediate bitstream format */
	U8  u8MaxQuant = 23;
	U8  u8MaxColorQuant = 23;
	U32 uSize, uNonDbgSize;

	pPicContext->FrameBSInfo.Hdr.PictureType = (U8) pStrategyBuf->nFrameType;
	pBsPicInfo->Hdr.PictureNum  = ((pInst->pEncoderPrivate->nKPeriodCntr - 1) % 255) + 1;

	/* BRC stuff */
	switch( pStrategyBuf->nFrameType ) { 			
		case PIC_TYPE_K:
		{
			I32 iKNumer, iPNumer, iDNumer, iDenom;
			I32 iP2Numer;


			/* first reset the PLeftPerGop and BLeftPerGop counters
			 * since this is the start of a GOP
			 */
			pPicContext->PLeftPerGop = pPicContext->PPerGop;
			pPicContext->DLeftPerGop = pPicContext->DPerGop;
			pPicContext->P2LeftPerGop = pPicContext->P2PerGop;
			/* now redistribute the GlobalByteBank for this GOP */
			iKNumer = pPicContext->KByteRatio;
			iPNumer = pPicContext->PByteRatio * pPicContext->PLeftPerGop;
			iDNumer = pPicContext->DByteRatio * pPicContext->DLeftPerGop;   
			iP2Numer = pPicContext->P2ByteRatio * pPicContext->P2LeftPerGop;
			iDenom = iKNumer + iPNumer + iDNumer + iP2Numer;  			

			/* now do the actual byte allocations for this frame */
			iByteDelta = pPicContext->MaxBuffer/2 - 
						 pPicContext->GlobalByteBankFullness;
			if( iByteDelta > 0 ) {	/* lower than half the buffer */
				iBytesForThisFrame = pPicContext->BytesPerK + 
									 (iByteDelta*iReactPos) / 256;
			} else { /* exceeded half the buffer */
				iBytesForThisFrame = pPicContext->BytesPerK +
									 (iByteDelta*iReactNeg) / 256;
			} /* endif exceeded half the buffer */

			/* partial update of globalbytebankfullness */
			pPicContext->GlobalByteBankFullness -= pPicContext->BytesPerK;

			} /* end case Key frame */
			break;

		case PIC_TYPE_P:
			iByteDelta = pPicContext->MaxBuffer/2 - 
						 pPicContext->GlobalByteBankFullness;
			if( iByteDelta > 0 )	{	/* lower than half the buffer */
				iBytesForThisFrame = 
							pPicContext->BytesPerP+(iByteDelta*iReactPos)/256;
			} else {	/* exceeded half the buffer */
				iBytesForThisFrame = 
							pPicContext->BytesPerP+(iByteDelta*iReactNeg)/256;
				if (iByteDelta < -(pPicContext->MaxBuffer/2) ) { /* overflow */
					pStrategyBuf->nFrameType = PIC_TYPE_R;
					pPicContext->FrameBSInfo.Hdr.PictureType = (U8)PIC_TYPE_R;
					iBytesForThisFrame = 0;
				}
			} /* endif iByteDelta > 0 */ 

			pPicContext->GlobalByteBankFullness -= pPicContext->BytesPerP;
			pPicContext->PLeftPerGop -= 1;

			break;  /* End case P Frame */

		case PIC_TYPE_P2:
			iByteDelta = pPicContext->MaxBuffer/2 - 
						 pPicContext->GlobalByteBankFullness;
			if( iByteDelta > 0 )	{	/* lower than half the buffer */
				iBytesForThisFrame = 
							pPicContext->BytesPerP2+(iByteDelta*iReactPos)/256;
			} else {	/* exceeded half the buffer */
				iBytesForThisFrame = 
							pPicContext->BytesPerP2+(iByteDelta*iReactNeg)/256;
				if (iByteDelta < -(pPicContext->MaxBuffer/2) ) { /* overflow */
					pStrategyBuf->nFrameType = PIC_TYPE_R;
					pPicContext->FrameBSInfo.Hdr.PictureType = (U8)PIC_TYPE_R;
  					iBytesForThisFrame = 0;
				}
			} /* endif iByteDelta > 0 */ 

			pPicContext->GlobalByteBankFullness -= pPicContext->BytesPerP2;
			pPicContext->P2LeftPerGop -= 1;

			break;  /* End case P2 Frame */

		case PIC_TYPE_D:
			iByteDelta = pPicContext->MaxBuffer/2 - 
						 pPicContext->GlobalByteBankFullness;
			if( iByteDelta > 0 ) {	/* lower than half the buffer */
				iBytesForThisFrame = 
						pPicContext->BytesPerD + (iByteDelta*iReactPos)/256;
			} else {	/* exceeded half the buffer */
				iBytesForThisFrame = 
							pPicContext->BytesPerD+(iByteDelta*iReactNeg)/256;
				if( iByteDelta < -(pPicContext->MaxBuffer/2) ) { /* overflow */
					pStrategyBuf->nFrameType = PIC_TYPE_R;
					pPicContext->FrameBSInfo.Hdr.PictureType = (U8)PIC_TYPE_R;
					iBytesForThisFrame = 0;
				}
			} 

			pPicContext->GlobalByteBankFullness -= pPicContext->BytesPerD;
			pPicContext->DLeftPerGop -= 1;

			break; /* end case D frame */
		default:  /* invalid frame type */
			longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
							(__LINE__ << LINE_OFFSET) |
							(ERR_ERROR << TYPE_OFFSET));
			break;
	}	/* end switch frame type */
		
	/* end BRC stuff */


	/* If the BRC code above decides to drop the frame, then don't bother with */
	/* any of the rest of this analysis, because it's irrelavent. */
	/* Try different quant levels to find best fit */
	if (pPicContext->FrameBSInfo.Hdr.PictureType != (U8)PIC_TYPE_R) {

#ifdef SIMULATOR
		pBsPicInfo->pSimInst = pInst->pEncoderPrivate->pEncSimInst;
		u8MaxQuant = (U8) EncSimMaxQuant(pPicContext->pSimInst);
		u8MaxColorQuant = (U8) EncSimMaxColorQuant(pPicContext->pSimInst);
		EncSimFrameQuantBias(pPicContext->pSimInst, aiTypeDelta);
#endif /* SIMULATOR */
	
		/* do an initial encoding at current global quant level 
		 * to establish a baseline 
		 */
		pBsPicInfo->Hdr.GlobalQuant = (U8)(* pPicContext->pnGlobalQuant);
		/* determine inheritance at picture level based upon initial GlobalQuant, 
		 * And it does not get changed during the BRC cycle 
		 */
		if ((pBsPicInfo->Hdr.GlobalQuant<=6) || (pBsPicInfo->Hdr.GlobalQuant>=17)	) { 
			/* At high or low end, we don't do adaptive quant and so inheritance is 
			 * turned off at picture level 
			 */
			pBsPicInfo->Hdr.PicInheritQ = FALSE;
		}  else {
			pBsPicInfo->Hdr.PicInheritQ = TRUE;
		}

		for (iPlane = 0; iPlane < 3; iPlane++) {
			pBsPicInfo->Planes[iPlane].GlobalQuant = 
					(U8) (*pPicContext->pnGlobalQuant 
						+ aiTypeDelta[pPicContext->FrameBSInfo.Hdr.PictureType]);

			/* Decrease the quantization for the color planes. */
			if(iPlane != COLOR_Y) {
				pBsPicInfo->Planes[iPlane].GlobalQuant = 
							CLIP(pBsPicInfo->Planes[iPlane].GlobalQuant, 0, u8MaxColorQuant);
			}

			pBsPicInfo->Planes[iPlane].GlobalQuant = 
								CLIP(pBsPicInfo->Planes[iPlane].GlobalQuant, 0, u8MaxQuant );
		
			EncPlane( pInst,
					  &pBsPicInfo->Hdr, 
					  &pBsPicInfo->Planes[iPlane],
					  pPicContext->EncPlaneCntx[iPlane], 
					  pStrategyBuf->CSBPicture[iPlane], 
					  pStrategyBuf->CSBRebuiltPic[iPlane],
					  ENC_MODE_TEST_WATER, jbEnv );
		}

		/* Do Transparency Band Encoding */
		if (pPicContext->pEncTransCntx != NULL)
			EncTrans(pPicContext->pEncTransCntx,
					 pBsPicInfo->pTransBand,
					 pStrategyBuf->pTransBM,
					 ENC_MODE_TEST_WATER,
					 jbEnv);

	#ifdef SIMULATOR /* simulation infrastructure */
		pBsPicInfo->pSimInst = pPicContext->pSimInst;
	#endif /* SIMULATOR, simulation infrastructure */

		EncodeBitstream(pBsPicInfo, ENC_MODE_TEST_WATER, jbEnv);

		CalcBitBuffSize(pBsPicInfo,&uSize, &uNonDbgSize, jbEnv);
		iBufSize =  uNonDbgSize + (*(pPicContext->pnFrameAudioOverhead));
		/* done initial encoding for baseline */

		if (iBufSize < iBytesForThisFrame) { 
			/* this quant level uses less bytes than allocated so reduce quant */
			iDelta = 0;
			while ( (iBufSize < iBytesForThisFrame) && 
					(*(pPicContext->pnGlobalQuant)>0) && 
					(iDelta<1) ) {
				HiveEncodeProgressFunc(pInst, PCT_STATUS, 10);

				*(pPicContext->pnGlobalQuant) -= 1;
				iDelta += 1;				
				for (iPlane = 0; iPlane < 3; iPlane++) {
					pBsPicInfo->Planes[iPlane].GlobalQuant = 
						(U8) (*pPicContext->pnGlobalQuant 
						+ aiTypeDelta[pPicContext->FrameBSInfo.Hdr.PictureType]);

					/* Decrease the quantization for the color planes. */
					if(iPlane != COLOR_Y) {
			            pBsPicInfo->Planes[iPlane].GlobalQuant = 
 							CLIP(pBsPicInfo->Planes[iPlane].GlobalQuant, 0, u8MaxColorQuant);

					}

					pBsPicInfo->Planes[iPlane].GlobalQuant = 
						CLIP(pBsPicInfo->Planes[iPlane].GlobalQuant, 0, u8MaxQuant ); 

					EncPlane( pInst, &pBsPicInfo->Hdr, 
							  &pBsPicInfo->Planes[iPlane],
							  pPicContext->EncPlaneCntx[iPlane], 
							  pStrategyBuf->CSBPicture[iPlane], 
							  pStrategyBuf->CSBRebuiltPic[iPlane],
							  ENC_MODE_TRIAL, jbEnv );
				}

				/* Do Transparency Band Encoding */
				if (pPicContext->pEncTransCntx != NULL)
					EncTrans(pPicContext->pEncTransCntx,
							 pBsPicInfo->pTransBand,
							 pStrategyBuf->pTransBM,
							 ENC_MODE_TRIAL,
							 jbEnv);

				EncodeBitstream(pBsPicInfo, ENC_MODE_TRIAL, jbEnv);	
				CalcBitBuffSize(pBsPicInfo,&uSize, &uNonDbgSize, jbEnv);
				iBufSize =  uNonDbgSize + (*(pPicContext->pnFrameAudioOverhead));
			} /* end descending trial loop */
		} else {	
			/* this quant level uses more bytes than allocated so increase quant */
			iDelta = 0;
			while((iBufSize > iBytesForThisFrame) && 
	  			  (*(pPicContext->pnGlobalQuant)<23) && 
				  (iDelta<2)) {
				HiveEncodeProgressFunc(pInst, PCT_STATUS, 10);
	
				*(pPicContext->pnGlobalQuant) += 1;
				iDelta += 1;				
				for (iPlane = 0; iPlane < 3; iPlane++) {
					pBsPicInfo->Planes[iPlane].GlobalQuant = 
						(U8) (*pPicContext->pnGlobalQuant 
						+ aiTypeDelta[pPicContext->FrameBSInfo.Hdr.PictureType]);

					/* Decrease the quantization for the color planes. */
					if(iPlane != COLOR_Y) {
			            pBsPicInfo->Planes[iPlane].GlobalQuant = 
 							CLIP(pBsPicInfo->Planes[iPlane].GlobalQuant, 0, u8MaxColorQuant);
					}
					
					pBsPicInfo->Planes[iPlane].GlobalQuant = 
						CLIP( pBsPicInfo->Planes[iPlane].GlobalQuant, 0, u8MaxQuant ); 

					EncPlane( pInst, &pBsPicInfo->Hdr, 
							  &pBsPicInfo->Planes[iPlane],
							  pPicContext->EncPlaneCntx[iPlane], 
							  pStrategyBuf->CSBPicture[iPlane], 
							  pStrategyBuf->CSBRebuiltPic[iPlane],
							  ENC_MODE_TRIAL, jbEnv );
				}

				/* Do Transparency Band Encoding */
				if (pPicContext->pEncTransCntx != NULL)
					EncTrans(pPicContext->pEncTransCntx,
							 pBsPicInfo->pTransBand,
							 pStrategyBuf->pTransBM,
							 ENC_MODE_TRIAL,
							 jbEnv);
	
				EncodeBitstream(pBsPicInfo, ENC_MODE_TRIAL, jbEnv);	
				CalcBitBuffSize(pBsPicInfo,&uSize, &uNonDbgSize, jbEnv);
				iBufSize = uNonDbgSize + (*(pPicContext->pnFrameAudioOverhead));
			} /* end ascending trial loop */
		} /* endif bytes used > target bytes */
		/* done trying different global quants to get best global quant */

	} /*** end of if (PICTYPE != PICTYPE_R) ***/

}    /*** End of ControlBitRate() ***/


/****************************************************************
 *	Sets the tile sizes given the parameters in RtParms.
 ****************************************************************/
static void SetTileSizes(PEncRtParmsSt RtParms, pGOPHdrSt pGOPHdr)
{
	I32	iUserTileWidth  = RtParms->iaTileSize[0];
	I32	iUserTileHeight = RtParms->iaTileSize[1];

	
	/* If the width or height is 0, that means use the corresponding 
	 * picture dimension for that tile.  If non-zero, make sure it isn't
	 * bigger than the picture size.
	 */
	if (iUserTileWidth == 0)
		pGOPHdr->TileWidth = pGOPHdr->PictureWidth;
	else 
		pGOPHdr->TileWidth = MIN(iUserTileWidth, pGOPHdr->PictureWidth); 

	if (iUserTileHeight == 0) {
		pGOPHdr->TileHeight = pGOPHdr->PictureHeight;
	} else {
		pGOPHdr->TileHeight = MIN(iUserTileHeight, pGOPHdr->PictureHeight); 
	}

}    /*** End of SetTileSizes() ***/




/* ---------------- Calculate BitBuffer Size ------------------*/
/*
 * Traverse the Picture structure and accumulate the sizes of
 * all of the bit buffers
 *		puTotalSize: return the total size of the bitstream
 *      puNonDbgSize: return the actual bitstream size when the Block Checksums are excluded.
 *   Note: This routine needs to guarantee that the puNonDbgSize of the Exp_BS version
 *         is the same as the puTotalSize of the non-Exp_BS version for an exact 
 *         compression setting, so that the including block checksums in BS for debug purpose
 *         won't in any way alter the BRC.
 */
static void CalcBitBuffSize(PPicBSInfoSt pBsPicInfo, 
							PU32 puTotalSize, 
							PU32 puNonDbgSize, 
							jmp_buf jbEnv)
{
	U32 uLen = 0, uDbgLen = 0, uBlkChksumLen, uNonDbgLen;					
	I32 iPlaneNum, iBandNum, iTileNum;	/* for loop indices */ 
	pPlaneSt pPlane;					/* short hand notation */
	pBandSt	 pBand; 					/* short hand notation */
	pTileSt	 pTile;						/* short hand notation */
	U32 uPadLength;
	
	uLen = BitBuffSize(pBsPicInfo->BsStartCode);
	if (pBsPicInfo->Hdr.PictureType!=PIC_TYPE_R) {
		uLen += BitBuffSize(pBsPicInfo->BsGOPHdr);
		uLen += BitBuffSize(pBsPicInfo->BsPicHdr);	 
		for (iPlaneNum = 0; iPlaneNum < pBsPicInfo->NumPlanes; iPlaneNum++) {
			pPlane = &pBsPicInfo->Planes[iPlaneNum];
			for (iBandNum = 0; iBandNum < pPlane->NumBands; iBandNum++) {
				pBand = &pPlane->Bands[iBandNum];
				/* Count band if the command line parms say write to bitstream */
				if (pBand->DropBand) {
					continue;	/* skip this band */
				}
				uLen += BitBuffSize(&(pBand->BsBandHdr));

				for (iTileNum = 0; iTileNum < pBand->NumTiles; iTileNum++) {
					pTile = &pBand->Tiles[iTileNum];
					uLen += BitBuffSize(&(pTile->BsTileHdr)); /* included BlkChecksum */
					if (pBsPicInfo->Hdr.GOPHdr.IsExpBS ) { /* the Blk Checksum info */
						uBlkChksumLen = (sizeof(U16)+pTile->TileHdr.NumBlks*sizeof(U16));
						if (pTile->TileHdr.IsTileDataSize) {
							if ( (pTile->TileHdr.TileDataSize > Max_Small_TD_Size) 
								&& ((pTile->TileHdr.TileDataSize-uBlkChksumLen-Large_TD_Width/8) < Max_Small_TD_Size) ) {
								/* The TileDataSize field uses 3 more bytes due to the BlkChecksums */
								uBlkChksumLen +=  Large_TD_Width/8;
							}
						}
						uDbgLen += uBlkChksumLen;
					}
					uLen += BitBuffSize(&(pTile->BsMacHdr));
					uLen += BitBuffSize(&(pTile->BsBlockDat));
				}
			}
		}
		/* Now do transparency band if it is present */
		if (pBsPicInfo->Hdr.GOPHdr.Transparency) {
			I32 iTile;
			PTransSt pTransBand = pBsPicInfo->pTransBand;
			PTransTileSt pTransTile;
			
			if (pTransBand->TransHdr.UseXOR_RLE) {
				pTransTile = pTransBand->XORTiles;
			} else {
				pTransTile = pTransBand->Tiles;
			}

			uLen += BitBuffSize(&(pTransBand->BsTransHdr));

			for (iTile = 0; iTile < pTransBand->NumTiles; iTile++, pTransTile++) {
				uLen += BitBuffSize(&(pTransTile->BsTransTileHdr));
				uLen += BitBuffSize(&(pTransTile->BsTransTileDat));
			}
		}
	}

	/* Pad to the next DWord and then pad an extra DWord.  This is needed
	   for the current implementation of the decoder. If the current
	   DWord is exactly full then 2 DWords are padded at the end.
	 */

	uNonDbgLen = uLen - uDbgLen;

	if (pBsPicInfo->Hdr.GOPHdr.Padding && (pBsPicInfo->Hdr.PictureType!=PIC_TYPE_R)) {
		/* figure out the padding amount */
		uPadLength = DWordSize - (uLen % DWordSize);
		if(uPadLength <= DWordSize) {
			uPadLength += DWordSize;
		}
		uLen += uPadLength;

		/* figure out the padding for the corresponding non-Exp_BS version */
		uPadLength = DWordSize - (uNonDbgLen % DWordSize);
		if(uPadLength <= DWordSize) {
			uPadLength += DWordSize;
		}
		uNonDbgLen += uPadLength;
	}

	if (!pBsPicInfo->Hdr.GOPHdr.IsExpBS ) {
		if (uLen!=uNonDbgLen) {
			longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
							(__LINE__ << LINE_OFFSET) |
							(ERR_ERROR << TYPE_OFFSET));
		}
	}
	*puTotalSize = uLen;
	*puNonDbgSize =uNonDbgLen;
	return;
}    /*** End of CalcBitBuffSize() ***/


/* --------------- Copy BitBuffers To One Buffer -----------------*/
/*
 * Traverse the Picture structure and copy the bit buffers to the
 * memory buffer so that they are all in one place
 */
static void CopyBitBuffs(PU8 pu8Dest, PPicBSInfoSt pBsPicInfo, jmp_buf jbEnv)
{
	U32 uCalcLen = 0;					/* sanity check value */
	U32 uCopyLen = 0;					/* sanity check value */
	I32 iPlaneNum, iBandNum, iTileNum;	/* for loop indices */ 
	pPlaneSt pPlane;					/* short hand notation */
	pBandSt	 pBand; 					/* short hand notation */
	pTileSt	 pTile;						/* short hand notation */
	PU8	pInitPtr = pu8Dest;			 	/* Initial pointer */
	U32 uPadLength;						/* D Word padding length */
	PU16 pu16Scramble = NULL;			/* Pointer to Band 0 Header Data to scramble */

	uCalcLen = BitBuffSize(pBsPicInfo->BsStartCode);	
	uCopyLen = BitBuff2Mem(pu8Dest, pBsPicInfo->BsStartCode, jbEnv);	 
	if (uCalcLen != uCopyLen)
		longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_BAD_PARM << TYPE_OFFSET));
	pu8Dest += uCopyLen;

	uCalcLen = BitBuffSize(pBsPicInfo->BsGOPHdr);	
	uCopyLen = BitBuff2Mem(pu8Dest, pBsPicInfo->BsGOPHdr, jbEnv);	 
	if (uCalcLen != uCopyLen)
		longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_BAD_PARM << TYPE_OFFSET));
	pu8Dest += uCopyLen;

	uCalcLen = BitBuffSize(pBsPicInfo->BsPicHdr);	
	uCopyLen = BitBuff2Mem(pu8Dest, pBsPicInfo->BsPicHdr, jbEnv);	 
	if (uCalcLen != uCopyLen)
		longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_BAD_PARM << TYPE_OFFSET));
	pu8Dest += uCopyLen;

	for (iPlaneNum = 0; iPlaneNum < pBsPicInfo->NumPlanes; iPlaneNum++) {
		pPlane = &pBsPicInfo->Planes[iPlaneNum];
		for (iBandNum = 0; iBandNum < pPlane->NumBands; iBandNum++) {
			pBand = &pPlane->Bands[iBandNum];
			/* Count band if the command line parms say write to bitstream */
			if (pBand->DropBand == TRUE) {
				continue;	/* skip this band */
			}
			uCalcLen = BitBuffSize(&(pBand->BsBandHdr));
			uCopyLen = BitBuff2Mem(pu8Dest, &(pBand->BsBandHdr), jbEnv);
			if (uCalcLen != uCopyLen)
				longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
								(__LINE__ << LINE_OFFSET) |
								(ERR_BAD_PARM << TYPE_OFFSET));

			/* Get pointer for KeyFrameLock Band 0 Scrambling */
			if ((iBandNum == 0) &&
			    (iPlaneNum == COLOR_Y) &&
				(pBsPicInfo->Hdr.PictureType == PIC_TYPE_K) &&
				(pBsPicInfo->Hdr.GOPHdr.UseKeyFrameLock)) {

				pu16Scramble = (PU16) (pu8Dest + 4);  /* Skip Past Flags and BandDataSize */
			}

			pu8Dest += uCopyLen;

			for (iTileNum = 0; iTileNum < pBand->NumTiles; iTileNum++) {
				pTile = &pBand->Tiles[iTileNum];
				uCalcLen = BitBuffSize(&(pTile->BsTileHdr));
				uCopyLen = BitBuff2Mem(pu8Dest, &(pTile->BsTileHdr), jbEnv);
				if (uCalcLen != uCopyLen)
					longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
									(__LINE__ << LINE_OFFSET) |
									(ERR_BAD_PARM << TYPE_OFFSET));
				pu8Dest += uCopyLen;

				uCalcLen = BitBuffSize(&(pTile->BsMacHdr));
				uCopyLen = BitBuff2Mem(pu8Dest, &(pTile->BsMacHdr), jbEnv);
				if (uCalcLen != uCopyLen)	
					longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
									(__LINE__ << LINE_OFFSET) |
									(ERR_BAD_PARM << TYPE_OFFSET));
				pu8Dest += uCopyLen;

				uCalcLen = BitBuffSize(&(pTile->BsBlockDat));
				uCopyLen = BitBuff2Mem(pu8Dest, &(pTile->BsBlockDat), jbEnv);
				if (uCalcLen != uCopyLen)
					longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
									(__LINE__ << LINE_OFFSET) |
									(ERR_BAD_PARM << TYPE_OFFSET));
				pu8Dest += uCopyLen;
			}
		}
	}
	/* Now do transparency band if it is present */
	if (pBsPicInfo->Hdr.GOPHdr.Transparency) {
		I32 iTile;
		PTransSt pTransBand = pBsPicInfo->pTransBand;
		PTransTileSt pTransTile;

		if (pTransBand->TransHdr.UseXOR_RLE) {
			pTransTile = pTransBand->XORTiles;
		} else {
			pTransTile = pTransBand->Tiles;
		}

		uCalcLen = BitBuffSize(&(pTransBand->BsTransHdr));
		uCopyLen = BitBuff2Mem(pu8Dest, &(pTransBand->BsTransHdr), jbEnv);
		if (uCalcLen != uCopyLen)
			longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
							(__LINE__ << LINE_OFFSET) |
							(ERR_BAD_PARM << TYPE_OFFSET));

		pu8Dest += uCopyLen;
		
		for (iTile = 0; iTile < pTransBand->NumTiles; iTile++, pTransTile++) {
			uCalcLen = BitBuffSize(&(pTransTile->BsTransTileHdr));
			uCopyLen = BitBuff2Mem(pu8Dest, &(pTransTile->BsTransTileHdr), jbEnv);
			if (uCalcLen != uCopyLen)
				longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
								(__LINE__ << LINE_OFFSET) |
								(ERR_BAD_PARM << TYPE_OFFSET));

			pu8Dest += uCopyLen;

			uCalcLen = BitBuffSize(&(pTransTile->BsTransTileDat));
			uCopyLen = BitBuff2Mem(pu8Dest, &(pTransTile->BsTransTileDat), jbEnv);
			if (uCalcLen != uCopyLen)
				longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
								(__LINE__ << LINE_OFFSET) |
								(ERR_BAD_PARM << TYPE_OFFSET));

			pu8Dest += uCopyLen;
		}
	}

	/* Pad to the next DWord and then pad an extra DWord.  This is needed
	   for the current implementation of the decoder. */
	uPadLength = DWordSize - ((pu8Dest - pInitPtr) % DWordSize);
	if(uPadLength <= DWordSize) {
		uPadLength += DWordSize;
	}
	memset(pu8Dest, 0, uPadLength);
	pu8Dest += uPadLength;

	/* Finally, scramble the bitstream for key frame lock if necessary */

	if (pu16Scramble != NULL) {

		U32 uCount, uKeyValue, uMultiplier, uKey, uScrambleLen;
				
		if (pBsPicInfo->Planes[COLOR_Y].Bands[0].BandHdr.BandDataSize >= 68)
			uScrambleLen = 64;
		else
			uScrambleLen = ((pBsPicInfo->Planes[COLOR_Y].Bands[0].BandHdr.BandDataSize - 4) >> 1) << 1;

		uKey = pBsPicInfo->Hdr.GOPHdr.AccessKey;

		uKeyValue = (0xffffffff - uKey) >> 16;

		uMultiplier = (((0x4000 + ((((uKey & 0xff) << 8) +
    		                         ((uKey & 0xff00) >> 8)) >> 1))
						              / 200) * 200) + 21;
				
		for (uCount = 0; uCount < uScrambleLen; uCount += 2) {
			uKeyValue = ((uKeyValue * uMultiplier) + 1) % 0x10000;
			*pu16Scramble = *pu16Scramble ^ (U16) uKeyValue;
			pu16Scramble++;
		}
	}

	return;

}    /*** End of CopyBitBuffs() ***/


/* --------------- Resize BitBuffers -----------------*/
/* Add the data size to the buffer.  Space should have already
   been set aside at the right location in the buffer.
*/
/*static*/
void AddPictureSize(U32 uDataSize, pBitBuffSt pBsPicHdr, jmp_buf jbEnv)
{
	I32 iTmp;
	U32 uSize = uDataSize;

	if (uDataSize>MaxPicDataSize) {	/* overflow? */
		uSize = 0;
	}
	/* Make sure the buffer is ready to read */
	BitBuffFlush(pBsPicHdr);
	iTmp = BitBuffRead(pBsPicHdr, LenPicFlags, jbEnv); 

	if(iTmp && PicFlags_DataSizeFlag ) {
		BitBuffOverWrite(pBsPicHdr, uSize, LenPicDataSize, jbEnv);
	}

	/* Reset the bit buffer for subsequent reading */
	BitBuffFlush(pBsPicHdr);	

}    /*** End of AddPictureSize() ***/
