/*
 * TRUETYPE.C
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
#include "truetype.h"
#include "orion.h"
#include "util.h"
#include "autogrid.h"
#include "ghints.h"

#ifdef ENABLE_T2KS
#include "t2kstrk1.h"
    #ifdef ENABLE_WRITE
        #include "cjkdata.h"
    #endif
#endif


/***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/
/*
 *	for the flags field
 */
#define Y_POS_SPECS_BASELINE		0x0001
#define X_POS_SPECS_LSB				0x0002
#define HINTS_USE_POINTSIZE			0x0004
#define USE_INTEGER_SCALING			0x0008

#define SFNT_MAGIC 					0x5F0F3CF5

#define SHORT_INDEX_TO_LOC_FORMAT	0
#define LONG_INDEX_TO_LOC_FORMAT	1
#define GLYPH_DATA_FORMAT			0


static int16 Read2B(void *p);


/***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/
void setT2KScaleFactors( long pixelsPerEm, long UPEM, T2KScaleInfo *si )
{
    int32 nScale;
    int16 dShift;
    int32 dScale;
    int32 dScaleDiv2;


    nScale = pixelsPerEm << 6;
    dScale = UPEM;

    while ( ((dScale | nScale) & 1) == 0 ) {
        nScale >>= 1;
        dScale >>= 1;
    }
    dScaleDiv2 = dScale>>1;
    si->scaleType  = T2K_FIXMUL;
    si->fixedScale = util_FixDiv( nScale, dScale );
    if ( -32768 < nScale && nScale < 32768 ) {
        si->nScale = (int16)nScale;
        for ( dShift = 0; ((dScale>>dShift) & 1) == 0; ) {
            dShift++;
        }
        if ( (dScale>>dShift) == 1 ) {
            si->scaleType = T2K_IMULSHIFT;
            assert( (1 << dShift) == dScale );
        } else {
			si->scaleType 	= T2K_IMULDIV;
		}
		si->dShift 		= dShift;
		si->dScale		= dScale;
		si->dScaleDiv2 	= dScaleDiv2;
    }
}

/***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/

static int IsFigure_cmapClass( cmapClass *t, uint16 gIndex )
{
    register int i;

    if ( t != NULL ) {
        for ( i = 0; i < NUM_FIGURES; i++ ) {
            if ( t->figIndex[i] == gIndex ) {
                return true; /*****/
            }
        }
    }
    return false; /*****/
}
#ifdef ENABLE_T2KS
/* 0xe994 == (3 << 14 ) | (2 << 12 ) | (2 << 10 ) | (1 <<  8 ) | 2 <<  6 ) | (1 <<  4 ) | (1 <<  2 ) | (0 <<  0 ); */
#define COUNT_LOW_4BITS(x)  ((( 0xe994 >> ( (x) & 0x0e )) & 3 ) + ((x)&1))
/*
 * Format 9999: Compact fixed size gap independent Unicode encoding table
 */
static uint16 Compute_cmapClass_Index9999( cmapClass *t, uint16 charCode )
{
    register int i, j, glyphCount;
    register uint16 index = 0, baseIndex;
    register uint8 *charP, *ptr, highByte = (uint8)(charCode>>8), lowByte = (uint8)(charCode & 0x00ff), x;

    charP = t->cmapData + t->platform[t->preferedEncodingTable]->offset; /* point at subTable */
	charP += 6; 										/* skip format, length, and version */

    ptr = charP + highByte * sizeof(uint16);
    assert ( sizeof(uint16) == 2 );

    baseIndex = *ptr++;
    baseIndex <<= 8;
    baseIndex |= *ptr;

    charP += 256 * sizeof(uint16);

    charP += highByte * 256/8;
    j = lowByte >> 3;
    ptr = charP + j;
    if ( *ptr & (1 << (lowByte&7)) ) {
        glyphCount = 0;
        for ( i = 0; i< j; i++ ) {
            x = *charP++;
            glyphCount += COUNT_LOW_4BITS(x);
            x >>= 4;
            glyphCount += COUNT_LOW_4BITS(x);
        }
        assert( ptr == charP );
        x = *charP;
        j = lowByte&7;
        for ( i = 0; i <= j; i++ ) {
            glyphCount += x & 1;
            x >>= 1;
            /* if ( x & (1 << i) ) glyphCount++; */
        }
        index = (uint16)(baseIndex + glyphCount);
    } else {
        ;
    }

	
	return index; /*****/
}
#endif /* ENABLE_T2KS */

/*
 * Format 0: Byte Encoding table
 */
static uint16 Compute_cmapClass_Index0( cmapClass *t, uint16 charCode )
{
    register uint16 index = 0;
    register uint8 *charP;
	if ( charCode < 256 ) {
		charP = t->cmapData + t->platform[t->preferedEncodingTable]->offset; /* point at subTable */
		charP += 6; 										/* skip format, length, and version */

        index = charP[charCode];
    }
#ifdef USE_SEAT_BELTS
    /* Only valid indexes */
    if (index <  t->numGlyphs) {
        return index;
    } else {
        return 0;
    }
#else 
    return index;
#endif
}

/*
 * Format 2: High Byte Mapping Through Encoding table
 */

/*
 *    Char2Index structures, including platform IDs
 */

typedef struct {
    uint16    format;
    uint16    length;
    uint16    version;
} sfnt_mappingTable;

typedef struct {
    uint16    firstCode;
    uint16    entryCount;
    int16     idDelta;
    uint16    idRangeOffset;
} sfnt_subHeader;

typedef struct {
    uint16         subHeadersKeys[256];
    sfnt_subHeader subHeaders[1];
} sfnt_mappingTable2;



static uint16 Compute_cmapClass_Index2( cmapClass *t, uint16 charCode )
{
    register uint16 index = 0;
    register uint8 *charP;

    uint16              mapMe;
    uint16              highByte;
    uint16              lowByte;
    sfnt_mappingTable2 *Table2;
    sfnt_subHeader     *subHeader;

    /* point at subTable */
    charP = (uint8 *)t->cmapData + t->platform[t->preferedEncodingTable]->offset;
    /* skip format, length, and version */
    charP += 6;

    highByte = (uint16)(charCode >> 8);
    lowByte  = (uint16)(charCode & 0x00ff);
    mapMe    = lowByte;

    /* Point to mapping table */
    Table2 = (sfnt_mappingTable2 *)charP;

    /* Get appropriate subHeader based on high byte */
    subHeader = (sfnt_subHeader *) ((char  *) &Table2->subHeaders + Read2B(&Table2->subHeadersKeys [highByte]));

    /* out of range */
    if (lowByte < Read2B(&subHeader->firstCode) )
        return 0;

    /* single byte character? */
    if (Table2->subHeadersKeys[highByte] == 0)
	{
		/* indicate one-byte character code */
		t->bytesConsumed = 1;

		/* check for two-byte specification */
		if (charCode > 0xff)
			return 0;
	}

    /* adjust for range */
    mapMe -= Read2B(&subHeader->firstCode);

    /* check if within range */
    if ( mapMe < Read2B(&subHeader->entryCount) )
    {
        uint16 *shortP;


        /* Set the idRangeOffset address */
        shortP = (uint16 *)&(subHeader->idRangeOffset);
        shortP = (uint16 *)((long)shortP + Read2B(shortP) + mapMe + mapMe);

        if (*shortP)
        {
            /* Get glyph index */
            index = (uint16)(Read2B(shortP) + Read2B(&subHeader->idDelta));
        }
    }


#ifdef USE_SEAT_BELTS
    /* Only valid indexes */
    if (index <  t->numGlyphs) {
        return index;
    } else {
        return 0;
    }
#else 
    return index;
#endif
}

/*
 * Format 4: Segment mapping to delta values
 */
static uint16 Compute_cmapClass_Index4( cmapClass *t, register uint16 charCode )
{
    register uint16 segCountX2, offset, idDelta, index, uTmp;
    register uint8 *charP;

    charP = t->cmapData + t->platform[t->preferedEncodingTable]->offset; /* point at subTable */
	charP += 6; 										/* skip format, length, and version */

    segCountX2 = *charP++; segCountX2 <<= 8; segCountX2 |= *charP++;
	charP += 6;											/* skip searchRange, entrySelector, and rangeShift */

    /* Start with a simple binary search to get close, if the table is "large" */
    if ( segCountX2 > 14 ) {
        register uint16 n0 = 0;
        register uint16 n1, n2 = (uint16)(segCountX2 - 2);

        do {
            n1 = (uint16)(((n0+n2) >> 1) & ~1);
            uTmp = charP[n1]; uTmp <<= 8; uTmp |= charP[n1+1];
            if ( charCode > uTmp ) {
                n0 = (uint16)(n1+2);
            } else {
                n2 = n1;
            }
        } while ( (n2 - n0) > 12 );
        charP += n0;  /* Fall through to the linear search below when we are close. */
    }
    /* Do a linear search. */
    /* go one too far, so we skip the reserved word */
    do {
        uTmp = *charP++; uTmp <<= 8; uTmp |= *charP++;
    } while ( charCode > uTmp );
    /* charP now points at endCount[n+1] */

    charP += segCountX2; /* point at startCount[n] */
    uTmp = *charP; uTmp <<= 8; uTmp |= *(charP+1);
    index = 0;
    if ( charCode >= uTmp ) {
        offset  = (unsigned short)(charCode - uTmp);
		charP += segCountX2; 	/* point at idDelta[n] */
        idDelta = *charP; idDelta <<= 8; idDelta |= *(charP+1);
		charP += segCountX2; 	/* point at idRangeOffset[n] */
        uTmp = *charP; uTmp <<= 8; uTmp |= *(charP+1);
        if ( uTmp == 0 ) {
			index	= (unsigned short)(charCode + idDelta);
		} else {
            charP += uTmp + offset + offset; /* point at glyphIdArray[n] */
            if (charP >= t->cmapData + t->length)
                return 0;
            uTmp = *charP; uTmp <<= 8; uTmp |= *(charP+1);
            index = (unsigned short)(uTmp + idDelta);
        }
    }

    /* Only valid indexes */
    if (index <  t->numGlyphs) {
        return index;
    } else {
        return 0;
    }

}

/*
 * Format 6: Trimmed table mapping
 */
static uint16 Compute_cmapClass_Index6( cmapClass *t, uint16 charCode )
{
    register uint16 entryCount, index;
    register uint8 *charP;

    charP = t->cmapData + t->platform[t->preferedEncodingTable]->offset; /* point at subTable */
	charP += 6; 										/* skip format, length, and version */

    index = *charP++;
    index <<= 8;
    index |= *charP++;
    charCode = (unsigned short)(charCode - index); /* -firstCode */
    entryCount = *charP++;
    entryCount <<= 8;
    entryCount |= *charP++; /* entryCount */
    index = 0;
    if ( charCode < entryCount ) {
        charP += charCode; /* *2, since word Array */
        charP += charCode; /* *2, since word Array */
        index = *charP++;
        index <<= 8;
        index |= *charP;
    }

#ifdef USE_SEAT_BELTS
    /* Only valid indexes */
    if (index <  t->numGlyphs) {
        return index;
    } else {
        return 0;
    }
#else 
    return index;
#endif

}


/*
 * charCode -> glyphIndex
 */
static uint16 Compute_cmapClass_GlyphIndex( cmapClass *t, uint16 charCode )
{
    uint16 gIndex = 0;

	/* Initialize */
	t->bytesConsumed = 2;

	if ( t != NULL ) {
        if ( t->preferedFormat == 0 ) {
            gIndex = Compute_cmapClass_Index0( t, charCode );
        } else if ( t->preferedFormat == 2 ) {
            gIndex = Compute_cmapClass_Index2( t, charCode );
        } else if ( t->preferedFormat == 4 ) {
            gIndex = Compute_cmapClass_Index4( t, charCode );
        } else if ( t->preferedFormat == 6 ) {
            gIndex = Compute_cmapClass_Index6( t, charCode );
        } else if ( t->preferedFormat == 9999 ) {
#ifdef ENABLE_T2KS
                    gIndex = Compute_cmapClass_Index9999( t, charCode );
#else
                    gIndex = 0;
#endif
        }
    }

    return gIndex; /*****/
}

/*
 *
 */
static cmapClass *New_cmapClass( tsiMemObject *mem, uint16 preferedPlatformID, uint16 preferedPlatformSpecificID, InputStream *in )
{
    long i, pass;
    uint16 format;
    uint8 *charP;
    cmapClass *t = (cmapClass *) tsi_AllocMem( mem, sizeof( cmapClass ) );

    t->mem               = mem;
    t->version           = ReadInt16(in);
    t->numEncodingTables = ReadInt16(in);

	/* Dummy initial value */    
	t->numGlyphs         = 0xFFFFFF;

    t->platform          = (sfnt_platformEntry **) tsi_AllocMem( mem, t->numEncodingTables * sizeof( sfnt_platformEntry * ) );

    for ( i = 0; i < t->numEncodingTables; i++ ) {
        t->platform[i] = (sfnt_platformEntry *)tsi_AllocMem( mem, sizeof( sfnt_platformEntry ) );

        t->platform[i]->platformID      = (uint16)ReadInt16(in);
        t->platform[i]->specificID      = (uint16)ReadInt16(in);
        /* 1/29/2001 Changed uint16 cast to uint32 */
        t->platform[i]->offset          = (uint32)ReadInt32(in);
    }

    Rewind_InputStream( in );
    t->length = SizeInStream( in );
	t->cmapData	= (unsigned char*) tsi_AllocMem( mem, (unsigned long)t->length );
    ReadSegment( in, t->cmapData, t->length );


/*
preferedPlatformID = 3;
preferedPlatformSpecificID = 1;
*/

    t->preferedEncodingTable = 0;

    for ( pass = 0; pass <= 4; pass++ ) {
        for ( i = 0; i < t->numEncodingTables; i++ ) {
            charP = t->cmapData + t->platform[i]->offset;    /* point at subTable */

            format = *charP++;
            format <<= 8;
            format |= *charP;

            if (pass == 0 && (format == 0 || format == 2 ||
                format == 4 || format == 6 || format == 9999 ) ) {
                int A, B;
                A = t->platform[i]->platformID == preferedPlatformID;
                B = t->platform[i]->specificID == preferedPlatformSpecificID;

                if ( (A && ( B || preferedPlatformSpecificID == 0xffff) ) ||
                     (B && ( A || preferedPlatformID         == 0xffff) ) ) {
                    pass = 999;
                    t->preferedEncodingTable = (short)i;
                    break;/*****/
                }
            }

            if ( (pass == 1 && format == 0) ||
                 (pass == 2 && format == 2) ||
                 (pass == 3 && format == 4) ||
                 (pass == 4 && format == 6) ||
                 (pass == 4 && format == 9999) ) {
                pass = 999;
                t->preferedEncodingTable = (short)i;
                break;/*****/
            }
        }
    }
	charP = t->cmapData + t->platform[t->preferedEncodingTable]->offset;	/* point at subTable */
    format = *charP++;
    format <<= 8;
    format |= *charP;
    t->preferedFormat = format;

    /* Store selected cmap designation */
    t->selectedPlatformID         = t->platform[t->preferedEncodingTable]->platformID;
    t->selectedPlatformSpecificID = t->platform[t->preferedEncodingTable]->specificID;

    tsi_Assert( mem, t->preferedFormat == 0 ||
                     t->preferedFormat == 2 ||
                     t->preferedFormat == 4 ||
                     t->preferedFormat == 6 ||
                     t->preferedFormat == 9999,
                     T2K_BAD_CMAP);

    {
        char c;
        i = 0;
		for ( c = '0'; c <= '9'; c++ ) {
            assert( i < NUM_FIGURES );
            t->figIndex[i] = Compute_cmapClass_GlyphIndex( t, c );
            if ( t->figIndex[i] == 0 ) t->figIndex[i] = 0xffff;
            i++;
        }
    }


    return t; /*****/
}


/*
 *
 */
static void Delete_cmapClass( cmapClass *t )
{
    if ( t != NULL ) {
        long i;
        for ( i = 0; i < t->numEncodingTables; i++ ) {
            tsi_DeAllocMem( t->mem, t->platform[i] );
        }
        tsi_DeAllocMem( t->mem, t->platform );
        tsi_DeAllocMem( t->mem, t->cmapData );
        tsi_DeAllocMem( t->mem, t );
    }
}


#ifdef ENABLE_WRITE
void TEST_T2K_CMAP( InputStream *in, uint16 *charCodeToGIndex, long N, uint16 platformID, uint16 encodingID)
{
    cmapClass *cmap;
    uint16 gIndex;
    long i;

	cmap = New_cmapClass( in->mem, platformID, encodingID, in);
    for ( i = 0; i < N; i++ ) {
        gIndex = Compute_cmapClass_GlyphIndex( cmap, (uint16)i );
		if (  gIndex !=  charCodeToGIndex[i] ) {
			printf("gIndex = %d, and charCodeToGIndex[%d] = %d\n",gIndex, i, charCodeToGIndex[i]);
		}
        assert( gIndex == charCodeToGIndex[i] );
    }
    printf("CMAP OK :-)\n");

    Delete_cmapClass( cmap );
}
#endif /* ENABLE_WRITE */

typedef struct {
	uint16	version;
	uint16	numTables;
	sfnt_platformEntry platform[1];	/* platform[numTables] */
} sfnt_char2IndexDirectory;



/***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/
#ifdef ENABLE_KERNING
/***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/
/*
 *
 */
static kernSubTable0Data *New_kernSubTable0Data( tsiMemObject *mem, InputStream *in )
{
    int i;
    kernSubTable0Data *t = (kernSubTable0Data *) tsi_AllocMem( mem, sizeof( kernSubTable0Data ) );
	t->mem				= mem;
	t->nPairs			= (uint16)ReadInt16(in);
	t->searchRange		= (uint16)ReadInt16(in);
	t->entrySelector	= (uint16)ReadInt16(in);
	t->rangeShift		= (uint16)ReadInt16(in);
	
	t->pairs			= (kernPair0Struct*) tsi_AllocMem( mem, t->nPairs * sizeof(kernPair0Struct) );
	for ( i = 0; i < t->nPairs; i++ ) {
		t->pairs[i].leftRightIndex	= (uint32)ReadInt32(in);
		t->pairs[i].value 			= ReadInt16(in);
    }
#ifdef OLD
    /* Check Hoefler Text ! */
    for ( i = 0; i < t->nPairs-1; i++ ) {
        printf("%d:%d %d\n", i, t->pairs[i].leftRightIndex, t->pairs[i+1].leftRightIndex );
        if ( t->pairs[i].leftRightIndex >= t->pairs[i+1].leftRightIndex ) printf("******************************************************");
        /* assert( t->pairs[i].leftRightIndex < t->pairs[i+1].leftRightIndex ); */
    }
#endif

    return t; /*****/
}

static int16 GetKernSubTable0Value( kernSubTable0Data *t, uint16 left, uint16 right )
{
    int low, mid, high;
    uint32 leftRightIndex, mid_leftRightIndex;
    int16 value = 0;

    leftRightIndex = left;
    leftRightIndex <<= 16;
    leftRightIndex |= right;

    low = 0;
    high = t->nPairs - 1;
    /* do a binary search */
    do {
        mid = (low + high) >> 1;
        mid_leftRightIndex = t->pairs[mid].leftRightIndex;
        if ( leftRightIndex > mid_leftRightIndex ) {
            low = mid+1;
        } else if ( leftRightIndex < mid_leftRightIndex ) {
            high = mid-1;
        } else { /* mid_leftRightIndex == leftRightIndex */
            value = t->pairs[mid].value;
            break; /*****/
        }
    } while ( high >= low );

    return value; /*****/
}

#ifdef ENABLE_PRINTF
static void Print_kernSubTable0Data( kernSubTable0Data *t )
{
    int i;
    printf("nPairs:         %d\n", t->nPairs);
    printf("searchRange:    %d\n", t->searchRange);
    printf("entrySelector:  %d\n", t->entrySelector);
    printf("rangeShift:     %d\n", t->rangeShift);

    for ( i = 0; i < t->nPairs; i++ ) {
        printf("%d: <%d - %d> -> %d\n", i, (uint16)(t->pairs[i].leftRightIndex >> 16), (uint16)(t->pairs[i].leftRightIndex & 0xffff), t->pairs[i].value );
        /*
        assert( GetKernSubTable0Value( t, (uint16)(t->pairs[i].leftRightIndex >> 16), (uint16)(t->pairs[i].leftRightIndex & 0xffff) ) == t->pairs[i].value );
        */
      }
}
#endif

/*
 *
 */
static void Delete_kernSubTable0Data( kernSubTable0Data *t )
{
    if ( t != NULL ) {
        if (t->pairs != NULL)
            tsi_DeAllocMem( t->mem, t->pairs );
        tsi_DeAllocMem( t->mem, t );
    }
}

/*
 *
 */
static kernSubTable *New_kernSubTable( tsiMemObject *mem, int appleFormat, InputStream *in )
{
    kernSubTable *t = (kernSubTable *) tsi_AllocMem( mem, sizeof( kernSubTable ) );
	t->mem			= mem;

    if ( appleFormat ) {
		t->length  		= ReadInt32(in);
		t->coverage		= (uint16)ReadInt16(in);
        ReadInt16(in); /* The tuple index */
		t->version 		= (uint16)(t->coverage & 0x00ff);
	} else {
		t->version		= (uint16)ReadInt16(in);
		t->length		= (uint16)ReadInt16(in);
		t->coverage		= (uint16)ReadInt16(in);
	}
	
	t->kernData			= NULL;
    if ( t->version == 0 && t->length > 0 ) {
		t->kernData			= New_kernSubTable0Data( mem, in );
    }

    return t; /*****/
}

#ifdef ENABLE_PRINTF
static void Print_kernSubTable(kernSubTable *t) {
    printf("---------- Begin kern subtable----------\n");
	printf("version:    	%d\n", t->version);
	printf("length:    		%d\n", t->length);
	printf("coverage:    	%d\n", t->coverage);
    if ( t->version == 0 ) {
        Print_kernSubTable0Data( (kernSubTable0Data *)t->kernData );
    }
    printf("----------  End kern  subtable----------\n");
}
#endif

/*
 *
 */
static void Delete_kernSubTable( kernSubTable *t )
{
    if ( t->version == 0 ) {
        Delete_kernSubTable0Data( (kernSubTable0Data *)t->kernData );
    }
    tsi_DeAllocMem( t->mem, t );
}

/*
 *
 */
