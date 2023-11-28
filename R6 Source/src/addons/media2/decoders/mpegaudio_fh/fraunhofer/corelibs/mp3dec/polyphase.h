/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1998)
 *                        All Rights Reserved
 *
 *   filename: polyphase.h
 *   project : ISO/MPEG-Decoder
 *   author  : Stefan Gewinner
 *   date    : 1998-05-26
 *   contents/description: polyphase class - HEADER
 *
 * MODIFICATIONS by em@be.com:
 *   Apply() takes adjustable frame count
 *
\***************************************************************************/

/*
 * $Date: 1999/06/24 10:31:16 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/polyphase.h,v 1.9 1999/06/24 10:31:16 sir Exp $
 */

#ifndef __POLYPHASE_H__
#define __POLYPHASE_H__

/* ------------------------ includes --------------------------------------*/

#include "mpeg.h"

/*-------------------------------------------------------------------------*/

#define HAN_SIZE 512

/*-------------------------------------------------------------------------*/

// Class for (inverse) Polyphase calculation.

class CPolyphase
{

public:

  CPolyphase(const MPEG_INFO &_info, int _qual, int _resl, int _downMix);

  ~CPolyphase() {}

  void Init();
  short *Apply(POLYSPECTRUM &sample, short *pPcm, int frames=SSLIMIT);
  float *Apply(POLYSPECTRUM &sample, float *pPcm, int frames=SSLIMIT);

protected:

  int   bufOffset;
  float syn_buf[2][HAN_SIZE];

  const MPEG_INFO &info ;      // info-structure
  int              qual;       // quality (full, half, quarter spectrum)
  int              resl;       // resolution (16, 8 bit PCM)
  int              downMix;    // downmix stereo to mono

  void window_band_m(int bufOffset, short *out_samples, int short_window);
  void window_band_s(int bufOffset, short *out_samples, int short_window);
  void window_band_m(int bufOffset, float *out_samples, int short_window);
  void window_band_s(int bufOffset, float *out_samples, int short_window);
};

/*-------------------------------------------------------------------------*/
#endif
