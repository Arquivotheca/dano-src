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

#ifndef __CCFLAGS_H__
#define __CCFLAGS_H__

#define CCFLAG_SKIP 0x00000001	/* use higher speed if available */
#define CCFLAG_INIT 0x00000002	/* initialize high speed output area */
#define CCFLAG_CLPL 0x00000004	/* special YVU12 planar CL-GD5480 */
#define CCFLAG_LUMA 0x00000008	/* only output luma if possible */
#define	CCFLAG_IYUV 0x00000010	/* Y,U,V plane orderring (Intel) */
#define CCFLAG_SVR3 0x00000020	/* move chroma down if possible */
#define CCFLAG_NEXT 0x00000040

#endif
