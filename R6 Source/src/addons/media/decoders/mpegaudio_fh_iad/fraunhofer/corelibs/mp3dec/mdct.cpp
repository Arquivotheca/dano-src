/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1998)
 *                        All Rights Reserved
 *
 *   filename: mdct.cpp
 *   project : ISO/MPEG-Decoder
 *   author  : Stefan Gewinner
 *   date    : 1998-05-26
 *   contents/description: mdct class
 *
 *
\***************************************************************************/

/*
 * $Date: 1998/06/02 08:56:31 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/mdct.cpp,v 1.3 1998/06/02 08:56:31 sir Exp $
 */

/* ------------------------ includes --------------------------------------*/

#include "mdct.h"

/*-------------------------------------------------------------------------*/

#ifndef min
  #define min(a,b) ((a) < (b) ? (a):(b))
#endif

/*-------------------------------------------------------------------------*/

static const float hybrid_win[4][36] =
  {
    {
    0.058937661350f, 0.158918619156f, 0.232585847378f, 0.277700960636f,
    0.292893201113f, 0.277700960636f, 0.232585847378f, 0.158918619156f,
    0.058937661350f,-0.064319171011f,-0.207106769085f,-0.365086615086f,
   -0.533458590508f,-0.707106769085f,-0.880754947662f,-1.049126982689f,
   -1.207106828690f,-1.349894404411f,-1.473151206970f,-1.573132276535f,
   -1.646799445152f,-1.691914558411f,-1.707106709480f,-1.691914439201f,
   -1.646799325943f,-1.573132157326f,-1.473151206970f,-1.349894404411f,
   -1.207106828690f,-1.049126982689f,-0.880754947662f,-0.707106769085f,
   -0.533458590508f,-0.365086644888f,-0.207106769085f,-0.064319171011f
    },
    {
    0.058937661350f, 0.158918619156f, 0.232585847378f, 0.277700960636f,
    0.292893201113f, 0.277700960636f, 0.232585847378f, 0.158918619156f,
    0.058937661350f,-0.064319171011f,-0.207106769085f,-0.365086615086f,
   -0.533458590508f,-0.707106769085f,-0.880754947662f,-1.049126982689f,
   -1.207106828690f,-1.349894404411f,-1.474554657936f,-1.586706638336f,
   -1.686782836914f,-1.774021625519f,-1.847759008408f,-1.907433867455f,
   -1.935887336731f,-1.831951141357f,-1.585196495056f,-1.216364026070f,
   -0.758819043636f,-0.254864394665f, 0.000000000000f, 0.000000000000f,
    0.000000000000f, 0.000000000000f, 0.000000000000f, 0.000000000000f
    },
    {
    0.079459309578f, 0.146446600556f, 0.079459309578f,-0.103553384542f,
   -0.353553384542f,-0.603553414345f,-0.786566138268f,-0.853553354740f,
   -0.786566078663f,-0.603553414345f,-0.353553384542f,-0.103553384542f,
    0.000000000000f, 0.000000000000f, 0.000000000000f, 0.000000000000f,
    0.000000000000f, 0.000000000000f, 0.000000000000f, 0.000000000000f,
    0.000000000000f, 0.000000000000f, 0.000000000000f, 0.000000000000f,
    0.000000000000f, 0.000000000000f, 0.000000000000f, 0.000000000000f,
    0.000000000000f, 0.000000000000f, 0.000000000000f, 0.000000000000f,
    0.000000000000f, 0.000000000000f, 0.000000000000f, 0.000000000000f
    },
    {
    0.000000000000f, 0.000000000000f, 0.000000000000f, 0.000000000000f,
    0.000000000000f, 0.000000000000f, 0.056502074003f, 0.099900424480f,
    0.053107600659f,-0.069211170077f,-0.241180941463f,-0.429175883532f,
   -0.601411581039f,-0.765366852283f,-0.923497200012f,-1.074599266052f,
   -1.217522859573f,-1.351180434227f,-1.473151206970f,-1.573132276535f,
   -1.646799445152f,-1.691914558411f,-1.707106709480f,-1.691914439201f,
   -1.646799325943f,-1.573132157326f,-1.473151206970f,-1.349894404411f,
   -1.207106828690f,-1.049126982689f,-0.880754947662f,-0.707106769085f,
   -0.533458590508f,-0.365086644888f,-0.207106769085f,-0.064319171011f
    }
  };

/*-------------------------------------------------------------------------*/

static const float cost9_c[9] =
  {
  1.000000000000f, 0.984807753012f, 0.939692620786f,
  0.866025403784f, 0.766044443119f, 0.642787609687f,
  0.500000000000f, 0.342020143326f, 0.173648177667f
  };

/*-------------------------------------------------------------------------*/

