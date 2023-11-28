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

/* ------------------------------------------------------------------------*/

//
// fixed-point type/operation definitions
//

const int32		fixed32_frac_bits = 16;
const double	fixed32_frac_domain = 65536.;

// precision sacrificed to avoid clipping
const int32		fixed32_guard_bits = 3;
const int32		fixed32_guard_domain = 4096; // 2^(32-fixed32_frac_bits-fixed32_guard_bits-1)

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

inline int32	fixed32_mul(int32 a, int32 b)
{
	int32 ret, dummy;
	__asm__(
		"imull              %%edx\n\t"
		"shrdl  $16, %%edx, %%eax"
		: "=a" (ret), "=d" (dummy)
		: "a" (a), "d" (b));
	return ret;
}

// debug version in mp3decode_int.cpp
//int32 fixed32_mul(int32 a, int32 b);

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
