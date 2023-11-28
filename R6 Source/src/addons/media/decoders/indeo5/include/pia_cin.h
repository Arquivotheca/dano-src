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
 *  pia_cin.h
 *
 *  DESCRIPTION:
 *  HIVE ColorIn converter include file
 *	FIX ME
 *
 *  Tabs set to 4
 */

/* This file depends on the following files:
 * #include "datatype.h"
 * #include "pia_main.h"
 */

#ifndef __PIA_CIN_H__
#define __PIA_CIN_H__

#if !defined __DATATYPE_H__
#error "DataType.H required for Pia_CIn.H"
#endif

#if !defined __PIA_MAIN_H__
#error "Pia_Main.H required for Pia_CIn.H"
#endif

/* Private Data Definitions
 * The output color converter instance structure includes two pointers to 
 * private data structures.
 */
#ifndef PRIVATE_COLORIN_DATA_DEFINED
#define PRIVATE_COLORIN_DATA_DEFINED
	typedef void FAR * PTR_PRIVATE_COLORIN_DATA;
	typedef void FAR * PTR_PASS_DOWN_COLORIN_PARMS;
#endif


/* COLORIN_FRAME_CONTROLS
 * The input color converter frame controls are defined as separate flags.
 * These flags can be set by HIVE for any 
 */
typedef enum {
	CIN_COPYRIGHT					= (1UL<<1),
	CIN_FRAME_CONTROLS_VALID		= (1UL<<31)
} COLORIN_FRAME_CONTROLS_LIST;
typedef PIA_ENUM COLORIN_FRAME_CONTROLS;


/* COLORIN_SEQUENCE_CONTROLS
 * The input color converter controls are defined as separate flags.
 */
typedef enum {
	CIN_TRANSPARENCY				= (1UL<<0),	/* transparency is enabled 
												 *	for the sequence
												 */
	CIN_SEQUENCE_CONTROLS_VALID		= (1UL<<31)

} COLORIN_SEQUENCE_CONTROLS_LIST;
typedef PIA_ENUM COLORIN_SEQUENCE_CONTROLS;


/* COLORIN_CONST_INFO
 * The COLORIN_CONST_INFO structure defines the static input color converter 
 * values.
 */
typedef struct {
	SUPPORTED_ALGORITHMS		listInputFormats;
	SUPPORTED_ALGORITHMS		listOutputFormats;

	COLORIN_FRAME_CONTROLS		cifcSuggestedFrameControls;
	COLORIN_FRAME_CONTROLS		cifcSupportedFrameControls;

	COLORIN_SEQUENCE_CONTROLS	ciscSuggestedSequenceControls;
	COLORIN_SEQUENCE_CONTROLS	ciscSupportedSequenceControls;
} COLORIN_CONST_INFO;
typedef COLORIN_CONST_INFO FAR * PTR_COLORIN_CONST_INFO;


/* COLORIN_INPUT_INFO
 * The COLORIN_INPUT_INFO structure contains input fields used in 
 * ColorInFrame().
 */
typedef struct {
	U32				uTag;
	U32				uFrameNumber;
	COLOR_FORMAT	cfSrcFormat;
	PIA_Boolean		bFlipImage;
	/* Move to: PLANAR_IO		pioInputData; in the future? */
	I32				iInputStride;
	PU8				pu8InputData;
	PTR_BGR_PALETTE	ppalCurrentPalette;
} COLORIN_INPUT_INFO;
typedef COLORIN_INPUT_INFO FAR * PTR_COLORIN_INPUT_INFO;
#define COLORIN_INPUT_INFO_TAG 0x19181716


/* COLORIN_OUTPUT_INFO
 * The COLORIN_OUTPUT_INFO structure contains output fields used in 
 * ColorInFrame().
 */
typedef struct {
	U32				uTag;
	U32				uOutputSize;
	COLOR_FORMAT	cfOutputFormat;
	/* Move to: PLANAR_IO		pioOutputData; in the future? */
	I32				iOutputStride;
	PU8				pu8OutputData;     /* If the colorin component allocates the output buffer,
										  it assigns pu8OutputData to the allocated buffer on the heap. */
	PU8				pu8ExternalOutputData; /* Else if HIVE owns the output buffer (from the environment),
											  Hive assigns a value to pu8ExternalOutputData */
} COLORIN_OUTPUT_INFO;
typedef COLORIN_OUTPUT_INFO FAR * PTR_COLORIN_OUTPUT_INFO;
#define COLORIN_OUTPUT_INFO_TAG 0x21222324


/* CCIN_INST
 * The CCIN_INST structure defines the instance specific input color 
 * converter data.
 */