static kernClass *New_kernClass( tsiMemObject *mem, InputStream *in )
{
    int i;
    int appleFormat = 0;

    kernClass *t = (kernClass *) tsi_AllocMem( mem, sizeof( kernClass ) );
	t->mem			= mem;
	t->version		= (uint16)ReadInt16(in);
	t->nTables		= ReadInt16(in);

    if ( t->version == 1 && t->nTables == 0 ) { /* Apple 1.0 in 16.16 format */
		t->nTables 	= ReadInt32(in);
		appleFormat	= 1;
	}
	
	t->table		= (kernSubTable **) tsi_AllocMem( mem, t->nTables * sizeof( kernSubTable * ) );
    for ( i = 0; i < t->nTables; i++ ) {
        t->table[i] = New_kernSubTable( mem, appleFormat, in );
    }

    return t; /*****/
}

static int16 GetKernValue( kernClass *t, uint16 left, uint16 right )
{
    int16 value = 0;
    if ( t->nTables > 0 && t->table[0]->version == 0 ) {
        value = GetKernSubTable0Value( (kernSubTable0Data *)t->table[0]->kernData, left, right );
    }

    return value; /*****/
}


#ifdef ENABLE_PRINTF
static void Print_kernClass(kernClass *t) {
    int i;
    printf("---------- Begin kern ----------\n");
	printf("version:    	%d\n", t->version);
	printf("nTables:    	%d\n", t->nTables);
    for ( i = 0; i < t->nTables; i++ ) {
        Print_kernSubTable( t->table[i] );
    }
    printf("----------  End kern  ----------\n");
}
#endif


/*
 *
 */
static void Delete_kernClass( kernClass *t )
{
    if ( t != NULL ) {
        int i;
        for ( i = 0; i < t->nTables; i++ ) {
            Delete_kernSubTable( t->table[i] );
        }
        tsi_DeAllocMem( t->mem, t->table );
        tsi_DeAllocMem( t->mem, t );
    }
}

#ifdef ENABLE_KERNING
 void GetSfntClassKernValue( sfntClass *t, uint16 leftGIndex, uint16 rightGIndex, int16 *xKern, int16 *yKern )
{
    *xKern = 0;
    *yKern = 0;
    if ( t->kern != NULL ) {
        *xKern = GetKernValue( t->kern, leftGIndex, rightGIndex );
    }
}
#endif /* ENABLE_KERNING */




/***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/
#endif /* ENABLE_KERNING */
/***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/

#ifdef ENABLE_GASP_TABLE_SUPPORT
/*
 *
 */
static gaspClass *New_gaspClass( tsiMemObject *mem, InputStream *in )
{
	int i;
	
	gaspClass *t = (gaspClass *) tsi_AllocMem( mem, sizeof( gaspClass ) );
	t->mem			= mem;
	t->version		= (uint16)ReadInt16(in);
	t->numRanges	= (uint16)ReadInt16(in);
	
	t->gaspRange    = (gaspRangeType	*)tsi_AllocMem( mem, t->numRanges * sizeof( gaspRangeType ) );
	
	for ( i = 0; i < t->numRanges; i++ ) {
		t->gaspRange[i].rangeMaxPPEM 		= (uint16)ReadInt16(in);
		t->gaspRange[i].rangeGaspBehavior	= (uint16)ReadInt16(in);
	}
	
	return t; /*****/
}

/*
 * IN: t, ppem
 * OUT: useGridFitting, useGrayScaleRendering
 * returns true if information found
 */
int Read_gasp( gaspClass *t, int ppem, int *useGridFitting, int *useGrayScaleRendering )
{
	register int i, numRanges, success = false;
	register gaspRangeType *gaspRange;
	
	if ( t != NULL ) {
		numRanges = t->numRanges;
		gaspRange = t->gaspRange;
		for ( i = 0; i < numRanges; i++ ) {
			if ( ppem <= gaspRange[i].rangeMaxPPEM ) {
				*useGridFitting 		= gaspRange[i].rangeGaspBehavior & 0x0001;
				*useGrayScaleRendering	= gaspRange[i].rangeGaspBehavior & 0x0002;
				success =  true; 
				break; /*****/
			}
		}
	}
	return success; /*****/
}

/*
 *
 */
static void Delete_gaspClass( gaspClass *t )
{
	if ( t != NULL ) {
		tsi_DeAllocMem( t->mem, t->gaspRange );
		tsi_DeAllocMem( t->mem, t );
	}
}
#endif /* ENABLE_GASP_TABLE_SUPPORT */


/*
 *
 */
static sfnt_DirectoryEntry *New_sfnt_DirectoryEntry( tsiMemObject *mem, InputStream *in )
{
    sfnt_DirectoryEntry *t = (sfnt_DirectoryEntry *) tsi_AllocMem( mem, sizeof( sfnt_DirectoryEntry ) );
	t->mem			= mem;
	t->tag			= ReadInt32(in);
	t->checkSum		= ReadInt32(in);
	t->offset		= (unsigned long)ReadInt32(in);
	t->length		= (unsigned long)ReadInt32(in);

    return t; /*****/
}


#ifdef ENABLE_WRITE
/*
 *
 */
static sfnt_DirectoryEntry *New_sfnt_EmptyDirectoryEntry( tsiMemObject *mem )
{
    sfnt_DirectoryEntry *t = (sfnt_DirectoryEntry *) tsi_AllocMem( mem, sizeof( sfnt_DirectoryEntry ) );
	t->mem			= mem;
	t->tag			= 0;
	t->checkSum		= 0;
	t->offset		= 0;
	t->length		= 0;

    return t; /*****/
}
#endif /* ENABLE_WRITE */

#ifdef ENABLE_WRITE
static void Write_sfnt_DirectoryEntry( sfnt_DirectoryEntry *t, OutputStream *out )
{
    WriteInt32( out, t->tag );
    WriteInt32( out, t->checkSum );
    WriteInt32( out, (int32)t->offset );
    WriteInt32( out, (int32)t->length );
}
#endif

#ifdef ENABLE_PRINTF
static void Print_sfnt_DirectoryEntry( sfnt_DirectoryEntry *t, long i)
{
    printf("%ld : %c%c%c%c offset = %ld, length = %ld\n", i,
                                (char)((t->tag >> 24) & 0xff),
                                (char)((t->tag >> 16) & 0xff),
                                (char)((t->tag >>  8) & 0xff),
                                (char)((t->tag >>  0) & 0xff),
                                t->offset, t->length );

}
#endif


/*
 *
 */
static void Delete_sfnt_DirectoryEntry( sfnt_DirectoryEntry *t )
{
    tsi_DeAllocMem( t->mem, t );
}

/***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/
/***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/



/*
 *
 */
static sfnt_OffsetTable *New_sfnt_OffsetTable( tsiMemObject *mem, InputStream *in )
{
    long i;
	sfnt_OffsetTable *t = (sfnt_OffsetTable *) tsi_AllocMem( mem, sizeof( sfnt_OffsetTable ) );
	t->mem				= mem;
	t->version			= ReadInt32(in);
	t->numOffsets		= ReadInt16(in);
	t->searchRange		= ReadInt16(in);
	t->entrySelector	= ReadInt16(in);
	t->rangeShift		= ReadInt16(in);
	
	t->table 			= (sfnt_DirectoryEntry **) tsi_AllocMem( mem, t->numOffsets * sizeof(sfnt_DirectoryEntry *) );
    for ( i = 0; i < t->numOffsets; i++ ) {
        t->table[i] = New_sfnt_DirectoryEntry( mem, in );
        /* Print_sfnt_DirectoryEntry( t->table[i], i ); */
    }
    return t; /*****/
}

#ifdef ENABLE_WRITE
/*
 *
 */
sfnt_OffsetTable *New_sfnt_EmptyOffsetTable( tsiMemObject *mem, short numberOfOffsets )
{
    long i;
	sfnt_OffsetTable *t = (sfnt_OffsetTable *) tsi_AllocMem( mem, sizeof( sfnt_OffsetTable ) );
	t->mem				= mem;
	t->version			= 0x10000;
	t->numOffsets		= numberOfOffsets;
	t->searchRange		= 0;
	t->entrySelector	= 0;
	t->rangeShift		= 0;
	
	t->table 			= (sfnt_DirectoryEntry **) tsi_AllocMem( mem, numberOfOffsets * sizeof(sfnt_DirectoryEntry *) );
    for ( i = 0; i < t->numOffsets; i++ ) {
        t->table[i] = New_sfnt_EmptyDirectoryEntry( mem );
    }
    return t; /*****/
}
#endif /* OLD_OLD_ENABLE_WRITE */



#ifdef ENABLE_WRITE
void SortTableDirectory( sfnt_OffsetTable *offsetTable )
{
	int						j, swap;
	sfnt_DirectoryEntry		temp;
	
	for ( swap = true; swap; ) {	/* Bubble-Sort it */
        swap = false;
        for ( j = offsetTable->numOffsets-2; j >= 0; j--) {
            if ( (unsigned long)(offsetTable->table[j]->tag) > (unsigned long)(offsetTable->table[j+1]->tag) ) {
				swap 					  = true;
				temp 					  = *offsetTable->table[j];
				*offsetTable->table[j] 	  = *offsetTable->table[j+1];
				*offsetTable->table[j+1]  = temp;
            }
        }
    }
}
#endif /* ENABLE_WRITE */



#ifdef ENABLE_WRITE
/*
 *
 */
static void Recompute_OffsetTableFields( sfnt_OffsetTable *t )
{
    register unsigned short i;

    t->searchRange = 0;
    for ( i = 1; i <= t->numOffsets*16; i = (unsigned short)(i+i) ) {
        t->searchRange = (short)i;
    }
    for ( i = 1; (1<<i) <= t->numOffsets; i++ ) {
        t->entrySelector = (short)i;
    }
    t->rangeShift = (short)(t->numOffsets*16 - t->searchRange);
}
#endif /* ENABLE_WRITE */

#ifdef ENABLE_WRITE

/*
 *
 */
void CreateTableIfMissing( sfnt_OffsetTable *t, long tag )
{
    long i;
    int found = false;

    for ( i = 0; i < t->numOffsets; i++ ) {
        if ( t->table[i]->tag == tag ) {
            found = true;
            break; /*****/
        }
    }
    if ( !found ) {
#ifdef ENABLE_PRINTF
printf("Adding Table %c%c%c%c\n", (char) (tag>>24)&0xff,(char) (tag>>16)&0xff,(char) (tag>>8)&0xff,(char) (tag>>0)&0xff );
#endif
        i = t->numOffsets++;
        if ( t->table != NULL ) {
            t->table = (sfnt_DirectoryEntry **)tsi_ReAllocMem( t->mem, t->table , t->numOffsets * sizeof(sfnt_DirectoryEntry *) );
        } else {
            t->table = (sfnt_DirectoryEntry **)tsi_AllocMem( t->mem, t->numOffsets * sizeof(sfnt_DirectoryEntry *) );
        }
        assert( t->table != NULL );
        t->table[i] = New_sfnt_EmptyDirectoryEntry( t->mem );
        t->table[i]->tag = tag;
        SortTableDirectory(t);
        Recompute_OffsetTableFields(t);
    }
}
#endif /* ENABLE_WRITE */

#ifdef ENABLE_WRITE

/*
 *
 */
static void DeleteTable( sfnt_OffsetTable *t, long tag )
{
    long i;
    int found = false;

    for ( i = 0; i < t->numOffsets; i++ ) {
        if ( t->table[i]->tag == tag ) {
            found = true;
            break; /*****/
        }
    }
    if ( found ) {
#ifdef ENABLE_PRINTF
printf("Deleting Table %c%c%c%c\n", (char) (tag>>24)&0xff,(char) (tag>>16)&0xff,(char) (tag>>8)&0xff,(char) (tag>>0)&0xff );
#endif
        Delete_sfnt_DirectoryEntry( t->table[i] );
        t->numOffsets--;
        t->table[i] = t->table[t->numOffsets];
        SortTableDirectory(t);
        Recompute_OffsetTableFields(t);
    }
}
#endif /* ENABLE_WRITE */


#ifdef ENABLE_WRITE
/*
 *
 */
void Write_sfnt_OffsetTable( sfnt_OffsetTable *t, OutputStream *out )
{
    register long i;

    Recompute_OffsetTableFields( t );
    WriteInt32( out, t->version );
    WriteInt16( out, t->numOffsets );
    WriteInt16( out, t->searchRange );
    WriteInt16( out, t->entrySelector );
    WriteInt16( out, t->rangeShift );

    for ( i = 0; i < t->numOffsets; i++ ) {
        Write_sfnt_DirectoryEntry( t->table[i], out );
    }
}
#endif /* ENABLE_WRITE */

/*
 *
 */
static void Delete_sfnt_OffsetTable( sfnt_OffsetTable *t )
{
    if ( t != NULL ) {
        long i;

        for ( i = 0; i < t->numOffsets; i++ ) {
            Delete_sfnt_DirectoryEntry( t->table[i] );
        }
        tsi_DeAllocMem( t->mem, t->table );
        tsi_DeAllocMem( t->mem, t );
    }
}


/***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/
/***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/
/*
 *
 */
static hheaClass *New_hheaClass( tsiMemObject *mem, InputStream *in )
{
    hheaClass *t = (hheaClass *) tsi_AllocMem( mem, sizeof( hheaClass ) );
	t->mem						= mem;
	t->version					= ReadInt32( in );
	t->Ascender					= ReadInt16( in );
	t->Descender				= ReadInt16( in );
	t->LineGap					= ReadInt16( in );
	t->advanceWidthMax			= (uint16)ReadInt16( in );
	t->minLeftSideBearing		= ReadInt16( in );
	t->minRightSideBearing		= ReadInt16( in );
	t->xMaxExtent				= ReadInt16( in );
	t->caretSlopeRise			= ReadInt16( in );
	t->caretSlopeRun			= ReadInt16( in );
	t->caretOffset				= ReadInt16( in );
	t->reserved2				= ReadInt16( in );
	t->reserved3				= ReadInt16( in );
	t->reserved4				= ReadInt16( in );
	t->reserved5				= ReadInt16( in );
	t->metricDataFormat			= ReadInt16( in );
	t->numberOfHMetrics			= (uint16)ReadInt16( in );

    return t; /*****/
}

#ifdef ENABLE_WRITE
void Write_hheaClass( hheaClass *t, OutputStream *out) {
    WriteInt32( out, t->version );

    WriteInt16( out, t->Ascender );
    WriteInt16( out, t->Descender );
    WriteInt16( out, t->LineGap );
    WriteInt16( out, (int16)t->advanceWidthMax );
    WriteInt16( out, t->minLeftSideBearing );
    WriteInt16( out, t->minRightSideBearing );
    WriteInt16( out, t->xMaxExtent );
    WriteInt16( out, t->caretSlopeRise );
    WriteInt16( out, t->caretSlopeRun );
    WriteInt16( out, t->caretOffset );
    WriteInt16( out, t->reserved2 );
    WriteInt16( out, t->reserved3 );
    WriteInt16( out, t->reserved4 );
    WriteInt16( out, t->reserved5 );
    WriteInt16( out, t->metricDataFormat );
    WriteInt16( out, (int16)t->numberOfHMetrics );

}
#endif /* ENABLE_WRITE */


#ifdef ENABLE_PRINTF
static void Print_hheaClass(hheaClass *t) {
    printf("---------- Begin hhea ----------\n");
    printf("Ascender:         %ld\n", (long)t->Ascender);
    printf("Descender:        %ld\n", (long)t->Descender);
    printf("LineGap:          %ld\n", (long)t->LineGap);
    printf("metricDataFormat: %ld\n", (long)t->metricDataFormat);
    printf("numberOfHMetrics: %ld\n", (long)t->numberOfHMetrics);
    printf("caretSlopeRise:   %ld\n", (long)t->caretSlopeRise);
    printf("caretSlopeRun:    %ld\n", (long)t->caretSlopeRun);
    printf("caretOffset:      %ld\n", (long)t->caretOffset);
    printf("----------  End hhea  ----------\n");
}
#endif

/*
 *
 */
static void Delete_hheaClass( hheaClass *t )
{
    if ( t != NULL ) {
        tsi_DeAllocMem( t->mem, t );
    }
}

/***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/
/***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/

hmtxClass *New_hmtxEmptyClass( tsiMemObject *mem, int32 numGlyphs, int32 numberOfHMetrics )
{
    hmtxClass *t = (hmtxClass *) tsi_AllocMem( mem, sizeof( hmtxClass ) );

	t->mem		 = mem;
	t->numGlyphs = numGlyphs;
    t->numberOfHMetrics = numberOfHMetrics;

    tsi_Assert( mem, numberOfHMetrics > 0, T2K_BAD_FONT );
    tsi_Assert( mem, numGlyphs > 0, T2K_BAD_FONT );

    t->lsb = (short *) tsi_AllocMem( mem, numGlyphs * sizeof(int16) );  assert( t->lsb != NULL );
    t->aw  = (unsigned short *) tsi_AllocMem( mem, numGlyphs * sizeof(uint16) ); assert( t->aw != NULL );

    return t; /*****/
}


static hmtxClass *New_hmtxClass( tsiMemObject *mem, InputStream *in, int32 numGlyphs, int32 numberOfHMetrics )
{
    register int32 i;
    uint16 last_aw;
    hmtxClass *t;

	/* USE_SEAT_BELTS */
	if ( numberOfHMetrics > numGlyphs ) {
		numberOfHMetrics = numGlyphs;
	}

    t = New_hmtxEmptyClass( mem, numGlyphs, numberOfHMetrics );

    for ( i = 0; i < numberOfHMetrics; i++ ) {
        t->aw[i]  = (uint16)ReadInt16( in );
        t->lsb[i] = ReadInt16( in );
    }
    for ( last_aw = t->aw[i-1]; i < numGlyphs; i++ ) {
        t->aw[i]  = last_aw;
        t->lsb[i] = ReadInt16( in );
    }
    return t; /*****/
}

#ifdef ENABLE_WRITE
void Write_hmtxClass( hmtxClass *t, OutputStream *out) {
    register int32 i;
    for ( i = 0; i < t->numberOfHMetrics; i++ ) {
        WriteInt16( out, (int16)t->aw[i] );
        WriteInt16( out, t->lsb[i] );
    }
    for ( ;i < t->numGlyphs; i++ ) {
        WriteInt16( out, t->lsb[i] );
    }
}
#endif /* ENABLE_WRITE */

#ifdef ENABLE_PRINTF
static void Print_hmtxClass(hmtxClass *t) {
    register long i;
    printf("---------- Begin hmtx ----------\n");
    for ( i = 0; i < 3; i++ ) {
        printf("aw[%ld]  = %ld\n", i, (long)t->aw[i] );
        printf("lsb[%ld] = %ld\n", i, (long)t->lsb[i] );
    }
    printf("...\n");
    for ( i = t->numGlyphs - 3; i < t->numGlyphs; i++ ) {
        printf("aw[%ld]  = %ld\n", i, (long)t->aw[i] );
        printf("lsb[%ld] = %ld\n", i, (long)t->lsb[i] );
    }
    printf("----------  End hmtx  ----------\n");
}
#endif

/*
 *
 */
void Delete_hmtxClass( hmtxClass *t )
{
    if ( t != NULL ) {
        tsi_DeAllocMem( t->mem, t->lsb );
        tsi_DeAllocMem( t->mem, t->aw );
        tsi_DeAllocMem( t->mem, t );
    }
}


/***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/

static ttcfClass *New_ttcfClass( tsiMemObject *mem, InputStream *in )
{
    uint32 info;
    ttcfClass *t = NULL;
    uint32 i;

    info = (uint32)ReadInt32( in );
    if ( info == tag_TTCollectionID ) {
		t 					= (ttcfClass *) tsi_AllocMem( mem, sizeof( ttcfClass ) );
		t->mem				= mem;
		t->version			= (uint32)ReadInt32( in );
		t->directoryCount	= (uint32)ReadInt32( in );
		t->tableOffsets		= (uint32 *) tsi_AllocMem( mem, sizeof(uint32 *) * t->directoryCount ); 
        for ( i = 0; i < t->directoryCount; i++ ) {
            t->tableOffsets[i] = (uint32)ReadInt32( in );
        }
#ifdef OLD
        {

            printf("ttcf:version = 0x%x\n", t->version );
            printf("ttcf:directoryCount = %d\n", t->directoryCount );
            for ( i = 0; i < t->directoryCount; i++ ) {
                printf("ttcf:tableOffsets[%d] = %d\n", i, t->tableOffsets[i] );
            }
        }
#endif
    }
    Rewind_InputStream( in );
    return t; /*****/
}

/*
 *
 */
static void Delete_ttcfClass( ttcfClass *t )
{
    if ( t != NULL ) {
        tsi_DeAllocMem( t->mem, t->tableOffsets );
        tsi_DeAllocMem( t->mem, t );
    }
}
/***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/


/*
 *
 */
static headClass *New_headClass( tsiMemObject *mem, InputStream *in )
{
    headClass *t = (headClass *) tsi_AllocMem( mem, sizeof( headClass ) );
    t->mem                  = mem;
    t->version              = ReadInt32( in );
    t->fontRevision         = ReadInt32( in );
    t->checkSumAdjustment   = (uint32)ReadInt32( in );
    t->magicNumber          = (uint32)ReadInt32( in );

    t->flags                = (uint16)ReadInt16( in );
    t->unitsPerEm           = (uint16)ReadInt16( in );

    t->created_bc           = ReadInt32( in );
    t->created_ad           = ReadInt32( in );
    t->modified_bc          = ReadInt32( in );
    t->modified_ad          = ReadInt32( in );

    t->xMin                 = ReadInt16( in );
    t->yMin                 = ReadInt16( in );
    t->xMax                 = ReadInt16( in );
    t->yMax                 = ReadInt16( in );

    t->macStyle             = (uint16)ReadInt16( in );
    t->lowestRecPPEM        = (uint16)ReadInt16( in );
    t->fontDirectionHint    = ReadInt16( in );
    t->indexToLocFormat     = ReadInt16( in );
    t->glyphDataFormat      = ReadInt16( in );

    return t; /*****/
}

#ifdef ENABLE_WRITE
void Write_headClass( headClass *t, OutputStream *out) {
    WriteInt32( out, t->version );
    WriteInt32( out, t->fontRevision );
    WriteInt32( out, (int32)t->checkSumAdjustment );
    WriteInt32( out, (int32)t->magicNumber );

    WriteInt16( out, (int16)t->flags );
    WriteInt16( out, (int16)t->unitsPerEm );

    WriteInt32( out, t->created_bc );
    WriteInt32( out, t->created_ad );
    WriteInt32( out, t->modified_bc );
    WriteInt32( out, t->modified_ad );

    WriteInt16( out, t->xMin );
    WriteInt16( out, t->yMin );
    WriteInt16( out, t->xMax );
    WriteInt16( out, t->yMax );

    WriteInt16( out, (int16)t->macStyle );
    WriteInt16( out, (int16)t->lowestRecPPEM );
    WriteInt16( out, t->fontDirectionHint );
    WriteInt16( out, t->indexToLocFormat );
    WriteInt16( out, t->glyphDataFormat );
}
#endif /* ENABLE_WRITE */

