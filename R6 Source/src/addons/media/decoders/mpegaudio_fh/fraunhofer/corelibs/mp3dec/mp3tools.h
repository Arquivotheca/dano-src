/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1998)
 *                        All Rights Reserved
 *
 *   filename: mp3tools.h
 *   project : ISO/MPEG-Decoder
 *   author  : Martin Sieler
 *   date    : 1998-05-26
 *   contents/description: HEADER - layer III processing
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/02/16 09:17:26 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/mp3tools.h,v 1.4 1999/02/16 09:17:26 sir Exp $
 */

/*-------------------------------------------------------------------------*/

#ifndef __MP3TOOLS_H__
#define __MP3TOOLS_H__

/* ------------------------ includes --------------------------------------*/

#include "mpeg.h"

/* ------------------------------------------------------------------------*/

#if defined(ENABLE_FLOATING_POINT)
//
// floating-point forms
//

void mp3StereoProcessing
    (
    float            *pLeft,
    float            *pRight,
    MP3SI_GRCH       &SiL,
    MP3SI_GRCH       &SiR,
    const MP3SCF     &ScaleFac, /* right channel!! */
    const MPEG_INFO  &Info,
    int               fDownMix
    );

void mp3Reorder
    (
    float            *pData, 
    const MP3SI_GRCH &Si, 
    const MPEG_INFO  &Info
    );

void mp3Antialias
    (
    float            *pData,
    MP3SI_GRCH       &Si, 
    const MPEG_INFO  &Info,
    int               nQuality
    );

#endif // ENABLE_FLOATING_POINT

#if defined(ENABLE_FIXED_POINT)

//
// fixed-point stereo-interleaved forms
//

void mp3StereoProcessing
    (
    int32            *pData, // stereo interleaved
    MP3SI_GRCH       &SiL,
    MP3SI_GRCH       &SiR,
    const MP3SCF     &ScaleFac, /* right channel!! */
    const MPEG_INFO  &Info,
    int               fDownMix
    );

void mp3Reorder
    (
    int32            *pData, 
    const MP3SI_GRCH &Si, 
    const MPEG_INFO  &Info
    );

void mp3Antialias
    (
    int32            *pData,
    MP3SI_GRCH       &Si, 
    const MPEG_INFO  &Info,
    int               nQuality
    );

#endif // ENABLE_FIXED_POINT

/*-------------------------------------------------------------------------*/

#endif
