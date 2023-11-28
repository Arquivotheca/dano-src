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
	
	$Id: cupl3.c,v 1.7 1999/07/13 18:42:15 robert Exp $
____________________________________________________________________________*/

/****  cupL3.c  ***************************************************
unpack Layer III


mod 8/18/97  bugfix crc problem

mod 10/9/97  add band_limit12 for short blocks

mod 10/22/97  zero buf_ptrs in init

mod 5/15/98 mpeg 2.5

mod 8/19/98 decode 22 sf bands

******************************************************************/

/*---------------------------------------
TO DO: Test mixed blocks (mixed long/short)
  No mixed blocks in mpeg-1 test stream being used for development

-----------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
#include <memory.h>
#include <string.h>
#include <assert.h>
#include "mhead.h"		/* mpeg header structure */
#include "L3.h"
#include "port.h"



/*====================================================================*/
static int mp_sr20_table[2][4] =
{{441, 480, 320, -999}, {882, 960, 640, -999}};
static int mp_br_tableL3[2][16] =
{{0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0},	/* mpeg 2 */
 {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0}};

/*====================================================================*/

/*-- global band tables */
/*-- short portion is 3*x !! --*/
int nBand[2][22];		/* [long/short][cb] */
int sfBandIndex[2][22];		/* [long/short][cb] */

/*====================================================================*/

/*----------------*/
extern DEC_INFO decinfo;

/*----------------*/
static int mpeg25_flag;

int iframe;

/*-------*/
static int band_limit = (576);
static int band_limit21 = (576);	// limit for sf band 21

static int band_limit12 = (576);	// limit for sf band 12 short

int band_limit_nsb = 32;	/* global for hybrid */
static int nsb_limit = 32;
static int gain_adjust = 0;	/* adjust gain e.g. cvt to mono */
static int id;
static int ncbl_mixed;		/* number of long cb's in mixed block 8 or 6 */
static int sr_index;
static int outvalues;
static int outbytes;
static int half_outbytes;
static int framebytes;
static int padframebytes;
static int crcbytes;
static int pad;
static int stereo_flag;
static int nchan;
static int ms_mode;
static int is_mode;
static unsigned int zero_level_pcm = 0;

/* cb_info[igr][ch], compute by dequant, used by joint */
static CB_INFO cb_info[2][2];
static IS_SF_INFO is_sf_info;	/* MPEG-2 intensity stereo */

/*---------------------------------*/
/* main data bit buffer */
#define NBUF (8*1024)
#define BUF_TRIGGER (NBUF-1500)
static unsigned char buf[NBUF];
static int buf_ptr0 = 0;
static int buf_ptr1 = 0;
static int main_pos_bit;

/*---------------------------------*/
static SIDE_INFO side_info;

static SCALEFACT sf[2][2];	/* [gr][ch] */

static int nsamp[2][2];		/* must start = 0, for nsamp[igr_prev] */

/*- sample union of int/float  sample[ch][gr][576] */
/* static SAMPLE sample[2][2][576]; */
extern SAMPLE sample[2][2][576];
static float yout[576];		/* hybrid out, sbt in */

typedef void (*SBT_FUNCTION) (float *sample, short *pcm, int ch);
void sbt_dual_L3(float *sample, short *pcm, int n);
static SBT_FUNCTION sbt_L3 = sbt_dual_L3;

typedef void (*XFORM_FUNCTION) (void *pcm, int igr);
static void Xform_dual(void *pcm, int igr);
static XFORM_FUNCTION Xform = Xform_dual;

IN_OUT L3audio_decode_MPEG1(unsigned char *bs, unsigned char *pcm);
IN_OUT L3audio_decode_MPEG2(unsigned char *bs, unsigned char *pcm);
typedef IN_OUT(*DECODE_FUNCTION) (unsigned char *bs, unsigned char *pcm);
static DECODE_FUNCTION decode_function = L3audio_decode_MPEG1;


/*====================================================================*/
int hybrid(void *xin, void *xprev, float *y,
	   int btype, int nlong, int ntot, int nprev);
int hybrid_sum(void *xin, void *xin_left, float *y,
	       int btype, int nlong, int ntot);
void sum_f_bands(void *a, void *b, int n);
void FreqInvert(float *y, int n);
void antialias(void *x, int n);
void ms_process(void *x, int n);	/* sum-difference stereo */
void is_process_MPEG1(void *x,	/* intensity stereo */
		      SCALEFACT * sf,
		      CB_INFO cb_info[2],	/* [ch] */
		      int nsamp, int ms_mode);
void is_process_MPEG2(void *x,	/* intensity stereo */
		      SCALEFACT * sf,
		      CB_INFO cb_info[2],	/* [ch] */
		      IS_SF_INFO * is_sf_info,
		      int nsamp, int ms_mode);

void unpack_huff(void *xy, int n, int ntable);
int unpack_huff_quad(void *vwxy, int n, int nbits, int ntable);
void dequant(SAMPLE sample[], int *nsamp,
	     SCALEFACT * sf,
	     GR * gr,
	     CB_INFO * cb_info, int ncbl_mixed);
void unpack_sf_sub_MPEG1(SCALEFACT * scalefac, GR * gr,
			 int scfsi,	/* bit flag */
			 int igr);
void unpack_sf_sub_MPEG2(SCALEFACT sf[],	/* return intensity scale */
			 GR * grdat,
			 int is_and_ch, IS_SF_INFO * is_sf_info);

