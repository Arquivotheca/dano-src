 /*************************************************************************
 *                                                                       *
 *              INTEL CORPORATION PROPRIETARY INFORMATION                *
 *                                                                       *
 *      This software is supplied under the terms of a license           *
 *      agreement or nondisclosure agreement with Intel Corporation      *
 *      and may not be copied or disclosed except in accordance          *
 *      with the terms of that agreement.                                *
 *                                                                       *
*************************************************************************
*                                                                       *
*               Copyright (C) 1994-1997 Intel Corp.                       *
*                         All Rights Reserved.                          *
*                                                                       *
************************************************************************/


/*************************************************************************
 * INDEO5.H - This file is meant to be included in every .c file in the 
 * Indeo 5 source base.  It contains structs and constants used widely.
 ************************************************************************/
 
/* The following include files are Windows-specific and will need to
 *  be replaced for other platforms.  They are included here so that they
 *  will not have to be removed from source files individually. 
 ***********************************************************/ 

#ifdef __INDEO5_H__
#pragma message("indeo5.h: multiple inclusion")
#else /* __INDEO5_H__ */
#define __INDEO5_H__

/*sms*/
#define DC_PORT_KIT

/* windows.h is needed for IA decoder only.
 * IA decoder is not used non-simulator builds 
 * and is not used for command line encoder
 */
/*sms*/
/*#if !defined SIMULATOR && !defined CMD_LINE_ENC && !defined BSWALK && !defined __NO_WINDOWS__ */ /* IA decoder compatability */
/*#pragma warning(disable:4005) */ /* redefinition of FAR and NEAR */
/*#include <windows.h> */ /* for HGLOBAL */
/*#endif */ /* !defined SIMULATOR && !defined CMD_LINE_ENC && !defined BSWALK */ /* IA decoder compatability */

/* 
 * This file is dependent on the following include files:
 * #include "pia_main.h"
 * #include "matrix.h"
 * #ifdef SIMULATOR
 * #include "decsmdef.h"
 * #endif
 * #include "bsutil.h"	 (which depends on other files)
 */

/* Verify inclusion of required files */
#ifndef __PIA_MAIN_H__
#pragma message("indeo5.h requires pia_main.h")
#endif /* __PIA_MAIN_H__ */
#ifdef SIMULATOR
#ifndef __DECSMDEF_H__
#pragma message("indeo5.h requires decsmdef.h")
#endif /* __DECSMDEF_H__ */
#endif /* SIMULATOR */
#ifndef __BSUTIL_H__
#pragma message("indeo5.h requires bsutil.h")
#endif /* __BSUTIL_H__ */

/* Remove static keyword from all declarations.
 * This may be useful in debugging since static functions are not 
 * visible outside the module in which they are defined.
 */
#if defined NO_STATICS
#define static 
#endif /* NO_STATICS */

/* This structure defines the context of the coder.  There will be one
** context for each color, and additional ones when coding for scalability.
*/
typedef struct _DecBandCntxSt {
	MatrixSt PicRef[2],		/* save for MC */
			 PicMc;			/* temp for MC; avoid excess alloc */
	I32 Color;
	RectSt rDecodeRect;		/* Rectangle to decode in plane */

#ifdef SIMULATOR /* SIM infrastucture */
	PTR_DEC_SIM_INST pSimInst;  /* Pointer to simulator Instance */
#endif /* SIMULATOR */

} DecBandCntxSt, *pDecBandCntxSt;

#ifdef SIMULATOR /* Multi-level decomposition */
#define MAX_BAND_NESTING_LEVELS 4
#define MAX_NUM_BANDS (1+3*MAX_BAND_NESTING_LEVELS)
#else /* not multi-level decomp */
#define MAX_BAND_NESTING_LEVELS 1
#define MAX_NUM_BANDS 4
#endif /* SIMULATOR */

#define MAXBANDS MAX_NUM_BANDS

/* Information for handling scalability and determining when to 
 * drop more (or fewer) bands 
 */
