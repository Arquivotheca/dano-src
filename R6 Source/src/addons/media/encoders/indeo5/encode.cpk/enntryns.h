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
#ifdef __ENCBAND_H__
#pragma message("***** ENCBAND.H Included Multiple Times")
#endif
#endif

#ifndef __ENCBAND_H__
#define __ENCBAND_H__


/* 	This is the public header file for EncBand.  It encodes 1 color
	plane of a sequence (or of a level or band of a sequence).
*/

/* 	PROTOTYPES FOR PUBLIC ROUTINES */

/* 	Initialize the sequence and return ptr to EncBandCntxSt. 
*/
PEncBandCntxSt EncBandOpen(
	PEncConstInfoSt pCInfo,
	PEncRtParmsSt pRtParms,
	pBandSt pBsBandInfo,
	ppBandSt ppBand0Info,
	PIA_Boolean bIsExpBS,
	jmp_buf jbEnv);

/* End the sequence; free any allocated storage
*/
void EncBandClose ( PEncBandCntxSt pBandContext,  
					pBandSt pBsBandInfo, jmp_buf jbEnv);

/* Encode target pic and return reconstructed pic.
This is the entry point for the (non-scalable) coding of a pic plane.
*/
void EncBand(
	PTR_ENC_INST pInst,
	PCPicHdrSt pcPicInfoHdr, /* Header with general pic info */
	pBandSt pBsBandInfo,
	PEncBandCntxSt pBandContext,
	MatrixSt mPicOrig,	/* orig */
	MatrixSt mPicX,		/* Reconstructed Band */
	U8 u8EncodeMode,
	jmp_buf jbEnv);

#endif   /* __ENCBAND_H__  */

