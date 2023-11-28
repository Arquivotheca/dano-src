
#include "mdct_int.h"
#include <string.h>
#include <stdio.h>

/*-------------------------------------------------------------------------*/

#ifndef min
  #define min(a,b) ((a) < (b) ? (a):(b))
#endif

#ifndef max
  #define max(a,b) ((a) > (b) ? (a):(b))
#endif

/*-------------------------------------------------------------------------*/

extern "C" {
	void CMDCTInt__cost12_h_m_asm(
		const int32 *vec,
		int32 *f_vec,
		const int16* win);
	
	void CMDCTInt__cost_long_combine_m_asm(
		int32 *vec);

	void CMDCTInt__cost9_m_asm(
		const int32 *vec,
		int32 *f_vec);

	void CMDCTInt__cost9_oddscale_m_asm(
		int32 *vec);
	
	void CMDCTInt__overlap_hybrid18_m_asm(
		int32 *prev,
		int32 *dest,
		const int32 *cost36_rese,
		const int32 *cost36_reso,
		const int16* win);

	void CMDCTInt__cost12_h_s_asm(
		const int32 *vec,
		int32 *f_vec,
		const int16* win_l,
		const int16* win_r);
	
	void CMDCTInt__cost_long_combine_s_asm(
		int32 *vec);

	void CMDCTInt__cost9_s_asm(
		const int32 *vec,
		int32 *f_vec);
	
	void CMDCTInt__cost9_oddscale_s_asm(
		int32 *vec);

	void CMDCTInt__overlap_hybrid18_s_asm(
		const int32 *vec,
		int32 *f_vec,
		const int32 *cost36_rese,
		const int32 *cost36_reso,
		const int16* win_l,
		const int16* win_r);
};

/*-------------------------------------------------------------------------*/

// 0.15, scaled by 0.5 from original
static const int16 hybrid_win_int[4][36] =
{
	{
		0x03c5, 0x0a2b, 0x0ee2, 0x11c5, 
		0x12be, 0x11c5, 0x0ee2, 0x0a2b, 
		0x03c5, 0xfbe3, 0xf2bf, 0xe8a3, 
		0xdddc, 0xd2bf, 0xc7a2, 0xbcdc, 
		0xb2bf, 0xa99c, 0xa1b8, 0x9b52, 
		0x969b, 0x93b8, 0x92bf, 0x93b8, 
		0x969b, 0x9b52, 0xa1b8, 0xa99c, 
		0xb2bf, 0xbcdc, 0xc7a2, 0xd2bf, 
		0xdddc, 0xe8a3, 0xf2bf, 0xfbe3, 
	},
	{
		0x03c5, 0x0a2b, 0x0ee2, 0x11c5, 
		0x12be, 0x11c5, 0x0ee2, 0x0a2b, 
		0x03c5, 0xfbe3, 0xf2bf, 0xe8a3, 
		0xdddc, 0xd2bf, 0xc7a2, 0xbcdc, 
		0xb2bf, 0xa99c, 0xa1a1, 0x9a74, 
		0x940c, 0x8e77, 0x89bf, 0x85ed, 
		0x841b, 0x8ac2, 0x9a8d, 0xb228, 
		0xcf70, 0xefb1, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x0000, 0x0000, 
    },
    {
		0x0515, 0x095f, 0x0515, 0xf960, 
		0xe960, 0xd960, 0xcda9, 0xc960, 
		0xcda9, 0xd960, 0xe960, 0xf960, 
		0x0000, 0x0000, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x0000, 0x0000, 
    },
    {
		0x0000, 0x0000, 0x0000, 0x0000, 
		0x0000, 0x0000, 0x039d, 0x0664, 
		0x0366, 0xfb93, 0xf091, 0xe489, 
		0xd983, 0xcf05, 0xc4e6, 0xbb3a, 
		0xb215, 0xa987, 0xa1b8, 0x9b52, 
		0x969b, 0x93b8, 0x92bf, 0x93b8, 
		0x969b, 0x9b52, 0xa1b8, 0xa99c, 
		0xb2bf, 0xbcdc, 0xc7a2, 0xd2bf, 
		0xdddc, 0xe8a3, 0xf2bf, 0xfbe3, 
   }
  };

