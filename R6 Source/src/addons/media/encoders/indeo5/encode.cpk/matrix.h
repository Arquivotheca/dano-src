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

#ifdef INCLUDE_NESTING_CHECK
#ifdef __MATRIX_H__
#pragma message("***** MATRIX.H Included Multiple Times")
#endif
#endif

#ifndef __MATRIX_H__
#define __MATRIX_H__

/*
 * 
 * DESCRIPTION:
 * This file contains the structure declarations, flag-bit
 * definitions, error code specification, and public function prototypes
 * relevant to the various operation routines found in tools/matrix.c.
 * 
 * $Header:   I:\proj50\src\encode\vcs\matrix.h_v   1.4   03 Jul 1997 16:25:30   preger  $
 * 
 */

/* 
 * This file is dependent on these include files:
 * datatype.h
 * <setjmp.h>
 */

/*
 * Function Prototypes
 */

/*
 * Allocate/init storage for matrix data.
 * This is a special matrix allocation routine for Indeo 5.  It
 * adds a padding line at both the top and the bottom of the image.
 * This is to remove special case handling when encoding images
 * to bitstreams.
 * Both dimensions are rounded up to a multiple of 8. This
 * prevents overflow when low res images are expanded up. The
 * problem arises because they had been extended to be a mult of 4
 * for VQ.
 */
#ifdef MEMDEBUG

void MatAllocFunc(const PChr File, I32 Line, PMatrixSt pMat, jmp_buf jbEnv);
#define MatAlloc(x,y) MatAllocFunc(__FILE__, __LINE__, (x), (y))

#else

void MatAllocFunc(PMatrixSt pMat, jmp_buf jbEnv);
#define MatAlloc(x,y) MatAllocFunc((x),(y))

#endif /* MEMDEBUG */

/*
 * This Free frees the padded matrix structure allocated by MatAlloc()
 * as described above.
 */
#ifdef MEMDEBUG

void MatFreeFunc(const PChr File, I32 Line, MatrixSt matrix, jmp_buf jbEnv);
#define MatFree(x,y) MatFreeFunc(__FILE__, __LINE__, (x),(y))

#else

void MatFreeFunc(MatrixSt matrix, jmp_buf jbEnv);
#define MatFree(x,y) MatFreeFunc((x),(y))

#endif

/*
 * Sets a rectangular area of matrix Out to value.  The actual
 * setting is done by memcpy. Currently, this function does not
 * support datatype conversion of any type.
 */
void MatSet(MatrixSt mOut, RectSt R, PI16 pi16Value, jmp_buf jbEnv);

/*
 * Sets entire matrix to value. 
 */
void MatSetFull(MatrixSt Matrix, PI16 pi16Value, jmp_buf jbEnv);

void MatSetFast(MatrixSt Matrix, PI16 pi16Value, jmp_buf jbEnv);

/*
 * Compute and return 16 bit checksum of matrix mat
 */
U16 MatCksum(MatrixSt mat);

/*
 * Compute and return 16 bit checksum of matrix where the matrix data 
 * is U8 data.
 */
U16 MatCksumU8(MatrixSt mat);

/*
 * Shift all matrix entries by Shift bits
 */
void
MatShiftLeft(MatrixSt mat, I32 Shift);
void
MatShiftRight(MatrixSt mat, I32 Shift);

#ifdef SIMULATOR
void EncSimMatShiftRight(MatrixSt mPicX, I16 i16Shift);	
#endif /* SIMULATOR */

/*
 * Converts I16->U8 returns TRUE if clamping needed to be done
 */
void
MatConvU8 (MatrixSt mi16);

/*
 * Adds matrices. Rectagular area contains the matrix.
 */
void MatAddFull(MatrixSt matA, MatrixSt matB);

/* 
 * Copies a rectangular area from matrix In to matrix Out.  
 */
void MatCopy(MatrixSt mIn, MatrixSt mOut, RectSt R, jmp_buf jbEnv);

/*
 * Copy an entire Matrix from In to Out.
 */
void MatCopyFull(MatrixSt mIn, MatrixSt mOut, jmp_buf jbEnv); 

void MatCopyFast(MatrixSt mIn, MatrixSt mOut, jmp_buf jbEnv); 
 
/*
 * Given a matrix and a rectangle, set a new matrix so that it
 * has the rectanle as it's origin.
 */
void SetMatrixRect(PMatrixSt pMnew, MatrixSt Mold, RectSt rRect, jmp_buf jbEnv);

#endif
