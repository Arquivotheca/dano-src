#ifndef __POLYPHASE_INT_H__
#define __POLYPHASE_INT_H__

/* ------------------------ includes --------------------------------------*/

#include "mpeg.h"

/*-------------------------------------------------------------------------*/

#define HAN_SIZE 512

/*-------------------------------------------------------------------------*/

// Class for (inverse) Polyphase calculation.
// Fixed-point stereo-interleaved version.

class CPolyphaseInt
{
public:

  CPolyphaseInt(const MPEG_INFO &_info, int _qual, int _resl, int _downMix);

  ~CPolyphaseInt();

  void Init();
  short *Apply(INT_SPECTRUM &sample, short *pPcm, int frames=SSLIMIT);

protected:

  int   bufOffset;
  int32 syn_buf[HAN_SIZE*2];

  const MPEG_INFO &info ;      // info-structure
  int              qual;       // quality (full, half, quarter spectrum)
  int              resl;       // resolution (16, 8 bit PCM)
  int              downMix;    // downmix stereo to mono

  void window_band_m(int bufOffset, short *out_samples, int short_window);
  void window_band_s(int bufOffset, short *out_samples, int short_window);
};

/*-------------------------------------------------------------------------*/
#endif