// inteleaved hybrid_win table for 31x16 multiply
static const int16 hybrid_win_int_lo[4][72] =
{
	{
		0x03c5, 0, 0x0a2b, 0, 0x0ee2, 0, 0x11c5, 0, 
		0x12be, 0, 0x11c5, 0, 0x0ee2, 0, 0x0a2b, 0, 
		0x03c5, 0, 0xfbe3, 0, 0xf2bf, 0, 0xe8a3, 0, 
		0xdddc, 0, 0xd2bf, 0, 0xc7a2, 0, 0xbcdc, 0, 
		0xb2bf, 0, 0xa99c, 0, 0xa1b8, 0, 0x9b52, 0, 
		0x969b, 0, 0x93b8, 0, 0x92bf, 0, 0x93b8, 0, 
		0x969b, 0, 0x9b52, 0, 0xa1b8, 0, 0xa99c, 0, 
		0xb2bf, 0, 0xbcdc, 0, 0xc7a2, 0, 0xd2bf, 0, 
		0xdddc, 0, 0xe8a3, 0, 0xf2bf, 0, 0xfbe3, 0, 
	},
	{
		0x03c5, 0, 0x0a2b, 0, 0x0ee2, 0, 0x11c5, 0, 
		0x12be, 0, 0x11c5, 0, 0x0ee2, 0, 0x0a2b, 0, 
		0x03c5, 0, 0xfbe3, 0, 0xf2bf, 0, 0xe8a3, 0, 
		0xdddc, 0, 0xd2bf, 0, 0xc7a2, 0, 0xbcdc, 0, 
		0xb2bf, 0, 0xa99c, 0, 0xa1a1, 0, 0x9a74, 0, 
		0x940c, 0, 0x8e77, 0, 0x89bf, 0, 0x85ed, 0, 
		0x841b, 0, 0x8ac2, 0, 0x9a8d, 0, 0xb228, 0, 
		0xcf70, 0, 0xefb1, 0, 0x0000, 0, 0x0000, 0, 
		0x0000, 0, 0x0000, 0, 0x0000, 0, 0x0000, 0, 
	},
	{
		0x0515, 0, 0x095f, 0, 0x0515, 0, 0xf960, 0, 
		0xe960, 0, 0xd960, 0, 0xcda9, 0, 0xc960, 0, 
		0xcda9, 0, 0xd960, 0, 0xe960, 0, 0xf960, 0, 
		0x0000, 0, 0x0000, 0, 0x0000, 0, 0x0000, 0, 
		0x0000, 0, 0x0000, 0, 0x0000, 0, 0x0000, 0, 
		0x0000, 0, 0x0000, 0, 0x0000, 0, 0x0000, 0, 
		0x0000, 0, 0x0000, 0, 0x0000, 0, 0x0000, 0, 
		0x0000, 0, 0x0000, 0, 0x0000, 0, 0x0000, 0, 
		0x0000, 0, 0x0000, 0, 0x0000, 0, 0x0000, 0, 
	},
	{
		0x0000, 0, 0x0000, 0, 0x0000, 0, 0x0000, 0, 
		0x0000, 0, 0x0000, 0, 0x039d, 0, 0x0664, 0, 
		0x0366, 0, 0xfb93, 0, 0xf091, 0, 0xe489, 0, 
		0xd983, 0, 0xcf05, 0, 0xc4e6, 0, 0xbb3a, 0, 
		0xb215, 0, 0xa987, 0, 0xa1b8, 0, 0x9b52, 0, 
		0x969b, 0, 0x93b8, 0, 0x92bf, 0, 0x93b8, 0, 
		0x969b, 0, 0x9b52, 0, 0xa1b8, 0, 0xa99c, 0, 
		0xb2bf, 0, 0xbcdc, 0, 0xc7a2, 0, 0xd2bf, 0, 
		0xdddc, 0, 0xe8a3, 0, 0xf2bf, 0, 0xfbe3, 0, 
	}
};

/*-------------------------------------------------------------------------*/

// 0.15
static const int16 cost9_c_int[9] =
{
	0x7fff, 0x7e0e, 0x7847, 
	0x6ed9, 0x620d, 0x5246, 
	0x4000, 0x2bc7, 0x163a, 
};

/*-------------------------------------------------------------------------*/

