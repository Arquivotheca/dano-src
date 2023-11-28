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
#ifdef __ENNTRY_H__
#pragma message("***** ENNTRY.H Included Multiple Times")
#endif
#endif

#ifndef __ENNTRY_H__
#define __ENNTRY_H__

/* This file is dependent on the following:
 * mbhuftbl.h
 * bkhuftbl.h
 * context.h
 */
#ifndef __DATATYPE_H__
#pragma message("enntry.h requires datatype.h")
#endif /* __DATATYPE_H__ */
#ifndef __MBHUFTBL_H__
#pragma message("enntry.h requires mbhuftbl.h")
#endif /* __MBHUFTBL_H__ */
#ifndef __BKHUFTBL_H__
#pragma message("enntry.h requires bkhuftbl.h")
#endif /* __BKHUFTBL_H__ */
#ifndef __CONTEXT_H__
#pragma message("enntry.h requires context.h")
#endif /* __CONTEXT_H__ */

/* This structure defines the context of the Encoder Picture Compressor.  
*/
typedef struct _EncPictureCntxSt {

	/* Linkages to vars at outer levels */
	PMatrixSt	    pPicDimensions;		/* (in Strategy ctxt) Just holds dimensions- no plane data ever */
	PMatrixSt	    pLeadingRef;		/* (in Strategy ctxt) earlier reference pic */
	PMatrixSt	    pTrailingRef;		/* (in Strategy ctxt) later reference pic */
	PI32			pnGlobalQuant;		/* (in RtParms) */
	PU32			pnFrameAudioOverhead;

	PEncPlaneCntxSt EncPlaneCntx[3];	/* contexts for each plane */
	PEncTransCntxSt pEncTransCntx;		/* Context for transparency plane */
	PicBSInfoSt		FrameBSInfo;		/* info for the current bitstream being built */

	/* BRC parameters */
	I32 MaxBuffer;
	U32 BytesPerFrame;
	I32 BRCReactPos;
	I32 BRCReactNeg;
	I32 KByteRate;
	I32 GlobalByteBankFullness;
	U32 KByteRatio, PByteRatio, DByteRatio;
	U32 P2ByteRatio, BytesPerP2, P2PerGop, P2LeftPerGop;
	U32 BytesPerK, BytesPerP, BytesPerD;
	U32 PPerGop, DPerGop;
	I32 PLeftPerGop, DLeftPerGop;
	PIA_Boolean BRCOn; /* switch off brc */
	/* Conditional Simulator Struct */
#ifdef SIMULATOR
	PTR_ENC_SIM_INST pSimInst;  /* Pointer to simulator Instance */
#endif /* SIMULATOR */
} EncPictureCntxSt, *PEncPictureCntxSt;

#endif
