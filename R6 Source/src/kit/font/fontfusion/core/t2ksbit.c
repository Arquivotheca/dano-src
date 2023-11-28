/*
 * T2KSBIT.c
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

#include "config.h"
#include "dtypes.h"
#include "tsimem.h"
#include "t2kstrm.h"
#include "glyph.h"
#include "truetype.h"
#include "util.h"
#include "t2ksbit.h"


#ifdef ENABLE_SBIT


/*
 * The bitmapSizeTable class constructor.
 * Each strike is defined by one bitmapSizeTable.
 */
static bitmapSizeTable *New_bitmapSizeTable( tsiMemObject *mem, InputStream *in, uint32 blocOffset )
{
	register int i;
	uint32 savepos;
	
	bitmapSizeTable *t	= (bitmapSizeTable *) tsi_AllocMem( mem, sizeof( bitmapSizeTable ) );
	t->mem				= mem;
	
	t->indexSubTableArrayOffset		= (uint32)ReadInt32(in);
	t->indexTableSize				= (uint32)ReadInt32(in);
	t->numberOfIndexSubTables		= (uint32)ReadInt32(in);
	t->colorRef						= (uint32)ReadInt32(in);
	
	for ( i = 0; i < NUM_SBIT_METRICS_BYTES; i++ ) {
		t->hori[i] = ReadUnsignedByteMacro( in );
	}
	for ( i = 0; i < NUM_SBIT_METRICS_BYTES; i++ ) {
		t->vert[i] = ReadUnsignedByteMacro( in );
	}
	t->startGlyphIndex				= (uint16)ReadInt16( in );
	t->endGlyphIndex				= (uint16)ReadInt16( in );

	t->ppemX						= ReadUnsignedByteMacro( in );
	t->ppemY						= ReadUnsignedByteMacro( in );
	t->bitDepth						= ReadUnsignedByteMacro( in );
	t->flags						= ReadUnsignedByteMacro( in );

	t->table = (indexSubTableArray *)tsi_AllocMem( mem, t->numberOfIndexSubTables * sizeof( indexSubTableArray ) );

	savepos = Tell_InputStream( in );
	Seek_InputStream( in, blocOffset + t->indexSubTableArrayOffset );
	for ( i = 0; i < (int)t->numberOfIndexSubTables; i++ ) {
		t->table[i].firstGlyphIndex		= (uint16)ReadInt16( in );
		t->table[i].lastGlyphIndex		= (uint16)ReadInt16( in );
		t->table[i].additionalOffsetToIndexSubTable	= (uint32)ReadInt32(in);
	}
	Seek_InputStream( in, savepos );
	return t; /*****/
}


/*
 * The bitmapSizeTable class destructor
 */
static void Delete_bitmapSizeTable( bitmapSizeTable *t )
{
	
	tsi_DeAllocMem( t->mem, t->table );
	tsi_DeAllocMem( t->mem, t );
}


/*
 * The blocClass class contructor (EBLC/bloc table)
 */
blocClass *New_blocClass( tsiMemObject *mem, int fontIsSbitOnly, InputStream *in )
{
	F16Dot16 version;
	uint32	startOffset;
	register int i;
	blocClass *t = NULL;
	
	startOffset			= Tell_InputStream( in );
	version				= ReadInt32(in);
	
	/* Assume we understand this version number range. */
	if ( version >= 0x00020000 && version < 0x00030000 ) {
		t 					= (blocClass *) tsi_AllocMem( mem, sizeof( blocClass ) );
		t->mem				= mem;
		t->startOffset  	= startOffset;
		t->fontIsSbitOnly 	= fontIsSbitOnly;
		
		/* Read the EBLC/bloc header. */
		t->version			= version;
		t->nTables			= (uint32)ReadInt32(in);
		
		/* The EBLC/bloc header is followed immediately by the bitmapSizeTable array(s). */
		t->table		= (bitmapSizeTable **) tsi_AllocMem( mem, t->nTables * sizeof( bitmapSizeTable * ) );
		for ( i = 0; i < (int)t->nTables; i++ ) {
			t->table[i] = New_bitmapSizeTable( mem, in, t->startOffset ); /* One bitmapSizeTable for each strike */
		}
		/* Initialize data fields */
		t->gInfo.offsetA		= 0;
		t->gInfo.offsetB		= 0;
		t->gInfo.imageFormat	= 0;
		t->gInfo.glyphIndex		= 0;
		t->gInfo.ppemX			= 0;
		t->gInfo.ppemY			= 0;
		
		t->gInfo.baseAddr		= NULL;
		t->gInfo.rowBytes		= 0;
	}
	
	return t; /*****/
}

/*
 * The blocClass class destructor (EBLC/bloc table)
 */
void Delete_blocClass( blocClass *t )
{
	register int i;
	if ( t != NULL ) {
		for ( i = 0; i < (int)t->nTables; i++ ) {
			Delete_bitmapSizeTable( t->table[i] );
		}
		tsi_DeAllocMem( t->mem, t->table );
		tsi_DeAllocMem( t->mem, t->gInfo.baseAddr );
		tsi_DeAllocMem( t->mem, t );
	}
}


/*
 * The ebscClass class contructor (EBSC table)
 */
