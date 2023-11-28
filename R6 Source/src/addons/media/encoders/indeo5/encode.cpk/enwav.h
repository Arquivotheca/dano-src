/***********************************************************************
*                                                                      *
*              INTEL CORPORATION PROPRIETARY INFORMATION               *
*                                                                      *
*   This listing is supplied under the terms of a license agreement    *
*     with INTEL Corporation and may not be copied nor disclosed       *
*       except in accordance with the terms of that agreement.         *
*                                                                      *
************************************************************************
*                                                                      *
*             Copyright (C) 1994-1997 Intel Corp.                       *
*                        All Rights Reserved.                          *
*                                                                      *
***********************************************************************/

#ifdef INCLUDE_NESTING_CHECK
#ifdef __ENPLANE_H__
#pragma message ("***** ENPLANE.H Included Multiple Times")
#endif
#endif

#ifndef __ENPLANE_H__
#define __ENPLANE_H__

/* PROTOTYPES FOR PUBLIC WAVELET ROUTINES */

/* Initialize the sequence and return ptr to WavCntxSt. */

PEncPlaneCntxSt EncPlaneOpen(
	pPlaneSt pBsPlaneInfo,
	pPicHdrSt pPicHdr,
	PEncConstInfoSt pCInfo,
	PEncRtParmsSt pParms,
	ppBandSt ppBand0,
	jmp_buf jbEnv);

/* End the sequence; free any allocated storage
*/
void EncPlaneClose (PEncPlaneCntxSt pPlaneContext, 
					pPlaneSt pBsPlaneInfo, jmp_buf jbEnv);


/* Encode pic: replaces predicted pic by reconstructed pic
*/
void EncPlane(
	PTR_ENC_INST pInst,
	PCPicHdrSt pcPicInfoHdr, /* Header with general pic info */
	pPlaneSt pBsPlaneInfo,
	PEncPlaneCntxSt pPlaneContext,
	MatrixSt mPicOrig,	/* orig */
	MatrixSt mPicX, 	/* in: predicted pic; out: reconstructed pic */
	U8 u8EncodeMode, /* mode to use to encode this plane (this is for multipass brc) */
	jmp_buf jbEnv);

#endif  /* __ENPLANE_H__ */