#ifdef ENABLE_PRINTF
static void Print_headClass(headClass *t) {
    printf("---------- Begin head ----------\n");
    printf("unitsPerEm:       %ld\n", t->unitsPerEm);
    printf("lowestRecPPEM:    %ld\n", t->lowestRecPPEM);
    printf("glyphDataFormat:  %ld\n", t->glyphDataFormat);
    printf("----------  End head  ----------\n");
}
#endif

/*
 *
 */
static void Delete_headClass( headClass *t )
{
    if ( t != NULL ) {
        tsi_DeAllocMem( t->mem, t );
    }
}

/***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/
/***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/

/*
 *
 */
static maxpClass *New_maxpClass( tsiMemObject *mem, InputStream *in )
{
    maxpClass *t = (maxpClass *) tsi_AllocMem( mem, sizeof( maxpClass ) );

    t->mem                      = mem;
    t->version                  = ReadInt32( in );
    t->numGlyphs                = (uint16)ReadInt16( in );
    t->maxPoints                = (uint16)ReadInt16( in );
    t->maxContours              = (uint16)ReadInt16( in );
    t->maxCompositePoints       = (uint16)ReadInt16( in );
    t->maxCompositeContours     = (uint16)ReadInt16( in );
    t->maxElements              = (uint16)ReadInt16( in );
    t->maxTwilightPoints        = (uint16)ReadInt16( in );
    t->maxStorage               = (uint16)ReadInt16( in );
    t->maxFunctionDefs          = (uint16)ReadInt16( in );
    t->maxInstructionDefs       = (uint16)ReadInt16( in );
    t->maxStackElements         = (uint16)ReadInt16( in );
    t->maxSizeOfInstructions    = (uint16)ReadInt16( in );
    t->maxComponentElements     = (uint16)ReadInt16( in );
    t->maxComponentDepth        = (uint16)ReadInt16( in );
    
#ifdef USE_SEAT_BELTS
	if ( t->maxFunctionDefs < 1 )		t->maxFunctionDefs 		= 1;
#endif

    return t; /*****/
}

#ifdef ENABLE_WRITE
void Write_maxpClass( maxpClass *t, OutputStream *out ) {
        WriteInt32( out, t->version );
                
        WriteInt16( out, (int16)t->numGlyphs                    );
        WriteInt16( out, (int16)t->maxPoints                    );
        WriteInt16( out, (int16)t->maxContours                  );
        WriteInt16( out, (int16)t->maxCompositePoints   );
    WriteInt16( out, (int16)t->maxCompositeContours);
        WriteInt16( out, (int16)t->maxElements                  );
        WriteInt16( out, (int16)t->maxTwilightPoints    );
        WriteInt16( out, (int16)t->maxStorage                   );
        WriteInt16( out, (int16)t->maxFunctionDefs              );
        WriteInt16( out, (int16)t->maxInstructionDefs   );
        WriteInt16( out, (int16)t->maxStackElements     );
    WriteInt16( out, (int16)t->maxSizeOfInstructions );
    WriteInt16( out, (int16)t->maxComponentElements );
        WriteInt16( out, (int16)t->maxComponentDepth    );
}
#endif /* ENABLE_WRITE */


#ifdef ENABLE_PRINTF
static void Print_maxpClass( maxpClass *t ) {
    printf("---------- Begin maxp ----------\n");
    printf("numGlyphs:             %ld\n", (long)t->numGlyphs);
    printf("maxPoints:             %ld\n", (long)t->maxPoints);
    printf("maxContours:           %ld\n", (long)t->maxContours);
    printf("maxCompositePoints:    %ld\n", (long)t->maxCompositePoints);
    printf("maxCompositeContours:  %ld\n", (long)t->maxCompositeContours);
    printf("maxElements:           %ld\n", (long)t->maxElements);
    printf("maxTwilightPoints:     %ld\n", (long)t->maxTwilightPoints);
    printf("maxStorage:            %ld\n", (long)t->maxStorage);
    printf("maxFunctionDefs:       %ld\n", (long)t->maxFunctionDefs);
    printf("maxInstructionDefs:    %ld\n", (long)t->maxInstructionDefs);
    printf("maxStackElements:      %ld\n", (long)t->maxStackElements);
    printf("maxSizeOfInstructions: %ld\n", (long)t->maxSizeOfInstructions);
    printf("maxComponentElements:  %ld\n", (long)t->maxComponentElements);
    printf("maxComponentDepth:     %ld\n", (long)t->maxComponentDepth);
    printf("----------  End maxp  ----------\n");
}
#endif

/*
 *
 */
static void Delete_maxpClass( maxpClass *t )
{
    if ( t != NULL ) {
        tsi_DeAllocMem( t->mem, t );
    }
}


/***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/
/***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/


/*
 *
 */
locaClass *New_locaClass( tsiMemObject *mem, InputStream *in, short indexToLocFormat, long length )
{
    register long i, n;
    locaClass *t = (locaClass *) tsi_AllocMem( mem, sizeof( locaClass ) );
	t->mem		= mem;
	/* n 			= numGlyphs + 1; */

    n = length >> (1 + indexToLocFormat); /* Modified 6/29/98 --- Sampo */

	t->n 				= n;
    t->indexToLocFormat = indexToLocFormat;
	t->offsets  		= (uint32 *) tsi_AllocMem( mem, n * sizeof( uint32 ) );

    if ( in != NULL ) {
        if ( indexToLocFormat ==  1 ) {
            for ( i = 0; i < n; i++ ) {
                t->offsets[i] = (uint32)ReadInt32( in );
            }
        } else if ( indexToLocFormat == 0 ) {
            for ( i = 0; i < n; i++ ) {
                uint16 offset = (uint16)ReadInt16( in );
                t->offsets[i] = 2L * (uint32)offset;
            }
        } else {
            tsi_Assert( mem, ((indexToLocFormat == 0) || (indexToLocFormat == 1)), T2K_BAD_FONT );
            /* printf( "locaClass:Bad indexToLocFormat\n"); */
        }
    }
    return t; /*****/
}


#ifdef ENABLE_WRITE
void Write_locaClass( locaClass *t, OutputStream *out ) {
    register long i, n = t->n;
    t->indexToLocFormat = 0;

    for ( i = 0; i < n; i++ ) {
        uint32 offset = t->offsets[i];
        if ( (offset > 0x0000ffff * 2) || (offset & 1) ) {
            t->indexToLocFormat = 1;
            break; /*****/
        }
    }

    if ( t->indexToLocFormat ==  1 ) {
        for ( i = 0; i < n; i++ ) {
            WriteInt32( out, (int32)t->offsets[i] );
        }
    } else {
        for ( i = 0; i < n; i++ ) {
            WriteInt16( out, (int16)(t->offsets[i]/2) );
        }
    }
}
#endif /* ENABLE_WRITE */



#ifdef ENABLE_PRINTF
static void Print_locaClass( locaClass *t ) {
    register long i;
    printf("---------- Begin loca ----------\n");
    for ( i = 0; i < 3; i++ ) {
        printf("offset[%ld] = %ld\n", i, t->offsets[i] );
    }
    printf("...\n");
    for ( i = t->n - 3; i < t->n; i++ ) {
        printf("offset[%ld] = %ld\n", i, t->offsets[i] );
    }
    printf("----------  End loca  ----------\n");
}
#endif

/*
 *
 */
static void Delete_locaClass( locaClass *t )
{
    if ( t != NULL ) {
        tsi_DeAllocMem( t->mem, t->offsets );
        tsi_DeAllocMem( t->mem, t );
    }
}
/***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/
/***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/
#ifdef ENABLE_T2KS


/*
 *
 */
slocClass *FF_New_slocClass( tsiMemObject *mem, InputStream *in )
{
    register long i;
    T2K_sloc_entry *sloc;
    slocClass *t = (slocClass *) tsi_AllocMem( mem, sizeof( slocClass ) );
	t->mem		= mem;
	
	t->version 			= (uint32)ReadInt32(in);
	t->num_sloc_entries	= (uint16)ReadInt16(in);
    t->sloc = sloc = (T2K_sloc_entry *)tsi_AllocMem( mem, t->num_sloc_entries * sizeof(T2K_sloc_entry) );
    for ( i = 0; i < t->num_sloc_entries; i++ ) {
		sloc[i].gIndex	= (uint16)ReadInt16(in);
		sloc[i].delta	= (uint16)ReadInt16(in);
		sloc[i].offset	= ReadInt32(in);
	}
	t->numCorrections	= (uint32)ReadInt32(in);
	t->correctionOffset	= (uint32)Tell_InputStream( in );

    return t; /*****/
}

uint32 FF_SLOC_MapIndexToOffset( slocClass *t, InputStream *in, int32 gIndex )
{
    uint8 correction;
    uint16 _gIndexA, _gIndexB;
    uint32 offset;
    int32 estimate;
    long low, high, mid = 0;
    T2K_sloc_entry *sloc = t->sloc;

    Seek_InputStream( in, t->correctionOffset + gIndex );
    correction = (uint8)ReadUnsignedByteMacro2( in );

    _gIndexA = _gIndexB = 0;
    low  = 0;
    high = t->num_sloc_entries - 2;
    /* Do a simple binary search */
    while ( high >= low ) {
        mid = (high + low + 1) >> 1;

        _gIndexA = sloc[mid+0].gIndex;
        _gIndexB = sloc[mid+1].gIndex;
        if ( gIndex >= _gIndexA && gIndex < _gIndexB ) {
            break; /*****/
        } else if ( gIndex < _gIndexA ) {
			high	= mid-1;
		} else {
			low		= mid+1;
        }
    }
#ifdef LINEAR_TEST
    for ( mid = 0; mid <  t->num_sloc_entries; mid++ ) {
        _gIndexA = _gIndexB;
        _gIndexB = sloc[mid+1].gIndex;
        if ( _gIndexB > gIndex ) break; /*****/
    }
#endif
    _gIndexB--; /* Make B inclusive */
    assert( mid < t->num_sloc_entries );
    assert( gIndex >= _gIndexA && gIndex <= _gIndexB );


    estimate = sloc[mid].offset + (gIndex - _gIndexA)*sloc[mid].delta / (_gIndexB - _gIndexA);
    offset = (uint32)(estimate - correction);

    return offset; /*****/
}


/*
 *
 */
void FF_Delete_slocClass( slocClass *t )
{
    if ( t != NULL ) {
        tsi_DeAllocMem( t->mem, t->sloc );
        tsi_DeAllocMem( t->mem, t );
    }
}

#endif /* ENABLE_T2KS */

/***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/
/***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/

#ifdef ONCURVE
#undef ONCURVE
#endif
/*
 * Outline Unpacking Constants
*/
#define ONCURVE			0x01
#define XSHORT			0x02
#define YSHORT			0x04
#define REPEAT_FLAGS	0x08 /* repeat flag n times */
/* IF XSHORT */
#define SHORT_X_IS_POS	0x10 /* the short vector is positive */
/* ELSE */
#define NEXT_X_IS_ZERO	0x10 /* the relative x coordinate is zero */
/* ENDIF */
/* IF YSHORT */
#define SHORT_Y_IS_POS	0x20 /* the short vector is positive */
/* ELSE */
#define NEXT_Y_IS_ZERO	0x20 /* the relative y coordinate is zero */
/* ENDIF */
/* 0x40, 0x80 are reserved */


/* #define FLIPTEST */
#ifdef FLIPTEST
/*
 *
 */
static void FlipContourDirection(GlyphClass *glyph)
{
	short	ctr, j;
	short	*oox = 	glyph->oox;
	short	*ooy = 	glyph->ooy;
	uint8 	*onCurve = glyph->onCurve;

    for ( ctr = 0; ctr < glyph->contourCount; ctr++ ) {
	 	short	flips, start, end;
	 	
	 	start	= glyph->sp[ctr];
	 	end		= glyph->ep[ctr];

        flips = (short)((end - start)/2);
        start++;
        for ( j = 0; j < flips; j++ ) {
			int16	tempX, tempY;
			uint8	pointType;
            int16   indexA = (int16)(start + j);
            int16   indexB = (int16)(end   - j);

	 		tempX				= oox[indexA];
	 		tempY				= ooy[indexA];
	 		pointType			= onCurve[indexA];
	 		
	 		oox[indexA]			= oox[indexB];
	 		ooy[indexA]			= ooy[indexB];
	 		onCurve[indexA]		= onCurve[indexB];

	 		oox[indexB]			= tempX;
	 		ooy[indexB]			= tempY;
	 		onCurve[indexB]		= pointType;
        }
    }
}
#endif


/*
 *
 */
static GlyphClass *New_GlyphClass( tsiMemObject *mem, register InputStream *in, char readHints, int16 lsb, uint16 aw, int16 tsb, uint16 ah )
{
    register int i;
    register short *oox, *ooy;
    register uint8 *onCurve;
    register int pointCount, contourCount;
    GlyphClass *t = (GlyphClass *) tsi_FastAllocN( mem, sizeof( GlyphClass ), T2K_FB_GLYPH );
	
	t->mem				= mem;
	t->sp	= t->ep		= NULL;
	t->componentData	= NULL;
	t->x = t->y 		= NULL;
	/* oox = ooy			= NULL; */
	
	contourCount			= ReadInt16( in );
	t->contourCount			= (int16)contourCount;
	t->xmin 				= ReadInt16( in );
	t->ymin 				= ReadInt16( in );
	t->xmax 				= ReadInt16( in );
	t->ymax 				= ReadInt16( in );
	pointCount 				= 0;
 	t->componentSize		= 0;
 	t->hintLength 			= 0;
 	t->hintFragment			= NULL;
 	
 	t->colorPlaneCount		= 0;
 	t->colorPlaneCountMax	= 0;
#ifdef ENABLE_T2KE
 	t->colors				= NULL;
#endif
	t->curveType			= 2;
 	t->contourCountMax		= 0;
 	t->pointCountMax		= 0;
    assert( contourCount != 0 ); /* should never happen */
    if ( contourCount < 0 ) {
        /* Composite Glyph */
        short flags;
        int weHaveInstructions;
        register short *componentData;
        register long componentSize = 0;

 		t->componentSizeMax		= 1024;
 		componentData			= (short *) tsi_AllocMem( t->mem, t->componentSizeMax * sizeof(short) );
        do {
             if ( componentSize >= t->componentSizeMax - 10 ) {
                 /* Reallocate */
                 t->componentSizeMax += t->componentSizeMax/2;
                 componentData = (short *) tsi_ReAllocMem( t->mem, componentData, t->componentSizeMax * sizeof(short) );
                 assert( componentData != NULL );
             }
             flags = ReadInt16( in );
             weHaveInstructions = (flags & WE_HAVE_INSTRUCTIONS) != 0;
             componentData[ componentSize++] = flags;
             componentData[ componentSize++] = ReadInt16( in ); /* The glyph index */
             if ( (flags & ARG_1_AND_2_ARE_WORDS) != 0 ) {
                 /* arg 1 and 2 */
                 componentData[ componentSize++] = ReadInt16( in );
                 componentData[ componentSize++] = ReadInt16( in );
             } else {
                 /* arg 1 and 2 as bytes */
                  componentData[ componentSize++] = ReadInt16( in );
             }

             if ( (flags & WE_HAVE_A_SCALE) != 0 ) {
                 /* scale */
                   componentData[ componentSize++] = ReadInt16( in );
             } else if ( (flags & WE_HAVE_AN_X_AND_Y_SCALE) != 0 ) {
                 /* xscale, yscale */
                    componentData[ componentSize++] = ReadInt16( in );
                   componentData[ componentSize++] = ReadInt16( in );
             } else if ( (flags & WE_HAVE_A_TWO_BY_TWO) != 0 ) {
                 /* xscale, scale01, scale10, yscale */
                    componentData[ componentSize++] = ReadInt16( in );
                   componentData[ componentSize++] = ReadInt16( in );
                    componentData[ componentSize++] = ReadInt16( in );
                   componentData[ componentSize++] = ReadInt16( in );
             }
         } while ( (flags & MORE_COMPONENTS) != 0 );
         t->hintLength = 0;
         if ( weHaveInstructions ) {
            t->hintLength = ReadInt16( in );
            if ( readHints ) {
                if ( t->hintLength > 0 ) {
                    t->hintFragment = (uint8 *) tsi_FastAllocN( t->mem, t->hintLength * sizeof( uint8 ), T2K_FB_HINTS );
                    ReadSegment( in, t->hintFragment, t->hintLength );
                }
            } else {
                in->pos += t->hintLength; t->hintLength = 0;
            }
         }
        AllocGlyphPointMemory( t, pointCount );
        oox = t->oox; ooy = t->ooy; /* onCurve = t->onCurve;*/
        t->componentData = componentData;
        t->componentSize = componentSize;
    } else if ( contourCount > 0 ) {
        uint8 flag;
        short stmp = 0;
        /* Regular Glyph */
        if ( contourCount <= T2K_CTR_BUFFER_SIZE ) {
            t->sp = t->ctrBuffer;
            t->ep = &t->sp[T2K_CTR_BUFFER_SIZE];
        } else {
            t->sp = (short *) tsi_AllocMem( t->mem, sizeof(short) * 2 * contourCount);
            t->ep   = &t->sp[contourCount];
        }
        for ( i = 0; i < contourCount; i++ ) {
		    t->sp[i]	= stmp;
            t->ep[i]    = stmp = ReadInt16( in );
             stmp++;
        }
        pointCount = stmp;
        assert( pointCount > 0 );

        t->hintLength = ReadInt16( in );
        if ( readHints ) {
            if ( t->hintLength > 0 ) {
                t->hintFragment = (uint8 *) tsi_FastAllocN( t->mem, t->hintLength * sizeof( uint8 ), T2K_FB_HINTS );
                ReadSegment( in, t->hintFragment, t->hintLength );
            }
        } else {
            in->pos += t->hintLength; t->hintLength = 0;
        }

        AllocGlyphPointMemory( t, pointCount );
        oox = t->oox; ooy = t->ooy; onCurve = t->onCurve;

	 	t->contourCountMax		= (int16)contourCount;
	 	t->pointCountMax		= (int16)pointCount;
        for ( i = 0; i < pointCount; ) {
            onCurve[i++] = flag = ReadUnsignedByteMacro(in);
            if ( (flag & REPEAT_FLAGS) != 0 ) {
                register int j;
                for ( j = ReadUnsignedByteMacro(in); j-- > 0 && i < pointCount; ) {
                    onCurve[i++] = flag;
                }
            }
        }

        /* Do x */
        stmp = 0;
        for ( i = 0; i < pointCount; i++ ) {
            flag = onCurve[i];
             if ( (flag & XSHORT) != 0 ) {
                 if ( (flag & SHORT_X_IS_POS) != 0 ) {
                     stmp = (short)(stmp + ReadUnsignedByteMacro(in));
                 } else {
                     stmp = (short)(stmp - ReadUnsignedByteMacro(in));
                 }
             } else if ( (flag & NEXT_X_IS_ZERO) == 0 ) {
                 /* stmp = (short)(stmp + ReadInt16( in )); */
                 register uint16 delta  = ReadUnsignedByteMacro( in ); delta <<= 8;
                 delta |= ReadUnsignedByteMacro( in );
                 stmp = (int16)(stmp + (int16)delta);
             }
            oox[i] = stmp;
        }
        /* Do y and onCurve */
        stmp = 0;
        for ( i = 0; i < pointCount; i++ ) {
            flag = onCurve[i];
            onCurve[i] = (uint8)(flag & ONCURVE);
             if ( (flag & YSHORT) != 0 ) {
                 if ( (flag & SHORT_Y_IS_POS) != 0 ) {
                     stmp = (short)(stmp + ReadUnsignedByteMacro(in));
                 } else {
                     stmp = (short)(stmp - ReadUnsignedByteMacro(in));
                 }
             } else if ( (flag & NEXT_Y_IS_ZERO) == 0 ) {
                 /* stmp = (short)(stmp + ReadInt16( in )); */
                 register uint16 delta  = ReadUnsignedByteMacro( in ); delta <<= 8;
                 delta |= ReadUnsignedByteMacro( in );
                 stmp = (int16)(stmp + (int16)delta);
             }
            ooy[i] = stmp;
        }
        assert( onCurve != NULL );
        /* tsi_DeAllocMem( t->mem, flags ); */
#ifdef WILL_MESS_UP_COMPOSITES
        if ( t->xmin != lsb ) {
            short error = (short)(t->xmin - lsb);
            for ( i = 0; i < pointCount; i++ ) {
                oox[i] = (short)(oox[i] - error);
            }
            t->xmin = lsb; t->xmax = (short)(t->xmax - error);
        }
#endif /* WILL_MESS_UP_COMPOSITES */
    } else {
        t->pointCount = 0;
        return t; /*****/
    }
    ooy[pointCount + 0] = 0;
    oox[pointCount + 0] = (short)(t->xmin - lsb); /* (t->xmin - lsb); */

    ooy[pointCount + 1] = 0;
    oox[pointCount + 1] = (short)(oox[pointCount + 0] + aw); /* (short)(oox[pointCount + 0] + aw); */

#if SbPtCount >= 4
    {
        long xMid = (oox[pointCount + 0] + oox[pointCount + 1]) >> 1;
        ooy[pointCount + 2] = (short)(t->ymax + tsb);
        oox[pointCount + 2] = (short)xMid;

        ooy[pointCount + 3] = (short)(ooy[pointCount + 2] - ah);
        oox[pointCount + 3] = (short)xMid;
    }
#else
tsb;ah;
#endif

    t->pointCount = (short)pointCount;
    /* printf("contourCount = %ld, pointCount = %ld\n", (long)t->contourCount, (long)t->pointCount ); */

#ifdef FLIPTEST
    FlipContourDirection( t );
#endif

    return t; /*****/
}

#ifdef ENABLE_WRITE
/*
 *
 */
