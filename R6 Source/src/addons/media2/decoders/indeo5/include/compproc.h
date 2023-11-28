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
/***********************************************************
 * COMPPROC.H - compressor constants & function prototypes
 ***********************************************************/

#ifndef COMPPROC_H
#define COMPPROC_H

/* Functions prototypes */
COMPRESSCONTEXT* CompressOpen (void);
U32 CompressClose (COMPRESSCONTEXT*);
void HPConvertToSSYUV (COMPRESSCONTEXT*, LPBITMAPINFOHEADER, PU8);
void CompressFree (COMPRESSCONTEXT*);

/* Default Quality for slider - range of 0 to 10,000. */
#define DEFAULT_SLIDER_QUALITY 6500

/* Default Key Frame Rate. */
#define DEFAULT_KEYFRAMERATE_IV4X 1

/* Default encode rate */
#define DEFAULT_ENCODE_RATE 50

/* Default decode rate */
#define DEFAULT_DECODE_RATE 50

#endif
