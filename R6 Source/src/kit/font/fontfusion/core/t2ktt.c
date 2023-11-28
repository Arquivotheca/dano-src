/*
 * T2KTT.c
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
#ifdef ENABLE_NATIVE_TT_HINTS


#include "t2kstrm.h"
#include "truetype.h"
#include "t2ktt.h"
#include "util.h"



/*
 * The T2KTTClass constructor
 */
T2KTTClass *New_T2KTTClass( tsiMemObject *mem, InputStream *in, void *fontPtr  )
{
	T2KTTClass *t = (T2KTTClass *) tsi_AllocMem( mem, sizeof( T2KTTClass ) );
	fnt_GlobalGraphicStateType *g = &t->globalGS;
	sfnt_DirectoryEntry *dirEntry;
	register long  i, n;
	sfntClass *font = (sfntClass *)fontPtr;
	maxpClass *maxp = font->maxp;
	
	g->maxp				= maxp;
	t->mem				= mem;
	t->UPEM 			= GetUPEM( font );
	assert( maxp != NULL );
	
	t->xPixelsPerEm = -1;	/* Initialize to bogus values */
	t->yPixelsPerEm	= 0x7fffa55a;
	
	/* Pointer to instruction definition area */
	g->function = (FntFunc *)tsi_AllocMem( mem, MAXBYTE_INSTRUCTIONS*sizeof(voidFunc) + MAXANGLES * (sizeof(fnt_FractPoint) + sizeof(int16)) );
	fnt_Init( g ); /* Also sets globalGS.anglePoint and globalGS.angleDistance */

	/* The stack area */
	g->stackBase	= (F26Dot6 *)tsi_AllocMem( mem, maxp->maxStackElements * sizeof (F26Dot6) );

	/* The storage area */
	g->store 		= (F26Dot6 *)tsi_AllocMem( mem, maxp->maxStorage * sizeof (F26Dot6) );
	/* Initialize the store to zero */
	for ( n = maxp->maxStorage, i = 0; i < n; i++) {
		g->store[i] = 0;
	}
	
	n = maxp->maxPoints;
	if ( maxp->maxCompositePoints > n ) {
		n = maxp->maxCompositePoints;
	}
	n 			= n + SbPtCount;
	t->ptr32	= (F26Dot6 *)tsi_AllocMem( t->mem, 2 * n * sizeof( F26Dot6 ) + n * sizeof( uint8 ) );

	/* The control value table */
	g->controlValueTable = NULL;
	t->ocvt				 = NULL;
	t->numCVTs		 	 = 0;
	/* The cvt table */
	dirEntry = GetTableDirEntry_sfntClass( font, tag_ControlValue );
	if ( dirEntry != NULL ) {
		int16 *ocvt;
		InputStream *stream = New_InputStream2( t->mem, in, dirEntry->offset, dirEntry->length , 0, NULL );
		
		n = (long)(dirEntry->length / sizeof(int16));
		g->cvtCount = n;
		assert( sizeof(int16) == 2 );
		g->controlValueTable = (F26Dot6 *)tsi_AllocMem( mem, n * sizeof (F26Dot6) );
		t->ocvt = ocvt = (int16 *)tsi_AllocMem( mem, n * sizeof(int16) );
		t->numCVTs = n;
		for ( ; n > 0; n--) {
			*ocvt++ = ReadInt16( stream );
		}
		Delete_InputStream( stream, NULL );
	}
	

	/* The function definitions identifiers */
	g->funcDef = (fnt_funcDef *)tsi_AllocMem( mem, maxp->maxFunctionDefs * sizeof (fnt_funcDef) );

	/* The instruction definitions identifiers */
	g->instrDef = (fnt_instrDef *)tsi_AllocMem( mem, maxp->maxInstructionDefs * sizeof (fnt_instrDef) );

	assert( PREPROGRAM		== 0 );
	assert( FONTPROGRAM 	== 1 );
	assert( MAXPREPROGRAMS	== 2 );
	/* PREPROGRAM, FONTPROGRAM */
	for ( i = 0; i < MAXPREPROGRAMS; i++ ) {
		g->pgmList[i] = NULL;
		dirEntry = GetTableDirEntry_sfntClass( font, i == 0 ? tag_PreProgram : tag_FontProgram );
		if ( dirEntry != NULL ) {
			Seek_InputStream( in, dirEntry->offset );
			t->pgmLength[i] = dirEntry->length;
			g->pgmList[i]   = (uint8 *)tsi_AllocMem( mem, t->pgmLength[i] );
			ReadSegment( in, g->pgmList[i], (long)(t->pgmLength[i]) );
		}
	}
	/* Initialize the DefaultParameterBlock */ 
	{
		register fnt_ParameterBlock *par = &g->defaultParBlock;
	
		par->RoundValue			= fnt_RoundToGrid;
		par->minimumDistance 	= fnt_pixelSize;
		par->wTCI				= fnt_pixelSize + fnt_pixelSize / 16; /* == 17*pixSize/16  */
		par->sWCI 				= 0;
		par->sW   				= 0;
		par->autoFlip 			= true;
		par->deltaBase 			= 9;
		par->deltaShift 		= 3;
		par->angleWeight 		= 128;
		par->scanControl 		= 0x7C0 | ACTIVATE_DROPOUTCONTROL | SMART_DROPOUT;
		par->instructControl 	= 0;
	}
#define TWILIGHT_ZONE_CONTOURS		1

	{
		/* Allocate the twilight :-o zone */
		F26Dot6 *ptr32;
		int16 *ptr16;
		uint8 *ptr8;

		n 					= t->maxTwilightPoints = (int16)maxp->maxTwilightPoints;
		ptr32				= (F26Dot6 *)tsi_AllocMem( mem, 4 * n * sizeof(F26Dot6) + 2 * n * sizeof(int16) + 2 * n * sizeof(uint8) );
		t->elements[0].x 	= ptr32; ptr32 += n;
		t->elements[0].y 	= ptr32; ptr32 += n;
		t->elements[0].ox 	= ptr32; ptr32 += n;
		t->elements[0].oy 	= ptr32; ptr32 += n;
		
		ptr16 = (int16 *)ptr32;
		t->elements[0].oox 	= ptr16; ptr16 += n;
		t->elements[0].ooy 	= ptr16; ptr16 += n;
		
		ptr8 = (uint8 *)ptr16;
		t->elements[0].onCurve 	= ptr8; ptr8 += n;
		t->elements[0].f 		= ptr8;
		
		t->spZeroWord = 0;
		t->epZeroWord = 0;
		t->elements[0].nc  	= TWILIGHT_ZONE_CONTOURS;
		t->elements[0].padWord = 0;
		t->elements[0].sp  	= &t->spZeroWord;
		t->elements[0].ep  	= &t->epZeroWord;
	}

	g->localParBlock = g->defaultParBlock;	/* moved from below 10/18/99 --- Sampo */
	if ( g->pgmList[FONTPROGRAM] != NULL ) {
		i = FONTPROGRAM;
		g->pgmIndex = (uint8)i;
		/*g->localParBlock = g->defaultParBlock;*/
		g->init = true;/* ??? */
		fnt_Execute( t->elements, g->pgmList[i], g->pgmList[i] + t->pgmLength[i], g, NULL);
	}
	return t; /******/
}


