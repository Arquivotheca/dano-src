/*

	File:		spmatrix.c		@(#)spmatrix.c	1.1 11/14/97
	
	Contains:	Matrix operations.

	Written by:	Drivin' Team

	Copyright:	(C) 1997 by Eastman Kodak Company, all rights reserved.

	Change History (most recent first):

*/

/*********************************************************************
 *********************************************************************
 * PROPRIETARY NOTICE:   The  software  information  contained
 * herein   is  the  sole  property   of  Eastman Kodak Company.  
 * and   is provided  to Eastman Kodak Company  users  under license  for use  
 *	on  their designated  equipment  only. Reproduction of this 
 *	matter in whole or in part is forbidden without the  express  
 * written consent of Eastman Kodak Company.
 *
 * COPYRIGHT (c) 1997 Eastman Kodak Company
 * As  an unpublished  work pursuant to Title 17 of the United
 * States Code.  All rights reserved.
 *********************************************************************
 *********************************************************************
 */

#include "sprof-pr.h"

static KpInt32_t isValidMatrix (SpMatrix_p);
static KpInt32_t getMatrixMinDim (SpMatrix_p, SpMatrix_p, SpMatrix_p);

/* This routine multiplies the two matricies specified by src1 and
 * src2 and puts the result in the  matrix dest
 * return:
 * -1	matrix dimension out of range
 * -2	matrix dimensions do not match properly
 */
KpInt32_t
	SpMatMul (	SpMatrix_p src1,
				SpMatrix_p src2,
				SpMatrix_p dest)
{
	KpInt32_t	row, col, i;

	if ((isValidMatrix (src1) != 1) ||	/* matrices must be valid */
		(isValidMatrix (src2) != 1) ||
		(Kp_IsBadWritePtr (dest, sizeof (SpMatrix_t)))) {
		return -1;
	}
	
	if (src1->nCols != src2->nRows) {	/* matrices must match */
		return -2;
	}

	dest->nRows = src1->nRows;			/* define size of result matrix */
	dest->nCols = src2->nCols;
	
	/* ok, multiply them */
	for (row = 0; row < dest->nRows; row++) {
		for (col = 0; col < dest->nCols; col++) {
			dest->coef[row][col] = 0.0;
			for (i = 0; i < src1->nCols; i++) {
				dest->coef[row][col] += src1->coef[row][i] * src2->coef[i][col];
			}
		}
	}
	
	return 1;
}
			
/*
 * This routine "dot-multiplies" two matrices.
 * It multiplies each element of the first matrix times
 * the corresponding element of the second matrix.
 */
KpInt32_t
	SpMatDotMul (	SpMatrix_p src1,
					SpMatrix_p src2,
					SpMatrix_p dest)
{
	KpInt32_t	row, col, error;

	error = getMatrixMinDim (src1, src2, dest);	/* get minimum dimensions */
	if (error != 1) {
		return error;
	}
		
	/* ok, divide them */
	for (row = 0; row < dest->nRows; row++) {
		for (col = 0; col < dest->nCols; col++) {
			dest->coef[row][col] = src1->coef[row][col] * src2->coef[row][col];
		}
	}	
	return 1;
}

/*
 * This routine "dot-divides" two matrices.
 * It divides each element of the first matrix by the
 * corresponding element of the second matrix.
 */
KpInt32_t
	SpMatDotDiv (	SpMatrix_p src1,
					SpMatrix_p src2,
					SpMatrix_p dest)
{
	KpInt32_t	row, col, error;

	error = getMatrixMinDim (src1, src2, dest);	/* get minimum dimensions */
	if (error != 1) {
		return error;
	}
		
	/* ok, divide them */
	for (row = 0; row < dest->nRows; row++) {
		for (col = 0; col < dest->nCols; col++) {
			dest->coef[row][col] = src1->coef[row][col] / src2->coef[row][col];
		}
	}
	
	return 1;
}

/*
 * This routine copies all of the entries of src to dest
 */
KpInt32_t
	SpMatCopy (	SpMatrix_p src,
				SpMatrix_p dest)
{
	KpInt32_t	row, col;

	/* source matrix must be valid */
	if (isValidMatrix (src) != 1) {
		return -1;
	}

	dest->nRows = src->nRows;	/* copy sizes */
	dest->nCols = src->nCols;
	
	/* copy elements */
	for (row = 0; row < src->nRows; row++) {
		for (col = 0; col < src->nCols; col++) {
			dest->coef[row][col] = src->coef[row][col];
		}
	}
	
	return 1;
}

/*
 * This routine zeros all of the entries of tMatrix 
 */
KpInt32_t
	SpMatZero (	SpMatrix_p src)
{
	KpInt32_t	row, col;

	if (isValidMatrix (src) != 1) {
		return 0;
	}

	/* zero all elements */
	for (row = 0; row < SP_MATRIX_MAX_DIM; row++) {
		for (col = 0; col < SP_MATRIX_MAX_DIM; col++) {
			src->coef[row][col] = 0.0;
		}
	}
	
	return 1;
}

/*
 *
 */
static KpInt32_t
	isValidMatrix (	SpMatrix_p src)
{
	if (Kp_IsBadWritePtr (src, sizeof (SpMatrix_t))) {
		return 0;
	}
	
	if ((src->nRows < 0) || (src->nRows > SP_MATRIX_MAX_DIM) ||
		(src->nCols < 0) || (src->nCols > SP_MATRIX_MAX_DIM)) {
		return 0;
	}
	else {
		return 1;
	}
}

/*
 *
 */
static KpInt32_t
	getMatrixMinDim (	SpMatrix_p	src1,
						SpMatrix_p	src2,
						SpMatrix_p	dest)
{

	/* matrices must be valid */
	if ((isValidMatrix (src1) != 1) ||
		(isValidMatrix (src2) != 1) ||
		(Kp_IsBadWritePtr (dest, sizeof (SpMatrix_t)))) {
		return 0;
	}
	
	dest->nRows = MIN (src1->nRows, src2->nRows);	/* set size of result matrix */
	dest->nCols = MIN (src1->nCols, src2->nCols);
	
	return 1;
}





