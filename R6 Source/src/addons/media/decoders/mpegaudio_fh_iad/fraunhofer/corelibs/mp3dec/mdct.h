/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1998)
 *                        All Rights Reserved
 *
 *   filename: mdct.h
 *   project : ISO/MPEG-Decoder
 *   author  : Stefan Gewinner
 *   date    : 1998-05-26
 *   contents/description: mdct class - HEADER
 *
 *
 \***************************************************************************/

/*
 * $Date: 1999/03/11 08:51:54 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/mdct.h,v 1.7 1999/03/11 08:51:54 sir Exp $
 */

#ifndef __MDCT_H__
#define __MDCT_H__

/* ------------------------ includes --------------------------------------*/

#include "mpeg.h"

/*-------------------------------------------------------------------------*/

//
// MDCT class.
//
//  This object performs the frequency-to-time mapping.
//

class CMdct
{

public :

  CMdct(const MPEG_INFO &_info, int _qual);
  ~CMdct() {}

  void Init();
  void Apply(int ch, const MP3SI_GRCH &SiGrCH, SPECTRUM &rs);

protected :

  void cos_t_h_12(const float *vec,float *f_vec,const float *win);
  void cos_t_h_long (float *prev,float *dest,const float *win);
  void cos_t_h_short(float *prev,float *dest,const float *win);
  void cost9 (const float *y, float *s) ;

  void overlap_hybrid18(float *prev, float *dest, const float *win);

  float hybrid_res[36];
  float cost36_rese[9];
  float cost36_reso[9];

  const MPEG_INFO &info;
  SPECTRUM         prevblck;
  int              qual;
};

/*-------------------------------------------------------------------------*/
#endif
