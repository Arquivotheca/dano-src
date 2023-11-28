/*
 * T2KSTRM.C
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

#ifdef USE_PRE_CACHING
void PreLoadT2KInputStream( InputStream *t, long requestedByteCount )
{
	if ( t->ReadToRamFunc != NULL ) {
		long byteCount;
		int ioCode;
		
		assert( t->ReadToRamFunc != NULL );
		
		byteCount 				= PRE_CACHE_SIZE;
		if ( requestedByteCount < PRE_CACHE_SIZE ) {
			byteCount = requestedByteCount;
		}
		
		t->bytesLeftToPreLoad 	= requestedByteCount - byteCount;
		
		
		t->privateBase			= t->cacheBase;
		ioCode = t->ReadToRamFunc( t->nonRamID, t->privateBase, t->pos, byteCount );
		tsi_Assert( t->mem, ioCode >= 0, T2K_EXT_IO_CALLBACK_ERR );
		t->cachePosition 		= t->pos;
		t->cacheCount			= (unsigned long)byteCount;
	}
}

int PrimeT2KInputStream(InputStream *t )
{
	long n = (long)(t->maxPos - t->pos);
	if ( n > 8 ) n = 8;
	if ( t->bytesLeftToPreLoad > n ) n = t->bytesLeftToPreLoad;
	if (n > (long)(t->maxPos - t->pos) )
		n = (long)(t->maxPos - t->pos);
	PreLoadT2KInputStream( t, n );
	return 0; /*****/
}
#endif /* USE_PRE_CACHING */



int32 ReadInt32( InputStream *t )
{
	register unsigned char *ptr = t->privateBase;
	unsigned long pos = t->pos;
	register uint32 lword;
#ifdef ENABLE_NON_RAM_STREAM
	unsigned long delta;
	unsigned char base[4];
	
	if ( ptr != NULL ) {		/* ptr == t->privateBase */
		delta = pos; 			/* ptr == &t->privateBase[pos] */
		#ifdef USE_PRE_CACHING
			if ( t->ReadToRamFunc != NULL ) {
				EnsureWeHaveDataInT2KInputStream( t, 4 );
				/* Combine additions into one step wo we never have a pointer out of bounds. */
				delta -= t->cachePosition; /* ptr = &t->privateBase[pos - t->cachePosition]; */
			}
		#endif
		ptr += delta;
	} else {
		int ioCode;
		ptr = base;
		ioCode = t->ReadToRamFunc( t->nonRamID, ptr, pos, 4 );
		tsi_Assert( t->mem, ioCode >= 0, T2K_EXT_IO_CALLBACK_ERR );
	}
#else
	ptr += pos;
#endif
	
	pos = pos + 4;
	assert( pos <= t->maxPos ); 
	t->pos = pos;
	
	lword = *ptr++;
	lword <<= 8;
	lword |= *ptr++;
	lword <<= 8;
	lword |= *ptr++;
	lword <<= 8;
	lword |= *ptr;
	
	return (int32)lword; /*****/
}



int16 ReadInt16( InputStream *t )
{
	register unsigned char *ptr = t->privateBase;
	unsigned long pos = t->pos;
	register uint16 word;
#ifdef ENABLE_NON_RAM_STREAM
	unsigned long delta;
	unsigned char base[2];
	
	if ( ptr != NULL ) {		/* ptr == t->privateBase */
		delta = pos; 			/* ptr == &t->privateBase[pos] */
		#ifdef USE_PRE_CACHING
			if ( t->ReadToRamFunc != NULL ) {
				EnsureWeHaveDataInT2KInputStream( t, 2 );
				/* Combine additions into one step wo we never have a pointer out of bounds. */
				delta -= t->cachePosition; /* ptr = &t->privateBase[pos - t->cachePosition]; */
			}
		#endif
		ptr += delta;
	} else {
		int ioCode;
		ptr = base;
		ioCode = t->ReadToRamFunc( t->nonRamID, ptr, pos, 2 );
		tsi_Assert( t->mem, ioCode >= 0, T2K_EXT_IO_CALLBACK_ERR );
	}
#else
	ptr += pos;
#endif
	
	pos = pos + 2;
	assert( pos <= t->maxPos ); 
	t->pos = pos;
	
	word = *ptr++;
	word <<= 8;
	word |= *ptr;
	
	return (int16)word; /*****/
}

