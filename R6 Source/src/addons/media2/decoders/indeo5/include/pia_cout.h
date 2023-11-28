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
 *
 *  pia_cout.h
 *
 *  DESCRIPTION:
 *  PIA ColorOut include file.
 *	FIX ME
 *
 *  Tabs set to 4
 */

/* This file depends on the following files:
 * #include "datatype.h"
 * #include "pia_main.h"
 */


#ifndef __PIA_COUT_H__
#define __PIA_COUT_H__

#if !defined __DATATYPE_H__
#error "DataType.H required for Pia_COut.H"
#endif

#if !defined __PIA_MAIN_H__
#error "Pia_Main.H required for Pia_COut.H"
#endif

/* Private Data Definitions
 * The output color converter instance structure includes two pointers to 
 * private data structures.
 */
#ifndef PRIVATE_COLOROUT_DATA_DEFINED
#define PRIVATE_COLOROUT_DATA_DEFINED
	typedef void FAR * PTR_PRIVATE_COLOROUT_DATA;
#endif


/* COUT_FRAME_CONTROLS
 * Output color converters support a number of different controls.  The 
 * controls are defined as separate flags.
 */
typedef enum {
	COUT_GET_PALETTE						= (1UL<<0),
	COUT_USE_THIS_PALETTE					= (1UL<<1),
	COUT_USE_FIXED_PALETTE					= (1UL<<2),
	COUT_RESIZING							= (1UL<<3),
	COUT_BOUNDING_RECT						= (1UL<<4),
	COUT_TRANSPARENCY_COLOR					= (1UL<<5),
	COUT_TRANSPARENCY_STREAM_MASK			= (1UL<<6),
	COUT_TRANSPARENCY_FORE_MASK				= (1UL<<7),
	COUT_FRAME_FLAGS_VALID					= (1UL<<31)
} COLOROUT_FRAME_CONTROLS_LIST;
typedef PIA_ENUM COLOROUT_FRAME_CONTROLS;


/* COUT_SEQUENCE_CONTROLS
 * Output color converters support a number of different controls.  The 
 * controls are defined as separate flags.
 */
typedef enum {
	COUT_TOP_DOWN_OUTPUT					= (1UL<<0),
	COUT_BOTTOM_UP_OUTPUT					= (1UL<<1),
	COUT_TRANSPARENCY_KIND					= (1UL<<2),
	COUT_ALT_LINE							= (1UL<<3),
	COUT_SEQUENCE_FLAGS_VALID				= (1UL<<31)
} COLOROUT_SEQUENCE_CONTROLS_LIST;
typedef PIA_ENUM COLOROUT_SEQUENCE_CONTROLS;


/* COLOROUT_CONST_INFO
 * The COLOROUT_CONST_INFO structure contains constant output color converter 
 * information.
 */
typedef struct {
	SUPPORTED_ALGORITHMS		listInputFormats;
	SUPPORTED_ALGORITHMS		listOutputFormats;

	COLOROUT_FRAME_CONTROLS		cofcSuggestedFrameControls;
	COLOROUT_FRAME_CONTROLS		cofcSupportedFrameControls;

	COLOROUT_SEQUENCE_CONTROLS	coscSuggestedSequenceControls;
	COLOROUT_SEQUENCE_CONTROLS	coscSupportedSequenceControls;

} COLOROUT_CONST_INFO;
typedef COLOROUT_CONST_INFO FAR * PTR_COLOROUT_CONST_INFO;


/* COLOROUT_FRAME_INPUT_INFO
 * The COLOROUT_FRAME_INPUT_INFO structure contains the input parameters 
 * that are needed when output color converting a frame.
 */
typedef struct {
	U32						uTag;
	DimensionSt				dInputDim;	  /* dimension of input image */
	COLOR_FORMAT			cfInputFormat;		/* input color format */
	PLANAR_IO				pioInputData;
	U32						uStartTime;		/* Refers to the start time of this frame being decoded.
											 * is assigned by HIVE before the frame is decoded. */
} COLOROUT_FRAME_INPUT_INFO;
typedef COLOROUT_FRAME_INPUT_INFO FAR * PTR_COLOROUT_FRAME_INPUT_INFO;
#define COLOROUT_FRAME_INPUT_INFO_TAG 0x98979695


/* COLOROUT_FRAME_OUTPUT_INFO
 * The COLOROUT_FRAME_OUTPUT_INFO structure contains the input parameters 
 * that are needed when output color converting a frame.
 */
typedef struct {
	U32						uTag;
	/*OUTPUT_GOAL				ogOutputGoal;      Add this at sometime in the future? */
	OUTPUT_DESTINATION		odDestination;		/* output destination */
	COLOR_FORMAT			cfOutputFormat;		/* output color format */
	I32						iOutputStride;
	PU8						pu8OutputData;
	PU32					puBankBoundaries;	/* when output dest is OD_BITMAP_ARRAY */
} COLOROUT_FRAME_OUTPUT_INFO;
typedef COLOROUT_FRAME_OUTPUT_INFO FAR * PTR_COLOROUT_FRAME_OUTPUT_INFO;
#define COLOROUT_FRAME_OUTPUT_INFO_TAG 0x98979695


/* CCOUT_INST
 * The CCOUT_INST structure contains the instance specific data.
 */
