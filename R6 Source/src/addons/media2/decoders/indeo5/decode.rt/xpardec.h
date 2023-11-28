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
 * XPARDEC.H -- function declarations and definitions needed for indeo5
 * transparency plane decode
 *************************************************************************/

#ifdef __XPARDEC_H__
#pragma message("xpardec.h: multiple inclusion")
#else /* __XPARDEC_H__ */
#define __XPARDEC_H__

#ifndef __CPK_BLK_H__
#pragma message("xpardec.h requires cpk_blk.h")
#endif /* __CPK_BLK_H__ */


typedef struct {
	int					initialized;
	HuffTabSt			hufftab;
	PBDESCRIPTOR_TYPE	descriptor[28];
} XparCntx, *pXparCntx;

typedef struct {
	PBDESCRIPTOR_TYPE	hdesc[32];
} XparSetCntx, *pXparSetCntx;



I32 DecodeTransparencyTile(
	pXparCntx		pXCntx,
	pXparSetCntx	pXSCntx,
	PU8				pMask,
	I32				stride,
	PU8				pCopyOfMask,
	I32				c_stride,
	U32				uBMXres,
	U32				uPicYres,
	U32				uPicXres,
	U32				uXOrigin,
	U32				uYOrigin,
	U32				uXExtent,
	U32				uYExtent,
	PU8				pTileData,
	U8				bit_depth,
	U8				empty,
	U8				empty_state,
	U8				bxor);


I32 DecodeTransparencyPlane(
	pXparCntx		pXCntx,			/* i/o */
	pXparSetCntx	pXSCntx,		/* input */
	PIA_Boolean			bDataSize,		/* input */
	PPointSt		ptFrameSize,	/* input */
	PPointSt		ptTileSize,		/* input */
	PPointSt		ptNTiles,		/* input */
	PU8				pTilesToDecode,	/* input */
	PU8				pBS,			/* input */
	PU32			bytesread,		/* output */
	PIA_Boolean			Skip,			/* input */
	PU8				pXparMask,		/* output */
	U32				uXparMaskPitch,	/* input */
	PU8				pXparMaskCopy,	/* output */
	U32				uXparMaskCopyPitch, /* input */
	PPIA_Boolean		pbDidXparDec,	/* output */
	PPIA_Boolean		pbDirtyUpdate,	/* output */
	PRectSt			rDirty);		/* output */

PU8
MakeRectMask(
	PU8 pOut,
	U32 uXres,
	U32 uYres,
	/* I32 iStride,*/
	U32 uXOriginD,
	U32 uYOriginD,
	U32 uXExtentD,
	U32 uYExtentD,
	U32 uXOriginV,
	U32 uYOriginV,
	U32 uXExtentV,
	U32 uYExtentV);

#endif /* __XPARDEC_H__ */
