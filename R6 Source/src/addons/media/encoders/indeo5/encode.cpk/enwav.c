/***********************************************************************
*                                                                      *
*              INTEL CORPORATION PROPRIETARY INFORMATION               *
*                                                                      *
*   This listing is supplied under the terms of a license agreement    *
*     with INTEL Corporation and may not be copied nor disclosed       *
*       except in accordance with the terms of that agreement.         *
*                                                                      *
************************************************************************
*                                                                      *
*             Copyright (C) 1994-1997 Intel Corp.                        *
*                        All Rights Reserved.                          *
*                                                                      *
***********************************************************************/

/* enwav.c
 *
 * This is the wavelet code that supports scalability.  A context is
 * created for each level of each band, but the wavelet code, itself,
 * executes in a single context (for each color).
 *
 * Functions:
 *	EncPlane		Encode a single band
 *	EncPlaneOpen	Malloc the space for a band and initialize it
 *  EncPlaneClose	Free that allocated memory
 */

#include <setjmp.h>
#include "datatype.h"	
#include "tksys.h"
#include "matrix.h"

#include "mc.h"			/* for ivi5bs.h */
#include "hufftbls.h"
#include "qnttbls.h"

#include "ivi5bs.h"		/* for bsutil.h */
#include "mbhuftbl.h"
#include "bkhuftbl.h"
#include "pia_main.h"
#ifdef SIMULATOR
#include "decsmdef.h"
#include "encsmdef.h"
#endif
#include "bsutil.h"
#include "indeo5.h"
#ifdef CMD_LINE_ENC
#include "CmdLParm.h"  
#endif	/*CMD_LINE_ENC*/
#include "pia_enc.h"

#include "ensyntax.h"
#include "encoder.h"
#include "enme.h"
#include "enmesrch.h"
#include "enseg.h"

#ifdef SIMULATOR 
#include "prefilt.h"
#endif /* SIMULATOR */

#include "context.h"
#include "const.h"
#include "errhand.h"
#include "entrans.h"
#include "common.h"

#include "enntry.h"
#include "enntryns.h"		 
#include "enwav.h"
#include "decomp.h"
#include "recomp.h"

#ifdef SIMULATOR
#include "encsim.h"
#endif



/* PROTOTYPES OF STATIC ROUTINES */

static void openBands(pPlaneSt pBsPlaneInfo,
					  pPicHdrSt pPicHdr,
					  PEncPlaneCntxSt pPlaneCntx,
					  PEncRtParmsSt pParms, 
					  PEncConstInfoSt pCInfo, 
					  PI32 piBandId, 
					  PI32 piSubDivNdx, 
					  ppBandSt ppBand0Info, 
					  PI32 piNesting, 
					  jmp_buf jbEnv);

static void encodeBands(	PTR_ENC_INST pInst,
							PCPicHdrSt pcPicInfoHdr, 
							pPlaneSt pBsPlaneInfo,
							PEncPlaneCntxSt pPlaneContext, 
							MatrixSt mPicOrig,
							MatrixSt mPicX,
							PI32 piBandId, 
							PI32 piSubDivNdx, 
							U8 u8EncodeMode,
							PI32 piNesting,
							jmp_buf jbEnv );
		

