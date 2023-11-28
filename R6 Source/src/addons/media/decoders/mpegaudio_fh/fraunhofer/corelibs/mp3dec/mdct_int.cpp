
#include "mdct_int.h"
#include <string.h>
#include <stdio.h>

#include <Debug.h>

/*-------------------------------------------------------------------------*/

#ifndef min
  #define min(a,b) ((a) < (b) ? (a):(b))
#endif

/*-------------------------------------------------------------------------*/

static const int32 hybrid_win_int[4][36] =
{
	{
		0x00000f17, 0x000028af, 0x00003b8b, 0x00004717,
		0x00004afb, 0x00004717, 0x00003b8b, 0x000028af,
		0x00000f17, 0xffffef8a, 0xffffcafc, 0xffffa28b,
		0xffff7770, 0xffff4afc, 0xffff1e88, 0xfffef36d,
		0xfffecafc, 0xfffea66e, 0xfffe86e1, 0xfffe6d48,
		0xfffe5a6c, 0xfffe4ee0, 0xfffe4afc, 0xfffe4ee0,
		0xfffe5a6c, 0xfffe6d48, 0xfffe86e1, 0xfffea66e,
		0xfffecafc, 0xfffef36d, 0xffff1e88, 0xffff4afc,
		0xffff7770, 0xffffa28b, 0xffffcafc, 0xffffef8a
	},
	{
		0x00000f17, 0x000028af, 0x00003b8b, 0x00004717,
		0x00004afb, 0x00004717, 0x00003b8b, 0x000028af,
		0x00000f17, 0xffffef8a, 0xffffcafc, 0xffffa28b,
		0xffff7770, 0xffff4afc, 0xffff1e88, 0xfffef36d,
		0xfffecafc, 0xfffea66e, 0xfffe8685, 0xfffe69cf,
		0xfffe5030, 0xfffe39db, 0xfffe26fa, 0xfffe17b3,
		0xfffe106b, 0xfffe2b06, 0xfffe6a32, 0xfffec89d,
		0xffff3dbf, 0xffffbec2, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
    },
    {
		0x00001457, 0x0000257e, 0x00001457, 0xffffe57f,
		0xffffa57f, 0xffff657f, 0xffff36a5, 0xffff257f,
		0xffff36a5, 0xffff657f, 0xffffa57f, 0xffffe57f,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
    },
    {
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000e77, 0x00001993,
		0x00000d98, 0xffffee49, 0xffffc243, 0xffff9223,
		0xffff660b, 0xffff3c12, 0xffff1397, 0xfffeece8,
		0xfffec851, 0xfffea61a, 0xfffe86e1, 0xfffe6d48,
		0xfffe5a6c, 0xfffe4ee0, 0xfffe4afc, 0xfffe4ee0,
		0xfffe5a6c, 0xfffe6d48, 0xfffe86e1, 0xfffea66e,
		0xfffecafc, 0xfffef36d, 0xffff1e88, 0xffff4afc,
		0xffff7770, 0xffffa28b, 0xffffcafc, 0xffffef8a,
    }
  };

/*-------------------------------------------------------------------------*/

static const int32 cost9_c_int[9] =
{
  	0x00010000,	0x0000fc1c,	0x0000f090,
	0x0000ddb4,	0x0000c41b,	0x0000a48e,
	0x00008000,	0x0000578f,	0x00002c74
};

/*-------------------------------------------------------------------------*/

static const int32 cost12_c1_int[3] = 
{
	0x0001ee8e,
	0x00016a0a,
	0x00008484
};

/*-------------------------------------------------------------------------*/

static const int32 cost12_c2_int[1] =
{
	0x0001bb68
};

/*-------------------------------------------------------------------------*/

static const int32 cost36_c0_int[] = 
{
	0x0001ff83,	0x0001fb9f,	0x0001f3dd,
	0x0001e84e,	0x0001d907,	0x0001c626,
	0x0001afd1,	0x00019632,	0x0001797c,
	0x000159e7,	0x000137b0,	0x00011319,
	0x0000ec6a,	0x0000c3ef,	0x000099f6,
	0x00006ed1,	0x000042d4,	0x00001655
};
/*-------------------------------------------------------------------------*/