#ifdef OBSOLETE
/* Obsolete since we rely on ReadUnsignedByteMacro and ReadUnsignedByteMacro2 in T2KSTRM.H */ 
uint8 ReadUnsignedByteSlow( InputStream *stream );

uint8 ReadUnsignedByteSlow( InputStream *t )
{
	unsigned long pos = t->pos;
	uint8 byte;
	register unsigned char *ptr;
#ifdef ENABLE_NON_RAM_STREAM
	unsigned char base[1];
	if ( t->privateBase == NULL ) {
		int ioCode;
		ptr = base;
		ioCode = t->ReadToRamFunc( t->nonRamID, ptr, pos, 1 );
		tsi_Assert( t->mem, ioCode >= 0, T2K_EXT_IO_CALLBACK_ERR );
	} else {
		ptr = &t->privateBase[pos];
	}
#else
	ptr = &t->privateBase[pos];
#endif
	
	assert( pos < t->maxPos ); 
	pos = pos + 1;
	byte = *ptr;
	t->pos = pos;
	return byte; /*****/
}
#endif


void ReadSegment( InputStream *t, uint8 *dest, long numBytes )
{
	if ( numBytes > 0 ) {
		unsigned long pos = t->pos;
		unsigned char *ptr;
		
#ifdef USE_SEAT_BELTS
		if ( pos + numBytes > t->maxPos ) {
			long numBytes2, i;
			numBytes2 = (long)(t->maxPos - pos);
			
			assert( numBytes2 >= 0  );
			for ( i = numBytes2; i < numBytes; i++ ) {
				dest[i] = 0; /* Initialize to a known state */
			}
			numBytes = numBytes2;
		}
#endif
#ifdef ENABLE_NON_RAM_STREAM
		if ( t->ReadToRamFunc != NULL ) {   /* prior to the USE_PRE_CACHING option the test was (t->privateBase == NULL) */
			int ioCode = t->ReadToRamFunc( t->nonRamID, dest, pos, numBytes );
			tsi_Assert( t->mem, ioCode >= 0, T2K_EXT_IO_CALLBACK_ERR );
		} else {
			ptr = &t->privateBase[pos];
			memcpy( dest, ptr, (unsigned long)numBytes );
		}
#else
		ptr = &t->privateBase[pos];	
		memcpy( dest, ptr, (unsigned long)numBytes );
#endif
		pos = pos + numBytes;
        tsi_Assert( t->mem, pos <= t->maxPos, T2K_BAD_FONT );
		t->pos = pos;
	}
}


long SizeInStream( InputStream *stream )
{
	return (long)(stream->maxPos - stream->posZero); /*****/
}

unsigned char *GetEntireStreamIntoMemory( InputStream *stream  )
{
#ifdef ENABLE_NON_RAM_STREAM /* changed 9/28/00 from MAYBE SOON */
    if ( stream->privateBase != NULL && stream->ReadToRamFunc == NULL ) {
        ; /* OK */
    } else if ( stream->privateBase == NULL && stream->ReadToRamFunc != NULL ) {
        int ioCode;
        stream->constructorType         = 1;
        stream->privateBase             = tsi_AllocMem(stream->mem, stream->maxPos );
        ioCode = stream->ReadToRamFunc( stream->nonRamID, stream->privateBase, 0, stream->maxPos );
        tsi_Assert( stream->mem, ioCode >= 0, T2K_EXT_IO_CALLBACK_ERR );
    } else {
        assert( false  );
    }
#else
    assert( stream->privateBase != NULL ); /* Only used for Type 1, does not work for non-RAM fonts */
#ifdef ENABLE_NON_RAM_STREAM
	assert( stream->ReadToRamFunc == NULL );
#endif
#endif

	return &stream->privateBase[stream->posZero]; /*****/
}

/*
 *
 */
