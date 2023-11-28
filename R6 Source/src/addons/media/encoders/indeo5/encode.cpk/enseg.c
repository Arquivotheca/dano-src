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
 * enseg.c
 *
 * These routines segment a color plane, determine the quantization
 * and coding method of each segment, and encode it.
 *
 * Functions:
 * 	EncSegOpen		Open up a segment context structure
 *  EncSegClose		Free a segment context structure
 *  EncSeg			Encode a segment
 */

#include <string.h>		/* for memset */
#include <math.h>		/* for log */
#include <setjmp.h>

#include "datatype.h"
#include "matrix.h"
#include "tksys.h"

#include "mc.h"			/* for ivi5bs.h */
#include "hufftbls.h"
#include "qnttbls.h"

#include "ivi5bs.h"		/* for bsutil.h */
#include "mbhuftbl.h"
#include "bkhuftbl.h"

#ifdef SIMULATOR
#include "encsmdef.h"
#include "decsmdef.h"
#endif

#include "bsutil.h"
#include "pia_main.h"
#include "indeo5.h"
#include "ensyntax.h"
#include "encoder.h"
#include "enseg.h"
#include "xfrm.h"
#include "errhand.h"
#include "common.h"

#ifdef SIMULATOR

#ifdef CMD_LINE_ENC
#include "CmdLParm.h"  
#endif	/*CMD_LINE_ENC*/
#include "pia_enc.h"   /* For encparse.h in encsim.h */
#include "encsim.h"
#endif	 /* SIMULATOR*/

const  I32 AdaptQThreshold[6]={0, 1, 2, 3, 5, 10000};  /* Adaptive Quant. Thresholds */
#define CLIP(a,lo,hi) ((a)<(lo)?(lo):((a)>(hi)?(hi):(a)))

/* ------------- PROTOTYPES FOR PRIVATE ROUTINES --------------- */
static I32 IntraGrad(MatrixSt Pic, RectSt Rect);

/* ----------------------- OPEN ------------------------- */
PEncSegCntxSt EncSegOpen(
	PEncConstInfoSt pCinfo,
	PEncRtParmsSt pRtParms,
	pTileSt	pTileInfo,
	I32 iNumTiles,
jmp_buf jbEnv)
{
	PEncSegCntxSt pSegContext;
	
	pSegContext = (PEncSegCntxSt) SysMalloc(sizeof(EncSegCntxSt), jbEnv);

#ifdef SIMULATOR
	pSegContext->pSimInst = pRtParms->pSimInst;  /* Pointer to simulator Instance */
#endif /* SIMULATOR */

	if( pCinfo->Color == COLOR_Y ) { 
		pSegContext->BlockSize = pRtParms->YBlockSize[pCinfo->BandId];
	} else { /* u or v plane */
		pSegContext->BlockSize = pRtParms->VUBlockSize[pCinfo->BandId];
	}
	pSegContext->XfrmBlock = 
		(PI32) SysMalloc(pSegContext->BlockSize*pSegContext->BlockSize * 
						sizeof(I32), jbEnv);
	
	pSegContext->Activity = (PDbl) 
			SysMalloc(	iNumTiles * pTileInfo->NumMacBlocks * sizeof(Dbl),
						jbEnv);

	return pSegContext;
}


/* ----------------------- CLOSE ------------------------- */
/* End the sequence; free any allocated storage
*/
void EncSegClose(PEncSegCntxSt pSegContext, jmp_buf jbEnv)
{
	SysFree((PU8) pSegContext->XfrmBlock, jbEnv );
	SysFree((PU8) pSegContext->Activity, jbEnv);
	SysFree((PU8) pSegContext, jbEnv);
}

/* ----------------------- ENCODE ------------------------- */
/*
 * Encode pic, tile by tile, rect by rect
 * Converts dQuant to iQLevel for VQ, SQ, etc.
 */