/* ----------------------  OPEN  ---------------------------------*/
/* Initialize the sequence and return ptr to WavCntxSt. 
*/
PEncPlaneCntxSt EncPlaneOpen(
pPlaneSt pBsPlaneInfo,
pPicHdrSt pPicHdr,
PEncConstInfoSt pCInfo,
PEncRtParmsSt pParms,
ppBandSt ppBand0Info,
jmp_buf jbEnv)
{
	PEncPlaneCntxSt pPlaneContext;
	I32 k, iLenSub, iNumBands=0, iNesting = 0;
	PU8 pu8SubDiv;
	
	pPlaneContext = (PEncPlaneCntxSt) SysMalloc(sizeof(EncPlaneCntxSt), jbEnv);
	pBsPlaneInfo->PlaneId = pPlaneContext->Color = pCInfo->Color;

#ifdef SIMULATOR /* Simulation infrastructure */
	pPlaneContext->pSimInst = pParms->pSimInst;  /* Pointer to simulator Instance */
#endif /* SIMULATOR */


	/* set up subdivision codes */
	if( pCInfo->Color == COLOR_Y ) {
		pu8SubDiv = pParms->YSubDiv;
		iNumBands = pParms->YNumBands;
		pPicHdr->GOPHdr.YDecompLevel = (U8) pParms->YLevels;	
	} else { /* this is the v or u plane */
		pu8SubDiv = pParms->VUSubDiv;
		iNumBands = pParms->VUNumBands;
		pPicHdr->GOPHdr.VUDecompLevel = (U8) pParms->VULevels;	
	}

	iLenSub = iNumBands*2;
	pBsPlaneInfo->SubDiv = (PU8) SysMalloc(iLenSub + 1, jbEnv);
	for( k=0; k<iLenSub+1; k++ ) {
	   pBsPlaneInfo->SubDiv[k] = pu8SubDiv[k];
 	}
	pPlaneContext->NumSubDivCodes = iLenSub;
	/* done setting up subdivision codes */

	pBsPlaneInfo->NumBands = pPlaneContext->NumBands = iNumBands;
	pBsPlaneInfo->Bands = (BandSt *)SysMalloc(sizeof(BandSt) * iNumBands, jbEnv);

	/* recursively open all bands */
	k = iNumBands = 0;				/* init index and band ctr */
	openBands(	pBsPlaneInfo, pPicHdr,
				pPlaneContext, pParms, pCInfo, &iNumBands, &k, 
				ppBand0Info, &iNesting, jbEnv);
	if (iNumBands != pPlaneContext->NumBands)
		longjmp(jbEnv, 	(ENWAV << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_BAD_PARM));

	return pPlaneContext;
}

/* ----------------------  CLOSE  ---------------------------------*/
/* End the sequence; free any allocated storage
*/
void EncPlaneClose(PEncPlaneCntxSt pPlaneContext, pPlaneSt pBsPlaneInfo,
				   jmp_buf jbEnv)
{
	I32 iBand, iLevel;
	
	for (iBand=0; iBand < pPlaneContext->NumBands; iBand++)
		EncBandClose(pPlaneContext->EncBandCntx[iBand], 
					&pBsPlaneInfo->Bands[iBand], jbEnv);

	if (pPlaneContext->Color == COLOR_Y) {
	/* since all the band descriptors are
	 * allocated once, we should free them just once.
	 */
		SysFree( (PU8)pBsPlaneInfo->Bands[0].pBandDscrptor, jbEnv);
	}

	for (iLevel = 0; iLevel < MAX_BAND_NESTING_LEVELS; iLevel++)
		for (iBand = 0; iBand < 4; iBand++)
			if (pPlaneContext->OrigBands[iLevel][iBand].pi16 != NULL)	{
				MatFree(pPlaneContext->OrigBands[iLevel][iBand], jbEnv);
				MatFree(pPlaneContext->XBands[iLevel][iBand], jbEnv);
			}


	SysFree((PU8) pPlaneContext, jbEnv);
	SysFree((PU8) pBsPlaneInfo->SubDiv, jbEnv);
	SysFree((PU8) pBsPlaneInfo->Bands, jbEnv);

}


