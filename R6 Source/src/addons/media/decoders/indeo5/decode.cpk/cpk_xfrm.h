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
/*
 * cpk_xfrm.h
 */
#ifdef __CPK_XFRM_H__
#pragma message("cpk_xfrm.h: multiple inclusion")
#else /* __CPK_XFRM_H__ */
#define __CPK_XFRM_H__

void SmearSlant8x8(PI32 src, PU8 dst, U32 flags);
void SmearSlant1x8(PI32 src, PU8 dst, U32 flags);
void SmearSlant8x1(PI32 src, PU8 dst, U32 flags);
void SmearNada8x8 (PI32 src, PU8 dst, U32 flags);
void SmearSlant4x4(PI32 src, PU8 dst, U32 flags);
void SmearSlant1x4(PI32 src, PU8 dst, U32 flags);
void SmearSlant4x1(PI32 src, PU8 dst, U32 flags);
void SmearNada4x4 (PI32 src, PU8 dst, U32 flags);

void InvSlant8x8(PI32 src, PU8 dst, U32 flags);
void InvSlant1x8(PI32 src, PU8 dst, U32 flags);
void InvSlant8x1(PI32 src, PU8 dst, U32 flags);
void InvNada8x8 (PI32 src, PU8 dst, U32 flags);
void InvSlant4x4(PI32 src, PU8 dst, U32 flags);
void InvSlant1x4(PI32 src, PU8 dst, U32 flags);
void InvSlant4x1(PI32 src, PU8 dst, U32 flags);
void InvNada4x4 (PI32 src, PU8 dst, U32 flags);

#endif /* __CPK_XFRM_H__ */
