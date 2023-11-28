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
*				Copyright (C) 1994-1997 Intel Corp.                       *
*						  All Rights Reserved.							*
*																		*
************************************************************************/

/*****************************************************************************
 *	decomp.c :
 *
 *    This file contains the specialized and optimized C routines for (5,3)
 *  wavelet decomposition . See recomp.c for the corresponding recomposition 
 *  function. Also See wavelet.c for generic wavelet implementation if the
 *  filter bank is not this particular (5,3) one.
 *
 *  It is recommended that you read the document i:\proj42\exp\sim\newwave.doc
 *  (esp. Section 4.1 ) before diving into the code itself.
 *
 *  The following is the description of this particular filter bank (1-D).
 *	The format is :
 *			{(filter tap number), (scaling factor),	(filter coefficients)} 
 *  
 *			 {  
 *			 	{5, 8, {-1,2,6,2,-1}}, 	    (H0)
 *				{3, 4, {1,-2,1} },			(H1)
 *				{3, 2, {1,2,1} },			(G0)
 *				{5, 4, {1,2,-6,2,1} }       (G1)
 *			 },
 *  Note that the scaling factors in decomposition and recomposition
 *  have to be matched such that the overall scaling for each band in 1-Dim
 *  is always 1/16, that is, the following equation must be satisfied:
 *  scaling(H0)*scaling(G0)=scaling(H1)*scaling(G1)=16.
 *  Similarly, in 2-Dim, the overall scaling in each band is always 1/256.
 *
 *  Functions:
 *  	OptWaveletDecomp()
 *******************************************************************************/

#include "datatype.h"
#include "decomp.h"

/* Optimized implementation for #2 wavelet decomposition use the following scaling factors.
 * This set of scaling factors is the set which keeps the typical dynamic 
 * range of	the output band data the same as the input raw data.
 * So this is intended to be used for the best implementation of new wavelet #2.
 */

const I16 i16DecompScale[4]={64,32,32,16};

/***********************************************************************************
 * 	OptWaveDecomp()
 *	
 *	   This function takes an 2-D image as input and applies (5,3) filter bank 
 *	   decomposition in 2-D. It generates 4 band subimages as its output. Each
 *     band has only 1/4 of its original resolution.
 *
 *     The implementation method is the 2-D weighted block summation method, in which,
 *	   2-Dim filtering is implemented as summation of the input negihborhood block
 *	   weighted by the weight matrix, which is determined by the (5,3) filter banks.
 *     The  weighted block summations are broken down into summations of 5 column 
 *     weighted	 summs. Notice there are some overlapping in the computations for 
 *     differenct bands. Overlapping also exists between two consecutive columns. This
 *     C code have tried to take advantage of both.
 *
 *     There is a weight matrix (block) associated with each band. For the (5,3) 
 *     filter bank, define the decomposition filters
 *       	 H0 = (-1, 2, 6, 2, -1)
 *       	 H1 = (1, -2, 1)
 *     the 4 weight matrices are :
 * 
 *							  1 -2 -6 -2  1
 * 						 	 -2  4 12  4 -2
 *  Band 0:    W0 = H0'H0 =  -6 12 36 12 -6
 *							 -2  4 12  4 -2
 * 							  1 -2 -6 -2  1
 *
 *						      0  0   0  0 0
 *						      0  0   0  0 0
 *  Band 1:	   W1 = H1'H0 =  -1  2   6  2 1
 * 							  2 -4 -12 -4 2
 *							 -1  2   6  2 1
 *
 *							 0  0 -1  2  -1
 *							 0  0  2  -4  2
 *  Band 2:	   W2 = H0'H1 =  0  0  6 -12  6
 *							 0  0  2  -4  2
 *							 0  0 -1   2 -1
 *
 *
 *							0 0  0	 0  0
 * 							0 0  0	 0  0
 *  Band 3:	   W3 = H1'H1 = 0 0  1  -2  1
 *							0 0 -2   4 -2
 *							0 0  1  -2  1		
 *
 *		The boundary extension used at top, bottom, left and right edges of the input
 *  image is always whole-sample symmetry, which is matched to the (5,3) filter bank's
 *  symmetry.  Whole-sample symmetry is illustrated as the following:
 *           	B | A B ... C D | C
 *	where A,B,...,C,D represent the pixel values in an image, | denotes the boundary.
 *
 *  To avoid boundary conditional testing, this function is written in 9 similarly chunks
 *  according to their corresponding boundary orientation in the image.	The code looks
 *  longer but it is more efficient.
 *
 *		Assumptions: 
 *		1) the input image's resolution is, at the very least, even number,
 *  horizontally and vertically. 
 *      2) the input image is in 16-bit representation and in general the 4 output
 *  bands are also in 16-bit. If /HBCLIP option is used in simulator, then the high
 *  frequency bands (1,2,3) can be clipped into 8-bit. With proper scaling factors, 
 *  8-bit typically is sufficient for the high frequency bands and so clipping can 
 *  only introduce very minor (if any) distortion, which usually is invisible.
 *  See i:\proj42\exp\sim\newwave.doc Section 5 on implementation precision issues.
 *
 ***********************************************************************************/


