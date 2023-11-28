/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1998)
 *                        All Rights Reserved
 *
 *   filename: mp3tools.cpp
 *   project : ISO/MPEG-Decoder
 *   author  : Martin Sieler
 *   date    : 1998-05-26
 *   contents/description: Layer III-processing-functions
 *                         stereo-processing, antialias & reordering
 *                         hybrid synthese
 *
 *
 \***************************************************************************/

/*
 * $Date: 1998/09/11 14:37:12 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/mp3tools.cpp,v 1.3 1998/09/11 14:37:12 sir Exp $
 */

/* ------------------------ includes --------------------------------------*/

#include "mp3tools.h"
#include "l3table.h"

/* ------------------------------------------------------------------------*/

#ifndef min
  #define min(a,b) ((a) < (b) ? (a):(b))
#endif

#ifndef max
  #define max(a,b) ((a) > (b) ? (a):(b))
#endif

/*-------------------------------------------------------------------------*/

static const double M_SQRT_2 = 0.707106781186547524401;

/*-------------------------------------------------------------------------*/

static const float tan12_tab1[7] =
  { 
  0.000000000000f, 0.211324865405f, 0.366025403784f,
	0.500000000000f, 0.633974596216f, 0.788675134595f,
  1.000000000000f
  };

/*-------------------------------------------------------------------------*/

static const float tan12_tab2[7] =
  {  
  1.000000000000f, 0.788675134595f, 0.633974596216f,
	0.500000000000f, 0.366025403784f, 0.211324865405f,
  0.000000000000f
  };

/*-------------------------------------------------------------------------*/

static const float mpeg2_itab[2][16] =
{
  { 
  1.000000000f, 0.840896415f, 0.707106781f, 0.594603557f,
  0.499999999f, 0.420448207f, 0.353553390f, 0.297301778f,
  0.249999999f, 0.210224103f, 0.176776695f, 0.148650889f,
  0.125000000f, 0.105112051f, 0.088388347f, 0.074325444f
  },
  {
  1.000000000f, 0.707106781f, 0.500000000f, 0.353553390f,
  0.250000000f, 0.176776695f, 0.125000000f, 0.088388347f,
  0.062500000f, 0.044194174f, 0.031250000f, 0.022097087f,
  0.015625000f, 0.011048543f, 0.007812500f, 0.005524272f
  }
};

/*-------------------------------------------------------------------------*/

static const float but_cs[8] =
  {
	0.857492925713f,  0.881741997318f,  0.949628649103f,  0.983314592492f,
	0.995517816068f,  0.999160558178f,  0.999899195244f,  0.999993155070f
  };

/*-------------------------------------------------------------------------*/

static const float but_ca[8] =
  {
 -0.514495755428f, -0.471731968565f, -0.313377454204f, -0.181913199611f,
 -0.094574192526f, -0.040965582885f, -0.014198568572f, -0.003699974674f
  };

/*-------------------------------------------------------------------------*/

/* Stereo ms mode routine */
static void III_process_stereo_ms
    (
    float *pLeft, 
    float *pRight,
    int    startNdx,
    int    endNdx,
    int    fDownMix
    )
{
  int    indx;
  double temp;

  if ( !fDownMix )
    {
    for ( indx=startNdx; indx<endNdx; indx++ )
      {
      temp         = ( pLeft[indx] + pRight[indx] ) * M_SQRT_2;
      pRight[indx] = (float)(( pLeft[indx] - pRight[indx] ) * M_SQRT_2);
      pLeft[indx]  = (float)(temp);
      }
    }
  else
    {
    for ( indx=startNdx; indx<endNdx; indx++ )
      pLeft[indx] = (float)(pLeft[indx] * M_SQRT_2);
    }
}

/*-------------------------------------------------------------------------*/