static long Write_GlyphClass( GlyphClass *t, OutputStream *out )
{
    long i, j;
    int lastPoint = t->pointCount - 1;
    short x, y, delta;
    uint8 *f;

    if ( t == NULL )  return SizeOutStream(out); /*****/
    if ( t->contourCount == 0 && t->componentSize == 0 ) return SizeOutStream(out); /*****/ /* Empty glyph */
    WriteInt16( out, (int16)(t->componentSize > 0 ? COMPONENTCTRCOUNT : t->contourCount) );

    /* bbox */
    if ( t->componentSize == 0 ) {
        t->xmin = t->xmax = t->oox[0];
        t->ymin = t->ymax = t->ooy[0];
        for ( i = 1; i <= lastPoint; i++ ) {
            x = t->oox[i];
            y = t->ooy[i];
            if ( x > t->xmax ) {
                t->xmax = x;
            } else if ( x < t->xmin  ) {
                t->xmin = x;
            }
            if ( y > t->ymax ) {
                t->ymax = y;
            } else if ( y < t->ymin  ) {
                t->ymin = y;
            }
        }
    }

    WriteInt16( out, t->xmin ); WriteInt16( out, t->ymin );
    WriteInt16( out, t->xmax ); WriteInt16( out, t->ymax );

    if ( t->componentSize > 0 ) {
#ifdef OLD
        for ( i = 0; i < t->componentSize; i++ ) {
            WriteInt16( out, t->componentData[i] );
        }

        if ( t->hintLength > 0 ) {
            WriteInt16( out, t->hintLength );
            Write( out, t->hintFragment, t->hintLength );
        }
#endif
        /* Composite Glyph */
        int16 flags;
         i = 0;
         do {
             flags = t->componentData[i++];
             if ( t->hintLength > 0 ) {
                 flags |= WE_HAVE_INSTRUCTIONS;
             } else {
                 flags &= ~WE_HAVE_INSTRUCTIONS;
             }
             WriteInt16( out, flags );

             WriteInt16( out, t->componentData[ i++ ] ); /* glyphIndex */
             if ( (flags & ARG_1_AND_2_ARE_WORDS) != 0 ) {
                 /* arg 1 and 2 */
                 WriteInt16( out, t->componentData[ i++ ] );
                 WriteInt16( out, t->componentData[ i++ ] );
             } else {
                 /* arg 1 and 2 as bytes */
                 WriteInt16( out, t->componentData[ i++ ] );
             }

             if ( (flags & WE_HAVE_A_SCALE) != 0 ) {
                 /* scale */
                 WriteInt16( out, t->componentData[ i++ ] );
             } else if ( (flags & WE_HAVE_AN_X_AND_Y_SCALE) != 0 ) {
                 /* xscale, yscale */
                 WriteInt16( out, t->componentData[ i++ ] );
                 WriteInt16( out, t->componentData[ i++ ] );
             } else if ( (flags & WE_HAVE_A_TWO_BY_TWO) != 0 ) {
                 /* xscale, scale01, scale10, yscale */
                 WriteInt16( out, t->componentData[ i++ ] );
                 WriteInt16( out, t->componentData[ i++ ] );
                 WriteInt16( out, t->componentData[ i++ ] );
                 WriteInt16( out, t->componentData[ i++ ] );
             }
         } while ( (flags & MORE_COMPONENTS) != 0 );
         assert( i == t->componentSize );
         if ( t->hintLength > 0 ) {
             WriteInt16( out, (int16)t->hintLength );
            Write( out, t->hintFragment, t->hintLength );
         }


        /* out.flush(); */
        return SizeOutStream(out); /*****/
    }


    for ( i = 0; i < t->contourCount; i++ ) {
        WriteInt16( out, t->ep[i] );
    }

    WriteInt16( out, (int16)t->hintLength );
    Write( out, t->hintFragment, t->hintLength );


    f = (uint8 *)tsi_AllocMem( t->mem, t->pointCount * sizeof(uint8) );
    /* Calculate flags */
    x = y = 0;
    for ( i = 0; i <= lastPoint; i++ ) {
        uint8 bitFlags = (uint8)(t->onCurve[i] ? ONCURVE : 0);

        delta = (short)(t->oox[i] - x); x = (short)t->oox[i];
        if ( delta == 0 ) {
            bitFlags |= NEXT_X_IS_ZERO;
        } else if ( delta >= -255 && delta <= 255 ) {
            bitFlags |= XSHORT;
            if ( delta > 0 ) {
                bitFlags |= SHORT_X_IS_POS;
            }
        }

        delta = (short)(t->ooy[i] - y); y = (short)t->ooy[i];
        if ( delta == 0 ) {
            bitFlags |= NEXT_Y_IS_ZERO;
        } else if ( delta >= -255 && delta <= 255 ) {
            bitFlags |= YSHORT;
            if ( delta > 0 ) {
                bitFlags |= SHORT_Y_IS_POS;
            }
        }
        f[i] = bitFlags;
    }

    /* Write out bitFlags */
    for ( i = 0; i <= lastPoint;) {
        short repeat = 0;
        for ( j = i+1; j <= lastPoint && f[i] == f[j] && repeat < 255; j++ ) {
            repeat++;
        }
        if ( repeat > 1 ) {
            WriteUnsignedByte( out, (uint8)(f[i] | REPEAT_FLAGS) );
            WriteUnsignedByte( out, (uint8)repeat );
            i = j;
        } else {
            WriteUnsignedByte( out, f[i++] );
        }
    }

    /* Write out X */
    x = 0;
    for ( i = 0; i <= lastPoint; i++ ) {
        delta = (short)(t->oox[i] - x); x = (short)t->oox[i];
        if ( (f[i] & XSHORT) != 0 ) {
            if ( (f[i] & SHORT_X_IS_POS) == 0 ) delta = (short)-delta;
            WriteUnsignedByte( out, (uint8)delta );
        } else if ( (f[i] & NEXT_X_IS_ZERO ) == 0 ) {
            WriteInt16( out, delta );
        }
    }

    /* Write out Y */
    y = 0;
    for ( i = 0; i <= lastPoint; i++ ) {
        delta = (short)(t->ooy[i] - y); y = (short)t->ooy[i];
        if ( (f[i] & YSHORT) != 0 ) {
            if ( (f[i] & SHORT_Y_IS_POS) == 0 ) delta = (short)-delta;
            WriteUnsignedByte( out, (uint8)delta );
        } else if ( (f[i] & NEXT_Y_IS_ZERO ) == 0 ) {
            WriteInt16( out, delta );
        }
    }
    assert( f != NULL );
    tsi_DeAllocMem( t->mem, f );
    /* out.flush(); */
    return SizeOutStream(out); /*****/
}

#endif /* ENABLE_WRITE */

#ifdef NOT_NEEDED
static GlyphClass *Clone_GlyphClass( GlyphClass *base )
{
	GlyphClass *t = tsi_AllocMem( mem, sizeof( GlyphClass ) );
    int pointCount = base->pointCount;
	
	
	t->mem				= base->mem;
	t->sp	= t->ep		= NULL;
	onCurve				= NULL;
	t->componentData	= NULL;
	t->x = t->y 		= NULL;
	

	t->contourCount			= ReadInt16( in );
	t->xmin 				= ReadInt16( in );
	t->ymin 				= ReadInt16( in );
	t->xmax 				= ReadInt16( in );
	t->ymax 				= ReadInt16( in );
	pointCount 				= 0;
 	t->componentSize		= 0;
 	t->hintLength 			= 0;
 	t->hintFragment			= NULL;

    t->sp = tsi_AllocMem( t->mem, sizeof(short) * 2 * t->contourCount);
    t->ep   = &t->sp[t->contourCount];

	t->oox				= tsi_AllocMem( t->mem, (pointCount+SbPtCount) * ( 2 * sizeof(short) + sizeof(uint8)) );
	t->ooy				= &oox[ pointCount+SbPtCount ];
	t->onCurve			= (uint8 *)&ooy[ pointCount+SbPtCount ];

     return t;
}
#endif


/***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/
/***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/
uint16 GetUPEM( sfntClass *t)
{
    assert( t != NULL );

    if ( t->upem == 0 ) {
        uint16 upem = 2048;

#ifdef ENABLE_T1
        if ( t->T1 != NULL ) {
            upem = (uint16)t->T1->upem;
        } else
#endif
#ifdef ENABLE_CFF
        if ( t->T2 != NULL ) {
            upem = (uint16)t->T2->upem;
        } else
#endif
#ifdef ENABLE_PFR
        if ( t->PFR != NULL ) {
            upem = (uint16)t->PFR->upem;
        } else
#endif
#ifdef ENABLE_PCL
        if ( t->PCLeo != NULL ) {
            upem = (uint16)t->PCLeo->upem;
        } else
#endif
#ifdef ENABLE_SPD
        if ( t->SPD != NULL ) {
            upem = (uint16)t->SPD->outlineRes;
        } else
#endif
        if ( t->head != NULL ) {
            upem = (uint16)t->head->unitsPerEm;
        }
        t->upem = upem;
    }

    return t->upem; /*****/
}

sfnt_DirectoryEntry *GetTableDirEntry_sfntClass( sfntClass *t, long tag ) {
    long i;
    for ( i = 0; i < t->offsetTable0->numOffsets; i++ ) {
        if ( t->offsetTable0->table[i]->tag == tag ) {
            return t->offsetTable0->table[i]; /*****/
        }
    }
    return NULL; /******/
}

/* caller need to do Delete_InputStream on the stream */
InputStream *GetStreamForTable( sfntClass *t, long tag  )
{
    InputStream *stream = NULL;
    sfnt_DirectoryEntry *dirEntry;

    dirEntry = GetTableDirEntry_sfntClass( t, tag );
    if ( dirEntry != NULL ) {
        stream = New_InputStream2( t->mem, t->in, dirEntry->offset, dirEntry->length, 0, NULL );
    }
    return stream; /*****/
}

#ifdef ENABLE_PCLETTO
static void GetTTOffsetAndLength(InputStream *in, unsigned long *offset, unsigned long *length);
#endif
#ifdef ENABLE_PCLETTO
static void GetTTOffsetAndLength(InputStream *in, unsigned long *offset, unsigned long *length)
{
uint8 aBuffer[10];
unsigned long curLen = in->maxPos, count;
    Seek_InputStream( in, 0 );
    ReadSegment( in, (uint8 *)&aBuffer[0], (long)(count=3) );
    do
    {
        aBuffer[0] = ReadUnsignedByteMacro(in);
        count++;
    } while (aBuffer[0] != 'W');
    Seek_InputStream( in, 0 );
    *offset = count/* to beginning of 86 byte PCL header*/
                 + 86 /* size of PCL header */
                 + 4; /* past GTxx */
    *length = curLen - *offset;
    return;
}
#endif


static void CacheKeyTables_sfntClass( sfntClass *t, InputStream *in, int32 logicalFontNumber )
{
    InputStream *stream;
    sfnt_DirectoryEntry *dirEntry;

    Delete_ttcfClass( t->ttcf );
    Delete_sfnt_OffsetTable( t->offsetTable0);
	Delete_headClass( t->head );	t->head = NULL;
	Delete_hheaClass( t->hhea );	t->hhea = NULL;
	Delete_hheaClass( t->vhea );	t->vhea = NULL;
	Delete_hmtxClass( t->hmtx);		t->hmtx = NULL;
	Delete_hmtxClass( t->vmtx);		t->vmtx = NULL;
	Delete_maxpClass( t->maxp );
	Delete_locaClass( t->loca );	t->loca = NULL;
#ifdef ENABLE_T2KS
	FF_Delete_slocClass( t->sloc );	t->sloc = NULL;
	FF_Delete_ffstClass( t->ffst );	t->ffst = NULL;
	FF_Delete_ffhmClass( t->ffhm );	t->ffhm = NULL;
#endif
#ifdef ENABLE_KERNING
    Delete_kernClass( t->kern );
#endif
#ifdef ENABLE_GASP_TABLE_SUPPORT
	Delete_gaspClass( t->gasp );
#endif
#ifdef ENABLE_SBIT
    Delete_blocClass( t->bloc );
    Delete_ebscClass( t->ebsc );
#endif


    /* The initial optional collection data */
    t->ttcf = New_ttcfClass( t->mem, in);

    if ( t->ttcf != NULL ) {
        t->numberOfLogicalFonts = (long)t->ttcf->directoryCount;
        assert( logicalFontNumber >=0 && logicalFontNumber < (int32)t->ttcf->directoryCount );
        Seek_InputStream( in, t->ttcf->tableOffsets[ logicalFontNumber ] );
    }
    /* The initial offset table */
    t->offsetTable0 = New_sfnt_OffsetTable( t->mem, in );

    /* The head table */
    dirEntry = GetTableDirEntry_sfntClass( t, tag_FontHeader );
#ifdef ENABLE_SBIT
    if ( dirEntry == NULL ) {
        dirEntry = GetTableDirEntry_sfntClass( t, tag_BFontHeader ); /* Try alternative form */
    }
#endif /* ENABLE_SBIT */
    if ( dirEntry != NULL ) {
        stream = New_InputStream2( t->mem, in, dirEntry->offset, dirEntry->length, 0, NULL );
        t->head = New_headClass( t->mem, stream );
        Delete_InputStream( stream, NULL );
        #ifdef VERBOSE
            Print_headClass( t->head );
        #endif
    }

    /* The hhea table */
    dirEntry = GetTableDirEntry_sfntClass( t, tag_HoriHeader );
    if ( dirEntry != NULL ) {
        stream = New_InputStream2( t->mem, in, dirEntry->offset, dirEntry->length , 0, NULL );
        t->hhea = New_hheaClass( t->mem, stream );
        Delete_InputStream( stream, NULL );
        #ifdef VERBOSE
            Print_hheaClass( t->hhea );
        #endif
    }
    /* The vhea table */
    dirEntry = GetTableDirEntry_sfntClass( t, tag_VertHeader );
    if ( dirEntry != NULL ) {
        stream = New_InputStream2( t->mem, in, dirEntry->offset, dirEntry->length , 0, NULL );
        t->vhea = New_hheaClass( t->mem, stream );
        Delete_InputStream( stream, NULL );
        #ifdef VERBOSE
            Print_hheaClass( t->vhea );
        #endif
    }

    /* The maxp table */
    dirEntry = GetTableDirEntry_sfntClass( t, tag_MaxProfile );
    stream = New_InputStream2( t->mem, in, dirEntry->offset, dirEntry->length, 0, NULL );
    t->maxp = New_maxpClass( t->mem, stream );
    Delete_InputStream( stream, NULL );
    #ifdef VERBOSE
        Print_maxpClass( t->maxp );
    #endif


    /* The loca table */
    dirEntry = GetTableDirEntry_sfntClass( t, tag_IndexToLoc );
    if ( dirEntry != NULL ) {
        stream = New_InputStream2( t->mem, in, dirEntry->offset, dirEntry->length, 0, NULL );
        t->loca = New_locaClass( t->mem, stream, t->head->indexToLocFormat, (long)dirEntry->length );
        Delete_InputStream( stream, NULL );
        #ifdef VERBOSE
            Print_locaClass( t->loca );
        #endif
    }

#ifdef ENABLE_T2KS
    /* The sloc table */
    dirEntry = GetTableDirEntry_sfntClass( t, tag_sloc );
    if ( dirEntry != NULL ) {
        /* We can not use a separate stream since the slocClass saves an offset for the stream. */
        /* stream = New_InputStream2( t->mem, in, dirEntry->offset, dirEntry->length, 0, NULL ); */
        Seek_InputStream( in, dirEntry->offset );
        t->sloc= FF_New_slocClass( t->mem, in );
        /* Delete_InputStream( stream, NULL ); */
    }


    /* The ffst table */
    dirEntry = GetTableDirEntry_sfntClass( t, tag_ffst );
    if ( dirEntry != NULL ) {
        stream = New_InputStream2( t->mem, in, dirEntry->offset, dirEntry->length, 0, NULL );
        t->ffst = FF_New_ffstClass( t->mem, stream, dirEntry->length );
        Delete_InputStream( stream, NULL );
    } else {
        t->ffst = FF_New_ffstClass( t->mem, NULL, 0 );
    }

    /* The ffhm table */
    dirEntry = GetTableDirEntry_sfntClass( t, tag_ffhm );
    if ( dirEntry != NULL ) {
        stream = New_InputStream2( t->mem, in, dirEntry->offset, dirEntry->length, 0, NULL );
        t->ffhm = FF_New_ffhmClass( t->mem, stream  );
        Delete_InputStream( stream, NULL );
    }
#endif

    /* The hmtx table */
    dirEntry = GetTableDirEntry_sfntClass( t, tag_HorizontalMetrics );
    if ( dirEntry != NULL && t->hhea != NULL ) {
        stream = New_InputStream2( t->mem, in, dirEntry->offset, dirEntry->length, 0, NULL );
        t->hmtx = New_hmtxClass( t->mem, stream, GetNumGlyphs_sfntClass(t), t->hhea->numberOfHMetrics );
        Delete_InputStream( stream, NULL );
        if ( t->StyleMetricsFunc != NULL ) {
            t->StyleMetricsFunc( t->hmtx, t->mem, (short)GetUPEM( t ), t->params );
        }
        #ifdef VERBOSE
            Print_hmtxClass( t->hmtx );
        #endif
    }

    /* The vmtx table */
    dirEntry = GetTableDirEntry_sfntClass( t, tag_VerticalMetrics );
    if ( dirEntry != NULL && t->vhea != NULL ) {
        stream = New_InputStream2( t->mem, in, dirEntry->offset, dirEntry->length, 0, NULL );
        t->vmtx = New_hmtxClass( t->mem, stream, GetNumGlyphs_sfntClass(t), t->vhea->numberOfHMetrics );
        Delete_InputStream( stream, NULL );
#ifdef SOON
        if ( false && t->StyleMetricsFunc != NULL ) {
            t->StyleMetricsFunc( t->vmtx, t->mem, (short)GetUPEM( t ), t->params );
        }
#endif
        #ifdef VERBOSE
            Print_hmtxClass( t->vmtx );
        #endif
    }




#ifdef ENABLE_KERNING
	/* The kern table */
	dirEntry = GetTableDirEntry_sfntClass( t, tag_Kerning );
	
	/* HACK to avoid unknown skia kerning format, added 1/14/98 */
	if ( dirEntry != NULL && GetTableDirEntry_sfntClass( t, tag_fvar ) != NULL ) {
		dirEntry = NULL; /* pretend we have no kern table */
	}
	
	t->kern = NULL;
	if ( dirEntry != NULL && dirEntry->length > 0 ) {
		stream = New_InputStream2( t->mem, in, dirEntry->offset, dirEntry->length, 0, NULL );
		t->kern = New_kernClass( t->mem, stream );
		Delete_InputStream( stream, NULL );
		#ifdef ENABLE_PRINTF
			/* Print_kernClass( t->kern ); */
		#endif
	}

#endif

#ifdef ENABLE_GASP_TABLE_SUPPORT
	/* The gasp table */
	dirEntry = GetTableDirEntry_sfntClass( t, tag_Gasp );
	t->gasp = NULL;
	if ( dirEntry != NULL && dirEntry->length > 0 ) {
		stream = New_InputStream2( t->mem, in, dirEntry->offset, dirEntry->length, 0, NULL );
		t->gasp = New_gaspClass( t->mem, stream );
		Delete_InputStream( stream, NULL );
	}

#endif

#ifdef ENABLE_ORION
	/* Delete_OrionModelClass( (OrionModelClass *)t->model ); */
#endif	
#ifdef ENABLE_ORION
    if ( t->model == NULL ) {
        dirEntry = GetTableDirEntry_sfntClass( t, tag_T2KC );
        if ( dirEntry != NULL ) {
            stream = New_InputStream2( t->mem, in, dirEntry->offset, dirEntry->length, 0, NULL );
            t->model = (void *)New_OrionModelClassFromStream( t->mem, stream );
            Delete_InputStream( stream, NULL );
        }
    }
#endif

#ifdef ENABLE_SBIT
    t->bdatOffset = 0;
    t->bloc = NULL;
    t->ebsc = NULL;
    dirEntry = GetTableDirEntry_sfntClass( t, tag_EBLC );
    if ( dirEntry == NULL ) {
        dirEntry = GetTableDirEntry_sfntClass( t, tag_bloc );
    }
    if ( dirEntry != NULL ) {
        /* do not use New_InputStream2 here */
        Seek_InputStream( in, dirEntry->offset );
        t->bloc = New_blocClass( t->mem, t->loca == NULL, in );

        dirEntry = GetTableDirEntry_sfntClass( t, tag_EBDT );
        if ( dirEntry == NULL ) {
            dirEntry = GetTableDirEntry_sfntClass( t, tag_bdat );
        }
        if ( dirEntry != NULL ) {
            F16Dot16 version;

            Seek_InputStream( in, dirEntry->offset );
            version = (F16Dot16)ReadInt32( in );
            if ( version >= 0x00020000 && version < 0x00030000 ) { /* Ok we know this format */
                t->bdatOffset = dirEntry->offset;
            }
        }
    }
    dirEntry = GetTableDirEntry_sfntClass( t, tag_EBSC );
    if ( dirEntry != NULL ) {
        stream = New_InputStream2( t->mem, in, dirEntry->offset, dirEntry->length, 0, NULL );
        t->ebsc = New_ebscClass( t->mem, stream );
        Delete_InputStream( stream, NULL );
    }
#endif

#ifdef ENABLE_NATIVE_TT_HINTS
    if ( t->head == NULL ||
        (t->head->glyphDataFormat >= 2000 &&
         t->head->glyphDataFormat <= 2002) ) {
        ; /* Not TrueType */ /* Added 10/18/99 --- Sampo */
        assert( t->t2kTT == NULL );
    } else {
        t->t2kTT = New_T2KTTClass( t->mem, in, (void *)t );
    }
#endif

    /* Get the UnderLineInfo here */
    dirEntry = GetTableDirEntry_sfntClass( t, tag_post );
    if ( dirEntry != NULL ) {
        Seek_InputStream( in, dirEntry->offset + 8 );
		t->post_underlinePosition	= ReadInt16( in );
		t->post_underlineThickness	= ReadInt16( in );
        t->isFixedPitch = (uint32)ReadInt32(in);
    } else {
		t->post_underlinePosition	= 0;
		t->post_underlineThickness	= 0;
        t->isFixedPitch = 0;
    }

    /* Get the usFirstCharIndex, usLastCharIndex here */
    dirEntry = GetTableDirEntry_sfntClass( t, tag_OS_2 );
    if ( dirEntry != NULL ) {
        Seek_InputStream( in, dirEntry->offset + 64 );
		t->firstCharCode	= (uint16)ReadInt16( in );
		t->lastCharCode	= (uint16)ReadInt16( in );
	} else {
		t->firstCharCode	= 0;
		t->lastCharCode		= 0xffff;
    }
}


