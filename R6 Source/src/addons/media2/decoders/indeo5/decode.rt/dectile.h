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
 * DECTILE.H -- function declarations and definitions needed for processing
 * the tile info.
 *************************************************************************/

#ifdef __DECTILE_H__
#pragma message("dectile.h: multiple inclusion")
#else /* __DECTILE_H__ */
#define __DECTILE_H__

#ifndef __RTDEC_H__
#pragma message("dectile.h requires rtdec.h")
#endif /* __RTDEC_H__ */

U32 DecodeTileInfo(pRTDecInst pCntx, PU8 pntr, PU32 bytes, U32 plane,
	 U32 band, Boo bDecodeThisBand);

#endif	/* __DECTILE_H__ */