static const float cost12_c1[3] = 
  {
  1.931851652578f, 1.414213562373f, 0.517638090205f
  };

/*-------------------------------------------------------------------------*/

static const float cost12_c2[1] =
  {
  1.732050807569f
  };

/*-------------------------------------------------------------------------*/

static const float cost36_c0[] = 
  {
  1.998096443164f, 1.982889722748f, 1.952592014240f,
  1.907433901496f, 1.847759065023f, 1.774021666356f,
  1.686782891626f, 1.586706680582f, 1.474554673620f,
  1.351180415231f, 1.217522858017f, 1.074599216694f,
  0.923497226470f, 0.765366864730f, 0.601411599009f,
  0.432879227876f, 0.261052384440f, 0.087238774731f
  };
/*-------------------------------------------------------------------------*/

static const float cost36_c1[9] =
  {
  1.992389396183f, 1.931851652578f, 1.812615574073f,
  1.638304088578f, 1.414213562373f, 1.147152872702f,
  0.845236523481f, 0.517638090205f, 0.174311485495f
  };

/* ------------------------------------------------------------------------*/

//-------------------------------------------------------------------------*
//
//                   C M d c t
//
//-------------------------------------------------------------------------*

//-------------------------------------------------------------------------*
//   constructor
//-------------------------------------------------------------------------*

CMdct::CMdct (const MPEG_INFO &_info, int _qual)
 : info(_info), qual(_qual)
{
  Init();
}

//-------------------------------------------------------------------------*
//   Init
//-------------------------------------------------------------------------*

void CMdct::Init()
{
  int i, j, k;

  for ( i=0;i<2;i++ )
    for ( j=0;j<SBLIMIT;j++ )
      for ( k=0;k<SSLIMIT;k++ )
        prevblck[i][j][k] = 0.0f;
}

//-------------------------------------------------------------------------*
//   Apply
//-------------------------------------------------------------------------*

void CMdct::Apply(int ch, const MP3SI_GRCH &SiGrCh, SPECTRUM &rs)
{
  int sb, ss;
  int bt, sblim;
  int SbLimMixed;

  //
  // gr_info->zeroSbStartNdx+1:
  // +1 because we have to process one more subband due to alias-reduction.
  // This wouldn't be neccessary for short blocks and in some cases of 
  // mixed blocks. We don't care about this one extra subband in these cases.
  //
  sblim      = min((SBLIMIT>>qual), SiGrCh.zeroSbStartNdx+1);
  SbLimMixed = ((info.fhgVersion==MPG_MPEG25)&&(info.sample_rate_ndx==MPG_SF_LOW)) ? 4 : 2;

  for ( sb=0; sb<(SBLIMIT>>qual); sb++ )
    {
    // get blocktype, override with long for mixed and sb<SbLimMixed
    bt = (SiGrCh.window_switching_flag&&SiGrCh.mixed_block_flag &&(sb<SbLimMixed)) ? 
         0 : SiGrCh.block_type;

    if ( sb < sblim )
      {
      // below limit -> calc i-mdct
      if ( bt == 2 )
        cos_t_h_short(prevblck[ch][sb], rs[ch][sb], hybrid_win[bt]);
      else
        cos_t_h_long (prevblck[ch][sb], rs[ch][sb], hybrid_win[bt]);
      }
    else
      {
      // above limit -> all zero, just (easy version of) overlap and add
      for ( ss=0; ss<SSLIMIT; ss++ )
        {
        rs[ch][sb][ss]       = prevblck[ch][sb][ss];
        prevblck[ch][sb][ss] = 0.0f;
        }
      }

    if ( sb&1 )
      for ( ss=1; ss<SSLIMIT; ss+=2 )
        rs[ch][sb][ss] = -rs[ch][sb][ss];
    }
}

/*-------------------------------------------------------------------------*/

