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

#define DEBUG 1
#include <Debug.h>

/* ------------------------------------------------------------------------*/

#ifndef min
  #define min(a,b) ((a) < (b) ? (a):(b))
#endif

#ifndef max
  #define max(a,b) ((a) > (b) ? (a):(b))
#endif

/*-------------------------------------------------------------------------*/

// sqrt(2)/2
//static const double M_SQRT_2 = 0.707106781186547524401;
static const int32 M_SQRT_2 = 0x0000b505;

/*-------------------------------------------------------------------------*/

static const int32 tan12_tab1_int[] = 
{
	0x00000000, 0x00003619, 0x00005db4, 0x00008000,
	0x0000a24c, 0x0000c9e7, 0x00010000
};

/*-------------------------------------------------------------------------*/

static const int32 tan12_tab2_int[] = 
{
	0x00010000, 0x0000c9e7, 0x0000a24c, 0x00008000,
	0x00005db4, 0x00003619, 0x00000000
};

/*-------------------------------------------------------------------------*/

static const int32 mpeg2_itab_int[2][16] =
{
	{ 
		0x00010000, 0x0000d745, 0x0000b505, 0x00009838,
		0x00008000, 0x00006ba2, 0x00005a82, 0x00004c1c,
		0x00004000, 0x000035d1, 0x00002d41, 0x0000260e,
		0x00002000, 0x00001ae9, 0x000016a1, 0x00001307
	},
	{ 
		0x00010000, 0x0000b505, 0x00008000, 0x00005a82,
		0x00004000, 0x00002d41, 0x00002000, 0x000016a1,
		0x00001000, 0x00000b50, 0x00000800, 0x000005a8,
		0x00000400, 0x000002d4, 0x00000200, 0x0000016a
	}
}; 

/*-------------------------------------------------------------------------*/

static const int32 but_cs_int[8] =
{
	0x0000db85, 0x0000e1ba, 0x0000f31b, 0x0000fbbb,
	0x0000feda, 0x0000ffc9, 0x0000fff9, 0x00010000
};

/*-------------------------------------------------------------------------*/

static const int32 but_ca_int[8] =
{
	0xffff7c4b, 0xffff873e, 0xffffafc7, 0xffffd16f,
	0xffffe7cb, 0xfffff584, 0xfffffc5e, 0xffffff0f
};

/*-------------------------------------------------------------------------*/

/* Stereo ms mode routine */
static void III_process_stereo_ms_int
    (
    int32 *pData, 
    int    startNdx,
    int    endNdx,
    int    fDownMix
    )
{
	int    indx;
	
	startNdx <<= 1;
	endNdx <<= 1;

	if ( !fDownMix )
	{
		for ( indx=startNdx; indx<endNdx; indx += 2)
		{
			int32 temp    = fixed32_mul(pData[indx] + pData[indx+1], M_SQRT_2);
			pData[indx+1] = fixed32_mul(pData[indx] - pData[indx+1], M_SQRT_2);
			pData[indx]   = temp;
		}
	}
	else
	{
		for ( indx=startNdx; indx<endNdx; indx += 2 )
			pData[indx] = fixed32_mul(pData[indx], M_SQRT_2);
	}
}

/*-------------------------------------------------------------------------*/

