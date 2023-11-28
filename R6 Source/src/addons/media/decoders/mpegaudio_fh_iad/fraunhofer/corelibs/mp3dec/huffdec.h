/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1998)
 *                        All Rights Reserved
 *
 *   filename: huffdec.h
 *   project : ISO/MPEG Decoder
 *   author  : Martin Sieler
 *   date    : 1998-05-26
 *   contents/description: main hufman decoding - HEADER
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/03/11 08:51:52 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/huffdec.h,v 1.5 1999/03/11 08:51:52 sir Exp $
 */

#ifndef __HUFFDEC_H__
#define __HUFFDEC_H__

/* ------------------------ includes --------------------------------------*/

#include "mpeg.h"
#include "huffmandecoder.h"

/* ------------------------------------------------------------------------*/

//
// MPEG Layer-3 huffman decoding class.
//
//  This class is derived from a CHuffmanDecoder object. In addition to
//  the CHuffmanDecoder object, this object calculates the number of 
//  spectral lines in the big value area, the number of spectral lines in the 
//  count-one area and the region boundaries within the big value area from 
//  an MPEG Layer-3 bitstream sideinfo.
//

class CMp3Huffman : protected CHuffmanDecoder
{
public:
  CMp3Huffman()  {}
  ~CMp3Huffman() {}

  void Read
      (
      CBitStream      &Bs,         // where to read from
      int32           *pISpectrum, // pointer to spectrum
      MP3SI_GRCH      &SiGrCh,     // side info (granule/channel)
      const MPEG_INFO &Info,       // mpeg info
      int              interleave  // channel interleave (1 or 2)
      );
};

/*-------------------------------------------------------------------------*/
#endif
