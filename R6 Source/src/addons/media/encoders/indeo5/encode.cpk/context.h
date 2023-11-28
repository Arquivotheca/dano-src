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
#ifdef __CONTEXT_H__
#pragma message("***** CONTEXT.H Included Multiple Times")
#endif
#endif

#ifndef __CONTEXT_H__
#define __CONTEXT_H__

/* This file depends on:

#include <datatype.h>
#include <encoder.h>
#include <enseg.h>
#include <enme.h>

#ifdef SIMULATOR 
#include <encsmdef.h>
#endif

*/


/* 	This structure defines the context of the coder.  There will be one
	context for each color, and additional ones when coding for scalability.
*/
typedef struct _EncBandCntxSt {
	MatrixSt PicRef[2],		/* save for ME */
		 	 PicMc,			/* temp for MC; avoid excess alloc */
			 PicXfrmRes;    /* temp for xfrmed residule image */
	U8 PicRefNum[2];		/* Save pic numbers of reference frames */
	I32 Color;
	PEncSegCntxSt EncSegCntx;
	PEncMeCntxSt EncMeCntx;
	PIA_Boolean McUseBand0, QuantUseBand0;
	PIA_Boolean UseVarQuant;
	pcBandSt Band0Info;	/* Pointer to band 0 info for inheritance */
	I32 Resolution;		/* re ME */
	I32	ErrMeasure;    	/* Error measure used in calculations */
	I32 GlobalQuant;
#ifdef SIMULATOR
	PTR_ENC_SIM_INST pSimInst;  /* Pointer to simulator Instance */
#endif /* SIMULATOR */
} EncBandCntxSt, *PEncBandCntxSt;



/* This is the public header file for the Non-Scalable Wavelets */
/* This structure defines the context of the Wavelet code.  
*/
typedef struct _EncPlaneCntxSt {
	PEncBandCntxSt EncBandCntx[MAX_NUM_BANDS];
	I32 NumBands;	/* total number of bands; min is 1 */
	I32 Color;
	I32 XfrmBands[MAX_NUM_BANDS];	/* transforms for each band */
	I32 NumSubDivCodes;
	MatrixSt OrigBands[MAX_BAND_NESTING_LEVELS][4];       /* Haar Subdiv Workspace */
	MatrixSt XBands[MAX_BAND_NESTING_LEVELS][4];		  /* Reconstructed Bands */
#ifdef SIMULATOR
	PTR_ENC_SIM_INST pSimInst;  /* Pointer to simulator Instance */
#endif /* SIMULATOR */

} EncPlaneCntxSt, *PEncPlaneCntxSt;


#endif
