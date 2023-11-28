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

/* 
	enmesrch.c

	DESCRIPTION:
		Low-level routines for motion analysis. These routines operate 
		on blocks, not whole images.

		(The "LOG_SEARCH" no longer has a special meaning. Both the 
		stepping search and full search can do a "log search", ie do the
		search with pregressively finer steps.)

		These routines have no context.

-----

General notes:
   r is generally a row index, and c, a column index.
   Arguments common to several routines are described in the prototype
for MeStep().
   If displacements "overshoot" their legal limits, they are eventually
"clipped".

-----

	All public prototypes are in enmesrch.h.
	All private prototypes are in this file.

	Routines:                      
		EncMeRect()             
		EncMeRectErr()        
		EncMeRectErrFast

		static MeDispCalc() 
		static MeFull()		  
		static MeStep()        Step search only
		static lin_search()    Step search only
		static error()         Step search only
		static MeSafeBin() 
		static MeSafeBin1d()
		static MeClipDispl() 

	Data:
		none

***********************************************************************/

#include <math.h>
#include <setjmp.h>
#ifndef __NO_WINDOWS__
#include <windows.h>
#endif
#include "datatype.h"
#include "tksys.h"
#ifdef SIMULATOR
#include "encsmdef.h"
#include "decsmdef.h"
#endif
#include "mc.h"
#include "hufftbls.h"
#include "qnttbls.h"
#include "ivi5bs.h"
#include "mbhuftbl.h"
#include "bkhuftbl.h"
#ifndef SIMULATOR
#include "blockdec.h"
#endif
#include "bsutil.h"
#include "pia_main.h"
#include "indeo5.h"

#include "ensyntax.h"
#include "enme.h"
#include "enmesrch.h"
#include "errhand.h"


#define ErrNorm 10000.          /* I32 <=> Dbl */

/* Find best match for block */


/***********************************************************************
		PRIVATE ROUTINE PROTOTYPES
***********************************************************************/

static Dbl
MeDispCalc(
	PCEncMeCntxSt	pcMeContext,
	RectSt			rCell,
	PMcVectorSt		pvDispl,
	jmp_buf jbEnv);

static void
MeFull(
	PCEncMeCntxSt 	pcMeContext,		/* Motion estimation parameters */
	I32				iNewDelta,
	RectSt     		rMBRect,		/* Macro block rect */
	PMcVectorSt 	pvLast,			/* Last vector to start from */
	I32	 			iDelta, 	  	/* Log search step */
	PI32			piBestErr);		/* the minimum error with the best MV */
	

static I32
MeStep(
	PCEncMeCntxSt pcMeContext, 		/* details of search */
	RectSt        rCell,			/* rectangular area to match */
	PMcVectorSt   pvDispl,     		/* starting/final displacement vector */
	I32           iDelta);			/* initial step size (1/16 pel) */

static I32
lin_search(
	PCEncMeCntxSt pcMeContext,
	RectSt     	 rCell,
	I32          iX,
	I32          iY,
	I32          iDx,
	I32          iDy,
	I32          iPrev,
	I32          iStart,
	PI32         piBest,
	PI32         piNextBest,
	PI32         piSign);

static I32
error(
	I32 iMaxDelta,
	PCEncMeCntxSt pcMeContext,
	RectSt rCell,
	I32 iX,
	I32 iY);

static void
MeSafeBin(
	PRectSt prBin,
	PRectSt prImBin,
	PMcVectorSt pvDispl,
	I32 iMargin);

static void
MeSafeBin1d(
	PI32 piLow,
	PI32 piLen,
	I32 iDispl,
	I32 iMargin,
	I32 iLoLim,
	I32 iHiLim);
                            
static PIA_Boolean
MeClipDispl(
	RectSt rImCell,
	RectSt rCell,
	PMcVectorSt pvDispl,
	jmp_buf jbEnv);

							
/***********************************************************************
	   PUBLIC FUNCTIONS
***********************************************************************/

/***********************************************************************
*
*  EncMeRect
*
*  In this incantation, just calls MeDispCalc
*
*  returns nothing
*
***********************************************************************/
Dbl EncMeRect(
	PCEncMeCntxSt pcMeContext,	/* Motion parameters */
	RectSt rMbRect,				/* Macro block rectangle */
	PMcVectorSt pvMcVect,		/* Starting and returned motion vector */
	jmp_buf jbEnv)
{
	
	RectSt rImgRect; /* the cell describing entire pic */
	
	rImgRect.r = rImgRect.c = 0;
	rImgRect.h = pcMeContext->pTarg->NumRows;
	rImgRect.w = pcMeContext->pTarg->NumCols;

	/* Find the best motion vector */
	return(MeDispCalc(pcMeContext, rMbRect, pvMcVect,jbEnv));

}