void EncSeg(
	pcBandSt pcBandInfo, 		/* Band level info structure */
	pTileSt pTileInfo,			/* ptr to the Tile array */
	PEncSegCntxSt pSegContext,
	MatrixSt mPicOrig,			/* orig */
	MatrixSt mPicMc,			/* in: predicted pic */
	MatrixSt mPicXfrmRes,		/* in: xfrmed residule pic */
	MatrixSt mPicX,				/* out: reconstructed pic */
	I32 iNumTiles,				/* number of tiles to process */
	U8 u8Band0GlobalQuant,		/* Used for Band 0 inheritance */
	U8 	u8EncodeMode,
	jmp_buf jbEnv)
{
	I32	iNdx;					/* array index into Activity*/
	I32 iTiles, iMBlocks;		/* for loop indices */
	I32 iQDelta;
	I32 dAct;					/* activity of block */
	I32	dRatioAct;				/* ratio of dAct to dAveAct */
	I32 dSum = 0;
	I32 iQLevel;				/* quantization LEVEL */
	pMacBlockSt pMBInfo;
	pMacHdrSt pMBInfoHdr;
	U8 u8Intra;
	U8 u8Cbp = 0;
	I32 iValue;
	I32 iPrevDC;
	I32 iZigzag;				/* ZigZag table entry value */
	I32 iCnt;					/* Block Pixel Counter */
	U8 u8Run;
	I32 iQuant;					/* Quant Table Value */
	U8 u8BlockFlag = 1;
	pBlockSt pBlockInfo;
	pRunValSt pRVal;
	PI32 piBlock;
	RectSt rCurrent;
	I32 iBlockNum;
	I32 iBlockSize;
	I32 iTotalNumBlks;			/* The total number of macro blocks */
	U8 u8Xfrm=pcBandInfo->pBandDscrptor->Xform;
	PIA_Boolean bDCDPCM;  /* If TRUE, we do DPCM for DC, otherwise, we don't. */
	PIA_Boolean bUseQuantDelta = pcBandInfo->BandHdr.QuantDeltaUsed;
	PIA_Boolean bQuantUseBand0 = pcBandInfo->BandHdr.InheritQ;
	I32 iDeQuantSize;
	U32 uCoded;
	I32 iQTest;
	I32 i;
	I32 q_jump_step;			/* Quantization step for adaptive Quant */     		


 /* DPCM of DC only for 2 Dim transform blocks */
	if  ( (u8Xfrm == XFORM_SLANT_8x8) || (u8Xfrm == XFORM_SLANT_4x4) ) {
		bDCDPCM = TRUE;
	} else {
		bDCDPCM = FALSE;
	}

	/* set up the blocksize and block pointer, for this band */
	iBlockSize = pSegContext->BlockSize;
	piBlock = pSegContext->XfrmBlock;
	rCurrent.h = rCurrent.w = iBlockSize;
		
	if (!bUseQuantDelta) { 	/* if variable quant has been disabled */
		if  (u8EncodeMode != ENC_MODE_FINAL) {
			/*then just copy the global quant into place for each macroblock */
			for (iTiles = 0; iTiles < iNumTiles; iTiles++) {	
				for (iMBlocks = 0; iMBlocks < (pTileInfo + iTiles)->NumMacBlocks;
					 iMBlocks++) { 
					pMBInfo = ((pTileInfo + iTiles)->MacBlocks) + iMBlocks;
					pMBInfo->MacHdr.QuantDelta = pcBandInfo->BandHdr.GlobalQuant;
				} /* end for (iMBlocks ...) */
			} /* end for (iTiles ...) */
		}
	} else {

		/* Otherwise, go ahead and do the variable quant if necessary */
		if (!bQuantUseBand0) { 
			/* the QuantDelta is not inherited from band 0: then derive here */

			/* step 1: compute the average activity throught out the band */
			if ( (u8EncodeMode == ENC_MODE_TEST_WATER) || (u8EncodeMode == ENC_MODE_ALL) ) {
				for (iTiles = 0, iTotalNumBlks = 0; iTiles < iNumTiles; iTiles++) {	
					for (iMBlocks = 0; iMBlocks < (pTileInfo + iTiles)->NumMacBlocks;
						iMBlocks++, iTotalNumBlks++) { 
						/* Calc the activity of each macroblock to get the avg */
						pMBInfo = ((pTileInfo + iTiles)->MacBlocks) + iMBlocks;
						iNdx = (iTiles * pTileInfo->NumMacBlocks) + iMBlocks;
						pSegContext->Activity[iNdx] = IntraGrad(mPicOrig, 
															pMBInfo->MacBlockRect);
						dSum += (I32) pSegContext->Activity[iNdx];
					} /* end for (iMBlocks ...) */
				} /* end for (iTiles ...) */
				pSegContext->AveActivity = dSum / iTotalNumBlks;

				if (pSegContext->AveActivity  == 0) {
					pSegContext->AveActivity  = 1.0;	/* avoid divide by 0 */
				}
			}	  /* end of if u8EncodeMode ... */
			
			/* step 2: now set the quant level based on local and avg activity */
			if  (u8EncodeMode != ENC_MODE_FINAL) {
				for (iTiles = 0; iTiles < iNumTiles; iTiles++) {	
					for (iMBlocks = 0; iMBlocks < (pTileInfo + iTiles)->NumMacBlocks;
						iMBlocks++) { 
						pMBInfo = ((pTileInfo + iTiles)->MacBlocks) + iMBlocks;
						iNdx = (iTiles * pTileInfo->NumMacBlocks) + iMBlocks;

						dAct = (I32) pSegContext->Activity[iNdx];
						if (dAct == 0) dAct =1;
						if (dAct <= (I32) pSegContext->AveActivity) {
							dRatioAct = (I32) (pSegContext->AveActivity /dAct);
							q_jump_step = 0;
						} else {
							dRatioAct = (I32) (dAct / pSegContext->AveActivity);
							if (dRatioAct <= 2) {
								q_jump_step = 1;
							} else {
								q_jump_step = 2;
							}
						} 
                
						/* dRatioAct is guarateed greater than 1.0 */
						if (dRatioAct<AdaptQThreshold[0])
							longjmp(jbEnv, 	(ENSEG        << FILE_OFFSET) |
											(__LINE__     << LINE_OFFSET) |
											(ERR_BAD_PARM << TYPE_OFFSET));
						i=5;  
						while (dRatioAct<=AdaptQThreshold[i]) i--;  /* find the right interval */

						iQDelta = q_jump_step*i;
						pMBInfo->MacHdr.QuantDelta = CLIP(pcBandInfo->BandHdr.GlobalQuant 
												+ iQDelta, 0, 23);

					} 	/* end for (iMBlocks ...) */
				}  /* end for (iTiles ...) */
			}	/* done step 2 */
		/* Done with the spatially adaptive Quantization */
		} else 	{	/* Inherit dQDelta from Band 0,	already in place */
			if  (u8EncodeMode != ENC_MODE_FINAL) {
				for (iTiles = 0; iTiles < iNumTiles; iTiles++) {
					for (iMBlocks = 0; iMBlocks < (pTileInfo + iTiles)->NumMacBlocks; iMBlocks++) {
						/* the quants are copied to all inherited bands in the routine above
						 * so only the global quant level adjustment needs to be made here
						 */

						pMBInfo = ((pTileInfo + iTiles)->MacBlocks) + iMBlocks;
						/* Note: pMBInfo->MacHdr.QuantDelta was set to the QuantDelta in Band0 of Y before Enseg() */
						pMBInfo->MacHdr.QuantDelta  += pcBandInfo->BandHdr.GlobalQuant 
												- u8Band0GlobalQuant;
						pMBInfo->MacHdr.QuantDelta = CLIP(pMBInfo->MacHdr.QuantDelta, 0, 23);

						if ((pMBInfo->MacHdr.QuantDelta < 0) || (pMBInfo->MacHdr.QuantDelta > 23))
							longjmp(jbEnv, 	(ENSEG        << FILE_OFFSET) |
											(__LINE__     << LINE_OFFSET) |
											(ERR_BAD_PARM << TYPE_OFFSET));
					} /* end for (iMBlocks ...) */
				} /* end for (iTiles ...) */
			}  /* end if !ENC_MODE_FINAL */
		}	/* End case inherit q delta from band 0 */
	
	}	/* end of if (!bUseQuantDelta) else ... */

	for (iTiles = 0; iTiles < iNumTiles; iTiles++) { 

		iPrevDC = 0;  /* initi prevDC for this tile */
		for (iMBlocks = 0; iMBlocks < (pTileInfo + iTiles)->NumMacBlocks; iMBlocks++) { 

			pMBInfo = ((pTileInfo + iTiles)->MacBlocks) + iMBlocks;
			pMBInfoHdr = &pMBInfo->MacHdr;
			iQLevel = pMBInfoHdr->QuantDelta;
			u8Intra = (U8) (pMBInfoHdr->MbType == TYPE_MV_I);

			iNdx = (iTiles * pTileInfo->NumMacBlocks) + iMBlocks;
			u8Cbp = 0;
			u8BlockFlag = 1;
			pBlockInfo = pMBInfo->Blocks;

			for (iBlockNum = 0; iBlockNum < pMBInfo->NumBlocks; iBlockNum++) {
				rCurrent = pBlockInfo[iBlockNum].BlockRect;
				rCurrent.h = rCurrent.w = iBlockSize;
				memset(piBlock, 0, iBlockSize*iBlockSize*sizeof(I32));

				switch (u8EncodeMode) {
				case ENC_MODE_TEST_WATER:
				case ENC_MODE_ALL:			/*  transfrom all the blocks */
						/* do the differencing and get the difference block */
						BlockGetDiff(mPicOrig, mPicMc, rCurrent, piBlock);
						/* forward transform */
						transform(piBlock, FORWARD, pcBandInfo->pBandDscrptor->Xform);
						/* put back the block into the final reconstructed pic */
						BlockPut(piBlock,iBlockSize, mPicXfrmRes, rCurrent);
						break;
				case ENC_MODE_TRIAL:
				case ENC_MODE_FINAL:
						/* get the transformed block from mPicXfrmRes */
						BlockGet(piBlock, mPicXfrmRes, rCurrent);
						break;
				default:
					longjmp(jbEnv,  (ENSEG << FILE_OFFSET) |
									(__LINE__ << LINE_OFFSET) |
									(ERR_BAD_PARM << TYPE_OFFSET));
					break;
				}

				if ((bDCDPCM) && (u8Intra)) { 
					/* If it is determined that DPCM for DC is appropriate */
					piBlock[0] -= iPrevDC; 
				}

				/* If the global quant level is in [6,16], this code, will attempt 
				 * to overquantize a block to force more empty blocks if possilbe.
				 * If fail, just quantize as normal.
				 */

				uCoded = 0;
					
				if (iQLevel > 6 && iQLevel <= 9) {
					iQTest = CLIP(iQLevel + 2 , 0, 23);
				} else if (iQLevel > 9 && iQLevel <= 13) {
                           iQTest = CLIP(iQLevel + 5 , 0, 23);
				} else if (iQLevel > 13 && iQLevel <=16) {
                           iQTest = CLIP(iQLevel + 7 , 0, 23);
				} else {
					    iQTest = iQLevel;
				}
	
				/* over quantize the block */
				for (iCnt = 0; iCnt < (I32) (iBlockSize*iBlockSize); iCnt++) {
					iValue = piBlock[iCnt];
		
					iQuant = pcBandInfo->uwQuantTable[u8Intra][iQTest][iCnt];
					if ((iValue >= iQuant) || (iValue <= -iQuant))	
						uCoded++;
				}

				if (uCoded == 0) {
					/* force the block to empty */
					for (iCnt = 0; iCnt < (I32) (iBlockSize*iBlockSize); iCnt++) {
						piBlock[iCnt] = 0;
					}
				} else {
				/* quantize the block as normal, using the original iQLevel, not the iQTest  */
					for (iCnt = 0; iCnt < (I32) (iBlockSize*iBlockSize); iCnt++) {
						iValue = piBlock[iCnt];
						iQuant = pcBandInfo->uwQuantTable[u8Intra][iQLevel][iCnt];
						if (iQuant != 1)  { /* No dither if iQuant == 1 */
							if (iValue > 0) {
								iValue = iValue / iQuant;
								piBlock[iCnt] = iValue;
							} else if (iValue < 0)  {
								iValue = -(-iValue / iQuant);
								piBlock[iCnt] = iValue;
							}
						}	/* end case iQuant > 1 */
					}
				}

				if (u8EncodeMode!=ENC_MODE_FINAL) {
					/* now do the scanning of the quantized transformed blocks */
					u8Run = 1;
					pBlockInfo[iBlockNum].NumRunVals = 0;
					pRVal = pBlockInfo[iBlockNum].RunVals;
					/* scan the block */
					for (iCnt = 0; iCnt < (I32)(iBlockSize*iBlockSize); iCnt++) {
						iZigzag = pcBandInfo->pubScan[iCnt]; /* next entry in zig-zag order */
						iValue = piBlock[iZigzag];	/* quantized iValue at this index */
						if (iValue == 0)
							u8Run++;
						else {
							pRVal->Run = u8Run;
							pRVal->Val = (I16) iValue;
							pBlockInfo[iBlockNum].NumRunVals++;
							u8Run = 1;	/* Reset run-length counter */
							pRVal++;   
						}
					} /* end for each element in this block */

					if(pBlockInfo[iBlockNum].NumRunVals > 0) 
						u8Cbp |= u8BlockFlag;				
					u8BlockFlag <<= 1;

					/* The test that follows is special case code to handle 
					 * when a macroblock is over half off the right 
					 * edge of the frame.  This results in a macroblock 
					 * with only two blocks... In this case, the Blockflag 
					 * below has to be bumped from 1 to 4 (instead of 2) 
					 * so that the CBP for the next block is correct. 
					 */
					if (pMBInfo->MacBlockRect.w <= pcBandInfo->pBandDscrptor->BlockSize)  {
						u8BlockFlag <<= 1;
					}

					pRVal->Run = EOB;
					pRVal->Val = (I16) 0;
					pBlockInfo[iBlockNum].NumRunVals++;
					pMBInfoHdr->Cbp = u8Cbp;
					pMBInfoHdr->IsEmpty = FALSE;  /* Defer decision until bs format time */

				}  /* end of if (u8EncodeMode!=ENC_MODE_FINAL) */

				if ((u8EncodeMode==ENC_MODE_FINAL) || (u8EncodeMode==ENC_MODE_ALL)) {
					iDeQuantSize = (I32)(iBlockSize*iBlockSize);
				} else {
					iDeQuantSize = 1;  /* only DC is dequantized if not ALL or FINLA modes */
				}

				/* dequantize the block */
				for (iCnt = 0; iCnt < iDeQuantSize; iCnt++) {
					iValue = piBlock[iCnt];
					iQuant = pcBandInfo->uwQuantTable[u8Intra][iQLevel][iCnt];
					if (iQuant == 1)	{ /* no dither for this case */
						piBlock[iCnt] = iValue;
					} else { /* dither */
						if ( iValue > 0 )
							piBlock[iCnt] = (iValue*iQuant + iQuant/2 - (iQuant & 1)) * (iValue != 0);
						else if (iValue < 0)
							piBlock[iCnt] = (iValue*iQuant - iQuant/2 + (iQuant & 1)) * (iValue != 0);
					} /* end case iQuant > 1 */
				}

				/* add prevDC to dc iValue and update prevdc if appropriate */
				if ( (bDCDPCM) && (u8Intra)) {
					piBlock[0] += iPrevDC;
					iPrevDC = piBlock[0]; 
				}

				if ((u8EncodeMode==ENC_MODE_FINAL) || (u8EncodeMode==ENC_MODE_ALL)) {
					/* inverse transform */
					transform(piBlock, INVERSE, pcBandInfo->pBandDscrptor->Xform);

					iValue = piBlock[iCnt];
					/* put back the piBlock */
					BlockPut(piBlock,iBlockSize, mPicX,rCurrent);
				}	/* end of if ((u8EncodeMode==ENC_MODE_FINAL) || (u8EncodeMode==ENC_MODE_ALL)) */

			} /* end for each block in this macroblock */
		} /* end for each macroblock in this tile */
	} /* end for each tile in this band */

	return;
}