void CMdct::cos_t_h_12 (const float *vec, float *f_vec, const float *win)
{
  double v0,v1,v2,v3,v4,v5;
  double re0,re1,re2;
  double ro0,ro1,ro2;

  /*  stage 0 */
  v0=vec[0]; v1=vec[1*3];v2=vec[2*3]; v3=vec[3*3];v4=vec[4*3];v5=vec[5*3];
  v4-=v5;
  v3-=v4;
  v2-=v3;
  v1-=v2;
  v0-=v1;

  /*  stage 2 */
  v3-=v5;
  v1-=v3;

  v2*=cost12_c2[0];
  v3*=cost12_c2[0];

  re0 = v0+v4+v2;
  re1 = v0-2.0*v4;
  re2 = v0+v4-v2;

  ro0 = (v1+v5+v3)*cost12_c1[0];
  ro1 = (v1-2.0*v5)*cost12_c1[1];
  ro2 = (v1+v5-v3)*cost12_c1[2];

  v0=(re0+ro0);   /*  *c0_0; in win[0..11] precalcd */
  v5=(re0-ro0);   /*  *c0_5; */
  v1=(re1+ro1);   /*  *c0_1; */
  v4=(re1-ro1);   /*  *c0_4; */
  v2=(re2+ro2);   /*  *c0_2; */
  v3=(re2-ro2);   /*  *c0_3; */

  f_vec[8]  += (float)v0*win[8];
  f_vec[9]  += (float)v0*win[9];
  f_vec[7]  += (float)v1*win[7];
  f_vec[10] += (float)v1*win[10];
  f_vec[6]  += (float)v2*win[6];
  f_vec[11] += (float)v2*win[11];

  f_vec[0]  +=  (float)v3*win[0];
  f_vec[5]  +=  (float)v3*win[5];
  f_vec[1]  +=  (float)v4*win[1];
  f_vec[4]  +=  (float)v4*win[4];
  f_vec[2]  +=  (float)v5*win[2];
  f_vec[3]  +=  (float)v5*win[3];
}

/*-------------------------------------------------------------------------*/

/* long block type MDCT subroutine */

void CMdct::cos_t_h_long (float *prev, float *dest, const float *win)
{
  dest[16] -= dest[17];
  dest[15] -= dest[16];
  dest[14] -= dest[15];
  dest[13] -= dest[14];
  dest[12] -= dest[13];
  dest[11] -= dest[12];
  dest[10] -= dest[11];
  dest[9]  -= dest[10];
  dest[8]  -= dest[9];
  dest[7]  -= dest[8];
  dest[6]  -= dest[7];
  dest[5]  -= dest[6];
  dest[4]  -= dest[5];
  dest[3]  -= dest[4];
  dest[2]  -= dest[3];
  dest[1]  -= dest[2];
  dest[0]  -= dest[1];

  dest[15] -= dest[17];
  dest[13] -= dest[15];
  dest[11] -= dest[13];
  dest[9]  -= dest[11];
  dest[7]  -= dest[9];
  dest[5]  -= dest[7];
  dest[3]  -= dest[5];
  dest[1]  -= dest[3];

  dest[0]  *= (float)0.5;
  dest[1]  *= (float)0.5;

  cost9(dest,   cost36_rese);
  cost9(dest+1, cost36_reso);

  cost36_reso[0] *= cost36_c1[0];
  cost36_reso[1] *= cost36_c1[1];
  cost36_reso[2] *= cost36_c1[2];
  cost36_reso[3] *= cost36_c1[3];
  cost36_reso[4] *= cost36_c1[4];
  cost36_reso[5] *= cost36_c1[5];
  cost36_reso[6] *= cost36_c1[6];
  cost36_reso[7] *= cost36_c1[7];
  cost36_reso[8] *= cost36_c1[8];

  overlap_hybrid18(prev, dest, win);
}

/*-------------------------------------------------------------------------*/

void CMdct::cos_t_h_short (float *prev, float *dest, const float *win)
{
  int i;

  for ( i=0; i<36; i++ )
    hybrid_res[i] = 0.0f;

  for ( i=0; i<3; i++ )
    cos_t_h_12(dest+i,&(hybrid_res[i*6+6]),win);

  for ( i=0; i<SSLIMIT; i++ )
    {
    dest[i] = hybrid_res[i]+prev[i];
    prev[i] =  hybrid_res[i+SSLIMIT];
    }
}

/*-------------------------------------------------------------------------*/

void CMdct::cost9 (const float *y, float *s)
{
  double tmp1,tmp2;

  s[4] = y[0*2]-y[2*2]+y[4*2]-y[6*2]+y[8*2];

  tmp1 = y[0*2]-y[6*2]+cost9_c[6]*(y[2*2]-y[4*2]-y[8*2]);
  tmp2 = cost9_c[3]*(y[1*2]-y[5*2]-y[7*2]);
  s[1] = (float)(tmp1+tmp2);
  s[7] = (float)(tmp1-tmp2);

  tmp1=  y[0*2]+cost9_c[2]*y[2*2]+cost9_c[4]*y[4*2]+cost9_c[6]*y[6*2]+cost9_c[8]*y[8*2];
  tmp2=  cost9_c[1]*y[1*2]+cost9_c[3]*y[3*2]+cost9_c[5]*y[5*2]+cost9_c[7]*y[7*2];
  s[0]=  (float)(tmp1+tmp2);
  s[8]=  (float)(tmp1-tmp2);

  tmp1 = y[0*2]-cost9_c[8]*y[2*2]-cost9_c[2]*y[4*2]+cost9_c[6]*y[6*2]+cost9_c[4]*y[8*2];
  tmp2 = cost9_c[5]*y[1*2]-cost9_c[3]*y[3*2]-cost9_c[7]*y[5*2]+cost9_c[1]*y[7*2];
  s[2] = (float)(tmp1+tmp2);
  s[6] = (float)(tmp1-tmp2);

  tmp1 = y[0*2]-cost9_c[4]*y[2*2]+cost9_c[8]*y[4*2]+cost9_c[6]*y[6*2]-cost9_c[2]*y[8*2];
  tmp2 = cost9_c[7]*y[1*2]-cost9_c[3]*y[3*2]+cost9_c[1]*y[5*2]-cost9_c[5]*y[7*2];
  s[3] = (float)(tmp1+tmp2);
  s[5] = (float)(tmp1-tmp2);
}

