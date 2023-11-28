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

/* decomp.h : header file for decomp.c 
 * 
 * #include "datatype.h"
 * #include "encsmdef.h"
 */
#ifndef __DECOMP_H__
#define __DECOMP_H__

/* Optimized implementation for #2 wavelet decomposition use the following scaling factors.
 * This set of scaling factors is the set which keeps the typical dynamic 
 * range of	the output band data the same as the input raw data.
 * So this is intended to be used for the best implementation of new wavelet #2.
 */

extern const I16 i16DecompScale[4];

 /* optimized implementation for (5,3) new wavelet (#2): decomposition module */  
void OptWaveletDecomp(MatrixSt matIn, MatrixSt matOut[4], const I16 i16Scale[4]);
#endif