/* Stereo intensity mode routine */
static void III_process_stereo_intens
    (
    float           *pLeft, 
    float           *pRight,
    int              startNdx,
    int              endNdx,
    int              position,
    int              intensity_scale,
    const MPEG_INFO &Info,
    int              fDownMix
    )
{
  int   indx;
  float tmp;

  if ( Info.IsMpeg1 )
    {
    /* MPEG 1 */
    if ( !fDownMix )
      {
      for ( indx=startNdx; indx<endNdx; indx++ )
        {
        tmp          = pLeft[indx];
        pLeft[indx]  = tmp * tan12_tab1[position];
        pRight[indx] = tmp * tan12_tab2[position];
        }
      }
    else
      {
      for ( indx=startNdx; indx<endNdx; indx++ )
        pLeft[indx] *= 0.5f;
      }
    }
  else
    {
    /* MPEG 2 */
    if ( !fDownMix )
      {
      if ( position & 1 )
        {
        position = (position+1)>>1;
        
        for ( indx=startNdx; indx<endNdx; indx++ )
          {
          pRight[indx]  = pLeft[indx];
          pLeft[indx]  *= mpeg2_itab[intensity_scale][position];
          }
        }
      else
        {
        position>>=1;

        for ( indx=startNdx; indx<endNdx; indx++ )
          {
          pRight[indx] = pLeft[indx] * mpeg2_itab[intensity_scale][position];
          }
        }
      }
    else
      {
      for ( indx=startNdx; indx<endNdx; indx++ )
        pLeft[indx] *= (1.0f + mpeg2_itab[intensity_scale][position]) * 0.5f;
      }
    }
}

/*-------------------------------------------------------------------------*/

static void III_process_stereo_lr
    (
    float *pLeft,
    float *pRight,
    int    startNdx,
    int    endNdx,
    int    fDownMix
    )
{
  int indx;

  if ( fDownMix )
    for ( indx=startNdx; indx<endNdx; indx++ )
      pLeft[indx] = ( pLeft[indx] + pRight[indx] ) * 0.5f;
}

/*-------------------------------------------------------------------------*/

/* Mixed block stereo processing */
static void III_stereo_mixed
    (
    float            *pLeft,
    float            *pRight,
    const MP3SI_GRCH &SiL,
    const MP3SI_GRCH &SiR,
    const MP3SCF     &ScaleFac, /* right channel!! */
    const MPEG_INFO  &Info,
    int               fDownMix
    )
{
  int ms_stereo = Info.mode_ext & 0x2;
  int i_stereo  = Info.mode_ext & 0x1;

  int cbZero;
  int cbMaxBand;
  int pt, cb, width;
  int startNdx,endNdx;
  int Position;
  int IllegalPos;

  cbMaxBand = max(SiL.zeroSfbStartNdxSMax,
                  SiR.zeroSfbStartNdxSMax);

  if ( i_stereo && SiR.zeroSfbStartNdxIsLong )
    cbZero = SiR.zeroSfbStartNdxL;
  else
    cbZero = 22;

  /* long-block part */
  for ( cb=0; cb<(Info.IsMpeg1 ? 8:6); cb++ )
    {
    Position   = ScaleFac.l[cb];
    IllegalPos = ScaleFac.l_iip[cb];

    startNdx = sfBandIndex[Info.fhgVersion][Info.sample_rate_ndx].l[cb];    
    endNdx   = sfBandIndex[Info.fhgVersion][Info.sample_rate_ndx].l[cb+1];

    if ( (cb<cbZero) || (i_stereo && (Position == IllegalPos)) )
      {
      if ( ms_stereo )
        III_process_stereo_ms(pLeft, pRight, startNdx, endNdx, fDownMix);
      else
        III_process_stereo_lr(pLeft, pRight, startNdx, endNdx, fDownMix);
      }
      
    if ( i_stereo && (cb>=cbZero) && (Position != IllegalPos) )
      III_process_stereo_intens(pLeft, pRight, 
                                startNdx, endNdx, 
                                Position, 
                                SiR.intensity_scale,
                                Info,
                                fDownMix);
    }

  /* short-block part */
  for ( pt=0; pt<3; pt++ )
    {
    /* get intensity-border */
    if ( i_stereo )
      {
      if ( SiR.zeroSfbStartNdxIsLong )
        cbZero = 0;
      else
        cbZero = SiR.zeroSfbStartNdxS[pt];      
      }
    else
      {
      cbZero=13;
      }

    for ( cb=3; cb<cbMaxBand; cb++ )
      {
      Position   = ScaleFac.s[pt][cb];
      IllegalPos = ScaleFac.s_iip[cb];

      width = sfBandIndex[Info.fhgVersion][Info.sample_rate_ndx].s[cb+1]-
              sfBandIndex[Info.fhgVersion][Info.sample_rate_ndx].s[cb];

      startNdx = sfBandIndex[Info.fhgVersion][Info.sample_rate_ndx].s[cb]*3+width*pt;
      endNdx   = startNdx+width;

      if ( (cb<cbZero) || (i_stereo && (Position == IllegalPos)) )
        {
        if ( ms_stereo )
          III_process_stereo_ms(pLeft, pRight, startNdx, endNdx, fDownMix);
        else
          III_process_stereo_lr(pLeft, pRight, startNdx, endNdx, fDownMix);
        }

      if ( i_stereo && (cb>=cbZero) && (Position != IllegalPos) )
        III_process_stereo_intens(pLeft, pRight,
                                  startNdx, endNdx, 
                                  Position, 
                                  SiR.intensity_scale,
                                  Info,
                                  fDownMix);
      }
    }
}

