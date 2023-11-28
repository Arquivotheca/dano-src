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
 * DESCRIPTION:
 * 
 * This module contains the source code for matrix manipulation routines.
 * There are basic routines:
 * 
 * MatAlloc
 * MatFree
 * MatSet (set a rectangular area of a matrix to a certain value)
 * The basic routines above are used by the rest of the routines:
 *
 * MatSetFull
 * MatSetFast
 * MatCksum
 * MatShiftLeft
 * MatShiftRight
 * MatAddFull
 * MatCopyFull
 * MatCopyFast
 * MatCopyFullOffset
 *
 * SetMatrixRect
 */
/*#ifdef DEBUG  */
/*#include <windows.h> Needed for errhand.h if DEBUG defined */
/*#endif */

#include <string.h>  /* for memcpy */
#include <setjmp.h>

#include "datatype.h"
#include "matrix.h"
#include "const.h"

/* #include "tksys.h" */
#include "pia_main.h"
#include "errhand.h"

/* 
 * Copies a rectangular area from matrix mIn to matrix Out.  The actual
 * copying is done by memcpy.
 */
void MatCopy(MatrixSt mIn, MatrixSt mOut, RectSt R, jmp_buf jbEnv)
{
	U32  i;         /* Number of rectangle row we are copying */
	PI16 pi16From;	/* the data loccation in In we are copying from */
	PI16 pi16To;	/* the data loccation in Out we are copying to */

#ifdef DEBUG
	/* Make sure we are working with 2 valid matrices */
	if ((mIn.pi16 == NULL) || (mOut.pi16 == NULL))
		longjmp(jbEnv,  (MATRIX << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_NULL_PARM << TYPE_OFFSET));
	
	/* Make sure that our RectSt fits into both matrices */
	if (((R.c + R.w) > mIn.NumCols) || ((R.r + R.h) > mIn.NumRows))
		longjmp(jbEnv,  (MATRIX << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_BAD_PARM << TYPE_OFFSET));
	if (((R.c + R.w) > mOut.NumCols) || ((R.r + R.h) > mOut.NumRows))
		longjmp(jbEnv,  (MATRIX << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_BAD_PARM << TYPE_OFFSET));
#endif
	/* Copy the lines over, one by one */
	for (i = 0; i < R.h; i++) {
		pi16From = mIn.pi16  + ((R.r + i) * mIn.Pitch)  + (R.c);
		pi16To   = mOut.pi16 + ((R.r + i) * mOut.Pitch) + (R.c);
		memcpy(pi16To, pi16From, R.w * sizeof(I16));
	}

	return;
}

/* Fast copy of an entire matrix.  That includes off screen buffers
 * The calculation for the padding size must be the same as the one
 * for MatAllocFunc()
 */
void MatCopyFast(MatrixSt mIn, MatrixSt mOut, jmp_buf jbEnv) 
{
	I32 iPadded = (mIn.NumRows + 7) & ~0x7; /* Num rows after padding */
	memcpy(mOut.pi16, mIn.pi16, iPadded*mIn.Pitch*sizeof(I16));
}

/* Fast Setting of an entire matrix */
void MatSetFast(MatrixSt mIn, PI16 pi16Value, jmp_buf jbEnv)
{
	I32 iPadded = (mIn.NumRows + 7) & ~0x7; /* Num rows after padding */
	memset( mIn.pi16, (I32)*pi16Value, iPadded*mIn.Pitch*sizeof(I16) );
}

/*
 * Copy an entire Matrix from mIn to mOut.
 */

void MatCopyFull(MatrixSt mIn, MatrixSt mOut, jmp_buf jbEnv) 
{
	RectSt rect;
	
	rect.r = rect.c = 0;
	rect.h = mIn.NumRows;
	rect.w = mIn.NumCols;
	MatCopy(mIn, mOut, rect, jbEnv);
}


/* 
 * Sets a rectangular area of matrix Out to value.  
 * Since the only values set are 0000 and 0xFFFF, this can be
 * treated as setting bytes to twice as many values.
 */