/* ----------------------- STATIC ROUTINES ------------------------- */


/* Calculate the magnitudes of the INTERNAL IntraGrad. 
Internal => stay within confines of the Rect.
IntraGrad: sum of the magnitudes of the row and col gradients.
(For large rect, it would be faster to use assembly code: EncMeRectErr.)
*/
static I32 IntraGrad(
MatrixSt mPic,
RectSt Rect)
{
	I32 iSum=0;
	I32 iCount;
	I32 iPel;
	I32 iRow;
	I32 iHeight = Rect.h - 1;	 /* "-1" keeps it internal */
	I32 iWidth = Rect.w - 1;   
	I32 iDiff = iHeight  * iWidth; /* num of (pairs of) values calculated */
	PI16 pi16;
	
	for (iRow = (I32)Rect.r; iRow < (I32)(Rect.r + iHeight); iRow++) {
		pi16 = mPic.pi16 + iRow * mPic.Pitch + Rect.c;
		for (iCount=0; iCount < iWidth; iCount++, pi16++) {
			iPel = (I32) *pi16;
			iSum += ABS(iPel - (I32)*(pi16 + 1)) + 
			ABS(iPel - (I32) *(pi16 + mPic.Pitch));
		}
	}
	
	return (iDiff > 0) ? (I32) iSum / iDiff : 0;
}
