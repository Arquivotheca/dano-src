
#include "mp3quant.h"

#include "l3table.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------------*/

static const int pretab[22] = 
  {
  0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,2,2,3,3,3,2,0
  };

/*-------------------------------------------------------------------------*/

static const int32 pow2_025_int[] = 
{
	0x80000000, 0x6ba27e80, 0x5a827980, 0x4c1bf800,
	0x40000000, 0x35d13f40, 0x2d413cc0, 0x260dfc00,
	0x20000000, 0x1ae89fa0, 0x16a09e60, 0x1306fe00,
	0x10000000, 0x0d744fd0, 0x0b504f30, 0x09837f00,
	0x08000000, 0x06ba27e8, 0x05a82798, 0x04c1bf80,
	0x04000000, 0x035d13f4, 0x02d413cc, 0x0260dfc0,
	0x02000000, 0x01ae89fa, 0x016a09e6, 0x01306fe0,
	0x01000000, 0x00d744fd, 0x00b504f3, 0x009837f0,
	0x00800000, 0x006ba27f, 0x005a827a, 0x004c1bf8,
	0x00400000, 0x0035d13f, 0x002d413d, 0x00260dfc,
	0x00200000, 0x001ae8a0, 0x0016a09e, 0x001306fe,
	0x00100000, 0x000d7450, 0x000b504f, 0x0009837f,
	0x00080000, 0x0006ba28, 0x0005a828, 0x0004c1c0,
	0x00040000, 0x00035d14, 0x0002d414, 0x000260e0,
	0x00020000, 0x0001ae8a, 0x00016a0a, 0x00013070,
	0x00010000, 0x0000d745, 0x0000b505, 0x00009838,
	0x00008000, 0x00006ba2, 0x00005a82, 0x00004c1c,
	0x00004000, 0x000035d1, 0x00002d41, 0x0000260e,
	0x00002000, 0x00001ae9, 0x000016a1, 0x00001307,
	0x00001000, 0x00000d74, 0x00000b50, 0x00000983,
	0x00000800, 0x000006ba, 0x000005a8, 0x000004c2,
	0x00000400, 0x0000035d, 0x000002d4, 0x00000261,
	0x00000200, 0x000001af, 0x0000016a, 0x00000130,
	0x00000100, 0x000000d7, 0x000000b5, 0x00000098,
	0x00000080, 0x0000006c, 0x0000005b, 0x0000004c,
	0x00000040, 0x00000036, 0x0000002d, 0x00000026,
	0x00000020, 0x0000001b, 0x00000017, 0x00000013,
	0x00000010, 0x0000000d, 0x0000000b, 0x0000000a,
	0x00000008, 0x00000007, 0x00000006, 0x00000005,
	0x00000004, 0x00000003, 0x00000003, 0x00000002,
	0x00000002, 0x00000002, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001
};

static const int32 powx_43_int[] = 
{
	0x00000000, 0x00010000, 0x00028514, 0x000453a6,
	0x00065980, 0x00088cc5, 0x000ae719, 0x000d63f9,
	0x00100000, 0x0012b883, 0x00158b5a, 0x001876ba,
	0x001b7920, 0x001e913e, 0x0021bdf3, 0x0024fe41,
	0x00285146, 0x002bb638, 0x002f2c60, 0x0032b31b,
	0x003649d2, 0x0039effa, 0x003da516, 0x004168b1,
	0x00453a5d, 0x004919b7, 0x004d0660, 0x00510000,
	0x00550645, 0x005918e2, 0x005d378c, 0x006161ff,
	0x006597fb, 0x0069d940, 0x006e2595, 0x00727cc1,
	0x0076de90, 0x007b4ace, 0x007fc14c, 0x008441db,
	0x0088cc4f, 0x008d607d, 0x0091fe3d, 0x0096a568,
	0x009b55d8, 0x00a00f69, 0x00a4d1f9, 0x00a99d65,
	0x00ae718e, 0x00b34e55, 0x00b8339a, 0x00bd2142,
	0x00c21730, 0x00c71549, 0x00cc1b72, 0x00d12992,
	0x00d63f90, 0x00db5d54, 0x00e082c7, 0x00e5afd1,
	0x00eae45e, 0x00f02057, 0x00f563a8, 0x00faae3c,
	0x01000000, 0x010558e0, 0x010ab8ca, 0x01101fac,
	0x01158d74, 0x011b0210, 0x01207d6e, 0x0125ff80,
	0x012b8836, 0x0131177e, 0x0136ad4a, 0x013c498a,
	0x0141ec30, 0x0147952c, 0x014d4472, 0x0152f9f4,
	0x0158b5a6, 0x015e7776, 0x01643f5a, 0x016a0d48,
	0x016fe12e, 0x0175bb04, 0x017b9abc, 0x0181804a,
	0x01876ba6, 0x018d5cc0, 0x01935390, 0x0199500c,
	0x019f5226, 0x01a559d4, 0x01ab6710, 0x01b179ca,
	0x01b791fe, 0x01bdaf9c, 0x01c3d2a0, 0x01c9fafe,
	0x01d028ac, 0x01d65ba4, 0x01dc93da, 0x01e2d146,
	0x01e913de, 0x01ef5b9e, 0x01f5a878, 0x01fbfa68,
	0x02025164, 0x0208ad64, 0x020f0e64, 0x02157454,
	0x021bdf34, 0x02224ef8, 0x0228c398, 0x022f3d14,
	0x0235bb5c, 0x023c3e70, 0x0242c640, 0x024952d0,
	0x024fe410, 0x02567a00, 0x025d1498, 0x0263b3cc,
	0x026a579c, 0x02710000, 0x0277acf0, 0x027e5e68
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
  int32  fScale, fSample;
  int    indx;

  if ( (iScale < 128) && (iScale >= 0) )
    {
    // +++++ when we settle on a good guard-bit value, this table should
    //       be prescaled!
    fScale = pow2_025_int[iScale] >> fixed32_guard_bits;

    for ( indx=startNdx*interleave; indx<endNdx*interleave; indx += interleave )
      {
      iSample = abs(pData[indx]);

      if ( iSample )
        {
        if ( iSample < 128 )
          fSample = powx_43_int[iSample];
        else
        {
          // +++++ do something clever here
          fSample = double_to_fixed32(pow((double)iSample, 4.0/3.0));
        }

        if ( pData[indx] > 0 )
          pData[indx] = fixed32_mul(fSample, fScale);
        else
          pData[indx] = fixed32_mul(-fSample, fScale);
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
	    memset(pData+startNdx, 0, sizeof(int32)*(endNdx-startNdx));
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
