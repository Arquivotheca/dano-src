/*
 * SCODER.c
 * Font Fusion Copyright (c) 1989-1999 all rights reserved by Bitstream Inc.
 * http://www.bitstream.com/
 * http://www.typesolutions.com/
 * Author: Sampo Kaasila
 *
 * This software is the property of Bitstream Inc. and it is furnished
 * under a license and may be used and copied only in accordance with the
 * terms of such license and with the inclusion of the above copyright notice.
 * This software or any other copies thereof may not be provided or otherwise
 * made available to any other person or entity except as allowed under license.
 * No title to and no ownership of the software or intellectual property
 * contained herein is hereby transferred. This information in this software
 * is subject to change without notice
 */

#include "syshead.h"

#include "dtypes.h"
#include "config.h"
#include "tsimem.h"
#include "t2kstrm.h"
#include "scoder.h"

#ifdef ENABLE_ORION
/*
 * Internal function used for sequencing the look-up table.
 */
void SCODER_SequenceLookUp( SCODER *t )
{
	unsigned char symbol[ No_of_chars ], thisSymbol;
	register int i, j, k, numSymbols, thisNumSlots;
	register unsigned char *numBitsUsed = t->numBitsUsed;
	unsigned long numBits;


	/* We have to align the symbols with the smallest bit-lengths first  */
	/* A block has to start at a multiple of it's number of slots used ! */
	numSymbols = 0;
	for ( numBits = 1; numBits <= t->maxBits; numBits++ ) {
		for ( i = 0; i < No_of_chars; i++ ) {
			if ( numBitsUsed[i] == ( unsigned char)numBits ) {
				symbol[numSymbols++] = (unsigned char)i;
			}
		}
		/* Now all entries numBits == numBitsUsed are done, go to the next bitlength */
	}
	assert( numSymbols <= No_of_chars );
	
	assert( (unsigned long)(1L << t->maxBits) == t->numEntries );
	for ( k = i = 0; i < numSymbols; i++ ) {
		thisSymbol = symbol[i];
		thisNumSlots = 1L << (t->maxBits - t->numBitsUsed[thisSymbol]);
		assert( k % thisNumSlots ==  0 );
		for ( j = 0; j < thisNumSlots; j++ ) {
			t->LookUpSymbol[k++] = thisSymbol;
		}
		assert( (unsigned long)k <= t->numEntries );
	}
}


/*
 * This standard constructor recreates the SCODER object from a stream.
 */
SCODER *New_SCODER_FromStream( tsiMemObject *mem, InputStream *in )
{
	int i;
	unsigned long maxBits;
	unsigned char value, bitCategory1, bitCategory2;
	SCODER *t = (SCODER *) tsi_AllocMem( mem, sizeof( SCODER ) );

	t->mem				= mem;
	t->numBitsUsed 		= (unsigned char *)tsi_AllocMem( mem, No_of_chars * sizeof( unsigned char ) );
	
	maxBits = ReadUnsignedByteMacro( in );
	for ( i = 0; i < No_of_chars; ) {
		value 			= ReadUnsignedByteMacro( in );
		bitCategory1	= (unsigned char)(value >> 4);
		bitCategory2	= (unsigned char)(value & 0x0f);
		t->numBitsUsed[i++] = (unsigned char)(bitCategory1 == 15 ? 0 : maxBits - bitCategory1);
		t->numBitsUsed[i++] = (unsigned char)(bitCategory2 == 15 ? 0 : maxBits - bitCategory2);
	}
	t->maxBits		= maxBits;	
	t->numEntries	= (unsigned long)(1L << maxBits);	
	t->LookUpSymbol	= (unsigned char *) tsi_AllocMem( mem,t->numEntries * sizeof( unsigned char ) );
	t->LookUpBits	= NULL;
	SCODER_SequenceLookUp( t );
	return t; /*****/
}




/*
 * Read a symbol from the input stream.
 * This table driven algorithm is key to our fast de-compression speed.
 * We never need to do bit-level reads and tests.
 */
unsigned char SCODER_ReadSymbol( register SCODER *t, register InputStream *in )
{
	register unsigned long 	tmp;
	register unsigned char 	symbol;
	register unsigned long	maxBits 	= t->maxBits;
	register unsigned long 	bitCountIn	= in->bitCountIn;
	register unsigned long 	bitBufferIn	= in->bitBufferIn;
	
	/* If not enough bits then read in more */
	while ( bitCountIn < maxBits ) {
		/* Note, this may read maxBits-1 bits too far... */
		tmp         = ReadUnsignedByteMacro( in );
		/* We always keep the bits slammed up againts the "left" edge" */
		tmp        <<= 24 - bitCountIn;
		bitBufferIn |= tmp;
		bitCountIn  += 8;
	}
	symbol			= t->LookUpSymbol[ bitBufferIn >> (32 - maxBits) ]; /* A simple look-up :-) */
	tmp				= t->numBitsUsed[ symbol ];							/* A simple look-up :-) */
	bitCountIn	   -= tmp; 			/* Discard the consumed bits */
	bitBufferIn	  <<= tmp;
	in->bitCountIn	= bitCountIn;	/* Store the state */
	in->bitBufferIn	= bitBufferIn;	/* Done! */
	return symbol; 					/*****/
}




/*
 * The destructor.
 */
void Delete_SCODER( SCODER *t )
{
	if ( t != NULL ) {
		tsi_DeAllocMem( t->mem, t->numBitsUsed );
		tsi_DeAllocMem( t->mem, t->LookUpSymbol );
		tsi_DeAllocMem( t->mem, t->LookUpBits );
		tsi_DeAllocMem( t->mem, t );
	}
}

#endif /*  ENABLE_ORION from top of the file */


/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: R:/src/FontFusion/Source/Core/rcs/scoder.c 1.5 2000/02/25 17:45:53 reggers release $
 *                                                                           *
 *     $Log: scoder.c $
 *     Revision 1.5  2000/02/25 17:45:53  reggers
 *     STRICT warning cleanup.
 *     Revision 1.4  1999/10/18 17:00:33  jfatal
 *     Changed all include file names to lower case.
 *     Revision 1.3  1999/09/30 15:11:39  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.2  1999/05/17 15:57:30  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/

