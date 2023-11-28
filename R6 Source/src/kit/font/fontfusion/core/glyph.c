/*
 * GLYPH.c
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
#ifdef ENABLE_FF_CURVE_CONVERSION
#include "util.h"
#endif
#ifdef ENABLE_ORION
#include "orion.h"
#endif




int ReadDeltaXYValue( InputStream *in, short *dxPtr, short *dyPtr )
{
	int dx, dy;
	int d1, d2; /* Main and second axis	 */
	int quadrant;
	unsigned long w1;
	
	w1 = ReadUnsignedByteMacro( in );
	w1 <<= 8;
	w1 |= ReadUnsignedByteMacro( in );
	/* ow1 = w1; */
	quadrant = (int)(w1 >> 14);
	w1 &= 0x3fff;
	
	if ( w1 < BASE1 ) {
		/* if ( ow1 == 0 || (ow1 == (1<<14)) ) */
		if ( w1 == 0 && quadrant <= 1 ) {
			short theDx, theDy;
			theDx = ReadUnsignedByteMacro( in );
			theDx <<= 8;
			theDx |= ReadUnsignedByteMacro( in );
			theDy = ReadUnsignedByteMacro( in );
			theDy <<= 8;
			theDy |= ReadUnsignedByteMacro( in );
			*dxPtr = theDx;
			*dyPtr = theDy;
			return quadrant == 0 ? 1 : 0; /*****/
		} else {
			d1 = (int)w1;
			d2 = 0;
		}
	} else if ( w1 < BASE2 ) {
		w1 -= BASE1;
		/* assert( w1 >= 0 && w1 < 102*102 ); */
		d1 = (int)((short)w1 / 102 + 1);
		d2 = (int)((short)w1 % 102 + 1);
	} else if ( w1 < BASE3 ) {
		w1 -= BASE2;
		w1 <<= 8;
		w1 |= ReadUnsignedByteMacro( in );
		/* assert( w1 >= 0 && w1 < 724*724 ); */
		d1 = (int)(w1 / 724 + 1);
		d2 = (int)(w1 % 724 + 1);
	} else {
		w1 -= BASE3;
		w1 <<= 8;
		w1 |= ReadUnsignedByteMacro( in );
		w1 <<= 8;
		w1 |= ReadUnsignedByteMacro( in );
		/* assert( w1 >= 0 && w1 < 10650*10650 ); */
		d1 = (int)(w1 / 10650);
		d2 = (int)(w1 % 10650);
	}

	dx = dy = 0; /* to avoid warning with MS C */
	switch ( quadrant ) {
	case 0:
		dx = d1;
		dy = d2;
		break; /*****/
	case 1:
		dx = -d2;
		dy = d1;
		break; /*****/
	case 2:
		dx = -d1;
		dy = -d2;
		break; /*****/
	case 3:
		dx = d2;
		dy = -d1;
		break; /*****/
	}
	d1 = 1 - (dx & 1); /* oncurve */
	*dxPtr = (short)(dx >>1);
	*dyPtr = (short)dy;
	return d1;
}


#ifdef ENABLE_ORION
int ReadOrionDeltaXYValue( InputStream *in, void *model, short *dxPtr, short *dyPtr )
{
	int dx, dy;
	int d1, d2; /* Main and second axis	 */
	int quadrant;
	unsigned long w1;
	unsigned char b0, b1;
	OrionModelClass *orion = (OrionModelClass *)model;
	
	assert( orion != 0 );
	
	b0 = SCODER_ReadSymbol( orion->literal[ orion->OrionState ], in );
	/* d2 = ((b0+b0)&6) + (orion->OrionState&1); */
	d2 = (b0+b0) + (orion->OrionState&1);
	d2 = d2 % orion->num_eb2;
	b1 = SCODER_ReadSymbol( orion->literal[ d2 + orion->num_eb1 ], in );
	
	w1 = b0;
	w1 <<= 8;
	w1 |= b1;
	/* ow1 = w1; */
	quadrant = (int)(w1 >> 14);
	w1 &= 0x3fff;
	
	if ( w1 < BASE1 ) {
		/* if ( ow1 == 0 || (ow1 == (1<<14)) ) */
		if ( w1 == 0 && quadrant <= 1 ) {
			short theDx, theDy;
			theDx = SCODER_ReadSymbol( orion->literal[orion->num_e-1], in );
			theDx <<= 8;
			theDx |= SCODER_ReadSymbol( orion->literal[orion->num_e-1], in );
			theDy = SCODER_ReadSymbol( orion->literal[orion->num_e-1], in );
			theDy <<= 8;
			theDy |= SCODER_ReadSymbol( orion->literal[orion->num_e-1], in );
			*dxPtr = theDx;
			*dyPtr = theDy;
			return quadrant == 0 ? 1 : 0; /*****/
		} else {
			d1 = (int)w1;
			d2 = 0;
		}
	} else if ( w1 < BASE2 ) {
		w1 -= BASE1;
		/* assert( w1 >= 0 && w1 < 102*102 ); */
		d1 = (int)(w1 / 102 + 1);
		d2 = (int)(w1 % 102 + 1);
	} else if ( w1 < BASE3 ) {
		w1 -= BASE2;
		w1 <<= 8;
		w1 |= SCODER_ReadSymbol( orion->literal[orion->num_e-1], in );
		/* assert( w1 >= 0 && w1 < 724*724 ); */
		d1 = (int)(w1 / 724 + 1);
		d2 = (int)(w1 % 724 + 1);
	} else {
		w1 -= BASE3;
		w1 <<= 8;
		w1 |= SCODER_ReadSymbol( orion->literal[orion->num_e-1], in );
		w1 <<= 8;
		w1 |= SCODER_ReadSymbol( orion->literal[orion->num_e-1], in );
		/* assert( w1 >= 0 && w1 < 10650*10650 ); */
		d1 = (int)(w1 / 10650);
		d2 = (int)(w1 % 10650);
	}

	dx = dy = 0; /* to avoid warning with MS C */
	switch ( quadrant ) {
	case 0:
		dx = d1;
		dy = d2;
		break; /*****/
	case 1:
		dx = -d2;
		dy = d1;
		break; /*****/
	case 2:
		dx = -d1;
		dy = -d2;
		break; /*****/
	case 3:
		dx = d2;
		dy = -d1;
		break; /*****/
	}
	d1 = 1 - (dx & 1); /* oncurve */
	*dxPtr = (short)(dx >>1);
	*dyPtr = (short)dy;
	return d1;
}

#endif /* ENABLE_ORION */

#ifndef ENABLE_WRITE
void TEST_T2K_GLYPH( tsiMemObject *mem )
{
	UNUSED(mem);
}
#endif



void glyph_AllocContours( GlyphClass *t, short contourCountMax )
{
	short ctr;
	short	*sp;			/* sp[contourCount] Start points */
	short	*ep;  			/* ep[contourCount] End points */
	
	if ( t->contourCountMax < contourCountMax ) {
		t->contourCountMax = contourCountMax;
		sp = (short*) tsi_AllocMem( t->mem, sizeof(short) * contourCountMax * 2);
		ep   = &sp[ contourCountMax ];
		
		for ( ctr = 0; ctr < t->contourCount; ctr++ ) {
			sp[ctr] = t->sp[ctr];
			ep[ctr] = t->ep[ctr];
		}
		if ( t->sp != t->ctrBuffer )tsi_DeAllocMem( t->mem, t->sp );
		t->sp = sp;
		t->ep = ep;
	}
}

#ifdef T1_OR_T2_IS_ENABLED
void glyph_CloseContour( GlyphClass *t )
{
    short ctr, point;

    /* This if block allows us to call glyph_CloseContour twice in a
						row with no ill effects. */
    if ( t->pointCount == 0 || ( t->contourCount > 0 &&
					t->ep[t->contourCount-1] == t->pointCount-1 ) ) {
            return; /*****/ /* No points added so don't close the path. */
    }

    glyph_AllocContours( t, (short)(t->contourCount + 2) );

    if ( t->pointCount <= 0 ) {
        t->ep[t->contourCount] = 0;
    } else {
        t->ep[t->contourCount] = (short)(t->pointCount - 1);
    }
    t->contourCount++;

    point = 0;
    for ( ctr = 0; ctr < t->contourCount; ctr++ ) {
        t->sp[ctr] = point;
        point = (short)(t->ep[ctr] + 1);
    }
    /* eliminate duplicate points */
    if ( t->pointCount > 0 ) {
        short ptA, ptB;

		ptA = t->sp[t->contourCount-1];
        ptB = t->ep[t->contourCount-1];

        if ( ptB > ptA && t->oox[ptA] == t->oox[ptB] && t->ooy[ptA]
				== t->ooy[ptB] && t->onCurve[ptA] == t->onCurve[ptB] ) {
            t->pointCount--;
            t->ep[t->contourCount-1] = (short)(t->pointCount - 1);
            }
    }
}
#endif /* T1_OR_T2_IS_ENABLED */

