/*____________________________________________________________________________
	
	FreeAmp - The Free MP3 Player

        MP3 Decoder originally Copyright (C) 1995-1997 Xing Technology
        Corp.  http://www.xingtech.com

	Portions Copyright (C) 1998 GoodNoise

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
	
	$Id: iupini.c,v 1.3 1999/07/29 21:29:16 elrod Exp $
____________________________________________________________________________*/

/*=========================================================
 initialization for iup.c - include to iup.c
 mpeg audio decoder portable "c"      integer


mods 11/15/95 for Layer I

mods 1/8/97 warnings

=========================================================*/
#include <limits.h>

#ifndef MIN
#define MIN(x, y) ((x)<(y)?(x):(y))
#endif /* MIN */

static long steps[18] =
{
   0, 3, 5, 7, 9, 15, 31, 63, 127,
   255, 511, 1023, 2047, 4095, 8191, 16383, 32767, 65535};
static int stepbits[18] =
{
   0, 2, 3, 3, 4, 4, 5, 6, 7,
   8, 9, 10, 11, 12, 13, 14, 15, 16};

/* ABCD_INDEX = lookqt[mode][sr_index][br_index]  */
/* -1 = invalid  */
static signed char lookqt[4][3][16] =
{
 {{1, -1, -1, -1, 2, -1, 2, 0, 0, 0, 1, 1, 1, 1, 1, -1},	/*  44ks stereo */
  {0, -1, -1, -1, 2, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, -1},	/*  48ks */
  {1, -1, -1, -1, 3, -1, 3, 0, 0, 0, 1, 1, 1, 1, 1, -1}},	/*  32ks */
 {{1, -1, -1, -1, 2, -1, 2, 0, 0, 0, 1, 1, 1, 1, 1, -1},	/*  44ks joint stereo */
  {0, -1, -1, -1, 2, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, -1},	/*  48ks */
  {1, -1, -1, -1, 3, -1, 3, 0, 0, 0, 1, 1, 1, 1, 1, -1}},	/*  32ks */
 {{1, -1, -1, -1, 2, -1, 2, 0, 0, 0, 1, 1, 1, 1, 1, -1},	/*  44ks dual chan */
  {0, -1, -1, -1, 2, -1, 2, 0, 0, 0, 0, 0, 0, 0, 0, -1},	/*  48ks */
  {1, -1, -1, -1, 3, -1, 3, 0, 0, 0, 1, 1, 1, 1, 1, -1}},	/*  32ks */
 {{1, 2, 2, 0, 0, 0, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1},	/*  44ks single chan */
  {0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1},	/*  48ks */
  {1, 3, 3, 0, 0, 0, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1}},	/*  32ks */
};

static long sr_table[8] =
{22050L, 24000L, 16000L, 1L,
 44100L, 48000L, 32000L, 1L};