/* Stereo intensity mode routine */
static void III_process_stereo_intens_int
    (
    int32           *pData,
    int              startNdx,
    int              endNdx,
    int              position,
    int              intensity_scale,
    const MPEG_INFO &Info,
    int              fDownMix
    )
{
	int   indx;	
	
	startNdx <<= 1;
	endNdx <<= 1;
	
	if ( Info.IsMpeg1 )
	{
		/* MPEG 1 */
		if ( !fDownMix )
		{
			for ( indx=startNdx; indx<endNdx; indx += 2 )
			{
				int32 tmp     = pData[indx];
				pData[indx]   = fixed32_mul(tmp, tan12_tab1_int[position]);
				pData[indx+1] = fixed32_mul(tmp, tan12_tab2_int[position]);
			}
		}
		else
		{
			for ( indx=startNdx; indx<endNdx; indx += 2 )
				//        pLeft[indx] *= 0.5f;
				pData[indx] >>= 1;
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
		
				for ( indx=startNdx; indx<endNdx; indx += 2 )
				{
					pData[indx+1]  = pData[indx];
					pData[indx]    = fixed32_mul(
						pData[indx],
						mpeg2_itab_int[intensity_scale][position]);
				}
			}
			else
			{
				position>>=1;
				
				for ( indx=startNdx; indx<endNdx; indx += 2 )
				{
					pData[indx+1]  = fixed32_mul(
						pData[indx],
						mpeg2_itab_int[intensity_scale][position]);
				}
			}
		}
		else
		{
			for ( indx=startNdx; indx<endNdx; indx += 2 )
				pData[indx] = fixed32_mul(
					pData[indx],
					(int32_to_fixed32(1) + mpeg2_itab_int[intensity_scale][position]) >> 1);
		}
	}
}

/*-------------------------------------------------------------------------*/

static void III_process_stereo_lr_int
    (
    int32 *pData,
    int    startNdx,
    int    endNdx,
    int    fDownMix
    )
{
	int indx;
	
	if ( fDownMix )
	{
		startNdx <<= 1;
		endNdx <<= 1;

		for ( indx=startNdx; indx<endNdx; indx += 2 )
			pData[indx] = ( pData[indx] + pData[indx+1] ) >> 1;
	}
}

/*-------------------------------------------------------------------------*/

/* Mixed block stereo processing */
static void III_stereo_mixed_int
    (
    int32            *pData,
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
        III_process_stereo_ms_int(pData, startNdx, endNdx, fDownMix);
      else
        III_process_stereo_lr_int(pData, startNdx, endNdx, fDownMix);
      }
      
    if ( i_stereo && (cb>=cbZero) && (Position != IllegalPos) )
      III_process_stereo_intens_int(
                                pData, 
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
          III_process_stereo_ms_int(pData, startNdx, endNdx, fDownMix);
        else
          III_process_stereo_lr_int(pData, startNdx, endNdx, fDownMix);
        }

      if ( i_stereo && (cb>=cbZero) && (Position != IllegalPos) )
        III_process_stereo_intens_int(
                                  pData,
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
static void III_stereo_short_int
    (
    int32            *pData,
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
          III_process_stereo_ms_int(pData, startNdx, endNdx, fDownMix);
        else
          III_process_stereo_lr_int(pData, startNdx, endNdx, fDownMix);
        }

      if ( i_stereo && (cb>=cbZero) && (Position != IllegalPos) )
        {
        /* sfb above intensitiy-border and legal intensity-position */
        III_process_stereo_intens_int(
                                  pData, 
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
static void III_stereo_long_int
    (
    int32            *pData,
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
        III_process_stereo_ms_int(pData, startNdx, endNdx, fDownMix);
      else
        III_process_stereo_lr_int(pData, startNdx, endNdx, fDownMix);
      }

    if ( i_stereo && (cb>=cbZero) && (Position != IllegalPos) )
      {
      /* sfb above intensitiy-border and legal intensity-position */
      III_process_stereo_intens_int(
                                pData,
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
    int32            *pData,
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
        III_stereo_mixed_int(pData, SiL, SiR, ScaleFac, Info, fDownMix);
      else
        III_stereo_short_int(pData, SiL, SiR, ScaleFac, Info, fDownMix);
      }
    else
      III_stereo_long_int(pData, SiL, SiR, ScaleFac, Info, fDownMix);

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

    III_process_stereo_lr_int(pData, 0, endNdx, fDownMix);
    }
}

/*-------------------------------------------------------------------------*/

/*
 * reorder one stereo-interleaved sfb
 */

static void reorder_sfb_int(int32 *pData, int startNdx, int endNdx)
{
	int32 fTmp[3*256];
	
	int indxr = 0;
	startNdx <<= 1;
	endNdx <<= 1;
	int width = (endNdx-startNdx)/3;
	int indx;
	
	for ( indx=startNdx; indx<startNdx+width; indx += 2 )
	{
		fTmp[indxr++] = pData[indx];
		fTmp[indxr++] = pData[indx+width];
		fTmp[indxr++] = pData[indx+width*2];
	}
	
	for ( indx=0; indx<(endNdx-startNdx); indx += 2 )
		pData[startNdx+indx] = fTmp[indx>>1];
}

/*-------------------------------------------------------------------------*/

void mp3Reorder
    (
    int32            *pData, 
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
			reorder_sfb_int(pData, startNdx, endNdx);
		}
	}
}

