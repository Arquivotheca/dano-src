
// 10jul00: note that there are floating-point calculations here.  the
// dequantize routine is not a bottleneck, and is very sensitive precision-wise
// (since all further calculations depend on it.)  with careful design an
// integer version would be doable, but it's probably not worth the effort at
// this point.  [em]

#include "mp3quant.h"

#include "l3table.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// amount to scale results by (to provide guard bits)
static const float pre_scale_factor = 0.125f;

/* ------------------------------------------------------------------------*/

static const int pretab[22] = 
  {
  0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,2,2,3,3,3,2,0
  };


/*-------------------------------------------------------------------------*/

static const float pow2_025[] =
  { 
   32768.000000000000f, 27554.493735033720f, 23170.475005920790f, 19483.969372204580f,
   16384.000000000000f, 13777.246867516860f, 11585.237502960400f,  9741.984686102291f,
    8192.000000000000f,  6888.623433758429f,  5792.618751480198f,  4870.992343051145f,
    4096.000000000000f,  3444.311716879215f,  2896.309375740099f,  2435.496171525573f,
    2048.000000000000f,  1722.155858439607f,  1448.154687870049f,  1217.748085762786f,
    1024.000000000000f,   861.077929219804f,   724.077343935025f,   608.874042881393f,
     512.000000000000f,   430.538964609902f,   362.038671967512f,   304.437021440697f,
     256.000000000000f,   215.269482304951f,   181.019335983756f,   152.218510720348f,
     128.000000000000f,   107.634741152476f,    90.509667991878f,    76.109255360174f,
      64.000000000000f,    53.817370576238f,    45.254833995939f,    38.054627680087f,
      32.000000000000f,    26.908685288119f,    22.627416997970f,    19.027313840044f,
      16.000000000000f,    13.454342644059f,    11.313708498985f,     9.513656920022f,
       8.000000000000f,     6.727171322030f,     5.656854249492f,     4.756828460011f,
       4.000000000000f,     3.363585661015f,     2.828427124746f,     2.378414230005f,
       2.000000000000f,     1.681792830507f,     1.414213562373f,     1.189207115003f,
       1.000000000000f,     0.840896415254f,     0.707106781187f,     0.594603557501f,
       0.500000000000f,     0.420448207627f,     0.353553390593f,     0.297301778751f,
       0.250000000000f,     0.210224103813f,     0.176776695297f,     0.148650889375f,
       0.125000000000f,     0.105112051907f,     0.088388347648f,     0.074325444688f,
       0.062500000000f,     0.052556025953f,     0.044194173824f,     0.037162722344f,
       0.031250000000f,     0.026278012977f,     0.022097086912f,     0.018581361172f,
       0.015625000000f,     0.013139006488f,     0.011048543456f,     0.009290680586f,
       0.007812500000f,     0.006569503244f,     0.005524271728f,     0.004645340293f,
       0.003906250000f,     0.003284751622f,     0.002762135864f,     0.002322670146f,
       0.001953125000f,     0.001642375811f,     0.001381067932f,     0.001161335073f,
       0.000976562500f,     0.000821187906f,     0.000690533966f,     0.000580667537f,
       0.000488281250f,     0.000410593953f,     0.000345266983f,     0.000290333768f,
       0.000244140625f,     0.000205296976f,     0.000172633492f,     0.000145166884f,
       0.000122070313f,     0.000102648488f,     0.000086316746f,     0.000072583442f,
       0.000061035156f,     0.000051324244f,     0.000043158373f,     0.000036291721f,
       0.000030517578f,     0.000025662122f,     0.000021579186f,     0.000018145861f,
       0.000015258789f,     0.000012831061f,     0.000010789593f,     0.000009072930f
  };

/*-------------------------------------------------------------------------*/

