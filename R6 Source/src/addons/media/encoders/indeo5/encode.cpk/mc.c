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

/***********************************************************************

	mc.c

	prototypes for all public routines in mc.h
	prototypes for all private routines are in this file

 DESCRIPTION:
	Motion Compensation, various interpolation schemes.

	Routines:                      
		McRectInterp()        
		McVectOk()           
		static LinTerp()  

	Data:
		McVectorSt NullVect = {0, 0}

***********************************************************************/

#include <setjmp.h>

#include "datatype.h"
#include "matrix.h"
#include "mc.h"
#include "errhand.h"

static const McVectorSt NullVect = {0, 0};

/***********************************************************************
		PRIVATE ROUTINE PROTOTYPES
***********************************************************************/
static PIA_Boolean McVectOk1D(
	I32 iSize, I32 iStart, I32 iLength, I32 iVect, I32 iMargin);

static void LinTerp(MatrixSt, MatrixSt, RectSt, McVectorSt, I32);

/***********************************************************************
		PUBLIC ROUTINES
***********************************************************************/
/***********************************************************************
*
*   McRectInterp
*
*   Returns nothing
*
***********************************************************************/

void
McRectInterp(
	MatrixSt mRef,    /* reference pic */
	MatrixSt mComp,   /* Compensated picture */
	RectSt rRect,     /* rectangle of interest */
	McVectorSt Vector,   /* displacement vector */
	jmp_buf jbEnv)
{
	I32 r, c;            /* row, col indices */

	if ( ! McVectOk(&mComp, rRect, NullVect, 0))
		longjmp(jbEnv,  (MATRIX << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_NULL_PARM << TYPE_OFFSET));
	if ( ! McVectOk(&mRef, rRect, Vector, 0))
		longjmp(jbEnv,  (MATRIX << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_NULL_PARM << TYPE_OFFSET));

	if (Vector.r & MC_MASK || Vector.c & MC_MASK) {    /* must interp */
		LinTerp(mRef, mComp, rRect, Vector, 0);
	} else {		                      /* just copy displaced rect */
		r = Vector.r >> MC_RES;
		c = Vector.c >> MC_RES;
		mRef.pi16 = mRef.pi16 + r * (I32)mRef.Pitch + c ;
		MatCopy(mRef, mComp, rRect, jbEnv);
	}
}


/***********************************************************************
*
*   McVectOk
*
*   Returns TRUE iff rect is predicted from within iMargin pels of border
*
***********************************************************************/
PIA_Boolean
McVectOk(
	PCMatrixSt pIn,
	RectSt cell,
	McVectorSt d,
	I32 iMargin)
{
	return McVectOk1D(pIn->NumRows, cell.r, cell.h, d.r, iMargin) &&
	       McVectOk1D(pIn->NumCols, cell.c, cell.w, d.c, iMargin);
}

/***********************************************************************
		PRIVATE ROUTINES
***********************************************************************/
/* McVectOk1D	- ck if a 1-d cell is in a 1-d pic
*/
static PIA_Boolean McVectOk1D(
	I32 iSize,		/* size of pic */
	I32 iStart,		/* start of 1-d cell */
	I32 iLength,		/* length of 1-d cell */
	I32 iVect,		/* displacement vector */
	I32 iMargin)
{
	I32 iVLow, iVHi;	/* limits of iVect */

	/* convert iVect to integer, and round algebraically */
	iVLow = iVect >> MC_RES;				/* round down */
	iVHi = (iVect + MC_UNIT - 1) >> MC_RES;	/* round up */

	return iStart + iVLow >= iMargin &&
	       iStart + iLength + iVHi <= iSize - iMargin;
}

/*
 *   LinTerp
 *
 * exact linear interpolator re: DLS specs
 * Compatible with MPEG when rnd=0. [not actually tested]
 *
 * Assume that the rect and vector has been tested against the image dimension
 * by McVectOk().
 *
 *   Returns nothing
 */
static void
LinTerp(
	MatrixSt mRef, 	/* Reference image */
	MatrixSt mComp,	/* Compensated values go here */
	RectSt rRect,		/* Rect of the corresponding macro block */
	McVectorSt vVect,	/* Motion vector */
	I32 iRnd)			/* A possible round value */
{
	U32 uRow, uCol;			/* Row and Column indices */
	I32 iFracR, iFracC;		/* Fractional part of the motion vector */
	I32 iPelsR, iPelsC;		/* Motion vector in pixels */
	I32 iDimRef, iDimComp;	/* Pitches used for pointer increments */
	I32 iCompPix;			/* Compensated value for the pixel */
	I32 ia, ib, ic, id, iac, ibd;  /* Used for interpolation calculation */
	PI16 pi16RefDat, pi16CompDat;	/* Pointer to the image data */

	/* Seperate the vector into integral and fractional parts */
	McModRes(vVect.r, &iPelsR, &iFracR);
	McModRes(vVect.c, &iPelsC, &iFracC);

	iFracR *= 256 / MC_UNIT; /* Fractional part of the vector */
	iFracC *= 256 / MC_UNIT; /* Fractional part of the vector */

	/* Set the initial pointers to their correct memory locations */
	pi16RefDat = mRef.pi16 + (rRect.r + iPelsR) * mRef.Pitch + rRect.c + iPelsC;
	pi16CompDat = mComp.pi16 + rRect.r * mComp.Pitch + rRect.c ;

	/* Set to the pitches minus the width of the rect for easy skip to the
	   next logical row in the matrix.
	*/
	iDimRef = mRef.Pitch - rRect.w;
	iDimComp = mComp.Pitch - rRect.w;

	for (uRow = rRect.r; uRow < rRect.r + rRect.h; uRow++) {

		/* Set the vertical interpolation value for pixels ac */
		ia = *pi16RefDat ;

		/* The following if... else ... is necessary to avoid going over the valid
		 * memory boundary. When iFracR==0, the testing done by McVectOk() assumes that
		 * no pixel c is needed for interpolation.
		 */
		if (iFracR==0) {
			iac = (ia << 8);
		} else { 
			ic = *(pi16RefDat + mRef.Pitch) ;
			iac = (ia << 8) + iFracR * (ic - ia) ;
		}
			
		uCol = rRect.w ;
		while (uCol--) {

			/* Set the vertical interpolation value for pixels bd */
			pi16RefDat++ ;
			ib = *pi16RefDat ;

			if (iFracR==0) {
				ibd = (ib << 8) ;
			}  else {
				id = *(pi16RefDat + mRef.Pitch) ;
				ibd = (ib << 8) + iFracR * (id - ib) ;
			}

			/* Do the horizontal interpolation from ac to bd */
			iCompPix = (iac << 8) + iFracC * (ibd - iac) + iRnd ;

			/* Shift it back to the right units */
			*pi16CompDat++ = (I16)(iCompPix >> 16) ;

			/* bd becomes the new ac for the next calculation */
			iac = ibd ;
		}

		/* Skip to the next logical row in the matrix */
		pi16RefDat  += iDimRef;
		pi16CompDat += iDimComp;
	}
}



