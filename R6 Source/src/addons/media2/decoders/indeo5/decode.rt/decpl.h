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
 * DECPL.H -- function declarations and definitions needed for processing
 * the picture layer.
 *************************************************************************/

#ifdef __DECPL_H__
#pragma message("decpl.h: multiple inclusion")
#else /* __DECPL_H__ */
#define __DECPL_H__

#ifndef __RTDEC_H__
#pragma message("decpl.h requires rtdec.h")
#endif /* __RTDEC_H__ */

#define PL_CODE_TRANSP		1
#define PL_CODE_EXPRMNTL	2
#define PL_FLAG_DID_DECODE	0x8000000
#define PL_FLAG_DID_DISPLAY	0x4000000

#define PIC_START_CODE	0x1f		/* 1 1111 */

#define PICTYPE_K	0	/* Key */
#define PICTYPE_P	1	/* Predicted */
#define PICTYPE_P2	2	/* 2nd level Predicted */
#define PICTYPE_D	3	/* Disposable */
#define PICTYPE_R	4	/* Repeat - usually caused by brc */
#define PICTYPE_RESERVED	5	/* reserved for future */
#define PICTYPE_V	7	/* access key violation */

/*	GOP Flags */
#define GOPF_HAS_SIZE   0x1
#define GOPF_YVU12      0x2
#define GOPF_EXPERMNTL  0x4
#define GOPF_XPAR       0x8
#define GOPF_SKIPBIT    0x10
#define GOPF_LOCKWORD   0x20
#define GOPF_TILESIZE   0x40
#define GOPF_PADDING    0x80

#define PICSIZE_ESCAPE	0xf			/* 1111 */
#define HUFF_ESCAPE		0x7

#define PICF_DATASIZE   0x1
#define PICF_SIDESTREAM 0x2
#define PICF_INHER_TMV  0x4
#define PICF_INHER_Q    0x8
#define PICF_XSUM       0x10
#define PICF_X10NN      0x20
#define PICF_MBHUFF     0x40
#define PICF_BANDDATASZ 0x80

PIA_RETURN_STATUS DecodePictureLayer(pRTDecInst pCntx, PU8 pntr, PU32 bytes,
	RectSt rDecodeRect, RectSt rViewRect);

PIA_RETURN_STATUS DecBufferSetup( pRTDecInst pCntx, PointSt tFrameSize,
	PointSt tTileSize, U32 uNYBands, RectSt rDecodeRect, PointSt tUVSubsample,
	Boo bTransparency, Boo bBFrames, Boo bDeltaFrames );

#endif	/* ___DECPL_H__ */

