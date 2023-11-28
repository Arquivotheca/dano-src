//========================================================================
//	MAIFFPlayer.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include "MAIFFPlayer.h"
#include "MBufferPlayer.h"

// ---------------------------------------------------------------------------
//		MAIFFPlayer
// ---------------------------------------------------------------------------
//	Constructor

MAIFFPlayer::MAIFFPlayer(
	const void *	inData,
	int32			inLength):
		fPlayer(nil)
{
	fCommChunk = nil;
	fSoundChunk = nil;

	const char *			data = (const char *) inData;
	const char *			ptr;
	const AIFFFileHeader *	fileHeader = (AIFFFileHeader *) inData;
	
	if (inLength > sizeof(AIFFFileHeader) &&
		fileHeader->magicWord == AIFFMagicWord &&
		fileHeader->fileType == AIFFFileType)
	{
		ptr = data + sizeof(AIFFFileHeader);
		const ChunkHeader*		chunk = (const ChunkHeader*) ptr;
		bool					good = true;

		while (ptr < data + inLength && good)
			good = ProcessAChunk(ptr);

		if (good && fCommChunk != nil && fSoundChunk != nil)
		{
			const void *	data = (const void *) ((char*) fSoundChunk + sizeof(SSNDChunk) + B_BENDIAN_TO_HOST_INT32(fSoundChunk->offset));
			int32 			bufSize = B_BENDIAN_TO_HOST_INT32(fSoundChunk->chunkSize);
			int32 			sampleSize = B_BENDIAN_TO_HOST_INT16(fCommChunk->bits) / 8;
			int32 			sampleFormat = B_LINEAR_SAMPLES;
			int32			byteOrder = B_BIG_ENDIAN;
			int32 			numChannels = B_BENDIAN_TO_HOST_INT16(fCommChunk->channels);
			int32 			sampleRate = (int32) ConvertFromIeeeExtended(fCommChunk->rate);

			sampleRate = 8000;		// hard-code because ConvertFromIeeeExtended doesn't work ????

			fPlayer = new MBufferPlayer(data, bufSize, sampleSize, sampleFormat, 
							byteOrder, numChannels, sampleRate);
		}
	}
}

// ---------------------------------------------------------------------------
//		ProcessAChunk
// ---------------------------------------------------------------------------

bool
MAIFFPlayer::ProcessAChunk(
	const char *&	ioData)
{
	const ChunkHeader*		chunk = (const ChunkHeader*) ioData;
	bool					good = true;

	switch (chunk->chunkType)
	{
		case COMMType:
			fCommChunk = (const COMMChunk*) chunk;
			if (B_BENDIAN_TO_HOST_INT32(fCommChunk->chunkSize) != 18 ||
				B_BENDIAN_TO_HOST_INT16(fCommChunk->channels) > 2 ||
				B_BENDIAN_TO_HOST_INT16(fCommChunk->channels) < 1 ||
				! (B_BENDIAN_TO_HOST_INT16(fCommChunk->bits) == 8 || B_BENDIAN_TO_HOST_INT16(fCommChunk->bits) == 16))
				good = false;
			break;
		
		case SSNDType:
			fSoundChunk = (const SSNDChunk*) chunk;
			if (fSoundChunk->blocksize != 0)
				good = false;
			break;
			
		default:
			break;
	}
	
	if (good)
		ioData = ioData + sizeof(ChunkHeader) + chunk->chunkSize;

	return good;
}

// ---------------------------------------------------------------------------
//		~MAIFFPlayer
// ---------------------------------------------------------------------------

MAIFFPlayer::~MAIFFPlayer()
{
	delete fPlayer;
}

// ---------------------------------------------------------------------------
//		Play
// ---------------------------------------------------------------------------

void
MAIFFPlayer::Play()
{
	if (fPlayer)
		fPlayer->Play(false);
}

/*
 * C O N V E R T   T O   I E E E   E X T E N D E D
 */

/* Copyright (C) 1988-1991 Apple Computer, Inc.
 * All rights reserved.
 *
 * Machine-independent I/O routines for IEEE floating-point numbers.
 *
 * NaN's and infinities are converted to HUGE_VAL or HUGE, which
 * happens to be infinity on IEEE machines.  Unfortunately, it is
 * impossible to preserve NaN's in a machine-independent way.
 * Infinities are, however, preserved on IEEE machines.
 *
 * These routines have been tested on the following machines:
 *    Apple Macintosh, MPW 3.1 C compiler
 *    Apple Macintosh, THINK C compiler
 *    Silicon Graphics IRIS, MIPS compiler
 *    Cray X/MP and Y/MP
 *    Digital Equipment VAX
 *
 *
 * Implemented by Malcolm Slaney and Ken Turkowski.
 *
 * Malcolm Slaney contributions during 1988-1990 include big- and little-
 * endian file I/O, conversion to and from Motorola's extended 80-bit
 * floating-point format, and conversions to and from IEEE single-
 * precision floating-point format.
 *
 * In 1991, Ken Turkowski implemented the conversions to and from
 * IEEE double-precision format, added more precision to the extended
 * conversions, and accommodated conversions involving +/- infinity,
 * NaN's, and denormalized numbers.
 */

