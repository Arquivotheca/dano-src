/*
			(c) Copyright 1998, 1999 - Tord Jansson
			=======================================

		This file is part of the BladeEnc MP3 Encoder, based on
		ISO's reference code for MPEG Layer 3 compression.

		This file doesn't contain any of the ISO reference code and
		is copyright Tord Jansson (tord.jansson@swipnet.se).

	BladeEnc is free software; you can redistribute this file
	and/or modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

*/

#include	<stdio.h>
#include	<stdlib.h>
#include	<malloc.h>
#include	"formatbitstream2.h"
#include	"common.h"


extern	char		* pEncodedOutput;
extern	int				outputBit;


/*____ Structure Definitions _________________________________________________*/

typedef	struct HeaderDef		Header;

struct HeaderDef
{
	int			size;
	int			frameSize;
	char		data[128];
	Header	* pNext;
};


/*____ Function Prototypes ___________________________________________________*/

static int writeMainDataBits( BF_FrameData * psFrame, BitHolder *psBH );
static void putbits(unsigned int val, int n);
static int generateHeader( BF_FrameData * psFrame );


/*____ Static Data ___________________________________________________________*/

static	int BitsRemaining = 0;

static	Header *	pHeaderChain;
static	Header *	pFreeHeaderChain;


/*____ initFormatBitstream() ________________________________________________*/

void	initFormatBitstream( void )
{
	BitsRemaining = 0;
	pHeaderChain = NULL;
	pFreeHeaderChain = NULL;
}

/*____ exitFormatBitstream() _________________________________________________*/

void 	exitFormatBitstream( void )
{
	Header * psHeader;
	Header * psFree;
	
	psHeader = pHeaderChain;
	while( psHeader != NULL )
	{
		psFree = psHeader;
		psHeader = psHeader->pNext;
		free( psFree );
	}
	
	psHeader = pFreeHeaderChain;
	while( psHeader != NULL )
	{
		psFree = psHeader;
		psHeader = psHeader->pNext;
		free( psFree );
	}
	
}


/*____ initBitHolder() ______________________________________________________*/

BitHolder * initBitHolder( BitHolder * wp, int elements )
{
	wp->element = (BitHolderElement *) malloc( sizeof( BitHolderElement ) * elements );
	wp->max_elements = elements;
	wp->nrEntries = 0;
	return	wp;
}

/*____ exitBitHolder() ______________________________________________________*/

void exitBitHolder( BitHolder * wp )
{
	if( wp->element != NULL )
		free( wp->element );
	wp->element = NULL;
}


/*____ addBits() ____________________________________________________________*/

void addBits( BitHolder * wp, unsigned int value, int length )
{
	if( length != 0 )
	{
		if( wp->nrEntries == wp->max_elements )
		{
			printf( "ERROR: BitHolder overflow!\n" );
			exit( -1 );
		}

		value = value & ((0xFFFFFFFF)>>(32-length)); 

		wp->element[wp->nrEntries].value = value;
		wp->element[wp->nrEntries].length = length;
		wp->nrEntries++;
	}
}


/*____ writeFrame() _________________________________________________________*/

void writeFrame( BF_FrameData *psFrame, BF_FrameResults *results )
{
	int	bits;
	int	ch, gr;

	int	sizeRemainHeaders, sizeRemainFrames;
	Header	* psHeader;

	/* Generate and save header, return size of SideInfo.*/
	
  results->SILength = generateHeader( psFrame );


	/* Put the bits and compute size of mainData */

	bits = 0;
  for ( gr = 0; gr < psFrame->nGranules; gr++ )
		for ( ch = 0; ch < psFrame->nChannels; ch++ )
		{
			bits += writeMainDataBits( psFrame, &psFrame->scaleFactors[gr][ch] );
			bits += writeMainDataBits( psFrame, &psFrame->codedData[gr][ch] );
			bits += writeMainDataBits( psFrame, &psFrame->userSpectrum[gr][ch] );
		}
  bits += writeMainDataBits( psFrame, &psFrame->userFrameData );

  results->mainDataLength = bits;


  /* calculate nextBackPointer */

	sizeRemainHeaders = 0;
	sizeRemainFrames = 0;

	for( psHeader = pHeaderChain ; psHeader != NULL ; psHeader = psHeader->pNext )
	{
		sizeRemainHeaders += psHeader->size;
		sizeRemainFrames += psHeader->frameSize;
	}

  results->nextBackPtr = (BitsRemaining / 8) + sizeRemainFrames - sizeRemainHeaders;	/* BitsRemaining must be dividable by 8 */

}


/*=============================================================================
									>>> Static Functions <<<
=============================================================================*/