/*
 * Integer mul and then shift rounding
 */
FUNCTION LOCAL_PROTO F26Dot6 fnt_FastIRound(register fnt_GlobalGraphicStateType *globalGS, register F26Dot6 value)
{
	value *= globalGS->nScale;
	value += globalGS->dScaleDiv2;
	value >>= globalGS->dShift;
	return( value );
}

/*
 * Integer mul and then div rounding
 */
FUNCTION LOCAL_PROTO F26Dot6 fnt_IRound(register fnt_GlobalGraphicStateType *globalGS, register F26Dot6 value)
{
	SROUND( value, globalGS->nScale, globalGS->dScale, globalGS->dScaleDiv2 );
	return( value );
}


/*
 * Fixed (16.16) Mul rounding
 */
FUNCTION LOCAL_PROTO F26Dot6 fnt_FixRound(fnt_GlobalGraphicStateType *globalGS, F26Dot6 value)
{
	return( util_FixMul( value, globalGS->fixedScale ) );
}

/*
 *
 */
void SetScale_T2KTTClass( T2KTTClass *t, long xPixelsPerEm, long yPixelsPerEm )
{
	if ( t != NULL  && (t->xPixelsPerEm != xPixelsPerEm || t->yPixelsPerEm != yPixelsPerEm) ) {
		fnt_GlobalGraphicStateType *g = &t->globalGS;
		register long i, n;
		int16 nScale;
		int16 dShift;
		int32 dScale;
		int32 dScaleDiv2;
		F16Dot16 fixedScale;
		
		T2KScaleInfo scale;
		

		t->xPixelsPerEm = xPixelsPerEm;
		t->yPixelsPerEm = yPixelsPerEm;
		g->xStretch = ONEFIX;
		g->yStretch = ONEFIX;
		if ( xPixelsPerEm == yPixelsPerEm ) {
			g->identityTransformation = true;
			g->pixelsPerEm = (uint16)xPixelsPerEm;
		} else if ( xPixelsPerEm > yPixelsPerEm ) {
			g->identityTransformation = false;
			g->pixelsPerEm = (uint16)xPixelsPerEm;
			g->yStretch = util_FixDiv( yPixelsPerEm, xPixelsPerEm );
		} else {
			g->identityTransformation = false;
			g->pixelsPerEm = (uint16)yPixelsPerEm;
			g->xStretch = util_FixDiv( xPixelsPerEm, yPixelsPerEm );
		}
		g->fpem = g->pixelsPerEm; g->fpem <<= 16;
		g->pointSize = g->pixelsPerEm;
		g->engine[0] = 0; /* Don't compensate for engine characteristics */
		g->engine[1] = 0;
		g->engine[2] = 0;
		g->engine[3] = 0;
		
		/* Set the scale factors */
		setT2KScaleFactors( g->pixelsPerEm, t->UPEM, &scale );
		nScale = scale.nScale;
		dShift = scale.dShift;
		dScale = scale.dScale;
		dScaleDiv2 = scale.dScaleDiv2;
		fixedScale = scale.fixedScale;

		n = t->numCVTs;
		switch ( scale.scaleType )  {
		case T2K_IMULSHIFT:
			g->ScaleFunc = fnt_FastIRound;
			for ( i = 0; i < n; i++ ) {
				register int32 tmp32 = t->ocvt[i];
				tmp32 *= nScale;
				tmp32 += dScaleDiv2;
				tmp32 >>= dShift;
				g->controlValueTable[i] = tmp32;
			}
			break; /*****/
		case T2K_IMULDIV:
			g->ScaleFunc = fnt_IRound;
			for ( i = 0; i < n; i++ ) {
				register int32 tmp32 = t->ocvt[i];
				SROUND( tmp32, nScale, dScale, dScaleDiv2);
				g->controlValueTable[i] = tmp32;
			}
			break; /*****/
		case T2K_FIXMUL:
			g->ScaleFunc = fnt_FixRound;
			for ( i = 0; i < n; i++ ) {
				g->controlValueTable[i] = util_FixMul( t->ocvt[i], fixedScale );
			}
			break; /*****/
		
		}
		
		/* Set the real versions in the global TT graphics state */
		g->nScale = nScale;
		g->dShift = dShift;
		g->dScale = dScale;
		g->dScaleDiv2 = dScaleDiv2;
		g->fixedScale = fixedScale;
		
		{
			fnt_ElementType	*elements = &t->elements[0];
			n 					= t->maxTwilightPoints;
			/* Clear the twilight zone */
			for ( i = 0; i < n; i++ ) {
				elements->x[i]			= 0;
				elements->y[i]			= 0;
				elements->ox[i]			= 0;
				elements->oy[i]			= 0;
				elements->oox[i]		= 0;
				elements->ooy[i]		= 0;
				elements->onCurve[i]	= 0;
				elements->f[i]			= 0;
			}
		}

		if ( g->pgmList[PREPROGRAM] != NULL ) {
			i = PREPROGRAM;
			g->pgmIndex = (uint8)i;
			g->localParBlock = g->defaultParBlock;

			g->init = true;
			g->instrDefCount = 0;
			fnt_Execute( t->elements, g->pgmList[i], g->pgmList[i] + t->pgmLength[i], g, NULL);
			g->defaultParBlock = g->localParBlock; /* change the DefaultParameterBlock */
		}
	}
}