static const float powx_43[] =
  {
    0.000000000000f,   1.000000000000f,   2.519842099790f,   4.326748710922f,
	  6.349604207873f,   8.549879733383f,  10.902723556993f,  13.390518279407f,
	 16.000000000000f,  18.720754407467f,  21.544346900319f,  24.463780996262f,
	 27.473141821280f,  30.567350940370f,  33.741991698453f,  36.993181114957f,
	 40.317473596636f,  43.711787041190f,  47.173345095760f,  50.699631325717f,
	 54.288352331898f,  57.937407704004f,  61.644865274419f,  65.408940536586f,
	 69.227979374756f,  73.100443455322f,  77.024897778592f,  81.000000000000f,
	 85.024491212519f,  89.097187944890f,  93.216975178616f,  97.382800224133f,
	101.593667325965f, 105.848632889862f, 110.146801243434f, 114.487320856601f,
	118.869380960207f, 123.292208510900f, 127.755065458361f, 132.257246277553f,
	136.798075734136f, 141.376906855692f, 145.993119085231f, 150.646116596629f,
	155.335326754347f, 160.060198702053f, 164.820202066734f, 169.614825766519f,
	174.443576911885f, 179.305979791126f, 184.201574932019f, 189.129918232576f,
	194.090580154497f, 199.083144973717f, 204.107210082969f, 209.162385341877f,
	214.248292470508f, 219.364564482778f, 224.510845156412f, 229.686788536522f,
	234.892058470132f, 240.126328169233f, 245.389279800185f, 250.680604097473f,
	256.000000000000f, 261.347174308289f, 266.721841361065f, 272.123722729860f,
	277.552546930380f, 283.008049149462f, 288.489970986599f, 293.998060209023f,
	299.532070519474f, 305.091761335830f, 310.676897581822f, 316.287249488156f,
	321.922592403372f, 327.582706613855f, 333.267377172437f, 338.976393735070f,
	344.709550405101f, 350.466645584700f, 356.247481833026f, 362.051865730751f,
	367.879607750583f, 373.730522133445f, 379.604426770021f, 385.501143087346f,
	391.420495940199f, 397.362313507024f, 403.326427190145f, 409.312671520063f,
	415.320884063608f, 421.350905335765f, 427.402578714976f, 433.475750361762f,
	439.570269140479f, 445.685986544083f, 451.822756621728f, 457.980435909091f,
	464.158883361278f, 470.357960288187f, 476.577530292236f, 482.817459208320f,
	489.077615045917f, 495.357867933236f, 501.658090063317f, 507.978155642004f,
	514.317940837697f, 520.677323732817f, 527.056184276906f, 533.454404241292f,
	539.871867175251f, 546.308458363615f, 552.764064785746f, 559.238575075842f,
	565.731879484504f, 572.243869841523f, 578.774439519834f, 585.323483400588f,
	591.890897839313f, 598.476580633093f, 605.080430988760f, 611.702349492036f,
	618.342238077592f, 625.000000000000f, 631.675539805538f, 638.368763304812f
  };

/*-------------------------------------------------------------------------*/

// 
// common scale routine for all blocks types 
//

static void III_scale_int
    (
    int32 *pData,
    int    startNdx, 
    int    endNdx, 
    int    iScale,
    int    interleave
    )
{
	int    iSample;
	double fScale;
	int    indx;
	
	if ( (iScale < 128) && (iScale >= 0) )
	{
		fScale = pow2_025[iScale] * pre_scale_factor;
		
		for ( indx=startNdx*interleave; indx<endNdx*interleave; indx += interleave )
		{
			iSample = abs(pData[indx]);
			
			int32 result;
			if ( iSample )
			{
				double v;
				if ( iSample < 128 )
				{	
					v = powx_43[iSample];
				}
				else
				{			
					v = pow((double)iSample, 4.0/3.0);
				}		

				if(pData[indx] <= 0)
					v *= -fScale;
				else
					v *= fScale;
			
				if(v > 32767.)
				{
//					debugger("clip+b");
					v = 32767.;
				}
				else if(v < -32768.)
				{
//					debugger("clip-b");
					v = -32768.;
				}
				pData[indx] = double_to_fixed32(v);
			}
			else
			{
				pData[indx] = 0;
			}
		}
	}
	else
	{
		if(interleave == 1)
			memset(pData+startNdx, 0, sizeof(int16)*(endNdx-startNdx));
		else
			for(indx=startNdx*interleave; indx<endNdx*interleave; indx += interleave)
				pData[indx] = 0;
	}
}

/*-------------------------------------------------------------------------*/

//
// long blocks  0,1,3
//
static void III_deq_long_int
    (
    int32            *pData,
    const MP3SI_GRCH &SiGrCh,
    const MP3SCF     &ScaleFac,
    const MPEG_INFO  &Info,
    int               interleave
    )
{
  int sfreq = Info.sample_rate_ndx;  
  int cb;
  int iScale;
  int startNdx, endNdx;

  for ( cb=0; cb<SiGrCh.zeroSfbStartNdxL; cb++ )
    {
    startNdx = sfBandIndex[Info.fhgVersion][sfreq].l[cb];
    endNdx   = sfBandIndex[Info.fhgVersion][sfreq].l[cb+1];

    iScale = 210 - SiGrCh.global_gain +
             2 * (1 + SiGrCh.scalefac_scale) * 
             (ScaleFac.l[cb] + SiGrCh.preflag * pretab[cb]);

    III_scale_int(pData, startNdx, endNdx, iScale, interleave);
    }
}

