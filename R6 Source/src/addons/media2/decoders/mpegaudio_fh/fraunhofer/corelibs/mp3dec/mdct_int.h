#ifndef __MDCT_INT_H__
#define __MDCT_INT_H__

/* ------------------------ includes --------------------------------------*/

#include "mpeg.h"

/*-------------------------------------------------------------------------*/

//
// MDCT class.
//
//  This object performs the frequency-to-time mapping.
//  Fixed-point stereo-interleaved version.

class CMdctInt
{

public :

  CMdctInt(const MPEG_INFO &_info, int _qual);
  ~CMdctInt() {}

  void Init();
  void Apply(int ch, const MP3SI_GRCH &SiGrCH, int32* spectrum);

protected :

  void cos_t_h_12(const int32 *vec,int32 *f_vec,const int32 *win);
  void cos_t_h_long (int32 *prev,int32 *dest,const int32 *win);
  void cos_t_h_short(int32 *prev,int32 *dest,const int32 *win);
  void cost9 (const int32 *y, int32 *s) ;

  void overlap_hybrid18(int32 *prev, int32 *dest, const int32 *win);

  int32 cost36_rese[9];
  int32 cost36_reso[9];

  const MPEG_INFO &info;
  INT_SPECTRUM     prevblck;
  int              qual;
};

/*-------------------------------------------------------------------------*/
#endif
