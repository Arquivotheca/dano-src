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
#include	<string.h>
#include	"system.h"
#include	"samplein.h"



/* Errorcodes
  -------------
    0 = No Error (OK).
   -1 = Unsupported filetype.
   -2	= Couldn't open file.
   -3 = Unexpected end of file.
   -4 = (Input file is not in the format the file-extension says).
   -5 = Important chunk missing.
   -6 = Samples are in unsupported (compressed?) format.
*/


#define		STR_COMM		0x4d4d4f43
#define		STR_SSND		0x444e5353
#define		STR_FORM		0x4d524f46
#define		STR_AIFF		0x46464941
#define		STR_RIFF		0x46464952
#define		STR_WAVE		0x45564157
#define		STR_fmt			0x20746d66
#define		STR_data		0x61746164


/*____ Function Prototypes ____________________________________________________*/

static int	initWAV( SplIn * psInfo );
static uint	readWAVSamples( SplIn * psInfo, int nSamples, short * wpSamples );

static int	initAIFF( SplIn * psInfo );
static uint	readAIFFSamples( SplIn * psInfo, int nSamples, short * wpSamples );

static int myFseek( FILE * fp, long offset );


ulong INLINE intlLong( char iLong[4] );
ulong INLINE mcLong( char mcLong[4] );
ushort INLINE intlShort( char iShort[2] );
ushort INLINE mcShort( char mcShort[2] );



/*____ Static Data ____________________________________________________________*/



/*____ openInput() ____________________________________________________________*/

int	openInput( SplIn * psInfo, char	* pFileName )
{
	int		x;
	char	header[3*4];

	psInfo->errcode = 0;


	/* Set Filepointer */

	if( pFileName == NULL )
	{
		psInfo->fp = stdin;
	}
  else
	{
		psInfo->fp = fopen( pFileName, "rb" );
		if( psInfo->fp == NULL ) goto couldNotOpen;
	}


	/* Read and analyze header */

	if( fread( header, 4, 3, psInfo->fp ) != 3 ) goto couldNotOpen;

	if( intlLong(&header[0]) == STR_RIFF && intlLong(&header[8]) == STR_WAVE )
		x = initWAV( psInfo );
	else if( intlLong(&header[0]) == STR_FORM && intlLong(&header[8]) == STR_AIFF )
		x = initAIFF( psInfo );
	else
    x = FALSE;

	if( x == FALSE )
	{
		if( psInfo->fp != stdin )
			fclose( psInfo->fp );
		psInfo->samplesLeft = 0;
		return	FALSE;
	}


	/* Set some flags */

	if( psInfo->fReadStereo )
		psInfo->outputType = STEREO;
	else
		psInfo->outputType = DOWNMIX_MONO;

	return	TRUE;

couldNotOpen:
	psInfo->errcode = -2;
	psInfo->samplesLeft = 0;
	return	FALSE;
}


/*____ readSamples() __________________________________________________________*/

