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
#ifdef __XFRM_H__
#pragma message("***** XFRM.H Included Multiple Times")
#endif
#endif

#if !defined __XFRM_H__
#define __XFRM_H__

#define FORWARD 0
#define INVERSE 1

/* utilities in xform.c */
void transform(	PI32 piTemp,	/* block to transform */
				PIA_Boolean bFwd,	/* Forward or Inverse */
				I32 iXform );	/* which transform to use (slant/Haar/none) */

void dec_transform(	PI32 piTemp,	/* block to transform */
					I32 iXform );	/* which transform to use (slant/Haar/none) */

void
BlockGetDiff(
   MatrixSt mSrcPic,   	/* src picture */
   MatrixSt mSrcMc,    	/* Motion Compensation */
   RectSt   rSrc,   	/* block of srcPic to get */
   PI32 	piDst       /* 8x8 destination block */
        );
void
BlockPut(
   PI32     piSrcBlock, /* 8x8 source block */
   I32	iBlockSize,
   MatrixSt mDestPic,	/* Pic to set */
   RectSt   rDest    	/* block of srcPic to set */
        );
void
	BlockGet(
	PI32 piDstBlock,        /* 8x8 destination block */
	MatrixSt mSrcPic,   	/* src picture */
	RectSt rSrc   			/* block of srcPic to get */
		);

#endif