/***********************************************************************
*
*  EncMeRectErr
*
*   Compute either the MAD or MSE for the the Rect specified.  
*	pcRef and pcmTarg Reference and Target areas to compute the error
*	for; Rect specifies the rectangle within the Target to compute; iMeasure
*	specifies whether to computed the MAE or MSE ; 
*   Vect specifies the vector to translate Rect by
*	when looking in pcRef.  That is, the actual rectangles compared are
* 	Rect within pcmTarg, and Rect shifted by Vect within pcRef.
*
*	The value returned is the MAD , as requested.
***********************************************************************/
Dbl
EncMeRectErr(PCMatrixSt pcRef,
			 PCMatrixSt pcmTarg,
			 RectSt		Rect,
			 I32 		iMeasure,		
			 McVectorSt	Vect)
{

    PIA_Boolean bMad = (iMeasure == ME_MAD);  /* MSE or MAD error measure */

	Dbl dErr;		   		/* Average error */
	I32 iRow, iCol;			/* Row and Column indices */
	I32	iPErr;				/* Used for pixel error calculation */
	I32 iFracR, iFracC;		/* Fractional part of the motion vector */
	I32 iPelsR, iPelsC;		/* Motion vector in pixels */
	I32 iErrSum = 0;		/* Sum of error values */
	I32 iDimRef, iDimTarg;	/* Pitches used for pointer increments */
	PI16 pi16RefDat, pi16TargDat;	/* Pointer to the image data */

	McModRes(Vect.r, &iPelsR, &iFracR);
	McModRes(Vect.c, &iPelsC, &iFracC);
	iFracR *= 256 / MC_UNIT;
	iFracC *= 256 / MC_UNIT;

	pi16RefDat = pcRef->pi16 + (Rect.r + iPelsR) * pcRef->Pitch + Rect.c + iPelsC;
	pi16TargDat = pcmTarg->pi16 + Rect.r * pcmTarg->Pitch + Rect.c;
	iDimRef = pcRef->Pitch - Rect.w;
	iDimTarg = pcmTarg->Pitch - Rect.w;

	if (iFracR == 0 && iFracC == 0) {  /* Do fast if interp. not necessary */
		if (Rect.w % 4) { /* only a partial block -- can't unroll loop */
			if (bMad) {
				for (iRow = 0; iRow < (I32)Rect.h; iRow++) {
					for (iCol = 0; iCol < (I32)Rect.w; iCol++) {
						iPErr = *pi16TargDat++ - *pi16RefDat++;
						iErrSum += ABS(iPErr);
					}
					pi16RefDat += iDimRef;
					pi16TargDat += iDimTarg;
				}
			} else {
				for (iRow = 0; iRow < (I32)Rect.h; iRow++) {
					for (iCol = 0; iCol < (I32)Rect.w; iCol++) {
						iPErr = *pi16TargDat++ - *pi16RefDat++;
						iErrSum += iPErr * iPErr;
					}
					pi16RefDat += iDimRef;
					pi16TargDat += iDimTarg;
				}
			}
		} else { /* Able to unroll loop */
			if (bMad) {
				for (iRow = 0; iRow < (I32)Rect.h; iRow++) {
					iCol = (I32)Rect.w;
					while (iCol) { /* loop unrolled 4-way */
						iCol-=4;
						iPErr = *pi16TargDat++ - *pi16RefDat++;
						iErrSum += ABS(iPErr);
						iPErr = *pi16TargDat++ - *pi16RefDat++;
						iErrSum += ABS(iPErr);
						iPErr = *pi16TargDat++ - *pi16RefDat++;
						iErrSum += ABS(iPErr);
						iPErr = *pi16TargDat++ - *pi16RefDat++;
						iErrSum += ABS(iPErr);
					}
					pi16RefDat += iDimRef;
					pi16TargDat += iDimTarg;
				}
			} else { /* !bMad */
				for (iRow = 0; iRow < (I32)Rect.h; iRow++) {
					iCol = (I32) Rect.w;
					while (iCol) { /* loop unrolled 4-way */
						iCol-=4;
						iPErr = *pi16TargDat++ - *pi16RefDat++;
						iErrSum += iPErr * iPErr;
						iPErr = *pi16TargDat++ - *pi16RefDat++;
						iErrSum += iPErr * iPErr;
						iPErr = *pi16TargDat++ - *pi16RefDat++;
						iErrSum += iPErr * iPErr;
						iPErr = *pi16TargDat++ - *pi16RefDat++;
						iErrSum += iPErr * iPErr;
					}
					pi16RefDat += iDimRef;
					pi16TargDat += iDimTarg;
				}
			} /* end if bMad */
		}
	} else {  /* interpolation is necessary */
		/* Variables for intermediate calculations */
		I32 ia, ic, iac, ib, ibd, id;
		I32 iRnd = 0;	/* Rounding could be done but isn't currently */

		if (bMad) {
			for (iRow = Rect.r; iRow < (I32)(Rect.r + Rect.h); iRow++) {
				ia = *pi16RefDat;
				ic = *(pi16RefDat + pcRef->Pitch);
				iac = (ia << 8) + iFracR * (ic - ia);
				iCol =(I32) Rect.w;
				while (iCol--) {
					pi16RefDat++;
					ib = *pi16RefDat;
					id = *(pi16RefDat + pcRef->Pitch);
					ibd = (ib << 8) + iFracR * (id - ib);
					iPErr  = (iac << 8) + iFracC * (ibd - iac) + iRnd;
					iPErr = *pi16TargDat++ - (iPErr >> 16);
					iErrSum += ABS(iPErr);
					iac = ibd;
				}
				pi16RefDat += iDimRef;
				pi16TargDat += iDimTarg;
			}
		} else { /* Not bMad */
			for (iRow = Rect.r; iRow < (I32)(Rect.r + Rect.h); iRow++) {
				ia = *pi16RefDat;
				ic = *(pi16RefDat + pcRef->Pitch);
				iac = (ia << 8) + iFracR * (ic - ia);
				iCol = (I32)Rect.w;
				while (iCol--) {
					pi16RefDat++;
					ib = *pi16RefDat;
					id = *(pi16RefDat + pcRef->Pitch);
					ibd = (ib << 8) + iFracR * (id - ib);
					iPErr  = (iac << 8) + iFracC * (ibd - iac) + iRnd;
					iPErr = *pi16TargDat++ - (iPErr >> 16);
					iErrSum += iPErr * iPErr;
					iac = ibd;
				}
				pi16RefDat += iDimRef;
				pi16TargDat += iDimTarg;
			} /* end for */
		} /* End if bMad.. else... */
	} /* end if interpolate... else... */

	dErr = (Dbl) iErrSum / ((I32)Rect.h * (I32)Rect.w);

	return dErr;
}