void GetTTNameProperty( sfntClass *font, uint16 languageID, uint16 nameID, uint8 **p8, uint16 **p16 )
{
    InputStream *stream;
    sfnt_DirectoryEntry *dirEntry;
    uint8 *p;
	uint16 platFormID	= font->preferedPlatformID;
	uint16 encodingID 	= font->preferedPlatformSpecificID;
	InputStream *in		= font->in;
	
	*p8 	= NULL;
	*p16 	= NULL;
    dirEntry = GetTableDirEntry_sfntClass( font, tag_NamingTable );
    if ( dirEntry != NULL ) {
        uint16 n, i;
        uint32 offset;
        uint16 stringStorageOffset;
        uint16 stringOffset;
        uint16 stringLength;
        int is16Bit = 0;

        stream = New_InputStream2( font->mem, in, dirEntry->offset, dirEntry->length, 0, NULL );

        ReadInt16( stream ); /* Format selector should be 0 */
		n 				= (uint16)ReadInt16( stream );
		stringStorageOffset	= (uint16)ReadInt16( stream );

        /* Scan through the name records */
        for ( i = 0; i < n; i++ ) {
            offset = Tell_InputStream( stream );
            if ( (uint16)ReadInt16( stream ) == platFormID &&
                 (uint16)ReadInt16( stream ) == encodingID &&
                 (uint16)ReadInt16( stream ) == languageID &&
			     (uint16)ReadInt16( stream ) == nameID			) {
                /*** We found it !!! ***/
                stringLength = (uint16)ReadInt16( stream );
                stringOffset = (uint16)ReadInt16( stream );

                Seek_InputStream( stream, (unsigned long)(stringStorageOffset + stringOffset) );
                p = (uint8 *)tsi_AllocMem( font->mem, (unsigned long)(stringLength + 2) );
                if ( (stringLength & 1) == 0 ) {
                    i = stringLength; if (i > 10) i = 10;
                    /* even number of bytes, could be a 16 bit string... */
                    while ( i-- > 0 ) {
                        uint8 c = ReadUnsignedByteMacro( stream );
                        if ( c == 0 ) {
                            is16Bit = true;
                            break; /*****/
                        }
                    }
                    Seek_InputStream( stream, (unsigned long)(stringStorageOffset + stringOffset) );
                }
                if ( is16Bit ) {
                    int index = 0;
                    uint16 *s16;
                    *p16 = s16 = (uint16 *)p;
                    for ( i = 0; i < stringLength; i++, i++ /* i+=2 */ ) {
                        s16[index++] = (uint16)ReadInt16( stream );
                    }

                } else {
                    *p8 = p;
                    for ( i = 0; i < stringLength; i++ ) {
                        p[i] = ReadUnsignedByteMacro( stream );
                    }
                }
                p[i++] = 0;
                p[i] = 0; /* NULL terminate */
                break; /*****/ /* Done, break out of the loop */
            }
            Seek_InputStream( stream, offset + 6*2 );
        }
        Delete_InputStream( stream, NULL );
    }
}





long GetNumGlyphs_sfntClass( sfntClass *t )
{
    long n;
#ifdef ENABLE_T1
    if ( t->T1 != NULL ) {
        return t->T1->NumCharStrings; /******/
    }
#endif
#ifdef ENABLE_CFF
    if ( t->T2 != NULL ) {
        return t->T2->NumCharStrings; /******/
    }
#endif
#ifdef ENABLE_PFR
    if ( t->PFR != NULL ) {
        return t->PFR->NumCharStrings; /******/
    }
#endif
#ifdef ENABLE_SPD
    if ( t->SPD != NULL ) {
        return t->SPD->NumLayoutChars; /******/
    }
#endif
#ifdef ENABLE_PCL
    if ( t->PCLeo != NULL ) {
        return t->PCLeo->NumCharStrings; /******/
    }
#endif
    assert( t->maxp != NULL );
    n = t->maxp->numGlyphs;
    if ( t->loca != NULL ) {
        if ( t->loca->n <= n ) { /* Added 6/29/98 --- Sampo */
            n = t->loca->n - 1;
        }
    }
    return n; /******/
}

#ifdef ALGORITHMIC_STYLES
#include "shapet.h"
#endif

/*
 *
 */
GlyphClass *GetGlyphByIndex( sfntClass *t, long index, char readHints, uint16 *aWidth, uint16 *aHeight )
{
    GlyphClass *glyph;
    sfnt_DirectoryEntry *dirEntry;
#if defined (ENABLE_NATIVE_T1_HINTS) && defined (ENABLE_T1)
    long PPEm;
#endif
	*aWidth		= 0;		/* Initialize */
	*aHeight	= 0;


#ifdef ENABLE_T1
    if ( t->T1 != NULL ) {

#ifdef ENABLE_NATIVE_T1_HINTS
        if ((t->xPPEm != -1) && (t->yPPEm != -1))
        {
            if (t->xPPEm < t->yPPEm)
                PPEm = t->xPPEm;
            else
                PPEm = t->yPPEm;
        }

        if (readHints != 0)
        {
            glyph = tsi_T1GetGlyphByIndex( t->T1, (uint16)index, aWidth, aHeight, t->ffhint, PPEm );
            if (glyph->pointCount == 0)
                FFT1HintClass_releaseMem(t->ffhint);
        }
        else
            glyph = tsi_T1GetGlyphByIndex( t->T1, (uint16)index, aWidth, aHeight, NULL , -1);
#else
        glyph = tsi_T1GetGlyphByIndex( t->T1, (uint16)index, aWidth, aHeight, NULL, -1 );
#endif



#		ifdef ALGORITHMIC_STYLES
			if ( t->StyleFunc != NULL ) {
				t->StyleFunc( glyph, t->mem, (short)GetUPEM( t ), t->params );
			}
#		endif
    } else
#endif
#ifdef ENABLE_CFF
    if ( t->T2 != NULL ) {
        glyph = tsi_T2GetGlyphByIndex( t->T2, (uint16)index, aWidth, aHeight );
#		ifdef ALGORITHMIC_STYLES
			if ( t->StyleFunc != NULL ) {
				t->StyleFunc( glyph, t->mem, (short)GetUPEM( t ), t->params );
			}
#		endif
    } else
#endif
#ifdef ENABLE_T2KE
    if ( t->T2KE != NULL && t->head->glyphDataFormat == 2001 ) {
        glyph = tsi_T2KEGetGlyphByIndex( t->T2KE, (uint16)index, aWidth, aHeight );
#		ifdef ALGORITHMIC_STYLES
			if ( t->StyleFunc != NULL ) {
				t->StyleFunc( glyph, t->mem, (short)GetUPEM( t ), t->params );
			}
#		endif
    } else
#endif
#ifdef ENABLE_PFR
    if ( t->PFR != NULL) {

#ifdef ENABLE_NATIVE_T1_HINTS
        if (readHints != 0)
        {
            glyph = tsi_PFRGetGlyphByIndex( t->PFR, (uint16)index, aWidth, aHeight ,t->ffhint);
            if (glyph->pointCount == 0)
                FFT1HintClass_releaseMem(t->ffhint);
        }
        else
            glyph = tsi_PFRGetGlyphByIndex( t->PFR, (uint16)index, aWidth, aHeight , NULL);
#else
        glyph = tsi_PFRGetGlyphByIndex( t->PFR, (uint16)index, aWidth, aHeight, NULL );
#endif


#		ifdef ALGORITHMIC_STYLES
			if ( t->StyleFunc != NULL ) {
				t->StyleFunc( glyph, t->mem, (short)GetUPEM( t ), t->params );
			}
#		endif
    } else
#endif
#ifdef ENABLE_SPD
    if ( t->SPD != NULL) {
        glyph = tsi_SPDGetGlyphByIndex( t->SPD, (uint16)index, aWidth, aHeight );
#		ifdef ALGORITHMIC_STYLES
			if ( t->StyleFunc != NULL ) {
				t->StyleFunc( glyph, t->mem, (short)GetUPEM( t ), t->params );
			}
#		endif
    } else
#endif
#ifdef ENABLE_PCL
    if ( t->PCLeo != NULL) {
        glyph = tsi_PCLGetGlyphByIndex( t->PCLeo, (uint16)index, aWidth, aHeight );
#		ifdef ALGORITHMIC_STYLES
			if ( t->StyleFunc != NULL ) {
				t->StyleFunc( glyph, t->mem, (short)GetUPEM( t ), t->params );
			}
#		endif
    } else
#endif
#ifdef ENABLE_T2KS
    if ( t->head->glyphDataFormat == 2002 ) {
        glyph = ff_New_GlyphClassT2KS( t, NULL, index, aWidth, aHeight, NULL, 0 );

#		ifdef ALGORITHMIC_STYLES
			if ( t->StyleFunc != NULL ) {
				t->StyleFunc( glyph, t->mem, (short)GetUPEM( t ), t->params );
			}
#		endif
    } else
#endif
    {
    dirEntry  = GetTableDirEntry_sfntClass( t, tag_GlyphData );

    if ( dirEntry != NULL && t->loca != NULL && (t->hmtx != NULL || t->vmtx != NULL) &&
         index >= 0 && index < t->numGlyphs ) {
        InputStream *stream;
        unsigned long offset1 = t->loca->offsets[index];
        unsigned long offset2 = t->loca->offsets[index+1];
        unsigned long length = offset2 - offset1;
        uint16 aw, ah;
        int16 lsb, tsb;

        if ( t->hmtx != NULL ) {
            aw  = t->hmtx->aw[index];
			lsb	= t->hmtx->lsb[index];
        } else {
            aw  = 2048;
            lsb = 0;
        }
        ah = 0; tsb = 0; /* Initialize */
        if ( t->vmtx != NULL ) {
            ah  = t->vmtx->aw[index];
			tsb	= t->vmtx->lsb[index];
        } else if ( t->hhea != NULL ) {
            /* Simulate it ! */
            ah  = (uint16)(t->hhea->Ascender - t->hhea->Descender + t->hhea->LineGap);
            tsb = (int16)(ah/10);
        }

        if ( offset2 > offset1  ) {
            stream = New_InputStream2( t->mem, t->in, dirEntry->offset + offset1, length, T2K_FB_IOSTREAM, NULL );
            /* printf("index = %ld, offset = %ld length = %ld\n", index, offset1, length ); */
            if ( t->head->glyphDataFormat == 2000 ) {
                glyph = New_GlyphClassT2K( t->mem, stream, readHints, lsb, aw, tsb, ah, t->model );
            } else {
#ifdef USE_SEAT_BELTS
				tsi_Assert( t->mem, t->head->glyphDataFormat == 0, T2K_BAD_FONT );
#else
				assert( t->head->glyphDataFormat == 0 );
#endif
                glyph = New_GlyphClass( t->mem, stream, readHints, lsb, aw, tsb, ah );
            }

            Delete_InputStream( stream, NULL );
            assert( glyph != NULL );
#ifdef ALGORITHMIC_STYLES
            if ( t->StyleFunc != NULL ) {
                t->StyleFunc( glyph, t->mem, (short)GetUPEM( t ), t->params );
            }
#endif
        } else {
            /* No outlines... We land here for the space character */
            glyph = New_EmptyGlyph( t->mem, lsb, aw, tsb, ah );
        }
        *aWidth  = aw;
        *aHeight = ah;
    } else {
#		ifdef ENABLE_PCLETTO
            HPXL_MetricsInfo_t XLMetricsInfo;
            int success, errCode;
            uint8 *ppCharData;
			uint16	nothing, size1;
			uint16 aw, ah;
			int16 lsb, tsb;
			InputStream *stream;
			
			success = tt_get_char_data(	(long) index,
                        0x0008 /*T2K_CODE_IS_GINDEX*/,
                        &ppCharData,
                        &size1,
                        &nothing,
						(HPXL_MetricsInfo_t	*)&XLMetricsInfo
                    );
            if (success)
            {/* go for it, baby! */
                if (XLMetricsInfo.lsbSet)
                    lsb = (int16)XLMetricsInfo.lsb;
                else if ( t->hmtx != NULL )
					lsb	= t->hmtx->lsb[index];
                else
                    lsb = 0;

                ah = 0; tsb = 0; /* Initialize */
                if (XLMetricsInfo.tsbSet && XLMetricsInfo.awSet)
                    ah = XLMetricsInfo.aw;
                else if ( t->vmtx != NULL )
                    ah  = t->vmtx->aw[index];
                else if ( t->hhea != NULL )
                    /* Simulate it ! */
                    ah  = (uint16)(t->hhea->Ascender - t->hhea->Descender + t->hhea->LineGap);
                if (XLMetricsInfo.tsbSet)
                    tsb = (int16)XLMetricsInfo.tsb;
                else if ( t->vmtx != NULL )
					tsb	= t->vmtx->lsb[index];
                else if ( t->hhea != NULL )
                    /* Simulate it ! */
                    tsb = (int16)(ah/10);
                if (XLMetricsInfo.awSet)
                    aw = XLMetricsInfo.aw;
                else if ( t->hmtx != NULL )
                    aw  = t->hmtx->aw[index];
                else
                    aw  = 2048;
                /* metrics all set! */
                *aWidth  = aw;
                *aHeight = ah;
                if ( ppCharData  ) {
                    stream = New_InputStream3( t->mem, ppCharData, size1, &errCode );
                    /* printf("index = %ld, offset = %ld length = %ld\n", index, offset1, length ); */
                    {
                        assert( t->head->glyphDataFormat == 0 );
                        glyph = New_GlyphClass( t->mem, stream, readHints, lsb, aw, tsb, ah );
                    }

                    Delete_InputStream( stream, NULL );
                    assert( glyph != NULL );
#			ifdef ALGORITHMIC_STYLES
					if ( t->StyleFunc != NULL ) {
						t->StyleFunc( glyph, t->mem, (short)GetUPEM( t ), t->params );
					}
#			endif
                }
            }
            else
            {
            uint16 aw = 0, ah = 0;
            int16 lsb = 0, tsb = 0;
                if ( t->hhea != NULL )
                {
                    ah  = (uint16)(t->hhea->Ascender - t->hhea->Descender + t->hhea->LineGap);
                    tsb = (int16)(ah/10);
					aw	= (uint16)(t->hhea->advanceWidthMax / 4);
                    lsb = (int16)(t->hhea->minLeftSideBearing);
                }
                glyph = New_EmptyGlyph( t->mem, lsb, aw, tsb, ah);
            }
#		else
        /* printf("index = %ld\n", index ); */
        glyph = New_EmptyGlyph( t->mem, 0, 0, 0, 0);
        /* assert( false ); */
#		endif
    }

    }
    glyph->myGlyphIndex = (uint16)index;
    /*
    glyph->x = NULL;
    glyph->y = NULL;
    */
#ifndef T2K_SCALER
    ResequenceContoursAndPoints( glyph, true );
#endif
    return glyph; /*****/
}

void ff_LoadCMAP( sfntClass *t )
{
    if ( t->cmap == NULL ) {
        InputStream *stream;
        sfnt_DirectoryEntry *dirEntry;

		if ( (dirEntry = GetTableDirEntry_sfntClass( t, tag_CharToIndexMap )) != NULL ) {
            /* Set up input stream to cmap */
            stream = New_InputStream2( t->mem, t->in, dirEntry->offset, dirEntry->length, 0, NULL );
            /* Create new cmap class */
            t->cmap = New_cmapClass( t->mem, t->preferedPlatformID, t->preferedPlatformSpecificID, stream );
            /* Store max number of glyphs in cmapClass */
            t->cmap->numGlyphs = t->numGlyphs;
            /* Delete cmap stream */
            Delete_InputStream( stream, NULL );
        }
    }
}

uint16 SfntClassPSNameTocharCode( sfntClass *t, char *PSName )
{
#if !defined ENABLE_T1 && !defined ENABLE_CFF
	UNUSED(t);
	UNUSED(PSName);
#endif
#ifdef ENABLE_T1
    if ( t->T1 != NULL ) {
		return tsi_T1PSName2CharCode( t->T1, PSName);
    }
#endif
#ifdef ENABLE_CFF
    if ( t->T2 != NULL ) {
		return tsi_T2PSName2CharCode( t->T2, PSName);
    }
#endif
	return 0;	/* got here? you get .notdef */
}

uint16 GetSfntClassGlyphIndex( sfntClass *t, uint16 charCode )
{

#ifdef ENABLE_T1
    if ( t->T1 != NULL ) {
        return tsi_T1GetGlyphIndex( t->T1, charCode );
    }
#endif
#ifdef ENABLE_CFF
    if ( t->T2 != NULL ) {
        return tsi_T2GetGlyphIndex( t->T2, charCode );
    }
#endif
#ifdef ENABLE_PFR
    if ( t->PFR != NULL ) {
        return tsi_PFRGetGlyphIndex( t->PFR, charCode );
    }
#endif
#ifdef ENABLE_SPD
    if ( t->SPD != NULL ) {
        return tsi_SPDGetGlyphIndex( t->SPD, charCode );
    }
#endif
#ifdef ENABLE_PCL
    if ( t->PCLeo != NULL ) {
        return tsi_PCLGetGlyphIndex( t->PCLeo, charCode );
    }
#endif

    /* assert( t->cmap != NULL ); */
    ff_LoadCMAP( t );
    if (t->cmap)
        return Compute_cmapClass_GlyphIndex( t->cmap, (uint16)charCode );
#ifdef ENABLE_PCLETTO
    else
    {/* get the gIndex via callback to application layer */
        HPXL_MetricsInfo_t XLMetricsInfo;
        int success;
        uint8 *ppCharData;
        uint16    glyphIndex, size1;

        success = tt_get_char_data(    (long) charCode,
                    0x0000 /* !T2K_CODE_IS_GINDEX, charCode is high-level code */,
                    &ppCharData,
                    &size1,
                    &glyphIndex,
                    (HPXL_MetricsInfo_t    *)&XLMetricsInfo
                );
        if (success)
            return glyphIndex;
        else
            return 0xffff;
    }
#endif
    return 0;
}



int IsFigure( sfntClass *t, uint16 gIndex )
{

#ifdef ENABLE_T1
    if ( t->T1 != NULL ) {
        uint16 charCode = t->T1->charCode[gIndex];
        return ( charCode >= '0' && charCode <= '9' ); /*****/
    }
#endif
#ifdef ENABLE_CFF
    if ( t->T2 != NULL ) {
        /* SOON */
        return false; /*****/
    }
#endif
#ifdef ENABLE_PFR
    if ( t->PFR != NULL ) {
        uint16 charCode = t->PFR->charMap[gIndex].charCode;
        return ( charCode >= '0' && charCode <= '9' ); /*****/
    }
#endif
#ifdef ENABLE_SPD
    if ( t->SPD != NULL ) {
        uint16 charCode = t->SPD->spdIndexToLoc[gIndex].charID;
        return ( charCode >= '0' && charCode <= '9' ); /*****/
    }
#endif
#ifdef ENABLE_PCL
    if ( t->PCLeo != NULL ) {
        uint8 *p;
        uint16 byteCount, charCode, nothing;
		eo_get_char_data(	(long) gIndex,
                        0x0008/* T2K_CODE_IS_GINDEX */,
                        (uint8 **)&p,
                        (uint16 *)&byteCount,
                        (uint16 *)&charCode,
                        (uint16 *)&nothing);
		return ( charCode >= '0' && charCode <= '9' );	/*****/
    }
#endif

    ff_LoadCMAP( t );
    return IsFigure_cmapClass( t->cmap, gIndex ); /*****/
}



static void SetStyling( sfntClass *t, T2K_AlgStyleDescriptor *styling )
{
    int i;
    if ( styling != NULL ) {
        assert( styling->StyleFunc != NULL );
        t->StyleFunc = styling->StyleFunc;
        t->StyleMetricsFunc = styling->StyleMetricsFunc;
        for ( i = 0; i < MAX_STYLE_PARAMS; i++ ) {
            t->params[i] = styling->params[i];
        }
    } else {
        t->StyleFunc = NULL;
        t->StyleMetricsFunc = NULL;
    }
}

#ifdef ENABLE_WRITE
static void CopyStyling( sfntClass *dst, sfntClass *src )
{
    int i;

    dst->StyleFunc = src->StyleFunc;
    dst->StyleMetricsFunc = src->StyleMetricsFunc;
    for ( i = 0; i < MAX_STYLE_PARAMS; i++ ) {
        dst->params[i] = src->params[i];
    }
}
#endif /* ENABLE_WRITE */

/*
 * for use with FF_Set_GetAWFuncPtr_Reference1
 */
static uint16 Get_hmtx_AW( void *param1, uint16 gIndex )
{
    register hmtxClass *hmtx = (hmtxClass *)param1;
    return hmtx->aw[gIndex];
}

/*
 * for use with FF_Set_GetAWFuncPtr_Reference1
 */
static uint16 Get_Cached_AW( void *param1, uint16 gIndex )
{
    uint16 width, key = (uint16)(gIndex % FF_AW_CACHE_SIZE);
    register sfntClass *sfnt = (sfntClass *)param1;

    /* See if in the cache */
    if ( sfnt->awCache_hashKey == NULL ) {
        register int i;
        register uint16 *p;
        sfnt->awCache_hashKey = p = (uint16 *)tsi_AllocMem( sfnt->mem, FF_AW_CACHE_SIZE * 2 * sizeof(uint16) );
        sfnt->awCache_aw      = &sfnt->awCache_hashKey[ FF_AW_CACHE_SIZE ];
        for ( i = 0; i < FF_AW_CACHE_SIZE; i++ ) {
            p[i] = 0xffff; /* same as sfnt->awCache_hashKey[i] = 0xffff; */
        }
        assert( sfnt->GetAWParam2 != NULL );
    } else {
        if ( sfnt->awCache_hashKey[key] == gIndex ) {
            return sfnt->awCache_aw[key]; /******/
        }
    }
    ;
    /* If not then call GetAWFuncPtr2 */
    width = sfnt->GetAWFuncPtr2( sfnt->GetAWParam2, gIndex );
    /* Put into the cache */
	sfnt->awCache_hashKey[key] 	= gIndex;
	sfnt->awCache_aw[key] 		= width;
    return width; /*****/
}

/*
 * for use with FF_Set_GetAWFuncPtr_Reference1
 */
static uint16 Get_Upem_Width( void *param1, uint16 gIndex )
{
    register sfntClass *sfnt = (sfntClass *)param1;
    UNUSED( gIndex );
    return sfnt->upem; /*****/
}



/*
 * OLD:
 *
sfntClass *New_sfntClassLogical( tsiMemObject *mem, short fontType, int32 fontNum, InputStream *in, T2K_AlgStyleDescriptor *styling, int *errCode )
 */

/*
 * Creates a new font (sfntClass) object.
 * Parameters:
 * mem: 		A pointer to tsiMemObject
 * fontType:	Type of font.
 * fontNum:		Logical font number.
 * in1:			The primary InputStream.
 * in2:			The seconday InputStream. This is normally == NULL.
 * styling:		An function pointer to a function that modifies the outlines algorithmically. This is normally == NULL.
 * errCode:		The returned errorcode.
 */
