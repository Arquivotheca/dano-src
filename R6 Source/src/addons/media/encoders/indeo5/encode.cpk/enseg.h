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
#ifdef __ENSEG_H__
#pragma message("***** ENSEG.H Included Multiple Times")
#endif
#endif


#ifndef __ENSEG_H__
#define __ENSEG_H__


/* This file dependss on :
#include <setjmp.h>
#include <datatype.h>
#include <encoder.h>
#ifdef SIMULATOR
#include <encsmdef.h>
#endif
#include <ensyntax.h>
*/


/* This is the public header file for EncSeg.  It segments a color
plane, determines the quantization and coding method of each segment,
and encodes it.
*/

/* This structure defines the context of the coder.  There will be one
context for each color, and additional ones when coding for scalability.
*/
typedef struct _EncSegCntxSt {
	I32 BlockSize;
	PI32 XfrmBlock;
	PDbl Activity;
	Dbl  AveActivity;
#ifdef SIMULATOR
	PTR_ENC_SIM_INST pSimInst;  /* Pointer to simulator Instance */
#endif /* SIMULATOR */
} EncSegCntxSt, *PEncSegCntxSt;


/* PROTOTYPES FOR PUBLIC ROUTINES */

PEncSegCntxSt EncSegOpen(
	PEncConstInfoSt pCinfo,
	PEncRtParmsSt pRtParms,
	pTileSt	pTileInfo,
	I32 iNumTiles,
	jmp_buf jbEnv);

void EncSegClose(PEncSegCntxSt pSegContext, jmp_buf jbEnv);


/* 
 * Encode pic, tile by tile, rect by rect
 */

void EncSeg(
	pcBandSt pcBandInfo,		/* Header with general band info */
	pTileSt pTileInfo,	
	PEncSegCntxSt pSegContext,
	MatrixSt mPicOrig,			/* orig */
	MatrixSt mPicMc,			/* in: predicted pic */
	MatrixSt mPicXfrmRes,			/* in: xfrmed residule pic */
	MatrixSt mPicX,				/* out: reconstructed pic */
	I32 iNumTiles,
	U8 u8Band0GlobalQuant,		/* Used for Band 0 inheritance */
	U8 u8EncodeMode,
	jmp_buf jbEnv);

#endif