void GridOutline_T2KTTClass( T2KTTClass *t, GlyphClass *glyph )
{
	if ( t != NULL  && glyph->hintFragment != NULL && glyph->hintLength > 0 ) {
		register long i,n;
		F26Dot6 *ptr32;
		register F16Dot16 multiplier;
		
		n 			= glyph->pointCount + SbPtCount;
		ptr32		= t->ptr32; /* tsi_AllocMem( t->mem, 2 * n * sizeof( F26Dot6 ) + n * sizeof( uint8 ) ); */
		
		t->elements[1].x			= glyph->x;
	  	t->elements[1].y			= glyph->y;
	    t->elements[1].ox			= ptr32; ptr32 += n;
	    t->elements[1].oy			= ptr32; ptr32 += n;
	    t->elements[1].oox			= glyph->oox;
	    t->elements[1].ooy			= glyph->ooy;
		t->elements[1].onCurve		= glyph->onCurve;
		t->elements[1].nc			= glyph->contourCount;
		t->elements[1].padWord		= 0;
		t->elements[1].sp			= glyph->sp;
		t->elements[1].ep			= glyph->ep;
		t->elements[1].f			= (uint8	*)ptr32;
		
		{
			register F26Dot6 *oz = t->elements[1].ox;
			register F26Dot6 *z  = glyph->x;
			register uint8 *f 	= t->elements[1].f;
			for ( i = 0; i < n; i++ ) {
				oz[i]	= z[i];
				f[i]	= 0;
			}
			oz = t->elements[1].oy; z  = glyph->y;
			for ( i = 0; i < n; i++ ) {
				oz[i]	= z[i];
			}
		
		}
		/* For now do this....., not necessarily the best for the long term... */
		if ( ( multiplier = t->globalGS.yStretch) != ONEFIX ) {
			register int16 *ptr16 = t->elements[1].ooy;
			for ( i = 0; i < n; i++ ) {
				ptr16[i] = (short)util_FixMul( ptr16[i], multiplier );
			}
		}
		if ( (multiplier = t->globalGS.xStretch) != ONEFIX ) {
			register int16 *ptr16 = t->elements[1].oox;
			for ( i = 0; i < n; i++ ) {
				ptr16[i] = (short)util_FixMul( ptr16[i], multiplier );
			}
		}
		/* round the Aw */
		{
			register F26Dot6 *x = glyph->x;
			long dx = x[glyph->pointCount + 1] - x[glyph->pointCount + 0];
			
			dx +=  32;
			dx &= ~63;
			
			x[glyph->pointCount + 0] += 32;
			x[glyph->pointCount + 0] &= ~63;
			x[glyph->pointCount + 1] = x[glyph->pointCount + 0] + dx;
		}
#if SbPtCount >= 4
		{
			register F26Dot6 *y = glyph->y;
			long dy = y[glyph->pointCount + 3] - y[glyph->pointCount + 2];
			
			dy +=  32;
			dy &= ~63;
			
			y[glyph->pointCount + 2] += 32;
			y[glyph->pointCount + 2] &= ~63;
			y[glyph->pointCount + 3] = y[glyph->pointCount + 2] + dy;
		}
#endif		
		
		{
			fnt_GlobalGraphicStateType *g = &(t->globalGS);

			i = PREPROGRAM; /* Ok since !=  FONTPROGRAM */
			g->pgmIndex = (uint8)i;
			g->localParBlock = g->defaultParBlock;
			g->init = false;
			fnt_Execute( t->elements, glyph->hintFragment, glyph->hintFragment + glyph->hintLength, g, NULL);
		}
		
		/* tsi_DeAllocMem( t->mem, t->elements[1].ox ); */
	}
}

