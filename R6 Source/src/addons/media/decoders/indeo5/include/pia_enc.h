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
 *  pia_enc.h
 *
 *  DESCRIPTION:
 *  PIA encoder include file  Included by the RTE and QC, and the offline encoder.
 *	FIX ME
 *  ?  I feel the description needs to be embellished.  But, I am not sure what
 *  other information should go here.
 *
 *  Tabs set to 4
 */

/* This file depends on the following include files:
 * #include "datatype.h"
 * #include "pia_main.h"
 * #ifdef CMD_LINE_ENC
 * #include "CmdLParm.h"
 * #endif
 */

#ifndef __PIA_ENC_H__
#define __PIA_ENC_H__

#if !defined __DATATYPE_H__
#error "DataType.H required for Pia_Enc.H"
#endif

#if !defined __PIA_MAIN_H__
#error "Pia_Main.H required for Pia_Enc.H"
#endif

#ifdef CMD_LINE_ENC
#if !defined __CMDLPARM_H__
#error "CmdLParm.H required for Pia_Enc.H"
#endif
#endif /* CMD_LINE_ENC */


#include "irte.h"

 /* Private Data Definitions
 * The encoder instance structure includes two pointers to private data 
 * structures.
 */
#ifndef PRIVATE_ENCODER_DATA_DEFINED
#define PRIVATE_ENCODER_DATA_DEFINED
	typedef void FAR * PTR_PRIVATE_ENCODER_DATA;
	typedef void FAR * PTR_PASS_DOWN_ENC_PARMS;
#endif


/* PROGRESS_CALL_TYPE
 * The PROGRESS_CALL_TYPE enumeration is used in the encoder progress call 
 * back function.
 */
typedef enum {
	PCT_UNDEFINED,
	PCT_STATUS,
	PCT_YIELD,
	PCT_START,
	PCT_END,
	MAX_PCT
} PROGRESS_CALL_TYPE_LIST;
typedef PIA_ENUM PROGRESS_CALL_TYPE;


/* ENC_FRAME_CONTROLS
 * Encoders support a number of different controls.  The encoder controls 
 * are defined as separate flags.
 */
typedef enum {
	EC_FORCE_KEY_FRAME					= (1UL<<0),	/* Instruct the encoder to
													 * produce a key frame as 
													 * soon as possible
													 */
	EC_FRAME_CONTROLS_VALID				= (1UL<<31)
} ENC_FRAME_CONTROLS_LIST;
typedef PIA_ENUM ENC_FRAME_CONTROLS;


/* ENC_SEQUENCE_CONTROLS
 * Encoders support a number of different controls.  The encoder controls 
 * are defined as separate flags.
 */
typedef enum {
	EC_TARGET_DATA_RATE					= (1UL<<0),
	EC_ABSOLUTE_QUALITY_SLIDER			= (1UL<<1),
	EC_CDROM_PADDING					= (1UL<<2),
	EC_VIEWPORT							= (1UL<<3),
	EC_TRANSPARENCY						= (1UL<<4),
#if 0
	EC_TARGET_PLAYBACK_PLATFORM			= (1UL<<5),
	EC_BI_DIRECTIONAL_PREDICTION		= (1UL<<6),
#endif
	EC_LOOK_AHEAD_COMPRESSION			= (1UL<<7),
	EC_SCALABILITY						= (1UL<<8),
	EC_ACCESS_KEY						= (1UL<<9),
	EC_FLAGS_VALID						= (1UL<<31)
} ENC_SEQUENCE_CONTROLS_LIST;
typedef PIA_ENUM ENC_SEQUENCE_CONTROLS;

#define MAX_NUMBER_OF_VIEWRECT_SIZES 8

/* SUPPORTED_VIEWRECT_SIZES
 * The SUPPORTED_VIEWRECT_SIZES structure defines which VIEWRECT SIZES the
 * encoder supports.  The viewrects are assumed to be square.  A size of 0 means
 * 'full image'.  Any other size is a width and a height (e.g. an entry of value
 * 64 refers to the viewrect 64x64).
 */
typedef struct {
	U16				u16NumberOfViewrectSizes;
	U16				eList[MAX_NUMBER_OF_VIEWRECT_SIZES];
} SUPPORTED_VIEWRECT_SIZES;

/* ENCODER_CONST_INFO
 * The ENCODER_CONST_INFO structure contains constant encoder information.
 */
typedef struct {
	SUPPORTED_ALGORITHMS	listInputFormats;
	SUPPORTED_ALGORITHMS	listOutputFormats;

	SUPPORTED_VIEWRECT_SIZES svs_list;

	KEY_FRAME_RATE_KIND		keySuggestedKeyFrameRateKind;
	U32						uSuggestedKeyFrameRate;
	U32						uSuggestedTargetDataRate;
	U32						uSuggestedAbsoluteQuality;

	ENC_FRAME_CONTROLS		ecfcSuggestedFrameControls;
	ENC_FRAME_CONTROLS		ecfcSupportedFrameControls;

	ENC_SEQUENCE_CONTROLS	ecscSuggestedSequenceControls;
	ENC_SEQUENCE_CONTROLS	ecscSupportedSequenceControls;

} ENCODER_CONST_INFO;
typedef ENCODER_CONST_INFO FAR * PTR_ENCODER_CONST_INFO;