ebscClass *New_ebscClass( tsiMemObject *mem, InputStream *in )
{
	F16Dot16 version;
	register int i, j;
	ebscClass *t = NULL;
	
	version			= ReadInt32(in);
	
	/* Assume we understand this version number range. */
	if ( version >= 0x00020000 && version < 0x00030000 ) {
		t 				= (ebscClass *) tsi_AllocMem( mem, sizeof( ebscClass ) );
		t->mem			= mem;

		t->version		= version;
		t->numSizes		= (uint32)ReadInt32(in);
		
		t->table		= (bitmapScaleEntry *) tsi_AllocMem( mem, t->numSizes * sizeof( bitmapScaleEntry ) );
		for ( i = 0; i < (int)t->numSizes; i++ ) {
			for ( j = 0; j < NUM_SBIT_METRICS_BYTES; j++ ) {
				t->table[i].hori[j] = ReadUnsignedByteMacro( in );
			}
			for ( j = 0; j < NUM_SBIT_METRICS_BYTES; j++ ) {
				t->table[i].vert[j] = ReadUnsignedByteMacro( in );
			}
			t->table[i].ppemX			= ReadUnsignedByteMacro( in );
			t->table[i].ppemY			= ReadUnsignedByteMacro( in );
			t->table[i].substitutePpemX	= ReadUnsignedByteMacro( in );
			t->table[i].substitutePpemY	= ReadUnsignedByteMacro( in );
		}
	}
	return t; /*****/
}

/*
 * The ebscClass class destructor (EBSC table)
 */
void Delete_ebscClass( ebscClass *t )
{
	if ( t != NULL ) {
		tsi_DeAllocMem( t->mem, t->table );
		tsi_DeAllocMem( t->mem, t );
	}
}


/*
 *
 */
static void ReadBigMetrics( bigGlyphMetrics *m, InputStream *in )
{
	m->height		= ReadUnsignedByteMacro( in );
	m->width		= ReadUnsignedByteMacro( in );
	m->horiBearingX	= (int8)ReadUnsignedByteMacro( in );
	m->horiBearingY	= (int8)ReadUnsignedByteMacro( in );
	m->horiAdvance	= ReadUnsignedByteMacro( in );
	m->vertBearingX	= (int8)ReadUnsignedByteMacro( in );
	m->vertBearingY	= (int8)ReadUnsignedByteMacro( in );
	m->vertAdvance	= ReadUnsignedByteMacro( in );
}

/*
 *
 */
static void ReadSmallMetrics( bigGlyphMetrics *m, InputStream *in )
{
	/* We read smallGlyphMetrics but put the result into bigGlyphMetrics to simplify. */
	
	/* smallGlyphMetrics *m */
	m->height	= ReadUnsignedByteMacro( in );
	m->width	= ReadUnsignedByteMacro( in );
	m->horiBearingX	= (int8)ReadUnsignedByteMacro( in );
	m->horiBearingY	= (int8)ReadUnsignedByteMacro( in );
	m->horiAdvance	= ReadUnsignedByteMacro( in );
	/* Copy over to the other direction */
	m->vertBearingX = m->horiBearingX;
	m->vertBearingY = m->horiBearingY;
	m->vertAdvance	= m->horiAdvance;
#ifdef OLD
	/* smallGlyphMetrics *m */
	m->height	= ReadUnsignedByteMacro( in );
	m->width	= ReadUnsignedByteMacro( in );
	m->BearingX	= (int8)ReadUnsignedByteMacro( in );
	m->BearingY	= (int8)ReadUnsignedByteMacro( in );
	m->Advance	= ReadUnsignedByteMacro( in );
#endif
}

/*
 * Finds a substitute size for ppemX and ppemY
 * This should only be used if there are no outlines in the font.
 */
static int FintBestSubstitute( blocClass * t, uint16 ppemX, uint16 ppemY )
{
	register int i, limit = (int)t->nTables;
	int errorX, errorY, error, bestI, smallestError;

	/* A simple heuristic :-) */
	bestI = -1;
	smallestError = 0x7fffffff;
	for ( i = 0; i < limit; i++ ) {
		errorX = t->table[i]->ppemX; errorX -= ppemX;
		if ( errorX < 0 ) errorX *= -4;
		errorY = t->table[i]->ppemY; errorY -= ppemY;
		if ( errorY < 0 ) errorY *= -4;
		error = errorX + errorY;
		if ( error < smallestError ) {
			smallestError  = error;
			bestI = i;
		}
	}
	return bestI; /*****/
}



/*
 * Returns NULL or a pointer to the bitmapSizeTable for this size.
 */
static bitmapSizeTable *FindBitmapSizeTable( blocClass *t, ebscClass *ebsc, uint16 ppemX, uint16 ppemY, sbitGlypInfoData *result )
{
	int i, limit = (int)t->nTables;
	bitmapSizeTable *bst = NULL;		/* Initialize */

	result->ppemX			= ppemX;
	result->ppemY			= ppemY;
	result->substitutePpemX	= ppemX;
	result->substitutePpemY = ppemY;
	/* Determine the strike */
	for (;;) {
		for ( i = 0; i < limit; i++ ) {
			if ( (uint16)t->table[i]->ppemX == ppemX && (uint16)t->table[i]->ppemY == ppemY ) {
				bst = t->table[i];
				break;/*****/
			}
		}
		if ( bst == NULL ) {
			if ( ebsc != NULL ) {
				/* See if we can scale another embedded bitmap to this size instead. */
				for ( i = 0; i < (int)ebsc->numSizes; i++ ) {
					if ( ebsc->table[i].ppemX == ppemX && ebsc->table[i].ppemY == ppemY ) {
						ppemX	= result->substitutePpemX = ebsc->table[i].substitutePpemX;
						ppemY	= result->substitutePpemY = ebsc->table[i].substitutePpemY;
						ebsc	= NULL;	/* Set to NULL, since we are done with this table */
						break; /*****/
					}
				}
				/* If we found a hit in the ebsc table then try again. */
				if ( ebsc == NULL ) continue; /*****/
			}
			/* Only do this if there are no outlines in the font. */
			if ( t->fontIsSbitOnly ) {
				if ((i = FintBestSubstitute(t, ppemX, ppemY)) >= 0 ) {
					result->substitutePpemX = (uint16)t->table[i]->ppemX;
					result->substitutePpemY = (uint16)t->table[i]->ppemY;
					bst = t->table[i];
				}
			}
		}
		break; /*****/
	}
	
	return bst; /*****/
}