#ifdef ENABLE_T2KE
/* Dynamically allocates color plane information */
static void AllocNewColorPlane( GlyphClass *glyph )
{
	glyph->colorPlaneCount++;


	if ( glyph->colorPlaneCount > glyph->colorPlaneCountMax ) {
		glyph->colorPlaneCountMax = glyph->colorPlaneCount + (glyph->colorPlaneCount>>1) + 8;
		if ( glyph->colors == NULL ) {
			glyph->colors = (tsiColorDescriptor *)tsi_AllocMem( glyph->mem, glyph->colorPlaneCountMax * sizeof(tsiColorDescriptor) );
		} else {
			glyph->colors = (tsiColorDescriptor *)tsi_ReAllocMem( glyph->mem, glyph->colors, glyph->colorPlaneCountMax * sizeof(tsiColorDescriptor) );
		}
	}
	assert( glyph->colors != NULL );
}

void glyph_CloseColorContour( GlyphClass *glyph, tsiColorDescriptor *color )
{
	long index = glyph->colorPlaneCount-1;
	glyph_CloseContour( glyph );
	if ( index >= 0 && glyph->colors[index].curveType == color->curveType &&
				glyph->colors[index].paintType 	== color->paintType && color->paintType == 0 &&
				glyph->colors[index].ARGB 		== color->ARGB ) {
		glyph->colors[index].numContoursInThisPlane++;
	} else {
		assert( glyph->contourCount != 1 || glyph->colorPlaneCount == 0 );
		AllocNewColorPlane( glyph );
		index = glyph->colorPlaneCount-1;
		glyph->colors[index] = *color;
		glyph->colors[index].numContoursInThisPlane = 1;
	}
}
#endif


#ifdef ENABLE_PRINTF
void glyph_PrintPoints( GlyphClass *t )
{
	int i;
	
	printf("+++++\n");
	printf("t->contourCount = %d\n", t->contourCount );
	if ( t->contourCount > 0 ) {
		for ( i = 0; i < t->contourCount; i++ ) {
			printf("%d: sp = %d, ep = %d\n", i, t->sp[i], t->ep[i] );
		}
		for ( i = 0; i <= t->ep[ t->contourCount-1 ]; i++ ) {
			printf("%d: x = %d, y = %d, %s\n", i, t->oox[i], t->ooy[i], t->onCurve[i] ? "on" : "off" );
		}
	}
}
#endif


#ifdef T1_OR_T2_IS_ENABLED
void glyph_AddPoint( GlyphClass *t, long x, long y, unsigned char onCurveBit )
{
	register short *oox, *ooy;
	register short *ooxOld, *ooyOld;
	register uint8 *onCurve;
	register uint8 *onCurveOld;
	F26Dot6 *memBase;
	int i, limit;
	
	if ( t->pointCount >= t->pointCountMax ) { 
		/* ReAllocate */
		memBase = t->x; ooxOld = t->oox; ooyOld = t->ooy; onCurveOld = t->onCurve;
		AllocGlyphPointMemory( t, t->pointCountMax + (t->pointCountMax >> 1 ) + 32 );
		oox = t->oox; ooy = t->ooy; onCurve = t->onCurve;
		
		limit = t->pointCount + SbPtCount;
		for ( i = 0; i < limit; i++ ) {
			oox[i]		= ooxOld[i];
			ooy[i]		= ooyOld[i];
			onCurve[i]	= onCurveOld[i];
		}
		tsi_FastDeAllocN( t->mem, memBase, T2K_FB_POINTS); /* contains all the old point data */
	}
	i = t->pointCount;
	t->oox[i]		= (short)x;
	t->ooy[i]		= (short)y;
	t->onCurve[i]	= onCurveBit;
	t->pointCount	= (short)(i + 1);
}

void glyph_StartLine( GlyphClass *t, long x, long y )
{
	int prevPoint = t->pointCount-1;
	
	if ( prevPoint < 0 || t->oox[prevPoint] != x || t->ooy[prevPoint] != y ) {
		glyph_AddPoint( t, x, y, 1 );
	}
}

#endif /* T1_OR_T2_IS_ENABLED */


/*
 *
 */
GlyphClass *New_EmptyGlyph( tsiMemObject *mem, int16 lsb, uint16 aw, int16 tsb, uint16 ah )
{
	register short *oox, *ooy;
	register int pointCount;
	GlyphClass *t = (GlyphClass *) tsi_FastAllocN( mem, sizeof( GlyphClass ), T2K_FB_GLYPH);

	t->mem				= mem;
	t->sp	= t->ep		= NULL;
	t->componentData	= NULL;
	t->x = t->y 		= NULL;
	
	t->colorPlaneCount		= 0;
 	t->colorPlaneCountMax	= 0;
#ifdef ENABLE_T2KE
 	t->colors				= NULL;
#endif
  	t->curveType			= 2;
	t->contourCountMax		= 0;
 	t->pointCountMax		= 0;
 	t->componentSizeMax		= 0;
 	
	t->contourCount			= 0;
	t->xmin 				= 0;
	t->ymin 				= 0;
	t->xmax 				= 0;
	t->ymax 				= 0;
	pointCount 				= 0;
 	t->componentSize		= 0;
 	t->hintLength 			= 0;
 	t->hintFragment			= NULL;
 	
	AllocGlyphPointMemory( t, pointCount );
	oox = t->oox; ooy = t->ooy;

	ooy[pointCount + 0] = 0;
	oox[pointCount + 0] = 0;
	UNUSED(lsb);
	
	ooy[pointCount + 1] = 0;
	oox[pointCount + 1] = (short)(oox[pointCount + 0] + aw);

#if SbPtCount >= 4
	ooy[pointCount + 2] = (short)(t->ymax + tsb);
	oox[pointCount + 2] = 0;
	
	ooy[pointCount + 3] = (short)(ooy[pointCount + 2] - ah);
	oox[pointCount + 3] = 0;
#else
	UNUSED(tsb);
#endif
	t->pointCount = (short)pointCount;
	/* printf("contourCount = %ld, pointCount = %ld\n", (long)t->contourCount, (long)t->pointCount ); */
	return t; /*****/
}


#ifdef ENABLE_ORION
static unsigned long ReadScoderUnsignedNumber( InputStream *in, SCODER *sc )
{
	unsigned char value;
	unsigned long n = 0;
	unsigned long shift = 0;
	
	do {
		value = SCODER_ReadSymbol( sc, in );
		n |= ((value & 0x7f) << shift );
		shift += 7;
	} while (value & 0x80);
	return n; /*****/
}
#endif

/* #define BAD_CONTOUR_DIRECTION_TEST */
#ifdef BAD_CONTOUR_DIRECTION_TEST
/*
 * REMOVE
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
#endif /* BAD_CONTOUR_DIRECTION_TEST */

/*
 *
 */
