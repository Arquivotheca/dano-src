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
	
	$Id: mhead.h,v 1.2 1998/10/31 01:47:55 jdw Exp $
____________________________________________________________________________*/

/* portable copy of eco\mhead.h */
/* mpeg audio header   */
typedef struct
{
   int sync;			/* 1 if valid sync */
   int id;
   int option;
   int prot;
   int br_index;
   int sr_index;
   int pad;
   int private_bit;
   int mode;
   int mode_ext;
   int cr;
   int original;
   int emphasis;
}
MPEG_HEAD;

/* portable mpeg audio decoder, decoder functions */
typedef struct
{
   int in_bytes;
   int out_bytes;
}
IN_OUT;


typedef struct
{
   int channels;
   int outvalues;
   long samprate;
   int bits;
   int framebytes;
   int type;
}
DEC_INFO;


#ifdef __cplusplus
extern "C"
{
#endif

   _EXPORT int head_info(unsigned char *buf, unsigned int n, MPEG_HEAD * h);
   _EXPORT int head_info2(unsigned char *buf,
	   unsigned int n, MPEG_HEAD * h, int *br);
	_EXPORT int head_info3(unsigned char *buf, unsigned int n, MPEG_HEAD *h, int*br, unsigned int *searchForward);
/* head_info returns framebytes > 0 for success */
/* audio_decode_init returns 1 for success, 0 for fail */
/* audio_decode returns in_bytes = 0 on sync loss */

   _EXPORT int audio_decode_init(MPEG_HEAD * h, int framebytes_arg,
		   int reduction_code, int transform_code, int convert_code,
			 int freq_limit);
   _EXPORT void audio_decode_info(DEC_INFO * info);
   _EXPORT IN_OUT audio_decode(unsigned char *bs, short *pcm);

   _EXPORT int audio_decode8_init(MPEG_HEAD * h, int framebytes_arg,
		   int reduction_code, int transform_code, int convert_code,
			  int freq_limit);
   _EXPORT void audio_decode8_info(DEC_INFO * info);
   _EXPORT IN_OUT audio_decode8(unsigned char *bs, short *pcmbuf);

/*-- integer decode --*/
   _EXPORT int i_audio_decode_init(MPEG_HEAD * h, int framebytes_arg,
		   int reduction_code, int transform_code, int convert_code,
			   int freq_limit);
   _EXPORT void i_audio_decode_info(DEC_INFO * info);
   _EXPORT IN_OUT i_audio_decode(unsigned char *bs, short *pcm);



#ifdef __cplusplus
}
#endif