/*
 * The T2KTTClass destructor
 */
void Delete_T2KTTClass( T2KTTClass *t )
{
	long i;
	if ( t != NULL ) {
		fnt_GlobalGraphicStateType *g = &t->globalGS;
		
		tsi_DeAllocMem( t->mem, g->function );
		tsi_DeAllocMem( t->mem, g->stackBase );
		tsi_DeAllocMem( t->mem, g->store );
		tsi_DeAllocMem( t->mem, g->controlValueTable );
		tsi_DeAllocMem( t->mem, g->funcDef );
		tsi_DeAllocMem( t->mem, g->instrDef );
		for ( i = 0; i < MAXPREPROGRAMS; i++ ) {
			tsi_DeAllocMem( t->mem, g->pgmList[i] );
		}

		
		tsi_DeAllocMem( t->mem, t->ptr32 );
		tsi_DeAllocMem( t->mem, t->ocvt );
		tsi_DeAllocMem( t->mem,	t->elements[0].x );	
		tsi_DeAllocMem( t->mem, t );
	}
}

#endif /* ENABLE_NATIVE_TT_HINTS */



/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: R:/src/FontFusion/Source/Core/rcs/t2ktt.c 1.7 2001/05/03 15:44:30 reggers Exp $
 *                                                                           *
 *     $Log: t2ktt.c $
 *     Revision 1.7  2001/05/03 15:44:30  reggers
 *     Cast to silence a warning.
 *     Revision 1.6  2001/05/02 17:20:34  reggers
 *     SEAT BELT mode added (Sampo)
 *     Revision 1.5  1999/12/09 21:18:16  reggers
 *     Sampo: multiple TrueType compatibility enhancements (scan convereter).
 *     Revision 1.4  1999/10/18 20:29:25  shawn
 *     Modified to correct a difference between debug and release
 *     versions.
 *     
 *     Revision 1.3  1999/09/30 15:12:28  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.2  1999/05/17 15:58:36  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/