void MatSet(MatrixSt mOut, RectSt R, PI16 pi16Value, jmp_buf jbEnv)
{
	I32 r, iLim; 
#ifdef DEBUG
	/* Make sure matrix and pointer are valid */
	if ((pi16Value == NULL) || (mOut.pi16 == NULL))
		longjmp(jbEnv,  (MATRIX << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_NULL_PARM << TYPE_OFFSET));
	
	/* Make sure that the RectSt fits into the matrix */
	if (((R.c + R.w) > mOut.NumCols) || ((R.r + R.h) > mOut.NumRows))
		longjmp(jbEnv,  (MATRIX << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_BAD_PARM << TYPE_OFFSET));
#endif
	/* All loop parameters are multiplied by mOut.Pitch to avoid the need
	 * to multiply within the loop.  The loop is looping R.h times, once for
	 * each row, with the looping index adjusted for minimum computation 
	 */
	iLim = (R.r + R.h) * mOut.Pitch; /* Loop maximum */
	for (r = R.r*mOut.Pitch; r < iLim; r += mOut.Pitch) {
		memset( mOut.pi16 + r + R.c, 
				(U8) *pi16Value, R.w * sizeof(I16));
	}

	return;
}

/* Set the entire matrix to *pi16Value.  Since this is only used
 * with values of 0 and FFFF, a memset with a byte value may be
 * used.  This just calls MatSet with a rectangle the size of the
 * matrix.  Since matrices have a Pitch, there is no guarantee that
 * rows are adjacent in memory so a single memset doesn't work.
 */
void MatSetFull(MatrixSt Matrix, PI16 pi16Value, jmp_buf jbEnv)
{
	RectSt Rect;
	Rect.r = Rect.c = 0;
	Rect.w = Matrix.NumCols;
	Rect.h = Matrix.NumRows;
	MatSet(Matrix, Rect, pi16Value, jbEnv);
}


/*
 *
 * MatAlloc
 *
 * Allocate/init storage for matrix data. 
 * This is a special matrix allocation routine for Indeo 5.  It 
 * adds a padding line at both the top and the bottom of the image.
 * This is to remove special case handling when encoding images
 * to bitstreams.
 *
 * Both dimensions are rounded up to a multiple of 8. This
 * prevents overflow when low res images are expanded up. The
 * problem arises because they had been extended to be a mult of 4
 * for VQ.
 *
 * Returns nothing
 */

void MatAllocFunc(PMatrixSt pMat, jmp_buf jbEnv)
{
	I32 iMSize;
	I32 iPadded = (pMat->NumRows + 7) & ~0x7; /* Num rows after padding */
	
	pMat->Pitch = (pMat->NumCols + 7) & ~0x7; 
	
	 /* matrix size is width x height x size of data object */
	iMSize = (I32) pMat->Pitch * iPadded * sizeof(I16);	
	pMat->pi16 = (PI16)HiveGlobalAllocPtr(iMSize, TRUE);
	
	return;	
}


void MatFreeFunc(MatrixSt matrix, jmp_buf jbEnv)
{
	HiveGlobalFreePtr(matrix.pi16);
}


/* 
 * Converts I16->U8 (eventually will be more general)
 * Also does clamping based on the input parameters.
 */

void
MatConvU8(MatrixSt mi16)
{
	PI16 pi16In;  /* Input */
	PU8  pu8Out;	/* Output */
	U8   u8Byte1, u8Byte2;
	U32  uRow, uCol;
	I16  i16Pix;

	for (uRow = 0; uRow < mi16.NumRows; uRow++) {
		pi16In = (PI16) mi16.pi16 + uRow * mi16.Pitch;
		pu8Out = (PU8)  mi16.pi16  + uRow * mi16.Pitch;
		for (uCol = 0; uCol < mi16.NumCols >> 1; uCol++) {
			i16Pix = *pi16In++ + 128;
			if(i16Pix > MaxClamp) {
				u8Byte1 = (U8) MaxClamp;
			}
			else if(i16Pix < MinClamp){
				u8Byte1 = (U8) MinClamp;
			}
			else {
				u8Byte1 = (U8) i16Pix;
			}
			*pu8Out++ = u8Byte1;

			i16Pix = *pi16In++ + 128;
			if(i16Pix > MaxClamp) {
				u8Byte2 = (U8) MaxClamp;
			}
			else if(i16Pix < MinClamp){
				u8Byte2 = (U8) MinClamp;
			}
			else {
				u8Byte2 = (U8) i16Pix;
			}
			*pu8Out++ = u8Byte2;
		}
		/* Odd number of pixels per row? */
		if ((mi16.NumCols % 2) == 1) {
			/* Pick up the last pixel */
			i16Pix = *pi16In++ + 128;
			if(i16Pix > MaxClamp) {
				u8Byte1 = (U8) MaxClamp;
			}
			else if(i16Pix < MinClamp){
				u8Byte1 = (U8) MinClamp;
			}
			else {
				u8Byte1 = (U8) i16Pix;
			}
			*pu8Out++ = u8Byte1;
		}
	}
	return;
}