/*
 * Rescales the value passed in to the new size.
 */
static int32 RescalePixelValue( int16 value, uint16 ppem, uint16 substitutePpem )
{
	if ( substitutePpem != ppem ) {
		int32 result = (value  * ppem + (substitutePpem>>1)) / substitutePpem;
		return result;	/*****/
	} else {
		return value;	/*****/
	}
}


/*
 * Returns scaled font wide sbit metrics
 */
void GetFontWideSbitMetrics( blocClass *bloc, ebscClass *ebsc, uint16 ppemX, uint16 ppemY,
							T2K_FontWideMetrics *hori, T2K_FontWideMetrics *vert )
{
	sbitGlypInfoData giData;
	bitmapSizeTable *bst;
	uint16 substitutePpemX, substitutePpemY;
	int i;

	bst = FindBitmapSizeTable( bloc, ebsc, ppemX, ppemY, &giData );
	if ( bst != NULL ) {
		substitutePpemX = giData.substitutePpemX;
		substitutePpemY = giData.substitutePpemY;
		
		hori->isValid	= true;
		hori->Ascender	= (int16)RescalePixelValue( (int8)bst->hori[0], ppemY, substitutePpemY );
		hori->Descender	= (int16)RescalePixelValue( (int8)bst->hori[1], ppemY, substitutePpemY );
		hori->LineGap	= 0;
		hori->maxAW		= (uint16)RescalePixelValue( bst->hori[2], ppemX, substitutePpemX );
		hori->caretDy	= (int16)RescalePixelValue( (int8)bst->hori[3], ppemX, substitutePpemX );
		hori->caretDx	= (int16)RescalePixelValue( (int8)bst->hori[4], ppemY, substitutePpemY );
		/* Scale up */
		for ( i = 0; i < 16; i++ ) {
			if ( hori->caretDx >= ONE16Dot16 || hori->caretDx <= -ONE16Dot16 ) break; /*****/
			if ( hori->caretDy >= ONE16Dot16 || hori->caretDy <= -ONE16Dot16 ) break; /*****/
			hori->caretDx <<= 1;
			hori->caretDy <<= 1;
		}
		
		vert->isValid	= true;
		vert->Ascender	= (int16)RescalePixelValue( (int8)bst->vert[0], ppemX, substitutePpemX );
		vert->Descender	= (int16)RescalePixelValue( (int8)bst->vert[1], ppemX, substitutePpemX );
		vert->LineGap	= 0;
		vert->maxAW		= (uint16)RescalePixelValue( bst->vert[2], ppemY, substitutePpemY );
		vert->caretDx	= (int16)RescalePixelValue( (int8)bst->vert[4], ppemX, substitutePpemX );
		vert->caretDy	= (int16)RescalePixelValue( (int8)bst->vert[3], ppemY, substitutePpemY );
		/* Scale up */
		for ( i = 0; i < 16; i++ ) {
			if ( vert->caretDx >= ONE16Dot16 || vert->caretDx <= -ONE16Dot16 ) break; /*****/
			if ( vert->caretDy >= ONE16Dot16 || vert->caretDy <= -ONE16Dot16 ) break; /*****/
			vert->caretDx <<= 1;
			vert->caretDy <<= 1;
		}
	} else {
		hori->isValid 	= false;
		vert->isValid 	= false;
	}
}

/*
 * Returns true if the glyph exists, and false otherwise
 * Caches some internal results in sbitGlypInfoData so that we can get to the bits faster when we need them next.
 */
