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
	
	$Id: dec8.c,v 1.2 1999/07/13 18:42:16 robert Exp $
____________________________________________________________________________*/

/****  dec8.c  ***************************************************


ANSI C
MPEG audio decoder Layer II only mpeg1 and mpeg2
output sample type and sample rate conversion
  decode mpeg to 8000Ks mono
  output 16 bit linear, 8 bit linear, or u-law


mod 6/29/95  bugfix in u-law table

mod 11/15/95 for Layer I

mod 1/7/97 minor mods for warnings  

******************************************************************/
/*****************************************************************

       MPEG audio software decoder portable ANSI c.
       Decodes all Layer II to 8000Ks mono pcm.
       Output selectable: 16 bit linear, 8 bit linear, u-law.

-------------------------------------
int audio_decode8_init(MPEG_HEAD *h, int framebytes_arg,
         int reduction_code, int transform_code, int convert_code,
         int freq_limit)

initilize decoder:
       return 0 = fail, not 0 = success

MPEG_HEAD *h    input, mpeg header info (returned by call to head_info)
framebytes      input, mpeg frame size (returned by call to head_info)
reduction_code  input, ignored
transform_code  input, ignored
convert_code    input, set convert_code = 4*bit_code + chan_code
                     bit_code:   1 = 16 bit linear pcm
                                 2 =  8 bit (unsigned) linear pcm
                                 3 = u-law (8 bits unsigned)
                     chan_code:  0 = convert two chan to mono
                                 1 = convert two chan to mono
                                 2 = convert two chan to left chan
                                 3 = convert two chan to right chan
freq_limit      input, ignored


---------------------------------
void audio_decode8_info( DEC_INFO *info)

information return:
          Call after audio_decode8_init.  See mhead.h for
          information returned in DEC_INFO structure.


---------------------------------
IN_OUT audio_decode8(unsigned char *bs, void *pcmbuf)

decode one mpeg audio frame:
bs        input, mpeg bitstream, must start with
          sync word.  Caution: may read up to 3 bytes
          beyond end of frame.
pcmbuf    output, pcm samples.

IN_OUT structure returns:
          Number bytes conceptually removed from mpeg bitstream.
          Returns 0 if sync loss.
          Number bytes of pcm output.  This may vary from frame
          to frame.

*****************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
#include "mhead.h"		/* mpeg header structure */


static unsigned char look_u[8192];


static short pcm[2304];
static int ncnt = 8 * 288;
static int ncnt1 = 8 * 287;
static int nlast = 287;
static int ndeci = 11;
static int kdeci = 8 * 288;
static short xsave;

/*------------------------------------------*/
static int output_code;
static int convert(unsigned char *pcm);
static int convert_8bit(unsigned char *pcm);
static int convert_u(unsigned char *pcm);
typedef int (*CVT_FUNCTION) (unsigned char *pcm);
static CVT_FUNCTION convert_routine;
static CVT_FUNCTION cvt_table[3] =
{
   convert,
   convert_8bit,
   convert_u,
};