static const int32 cost36_c1_int[9] =
{
	0x0001fe0d,	0x0001ee8e,	0x0001d008,
	0x0001a368,	0x00016a0a,	0x000125ac,
	0x0000d861,	0x00008484,	0x00002ca0
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

CMdctInt::CMdctInt (const MPEG_INFO &_info, int _qual)
 : info(_info), qual(_qual)
{
  Init();
}

//-------------------------------------------------------------------------*
//   Init
//-------------------------------------------------------------------------*

void CMdctInt::Init()
{
	memset(prevblck, 0, sizeof(INT_SPECTRUM)); 
}

//-------------------------------------------------------------------------*
//   Apply
//-------------------------------------------------------------------------*

// INT_SPECTRUM: [SBLIMIT][SSLIMIT][2]
void CMdctInt::Apply(int ch, const MP3SI_GRCH &SiGrCh, int32* spectrum)
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

	int ssoffset = ch;
	for ( sb=0; sb<(SBLIMIT>>qual); sb++, ssoffset += (SSLIMIT*2) )
	{
		// get blocktype, override with long for mixed and sb<SbLimMixed
		bt = (SiGrCh.window_switching_flag&&SiGrCh.mixed_block_flag &&(sb<SbLimMixed)) ? 
			0 : SiGrCh.block_type;
		
//		fprintf(stderr, "pre-transform int chunk:\n");
//		for(int32 n = 0; n < SSLIMIT; n++)
//		{
//			fprintf(stderr, "%.2f ", fixed32_to_float(spectrum[(sb*SSLIMIT+n)*2+ch]));
//		}
//		fprintf(stderr, "\n");

		if ( sb < sblim )
		{
			// below limit -> calc i-mdct
			if ( bt == 2 )
				cos_t_h_short(prevblck+ssoffset, spectrum+ssoffset, hybrid_win_int[bt]);
			else
				cos_t_h_long (prevblck+ssoffset, spectrum+ssoffset, hybrid_win_int[bt]);			
		}
		else
		{
			// above limit -> all zero, just (easy version of) overlap and add
			for ( ss=0; ss<SSLIMIT*2; ss+=2 )
			{
				spectrum[ssoffset+ss] = prevblck[ssoffset+ss];
				prevblck[ssoffset+ss] = 0;
			}
		}

		if ( sb&1 )
			for ( ss=2; ss<SSLIMIT*2; ss+=4 )
				spectrum[ssoffset+ss] = -spectrum[ssoffset+ss];
	}
}

/*-------------------------------------------------------------------------*/

// vec is stereo-interleaved; f_vec and win are not.
void CMdctInt::cos_t_h_12 (const int32 *vec, int32 *f_vec, const int32 *win)
{
  int32 v0,v1,v2,v3,v4,v5;
  int32 re0,re1,re2;
  int32 ro0,ro1,ro2;

  /*  stage 0 */
  v0=vec[0]; v1=vec[2*3];v2=vec[4*3]; v3=vec[6*3];v4=vec[8*3];v5=vec[10*3];
  v4-=v5;
  v3-=v4;
  v2-=v3;
  v1-=v2;
  v0-=v1;

  /*  stage 2 */
  v3-=v5;
  v1-=v3;

  v2 = fixed32_mul(v2, cost12_c2_int[0]);
  v3 = fixed32_mul(v3, cost12_c2_int[0]);

  re0 = v0+v4+v2;
  re1 = v0-(v4 << 1);
  re2 = v0+v4-v2;

  ro0 = fixed32_mul(v1+v5+v3, cost12_c1_int[0]);
  ro1 = fixed32_mul(v1-(v5 << 1), cost12_c1_int[1]);
  ro2 = fixed32_mul(v1+v5-v3, cost12_c1_int[2]);

  v0=(re0+ro0);   /*  *c0_0; in win[0..11] precalcd */
  v5=(re0-ro0);   /*  *c0_5; */
  v1=(re1+ro1);   /*  *c0_1; */
  v4=(re1-ro1);   /*  *c0_4; */
  v2=(re2+ro2);   /*  *c0_2; */
  v3=(re2-ro2);   /*  *c0_3; */

  f_vec[8]   += fixed32_mul(v0, win[8]);
  f_vec[9]   += fixed32_mul(v0, win[9]);
  f_vec[7]   += fixed32_mul(v1, win[7]);
  f_vec[10]  += fixed32_mul(v1, win[10]);
  f_vec[6]   += fixed32_mul(v2, win[6]);
  f_vec[11]  += fixed32_mul(v2, win[11]);

  f_vec[0]   += fixed32_mul(v3, win[0]);
  f_vec[5]   += fixed32_mul(v3, win[5]);
  f_vec[1]   += fixed32_mul(v4, win[1]);
  f_vec[4]   += fixed32_mul(v4, win[4]);
  f_vec[2]   += fixed32_mul(v5, win[2]);
  f_vec[3]   += fixed32_mul(v5, win[3]);
}

/*-------------------------------------------------------------------------*/

/* long block type MDCT subroutine */

