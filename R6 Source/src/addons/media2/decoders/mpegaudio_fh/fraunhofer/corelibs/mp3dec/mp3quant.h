/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1995)
 *                        All Rights Reserved
 *
 *   filename: mp_quant.h
 *   project : ISO/MPEG-Decoder
 *   author  : Markus Werner, addings: Martin Sieler
 *   date    : 1995-07-07
 *   contents/description: HEADER - sample-dequantization
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/02/16 09:06:44 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/mp3quant.h,v 1.3 1999/02/16 09:06:44 sir Exp $
 */

/*-------------------------------------------------------------------------*/

#ifndef __MP3QUANT_H__
#define __MP3QUANT_H__

/* ------------------------ includes --------------------------------------*/

#include "mpeg.h"
#include <SupportDefs.h>

/* ------------------------------------------------------------------------*/

#if defined(ENABLE_FLOATING_POINT)
void mp3DequantizeSpectrum
    (
    int32      *pIData,
    float      *pFData,
    const MP3SI_GRCH &SiGrCh,
    const MP3SCF     &ScaleFac,
    const MPEG_INFO  &Info
    );
#endif

// fixed-point, channel-interleaved
#if defined(ENABLE_FIXED_POINT)
void mp3DequantizeSpectrum
    (
    int32      *pIOData,
    const MP3SI_GRCH &SiGrCh,
    const MP3SCF     &ScaleFac,
    const MPEG_INFO  &Info
    );
#endif

/*-------------------------------------------------------------------------*/

#endif
