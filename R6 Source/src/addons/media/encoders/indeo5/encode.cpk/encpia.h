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
#ifdef __ENCPIA_H__
#pragma message("***** ENCPIA.H Included Multiple Times")
#endif
#endif

#ifndef __ENCPIA_H__
#define __ENCPIA_H__

/* This file depends on the following header files:
 * #include <setjmp.h>
 * #include <datatype.h>
 * #include <mc.h>
 * #include <hufftbls.h>
 * #include <qnttbls.h>
 * #include <mbhuftbl.h>
 * #include <bkhuftbl.h>
 * #include <bsutil.h>
 * #include <ensyntax.h>
 * #include <entrans.h>
 * #include <encoder.h>
 * #include <enseg.h>
 * #include <enme.h>
 * #include <context.h>
 * #include <enntry.h>
 * #ifdef SIMULATOR 
 * #include "encsmdef.h"
 * #endif
 */
/* Verify inclusion of required files */
#ifndef __DATATYPE_H__
#pragma message("encpia.h requires datatype.h")
#endif /* __DATATYPE_H__ */
#ifndef __MC_H__
#pragma message("encpia.h requires mc.h")
#endif /* __MC_H__ */
#ifndef __HUFFTBLS_H__
#pragma message("encpia.h requires hufftbls.h")
#endif /* __HUFFTBLS_H__ */
#ifndef __BKHUFTBL_H__
#pragma message("encpia.h requires bkhuftbl.h")
#endif /* __BKHUFTBL_H__ */
#ifndef __BSUTIL_H__
#pragma message("encpia.h requires bsutil.h")
#endif /* __BSUTIL_H__ */
#ifndef __ENSYNTAX_H__
#pragma message("encpia.h requires ensyntax.h")
#endif /* __ENSYNTAX_H__ */
#ifndef __ENTRANS_H__
#pragma message("encpia.h requires entrans.h")
#endif /* __ENTRANS_H__ */
#ifndef __ENCODER_H__
#pragma message("encpia.h requires encoder.h")
#endif /* __ENCODER_H__ */
#ifndef __ENSEG_H__
#pragma message("encpia.h requires enseg.h")
#endif /* __ENSEG_H__ */
#ifndef __ENME_H__
#pragma message("encpia.h requires enme.h")
#endif /* __ENME_H__ */
#ifndef __ENNTRY_H__
#pragma message("encpia.h requires enntry.h")
#endif /* __ENNTRY_H__ */
#ifndef __CONTEXT_H__
#pragma message("encpia.h requires context.h")
#endif /* __CONTEXT_H__ */

/* If this is a simulator build */

#ifdef SIMULATOR
#ifndef __ENCSMDEF_H__
#pragma message("encpia.h requires encsmdef.h")
#endif /* __ENCSMDEF_H__ */



#else

typedef void * PTR_ENC_SIM_INST;   /* Otherwise define this type as pointer to void */

#endif

#define PRIVATE_ENCODER_DATA_DEFINED  /* For HIVE Code, to prevent redefintion there */

	/* The compressor strategy routine's intermediate storage structure, also known as
	 * "Compressor Strategy Buffers", or CSB's.
	 */
	typedef struct {
		MatrixSt		  CSBPicture[3];	/* picture planar data & dimensions */
		MatrixSt          CSBReference[3];	/* picture before it was filtered */
		BitStrmBuf 		  CSBBitStream;		/* the result of compressing the pic */
		MatrixSt		  CSBRebuiltPic[3];	/* the reconstituted image of this pic */
		PU8		  		  pTransBM;			/* the transparency bitmask for this frame */
 		I32				  nFrameType;		/* type of bitstream frame the pic was compressed into */
	} CmpStrategyStorageSt, *PCmpStrategyStorageSt;

	typedef struct {
	  /* The following data can be passed in/out through HIVE */
		U32						uAccessKey; 
		PIA_Boolean				bUseTransColor;
		BGR_ENTRY				bgrTransparencyColor;	/* color bitmask transparency */
		PTR_TRANSPARENCY_MASK   pTransparencyMask;
		DimensionSt				dViewportDim;			/* decode viewport */
		U32						uScaleLevel;
	} PassDownEncParmsSt, *pPassDownEncParmsSt;

	typedef PassDownEncParmsSt * PTR_PASS_DOWN_ENC_PARMS;

	/* Persistent context for the compression strategy routines */

	typedef struct {

	/* output Buffer to store compressed data: owned by encoder module
	 * The pointer is passed back to the pOutputSt in the PIA level 
	 */
/*		PU8						pu8CompressedData; */

	    PEncRtParmsSt	 		pRtParms;		/* Compressor runtime initialization parameters */
	    PEncPictureCntxSt		pPicContext;	/* Nested contexts for lower level routines */

	  /* Global Encoding Data */
		NaturalInt				nKPeriodCntr;	/* Count frames since last frame of type X */
		NaturalInt				nPicNumber;		/* recv'd seq # of cur pic (counts from 0) is display-order-index */
		NaturalUnsigned			uSequencerState;

	  /* Planar dimensional data for current sequence - NEVER has any pixel data buffers */
 		MatrixSt				PicDimensions[3];

	  /* Compressor Strategy intermediate buffers */

		CmpStrategyStorageSt	StrategyBuf;	/* Compression strategy buffer */
		U32						TransBMSize;	/* Transparency Bitmask Size */
		PIA_Boolean				bFirstTime;		/* First time through SequenceSetup */

		/* Simulator Data Structure */	
		/* These need to compile all of the time!!!  No conditionals here or there */		 
		/* could be linking problems.  These are needed for the handshake over simulation. */
		U32 uSimDataTag;		   		/* Indicates that following pointer is valid. */
		PTR_ENC_SIM_INST pEncSimInst;   /* Pointer to SimInst struct when above tag is set to */
										/* SIM_DATA_TAG */																										

	} EncoderPrivateData, *PEncoderPrivateData;

	typedef EncoderPrivateData * PTR_PRIVATE_ENCODER_DATA;

#include "imagedef.h"

#endif  /* __ENCPIA_H__  */
