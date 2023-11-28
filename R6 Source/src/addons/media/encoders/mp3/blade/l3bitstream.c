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
#include "system.h"

#include "l3bitstream.h" /* the public interface */
#include "l3psy.h"
#include "mdct.h"
#include "loop.h"
#include "formatbitstream2.h"
#include "huffman.h"
#include <assert.h>
#include "l3bitstream-pvt.h"

static int stereo = 1;
static frame_params *fr_ps  = NULL;


int PartHoldersInitialized = 0;

BitHolder *headerPH;
BitHolder *frameSIPH;
BitHolder *channelSIPH[ MAX_CHANNELS ];
BitHolder *spectrumSIPH[ MAX_GRANULES ][ MAX_CHANNELS ];
BitHolder *scaleFactorsPH[ MAX_GRANULES ][ MAX_CHANNELS ];
BitHolder *codedDataPH[ MAX_GRANULES ][ MAX_CHANNELS ];
BitHolder *userSpectrumPH[ MAX_GRANULES ][ MAX_CHANNELS ];
BitHolder *userFrameDataPH;


BF_FrameData			sFrameData;
BF_FrameResults		sFrameResults;



/*
  III_format_bitstream()
  
  This is called after a frame of audio has been quantized and coded.
  It will write the encoded audio to the bitstream. Note that
  from a layer3 encoder's perspective the bit stream is primarily
  a series of main_data() blocks, with header and side information
  inserted at the proper locations to maintain framing. (See Figure A.7
  in the IS).
  */

void III_format_bitstream(	int              bitsPerFrame,
														frame_params     *in_fr_ps,
														int              l3_enc[2][2][576],
														III_side_info_t  *l3_side,
														III_scalefac_t   *scalefac,
														double           (*xr)[2][576],
														char             *ancillary,
														int              ancillary_bits )
{
  int gr, ch, i, mode_gr;
  fr_ps = in_fr_ps;
  stereo = fr_ps->stereo;
  mode_gr = 2;
  
  if ( !PartHoldersInitialized )
  {
		headerPH = initBitHolder( &sFrameData.header, 16*2 );
		frameSIPH = initBitHolder( &sFrameData.frameSI, 4*2 );

		for ( ch = 0; ch < MAX_CHANNELS; ch++ )
	    channelSIPH[ch] = initBitHolder( &sFrameData.channelSI[ch], 8*2 );

		for ( gr = 0; gr < MAX_GRANULES; gr++ )	
	    for ( ch = 0; ch < MAX_CHANNELS; ch++ )
	    {
				spectrumSIPH[gr][ch]   = initBitHolder( &sFrameData.spectrumSI[gr][ch], 32*2 );
				scaleFactorsPH[gr][ch] = initBitHolder( &sFrameData.scaleFactors[gr][ch], 64*2 );
				codedDataPH[gr][ch]    = initBitHolder( &sFrameData.codedData[gr][ch], 576*2 );
				userSpectrumPH[gr][ch] = initBitHolder( &sFrameData.userSpectrum[gr][ch], 4*2 );
	    }
		userFrameDataPH = initBitHolder( &sFrameData.userFrameData, 8*2 );
		
		
		PartHoldersInitialized = 1;
  }

#if 1
  for ( gr = 0; gr < mode_gr; gr++ )
		for ( ch =  0; ch < stereo; ch++ )
		{
	    int *pi = &l3_enc[gr][ch][0];
	    double *pr = &xr[gr][ch][0];
	    for ( i = 0; i < 576; i++, pr++, pi++ )
		  {
				if ( (*pr < 0) && (*pi > 0) )
				  *pi *= -1;
	    }
		}
#endif

  encodeSideInfo( l3_side );
  encodeMainData( l3_enc, l3_side, scalefac );
  write_ancillary_data( ancillary, ancillary_bits );

  if ( l3_side->resvDrain )
		drain_into_ancillary_data( l3_side->resvDrain );

  sFrameData.frameLength = bitsPerFrame;
  sFrameData.nGranules   = mode_gr;
  sFrameData.nChannels   = stereo;

  writeFrame( &sFrameData, &sFrameResults );

  /* we set this here -- it will be tested in the next loops iteration */
  l3_side->main_data_begin = sFrameResults.nextBackPtr;
}