int FindGlyph_blocClass( blocClass *t, ebscClass *ebsc, InputStream *in, uint16 glyphIndex, uint16 ppemX, uint16 ppemY, sbitGlypInfoData *result )
{
	int i;
	uint32 offsetA, offsetB;
	uint16 imageFormat, firstGlyphIndex;
	bitmapSizeTable *bst;
	
	assert( t != NULL );
	assert( result != NULL );
	
	
	bst = FindBitmapSizeTable( t, ebsc, ppemX, ppemY, result );
	
	offsetA = offsetB = 0;
	imageFormat = 0;
	
	/* See if we found a strike and if the glyphIndex is within the strike. */
	if ( bst != NULL && glyphIndex >= bst->startGlyphIndex && glyphIndex <= bst->endGlyphIndex ) {
		/* Search for the range containing the glyph, since there can be multiple ranges within one strike. */
		for ( i = 0; i < (int)bst->numberOfIndexSubTables; i++ ) {
			firstGlyphIndex = bst->table[i].firstGlyphIndex;
			if ( glyphIndex >= firstGlyphIndex && glyphIndex <= bst->table[i].lastGlyphIndex ) {
				/* Ok we found a range containing the glyphIndex! */
				uint16 indexFormat;
				uint32 imageDataOffset, dataOffset;
				
				result->bitDepth	= bst->bitDepth;
				result->flags		= bst->flags;

				Seek_InputStream( in, t->startOffset + bst->indexSubTableArrayOffset + bst->table[i].additionalOffsetToIndexSubTable );
				/* Here we have the indexSubHeader data */
				/* All 5 formats start with the same header. */
				indexFormat 		= (uint16)ReadInt16( in );	/* Format of this indexSubTable. */
				imageFormat			= (uint16)ReadInt16( in );	/* Format of the EBDT/bdat data. */
				imageDataOffset		= (uint32)ReadInt32( in );	/* Offset to the image data in the EBDT/bdat table. */
				/* End indexSubHeader data. */
				
				dataOffset = Tell_InputStream( in );
				switch ( indexFormat ) {
				case 1: 
					/* Variable metrics, 4 byte offsets to image. */
					Seek_InputStream( in, dataOffset + (glyphIndex-firstGlyphIndex) * sizeof(uint32) );
					offsetA = imageDataOffset + (uint32)ReadInt32( in );
					offsetB = imageDataOffset + (uint32)ReadInt32( in );
					break; /*****/
				case 2: 
					/* Constant metrics, constant image size. */
					offsetB = (uint32)ReadInt32( in ); /* == imageSize */
					offsetA = imageDataOffset + (glyphIndex-firstGlyphIndex) * offsetB;
					offsetB = offsetA + offsetB;
					ReadBigMetrics( &(result->bigM), in );
					break; /*****/
				case 3: 
					/* Variable metrics, 2 byte offsets to image. */
					Seek_InputStream( in, dataOffset + (glyphIndex-firstGlyphIndex) * sizeof(uint16) );
					offsetA = imageDataOffset + (uint16)ReadInt16( in );
					offsetB = imageDataOffset + (uint16)ReadInt16( in );
					break; /*****/
				case 4:
					/* Variable metrics, sparse glyph codes, 2 byte offsets to image. */
					{
						uint32 numGlyphs, index;
						uint16 glyphCode, offset;
						
						numGlyphs = (uint32)ReadInt32( in );
						for ( index = 0; index < numGlyphs; index++ ) {
							glyphCode	= (uint16)ReadInt16( in );
							offset		= (uint16)ReadInt16( in );
							if ( glyphCode == glyphIndex ) {
								offsetA = imageDataOffset + offset;
								Seek_InputStream(in, in->pos + 2L);		/* Skip glyphCode. */
								offset		= (uint16)ReadInt16( in );  /* Read the next offset. */
								offsetB = imageDataOffset + offset;
								break; /*****/
							}
						}
					
					}
					break; /*****/
				case 5: 
					/* Constant metrics, sparse glyph codes, constant image size. */
					{
						uint32 imageSize, numGlyphs, index;
						uint16 glyphCode;
						
						imageSize = (uint32)ReadInt32( in ); /* == imageSize */
						ReadBigMetrics( &(result->bigM), in);
						numGlyphs = (uint32)ReadInt32( in ); /* == numGlyphs */
						for ( index = 0; index < numGlyphs; index++ ) {
							glyphCode	= (uint16)ReadInt16( in );
							if ( glyphCode == glyphIndex ) {
								offsetA = imageDataOffset + index * imageSize;
								offsetB = offsetA + imageSize;
								break; /*****/
							}
						}
					
					}
					break; /*****/
				} /* switch */
				break; /*****/  /* Break out of surrounding for loop since we are done! */
			} /* if */
		} /* for */
	} /* if */
	result->offsetA		= offsetA;
	result->offsetB		= offsetB;
	result->imageFormat	= imageFormat;
	result->glyphIndex	= glyphIndex;
	return offsetA != 0; /*****/
}

#define MY_BLACK_VALUE 126

/*
 * Reads a bitmap from the input stream.
 */