/* ----------------------  ENCODE WAVELET  -------------------------*/
/* Encode pic; calc reconstructed pic
*/
void EncPlane(
	PTR_ENC_INST pInst,
	PCPicHdrSt pcPicInfoHdr, /* Header with general pic info */
	pPlaneSt pBsPlaneInfo,
	PEncPlaneCntxSt pPlaneContext,
	MatrixSt mPicOrig,	/* orig */
	MatrixSt mPicX, 	/* reconstructed pic */
	U8 u8EncodeMode,
	jmp_buf jbEnv ) /* mode to use for encoding (this is for multipass brc) */
{
	I32 iBandId=0, iSubDivNdx=0, iNesting=0;
	I32 i;

	/* If it is the Y plane and this is the first time the encoder has been
	   called for this frame as signified by the u8EncodeMode then do spatial
	   prefiltering.
	 */

#ifdef SIMULATOR
	if(pBsPlaneInfo->PlaneId == COLOR_Y) {
		if((u8EncodeMode == ENC_MODE_ALL) || (u8EncodeMode == ENC_MODE_TEST_WATER)) {
			if (EncSimSpatPrefilt(pPlaneContext->pSimInst)) {
				SpatPreFilter(mPicOrig,pBsPlaneInfo->GlobalQuant, 0, 23);
			}
		}
	}
#endif /* SIMULATOR */

	for( i=0; i<pBsPlaneInfo->NumBands; i++ )
		pBsPlaneInfo->Bands[i].BandHdr.GlobalQuant = pBsPlaneInfo->GlobalQuant;

	/* Shift mPicOrig left to save precision (only for Y plane) */
	if((u8EncodeMode == ENC_MODE_ALL) || (u8EncodeMode == ENC_MODE_TEST_WATER)) {
		if ( (pBsPlaneInfo->PlaneId == COLOR_Y) && (pBsPlaneInfo->NumBands > 1) )	{
			MatShiftLeft(mPicOrig, 2);
		} /* End if COLOR_Y */
	}

	/* recursively encode bands */
	encodeBands(pInst, pcPicInfoHdr, 
					pBsPlaneInfo,
					pPlaneContext, 
					mPicOrig,
					mPicX,
					&iBandId, 
					&iSubDivNdx, 
					u8EncodeMode,
					&iNesting, 
					jbEnv ); 

if((u8EncodeMode == ENC_MODE_ALL) || (u8EncodeMode == ENC_MODE_FINAL)) {
		if ( (pBsPlaneInfo->PlaneId == COLOR_Y) && (pBsPlaneInfo->NumBands > 1) )	{
			MatShiftRight(mPicX,2);
		} /* End if COLOR_Y */
    }


	if((u8EncodeMode == ENC_MODE_ALL) || (u8EncodeMode == ENC_MODE_FINAL)) {
		/* Translate and clamp the mPicX plane. Values will be centered on 0x80. */
		I16 val;
		U32 i,j;
	  	PI16 pi16Data;

		pBsPlaneInfo->Clamped = FALSE;

		pi16Data = mPicX.pi16;
		for (i=0;i<mPicX.NumRows;i++) {
			for (j=0;j<mPicX.NumCols;j++) {
				val = *pi16Data + 128;
				if (val > MaxClamp) {
					val = MaxClamp;
					pBsPlaneInfo->Clamped = TRUE;
				} else
					if (val < MinClamp) {
						val = MinClamp;
						pBsPlaneInfo->Clamped = TRUE;
					}
				*pi16Data++ = val;
		  	}
			pi16Data += mPicX.Pitch - mPicX.NumCols;
		}
	}	/*** end of mPicX plane translate & clamp ***/

#ifdef SIMULATOR  /* For diagnostic option /SNR:
                   *   compare the reconstructed frames with the original ones, collect statistics
                   * like SNR, pixel-wise maximum difference, etc. 
                   */
	if((u8EncodeMode == ENC_MODE_ALL) || (u8EncodeMode == ENC_MODE_FINAL)) {
		if (pBsPlaneInfo->PlaneId == COLOR_Y) {	
			EncSimSNR(&mPicOrig, &mPicX,pPlaneContext->pSimInst,0, FALSE ); 
		}
	}
#endif /* SIMULATOR */


}    /*** End of EncPlane() ***/


/* -------------- recursive code to encode bands ------------------- */