int	readSamples( SplIn * psInfo, uint nSamples, short * wpSamples )
{
	int		retVal = 0;
	uint		i;
	uint	readSamples;
	char	temp;
	short	tmp;

	if( psInfo->fReadStereo == TRUE && (psInfo->outputType == DOWNMIX_MONO
      || psInfo->outputType == LEFT_CHANNEL_MONO || psInfo->outputType == RIGHT_CHANNEL_MONO) )
		readSamples = nSamples*2;
	else
		readSamples = nSamples;


	if( psInfo->samplesLeft == 0 )
		return	0;

	if( readSamples < psInfo->samplesLeft )
		psInfo->samplesLeft -= readSamples;
	else
	{
		readSamples = psInfo->samplesLeft;
		psInfo->samplesLeft = 0;
	}


	if( psInfo->filetype == WAV )
		retVal = readWAVSamples( psInfo, readSamples, wpSamples );
	else if( psInfo->filetype == AIFF )
		retVal = readAIFFSamples( psInfo, readSamples, wpSamples );

	if( psInfo->samplesLeft == 0 || retVal == FALSE )
	{
		psInfo->samplesLeft = 0;
    if( psInfo->fp != stdin )
			fclose( psInfo->fp );
	}


	/* Possibly swap byteorder */

	if( psInfo->bits == 16 && psInfo->byteorder != BYTEORDER )
	{
		for( i = 0 ; i < readSamples ; i++ )
		{
			temp = ((char *)wpSamples)[i*2];
			((char *)wpSamples)[i*2] = ((char *)wpSamples)[i*2+1];
			((char *)wpSamples)[i*2+1] = temp;
		}
	}


	/* Convert between 8/16-bit */

	if( psInfo->bits == 8 )
	{
 		for( i = readSamples-1 ; i > 0 ; i-- )
    	wpSamples[i] = ((short)((unsigned char *) wpSamples)[i]) << 8;
 		wpSamples[i] = ((short)((unsigned char *) wpSamples)[i]) << 8;
	}

	/* Convert unsigned to signed */

	if( psInfo->fSign == FALSE )
	{
		for( i = 0 ; i < readSamples ; i++ )
			wpSamples[i] ^= 0x8000;
	}

	/* Convert from Stereo to Mono or inverse stereo in a number of ways */

	if( psInfo->outputType != STEREO && psInfo->fReadStereo == TRUE )
	{
		if( psInfo->outputType == DOWNMIX_MONO )
			for( i = 0 ; i < readSamples/2 ; i++ )
				wpSamples[i] = (short)((((int)wpSamples[i*2]) + ((int)wpSamples[i*2+1])) >> 1);

		if( psInfo->outputType == LEFT_CHANNEL_MONO )
			for( i = 0 ; i < readSamples/2 ; i++ )
				wpSamples[i] = wpSamples[i*2];

		if( psInfo->outputType == RIGHT_CHANNEL_MONO )
			for( i = 0 ; i < readSamples/2 ; i++ )
				wpSamples[i] = wpSamples[i*2+1];

		if( psInfo->outputType == INVERSE_STEREO )
		{
			for( i = 0 ; i < readSamples ; i += 2 )
			{
				tmp = wpSamples[i];
				wpSamples[i] = wpSamples[i+1];
				wpSamples[i+1] = tmp;
			}
		}

	}
	
	return	retVal;
}


/*____ closeInput() ___________________________________________________________*/

int	closeInput( SplIn * psInfo )
{
	if( psInfo->samplesLeft != 0 )
	{
		if( psInfo->fp != stdin )
			fclose( psInfo->fp );
		psInfo->samplesLeft = 0;
		return	TRUE;
	}
	return	FALSE;
}


/*____ initWAV() ______________________________________________________________*/

static int	initWAV( SplIn * psInfo )

