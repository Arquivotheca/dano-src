/*
 * @(#)csmatrix.h	1.18 97/12/22
				This module contains private definitions required
				by the color space to transform software
				Created by PGT, Nov 1, 1993
				From RFP prototype on Sun

	Written by:	The Kodak CMS MS Windows Team

	Copyright:	(C) 1993-1994 by Eastman Kodak Company, all rights reserved.

*/

/**********************************************************************
 **********************************************************************
 **********************************************************************
 *** COPYRIGHT (c) Eastman Kodak Company, 1993                      ***
 *** As  an unpublished  work pursuant to Title 17 of the United    ***
 *** States Code.  All rights reserved.                             ***
 **********************************************************************
 **********************************************************************
 **********************************************************************/


#if !defined(CSMATRIX_H)
#define CSMATRIX_H

#include <math.h>
#include "matdata.h"


/* Convenient Notations */

#ifndef NULL
#define NULL	(0)
#endif

#define ALLOC(number,size)	(allocBufferPtr((int32)((number)*(size))))

#define	DALLOC(ptr)	(freeBufferPtr((KpChar_p)(ptr)),\
				(ptr) = NULL)	/* deallocate memory */

/* Private Function Prototypes */

double f4l ARGS((double x, double xtab[], double ytab[], int n, int *hint));
double f4lFPU ARGS((double x, double xtab[], double ytab[], int n, int *hint));
void calcGtbl3 ARGS((fut_gtbldat_ptr_t*, int32*, double**,bool));
void calcItbl1 ARGS((fut_itbldat_ptr_t, int32, double));
void calcItbl256 ARGS((fut_itbldat_ptr_t, int32, KpUInt16_p));
PTErr_t calcItblN ARGS((fut_itbldat_ptr_t table, int32 gridSize, ResponseRecord* rrp, KpUInt32_t interpMode));
void calcOtbl0 ARGS((fut_otbldat_ptr_t));
void calcOtbl1 ARGS((fut_otbldat_ptr_t, double));
PTErr_t calcOtblN ARGS((fut_otbldat_ptr_t table, ResponseRecord* rrp, KpUInt32_t interpMode));
double calcInvertTRC ARGS((double p, KpUInt16_p data, KpUInt32_t length));
int32 solvemat ARGS((int, double**, double*));

void calcGtbl3noFPU ARGS((fut_gtbldat_ptr_t*, int32*, double**,bool));
void calcItbl1noFPU ARGS((fut_itbldat_ptr_t, int32, double));
void calcItbl256noFPU ARGS((fut_itbldat_ptr_t, int32, KpUInt16_p));
PTErr_t calcItblNnoFPU ARGS((fut_itbldat_ptr_t table, int32 gridSize, ResponseRecord* rrp, KpUInt32_t interpMode));
void calcOtbl0noFPU ARGS((fut_otbldat_ptr_t));
void calcOtbl1noFPU ARGS((fut_otbldat_ptr_t, double));
PTErr_t calcOtblNnoFPU ARGS((fut_otbldat_ptr_t table, ResponseRecord* rrp, KpUInt32_t interpMode));
double calcInvertTRCnoFPU ARGS((double p, KpUInt16_p data, KpUInt32_t length));

void calcGtbl3FPU ARGS((fut_gtbldat_ptr_t*, int32*, double**,bool));
void calcItbl1FPU ARGS((fut_itbldat_ptr_t, int32, double));
void calcItbl256FPU ARGS((fut_itbldat_ptr_t, int32, KpUInt16_p));
PTErr_t calcItblNFPU ARGS((fut_itbldat_ptr_t table, int32 gridSize, ResponseRecord* rrp, KpUInt32_t interpMode));
void calcOtbl0FPU ARGS((fut_otbldat_ptr_t));
void calcOtbl1FPU ARGS((fut_otbldat_ptr_t, double));
PTErr_t calcOtblNFPU ARGS((fut_otbldat_ptr_t table, ResponseRecord* rrp, KpUInt32_t interpMode));
double calcInvertTRCFPU ARGS((double p, KpUInt16_p data, KpUInt32_t length));

#if defined(KCP_FPU)			/* using the FPU? */
_float_eval log10fpu(_float_eval _x); 
_float_eval powfpu(_float_eval _x,_float_eval _y);
#endif	/* #if defined(KCP_FPU) */

#endif

