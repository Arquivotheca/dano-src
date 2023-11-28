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
 * GCOMPASM.H -- function declarations and definitions needed for 
 * the global composition.
 *************************************************************************/

#ifdef __GCOMPASM_H__
#pragma message("gcompasm.h: multiple inclusion")
#else /* __GCOMPASM_H__ */
#define __GCOMPASM_H__

#ifndef __RTDEC_H__
#pragma message("gcompasm.h requires rtdec.h")
#endif /* __RTDEC_H__ */

void ComposeBegin(void);
void Compose(pRTDecInst pCntx, RectSt rComposeRect, U32 fDoBands);

#define BOFFSET (((0x80*4+2)<<2) * 0x10001)
#define BIAS ((0x4000) * 0x10001)
#define BIAS1 ((0x80) * 0x10001)

#define BANDDELTA 0x400		/* implied offset between bands */

#endif /* __GCOMPASM_H__ */
