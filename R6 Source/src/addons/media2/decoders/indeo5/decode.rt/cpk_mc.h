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
 * CPK_MC.H -- function declarations and definitions needed for adding
 * predicted band data to current frame.
 *************************************************************************/

#ifndef ___CPK_MC_H__
#define ___CPK_MC_H__

void AddMotionToDelta_C(U32 uFrameType, pBlkInfoSt pBlockInfo, U32 uNBlocks,
	U32 uBlockSize, U32 uMVres,
	PU32 currbase, PU32 fwdbase, PU32 bkwdbase);

#endif /* ___CPK_MC_H__ */
