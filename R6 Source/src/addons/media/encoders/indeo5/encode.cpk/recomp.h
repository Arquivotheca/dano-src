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

/* recomp.h: header file for recomp.c  
 * 
 * #include "datatype.h"
 */
#ifndef __RECOMP_H__
#define __RECOMP_H__

/* Optimized implementation for #2 wavelet recomposition use the scaling factors {(4,2,2,1)/16 }.
 * This set of scaling factors is the set which keeps the typical dynamic 
 * range of	the output band data the same as the input raw data.
 * So this is intended to be used for the best implementation of new wavelet #2.
 */
extern const I16 i16RecompScale[4];

 /* optimized implementation for (5,3) new wavelet (#2): recomposition module */  
void OptWaveletRecomp(MatrixSt matIn[4], MatrixSt matOut, const I16 i16Scale[4]);
#endif