InputStream *New_InputStream( tsiMemObject *mem, unsigned char *dataPtr, unsigned long length, int *errCode )
{
	InputStream *t;
	
	if ( errCode == NULL || (*errCode = setjmp( mem->env)) == 0 ) {
		/* try */
		t  = (InputStream*) tsi_AllocMem( mem, sizeof( InputStream ) );
		
		t->mem				= mem;
		t->privateBase		= dataPtr;
#ifdef ENABLE_NON_RAM_STREAM
		t->ReadToRamFunc	= NULL;
		t->nonRamID			= 0;
		t->cacheCount		= 0;
		t->cachePosition	= 0;
#endif
		t->pos				= 0;
		t->posZero			= 0;
		t->maxPos			= length;
		t->bitBufferIn  	= 0;
		t->bitCountIn		= 0;
		t->constructorType 	= 1;
	} else {
		/* catch */
		t = NULL;
		tsi_EmergencyShutDown( mem );
	}
	
	return t; /*****/
}

/*
 * Only for Internal T2K use
 */
InputStream *New_InputStream2( tsiMemObject *mem, InputStream *in, unsigned long offset, unsigned long length, int fastIndex, int *errCode )
{
	InputStream *t;
	
	if ( errCode == NULL || (*errCode = setjmp( mem->env)) == 0 ) {
		/* try */
		t = (InputStream*) (fastIndex ? tsi_FastAllocN( mem, sizeof( InputStream ), T2K_FB_IOSTREAM) : tsi_AllocMem( mem, sizeof( InputStream ) ) );
		
		t->mem				= mem;
		t->privateBase		= in->privateBase;
#ifdef ENABLE_NON_RAM_STREAM
		t->ReadToRamFunc	= in->ReadToRamFunc;
		t->nonRamID			= in->nonRamID;
		t->cacheCount		= 0;
		t->cachePosition	= 0;
#endif
#if 0
		t->pos				= offset;
		t->posZero			= offset;
		t->maxPos			= offset + length;
#else
		t->posZero			= in->posZero + offset;
		t->pos				= t->posZero;
		t->maxPos			= t->posZero + length;
#endif
		t->bitBufferIn  	= 0;
		t->bitCountIn		= 0;
		t->constructorType 	= 2;
#ifdef USE_PRE_CACHING
		PreLoadT2KInputStream( t, (long)length );
#endif			
	} else {
		/* catch */
		t = NULL;
		tsi_EmergencyShutDown( mem );
	}
	
	return t; /*****/
}

InputStream *New_InputStream3( tsiMemObject *mem, unsigned char *dataPtr, unsigned long length, int *errCode )
{
	InputStream *t;
	
	if ( errCode == NULL || (*errCode = setjmp( mem->env)) == 0 ) {
		/* try */
		t = (InputStream*) tsi_AllocMem( mem, sizeof( InputStream ) );
		
		t->mem				= mem;
		t->privateBase		= dataPtr;
#ifdef ENABLE_NON_RAM_STREAM
		t->ReadToRamFunc	= NULL;
		t->nonRamID			= 0;
		t->cacheCount		= 0;
		t->cachePosition	= 0;
#endif
		t->pos				= 0;
		t->posZero			= 0;
		t->maxPos			= length;
		t->bitBufferIn  	= 0;
		t->bitCountIn		= 0;
		t->constructorType 	= 3;
	} else {
		/* catch */
		t = NULL;
		tsi_EmergencyShutDown( mem );
	}
	
	return t; /*****/
}

#ifdef ENABLE_NON_RAM_STREAM
InputStream *New_NonRamInputStream( tsiMemObject *mem, void *nonRamID, PF_READ_TO_RAM readFunc, unsigned long length, int *errCode )
{
	InputStream *t;
	
	if ( errCode == NULL || (*errCode = setjmp( mem->env)) == 0 ) {
		/* try */
		t  = (InputStream*) tsi_AllocMem( mem, sizeof( InputStream ) );
		
		t->mem				= mem;
		t->privateBase		= NULL;
		t->ReadToRamFunc	= readFunc;
		t->nonRamID			= nonRamID;
		t->cacheCount		= 0;
		t->cachePosition	= 0;
		t->pos				= 0;
		t->posZero			= 0;
		t->maxPos			= length;
		t->bitBufferIn  	= 0;
		t->bitCountIn		= 0;
		t->constructorType 	= 4;
	} else {
		/* catch */
		t = NULL;
		tsi_EmergencyShutDown( mem );
	}
	
	return t; /*****/
}
#endif /* ENABLE_NON_RAM_STREAM */


/*
 *
 */