static void encodeBands(
	PTR_ENC_INST pInst,
	PCPicHdrSt pcPicInfoHdr, /* Header with general pic info */
	pPlaneSt pBsPlaneInfo,
	PEncPlaneCntxSt pPlaneContext, 
	MatrixSt mPicOrig,		/* orig */
	MatrixSt mPicX,		/* Reconstructed */
	PI32 piBandId,
	PI32 piSubDivNdx,
	U8 u8EncodeMode,
	PI32 piNesting,
	jmp_buf jbEnv )
{
	I32 band;
	
	if (*piSubDivNdx >= pPlaneContext->NumSubDivCodes)
		longjmp(jbEnv, 	(ENWAV        << FILE_OFFSET) |
						(__LINE__     << LINE_OFFSET) |
						(ERR_BAD_PARM << TYPE_OFFSET));

	switch (pBsPlaneInfo->SubDiv[(*piSubDivNdx)++]) {	/* process next code */
		case SUBDIVQUAD:

			/* form bands */

			/* We only need to do the band subdivision on the first time through */
			/* for a given frame.  This shouldn't change between recompressions. */

			if ((u8EncodeMode == ENC_MODE_TEST_WATER) || (u8EncodeMode == ENC_MODE_ALL)) {
				OptWaveletDecomp(mPicOrig, &(pPlaneContext->OrigBands[*piNesting][0]), i16DecompScale);
			}

			(*piNesting)++;
			/* Encode each band of this level */
			for (band=0; band < 4; band++) {
				encodeBands(pInst, 
							pcPicInfoHdr, 
							pBsPlaneInfo, 
							pPlaneContext, 
							pPlaneContext->OrigBands[*piNesting-1][band],
							pPlaneContext->XBands[*piNesting-1][band], 
							piBandId, 
							piSubDivNdx, 
							u8EncodeMode,
							piNesting,
							jbEnv );
			}
			(*piNesting)--;

			if ((u8EncodeMode == ENC_MODE_FINAL) || (u8EncodeMode == ENC_MODE_ALL))	  {
				OptWaveletRecomp(&(pPlaneContext->XBands[*piNesting][0]), mPicX,i16RecompScale);
			}


			break;

		case SUBDIVLEAF:
			if (*piBandId >= pPlaneContext->NumBands)
				longjmp(jbEnv, 	(ENWAV        << FILE_OFFSET) |
								(__LINE__     << LINE_OFFSET) |
								(ERR_BAD_PARM << TYPE_OFFSET));
			EncBand(pInst,
					pcPicInfoHdr, 
					&pBsPlaneInfo->Bands[*piBandId], 
					pPlaneContext->EncBandCntx[*piBandId], 
					mPicOrig, 
					mPicX,
					u8EncodeMode,
					jbEnv );
			
#ifdef SIMULATOR   /* /BANDERR option: collect error distribution across the bands   */
			if ((u8EncodeMode == ENC_MODE_FINAL) || (u8EncodeMode == ENC_MODE_ALL))	  {
				if (pBsPlaneInfo->PlaneId == COLOR_Y)  { /* For band0 in Y plane */
					EncSimBandErr(*piBandId, &mPicOrig, &mPicX, pPlaneContext->pSimInst, 0,FALSE);
				}
			}
#endif  /* SIMULATOR */

			(*piBandId)++;
			break;

		default:
			longjmp(jbEnv, 	(ENWAV        << FILE_OFFSET) |
							(__LINE__     << LINE_OFFSET) |
							(ERR_BAD_PARM << TYPE_OFFSET));
	}		
}

