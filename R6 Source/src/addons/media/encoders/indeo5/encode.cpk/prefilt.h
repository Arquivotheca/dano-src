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
#ifdef __PREFILT_H__
#pragma message("***** PREFILT.H Included Multiple Times")
#endif
#endif

#ifndef __PREFILT_H__
#define __PREFILT_H__

/* Header file for PREFILT.c */

#ifdef SIMULATOR /* The spatial prefilter is under simulator build only */

/* Function to do the spatial prefiltering. */
void SpatPreFilter(
	MatrixSt mat, /* image plane on which filtering is to be carried out */
	I32 iQuant,		/* Current quantization level */
	I32 iQuantMin,	/* Minimum quantization level */
	I32 iQuantMax);	/* Maximum quantization level */

#endif /* SIMULATOR */

/* Function to do image temporal prefiltering */
void TempPreFilter(
	MatrixSt mPicOrig,	    /* image plane on which filtering is to be carried out */
	MatrixSt mLastPicOrig,	/* image plane on which filtering is to be carried out */
	I32      iPlane);       /* Image Plane Number */
#endif /* __PREFILT_H__ */