{
	char	header[3*4];	
	int		fFmtChunkFound = FALSE;

struct 
{ 
	short		wFormatTag;						/* Format category */
  short		wChannels;						/* Number of channels */
  long		dwSamplesPerSec;			/* Sampling rate */
  long		dwAvgBytesPerSec;			/* For buffer estimation */
  short		wBlockAlign;					/* Data block size */
	short		bitsPerSample;				/* Actually a PCM-specific additional byte... */
} sFmtChunk;

	char	aTemp[sizeof( sFmtChunk )];


	/* Go through the chunks until we have found 'data'. */


	if( fread( header, 4, 2, psInfo->fp ) != 2 ) goto unexpEndOfFile;

	while( intlLong( &header[0] ) != STR_data )
	{
		if( intlLong( &header[0] ) == STR_fmt )
		{
			if( fread( aTemp, sizeof( sFmtChunk ), 1, psInfo->fp ) != 1 ) 
				goto unexpEndOfFile;
			myFseek( psInfo->fp, intlLong( &header[4] ) - sizeof( sFmtChunk ) );
			fFmtChunkFound = TRUE;
		}
		else
			myFseek( psInfo->fp, intlLong( &header[4] ) );

		if( fread( header, 4, 2, psInfo->fp ) != 2 ) goto unexpEndOfFile;
	}


	/* Fill in sFmtChunk */

	sFmtChunk.wFormatTag = intlShort( aTemp );
	sFmtChunk.wChannels = intlShort( aTemp + 2);
	sFmtChunk.dwSamplesPerSec = intlLong( aTemp + 4 );
	sFmtChunk.dwAvgBytesPerSec = intlLong( aTemp + 8 );
	sFmtChunk.wBlockAlign = intlShort( aTemp + 12 );
	sFmtChunk.bitsPerSample = intlShort( aTemp + 14 );


	/* Process the data in sFmtChunk */

	if( fFmtChunkFound != TRUE )
	{
		psInfo->errcode = -5;
		return	FALSE;
	}
	
	if( sFmtChunk.wFormatTag != 1 )
	{
		psInfo->errcode = -6;
		return	FALSE;									/* Not a PCM-sample. */
	}

	if( sFmtChunk.wChannels > 2 )
	{
		psInfo->errcode = -6;
		return	FALSE;									/* More than two channels. */
	}

	psInfo->freq = sFmtChunk.dwSamplesPerSec;
	psInfo->fReadStereo = sFmtChunk.wChannels - 1;
	psInfo->bits = sFmtChunk.bitsPerSample;

	if( sFmtChunk.bitsPerSample == 8 )
		psInfo->fSign = FALSE;
	else
		psInfo->fSign = TRUE;

	psInfo->length = intlLong(&header[4]);
	if( sFmtChunk.bitsPerSample == 16 )
		psInfo->length /= 2;
	psInfo->samplesLeft = psInfo->length;

	psInfo->byteorder = LITTLE_ENDIAN;
	psInfo->filetype = WAV;

	return	TRUE;

unexpEndOfFile:
	psInfo->errcode = -3;
	return	FALSE;

}


/*____ readWAVsamples() _______________________________________________________*/

static uint	readWAVSamples( SplIn * psInfo, int nSamples, short * wpSamples )
{
	return fread( wpSamples, psInfo->bits/8, nSamples, psInfo->fp );
}


/*____ initAIFF() _____________________________________________________________*/