static uint8 *CreateBitMap( tsiMemObject *mem, InputStream *in, sbitGlypInfoData *gInfo, int width, int height, int bitDepth, uint8 greyScaleLevel, int byteAligned, int32 *rowBytesPtr )
{
	register uint32 N, rowBytes;
	register uint8 *baseAddr, *row;
	int x,y, bitCount, bitBuffer;
	
	
	if ( greyScaleLevel == 0 ) {
#ifdef MAKE_SC_ROWBYTES_A_4BYTE_MULTIPLE
			rowBytes = (width+31) / 32;
			rowBytes <<= 2;
#else
			rowBytes = (uint32)((width+7) / 8);
#endif
	} else {
#ifdef MAKE_SC_ROWBYTES_A_4BYTE_MULTIPLE
			rowBytes = (width+3)/4;
			rowBytes <<= 2;
#else
			rowBytes = (uint32)width;
#endif
	}

	N 			= rowBytes * height;
	
	baseAddr = row = (unsigned char*) tsi_AllocMem( mem, N * sizeof( unsigned char ) );
	gInfo->N = N;	
	bitCount 	= 0;
	bitBuffer 	= 0;
	if ( greyScaleLevel > 0 ) {
		if ( bitDepth == 1 ) {
			for ( y = 0; y < height; y++ ) {
				for ( x = 0; x < width; x++ ) {
					if ( bitCount-- == 0 ) {
						bitBuffer	= ReadUnsignedByteMacro( in );
						bitCount	= 7;
					}
					bitBuffer += bitBuffer; /* same as bitBuffer <= 1 */
					row[x] 	= (uint8)(bitBuffer & 0x100 ? MY_BLACK_VALUE : 0);
				}
				row += rowBytes;					/* Go down one scanline/row */
				if ( byteAligned ) bitCount = 0; 	/* flush out remaining pad bits */
			}
		} else {
			int depth, value, maxValue, maxValueDiv2;
			
			maxValue 		= (1 << bitDepth) - 1;
			maxValueDiv2	= maxValue >> 1;
			for ( y = 0; y < height; y++ ) {
				for ( x = 0; x < width; x++ ) {
					value = 0;
					for ( depth = 0; depth < bitDepth; depth++ ) {
						if ( bitCount-- == 0 ) {
							bitBuffer	= ReadUnsignedByteMacro( in );
							bitCount	= 7;
						}
						bitBuffer	+= bitBuffer; /* same as bitBuffer <= 1 */
						value		+= value;
						if ( bitBuffer & 0x100 ) value++;
					}
					value		= (value * MY_BLACK_VALUE + maxValueDiv2) / maxValue;		/* Normalize value to the range [0,MY_BLACK_VALUE] */
					row[x]		= (uint8)value;
				}
				row += rowBytes;					/* Go down one scanline/row */
				if ( byteAligned ) bitCount = 0;	/* flush out remaining pad bits */
			}
		}
	} else {
		register uint8 outByte; /* Cache in a register to minimize memory read and writes. */
		if ( bitDepth == 1 ) {
			for ( y = 0; y < height; y++ ) {
				outByte = 0;						/* Initialize to zero. */
				for ( x = 0; x < width; x++ ) {
					if ( bitCount-- == 0 ) {
						bitBuffer = ReadUnsignedByteMacro( in );
						bitCount = 7;
					}
					bitBuffer += bitBuffer; 		/* Same as bitBuffer <= 1 */
					if ( bitBuffer & 0x100 ) {
						outByte = (uint8)(outByte | (0x80 >> ((uint8)x&07)));
					}
					if ( (x & 0x07) == 0x07 ) {
						row[x>>3] = outByte; 		/* Write out the outByte, since we now have 8 valid bits. */
						outByte = 0;
					}
				}
				if ( (x & 0x07) != 0 ) {
					row[x>>3] = outByte;			/* Write out the outByte, if there are bits remaining. */
				}
				row += rowBytes;					/* Go down one scanline/row */
				if ( byteAligned ) bitCount = 0;	/* flush out remaining pad bits */
			}
		} else {
			int depth, value, maxValue, maxValueDiv2;
			
			maxValue 		= (1 << bitDepth) - 1;
			maxValueDiv2	= maxValue >> 1;
			for ( y = 0; y < height; y++ ) {
				outByte = 0;
				for ( x = 0; x < width; x++ ) {
					value = 0;						/* Initialize to zero. */
					for ( depth = 0; depth < bitDepth; depth++ ) {
						if ( bitCount-- == 0 ) {
							bitBuffer	= ReadUnsignedByteMacro( in );
							bitCount	= 7;
						}
						bitBuffer	+= bitBuffer; 	/* Same as bitBuffer <= 1 */
						value		+= value;
						if ( bitBuffer & 0x100 ) value++;
					}
					if ( value >= maxValueDiv2 ) {
						outByte = (uint8)(outByte | (0x80 >> (x&7)));
					}
					if ( (x & 0x07) == 0x07 ) {
						row[x>>3] = outByte; 		/* Write out the outByte, since we now have 8 valid bits. */
						outByte = 0;
					}
				}
				if ( (x & 0x07) != 0 ) {
					row[x>>3] = outByte;			/* Write out the outByte, if there are bits remaining. */
				}
				row += rowBytes;					/* Go down one scanline/row */
				if ( byteAligned ) bitCount = 0;	/* flush out remaining pad bits */
			}
		}
	}
	*rowBytesPtr = (int32)rowBytes;
	return baseAddr; /*****/
}


extern void  tsi_ValidateMemory( register tsiMemObject *t );

/*
 * Or bitmap-2 into bitmap-1.
 * bitmap1 = bitmap1 OR bitmap2;
 */
static void MergeBits( sbitGlypInfoData *bit1, sbitGlypInfoData *bit2, uint8 xOffset2, uint8 yOffset2, uint8 greyScaleLevel  )
{
	register int x2, x1;
	register uint8 *row1	= bit1->baseAddr;
	register uint8 *end1;
	register uint8 *row2	= bit2->baseAddr;
	register int width1		= bit1->bigM.width;
	register int width2		= bit2->bigM.width;
	int32 rowBytes1 		= bit1->rowBytes;
	int32 rowBytes2 		= bit2->rowBytes;
	int	y;
	
	if ( row1 == NULL || row2 == NULL )	return;			/****** Bail out ******/
	end1 = row1 + bit1->bigM.height * rowBytes1;
	row1 += rowBytes1 * yOffset2;						/* Apply yOffset2 */
	if ( greyScaleLevel > 0 ) {
		/* Gray scale bitmap */
		for ( y = bit2->bigM.height; y > 0; y-- ) {		/* Equivalent to for ( y = 0; y < height; y++ ), since y is not used inside the loop. */
			for ( x2 = 0, x1 = xOffset2; x2 < width2 && x1 < width1; x2++, x1++ ) {
				/* Not clear what is best, so do a fuzzy logic OR which is the same as a numeric max operator */
				register uint8 v2 = row2[x2];			/* Read the value from bitmap2. */
				if ( v2 != 0 && v2 > row1[x1] ) {		/* See if the value greater than the value in bitmap 1. */
					row1[x1] = v2;						/* If so, then transfer to bitmap 1. */
				}
			}
			row1 += rowBytes1;							/* Go down one scanline/row */
			row2 += rowBytes2;
			if ( row1 >= end1 ) break; /***** out of bounds ******/
		}
	} else {
		register int bitBuffer = 0;						/* Cache input bits here to gain speed */
		/* Black and white bitmap */
		for ( y = bit2->bigM.height; y > 0; y-- ) {		/* Equivalent to for ( y = 0; y < height; y++ ), since y is not used inside the loop. */
			for ( x2 = 0, x1 = xOffset2; x2 < width2 && x1 < width1; x2++, x1++ ) {
				if ( (x2 & 7) == 0 ) {
					bitBuffer = row2[x2>>3];
				}
				bitBuffer += bitBuffer; 		/* Same as bitBuffer <= 1 */
				if ( bitBuffer & 0x100 ) {      /* Same as if ( row2[x2>>3] & (0x80 >> (x2&7)) ) */
					row1[x1>>3] = (uint8)(row1[x1>>3] | (0x80 >> (x1&7)));	/* Set the bit in bitmap 1 */
				}
			}
			row1 += rowBytes1;							/* Go down one scanline/row */
			row2 += rowBytes2;
			if ( row1 >= end1 ) break; /***** out of bounds ******/
		}
	}
}