/* -------------- recursive code to open bands -------------------
*/
static void openBands(
pPlaneSt pBsPlaneInfo, 
pPicHdrSt pPicHdr,
PEncPlaneCntxSt pPlaneContext, 
PEncRtParmsSt pParms, 
PEncConstInfoSt pCInfo, 
PI32 pBandId,
PI32 piSubDivNdx,					/* index of subdivision code */
ppBandSt ppBand0Info,
PI32 piNesting,
jmp_buf jbEnv)
{
	I32 i;
	EncConstInfoSt CInfo;
	U8 u8NumYBands;
	U8 u8NumBands;
	
	CInfo = *pCInfo;	
	if (*piSubDivNdx >= pPlaneContext->NumSubDivCodes)
		longjmp(jbEnv, 	(ENWAV        << FILE_OFFSET) |
						(__LINE__     << LINE_OFFSET) |
						(ERR_BAD_PARM << TYPE_OFFSET));

	switch (pBsPlaneInfo->SubDiv[(*piSubDivNdx)++]) {	/* process next code */
		case SUBDIVQUAD:
			CInfo.Pic.NumRows = CInfo.Pic.NumRows / 2;
			CInfo.Pic.NumCols = CInfo.Pic.NumCols / 2;

			for (i=0; i<4; i++) {
				pPlaneContext->OrigBands[*piNesting][i] = CInfo.Pic;
				MatAlloc(&(pPlaneContext->OrigBands[*piNesting][i]), jbEnv);
				pPlaneContext->XBands[*piNesting][i] = CInfo.Pic;
				MatAlloc(&(pPlaneContext->XBands[*piNesting][i]), jbEnv);
			}
			
			(*piNesting)++;
			openBands(	pBsPlaneInfo, pPicHdr, pPlaneContext, pParms, &CInfo, pBandId, 
						piSubDivNdx, ppBand0Info, piNesting, jbEnv);
			openBands(	pBsPlaneInfo, pPicHdr, pPlaneContext, pParms, &CInfo, pBandId, 
						piSubDivNdx, ppBand0Info, piNesting, jbEnv);
			openBands(	pBsPlaneInfo, pPicHdr, pPlaneContext, pParms, &CInfo, pBandId, 
						piSubDivNdx, ppBand0Info, piNesting, jbEnv);
			openBands(	pBsPlaneInfo, pPicHdr, pPlaneContext, pParms, &CInfo, pBandId, 
						piSubDivNdx, ppBand0Info, piNesting, jbEnv);
			(*piNesting)--;
			break;
		case SUBDIVLEAF:
			if (*pBandId >= pPlaneContext->NumBands)
				longjmp(jbEnv, 	(ENWAV        << FILE_OFFSET) |
								(__LINE__     << LINE_OFFSET) |
								(ERR_BAD_PARM << TYPE_OFFSET));
			CInfo.BandId = *pBandId;
			u8NumYBands = 3*pPicHdr->GOPHdr.YDecompLevel+1;

			
			switch (pPlaneContext->Color) {
				case COLOR_Y:
					if (*pBandId==0) {
						/* allocate all the band descriptor structures at once */
						u8NumBands = u8NumYBands + 3*pPicHdr->GOPHdr.VUDecompLevel+1;
						pPicHdr->GOPHdr.pBandDscrpts =
						 (pBandDscrptSt)SysMalloc(sizeof(BandDscrptSt)*u8NumBands, jbEnv);
					}
						/* connect the pointer in the BandSt to the pointer in the GOPHdr */
					pBsPlaneInfo->Bands[*pBandId].pBandDscrptor = 
						pPicHdr->GOPHdr.pBandDscrpts+(*pBandId);
					break;
				case COLOR_V:
						/* connect the pointer in the BandSt to the pointer in the GOPHdr */
					pBsPlaneInfo->Bands[*pBandId].pBandDscrptor = 
						pPicHdr->GOPHdr.pBandDscrpts+(*pBandId)+u8NumYBands;
					break;
				case COLOR_U:		/* share with V */
					pBsPlaneInfo->Bands[*pBandId].pBandDscrptor = 
						pPicHdr->GOPHdr.pBandDscrpts+(*pBandId)+u8NumYBands;
					break;
			}

			pPlaneContext->EncBandCntx[*pBandId] = 
			EncBandOpen(&CInfo, pParms, &pBsPlaneInfo->Bands[*pBandId], 
						ppBand0Info, pPicHdr->GOPHdr.IsExpBS, jbEnv);

			/* Copy the transform type and drop bands CInfo from rtparms to the 
			*  context and the bs CInfo structure 
			*/
			if (pPlaneContext->Color == COLOR_Y)
			{
				pPlaneContext->XfrmBands[*pBandId] = pParms->YXfrm[*pBandId];
				pBsPlaneInfo->Bands[*pBandId].DropBand = FALSE;
			} else {
				pPlaneContext->XfrmBands[*pBandId] = pParms->VUXfrm[*pBandId];
				pBsPlaneInfo->Bands[*pBandId].DropBand = FALSE;
			}
			/* set the band xform in the CInfo structure */
			pBsPlaneInfo->Bands[*pBandId].pBandDscrptor->Xform = 
									(U8) pPlaneContext->XfrmBands[*pBandId];
			(*pBandId)++;
			break;
		default:
			longjmp(jbEnv, 	(ENWAV        << FILE_OFFSET) |
							(__LINE__     << LINE_OFFSET) |
							(ERR_BAD_PARM << TYPE_OFFSET));
	}
}
