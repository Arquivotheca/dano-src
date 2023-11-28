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

#ifndef	HUFFMAN_DOT_H
#define	HUFFMAN_DOT_H

/**********************************************************************
 * ISO MPEG Audio Subgroup Software Simulation Group (1996)
 * ISO 13818-3 MPEG-2 Audio Encoder - Lower Sampling Frequency Extension
 *
 * $Id: huffman.h,v 1.1 1996/02/14 04:04:23 rowlands Exp $
 *
 * $Log: huffman.h,v $
 * Revision 1.1  1996/02/14 04:04:23  rowlands
 * Initial revision
 *
 * Received from Mike Coleman
 **********************************************************************/
/**********************************************************************
 *   date   programmers                comment                        *
 *  27.2.92 F.O.Witte (ITT Intermetall)				      *
 *  8/24/93 M. Iwadare          Changed for 1 pass decoding.          *
 *  7/14/94 J. Koller		useless 'typedef' before huffcodetab  *
 *				removed				      *
 *********************************************************************/	
 
#define HUFFBITS unsigned long int
#define HTN	34
#define MXOFF	250
 
struct huffcodetab {
  char tablename[3];	/*string, containing table_description	*/
  unsigned int xlen; 	/*max. x-index+			      	*/ 
  unsigned int ylen;	/*max. y-index+				*/
  unsigned int linbits; /*number of linbits			*/
  unsigned int linmax;	/*max number to be stored in linbits	*/
  int ref;		/*a positive value indicates a reference*/
  HUFFBITS *table;	/*pointer to array[xlen][ylen]		*/
  unsigned char *hlen;	/*pointer to array[xlen][ylen]		*/
  unsigned char(*val)[2];/*decoder tree				*/ 
  unsigned int treelen;	/*length of decoder tree		*/
};

extern struct huffcodetab ht[HTN];/* global memory block		*/
				/* array of all huffcodtable headers	*/
				/* 0..31 Huffman code table 0..31	*/
				/* 32,33 count1-tables			*/
#ifdef PROTO_ARGS

extern int read_huffcodetab( void ); 
extern int read_decoder_table(FILE *);
 
extern void huffman_coder(unsigned int, unsigned int,
			  struct huffcodetab *, Bit_stream_struc *);
			  
extern int huffman_decoder(struct huffcodetab *,
			   /* unsigned */ int *, /* unsigned */ int*, int*, int*);

#else

extern int read_huffcodetab(); 
extern int read_decoder_table(); 
extern void huffman_coder();
extern int huffman_decoder();

#endif



#endif	/* HUFFMAN_DOT_H */
