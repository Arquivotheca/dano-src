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
 * $Id: loop-pvt.h,v 1.1 1996/02/14 04:04:23 rowlands Exp $
 *
 * Private interface declarations for loop.c
 *
 * $Log: loop-pvt.h,v $
 * Revision 1.1  1996/02/14 04:04:23  rowlands
 * Initial revision
 *
 * Received from Mike Coleman
 **********************************************************************/

#ifndef LOOP_PVT_H
#define LOOP_PVT_H

/*
  Revision History:

  Date        Programmer                Comment
  ==========  ========================= ===============================
  1995/10/01  mc@fivebats.com           created

*/


void quantize_tj( double xr[576], int ix[576], gr_info *cod_info );


int outer_loop( double xr[2][2][576],     /*vector of the magnitudees of the spectral values */
                int max_bits,
                III_psy_xmin  *l3_xmin, /* the allowed distortion of the scalefactor */
                int l3_enc[2][2][576],    /* vector of quantized values ix(0..575) */
		frame_params *fr_ps,
                III_scalefac_t *scalefac, /* scalefactors */
                int gr,
                int ch,
		III_side_info_t *l3_side );

int part2_length( III_scalefac_t *scalefac,
		  frame_params *fr_ps,
		  int gr,
		  int ch,
		  III_side_info_t *si );

int quantanf_init( double xr[576] );

int inner_loop( double * xrs,
                int * ix,
                int max_bits,
                gr_info *cod_info );
void calc_xmin( double xr[2][2][576],
               III_psy_ratio *ratio,
               gr_info *cod_info,
               III_psy_xmin *l3_xmin,
               int gr,
               int ch );
double xr_max( double xr[576] );

void calc_scfsi( double  xr[576],
                 III_side_info_t *l3_side,
                 III_psy_xmin  *l3_xmin,
                 int ch,
                 int gr );

void gr_deco( gr_info *cod_info );



int count_bit( int ix[576], unsigned int start, unsigned int end, unsigned int table);
int bigv_bitcount( int ix[576], gr_info *cod_info );
int choose_table( int max);
void bigv_tab_select( int ix[576], gr_info *cod_info );
void subdivide( gr_info *cod_info );
int count1_bitcount( int ix[576], gr_info *cod_info );
void  calc_runlen( int ix[576],
                   gr_info *cod_info );
int scale_bitcount( III_scalefac_t *scalefac,
                    gr_info *cod_info,
                    int gr,

                    int ch );
void calc_noise( double xr[576],
                 int ix[576],
                 gr_info *cod_info,
                 double xfsf[4][CBLIMIT] );


int loop_break( III_scalefac_t *scalefac,
                gr_info *cod_info,
                int gr,
                int ch );
void preemphasis( double xr[576],
                  double xfsf[4][CBLIMIT],
                  III_psy_xmin  *l3_xmin,
                  int gr,
                  int ch,
		  III_side_info_t *l3_side );
int amp_scalefac_bands( double xr[576],
                        double xfsf[4][CBLIMIT],
                        III_psy_xmin  *l3_xmin,
			III_side_info_t *l3_side,
                        III_scalefac_t *scalefac,
                        int gr,
                        int ch,
			int iteration );
void quantize( double xr[576],
               int  ix[576],
               gr_info *cod_info );

int ix_max( int ix[576],
            unsigned int begin,
            unsigned int end );


int
new_choose_table( int ix[576],
		  unsigned int begin,
		  unsigned int end );



int	count_bit_tj( int abs_ix[576], unsigned int start, unsigned int end, unsigned int table );
int new_choose_table_tj( int abs_ix[576], unsigned int begin, unsigned int end );
void bigv_tab_select_tj( int abs_ix[576], gr_info *cod_info );
void calc_runlen_tj( int abs_ix[576], gr_info *cod_info );

#endif
