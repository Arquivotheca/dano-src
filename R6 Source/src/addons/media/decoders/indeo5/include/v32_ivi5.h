/*************************************************************************
 *                                                                       *
 *              INTEL CORPORATION PROPRIETARY INFORMATION                *
 *                                                                       *
 *      This software is supplied under the terms of a license           *
 *      agreement or nondisclosure agreement with Intel Corporation      *
 *      and may not be copied or disclosed except in accordance          *
 *      with the terms of that agreement.                                *
 *                                                                       *
 *************************************************************************/
/*************************************************************************/
/*                                                                       */
/* Copyright (C) 1994-1997 Intel Corp. All Rights Reserved.              */
/*                                                                       */
/*************************************************************************/

/*
 *  Tabs set to 4
 *
 *  v32_ivi5.h
 *
 *  DESCRIPTION:
 *  The IVI5 include file for 32 bit Intel Architecture platforms.
 *
 *  DEPENDENCIES: pia_cin.h pia_cout.h pia_dec.h pia_enc.h
 */

/* $Id$
 */

#ifndef __V32_IVI5_H__
#define __V32_IVI5_H__

/* This file is dependent on the following files:
#include "pia_main.h"
#include "pia_cin.h"
#include "pia_cout.h"
#include "pia_dec.h"
#include "pia_enc.h"
*/

/* Verify inclusion of required files */
#ifndef __PIA_MAIN_H__
#pragma message("v32_ivi5.h requires pia_main.h")
#endif
#if ! defined( __PIA_CIN_H__ ) && ! defined( DECODE_ONLY )
#pragma message("v32_ivi5.h requires pia_cin.h")
#endif
#ifndef __PIA_COUT_H__
#pragma message("v32_ivi5.h requires pia_cout.h")
#endif
#ifndef __PIA_DEC_H__
#pragma message("v32_ivi5.h requires pia_dec.h")
#endif
#if ! defined( __PIA_CIN_H__ ) && ! defined( DECODE_ONLY )
#pragma message("v32_ivi5.h requires pia_enc.h")
#endif

/* All OE_* are defined in VfW_Spec.H.  The 16-bit version of the code should 
 * be modified to OE_16.  This is not in VfW_Spec.H because we do not want to
 * depend on the users setting WIN32 define.
 */
#define HOST_ENVIRONMENT	OE_32

typedef ICINFO* 		PTR_ICINFO;
typedef ICOPEN* 		PTR_ICOPEN;
typedef ICCOMPRESS* 		PTR_ICCOMPRESS;
typedef ICCOMPRESSFRAMES* 	PTR_ICCOMPRESSFRAMES;

/* Define a platform independent FCC macro name */
/* The WIN32 defintion follows, add other platform */
/* defintions in the future */

#if defined (QTW)
	/* This unsigned 32-bit integer must be compatible with Apple's QT
	 * identifiers.
	 */
	#define PIA_FOURCC( ch3, ch2, ch1, ch0 )							\
			( (U32)(U8)(ch0) << 0    | ( (U32)(U8)(ch1) << 8 ) |	\
			( (U32)(U8)(ch2) << 16 ) | ( (U32)(U8)(ch3) << 24 ) )
#else	// WIN32
	#define PIA_FOURCC mmioFOURCC
#endif /* #if defined (QTW) */


/* all Indeo FOURCC_xxxx should be Upper Case */

#define FOURCC_IV50 PIA_FOURCC('I','V','5','0')
#define FOURCC_1632 PIA_FOURCC('1','6','3','2')
#define FOURCC_IF09 PIA_FOURCC('I','F','0','9')
#define FOURCC_YVU9 PIA_FOURCC('Y','V','U','9')
#define FOURCC_YUY2 PIA_FOURCC('Y','U','Y','2')
#define FOURCC_YVU12 PIA_FOURCC('I','4','2','0')
#define FOURCC_CLPL PIA_FOURCC('C','L','P','L')	/* *y, *u, *v surface pointer */
#define FOURCC_I420 PIA_FOURCC('I','4','2','0') /* Old name for IYUV */
#define FOURCC_IYUV PIA_FOURCC('I','Y','U','V')	/* Y then U then V 12b planar */
#define FOURCC_YV12 PIA_FOURCC('Y','V','1','2')	/* Y then V then U 12b planar */
#define FOURCC_YUYV PIA_FOURCC('Y','U','Y','V')	/* synonym for YUY2 */
#define FOURCC_Y211 PIA_FOURCC('Y','2','1','1')	/* synonym for YUY2 */
#define FOURCC_UYVY PIA_FOURCC('U','Y','V','Y')	/* opossite endian 422 */

/*
 * This definition of BI_BITMAP is here because MS has not provided
 * a "standard" definition. When MS does provide it, it will likely be
 * in vfw.h. At that time this definition should be removed.
 */
#define BI_BITMAP PIA_FOURCC('B', 'I', 'T', 'M')

/* Indeo TWOCC definition */
#define TWOCC_IV41 aviTWOCC('i','v')

#define ABS(a) ((a) < 0 ? (-(a)) : (a))
#ifdef QTW
/* The following macros are for QTW */
#define RND4(x) (((x)+3) & ~3)
/* clip to min 32x32 */
#define MIN32(x) ((x) < 32 ? (32) : (x))
#endif

