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
 * prefilt.c
 *
 * Routines for pre filtering of the image.  
 *
 * Functions:
 *	SpatPreFilter			Perform the spatial prefiltering
 *  TempPreFilter			Do a temporal pre-filtering
 */



#include <setjmp.h>
#include <math.h>

#include "datatype.h"
#include "prefilt.h"
#include "const.h"

/*************************************************************************
                PUBLIC ROUTINES
*************************************************************************/

#ifdef SIMULATOR /* The spatial prefilter is under simulator build only */

/*************************************************************************
*
*   SpatPreFilter
*
*	Prefilters the image using an adaptive unsharp masking filter.  This
*	filter trades off low pass versus enhancement on a pixel by pixel
*   basis.	If the pixel is in a smooth part of the image then it low
*   pass filters the pixel.  If it is an edge or detailed part of the image
*   then there may be some enhancement depending on the amount of the detail
*   or how sharp the edge is.
*
*	The idea behind the adaptive unsharp masking filter is that you first
*	low pass the pixel u(m,n) creating pixel v(m,n) and create an error 
*	between the two g(m,n) = u(m,n) - v(m,n).  The new filtered pixel is
*	u'(m,n) = u(m,n) + a * g(m,n).  Where if a > 0 then the pixel is enhanced
*   and if  -1.0 <= a < 0 then it is low pass filtered.  The algorithm adapts
*	a depending on the standard deviation of the 8 surrounding pixels. u(m,n)
*	is not included in that calculation so that if the sequnce has speckled
*	noise then the value of the noise will not effect the value for a.
*
*	The value of a is mapped using a piecewise linear function of the standard
*	deviation of the 8 surrounding pixels.  What the parameters are to this
*	function depends on the quantization.  If Q increases above a certain
*	value then the amount of enhancement is reduced and possibly the amount of
*	low pass filtering is increased.  This is necessary because enhancement 
*	can increase the bitrate at a given Q and if Q is rising too high then
*	enhancement must be turned off in order to make sure the algorithm can
*	make rate and to get the right trade-off between sharpness and blockiness.
*	
*
*   Returns nothing
*
*   prototype in prefilt.h
*
*************************************************************************/
void SpatPreFilter(
	MatrixSt mat,	/* image plane on which filtering is to be carried out */
	I32 iQuant,		/* Current quantization level */
	I32 iQuantMin,	/* Minimum quantization level */
	I32 iQuantMax)	/* Maximum quantization level */
{
	I32 i, j, g, c;		/* Various indices */
	I32 iNeighborSum;	/* Sum of the 8 pixel neighbors */
	I32 iNeighborSumSq;	/* Sum squared of the 8 pixel neighbors */
	PI16 pi16Out;		/* Pointer to the output image */
	PI16 pi16PrevRow, pi16CurRow, pi16NextRow;	/* Pointers to the rows */
	PI16 pi16PrevCol, pi16CurCol, pi16NextCol; /* Pointers to the pixels */
	const I16 ai16Filt[3][3] = { { -1, -1, -1},	/* Low pass filter used */
							   { -1,  8, -1},
							   { -1, -1, -1} };
	const I32 iShift = 3;		/* Used to divide by 8 */
	const I32 iCenterCoef = 8;
	const I32 iMaxVal = MaxClampIn-128;	/* Maximum value of a pixel = 240-128 */
	const I32 iMinVal = MinClampIn-128;	/* Minimum value of a pixel = 16-128 */

	const Sngl sHighLambda = (Sngl) 70.0;	/* Upper target for the pixel STD */
	const Sngl sMidLambda = (Sngl) 16.0;		/* Mid target for the pixel STD */
	const Sngl sLowLambda = (Sngl) 1.0;		/* Low target for the pixel STD */

	const Sngl sNoImgEnQFact = (Sngl) 0.6;	/* % of Max Q where no enhancement is done */
	const Sngl sNormImgEnQFact = (Sngl) 0.2;	/* % of Max Q where the amount
												   of enhancement starts to
												   decrease. */

	const Sngl sOutLow = (Sngl) -0.3;	/* Amount of the low pass value	added
										   in for the lowest value of Lambda */
	const Sngl sOutMid = (Sngl) 0.0;		/* Mid value for Lambda */
	Sngl sOutHigh = (Sngl) 0.6;			/* High value for Lambda */
	const Sngl sNormalOutHigh = (Sngl) 0.6;	/*  sOutHigh for the normal range */
	const Sngl sLowOutHigh = (Sngl) 0.0;	/*  sOutHigh for the High Q range */

	Sngl sLambda = (Sngl) 1.0;	/* Low pass add in factor */
	Sngl sLambdaLowSlope;		/* Slope to adjust sLambda based on the STD of
								   the 8 neighboring pixels for the STD less
								   than sMidLambda. */
	Sngl sLambdaHighSlope;		/* Slope to adjust sLambda for the High STD
								   range */
	Sngl sLambdaLowOffset;		/* Offset to the linear adjustment of sLambda */
	Sngl sLambdaHighOffset;		/* Offset to the linear adjustment of sLambda */
	Sngl sMean;					/* Mean of surrounding pixels */
	Sngl sMeanSq;				/* Mean of squares of surrounding pixels */

	/*  sOutHigh adjusts the amount of enhancment done on the image.  If iQuant
		is lower than a percentage of iQuantMax then normal enhancemnt is done.
		If it is greater than a different percentage of iQuantMax then a small
		or no enhancment is done.  If it is in between the normal range and
		low range then the amount of enhancment is linearily decreased as
		iQuant increases.
	 */
	if(iQuant < (I32) (iQuantMax * sNormImgEnQFact)) {
		/* Normal enhancement range */
		sOutHigh = sNormalOutHigh;
	} else if(iQuant > (I32) ((Sngl) iQuantMax * sNoImgEnQFact)) {
		/* Low to no enhancemnt range */
		sOutHigh = sLowOutHigh;
	} else {
		/* Linearily decrease the amount of enhancement if iQuant is between
		   iQuantMax * sNormImgEnQFact and iQuantMax * sNormImgEnQFact.  This
		   equation is simply a linear equation of the form y = mx + b.
		 */
		sOutHigh = (Sngl) ((sLowOutHigh - sNormalOutHigh) /
			(iQuantMax * sNoImgEnQFact - iQuantMax * sNormImgEnQFact)) *
			(Sngl) iQuant + sNormalOutHigh -
			((Sngl) ((sLowOutHigh - sNormalOutHigh) /
			(iQuantMax * sNoImgEnQFact - iQuantMax * sNormImgEnQFact)) *
			(Sngl) (iQuantMax * sNormImgEnQFact));
	}

	/* The amount of enhancement done on a particular pixel is a piecewise 
	   linear function of the standard deviation of the 8 neighboring 
	   pixels.  There is a low range which actually low pass filters the
	   pixel and a high range which enhances the pixel.
	 */

	/* Slope and offset for the low range of sLambda */
	sLambdaLowSlope = (Sngl) (sOutMid - sOutLow) / (sMidLambda - sLowLambda);
	sLambdaLowOffset = -sMidLambda * sLambdaLowSlope;

	/* Slope and offset for the high range of sLambda */
	sLambdaHighSlope = (Sngl) (sOutHigh - sOutMid) / (sHighLambda - sMidLambda);
	sLambdaHighOffset = -sMidLambda * sLambdaHighSlope;

	/* Set up the row pointers */
	pi16PrevRow = mat.pi16;
	pi16CurRow = pi16PrevRow + mat.Pitch;
	pi16NextRow = pi16CurRow + mat.Pitch;
	for (i = 1; i < (I32)mat.NumRows - 1; i++) {
		/* Set the column pointers to the start of the rows */
		pi16CurCol = pi16CurRow;
		pi16PrevCol = pi16PrevRow;
		pi16NextCol = pi16NextRow;
		pi16Out = pi16CurCol + 1;
		for (j = 1; j < (I32)mat.NumCols - 1; j++, pi16CurCol++, pi16PrevCol++,
				pi16NextCol++, pi16Out++) {
			/* For each pixel find the low pass value and the standard
			   deviation of the neighboring pixels.
			 */

			/* g is for the calculation of the filtered value. */
			g = 0;
			iNeighborSum = iNeighborSumSq = 0;
			for(c = 0; c < 3; c++) {
				/* Find the square and filter contribution of the pixel
				   value at u(i - 1, j - 1 + c) */
				iNeighborSumSq += *(pi16PrevCol + c) * *(pi16PrevCol + c);
				g += ai16Filt[0][c] * *(pi16PrevCol + c);

				/* Find the square and filter contribution of the pixel
				   value at u(i, j - 1 + c) */
				iNeighborSumSq += *(pi16CurCol + c) * *(pi16CurCol + c);;
				g += ai16Filt[1][c] * *(pi16CurCol + c);

				/* Find the square and filter contribution of the pixel
				   value at u(i + 1, j - 1 + c) */
				iNeighborSumSq += *(pi16NextCol + c) * *(pi16NextCol + c);
				g += ai16Filt[2][c] * *(pi16NextCol + c);
			}

			/* The calculation of iNeighborSum assumes 
			   the filter { { -1, -1, -1},
							{ -1,  ?, -1},
						    { -1, -1, -1} };
			   If the filter values for the neighborhood pixels are not
			   -1 then iNeighborSum can not use g as it's base.
			 */

			/* Subract off the filter contribution of the middle pixel
			   and create an average by dividing by the number of neighboring
			   pixels.  Invert the sign since the filter has them all neg. 
			 */
			sMean = -((Sngl) (g - (*(pi16CurCol + 1) * ai16Filt[1][1])) /
				(Sngl) iCenterCoef);
			
			/* Divide the filter sum by the value of the center coefficient */
			g = g / iCenterCoef;

			/* Subtract of the contributions from the middle pixel and 
			   divide by the value of the center coefficient.
			 */
			sMeanSq = (Sngl) (iNeighborSumSq - (*(pi16CurCol + 1) *
				*(pi16CurCol + 1))) / (Sngl) iCenterCoef;

			/* Find the variance of the 8 neighboring pixels by taking
			   MeanSumSq - Mean^2.
			 */
			sLambda = (Sngl) (sMeanSq - (sMean * sMean));

			/* sLambda may be negative because of round off error by the integer
			   divide instead of a floating point divide by the center coefficient.
			 */
			if(sLambda < 0.0) {
				sLambda = (Sngl) 1.0;
			}

			/* Take the square root to get the standard deviation */
			sLambda = (Sngl) sqrt(sLambda);
	
			if(sLambda < sLowLambda) {
				/* Use the lowest Lambda */
				sLambda = sOutLow;
			} else if(sLambda <= sMidLambda) {
				/* Use the low linear function to get the desired sLambda */
				sLambda = (sLambdaLowSlope * sLambda) + sLambdaLowOffset;
			} else if(sLambda <= sHighLambda) {
				/* Use the High linear function to get the desired sLambda */
				sLambda = (sLambdaHighSlope * sLambda) + sLambdaHighOffset;
			} else {
				/* If the standard deviation is too high then just use the
				   highest.
				 */
				sLambda = sOutHigh;
			}

			/* The output pixel is the original pixel + sLambda * g where
			   sLambda adjusts the enhancement or filtering and g is the
			   error between the original pixel and the low pass of the
			   original pixel.
			 */
			*pi16Out = (I16)(*pi16Out + (I16) (sLambda * (Sngl) g));

			/* Make sure that the output value is in range. */
			if (*pi16Out < iMinVal) *pi16Out = (I16) iMinVal;
			else if (*pi16Out > iMaxVal) *pi16Out = (I16) iMaxVal;

		}

		/* Set the row pointers to point at the next row */
		pi16PrevRow = pi16CurRow;
		pi16CurRow = pi16NextRow;
		pi16NextRow += mat.Pitch;
    }
}    /* SpatPreFilter() */

