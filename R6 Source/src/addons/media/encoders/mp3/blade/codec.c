/*
			(c) Copyright 1998, 1999 - Tord Jansson
			=======================================

		This file is part of the BladeEnc MP3 Encoder, based on
		ISO's reference code for MPEG Layer 3 compression, and might
		contain smaller or larger sections that are directly taken
		from ISO's reference code.

		All changes to the ISO reference code herein are either
		copyrighted by Tord Jansson (tord.jansson@swipnet.se)
		or sublicensed to Tord Jansson by a third party.

	BladeEnc is free software; you can redistribute this file
	and/or modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

*/

#include <stdlib.h>
#include "common.h"
#include "encoder.h"
#include "l3psy.h"
#include "mdct.h"
#include "loop.h"
#include "l3bitstream.h"
#include "formatbitstream2.h"
#include  "codec.h"





/************************************************************************/

    typedef double SBS[2][3][SCALE_BLOCK][SBLIMIT];
    SBS         *sb_sample;
    L3SBS       *l3_sb_sample;
    typedef double JSBS[3][SCALE_BLOCK][SBLIMIT];
    JSBS        *j_sample;
    typedef double IN[2][HAN_SIZE];
    IN          *win_que;
    typedef unsigned int SUB[2][3][SCALE_BLOCK][SBLIMIT];
    SUB         *subband;
    
    frame_params fr_ps;
    layer info;
    short **win_buf;
    static short buffer[2][1152];
    FLOAT snr32[32];
    short sam[2][1344]; 
    int whole_SpF, extra_slot;
    double avg_slots_per_frame, frac_SpF, slot_lag;
    int stereo, error_protection;

 		unsigned long samplesPerFrame;


extern int	fInit_L3psycho_anal;
extern int	init_L3;
 
extern	int	fInit_windowFilterSubband;

extern	int	fInit_mdct_sub;
extern	int	fInit_mdct;

extern	int fInit_fft;

extern	int	fInit_iteration_loop;
extern	int fInit_huffman_read_flag;

extern	void	fixStatic_reservoir();

CodecInitOut	sOut;

char				* pEncodedOutput;
int						outputBit;

/*____ codecInit() ____________________________________________________________*/

CodecInitOut * codecInit( CodecInitIn * psIn )
{
      layer * pInfo;
      int     brt;
			int			retVal;
			int			j;
	
			fInit_L3psycho_anal = 0;
			init_L3 = 0;
			fInit_windowFilterSubband = 0;


			fInit_mdct_sub = 0;
			fInit_mdct = 0;

			fInit_fft = 0;

			fInit_iteration_loop = 0;
			fInit_huffman_read_flag = 0; 

			
/*_______ Static-fix _______________*/


	fixStatic_loop();
	fixStatic_reservoir();



/*___________________________________*/


	initFormatBitstream();


/*     Most large variables are declared dynamically to ensure
       compatibility with smaller machines  */
    
    sb_sample = (SBS *) mem_alloc(sizeof(SBS), "sb_sample");
    l3_sb_sample = (L3SBS *) mem_alloc(sizeof(SBS), "l3_sb_sample");
    j_sample = (JSBS *) mem_alloc(sizeof(JSBS), "j_sample");
    win_que = (IN *) mem_alloc(sizeof(IN), "Win_que");
    subband = (SUB *) mem_alloc(sizeof(SUB),"subband");
    win_buf = (short **) mem_alloc(sizeof(short *)*2, "win_buf");
 
/*     clear buffers */
    memset((char *) buffer, 0, sizeof(buffer));
    memset((char *) snr32, 0, sizeof(snr32));
    memset((char *) sam, 0, sizeof(sam));
 
    fr_ps.header = &info;
    fr_ps.tab_num = -1;             /* no table loaded */
    fr_ps.alloc = NULL;
    info.version = 1;   /* Default: MPEG-1 */


    /* Read psIn */

    pInfo = fr_ps.header;

    switch (psIn->frequency) 
    {
      case 48000 : pInfo->sampling_frequency = 1;
        break;
      case 44100 : pInfo->sampling_frequency = 0;
        break;
      case 32000 : pInfo->sampling_frequency = 2;
        break;
      default:    
        return  FALSE;
    }


    pInfo->lay = 3;


    switch( psIn->mode)
    {
      case 0:
        pInfo->mode = MPG_MD_STEREO; pInfo->mode_ext = 0;
        break;
      case 2:
        pInfo->mode = MPG_MD_DUAL_CHANNEL; pInfo->mode_ext = 0;
        break;
      case 3:
        pInfo->mode = MPG_MD_MONO; pInfo->mode_ext = 0;
        break;
      default:
        return FALSE;
    }

    
    brt = psIn->bitrate;
    j = 0;
    while ( j < 15 )
    {
      if ( bitratex[1][j] == brt )
        break;
      j++;
    }

    pInfo->bitrate_index = j;

 
 
    pInfo->emphasis = psIn->emphasis;
    pInfo->extension = psIn->fPrivate;            
    pInfo->error_protection = psIn->fCRC;
    pInfo->copyright = psIn->fCopyright;           
    pInfo->original = psIn->fOriginal;
    


    
    hdr_to_frps(&fr_ps);
    stereo = fr_ps.stereo;
    error_protection = info.error_protection;
    
	  samplesPerFrame = 1152;	  

    avg_slots_per_frame = ((double)samplesPerFrame /
                           s_freq[1][info.sampling_frequency]) *
			   ((double)bitratex[1][info.bitrate_index] / 8.0);
    whole_SpF = (int) avg_slots_per_frame;
    frac_SpF  = avg_slots_per_frame - (double)whole_SpF;
    slot_lag  = -frac_SpF;
    
    if (frac_SpF == 0)
      info.padding = 0;


		genNoisePowTab();

/*________________________*/


	retVal = 2304;

	if(stereo != 2)
		retVal /= 2;			/* Half amount of samples if just mono... */

	sOut.nSamples = retVal;
	sOut.bufferSize = 2048;

  return  &sOut;			/* How many samples we want in each chunk... */
}


