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

#ifndef _irte_h_
#define _irte_h_

#ifdef NO_CONFLICTING_ENCODER_TYPES
#include <wtypes.h>
#else
#ifndef __NO_WINDOWS__
#define WINAPI __stdcall
#else
#define WINAPI
#endif
#define HRESULT long
#endif

typedef struct _tag_IVidCompress {

	struct _tag_IVidCompress *lpVtbl;

	HRESULT (WINAPI *Compress)();
	HRESULT (WINAPI *CompressQuery)();
	HRESULT (WINAPI *CompressBegin)();
	HRESULT (WINAPI *CompressEnd)();

} IVidCompress;

typedef IVidCompress *pIVidCompress;

typedef struct _tag_IVidConfigure {

	struct _tag_IVidConfigure *lpVtbl;

	HRESULT (WINAPI *SetScalability)();
	HRESULT (WINAPI *CompressFramesInfo)();
	HRESULT (WINAPI *SetCPUID)();
} IVidConfigure;

typedef IVidConfigure *pIVidConfigure;

/* for RTE license key */
#define RTE_KEY         0x80719708
#define RTE_ACK         0x80423718

#endif /* _irte_h_ */