/*-------------------------------------------------------------------------*/

static void III_calcSbLimit_int(MP3SI_GRCH &SiGrCh, const MPEG_INFO &Info)
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

static void antialias_sb_int(int32 *sb1, int32 *sb2)
{
  int32 temp;

  temp   = fixed32_mul(sb1[34], but_cs_int[0]) - fixed32_mul(sb2[0],  but_ca_int[0]);
  sb2[0] = fixed32_mul(sb2[0],  but_cs_int[0]) + fixed32_mul(sb1[34], but_ca_int[0]);
  sb1[34]= temp;

  temp   = fixed32_mul(sb1[32], but_cs_int[1]) - fixed32_mul(sb2[2],  but_ca_int[1]);
  sb2[2] = fixed32_mul(sb2[2],  but_cs_int[1]) + fixed32_mul(sb1[32], but_ca_int[1]);
  sb1[32]= temp;

  temp   = fixed32_mul(sb1[30], but_cs_int[2]) - fixed32_mul(sb2[4],  but_ca_int[2]);
  sb2[4] = fixed32_mul(sb2[4],  but_cs_int[2]) + fixed32_mul(sb1[30], but_ca_int[2]);
  sb1[30]= temp;

  temp   = fixed32_mul(sb1[28], but_cs_int[3]) - fixed32_mul(sb2[6],  but_ca_int[3]);
  sb2[6] = fixed32_mul(sb2[6],  but_cs_int[3]) + fixed32_mul(sb1[28], but_ca_int[3]);
  sb1[28]= temp;

  temp   = fixed32_mul(sb1[26], but_cs_int[4]) - fixed32_mul(sb2[8],  but_ca_int[4]);
  sb2[8] = fixed32_mul(sb2[8],  but_cs_int[4]) + fixed32_mul(sb1[26], but_ca_int[4]);
  sb1[26]= temp;

  temp   = fixed32_mul(sb1[24], but_cs_int[5]) - fixed32_mul(sb2[10], but_ca_int[5]);
  sb2[10]= fixed32_mul(sb2[10], but_cs_int[5]) + fixed32_mul(sb1[24], but_ca_int[5]);
  sb1[24]= temp;

  temp   = fixed32_mul(sb1[22], but_cs_int[6]) - fixed32_mul(sb2[12], but_ca_int[6]);
  sb2[12]= fixed32_mul(sb2[12], but_cs_int[6]) + fixed32_mul(sb1[22], but_ca_int[6]);
  sb1[22]= temp;

  temp   = fixed32_mul(sb1[20], but_cs_int[7]) - fixed32_mul(sb2[14], but_ca_int[7]);
  sb2[14]= fixed32_mul(sb2[14], but_cs_int[7]) + fixed32_mul(sb1[20], but_ca_int[7]);
  sb1[20]= temp;
}

/*-------------------------------------------------------------------------*/

void mp3Antialias
    (
    int32           *pData,
    MP3SI_GRCH      &SiGrCh,
    const MPEG_INFO &Info,
    int              nQuality
    )
{
  int sb;
  int sblim;

  III_calcSbLimit_int(SiGrCh, Info);

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
   
	// +++++ OPTIMIZE: ditch SSLIMIT*2 multiply
	for ( sb=0; sb<sblim; sb++ )
		antialias_sb_int(&pData[sb*SSLIMIT*2], &pData[(sb+1)*SSLIMIT*2]);
}

/*-------------------------------------------------------------------------*/