/*====================================================================*/
IN_OUT audio_decode8(unsigned char *bs, signed short *pcmbuf)
{
   IN_OUT x;

   x = audio_decode(bs, pcm);
   if (x.in_bytes <= 0)
      return x;
   x.out_bytes = convert_routine((void *) pcmbuf);

   return x;
}
/*--------------8Ks 16 bit pcm --------------------------------*/
static int convert(unsigned char y0[])
{
   int i, k;
   long alpha;
   short *y;

   y = (short *) y0;
   k = 0;
   if (kdeci < ncnt)
   {
      alpha = kdeci & 7;
      y[k++] = (short) (xsave + ((alpha * (pcm[0] - xsave)) >> 3));
      kdeci += ndeci;
   }
   kdeci -= ncnt;
   for (; kdeci < ncnt1; kdeci += ndeci)
   {
      i = kdeci >> 3;
      alpha = kdeci & 7;
      y[k++] = (short) (pcm[i] + ((alpha * (pcm[i + 1] - pcm[i])) >> 3));
   }
   xsave = pcm[nlast];

/* printf("\n k out = %4d", k);   */

   return sizeof(short) * k;
}
/*----------------8Ks 8 bit unsigned pcm ---------------------------*/
static int convert_8bit(unsigned char y[])
{
   int i, k;
   long alpha;

   k = 0;
   if (kdeci < ncnt)
   {
      alpha = kdeci & 7;
      y[k++] = (unsigned char) (((xsave + ((alpha * (pcm[0] - xsave)) >> 3)) >> 8) + 128);
      kdeci += ndeci;
   }
   kdeci -= ncnt;
   for (; kdeci < ncnt1; kdeci += ndeci)
   {
      i = kdeci >> 3;
      alpha = kdeci & 7;
      y[k++] = (unsigned char) (((pcm[i] + ((alpha * (pcm[i + 1] - pcm[i])) >> 3)) >> 8) + 128);
   }
   xsave = pcm[nlast];

/* printf("\n k out = %4d", k);   */

   return k;
}
/*--------------8Ks u-law --------------------------------*/
static int convert_u(unsigned char y[])
{
   int i, k;
   long alpha;
   unsigned char *look;

   look = look_u + 4096;

   k = 0;
   if (kdeci < ncnt)
   {
      alpha = kdeci & 7;
      y[k++] = look[(xsave + ((alpha * (pcm[0] - xsave)) >> 3)) >> 3];
      kdeci += ndeci;
   }
   kdeci -= ncnt;
   for (; kdeci < ncnt1; kdeci += ndeci)
   {
      i = kdeci >> 3;
      alpha = kdeci & 7;
      y[k++] = look[(pcm[i] + ((alpha * (pcm[i + 1] - pcm[i])) >> 3)) >> 3];
   }
   xsave = pcm[nlast];

/* printf("\n k out = %4d", k);   */

   return k;
}
/*--------------------------------------------------------------------*/
static int ucomp3(int x)	/* re analog devices CCITT G.711 */
{
   int s, p, y, t, u, u0, sign;

   sign = 0;
   if (x < 0)
   {
      x = -x;
      sign = 0x0080;
   }
   if (x > 8031)
      x = 8031;
   x += 33;
   t = x;
   for (s = 0; s < 15; s++)
   {
      if (t & 0x4000)
	 break;
      t <<= 1;
   }
   y = x << s;
   p = (y >> 10) & 0x0f;	/* position */
   s = 9 - s;			/* segment */
   u0 = (((s << 4) | p) & 0x7f) | sign;
   u = u0 ^ 0xff;

   return u;
}
/*------------------------------------------------------------------*/
static void table_init()
{
   int i;

   for (i = -4096; i < 4096; i++)
      look_u[4096 + i] = (unsigned char) (ucomp3(2 * i));

}
/*-------------------------------------------------------------------*/
int audio_decode8_init(MPEG_HEAD * h, int framebytes_arg,
		   int reduction_code, int transform_code, int convert_code,
		       int freq_limit)
{
   int istat;
   int outvals;
   static int sr_table[2][4] =
   {{22, 24, 16, 0}, {44, 48, 32, 0}};
   static int first_pass = 1;

   if (first_pass)
   {
      table_init();
      first_pass = 0;
   }

   if ((h->sync & 1) == 0)
      return 0;			// fail mpeg 2.5

   output_code = convert_code >> 2;
   if (output_code < 1)
      output_code = 1;		/* 1= 16bit 2 = 8bit 3 = u */
   if (output_code > 3)
      output_code = 3;		/* 1= 16bit 2 = 8bit 3 = u */

   convert_code = convert_code & 3;
   if (convert_code <= 0)
      convert_code = 1;		/* always cvt to mono */

   reduction_code = 1;
   if (h->id)
      reduction_code = 2;

/* select convert routine */
   convert_routine = cvt_table[output_code - 1];

/* init decimation/convert routine */
/*-- MPEG-2 layer III --*/
   if ((h->option == 1) && h->id == 0)
      outvals = 576 >> reduction_code;
   else if (h->option == 3)
      outvals = 384 >> reduction_code;
/*-- layer I --*/
   else
      outvals = 1152 >> reduction_code;
   ncnt = 8 * outvals;
   ncnt1 = 8 * (outvals - 1);
   nlast = outvals - 1;
   ndeci = sr_table[h->id][h->sr_index] >> reduction_code;
   kdeci = 8 * outvals;
/* printf("\n outvals %d", outvals);  */

   freq_limit = 3200;
   istat = audio_decode_init(h, framebytes_arg,
			     reduction_code, transform_code, convert_code,
			     freq_limit);


   return istat;
}
/*-----------------------------------------------------------------*/
void audio_decode8_info(DEC_INFO * info)
{

   audio_decode_info(info);
   info->samprate = 8000;
   if (output_code != 1)
      info->bits = 8;
   if (output_code == 3)
      info->type = 10;
}