void Rewind_InputStream( InputStream *t )
{
	t->pos = t->posZero;
#ifdef ENABLE_NON_RAM_STREAM
	if ( t->pos < t->cachePosition ) {
		t->cacheCount		= 0; /* "flush" the cache */
		t->cachePosition	= 0;
	}
#endif
}

/*
 *
 */
void Seek_InputStream( InputStream *t, uint32 offset )
{
	t->pos = t->posZero + offset;
#ifdef ENABLE_NON_RAM_STREAM
	if ( t->pos < t->cachePosition ) {
		t->cacheCount		= 0; /* "flush" the cache */
		t->cachePosition	= 0;
	}
#endif
}
/*
 *
 */
uint32 Tell_InputStream( InputStream *t )
{
	return t->pos - t->posZero; /*****/
}



/*
 *
 */
void Delete_InputStream( InputStream *t, int *errCode )
{
	if ( t != NULL ) {
		if ( errCode == NULL || (*errCode = setjmp( t->mem->env)) == 0 ) {
			/* try */
			if ( t->constructorType == 1 ) {
				tsi_DeAllocMem( t->mem, t->privateBase );
				tsi_DeAllocMem( t->mem, t );
			} else {
				tsi_FastDeAllocN( t->mem, t, T2K_FB_IOSTREAM );
			}
		} else {
			/* catch */
			tsi_EmergencyShutDown( t->mem );
		}
	}
}


#ifdef ENABLE_WRITE
void WriteInt32( OutputStream *stream, int32 value )
{
	register unsigned long pos = stream->pos;
	register unsigned char *ptr;
	register uint32 lword = (uint32)value;
	
	pos += 4;
	if ( pos > stream->maxLength ) {
		stream->maxLength = pos + (pos>>1);
		stream->base = (uint8 *)tsi_ReAllocMem( stream->mem, stream->base, stream->maxLength );
		assert( stream->base != NULL );
	}
	ptr = &stream->base[stream->pos];
	stream->pos = pos;
	
	*ptr++ = (unsigned char)(lword >> 24);
	*ptr++ = (unsigned char)(lword >> 16);
	*ptr++ = (unsigned char)(lword >> 8);
	*ptr   = (unsigned char)(lword);
}	


void WriteInt16( OutputStream *stream, int16 value )
{
	register unsigned long pos = stream->pos;
	register unsigned char *ptr;
	register uint16 word = (uint16)value;
	
	pos += 2;
	if ( pos > stream->maxLength ) {
		stream->maxLength = pos + (pos>>1);
		stream->base = (uint8 *)tsi_ReAllocMem( stream->mem, stream->base, stream->maxLength );
		assert( stream->base != NULL );
	}
	ptr = &stream->base[stream->pos];
	stream->pos = pos;
	
	*ptr++ = (unsigned char)(word >> 8);
	*ptr   = (unsigned char)word;
}	


void WriteUnsignedByte( OutputStream *stream, uint8 value )
{
	register unsigned long pos = stream->pos;
	register unsigned char *ptr;
	
	pos++;
	if ( pos > stream->maxLength ) {
		stream->maxLength = pos + (pos>>1);
		stream->base = (uint8 *)tsi_ReAllocMem( stream->mem, stream->base, stream->maxLength );
		assert( stream->base != NULL );
	}
	ptr = &stream->base[stream->pos];
	stream->pos = pos;
	
	*ptr   = value;
}

void Write( OutputStream *stream, uint8 *src, long numBytes )
{
	if ( numBytes > 0 ) {
		register unsigned long pos = stream->pos;
		register unsigned char *ptr;
		
		pos += numBytes;
		if ( pos > stream->maxLength ) {
			stream->maxLength = pos + (pos>>1);
			stream->base = (uint8 *)tsi_ReAllocMem( stream->mem, stream->base, stream->maxLength );
			assert( stream->base != NULL );
		}
		ptr = &stream->base[stream->pos];
		stream->pos = pos;
		memcpy( ptr, src, (size_t)numBytes );
	}
}

long SizeOutStream( OutputStream *stream )
{
	if ( stream->pos > stream->maxPos ) {
		stream->maxPos = stream->pos;
	}
	return (long)(stream->maxPos); /*****/
}

long OutStreamPos( OutputStream *stream )
{
	if ( stream->pos > stream->maxPos ) {
		stream->maxPos = stream->pos;
	}
	return (long)stream->pos; /*****/
}