static int	initAIFF( SplIn * psInfo )
{
	char		header[3*4];	

	int			fPosAtSample = FALSE;
	int			fCommChunkFound = FALSE;
	uchar	* pFreq;
	int			expo;

	double	sampleRate;

	struct 
	{ 
		short           numChannels;
    unsigned long   numSampleFrames;
    short           sampleSize;
/*    char						sampleRate[10]; */

	} sCommChunk;

	char	aTemp[18];


	/* Go through the file and get COMM and SSND chunks */

	while( fPosAtSample == FALSE )
	{
		if( fread( header, 4, 2, psInfo->fp ) != 2 ) goto unexpEndOfFile;

		switch( intlLong( &header[0] ) )
		{
			case	STR_COMM:
				if( fread( aTemp, 18, 1, psInfo->fp ) != 1 )
					goto unexpEndOfFile;
				fCommChunkFound = TRUE;
				break;
			case	STR_SSND:
				myFseek( psInfo->fp, 8 );
				fPosAtSample = TRUE;
				break;
			default:
				myFseek( psInfo->fp, (mcLong( &header[4] ) + 1) &0xFFFFFFFE );
		}
	}

	if( fPosAtSample != TRUE || fCommChunkFound != TRUE )
		return	FALSE;

	/* Fill in sCommChunk */

	sCommChunk.numChannels = mcShort( aTemp );
	sCommChunk.numSampleFrames = mcLong( aTemp + 2 );
	sCommChunk.sampleSize = mcShort( aTemp + 6 );

	/* Read Samplerate */

	pFreq = (uchar *) aTemp + 8;


	sampleRate = pFreq[9];
	sampleRate /= 256;
	sampleRate += pFreq[8];
	sampleRate /= 256;
	sampleRate += pFreq[7];
	sampleRate /= 256;
	sampleRate += pFreq[6];
	sampleRate /= 256;
	sampleRate += pFreq[5];
	sampleRate /= 256;
	sampleRate += pFreq[4];
	sampleRate /= 256;
	sampleRate += pFreq[3];
	sampleRate /= 256;
	sampleRate += pFreq[2];

	sampleRate /= 256;


	expo = (pFreq[0] << 8) + pFreq[1];
	expo -= 16383;
	expo += 1;

	while( expo != 0 )
	{
		if( expo < 0 )
		{
			sampleRate /= 2;
			expo++;
		}
		else
		{
			sampleRate *= 2;
			expo--;
		}
	}

	
	/* compensate for some apps or Macs which write slightly off sample rates */
		
	if ((sampleRate == 44099) || (sampleRate == 44101))
		sampleRate = 44100;
	if ((sampleRate == 31999) || (sampleRate == 32001))
		sampleRate = 32000;
	if ((sampleRate == 47999) || (sampleRate == 48001))
		sampleRate = 48000;

			
	/* Check number of channles and samplesize, just to be sure... */

	if( sCommChunk.numChannels > 2 )
	{
		psInfo->errcode = -6;
		return	FALSE;									/* More than two channels. */
	}

	if( sCommChunk.sampleSize != 16 && sCommChunk.sampleSize != 8 )
	{
		psInfo->errcode = -6;
		return	FALSE;									/* Strange samplesize. */
	}

	
	/* Fill in psInfo-struct */
	
	psInfo->freq = (int) (sampleRate + 0.5);
	psInfo->fReadStereo = sCommChunk.numChannels - 1;
	psInfo->bits = sCommChunk.sampleSize;

	psInfo->fSign = TRUE;							/* Always signed ? */

	psInfo->length = sCommChunk.numSampleFrames * sCommChunk.numChannels;
	psInfo->samplesLeft = psInfo->length;

	psInfo->byteorder = BIG_ENDIAN;
	psInfo->filetype = AIFF;


	return	TRUE;

unexpEndOfFile:
	psInfo->errcode = -3;
	return	FALSE;


}


/*____ readAIFFsamples() ______________________________________________________*/

static uint	readAIFFSamples( SplIn * psInfo, int nSamples, short * wpSamples )
{
	return fread( wpSamples, psInfo->bits/8, nSamples, psInfo->fp );
}


/*____ intlLong() _____________________________________________________________*/

ulong INLINE intlLong( char iLong[4] )
{
	return	((ulong)((uchar*)iLong)[0]) + (((ulong)((uchar*)iLong)[1]) << 8)
		    + (((ulong)((uchar*)iLong)[2]) << 16) + (((ulong)((uchar*)iLong)[3]) << 24);
}


/*____ mcLong() _______________________________________________________________*/

ulong INLINE mcLong( char mcLong[4] )
{
	return	((ulong)((uchar*)mcLong)[3]) + (((ulong)((uchar*)mcLong)[2]) << 8 )
		    + (((ulong)((uchar*)mcLong)[1]) << 16) + (((ulong)((uchar*)mcLong)[0]) << 24);
}


/*____ intlShort() ____________________________________________________________*/

ushort INLINE intlShort( char iShort[2] )
{
	return	((ushort)((uchar*)iShort)[0]) + (((ushort)((uchar*)iShort)[1]) << 8);
}


/*____ mcShort() ______________________________________________________________*/

ushort INLINE mcShort( char mcShort[2] )
{
	return	((ushort)((uchar*)mcShort)[1]) + (((ushort)((uchar*)mcShort)[0]) << 8);
}

/*____ myFseek() ______________________________________________________________*/

/* We can't use the real fseek() since you can't seek in a stream (stdin) */

int myFseek( FILE * fp, long offset )
{
	char	dummy[256];

	while( offset / 256 )
	{
		fread( dummy, 256, 1, fp );
		offset -= 256;
  }

	if( offset )
		fread( dummy, offset, 1, fp );

	return	0;
}