/***********************************************************************
*
*  EncMeRectErrFast
*
*   Compute the MAD for the the Rect specified.  
*	pcRef and pcmTarg Reference and Target areas to compute the error
*	for; Rect specifies the rectangle within the Target to compute;
*   Vect specifies the vector to translate Rect by 
*	when looking in pcRef.  That is, the actual rectangles compared are
* 	Rect within pcmTarg, and Rect shifted by Vect within pcRef.
*   iMaxErr is the maximum Error Value that will be calculated,  If the 
*   Error exceeds this value at any point, the calculation ends.  This
*   saves time during motion search.
*
*	The value returned is the I32 representation of the MAD.
***********************************************************************/
I32
EncMeRectErrFast(PCMatrixSt pcRef,
			     PCMatrixSt pcmTarg,
			     RectSt		Rect,
			     I32 		iMaxErr,		
			     McVectorSt	Vect)
{

	I32 iRow, iCol;			/* Row and Column indices */
	I32	iPErr;				/* Used for pixel error calculation */
	I32 iFracR, iFracC;		/* Fractional part of the motion vector */
	I32 iPelsR, iPelsC;		/* Motion vector in pixels */
	I32 iErrSum = 0;		/* Sum of error values */
	I32 iDimRef, iDimTarg;	/* Pitches used for pointer increments */
	PI16 pi16RefDat, pi16TargDat;	/* Pointer to the image data */

	McModRes(Vect.r, &iPelsR, &iFracR);
	McModRes(Vect.c, &iPelsC, &iFracC);
	iFracR *= 256 / MC_UNIT;
	iFracC *= 256 / MC_UNIT;

	pi16RefDat = pcRef->pi16 + (Rect.r + iPelsR) * pcRef->Pitch + Rect.c + iPelsC;
	pi16TargDat = pcmTarg->pi16 + Rect.r * pcmTarg->Pitch + Rect.c;
	iDimRef = pcRef->Pitch - Rect.w;
	iDimTarg = pcmTarg->Pitch - Rect.w;

	if (iFracR == 0 && iFracC == 0) {  /* Do fast if interp. not necessary */
		if (Rect.w % 4) { /* only a partial block -- can't unroll loop */
			for (iRow = 0; iRow < (I32)Rect.h; iRow++) {
				for (iCol = 0; iCol < (I32)Rect.w; iCol++) {
					iPErr = *pi16TargDat++ - *pi16RefDat++;
					iErrSum += ABS(iPErr);
				}
				pi16RefDat += iDimRef;
				pi16TargDat += iDimTarg;
				if (iErrSum > iMaxErr)	/* Early Out if iMaxErr Exceeded */
					break;
			}
		} else { /* Able to unroll loop */
			for (iRow = 0; iRow < (I32)Rect.h; iRow++) {
				iCol = (I32)Rect.w;
				while (iCol) { /* loop unrolled 4-way */
					iCol-=4;
					iPErr = *pi16TargDat++ - *pi16RefDat++;
					iErrSum += ABS(iPErr);
					iPErr = *pi16TargDat++ - *pi16RefDat++;
					iErrSum += ABS(iPErr);
					iPErr = *pi16TargDat++ - *pi16RefDat++;
					iErrSum += ABS(iPErr);
					iPErr = *pi16TargDat++ - *pi16RefDat++;
					iErrSum += ABS(iPErr);
				}
				pi16RefDat += iDimRef;
				pi16TargDat += iDimTarg;
				if (iErrSum > iMaxErr)	/* Early Out if iMaxErr Exceeded */
					break;
			}
		}
	} else {  /* interpolation is necessary */
		/* Variables for intermediate calculations */
		I32 ia, ic, iac, ib, ibd, id;
		for (iRow = Rect.r; iRow < (I32)(Rect.r + Rect.h); iRow++) {
			ia = *pi16RefDat;
			ic = *(pi16RefDat + pcRef->Pitch);
			iac = (ia << 8) + iFracR * (ic - ia);
			iCol = (I32) Rect.w;
			while (iCol--) {
				pi16RefDat++;
				ib = *pi16RefDat;
				id = *(pi16RefDat + pcRef->Pitch);
				ibd = (ib << 8) + iFracR * (id - ib);
				iPErr  = (iac << 8) + iFracC * (ibd - iac);
				iPErr = *pi16TargDat++ - (iPErr >> 16);
				iErrSum += ABS(iPErr);
				iac = ibd;
			}
			pi16RefDat += iDimRef;
			pi16TargDat += iDimTarg;
			if (iErrSum > iMaxErr)	/* Early Out if iMaxErr Exceeded */
				break;
		}
	} /* end if interpolate... else... */

	return iErrSum;
}

