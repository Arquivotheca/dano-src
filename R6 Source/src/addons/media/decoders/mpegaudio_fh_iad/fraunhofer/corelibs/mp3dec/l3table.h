/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1998)
 *                        All Rights Reserved
 *
 *   filename: l3table.h
 *   project : ISO/MPEG-Decoder
 *   author  : Martin Sieler
 *   date    : 1998-05-26
 *   contents/description: HEADER - tables for iso/mpeg-decoding (layer3)
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/02/16 09:00:31 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/l3table.h,v 1.3 1999/02/16 09:00:31 sir Exp $
 */

/*-------------------------------------------------------------------------*/

#ifndef __L3TABLE_H__
#define __L3TABLE_H__

/* ------------------------ includes --------------------------------------*/

/* ------------------------------------------------------------------------*/

typedef struct
{
  int l[23];
  int s[14];
} SF_BAND_INDEX[3][3];

/* ------------------------------------------------------------------------*/

extern const SF_BAND_INDEX sfBandIndex;

/*-------------------------------------------------------------------------*/

#endif