void III_FlushBitstream()
{
	int	ch, gr;

  if ( PartHoldersInitialized )
  {
				
		exitBitHolder( &sFrameData.header );
		exitBitHolder( &sFrameData.frameSI );

		for ( ch = 0; ch < MAX_CHANNELS; ch++ )
	    exitBitHolder( &sFrameData.channelSI[ch] );


		for ( gr = 0; gr < MAX_GRANULES; gr++ )	
	    for ( ch = 0; ch < MAX_CHANNELS; ch++ )
	    {
				exitBitHolder( &sFrameData.spectrumSI[gr][ch] );
				exitBitHolder( &sFrameData.scaleFactors[gr][ch] );
				exitBitHolder( &sFrameData.codedData[gr][ch] );
				exitBitHolder( &sFrameData.userSpectrum[gr][ch] );
	    }
		exitBitHolder( &sFrameData.userFrameData );
				
		PartHoldersInitialized = 0;
  }
	

 /*	BF_FlushBitstream( frameData, frameResults ); */
}

static unsigned slen1_tab[16] = { 0, 0, 0, 0, 3, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4 };
static unsigned slen2_tab[16] = { 0, 1, 2, 3, 0, 1, 2, 3, 1, 2, 3, 1, 2, 3, 2, 3 };

static void encodeMainData( int l3_enc[2][2][576],
														III_side_info_t  *si,
														III_scalefac_t   *scalefac )
{
  int gr, ch, sfb, window, mode_gr;

	mode_gr = 2;

  for ( gr = 0; gr < mode_gr; gr++ )
		for ( ch = 0; ch < stereo; ch++ )
	    scaleFactorsPH[gr][ch]->nrEntries = 0;

  for ( gr = 0; gr < mode_gr; gr++ )
		for ( ch = 0; ch < stereo; ch++ )
	    codedDataPH[gr][ch]->nrEntries = 0;

	for ( gr = 0; gr < 2; gr++ )
	{
	  for ( ch = 0; ch < stereo; ch++ )
	  {
			BitHolder **pph = &scaleFactorsPH[gr][ch];		
			gr_info *gi = &(si->gr[gr].ch[ch].tt);
			unsigned slen1 = slen1_tab[ gi->scalefac_compress ];
			unsigned slen2 = slen2_tab[ gi->scalefac_compress ];
			int *ix = &l3_enc[gr][ch][0];

			if ( (gi->window_switching_flag == 1) && (gi->block_type == 2) )
			{
				if ( gi->mixed_block_flag )
				{
					for ( sfb = 0; sfb < 8; sfb++ )
						addBits( *pph,  scalefac->l[gr][ch][sfb], slen1 );

					for ( sfb = 3; sfb < 6; sfb++ )
						for ( window = 0; window < 3; window++ )
							addBits( *pph,  scalefac->s[gr][ch][sfb][window], slen1 );

					for ( sfb = 6; sfb < 12; sfb++ )
						for ( window = 0; window < 3; window++ )
							addBits( *pph,  scalefac->s[gr][ch][sfb][window], slen2 );

				}
				else
				{
					for ( sfb = 0; sfb < 6; sfb++ )
						for ( window = 0; window < 3; window++ )
							addBits( *pph,  scalefac->s[gr][ch][sfb][window], slen1 );

					for ( sfb = 6; sfb < 12; sfb++ )
						for ( window = 0; window < 3; window++ )
							addBits( *pph,  scalefac->s[gr][ch][sfb][window], slen2 );
				}
			}
			else
			{
				if ( (gr == 0) || (si->scfsi[ch][0] == 0) )
					for ( sfb = 0; sfb < 6; sfb++ )
						addBits( *pph,  scalefac->l[gr][ch][sfb], slen1 );

				if ( (gr == 0) || (si->scfsi[ch][1] == 0) )
					for ( sfb = 6; sfb < 11; sfb++ )
						addBits( *pph,  scalefac->l[gr][ch][sfb], slen1 );

				if ( (gr == 0) || (si->scfsi[ch][2] == 0) )
					for ( sfb = 11; sfb < 16; sfb++ )
						addBits( *pph,  scalefac->l[gr][ch][sfb], slen2 );

				if ( (gr == 0) || (si->scfsi[ch][3] == 0) )
					for ( sfb = 16; sfb < 21; sfb++ )
						addBits( *pph,  scalefac->l[gr][ch][sfb], slen2 );
			}
			Huffmancodebits( &codedDataPH[gr][ch], ix, gi );
	  } /* for ch */
	} /* for gr */
} /* main_data */