/***********************************************************************
	   PRIVATE FUNCTIONS
***********************************************************************/

/*
 * MeDispCalc
 *
 * Searches the REF image for the best representation of CELL within
 * the CUR image, and returns the corresponding DISPL. To allow 
 * for the finite size of images, only that part of the cell within
 * REF is used in the calculation.  If it gives up, it sets DISPL=0.
 *
 * returns error of best match
 *
 */

static Dbl
MeDispCalc(
	PCEncMeCntxSt 	pcMeContext,
	RectSt     		rCell,
	PMcVectorSt 	pvDispl,
	jmp_buf jbEnv)
{
	Dbl ret;    /* return code. error of best match */
	
	PIA_Boolean ok = TRUE;   /* TRUE iff search is "successful" */
	RectSt rImBin;      /* rImBin describes entire pic */
	I32 iRss = pcMeContext->Resolution; /* final displacement res */
	I32 iDelta;
	I32 iNewMaxDelta;
	I32 iBestErr = 1 << 30 ;
	McVectorSt		mvCandidate;

	rImBin.r = rImBin.c = 0;
	rImBin.h = pcMeContext->pTarg->NumRows;
	rImBin.w = pcMeContext->pTarg->NumCols;

	/* insure iDelta a mult of iRss */
	iDelta = MAX(pcMeContext->InitStep / iRss, 1) * iRss;	

	if (pcMeContext->Tactic & ME_GRAD) {
		while (iDelta >= iRss) {
			ok = MeStep(pcMeContext, rCell, pvDispl, iDelta);
			iDelta /= pcMeContext->StepRatio;
			iDelta = (iDelta / iRss) * iRss;   /* ensure still a mult of iRss */
			iNewMaxDelta = pcMeContext->MaxDelta;  /* Keep it the same */
		}
	} else {
		iNewMaxDelta = pcMeContext->MaxDelta;
		while (iDelta >= iRss) {
			MeFull(pcMeContext, iNewMaxDelta, rCell, pvDispl, iDelta, &iBestErr);
			iDelta /= pcMeContext->StepRatio;
			iDelta = (iDelta / iRss) * iRss;	/* ensure still a mult of iRss */
			iNewMaxDelta = iDelta;	/* stay within prev grid */
		}
		ret = (Dbl)iBestErr/(rCell.w*rCell.h);
	}

	mvCandidate = *pvDispl;
	/* ensure encodable by the MB Huffman table :
	 * Out of 8 macro block Huffman tables, (see mbhuftbl.h). 7 of them have only 93
	 * codewords. So the valid differential MV coding range is [-46,46], which leads to 
	 * the safe range of [-23,23] (in pixel) for individual motion vector.
	 * The actual range of pvDispl (in unit of 1/16 pixel) then depends upon the 
	 * resolution iRss.
	*/

	pvDispl->r = MAX(pvDispl->r, -23 * iRss );
	pvDispl->r = MIN(pvDispl->r,  23 * iRss );
	pvDispl->c = MAX(pvDispl->c, -23 * iRss );
	pvDispl->c = MIN(pvDispl->c,  23 * iRss );

	/* Clip if necessary.  This should only be needed for the step search */
	if (pcMeContext->Tactic & ME_GRAD) {
		if (ok) {
			MeSafeBin(&rCell, &rImBin, pvDispl, 0);
		}
		MeClipDispl(rImBin, rCell, pvDispl, jbEnv);
		ret = (Dbl)error(iNewMaxDelta, pcMeContext, rCell, pvDispl->c, pvDispl->r) / 
			(rCell.w*rCell.h);
	}  else { /* FULL search */
		if ((mvCandidate.r != pvDispl->r) || (mvCandidate.c != pvDispl->c))	{
			/* recalculate the error only if the pvDispl is changed due to the MAX and MIN
			 * operation above.
			 */
			ret = (Dbl)error(iNewMaxDelta, pcMeContext, rCell, pvDispl->c, pvDispl->r) / 
				(rCell.w*rCell.h);
		}
	}

	return ret;   
}                  