typedef struct _ScaleCntxSt {
	I32			iYBandPercent[MAX_NUM_BANDS]; /* % of time for each band */
	I32			iVUBandPercent[MAX_NUM_BANDS];
	I32			iComposeNColorConvertPercent; /* % time for color convert */
	I32			iFrameStartPercent;			  /* % time rem. at start of dec */
	I32			iComposeStartPercent;		  
	I32			iScaleLevel;				  /* Specifies amount to scale back */
	I32			iAverageFramePercent;
	I32			iPercentFramesDecoded;
	U32			uNYBands;					  /* Number of Y bands decoded */
} ScaleCntxSt, * pScaleCntxSt;

/* This structure defines the context of the Wavelet code. */
typedef struct _DecPlaneCntxSt {
	pDecBandCntxSt DecBandCntx[MAX_NUM_BANDS];
	I32 iNumBands;
	MatrixSt mOrigBands[MAX_BAND_NESTING_LEVELS][4];       /* Haar Subdiv Workspace */
	MatrixSt mXBands[MAX_BAND_NESTING_LEVELS][4];		  /* Reconstructed Bands */
#ifdef SIMULATOR /* SIM infrastucture */
	PTR_DEC_SIM_INST pSimInst;  /* Pointer to simulator Instance */
#endif /* SIMULATOR */

} DecPlaneCntxSt, *pDecPlaneCntxSt;


/* This structure contains frame information needed in order
   to display an already decoded sequence.
 */
typedef struct _DecFrameInfo {
	MatrixSt 	PicX[3];  		/* Reconstruted picture */	
	MatrixSt	PicTrans;		/* transparency bitmask */
	TransHdrSt	TransHdr;			/* Transparency header */
	TransDscrptSt TransDscrptor;  	/* Transparency band descriptor */
	PIA_Boolean	bIsTransPlane;	/* True if there is a transp. plane */
	I32 		iNumTiles;		/* The number of tiles in the frame */
	I32			iTransTileHeight;	/* Height of a tile in transparency band */
} DecFrameInfoSt, *pDecFrameInfoSt;

/* This structure defines the context of the DecPicture. */
typedef struct _DecPictureCntxSt {
	pDecPlaneCntxSt DecPlaneCntx[3];
	I32 PicNumber;       	/* current picture; counts from 0 */
	PIA_Boolean bKeyLockError;	/* Report an invalid key as an error */
#ifdef SIMULATOR /* SIM infrastucture */
	PTR_DEC_SIM_INST pSimInst;  /* Pointer to simulator Instance */
#endif /* SIMULATOR */

} DecPictureCntxSt, *pDecPictureCntxSt;

/* This is the top level context for the Decoder  */
typedef struct _DecCntxSt {
	PPicBSInfoSt PicInfo;
	pDecPictureCntxSt DecPictureCntx;
	DecFrameInfoSt	PrevFrmInfo;	/* Contains info to display the previous frame */
	BitStrmBuf	BsBuff;
	U32			uTransColor;	/* default 0 */
	PU8			pLDMaskStorage;	/* pointer to private ld mask */
	PU8			pLDMask;		/* pointer to current ld mask */
	PU8			pTranspMask;	/* pointer to private transparency mask */
	U32			uMaskStride;	/* initialized by plane decoder */
	PU8			pMaskStorage;	/* unaligned storage to optimize mask handling*/
	U32			uMaskSize;		/* size of LD & Transp Masks */
	I32			iLastPictureNum;/* Frames before displaying the last held frame */
	PIA_Boolean		bNotInitialized; /* Is the context initialized ? */
	PIA_Boolean     bPicInfoInitialized;  /* Is the GOP Header already initialized? */
	ScaleCntxSt	ScaleCntx;
	/* Conditional Simulator Struct */
#ifdef SIMULATOR /* SIM infrastucture */
	PTR_DEC_SIM_INST pSimInst;  /* Pointer to simulator Instance */
#endif /* SIMULATOR */

} CDecInst, *pCDecInst;

#endif /* __INDEO5_H__ */