/*
 * Scales the bitmap in the y direction.
 */
static void ScaleYBits( uint8 *baseAddr1, uint8* baseAddr2, int32 height1, int32 height2, int32 rowBytes )
{
	register uint8 *row1 = baseAddr1;
	register uint8 *row2 = baseAddr2;
	register int32 i, pos1, pos2, totalDistance;
	
	/* We note that height2 * height1 == height * height2 == total distance */
	totalDistance = height2 * height1;
	/* We can think of pos1 and pos2 as positions in bitmap 1 and 2 */
	pos2 = height1>>1;	/* Start at a pixel center */
	/* pos1 = height2>>1; since we ensure pos1 >= pos2 below, this would result in a positional error between [0,height2] */
	pos1 = height2; /* To center the positional error for pos1 add 1/2 pixel bias so error is beween [-height2/2,height2/2] */
	/* We step exactly on pixel centers in the destination bitmap, so it's positional error is zero */
	if ( height2 > height1 ) {
		/* We will increase the number scanlines. */
		while ( pos2 < totalDistance ) {
			if ( pos1 < pos2 ) {
				pos1 += height2;
				row1 += rowBytes;
			}
			/* Copy row1 to row2 */
			for ( i = 0; i < rowBytes; i++ ) row2[i] = row1[i];
			pos2 += height1;			/* Go down one scanline/row, in the output bitmap (2) */
			row2 += rowBytes;
		}
	} else {
		/* We will decrease the number of scanlines. */
		while ( pos2 < totalDistance ) {
			while ( pos1 < pos2 ) {
				pos1 += height2;
				row1 += rowBytes;
			}
			/* Copy row1 to row2 */
			for ( i = 0; i < rowBytes; i++ ) row2[i] = row1[i];
			pos2 += height1;			/* Go down one scanline/row, in the output bitmap (2) */
			row2 += rowBytes;
		}
	}
}


/*
 * Scales the bitmap in the x direction. (this is more tricky than the y direction)
 */
static void ScaleXBits( uint8 *baseAddr1, uint8* baseAddr2, int32 height, int32 width1, int32 width2,
					int32 rowBytes1, int32 rowBytes2, uint8 greyScaleLevel )
{
	register uint8 *row1 = baseAddr1;
	register uint8 *row2 = baseAddr2;
	register int32 pos1, pos2;
	int32 x1, x2, y;

	/* We note that width2 * width1 == width * width2 == total distance */
	for ( y = 0; y < height; y++ ) {
		/* We can think of pos1 and pos2 as positions in bitmap 1 and 2 */
		pos2 = width1>>1;	/* Start at a pixel center */
		/* pos1 = width2>>1; since we ensure pos1 >= pos2 below, this would result in a positional error between [0,width2[ */
		pos1 = width2; /* To center the positional error for pos1 add 1/2 pixel bias so error is beween [-width2/2,width2/2[ */
		/* We step exactly on pixel centers in the destination bitmap, so it's positional error is zero */
		/* We will increase or decrease the number of pixels. */
		if ( greyScaleLevel > 0 ) {
			/* Gray scale */
			for ( x1 = x2 = 0; x2 < width2; ) {
				while ( pos1 < pos2 ) {
					x1++;
					pos1 += width2;
				}
				/* Copy pixel from x1 to x2 */
				row2[x2] = row1[x1];
				x2++;
				pos2 += width1;
			}
		} else {
			/* Monochrome */
			register uint8 outByte = 0;			/* Use a register to minimize memory reads and writes. */
			register uint8 inByte = row1[0];	/* Use a register to minimize memory reads. */

			for ( x1 = x2 = 0; x2 < width2; ) {
				while ( pos1 < pos2 ) {
					x1++;
					pos1 += width2;
					inByte = (uint8)(inByte + inByte); 			/* Shift left by one */
					if ( (x1 & 0x07) == 0 ) {
						inByte = row1[x1>>3];
					}
				}
				/* Copy pixel from x1 to x2 */
				if ( inByte & 0x80 ) {
					outByte = (uint8)(outByte | (0x80 >> (x2&7)));
				}
				if ( (x2 & 0x07) == 0x07 ) {
					row2[x2>>3] = outByte;		/* Write it out, since we now have 8 valid bits */
					outByte = 0;
				}
				x2++;
				pos2 += width1;
			}
			if ( (x2 & 0x07) != 0 ) {
				row2[(x2-1)>>3] = outByte;		/* Write it out if there are bits left. */
			}
		}
		row1 += rowBytes1;						/* Go to the next scanline. */
		row2 += rowBytes2;
	}
}


/*
 * Scales the bitmap and metrics in the X and Y direction.
 */