/*
 * MeFull
 *
 * returns: The error for the best vector 
 *
 */
static void
MeFull(
	PCEncMeCntxSt	pcMeContext,/* Motion estimation parameters */
	I32 	iNewMaxDelta,
	RectSt		rMBRect,	/* Macro block rect */
	PMcVectorSt	pvLast,		/* Last vector to start from */
	I32			iDelta,		/* Log search step */
	PI32 piBestErr)			/* the minimum error with the best MV known so far */
{
	I32 iMsRLow, iMsRHigh;	/* Row motion search range  */
	I32 iMsCLow, iMsCHigh;  /* Column motion search range */
	I32 iRow, iCol;			/* Row and column indices */
	I32 iErr, iBestErr = *piBestErr;  /* Errors */
	McVectorSt vBest=*pvLast, vCurr;	/* Best and current ME vectors */
	I32 iErrThresh;

	/* Figure out the Integer Good Enough Thresh */

	iErrThresh = (I32) (pcMeContext->ErrThresh * (Dbl)(rMBRect.h * rMBRect.w));

	/* Set the search range for the Row */
	iMsRLow = MAX(pvLast->r - iNewMaxDelta, - ((I32)rMBRect.r << MC_RES));
	iMsRHigh = MIN(pvLast->r + iNewMaxDelta,
		(I32)(pcMeContext->pRef->NumRows - (rMBRect.r + rMBRect.h)) << MC_RES);


	/* Set the search range for the Column */
	iMsCLow = MAX(pvLast->c - iNewMaxDelta, -((I32)rMBRect.c << MC_RES));
	iMsCHigh = MIN(pvLast->c + iNewMaxDelta,
		(I32)(pcMeContext->pRef->NumCols - (rMBRect.c + rMBRect.w) ) << MC_RES);

	/* Loop through the range finding the vector with the minimum error */
	for (iRow = iMsRLow; iRow <= iMsRHigh; iRow += iDelta)  {
		vCurr.r = iRow;
		for (iCol = iMsCLow; iCol <= iMsCHigh; iCol += iDelta) {
			vCurr.c = iCol;
			iErr = EncMeRectErrFast(pcMeContext->pRef, pcMeContext->pTarg, rMBRect,
								    iBestErr, vCurr);

			/* This error is less than the best so far */
			if (iBestErr > iErr) {
				iBestErr = iErr;
				vBest = vCurr;

				/* Thresh hold to exit if the error is good enough */
				if (iBestErr < iErrThresh) break;
			}
		} 
	}

	/* pvLast vector is set to the best vector */
	*pvLast = vBest;
	*piBestErr = iBestErr;

}
                     
