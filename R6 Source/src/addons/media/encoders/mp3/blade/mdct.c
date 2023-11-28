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

/**********************************************************************
 * ISO MPEG Audio Subgroup Software Simulation Group (1996)
 * ISO 13818-3 MPEG-2 Audio Encoder - Lower Sampling Frequency Extension
 *
 * $Id: mdct.c,v 1.1 1996/02/14 04:04:23 rowlands Exp $
 *
 * $Log: mdct.c,v $
 * Revision 1.1  1996/02/14 04:04:23  rowlands
 * Initial revision
 *
 * Received from Mike Coleman
 **********************************************************************/

#include "common.h"
#include "l3side.h"
#include "mdct.h"

double ca[8], cs[8];

/*
  This is table B.9: coefficients for aliasing reduction
*/
static double c[8] = { -0.6,-0.535,-0.33,-0.185,-0.095,-0.041,-0.0142, -0.0037 };

int	fInit_mdct_sub;

void mdct_sub( L3SBS (*sb_sample), double (*mdct_freq)[2][576], int stereo, III_side_info_t *l3_side, int mode_gr )
{
  gr_info *cod_info;
  double mdct_in[36];
  int ch,gr,band,k,j;
  double bu,bd;
  int	block_type;
  double (*mdct_enc)[2][32][18] = (double (*)[2][32][18]) mdct_freq;
    
  if ( fInit_mdct_sub == 0 )
  {
		/* prepare the aliasing reduction butterflies */
		for ( k = 0; k < 8; k++ )
		{
			double sq;
			sq = sqrt( 1.0 + c[k] * c[k] );
			ca[k] = c[k] / sq;
			cs[k] = 1.0 / sq;
		}
		fInit_mdct_sub++;
	}
    
  for ( gr = 0; gr < mode_gr; gr++ )
		for ( ch = 0; ch < stereo; ch++ )
		{
			cod_info = (gr_info *) &(l3_side->gr[gr].ch[ch]) ;
			block_type = cod_info->block_type;
	    
			/* Compensate for inversion in the analysis filter */
			for ( band = 0; band < 32; band++ )
				for ( k = 0; k < 18; k++ )
					if ( (band & 1) && (k & 1) )
						(*sb_sample)[ch][gr+1][k][band] *= -1.0;
	    
			/*
				Perform imdct of 18 previous subband samples
				+ 18 current subband samples
			*/

			for ( band = 0; band < 32; band++ )
			{
				for ( k = 0; k < 18; k++ )
				{
					mdct_in[k]    = (*sb_sample)[ch][ gr ][k][band];
					mdct_in[k+18] = (*sb_sample)[ch][gr+1][k][band];
				}
				if ( cod_info->mixed_block_flag && (band < 2) )
					block_type = 0;
		
				mdct( mdct_in, &mdct_enc[gr][ch][band][0], block_type );
	    }
	    
	    /*
	      Perform aliasing reduction butterfly
	      on long blocks
	    */

			if ( block_type != 2 )
				for ( band = 0; band < 31; band++ )
					for ( k = 0; k < 8; k++ )
					{
						bu = mdct_enc[gr][ch][band][17-k] * cs[k] + mdct_enc[gr][ch][band+1][k] * ca[k];
						bd = mdct_enc[gr][ch][band+1][k] * cs[k] - mdct_enc[gr][ch][band][17-k] * ca[k];
						mdct_enc[gr][ch][band][17-k] = bu;
						mdct_enc[gr][ch][band+1][k]  = bd;
			    }
	    
		}
    
    /*
      Save latest granule's subband samples to be used in
      the next mdct call
    */
    
	for ( ch = 0; ch < stereo; ch++ )
		for ( j = 0; j < 18; j++ )
			for ( band = 0; band < 32; band++ )
				(*sb_sample)[ch][0][j][band] = (*sb_sample)[ch][mode_gr][j][band];
}

int	fInit_mdct;

void mdct( double *in, double *out, int block_type )
{
/*-------------------------------------------------------------------*/
/*                                                                   */
/*   Function: Calculation of the MDCT                               */
/*   In the case of long blocks ( block_type 0,1,3 ) there are       */
/*   36 coefficents in the time domain and 18 in the frequency       */
/*   domain.                                                         */
/*   In the case of short blocks (block_type 2 ) there are 3         */
/*   transformations with short length. This leads to 12 coefficents */
/*   in the time and 6 in the frequency domain. In this case the     */
/*   results are stored side by side in the vector out[].            */
/*                                                                   */
/*   New layer3                                                      */
/*                                                                   */
/*-------------------------------------------------------------------*/

  int l,k,i,m,N;
  double sum;
  static double win[4][36];
  static double cos_s[6][12], cos_l[18][36];


  if ( fInit_mdct == 0 )
  {
    /* type 0 */
    for ( i = 0; i < 36; i++ )
      win[0][i] = sin( PI/36 * (i + 0.5) );
    /* type 1*/
    for ( i = 0; i < 18; i++ ) 
      win[1][i] = sin( PI/36 * (i + 0.5) );
    for ( i = 18; i < 24; i++ )
      win[1][i] = 1.0;
    for ( i = 24; i < 30; i++ )
      win[1][i] = sin( PI/12 * ( i + 0.5 - 18) );
    for ( i = 30; i < 36; i++ )
      win[1][i] = 0.0;
    /* type 3*/
    for ( i = 0; i < 6; i++ )
      win[3][i] = 0.0;
    for ( i = 6; i < 12; i++ ) 
      win[3][i] = sin( PI/12 * (i + 0.5 - 6) );
    for ( i = 12; i < 18; i++ )
      win[3][i] = 1.0;
    for ( i = 18; i < 36; i++ )
      win[3][i] = sin( PI/36 * (i + 0.5) );
    /* type 2*/
    for ( i = 0; i < 12; i++ )
    win[2][i] = sin( PI/12 * (i + 0.5) );
    for ( i = 12; i < 36; i++ )
      win[2][i] = 0.0;

    N = 12;
    for ( m = 0; m < N / 2; m++ )
      for ( k = 0; k < N; k++ )
        cos_s[m][k] = cos( (PI /(2 * N)) * (2 * k + 1 + N / 2) *
                     (2 * m + 1) ) / (N / 4);

    N = 36;
    for ( m = 0; m < N / 2; m++ )
      for ( k = 0; k < N; k++ )
        cos_l[m][k] = cos( (PI / (2 * N)) * (2 * k + 1 + N / 2) *
                     (2 * m + 1) ) / (N / 4);

    fInit_mdct++;
  }

  if ( block_type == 2 )
  {
    N = 12;
    for ( l = 0; l < 3; l++ )
    {
      for ( m = 0; m < N / 2; m++ )
      {
        for ( sum = 0.0, k = 0; k < N; k++ )
          sum += win[block_type][k] * in[k + 6 * l + 6] * cos_s[m][k];
        out[ 3 * m + l] = sum;
      }
    }
  }
  else
  {
    N = 36;
    for ( m = 0; m < N / 2; m++ )
    {
      for ( sum = 0.0, k = 0; k < N; k++ )
        sum += win[block_type][k] * in[k] * cos_l[m][k];
      out[m] = sum;
    }
  }
}

