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
  void Apply(
  	int32 ch,
  	const MP3SI_GRCH &Si,
  	int32* spectrum);
  void Apply(
  	const MP3SI_GRCH &SiL,
  	const MP3SI_GRCH &SiR,
  	int32* spectrum);

protected :

  void cos_t_h_long (int32 *prev,int32 *dest,const int16 *win);
  void cos_t_h_short(int32 *prev,int32 *dest,const int16 *win);
  void cos_t_h_long (int32 *prev,int32 *dest,const int16 *win_l,const int16 *win_r);
  void cos_t_h_short(int32 *prev,int32 *dest,const int16 *win_l,const int16 *win_r);

  int32 cost36_rese[18];
  int32 cost36_reso[18];

  const MPEG_INFO &info;
  INT_SPECTRUM     prevblck;
  int              qual;
};

/*-------------------------------------------------------------------------*/
#endif
