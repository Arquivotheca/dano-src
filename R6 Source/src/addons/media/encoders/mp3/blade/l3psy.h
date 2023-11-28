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
 * $Id: l3psy.h,v 1.1 1996/02/14 04:04:23 rowlands Exp $
 *
 * $Log: l3psy.h,v $
 * Revision 1.1  1996/02/14 04:04:23  rowlands
 * Initial revision
 *
 * Received from Mike Coleman
 **********************************************************************/

#ifndef L3PSY_DOT_H_
#define L3PSY_DOT_H_
/* #define CBANDS 63 */
#define CBANDS_s 42
#define BLKSIZE_s 256
#define HBLKSIZE_s 129
#define TCBMAX_l 63
#define TCBMAX_s 42
#define SBMAX_l 21
#define SBMAX_s 12

/* #define switch_pe        1800 */
#define NORM_TYPE       0
#define START_TYPE      1
#define SHORT_TYPE      2
#define STOP_TYPE       3

/* l3psy.c */
#include "l3side.h"
void L3psycho_anal( short int *buffer, short int savebuf[1344], int chn, int lay, FLOAT snr32[32],
					double sfreq, double ratio_d[21], double ratio_ds[12][3],
					double *pe, gr_info *cod_info );
#endif