sfntClass *FF_New_sfntClass( tsiMemObject *mem, short fontType, int32 fontNum, InputStream *in1, InputStream *in2,
                             T2K_AlgStyleDescriptor *styling, int *errCode )
{
    sfntClass *t;
#ifdef ENABLE_PCLETTO
    InputStream *stream;
#endif
    assert( mem != NULL );
    assert( in1 != NULL );

    if ( errCode == NULL || (*errCode = setjmp(mem->env)) == 0 ) {
        /* try */
        t = (sfntClass *) tsi_AllocMem( mem, sizeof( sfntClass ) );
		t->mem		= mem;
		t->in 		= in1;
		t->in2 		= in2;
		t->out		= NULL;
		
		/* Initialize the width caching fields. */
		t->GetAWFuncPtr1 	= NULL;
		t->GetAWParam1		= NULL;
		t->GetAWFuncPtr2	= NULL;
		t->GetAWParam2		= NULL;
		t->awCache_hashKey	= NULL;
		t->awCache_aw		= NULL;

        t->numberOfLogicalFonts = 1; /* Initialize. */
        t->offsetTable0 = NULL;
        t->head = NULL;
        t->hhea = NULL;
        t->vhea = NULL;
        t->hmtx = NULL;
        t->vmtx = NULL;
        t->maxp = NULL;
        t->loca = NULL;
#ifdef ENABLE_T2KS
		t->sloc	= NULL;
		t->ffst	= NULL;
		t->ffhm	= NULL;
#endif
        t->cmap = NULL;
        t->kern = NULL;
		t->gasp = NULL;

        t->model = NULL;
#ifdef ENABLE_SBIT
        t->bloc = NULL;
        t->ebsc = NULL;
#endif
        t->upem = 0;
        SetStyling( t, styling );
        t->globalHintsCache = NULL; /* Used/Allocated/Deallocated by the T2K scaler, sort of weird but there are reasons */
#ifdef ENABLE_T1
        t->T1 = NULL;
#endif
#ifdef ENABLE_CFF
        t->T2 = NULL;
#endif
#ifdef ENABLE_T2KE
        t->T2KE = NULL;
#endif
#ifdef ENABLE_PFR
        t->PFR = NULL;
#endif
#ifdef ENABLE_SPD
        t->SPD = NULL;
#endif
#ifdef ENABLE_PCL
        t->PCLeo = NULL;
#endif
#ifdef ENABLE_NATIVE_TT_HINTS
        t->t2kTT = NULL;
#endif


#ifdef ENABLE_NATIVE_T1_HINTS
        t->ffhint = NULL;
#endif

        t->xPPEm = -1;
        t->yPPEm = -1;

        t->ttcf = NULL;
        t->xScale = 0;
        t->yScale = 0;
        t->useNativeHints = false;

        if ( fontType == FONT_TYPE_TT_OR_T2K  || fontType == FONT_TYPE_PCLETTO ) {
#			ifdef ENABLE_PCLETTO
            if (fontType == FONT_TYPE_PCLETTO)
            {
            unsigned long offset, length;
                GetTTOffsetAndLength(t->in, &offset, &length);
                stream = New_InputStream2( t->mem, t->in, offset, length, 0, NULL );
                CacheKeyTables_sfntClass( t, stream, fontNum );
                Delete_InputStream( stream, NULL );
            }
            else
#			endif
                CacheKeyTables_sfntClass( t, t->in, fontNum );
#		ifdef ENABLE_T2KE
            if ( t->head->glyphDataFormat == 2001 ) {
                t->T2KE = tsi_NewT2KEClass( t, t->in, fontNum );
            }
#		endif
#ifdef ENABLE_T1
        } else if ( fontType == FONT_TYPE_1 ) {
            /* Fix tsi_NewT1Class to accept InputStream... */
            /* t->T1 = tsi_NewT1Class( mem, in->base, in->maxPos ); */
            t->T1 = tsi_NewT1Class( mem, GetEntireStreamIntoMemory(in1), SizeInStream(in1) );

#ifdef ENABLE_NATIVE_T1_HINTS
            t->ffhint = New_FFT1HintClass(mem, (short)GetUPEM( t ));
#endif

#ifdef ENABLE_KERNING
            if (in2 != NULL)
            {
                t->kern = New_T1kernClass(t->T1,  mem, GetEntireStreamIntoMemory(in2), SizeInStream(in2) );
                if (((kernSubTable0Data *)(t->kern)->table[0]->kernData)->nPairs == 0)
                {
                 Delete_kernClass(t->kern);
                 t->kern = NULL;
                }
            }
#endif

            FF_Set_GetAWFuncPtr_Reference2( t, FF_GetAW_T1Class, t->T1  );
            if ( t->StyleMetricsFunc != NULL ) {
                t->StyleMetricsFunc( t->hmtx, t->mem, (short)GetUPEM( t ), t->params );
            }
            t->isFixedPitch = (uint32)t->T1->isFixedPitch;
            t->firstCharCode = t->T1->firstCharCode;
            t->lastCharCode = t->T1->lastCharCode;
#endif
#ifdef ENABLE_CFF
        } else if ( fontType == FONT_TYPE_2 ) {
            t->T2 = tsi_NewCFFClass( mem, t->in, fontNum ); /* last argument is the fontNumber */


            FF_Set_GetAWFuncPtr_Reference2( t, FF_GetAW_CFFClass, t->T2  );
            if ( t->StyleMetricsFunc != NULL ) {
                t->StyleMetricsFunc( t->hmtx, t->mem, (short)GetUPEM( t ), t->params );
            }
            t->isFixedPitch = (uint32)t->T2->isFixedPitch;
            t->firstCharCode = t->T2->firstCharCode;
            t->lastCharCode = t->T2->lastCharCode;
#endif
#ifdef ENABLE_PCL
        } else if (fontType == FONT_TYPE_PCL) {
            t->PCLeo = tsi_NewPCLClass( mem, t->in, fontNum ); /* last argument is the fontNumber */


            t->hmtx = NULL;
/*			t->PCLeo->hmtx = NULL; */
            FF_Set_GetAWFuncPtr_Reference2( t, FF_GetAW_PCLClass, t->PCLeo  );
            t->firstCharCode = t->PCLeo->firstCharCode;
            t->lastCharCode = t->PCLeo->lastCharCode;
            t->isFixedPitch = (uint32)t->PCLeo->isFixedPitch;
#endif
#ifdef ENABLE_PFR
        } else if (fontType == FONT_TYPE_PFR) {
            t->PFR = tsi_NewPFRClass( mem, t->in, fontNum ); /* last argument is the fontNumber */


            t->kern = t->PFR->kern;
            t->hmtx = t->PFR->hmtx;
            t->PFR->hmtx = NULL;
              t->numberOfLogicalFonts = (long)t->PFR->directoryCount;
            t->firstCharCode = t->PFR->firstCharCode;
            t->lastCharCode = t->PFR->lastCharCode;
            t->isFixedPitch = (uint32)t->PFR->isFixedPitch;
            if ( t->StyleMetricsFunc != NULL ) {
                t->StyleMetricsFunc( t->hmtx, t->mem, (short)GetUPEM( t ), t->params );
            }

#ifdef ENABLE_NATIVE_T1_HINTS
            if (t->ffhint == NULL)
            t->ffhint = New_FFT1HintClass(mem, (short)GetUPEM( t ));
#endif
#endif
#ifdef ENABLE_SPD
        } else if (fontType == FONT_TYPE_SPD) {
            t->SPD = tsi_NewSPDClass( mem, t->in, fontNum ); /* last argument is the fontNumber */


            t->hmtx = t->SPD->hmtx;
            t->SPD->hmtx = NULL;
            t->kern = t->SPD->kern;
              t->numberOfLogicalFonts = 1;
            t->firstCharCode = t->SPD->firstCharCode;
            t->lastCharCode = t->SPD->lastCharCode;
            t->isFixedPitch = (uint32)t->SPD->isFixedPitch;
            if ( t->StyleMetricsFunc != NULL ) {
                t->StyleMetricsFunc( t->hmtx, t->mem, (short)GetUPEM( t ), t->params );
            }
#endif
        } else {
            assert( false );
        }

        /* If a reader does not set hmtx, then it should typically instead do FF_Set_GetAWFuncPtr_Reference2 instead!!! */
        if ( t->hmtx != NULL ) {
            FF_Set_GetAWFuncPtr_Reference1( t, Get_hmtx_AW, t->hmtx );
        } else {
            FF_Set_GetAWFuncPtr_Reference1( t, Get_Cached_AW, t  );
        }
        assert( t->GetAWFuncPtr1 != NULL || t->GetAWFuncPtr2 != NULL );

        t->numGlyphs = GetNumGlyphs_sfntClass( t );
#ifdef ENABLE_T2KS
        if ( (t->head != NULL) && (t->head->glyphDataFormat == 2002) ) {
        /* Initialize if we have a stroke font */
        if ( t->ffhm != NULL ) {
            FF_Set_GetAWFuncPtr_Reference2( t, FF_GetAW_ffhmClass, t->ffhm  );
        } else if (t->GetAWFuncPtr2 == NULL && t->hmtx != NULL) {
            FF_Set_GetAWFuncPtr_Reference1( t, Get_Upem_Width, t  ); /* override Get_Cached_AW */
            }
        }
        t->currentCoordinate[0] = ONE16Dot16/2;
        t->currentCoordinate[1] = ONE16Dot16/2;
#endif

        /* longjmp( t->mem->env, 9999 ); */
    } else {
        /* catch */
        tsi_EmergencyShutDown( mem );
        t = (sfntClass *)NULL;
    }
    return t; /*****/
}

#ifdef ENABLE_WRITE
uint8 *GetTTFPointer( sfntClass *sfnt )
{
    /* return sfnt->out ? sfnt->out->base : sfnt->in->base; */
    return sfnt->out ? sfnt->out->base : GetEntireStreamIntoMemory( sfnt->in );
}


uint32 GetTTFSize( sfntClass *sfnt )
{
    return (uint32)(sfnt->out ? SizeOutStream( sfnt->out ) : SizeInStream( sfnt->in ));
}
#endif /* ENABLE_WRITE */

#ifdef ENABLE_WRITE
void CalculateNewCheckSums( sfntClass *sfnt )
{
    long i, j;
    InputStream *table;
    uint8 *base = GetTTFPointer( sfnt );
    assert( base != NULL );

    /* assumes head.checkSumAdjustment == 0L; */
    for ( i = 0; i < sfnt->offsetTable0->numOffsets; i++) {
		long offset	= (long)sfnt->offsetTable0->table[i]->offset;
		long length	= (long)sfnt->offsetTable0->table[i]->length;
		uint32 ck	= 0;
		length 		+= 3; length &= ~3;

        table = New_InputStream3( sfnt->mem, &base[offset], (unsigned long)length, NULL );
        for ( j = 0; j < length; j += 4 ) {
            ck += (uint32)ReadInt32( table );
        }
        sfnt->offsetTable0->table[i]->checkSum = (int32)ck;
        Delete_InputStream( table, NULL );
    }
}
#endif /* ENABLE_WRITE */

#ifdef ENABLE_WRITE

/* Result to be put into head.checkSumAdjustment */
long CalculateCheckSumAdjustment( sfntClass *sfnt )
{

    InputStream *ttf;
	uint32 checkSumAdjustment, sum	= 0;
    uint8 *base = GetTTFPointer( sfnt );
    uint32 j, length = GetTTFSize( sfnt );

    ttf = New_InputStream3( sfnt->mem, base, length, NULL);
    for ( j = 0; j < length; j += 4 ) {
        sum += (uint32)ReadInt32( ttf );
    }
    Delete_InputStream( ttf, NULL );
    checkSumAdjustment = 0x0b1b0afba - sum;
    return (long)checkSumAdjustment; /*****/
}
#endif /* ENABLE_WRITE */

#ifdef ENABLE_WRITE
static void HandleSbits( sfntClass *sfnt0, sfntClass *sfnt1 )
{
    char buffer[256];

    if ( GetTableDirEntry_sfntClass( sfnt0, tag_EBLC ) != NULL ||
         GetTableDirEntry_sfntClass( sfnt0, tag_bloc ) != NULL ||
         GetTableDirEntry_sfntClass( sfnt0, tag_EBDT ) != NULL ||
         GetTableDirEntry_sfntClass( sfnt0, tag_bdat ) != NULL ||
         GetTableDirEntry_sfntClass( sfnt0, tag_EBSC ) != NULL ) {


        printf("**************\n");
        printf("The font contains embedded bitmaps!\n");
        printf("Please answer Y or N followed by <CR>.\n");
        do {
            printf("Should I go ahead and delete them ?\n");
            buffer[0] = 0;
            scanf("%s", buffer );
            assert( strlen(buffer) < 100 );
        } while ( buffer[0] != 'Y' && buffer[0] != 'y' && buffer[0] != 'N' && buffer[0] != 'n' );

        if ( buffer[0] == 'Y' || buffer[0] == 'y' ) {
            DeleteTable( sfnt0->offsetTable0, tag_EBLC );
            DeleteTable( sfnt1->offsetTable0, tag_EBLC );
            DeleteTable( sfnt0->offsetTable0, tag_bloc );
            DeleteTable( sfnt1->offsetTable0, tag_bloc );
            DeleteTable( sfnt0->offsetTable0, tag_EBDT );
            DeleteTable( sfnt1->offsetTable0, tag_EBDT );
            DeleteTable( sfnt0->offsetTable0, tag_bdat );
            DeleteTable( sfnt1->offsetTable0, tag_bdat );

            DeleteTable( sfnt0->offsetTable0, tag_EBSC );
            DeleteTable( sfnt1->offsetTable0, tag_EBSC );
        }

        printf("**************\n");
    }
}
#endif /* ENABLE_WRITE */


#ifdef ENABLE_WRITE
/*
 *
 */