void CMdctInt::cos_t_h_long (int32 *prev, int32 *dest, const int32 *win)
{
  dest[32] -= dest[34];
  dest[30] -= dest[32];
  dest[28] -= dest[30];
  dest[26] -= dest[28];
  dest[24] -= dest[26];
  dest[22] -= dest[24];
  dest[20] -= dest[22];
  dest[18] -= dest[20];
  dest[16] -= dest[18];
  dest[14] -= dest[16];
  dest[12] -= dest[14];
  dest[10] -= dest[12];
  dest[8]  -= dest[10];
  dest[6]  -= dest[8];
  dest[4]  -= dest[6];
  dest[2]  -= dest[4];
  dest[0]  -= dest[2];

  dest[30] -= dest[34];
  dest[26] -= dest[30];
  dest[22] -= dest[26];
  dest[18] -= dest[22];
  dest[14] -= dest[18];
  dest[10] -= dest[14];
  dest[6]  -= dest[10];
  dest[2]  -= dest[6];

  dest[0]  >>= 1;
  dest[2]  >>= 1;

  cost9(dest,   cost36_rese);
  cost9(dest+2, cost36_reso);

  cost36_reso[0] = fixed32_mul(cost36_reso[0], cost36_c1_int[0]);
  cost36_reso[1] = fixed32_mul(cost36_reso[1], cost36_c1_int[1]);
  cost36_reso[2] = fixed32_mul(cost36_reso[2], cost36_c1_int[2]);
  cost36_reso[3] = fixed32_mul(cost36_reso[3], cost36_c1_int[3]);
  cost36_reso[4] = fixed32_mul(cost36_reso[4], cost36_c1_int[4]);
  cost36_reso[5] = fixed32_mul(cost36_reso[5], cost36_c1_int[5]);
  cost36_reso[6] = fixed32_mul(cost36_reso[6], cost36_c1_int[6]);
  cost36_reso[7] = fixed32_mul(cost36_reso[7], cost36_c1_int[7]);
  cost36_reso[8] = fixed32_mul(cost36_reso[8], cost36_c1_int[8]);

  overlap_hybrid18(prev, dest, win);
}

/*-------------------------------------------------------------------------*/

void CMdctInt::cos_t_h_short (int32 *prev, int32 *dest, const int32 *win)
{
  int32 hybrid_res[36];
  memset(hybrid_res, 0, sizeof(hybrid_res));

  for ( int n = 0; n < 3; n++ )
    cos_t_h_12(dest+n*2,&(hybrid_res[n*6+6]),win);

  for ( int n=0; n<SSLIMIT; n++ )
    {
    dest[n*2] = hybrid_res[n]+prev[n*2];
    prev[n*2] = hybrid_res[n+SSLIMIT];
    }
}

/*-------------------------------------------------------------------------*/

// y is stereo-interleaved; s isn't
void CMdctInt::cost9 (const int32 *y, int32 *s)
{
  int32 tmp1,tmp2;

  s[4] = y[0*2]-y[4*2]+y[8*2]-y[12*2]+y[16*2];

  tmp1 = y[0*2]-y[12*2] +
         fixed32_mul(cost9_c_int[6], y[4*2]-y[8*2]-y[16*2]);
  tmp2 = fixed32_mul(cost9_c_int[3], y[2*2]-y[10*2]-y[14*2]);
  s[1] = tmp1+tmp2;
  s[7] = tmp1-tmp2;

  tmp1=  y[0*2] +
         fixed32_mul(cost9_c_int[2], y[4*2]) +
         fixed32_mul(cost9_c_int[4], y[8*2]) +
         fixed32_mul(cost9_c_int[6], y[12*2]) +
         fixed32_mul(cost9_c_int[8], y[16*2]);
  tmp2=  fixed32_mul(cost9_c_int[1], y[2*2]) +
         fixed32_mul(cost9_c_int[3], y[6*2]) +
         fixed32_mul(cost9_c_int[5], y[10*2]) +
         fixed32_mul(cost9_c_int[7], y[14*2]);
  s[0]=  tmp1+tmp2;
  s[8]=  tmp1-tmp2;

  tmp1 = y[0*2] -
         fixed32_mul(cost9_c_int[8], y[4*2]) -
         fixed32_mul(cost9_c_int[2], y[8*2]) +
         fixed32_mul(cost9_c_int[6], y[12*2]) +
         fixed32_mul(cost9_c_int[4], y[16*2]);
  tmp2 = fixed32_mul(cost9_c_int[5], y[2*2]) -
         fixed32_mul(cost9_c_int[3], y[6*2]) -
         fixed32_mul(cost9_c_int[7], y[10*2]) +
         fixed32_mul(cost9_c_int[1], y[14*2]);
  s[2] = tmp1+tmp2;
  s[6] = tmp1-tmp2;

  tmp1 = y[0*2] -
         fixed32_mul(cost9_c_int[4], y[4*2]) +
         fixed32_mul(cost9_c_int[8], y[8*2]) +
         fixed32_mul(cost9_c_int[6], y[12*2]) -
         fixed32_mul(cost9_c_int[2], y[16*2]);
  tmp2 = fixed32_mul(cost9_c_int[7], y[2*2]) -
         fixed32_mul(cost9_c_int[3], y[6*2]) +
         fixed32_mul(cost9_c_int[1], y[10*2]) -
         fixed32_mul(cost9_c_int[5], y[14*2]);
  s[3] = tmp1+tmp2;
  s[5] = tmp1-tmp2;
}