typedef struct {
	/* The following are set by the Interface Code */
	U32							uTag;
	PTR_ENVIRONMENT_INFO		peiEnvironment;
	ENVIRONMENT_MODE			eMode;
	DimensionSt					dImageDim;		/* dimension of destination */
	U32						    uMaxFrameDuration; /* How long in milliseconds should the CODEC
													use to decode any frame. */


	/* The following are set by ColorOutConstantInit() */
	COLOROUT_CONST_INFO			cociInfo; 

	/* The following are set before calling ColorOutQuery() 
	 * or ColorOutSequenceSetup() 
	 */
	/* The following are set by the interface before calling ColorOutFrame(). */
	COLOROUT_FRAME_CONTROLS		cofcFrameControlsUsed;
	COLOROUT_FRAME_CONTROLS		cofcBadFrameControls;

	/* The following are set by the interface before calling ColorOutQuery() or 
	   ColorInSequenceSetup() */
	COLOROUT_SEQUENCE_CONTROLS	coscSequenceControlsUsed;
	COLOROUT_SEQUENCE_CONTROLS	coscBadSequenceControls;


	/* The following can be set to provide additional error 
	 * information 
	 */
	U32							uAdditionalErrorInfo1;
	U32							uAdditionalErrorInfo2;

	/* Private Data */
	PTR_PRIVATE_COLOROUT_DATA pColorOutPrivate;
} CCOUT_INST;
typedef CCOUT_INST FAR * PTR_CCOUT_INST;
#define CCOUT_INST_TAG 0x35393931
#define CCOUT_PARTIAL_INST_TAG 0x53939391


/***************************************************************************/
/*                                                                         */
/* ColorOut Functions                                                      */
/*                                                                         */
/***************************************************************************/

extern PIA_RETURN_STATUS ColorOutStartup( void );
extern PIA_RETURN_STATUS ColorOutConstantInit( PTR_CCOUT_INST pInst );
extern PIA_RETURN_STATUS ColorOutImageDimInit(
	PTR_CCOUT_INST pInst, 
	PTR_COLOROUT_FRAME_INPUT_INFO pInput,
	PTR_COLOROUT_FRAME_OUTPUT_INFO pOutput
);

extern PIA_RETURN_STATUS ColorOutQuery(
	PTR_CCOUT_INST pInst, 
	PTR_COLOROUT_FRAME_INPUT_INFO pInput,
	PTR_COLOROUT_FRAME_OUTPUT_INFO pOutput,
	U32 uHeight,   
	U32 uWidth,
	PIA_Boolean bSpeedOverQuality
);

extern PIA_RETURN_STATUS ColorOutSequenceSetup(
	PTR_CCOUT_INST pInst, 
	PTR_COLOROUT_FRAME_INPUT_INFO pInput,
	PTR_COLOROUT_FRAME_OUTPUT_INFO pOutput
);

extern PIA_RETURN_STATUS ColorOutFrame(
	PTR_CCOUT_INST pInst, 
	PTR_COLOROUT_FRAME_INPUT_INFO pInput,
	PTR_COLOROUT_FRAME_OUTPUT_INFO pOutput
);

extern PIA_RETURN_STATUS ColorOutSequenceEnd( PTR_CCOUT_INST pInst );
extern PIA_RETURN_STATUS ColorOutFreePrivateData( PTR_CCOUT_INST pInst );
extern PIA_RETURN_STATUS ColorOutShutdown( void );


/* bounding rectangle */
extern PIA_RETURN_STATUS ColorOutSetBoundingRectangle( PTR_CCOUT_INST pInst, RectSt rBound );


/* Debug */
extern PIA_RETURN_STATUS ColorOutDebugGet( PTR_CCOUT_INST pInst, PVOID_GLOBAL vgpVoidPtr );
extern PIA_RETURN_STATUS ColorOutDebugSet( PTR_CCOUT_INST pInst, PVOID_GLOBAL vgpVoidPtr );


/* COUT_USE_THIS_PALETTE Palette stuff */
extern PIA_RETURN_STATUS ColorOutSetPaletteConfiguration(
	I32 iSetPal,
	I32 iFirst,
	I32 iLast,
	PTR_BGR_ENTRY pPal,
	I32 iSetDither,
	I32 iDither);

extern PIA_RETURN_STATUS ColorOutUseThisPalette(
	PTR_CCOUT_INST pInst, 
	PTR_BGR_PALETTE ppalNewPalette,
	PPIA_Boolean pbFixedPaletteDetected);

/* COUT_GET_PALETTE */
extern PIA_RETURN_STATUS ColorOutGetPalette(
	PTR_CCOUT_INST pInst, 
	PTR_BGR_PALETTE ppalCurrentPalette);

/* COUT_USE_FIXED_PALETTE Palette stuff */
extern PIA_RETURN_STATUS ColorOutUseFixedPalette(
	PTR_CCOUT_INST pInst, 
	PTR_BGR_PALETTE ppalCurrentPalette);

/* transparency mask */
extern PIA_RETURN_STATUS	ColorOutGetDefTransparencyKind( PTR_CCOUT_INST, PTRANSPARENCY_KIND );
extern PIA_RETURN_STATUS	ColorOutGetTransparencyKind( PTR_CCOUT_INST, PTRANSPARENCY_KIND );
extern PIA_RETURN_STATUS	ColorOutSetTransparencyKind( PTR_CCOUT_INST, TRANSPARENCY_KIND );

extern PIA_RETURN_STATUS	ColorOutGetDefViewRect( PTR_CCOUT_INST pInst, PRectSt pViewRect );
extern PIA_RETURN_STATUS	ColorOutGetViewRect( PTR_CCOUT_INST pInst, PRectSt pViewRect );
extern PIA_RETURN_STATUS	ColorOutSetViewRect( PTR_CCOUT_INST pInst, RectSt rViewRect );

#else  /* __PIA_COUT_H__ */
#error "PIA_COUT.H already included."
#endif
