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

/****************************************************************************
 *	recomp.c :
 *
 *    This file contains the specialized and optimized C routines for (5,3)
 *  wavelet recomposition. See decomp.c for the corresponding recomposition 
 *  function. See wavelet.c for generic wavelet implementation if the filter
 *  bank is not this particular (5,3) one.
 *    It is recommended to read the detail document on this wavelet simulation 
 *  i:\proj42\doc\sim\newwave.doc, esp. section 4.1, before trying to understand 
 *  code itself.
 *
 *  The following is the description of this particular filter bank in 1-Dim.
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
 *  	OptWaveletRecomp()
 *********************************************************************************/

#include "datatype.h"
#include "recomp.h"

/* Optimized implementation for #2 wavelet recomposition use the scaling factors {(4,2,2,1)/16 }.
 * This set of scaling factors is the set which keeps the typical dynamic 
 * range of	the output band data the same as the input raw data.
 * So this is intended to be used for the best implementation of new wavelet #2.
 */
const I16 i16RecompScale[4]={4,2,2,1};

/*************************************************************************************
 * 	OptWaveletRecomp(): 
 *	
 *	   This function takes 4 2-Dim subband images as input and applies (5,3) filter 
 * bank recomposition in 2-Dim and generates one reconstructed image as its output. 
 * It is assumed that all four band	has the same size and the reconstructed image is 
 * double resolution of the band images both horizontally and vertically. 
 * 	   The scaling factors for 4 bands are taken as a argument to the function. 
 * Currently, if /WAVE:5 is specified as a wavelet option, then {4,4,4,1}/16 is the set
 * of scaling factors to use. If /WAVE:5 is specified, then {4,2,2,1}/16 is the set of
 * scaling factors.  
 *
 *     The implementation method is the 2-D weighted block summation method, in which,
 * 2-Dim filtering is implemented as summation of the input negihborhood block
 * weighted by the weight matrix, which is determined by the (5,3) filter banks.
 * 	   In the very inner loop, 4 pixel values in the reconstructed image are produced
 * for every one pixel at subband, these 4 pixels corresponding to
 *			       R0(iRow, iCol)    R2(iRow, iCol+1)
 *			       R1(iRow+1, iCol)  R3(iRow+1, iCol+1).
 * And at each band, only those pixels in a small neighborhood are involved for
 * producing these reconstructed pixel values. The neighborhoods for 0, 1, 2, and 3
 * band are 2*2, 3*2, 2*3 and 3*3, respectively. The weighted block summations for 
 * these four pixels are broken down into summations of some column summations 
 *(namely iSumA, iSumB, iSumC, iSumC as in this function). In general, to generate R0,
 * R1, R2 and R3, we need to have coulmn summs along iCol-1, iCol, and iCol+1, so we 
 * index these summations from 0, 1 to 2.
 *
 *	   Since we have to deal with boundary carefully, there are two styles of writing
 * the code here. One way is to divide the image into nine sections as shown in the 
 * following, and write the code sequentially in nine chunks. 
 *		*	--------   *
 *		|	$$$$$$$$   |
 *		|	$$$$$$$$   |
 *		*	--------   *
 * Another way is to write only one chunk, but with boundary condition testing all
 * over the code. Personally I prefer the first style and so this function is written
 * in that way.
 *		The boundary extension methods (at top, bottom, left and right edges)
 * are shown as following for 4 bands:
 *
 *		  Band 0         Band1         Band2          Band3
 *
 *         WS			  HS			WS				HS
 *       -------     	-------		  -------		 -------
 *		 |	   |		|	  |		  |		|		 |	   |
 *	   WS|	   |HS    WS|	  |HS   HS|		|WS    HS|	   |WS
 *		 |	   |		|	  |		  |		|		 |	   |
 *       -------     	-------		  -------		 -------
 *         HS			  WS			HS			   WS
 *
 *     Notation: WS (whole-sample symmetry): B | A B ... C D | C
 *               HS (half-sample symmetry):  A | A B ... C D | D ,
 *	where A,B,...,C,D represent the pixel values in an image, | denotes the boundary.
 * 	  
 *
 *************************************************************************************/