/* ENCODE_FRAME_INPUT_INFO
 * The ENCODE_FRAME_INPUT_INFO structures contains input fields that are 
 * used when encoding a frame.
 */
typedef struct {
	U32					uTag;
	U32					uFrameNumber;
	U32					uAbsoluteQuality;		/* absolute quality: is the linear range 0 -> 10000
												   with 0 meaning 0%, and 10000 meaning 100% */
	U32					uOverheadSize;			/* audio data rate */
	COLOR_FORMAT		cfSrcFormat;			/* input color format */
	/* Move to: PLANAR_IO pioUncompressedData in the future? */
	I32					iStride;
	PU8					pu8UncompressedData;
} ENCODE_FRAME_INPUT_INFO;
typedef ENCODE_FRAME_INPUT_INFO FAR * PTR_ENCODE_FRAME_INPUT_INFO;
#define ENCODE_FRAME_INPUT_INFO_TAG	0x45464949


/* ENCODE_FRAME_OUTPUT_INFO
 * The ENCODE_FRAME_INFO structures contains the output fields that are 
 * used when encoding a frame.
 */
typedef struct {
	U32					uTag;
	U32					uFrameSize;
	PIA_Boolean			bIsKeyFrame;
	COLOR_FORMAT		cfOutputFormat;
	PU8					pu8CompressedData;
} ENCODE_FRAME_OUTPUT_INFO;
typedef ENCODE_FRAME_OUTPUT_INFO FAR * PTR_ENCODE_FRAME_OUTPUT_INFO;
#define ENCODE_FRAME_OUTPUT_INFO_TAG 0x45464F49

/* QUICK_COMPRESS_INST
 * The QUICK_COMPRESS_INST structure contains the data specific to the 
 * quick compressor, including pointers into the QC .dll for the compression
 * functions.
 */
typedef struct {
	IVidConfigure	* pICfg;	/* configuration functions */
	IVidCompress	* pIComp;	/* Functions to handle compression */
	U32	fQuickCompress;	/* Use the quick compressor? */
	U32	fScalability;	/* Encode with scalability enabled? */
	int	ciToken;	/* A token provided/used by quick compressor */
} QUICK_COMPRESS_INST;
typedef QUICK_COMPRESS_INST * PTR_QUICK_COMPRESS_INST;

/* ENC_INST
 * The ENC_INST structure contains the instance specific data.
 */
typedef struct {
	/* The following are set by the Interface Code */
	U32						uTag;
	PTR_ENVIRONMENT_INFO	peiEnvironment;
	ENVIRONMENT_MODE		eMode;
	DimensionSt				dImageDim;
	U32						uFrameTime;				/* to obtain usec per frame, */ 
													/* divide by uFrameScale */
	U32						uFrameScale;			/* value used to scale uFrameTime */
													/* to obtain usec per frame */

	/* The following are set by EncodeConstantInit() */
	ENCODER_CONST_INFO		eciInfo; 

	/* The following are set before calling EncodeFrame() */
	ENC_FRAME_CONTROLS		ecfcFrameControlsUsed;
	ENC_FRAME_CONTROLS		ecfcBadFrameControls;

	/* The following are set before calling EncodeQuery() or 
	 * EncodeSequenceSetup()
	 */
	KEY_FRAME_RATE_KIND		keyKeyFrameRateKind;	/* key frame rate kind */
	U32						uKeyFrameRate;			/* key frame rate value */
	U32						uTargetDataRate;		/* data rate - bytes per second */

	ENC_SEQUENCE_CONTROLS	ecscSequenceControlsUsed;
	ENC_SEQUENCE_CONTROLS	ecscBadSequenceControls;
 
	/* The following can be set to provide additional error 
	 * information.
	 */
	U32						uAdditionalErrorInfo1;
	U32						uAdditionalErrorInfo2;

	/* Private data structures */
	PTR_PASS_DOWN_ENC_PARMS  pEncParms;
	PTR_PRIVATE_ENCODER_DATA pEncoderPrivate;

	/* Data for the Quick Compressor, if present */
	PTR_QUICK_COMPRESS_INST pQCInst;
	
#ifdef CMD_LINE_ENC  /* for command line encoder only */
	PTR_EncCmdLineParms_St pParms;
#endif /* CMD_LINE_ENC */

} ENC_INST;
typedef ENC_INST FAR * PTR_ENC_INST;
#define ENC_INST_TAG	0xdeadbeef
#define ENC_PARTIAL_INST_TAG 0xeddaebfe


/***************************************************************************/
/*                                                                         */
/* Encoder Functions                                                       */
/*                                                                         */
/***************************************************************************/

extern PIA_RETURN_STATUS EncodeStartup( void );
extern PIA_RETURN_STATUS EncodeConstantInit( PTR_ENC_INST pInst );

