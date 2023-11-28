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
#ifdef __ENBS_H__
#pragma message("***** ENBS.H Included Multiple Times")
#endif
#endif

#ifndef __ENBS_H__
#define __ENBS_H__

void EncodeBitstream(PPicBSInfoSt pPicInfo, 
					U8 u8EncodeMode, 
					jmp_buf jbEnv);

I32 PicDRCType(I32 iPictureType, jmp_buf jbEnv);

#endif /* __ENBS_H__ */
