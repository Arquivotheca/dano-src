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

/*
 *  pia_dec.h
 *
 *  DESCRIPTION:
 *  PIA decoder include file.  Need to embellish the description a little.
 *	FIX ME
 *
 *  Tabs set to 4
 */

/* This file depends on the following include files:
 * #include "datatype.h"
 * #include "pia_main.h"
 * #ifdef CMD_LINE_DEC
 * #include "CmdLParm.h"
 * #endif
 */

#ifndef __PIA_DEC_H__
#define __PIA_DEC_H__

#if !defined __DATATYPE_H__
#error "DataType.H required for Pia_Dec.H"
#endif

#if !defined __PIA_MAIN_H__
#error "Pia_Main.H required for Pia_Dec.H"
#endif

 /* Private Data Definitions
 * The decoder instance structure includes two pointers to
 * private data structures.
 */
#ifndef PRIVATE_DECODER_DATA_DEFINED
#define PRIVATE_DECODER_DATA_DEFINED
	typedef void FAR * PTR_PRIVATE_DECODER_DATA;
#endif


/* DEC_FRAME_CONTROLS
 * Decoders support a number of different controls.  The decoder controls 
 * are defined as separate flags.
 */
typedef enum {
	DC_CLIPPING							= (1UL<<0),
	DC_TIME_TO_DECODE					= (1UL<<1),
	DC_TRANSPARENCY						= (1UL<<2),
	DC_BRIGHTNESS						= (1UL<<3),
	DC_SATURATION						= (1UL<<4),
	DC_CONTRAST							= (1UL<<5),
	DC_GAMMA							= (1UL<<6),
	DC_FRAME_FLAGS_VALID				= (1UL<<31)
} DEC_FRAME_CONTROLS_LIST;
typedef PIA_ENUM DEC_FRAME_CONTROLS;


/* DEC_SEQUENCE_CONTROLS
 * Decoders support a number of different controls.  The decoder controls 
 * are defined as separate flags.
 */
typedef enum {
	DC_USES_DIFF_FRAMES					= (1UL<<0),
	DC_USES_BI_DIR_DIFF_FRAMES			= (1UL<<1),
	DC_USES_LAST_OUTPUT_BUFFER			= (1UL<<2),
	DC_ACCESS_KEY						= (1UL<<3),
	DC_DONT_DROP_FRAMES					= (1UL<<4),
	DC_DONT_DROP_QUALITY				= (1UL<<5),
	DC_SEQUENCE_FLAGS_VALID				= (1UL<<31)
} DEC_SEQUENCE_CONTROLS_LIST;
typedef PIA_ENUM DEC_SEQUENCE_CONTROLS;

/* DECODER_CONST_INFO
 * The DECODER_CONST_INFO structure contains constant decoder information.
 */
typedef struct {
	SUPPORTED_ALGORITHMS	listInputFormats;
	SUPPORTED_ALGORITHMS	listOutputFormats;

	DEC_FRAME_CONTROLS		dcfcSuggestedFrameControls;
	DEC_FRAME_CONTROLS		dcfcSupportedFrameControls;

	DEC_SEQUENCE_CONTROLS	dcscSuggestedSequenceControls;
	DEC_SEQUENCE_CONTROLS	dcscSupportedSequenceControls;

} DECODER_CONST_INFO;
typedef DECODER_CONST_INFO FAR * PTR_DECODER_CONST_INFO;


/* DECODE_FRAME_INPUT_INFO
 * The DECODE_FRAME_INPUT_INFO structure contains the input parameters that 
 * are only needed when decoding a frame.
 */
typedef struct {
	U32						uTag;
	COLOR_FORMAT			cfInputFormat;			/* Input format */
	PU8						pu8CompressedData;
	U32						uSizeCompressedData;
	U32						uSequenceID;
	I32						iStride;
	PIA_Boolean				bUpdateFrame;		/* Don't decode -- just compose and 
												color convert */
	U32						uStartTime;        /* The start time for decoding this frame
												(from HiveReadTime() initialized by Hive). */
} DECODE_FRAME_INPUT_INFO;
typedef DECODE_FRAME_INPUT_INFO FAR * PTR_DECODE_FRAME_INPUT_INFO;
#define DECODE_FRAME_INPUT_INFO_TAG 0x0E0F1011


/* DECODE_FRAME_OUTPUT_INFO
 * The DECODE_FRAME_OUTPUT_INFO structure contains the output parameters 
 * that are only needed when decoding a frame.
 */