/*
 *
 */
OutputStream *New_OutputStream( tsiMemObject *mem, long initialSize )
{
	OutputStream *t = (OutputStream*) tsi_AllocMem( mem, sizeof( OutputStream ) );
	
	t->mem			= mem;
	t->pos			= 0;
	t->maxPos		= 0;
	if ( initialSize <= 0 ) initialSize = 1024;
	t->maxLength	= (uint32)initialSize;
	
	t->base 		= (uint8 *)tsi_AllocMem( t->mem, t->maxLength );
	assert( t->base != NULL );
	
	t->bitBufferOut  	= 0;
	t->bitCountOut		= 0;
	return t; /*****/
}

void Rewind_OutputStream( OutputStream *t )
{
	assert( t->pos <= t->maxLength );
	if ( t->pos > t->maxPos ) {
		t->maxPos = t->pos;
	}
	t->pos = 0;
}

/*
 *
 */
void Delete_OutputStream( OutputStream *t )
{
	if ( t != NULL ) {
		assert( t->pos <= t->maxLength );
		tsi_DeAllocMem( t->mem, t->base );
		tsi_DeAllocMem( t->mem, t );
	}
}

#endif /* ENABLE_WRITE */


#ifdef ENABLE_WRITE

void WriteBitsToStream( OutputStream *out, unsigned long bits, unsigned long count )
{
	/* First we have t->bitCountOut bits, followed by count bits */
	/* We always keep the bits slammed up againts the "left edge" */
	bits       <<= 32 - count - out->bitCountOut;
	out->bitCountOut += count;
	out->bitBufferOut |= bits;

	while ( out->bitCountOut >= 8 ) {
		WriteUnsignedByte( out, (unsigned char)(out->bitBufferOut >> 24) );
		out->bitBufferOut <<= 8;
		out->bitCountOut -= 8;
	}

}

void FlushOutStream( OutputStream *out )
{
	assert( out->bitCountOut < 8 );
	if ( out->bitCountOut > 0 ) {
		WriteUnsignedByte( out, (unsigned char)(out->bitBufferOut >> 24) );
		out->bitCountOut = 0;
	}
	out->bitBufferOut = 0;
}


void WriteUnsignedNumber( OutputStream *out, unsigned long n )
{
	unsigned char value;
	
	do {
		value = (unsigned char)(n & 0x7f);
		if ( n > 0x7f ) {
			value |= 0x80;
			n >>= 7;
		}
		WriteUnsignedByte( out, value );
	} while (value & 0x80);
}
#endif /* ENABLE_WRITE */

unsigned long ReadUnsignedNumber( InputStream *in )
{
	unsigned char value;
	unsigned long n = 0;
	unsigned long shift = 0;
	
	do {
		value = ReadUnsignedByteMacro( in );
		n |= ((value & 0x7f) << shift );
		shift += 7;
	} while (value & 0x80);
	return n; /*****/
}



/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: R:/src/FontFusion/Source/Core/rcs/t2kstrm.c 1.13 2001/05/02 20:03:11 reggers Exp $
 *                                                                           *
 *     $Log: t2kstrm.c $
 *     Revision 1.13  2001/05/02 20:03:11  reggers
 *     Titanium seat belts added. (Sampo)
 *     Revision 1.11  2000/10/04 17:40:20  reggers
 *     Corrected GetEntireStreamIntoMemory() for ENABLE_NON_RAM_STREAM (Sampo).
 *     Revision 1.10  2000/06/14 21:31:53  reggers
 *     Changed an assert to a tsi_Assert().
 *     Revision 1.9  2000/01/31 20:37:33  reggers
 *     Extra measure of checking we don't try to read past eof.
 *     Revision 1.8  2000/01/14 18:21:27  reggers
 *     Adapt New_InputStream2() to allow nesting > 1 level.
 *     Revision 1.7  1999/12/23 22:03:13  reggers
 *     New ENABLE_PCL branches. Rename any 'code' and 'data' symbols.
 *     Revision 1.6  1999/11/19 01:42:19  reggers
 *     Make non-ram stream error return possible.
 *     Revision 1.3  1999/07/16 17:52:06  sampo
 *     Sampo work. Drop #8 July 16, 1999
 *     Revision 1.2  1999/05/17 15:58:30  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/