// 0.15, scaled by 0.5
static const int16 cost12_c1_int[3] = 
{
	0x7ba3, 0x5a82, 0x2120, 
};

/*-------------------------------------------------------------------------*/

// 0.15, scaled by 0.5
static const int16 cost12_c2_int[1] =
{
	0x6ed9,
};

/*-------------------------------------------------------------------------*/

// 0.15, scaled by 0.5
static const int16 cost36_c1_int[9] =
{
	0x7f83, 0x7ba3, 0x7401, 
	0x68d9, 0x5a82, 0x496a, 
	0x3618, 0x2120, 0x0b27, 
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
void 
CMdctInt::Apply(int32 ch, const MP3SI_GRCH &Si, int32 *spectrum)
{
	int sb, ss;
	
	//
	// gr_info->zeroSbStartNdx+1:
	// +1 because we have to process one more subband due to alias-reduction.
	// This wouldn't be neccessary for short blocks and in some cases of 
	// mixed blocks. We don't care about this one extra subband in these cases.
	//
	int sblim = min((SBLIMIT>>qual), Si.zeroSbStartNdx+1);
	int SbLimMixed = ((info.fhgVersion==MPG_MPEG25)&&(info.sample_rate_ndx==MPG_SF_LOW)) ? 4 : 2;

	int ssoffset = ch;
	for ( sb=0; sb<(SBLIMIT>>qual); sb++, ssoffset += (SSLIMIT*2) )
	{
		// get blocktype, override with long for mixed and sb<SbLimMixed
		
		int bt = (Si.window_switching_flag && Si.mixed_block_flag && (sb<SbLimMixed)) ? 
			0 : Si.block_type;

		if ( sb < sblim )
		{
			// below limit -> calc i-mdct
			if ( bt == 2 )
				cos_t_h_short(prevblck+ssoffset, spectrum+ssoffset,
					hybrid_win_int_lo[bt]);
			else
				cos_t_h_long (prevblck+ssoffset, spectrum+ssoffset,
					hybrid_win_int_lo[bt]);
		}
		else
		{
			// above limit -> all zero, just (easy version of) overlap and add
//			memcpy(&spectrum[ssoffset+ss], &prevblck[ssoffset+ss], (SSLIMIT*2)-ss);
//			memset(&prevblck[ssoffset+ss], 0, sizeof(int32)*((SSLIMIT*2)-ss));
			for ( ss=0; ss<SSLIMIT*2; ss+=2 )
			{
				spectrum[ssoffset+ss] = prevblck[ssoffset+ss];
				prevblck[ssoffset+ss] = 0;
			}
		}

		if ( sb&1 )
			for ( ss=2; ss<SSLIMIT*2; ss+=4 )
			{
				spectrum[ssoffset+ss]   = -spectrum[ssoffset+ss];
//				spectrum[ssoffset+ss+1] = -spectrum[ssoffset+ss+1];
			}
	}
}

void 
CMdctInt::Apply(const MP3SI_GRCH &SiL, const MP3SI_GRCH &SiR, int32 *spectrum)
{

	int sblim = max(
		min((SBLIMIT>>qual), SiL.zeroSbStartNdx+1),
		min((SBLIMIT>>qual), SiR.zeroSbStartNdx+1));
	int SbLimMixed = ((info.fhgVersion==MPG_MPEG25)&&(info.sample_rate_ndx==MPG_SF_LOW)) ? 4 : 2;

	int32 ssoffset = 0;
	for (int sb=0; sb<(SBLIMIT>>qual); sb++, ssoffset += (SSLIMIT*2))
	{
		// get blocktype, override with long for mixed and sb<SbLimMixed
		
		int btL = (SiL.window_switching_flag && SiL.mixed_block_flag && (sb<SbLimMixed)) ? 
			0 : SiL.block_type;

		int btR = (SiR.window_switching_flag && SiR.mixed_block_flag && (sb<SbLimMixed)) ? 
			0 : SiR.block_type;
		
		if (sb < sblim)
		{
			// perform i-MDCT
			if (btL == btR)
			{
				// fast case: process in stereo
				if (btL == 2)
					cos_t_h_short(prevblck+ssoffset, spectrum+ssoffset,
						hybrid_win_int_lo[btL],
						hybrid_win_int_lo[btR]);
				else
					cos_t_h_long(prevblck+ssoffset, spectrum+ssoffset,
						hybrid_win_int_lo[btL],
						hybrid_win_int_lo[btR]);
			}
			else
			{
				// left
				if (btL == 2)
					cos_t_h_short(prevblck+ssoffset, spectrum+ssoffset,
						hybrid_win_int_lo[btL]);
				else
					cos_t_h_long(prevblck+ssoffset, spectrum+ssoffset,
						hybrid_win_int_lo[btL]);
				// right
				if (btR == 2)
					cos_t_h_short(prevblck+ssoffset+1, spectrum+ssoffset+1,
						hybrid_win_int_lo[btR]);
				else
					cos_t_h_long(prevblck+ssoffset+1, spectrum+ssoffset+1,
						hybrid_win_int_lo[btR]);
			}
		}
		else
		{
			// above limit -> all zero, just (easy version of) overlap and add
			memcpy(spectrum + ssoffset, prevblck + ssoffset, sizeof(int32)*(SSLIMIT*2));
			memset(prevblck + ssoffset, 0, sizeof(int32)*(SSLIMIT*2));
		}

		if (sb & 1)
		{
			// invert odd subbands
			for (int ss = 2; ss < SSLIMIT*2; ss += 4)
			{
				spectrum[ssoffset+ss]   = -spectrum[ssoffset+ss];
				spectrum[ssoffset+ss+1] = -spectrum[ssoffset+ss+1];
			}
		}
	}
}

/*-------------------------------------------------------------------------*/

/* long block type MDCT subroutine */

void 
CMdctInt::cos_t_h_long(int32 *prev, int32 *dest, const int16 *win)
{

	CMDCTInt__cost_long_combine_m_asm(dest);

	CMDCTInt__cost9_m_asm(dest,   cost36_rese);
	CMDCTInt__cost9_m_asm(dest+2, cost36_reso);
  
	CMDCTInt__cost9_oddscale_m_asm(cost36_reso);

	CMDCTInt__overlap_hybrid18_m_asm(prev, dest, cost36_rese, cost36_reso, win);
}

void 
CMdctInt::cos_t_h_short(int32 *prev, int32 *dest, const int16 *win)
{
	int32 hybrid_res[36];
	memset(hybrid_res, 0, sizeof(hybrid_res));
	
	for ( int n = 0; n < 3; n++ )
	{
		CMDCTInt__cost12_h_m_asm(dest+n*2, &hybrid_res[n*6+6], win);
	}
	
	for ( int n=0; n<SSLIMIT; n++ )
	{
		dest[n*2] = fixed32_add(hybrid_res[n], prev[n*2]); // +++++
		prev[n*2] = hybrid_res[n+SSLIMIT];
	}
}

void 
CMdctInt::cos_t_h_long(int32 *prev, int32 *dest, const int16 *winL, const int16 *winR)
{
	CMDCTInt__cost_long_combine_s_asm(dest);
	CMDCTInt__cost9_s_asm(dest,   cost36_rese);
	CMDCTInt__cost9_s_asm(dest+2, cost36_reso);
	CMDCTInt__cost9_oddscale_s_asm(cost36_reso);
	CMDCTInt__overlap_hybrid18_s_asm(prev, dest, cost36_rese, cost36_reso, winL, winR);
}

void 
CMdctInt::cos_t_h_short(int32 *prev, int32 *dest, const int16 *winL, const int16 *winR)
{
	int32 hybrid_res[72];
	memset(hybrid_res, 0, sizeof(hybrid_res));
	
	for(int n = 0; n < 3; n++)
	{
		CMDCTInt__cost12_h_s_asm(dest+n*2, &hybrid_res[n*12+12], winL, winR);
	}
	
	for(int n = 0; n < SSLIMIT*2; n += 2)
	{
		dest[n] = fixed32_add(hybrid_res[n], prev[n]); // +++++
		dest[n+1] = fixed32_add(hybrid_res[n+1], prev[n+1]); // +++++
		prev[n] = hybrid_res[n+SSLIMIT*2];
		prev[n+1] = hybrid_res[n+SSLIMIT*2+1];
	}
}

/*-------------------------------------------------------------------------*/
