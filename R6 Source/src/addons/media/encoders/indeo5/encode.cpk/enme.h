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
#ifdef __ENME_H__
#pragma message("***** ENME.H Included Multiple Times")
#endif
#endif

#ifndef __ENME_H__
#define __ENME_H__

/* This is the public header file for EncMe. */

typedef struct _EncMeCntxSt {
	Dbl 	ErrThresh;  /* conclude search if err falls BELOW this */
	I32		Measure;    /* error measure (see below) */
	I32 	MaxDelta;	/*max search range in units of 1/16 pel*/
	I32 	Resolution; /* registration res in units of 1/16 pel */
	I32 	InitStep;   /* initial step size in units of 1/16 pel */
	I32 	Tactic;     /* method of search at full res (see below)*/
	I32 	StepRatio;  /* re "log search" */
	PCMatrixSt pRef;  /* Reference Image (const) */
	PCMatrixSt pTarg; /* Target Image (const) */
#ifdef SIMULATOR
	PTR_ENC_SIM_INST pSimInst;  /* Pointer to simulator Instance */
#endif /* SIMULATOR */
} EncMeCntxSt, *PEncMeCntxSt;

typedef const EncMeCntxSt *PCEncMeCntxSt;

/* PROTOTYPES FOR PUBLIC ROUTINES */

PEncMeCntxSt EncMeOpen(PEncRtParmsSt RtParms, jmp_buf jbEnv);

void EncMeClose(PEncMeCntxSt pMeContext, jmp_buf jbEnv);

void EncMe(
	PEncMeCntxSt pMeContext,
	MatrixSt mPicRef,		/* reference pic */
	MatrixSt mPicOrig,		/* orig pic */
	I32 iNumTiles,			/* Number of tiles */
	pTileSt pTile,			/* Pointer to info tiles */
	jmp_buf jbEnv);			/* environment to return to if error */

Dbl CalcIntraErr(MatrixSt Orig, RectSt rect, I32 measure);

#endif