/*-------------------------------------------------------------------------*/

/* short block stereo processing */
static void III_stereo_short
    (
    float            *pLeft,
    float            *pRight,
    const MP3SI_GRCH &SiL,
    const MP3SI_GRCH &SiR,
    const MP3SCF     &ScaleFac, /* right channel!! */
    const MPEG_INFO  &Info,
    int               fDownMix
    )
{
  int ms_stereo = Info.mode_ext & 0x2;
  int i_stereo  = Info.mode_ext & 0x1;

  int cbZero;
  int cbMaxBand;
  int pt, cb, width;
  int startNdx, endNdx;
  int Position;
  int IllegalPos;

  cbMaxBand = max(SiL.zeroSfbStartNdxSMax,
                  SiR.zeroSfbStartNdxSMax);

  for ( pt=0; pt<3; pt++ )
    {
    /* get intensity-border */
    if ( i_stereo )
      cbZero = SiR.zeroSfbStartNdxS[pt];
    else
      cbZero=13;

    for ( cb=0; cb<cbMaxBand; cb++ )
      {
      Position   = ScaleFac.s[pt][cb];
      IllegalPos = ScaleFac.s_iip[cb];

      width = sfBandIndex[Info.fhgVersion][Info.sample_rate_ndx].s[cb+1]-
              sfBandIndex[Info.fhgVersion][Info.sample_rate_ndx].s[cb];

      startNdx = sfBandIndex[Info.fhgVersion][Info.sample_rate_ndx].s[cb]*3+width*pt;
      endNdx   = startNdx+width;

      if ( (cb<cbZero) || (i_stereo && (Position == IllegalPos)) )
        {
        /* sfb below intensity-border or illegal intensity-position */
        if ( ms_stereo )
          III_process_stereo_ms(pLeft, pRight, startNdx, endNdx, fDownMix);
        else
          III_process_stereo_lr(pLeft, pRight, startNdx, endNdx, fDownMix);
        }

      if ( i_stereo && (cb>=cbZero) && (Position != IllegalPos) )
        {
        /* sfb above intensitiy-border and legal intensity-position */
        III_process_stereo_intens(pLeft, pRight, 
                                  startNdx, endNdx, 
                                  Position, 
                                  SiR.intensity_scale,
                                  Info,
                                  fDownMix);
        }
      }
    }
}

/*-------------------------------------------------------------------------*/

/* long block stereo processing */
static void III_stereo_long
    (
    float            *pLeft,
    float            *pRight,
    const MP3SI_GRCH &SiL,
    const MP3SI_GRCH &SiR,
    const MP3SCF     &ScaleFac, /* right channel!! */
    const MPEG_INFO  &Info,
    int               fDownMix
    )
{
  int ms_stereo = Info.mode_ext & 0x2;
  int i_stereo  = Info.mode_ext & 0x1;

  int cbZero;
  int cbMaxBand;
  int cb;
  int startNdx,endNdx;
  int Position;
  int IllegalPos;

  /* get intensity-border */
  if ( i_stereo )
    cbZero = SiR.zeroSfbStartNdxL;
  else
    cbZero = 22;

  cbMaxBand = max(SiL.zeroSfbStartNdxL,
                  SiR.zeroSfbStartNdxL);

  for ( cb=0; cb<cbMaxBand; cb++ )
    {
    Position   = ScaleFac.l[cb];
    IllegalPos = ScaleFac.l_iip[cb];

    startNdx = sfBandIndex[Info.fhgVersion][Info.sample_rate_ndx].l[cb];
    endNdx   = sfBandIndex[Info.fhgVersion][Info.sample_rate_ndx].l[cb+1];

    if ( (cb<cbZero) || (i_stereo && (Position == IllegalPos)) )
      {
      /* sfb below intensity-border or illegal intensity-position */
      if ( ms_stereo )
        III_process_stereo_ms(pLeft, pRight, startNdx, endNdx, fDownMix);
      else
        III_process_stereo_lr(pLeft, pRight, startNdx, endNdx, fDownMix);
      }

    if ( i_stereo && (cb>=cbZero) && (Position != IllegalPos) )
      {
      /* sfb above intensitiy-border and legal intensity-position */
      III_process_stereo_intens(pLeft, pRight,
                                startNdx, endNdx,
                                Position,
                                SiR.intensity_scale,
                                Info,
                                fDownMix);
      }
    }
}