sfntClass *New_sfntClass2( sfntClass *sfnt0, int cmd, int param )
{
    int err;
    ag_HintHandleType hintHandle;
    ag_FontCategory fontType;
    ag_GlobalDataType gData;
    unsigned char *fpgm, *ppgm;
    short *cvt;
    long fpgmLength, ppgmLength, cvtCount;
    int hint = 1;
    char xWeightIsOne;
    long offset, length, pos = 0;
    long headIndex, locaIndex, hmtxIndex, maxpIndex, i;
    uint32 checkSumAdjustmentOffset = 0;
    sfntClass *sfnt1 = (sfntClass *)tsi_AllocMem( sfnt0->mem, sizeof( sfntClass ) );
    int ppem = 0;

    assert( sfnt1 != NULL );
    sfnt1->mem = sfnt0->mem;
    sfnt1->upem = 0;
#ifdef ENABLE_NATIVE_TT_HINTS
    sfnt1->t2kTT = NULL;
#endif
#ifdef ENABLE_T1
    sfnt1->T1 = NULL;
#endif
#ifdef ENABLE_CFF
    sfnt1->T2 = NULL;
#endif
#ifdef ENABLE_T2KE
    sfnt1->T2KE = NULL;
#endif
#ifdef ENABLE_PFR
    sfnt1->PFR = NULL;
#endif
#ifdef ENABLE_SPD
    sfnt1->SPD = NULL;
#endif
    sfnt1->ttcf = NULL;
    assert( sfnt1->mem != NULL );
    if ( cmd == CMD_HINT_ROMAN || cmd == CMD_HINT_OTHER || cmd == CMD_TT_TO_T2K || cmd == CMD_T2K_TO_TT || cmd == CMD_TT_TO_T2KE ) {
#ifndef ENABLE_AUTO_HINTING
        assert( false );
#endif
        ;
    } else if ( cmd == CMD_GRID ) {
#ifndef ENABLE_AUTO_GRIDDING
        assert( false );
#endif
        ppem = param;
    }
    headIndex = locaIndex = hmtxIndex = maxpIndex = -1;

    sfnt1->xScale = 0;
    sfnt1->yScale = 0;
    sfnt1->useNativeHints = false;

    /* Initialize the width caching fields. */
	sfnt1->GetAWFuncPtr1 	= NULL;
	sfnt1->GetAWParam1		= NULL;
	sfnt1->GetAWFuncPtr2	= NULL;
	sfnt1->GetAWParam2		= NULL;
	sfnt1->awCache_hashKey	= NULL;
	sfnt1->awCache_aw		= NULL;

	sfnt1->in		= NULL;
	sfnt1->out		= New_OutputStream( sfnt1->mem, 100L * 1000L ); /* Set to intelligent size later... */
    sfnt1->offsetTable0 = NULL;
    sfnt1->head = NULL;
    sfnt1->hhea = NULL;
    sfnt1->vhea = NULL;
    sfnt1->hmtx = NULL;
    sfnt1->vmtx = NULL;
    sfnt1->maxp = NULL;
    sfnt1->loca = NULL;
    sfnt1->cmap = NULL;
    sfnt1->kern = NULL;
	sfnt1->gasp = NULL;

#ifdef ENABLE_SBIT
    sfnt1->bloc = NULL;
    sfnt1->ebsc = NULL;
    sfnt1->bdatOffset = 0;
#endif

    sfnt1->model = sfnt0->model; sfnt0->model = NULL;

    assert( sfnt0->in != NULL );

    CopyStyling( sfnt1, sfnt0 );


    Rewind_InputStream( sfnt0->in );
    CacheKeyTables_sfntClass( sfnt1, sfnt0->in, 0 );

err = ag_HintInit( sfnt1->mem, (short)(sfnt1->maxp->maxPoints + 2), (short)sfnt0->head->unitsPerEm, &hintHandle );
assert( err == 0 );
fontType = cmd == CMD_HINT_ROMAN ? ag_ROMAN : ag_KANJI;
if ( cmd == CMD_HINT_ROMAN ) {
    printf("Hint Roman\n");
} else if( cmd == CMD_HINT_OTHER ) {
    printf("Hint Other/Kanji\n");
}

t2k_ComputeGlobalHints( sfnt0, hintHandle, &gData, false);
/* SetGlobalDataForTms( &gData ); */
err = ag_SetHintInfo( hintHandle, &gData, fontType );
assert( err == 0 );
err = ag_SetScale( hintHandle, ppem, ppem, &xWeightIsOne );
assert( err == 0 );
#ifdef ENABLE_AUTO_HINTING
err = ag_GetGlobalHints( hintHandle, &fpgm, &fpgmLength, &ppgm, &ppgmLength, &cvt, &cvtCount );
if ( err != 0 ) {
    printf("TrueType autohinting is disabled\n");
    fpgm = ppgm = 0;
    fpgmLength = ppgmLength = 0;
}
#else
fpgm = ppgm = 0;
cvt = 0;
fpgmLength = ppgmLength = cvtCount = 0;
#endif


    CreateTableIfMissing( sfnt0->offsetTable0, tag_PreProgram );
    CreateTableIfMissing( sfnt0->offsetTable0, tag_FontProgram );
    CreateTableIfMissing( sfnt0->offsetTable0, tag_ControlValue );

    CreateTableIfMissing( sfnt1->offsetTable0, tag_PreProgram );
    CreateTableIfMissing( sfnt1->offsetTable0, tag_FontProgram );
    CreateTableIfMissing( sfnt1->offsetTable0, tag_ControlValue );

    if ( cmd == CMD_T2K_TO_TT ) {
        assert( sfnt1->head->glyphDataFormat == 2000 );
    }

    if ( cmd == CMD_TT_TO_T2K || cmd == CMD_TT_TO_T2KE ) {
        assert( sfnt1->head->glyphDataFormat == 0 );

        HandleSbits( sfnt0, sfnt1 );

        DeleteTable( sfnt0->offsetTable0, tag_PreProgram );
        DeleteTable( sfnt1->offsetTable0, tag_PreProgram );
        DeleteTable( sfnt0->offsetTable0, tag_FontProgram );
        DeleteTable( sfnt1->offsetTable0, tag_FontProgram );
        DeleteTable( sfnt0->offsetTable0, tag_ControlValue );
        DeleteTable( sfnt1->offsetTable0, tag_ControlValue );

        /* Added post and PCLT deletion on 4/14/99 */
        DeleteTable( sfnt0->offsetTable0, tag_post );
        DeleteTable( sfnt1->offsetTable0, tag_post );
        DeleteTable( sfnt0->offsetTable0, tag_PCLT );
        DeleteTable( sfnt1->offsetTable0, tag_PCLT );

        CreateTableIfMissing( sfnt0->offsetTable0, tag_T2KG );
        CreateTableIfMissing( sfnt1->offsetTable0, tag_T2KG );

        if ( sfnt1->model != NULL ) {
            CreateTableIfMissing( sfnt0->offsetTable0, tag_T2KC );
            CreateTableIfMissing( sfnt1->offsetTable0, tag_T2KC );
        }
    } else {
        DeleteTable( sfnt0->offsetTable0, tag_T2KG );
        DeleteTable( sfnt1->offsetTable0, tag_T2KG );
    }
    DeleteTable( sfnt0->offsetTable0, tag_HoriDeviceMetrics );
    DeleteTable( sfnt1->offsetTable0, tag_HoriDeviceMetrics );
    DeleteTable( sfnt0->offsetTable0, tag_VDMX );
    DeleteTable( sfnt1->offsetTable0, tag_VDMX );
    DeleteTable( sfnt0->offsetTable0, tag_LTSH );
    DeleteTable( sfnt1->offsetTable0, tag_LTSH );


    Write_sfnt_OffsetTable( sfnt1->offsetTable0, sfnt1->out );
    for ( i = 0; i < sfnt0->offsetTable0->numOffsets; i++ ) {
        offset = (long)sfnt0->offsetTable0->table[i]->offset;
        length = (long)sfnt0->offsetTable0->table[i]->length;

		sfnt1->offsetTable0->table[i]->tag		= sfnt0->offsetTable0->table[i]->tag;
		sfnt1->offsetTable0->table[i]->checkSum	= sfnt0->offsetTable0->table[i]->checkSum;
		pos										= SizeOutStream(sfnt1->out);
		sfnt1->offsetTable0->table[i]->offset	= (uint32)pos;
        /* sfnt1->offsetTable0->table[i]->length is set below */
#ifdef VERBOSE
printf("%ld *** %c%c%c%c offset = %ld, length = %ld\n", i,
                            (char)((sfnt1->offsetTable0->table[i]->tag >> 24) & 0xff),
                            (char)((sfnt1->offsetTable0->table[i]->tag >> 16) & 0xff),
                            (char)((sfnt1->offsetTable0->table[i]->tag >>  8) & 0xff),
                            (char)((sfnt1->offsetTable0->table[i]->tag >>  0) & 0xff),
                            offset, length );
#endif
        switch ( sfnt0->offsetTable0->table[i]->tag ) {
        case tag_FontHeader:
			sfnt1->head->checkSumAdjustment	 = 0; /* Zero this to calc checksums right */
			sfnt1->head->magicNumber		 = SFNT_MAGIC;
            headIndex = i; /* Write out the loca after the glyph table is done */
            /* Write_headClass( sfnt1->head, sfnt1->out ); */
            break; /*****/
        case tag_IndexToLoc:
            locaIndex = i; /* Write out the loca after the glyph table is done */
            break;/******/
        case tag_HorizontalMetrics:
            hmtxIndex = i; /* Write out the hmtx after the glyph table is done */
            break;/******/
        case tag_MaxProfile:
            maxpIndex = i;
            /* Write_maxpClass( sfnt1->maxp, sfnt1->out ); */
            break; /*****/
        case tag_GlyphData:
            {
                long i, n, startpos;
#ifdef TIME_GLYPH_DATA
                clock_t t1, t2;
                double duration;
#endif
                uint16 theAW;

                n = GetNumGlyphs_sfntClass( sfnt0 );
                startpos = SizeOutStream(sfnt1->out);
#ifdef TIME_GLYPH_DATA
                t2 = clock();
#endif
                for ( i = 0; i < n; i++ ) {
                    uint16 aHeight;
                    GlyphClass *glyph;
                    short isFigure = false;

                    glyph = GetGlyphByIndex( sfnt0, i, true, &theAW, &aHeight );
#ifdef ENABLE_T2KE
                    glyph->gIndex = (uint16)i;
#endif
                    sfnt1->loca->offsets[i] = (uint32)(SizeOutStream(sfnt1->out) - startpos);
                    if ( glyph != NULL ) {
                        ag_ElementType elem;
                        uint8 *hintFragment = NULL;
                        long hintLength = 0;

						elem.contourCount	= glyph->contourCount; 
						elem.pointCount		= glyph->pointCount;   
						elem.sp				= glyph->sp;  		
						elem.ep				= glyph->ep;  		
						elem.oox			= glyph->oox;		
						elem.ooy			= glyph->ooy;		
						elem.onCurve		= glyph->onCurve;

                        if ( glyph->hintLength > 0 ) {
                            assert( glyph->hintFragment != NULL );
                            tsi_FastDeAllocN( sfnt1->mem, (char *)glyph->hintFragment, T2K_FB_HINTS );
                            glyph->hintFragment = NULL;
                            glyph->hintLength = 0;
                        }
                        assert( sfnt1->hmtx != NULL );
                        elem.ooy[glyph->pointCount + 0] = 0;
                        elem.oox[glyph->pointCount + 0] = (short)(glyph->xmin - sfnt1->hmtx->lsb[i]);

                        elem.ooy[glyph->pointCount + 1] = 0;
                        elem.oox[glyph->pointCount + 1] = (short)(elem.oox[glyph->pointCount + 0] + sfnt1->hmtx->aw[i]);
                        if ( cmd == CMD_TT_TO_T2K || cmd == CMD_T2K_TO_TT || cmd == CMD_TT_TO_T2KE ) {
                            glyph->hintLength = 0;
                        } else if ( cmd == CMD_HINT_ROMAN  || cmd == CMD_HINT_OTHER) {
#ifdef ENABLE_AUTO_HINTING
                            if ( glyph->contourCount > 0 ) {
                                isFigure = (short)IsFigure( sfnt0, (unsigned short)i );
                                err = ag_AutoHintOutline( hintHandle, &elem, isFigure, 2, &hintFragment, &hintLength );
                                assert( err == 0 );
                                glyph->hintLength = hintLength;
                                if ( glyph->hintLength > 0 ) {
                                    glyph->hintFragment = hintFragment;
                                }
                            }
#endif
                        } else if ( cmd == CMD_GRID ) {
#ifdef ENABLE_AUTO_GRIDDING
                            if ( glyph->contourCount > 0 ) {
                                long point;
                                /* Does not use SbPtCount at this point */
                                elem.x = (long *)tsi_AllocMem( sfnt0->mem, 2 * (glyph->pointCount+2) * sizeof(long) );
                                elem.y = &elem.x[(glyph->pointCount+2)];
                                assert( elem.x != NULL );

                                isFigure = (short)IsFigure( sfnt0, (uint16)i );
                                err = ag_AutoGridOutline( hintHandle, &elem, isFigure, (short)2, false /* Assume B&W for now */ );
                                glyph->hintLength = 0;
                                assert( err == 0 );
                                /* scale =  ppem / upem * 64 */
                                for ( point = 0; point < glyph->pointCount+2; point++ ) {
                                    glyph->oox[point] = (short)(elem.x[point] * sfnt0->head->unitsPerEm / 64 / ppem);
                                    glyph->ooy[point] = (short)(elem.y[point] * sfnt0->head->unitsPerEm / 64 / ppem);
                                }

                                tsi_DeAllocMem( sfnt0->mem, elem.x );
                            }
#endif
                        }
                        if ( cmd == CMD_TT_TO_T2K ) {
                            Write_GlyphClassT2K( glyph, sfnt1->out, sfnt1->model );
                            sfnt1->head->glyphDataFormat = 2000;
                        } else if ( cmd == CMD_TT_TO_T2KE ) {
#ifdef ENABLE_T2KE
                            if ( i == 0 ) {
                                Write_GlobalFuncsT2KE( sfnt1->out );
                            }
                            Write_GlyphClassT2KE( glyph, sfnt1->out );
                            sfnt1->head->glyphDataFormat = 2001;
#else
                            assert( false );
#endif
                        } else {
                            Write_GlyphClass( glyph, sfnt1->out  );
                            sfnt1->head->glyphDataFormat = 0;
                        }
                        if ( glyph->hintLength > 0 ) {
                            assert( glyph->hintFragment != NULL );
                            tsi_FastDeAllocN( sfnt1->mem, (char *)glyph->hintFragment, T2K_FB_HINTS );
                            glyph->hintFragment = NULL;
                            glyph->hintLength = 0;
                        }
                        if ( cmd == CMD_GRID /* && glyph->contourCount > 0 */ ) {
                        /*
                            assert( (elem.x[glyph->pointCount+0] & 63) == 0 );
                            assert( (elem.x[glyph->pointCount+1] & 63) == 0 );
                        */
                            assert( sfnt1->hmtx != NULL );
                            sfnt1->hmtx->lsb[i] = (int16)(glyph->xmin - elem.oox[glyph->pointCount + 0]);
                            sfnt1->hmtx->aw[i]  = (uint16)(elem.oox[glyph->pointCount + 1] - elem.oox[glyph->pointCount + 0]);
                        }
                    }
                    if ( SizeOutStream(sfnt1->out) & 1 ) {
                        /* Word align */
                        WriteUnsignedByte( sfnt1->out, 0 );
                    }
                    Delete_GlyphClass( glyph );
                }
                sfnt1->loca->offsets[i] = (uint32)(SizeOutStream(sfnt1->out) - startpos);
#ifdef TIME_GLYPH_DATA
                t1 = clock();
                duration = ((difftime(t1,t2))/ (CLOCKS_PER_SEC));
                /* printf("Time = %f, %f c/s\n", (double)duration, (double)n/(double)duration); */
#endif
            }
            break; /*****/
        case tag_PreProgram:
            if ( hint ) {
                Write( sfnt1->out, ppgm, ppgmLength );
                /* if ( cmd == CMD_GRID ) WriteDataToFile( "NEW_PPGM.BIN", &sfnt0->in->base[offset], length );; */
                if ( cmd == CMD_GRID ) WriteDataToFile( "NEW_PPGM.BIN", GetEntireStreamIntoMemory(sfnt0->in) + offset, (uint32)length );;
            }
#ifdef OLD
            else {
                Write( sfnt1->out, &sfnt0->in->base[offset], length );
            }
#endif
            break;/******/
        case tag_FontProgram:
            if ( hint ) {
                Write( sfnt1->out, fpgm, fpgmLength );
                if ( cmd == CMD_GRID ) WriteDataToFile( "NEW_FPGM.BIN", GetEntireStreamIntoMemory(sfnt0->in) + offset, (uint32)length );;
            }
#ifdef OLD
            else {
                Write( sfnt1->out, &sfnt0->in->base[offset], length );
            }
#endif
            break;/******/
        case tag_ControlValue:
            if ( hint ) {
                Write( sfnt1->out, (unsigned char *)cvt, cvtCount*2 );
            }
#ifdef OLD
            else {
                Write( sfnt1->out, &sfnt0->in->base[offset], length );
            }
#endif
            break;/******/

        case tag_T2KG:
            t2k_WriteGHints( &gData, sfnt1->out );
            break;/******/
#ifdef ENABLE_ORION
        case tag_T2KC:
            Save_OrionModelClass( (OrionModelClass *)sfnt1->model, sfnt1->out );
            break;/******/
#endif
        default:
            Write( sfnt1->out, GetEntireStreamIntoMemory(sfnt0->in) + offset, length );
            break;/******/
        }
        /* set the length */
        sfnt1->offsetTable0->table[i]->length   = SizeOutStream( sfnt1->out ) - sfnt1->offsetTable0->table[i]->offset;
        while ( (pos = SizeOutStream( sfnt1->out )) & 3 ) {
            /* long word align */
            WriteUnsignedByte( sfnt1->out, 0 );
        }
    }
    assert( headIndex >= 0 );
    assert( locaIndex >= 0 );
    assert( hmtxIndex >= 0 );
    assert( maxpIndex >= 0 );
    /* Write out the hmtx table */
    sfnt1->offsetTable0->table[hmtxIndex]->offset = (uint32)pos;
    Write_hmtxClass( sfnt1->hmtx, sfnt1->out );
    pos = SizeOutStream( sfnt1->out );
    sfnt1->offsetTable0->table[hmtxIndex]->length = pos - sfnt1->offsetTable0->table[hmtxIndex]->offset;
    while ( (pos = SizeOutStream( sfnt1->out )) & 3 ) {
        /* long word align */
        WriteUnsignedByte( sfnt1->out, 0 );
    }

    /* Write out the maxp table */
    {
        ag_HintMaxInfoType maxInfo;

        ag_GetHintMaxInfo( hintHandle, &maxInfo );

		sfnt1->maxp->maxElement					= maxInfo.maxZones;
		sfnt1->maxp->maxTwilightPoints			= maxInfo.maxTwilightPoints;
		sfnt1->maxp->maxStorage					= maxInfo.maxStorage;
		sfnt1->maxp->maxFunctionDefs			= maxInfo.maxFunctionDefs;
		sfnt1->maxp->maxInstructionDefs			= maxInfo.maxInstructionDefs;
		sfnt1->maxp->maxStackElements			= maxInfo.maxStackElements;
        sfnt1->maxp->maxSizeOfInstructions		= maxInfo.maxSizeOfInstructions;


        sfnt1->offsetTable0->table[maxpIndex]->offset = (uint32)pos;
        Write_maxpClass( sfnt1->maxp, sfnt1->out );
        pos = SizeOutStream( sfnt1->out );
        sfnt1->offsetTable0->table[maxpIndex]->length = pos - sfnt1->offsetTable0->table[maxpIndex]->offset;
        while ( (pos = SizeOutStream( sfnt1->out )) & 3 ) {
            /* long word align */
            WriteUnsignedByte( sfnt1->out, 0 );
        }
    }

    /* Write out the loca table */
    sfnt1->offsetTable0->table[locaIndex]->offset = (uint32)pos;
    Write_locaClass( sfnt1->loca, sfnt1->out );
    pos = SizeOutStream( sfnt1->out );
    sfnt1->offsetTable0->table[locaIndex]->length = pos - sfnt1->offsetTable0->table[locaIndex]->offset;
    while ( (pos = SizeOutStream( sfnt1->out )) & 3 ) {
        /* long word align */
        WriteUnsignedByte( sfnt1->out, 0 );
    }

    /* Write out the head table */
	checkSumAdjustmentOffset 		 = (uint32)(pos + 8);
    sfnt1->head->indexToLocFormat = sfnt1->loca->indexToLocFormat; /* propagate to the head table */
    sfnt1->offsetTable0->table[headIndex]->offset = (uint32)pos;
    Write_headClass( sfnt1->head, sfnt1->out );
    pos = SizeOutStream( sfnt1->out );
    sfnt1->offsetTable0->table[headIndex]->length = pos - sfnt1->offsetTable0->table[headIndex]->offset;
    while ( (pos = SizeOutStream( sfnt1->out )) & 3 ) {
        /* long word align */
        WriteUnsignedByte( sfnt1->out, 0 );
    }




    /* Sort the table directory */
    SortTableDirectory( sfnt1->offsetTable0 );
    /* Set all the table check sums */
    CalculateNewCheckSums( sfnt1 );

    /* Write out the offset table */
    Rewind_OutputStream( sfnt1->out );
    Write_sfnt_OffsetTable( sfnt1->offsetTable0, sfnt1->out );


    /* Set the global checksum adjustment */
    {
        uint32 checkSumAdjustment = (uint32)CalculateCheckSumAdjustment( sfnt1 );
        uint8 *ttf =  GetTTFPointer( sfnt1 );

        ttf[checkSumAdjustmentOffset + 0] = (uint8)((checkSumAdjustment >> 24) & 0xff);
        ttf[checkSumAdjustmentOffset + 1] = (uint8)((checkSumAdjustment >> 16) & 0xff);
        ttf[checkSumAdjustmentOffset + 2] = (uint8)((checkSumAdjustment >>  8) & 0xff);
        ttf[checkSumAdjustmentOffset + 3] = (uint8)((checkSumAdjustment >>  0) & 0xff);
    }
    sfnt1->StyleFunc = NULL; /* No styling on this font anymore. It has already been applied */

    /* Rebuild the cached tables */
    {
        InputStream *ttf;
        uint8 *base = GetTTFPointer( sfnt1 );
        uint32 length = GetTTFSize( sfnt1 );
        ttf = New_InputStream3( sfnt1->mem, base, length, NULL);
        CacheKeyTables_sfntClass( sfnt1, ttf, 0);
        Delete_InputStream( ttf, NULL );
    }
tsi_DeAllocMem( sfnt1->mem, (char *)fpgm );
tsi_DeAllocMem( sfnt1->mem, (char *)ppgm );
tsi_DeAllocMem( sfnt1->mem, (char *)cvt );
err = ag_HintEnd( hintHandle );
assert( err == 0 );

    assert( sfnt1 != NULL );
    return sfnt1; /*****/
}
#endif /* ENABLE_WRITE */

#ifdef ENABLE_WRITE
void WriteToFile_sfntClass( sfntClass *t, const char *fname )
{
    uint8 *base  = GetTTFPointer( t );
    uint32 length = GetTTFSize( t );

    WriteDataToFile( fname, base, length );
}
#endif /* ENABLE_WRITE */



int GetMaxPoints( sfntClass *t)
{
    int result;
    assert( t != NULL );
#ifdef ENABLE_T1
    if ( t->T1 != NULL ) {
        return (int)t->T1->maxPointCount; /*****/
    }
#endif
#ifdef ENABLE_CFF
    if ( t->T2 != NULL ) {
        return (int)t->T2->maxPointCount; /*****/
    }
#endif
#ifdef ENABLE_PFR
    if ( t->PFR != NULL ) {
        return (int)t->PFR->maxPointCount; /*****/
    }
#endif
#ifdef ENABLE_SPD
    if ( t->SPD != NULL ) {
        return (int)t->SPD->maxPointCount; /*****/
    }
#endif
#ifdef ENABLE_PCL
    if ( t->PCLeo != NULL ) {
        return (int)t->PCLeo->maxPointCount; /*****/
    }
#endif
    assert( t->maxp != NULL );
    result = (t->maxp->maxPoints > t->maxp->maxCompositePoints ? t->maxp->maxPoints : t->maxp->maxCompositePoints);

    return result; /*****/
}

void GetFontWideOutlineMetrics( sfntClass *font, T2K_FontWideMetrics *hori, T2K_FontWideMetrics *vert )
{
    int i;

    vert->isValid   = false; /* Initialize */
    hori->isValid   = false;

	hori->underlinePosition		= 0;	/* Initialize to unknown state */
	hori->underlineThickness	= 0;
	vert->underlinePosition		= 0;
	vert->underlineThickness	= 0;
#ifdef ENABLE_T1
    if ( font->T1 != NULL ) {
		F16Dot16 	angle;
		hori->isValid   = true;
		hori->Ascender	= (short)font->T1->ascent; 
		hori->Descender	= (short)font->T1->descent; 
		hori->LineGap	= (short)font->T1->lineGap;
		hori->maxAW		= (unsigned short)font->T1->advanceWidthMax;
		angle 		= font->T1->italicAngle;
		hori->caretDx	= 0;
		hori->caretDy	= ONE16Dot16;
		if ( angle != 0 ) {
			if ( angle < 0 ) angle = -angle;
			/* The angle is measured from the y axis */
			hori->caretDx = util_FixSin( angle );
			hori->caretDy = util_FixCos( angle );
		}
		hori->underlinePosition		= (short)font->T1->UnderlinePosition;
		hori->underlineThickness	= (short)font->T1->UnderlineThickness;
        return; /*****/
    }
#endif
#ifdef ENABLE_CFF
    if ( font->T2 != NULL ) {
		F16Dot16 	angle;
		hori->isValid   = true;
		hori->Ascender	= (short)font->T2->ascent; 
		hori->Descender	= (short)font->T2->descent; 
		hori->LineGap	= (short)font->T2->lineGap;
		hori->maxAW		= (unsigned short)font->T2->advanceWidthMax;
		angle 		= font->T2->italicAngle;
		hori->caretDx	= 0;
		hori->caretDy	= ONE16Dot16;
		if ( angle != 0 ) {
			if ( angle < 0 ) angle = -angle;
			/* The angle is measured from the y axis */
			hori->caretDx = util_FixSin( angle );
			hori->caretDy = util_FixCos( angle );
		}
		hori->underlinePosition		= (short)font->T2->topDictData.UnderlinePosition;
		hori->underlineThickness	= (short)font->T2->topDictData.UnderlineThickness; 
        return; /*****/
    }
#endif
#ifdef ENABLE_PFR
    if ( font->PFR != NULL ) {
		F16Dot16 	angle;
		hori->isValid   = true;
		hori->Ascender	= (short)font->PFR->ascent; 
		hori->Descender	= (short)font->PFR->descent; 
		hori->LineGap	= (short)font->PFR->lineGap;
		hori->maxAW		= (unsigned short)font->PFR->advanceWidthMax;
		angle			= font->PFR->italicAngle;
		hori->caretDx	= 0;
		hori->caretDy	= ONE16Dot16;
        if ( angle != 0 ) {
            if ( angle < 0 ) angle = -angle;
            /* The angle is measured from the y axis */
            hori->caretDx = util_FixSin( angle );
            hori->caretDy = util_FixCos( angle );
        }
        return; /*****/
    }
#endif
#ifdef ENABLE_SPD
    if ( font->SPD != NULL ) {
		F16Dot16 	angle;
		hori->isValid   = true;
		hori->Ascender	= (short)font->SPD->ascent; 
		hori->Descender	= (short)font->SPD->descent; 
		hori->LineGap	= (short)font->SPD->lineGap;
		hori->maxAW		= (unsigned short)font->SPD->advanceWidthMax;
		angle			= font->SPD->italicAngle;
		hori->caretDx	= 0;
		hori->caretDy	= ONE16Dot16;
		if ( angle != 0 ) {
			if ( angle < 0 ) angle = -angle;
			/* The angle is measured from the y axis */
			hori->caretDx = util_FixSin( angle );
			hori->caretDy = util_FixCos( angle );
		}
		return; /*****/
	}
#endif
	hori->underlinePosition		= font->post_underlinePosition;
	hori->underlineThickness	= font->post_underlineThickness;

    if ( font->hhea != NULL ) {
        hori->isValid   = true;
		hori->Ascender	= font->hhea->Ascender;
		hori->Descender	= font->hhea->Descender;
		hori->LineGap	= font->hhea->LineGap;
		hori->maxAW		= font->hhea->advanceWidthMax;
		hori->caretDx	= font->hhea->caretSlopeRun;
		hori->caretDy	= font->hhea->caretSlopeRise;
        /* Scale up */
        for ( i = 0; i < 16; i++ ) {
            if ( hori->caretDx >= ONE16Dot16 || hori->caretDx <= -ONE16Dot16 ) break; /*****/
            if ( hori->caretDy >= ONE16Dot16 || hori->caretDy <= -ONE16Dot16 ) break; /*****/
            hori->caretDx <<= 1;
            hori->caretDy <<= 1;
        }
    }
    if ( font->vhea != NULL ) {
        vert->isValid   = true;
		vert->Ascender	= font->vhea->Ascender;
		vert->Descender	= font->vhea->Descender;
		vert->LineGap	= font->vhea->LineGap;
		vert->maxAW		= font->vhea->advanceWidthMax;
		vert->caretDx	= font->vhea->caretSlopeRun;
		vert->caretDy	= font->vhea->caretSlopeRise;
        /* Scale up */
        for ( i = 0; i < 16; i++ ) {
            if ( vert->caretDx >= ONE16Dot16 || vert->caretDx <= -ONE16Dot16 ) break; /*****/
            if ( vert->caretDy >= ONE16Dot16 || vert->caretDy <= -ONE16Dot16 ) break; /*****/
            vert->caretDx <<= 1;
            vert->caretDy <<= 1;
        }
    }
}

/*
 *
 */
void Purge_cmapMemory( sfntClass *t )
{
    Delete_cmapClass( t->cmap );
    t->cmap = NULL;
}

/*
 *
 */
void FF_Delete_sfntClass( sfntClass *t, int *errCode )
{
    if ( errCode == NULL || (*errCode = setjmp(t->mem->env)) == 0 ) {
        /* try */
        /* Delete_InputStream( t->in ); */
    #ifdef ENABLE_WRITE
        Delete_OutputStream( t->out );
    #endif
        Delete_ttcfClass( t->ttcf );
        Delete_sfnt_OffsetTable( t->offsetTable0);
        Delete_headClass( t->head );
        Delete_hheaClass( t->hhea );
        Delete_hheaClass( t->vhea );
        Delete_hmtxClass( t->hmtx );
        Delete_hmtxClass( t->vmtx );
        Delete_maxpClass( t->maxp );
        Delete_locaClass( t->loca );
#ifdef ENABLE_T2KS
        FF_Delete_slocClass( t->sloc );
        FF_Delete_ffstClass( t->ffst );
        FF_Delete_ffhmClass( t->ffhm );
#endif
        Delete_cmapClass( t->cmap );
    #ifdef ENABLE_KERNING
        Delete_kernClass( t->kern );
    #endif
	#ifdef ENABLE_GASP_TABLE_SUPPORT
		Delete_gaspClass( t->gasp );
	#endif
    #ifdef ENABLE_T1
        tsi_DeleteT1Class( t->T1 );
    #endif
    #ifdef ENABLE_CFF
        tsi_DeleteCFFClass( t->T2 );
    #endif
    #ifdef ENABLE_T2KE
        tsi_DeleteT2KEClass( t->T2KE );
    #endif
    #ifdef ENABLE_PFR
        tsi_DeletePFRClass( t->PFR );
    #endif
    #ifdef ENABLE_SPD
        tsi_DeleteSPDClass( t->SPD );
    #endif
    #ifdef ENABLE_PCL
        tsi_DeletePCLClass( t->PCLeo );
    #endif
    #ifdef ENABLE_ORION
        Delete_OrionModelClass( (OrionModelClass *)(t->model) );
    #endif
    #ifdef ENABLE_SBIT
        Delete_blocClass( t->bloc );
        Delete_ebscClass( t->ebsc );
    #endif
    #ifdef ENABLE_NATIVE_TT_HINTS
        Delete_T2KTTClass( (T2KTTClass *)t->t2kTT );
    #endif
    #ifdef ENABLE_NATIVE_T1_HINTS
        Delete_FFT1HintClass( t->ffhint );
    #endif


        tsi_DeAllocMem( t->mem, t->awCache_hashKey );
        tsi_DeAllocMem( t->mem, t );
    } else {
        /* catch */
        tsi_EmergencyShutDown( t->mem );
    }
}