void OptWaveletRecomp(MatrixSt matIn[4],		/* input: 4 Quad-splitted bands */
					  MatrixSt matOut,			/* output: reconstructed image */
					  const I16 i16Scale[4]		/* scaling factors for 4 bands,
											 * the actual scaling factors are i16Scale[]/16
											 */
					  )	       
{
	 PI16 pi16Out, pi16B0, pi16B1, pi16B2, pi16B3;  /* ptrs to input image and 4 output bands*/
	 I32 iRow, iCol;
	 I32 iPitch = matOut.Pitch;
	 I32 iPitch0 = matIn[0].Pitch;
	 I32 iPitch1 = matIn[1].Pitch;
	 I32 iPitch2 = matIn[2].Pitch;
	 I32 iPitch3 = matIn[3].Pitch;
	 I32 iNumRows = matIn[0].NumRows;
	 I32 iNumCols = matIn[0].NumCols;
	 I32 R[4]; /* for 4 reconstructed pixels */
	 I32		 iSumA1, iSumA2;  /* partial summations along the column. These summations are 
								     used to reconstruct the output pixels.	See newwave.doc 
									 section 4.1 for detail derivation of the formular.
								     index 1 corresponds to current column, 2 to next column, 
									 and 0 to the previous column.
									 Same rule of indexing applies to iSumB, iSumC and iSumD too.
								   */
	 I32 iSumB0, iSumB1, iSumB2;
	 I32 iSumC0, iSumC1, iSumC2; 
	 I32		 iSumD1, iSumD2;

	/*  upper left corner: iCol==0, iRow==0, special boundary treatment */

	pi16Out = (PI16)matOut.pi16;
	pi16B0 = (PI16)matIn[0].pi16;
	pi16B1 = (PI16)matIn[1].pi16;
	pi16B2 = (PI16)matIn[2].pi16;
	pi16B3 = (PI16)matIn[3].pi16;

	/* initilize the partial sums */
	iSumA1 = ( (*pi16B1)*2*i16Scale[1] + (*pi16B0)*i16Scale[0] ) <<1;
	iSumA2 = ( (*(pi16B1+1))*2*i16Scale[1] + (*(pi16B0+1 ))*i16Scale[0] )<<1;
	iSumB0 = iSumB1 = ( (*pi16B3)*2*i16Scale[3] + (*pi16B2)*i16Scale[2] )<<1;
	iSumB2 = ( (*(pi16B3+1))*2*i16Scale[3] + (*(pi16B2+1))*i16Scale[2] )<<1 ;
	iSumC0 = iSumC1 = ( *pi16B2 + *(pi16B2+iPitch2) )*i16Scale[2] 
				+ ( *(pi16B3+iPitch3)-5*(*pi16B3) )*i16Scale[3];
	iSumC2 = (*(pi16B2+1) + *(pi16B2+iPitch2+1))*i16Scale[2] 
					+ ( *(pi16B3+iPitch3+1) - 5*(*(pi16B3+1)) )*i16Scale[3];
	iSumD1 = ( *pi16B0 + *(pi16B0+iPitch0) )*i16Scale[0] 
				+ ( *(pi16B1+iPitch1)-5*(*pi16B1) )*i16Scale[1] ;
	iSumD2 = ( *(pi16B0+iPitch0+1) + *(pi16B0+1) )*i16Scale[0]
					+ ( *(pi16B1+1+iPitch1) -5*(*(pi16B1+1)) )*i16Scale[1];

	/* reconstruct 4 pixels from these partial sums */
	R[0] = (iSumA1 + iSumB0 + iSumB1)<<1;
	R[2] = iSumA1 +	iSumB0 + iSumA2 + iSumB2 - 6*iSumB1;
	R[1] = (iSumC0 + iSumD1 +iSumC1)<<1;
	R[3] = iSumC0 + iSumD1 + iSumD2 + iSumC2 - 6*iSumC1;
	*(pi16Out) = (I16)(R[0]>>4);
	*(pi16Out+1) = (I16)(R[2]>>4);
	*(pi16Out+iPitch) = (I16)(R[1]>>4);
	*(pi16Out+iPitch+1) = (I16)(R[3]>>4);

	/* update the partial summs for next iteration: 
	 * taking advantage of the overlapping computations
	 */
	iSumA1 = iSumA2;
	iSumB0 = iSumB1;
	iSumB1 = iSumB2;
	iSumC0 = iSumC1;
	iSumC1 = iSumC2;
	iSumD1 = iSumD2;

	/* move pointers forward */
	pi16Out += 2;
	pi16B0 ++;
	pi16B1 ++;
	pi16B2 ++;
	pi16B3 ++;
	/* done with the left upper corner */

	/* top edge loop */

  	for (iCol=1; iCol<iNumCols-1;iCol++)  {
		/* compute the new partial summs based upon pixels along the new column */
		iSumA2 = ( (*(pi16B1+1))*2*i16Scale[1] + (*(pi16B0+1))*i16Scale[0] )<<1;
		iSumB2 = ( (*(pi16B3+1))*2*i16Scale[3] + (*(pi16B2+1))*i16Scale[2] )<<1 ;
		iSumC2 = (*(pi16B2+1) + *(pi16B2+iPitch2+1))*i16Scale[2] 
					+ ( *(pi16B3+iPitch3+1) -5*(*(pi16B3+1)) )*i16Scale[3];
		iSumD2 = ( *(pi16B0+iPitch0+1) + *(pi16B0+1) )*i16Scale[0]
						+ ( *(pi16B1+1+iPitch1) -5*(*(pi16B1+1)) )*i16Scale[1];

		/* reconstruct 4 pixels from these partial sums */
		R[0] = (iSumA1 + iSumB0 + iSumB1)<<1;
		R[2] = iSumA1 +	iSumB0 + iSumA2 + iSumB2 - 6*iSumB1;
		R[1] = (iSumC0 + iSumD1 +iSumC1)<<1;
		R[3] = iSumC0 + iSumD1 + iSumD2 + iSumC2 - 6*iSumC1;
		*(pi16Out) = (I16)(R[0]>>4);
		*(pi16Out+1) = (I16)(R[2]>>4);
		*(pi16Out+iPitch) = (I16)(R[1]>>4);
		*(pi16Out+iPitch+1) = (I16)(R[3]>>4);

		/* update the partial summs for next iteration: 
		 * taking advantage of the overlapping computations
		 */
		iSumA1 = iSumA2;
		iSumB0 = iSumB1;
		iSumB1 = iSumB2;
		iSumC0 = iSumC1;
		iSumC1 = iSumC2;
		iSumD1 = iSumD2;

		/* move pointers forward */
		pi16Out += 2;
		pi16B0 ++;
		pi16B1 ++;
		pi16B2 ++;
		pi16B3 ++;
	} /* end of for (iCol) */

	/* upper right  corner: iCol==(iNumCols-1) and iRow ==0 */

	/* compute the new partial summs based upon pixels along the new column */
	iSumA2 = ( (*pi16B1)*2*i16Scale[1] + (*pi16B0)*i16Scale[0] ) <<1;
	iSumB2 = ( (*(pi16B3-1))*2*i16Scale[3] + (*(pi16B2-1))*i16Scale[2] )<<1;
	iSumC2 = (*(pi16B2+iPitch2-1) + *(pi16B2-1) )*i16Scale[2]
					+ (*(pi16B3+iPitch3-1) - 5*( *(pi16B3-1)) )* i16Scale[3] ;
	iSumD2 = ( *pi16B0 + *(pi16B0+iPitch0) )*i16Scale[0]
					+ ( *(pi16B1+iPitch1)- 5*(*pi16B1) )*i16Scale[1];

	/* reconstruct 4 pixels from these partial sums */
	R[0] = (iSumA1 + iSumB0 + iSumB1)<<1;
	R[2] = iSumA1 +	iSumB0 + iSumA2 + iSumB2 - 6*iSumB1;
	R[1] = (iSumC0 + iSumD1 +iSumC1)<<1;
	R[3] = iSumC0 + iSumD1 + iSumD2 + iSumC2 - 6*iSumC1;
	*(pi16Out) = (I16)(R[0]>>4);
	*(pi16Out+1) = (I16)(R[2]>>4);
	*(pi16Out+iPitch) = (I16)(R[1]>>4);
	*(pi16Out+iPitch+1) = (I16)(R[3]>>4);
	/* done with top edge, i.e. first row */

	/* winthin top edge and bottom edge: main loop */
	
  	for (iRow=1; iRow<iNumRows-1;iRow++)  {	 

		pi16Out = (PI16)matOut.pi16 + 2*iRow*iPitch ;
		pi16B0 = (PI16)matIn[0].pi16 + iRow*iPitch0;
		pi16B1 = (PI16)matIn[1].pi16 + iRow*iPitch1;
		pi16B2 = (PI16)matIn[2].pi16 + iRow*iPitch2;
		pi16B3 = (PI16)matIn[3].pi16 + iRow*iPitch3;

		/* left edge: iCol==0 : initialization for partial summs */
		iSumA1 = ( (*pi16B0)*i16Scale[0] + ( *pi16B1 + *(pi16B1-iPitch1) )*i16Scale[1] ) <<1;
		iSumA2 = ( (*(pi16B1+1) + *(pi16B1+1-iPitch1))*i16Scale[1] + (*(pi16B0+1))*i16Scale[0] )<<1;
		iSumB0 = iSumB1 = ( (*pi16B3 + *(pi16B3-iPitch3))*i16Scale[3] + (*pi16B2)*i16Scale[2] )<<1;
		iSumB2 = ( (*(pi16B3+1) + *(pi16B3+1-iPitch3))*i16Scale[3] + (*(pi16B2+1))*i16Scale[2] )<<1 ;
		iSumC0 = iSumC1 = ( *pi16B2 + *(pi16B2+iPitch2) )*i16Scale[2] 
						+ ( *(pi16B3-iPitch3) + *(pi16B3+iPitch3)- 6*(*pi16B3) )*i16Scale[3];
		iSumC2 = (*(pi16B2+1) + *(pi16B2+iPitch2+1))*i16Scale[2] 
						+ (*(pi16B3+iPitch3+1) + *(pi16B3+1-iPitch3) - 6*(*(pi16B3+1)) )*i16Scale[3];
		iSumD1 = ( *pi16B0 + *(pi16B0+iPitch0) )*i16Scale[0]
						+ ( *(pi16B1-iPitch1) + *(pi16B1+iPitch1)-6*(*pi16B1) )*i16Scale[1] ;
		
		iSumD2 = ( *(pi16B0+iPitch0+1) + *(pi16B0+1) )*i16Scale[0]
						+ ( *(pi16B1+1-iPitch1)+ *(pi16B1+1+iPitch1) -6*(*(pi16B1+1)) )*i16Scale[1];

		/* reconstruct 4 pixels from these partial sums */
		R[0] = (iSumA1 + iSumB0 + iSumB1)<<1;
		R[2] = iSumA1 +	iSumB0 + iSumA2 + iSumB2 - 6*iSumB1;
		R[1] = (iSumC0 + iSumD1 +iSumC1)<<1;
		R[3] = iSumC0 + iSumD1 + iSumD2 + iSumC2 - 6*iSumC1;
		*(pi16Out) = (I16)(R[0]>>4);
		*(pi16Out+1) = (I16)(R[2]>>4);
		*(pi16Out+iPitch) = (I16)(R[1]>>4);
		*(pi16Out+iPitch+1) = (I16)(R[3]>>4);

		/* update the partial summs for next iteration:
		 * taking advantage of the overlapping computations
		 */
		iSumA1 = iSumA2;
		iSumB0 = iSumB1;
		iSumB1 = iSumB2;
		iSumC0 = iSumC1;
		iSumC1 = iSumC2;
		iSumD1 = iSumD2;

		/* move pointers forward */
		pi16Out += 2;
		pi16B0 ++;
		pi16B1 ++;
		pi16B2 ++;
		pi16B3 ++;

		for (iCol=1; iCol<iNumCols-1;iCol++)  {	 /* inner loop: no boundary condition at all */

			/* update the partial summs based upon new pixels along the new column */
			iSumA2 = ( (*(pi16B1+1) + *(pi16B1+1-iPitch1))*i16Scale[1] + (*(pi16B0+1))*i16Scale[0] )<<1;
			iSumB2 = ( (*(pi16B3+1) + *(pi16B3+1-iPitch3))*i16Scale[3] + (*(pi16B2+1))*i16Scale[2] )<<1 ;
			iSumC2 = (*(pi16B2+1) + *(pi16B2+iPitch2+1) )*i16Scale[2] 
						+ ( *(pi16B3+iPitch3+1) + *(pi16B3+1-iPitch3) - 6*(*(pi16B3+1)) )*i16Scale[3];
			iSumD2 = (*(pi16B0+iPitch0+1) + *(pi16B0+1) )*i16Scale[0]
						+ ( *(pi16B1+1-iPitch1) + *(pi16B1+1+iPitch1) - 6*(*(pi16B1+1)) )*i16Scale[1];

			/* reconstruct 4 pixels from these partial sums */
			R[0] = (iSumA1 + iSumB0 + iSumB1)<<1;
			R[2] = iSumA1 +	iSumB0 + iSumA2 + iSumB2 - 6*iSumB1;
			R[1] = (iSumC0 + iSumD1 +iSumC1)<<1;
			R[3] = iSumC0 + iSumD1 + iSumD2 + iSumC2 - 6*iSumC1;
			*(pi16Out) = (I16)(R[0]>>4);
			*(pi16Out+1) = (I16)(R[2]>>4);
			*(pi16Out+iPitch) = (I16)(R[1]>>4);
			*(pi16Out+iPitch+1) = (I16)(R[3]>>4);

			/* update the partial summs for next iteration:
			 * taking advantage of the overlapping computations 
			 */
			iSumA1 = iSumA2;
			iSumB0 = iSumB1;
			iSumB1 = iSumB2;
			iSumC0 = iSumC1;
			iSumC1 = iSumC2;
			iSumD1 = iSumD2;

			/* move pointers forward */
			pi16Out += 2;
			pi16B0 ++;
			pi16B1 ++;
			pi16B2 ++;
			pi16B3 ++;
		}	 /* end of for (iCol): inner loop */

		/* right edge : iCol== (iNumCols-1) */
		iSumA2 = ( (*(pi16B1-iPitch1) + *pi16B1)*i16Scale[1] + (*pi16B0)*i16Scale[0] ) <<1;
		iSumB2 = ( (*(pi16B3-1) + *(pi16B3-iPitch3-1))*i16Scale[3] + (*(pi16B2-1))*i16Scale[2] )<<1;
		iSumC2 =  (*(pi16B2+iPitch2-1) + *(pi16B2-1) )*i16Scale[2] 
						+ ( *(pi16B3+iPitch3-1) + *(pi16B3-iPitch3-1) - 6*( *(pi16B3-1)) )*i16Scale[3];
		iSumD2 =  (*pi16B0 + *(pi16B0+iPitch0))*i16Scale[0] 
						+ ( *(pi16B1-iPitch1) + *(pi16B1+iPitch1) -6*(*pi16B1) )*i16Scale[1];
 		
		/* reconstruct 4 pixels from these partial sums */
		R[0] = (iSumA1 + iSumB0 + iSumB1)<<1;
		R[2] = iSumA1 +	iSumB0 + iSumA2 + iSumB2 - 6*iSumB1;
		R[1] = (iSumC0 + iSumD1 +iSumC1)<<1;
		R[3] = iSumC0 + iSumD1 + iSumD2 + iSumC2 - 6*iSumC1;
		*(pi16Out) = (I16)(R[0]>>4);
		*(pi16Out+1) = (I16)(R[2]>>4);
		*(pi16Out+iPitch) = (I16)(R[1]>>4);
		*(pi16Out+iPitch+1) = (I16)(R[3]>>4);
	}  /* end of for (iRow) : main loop */

	/* last row ( bottom edge ): iRow == iNumRows-1 */

	pi16Out = (PI16)matOut.pi16 + 2*iRow*iPitch ;
	pi16B0 = (PI16)matIn[0].pi16 + iRow*iPitch0;
	pi16B1 = (PI16)matIn[1].pi16 + iRow*iPitch1;
	pi16B2 = (PI16)matIn[2].pi16 + iRow*iPitch2;
	pi16B3 = (PI16)matIn[3].pi16 + iRow*iPitch3;

	/* lower left  corner : initialization of the partial summs */
	iSumA1 = ( (*pi16B0)*i16Scale[0] + (*pi16B1 + *(pi16B1-iPitch1))*i16Scale[1] ) <<1;
	iSumA2 = ( (*(pi16B1+1) + *(pi16B1+1-iPitch1))*i16Scale[1] + (*(pi16B0+1))*i16Scale[0] )<<1;
	iSumB0 = iSumB1 = ( (*pi16B3 + *(pi16B3-iPitch3))*i16Scale[3] + (*pi16B2)*i16Scale[2] )<<1;
	iSumB2 = ( (*(pi16B3+1) + *(pi16B3+1-iPitch3))*i16Scale[3] + (*(pi16B2+1))*i16Scale[2] )<<1 ;
	iSumC0 = iSumC1 = (*pi16B2)*2*i16Scale[2] + ( (*(pi16B3-iPitch3))*2- 6*(*pi16B3) )*i16Scale[3];
	iSumC2 =  (*(pi16B2+1))*2*i16Scale[2] + ( (*(pi16B3-iPitch3+1))*2 - 6*(*(pi16B3+1)) )*i16Scale[3];
	iSumD1 = (*pi16B0)*2*i16Scale[0] + ( (*(pi16B1-iPitch1))*2 -6*(*pi16B1))*i16Scale[1];
	iSumD2 = (*(pi16B0+1))*2*i16Scale[0] + ( (*(pi16B1+1-iPitch1))*2-6*(*(pi16B1+1)) )*i16Scale[1];

	/* reconstruct 4 pixels from these partial sums */
	R[0] = (iSumA1 + iSumB0 + iSumB1)<<1;
	R[2] = iSumA1 +	iSumB0 + iSumA2 + iSumB2 - 6*iSumB1;
	R[1] = (iSumC0 + iSumD1 +iSumC1)<<1;
	R[3] = iSumC0 + iSumD1 + iSumD2 + iSumC2 - 6*iSumC1;
	*(pi16Out) = (I16)(R[0]>>4);
	*(pi16Out+1) = (I16)(R[2]>>4);
	*(pi16Out+iPitch) = (I16)(R[1]>>4);
	*(pi16Out+iPitch+1) = (I16)(R[3]>>4);

	/* update the partial summs for next iteration:
	 * taking advantage of the overlapping computations 
	 */
	iSumA1 = iSumA2;
	iSumB0 = iSumB1;
	iSumB1 = iSumB2;
	iSumC0 = iSumC1;
	iSumC1 = iSumC2;
	iSumD1 = iSumD2;

	/* move pointers forward */
	pi16Out += 2;
	pi16B0 ++;
	pi16B1 ++;
	pi16B2 ++;
	pi16B3 ++;

  	for (iCol=1; iCol<iNumCols-1;iCol++)  {	
		
		/* update the partial summs based upon new pixels along the new column */
		iSumA2 = ( (*(pi16B1+1) + *(pi16B1+1-iPitch1))*i16Scale[1] + (*(pi16B0+1))*i16Scale[0] )<<1;
		iSumB2 = ( (*(pi16B3+1) + *(pi16B3+1-iPitch3))*i16Scale[3] + (*(pi16B2+1))*i16Scale[2] )<<1 ;
		iSumC2 = (*(pi16B2+1))*2*i16Scale[2] + ( (*(pi16B3-iPitch3+1))*2 - 6*(*(pi16B3+1)) )*i16Scale[3];
		iSumD2 = (*(pi16B0+1))*2*i16Scale[0] + ( (*(pi16B1+1-iPitch1))*2 - 6*(*(pi16B1+1)) )*i16Scale[1];

		/* reconstruct 4 pixels from these partial sums */
		R[0] = (iSumA1 + iSumB0 + iSumB1)<<1;
		R[2] = iSumA1 +	iSumB0 + iSumA2 + iSumB2 - 6*iSumB1;
		R[1] = (iSumC0 + iSumD1 +iSumC1)<<1;
		R[3] = iSumC0 + iSumD1 + iSumD2 + iSumC2 - 6*iSumC1;
		*(pi16Out) = (I16)(R[0]>>4);
		*(pi16Out+1) = (I16)(R[2]>>4);
		*(pi16Out+iPitch) = (I16)(R[1]>>4);
		*(pi16Out+iPitch+1) = (I16)(R[3]>>4);

		/* update the partial summs for next iteration: 
		 * taking advantage of the overlapping computations
		 */
		iSumA1 = iSumA2;
		iSumB0 = iSumB1;
		iSumB1 = iSumB2;
		iSumC0 = iSumC1;
		iSumC1 = iSumC2;
		iSumD1 = iSumD2;

		/* move pointers forward */
		pi16Out += 2;
		pi16B0 ++;
		pi16B1 ++;
		pi16B2 ++;
		pi16B3 ++;
	} /* end of for (iCol) */

	/* right lower corner :iCol == iNumCols-1 */

	/* update the partial summs based upon new pixels along the new column */
	iSumA2 = ( (*(pi16B1-iPitch1) + *pi16B1)*i16Scale[1] + (*pi16B0)*i16Scale[0]) <<1;
	iSumB2 = ( (*(pi16B3-1) + *(pi16B3-iPitch3-1))*i16Scale[3] + (*(pi16B2-1))*i16Scale[2] )<<1;
	iSumC2 = (*(pi16B2-1))*2*i16Scale[2] + ((*(pi16B3-iPitch3-1))*2 - 6*(*(pi16B3-1)) )*i16Scale[3];
	iSumD2 =  (*pi16B0)*2*i16Scale[0] + ( (*(pi16B1-iPitch1))*2 -6*(*pi16B1) )*i16Scale[1];
	
	/* reconstruct the last 4 pixels */
	R[0] = (iSumA1 + iSumB0 + iSumB1)<<1;
	R[2] = iSumA1 +	iSumB0 + iSumA2 + iSumB2 - 6*iSumB1;
	R[1] = (iSumC0 + iSumD1 +iSumC1)<<1;
	R[3] = iSumC0 + iSumD1 + iSumD2 + iSumC2 - 6*iSumC1;
	*(pi16Out) = (I16)(R[0]>>4);
	*(pi16Out+1) = (I16)(R[2]>>4);
	*(pi16Out+iPitch) = (I16)(R[1]>>4);
	*(pi16Out+iPitch+1) = (I16)(R[3]>>4);

 }	 /* end of OptWaveletRecomp() */
