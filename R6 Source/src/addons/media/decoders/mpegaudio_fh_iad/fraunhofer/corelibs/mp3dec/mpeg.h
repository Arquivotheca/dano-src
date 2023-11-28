/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1995)
 *                        All Rights Reserved
 *
 *   filename: mpeg.h
 *   project : ISO/MPEG-Decoder
 *   author  : Markus Werner, addings: Martin Sieler
 *   date    : 1995-07-07
 *   contents/description: HEADER - iso/mpeg-definitions
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/02/16 08:06:44 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/mpeg.h,v 1.4 1999/02/16 08:06:44 sir Exp $
 */

/*-------------------------------------------------------------------------*/

#ifndef __MPEG_H__
#define __MPEG_H__

#include <SupportDefs.h>
#include <Debug.h>
#include <limits.h>

/* ------------------------------------------------------------------------*/

//
// fixed-point type/operation definitions
//

extern "C" {
	// multiply a 32-bit (15.15) value in-place by a 16-bit value (0.15)
	void mx31x16_single(int32* a31, int16* b15);
};

extern int32* mx31x16_sign_mask;

inline int32	fixed32_mul16(int32 a, int16 b)
{
	int32 v = a;
	mx31x16_single(&v, &b);
	return v;
}

inline int32	fixed32_mul16_inline(int32 a, int16 b)
{
	int32 ret, dummy;
	__asm__(
		"movd		%%eax,	%%mm6\n\t"
		"movq		%%mm6,	%%mm7\n\t"
		"pand		mx31x16_sign_mask, %%mm7\n\t"
		"movd		%%edx,	%%mm4\n\t"
		"movq		%%mm4,	%%mm5\n\t"
		"pslld		$16,	%%mm5\n\t"
		"movq		%%mm6,	%%mm0\n\t"
		"psrlw		$1,		%%mm0\n\t"
		"por		%%mm7,	%%mm0\n\t"
		"pmaddwd	%%mm4,	%%mm0\n\t"
		"psrad		$15,	%%mm0\n\t"
		"movq		%%mm6,	%%mm1\n\t"
		"pmaddwd	%%mm5,	%%mm1\n\t"
		"paddd		%%mm1,	%%mm0\n\t"
		"pslld		$1,		%%mm0\n\t"
		"movd		%%mm0,	%%eax\n\t"
		"emms"
		: "=a" (ret), "=d" (dummy)
		: "a" (a), "d" (b));
	return ret;
}

inline int32	fixed32_saturate(int64 value)
{
	if(value > LONG_MAX)
	{
//		debugger("clip+");
		value = LONG_MAX;
	}
	else if(value < LONG_MIN)
	{
//		debugger("clip-");
		value = LONG_MIN;
	}
	return value;
}

inline int32	fixed32_add(int32 a, int32 b)
{
	return fixed32_saturate(int64(a) + int64(b));
}

inline int32	fixed32_sub(int32 a, int32 b)
{
	return fixed32_saturate(int64(a) - int64(b));
}

const int16		fixed16_frac_bits = 15;
const double	fixed16_frac_domain = 32768.;

inline int16	int16_to_fixed16(int16 value)		{ return value << fixed16_frac_bits; }
inline int16	fixed16_to_int16(int16 value)		{ return value >> fixed16_frac_bits; }
inline int16	fixed16_to_int16_round(int16 value)	{ return (value + int16(fixed16_frac_domain)/2) >> fixed16_frac_bits; }

inline int16	float_to_fixed16(float value)		{ return int16(value * fixed16_frac_domain + 0.5); } 
inline float	fixed16_to_float(int16 value)		{ return float(value) / fixed16_frac_domain; }

inline int16	double_to_fixed16(double value)		{ return int16(value * fixed16_frac_domain + 0.5); }
inline double	fixed16_to_double(int16 value)		{ return double(value) / fixed16_frac_domain; }

inline int16	fixed16_saturate(int32 value)
{
	if(value > 32767L)
	{
//		debugger("clip+");
		value = 32767L;
	}
	else if(value < -32768L)
	{
//		debugger("clip-");
		value = -32768L;
	}
	return int16(value);
}

inline int16	fixed16_add(int16 a, int16 b)
{
	return fixed16_saturate(int32(a) + int32(b));
}

inline int16	fixed16_sub(int16 a, int16 b)
{
	return fixed16_saturate(int32(a) - int32(b));
}

inline int16	fixed16_mul_round(int16 a, int16 b)
{
	int32 temp = int32(a) * int32(b);
	temp += int32(fixed16_frac_domain)/2;
	return fixed16_saturate(temp >> fixed16_frac_bits);
}

inline int16	fixed16_mul(int16 a, int16 b)
{
	int32 temp = int32(a) * int32(b);
//	temp += int32(fixed16_frac_domain)/2; // +++++
	return fixed16_saturate(temp >> fixed16_frac_bits);
}

inline int16	fixed16_div(int16 num, int16 denom)
{
	int32 x = int32(num) << fixed16_frac_bits;
	return fixed16_saturate(x / int32(denom));
}

