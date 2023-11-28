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
#ifdef 	__ENTRANS_H__
#pragma message("*****  ENTRANS.H Included Multiple Times")
#endif
#endif

#ifndef __ENTRANS_H__
#define __ENTRANS_H__

/* This structure defines the context of the Encoder Transparency Compressor. */

typedef struct _EncTransCntxSt {

	I32			TransWidth;		/* Width of Transparency Band */
	I32			OrigWidth;		/* Original Width of Image */
	I32			TransHeight;	/* Height of Transparency Band */
	I32			TileWidth;		/* Width of tiles in transparency band */
	I32			TileHeight;		/* Height of tiles in transparency band */
	I32			NumTilesWide;	/* Number of tiles across transparency band */
	I32			NumTilesHigh;	/* Number of tiles down transparency band */
	PU8			pu8TransByteMask;		/* Transparency ByteMap, 8bpp */
#ifdef SIMULATOR
	PTR_ENC_SIM_INST pSimInst;  /* Pointer to simulator Instance */
#endif /* SIMULATOR */	
} EncTransCntxSt, *PEncTransCntxSt;

#endif /* __ENTRANS_H__ */

/* PROTOTYPES FOR PUBLIC ROUTINES */

PEncTransCntxSt EncTransOpen(PTransSt *ppTransBand,
							 pPicHdrSt pPicInfoHdr, 
							 PEncRtParmsSt pRtParms,
							 jmp_buf jbEnv);

void EncTransClose(PEncTransCntxSt pTransContext,
				   PTransSt pTransBand,
				   jmp_buf jbEnv);

void EncTrans(PEncTransCntxSt pTransContext,
			  PTransSt pTransBand,
			  PU8 pTransBM,
			  U8 EncodeMode,
			  jmp_buf jbEnv);