/*-------------------------------------------------------------------------*/

//
// short blocks  2
//
static void III_deq_short_int
    (
    int32            *pData,
    const MP3SI_GRCH &SiGrCh,
    const MP3SCF     &ScaleFac,
    const MPEG_INFO  &Info,
    int               interleave
    )
{
  int    sfreq = Info.sample_rate_ndx;
  int    cb;
  int    pt;
  int    width;
  int    iScale;
  int    startNdx, endNdx;

  for ( cb=0; cb < SiGrCh.zeroSfbStartNdxSMax; cb++ )
    {
    width = sfBandIndex[Info.fhgVersion][sfreq].s[cb+1] - 
            sfBandIndex[Info.fhgVersion][sfreq].s[cb];

    for ( pt=0; pt<3; pt++ )
      {
      startNdx = sfBandIndex[Info.fhgVersion][sfreq].s[cb]*3+width*pt;
      endNdx   = startNdx+width;

      iScale = 210 - SiGrCh.global_gain +
               8 * SiGrCh.subblock_gain[pt] + 
               2 * (1 + SiGrCh.scalefac_scale) * ScaleFac.s[pt][cb];

      III_scale_int(pData, startNdx, endNdx, iScale, interleave);
      }
    }
}

/*-------------------------------------------------------------------------*/

//
// mixed blocks  2
//
static void III_deq_mixed_int
    (
    int32            *pData,
    const MP3SI_GRCH &SiGrCh,
    const MP3SCF     &ScaleFac,
    const MPEG_INFO  &Info,
    int               interleave
    )
{
  int sfreq = Info.sample_rate_ndx;
  int cb;
  int pt;
  int width;
  int iScale;
  int startNdx, endNdx;

  // long block part
  for ( cb=0; cb<(Info.IsMpeg1 ? 8:6) ; cb++ )
    {
    startNdx = sfBandIndex[Info.fhgVersion][sfreq].l[cb];
    endNdx   = sfBandIndex[Info.fhgVersion][sfreq].l[cb+1];

    iScale = 210 - SiGrCh.global_gain +
             2 * (1 + SiGrCh.scalefac_scale) * 
             (ScaleFac.l[cb] + SiGrCh.preflag * pretab[cb]);

    III_scale_int(pData, startNdx, endNdx, iScale, interleave);
    }

  // short block part
  if ( !SiGrCh.zeroSfbStartNdxIsLong )
    {
    for ( cb=3; cb<SiGrCh.zeroSfbStartNdxSMax; cb++ )
      {
      width = sfBandIndex[Info.fhgVersion][sfreq].s[cb+1] - 
              sfBandIndex[Info.fhgVersion][sfreq].s[cb];
      
      for ( pt=0; pt<3; pt++ )
        {
        startNdx = sfBandIndex[Info.fhgVersion][sfreq].s[cb]*3+width*pt;
        endNdx   = startNdx+width;
        
        iScale = 210 - SiGrCh.global_gain +
                 8 * SiGrCh.subblock_gain[pt] + 
                 2 * (1 + SiGrCh.scalefac_scale) * ScaleFac.s[pt][cb];
        
        III_scale_int(pData, startNdx, endNdx, iScale, interleave);
        }
      }
    }
  }

/*-------------------------------------------------------------------------*/

void mp3DequantizeSpectrum
    (
    int32            *pData,
    const MP3SI_GRCH &SiGrCh,
    const MP3SCF     &ScaleFac,
    const MPEG_INFO  &Info
    )
{
	/* dequantize all sfb up to zeroSfbStartNdx[LS] */
	if ( SiGrCh.window_switching_flag && (SiGrCh.block_type == 2) )
	{
		if ( SiGrCh.mixed_block_flag )
			III_deq_mixed_int(pData, SiGrCh, ScaleFac, Info, 2);
		else
			III_deq_short_int(pData, SiGrCh, ScaleFac, Info, 2);
	}
	else
	{
		III_deq_long_int(pData, SiGrCh, ScaleFac, Info, 2);
	}

	/* zero samples above zeroStartNdx */
	for(int n = SiGrCh.zeroStartNdx*2; n < SBLIMIT*SSLIMIT*2; n += 2)
		pData[n] = 0;		
}

/*-------------------------------------------------------------------------*/
