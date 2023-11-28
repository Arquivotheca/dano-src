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
 * $Id: l3bitstream-pvt.h,v 1.1 1996/02/14 04:04:23 rowlands Exp $
 *
 * $Log: l3bitstream-pvt.h,v $
 * Revision 1.1  1996/02/14 04:04:23  rowlands
 * Initial revision
 *
 * Received from Mike Coleman
 **********************************************************************/

#ifndef L3BITSTREAM_PVT_H
#define L3BITSTREAM_PVT_H

static int encodeSideInfo( III_side_info_t  *si );

static void encodeMainData( int              l3_enc[2][2][576],
			    III_side_info_t  *si,
			    III_scalefac_t   *scalefac );

static void write_ancillary_data( char *theData, int lengthInBits );

static void drain_into_ancillary_data( int lengthInBits );

static void Huffmancodebits( BitHolder **pph, int *ix, gr_info *gi );

#endif