/*-------------------------------------------------------------------------*/

void CMdct::overlap_hybrid18 (float *prev, float *dest, const float *win)
{
  /*
    original result is

    for(i=0;i<9;i++)
      {
      cost36_res[i] =(cost36_rese[i]+cost36_reso[i])*cost36_c0[i];
      cost36_res[17-i]=(cost36_rese[i]-cost36_reso[i])*cost36_c0[17-i]
      }
    cost36_c0 Koeffs are precalced in win[0..35], so
    we save these multiplies
  */

  dest[0]    = prev[0]+(cost36_rese[8]-cost36_reso[8])*win[0];
  dest[17]   = prev[17]+(cost36_rese[8]-cost36_reso[8])*win[17];
  prev[0]    = (cost36_rese[8]+cost36_reso[8])*win[18];
  prev[17]   = (cost36_rese[8]+cost36_reso[8])*win[35];

  dest[1]    = prev[1]+(cost36_rese[7]-cost36_reso[7])*win[1];
  dest[16]   = prev[16]+(cost36_rese[7]-cost36_reso[7])*win[16];
  prev[1]    = (cost36_rese[7]+cost36_reso[7])*win[19];
  prev[16]   = (cost36_rese[7]+cost36_reso[7])*win[34];

  dest[2]    = prev[2]+(cost36_rese[6]-cost36_reso[6])*win[2];
  dest[15]   = prev[15]+(cost36_rese[6]-cost36_reso[6])*win[15];
  prev[2]    = (cost36_rese[6]+cost36_reso[6])*win[20];
  prev[15]   = (cost36_rese[6]+cost36_reso[6])*win[33];

  dest[3]    = prev[3]+(cost36_rese[5]-cost36_reso[5])*win[3];
  dest[14]   = prev[14]+(cost36_rese[5]-cost36_reso[5])*win[14];
  prev[3]    = (cost36_rese[5]+cost36_reso[5])*win[21];
  prev[14]   = (cost36_rese[5]+cost36_reso[5])*win[32];

  dest[4]    = prev[4]+(cost36_rese[4]-cost36_reso[4])*win[4];
  dest[13]   = prev[13]+(cost36_rese[4]-cost36_reso[4])*win[13];
  prev[4]    = (cost36_rese[4]+cost36_reso[4])*win[22];
  prev[13]   = (cost36_rese[4]+cost36_reso[4])*win[31];

  dest[5]    = prev[5]+(cost36_rese[3]-cost36_reso[3])*win[5];
  dest[12]   = prev[12]+(cost36_rese[3]-cost36_reso[3])*win[12];
  prev[5]    = (cost36_rese[3]+cost36_reso[3])*win[23];
  prev[12]   = (cost36_rese[3]+cost36_reso[3])*win[30];

  dest[6]    = prev[6]+(cost36_rese[2]-cost36_reso[2])*win[6];
  dest[11]   = prev[11]+(cost36_rese[2]-cost36_reso[2])*win[11];
  prev[6]    = (cost36_rese[2]+cost36_reso[2])*win[24];
  prev[11]   = (cost36_rese[2]+cost36_reso[2])*win[29];

  dest[7]    = prev[7]+(cost36_rese[1]-cost36_reso[1])*win[7];
  dest[10]   = prev[10]+(cost36_rese[1]-cost36_reso[1])*win[10];
  prev[7]    = (cost36_rese[1]+cost36_reso[1])*win[25];
  prev[10]   = (cost36_rese[1]+cost36_reso[1])*win[28];

  dest[8]    = prev[8]+(cost36_rese[0]-cost36_reso[0])*win[8];
  dest[9]    = prev[9]+(cost36_rese[0]-cost36_reso[0])*win[9];
  prev[8]    = (cost36_rese[0]+cost36_reso[0])*win[26];
  prev[9]    = (cost36_rese[0]+cost36_reso[0])*win[27];
}

/*-------------------------------------------------------------------------*/