typedef struct {
	U32						uTag;
	COLOR_FORMAT			cfOutputFormat;			/* output format */
	OUTPUT_DESTINATION		odDestination;			/* output destination */
	PIA_Boolean				bOutputTransparencyBitmask;	/* transparency */
	PIA_Boolean				bOutputBoundingRect;		/* transparency */
	PLANAR_IO				pioOutputData;
	PU8						pu8IF09BaseAddress;
	U32						uMaxOutputBufferSize;		
} DECODE_FRAME_OUTPUT_INFO;
typedef DECODE_FRAME_OUTPUT_INFO FAR * PTR_DECODE_FRAME_OUTPUT_INFO;
#define DECODE_FRAME_OUTPUT_INFO_TAG 0x0A0B0C0D


/* decoder persistent data for initialization */
typedef struct {
	PIA_Boolean	bPDUsed;				/* true when the decoder instance has been 
										 * initialized using this persistent data
										 */
	PIA_Boolean	bAccessKeyInBS;			/* True if Access Key in Bit Stream */
#if 0
	PIA_Boolean	bBidir;					/* B frames? */
#endif
	PIA_Boolean	bDeltaFrames;
	PIA_Boolean	bFrameDecoded;			/* true after at least one frame is decoded */
	PIA_Boolean	bTransparency;
	U32			uNLevels;				/* scalable bitstream? (0,1,2); */
	U32			uTileWidth;
	U32			uTileHeight;
} DECODER_PERS_DATA, FAR * PTR_DECODER_PERS_DATA;


/* DEC_INST
 * The DEC_INST structure contains the instance specific data.
 */
typedef struct {
	/* The following are set by the Interface Code */
	U32						uTag;
	PTR_ENVIRONMENT_INFO	peiEnvironment;
	ENVIRONMENT_MODE		eMode;
	DimensionSt				dImageDim;

/*	How long in milliseconds should the CODEC use to decode any frame.. */
	U32						uMaxFrameDuration;

/*	DS Interface - time to decode in milliseconds - instance specific info */
	U32						uExpectedTime;

	/* The following are set by DecodeConstantInit() */
	DECODER_CONST_INFO		dciInfo; 

	/* The following are set b4 calling DecodeQuery() 
	 * or DecodeSequenceSetup() 
	 */

	/* The following are set by the interface before calling DecodeFrame(). */
	DEC_FRAME_CONTROLS		dcfcFrameControlsUsed;
	DEC_FRAME_CONTROLS		dcfcBadFrameControls;

	/* The following are set by the interface before calling DecodeQuery() or 
	   DecodeSequenceSetup() */
	DEC_SEQUENCE_CONTROLS	dcscSequenceControlsUsed;
	DEC_SEQUENCE_CONTROLS	dcscBadSequenceControls;

	/* The following are set by DecodeQuery() or 
	 * DecodeSequenceSetup() 
	 */

	/* The following can be set to provide additional error 
	 * information 
	 */
	U32						uAdditionalErrorInfo1;
	U32						uAdditionalErrorInfo2;

	/* Private Data */
	PTR_DECODER_PERS_DATA	pPersData;	/* pointer to persistent data */
	PTR_PRIVATE_DECODER_DATA pDecodePrivate;

} DEC_INST;
typedef DEC_INST FAR * PTR_DEC_INST;
#define DEC_INST_TAG 0x31393935
#define DEC_PARTIAL_INST_TAG 0x13939353


/***************************************************************************/
/*                                                                         */
/* Decoder Functions                                                       */
/*                                                                         */
/***************************************************************************/


/* standard */
extern PIA_RETURN_STATUS DecodeStartup( void );
extern PIA_RETURN_STATUS DecodeConstantInit( PTR_DEC_INST pInst );
extern PIA_RETURN_STATUS DecodeImageDimInit(
	PTR_DEC_INST					pInst, 
	PTR_DECODE_FRAME_INPUT_INFO		pInput,
	PTR_DECODE_FRAME_OUTPUT_INFO	pOutput
);

extern PIA_RETURN_STATUS DecodeQuery(
	PTR_DEC_INST					pInst, 
	PTR_DECODE_FRAME_INPUT_INFO		pInput,
	PTR_DECODE_FRAME_OUTPUT_INFO	pOutput,
	U32 uHeight,
	U32 uWidth,
	PIA_Boolean bSpeedOverQuality
);

extern PIA_RETURN_STATUS DecodeSequenceSetup(
	PTR_DEC_INST					pInst, 
	PTR_DECODE_FRAME_INPUT_INFO		pInput,
	PTR_DECODE_FRAME_OUTPUT_INFO	pOutput
);

extern PIA_RETURN_STATUS DecodeFrame(
	PTR_DEC_INST					pInst, 
	PTR_DECODE_FRAME_INPUT_INFO		pInput,
	PTR_DECODE_FRAME_OUTPUT_INFO	pOutput
);