void ff_KernShellSort(kernPair0Struct *pairs, int num_pair)
{
    int i, j, incr = num_pair/2;
    uint32 tempindex;
    int16  tempvalue;

    while (incr > 0)
    {
        for (i = incr; i < num_pair; i++)
        {
            j = i - incr;
            while (j >= 0)
            {
                if (pairs[j].leftRightIndex > pairs[j+incr].leftRightIndex)
                {
                /* swap the two numbers */
                tempindex = pairs[j].leftRightIndex;
                tempvalue = pairs[j].value;

                pairs[j].leftRightIndex = pairs[j+incr].leftRightIndex;
                pairs[j].value = pairs[j+incr].value;

                pairs[j+incr].leftRightIndex = tempindex;
                pairs[j+incr].value = tempvalue;

                j -= incr;
                }
                else
                {
                j = -1;
                }
            }
        }
        incr = incr/2;
    }
}

/**/
#if 1
/* Format 2: */

typedef struct
{
	uint16	format;
	uint16	length;
	uint16	version;
	uint16	subHeaderKey[256];
}	HighByteMappingTable;

/* Format 2: */

typedef struct
{
    uint16 startCount;
    uint16 endCount;
    int16  idDelta;
    uint16 idRangeOffset;
}	rangeCount;

typedef struct
{
    uint16 firstCode;
    uint16 entryCount;
    int16  idDelta;
    uint16 idRangeOffset;
}	subHeaders;

/* Format 4: */

typedef struct
{
	uint16	format;
	uint16	length;
	uint16	version;
	uint16	segCountX2;
	uint16	searchRange;
	uint16	entrySelector;
	uint16	rangeShift;
	uint16	endCount;
}	segMappingTable;

/* Format 6: */

typedef struct
{
    uint16  format;
    uint16  length;
    uint16  version;
    uint16  firstCode;
    uint16  entryCount;
    uint16 glyphIdArray[1];
}   trimmedTableMapping;


static int16 Read2B(
    void *p)
{
uint16 result;
uint8 *pBuff = (uint8 *)p;

result = (uint16)(*(pBuff++));
result = (uint16)((result << 8) + (uint16)(*(pBuff)));

return (int16)result;
}

static int Read_PreferredMappingTable(void *userArg, cmapClass *t, void *ctxPtr, int ListCharsFn(void *userArg, void *p, uint16 code));

static int Read_PreferredMappingTable(void *userArg, cmapClass *t, void *ctxPtr, int ListCharsFn(void *userArg, void *p, uint16 code))
{
uint16 ii, jj, kk, nr, *cmapPtr;
uint16 *endCount, *startCount, *idDelta, *idOffset, glyphIndex;
uint32 key;

rangeCount *charRange;
segMappingTable *seg_table;
HighByteMappingTable *highbyte_table;
subHeaders *sub_header, subRange;
trimmedTableMapping *trim_table;
int checkStop = 0;
uint16 *glyphIds;
uint8 *p;

    /* Update the cmapPtr to point to the current table */
    cmapPtr = (uint16 *)(t->cmapData + t->platform[t->preferedEncodingTable]->offset); /* point at subTable */
    /* Process the CMAP based on format */
    switch(t->preferedFormat)
    {
        /* FORMAT - 0: BYTE ENCODING TABLE */
        case 0:

            /* Point to glyphIndexArray[0] */
            p = (uint8 *)cmapPtr + 6;

            for (ii=0; !checkStop && ii<256; ii++)
            {
                /* Report all non-zero character codes */
                if ( (p[ii] != 0) &&
                     (p[ii] < t->numGlyphs) )
                {
                    checkStop = ListCharsFn(userArg, ctxPtr, ii);
                }
            }

            break;

        /* FORMAT - 2: HIGH-BYTE THROUGH MAPPING */
        case 2:

            /* Set up the High Byte Mapping Table pointer */
            highbyte_table = (HighByteMappingTable *)cmapPtr;

            /* Loop through all of the character ranges */
            for ( ii = 0; !checkStop && ii < 256 ; ii++ )
            {
                /* Set up the subHeader key */
                key = (uint32)Read2B(&highbyte_table->subHeaderKey[ii]);
                sub_header = (subHeaders*)((char*)cmapPtr+518+key);

                subRange.firstCode     = (uint16)Read2B(&sub_header->firstCode);
                subRange.entryCount    = (uint16)Read2B(&sub_header->entryCount);
                subRange.idDelta       =  (int16)Read2B(&sub_header->idDelta);
                subRange.idRangeOffset = (uint16)Read2B(&sub_header->idRangeOffset);

                /* skip empty 2-byte ranges */
                if (ii != 0)
                    if (highbyte_table->subHeaderKey[ii] == 0)
                        continue;

                /* Point to idRangeOffset */
                p = (uint8 *)&sub_header->idRangeOffset;
                glyphIds = (uint16 *)((long)p + Read2B(p));


                /* Loop through all of the characters in the range */
                /* insert them into the character mapping array */
                for (jj = subRange.firstCode;
                     jj < subRange.firstCode+subRange.entryCount;
                     jj++)
                {

                    /* Check for 2-byte codes in the range 0 */
                    if (ii == 0)
                        if (highbyte_table->subHeaderKey[jj] != 0)
                            continue;

                    /* Check the glyph index array entry */
                    glyphIndex = (uint16)Read2B(&glyphIds[jj - subRange.firstCode]);
                    if (glyphIndex)
                    {
                        /* Store actual glyph index */
                        glyphIndex += subRange.idDelta;

                        /* Only valid character codes */
                        if (glyphIndex < t->numGlyphs)
                        {
                           /* callback with the character code */
                           checkStop = ListCharsFn(userArg, ctxPtr, (uint16)((ii << 8) | jj) );
                        }
                    }   /* End of 'glyphIndex != 0' */

                }   /* End of 'subRange' loop */
            }   /* End of 'for (ii<256)' */

            break;

        /* FORMAT - 4: SEGMENT MAPPING TO DELTA VALUES */
        case 4:
            /* Set up the segment mapping table pointer */
            seg_table = (segMappingTable *)cmapPtr;
            /* Store the number of character ranges */
            nr = (uint16)((uint16)Read2B(&seg_table->segCountX2) >> 1);
            /* Set pointers to the various arrays */
            endCount = (uint16*)&(seg_table->endCount);
            startCount = &endCount[nr+1];
            idDelta = &endCount[2*nr+1];
            idOffset = &endCount[3*nr+1];
            /* Allocate a local table for character range data */
            charRange = (rangeCount *)tsi_AllocMem( t->mem, (size_t) sizeof(rangeCount)*nr );
            if (charRange == NULL)
            {
#ifdef ENABLE_PRINTF
                printf("*****  ERROR - Read_PreferredMappingTable(): malloc(format 4:charRange) failed\n");
#endif
                return -1;
            }
                /* Set up the table of range values */
            for ( jj = 0 ; jj < nr ; jj++ )
            {
                charRange[jj].startCount    = (uint16)Read2B(&startCount[jj]);
                charRange[jj].endCount      = (uint16)Read2B(&endCount[jj]);
                charRange[jj].idDelta       = Read2B(&idDelta[jj]);
                charRange[jj].idRangeOffset = (uint16)Read2B(&idOffset[jj]);
            }

            /* Loop through all (but the last=0xFFFF) character ranges */
            for ( jj = 0; !checkStop && jj < (nr-1); jj++ )
            {
                    /* Loop through all of the characters in the range */
                for ( kk = charRange[jj].startCount; !checkStop && kk <= charRange[jj].endCount; kk++ )
                {
                        /* Process the correct indexing scheme */
                    if (charRange[jj].idRangeOffset == 0)
                    {
                        if ( ((glyphIndex = (uint16)((charRange[jj].idDelta + kk) % 65536)) != 0)  &&
                              (glyphIndex < t->numGlyphs) )
                        {
                            /* The characters are packed */
                             checkStop = ListCharsFn(userArg, ctxPtr, kk);
                        }
                    }
                    else
                    {
                            /* Calculate the glyphIndex address */
                        glyphIndex = (uint16)Read2B(charRange[jj].idRangeOffset/2 + (kk - charRange[jj].startCount) + &idOffset[jj]);
                        if (glyphIndex != 0)
                        {
                            /* Store the actual glyph index */
                            glyphIndex += charRange[jj].idDelta;
                            /* Only valid character codes */
                            if (glyphIndex < t->numGlyphs)
                            {
                                /* The characters are packed */
                                checkStop = ListCharsFn(userArg, ctxPtr, kk);
                            }
                        }

                    }   /* End of if (charRange[jj].idRangeOffset == 0) */

                }   /* End of character-loop */

            }   /* End of range-loop */

            tsi_DeAllocMem( t->mem, charRange );

            break;

        /* FORMAT - 6: TRIMMED TABLE MAPPING */
        case 6:

            /* Assign the table pointer */
            trim_table = (trimmedTableMapping *)cmapPtr;

            /* Do the following if we are trying to generate the character ID list */
            /* Loop through all of the characters in the range */
            for (kk = (uint16)Read2B(&trim_table->firstCode);
                    !checkStop &&
                    kk < ((uint16)Read2B(&trim_table->firstCode) + (uint16)Read2B(&trim_table->entryCount));
                    kk++ )
            {
                    /* If glyphIndex != 0 */
                if ( ((glyphIndex =(uint16)Read2B(&trim_table->glyphIdArray[kk - (uint16)Read2B(&trim_table->firstCode)])) != 0) &&
                      (glyphIndex < t->numGlyphs) )
                {
                    /* Store the character IDs */
                     checkStop = ListCharsFn(userArg, ctxPtr, kk);
                }   /* End of 'glyphIndex != 0' */
            }   /* End of character-loop */
            break;

        /* FORMAT - 9999: FontFusion specific cmap */
        case 9999:
            {
            uint16 glyphCode;
            uint8 *bPtr, mask;
                bPtr = (uint8 *)cmapPtr;
                bPtr += (256 + 3) * sizeof(uint16);
                /* bPtr is now pointing at what is an 8K bitmap */
                glyphCode = 0; checkStop = 0;
                for (ii = 0; !checkStop && (ii < (8 * 1024)); ii++, bPtr++)
                {
                    for (jj = 0, mask = 1; !checkStop && (jj < 8); jj++, mask <<= 1, glyphCode++)
                        if (*bPtr & mask)
                        {
                            checkStop = ListCharsFn(userArg, ctxPtr, glyphCode);
                        }
                }
            }
            break;

        default:
#ifdef ENABLE_PRINTF
            printf("*****  ERROR - Read_PreferredMappingTable(): Undefined Character Mapping Format: %d\n", (int)t->preferedFormat);
#endif
            return -1;

        }       /* End of switch(t->preferedFormat) */
    /* Return - OK */
    return 0;
}

void T2K_SfntListChars(void *userArg, sfntClass *t, void *ctxPtr, int ListCharsFn(void *userArg, void *p, uint16 code), int *errCode)
{

    *errCode = 0;
#ifdef ENABLE_T1
    if ( t->T1 != NULL ) {
        tsi_T1ListChars(userArg, t->T1, ctxPtr, ListCharsFn);
        return ; /******/
    }
#endif
#ifdef ENABLE_CFF
    if ( t->T2 != NULL ) {
        tsi_T2ListChars(userArg, t->T2, ctxPtr, ListCharsFn);
        return ; /******/
    }
#endif
#ifdef ENABLE_PFR
    if ( t->PFR != NULL ) {
        tsi_PFRListChars(userArg, t->PFR, ctxPtr, ListCharsFn);
        return ; /******/
    }
#endif
#ifdef ENABLE_SPD
    if ( t->SPD != NULL ) {
        tsi_SPDListChars(userArg, t->SPD, ctxPtr, ListCharsFn);
        return ; /******/
    }
#endif
#ifdef ENABLE_PCL
    if ( t->PCLeo != NULL ) {
		return ; 	/* we don't do anything for PCL */
    }
#endif
/* else: TrueType or PCLetto */
    ff_LoadCMAP( t );
    if (t->cmap)
    {/* TrueType */
        Read_PreferredMappingTable(userArg, t->cmap, ctxPtr, ListCharsFn);
        return;
    }
#ifdef ENABLE_PCLETTO
    else
    {/* PCLetto */
		return;	/* we don't do anything for PCLetto */
	}
#endif
}



#endif
/**/


/*********************** R E V I S I O N   H I S T O R Y **********************
 *
 *     $Header: R:/src/FontFusion/Source/Core/rcs/truetype.c 1.68 2001/05/07 20:33:41 shawn preliminary $
 *                                                                           *
 *     $Log: truetype.c $
 *     Revision 1.68  2001/05/07 20:33:41  shawn
 *     Initialize 'cmap->bytesConsumed = 2' in
 *          Compute_cmapClass_GlyphIndex().
 *     Set 'cmap->bytesConsumed = 1' for one-byte character codes in
 *          Compute_cmapClass_Index2().
 *     Revision 1.67  2001/05/07 18:22:12  reggers
 *     Fixed warnings when T! and CFF not enabled.
 *     Revision 1.66  2001/05/04 21:44:32  reggers
 *     Warning cleanup
 *     Revision 1.65  2001/05/03 20:46:46  reggers
 *     LoadCMAP mapped to ff_LoadCMAP
 *     Revision 1.64  2001/05/02 20:04:48  reggers
 *     Changed a tsi_Assert test in New_locaClass that was bogus. (Sampo).
 *     Revision 1.63  2001/05/02 17:20:38  reggers
 *     SEAT BELT mode added (Sampo)
 *     Revision 1.62  2001/04/27 20:33:35  reggers
 *     Added new API function T2K_ForceCMAPChange()
 *     Revision 1.61  2001/04/27 19:35:48  reggers
 *     Added new API function GetTTTablePointer for loading any TrueType
 *     Table into RAM memory.
 *     Revision 1.60  2001/04/24 21:57:13  reggers
 *     Added GASP table support (Sampo).
 *     Revision 1.59  2001/04/19 17:38:48  reggers
 *     Internal API for PSName to ccode conversion.
 *     Revision 1.58  2001/03/30 15:39:40  shawn
 *     Added support for Truetype cmap Format 2. Added code to save the selected
 *     cmap platformID and platformSpecificID in the cmapClass. Also added code to
 *     set the numGlyphs member of cmapClass to be used by the listCharCallback()
 *     function to determine valid character codes.
 *     Cleaned up problems in listChar processes and character code to
 *     glyph index processes.
 *     
 *     Revision 1.57  2001/01/31 15:40:29  reggers
 *     Changed cast in New_cmapClass() for offset to uint32 from uint16.
 *     Revision 1.56  2000/12/06 21:31:27  reggers
 *     Compute_cmapClass_Index4() now checks walking off end of the
 *     table and returns index 0 if it does.
 *     Revision 1.55  2000/12/06 20:11:36  reggers
 *     Added bullet-proofing in New_hmtxEmptyClass(). Thanks Derek!
 *     Revision 1.54  2000/11/02 21:57:43  reggers
 *     Silence insignificant warnings.
 *     Revision 1.53  2000/11/02 17:21:43  reggers
 *     Treat maxStackElements as unsigned.
 *     Revision 1.52  2000/10/18 17:42:12  reggers
 *     Removed extraneous prototype of Ff_FontTypeFromStream()
 *     Revision 1.51  2000/08/09 19:23:19  reggers
 *     Made #include of shaspet,h lower case.
 *     Revision 1.50  2000/07/11 17:33:08  reggers
 *     Borland STRICK warning fixes
 *     Revision 1.49  2000/06/16 18:38:27  reggers
 *     Warnings cleanup.
 *     Revision 1.48  2000/06/15 18:38:00  reggers
 *     Borland STRICKT warning suppression.
 *     Revision 1.47  2000/06/14 21:32:31  reggers
 *     Check entry length of kerning table.
 *     Changed an assert to tsi_Assert()
 *     Revision 1.46  2000/06/13 14:28:49  reggers
 *     Got rid of unneeded assignments that Borland STRICT hated.
 *     Revision 1.45  2000/06/09 15:25:25  reggers
 *     Zap unused hint arrays when glyph has no points.
 *     Revision 1.44  2000/06/06 14:35:15  reggers
 *     Fixed warning/error on Code Warrior with format 2 table mapping code.
 *     Revision 1.43  2000/06/02 17:27:04  reggers
 *     Corrected format 2 mapping table expansion/reading for list chars.
 *     Revision 1.42  2000/06/01 16:55:46  reggers
 *     Fixed non stroke font metrics calc (Sampo).
 *     Revision 1.41  2000/05/26 16:44:00  reggers
 *     Support for PCLeo GetAWFuncPtr.
 *     Revision 1.40  2000/05/24 18:04:15  reggers
 *     Made Read_OneMappingTable portable to Intel platforms.
 *     Revision 1.39  2000/05/18 15:51:10  reggers
 *     Silence STRICT Borland warnings.
 *     Revision 1.38  2000/05/17 16:58:27  reggers
 *     Removed C++ style comment.
 *     Revision 1.37  2000/05/17 15:36:09  reggers
 *     Set hmtx pointer for Speedo.
 *     Revision 1.36  2000/05/17 14:07:56  reggers
 *     Fix for 2 byte integer environments
 *     Revision 1.35  2000/05/12 19:16:18  reggers
 *     Cmap format 4 binary search (Sampo)
 *     Revision 1.34  2000/05/09 20:31:39  reggers
 *     Better test for GetAWFuncPtr2 under stroke fonts.
 *     Faster font open for T1 and CFF: no more hmtx tables. Use AWFuncPtr2
 *     for these formats.
 *     Fixed IsFigure() for Speedo. Can now get code from index.
 *     Revision 1.33  2000/05/09 19:56:21  mdewsnap
 *     Hint related code... added in Type1 style hooks.
 *     Revision 1.32  2000/04/19 19:01:02  mdewsnap
 *     Added in code to deal with T1 hints
 *     Revision 1.31  2000/04/14 17:00:58  reggers
 *     First cut applying selective hints to stroke font glyphs.
 *     Revision 1.30  2000/04/13 18:14:34  reggers
 *     Updated list chars for user argument or context pass through.
 *     Revision 1.29  2000/03/10 19:18:29  reggers
 *     Enhanced for enumeration of character codes in font.
 *     Revision 1.28  2000/02/25 17:46:22  reggers
 *     STRICT warning cleanup.
 *     Revision 1.27  2000/02/18 18:56:16  reggers
 *     Added Speedo processor capability.
 *     Revision 1.26  2000/01/21 17:02:12  reggers
 *     In GetGlyphbyIndex, PCLETTO block overlooked setting *aWidth and
 *     *aHeight for the glyph metrics. Corrected.
 *     Revision 1.25  2000/01/19 19:21:25  reggers
 *     Changed all references to PCLClass member PCL to PCLeo to
 *     avoid nasty namespace conflict on Windows builds.
 *     Revision 1.24  2000/01/18 20:53:55  reggers
 *     Changes to abstract the character directory and character string
 *     storage to the application environment for ENABLE_PCL.
 *     Revision 1.22  2000/01/07 19:46:00  reggers
 *     Sampo enhancements for FFS fonts.
 *     Revision 1.21  1999/12/23 22:03:16  reggers
 *     New ENABLE_PCL branches. Rename any 'code' and 'data' symbols.
 *     Revision 1.20  1999/11/19 01:42:52  reggers
 *     Make non-ram stream error return possible. Watch possible NULL
 *     sfntClass pointer.
 *     Revision 1.19  1999/11/15 20:07:45  reggers
 *     Improved handling of firstCharCode, lastCharCode and isFixedPitch,
 *     all technologies.
 *     Revision 1.18  1999/11/04 20:20:35  reggers
 *     Added code for getting fixed/proportional setting, firstCharCode and
 *     lastCharCode.
 *     Revision 1.17  1999/10/21 20:41:16  jfatal
 *     Fix compile error if ENABLE_KERNING is turned off.
 *     Revision 1.16  1999/10/18 20:30:11  shawn
 *     Modified to avoid TrueType sfnt class generation for non-TrueType
 *     fonts.
 *
 *     Revision 1.15  1999/09/30 15:10:55  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.14  1999/09/30 14:38:34  mdewsnap
 *     Added check on pairs pointer in Delete_kernSubTable0Data
 *     routine.
 *     Revision 1.13  1999/09/29 20:28:26  mdewsnap
 *     Added in check for number of kern pairs in T1 font -- deleted
 *     kern class if no pairs exist.
 *     Revision 1.12  1999/09/20 18:18:57  reggers
 *     Added a cast for warnings silence.
 *     Revision 1.11  1999/08/30 19:11:06  reggers
 *     Prevent test of head pointer for stroke fonts being tested before all other
 *     font types are eliminated.
 *     Revision 1.10  1999/08/27 20:08:51  reggers
 *     Latest changes from Sampo
 *     Revision 1.8  1999/07/29 16:10:50  sampo
 *     First revision for T2KS
 *     Revision 1.7  1999/07/19 16:59:19  sampo
 *     Moved kern shell sort here from util.c
 *     Revision 1.6  1999/07/16 18:22:23  mdewsnap
 *     Added call to New_T1kernClass
 *     Revision 1.5  1999/07/16 19:30:24  sampo
 *     Setting of t->kern for PFR.
 *     Revision 1.4  1999/07/16 17:52:10  sampo
 *     Sampo work. Drop #8 July 16, 1999
 *     Revision 1.3  1999/06/08 13:53:52  Krode
 *     Corrected byte order of 16 bit name strings
 *     Revision 1.2  1999/05/17 15:58:42  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/

/***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/

