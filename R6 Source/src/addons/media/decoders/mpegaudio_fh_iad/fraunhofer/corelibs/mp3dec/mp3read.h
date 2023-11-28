/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1998)
 *                        All Rights Reserved
 *
 *   filename: mp3read.h
 *   project : ISO/MPEG-Decoder
 *   author  : Martin Sieler
 *   date    : 1998-05-26
 *   contents/description: mp3 read-functions: sideinfo, main data, 
 *                                             scalefactors
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/02/16 09:10:49 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/mp3read.h,v 1.4 1999/02/16 09:10:49 sir Exp $
 */

/*-------------------------------------------------------------------------*/

#ifndef __MP3READ_H__
#define __MP3READ_H__

/* ------------------------ includes --------------------------------------*/

#include "mpeg.h"

/* ------------------------------------------------------------------------*/

class CBitStream;

/* ------------------------------------------------------------------------*/

bool mp3SideInfoRead
    (
    CBitStream      &Bs,
    MP3SI           &Si,
    const MPEG_INFO &Info
    );

bool mp3MainDataRead
    (
    CBitStream      &Bs, // bitstream
    CBitStream      &Db, // dynamic buffer
    const MP3SI     &Si,
    const MPEG_INFO &Info
    );

void mp3ScaleFactorRead
    (
    CBitStream      &Bs,
    MP3SI_GRCH      &SiGrCh,
    MP3SCF          &ScaleFac,
    const MPEG_INFO &Info,
    const int       *pScfsi,
    int              gr,
    int              ch
    );

/*-------------------------------------------------------------------------*/
#endif