/*
 * MeStep
 *
 * Note that the parameters of search are common to several routines
 * here. Where the association is obvious, the explanation will not be 
 * repeated.
 *
 * returns: 0 if not found, else 1
 *
 */
static I32
MeStep(
	PCEncMeCntxSt 	pcMeContext,	/* details of search */
	RectSt        	rCell,		/* rectangular area to match */
	PMcVectorSt 	pvDispl,		/* starting/final displacement vector */
	I32             iDelta)		/* initial step size (1/16 pel) */
{
	I32 iRet;
	I32 iY = (I32)pvDispl->r;
	I32 iX = (I32)pvDispl->c;
	I32 e0, ex = 0, ey = 0, exy, iDx=0, iDy=0, iMinArea;
	I32 t, iMinXY, iEBest, iENextBest, iSign, iStep, iMargin;
	I32 iDone = 0;
	RectSt rImBin;
	
	iMargin = (iDelta + MC_UNIT - 1) / MC_UNIT;
	iMinArea = (rCell.h - 1) * (rCell.w - 1);
	rImBin.r = rImBin.c = 0;
	rImBin.h = pcMeContext->pTarg->NumRows;
	rImBin.w = pcMeContext->pTarg->NumCols;

	while (!iDone) {
		pvDispl->r = (I32)iY; pvDispl->c = (I32)iX;
		MeSafeBin(&rCell, &rImBin, pvDispl, iMargin);
			/* shrink rCell, if nec, to stay in image */
		if ( (I32)(rCell.h * rCell.w) < iMinArea) {
			iRet = 0;       /* give up */
			goto bail;
		}

		e0 = error(pcMeContext->MaxDelta, pcMeContext, rCell, iX, iY);
		if (e0 < ErrNorm * pcMeContext->ErrThresh)
			break;

		if (iDx == 0)
			ex = error(pcMeContext->MaxDelta, pcMeContext, rCell, iX+(iDx=iDelta), iY);
		if (ex >= e0 || TRUE) {	/* no shortcuts */
			t = error(pcMeContext->MaxDelta, pcMeContext, rCell, iX-iDx, iY);
			if (t < ex) {
				ex = t;
				iDx = -iDx;
			}
		}
		if (iDy == 0)
			ey = error(pcMeContext->MaxDelta, pcMeContext, rCell, iX, iY+(iDy=iDelta));
		if (ey >= e0 || TRUE) {	/* no shortcuts */
			t = error(pcMeContext->MaxDelta, pcMeContext, rCell, iX, iY-iDy);
			if (t < ey) {
				ey = t;
				iDy = -iDy;
			}
		}
		exy = error(pcMeContext->MaxDelta, pcMeContext, rCell, iX+iDx, iY+iDy);
		iMinXY = (e0 < exy) ? e0 : exy; t = (ey < ex) ? ey : ex;
		if (iMinXY > t)
			iMinXY = t;

		if (e0 == iMinXY)
			break;
		else if (ex == iMinXY) {
			iStep = lin_search(pcMeContext, rCell, iX, iY, iDx, 0, e0,
			                   iMinXY, &iEBest, &iENextBest, &iSign);
			e0 = iEBest;
			ex = iENextBest;
			iX += iStep * iDx;
			iDx *= iSign;
			if (iStep == 1)
				ey = exy;
			else
				iDy = 0;
		} else if (ey == iMinXY) {
			iStep = lin_search(pcMeContext, rCell, iX, iY, 0, iDy, e0,
			                   iMinXY, &iEBest, &iENextBest, &iSign);
			e0 = iEBest;
			ey = iENextBest;
			iY += iStep * iDy; 
			iDy *= iSign;
			if (iStep == 1)
				ex = exy;
			else
				iDx = 0;
		} else 	{   /* exy = iMinXY */
			iStep = lin_search(pcMeContext, rCell, iX, iY, iDx, iDy, e0,
			iMinXY, &iEBest, &iENextBest, &iSign);
			e0 = iEBest;
			iX += iStep * iDx; iY += iStep * iDy;
			if (iStep == 1) {
				t = ex; ex = ey; ey = t;
				iDx = -iDx;
				iDy = -iDy;
			} else
				iDx = iDy = 0;
		}
	}  

	pvDispl->r = (I32)iY ;
	pvDispl->c = (I32)iX;
	iRet = 1;
bail:
	return iRet;
}