/*====================================================================*/
/* get bits from bitstream in endian independent way */

BITDAT bitdat;			/* global for inline use by Huff */

/*------------- initialize bit getter -------------*/
static void bitget_init(unsigned char *buf)
{
   bitdat.bs_ptr0 = bitdat.bs_ptr = buf;
   bitdat.bits = 0;
   bitdat.bitbuf = 0;
}
/*------------- initialize bit getter -------------*/
static void bitget_init_end(unsigned char *buf_end)
{
   bitdat.bs_ptr_end = buf_end;
}
/*------------- get n bits from bitstream -------------*/
int bitget_bits_used()
{
   int n;			/* compute bits used from last init call */

   n = ((bitdat.bs_ptr - bitdat.bs_ptr0) << 3) - bitdat.bits;
   return n;
}
/*------------- check for n bits in bitbuf -------------*/
void bitget_check(int n)
{
   if (bitdat.bits < n)
   {
      while (bitdat.bits <= 24)
      {
	 bitdat.bitbuf = (bitdat.bitbuf << 8) | *bitdat.bs_ptr++;
	 bitdat.bits += 8;
      }
   }
}
/*------------- get n bits from bitstream -------------*/
unsigned int bitget(int n)
{
   unsigned int x;

   if (bitdat.bits < n)
   {				/* refill bit buf if necessary */
      while (bitdat.bits <= 24)
      {
	 bitdat.bitbuf = (bitdat.bitbuf << 8) | *bitdat.bs_ptr++;
	 bitdat.bits += 8;
      }
   }
   bitdat.bits -= n;
   x = bitdat.bitbuf >> bitdat.bits;
   bitdat.bitbuf -= x << bitdat.bits;
   return x;
}
/*------------- get 1 bit from bitstream -------------*/
unsigned int bitget_1bit()
{
   unsigned int x;

   if (bitdat.bits <= 0)
   {				/* refill bit buf if necessary */
      while (bitdat.bits <= 24)
      {
	 bitdat.bitbuf = (bitdat.bitbuf << 8) | *bitdat.bs_ptr++;
	 bitdat.bits += 8;
      }
   }
   bitdat.bits--;
   x = bitdat.bitbuf >> bitdat.bits;
   bitdat.bitbuf -= x << bitdat.bits;
   return x;
}
/*====================================================================*/
static void Xform_mono(void *pcm, int igr)
{
   int igr_prev, n1, n2;

/*--- hybrid + sbt ---*/
   n1 = n2 = nsamp[igr][0];	/* total number bands */
   if (side_info.gr[igr][0].block_type == 2)
   {				/* long bands */
      n1 = 0;
      if (side_info.gr[igr][0].mixed_block_flag)
	 n1 = sfBandIndex[0][ncbl_mixed - 1];
   }
   if (n1 > band_limit)
      n1 = band_limit;
   if (n2 > band_limit)
      n2 = band_limit;
   igr_prev = igr ^ 1;

   nsamp[igr][0] = hybrid(sample[0][igr], sample[0][igr_prev],
	 yout, side_info.gr[igr][0].block_type, n1, n2, nsamp[igr_prev][0]);
   FreqInvert(yout, nsamp[igr][0]);
   sbt_L3(yout, pcm, 0);

}
/*--------------------------------------------------------------------*/
static void Xform_dual_right(void *pcm, int igr)
{
   int igr_prev, n1, n2;

/*--- hybrid + sbt ---*/
   n1 = n2 = nsamp[igr][1];	/* total number bands */
   if (side_info.gr[igr][1].block_type == 2)
   {				/* long bands */
      n1 = 0;
      if (side_info.gr[igr][1].mixed_block_flag)
	 n1 = sfBandIndex[0][ncbl_mixed - 1];
   }
   if (n1 > band_limit)
      n1 = band_limit;
   if (n2 > band_limit)
      n2 = band_limit;
   igr_prev = igr ^ 1;
   nsamp[igr][1] = hybrid(sample[1][igr], sample[1][igr_prev],
	 yout, side_info.gr[igr][1].block_type, n1, n2, nsamp[igr_prev][1]);
   FreqInvert(yout, nsamp[igr][1]);
   sbt_L3(yout, pcm, 0);

}
/*--------------------------------------------------------------------*/
static void Xform_dual(void *pcm, int igr)
{
   int ch;
   int igr_prev, n1, n2;

/*--- hybrid + sbt ---*/
   igr_prev = igr ^ 1;
   for (ch = 0; ch < nchan; ch++)
   {
      n1 = n2 = nsamp[igr][ch];	/* total number bands */
      if (side_info.gr[igr][ch].block_type == 2)
      {				/* long bands */
	 n1 = 0;
	 if (side_info.gr[igr][ch].mixed_block_flag)
	    n1 = sfBandIndex[0][ncbl_mixed - 1];
      }
      if (n1 > band_limit)
	 n1 = band_limit;
      if (n2 > band_limit)
	 n2 = band_limit;
      nsamp[igr][ch] = hybrid(sample[ch][igr], sample[ch][igr_prev],
       yout, side_info.gr[igr][ch].block_type, n1, n2, nsamp[igr_prev][ch]);
      FreqInvert(yout, nsamp[igr][ch]);
      sbt_L3(yout, pcm, ch);
   }

}
/*--------------------------------------------------------------------*/
static void Xform_dual_mono(void *pcm, int igr)
{
   int igr_prev, n1, n2, n3;

/*--- hybrid + sbt ---*/
   igr_prev = igr ^ 1;
   if ((side_info.gr[igr][0].block_type == side_info.gr[igr][1].block_type)
       && (side_info.gr[igr][0].mixed_block_flag == 0)
       && (side_info.gr[igr][1].mixed_block_flag == 0))
   {

      n2 = nsamp[igr][0];	/* total number bands max of L R */
      if (n2 < nsamp[igr][1])
	 n2 = nsamp[igr][1];
      if (n2 > band_limit)
	 n2 = band_limit;
      n1 = n2;			/* n1 = number long bands */
      if (side_info.gr[igr][0].block_type == 2)
	 n1 = 0;
      sum_f_bands(sample[0][igr], sample[1][igr], n2);
      n3 = nsamp[igr][0] = hybrid(sample[0][igr], sample[0][igr_prev],
	 yout, side_info.gr[igr][0].block_type, n1, n2, nsamp[igr_prev][0]);
   }
   else
   {				/* transform and then sum (not tested - never happens in test) */
/*-- left chan --*/
      n1 = n2 = nsamp[igr][0];	/* total number bands */
      if (side_info.gr[igr][0].block_type == 2)
      {
	 n1 = 0;		/* long bands */
	 if (side_info.gr[igr][0].mixed_block_flag)
	    n1 = sfBandIndex[0][ncbl_mixed - 1];
      }
      n3 = nsamp[igr][0] = hybrid(sample[0][igr], sample[0][igr_prev],
	 yout, side_info.gr[igr][0].block_type, n1, n2, nsamp[igr_prev][0]);
/*-- right chan --*/
      n1 = n2 = nsamp[igr][1];	/* total number bands */
      if (side_info.gr[igr][1].block_type == 2)
      {
	 n1 = 0;		/* long bands */
	 if (side_info.gr[igr][1].mixed_block_flag)
	    n1 = sfBandIndex[0][ncbl_mixed - 1];
      }
      nsamp[igr][1] = hybrid_sum(sample[1][igr], sample[0][igr],
			     yout, side_info.gr[igr][1].block_type, n1, n2);
      if (n3 < nsamp[igr][1])
	 n1 = nsamp[igr][1];
   }

/*--------*/
   FreqInvert(yout, n3);
   sbt_L3(yout, pcm, 0);

}
/*--------------------------------------------------------------------*/
/*====================================================================*/
static int unpack_side_MPEG1()
{
   int prot;
   int br_index;
   int igr, ch;
   int side_bytes;

/* decode partial header plus initial side info */
/* at entry bit getter points at id, sync skipped by caller */

   id = bitget(1);		/* id */
   bitget(2);			/* skip layer */
   prot = bitget(1);		/* bitget prot bit */
   br_index = bitget(4);
   sr_index = bitget(2);
   pad = bitget(1);
   bitget(1);			/* skip to mode */
   side_info.mode = bitget(2);	/* mode */
   side_info.mode_ext = bitget(2);	/* mode ext */

   if (side_info.mode != 1)
      side_info.mode_ext = 0;

/* adjust global gain in ms mode to avoid having to mult by 1/sqrt(2) */
   ms_mode = side_info.mode_ext >> 1;
   is_mode = side_info.mode_ext & 1;


   crcbytes = 0;
   if (prot)
      bitget(4);		/* skip to data */
   else
   {
      bitget(20);		/* skip crc */
      crcbytes = 2;
   }

   if (br_index > 0)		/* framebytes fixed for free format */
	{
      framebytes =
	 2880 * mp_br_tableL3[id][br_index] / mp_sr20_table[id][sr_index];
   }

   side_info.main_data_begin = bitget(9);
   if (side_info.mode == 3)
   {
      side_info.private_bits = bitget(5);
      nchan = 1;
      stereo_flag = 0;
      side_bytes = (4 + 17);
/*-- with header --*/
   }
   else
   {
      side_info.private_bits = bitget(3);
      nchan = 2;
      stereo_flag = 1;
      side_bytes = (4 + 32);
/*-- with header --*/
   }
   for (ch = 0; ch < nchan; ch++)
      side_info.scfsi[ch] = bitget(4);
/* this always 0 (both igr) for short blocks */

   for (igr = 0; igr < 2; igr++)
   {
      for (ch = 0; ch < nchan; ch++)
      {
	 side_info.gr[igr][ch].part2_3_length = bitget(12);
	 side_info.gr[igr][ch].big_values = bitget(9);
	 side_info.gr[igr][ch].global_gain = bitget(8) + gain_adjust;
	 if (ms_mode)
	    side_info.gr[igr][ch].global_gain -= 2;
	 side_info.gr[igr][ch].scalefac_compress = bitget(4);
	 side_info.gr[igr][ch].window_switching_flag = bitget(1);
	 if (side_info.gr[igr][ch].window_switching_flag)
	 {
	    side_info.gr[igr][ch].block_type = bitget(2);
	    side_info.gr[igr][ch].mixed_block_flag = bitget(1);
	    side_info.gr[igr][ch].table_select[0] = bitget(5);
	    side_info.gr[igr][ch].table_select[1] = bitget(5);
	    side_info.gr[igr][ch].subblock_gain[0] = bitget(3);
	    side_info.gr[igr][ch].subblock_gain[1] = bitget(3);
	    side_info.gr[igr][ch].subblock_gain[2] = bitget(3);
	  /* region count set in terms of long block cb's/bands */
	  /* r1 set so r0+r1+1 = 21 (lookup produces 576 bands ) */
	  /* if(window_switching_flag) always 36 samples in region0 */
	    side_info.gr[igr][ch].region0_count = (8 - 1);	/* 36 samples */
	    side_info.gr[igr][ch].region1_count = 20 - (8 - 1);
	 }
	 else
	 {
	    side_info.gr[igr][ch].mixed_block_flag = 0;
	    side_info.gr[igr][ch].block_type = 0;
	    side_info.gr[igr][ch].table_select[0] = bitget(5);
	    side_info.gr[igr][ch].table_select[1] = bitget(5);
	    side_info.gr[igr][ch].table_select[2] = bitget(5);
	    side_info.gr[igr][ch].region0_count = bitget(4);
	    side_info.gr[igr][ch].region1_count = bitget(3);
	 }
	 side_info.gr[igr][ch].preflag = bitget(1);
	 side_info.gr[igr][ch].scalefac_scale = bitget(1);
	 side_info.gr[igr][ch].count1table_select = bitget(1);
      }
   }



/* return  bytes in header + side info */
   return side_bytes;
}
/*====================================================================*/
static int unpack_side_MPEG2(int igr)
{
   int prot;
   int br_index;
   int ch;
   int side_bytes;

/* decode partial header plus initial side info */
/* at entry bit getter points at id, sync skipped by caller */

   id = bitget(1);		/* id */
   bitget(2);			/* skip layer */
   prot = bitget(1);		/* bitget prot bit */
   br_index = bitget(4);
   sr_index = bitget(2);
   pad = bitget(1);
   bitget(1);			/* skip to mode */
   side_info.mode = bitget(2);	/* mode */
   side_info.mode_ext = bitget(2);	/* mode ext */

   if (side_info.mode != 1)
      side_info.mode_ext = 0;

/* adjust global gain in ms mode to avoid having to mult by 1/sqrt(2) */
   ms_mode = side_info.mode_ext >> 1;
   is_mode = side_info.mode_ext & 1;

   crcbytes = 0;
   if (prot)
      bitget(4);		/* skip to data */
   else
   {
      bitget(20);		/* skip crc */
      crcbytes = 2;
   }

   if (br_index > 0)
   {				/* framebytes fixed for free format */
      if (mpeg25_flag == 0)
      {
	 framebytes =
	    1440 * mp_br_tableL3[id][br_index] / mp_sr20_table[id][sr_index];
      }
      else
      {
	 framebytes =
	    2880 * mp_br_tableL3[id][br_index] / mp_sr20_table[id][sr_index];
       //if( sr_index == 2 ) return 0;  // fail mpeg25 8khz
      }
   }
   side_info.main_data_begin = bitget(8);
   if (side_info.mode == 3)
   {
      side_info.private_bits = bitget(1);
      nchan = 1;
      stereo_flag = 0;
      side_bytes = (4 + 9);
/*-- with header --*/
   }
   else
   {
      side_info.private_bits = bitget(2);
      nchan = 2;
      stereo_flag = 1;
      side_bytes = (4 + 17);
/*-- with header --*/
   }
   side_info.scfsi[1] = side_info.scfsi[0] = 0;


   for (ch = 0; ch < nchan; ch++)
   {
      side_info.gr[igr][ch].part2_3_length = bitget(12);
      side_info.gr[igr][ch].big_values = bitget(9);
      side_info.gr[igr][ch].global_gain = bitget(8) + gain_adjust;
      if (ms_mode)
	 side_info.gr[igr][ch].global_gain -= 2;
      side_info.gr[igr][ch].scalefac_compress = bitget(9);
      side_info.gr[igr][ch].window_switching_flag = bitget(1);
      if (side_info.gr[igr][ch].window_switching_flag)
      {
	 side_info.gr[igr][ch].block_type = bitget(2);
	 side_info.gr[igr][ch].mixed_block_flag = bitget(1);
	 side_info.gr[igr][ch].table_select[0] = bitget(5);
	 side_info.gr[igr][ch].table_select[1] = bitget(5);
	 side_info.gr[igr][ch].subblock_gain[0] = bitget(3);
	 side_info.gr[igr][ch].subblock_gain[1] = bitget(3);
	 side_info.gr[igr][ch].subblock_gain[2] = bitget(3);
       /* region count set in terms of long block cb's/bands  */
       /* r1 set so r0+r1+1 = 21 (lookup produces 576 bands ) */
       /* bt=1 or 3       54 samples */
       /* bt=2 mixed=0    36 samples */
       /* bt=2 mixed=1    54 (8 long sf) samples? or maybe 36 */
       /* region0 discussion says 54 but this would mix long */
       /* and short in region0 if scale factors switch */
       /* at band 36 (6 long scale factors) */
	 if ((side_info.gr[igr][ch].block_type == 2))
	 {
	    side_info.gr[igr][ch].region0_count = (6 - 1);	/* 36 samples */
	    side_info.gr[igr][ch].region1_count = 20 - (6 - 1);
	 }
	 else
	 {			/* long block type 1 or 3 */
	    side_info.gr[igr][ch].region0_count = (8 - 1);	/* 54 samples */
	    side_info.gr[igr][ch].region1_count = 20 - (8 - 1);
	 }
      }
      else
      {
	 side_info.gr[igr][ch].mixed_block_flag = 0;
	 side_info.gr[igr][ch].block_type = 0;
	 side_info.gr[igr][ch].table_select[0] = bitget(5);
	 side_info.gr[igr][ch].table_select[1] = bitget(5);
	 side_info.gr[igr][ch].table_select[2] = bitget(5);
	 side_info.gr[igr][ch].region0_count = bitget(4);
	 side_info.gr[igr][ch].region1_count = bitget(3);
      }
      side_info.gr[igr][ch].preflag = 0;
      side_info.gr[igr][ch].scalefac_scale = bitget(1);
      side_info.gr[igr][ch].count1table_select = bitget(1);
   }

/* return  bytes in header + side info */
   return side_bytes;
}
/*-----------------------------------------------------------------*/
static void unpack_main(unsigned char *pcm, int igr)
{
   int ch;
   int bit0;
   int n1, n2, n3, n4, nn2, nn3;
   int nn4;
   int qbits;
   int m0;


   for (ch = 0; ch < nchan; ch++)
   {
      bitget_init(buf + (main_pos_bit >> 3));
      bit0 = (main_pos_bit & 7);
      if (bit0)
	 bitget(bit0);
      main_pos_bit += side_info.gr[igr][ch].part2_3_length;
      bitget_init_end(buf + ((main_pos_bit + 39) >> 3));
/*-- scale factors --*/
      if (id)
	 unpack_sf_sub_MPEG1(&sf[igr][ch],
			  &side_info.gr[igr][ch], side_info.scfsi[ch], igr);
      else
	 unpack_sf_sub_MPEG2(&sf[igr][ch],
			 &side_info.gr[igr][ch], is_mode & ch, &is_sf_info);
/*--- huff data ---*/
      n1 = sfBandIndex[0][side_info.gr[igr][ch].region0_count];
      n2 = sfBandIndex[0][side_info.gr[igr][ch].region0_count
			  + side_info.gr[igr][ch].region1_count + 1];
      n3 = side_info.gr[igr][ch].big_values;
      n3 = n3 + n3;


      if (n3 > band_limit)
	 n3 = band_limit;
      if (n2 > n3)
	 n2 = n3;
      if (n1 > n3)
	 n1 = n3;
      nn3 = n3 - n2;
      nn2 = n2 - n1;
      unpack_huff(sample[ch][igr], n1, side_info.gr[igr][ch].table_select[0]);
      unpack_huff(sample[ch][igr] + n1, nn2, side_info.gr[igr][ch].table_select[1]);
      unpack_huff(sample[ch][igr] + n2, nn3, side_info.gr[igr][ch].table_select[2]);
      qbits = side_info.gr[igr][ch].part2_3_length - (bitget_bits_used() - bit0);
      nn4 = unpack_huff_quad(sample[ch][igr] + n3, band_limit - n3, qbits,
			     side_info.gr[igr][ch].count1table_select);
      n4 = n3 + nn4;
      nsamp[igr][ch] = n4;
    //limit n4 or allow deqaunt to sf band 22
      if (side_info.gr[igr][ch].block_type == 2)
	 n4 = min(n4, band_limit12);
      else
	 n4 = min(n4, band_limit21);
      if (n4 < 576)
	 memset(sample[ch][igr] + n4, 0, sizeof(SAMPLE) * (576 - n4));
      if (bitdat.bs_ptr > bitdat.bs_ptr_end)
      {				// bad data overrun

	 memset(sample[ch][igr], 0, sizeof(SAMPLE) * (576));
      }
   }



/*--- dequant ---*/
   for (ch = 0; ch < nchan; ch++)
   {
      dequant(sample[ch][igr],
	      &nsamp[igr][ch],	/* nsamp updated for shorts */
	      &sf[igr][ch], &side_info.gr[igr][ch],
	      &cb_info[igr][ch], ncbl_mixed);
   }

/*--- ms stereo processing  ---*/
   if (ms_mode)
   {
      if (is_mode == 0)
      {
	 m0 = nsamp[igr][0];	/* process to longer of left/right */
	 if (m0 < nsamp[igr][1])
	    m0 = nsamp[igr][1];
      }
      else
      {				/* process to last cb in right */
	 m0 = sfBandIndex[cb_info[igr][1].cbtype][cb_info[igr][1].cbmax];
      }
      ms_process(sample[0][igr], m0);
   }

/*--- is stereo processing  ---*/
   if (is_mode)
   {
      if (id)
	 is_process_MPEG1(sample[0][igr], &sf[igr][1],
			  cb_info[igr], nsamp[igr][0], ms_mode);
      else
	 is_process_MPEG2(sample[0][igr], &sf[igr][1],
			  cb_info[igr], &is_sf_info,
			  nsamp[igr][0], ms_mode);
   }

/*-- adjust ms and is modes to max of left/right */
   if (side_info.mode_ext)
   {
      if (nsamp[igr][0] < nsamp[igr][1])
	 nsamp[igr][0] = nsamp[igr][1];
      else
	 nsamp[igr][1] = nsamp[igr][0];
   }

/*--- antialias ---*/
   for (ch = 0; ch < nchan; ch++)
   {
      if (cb_info[igr][ch].ncbl == 0)
	 continue;		/* have no long blocks */
      if (side_info.gr[igr][ch].mixed_block_flag)
	 n1 = 1;		/* 1 -> 36 samples */
      else
	 n1 = (nsamp[igr][ch] + 7) / 18;
      if (n1 > 31)
	 n1 = 31;
      antialias(sample[ch][igr], n1);
      n1 = 18 * n1 + 8;		/* update number of samples */
      if (n1 > nsamp[igr][ch])
	 nsamp[igr][ch] = n1;
   }



/*--- hybrid + sbt ---*/
   Xform(pcm, igr);


/*-- done --*/
}
/*--------------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
IN_OUT L3audio_decode(unsigned char *bs, unsigned char *pcm)
{
   return decode_function(bs, pcm);
}

/*--------------------------------------------------------------------*/
IN_OUT L3audio_decode_MPEG1(unsigned char *bs, unsigned char *pcm)
{
   int sync;
   IN_OUT in_out;
   int side_bytes;
   int nbytes;
   
   iframe++;

   bitget_init(bs);		/* initialize bit getter */
/* test sync */
   in_out.in_bytes = 0;		/* assume fail */
   in_out.out_bytes = 0;
   sync = bitget(12);

   if (sync != 0xFFF)
      return in_out;		/* sync fail */
/*-----------*/

/*-- unpack side info --*/
   side_bytes = unpack_side_MPEG1();
   padframebytes = framebytes + pad;
   in_out.in_bytes = padframebytes;

/*-- load main data and update buf pointer --*/
/*------------------------------------------- 
   if start point < 0, must just cycle decoder 
   if jumping into middle of stream, 
w---------------------------------------------*/
   buf_ptr0 = buf_ptr1 - side_info.main_data_begin;	/* decode start point */
   if (buf_ptr1 > BUF_TRIGGER)
   {				/* shift buffer */
      memmove(buf, buf + buf_ptr0, side_info.main_data_begin);
      buf_ptr0 = 0;
      buf_ptr1 = side_info.main_data_begin;
   }
   nbytes = padframebytes - side_bytes - crcbytes;

   // RAK: This is no bueno. :-(
	if (nbytes < 0 || nbytes > NBUF)
	{
	    in_out.in_bytes = 0;
		 return in_out;
   }

   memmove(buf + buf_ptr1, bs + side_bytes + crcbytes, nbytes);
   buf_ptr1 += nbytes;
/*-----------------------*/

   if (buf_ptr0 >= 0)
   {
// dump_frame(buf+buf_ptr0, 64);
      main_pos_bit = buf_ptr0 << 3;
      unpack_main(pcm, 0);
      unpack_main(pcm + half_outbytes, 1);
      in_out.out_bytes = outbytes;
   }
   else
   {
      memset(pcm, zero_level_pcm, outbytes);	/* fill out skipped frames */
      in_out.out_bytes = outbytes;
/* iframe--;  in_out.out_bytes = 0;  // test test */
   }

   return in_out;
}
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
IN_OUT L3audio_decode_MPEG2(unsigned char *bs, unsigned char *pcm)
{
   int sync;
   IN_OUT in_out;
   int side_bytes;
   int nbytes;
   static int igr = 0;

   iframe++;


   bitget_init(bs);		/* initialize bit getter */
/* test sync */
   in_out.in_bytes = 0;		/* assume fail */
   in_out.out_bytes = 0;
   sync = bitget(12);

// if( sync != 0xFFF ) return in_out;       /* sync fail */

   mpeg25_flag = 0;
   if (sync != 0xFFF)
   {
      mpeg25_flag = 1;		/* mpeg 2.5 sync */
      if (sync != 0xFFE)
	 return in_out;		/* sync fail */
   }
/*-----------*/


/*-- unpack side info --*/
   side_bytes = unpack_side_MPEG2(igr);
   padframebytes = framebytes + pad;
   in_out.in_bytes = padframebytes;

   buf_ptr0 = buf_ptr1 - side_info.main_data_begin;	/* decode start point */
   if (buf_ptr1 > BUF_TRIGGER)
   {				/* shift buffer */
      memmove(buf, buf + buf_ptr0, side_info.main_data_begin);
      buf_ptr0 = 0;
      buf_ptr1 = side_info.main_data_begin;
   }
   nbytes = padframebytes - side_bytes - crcbytes;
   // RAK: This is no bueno. :-(
	if (nbytes < 0 || nbytes > NBUF)
	{
	    in_out.in_bytes = 0;
		 return in_out;
   }
   memmove(buf + buf_ptr1, bs + side_bytes + crcbytes, nbytes);
   buf_ptr1 += nbytes;
/*-----------------------*/

   if (buf_ptr0 >= 0)
   {
      main_pos_bit = buf_ptr0 << 3;
      unpack_main(pcm, igr);
      in_out.out_bytes = outbytes;
   }
   else
   {
      memset(pcm, zero_level_pcm, outbytes);	/* fill out skipped frames */
      in_out.out_bytes = outbytes;
// iframe--;  in_out.out_bytes = 0; return in_out;// test test */
   }



   igr = igr ^ 1;
   return in_out;
}
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
static int const sr_table[8] =
{22050, 24000, 16000, 1,
 44100, 48000, 32000, 1};