extern PIA_RETURN_STATUS EncodeImageDimInit(
	PTR_ENC_INST					pInst,
	PTR_ENCODE_FRAME_INPUT_INFO		pInput, 
	PTR_ENCODE_FRAME_OUTPUT_INFO	pOutput
);

extern PIA_RETURN_STATUS EncodeGetMaxCompressedSize(
	U32				uHeight, 
	U32				uWidth,
	PU32			puSize
);
extern PIA_RETURN_STATUS EncodeQuery(
	PTR_ENC_INST					pInst,
	PTR_ENCODE_FRAME_INPUT_INFO		pInput, 
	PTR_ENCODE_FRAME_OUTPUT_INFO	pOutput,
	U32								uHeight,
	U32								uWidth,
	PIA_Boolean						bSpeedOverQuality
);
extern PIA_RETURN_STATUS EncodeSequenceSetup(
	PTR_ENC_INST					pInst,
	PTR_ENCODE_FRAME_INPUT_INFO		pInput, 
	PTR_ENCODE_FRAME_OUTPUT_INFO	pOutput
);

extern PIA_RETURN_STATUS EncodeFrame(
	PTR_ENC_INST					pInst, 
	PTR_ENCODE_FRAME_INPUT_INFO		pInput, 
	PTR_ENCODE_FRAME_OUTPUT_INFO	pOutput
);

extern PIA_RETURN_STATUS EncodeSequenceEnd( PTR_ENC_INST pInst );
extern PIA_RETURN_STATUS EncodeFreePrivateData( PTR_ENC_INST pInst );
extern PIA_RETURN_STATUS EncodeShutdown( void );

/* access key */
extern PIA_RETURN_STATUS EncodeGetDefAccessKey( PTR_ENC_INST pInst, PU32 puAccessKey );
extern PIA_RETURN_STATUS EncodeGetAccessKey( PTR_ENC_INST pInst,  PU32 puAccessKey );
extern PIA_RETURN_STATUS EncodeSetAccessKey( PTR_ENC_INST pInst,  U32 uAccessKey );

/* Debug */
extern PIA_RETURN_STATUS EncodeDebugGet( PTR_ENC_INST pInst, PVOID_GLOBAL vgpVoidPtr );
extern PIA_RETURN_STATUS EncodeDebugSet( PTR_ENC_INST pInst, PVOID_GLOBAL vgpVoidPtr );

/* key frame */
/*//extern PIA_RETURN_STATUS EncodeSetMustBeKeyFrame( PTR_ENC_INST pInst );*/

/* scalability */
extern PIA_RETURN_STATUS EncodeGetDefScaleLevel( PTR_ENC_INST pInst, PU32 puScaleLevel );
extern PIA_RETURN_STATUS EncodeGetScaleLevel( PTR_ENC_INST pInst, PU32 puScaleLevel );
extern PIA_RETURN_STATUS EncodeSetScaleLevel( PTR_ENC_INST pInst, U32 uScaleLevel  );

/* transparency bitmask */
extern PIA_RETURN_STATUS EncodeSetTransBitmask( PTR_ENC_INST pInst, PTR_TRANSPARENCY_MASK pTransMask );

/* transparency color */
extern PIA_RETURN_STATUS EncodeSetTransColor( PTR_ENC_INST pInst, BGR_ENTRY bgrColor );

/* target playforms */
extern PIA_RETURN_STATUS EncodeGetDefTargetPlaybackPlatform(PTR_ENC_INST pInst,PU32 puTargetPlatform);
extern PIA_RETURN_STATUS EncodeGetTargetPlaybackPlatform(PTR_ENC_INST pInst,PU32 puTargetPlatform);
extern PIA_RETURN_STATUS EncodeSetTargetPlaybackPlatform(PTR_ENC_INST pInst,U32 uTargetPlatform); 

/* view port for local decode */
extern PIA_RETURN_STATUS EncodeGetDefViewPort( PTR_ENC_INST pInst, PDimensionSt pdViewPort );
extern PIA_RETURN_STATUS EncodeGetMinViewPort( PTR_ENC_INST pInst, PDimensionSt pdViewPort );
extern PIA_RETURN_STATUS EncodeGetViewPort( PTR_ENC_INST pInst, PDimensionSt pdViewPort );
extern PIA_RETURN_STATUS EncodeSetViewPort( PTR_ENC_INST pInst, DimensionSt dViewPort );

/**********************************************************************
 * H I V E  C A L L B A C K  F U N C T I O N S  F O R  E N C O D E R. *
 **********************************************************************/
/* The callback functions defined using reserved names
 */

extern PIA_RETURN_STATUS	HiveEncodeProgressFunc(
		PTR_ENC_INST		pEncInst,
		PROGRESS_CALL_TYPE	pctType,
		U32					uPercentDone
);
	/* Called by encoders to inform the application layer how far the encode
	 * has progressed
	 */


#else  /* __PIA_ENC_ */
#error "PIA_ENC.H already included."
#endif /*  __PIA_ENC_H__ */