/*
 * lin_search
 *
 * Perform a linear search along the vector specified by iX, iY and iDx, iDy.
 *
 * Returns the number of iterations along this vector (in iDx, iDy increments)
 * to the spot with the least error.
 * piBest is filled with the error at this location; piNextBest with the error
 * of the following iteration.  piSign is 1 if iterating stopped because the 
 * vector was outside of the rectangle being checked (in which case Best is just
 * the last value), and -1 if it determined a "best" value before that point.
 */
static I32
lin_search(
	PCEncMeCntxSt pcMeContext,
	RectSt     	 rCell,
	I32          iX,
	I32          iY,
	I32          iDx,
	I32          iDy,
	I32          iPrev,
	I32          iStart,
	PI32         piBest,
	PI32         piNextBest,
	PI32         piSign)
{
	I32 iStep = 1;
	McVectorSt vt;
	I32 e = iStart, iNext = 2000000000;
	I32 iDone = 0;
	
	iX += iDx; iY += iDy;

	while (!iDone) {
		vt.r = (I32)(iY + iDy); 
		vt.c = (I32)(iX + iDx);
		
		if (!McVectOk(pcMeContext->pTarg, rCell, vt, 0))
			break;
		iNext = error(pcMeContext->MaxDelta, pcMeContext, rCell, iX + iDx, iY + iDy);

		if (iNext >= e)
			break;
		iStep++;
		iPrev = e; 
		e = iNext;
		iNext = 2000000000;
		
		iX += iDx; 
		iY += iDy;
	}

	if (iPrev < iNext) {
		iNext = iPrev;
		*piSign = -1;
	} else
		*piSign = 1;

	*piNextBest = iNext;
	*piBest = e;
	return(iStep);
}

/*
 * error
 *
 * Returns the error between rCell in pcMeContext->pTarg and rCell 
 * translated by iX,iY in pcMeContext->pRef.  The error is either 
 * the MSE or the MAd, as specified in pcMeContext->Measure.
 *
 * If the vectors are larger than the maximum motion vector allowed, then
 * a ridiculously large value of 2000000000 is returned instead.
 */
static I32
error(
	I32 MaxDelta,
	PCEncMeCntxSt pcMeContext,
	RectSt rCell,
	I32 iX,
	I32 iY)
{
	I32 iRet;
	Dbl derr;
	McVectorSt v;
   
	if (ABS(iX) >= MaxDelta || ABS(iY) >= MaxDelta) {
		iRet = (2000000000);
	} else {
		v.r = (I32) iY;
		v.c = (I32) iX;
		derr = EncMeRectErr(pcMeContext->pRef, pcMeContext->pTarg, rCell, 
							pcMeContext->Measure, v);
		iRet = (I32) (derr * ErrNorm + 0.4999);  
	}
	return iRet;  /* use I32 for convenience */
}