static const struct
{
   int l[23];
   int s[14];
}
sfBandIndexTable[3][3] =
{
/* mpeg-2 */
   {
      {
	 {
	    0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 116, 140, 168, 200, 238, 284, 336, 396, 464, 522, 576
	 }
	 ,
	 {
	    0, 4, 8, 12, 18, 24, 32, 42, 56, 74, 100, 132, 174, 192
	 }
      }
      ,
      {
	 {
	    0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 114, 136, 162, 194, 232, 278, 332, 394, 464, 540, 576
	 }
	 ,
	 {
	    0, 4, 8, 12, 18, 26, 36, 48, 62, 80, 104, 136, 180, 192
	 }
      }
      ,
      {
	 {
	    0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 116, 140, 168, 200, 238, 284, 336, 396, 464, 522, 576
	 }
	 ,
	 {
	    0, 4, 8, 12, 18, 26, 36, 48, 62, 80, 104, 134, 174, 192
	 }
      }
      ,
   }
   ,
/* mpeg-1 */
   {
      {
	 {
	    0, 4, 8, 12, 16, 20, 24, 30, 36, 44, 52, 62, 74, 90, 110, 134, 162, 196, 238, 288, 342, 418, 576
	 }
	 ,
	 {
	    0, 4, 8, 12, 16, 22, 30, 40, 52, 66, 84, 106, 136, 192
	 }
      }
      ,
      {
	 {
	    0, 4, 8, 12, 16, 20, 24, 30, 36, 42, 50, 60, 72, 88, 106, 128, 156, 190, 230, 276, 330, 384, 576
	 }
	 ,
	 {
	    0, 4, 8, 12, 16, 22, 28, 38, 50, 64, 80, 100, 126, 192
	 }
      }
      ,
      {
	 {
	    0, 4, 8, 12, 16, 20, 24, 30, 36, 44, 54, 66, 82, 102, 126, 156, 194, 240, 296, 364, 448, 550, 576
	 }
	 ,
	 {
	    0, 4, 8, 12, 16, 22, 30, 42, 58, 78, 104, 138, 180, 192
	 }
      }
   }
   ,

/* mpeg-2.5, 11 & 12 KHz seem ok, 8 ok */
   {
      {
	 {
	    0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 116, 140, 168, 200, 238, 284, 336, 396, 464, 522, 576
	 }
	 ,
	 {
	    0, 4, 8, 12, 18, 26, 36, 48, 62, 80, 104, 134, 174, 192
	 }
      }
      ,
      {
	 {
	    0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 116, 140, 168, 200, 238, 284, 336, 396, 464, 522, 576
	 }
	 ,
	 {
	    0, 4, 8, 12, 18, 26, 36, 48, 62, 80, 104, 134, 174, 192
	 }
      }
      ,
// this 8khz table, and only 8khz, from mpeg123)
      {
	 {
	    0, 12, 24, 36, 48, 60, 72, 88, 108, 132, 160, 192, 232, 280, 336, 400, 476, 566, 568, 570, 572, 574, 576
	 }
	 ,
	 {
	    0, 8, 16, 24, 36, 52, 72, 96, 124, 160, 162, 164, 166, 192
	 }
      }
      ,
   }
   ,
};