/*____ encodeSideInfo() _____________________________________________________*/

static int encodeSideInfo( III_side_info_t  *si )
{
  int gr, ch, scfsi_band, region, window, bits_sent, mode_gr;
  layer *info = fr_ps->header;

  mode_gr =  2;

  headerPH->nrEntries = 0;
  addBits( headerPH, 0xfff,                   12 );
  addBits( headerPH, 1,												 1 );
  addBits( headerPH, 4 - info->lay,            2 );
  addBits( headerPH, !info->error_protection,  1 );
  addBits( headerPH, info->bitrate_index,      4 );
  addBits( headerPH, info->sampling_frequency, 2 );
  addBits( headerPH, info->padding,            1 );
  addBits( headerPH, info->extension,          1 );
  addBits( headerPH, info->mode,               2 );
  addBits( headerPH, info->mode_ext,           2 );
  addBits( headerPH, info->copyright,          1 );
  addBits( headerPH, info->original,           1 );
  addBits( headerPH, info->emphasis,           2 );
    
  bits_sent = 32;

  if ( info->error_protection )
  {
		addBits( headerPH, 0, 16 );					/* Just a dummy add. Real CRC calculated & inserted in writeSideInfo() */
		bits_sent += 16;
  }
	

  frameSIPH->nrEntries = 0;

  for (ch = 0; ch < stereo; ch++ )
		channelSIPH[ch]->nrEntries = 0;

  for ( gr = 0; gr < 2; gr++ )
		for ( ch = 0; ch < stereo; ch++ )
	    spectrumSIPH[gr][ch]->nrEntries = 0;

	addBits( frameSIPH, si->main_data_begin, 9 );

	if ( stereo == 2 )
	  addBits( frameSIPH, si->private_bits, 3 );
	else
	  addBits( frameSIPH, si->private_bits, 5 );

	for ( ch = 0; ch < stereo; ch++ )
	  for ( scfsi_band = 0; scfsi_band < 4; scfsi_band++ )
	  {
			BitHolder **pph = &channelSIPH[ch];
			addBits( *pph, si->scfsi[ch][scfsi_band], 1 );
	  }

	for ( gr = 0; gr < 2; gr++ )
	  for ( ch = 0; ch < stereo; ch++ )
	  {
			BitHolder **pph = &spectrumSIPH[gr][ch];
			gr_info *gi = &(si->gr[gr].ch[ch].tt);
			addBits( *pph, gi->part2_3_length,        12 );
			addBits( *pph, gi->big_values,            9 );
			addBits( *pph, gi->global_gain,           8 );
			addBits( *pph, gi->scalefac_compress,     4 );
			addBits( *pph, gi->window_switching_flag, 1 );

			if ( gi->window_switching_flag )
			{   
				addBits( *pph, gi->block_type,       2 );
				addBits( *pph, gi->mixed_block_flag, 1 );

				for ( region = 0; region < 2; region++ )
					addBits( *pph, gi->table_select[region],  5 );
				for ( window = 0; window < 3; window++ )
					addBits( *pph, gi->subblock_gain[window], 3 );
			}
			else
			{
/*				assert( gi->block_type == 0 ); */
				for ( region = 0; region < 3; region++ )
					addBits( *pph, gi->table_select[region], 5 );

				addBits( *pph, gi->region0_count, 4 );
				addBits( *pph, gi->region1_count, 3 );
			}

			addBits( *pph, gi->preflag,            1 );
			addBits( *pph, gi->scalefac_scale,     1 );
			addBits( *pph, gi->count1table_select, 1 );
		}


	if ( stereo == 2 )
		bits_sent += 256;
	else
		bits_sent += 136;
  return bits_sent;
}