GlyphClass *New_GlyphClassT2K( tsiMemObject *mem, register InputStream *in, char readHints, int16 lsb, uint16 aw, int16 tsb, uint16 ah, void *model )
{
	register long i;
	register short *oox, *ooy;
	register uint8 *onCurve;
	register int pointCount;
	GlyphClass *t = (GlyphClass *) tsi_FastAllocN( mem, sizeof( GlyphClass ), T2K_FB_GLYPH);
	register short xmin, ymax;
	
	
	UNUSED(model);
	UNUSED(readHints);
	
	t->mem				= mem;
	oox	= ooy			= NULL;
	t->sp	= t->ep		= NULL;
	/* onCurve				= NULL; */
	t->componentData	= NULL;
	t->x = t->y 		= NULL;

	t->colorPlaneCount		= 0;
 	t->colorPlaneCountMax	= 0;
#ifdef ENABLE_T2KE
 	t->colors				= NULL;
#endif
 	t->curveType			= 2;
 	t->contourCountMax		= 0;
 	t->pointCountMax		= 0;

	t->contourCount			= ReadInt16( in );
	t->xmin 				= 0;
	t->ymin 				= 0;
	t->xmax 				= 0;
	t->ymax 				= 0;
	pointCount 				= 0;
 	t->componentSize		= 0;
 	t->hintLength 			= 0;
 	t->hintFragment			= NULL;
	if ( t->contourCount < 0 ) {
		/* Composite Glyph */
		short flags;
 		int glyphIndex;
 		register short *componentData;
 		register long componentSize = 0;
 		
 		t->componentSizeMax		= 1024;
 		componentData			= (short*) tsi_AllocMem( t->mem, t->componentSizeMax * sizeof(short) );
 		do {
 			if ( componentSize >= t->componentSizeMax - 10 ) {
 				/* Reallocate */
 				t->componentSizeMax += t->componentSizeMax/2;
 				componentData = (short*) tsi_ReAllocMem( t->mem, componentData, t->componentSizeMax * sizeof(short) );
 				assert( componentData != NULL );
 			}
 			flags = ReadInt16( in );
 			/* weHaveInstructions = (flags & WE_HAVE_INSTRUCTIONS) != 0; */
 			componentData[ componentSize++] = flags;
 			
 			glyphIndex = ReadInt16( in );
 			assert( glyphIndex >= 0 );
 			componentData[ componentSize++] = (short)glyphIndex;
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
		AllocGlyphPointMemory( t, pointCount );
		oox = t->oox; ooy = t->ooy; /* onCurve = t->onCurve; */
		t->componentData = componentData;
		t->componentSize = componentSize;
	} else if ( t->contourCount > 0 ) {
		short x, y;
		short stmp = 0;
#ifdef ENABLE_ORION
		OrionModelClass *orion = (OrionModelClass *)model;
		long			switchPos, numSwitches;
		unsigned char  *switchData = NULL;
#endif
		
		
		
		
		/* Regular Glyph */
		if ( t->contourCount <= T2K_CTR_BUFFER_SIZE ) {
			t->sp = t->ctrBuffer;
			t->ep = &t->sp[T2K_CTR_BUFFER_SIZE];
		} else {
			t->sp = (short *) tsi_AllocMem( t->mem, sizeof(short) * 2 * t->contourCount);
			t->ep   = &t->sp[t->contourCount];
		}
		
		
		for ( i = 0; i < t->contourCount; i++ ) {
		    t->sp[i]	= stmp;
		    
#ifdef ENABLE_ORION
			if ( orion != NULL ) {
				t->ep[i]    = (short)(ReadScoderUnsignedNumber( in, orion->ep) + stmp);
			} else {
		    	t->ep[i]    = (short)(ReadUnsignedNumber( in ) + stmp);
			}
#else
		    t->ep[i]    = (short)(ReadUnsignedNumber( in ) + stmp);
#endif
		    
		 	stmp = (short)(t->ep[i] + 1);
		}
		pointCount = stmp;

		t->hintLength = 0;
		
#ifdef ENABLE_ORION
		if ( orion != NULL ) {
			numSwitches = (pointCount+7)/8;;
			switchData = (unsigned char *) tsi_AllocMem( mem, numSwitches * sizeof( unsigned char ) );
			for ( switchPos = 0; switchPos < numSwitches; switchPos++ ) {
				switchData[ switchPos ] = SCODER_ReadSymbol( orion->control, in ); /* Initialize */
			}
			orion->OrionState = ORION_STATE_0 % orion->num_eb1; /* Initialize */;
		}
		switchPos = 0;

#endif
		

		AllocGlyphPointMemory( t, pointCount );
		oox = t->oox; ooy = t->ooy; onCurve = t->onCurve;

	 	t->contourCountMax	= t->contourCount;
	 	t->pointCountMax	= (short)pointCount;
		x = y = 0;
		xmin = 0x7fff;
		ymax = -0x7fff;
		for ( i = 0; i < pointCount; i++ ) {
			short dx, dy;
#ifdef ENABLE_ORION
			if ( orion != NULL ) {
				if ( switchData[ switchPos>>3 ] & (1 << (switchPos&7)) ) {
					unsigned char index;
					int i256 = orion->OrionState << 8;
					/* copy */
					index		= SCODER_ReadSymbol( orion->copy[ orion->OrionState ], in );
					/* printf("%d, %d\n",orion->OrionState, index ); */
					i256       += index;
					dx 			= orion->dx[i256];
					dy 			= orion->dy[i256];
					onCurve[i]	= (unsigned char)orion->onCurve[i256];
					/* printf("C dx = %d, dy = %d, onCurve = %d\n", dx, dy, (int)onCurve[i] ); */
				} else {
					/* literal */
					onCurve[i] = (unsigned char)ReadOrionDeltaXYValue( in, orion, &dx, &dy );
					/* printf("L dx = %d, dy = %d, onCurve = %d\n", dx, dy, (int)onCurve[i] ); */
				}
				switchPos++;
				Set_OrionState( orion, dx, dy, (char)onCurve[i] );
			} else {
				onCurve[i] = (unsigned char)ReadDeltaXYValue( in, &dx, &dy );
			}

#else
			onCurve[i] = (unsigned char)ReadDeltaXYValue( in, &dx, &dy );
#endif
			x = (short)(x + dx);
			y = (short)(y + dy);
			if ( x < xmin ) xmin = x;
			if ( y > ymax ) ymax = y;
			oox[i] = x;
			ooy[i] = y;
		}
		t->xmin = xmin;
		t->ymax = ymax;
#ifdef ENABLE_ORION
		tsi_DeAllocMem( mem, switchData );
#endif			
#ifdef WILL_MESS_UP_COMPOSITES
		if ( t->xmin != lsb ) {
			short error = (short)(t->xmin - lsb);
			for ( i = 0; i < pointCount; i++ ) {
				oox[i] = (short)(oox[i] - error);
			}
			t->xmin = lsb; t->xmax = (short)(t->xmax - error);
		}
#endif /* WILL_MESS_UP_COMPOSITES */
	}

	ooy[pointCount + 0] = 0;
	oox[pointCount + 0] = (short)(t->xmin - lsb);
	
	ooy[pointCount + 1] = 0;
	oox[pointCount + 1] = (short)(oox[pointCount + 0] + aw);
	
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
	
#ifdef BAD_CONTOUR_DIRECTION_TEST
	FlipContourDirection(t);
#endif
	return t; /*****/
}




void Add_GlyphClass( GlyphClass **tPtr, GlyphClass *addMe, uint16 flags, long arg1, long arg2 )
{
	register int i, j;
	GlyphClass *t;
	int pointCount, n, contourCount;
	short *sp, *ep, *oox, *ooy;
	uint8 *onCurve;
	F26Dot6 *x, *y, xDelta, yDelta;
	
	assert( addMe != NULL );
	t = *tPtr;
	if ( t == NULL ) {
		if ( flags & ARGS_ARE_XY_VALUES ) { /* Added Feb 9, 98 ---Sampo */
			xDelta = arg1;
			yDelta = arg2; /* Assume they have already been scaled */
			if ( flags & ROUND_XY_TO_GRID ) {
				xDelta = (xDelta+32) & ~63;
				yDelta = (yDelta+32) & ~63;
			}
			if ( xDelta != 0 || yDelta != 0 ) {
				j = addMe->pointCount;
				for ( i = 0; i < j; i++ ) {
					addMe->x[i] += xDelta;
					addMe->y[i] += yDelta;
				}
			}
		}
		*tPtr = addMe;
		return; /*****/
	}


	pointCount = t->pointCount + addMe->pointCount;
	n = pointCount + SbPtCount;
	contourCount = t->contourCount + addMe->contourCount;
	
	x				= (long *) tsi_AllocMem( t->mem, n * ( 2 * sizeof(long) + 2 * sizeof(short) + sizeof(uint8)) );
	y 				= &x[n];
	oox				= (short *)&y[n];
	ooy				= &oox[n];
	onCurve			= (uint8 *)&ooy[n];
	
	
	sp 				= (short *) tsi_AllocMem( t->mem, sizeof(short) * 2 * contourCount);
	ep   			= &sp[contourCount];
	
	t->pointCountMax   = (short)pointCount;
	t->contourCountMax = (short)contourCount;

	
	for ( i = 0; i < t->pointCount; i++ ) {
		x[i]		= t->x[i];
		y[i]		= t->y[i];
		
		/*
		oox[i]		= t->oox[i];
		ooy[i]		= t->ooy[i];
		*/
		onCurve[i]	= t->onCurve[i] ;
	}
	if ( !(flags & USE_MY_METRICS) ) {
		/* preserve the metrics from the previous glyph */
		x[pointCount + 0] = t->x[t->pointCount+0];
		y[pointCount + 0] = t->y[t->pointCount+0];
		x[pointCount + 1] = t->x[t->pointCount+1];
		y[pointCount + 1] = t->y[t->pointCount+1];
	}
	if ( flags & ARGS_ARE_XY_VALUES ) {
		xDelta = arg1;
		yDelta = arg2; /* Assume they have already been scaled */
		if ( flags & ROUND_XY_TO_GRID ) {
			xDelta = (xDelta+32) & ~63;
			yDelta = (yDelta+32) & ~63;
		}
	} else {
		xDelta = t->x[arg1] - addMe->x[arg2];
		yDelta = t->y[arg1] - addMe->y[arg2];
	}
	
	for ( j = 0, i = t->pointCount; i < pointCount; i++, j++ ) {
		x[i]		= addMe->x[j] + xDelta;
		y[i]		= addMe->y[j] + yDelta;
		/*
		oox[i]		= t16 = addMe->oox[j];
		*/
		/*
		if ( t16 < t->xmin ) {
			t->xmin = t16;
		} else if ( t->t16 > t->xmax ) {
			t->xmax = t16;
		}
		*/
		/*
		ooy[i]		= t16 = addMe->ooy[j];
		*/
		/*
		if ( t16 < t->ymin ) {
			t->ymin = t16;
		} else if ( t->t16 > t->ymax ) {
			t->ymax = t16;
		}
		*/
		onCurve[i]	= addMe->onCurve[j];
	}
	
	assert( t->ep[t->contourCount-1] == t->pointCount-1 );
	for ( i = 0; i < t->contourCount; i++ ) {
		sp[i]	= t->sp[i];
		ep[i]	= t->ep[i];
	}	
	for ( j = 0, i = t->contourCount; i < contourCount; i++, j++ ) {
		sp[i]	= (short)(addMe->sp[j] + t->pointCount);
		ep[i]	= (short)(addMe->ep[j] + t->pointCount);
	}	
	
	t->pointCount 	= (short)pointCount;
	t->contourCount = (short)contourCount;

	tsi_FastDeAllocN( t->mem, t->x, T2K_FB_POINTS); /* Contains all the point data */
	/* tsi_DeAllocMem( t->mem, t->oox ); */
	/* t->ooy points into t->oox */
	/* t->y points into t->x */
	if ( t->sp != t->ctrBuffer ) tsi_DeAllocMem( t->mem, t->sp );
	/* t->ep points into t->sp */
	/* tsi_DeAllocMem( t->mem, t->onCurve ); point into oox */

	t->x = x;
	t->y = y;
	t->oox = oox;
	t->ooy = ooy;
	t->onCurve = onCurve;
	t->sp = sp;
	t->ep = ep;
	
	t->curveType = addMe->curveType;
}

void AllocGlyphPointMemory( GlyphClass *t, long pointCountMax )
{
	register long n = pointCountMax + SbPtCount;
	
	t->x				= (long *) tsi_FastAllocN( t->mem, n * ( 2 * sizeof(long) + 2 * sizeof(short) + sizeof(uint8)), T2K_FB_POINTS );
	t->y 				= &t->x[n];
	t->oox				= (short *)&t->y[n];
	t->ooy				= &t->oox[n];
	t->onCurve			= (uint8 *)&t->ooy[n];
	t->pointCountMax 	= pointCountMax;
}
/*
 *
 */
void Delete_GlyphClass( GlyphClass *t )
{
	if ( t != NULL ) {
		tsi_FastDeAllocN( t->mem, t->x, T2K_FB_POINTS); /* contains all the point data now! */
		/* t->y points into t->x */
		/* tsi_DeAllocMem( t->mem, t->oox ); */
		/* t->ooy points into t->oox */
		/* tsi_DeAllocMem( t->mem, t->onCurve ); point into oox */
		if ( t->sp != t->ctrBuffer ) tsi_DeAllocMem( t->mem, t->sp );
		/* t->ep points into t->sp */
		tsi_FastDeAllocN( t->mem, t->hintFragment, T2K_FB_HINTS );
		tsi_DeAllocMem( t->mem, t->componentData );
		
#ifdef ENABLE_T2KE
		tsi_DeAllocMem( t->mem, t->colors );
#endif
		tsi_FastDeAllocN( t->mem, t, T2K_FB_GLYPH );
	}
}

#ifdef ENABLE_FF_CURVE_CONVERSION

/*
 * Adds a point to the glyph->x/y arrays.
 */
static void glyph_AddPointToXY( GlyphClass *t, F26Dot6 xAdd, F26Dot6 yAdd, unsigned char onCurveBit )
{
	register int i, limit;
	register F26Dot6 *x, *y, *xOld, *yOld;
	register uint8 *onCurve, *onCurveOld;
	F26Dot6 *memBase;
	
	if ( t->pointCount >= t->pointCountMax ) { 
		/* ReAllocate */
		memBase = t->x; xOld = t->x; yOld = t->y; onCurveOld = t->onCurve;
		AllocGlyphPointMemory( t, t->pointCount + (t->pointCount >> 2 ) + 64 );
		x = t->x; y = t->y; onCurve = t->onCurve;
		
		limit = t->pointCount + SbPtCount;
		for ( i = 0; i < limit; i++ ) {
			x[i]		= xOld[i];
			y[i]		= yOld[i];
			onCurve[i]	= onCurveOld[i];
		}
		tsi_FastDeAllocN( t->mem, memBase, T2K_FB_POINTS); /* contains all the old point data */
	}
	i = t->pointCount;
	t->x[i]			= xAdd;
	t->y[i]			= yAdd;
	t->onCurve[i]	= onCurveBit;
	t->pointCount	= (short)(i + 1);
}

/*
 * Computes the intersection of line 1 and line 2 in *x, *y.
 *
 *
 * (1) x2 + dx2 * t2 = x1 + dx1 * t1
 * (2) y2 + dy2 * t2 = y1 + dy1 * t1
 *
 *  1  =>  t1 = ( x2 - x1 + dx2 * t2 ) / dx1
 *  +2 =>  y2 + dy2 * t2 = y1 + dy1/dx1 * [ x2 - x1 + dx2 * t2 ]
 *
 *     => t2 * [dy1/dx1 * dx2 - dy2] = y2 - y1 - dy1/dx1*(x2-x1)
 *     => t2(dy1*dx2 - dy2*dx1) = dx1(y2 - y1) + dy1(x1-x2)
 *     => t2 = [dx1(y2-y1) + dy1(x1-x2)] / [dy1*dx2 - dy2*dx1]
 *     => t2 = [dx1(y2-y1) - dy1(x2-x1)] / [dx2*dy1 - dy2*dx1]
 *     t2 = Num/Denom
 *     =>
 *	    Num   = (y2 - y1) * dx1 - (x2 - x1) * dy1;
 *		Denom = dx2 * dy1 - dy2 * dx1;
 *
 */
static void ComputeIntersection( F26Dot6 line1_pt1_x, F26Dot6 line1_pt1_y, F26Dot6 line1_pt2_x, F26Dot6 line1_pt2_y,
								 F26Dot6 line2_pt1_x, F26Dot6 line2_pt1_y, F26Dot6 line2_pt2_x, F26Dot6 line2_pt2_y,
								 F26Dot6 *x, F26Dot6 *y )
{
 	long dx1, dy1, dx2, dy2, x1, y1, x2, y2;
 	long absDX1, absDY1;
	long Num,Denom, t;
	
	
	dx1 = line1_pt2_x - line1_pt1_x;	dy1 = line1_pt2_y - line1_pt1_y;
	dx2 = line2_pt2_x - line2_pt1_x;	dy2 = line2_pt2_y - line2_pt1_y;
 
 	x1 = line1_pt1_x; y1 = line1_pt1_y;
 	x2 = line2_pt1_x; y2 = line2_pt1_y;


	/* This shows what we want to do, unfortuntely this pseudo code would
	 * blow up for large sizes.
	 *
	 *
	 *	Num   = (y2 - y1) * dx1 - (x2 - x1) * dy1;
	 *	Denom = dx2 * dy1 - dy2 * dx1;
	 */
	absDX1 =  dx1; if ( absDX1 < 0 ) absDX1 = -absDX1;
	absDY1 =  dy1; if ( absDY1 < 0 ) absDY1 = -absDY1;
	
	if ( absDX1 >  absDY1 ) {
		Num   = (y2 - y1) - util_FixMul( (x2 - x1), util_FixDiv( dy1, dx1 ) );
		Denom = util_FixMul( dx2, util_FixDiv(dy1,dx1) ) - dy2 ;
	} else if ( absDX1 <  absDY1  ) {
		Num   = util_FixMul((y2 - y1), util_FixDiv(dx1,dy1) ) - (x2 - x1);
		Denom = dx2  - util_FixMul(dy2, util_FixDiv(dx1,dy1) );
	} else { /* absDX1 == absDY1 */
		if ( absDX1 != 0 ) {
			dx1 /= absDX1;
			dy1 /= absDY1;
		
			Num   = (y2 - y1) * dx1 - (x2 - x1) * dy1;
			Denom = dx2 * dy1 - dy2 * dx1;
		} else {
			Num = Denom = 0;
		}
	}

	if ( Denom != 0 ) { /* 3/19/98 changed 0.0 to 0 ---Sampo */
		t = util_FixDiv( Num, Denom );
		*x = (F26Dot6)( x2 + util_FixMul( dx2, t ) );
		*y = (F26Dot6)( y2 + util_FixMul( dy2, t ) );
	} else {
		*x = (F26Dot6)((line1_pt2_x+line2_pt1_x)/2);
		*y = (F26Dot6)((line1_pt2_y+line2_pt1_y)/2);
	}
}

#define fix15 int16
#define fix31 int32
#define ufix16 uint16

/*
 * 
 */
static void SolveQuadratic(
    fix31 a,
    fix31 b,
    fix31 c,
	F16Dot16 tArr[], int *numRoots)
/*
 *  Solves the quadratic equation a*t*t + b*t + c = 0 for t and adds
 *  the roots, if any, to the curve split list.
 */
{
	if ( a != 0 ) {
		F16Dot16 t1, t0, dt;
		int32 val, derrivative, a_t0;
		int i, loop;
		
		/* Ok we really have a 2nd degree equations :-o */
		/* When using a Newton Raphson approximation t1 = t0 - f(t0) / f'(t0) */
		/* f(t)  = a*t*t + b*t + c*/
		/* f'(t) = 2at + b */
		/* We only attempt to find ONE root between 0 and 1.0 */
		
		*numRoots = 0;
		t1 = 0; /* Initial value for first pass */
		for ( loop = 0; loop < 2; loop++ ) {
			for ( i = 0; i < 8; i++ ) {
				t0			= t1;
				a_t0 		= util_FixMul(t0, a);
				val 		= util_FixMul( t0, b + a_t0 ) + c;
				if ( val == 0 ) 					break; /*****/ /* We found a solution! */
				derrivative =  (a_t0 + a_t0) + b;
				if ( derrivative == 0 )				break; /*****/
				t1 			= t0 - util_FixDiv(val, derrivative );
				if ( t1 > ONE16Dot16 || t1 < 0  )	break; /*****/
				dt			= t1 - t0;
				if ( dt < 2 && dt > -2 )			break; /*****/ /* We found a solution! */
			}
			if ( t1 > 0 && t1 < ONE16Dot16 ) {
				*numRoots = 1;
				tArr[0] = t1;
				break; /*****/
			}
			t1 = ONE16Dot16; /* Initial value for second pass */
		}
	} else {
		/* We have just b*t + c = 0 :-) */
		*numRoots = 1;
		tArr[0] = util_FixDiv( -c, b );
	}

#ifdef OLD_FLOATING_POINT_WAY
	/*  This was adapted from csr_crv.c by John Collins. */

	double  d;
	double  root;
	double  t;

	*numRoots = 0; /* Initialize */
	if (a != 0)
    {
    d = (double)b * (double)b - (double)a * (double)c * 4.0;
    if (d >= 0)
        {
        root = sqrt(d);
        t = (root - b) / (double)(a << 1); 
        tArr[0] = (F16Dot16)(ONE16Dot16 * t);
        t = (-root - b) / (double)(a << 1);
        tArr[1] = (F16Dot16)(ONE16Dot16 * t);
        *numRoots = 2;
        }
    }
	else
    {
    if (b != 0)
        {
        t = (double)(-c) / (double)b;
        tArr[0] = (F16Dot16)(ONE16Dot16 * t);
        *numRoots = 1;
        }
    }
#endif /* OLD_FLOATING_POINT_WAY */
}

/*
 * Adapted from csr_crv.c by John Collins.
 */
static void FindCubicInflections(
    fix15 x0,
    fix15 y0,
    fix15 x1,
    fix15 y1,
    fix15 x2,
    fix15 y2,
    fix15 x3,
    fix15 y3,
    F16Dot16 *t)
/*
 *  Tests for an inflection point within the given cubic curve.
 *  If one is found, it's t is return in *t ( between 0 and 1.0 ).
 */
{
	fix31  ax;
	fix31  bx;
	fix31  cx;
	fix31  ay;
	fix31  by;
	fix31  cy;
	fix31  a, b, c;
	F16Dot16 tArr[2];
	int numRoots;

#if CSR_DEBUG >= 2
printf("FindCubicInflections(%d, %d, %d, %d, %d, %d, %d, %d)\n",
    x0, y0, x1, y1, x2, y2, x3, y3);
#endif

	ax = -x0 + (x1 - x2) * 3 + x3;
	bx = (x0 - (x1 << 1) + x2) << 1;
	cx = x1 - x0;

	ay = -y0 + (y1 - y2) * 3 + y3;       
	by = (y0 - (y1 << 1) + y2) << 1;       
	cy = y1 - y0;                        

	a = ((fix31)ay * bx) - ((fix31)by * ax);
	b = (((fix31)ay * cx) - ((fix31)cy * ax)) << 1;
	c = ((fix31)by * cx) - ((fix31)cy * bx);
	tArr[0] = ONE16Dot16 + ONE16Dot16; 	/* Initialize to out of bounds value */
	tArr[1] = ONE16Dot16 + ONE16Dot16;
	*t = ONE16Dot16 + ONE16Dot16; 		/* Initialize to an out of bounds value */
	SolveQuadratic(a, b, c, tArr, &numRoots);
	if ( numRoots > 0 ) {
		if ( tArr[0] > 0 && tArr[0] < ONE16Dot16 ) {
			*t = tArr[0];
			/* if ( tArr[1] > 0 && tArr[1] < ONE16Dot16 ) printf("*****TWO INFLECTIONS ???\n"); */
		} else if ( tArr[1] > 0 && tArr[1] < ONE16Dot16 ) {
			*t = tArr[1];
		}
	}
}


/*
 *  Splits the given cubic at the split points in the given curve
 *  split list.
 *  Note:
 *  The t and c values used in this function are represented as unsigned
 *  16-bit fractions in which the values 0 and 1 are never used. This
 *  allows a value such as 1 - t to be simply calculated as -t. Although
 *  some compilers will generate a warning about a unary minus applied
 *  to an unsigned short, the intended result of producing the two's
 *  complement should be generated.
 *  Returns:
 *      0:  Success
 *      1:  Memory overflow
 *
 *
 * Adapted from csr_crv.c by John Collins.
 */
static int SplitCubic(
    fix15 x0, 
    fix15 y0, 
    fix15 x1, 
    fix15 y1, 
    fix15 x2, 
    fix15 y2, 
    fix15 x3, 
    fix15 y3, 
    F16Dot16 split_t,
    fix15 xA[],
    fix15 yA[],
    fix15 xB[],
    fix15 yB[] )
{
	ufix16  c10, c11, c20, c21, c22, c30, c31, c32, c33;
	fix15   xSplit, ySplit;


#define rnd16 32768L

    /* Set up interpolation coefficients */
    assert( split_t > 0 && split_t < 0x10000 );
    c11 = (ufix16)split_t;
    c22 = (ufix16)(((fix31)c11 * c11 + rnd16) >> 16);
    c33 = (ufix16)(((fix31)c22 * c11 + rnd16) >> 16);
    c10 = (ufix16)(-c11);                 /* See note in function header! */
    c20 = (ufix16)(((fix31)c10 * c10 + rnd16) >> 16);
    c30 = (ufix16)(((fix31)c20 * c10 + rnd16) >> 16);
    c21 = (ufix16)-(c22 + c20);         /* See note in function header! */
    c32 = (ufix16)((c22 - c33) * 3);
    c31 = (ufix16)((c20 - c30) * 3);

    /* Calculate coordinates of split point */
    xSplit = (fix15)(
        ((fix31)x0 * c30 + 
         (fix31)x1 * c31 + 
         (fix31)x2 * c32 + 
         (fix31)x3 * c33 + 
         rnd16) >> 16);
    ySplit = (fix15)(
        ((fix31)y0 * c30 + 
         (fix31)y1 * c31 + 
         (fix31)y2 * c32 + 
         (fix31)y3 * c33 + 
         rnd16) >> 16);

    /* Output sub-curve up to split point */
    xA[0] = x0;
    yA[0] = y0;
    
    xA[1] = (fix15)(((fix31)x0 * c10 + (fix31)x1 * c11 + rnd16) >> 16);
    yA[1] = (fix15)(((fix31)y0 * c10 + (fix31)y1 * c11 + rnd16) >> 16);
    
    xA[2] = (fix15)(((fix31)x0 * c20 + (fix31)x1 * c21 + (fix31)x2 * c22 + rnd16) >> 16);
    yA[2] = (fix15)(((fix31)y0 * c20 + (fix31)y1 * c21 + (fix31)y2 * c22 + rnd16) >> 16);
    
    xA[3] = xSplit;
    yA[3] = ySplit;
        
    /* Set up remainder of curve */
    xB[0] = xSplit;
    yB[0] = ySplit;
    
    xB[1] = (fix15)(((fix31)x1 * c20 + (fix31)x2 * c21 + (fix31)x3 * c22 +  rnd16) >> 16);
    yB[1] = (fix15)(((fix31)y1 * c20 + (fix31)y2 * c21 + (fix31)y3 * c22 +  rnd16) >> 16);
    
    xB[2] = (fix15)(((fix31)x2 * c10 + (fix31)x3 * c11 + rnd16) >> 16);
    yB[2] = (fix15)(((fix31)y2 * c10 + (fix31)y3 * c11 + rnd16) >> 16);
    
	xB[3] = x3;
	yB[3] = y3;
	
	return 0; /*****/
}

/*
 * Computes the mid point and tangent for the 3rd degree bezier
 * described by x0-3, and y0-3.
 */
static void ComputeMidPointInfo( F26Dot6 x0, F26Dot6 y0, F26Dot6 x1, F26Dot6 y1, F26Dot6 x2, F26Dot6 y2, F26Dot6 x3, F26Dot6 y3,
								 F26Dot6 *midX, F26Dot6 *midY, F26Dot6 *dx, F26Dot6 *dy )
{
	/* Coordinates at t = 0.5 */
	*midX = (F26Dot6)((x0 + 3*(x1+x2) + x3 + 4) >> 3);
	*midY = (F26Dot6)((y0 + 3*(y1+y2) + y3 + 4) >> 3);
	/* Direction at midX, midY */
	*dx   = (F26Dot6)(x3 + x2 - x1 - x0);
	*dy   = (F26Dot6)(y3 + y2 - y1 - y0);
}

/*	
 * IN: 4 points in x0-3, y0-3
 * ASSUMPTIONS: There can be no inflection points in the input data!!!
 *        		They have to be removed before we get here by splitting
 *				the 3rd degree curves at the inflection points.
 * 
 */
static void Convert3rdDegreeBezierTo2ndDegreeBSpline( GlyphClass *glyph, F26Dot6 x0, F26Dot6 y0, F26Dot6 x1, F26Dot6 y1, F26Dot6 x2, F26Dot6 y2, F26Dot6 x3, F26Dot6 y3 )
{
	F26Dot6 midX, midY, dx, dy;
	F26Dot6 off1x, off1y, off2x, off2y;
	F26Dot6 xqOut, yqOut;
	uint8 onCurveOut;

	ComputeMidPointInfo( x0, y0, x1, y1, x2, y2, x3, y3, &midX, &midY, &dx, &dy );	
	
	ComputeIntersection( x0, y0, x1, y1, midX, midY, (F26Dot6)(midX-dx), (F26Dot6)(midY-dy), &off1x, &off1y);
	ComputeIntersection( x3, y3, x2, y2, midX, midY, (F26Dot6)(midX+dx), (F26Dot6)(midY+dy), &off2x, &off2y);

	/* on--off1-on-off2--on */ /* preserves the middle tangent direction and position, but the price is one more point  */
	xqOut	 		= x0;
	yqOut	 		= y0;
	onCurveOut		= 1;
	glyph_AddPointToXY( glyph, xqOut, yqOut, onCurveOut );
		
	xqOut	 		= off1x;
	yqOut	 		= off1y;
	onCurveOut		= 0;
	glyph_AddPointToXY( glyph, xqOut, yqOut, onCurveOut );
	
	xqOut	 		= midX;
	yqOut	 		= midY;
	onCurveOut		= 1;
	glyph_AddPointToXY( glyph, xqOut, yqOut, onCurveOut );
		
	xqOut	 		= off2x;
	yqOut	 		= off2y;
	onCurveOut		= 0;
	glyph_AddPointToXY( glyph, xqOut, yqOut, onCurveOut );
	
	/*
	xqOut	 		= x3;
	yqOut	 		= y3;
	onCurveOut		= 1;
	*/
}

/*	
 * IN: 4 points in x0-3, y0-3
 * ASSUMPTIONS: There can be no inflection points in the input data!!!
 *        		They have to be removed before we get here by splitting
 *				the 3rd degree curves at the inflection points.
 * 
 */
static void Convert3rdDegreeBezierTo1stDegreeBSpline( GlyphClass *glyph, F26Dot6 x0, F26Dot6 y0, F26Dot6 x1, F26Dot6 y1, F26Dot6 x2, F26Dot6 y2, F26Dot6 x3, F26Dot6 y3 )
{
	long dZ, error,maxError = 8; 
	F26Dot6 Arr[16*9], *wp, count, midX, midY;;

	wp = &Arr[0];
	/* Midpoint 50% of line 0-3 to parametric midpoint 0.5 of the curve. */
	/* dZ = (z0+3*z1+3*z2+z3)/8 - (4*(z0+z3)/8), => */
	dZ		= (( 3*(x1+x2-x0-x3) + 4) >> 3); if ( dZ < 0 ) dZ = -dZ;
	error 	= dZ;
	dZ		= (( 3*(y1+y2-y0-y3) + 4) >> 3); if ( dZ < 0 ) dZ = -dZ;
	if ( error < dZ ) error = dZ;

	for ( count = 0; error > maxError; count++ ) {
		error >>= 2; /* We could solve this directly instead... */
	}
	
	
	for (;;) {
		if ( count > 0 ) {
			F26Dot6 xB, yB, xC, yC;
			
			midX = ((x0 + 3*(x1+x2) + x3 + 4) >> 3);
			midY = ((y0 + 3*(y1+y2) + y3 + 4) >> 3);
			count--;
			xC = ((x2+x3+1)>>1);
			yC = ((y2+y3+1)>>1);
			xB = ((xC + ((x1+x2+1)>>1) + 1 ) >> 1);
			yB = ((yC + ((y1+y2+1)>>1) + 1 ) >> 1);
			/* mid, B, C, 3  */
			
			*wp++ = midX;
			*wp++ = midY;
			*wp++ = xB;
			*wp++ = yB;
			*wp++ = xC;
			*wp++ = yC;
			*wp++ = x3;
			*wp++ = y3;
			*wp++ = count;

			/* 0, B, C, mid */
			xB = ((x0+x1+1)>>1);
			yB = ((y0+y1+1)>>1);
			xC = ((xB + ((x1+x2+1)>>1) + 1 ) >> 1);
			yC = ((yB + ((y1+y2+1)>>1) + 1 ) >> 1);
			
			
			x1 = xB;
			y1 = yB;
			x2 = xC;
			y2 = yC;
			x3 = midX;
			y3 = midY;
			continue; /*****/
		} else {
			/* drawLine( t, x0, y0, x3, y3 ); */
			glyph_AddPointToXY( glyph, x0, y0, 1 );

		}
		if ( wp > Arr ) {
			count	= *(--wp);
			y3 		= *(--wp);
			x3		= *(--wp);
			y2 		= *(--wp);
			x2		= *(--wp);
			y1		= *(--wp);
			x1		= *(--wp);
			y0		= *(--wp);
			x0		= *(--wp);
			
			continue; /*****/
		}
		break; /*****/
	}
	
}

/*	
 * IN: 3 points in x0-2, y0-3
 * 
 */
static void Convert2ndDegreeParabolaTo3rdDegreeBezier( GlyphClass *glyph, F26Dot6 x0, F26Dot6 y0, F26Dot6 x1, F26Dot6 y1, F26Dot6 x2, F26Dot6 y2 )
{
	F26Dot6 xqOut, yqOut;
	uint8 onCurveOut;
	
	xqOut 			= x0;
	yqOut 			= y0;
	onCurveOut		= 1;
	glyph_AddPointToXY( glyph, xqOut, yqOut, onCurveOut );
	
	xqOut	 		= ((x0 + x1 + x1 + 1)/3);
	yqOut	 		= ((y0 + y1 + y1 + 1)/3);
	onCurveOut		= 0;
	glyph_AddPointToXY( glyph, xqOut, yqOut, onCurveOut );
	
	xqOut	 		= ((x2 + x1 + x1 + 1)/3);
	yqOut	 		= ((y2 + y1 + y1 + 1)/3);
	onCurveOut		= 0;
	glyph_AddPointToXY( glyph, xqOut, yqOut, onCurveOut );
	
	/*
	xqOut	 		= x2;
	yqOut	 		= y2;
	onCurveOut		= 1;
	*/
}

/*	
 * IN: 3 points in x0-2, y0-32
 */
static void Convert2ndDegreeParabolaTo1stDegreeBezier( GlyphClass *glyph, F26Dot6 x0, F26Dot6 y0, F26Dot6 x1, F26Dot6 y1, F26Dot6 x2, F26Dot6 y2 )
{
	long dZ, error, maxError = 8;
	F26Dot6 Arr[16*7], *wp, midX, midY, count;

	wp = &Arr[0];
	/*
	dX		= ((x0+x2+1)>>1) - ((x0 + x1 + x1 + x2 + 2) >> 2); if ( dX < 0 ) dX = -dX;
	dY		= ((y0+y2+1)>>1) - ((y0 + y1 + y1 + y2 + 2) >> 2); if ( dY < 0 ) dY = -dY;
	*/
	dZ		= ((x0 - x1 - x1 + x2 ) >> 2); if ( dZ < 0 ) dZ = -dZ;
	error 	= dZ;
	dZ		= ((y0 - y1 - y1 + y2 ) >> 2); if ( dZ < 0 ) dZ = -dZ;
	if ( dZ > error ) error = dZ;
	for ( count = 0; error > maxError; count++ ) {
		error >>= 2; /* We could solve this directly instead... */
	}

	for (;;) {
		if ( count > 0 ) {
			midX = ((x0 + x1 + x1 + x2 + 2) >> 2);
			midY = ((y0 + y1 + y1 + y2 + 2) >> 2);
			count--;
			/* InnerDrawParabola( t, x0, y0, ((x0+x1+1)>>1), ((y0+y1+1)>>1), midX, midY, count); */
			
			*wp++ = midX;
			*wp++ = midY;
			*wp++ = ((x1+x2+1)>>1);
			*wp++ = ((y1+y2+1)>>1);
			*wp++ = x2;
			*wp++ = y2;
			*wp++ = count;
			/* InnerDrawParabola( t, midX, midY, ((x1+x2+1)>>1), ((y1+y2+1)>>1), x2, y2, count); */
			x1 = ((x0+x1+1)>>1);
			y1 = ((y0+y1+1)>>1);
			x2 = midX;
			y2 = midY;
			continue; /*****/
		} else {
			/* DrawLine( t, x0, y0, x2, y2 )); */
			glyph_AddPointToXY( glyph, x0, y0, 1 );
		}
		if ( wp > Arr ) {
			count	= *(--wp);
			y2 		= *(--wp);
			x2		= *(--wp);
			y1		= *(--wp);
			x1		= *(--wp);
			y0		= *(--wp);
			x0		= *(--wp);
			
			continue; /*****/
		}
		break; /*****/
	}
}


typedef void (*FF_Convert3rdDegreeFuncPtr)( GlyphClass *glyph, F26Dot6, F26Dot6, F26Dot6, F26Dot6, F26Dot6, F26Dot6, F26Dot6, F26Dot6);
typedef void (*FF_Convert2ndDegreeFuncPtr)( GlyphClass *glyph, F26Dot6, F26Dot6, F26Dot6, F26Dot6, F26Dot6, F26Dot6);

/*
 * Converts a glyph with glyph->curveType == 3 or 2 into a glyph with
 * glyph->curveType == curveTypeOut.
 * curveTypeOut can be set to 1,2 or 3.
 */
GlyphClass *FF_ConvertGlyphSplineTypeInternal( GlyphClass *glyph, short curveTypeOut )
{
    int startPoint, lastPoint, ctr, oldPointCount, first_ptA;
    int ptA, ptB;
	F26Dot6 Ax, Bx, Ay, By;
	register F26Dot6 *x, *y;
	uint8 *onCurve;
	long pointCount, totalPointCount;
	FF_Convert3rdDegreeFuncPtr map3F = NULL;
	FF_Convert2ndDegreeFuncPtr map2F = NULL;
	short curveTypeIn = glyph->curveType;

	assert( curveTypeOut == 3 || curveTypeOut == 2 || curveTypeOut == 1 );
	assert( curveTypeIn == 3 || curveTypeIn == 2 );
	if ( curveTypeIn == curveTypeOut ) return glyph; /*****/
	if ( curveTypeIn == 2 ) {
		map2F =  curveTypeOut == 3 ? Convert2ndDegreeParabolaTo3rdDegreeBezier : Convert2ndDegreeParabolaTo1stDegreeBezier;
	} else if ( curveTypeIn == 3 ) {
		map3F =  curveTypeOut == 2 ? Convert3rdDegreeBezierTo2ndDegreeBSpline  : Convert3rdDegreeBezierTo1stDegreeBSpline;
	}
							
	oldPointCount 	= glyph->pointCount;
	totalPointCount	= oldPointCount + SbPtCount;
	if ( totalPointCount <= SbPtCount ) return glyph; /*****/
	
	x = (F26Dot6 *)tsi_AllocMem( glyph->mem, sizeof(F26Dot6) * totalPointCount * 3);
	y = &x[totalPointCount];
	onCurve = (uint8 *)&y[totalPointCount];
	for ( ptA = 0; ptA < totalPointCount; ptA++ ) {
		x[ptA] = glyph->x[ptA];
		y[ptA] = glyph->y[ptA];
		onCurve[ptA] = glyph->onCurve[ptA];
	}
	glyph->pointCount = 0;
	
	for ( ctr = 0; ctr < glyph->contourCount; ctr++ ) {
		ptA = startPoint = glyph->sp[ctr];
		lastPoint = glyph->ep[ctr];
		pointCount = lastPoint - startPoint + 1;
		
		/* Reset sp */
		glyph->sp[ctr] = glyph->pointCount;

		if ( curveTypeIn == 3 ) {
				while ( !onCurve[ ptA ] && ptA <= lastPoint) {
					ptA = (ptA + 1);
				}
				if ( ptA > lastPoint ) {
					/* Something is bogus in the outlines */
					for ( ptA = startPoint;  ptA <= lastPoint; ptA++ ) {
						glyph_AddPointToXY( glyph, x[ptA], y[ptA], onCurve[ptA] );
					}
					/* Reset ep */
					glyph->ep[ctr] = (short)(glyph->pointCount-1);
					continue; /*****/
				}
				assert ( ptA <= lastPoint );
				assert( onCurve[ ptA ] );
				Ax  = x[ ptA ];	Ay  = y[ ptA ];
		} else {
			if ( !onCurve[ ptA ]  ) {
				ptA = lastPoint;
				if ( onCurve[ptA] ) {
					Ax  = x[ ptA ];	Ay  = y[ ptA ];
				} else {
					Ax  = (F26Dot6)((x[ ptA ] + x[ startPoint ] + 1) >> 1);
					Ay  = (F26Dot6)((y[ ptA ] + y[ startPoint ] + 1) >> 1);
				}
			} else {
				Ax  = x[ ptA ];	Ay  = y[ ptA ];
			}
		}
		
		first_ptA = ptA;
		while ( pointCount > 0) {
			ptB = (ptA + 1);
			if ( ptB > lastPoint ) ptB = startPoint;
			Bx  = x[ ptB ];	By  = y[ ptB ];
			if ( onCurve[ ptB ] ) {
				/* A straight line. */
				/* line( t, Ax, Ay, Bx, By ); */
				glyph_AddPointToXY( glyph, Ax, Ay, 1 );
				ptA = ptB; Ax = Bx; Ay = By;
				pointCount--;
			} else {
				F26Dot6 Cx, Dx, Cy, Dy;
				int ptC, ptD;
				/* A curve. */
				ptC = (ptB + 1);
				if ( ptC > lastPoint ) ptC = startPoint;
				if ( curveTypeIn == 2 ) {
					if ( onCurve[ ptC ] ) {
						Cx  = x[ ptC ];	Cy  = y[ ptC ];
						/* for the next pass */
						ptA = ptC; pointCount -= 2;
					} else {
						Cx  = (F26Dot6)((Bx + x[ ptC ] + 1) >> 1);
						Cy  = (F26Dot6)((By + y[ ptC ] + 1) >> 1);
						/* for the next pass */
						ptA = ptB; pointCount--;
					}
					/* 2ndDegreeBezier( Ax, Ay, Bx, By, Cx, Cy ) */
					map2F( glyph, Ax, Ay, Bx, By, Cx, Cy );
					Ax = Cx; Ay = Cy;
				} else {
					assert( curveTypeIn == 3 );
					ptD = (ptC + 1);
					if ( ptD > lastPoint ) ptD = startPoint;
					
					assert( !onCurve[ ptC ] );
					assert( onCurve[ ptD ] );
					Cx  = x[ ptC ];	Cy  = y[ ptC ];
					Dx  = x[ ptD ];	Dy  = y[ ptD ];
					/* 3rdDegreeBezier( Ax, Ay, Bx, By, Cx, Cy, Dx, Dy ) */
					{
						int dxAB, dxBC, dxCD;
						int dyAB, dyBC, dyCD;
						long area1, area2;
						F16Dot16 tInflection;
						int16 Axs, Ays, Bxs, Bys, Cxs, Cys, Dxs, Dys, sc = 0;
						F26Dot6 tmp, max;
						
						dxAB = Bx - Ax; dyAB = By - Ay;
						dxBC = Cx - Bx; dyBC = Cy - By;
						dxCD = Dx - Cx; dyCD = Dy - Cy;
						
						area1 = dxAB * dyBC - dyAB * dxBC;
						area2 = dxBC * dyCD - dyBC * dxCD;
						
						tInflection = 0; /* Initialize to out of bounds value */
						Axs = Ays = Bxs = Bys = Cxs = Cys = Dxs = Dys = 0;
						if ( (area1 > 0 && area2 < 0) || (area1 < 0 && area2 > 0) ) {
							/* We have an inflection! */
							tmp = Ax; if ( tmp < 0 ) tmp = -tmp; max = tmp;
							tmp = Ay; if ( tmp < 0 ) tmp = -tmp; if ( tmp > max ) max = tmp;
							tmp = Bx; if ( tmp < 0 ) tmp = -tmp; if ( tmp > max ) max = tmp;
							tmp = By; if ( tmp < 0 ) tmp = -tmp; if ( tmp > max ) max = tmp;
							tmp = Cx; if ( tmp < 0 ) tmp = -tmp; if ( tmp > max ) max = tmp;
							tmp = Cy; if ( tmp < 0 ) tmp = -tmp; if ( tmp > max ) max = tmp;
							tmp = Dx; if ( tmp < 0 ) tmp = -tmp; if ( tmp > max ) max = tmp;
							tmp = Dy; if ( tmp < 0 ) tmp = -tmp; if ( tmp > max ) max = tmp;

							if ( max > 32768 ) {
								int16 addMe;

								do {
									sc++;
									max >>= 1;
								} while ( max > 32768 );
								addMe = (int16)(1 << (sc-1));
								Axs = (int16)((Ax + addMe) >> sc);
								Ays = (int16)((Ay + addMe) >> sc);
								Bxs = (int16)((Bx + addMe) >> sc);
								Bys = (int16)((By + addMe) >> sc);
								Cxs = (int16)((Cx + addMe) >> sc);
								Cys = (int16)((Cy + addMe) >> sc);
								Dxs = (int16)((Dx + addMe) >> sc);
								Dys = (int16)((Dy + addMe) >> sc);
							} else {
								Axs = (int16)Ax;
								Ays = (int16)Ay;
								Bxs = (int16)Bx;
								Bys = (int16)By;
								Cxs = (int16)Cx;
								Cys = (int16)Cy;
								Dxs = (int16)Dx;
								Dys = (int16)Dy;
							}

							FindCubicInflections(Axs, Ays, Bxs, Bys, Cxs, Cys, Dxs, Dys, &tInflection );
						}
						if ( tInflection > 0 && tInflection < ONE16Dot16 ) {
							fix15 xA[4], yA[4], xB[4], yB[4];
														
							SplitCubic( Axs, Ays, Bxs, Bys, Cxs, Cys, Dxs, Dys, tInflection, xA, yA, xB, yB );
							map3F( glyph, 	(F26Dot6)xA[0] << sc, (F26Dot6)yA[0] << sc, (F26Dot6)xA[1] << sc, (F26Dot6)yA[1] << sc,
											(F26Dot6)xA[2] << sc, (F26Dot6)yA[2] << sc, (F26Dot6)xA[3] << sc, (F26Dot6)yA[3] << sc );
							map3F( glyph,	(F26Dot6)xB[0] << sc, (F26Dot6)yB[0] << sc, (F26Dot6)xB[1] << sc, (F26Dot6)yB[1] << sc,
											(F26Dot6)xB[2] << sc, (F26Dot6)yB[2] << sc, (F26Dot6)xB[3] << sc, (F26Dot6)yB[3] << sc );
						} else {
							map3F( glyph, Ax, Ay, Bx, By, Cx, Cy, Dx, Dy );
						}
					}
					ptA = ptD; Ax = Dx; Ay = Dy;
					pointCount -= 3;
				}   /* curvetype == 3 */
			}
		}
		/* Reset ep */
		glyph->ep[ctr] = (short)(glyph->pointCount-1);
	}
	ptA = glyph->pointCount;
	for ( ptB = totalPointCount -  SbPtCount; ptB < totalPointCount; ptB++ ) {
		glyph->x[ptA] = x[ptB];
		glyph->y[ptA] = y[ptB];
		glyph->onCurve[ptA] = onCurve[ptB];
		ptA++;
	}

	glyph->curveType = curveTypeOut;
	tsi_DeAllocMem( glyph->mem, x );

	return glyph; /*****/
}

#endif /* ENABLE_FF_CURVE_CONVERSION */


/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: R:/src/FontFusion/Source/Core/rcs/glyph.c 1.11 2000/05/11 13:37:39 reggers release $
 *                                                                           *
 *     $Log: glyph.c $
 *     Revision 1.11  2000/05/11 13:37:39  reggers
 *     Added support for outline curve conversion. (Sampo)
 *     Revision 1.10  2000/04/19 19:30:50  reggers
 *     Slight performance tweak.
 *     Revision 1.9  2000/02/25 17:45:38  reggers
 *     STRICT warning cleanup.
 *     Revision 1.8  2000/01/27 17:15:02  reggers
 *     Re-did change made last time, only affected formatting!
 *     Revision 1.7  2000/01/27 16:37:40  reggers
 *     Prevent double close contour.
 *     Revision 1.6  1999/10/19 16:21:43  shawn
 *     Removed the UNUSED() macro from function declarations.
 *     
 *     Revision 1.5  1999/10/18 16:59:44  jfatal
 *     Changed all include file names to lower case.
 *     Revision 1.4  1999/09/30 15:11:24  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.3  1999/07/16 17:51:55  sampo
 *     Sampo work. Drop #8 July 16, 1999
 *     Revision 1.2  1999/05/17 15:56:58  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/