void sbt_mono_L3(float *sample, signed short *pcm, int ch);
void sbt_dual_L3(float *sample, signed short *pcm, int ch);
void sbt16_mono_L3(float *sample, signed short *pcm, int ch);
void sbt16_dual_L3(float *sample, signed short *pcm, int ch);
void sbt8_mono_L3(float *sample, signed short *pcm, int ch);
void sbt8_dual_L3(float *sample, signed short *pcm, int ch);

void sbtB_mono_L3(float *sample, unsigned char *pcm, int ch);
void sbtB_dual_L3(float *sample, unsigned char *pcm, int ch);
void sbtB16_mono_L3(float *sample, unsigned char *pcm, int ch);
void sbtB16_dual_L3(float *sample, unsigned char *pcm, int ch);
void sbtB8_mono_L3(float *sample, unsigned char *pcm, int ch);
void sbtB8_dual_L3(float *sample, unsigned char *pcm, int ch);



static SBT_FUNCTION sbt_table[2][3][2] =
{
{{ (SBT_FUNCTION) sbt_mono_L3,
   (SBT_FUNCTION) sbt_dual_L3 } ,
 { (SBT_FUNCTION) sbt16_mono_L3,
   (SBT_FUNCTION) sbt16_dual_L3 } ,
 { (SBT_FUNCTION) sbt8_mono_L3,
   (SBT_FUNCTION) sbt8_dual_L3 }} ,
/*-- 8 bit output -*/
{{ (SBT_FUNCTION) sbtB_mono_L3,
   (SBT_FUNCTION) sbtB_dual_L3 },
 { (SBT_FUNCTION) sbtB16_mono_L3,
   (SBT_FUNCTION) sbtB16_dual_L3 },
 { (SBT_FUNCTION) sbtB8_mono_L3,
   (SBT_FUNCTION) sbtB8_dual_L3 }}
};