static void ScaleBits( tsiMemObject *mem, sbitGlypInfoData *bits, uint8 greyScaleLevel )
{
	int32 width2, height2;
	uint8 *baseAddr2;
	uint16 substitutePpemX = bits->substitutePpemX;
	uint16 substitutePpemY = bits->substitutePpemY;
	uint16 ppemX	= bits->ppemX;
	uint16 ppemY	= bits->ppemY;
	int32 width1	= bits->bigM.width;
	int32 height1	= bits->bigM.height;
	int yOrder, xOrder, n;
	
	width2 	= (width1  * ppemX + (substitutePpemX>>1)) / substitutePpemX;
	height2	= (height1 * ppemY + (substitutePpemY>>1)) / substitutePpemY;
	
	/* The following logic ensures that we do x/y scaling in fastest order,
	 * since x scaling is slower than y scaling.
	 */
	yOrder = -1;	/* Initialize */
	xOrder = 0;
	if ( height2 > height1 ) {
		yOrder = 1; /* If we increase the number of scanlines do y second. */
		/* xOrder == 0, Less work if x is done first. */
	} else if ( height2 < height1 ) {
		yOrder = 0; /* Do y first since we decrease the number of scanlines. */
		xOrder = 1;	/* Less work if x is done second. */
	}
	if ( width1 == width2 ) xOrder = -1; /* Only scale x if needed. */
	
	for ( n = 0; n < 2; n++ ) {
		if ( yOrder == n ) {
			/* Y scale */
			baseAddr2 = (uint8 *)tsi_AllocMem( mem, (size_t)(height2 * bits->rowBytes) );
			ScaleYBits( bits->baseAddr, baseAddr2, height1, height2, bits->rowBytes );
			tsi_DeAllocMem( mem, bits->baseAddr );
			bits->baseAddr 			= baseAddr2;
			bits->bigM.height 		= (uint16)height2;
			bits->bigM.horiBearingY = ( int16)(( bits->bigM.horiBearingY * ppemY + (substitutePpemY>>1)) / substitutePpemY);
			bits->bigM.vertBearingY = ( int16)(( bits->bigM.vertBearingY * ppemY + (substitutePpemY>>1)) / substitutePpemY);
			bits->bigM.vertAdvance 	= (uint16)(( bits->bigM.vertAdvance  * ppemY + (substitutePpemY>>1)) / substitutePpemY);
		} else if ( xOrder == n ) {
			/* X scale */
			int32 rowBytes2;
			if ( greyScaleLevel == 0 ) {
#ifdef MAKE_SC_ROWBYTES_A_4BYTE_MULTIPLE
					rowBytes2 = (width2+31) / 32;
					rowBytes2 <<= 2;
#else
					rowBytes2 = (width2+7) / 8;
#endif
			} else {
#ifdef MAKE_SC_ROWBYTES_A_4BYTE_MULTIPLE
					rowBytes2 = (width2+3)/4;
					rowBytes2 <<= 2;
#else
					rowBytes2 = width2;
#endif
			}
			baseAddr2 = (uint8 *)tsi_AllocMem( mem, (size_t)(rowBytes2 * height2) );
			ScaleXBits( bits->baseAddr, baseAddr2, height2, width1, width2, bits->rowBytes, rowBytes2, greyScaleLevel );
			tsi_DeAllocMem( mem, bits->baseAddr );
			bits->baseAddr 			= baseAddr2;
			bits->rowBytes 			= rowBytes2;
			bits->bigM.width 		= (uint16)width2;
			bits->bigM.horiBearingX = ( int16)(( bits->bigM.horiBearingX * ppemX + (substitutePpemX>>1)) / substitutePpemX);
			bits->bigM.vertBearingX = ( int16)(( bits->bigM.vertBearingX * ppemX + (substitutePpemX>>1)) / substitutePpemX);
			bits->bigM.horiAdvance 	= (uint16)(( bits->bigM.horiAdvance  * ppemX + (substitutePpemX>>1)) / substitutePpemX);
		}
	}
}


/* T2K internal local data type used for composite bitmaps. */
typedef struct {
	uint16 	glyphIndex;
	uint8 	xOffset, yOffset;
	sbitGlypInfoData gInfo;
} compositeGlyphDescriptor;

/*
 * Gets the bits.
 * The function is recursive.
 * sets: baseAddr, rowBytes, bigM
 * needs: GIndex -> offsetA, imageFormat, bitDepth, bigM, (mem)
 */
