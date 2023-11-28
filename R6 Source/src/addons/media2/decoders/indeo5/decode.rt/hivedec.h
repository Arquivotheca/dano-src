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
*               Copyright (C) 1994-1997 Intel Corp.                     *
*                         All Rights Reserved.                          *
*                                                                       *
************************************************************************/
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

/*************************************************************************
 * HIVEDEC.H -- declarations and definitions for HIVEDec.c.				 *
 *************************************************************************/

#ifdef __HIVEDEC_H__
#pragma message("hivedec.h: multiple inclusion")
#else /* __HIVEDEC_H__ */
#define __HIVEDEC_H__

/* 
 * This file is dependent on the following include files:
 * pia_main.h
 * indeo5.h, which is dependent on
 *	bsutil.h  (which has further dependencies)
 */

/* Verify inclusion of required files */
#ifndef __RTDEC_H__
#pragma message("hivedec.h requires rtdec.h")
#endif /* __RTDEC_H__ */


#define PALETTE_SIZE 256
#define NON_SYSTEM_ENTRIES 236  /* from VfW -- must follow for all */
#define SKIP_ENTRIES 10         /* 10 system, 236 valid, 10 system */

#define PRIVATE_DECODER_DATA_DEFINED

typedef RTDecInst PRIVATE_DECODER_DATA;
typedef PRIVATE_DECODER_DATA * PTR_PRIVATE_DECODER_DATA;

/* Possible palette types for nPaletteType: */
#define PT_NONE			0	/* no palette defined */
#define PT_FIXED		1	/* standard, fixed palette */
#define PT_CONFIGURABLE	2	/* configurable palette - same as fixed */
							/* except uses different tables */
#define PT_ACTIVE		3	/* active palette */


/* Image height and width must be a multiple of IMAGE_MULTIPLE */
#define IMAGE_MULTIPLE 4

/* legal values for decoder persistent data */
/* nLevels */
#define PD_SCALE0	0			/* no decomposition */
#define PD_SCALE1	1			/* one-level decomposition both horiz & vert */

#ifdef DEBUG
typedef struct _BANDDROPINFO {
		U32 uDebugInfoType;	/* MUST BE 1 - version number that specifies */
							/* this type of structure */
/* Indicate number of bands that are present in current frame */
        U32 fYBands : 3; /* Up to seven. Zero should never happen. */  
        U32 fUBands : 2; /* Up to four. Zero is interpreted as four */
        U32 fVBands : 2; /* Up to four. Zero is interpreted as four */
        U32 fTBands : 1; /* Either it's there or not. */

/* Indicate bands that were decoded in this frame */
        U32 fYDecoded : 7; /* Set bit for each band decoded. */
        U32 fUDecoded : 4;
        U32 fVDecoded : 4;
        U32 fTDecoded : 1; /* Was the transparency mask applied? */
        U32 fUnused   : 8; /* rest of bits */
} BANDDROPINFO, BandDropInfoSt, * pBandDropInfoSt;
#endif /* DEBUG */

#endif /* __HIVEDEC_H__ */
