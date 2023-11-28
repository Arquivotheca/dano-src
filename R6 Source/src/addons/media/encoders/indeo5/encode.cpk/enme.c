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
*               Copyright (c) 1994-1997 Intel Corp.			            *
*                         All Rights Reserved.                          *
*                                                                       *
************************************************************************/

/* Perform Motion Estimation for the band */

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

#ifdef SIMULATOR
#include "encsmdef.h"
#include "decsmdef.h"
#endif

#include "bsutil.h"
#include "pia_main.h"
#include "indeo5.h"
#include "ensyntax.h"
#include "enme.h"
#include "enmesrch.h"

/* ------------------------  OPEN  --------------------------*/
/* Initialize the sequence and return ptr to EncMeCntxSt. 
*/
PEncMeCntxSt EncMeOpen(PEncRtParmsSt pRtParms, jmp_buf jbEnv)
{
	PEncMeCntxSt pMeContext;
	
	/* get/setup context */
	pMeContext = (PEncMeCntxSt) SysMalloc(sizeof(EncMeCntxSt), jbEnv);

#ifdef SIMULATOR
	pMeContext->pSimInst = pRtParms->pSimInst;  /* Pointer to simulator Instance */
#endif /* SIMULATOR */

	/* In EnSyntax() 4 different motion estimations are defined at this
	   point those parameters defined there are copied over.
	*/
	pMeContext->Measure = pRtParms->MeMeasure;
	pMeContext->Resolution = pRtParms->MeResolution;
	pMeContext->Tactic = pRtParms->MeSearch[0];
	pMeContext->StepRatio = pRtParms->MeSearch[1];
	pMeContext->InitStep = pRtParms->MeInitSs;
	/* Set range as set at the command line and make sure it is 
	   a multiple of the initial step size.
	*/
	pMeContext->MaxDelta = ((pRtParms->MeRange*MC_UNIT)/pMeContext->InitStep) *
							pMeContext->InitStep;

	return pMeContext;
}


/* ------------------------  CLOSE  --------------------------*/
/* End the sequence; free any allocated storage
*/
void EncMeClose(PEncMeCntxSt pMeContext, jmp_buf jbEnv)
{
	SysFree((PU8)pMeContext, jbEnv);
}


/* ---------------  ESTIMATE MOTION - SMOOTH RESULTS  --------------*/

void EncMe(
	PEncMeCntxSt pMeContext,
	MatrixSt mPicRef,		/* reference pic */
	MatrixSt mPicOrig,		/* orig pic */
	I32 iNumTiles,			/* Number of tiles */
	pTileSt pTileInfo,		/* Pointer to tile info */
	jmp_buf jbEnv)
{
	MatrixSt TilePicRef;	/* Used to restrict motion search to tile boundry */
	MatrixSt TilePicOrig;	/* Used to restrict motion search to tile boundry */
	I32 h = mPicOrig.NumRows, w = mPicOrig.NumCols;  /* pic height/width */
	pMeBlockSt pMEInfo;
	I32 iTile, iMB;			/* Indices to loop through tiles and MB's */
	pTileSt pTile;
	pMacBlockSt pMBInfo;
	RectSt rMbRect;
	
	/* Do motion estimation */
	pTile = pTileInfo;
	for(iTile = 0; iTile < iNumTiles; iTile++, pTile++) {
		SetMatrixRect(&TilePicRef, mPicRef, pTile->TileRect, jbEnv);
		SetMatrixRect(&TilePicOrig, mPicOrig, pTile->TileRect, jbEnv);
		pMBInfo = pTile->MacBlocks;

		for (iMB = 0; iMB < pTile->NumMacBlocks; iMB++, pMBInfo++) {
			pMEInfo =  &pMBInfo->pMEInfo[TYPE_MV_FW];

			/* Set the rect to the literal rect values */
			rMbRect = pMBInfo->MacBlockRect;

			/* Set the rect with respect to the tile boundries */
			rMbRect.r -= pTile->TileRect.r;
			rMbRect.c -= pTile->TileRect.c;

			/* TODO In general it can beneficial to start your motion estimation 
			   search from the same location as the last motion vector for
			   this block.  Currently we set the initial
			   search point to zero as seen in the statement below.
			   This should be investigated further in the future.
			*/
			pMEInfo->Vect.r  = pMEInfo->Vect.c = 0 ;
			
			/* Fill in the Ref and Targ (Orig) fields */

			pMeContext->pRef = &TilePicRef;
			pMeContext->pTarg = &TilePicOrig;

			/* Find the best motion vector */
			pMEInfo->Error = EncMeRect(pMeContext, rMbRect, &pMEInfo->Vect, jbEnv);

		}
	}
}

/* Returns the intra error for this rect given the original image.
*/
Dbl CalcIntraErr(
	MatrixSt mOrig,
	RectSt rRect,
	I32 iMeasure)
{
	PI16 pPel;
	I32 iWcnt, iTmp, iSum;
	I32 iY, iRow = rRect.r, iCol = rRect.c, iHeight = rRect.h, iWidth = rRect.w ;
	I32 iAvg;				/* avg pel value - basis for err calc */
	Dbl dErr, dArea = iHeight * iWidth ;
	
	iSum = 0;			/* first calc ave */
	for (iY = 0; iY < iHeight; iY++) {
		pPel = mOrig.pi16 + (I32) mOrig.Pitch * (iY + iRow) + iCol;
		iWcnt = iWidth;
		while (iWcnt--)
			 iSum += *pPel++;
	}
	iAvg = (I32) (iSum / dArea);

	iSum = 0;
	for (iY = 0; iY < iHeight; iY++) {
		pPel = mOrig.pi16 + (I32) mOrig.Pitch * (iY + iRow) + iCol;
		iWcnt = iWidth;
		while (iWcnt--) {
			iTmp = *pPel++ - iAvg;
			if (iMeasure == ME_MSE)
			    iTmp *= iTmp;
			else
			    iTmp = ABS(iTmp);
			iSum += iTmp;
		}
	}
	dErr = iSum / dArea;
	
	return dErr;
}                 