typedef struct {
	/* The following are set by the Interface Code */
	U32							uTag;
	PTR_ENVIRONMENT_INFO		peiEnvironment;
	ENVIRONMENT_MODE			eMode;
	DimensionSt					dImageDim;
	U32							uFrameTime;		/* to obtain usec per frame, */
												/* divide by uFrameScale */
	U32							uFrameScale;	/* value used to scale uFrameTime */
												/* to obtain usec per frame */
									
	/* The following are set by ColorInConstantInit() */
	COLORIN_CONST_INFO			ciciInfo;

	/* The following are set by the interface before calling ColorInFrame(). */
	COLORIN_FRAME_CONTROLS		cifcFrameControlsUsed;
	COLORIN_FRAME_CONTROLS		cifcBadFrameControls;

	/* The following are set by the interface before calling ColorInQuery() or 
	   ColorInSequenceSetup() */
	COLORIN_SEQUENCE_CONTROLS	ciscSequenceControlsUsed;
	COLORIN_SEQUENCE_CONTROLS	ciscBadSequenceControls;

	/* The following can be set to provide additional error 
	 * information 
	 */
	U32							uAdditionalErrorInfo1;
	U32							uAdditionalErrorInfo2;

	/* private data structures */
	PTR_PASS_DOWN_COLORIN_PARMS  pColorInParms;
	PTR_PRIVATE_COLORIN_DATA	pColorInPrivate;
} CCIN_INST;
typedef CCIN_INST FAR * PTR_CCIN_INST;
#define CCIN_INST_TAG	0x86753090
#define CCIN_PARTIAL_INST_TAG 0x68570309

/***************************************************************************/
/*                                                                         */
/* PIA ColorIn Functions                                                   */
/*                                                                         */
/***************************************************************************/
extern PIA_RETURN_STATUS ColorInStartup( void );
extern PIA_RETURN_STATUS ColorInConstantInit( PTR_CCIN_INST pInst );
extern PIA_RETURN_STATUS ColorInImageDimInit(
	PTR_CCIN_INST				pInst,
	PTR_COLORIN_INPUT_INFO		pInput, 
	PTR_COLORIN_OUTPUT_INFO		pOutput
);

extern PIA_RETURN_STATUS ColorInGetMaxOutputBuffSize(
	COLOR_FORMAT    cfFormat,
	U32				uHeight, 
	U32				uWidth,
	PU32			puSize
);

extern PIA_RETURN_STATUS ColorInQuery( 
	PTR_CCIN_INST				pInst, 
	PTR_COLORIN_INPUT_INFO		pInput, 
	PTR_COLORIN_OUTPUT_INFO		pOutput,
	U32							uHeight,
	U32							uWidth,
	PIA_Boolean					bSpeedOverQuality
);

extern PIA_RETURN_STATUS ColorInSequenceSetup(
	PTR_CCIN_INST				pInst,
	PTR_COLORIN_INPUT_INFO		pInput, 
	PTR_COLORIN_OUTPUT_INFO		pOutput
);

extern PIA_RETURN_STATUS ColorInFrame(
	PTR_CCIN_INST				pInst, 
	PTR_COLORIN_INPUT_INFO		pInput, 
	PTR_COLORIN_OUTPUT_INFO		pOutput
);

extern PIA_RETURN_STATUS ColorInSequenceEnd( PTR_CCIN_INST pInst );
extern PIA_RETURN_STATUS ColorInFreePrivateData( PTR_CCIN_INST pInst ); 
extern PIA_RETURN_STATUS ColorInShutdown( void );


/* Debug */
extern PIA_RETURN_STATUS ColorInDebugGet( PTR_CCIN_INST pInst, PVOID_GLOBAL vgpVoidPtr );
extern PIA_RETURN_STATUS ColorInDebugSet( PTR_CCIN_INST pInst, PVOID_GLOBAL vgpVoidPtr );

/*	For transparency operation, there are four mutually exclusive methods of
 *	specifing the transparent pixels:
 *		- Alpha channel
 *		- Color range from first frame analysis
 *		- User supplied color range  and
 *		- User supplied bitmask
 */

/* Transparency input - alpha channel */
extern PIA_RETURN_STATUS ColorInSetTransAlphaChannel( PTR_CCIN_INST pInst );

/* Transparency input - color range from first frame
 * The calculated color range can be obtained by calling
 * ColorInGetTransColorRange() after the first frame has been processed.
 */
extern PIA_RETURN_STATUS ColorInSetCalcTransColorRange( PTR_CCIN_INST pInst );

/* Transparency input - user supplied color range */
extern PIA_RETURN_STATUS ColorInGetDefTransColorRange( PTR_CCIN_INST pInst, PTR_COLOR_RANGE pcrRange );
extern PIA_RETURN_STATUS ColorInGetTransColorRange( PTR_CCIN_INST pInst, PTR_COLOR_RANGE pcrRange );
extern PIA_RETURN_STATUS ColorInSetTransColorRange( PTR_CCIN_INST pInst, COLOR_RANGE crRange );

/* transparency input/output - user supplied bitmask */
extern PIA_RETURN_STATUS ColorInGetTransMask( PTR_CCIN_INST pInst, PTR_PTR_TRANSPARENCY_MASK ppTransMask);
extern PIA_RETURN_STATUS ColorInSetTransMask( PTR_CCIN_INST pInst, PTR_TRANSPARENCY_MASK pTransMask);
extern PIA_RETURN_STATUS ColorInGetTransMaskSize( PTR_CCIN_INST pInst, PU32 puSize );

/* transparency output - representative color */
extern PIA_RETURN_STATUS ColorInGetRepresentativeColor( PTR_CCIN_INST pInst, PTR_BGR_ENTRY pbgrColor );

#if defined (BETA_BANNER)
/* Copyright bitmap */
extern PMatrixSt HiveGetCopyrightBits( PTR_CCIN_INST pInst );
#endif


#else  /* __PIA_CIN_H__ */
#error "PIA_CIN.H already included."
#endif /* __PIA_CIN_H__ */