void OptWaveletDecomp(MatrixSt matIn,		/* input: original image */
					  MatrixSt matOut[4],	/* output: 4 Quad-splitted bands */
					  const I16 i16Scale[4] /* the scaling factors for 4 bands are 1/i16Scale[] */ 
					  )	
{
	static const I16  iLowCoeff[5]={-1, 2, 1, 2, -1}; 
		/* coefficients of the low pass filter, which are used for scaling the weighted column sums.
	 	 * Note that the actual coefficient in the center is 6 instead of 1, we use 1 here
		 * for the purpose of avoiding division operation in the whole implementation,
	 	 * we wait till later to do multiplication by 6.
	     */
	PI16 pi16In, pi16B0, pi16B1, pi16B2, pi16B3;  /* ptrs to	input image and 4 output bands*/
	PI16 pi16InCol;
	U32 uRow, uCol;
	I32 iPitch = matIn.Pitch;
	I32 iPitch0 = matOut[0].Pitch;
	I32 iPitch1 = matOut[1].Pitch;
	I32 iPitch2 = matOut[2].Pitch;
	I32 iPitch3 = matOut[3].Pitch;
	I32 iColSum0[5]; /* store the 5 weighted column sums with weight matrix W0, 
	 				  * use 32 bit here for generic simulation, 16 bit maybe enough
					  * for actual implementation. See newwave.doc Section 5 for detail
				      * on precision issues. 
					  */ 
	I32 iColSum1[5]; /* store the 5 weighted column sums with weight matrix W1. */
	I32 c;

	/* first row: top edge */

	/* initialize at the first column: upper left corner */
	pi16In = (PI16)matIn.pi16 ;
	pi16B0 = (PI16)matOut[0].pi16 ;
	pi16B1 = (PI16)matOut[1].pi16 ;
	pi16B2 = (PI16)matOut[2].pi16;
	pi16B3 = (PI16)matOut[3].pi16 ;

	for (c=2;c<5;c++) {/* get the 3 column summs in the 5*5 neighborhood */
		pi16InCol = pi16In+c-2;
		iColSum0[c] = iLowCoeff[c]*( 6*(*pi16InCol) + 4*(*(pi16InCol+iPitch)) - 2*(*(pi16InCol+2*iPitch)));
		iColSum1[c] = iLowCoeff[c]*( *pi16InCol + *(pi16InCol+2*iPitch) -2*(*(pi16InCol+iPitch)) );
	}

	/* generate the first (leftest) pixel for each band */
	*pi16B0 = (6*iColSum0[2] + 2*iColSum0[3] + 2*iColSum0[4])/i16Scale[0];
	*pi16B1 = (6*iColSum1[2] + 2*iColSum1[3] + 2*iColSum1[4])/i16Scale[1];
	*pi16B2 = (iColSum0[2] - iColSum0[3] - iColSum0[4])/i16Scale[2];
	*pi16B3 = (iColSum1[2] - iColSum1[3] - iColSum1[4])/i16Scale[3];

	/* moving forward: update the pointers and the partial summs for next column */
	pi16B0++;
	pi16B1++;
	pi16B2++;
	pi16B3++;
	pi16In+=2;

	/* Notice the overlapping between two iterations (columns):
	 *		current column: 0   1   2   3   4
	 *		next column:            0   1   2  3   4  (after moving forward 2-pixels )
	 * Look up the matrices W0 and W1, hopefully you will now understand why we used 1 instead of 
	 * 6 for the iLowCoeff[2]: otherwise, we would need to do division operation.
	 * Also be careful about the sign here. 
	 */

	iColSum0[1] = iColSum0[3];
	iColSum0[0] = -iColSum0[2];
	iColSum0[2] = -iColSum0[4];

	iColSum1[1] = iColSum1[3];
	iColSum1[0] = -iColSum1[2];
	iColSum1[2] = -iColSum1[4];	

	/* the top edge */
	for (uCol=1; uCol<matOut[0].NumCols-1; uCol++) {
		for (c=2;c<5;c++) {/* get the column summs for the 2 new columns in the 5*5 neighborhood */
			pi16InCol = pi16In+c-2;
			iColSum0[c] = iLowCoeff[c]*( 6*(*pi16InCol) + 4*(*(pi16InCol+iPitch)) - 2*(*(pi16InCol+2*iPitch)));
			iColSum1[c] = iLowCoeff[c]*( *pi16InCol + *(pi16InCol+2*iPitch) - 2*(*(pi16InCol+iPitch)) );
		}
		/* generate the four pixels for four bands */
		*pi16B0 = (iColSum0[0] +iColSum0[1] +6*iColSum0[2] +iColSum0[3] +iColSum0[4])/i16Scale[0];
		*pi16B1 = (iColSum1[0] +iColSum1[1] +6*iColSum1[2] +iColSum1[3] +iColSum1[4])/i16Scale[1];
		*pi16B2 = (iColSum0[2] - iColSum0[3] - iColSum0[4])/i16Scale[2];
		*pi16B3 = (iColSum1[2] - iColSum1[3] - iColSum1[4])/i16Scale[3];

		/* moving forward: update the pointers and the partial summs for next column */
		pi16B0++;
		pi16B1++;
		pi16B2++;
		pi16B3++;
		pi16In+=2;

		iColSum0[1] = iColSum0[3];
		iColSum0[0] = -iColSum0[2];
		iColSum0[2] = -iColSum0[4];

		iColSum1[1] = iColSum1[3];
		iColSum1[0] = -iColSum1[2];
		iColSum1[2] = -iColSum1[4];	

	}	/* end of for (iCol) */

	/* At the upper right corner of the image :
	 * we need to calculate iColSum0[3] and iColSum1[3], and use the mirror-symmetry 
	 * assumption of the input image and symmetry property of the weight block to get
	 * iColSum0[4] and iColSum1[4].
	 * Note that we assume the input image size is always even number, which implies 
	 * that at the right boundary, only one of the 5 columns is outside the original
	 * image boundary 
	 */
	pi16InCol = pi16In+1; /* points to next column, which is indexed as 3 */
	iColSum0[3] = iLowCoeff[3]*( 6*(*pi16InCol) + 4*(*(pi16InCol+iPitch)) - 2*(*(pi16InCol+2*iPitch)) );
	iColSum1[3] = iLowCoeff[3]*( *pi16InCol + *(pi16InCol+2*iPitch) - 2*(*(pi16InCol+iPitch)) );

	iColSum0[4] = -iColSum0[2];
	iColSum1[4] = -iColSum1[2];
	/* generate the four pixels for four bands */
	*pi16B0 = (iColSum0[0] +iColSum0[1] +6*iColSum0[2] +iColSum0[3] +iColSum0[4])/i16Scale[0];
	*pi16B1 = (iColSum1[0] +iColSum1[1] +6*iColSum1[2] +iColSum1[3] +iColSum1[4])/i16Scale[1];
	*pi16B2 = (iColSum0[2] - iColSum0[3] - iColSum0[4])/i16Scale[2];
	*pi16B3 = (iColSum1[2] - iColSum1[3] - iColSum1[4])/i16Scale[3];
	/* done with first row */

	/* between the first row and the last row : main loop */
	for (uRow=1; uRow<matOut[0].NumRows-1; uRow++) 	 {
		 /* Initialize the pointers */
		 pi16In = (PI16)matIn.pi16 + 2 * uRow * iPitch;
		 pi16B0 = (PI16)matOut[0].pi16 + uRow *  iPitch0;
		 pi16B1 = (PI16)matOut[1].pi16 + uRow *  iPitch1;
		 pi16B2 = (PI16)matOut[2].pi16 + uRow *  iPitch2;
		 pi16B3 = (PI16)matOut[3].pi16 + uRow *  iPitch3;

 		 /* At the left boundary of the image */
		 for (c=2;c<5;c++) {/* get the last 3 column summs in the 5*5 neighborhood */
			pi16InCol = pi16In+c-2;
			iColSum0[c] = iLowCoeff[c]*( 6*(*pi16InCol) + 2*( *(pi16InCol-iPitch) + *(pi16InCol+iPitch))
							- *(pi16InCol+2*iPitch) - *(pi16InCol-2*iPitch) );
			iColSum1[c] = iLowCoeff[c]*( *pi16InCol + *(pi16InCol+2*iPitch) - 2*(*(pi16InCol+iPitch)) );
		 }
		/* generate the four pixels for four bands, use mirror-symmetry for iColSum[0-1] */
		*pi16B0 = (6*iColSum0[2] + 2*iColSum0[3] + 2*iColSum0[4])/i16Scale[0];
		*pi16B1 = (6*iColSum1[2] + 2*iColSum1[3] + 2*iColSum1[4])/i16Scale[1];
		*pi16B2 = (iColSum0[2] - iColSum0[3] - iColSum0[4])/i16Scale[2];
		*pi16B3 = (iColSum1[2] - iColSum1[3] - iColSum1[4])/i16Scale[3];

		/* moving forward: update the pointers and the partial summs for next column*/
		pi16B0++;
		pi16B1++;
		pi16B2++;
		pi16B3++;
		pi16In+=2;

		iColSum0[1] = iColSum0[3];
		iColSum0[0] = -iColSum0[2];
		iColSum0[2] = -iColSum0[4];
						
		iColSum1[1] = iColSum1[3];
		iColSum1[0] = -iColSum1[2];
		iColSum1[2] = -iColSum1[4];	

		for (uCol=1; uCol<matOut[0].NumCols-1; uCol++) { /* the very inner loop */
			for (c=3;c<5;c++) { /* get the two new column summs */	   
				pi16InCol = pi16In+c-2;
				iColSum0[c] = iLowCoeff[c]*( 6*(*pi16InCol) + 2*( *(pi16InCol-iPitch) + *(pi16InCol+iPitch))
							- *(pi16InCol+2*iPitch) - *(pi16InCol-2*iPitch) );
				iColSum1[c] = iLowCoeff[c]*( *pi16InCol + *(pi16InCol+2*iPitch) - 2*(*(pi16InCol+iPitch)) );
			}
			/* generate the four pixels for four bands */
			*pi16B0 = (iColSum0[0] +iColSum0[1] +6*iColSum0[2] +iColSum0[3] +iColSum0[4])/i16Scale[0];
			*pi16B1 = (iColSum1[0] +iColSum1[1] +6*iColSum1[2] +iColSum1[3] +iColSum1[4])/i16Scale[1];
			*pi16B2 = (iColSum0[2] - iColSum0[3] - iColSum0[4])/i16Scale[2];
			*pi16B3 = (iColSum1[2] - iColSum1[3] - iColSum1[4])/i16Scale[3];

			/* moving forward: update the pointers and the partial summs for next column */
			pi16B0++;
			pi16B1++;
			pi16B2++;
			pi16B3++;
			pi16In+=2;

			iColSum0[1] = iColSum0[3];
			iColSum0[0] = -iColSum0[2];
			iColSum0[2] = -iColSum0[4];

			iColSum1[1] = iColSum1[3];
			iColSum1[0] = -iColSum1[2];
			iColSum1[2] = -iColSum1[4];	

		}	/* end of for (iCol) */

		/* At the right boundary 
		 * we need to calculate iColSum0[3] and iColSum1[3], and use the mirror-symmetry 
		 * assumption of the input image and symmetry property of the weight block to get
		 * iColSum0[4] and iColSum1[4].
		 * Note that we assume the input image size is always even number, which implies 
		 * that at the right boundary, only one of the 5 columns is outside the original
		 * image boundary 
		 */
		pi16InCol = pi16In+1; /* points to next column, which is indexed as 3 */
		iColSum0[3] = iLowCoeff[3]*( 6*(*pi16InCol) + 2*( *(pi16InCol-iPitch) + *(pi16InCol+iPitch))
							- *(pi16InCol+2*iPitch) - *(pi16InCol-2*iPitch) );
		iColSum1[3] = iLowCoeff[3]*( *pi16InCol + *(pi16InCol+2*iPitch) - 2*(*(pi16InCol+iPitch)) );
		/* use boundary condition */
		iColSum0[4] = -iColSum0[2];
		iColSum1[4] = -iColSum1[2];
		/* generate the four pixels for four bands */
		*pi16B0 = (iColSum0[0] +iColSum0[1] +6*iColSum0[2] +iColSum0[3] +iColSum0[4])/i16Scale[0];
		*pi16B1 = (iColSum1[0] +iColSum1[1] +6*iColSum1[2] +iColSum1[3] +iColSum1[4])/i16Scale[1];
		*pi16B2 = (iColSum0[2] - iColSum0[3] - iColSum0[4])/i16Scale[2];
		*pi16B3 = (iColSum1[2] - iColSum1[3] - iColSum1[4])/i16Scale[3];
	} /* end of (iRow) : done with the main loop*/

	/* last row : note that in the 5x5 neighborhood, only the row iRow+2 is out of the image boundary,
	 *            which, by symmetry assuption, is the same as the current row (iRow).
	 */

	/* Initialize the pointers */
	pi16In = (PI16)matIn.pi16 + 2 * uRow * iPitch;
	pi16B0 = (PI16)matOut[0].pi16 + uRow *  iPitch0;
	pi16B1 = (PI16)matOut[1].pi16 + uRow *  iPitch1;
	pi16B2 = (PI16)matOut[2].pi16 + uRow *  iPitch2;
	pi16B3 = (PI16)matOut[3].pi16 + uRow *  iPitch3;
 	/* At the left boundary of the image */
	for (c=2;c<5;c++) {/* get the last 3 column summs in the 5*5 neighborhood */
		pi16InCol = pi16In+c-2;
		iColSum0[c] = iLowCoeff[c]*( 6*(*pi16InCol) + 2*( *(pi16InCol-iPitch) + *(pi16InCol+iPitch))
							- *(pi16InCol) - *(pi16InCol-2*iPitch) );
		iColSum1[c] = iLowCoeff[c]*( *pi16InCol + *(pi16InCol) - 2*(*(pi16InCol+iPitch)) );
	}
	/* generate the four pixels for four bands, use mirror-symmetry for iColSum[0-1] */
	*pi16B0 = (6*iColSum0[2] + 2*iColSum0[3] + 2*iColSum0[4])/i16Scale[0];
	*pi16B1 = (6*iColSum1[2] + 2*iColSum1[3] + 2*iColSum1[4])/i16Scale[1];
	*pi16B2 = (iColSum0[2] - iColSum0[3] - iColSum0[4])/i16Scale[2];
	*pi16B3 = (iColSum1[2] - iColSum1[3] - iColSum1[4])/i16Scale[3];

	/* moving forward: update the pointers and the partial summs for next column*/
	pi16B0++;
	pi16B1++;
	pi16B2++;
	pi16B3++;
	pi16In+=2;

	iColSum0[1] = iColSum0[3];
	iColSum0[0] = -iColSum0[2];
	iColSum0[2] = -iColSum0[4];
						
	iColSum1[1] = iColSum1[3];
	iColSum1[0] = -iColSum1[2];
	iColSum1[2] = -iColSum1[4];	

	for (uCol=1; uCol<matOut[0].NumCols-1; uCol++) {
		for (c=3;c<5;c++) {	   
			pi16InCol = pi16In+c-2;
			iColSum0[c] = iLowCoeff[c]*( 6*(*pi16InCol) + 2*( *(pi16InCol-iPitch) + *(pi16InCol+iPitch))
								- *(pi16InCol) - *(pi16InCol-2*iPitch) );
			iColSum1[c] = iLowCoeff[c]*( *pi16InCol + *(pi16InCol) - 2*(*(pi16InCol+iPitch)) );
		}
		/* generate the four pixels for four bands, use mirror-symmetry for iColSum[0-1] */
		*pi16B0 = (iColSum0[0] +iColSum0[1] +6*iColSum0[2] +iColSum0[3] +iColSum0[4])/i16Scale[0];
		*pi16B1 = (iColSum1[0] +iColSum1[1] +6*iColSum1[2] +iColSum1[3] +iColSum1[4])/i16Scale[1];
		*pi16B2 = (iColSum0[2] - iColSum0[3] - iColSum0[4])/i16Scale[2];
		*pi16B3 = (iColSum1[2] - iColSum1[3] - iColSum1[4])/i16Scale[3];

		/* moving forward: update the pointers and the partial summs for next column*/
		pi16B0++;
		pi16B1++;
		pi16B2++;
		pi16B3++;
		pi16In+=2;

		iColSum0[1] = iColSum0[3];
		iColSum0[0] = -iColSum0[2];
		iColSum0[2] = -iColSum0[4];
							
		iColSum1[1] = iColSum1[3];
		iColSum1[0] = -iColSum1[2];
		iColSum1[2] = -iColSum1[4];	
	}  /* end of (iCol) */

	/* At the lower right corner of the image :	*/
	pi16InCol = pi16In+1;
	iColSum0[3] = iLowCoeff[3]*( 6*(*pi16InCol) + 2*( *(pi16InCol-iPitch) + *(pi16InCol+iPitch))
							- *(pi16InCol) - *(pi16InCol-2*iPitch) );
	iColSum1[3] = iLowCoeff[3]*( *pi16InCol + *(pi16InCol) - 2*(*(pi16InCol+iPitch)) );

	iColSum0[4] = -iColSum0[2];
	iColSum1[4] = -iColSum1[2];

	/* generate the four pixels for four bands, use mirror-symmetry for iColSum[0-1] */
	*pi16B0 = (iColSum0[0] +iColSum0[1] +6*iColSum0[2] +iColSum0[3] +iColSum0[4])/i16Scale[0];
	*pi16B1 = (iColSum1[0] +iColSum1[1] +6*iColSum1[2] +iColSum1[3] +iColSum1[4])/i16Scale[1];
	*pi16B2 = (iColSum0[2] - iColSum0[3] - iColSum0[4])/i16Scale[2];
	*pi16B3 = (iColSum1[2] - iColSum1[3] - iColSum1[4])/i16Scale[3];
			 
}

