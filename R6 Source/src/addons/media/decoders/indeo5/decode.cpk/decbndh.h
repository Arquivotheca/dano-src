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
 * DECBNDH.H -- function declarations and definitions needed for processing
 * the band header.
 *************************************************************************/

#ifdef __DECBNDH_H__
#pragma message("decbndh.h: multiple inclusion")
#else /* __DECBNDH_H__ */
#define __DECBNDH_H__

#ifndef __RTDEC_H__
#pragma message("decbndh.h requires rtdec.h")
#endif /* __RTDEC_H__ */

#define BANDF_DROPPED   0x01
#define BANDF_INTMV     0x02
#define BANDF_QDELTA    0x04
#define BANDF_INHQ      0x08
#define BANDF_RVCHANGE  0x10
#define BANDF_X10NN     0x20
#define BANDF_RVMAP     0x40
#define BANDF_BLOCKHUFF 0x80

void ZeroBand(pRTDecInst pCntx, U32 plane, U32 band);
U32 DecodeBandHeader(pRTDecInst pCntx, PU8 pntr, PU32 bytesread, U32 plane, U32 band);

#endif	/* __DECBNDH_H__ */