void ExtractBitMap_blocClass( blocClass *t, ebscClass *ebsc, sbitGlypInfoData *gInfo, InputStream *in, uint32 bdatOffset, uint8 greyScaleLevel, int recursionLevel )
{
	int i, numComponents;
	int32 rowBytes;
	uint32 N;
	compositeGlyphDescriptor *comp;
	uint8 *baseAddr	= NULL;
	
	rowBytes	= 0;
	Seek_InputStream( in, bdatOffset +  gInfo->offsetA );
	gInfo->smallMetricsUsed = false;

	switch( gInfo->imageFormat ) {
	case 1:
		/* Small metrics, byte-aligned data (a row is an integer number of bytes) */
		ReadSmallMetrics( &(gInfo->bigM), in );
		gInfo->smallMetricsUsed = true;
		/* Here we have the image data */
		baseAddr = CreateBitMap( t->mem, in, gInfo, gInfo->bigM.width, gInfo->bigM.height, gInfo->bitDepth, greyScaleLevel, true, &rowBytes );
		break; /*****/
	case 2:
		/* Small metrics, bit-aligned data (rows are bit-aligned) */
		ReadSmallMetrics( &(gInfo->bigM), in );
		gInfo->smallMetricsUsed = true;
		/* Here we have the image data */
		baseAddr = CreateBitMap( t->mem, in, gInfo, gInfo->bigM.width, gInfo->bigM.height, gInfo->bitDepth, greyScaleLevel, false, &rowBytes );
		break; /*****/
	/* Format 3 is obsolete */
	/* Format 4 is not supported, a private Apple format used in a few Apple fonts */
	case 5:
		/* metrics in EBLC/bloc, bit-aligned image data */
		/* Here we have the image data */
		baseAddr = CreateBitMap( t->mem, in, gInfo, gInfo->bigM.width, gInfo->bigM.height, gInfo->bitDepth, greyScaleLevel, false, &rowBytes );
		break; /*****/
	case 6:
		/* Big metrics, byte-aligned data (a row is an integer number of bytes) */
		ReadBigMetrics( &(gInfo->bigM), in );
		/* Here we have the image data */
		baseAddr = CreateBitMap( t->mem, in, gInfo, gInfo->bigM.width, gInfo->bigM.height, gInfo->bitDepth, greyScaleLevel, true, &rowBytes );
		break; /*****/
	case 7:
		/* Big metrics, bit-aligned data (rows are bit-aligned) */
		ReadBigMetrics( &(gInfo->bigM), in );
		/* Here we have the image data */
		baseAddr = CreateBitMap( t->mem, in, gInfo, gInfo->bigM.width, gInfo->bigM.height, gInfo->bitDepth, greyScaleLevel, false, &rowBytes );
		break; /*****/
		
	case 8:
		/* Small metrics, component data */
		ReadSmallMetrics( &(gInfo->bigM), in );
		gInfo->smallMetricsUsed = true;
		Seek_InputStream(in, in->pos + 1L);		/* skip pad byte */
		goto readNumComponents; /*****/
	case 9:
		/* Big metrics, component data */
		ReadBigMetrics( &(gInfo->bigM), in );
		/* fall through */
		
readNumComponents:
		/* Joint code path for case 8 and 9. */
		/* Here we have the component data. */
		numComponents = (uint16)ReadInt16( in );
		comp = (compositeGlyphDescriptor *) tsi_AllocMem( t->mem, numComponents * sizeof( compositeGlyphDescriptor ) );
		for ( i = 0; i < numComponents; i++ ) {
			comp[i].glyphIndex 	= (uint16)ReadInt16( in );
			comp[i].xOffset		= ReadUnsignedByteMacro( in ); /* The documentation claims these are signed but that seems bogus... ---Sampo */
			comp[i].yOffset		= ReadUnsignedByteMacro( in );
		}
		/* Do FindGlyph_blocClass() after the above loop so that we do not have to skip back and forth in the input stream */
		for ( i = 0; i < numComponents; i++ ) {
			FindGlyph_blocClass( t, ebsc, in, comp[i].glyphIndex, gInfo->substitutePpemX, gInfo->substitutePpemY, &(comp[i].gInfo) );
		}
		if ( greyScaleLevel == 0 ) {
#ifdef MAKE_SC_ROWBYTES_A_4BYTE_MULTIPLE
				rowBytes = (gInfo->bigM.width+31) / 32;
				rowBytes <<= 2;
#else
				rowBytes = (gInfo->bigM.width+7) / 8;
#endif
		} else {
#ifdef MAKE_SC_ROWBYTES_A_4BYTE_MULTIPLE
				rowBytes = (gInfo->bigM.width+3)/4;
				rowBytes <<= 2;
#else
				rowBytes = gInfo->bigM.width;
#endif
		}

		N			= (uint32)(rowBytes * gInfo->bigM.height);
	
		baseAddr = (unsigned char*) tsi_AllocMem( t->mem, N * sizeof( unsigned char ) );
		gInfo->N = N;

		for ( i = 0; i < (int)N; i++ ) baseAddr[i] = 0;	/* Clear it */
		gInfo->baseAddr = baseAddr;
		gInfo->rowBytes = rowBytes;
		if ( ++recursionLevel <= MAX_SBIT_RECURSION_DEPTH ) {
			for ( i = 0; i < numComponents; i++ ) {
				ExtractBitMap_blocClass( t, ebsc, &(comp[i].gInfo), in, bdatOffset, greyScaleLevel, recursionLevel );
				/* Combine this data with the gInfo bitmap */
				MergeBits( gInfo, &(comp[i].gInfo), comp[i].xOffset, comp[i].yOffset, greyScaleLevel );
				/* Free up this bitmap */
				tsi_DeAllocMem( t->mem, comp[i].gInfo.baseAddr ); comp[i].gInfo.baseAddr = NULL;
			}
		}
		tsi_DeAllocMem( t->mem, comp );
		break; /*****/
	}
	gInfo->baseAddr = baseAddr;
	gInfo->rowBytes = rowBytes;

	if ( gInfo->substitutePpemX != gInfo->ppemX || gInfo->substitutePpemY != gInfo->ppemY ) {
		ScaleBits( t->mem, gInfo, greyScaleLevel );
	}
}


#endif /* ENABLE_SBIT */


/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: R:/src/FontFusion/Source/Core/rcs/t2ksbit.c 1.12 2001/05/03 20:47:00 reggers Exp $
 *                                                                           *
 *     $Log: t2ksbit.c $
 *     Revision 1.12  2001/05/03 20:47:00  reggers
 *     Warning suppression.
 *     Revision 1.11  2001/05/03 15:38:34  reggers
 *     Casts for warning suppression.
 *     Revision 1.10  2000/10/26 17:00:50  reggers
 *     Changes for SBIT: use cache based on setting of
 *     scaler->okForBitCreationToTalkToCache. Able to handle caching
 *     SBITs now.
 *     Revision 1.9  2000/05/18 15:37:27  reggers
 *     Silence STRICT Borland warnings.
 *     Revision 1.8  2000/05/11 15:13:11  reggers
 *     SBIT rowBytes a 4 byte multiple implemented.
 *     Revision 1.7  2000/02/25 17:46:12  reggers
 *     STRICT warning cleanup.
 *     Revision 1.6  1999/12/23 22:03:07  reggers
 *     New ENABLE_PCL branches. Rename any 'code' and 'data' symbols.
 *     Revision 1.5  1999/10/19 12:14:55  jfatal
 *     Typo, forgot to change one letter in the include file name to lower case
 *     Revision 1.4  1999/10/18 17:01:40  jfatal
 *     Changed all include file names to lower case.
 *     Revision 1.3  1999/09/30 15:12:13  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.2  1999/05/17 15:58:19  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/

