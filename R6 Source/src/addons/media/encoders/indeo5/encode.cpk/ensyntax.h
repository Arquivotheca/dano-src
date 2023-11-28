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
*               Copyright (c) 1994-1997 Intel Corp.				        *
*                         All Rights Reserved.                          *
*                                                                       *
************************************************************************/

#ifdef INCLUDE_NESTING_CHECK
#ifdef 	__ENSYNTAX_H__
#pragma message("*****  ENSYNTAX.H Included Multiple Times")
#endif
#endif

#ifndef __ENSYNTAX_H__
#define __ENSYNTAX_H__

/* This file depends on the following files: 
#include "indeo5.h"
#ifdef SIMULATOR
#include "encsmdef.h"
#endif 
*/

/* Verify inclusion of required files */
#ifndef __INDEO5_H__
#pragma message("ensyntax.h requires indeo5.h")
#endif
#ifdef SIMULATOR
#ifndef __ENCSMDEF_H__
#pragma message("ensyntax.h requires encsmdef.h for simulator build")
#endif
#endif


/*
 * This is the public header file for EncSyntax (which isn't a CS).  It
 * defines the the structure containing the encoders run-time parameters.
 */

typedef struct _EncRtParmsSt {

	Boo Transparency;			/* Compress Tranparency Plane */
	Boo UseTransColor;			/* Encode Transparency Color */
	BGR_ENTRY TransColor;		/* Representative Transparency Color */

	Boo bExpBS;
	
	Boo bYVU12;                 /* TRUE/FALSE: YVU12/YVU9 */
	I32 Frames[2];				/* first and last frame to compress */
	U32 PeriodK, PeriodP;		/* periodicity of pic types */
	U32 PeriodP2;
	I32 MeRange;				/* max vert/horiz search in pels */
	I32 MeSearch[2];			/* tactic; log-search ratio */
	I32 MeResolution, MeInitSs;	/* res, step size - units of MC_UNIT */
	I32 MeTactic, MeRatio;		/* see enme.h for Tactic; log-search */
	I32 MeMeasure;				/* 0=>mse; 1=>mad */
	I32 QuantUseBand0;          /* reuse quant from Y - Band 0 */
	I32 McUseBand0;				/* reuse vectors from Y - band 0 */
	I32 UseVarQuant;			/* Use variable quantization in the bitstream */
	
	U32 McBlk8Sw;
	U32 MeDFiltSw, MePFiltSw;	/* bdr filter for D, P pic - Y only */
	U32 UseRVChange;			/* Use RV change lists */

	I32	iaTileSize[2];			/* Simulates dialog box values for viewport */
	I32 HTMb;					/* Forced Huffman table for MB headers */
	I32 HTBlock;				/* Forced Huffman table for Block headers */
	I32 FRVTbl;					/* Forced run val mapping table */

	I32 DRCmode;   		/* DRC mode. (0,1,2,3,4,5,6) def 0 implies sl88, sl88, sl88, sl88, sl44, sl44 */
	Boo	bShowDRC;		/* Show DRC information in output window */
	I32 MaxBudget;		/* Maximum cycles we can spend on a frame */
	I32 TargetBudget;	/* Target number of cycles to spend on this frame */
	Dbl DeltaTA;		/* Gain constant for DRC reactivity */

	I32 YLevels, VULevels;			/* the # lev; 0 => # bands=1 */
	I32 YNumBands, VUNumBands;		/* the # lev; 0 => # bands=1 */
	U8 YSubDiv[18], VUSubDiv[18];	/* subdivision descriptors - ascii */
	I32 YXfrm[MAX_NUM_BANDS];		/* transforms for Y bands  */
	I32 YQuant[MAX_NUM_BANDS];		/* quant table sets for Y bands  */
	I32 VUXfrm[MAX_NUM_BANDS];     	/* transforms for UV bands */
	I32 VUQuant[MAX_NUM_BANDS];     /* quant table sets for UV bands */
	I32 YMBSize[MAX_NUM_BANDS];		/* macroblock sizes for Y bands  */
	I32 VUMBSize[MAX_NUM_BANDS];	/* macroblock sizes for UV bands */
	I32 YBlockSize[MAX_NUM_BANDS];	/* block sizes for Y bands  */
	I32 VUBlockSize[MAX_NUM_BANDS];	/* block sizes for UV bands */
	
	I32 FrameTime;		/* FrameTime from input video stream 
						 * to obtain usec per frame, 
						 * divide by uFrameScale 
						 */

	I32 FrameScale;		/* Scale from input video stream */

/* BRC parameters */
	U32 KByteRate;		/* bitrate in KBytes per sec */
	U32 KByteRatio; 	/* K bit ratio */
	U32 PByteRatio; 	/* P bit ratio */
	U32 DByteRatio; 	/* D bit ratio */
	U32 P2ByteRatio; 	/* P2 bit ratio */
	I32 MaxBuffer;		/* max buffer size for BRC in bytes */
	I32 BRCReactPos;	/* BRC Positive reactivity on a scale of 256 */
	I32 BRCReactNeg;	/* BRC Negative reactivity on a scale of 256 */
	U32 BRCOn;			/* switch on BRC */
	I32 GlobalQuant;	/* global quant used for all bands if BRC is off */

#ifdef SIMULATOR

	PTR_ENC_SIM_INST pSimInst;  /* Pointer to simulator Instance */

#endif /* SIMULATOR */

} EncRtParmsSt, *PEncRtParmsSt;

typedef const EncRtParmsSt *PCEncRtParmsSt;

#endif