/*
 * Add full matrices: A = A + B
 */
void
MatAddFull(MatrixSt matA, MatrixSt matB)
{
	RectSt Rect;
	U32 i,j;
	register PI16 pi16A, pi16B;
	U32 uPitchDiff; 
	
	Rect.r = Rect.c = 0;
	Rect.h = matA.NumRows;
	Rect.w = matA.NumCols;
	uPitchDiff = (matA.Pitch - Rect.w);
	
	pi16A =  (PI16) matA.pi16 ;
	pi16B =  (PI16) matB.pi16 ;
	for (i=0; i< Rect.h; i++) {
		j = Rect.w ;
		while (j--) {
			*pi16A = *pi16A + *pi16B;
			pi16A++; 
			pi16B++;
		}
		pi16A += uPitchDiff ;
		pi16B += uPitchDiff ;
	}
}

U16
MatCksum(MatrixSt mat)
/* compute and return 16 bit checksum of matrix */
{
	I32 iSum = 0;
	PI16 pi16MatDat;
	U32 uRow, uCol;
	
	for (uRow = 0; uRow < mat.NumRows; uRow++) {
		pi16MatDat = (PI16) mat.pi16 + uRow * mat.Pitch;
		for (uCol = 0; uCol < mat.NumCols; uCol++) {
			iSum += *pi16MatDat++;
		}
	}
	
	return((U16) iSum);
}

U16
MatCksumU8(MatrixSt mat)
/* compute and return 16 bit checksum of matrix where
 * the data in the matrix is U8 data.
 */
{
	I32 iSum = 0;
	PU8 pu8MatDat;
	U32 uRow, uCol;
	
	for (uRow = 0; uRow < mat.NumRows; uRow++) {
		pu8MatDat = (PU8) mat.pi16 + uRow * mat.Pitch;
		for (uCol = 0; uCol < mat.NumCols; uCol++) {
			iSum += *pu8MatDat++;
		}
	}
	
	return((U16) iSum);
}

void
MatShiftLeft(MatrixSt mat, I32 Shift)
/* Shift matirx entries Shift bits to the left */
{
	PI16 pi16MatDat;
	U32 uRow, uCol;
	
	for (uRow = 0; uRow < mat.NumRows; uRow++) {
		pi16MatDat = (PI16) mat.pi16 + uRow * mat.Pitch;
		for (uCol = 0; uCol < mat.NumCols; uCol++) {
			*pi16MatDat <<= Shift;			
			pi16MatDat++;
		}
	}
	
	return;
}

void
MatShiftRight(MatrixSt mat, I32 Shift)
/* Shift matrix entries Shift bits to the right */
{
	PI16 pi16MatDat;
	U32 uRow, uCol;
	
	for (uRow = 0; uRow < mat.NumRows; uRow++) {
		pi16MatDat = (PI16) mat.pi16 + uRow * mat.Pitch;
		for (uCol = 0; uCol < mat.NumCols; uCol++) {
			*pi16MatDat >>= Shift;			
			pi16MatDat++;
		}
	}
	
	return;
}

/*
 * Given a matrix and a rectangle, set a new matrix so that it
 * has the rectangle as it's origin.
 *
 * Set the new matrix via the pointer passed in.
 *
 * Assumes that the rectangle is always bounded to the original
 * matrix (i.e., that the rectangle will not fall outside the
 * boundaries of the matrix)
 */
void SetMatrixRect(PMatrixSt pmNew, MatrixSt mOld, RectSt rRect, jmp_buf jbEnv)
{
#if DEBUG
	/*
	 * Validate the assumptions
	 */
	if ((rRect.r + rRect.h) > mOld.NumRows)
		longjmp(jbEnv,  (MATRIX << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_BAD_PARM << TYPE_OFFSET));

	if ((rRect.c + rRect.w) > mOld.NumCols)
		longjmp(jbEnv,  (MATRIX << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_BAD_PARM << TYPE_OFFSET));
	if (mOld.pi16 == NULL)
		longjmp(jbEnv,  (MATRIX << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_BAD_PARM << TYPE_OFFSET));
#endif
	/*
	 * Set the output matrix to the right size and kind
	 */
	pmNew->NumRows = rRect.h;
	pmNew->NumCols = rRect.w;
	pmNew->Pitch = mOld.Pitch;
	
	/*
	 * Set the output matrix to point to the start of the rect data
	 */

	pmNew->pi16 = mOld.pi16 + (rRect.r * mOld.Pitch) + rRect.c;

	return;
}
