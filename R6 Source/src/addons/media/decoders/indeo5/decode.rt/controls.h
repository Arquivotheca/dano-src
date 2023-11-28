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
 * CONTROLS.H -- Include file for controls.c 							 *
 *************************************************************************/

#ifdef __CONTROLS_H__
#pragma message("controls.h: multiple inclusion")
#else /* __CONTROLS_H__ */
#define __CONTROLS_H__

#ifndef __PIA_DEC_H__
#pragma message("controls.h requires pia_dec.h")
#endif /* __PIA_DEC_H__ */

PIA_RETURN_STATUS DecodeChangeBrightness(PTR_DEC_INST, I32);
PIA_RETURN_STATUS DecodeChangeContrast	(PTR_DEC_INST, I32);
PIA_RETURN_STATUS DecodeChangeSaturation(PTR_DEC_INST, I32);

PIA_RETURN_STATUS DecodeResetBrightness	(PTR_DEC_INST);
PIA_RETURN_STATUS DecodeResetContrast	(PTR_DEC_INST);
PIA_RETURN_STATUS DecodeResetSaturation	(PTR_DEC_INST);  		

void DecodeUpdateLuma(PU8, U32, U32, U32, PU8);
void DecodeUpdateChroma(PU8, U32, U32, U32, PU8);

#endif /* __CONTROLS_H__ */