void Xform_mono(void *pcm, int igr);
void Xform_dual(void *pcm, int igr);
void Xform_dual_mono(void *pcm, int igr);
void Xform_dual_right(void *pcm, int igr);

static XFORM_FUNCTION xform_table[5] =
{
   Xform_mono,
   Xform_dual,
   Xform_dual_mono,
   Xform_mono,			/* left */
   Xform_dual_right,
};
int L3table_init();
void msis_init();
void sbt_init();
typedef int iARRAY22[22];
iARRAY22 *quant_init_band_addr();
iARRAY22 *msis_init_band_addr();

/*---------------------------------------------------------*/
/* mpeg_head defined in mhead.h  frame bytes is without pad */
int L3audio_decode_init(MPEG_HEAD * h, int framebytes_arg,
		   int reduction_code, int transform_code, int convert_code,
			int freq_limit)
{
   int i, j, k;
   // static int first_pass = 1;
   int samprate;
   int limit;
   int bit_code;
   int out_chans;

   buf_ptr0 = 0;
   buf_ptr1 = 0;

/* check if code handles */
   if (h->option != 1)
      return 0;			/* layer III only */

   if (h->id)
      ncbl_mixed = 8;		/* mpeg-1 */
   else
      ncbl_mixed = 6;		/* mpeg-2 */

   framebytes = framebytes_arg;

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


   samprate = sr_table[4 * h->id + h->sr_index];
   if ((h->sync & 1) == 0)
      samprate = samprate / 2;	// mpeg 2.5 
/*----- compute nsb_limit --------*/
   nsb_limit = (freq_limit * 64L + samprate / 2) / samprate;
/*- caller limit -*/
   limit = (32 >> reduction_code);
   if (limit > 8)
      limit--;
   if (nsb_limit > limit)
      nsb_limit = limit;
   limit = 18 * nsb_limit;

   k = h->id;
   if ((h->sync & 1) == 0)
      k = 2;			// mpeg 2.5 

   if (k == 1)
   {
      band_limit12 = 3 * sfBandIndexTable[k][h->sr_index].s[13];
      band_limit = band_limit21 = sfBandIndexTable[k][h->sr_index].l[22];
   }
   else
   {
      band_limit12 = 3 * sfBandIndexTable[k][h->sr_index].s[12];
      band_limit = band_limit21 = sfBandIndexTable[k][h->sr_index].l[21];
   }
   band_limit += 8;		/* allow for antialias */
   if (band_limit > limit)
      band_limit = limit;

   if (band_limit21 > band_limit)
      band_limit21 = band_limit;
   if (band_limit12 > band_limit)
      band_limit12 = band_limit;


   band_limit_nsb = (band_limit + 17) / 18;	/* limit nsb's rounded up */
/*----------------------------------------------*/
   gain_adjust = 0;		/* adjust gain e.g. cvt to mono sum channel */
   if ((h->mode != 3) && (convert_code == 1))
      gain_adjust = -4;

   outvalues = 1152 >> reduction_code;
   if (h->id == 0)
      outvalues /= 2;

   out_chans = 2;
   if (h->mode == 3)
      out_chans = 1;
   if (convert_code)
      out_chans = 1;

   sbt_L3 = sbt_table[bit_code][reduction_code][out_chans - 1];
   k = 1 + convert_code;
   if (h->mode == 3)
      k = 0;
   Xform = xform_table[k];


   outvalues *= out_chans;

   if (bit_code)
      outbytes = outvalues;
   else
      outbytes = sizeof(short) * outvalues;

   if (bit_code)
      zero_level_pcm = 128;	/* 8 bit output */
   else
      zero_level_pcm = 0;


   decinfo.channels = out_chans;
   decinfo.outvalues = outvalues;
   decinfo.samprate = samprate >> reduction_code;
   if (bit_code)
      decinfo.bits = 8;
   else
      decinfo.bits = sizeof(short) * 8;

   decinfo.framebytes = framebytes;
   decinfo.type = 0;

   half_outbytes = outbytes / 2;
/*------------------------------------------*/

/*- init band tables --*/


   k = h->id;
   if ((h->sync & 1) == 0)
      k = 2;			// mpeg 2.5 

   for (i = 0; i < 22; i++)
      sfBandIndex[0][i] = sfBandIndexTable[k][h->sr_index].l[i + 1];
   for (i = 0; i < 13; i++)
      sfBandIndex[1][i] = 3 * sfBandIndexTable[k][h->sr_index].s[i + 1];
   for (i = 0; i < 22; i++)
      nBand[0][i] =
	 sfBandIndexTable[k][h->sr_index].l[i + 1]
	 - sfBandIndexTable[k][h->sr_index].l[i];
   for (i = 0; i < 13; i++)
      nBand[1][i] =
	 sfBandIndexTable[k][h->sr_index].s[i + 1]
	 - sfBandIndexTable[k][h->sr_index].s[i];


/* init tables */
   L3table_init();
/* init ms and is stereo modes */
   msis_init();

/*----- init sbt ---*/
   sbt_init();



/*--- clear buffers --*/
   for (i = 0; i < 576; i++)
      yout[i] = 0.0f;
   for (j = 0; j < 2; j++)
   {
      for (k = 0; k < 2; k++)
      {
	 for (i = 0; i < 576; i++)
	 {
	    sample[j][k][i].x = 0.0f;
	    sample[j][k][i].s = 0;
	 }
      }
   }

   if (h->id == 1)
      decode_function = L3audio_decode_MPEG1;
   else
      decode_function = L3audio_decode_MPEG2;

   return 1;
}
/*---------------------------------------------------------*/
/*==========================================================*/