/*-------------------------------------------------------------------------*/

// prev and dest are stereo-interleaved, win isn't
void CMdctInt::overlap_hybrid18 (int32 *prev, int32 *dest, const int32 *win)
{
  dest[0]    = prev[0] + fixed32_mul(cost36_rese[8]-cost36_reso[8], win[0]);
  dest[34]   = prev[34]+ fixed32_mul(cost36_rese[8]-cost36_reso[8], win[17]);
  prev[0]    = fixed32_mul(cost36_rese[8]+cost36_reso[8], win[18]);
  prev[34]   = fixed32_mul(cost36_rese[8]+cost36_reso[8], win[35]);

  dest[2]    = prev[2] + fixed32_mul(cost36_rese[7]-cost36_reso[7], win[1]);
  dest[32]   = prev[32]+ fixed32_mul(cost36_rese[7]-cost36_reso[7], win[16]);
  prev[2]    = fixed32_mul(cost36_rese[7]+cost36_reso[7], win[19]);
  prev[32]   = fixed32_mul(cost36_rese[7]+cost36_reso[7], win[34]);

  dest[4]    = prev[4]+  fixed32_mul(cost36_rese[6]-cost36_reso[6], win[2]);
  dest[30]   = prev[30]+ fixed32_mul(cost36_rese[6]-cost36_reso[6], win[15]);
  prev[4]    = fixed32_mul(cost36_rese[6]+cost36_reso[6], win[20]);
  prev[30]   = fixed32_mul(cost36_rese[6]+cost36_reso[6], win[33]);

  dest[6]    = prev[6]+  fixed32_mul(cost36_rese[5]-cost36_reso[5], win[3]);
  dest[28]   = prev[28]+ fixed32_mul(cost36_rese[5]-cost36_reso[5], win[14]);
  prev[6]    = fixed32_mul(cost36_rese[5]+cost36_reso[5], win[21]);
  prev[28]   = fixed32_mul(cost36_rese[5]+cost36_reso[5], win[32]);

  dest[8]    = prev[8]+  fixed32_mul(cost36_rese[4]-cost36_reso[4], win[4]);
  dest[26]   = prev[26]+ fixed32_mul(cost36_rese[4]-cost36_reso[4], win[13]);
  prev[8]    = fixed32_mul(cost36_rese[4]+cost36_reso[4], win[22]);
  prev[26]   = fixed32_mul(cost36_rese[4]+cost36_reso[4], win[31]);

  dest[10]   = prev[10]+  fixed32_mul(cost36_rese[3]-cost36_reso[3], win[5]);
  dest[24]   = prev[24]+ fixed32_mul(cost36_rese[3]-cost36_reso[3], win[12]);
  prev[10]   = fixed32_mul(cost36_rese[3]+cost36_reso[3], win[23]);
  prev[24]   = fixed32_mul(cost36_rese[3]+cost36_reso[3], win[30]);

  dest[12]   = prev[12]+  fixed32_mul(cost36_rese[2]-cost36_reso[2], win[6]);
  dest[22]   = prev[22]+ fixed32_mul(cost36_rese[2]-cost36_reso[2], win[11]);
  prev[12]   = fixed32_mul(cost36_rese[2]+cost36_reso[2], win[24]);
  prev[22]   = fixed32_mul(cost36_rese[2]+cost36_reso[2], win[29]);

  dest[14]   = prev[14]+fixed32_mul(cost36_rese[1]-cost36_reso[1], win[7]);
  dest[20]   = prev[20]+fixed32_mul(cost36_rese[1]-cost36_reso[1], win[10]);
  prev[14]   = fixed32_mul(cost36_rese[1]+cost36_reso[1], win[25]);
  prev[20]   = fixed32_mul(cost36_rese[1]+cost36_reso[1], win[28]);

  dest[16]   = prev[16]+fixed32_mul(cost36_rese[0]-cost36_reso[0], win[8]);
  dest[18]   = prev[18]+fixed32_mul(cost36_rese[0]-cost36_reso[0], win[9]);
  prev[16]   = fixed32_mul(cost36_rese[0]+cost36_reso[0], win[26]);
  prev[18]   = fixed32_mul(cost36_rese[0]+cost36_reso[0], win[27]);
}

/*-------------------------------------------------------------------------*/