#endif /* SIMULATOR */

/*************************************************************************
*
*   TempPreFilter
*
* 	Function to do image temporal prefiltering.
*
*   Returns nothing
*
*   prototype in prefilt.h
*
*************************************************************************/
void TempPreFilter(
	MatrixSt mPicOrig,	   /* image plane on which filtering is to be carried out */
	MatrixSt mLastPicOrig, /* Previous image plane */
	I32 iPlane)			   /* Image Plane */
{
	I32 i, j;		/* Various indices */
	PI16 pi16PrevRow, pi16CurRow;	/* Pointers to the start of the image rows */
	PI16 pi16PrevCol, pi16CurCol;	/* Pointers to column pixels */
	I16 i16CoefThresh;

	/* Threshold for deciding to filter */
	if (iPlane == 0) { /* Color Plane is Y */
		i16CoefThresh = 9;
	} else {  /* Color Plane is U or V */
		i16CoefThresh = 6;
	}

	/* Set the row pointers to the start of the data */
	pi16CurRow = mPicOrig.pi16;
	pi16PrevRow = mLastPicOrig.pi16;
	for (i = 0; i < (I32)mPicOrig.NumRows; i++) {
		/* Set the column pointers to the start of the rows */
		pi16CurCol = pi16CurRow;
		pi16PrevCol = pi16PrevRow;
		for (j = 0; j < (I32)mPicOrig.NumCols; j++, pi16CurCol++, pi16PrevCol++) {
			/* If the error between the pixel in this frame and the previous
			   is small then filter 50/50.
			 */
			if(ABS(*pi16CurCol - *pi16PrevCol) < i16CoefThresh) {
				/* A 50/50 filter */
				/* The 257 value restores the -128 bias for each pixel and
				   adds 1 as a rounding value.
				 */
				*pi16CurCol = ((*pi16CurCol + *pi16PrevCol + 257) / 2) - 128;
			}
		}

		/* Set the row pointers to point at the next row */
		pi16CurRow += mPicOrig.Pitch;
		pi16PrevRow += mLastPicOrig.Pitch;
    }
}    /* TempPreFilter() */