/*____ write_ancillary_data() _______________________________________________*/

static void write_ancillary_data( char *theData, int lengthInBits )
{
    /*
     */
  int bytesToSend = lengthInBits / 8;
  int remainingBits = lengthInBits % 8;
  unsigned wrd;
  int i;

  userFrameDataPH->nrEntries = 0;

  for ( i = 0; i < bytesToSend; i++ )
  {
		wrd = theData[i];
		addBits( userFrameDataPH, wrd, 8 );
  }
  if ( remainingBits )
  {
		/* right-justify remaining bits */
		wrd = theData[bytesToSend] >> (8 - remainingBits);
		addBits( userFrameDataPH, wrd, remainingBits );
  }
    
}

/*
  Some combinations of bitrate, Fs, and stereo make it impossible to stuff
  out a frame using just main_data, due to the limited number of bits to
  indicate main_data_length. In these situations, we put stuffing bits into
  the ancillary data...
*/
static void drain_into_ancillary_data( int lengthInBits )
{
  /*
   */
  int wordsToSend   = lengthInBits / 32;
  int remainingBits = lengthInBits % 32;
  int i;

  /*
    userFrameDataPH->part->nrEntries set by call to write_ancillary_data()
  */

  for ( i = 0; i < wordsToSend; i++ )
		addBits( userFrameDataPH, 0, 32 );
  if ( remainingBits )
		addBits( userFrameDataPH, 0, remainingBits );    
}

/*
  Note the discussion of huffmancodebits() on pages 28
  and 29 of the IS, as well as the definitions of the side
  information on pages 26 and 27.
  */