extern PIA_RETURN_STATUS DecodeFrameComplete(
	PTR_DEC_INST					pInst
);

extern PIA_RETURN_STATUS DecodeGetCompressedSize(
	COLOR_FORMAT cfInputFormat,
	PU8 pu8CompressedData, 
	PU32 puSize);

extern PIA_RETURN_STATUS DecodeSequenceEnd( PTR_DEC_INST pInst );
extern PIA_RETURN_STATUS DecodeFreePrivateData( PTR_DEC_INST pInst );
extern PIA_RETURN_STATUS DecodeShutdown( void );


/* access key */
extern PIA_RETURN_STATUS	DecodeSetAccessKey( PTR_DEC_INST pInst, U32 uAccessKey );
extern PIA_RETURN_STATUS	DecodeGetAccessKeyStatus( PTR_DEC_INST pInst, PPIA_Boolean pbAccessKeyViolation );

/* bounding rectangle */
extern PIA_RETURN_STATUS	DecodeGetBoundingRectangle( PTR_DEC_INST pInst, PRectSt pRect );

/* Debug */
extern PIA_RETURN_STATUS	DecodeDebugGet( PTR_DEC_INST pInst, PVOID_GLOBAL vgpVoidPtr );
extern PIA_RETURN_STATUS	DecodeDebugSet( PTR_DEC_INST pInst, PVOID_GLOBAL vgpVoidPtr );

/* decode rectangle */
extern PIA_RETURN_STATUS	DecodeGetDefDecodeRect( PTR_DEC_INST pInst, PRectSt pDecodeRect );
extern PIA_RETURN_STATUS	DecodeGetDecodeRect( PTR_DEC_INST pInst, PRectSt pDecodeRect );
extern PIA_RETURN_STATUS	DecodeSetDecodeRect( PTR_DEC_INST pInst, RectSt rDecodeRect );


/* Key frame */
extern PIA_RETURN_STATUS	DecodeIsKeyFrame(
	PTR_DEC_INST pInst, 
	PU8 pu8CompressedData, 
	PPIA_Boolean pbIsKeyFrame
);

/* Persistant data */
extern PIA_RETURN_STATUS	DecodeGetDefPersData(
	PTR_DECODER_PERS_DATA pPersData
);

extern PIA_RETURN_STATUS	DecodeGetPersData(
	PTR_DEC_INST pInst,
	PTR_DECODER_PERS_DATA pPersData
);

extern PIA_RETURN_STATUS	DecodeSetPersData(
	PTR_DECODER_PERS_DATA pInputPersData,
	PTR_DECODER_PERS_DATA pInstPersData
);


/* Real time effects */
extern PIA_RETURN_STATUS	DecodeGetDefContrast( PTR_DEC_INST, PI32 );
extern PIA_RETURN_STATUS	DecodeGetContrast( PTR_DEC_INST, PI32 );
extern PIA_RETURN_STATUS	DecodeSetContrast( PTR_DEC_INST, I32 );

extern PIA_RETURN_STATUS	DecodeGetDefGamma( PTR_DEC_INST, PI32 );
extern PIA_RETURN_STATUS	DecodeGetGamma( PTR_DEC_INST, PI32 );
extern PIA_RETURN_STATUS	DecodeSetGamma( PTR_DEC_INST, I32 );

extern PIA_RETURN_STATUS	DecodeGetDefSaturation( PTR_DEC_INST, PI32 );
extern PIA_RETURN_STATUS	DecodeGetSaturation( PTR_DEC_INST, PI32 );
extern PIA_RETURN_STATUS	DecodeSetSaturation( PTR_DEC_INST, I32 );

extern PIA_RETURN_STATUS	DecodeGetDefBrightness( PTR_DEC_INST, PI32 );
extern PIA_RETURN_STATUS	DecodeGetBrightness( PTR_DEC_INST, PI32 );
extern PIA_RETURN_STATUS	DecodeSetBrightness( PTR_DEC_INST, I32 );

/* view rectangle */
extern PIA_RETURN_STATUS	DecodeSetViewRect( PTR_DEC_INST, RectSt );

/* Transparency kind */
PIA_RETURN_STATUS
DecodeSetTransparencyKind( PTR_DEC_INST, TRANSPARENCY_KIND);


/* Callback to HIVE whihc initializes the persistant data */
extern void HiveInitDecoderPersistentData( PTR_DECODER_PERS_DATA pPersData );

#else  /* __PIA_DEC_H__ */
#error "PIA_DEC.H already included."
#endif
