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

typedef enum FileType
{
	WAV,
	AIFF
} FileType; 

typedef enum	SampleType
{
	STEREO,
	INVERSE_STEREO,
	DOWNMIX_MONO,
	LEFT_CHANNEL_MONO,
	RIGHT_CHANNEL_MONO
} SampleType;

typedef struct SplInDef
{
	/* These may be set from the outside after openInput */

	SampleType		outputType;

	/* These may NOT be set from the outside */	
	
	int						freq;									/* Hz */
	unsigned int	length;								/* Length in samples */
	int						errcode;					
	FILE				* fp;
	unsigned int	samplesLeft;
	int						fReadStereo;					/* TRUE = Read sample is in stereo */

	/* Input format. Output is always 16-bit signed. */

	int						bits;									/* Bits/sample (8 or 16). */
	int						fSign;								/* Signed/unsigned */
	int						byteorder;						/* LITTLE_ENDIAN or BIG_ENDIAN */
	FileType			filetype;							/* WAV or AIFF */
} SplIn;



int	openInput( SplIn * psInfo, char	* pFileName );
int	readSamples( SplIn * psInfo, unsigned int nSamples, short * wpSamples );
int	closeInput( SplIn * psInfo );