/* What modules are in use during compression / decompression?  Colorin? Encoder? Decoder? ColorOut Converter?  Both? */
#define MODULE_FLAGS_COLORIN_IN   (1<<0)  /* We know from the input  parameter, the COLORIN  MOD will be used. */
#define MODULE_FLAGS_COLORIN_OUT  (1<<1)  /* We know from the output parameter, the COLORIN  MOD will be used. */
#define MODULE_FLAGS_ENCODER_IN   (1<<2)  /* We know from the input  parameter, the ENCODER  MOD will be used. */
#define MODULE_FLAGS_ENCODER_OUT  (1<<3)  /* We know from the output parameter, the ENCODER  MOD will be used. */
#define MODULE_FLAGS_DECODER_IN   (1<<4)  /* We know from the input  parameter, the DECODER  MOD will be used. */
#define MODULE_FLAGS_DECODER_OUT  (1<<5)  /* We know from the output parameter, the DECODER  MOD will be used. */
#define MODULE_FLAGS_COLOROUT_IN  (1<<6)  /* We know from the input  parameter, the COLOROUT MOD will be used. */
#define MODULE_FLAGS_COLOROUT_OUT (1<<7)  /* We know from the output parameter, the COLOROUT MOD will be used. */
#define MODULE_FLAGS_NONE         (0)

#define COLORIN_MODULE_IN(PI)   ((PI)->uModFlags & MODULE_FLAGS_COLORIN_IN)
#define COLORIN_MODULE_OUT(PI)  ((PI)->uModFlags & MODULE_FLAGS_COLORIN_OUT)
#define ENCODER_MODULE_IN(PI)   ((PI)->uModFlags & MODULE_FLAGS_ENCODER_IN)
#define ENCODER_MODULE_OUT(PI)  ((PI)->uModFlags & MODULE_FLAGS_ENCODER_OUT)
#define DECODER_MODULE_IN(PI)   ((PI)->uModFlags & MODULE_FLAGS_DECODER_IN)
#define DECODER_MODULE_OUT(PI)  ((PI)->uModFlags & MODULE_FLAGS_DECODER_OUT)
#define COLOROUT_MODULE_IN(PI)  ((PI)->uModFlags & MODULE_FLAGS_COLOROUT_IN)
#define COLOROUT_MODULE_OUT(PI) ((PI)->uModFlags & MODULE_FLAGS_COLOROUT_OUT)

#define COLORIN_MODULE_ACTIVE(PI)  ((PI)->uModFlags & (MODULE_FLAGS_COLORIN_IN | MODULE_FLAGS_COLORIN_OUT))
#define ENCODER_MODULE_ACTIVE(PI)  ((PI)->uModFlags & (MODULE_FLAGS_ENCODER_IN | MODULE_FLAGS_ENCODER_OUT))
#define DECODER_MODULE_ACTIVE(PI)  ((PI)->uModFlags & (MODULE_FLAGS_DECODER_IN  | MODULE_FLAGS_DECODER_OUT))
#define COLOROUT_MODULE_ACTIVE(PI) ((PI)->uModFlags & (MODULE_FLAGS_COLOROUT_IN | MODULE_FLAGS_COLOROUT_OUT))

/* 
 * This is the INST_INFO struct: all of the other instance structs
 * are members of this struct. The uFlags member is used to copy the 
 * flags that come in from the ICINFO struct when we are opened.
 */

typedef struct {
	U8              uModFlags;
	U32             uFlags;		/* flags from ICINFO struct */
	U32		bFatalError;/* flag if instance error */
	I32		mtType;		/* ICM_GETCODECSTATE queue - 16-bit apps only */
	U32		fccHandler; /* From icinfo->fccHandler @ time of DRV_OPEN */
	U32		bQCChecked; /* CoInitialize done? (checking for QC) */
	U32		bRTEAvailable; /* Is the RTE available to be used? */
	U32             bUseRTE;    /* Are we using QC as RTE? */
#ifdef QTW
/*  The following is for QTW. A ComponentInstance is really U32 and is 
 *  defined as such to avoid including QTW definitions in VFW/AM/DS codec.
 *  In QTW code, we must cast as used. Ugly, but it allows QTW code to 
 *  use this data structure as is.
 */
//  ComponentInstance	delegateComponent;	
	U32		delegateComponent;	/* ComponentInstance of QTW Base Codec */
#endif	// QTW
	ENVIRONMENT_INFO                     eiEnvironment;
#ifndef DECODE_ONLY
	PTR_PRIVATE_INTERFACE_COLOR_IN_DATA  pInterfaceColorInPrivate;
	PTR_PRIVATE_INTERFACE_ENCODER_DATA   pInterfaceEncodePrivate;
#endif /* #ifndef DECODE_ONLY */
	PTR_PRIVATE_INTERFACE_DECODER_DATA   pInterfaceDecodePrivate; /* private interface decoder data. */
	PTR_PRIVATE_INTERFACE_COLOR_OUT_DATA pInterfaceColorOutPrivate;
} INST_INFO;
typedef INST_INFO* PTR_INST_INFO;


#endif /* ___V32_IVI5_H__ */