/* bit allocation table look up */
/* table per mpeg spec tables 3b2a/b/c/d  /e is mpeg2 */
/* look_bat[abcd_index][4][16]  */
static unsigned char look_bat[5][4][16] =
{
/* LOOK_BATA */
 {{0, 1, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17},
  {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 17},
  {0, 1, 2, 3, 4, 5, 6, 17, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 2, 17, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
/* LOOK_BATB */
 {{0, 1, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17},
  {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 17},
  {0, 1, 2, 3, 4, 5, 6, 17, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 2, 17, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
/* LOOK_BATC */
 {{0, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 2, 4, 5, 6, 7, 8, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
/* LOOK_BATD */
 {{0, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 2, 4, 5, 6, 7, 8, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
/* LOOK_BATE */
 {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 2, 4, 5, 6, 7, 8, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 2, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
};

/* look_nbat[abcd_index]][4] */
static unsigned char look_nbat[5][4] =
{
  {3, 8, 12, 4},
  {3, 8, 12, 7},
  {2, 0, 6, 0},
  {2, 0, 10, 0},
  {4, 0, 7, 19},
};


void i_sbt_mono(SAMPLEINT * sample, short *pcm, int n);
void i_sbt_dual(SAMPLEINT * sample, short *pcm, int n);
void i_sbt_dual_mono(SAMPLEINT * sample, short *pcm, int n);
void i_sbt_dual_left(SAMPLEINT * sample, short *pcm, int n);
void i_sbt_dual_right(SAMPLEINT * sample, short *pcm, int n);
void i_sbt16_mono(SAMPLEINT * sample, short *pcm, int n);
void i_sbt16_dual(SAMPLEINT * sample, short *pcm, int n);
void i_sbt16_dual_mono(SAMPLEINT * sample, short *pcm, int n);
void i_sbt16_dual_left(SAMPLEINT * sample, short *pcm, int n);
void i_sbt16_dual_right(SAMPLEINT * sample, short *pcm, int n);
void i_sbt8_mono(SAMPLEINT * sample, short *pcm, int n);
void i_sbt8_dual(SAMPLEINT * sample, short *pcm, int n);
void i_sbt8_dual_mono(SAMPLEINT * sample, short *pcm, int n);
void i_sbt8_dual_left(SAMPLEINT * sample, short *pcm, int n);
void i_sbt8_dual_right(SAMPLEINT * sample, short *pcm, int n);

/*--- 8 bit output ---*/
void i_sbtB_mono(SAMPLEINT * sample, unsigned char *pcm, int n);
void i_sbtB_dual(SAMPLEINT * sample, unsigned char *pcm, int n);
void i_sbtB_dual_mono(SAMPLEINT * sample, unsigned char *pcm, int n);
void i_sbtB_dual_left(SAMPLEINT * sample, unsigned char *pcm, int n);
void i_sbtB_dual_right(SAMPLEINT * sample, unsigned char *pcm, int n);
void i_sbtB16_mono(SAMPLEINT * sample, unsigned char *pcm, int n);
void i_sbtB16_dual(SAMPLEINT * sample, unsigned char *pcm, int n);
void i_sbtB16_dual_mono(SAMPLEINT * sample, unsigned char *pcm, int n);
void i_sbtB16_dual_left(SAMPLEINT * sample, unsigned char *pcm, int n);
void i_sbtB16_dual_right(SAMPLEINT * sample, unsigned char *pcm, int n);
void i_sbtB8_mono(SAMPLEINT * sample, unsigned char *pcm, int n);
void i_sbtB8_dual(SAMPLEINT * sample, unsigned char *pcm, int n);
void i_sbtB8_dual_mono(SAMPLEINT * sample, unsigned char *pcm, int n);
void i_sbtB8_dual_left(SAMPLEINT * sample, unsigned char *pcm, int n);
void i_sbtB8_dual_right(SAMPLEINT * sample, unsigned char *pcm, int n);



static SBT_FUNCTION sbt_table[2][3][5] =
{
 {{i_sbt_mono, i_sbt_dual, i_sbt_dual_mono, i_sbt_dual_left, i_sbt_dual_right},
  {i_sbt16_mono, i_sbt16_dual, i_sbt16_dual_mono, i_sbt16_dual_left, i_sbt16_dual_right},
  {i_sbt8_mono, i_sbt8_dual, i_sbt8_dual_mono, i_sbt8_dual_left, i_sbt8_dual_right}},

 {{(SBT_FUNCTION) i_sbtB_mono,
   (SBT_FUNCTION) i_sbtB_dual,
   (SBT_FUNCTION) i_sbtB_dual_mono,
   (SBT_FUNCTION) i_sbtB_dual_left,
   (SBT_FUNCTION) i_sbtB_dual_right},
  {(SBT_FUNCTION) i_sbtB16_mono,
   (SBT_FUNCTION) i_sbtB16_dual,
   (SBT_FUNCTION) i_sbtB16_dual_mono,
   (SBT_FUNCTION) i_sbtB16_dual_left,
   (SBT_FUNCTION) i_sbtB16_dual_right},
  {(SBT_FUNCTION) i_sbtB8_mono,
   (SBT_FUNCTION) i_sbtB8_dual,
   (SBT_FUNCTION) i_sbtB8_dual_mono,
   (SBT_FUNCTION) i_sbtB8_dual_left,
   (SBT_FUNCTION) i_sbtB8_dual_right}},
};
static int out_chans[5] =
{1, 2, 1, 1, 1};

/*---------------------------------------------------------*/

#ifdef _MSC_VER
#pragma warning(disable: 4056)
#endif

static void table_init()
{
   int i, j;
   int code;
   int bits;
   long tmp, sfmax;

/*--  c_values (dequant) --*/
   for (i = 1; i < 18; i++)
      look_c_value[i] = (int) (32768.0 * 2.0 / steps[i]);
   for (i = 1; i < 18; i++)
      look_c_shift[i] = 16 - stepbits[i];

/*--  scale factor table, scale by 32768 for 16 pcm output  --*/
   bits = MIN(8 * sizeof(SAMPLEINT), 8 * sizeof(sf_table[0]));
   tmp = 1L << (bits - 2);
   sfmax = tmp + (tmp - 1);
   for (i = 0; i < 64; i++)
   {
      tmp = (long) (32768.0 * 2.0 * pow(2.0, -i / 3.0));
      if (tmp > sfmax)
	 tmp = sfmax;
      sf_table[i] = tmp;
   }
/*--  grouped 3 level lookup table 5 bit token --*/
   for (i = 0; i < 32; i++)
   {
      code = i;
      for (j = 0; j < 3; j++)
      {
	 group3_table[i][j] = (char) ((code % 3) - 1);
	 code /= 3;
      }
   }
/*--  grouped 5 level lookup table 7 bit token --*/
   for (i = 0; i < 128; i++)
   {
      code = i;
      for (j = 0; j < 3; j++)
      {
	 group5_table[i][j] = (char) ((code % 5) - 2);
	 code /= 5;
      }
   }
/*--  grouped 9 level lookup table 10 bit token --*/
   for (i = 0; i < 1024; i++)
   {
      code = i;
      for (j = 0; j < 3; j++)
      {
	 group9_table[i][j] = (short) ((code % 9) - 4);
	 code /= 9;
      }
   }


}

#ifdef _MSC_VER
#pragma warning(default: 4056)
#endif

/*---------------------------------------------------------*/
int i_audio_decode_initL1(MPEG_HEAD * h, int framebytes_arg,
		   int reduction_code, int transform_code, int convert_code,
			  int freq_limit);
void i_sbt_init();

/*---------------------------------------------------------*/
/* mpeg_head defined in mhead.h  frame bytes is without pad */
int i_audio_decode_init(MPEG_HEAD * h, int framebytes_arg,
		   int reduction_code, int transform_code, int convert_code,
			int freq_limit)
{
   int i, j, k;
   static int first_pass = 1;
   int abcd_index;
   long samprate;
   int limit;
   int bit_code;

   if (first_pass)
   {
      table_init();
      first_pass = 0;
   }

/* check if code handles */
   if (h->option == 3)		/* layer I */
      return i_audio_decode_initL1(h, framebytes_arg,
		  reduction_code, transform_code, convert_code, freq_limit);
   if (h->option != 2)
      return 0;			/* layer II only */


   unpack_routine = unpack;

   transform_code = transform_code;	/* not used, asm compatability */
   bit_code = 0;
   if (convert_code & 8)
      bit_code = 1;
   convert_code = convert_code & 3;	/* higher bits used by dec8 freq cvt */
   if (reduction_code < 0)
      reduction_code = 0;
   if (reduction_code > 2)
      reduction_code = 2;
   if (freq_limit < 1000)
      freq_limit = 1000;


   framebytes = framebytes_arg;

/* compute abcd index for bit allo table selection */
   if (h->id)			/* mpeg 1 */
      abcd_index = lookqt[h->mode][h->sr_index][h->br_index];
   else
      abcd_index = 4;		/* mpeg 2 */
   for (i = 0; i < 4; i++)
      for (j = 0; j < 16; j++)
	 bat[i][j] = look_bat[abcd_index][i][j];
   for (i = 0; i < 4; i++)
      nbat[i] = look_nbat[abcd_index][i];
   max_sb = nbat[0] + nbat[1] + nbat[2] + nbat[3];
/*----- compute nsb_limit --------*/
   samprate = sr_table[4 * h->id + h->sr_index];
   nsb_limit = (freq_limit * 64L + samprate / 2) / samprate;
/*- caller limit -*/
/*---- limit = 0.94*(32>>reduction_code);  ----*/
   limit = (32 >> reduction_code);
   if (limit > 8)
      limit--;
   if (nsb_limit > limit)
      nsb_limit = limit;
   if (nsb_limit > max_sb)
      nsb_limit = max_sb;

   outvalues = 1152 >> reduction_code;
   if (h->mode != 3)
   {				/* adjust for 2 channel modes */
      for (i = 0; i < 4; i++)
	 nbat[i] *= 2;
      max_sb *= 2;
      nsb_limit *= 2;
   }

/* set sbt function */
   nsbt = 36;
   k = 1 + convert_code;
   if (h->mode == 3)
   {
      k = 0;
   }
   sbt = sbt_table[bit_code][reduction_code][k];
   outvalues *= out_chans[k];
   if (bit_code != 0)
      outbytes = outvalues;
   else
      outbytes = sizeof(short) * outvalues;

   decinfo.channels = out_chans[k];
   decinfo.outvalues = outvalues;
   decinfo.samprate = samprate >> reduction_code;
   if (bit_code != 0)
      decinfo.bits = 8;
   else
      decinfo.bits = sizeof(short) * 8;

   decinfo.framebytes = framebytes;
   decinfo.type = 0;


/* clear sample buffer, unused sub bands must be 0 */
   for (i = 0; i < 2304; i++)
      sample[i] = 0;


/* init sub-band transform */
   i_sbt_init();

   return 1;
}
/*---------------------------------------------------------*/
void i_audio_decode_info(DEC_INFO * info)
{
   *info = decinfo;		/* info return, call after init */
}
/*---------------------------------------------------------*/