static void Huffmancodebits( BitHolder **pph, int *ix, gr_info *gi )
{
  int L3_huffman_coder_count1( BitHolder **pph, struct huffcodetab *h, int v, int w, int x, int y );
  int bigv_bitcount( int ix[576], gr_info *cod_info );

  int region1Start;
  int region2Start;
  int i, bigvalues, count1End;
  int v, w, x, y, bits, cbits, xbits, stuffingBits;
  unsigned int code, ext;
  struct huffcodetab *h;
  int bvbits, c1bits, tablezeros, r0, r1, r2, rt, *pr;
  int bitsWritten = 0;
  tablezeros = 0;
  r0 = r1 = r2 = 0;
  
  /* 1: Write the bigvalues */
  bigvalues = gi->big_values * 2;
  if ( bigvalues )
  {
		if ( !(gi->mixed_block_flag) && gi->window_switching_flag && (gi->block_type == 2) )
		{ /* Three short blocks */
	    /*
	      Within each scalefactor band, data is given for successive
	      time windows, beginning with window 0 and ending with window 2.
	      Within each window, the quantized values are then arranged in
	      order of increasing frequency...
	      */
	    int sfb, window, line, start, end;

	    I192_3 *ix_s;
	    int *scalefac = &sfBandIndex[fr_ps->header->sampling_frequency].s[0];
	    
	    ix_s = (I192_3 *) ix;
	    region1Start = 12;
	    region2Start = 576;

	    for ( sfb = 0; sfb < 13; sfb++ )
	    {
				unsigned tableindex = 100;
				start = scalefac[ sfb ];
				end   = scalefac[ sfb+1 ];

				if ( start < region1Start )
			    tableindex = gi->table_select[ 0 ];
				else
			    tableindex = gi->table_select[ 1 ];
/*		assert( tableindex < 32 ); */

				for ( window = 0; window < 3; window++ )
			    for ( line = start; line < end; line += 2 )
			    {
						x = (*ix_s)[line][window];
						y = (*ix_s)[line + 1][window];
/*					assert( idx < 576 );
						assert( idx >= 0 ); */
						bits = HuffmanCode( tableindex, x, y, &code, &ext, &cbits, &xbits );
						addBits( *pph,  code, cbits );
						addBits( *pph,  ext, xbits );
						bitsWritten += bits;
			    }
		
	    }
		}
		else if ( gi->mixed_block_flag && gi->block_type == 2 )
	  {  /* Mixed blocks long, short */
			int sfb, window, line, start, end;
			unsigned tableindex;
			I192_3 *ix_s;
			int *scalefac = &sfBandIndex[fr_ps->header->sampling_frequency].s[0];
		
			ix_s = (I192_3 *) ix;

			/* Write the long block region */
			tableindex = gi->table_select[0];
			if ( tableindex )
		    for ( i = 0; i < 36; i += 2 )
		    {
					x = ix[i];
					y = ix[i + 1];
					bits = HuffmanCode( tableindex, x, y, &code, &ext, &cbits, &xbits );
					addBits( *pph,  code, cbits );
					addBits( *pph,  ext, xbits );
					bitsWritten += bits;
		    }
			/* Write the short block region */
			tableindex = gi->table_select[ 1 ];
/*		assert( tableindex < 32 ); */

			for ( sfb = 3; sfb < 13; sfb++ )
			{
		    start = scalefac[ sfb ];
		    end   = scalefac[ sfb+1 ];           
		    
		    for ( window = 0; window < 3; window++ )
					for ( line = start; line < end; line += 2 )
					{
						x = (*ix_s)[line][window];
						y = (*ix_s)[line + 1][window];
						bits = HuffmanCode( tableindex, x, y, &code, &ext, &cbits, &xbits );
						addBits( *pph,  code, cbits );
						addBits( *pph,  ext, xbits );
						bitsWritten += bits;
					}
			}

    }
    else
    { /* Long blocks */
			int *scalefac = &sfBandIndex[fr_ps->header->sampling_frequency].l[0];
			unsigned scalefac_index = 100;
		
			if ( gi->mixed_block_flag )
			{
		    region1Start = 36;
		    region2Start = 576;
			}
			else
			{
		    scalefac_index = gi->region0_count + 1;
		    region1Start = scalefac[ scalefac_index ];
		    scalefac_index += gi->region1_count + 1;
		    region2Start = scalefac[ scalefac_index ];
			}
			for ( i = 0; i < bigvalues; i += 2 )
			{
		    unsigned tableindex = 100;
		    /* get table pointer */
		    if ( i < region1Start )
		    {
					tableindex = gi->table_select[0];
					pr = &r0;
		    }
		    else if ( i < region2Start )
				{
			    tableindex = gi->table_select[1];
			    pr = &r1;
				}
				else
				{
			    tableindex = gi->table_select[2];
			    pr = &r2;
				}
/*		    assert( tableindex < 32 ); */
		    h = &ht[ tableindex ];
		    /* get huffman code */
		    x = ix[i];
		    y = ix[i + 1];
		    if ( tableindex )
		    {
					bits = HuffmanCode( tableindex, x, y, &code, &ext, &cbits, &xbits );
					addBits( *pph,  code, cbits );
					addBits( *pph,  ext, xbits );
					bitsWritten += rt = bits;
					*pr += rt;
		    }
		    else
		    {
					tablezeros += 1;
					*pr = 0;
		    }
			}
    }
  }
  bvbits = bitsWritten; 

  /* 2: Write count1 area */
  h = &ht[gi->count1table_select + 32];
  count1End = bigvalues + (gi->count1 * 4);
  for ( i = bigvalues; i < count1End; i += 4 )
  {
		v = ix[i];
		w = ix[i+1];
		x = ix[i+2];
		y = ix[i+3];
		bitsWritten += L3_huffman_coder_count1( pph, h, v, w, x, y );
  }
  c1bits = bitsWritten - bvbits;
  if ( (stuffingBits = gi->part2_3_length - gi->part2_length - bitsWritten) )
  {
		int stuffingWords = stuffingBits / 32;
		int remainingBits = stuffingBits % 32;
/*	assert( stuffingBits > 0 ); */

	/*
	  Due to the nature of the Huffman code
	  tables, we will pad with ones
	*/
		while ( stuffingWords-- )
	    addBits( *pph, ~0, 32 );
		if ( remainingBits )
	    addBits( *pph, ~0, remainingBits );
		bitsWritten += stuffingBits;
  }
}