/*____ writeBitHolder() _____________________________________________________*/

static void writeBitHolder( BitHolder * part )
{
  BitHolderElement * ep;
  int i;

  ep = part->element;
  for ( i = 0; i < part->nrEntries; i++, ep++ )
		putbits( ep->value, ep->length );
}

/*____ calcCRC() ____________________________________________________________*/

static int	calcCRC( char * pData, int size )
{
	int		i;
	int		crc = 0xffff;
	int		masking;
	int		carry;
	int		data;
			
	for( i = 2 ; i < size ; i++ )
	{
		if( i != 4 && i != 5 )
		{
			masking = 1 << 8;
			data = pData[i];
			while( masking >>= 1 )
			{
				carry = crc & 0x8000;
		    crc <<= 1;
		    if (!carry ^ !(data & masking))
					crc ^= 0x8005;
      }			
		}
	}
	
	crc &= 0xffff;
	return crc;
}

/*____ generateHeader() ____________________________________________________*/

static int generateHeader( BF_FrameData * psFrame )
{
  int ch, gr;
	int	crc;
	Header * psHeader;
	Header **wpLink;
	char	 * pOldEncodedOutput;

	/* Get a Free Header structure */

	if( pFreeHeaderChain == NULL )
		psHeader = (Header *) malloc( sizeof( Header ) );		
	else
	{
		psHeader = pFreeHeaderChain;
		pFreeHeaderChain = psHeader->pNext;
	}
	psHeader->pNext = NULL;
	for( wpLink = &pHeaderChain ; * wpLink != NULL ; wpLink = &((*wpLink)->pNext) );
	*wpLink = psHeader;

	/* Generate the Header */

	pOldEncodedOutput = pEncodedOutput;
	pEncodedOutput = psHeader->data;
	

  writeBitHolder( &psFrame->header );
  writeBitHolder( &psFrame->frameSI );

  for ( ch = 0; ch < psFrame->nChannels; ch++ )
		writeBitHolder( &psFrame->channelSI[ch] );

  for ( gr = 0; gr < psFrame->nGranules; gr++ )
		for ( ch = 0; ch < psFrame->nChannels; ch++ )
	    writeBitHolder( &psFrame->spectrumSI[gr][ch] );

	/* Checksum generation (if CRC enabled).*/

	if( !(psHeader->data[1] & 0x1) )
	{
		crc = calcCRC( psHeader->data, pEncodedOutput - psHeader->data );
		psHeader->data[4] = (char) (crc >> 8);
		psHeader->data[5] = (char) crc;
	}

	psHeader->size = pEncodedOutput - psHeader->data;
	psHeader->frameSize = psFrame->frameLength/8;
	pEncodedOutput = pOldEncodedOutput;
	return	psHeader->size*8;
}


/*____ writeHeader() _______________________________________________________*/

static int writeHeader( void )
{
	Header * psHeader;
	
	psHeader = pHeaderChain;
	memcpy( pEncodedOutput, psHeader->data, psHeader->size );
	pEncodedOutput += psHeader->size;
	* pEncodedOutput = 0;
	
	pHeaderChain = psHeader->pNext;
	psHeader->pNext = pFreeHeaderChain;
	pFreeHeaderChain = psHeader;

	return	(psHeader->frameSize - psHeader->size)*8;
}


/*____ writeMainDataBits() __________________________________________________*/

static int writeMainDataBits( BF_FrameData * psFrame, BitHolder *psBH )
{
  BitHolderElement * psElem = psBH->element;
  int i, bits = 0;
	unsigned int	val;
	int nBits;
	
  for ( i = 0; i < psBH->nrEntries; i++, psElem++ )
  {
		val = psElem->value;
		nBits = psElem->length;

		if ( BitsRemaining == 0 )
			BitsRemaining = writeHeader();

		if ( nBits > BitsRemaining )
		{
			nBits -= BitsRemaining;
			putbits( val >> nBits, BitsRemaining );
			BitsRemaining = writeHeader();
			putbits( val, nBits );
		}
		else
			putbits( val, nBits );
		BitsRemaining -= nBits;
		
		bits += psElem->length;
  }
  return	bits;
}

/*____ putbits() _____________________________________________________________*/


/*write n bits into the bit stream */

static void putbits(unsigned int val, int n)
{
	int  x;

  while (n > 0) 
  {
		x = MIN(n, outputBit);
		n -= x;
		outputBit -= x;
		* pEncodedOutput |= (val >> n) << outputBit;
		if (!outputBit)
		{
			outputBit = 8;
			pEncodedOutput++;
			* pEncodedOutput = 0;
		}
  }
}