#ifndef HUGE_VAL
# define HUGE_VAL HUGE
#endif /*HUGE_VAL*/

# define FloatToUnsigned(f)      ((unsigned long)(((long)(f - 2147483648.0)) + 2147483647L) + 1)

void
MAIFFPlayer::ConvertToIeeeExtended(double num, unsigned char * bytes)
{
    int    		sign;
    int 		expon;
    double 		fMant, fsMant;
    unsigned long hiMant, loMant;

    if (num < 0) {
        sign = 0x8000;
        num *= -1;
    } else {
        sign = 0;
    }

    if (num == 0) {
        expon = 0; hiMant = 0; loMant = 0;
    }
    else {
        fMant = frexp(num, &expon);
        if ((expon > 16384) || !(fMant < 1)) {    /* Infinity or NaN */
            expon = sign|0x7FFF; hiMant = 0; loMant = 0; /* infinity */
        }
        else {    /* Finite */
            expon += 16382;
            if (expon < 0) {    /* denormalized */
                fMant = ldexp(fMant, expon);
                expon = 0;
            }
            expon |= sign;
            fMant = ldexp(fMant, 32);          
            fsMant = floor(fMant); 
            hiMant = FloatToUnsigned(fsMant);
            fMant = ldexp(fMant - fsMant, 32); 
            fsMant = floor(fMant); 
            loMant = FloatToUnsigned(fsMant);
        }
    }
    
    bytes[0] = expon >> 8;
    bytes[1] = expon;
    bytes[2] = hiMant >> 24;
    bytes[3] = hiMant >> 16;
    bytes[4] = hiMant >> 8;
    bytes[5] = hiMant;
    bytes[6] = loMant >> 24;
    bytes[7] = loMant >> 16;
    bytes[8] = loMant >> 8;
    bytes[9] = loMant;
}


/*
 * C O N V E R T   F R O M   I E E E   E X T E N D E D  
 */

/* 
 * Copyright (C) 1988-1991 Apple Computer, Inc.
 * All rights reserved.
 *
 * Machine-independent I/O routines for IEEE floating-point numbers.
 *
 * NaN's and infinities are converted to HUGE_VAL or HUGE, which
 * happens to be infinity on IEEE machines.  Unfortunately, it is
 * impossible to preserve NaN's in a machine-independent way.
 * Infinities are, however, preserved on IEEE machines.
 *
 * These routines have been tested on the following machines:
 *    Apple Macintosh, MPW 3.1 C compiler
 *    Apple Macintosh, THINK C compiler
 *    Silicon Graphics IRIS, MIPS compiler
 *    Cray X/MP and Y/MP
 *    Digital Equipment VAX
 *
 *
 * Implemented by Malcolm Slaney and Ken Turkowski.
 *
 * Malcolm Slaney contributions during 1988-1990 include big- and little-
 * endian file I/O, conversion to and from Motorola's extended 80-bit
 * floating-point format, and conversions to and from IEEE single-
 * precision floating-point format.
 *
 * In 1991, Ken Turkowski implemented the conversions to and from
 * IEEE double-precision format, added more precision to the extended
 * conversions, and accommodated conversions involving +/- infinity,
 * NaN's, and denormalized numbers.
 */

#ifndef HUGE_VAL
# define HUGE_VAL HUGE
#endif /*HUGE_VAL*/

# define UnsignedToFloat(u)         (((double)((long)(u - 2147483647L - 1))) + 2147483648.0)

/****************************************************************
 * Extended precision IEEE floating-point conversion routine.
 ****************************************************************/

double 
MAIFFPlayer::ConvertFromIeeeExtended(
	const unsigned char *		bytes)
{
	double		f;
	int			expon;
	unsigned long hiMant, loMant;
    
    expon = ((bytes[0] & 0x7F) << 8) | (bytes[1] & 0xFF);
    hiMant    =    ((unsigned long)(bytes[2] & 0xFF) << 24)
            |    ((unsigned long)(bytes[3] & 0xFF) << 16)
            |    ((unsigned long)(bytes[4] & 0xFF) << 8)
            |    ((unsigned long)(bytes[5] & 0xFF));
    loMant    =    ((unsigned long)(bytes[6] & 0xFF) << 24)
            |    ((unsigned long)(bytes[7] & 0xFF) << 16)
            |    ((unsigned long)(bytes[8] & 0xFF) << 8)
            |    ((unsigned long)(bytes[9] & 0xFF));

    if (expon == 0 && hiMant == 0 && loMant == 0) {
        f = 0;
    }
    else {
        if (expon == 0x7FFF) {    /* Infinity or NaN */
           f = HUGE_VAL;
        }
        else {
            expon -= 16383;
            f  = ldexp(UnsignedToFloat(hiMant), expon-=31);
            f += ldexp(UnsignedToFloat(loMant), expon-=32);
        }
    }

    if (bytes[0] & 0x80)
        return -f;
    else
        return f;
}