void rebuffer_audio( short buffer[2][1152], short * insamp, unsigned long samples_read, int stereo );

/*____ codecEncodeChunk() _____________________________________________________*/

unsigned int codecEncodeChunk( int nSamples, short * pSamples, char * pDest )
{

	static double xr[2][2][576];
	static double xr_dec[2][2][576];
	static double pe[2][2];
	static int		l3_enc[2][2][576];
	static				III_psy_ratio ratio;
	static				III_side_info_t l3_side;
	static				III_scalefac_t  scalefac;
	int						gr, ch;
	int						mean_bits, sideinfo_len;
	int						bitsPerFrame;
	int						j;

	pEncodedOutput = pDest;
	outputBit = 8;
	pEncodedOutput[0] = 0;

  rebuffer_audio( buffer, pSamples, nSamples, stereo );

  win_buf[0] = &buffer[0][0];
	win_buf[1] = &buffer[1][0];
	if (frac_SpF != 0) 
	{
    if (slot_lag > (frac_SpF-1.0) ) 
		{
			slot_lag -= frac_SpF;
			extra_slot = 0;
			info.padding = 0;
    }
    else 
		{
			extra_slot = 1;
			info.padding = 1;
			slot_lag += (1-frac_SpF);
	  }
	}
	
	      
  bitsPerFrame = 8 * whole_SpF + (info.padding * 8);

/*		determine the mean bitrate for main data */

  sideinfo_len = 32;

	if ( stereo == 1 )
		sideinfo_len += 136;
	else
		sideinfo_len += 256;

	if ( info.error_protection )
		sideinfo_len += 16;
	mean_bits = (bitsPerFrame - sideinfo_len) / 2;

/*		psychoacoustic model */

  for ( gr = 0; gr < 2; gr++ )
		for ( ch = 0; ch < stereo; ch++ )
		{
		    L3psycho_anal( &buffer[ch][gr*576], &sam[ch][0], ch, 3,
				   snr32, s_freq[1][info.sampling_frequency] * 1000.0,
				   &ratio.l[gr][ch][0], &ratio.s[gr][ch][0],
				   &pe[gr][ch], &l3_side.gr[gr].ch[ch].tt );
		}

/*		polyphase filtering */

  for( gr = 0; gr < 2; gr++ )
		for ( ch = 0; ch < stereo; ch++ )
		  for ( j = 0; j < 18; j++ )
		    windowFilterSubband( &win_buf[ch], ch, &(*l3_sb_sample)[ch][gr+1][j][0] );


/*		apply mdct to the polyphase outputs */

  mdct_sub( l3_sb_sample, xr, stereo, &l3_side, 2 );


/*    bit and noise allocation */

  iteration_loop( pe, xr, &ratio, &l3_side, l3_enc, mean_bits,
		stereo, xr_dec, &scalefac, &fr_ps, 0, bitsPerFrame );

/*		write the frame to the bitstream */

  III_format_bitstream( bitsPerFrame, &fr_ps, l3_enc, &l3_side, &scalefac,
		xr, NULL, 0 );
	    

  return  pEncodedOutput - pDest;
}

/*____ codecExit() ____________________________________________________________*/

unsigned int codecExit( char * pDest )
{
	pEncodedOutput = pDest;
	outputBit = 8;
	pEncodedOutput[0] = 0;


  free( sb_sample );
  free( l3_sb_sample );
  free( j_sample );
	free( win_que );
	free( subband );
	free( win_buf );

	exitFormatBitstream();
 	III_FlushBitstream();

  return  pEncodedOutput - pDest;
}