/*-------------------------------------------------------------------------*/

void mp3StereoProcessing
    (
    float            *pLeft,
    float            *pRight,
    MP3SI_GRCH       &SiL,
    MP3SI_GRCH       &SiR,
    const MP3SCF     &ScaleFac, /* right channel!! */
    const MPEG_INFO  &Info,
    int               fDownMix
    )
{
  if ( (Info.stereo == STEREO) && (Info.mode == MPG_MD_JOINT_STEREO) )
    {
    /* joint stereo */
    if ( SiL.window_switching_flag && (SiL.block_type == 2) )
      {
      if ( SiL.mixed_block_flag )
        III_stereo_mixed(pLeft, pRight, SiL, SiR, ScaleFac, Info, fDownMix);
      else
        III_stereo_short(pLeft, pRight, SiL, SiR, ScaleFac, Info, fDownMix);
      }
    else
      III_stereo_long(pLeft, pRight, SiL, SiR, ScaleFac, Info, fDownMix);

    //
    //  copy _max_ of zero information to left _and_ right
    //
    SiL.zeroStartNdx        = max(SiL.zeroStartNdx,        SiR.zeroStartNdx);
    SiL.zeroSfbStartNdxL    = max(SiL.zeroSfbStartNdxL,    SiR.zeroSfbStartNdxL);
    SiL.zeroSfbStartNdxSMax = max(SiL.zeroSfbStartNdxSMax, SiR.zeroSfbStartNdxSMax);
    SiL.zeroSfbStartNdxS[0] = max(SiL.zeroSfbStartNdxS[0], SiR.zeroSfbStartNdxS[0]);
    SiL.zeroSfbStartNdxS[1] = max(SiL.zeroSfbStartNdxS[1], SiR.zeroSfbStartNdxS[1]);
    SiL.zeroSfbStartNdxS[2] = max(SiL.zeroSfbStartNdxS[2], SiR.zeroSfbStartNdxS[2]);

    SiR.zeroStartNdx        = SiL.zeroStartNdx;
    SiR.zeroSfbStartNdxL    = SiL.zeroSfbStartNdxL;
    SiR.zeroSfbStartNdxSMax = SiL.zeroSfbStartNdxSMax;
    SiR.zeroSfbStartNdxS[0] = SiL.zeroSfbStartNdxS[0];
    SiR.zeroSfbStartNdxS[1] = SiL.zeroSfbStartNdxS[1];
    SiR.zeroSfbStartNdxS[2] = SiL.zeroSfbStartNdxS[2];
    }
  else if ( (Info.stereo == STEREO) && fDownMix )
    {
    /* process_stereo downmix */
    int endNdx = max(SiL.zeroStartNdx, SiR.zeroStartNdx);

    III_process_stereo_lr(pLeft, pRight, 0, endNdx, fDownMix);
    }
}

/*-------------------------------------------------------------------------*/

/*
 * reorder one sfb
 */
static void reorder_sfb(float *pData, int startNdx, int endNdx)
{
  float fTmp[3*256];
  
  int indxr = 0;
  int width = (endNdx-startNdx)/3;
  int indx;

  for ( indx=startNdx; indx<startNdx+width; indx++ )
    {
    fTmp[indxr++] = pData[indx];
    fTmp[indxr++] = pData[indx+width];
    fTmp[indxr++] = pData[indx+width*2];
    }

  for ( indx=0; indx<width*3; indx++ )
    pData[startNdx+indx] = fTmp[indx];
}

/*-------------------------------------------------------------------------*/

void mp3Reorder
    (
    float            *pData, 
    const MP3SI_GRCH &Si, 
    const MPEG_INFO  &Info
    )
{  
  int sfb;
  int startNdx;
  int endNdx;

  if ( Si.window_switching_flag && (Si.block_type == 2) )
    {
    /* mixed/short Block */
    for ( sfb=(Si.mixed_block_flag ? 3:0); sfb<13; sfb++ )
      {
      startNdx = sfBandIndex[Info.fhgVersion][Info.sample_rate_ndx].s[sfb]*3;
      endNdx   = sfBandIndex[Info.fhgVersion][Info.sample_rate_ndx].s[sfb+1]*3;
      reorder_sfb(pData, startNdx, endNdx);
      }
    }
}