/***********************************************************************
*
*   MeSafeBin
*
*   Find part of prBin that is comfortably in prImBin and update prBin
*	with this safe rectangle.
*
***********************************************************************/
static void
MeSafeBin(
	PRectSt prBin,
	PRectSt prImBin,
	PMcVectorSt pvDispl,
	I32 iMargin)
{
	I32 iMargR, iMargC, dr, dc, dh, dw;

	/* the following will quickly clear most cells far from the border*/
	dr = prBin->r - prImBin->r ;            /* row diff */
	dc = prBin->c - prImBin->c ;            /* col diff */
	dh = prImBin->h - prBin->h ;            /* height diff */
	dw = prImBin->w - prBin->w ;            /* width diff */
	iMargR = iMargin + (ABS(pvDispl->r) >> MC_RES) + 1 ; /*conservative*/
	iMargC = iMargin + (ABS(pvDispl->c) >> MC_RES) + 1 ;
	if (!(dr > iMargR && dh - dr > iMargR) ||
		!(dc > iMargC && dw - dc > iMargC)) {

		MeSafeBin1d((I32*)&prBin->r, (I32*)&prBin->h, (I32)pvDispl->r,
			(I32)iMargin, (I32)prImBin->r, (I32)prImBin->h);
		MeSafeBin1d((I32*)&prBin->c, (I32*)&prBin->w, (I32) pvDispl->c,
			(I32)iMargin, (I32)prImBin->c, (I32)prImBin->w);
	}
}


/***********************************************************************
*
*   MeSafeBin1d
*
*	iDispl  is one dimension of a displacement vector
*	iMargin is the amount of margin to allow on each side
*	iLoLim is the start of this dimension; iHiLim is the length of it.
*
*	Adjusts piLow and piLen so that the dimension they specify is within
*	that specified by iLoLim and iHiLim, modified by iDispl, with iMargin
*	of space on either side.
*
***********************************************************************/
static void
MeSafeBin1d(
	PI32 piLow,
	PI32 piLen,
	I32 iDispl,
	I32 iMargin,
	I32 iLoLim,
	I32 iHiLim)
{
	I32 iLowPnt, iHiPnt;
	I32 iLow = *piLow;
	I32 iLen = *piLen;
	I32 iDist;

	iLowPnt = iLow + (iDispl >> MC_RES) ;    /* low end of rCell */
	iDist = iLoLim + iMargin - iLowPnt ;        	/* dist to clip */ 
	if (iDist > 0) {
		*piLow += iDist;
		*piLen -= iDist;
	}
	iHiPnt = iLow + iLen - 1 + ((iDispl + MC_UNIT - 1) >> MC_RES) ;
	iDist = iMargin - (iHiLim - 1 - iHiPnt) ;   	/* dist to clip */
	if (iDist > 0) {
		*piLen -= iDist ;
	}
	if (*piLen < 0) 
		*piLen = 0 ;
}


/***********************************************************************
*
*   MeClipDispl
*
*   Make minimum change to displ so that rCell is fully within imcell.
*
*   Returns true iff displ is clipped.
*
*
***********************************************************************/
static PIA_Boolean
MeClipDispl(
	RectSt rImCell,
	RectSt rCell,
	PMcVectorSt pvDispl,
	jmp_buf jbEnv)
{
	PIA_Boolean  bRet;
	I32 iMinR, iMinc, iMaxR, iMaxC;
	McVectorSt vDispl, vDisplClip;
	MatrixSt imMatrix;

	iMinR = (rImCell.r - rCell.r) * MC_UNIT;       /* <= 0 */
	iMinc = (rImCell.c - rCell.c) * MC_UNIT;
	iMaxR = (rImCell.r + rImCell.h - (rCell.r + rCell.h))*MC_UNIT;
	iMaxC = (rImCell.c + rImCell.w - (rCell.c + rCell.w))*MC_UNIT;
	vDisplClip = vDispl = *pvDispl ;
	vDisplClip.r = MAX(vDisplClip.r, iMinR) ;
	vDisplClip.r = MIN(vDisplClip.r, iMaxR) ;
	vDisplClip.c = MAX(vDisplClip.c, iMinc) ;
	vDisplClip.c = MIN(vDisplClip.c, iMaxC) ;
	imMatrix.NumRows = rImCell.h;        /* only need Rows and Cols */
	imMatrix.NumCols = rImCell.w;
	if ( ! McVectOk(&imMatrix, rCell, vDisplClip, 0))
		longjmp(jbEnv,  (ENMESRCH << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_BAD_VECT << TYPE_OFFSET));
	*pvDispl = vDisplClip;
	bRet = vDisplClip.r != vDispl.r || vDisplClip.c != vDispl.c ;

	return(bRet);
}                 