const int32		fixed32_frac_bits = 16;
const double	fixed32_frac_domain = 65536.;

//// precision sacrificed to avoid clipping
//const int32		fixed32_guard_bits = 3;
//const int32		fixed32_guard_domain = 4096; // 2^(32-fixed32_frac_bits-fixed32_guard_bits-1)

inline int32	int32_to_fixed32(int32 value)		{ return value << fixed32_frac_bits; }
inline int32	fixed32_to_int32(int32 value)		{ return value >> fixed32_frac_bits; }
inline int32	fixed32_to_int32_round(int32 value)	{ return (value + int32(fixed32_frac_domain)/2) >> fixed32_frac_bits; }

inline int32	float_to_fixed32(float value)		{ return int32(value * fixed32_frac_domain + 0.5); } 
inline float	fixed32_to_float(int32 value)		{ return float(value) / fixed32_frac_domain; }

inline int32	double_to_fixed32(double value)		{ return int32(value * fixed32_frac_domain + 0.5); }
inline double	fixed32_to_double(int32 value)		{ return double(value) / fixed32_frac_domain; }

inline int32	fixed32_mul_round(int32 a, int32 b)
{
	int64 temp = int64(a) * int64(b);
	return int32(temp >> fixed32_frac_bits);
}

//inline int32	fixed32_mul(int32 a, int32 b)
//{
//	int32 ret, dummy;
//	__asm__(
//		"imull              %%edx\n\t"
//		"shrdl  $16, %%edx, %%eax"
//		: "=a" (ret), "=d" (dummy)
//		: "a" (a), "d" (b));
//	return ret;
//}

// debug version in mp3decode_int.cpp
int32 fixed32_mul(int32 a, int32 b);

inline int32	fixed32_div(int32 num, int32 denom)
{
	int64 x = int64(num) << fixed32_frac_bits;
	return int32(x / int64(denom));
}

/* ------------------------------------------------------------------------*/

//
// MPEG ID (fhgVersion)
//
#define         MPG_MPEG1               1
#define         MPG_MPEG2               0
#define         MPG_MPEG25              2

/* ------------------------------------------------------------------------*/

//
// sample rate
//
#define         MPG_SF_LOW              2

/* ------------------------------------------------------------------------*/

//
// header-mode field
//
#define         MPG_MD_STEREO           0
#define         MPG_MD_JOINT_STEREO     1
#define         MPG_MD_DUAL_CHANNEL     2
#define         MPG_MD_MONO             3

/*-------------------------------------------------------------------------*/

//
// channels
//
#define         MONO                    1
#define         STEREO                  2

/* ------------------------------------------------------------------------*/

//
// subbands, samples/subband
//
#define         SBLIMIT                 32
#define         SSLIMIT                 18

/* ------------------------------------------------------------------------*/

//
// info structure
//
typedef struct
  {
  int  stereo;
  int  sample_rate_ndx;
  int  frame_bits;
  int  mode;
  int  mode_ext;
  int  header_size;
  int  fhgVersion;
  int  protection;
  bool IsMpeg1;
  } MPEG_INFO;

/* ------------------------------------------------------------------------*/

//
// MPEG Layer-3 sideinfo (per channel/granule)
//
typedef struct 
  {
  int part2_3_length;
  int big_values;
  int global_gain;
  int scalefac_compress;
  int window_switching_flag;
  int block_type;
  int mixed_block_flag;
  int table_select[3];
  int subblock_gain[3];
  int region0_count;
  int region1_count;
  int preflag;
  int scalefac_scale;
  int count1table_select;

  // additional calced values
  int intensity_scale; // MPEG 2, MPEG 2.5 only
  int zeroStartNdx;
  int zeroSfbStartNdxIsLong;
  int zeroSfbStartNdxL;
  int zeroSfbStartNdxSMax;
  int zeroSfbStartNdxS[3];
  int zeroSbStartNdx;
  } MP3SI_GRCH;

/* ------------------------------------------------------------------------*/

//
// MPEG Layer-3 sideinfo
//
typedef struct
  {
  int main_data_begin;
  int private_bits;
  struct
    {
    int        scfsi[4];
    MP3SI_GRCH gr[2];
    } ch[2];
  } MP3SI;

/* ------------------------------------------------------------------------*/

//
// MPEG Layer-3 scalefactors
//
typedef struct
  {
  // scalefactors
  int l[23];
  int s[3][13];

  // illegal intensity position
  int l_iip[23];
  int s_iip[13];
  } MP3SCF;

/* ------------------------------------------------------------------------*/

//
// spectrum (as transmitted)
//
typedef float SPECTRUM[2][SBLIMIT][SSLIMIT];

//
// spectrum (after mdct)
//
typedef float POLYSPECTRUM[2][SSLIMIT][SBLIMIT];

//
// fixed-point spectrum buffers (channel-interleaved)
//

typedef int32 INT_SPECTRUM[SBLIMIT*SSLIMIT*2];

/* ------------------------------------------------------------------------*/
#endif