int INLINE abs_and_sign( int *x )
{
  if ( *x > 0 )
		return 0;
  *x *= -1;
    return 1;
}

int L3_huffman_coder_count1( BitHolder **pph, struct huffcodetab *h, int v, int w, int x, int y )
{
  HUFFBITS huffbits;
  unsigned int signv, signw, signx, signy, p;
  int len;
  int totalBits = 0;
  
  signv = abs_and_sign( &v );
  signw = abs_and_sign( &w );
  signx = abs_and_sign( &x );
  signy = abs_and_sign( &y );
  
  p = v + (w << 1) + (x << 2) + (y << 3);
  huffbits = h->table[p];
  len = h->hlen[ p ];
  addBits( *pph,  huffbits, len );
  totalBits += len;
  if ( v )
  {
		addBits( *pph,  signv, 1 );
		totalBits += 1;
  }
  if ( w )
  {
		addBits( *pph,  signw, 1 );
		totalBits += 1;
  }

  if ( x )
  {
		addBits( *pph,  signx, 1 );
		totalBits += 1;
  }
  if ( y )
  {
		addBits( *pph,  signy, 1 );
		totalBits += 1;
  }
  return totalBits;
}

/*
  Implements the pseudocode of page 98 of the IS
  */
int HuffmanCode( int table_select, int x, int y, unsigned int *code, unsigned int *ext, int *cbits, int *xbits )
{
  unsigned signx, signy, linbitsx, linbitsy, linbits, xlen, ylen, idx;
  struct huffcodetab *h;

  *cbits = 0;
  *xbits = 0;
  *code  = 0;
  *ext   = 0;
  
  if ( table_select == 0 )
		return 0;
    
  signx = abs_and_sign( &x );
  signy = abs_and_sign( &y );
  h = &(ht[table_select]);
  xlen = h->xlen;
  ylen = h->ylen;
  linbits = h->linbits;
  linbitsx = linbitsy = 0;

  if ( table_select > 15 )
  { /* ESC-table is used */
		if ( x > 14 )
		{
	    linbitsx = x - 15;
/*	    assert( linbitsx <= h->linmax ); */
	    x = 15;
		}
		if ( y > 14 )
		{
	    linbitsy = y - 15;
/*	    assert( linbitsy <= h->linmax ); */
	    y = 15;
		}
		idx = (x * ylen) + y;
		*code = h->table[idx];
		*cbits = h->hlen[ idx ];
		if ( x > 14 )
		{
	    *ext |= linbitsx;
	    *xbits += linbits;
		}
		if ( x != 0 )
		{
	    *ext <<= 1;
	    *ext |= signx;
	    *xbits += 1;
		}
		if ( y > 14 )
		{
	    *ext <<= linbits;
	    *ext |= linbitsy;
	    *xbits += linbits;
		}
		if ( y != 0 )
		{
	    *ext <<= 1;
	    *ext |= signy;
	    *xbits += 1;
		}
  }
  else
  { /* No ESC-words */
		idx = (x * ylen) + y;
		*code = h->table[idx];
		*cbits += h->hlen[ idx ];
		if ( x != 0 )
		{
	    *code <<= 1;
	    *code |= signx;
	    *cbits += 1;
		}
		if ( y != 0 )
		{
	    *code <<= 1;
	    *code |= signy;
	    *cbits += 1;
		}
  }
/*assert( *cbits <= 32 );
  assert( *xbits <= 32 ); */
  return *cbits + *xbits;
}