/*-------------------------------------------------------------------------*/

static void III_calcSbLimit(MP3SI_GRCH &SiGrCh, const MPEG_INFO &Info)
{
  // 
  // pSi->zeroSbStartNdx:
  // index of first subband with all zero
  //
  if  ( SiGrCh.window_switching_flag && (SiGrCh.block_type == 2) )
    {
    // might not work for mixed blocks, never tested
    int ndx = sfBandIndex[Info.fhgVersion][Info.sample_rate_ndx].s[SiGrCh.zeroSfbStartNdxSMax];
    
    SiGrCh.zeroSbStartNdx = (ndx*3)/SSLIMIT + ( (((ndx*3)%SSLIMIT)!=0) ? 1:0 );
    }
  else
    {
    SiGrCh.zeroSbStartNdx = SiGrCh.zeroStartNdx / SSLIMIT +
      ( ((SiGrCh.zeroStartNdx % SSLIMIT)!= 0) ? 1:0 );
    }
}

/*-------------------------------------------------------------------------*/

static void antialias_sb(float *sb1,float *sb2)
{
  double temp;

  temp   = sb1[17]*but_cs[0]-sb2[0]*but_ca[0];
  sb2[0] = sb2[0]*but_cs[0]+sb1[17]*but_ca[0];
  sb1[17]= (float)temp;

  temp   = sb1[16]*but_cs[1]-sb2[1]*but_ca[1];
  sb2[1] = sb2[1]*but_cs[1]+sb1[16]*but_ca[1];
  sb1[16]= (float)temp;

  temp   = sb1[15]*but_cs[2]-sb2[2]*but_ca[2];
  sb2[2] = sb2[2]*but_cs[2]+sb1[15]*but_ca[2];
  sb1[15]= (float)temp;

  temp   = sb1[14]*but_cs[3]-sb2[3]*but_ca[3];
  sb2[3] = sb2[3]*but_cs[3]+sb1[14]*but_ca[3];
  sb1[14]= (float)temp;

  temp   = sb1[13]*but_cs[4]-sb2[4]*but_ca[4];
  sb2[4] = sb2[4]*but_cs[4]+sb1[13]*but_ca[4];
  sb1[13]= (float)temp;

  temp   = sb1[12]*but_cs[5]-sb2[5]*but_ca[5];
  sb2[5] = sb2[5]*but_cs[5]+sb1[12]*but_ca[5];
  sb1[12]= (float)temp;

  temp   = sb1[11]*but_cs[6]-sb2[6]*but_ca[6];
  sb2[6] = sb2[6]*but_cs[6]+sb1[11]*but_ca[6];
  sb1[11]= (float)temp;

  temp   = sb1[10]*but_cs[7]-sb2[7]*but_ca[7];
  sb2[7] = sb2[7]*but_cs[7]+sb1[10]*but_ca[7];
  sb1[10]= (float)temp;
}

/*-------------------------------------------------------------------------*/

void mp3Antialias
    (
    float           *pData,
    MP3SI_GRCH      &SiGrCh,
    const MPEG_INFO &Info,
    int              nQuality
    )
{
  int sb;
  int sblim;

  III_calcSbLimit(SiGrCh, Info);

  if ( SiGrCh.window_switching_flag && 
      (SiGrCh.block_type == 2) &&
      !SiGrCh.mixed_block_flag 
     )
    {
    /* short blocks, but no mixed blocks -> no antialias */
    return;
    }

  if ( SiGrCh.window_switching_flag && 
       SiGrCh.mixed_block_flag &&
      (SiGrCh.block_type == 2)
     )
    {
    /* short blocks AND mixed blocks -> antialias for lowest 2 (4) subbands */
    sblim = ((Info.fhgVersion==MPG_MPEG25)&&(Info.sample_rate_ndx==MPG_SF_LOW)) ? 3 : 1;
    }
  else
    {
    sblim = min((SBLIMIT>>nQuality)-1,SiGrCh.zeroSbStartNdx);
    }

  /* 
   * max 31 alias-reduction operations between each pair of sub-bands
   * with 8 butterflies between each pair
   */
  for ( sb=0; sb<sblim; sb++ )
    {
    antialias_sb(&pData[sb*SSLIMIT], &pData[(sb+1)*SSLIMIT]);
    }
}

/*-------------------------------------------------------------------------*/
