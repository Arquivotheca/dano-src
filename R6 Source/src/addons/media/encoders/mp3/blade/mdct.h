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
 * $Id: mdct.h,v 1.1 1996/02/14 04:04:23 rowlands Exp $
 *
 * $Log: mdct.h,v $
 * Revision 1.1  1996/02/14 04:04:23  rowlands
 * Initial revision
 *
 * Received from Mike Coleman
 **********************************************************************/

#ifndef MDCT_DOT_H
#define MDCT_DOT_H
void mdct(double *in, double *out, int block_type);
void inv_mdct(double *in, double *out, int block_type);

typedef double D32_18[SBLIMIT][18];
typedef double L3SBS[2][3][18][SBLIMIT]; /* [gr][ch] */

void mdct_sub(L3SBS (*sb_sample), double (*mdct_freq)[2][576], int stereo, III_side_info_t *l3_side, int mode_gr );
void mdct_sub_dec(double (*mdct_freq)[2][576], double inv_mdct_dec[3][2][18][32], int stereo, III_side_info_t *l3_side);
void delay(double (*xr)[2][576], int stereo);
#endif
