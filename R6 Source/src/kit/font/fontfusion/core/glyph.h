/*
 * Glyph.h
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

#ifndef __T2K_GLYPH__
#define __T2K_GLYPH__
#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#ifdef ENABLE_T2KE
/* We need the tsiColorDescriptor structure */
#include "t2ksc.h"
#include "t2kclrsc.h"
#endif

/* private constants */
#define BASE0 0
#define BASE1 2200
#define BASE2 12604
#define BASE3 14652
#define BASEMAX 16384

/* Number of sidebearing points */
#define SbPtCount	4

/*
 * Composite glyph constants
 */
#define COMPONENTCTRCOUNT 			-1		/* ctrCount == -1 for composite */
#define ARG_1_AND_2_ARE_WORDS		0x0001	/* if set args are words otherwise they are bytes */
#define ARGS_ARE_XY_VALUES			0x0002	/* if set args are xy values, otherwise they are points */
#define ROUND_XY_TO_GRID			0x0004	/* for the xy values if above is true */
#define WE_HAVE_A_SCALE				0x0008	/* Sx = Sy, otherwise scale == 1.0 */
#define NON_OVERLAPPING				0x0010	/* set to same value for all components */
#define MORE_COMPONENTS				0x0020	/* indicates at least one more glyph after this one */
#define WE_HAVE_AN_X_AND_Y_SCALE	0x0040	/* Sx, Sy */
#define WE_HAVE_A_TWO_BY_TWO		0x0080	/* t00, t01, t10, t11 */
#define WE_HAVE_INSTRUCTIONS		0x0100	/* instructions follow */
#define USE_MY_METRICS				0x0200	/* */


#define T2K_CTR_BUFFER_SIZE 8
typedef struct {
	/* private */
	tsiMemObject *mem;
	
	short contourCountMax;
	long pointCountMax;
	
	long colorPlaneCount;
	long colorPlaneCountMax;
	
	short ctrBuffer[T2K_CTR_BUFFER_SIZE + T2K_CTR_BUFFER_SIZE];
#ifdef ENABLE_T2KE
	tsiColorDescriptor *colors;
	uint16 gIndex; /* Glyph Index, just for editing purposes */
#endif
	
	/* public */
	short 	curveType;		/* 2 or 3 */
	short	contourCount;	/* number of contours in the character */
	short 	pointCount;		/* number of points in the characters + 0 for the sidebearing points */
	int16	*sp;			/* sp[contourCount] Start points */
	int16	*ep;  			/* ep[contourCount] End points */
	int16	*oox;			/* oox[pointCount] Unscaled Unhinted Points, add two extra points for lsb, and rsb */
	int16	*ooy;			/* ooy[pointCount] Unscaled Unhinted Points, set y to zero for the two extra points */
							/* Do NOT include the two extra points in sp[], ep[], contourCount */
							/* Do NOT include the two extra points in pointCount */
	uint8 *onCurve;			/* onCurve[pointCount] indicates if a point is on or off the curve, it should be true or false */

	F26Dot6 *x, *y;			/* The actual points in device coordinates. */
	
	short *componentData;
	long  componentSize;
	long  componentSizeMax;
	
	uint8 *hintFragment;
	long hintLength;
	
	short	xmin, ymin, xmax, ymax;
	
	char	dropOutControl; /* true if on, false otherwise */
	uint16  myGlyphIndex;
} GlyphClass;

#ifdef ENABLE_FF_CURVE_CONVERSION
/*
 * Converts a glyph with glyph->curveType == 3 or 2 into a glyph with
 * glyph->curveType == curveTypeOut.
 */
GlyphClass *FF_ConvertGlyphSplineTypeInternal( GlyphClass *glyph, short curveTypeOut );
#endif /* ENABLE_FF_CURVE_CONVERSION */

GlyphClass *New_EmptyGlyph( tsiMemObject *mem, int16 lsb, uint16 aw, int16 tsb, uint16 ah );

void ResequenceContoursAndPoints( GlyphClass *glyph, int closed );
long Write_GlyphClassT2K( GlyphClass *glyph, OutputStream *out, void *model );
GlyphClass *New_GlyphClassT2K( tsiMemObject *mem, InputStream *in, char readHints, int16 lsb, uint16 aw, int16 tsb, uint16 ah, void *model );

GlyphClass *New_GlyphClassT2KE( void *t, register InputStream *in, long byteCount, int16 lsb, uint16 aw, int16 tsb, uint16 ah);
long Write_GlyphClassT2KE( GlyphClass *glyph, OutputStream *out );


void TEST_T2K_GLYPH( tsiMemObject *mem );

#ifdef T1_OR_T2_IS_ENABLED
void glyph_CloseContour( GlyphClass *t );
void glyph_AddPoint( GlyphClass *t, long x, long y, unsigned char onCurveBit );
void glyph_StartLine( GlyphClass *t, long x, long y );
#endif /* T1_OR_T2_IS_ENABLED */

#ifdef ENABLE_T2KE
void glyph_CloseColorContour( GlyphClass *glyph, tsiColorDescriptor *color );
#endif

#ifdef ENABLE_PRINTF
void glyph_PrintPoints( GlyphClass *t );
#endif

#ifdef ENABLE_WRITE
#define CURVE_CONVERSION
#ifdef CURVE_CONVERSION
GlyphClass *glyph_Convert3rdDegreeGlyphTo2ndDegree( GlyphClass *glyph1 );
int glyph_RemoveRedundantPoints( GlyphClass *glyph, int pass );
#endif /* CURVE_CONVERSION */
#endif /* ENABLE_WRITE */
#ifdef ENABLE_WRITE
void WriteDeltaXYValue( OutputStream *out, int dx, int dy, char onCurve );
#endif
int ReadDeltaXYValue( InputStream *in, short *dxPtr, short *dyPtr );
#ifdef ENABLE_ORION
int ReadOrionDeltaXYValue( InputStream *in, void *model, short *dxPtr, short *dyPtr );
#endif


void Add_GlyphClass( GlyphClass **tPtr, GlyphClass *addMe, uint16 flags, long arg1, long arg2 );

void AllocGlyphPointMemory( GlyphClass *t, long pointCount );
void glyph_AllocContours( GlyphClass *t, short contourCountMax );

void Delete_GlyphClass( GlyphClass *t );

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* __T2K_GLYPH__ */

/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: R:/src/FontFusion/Source/Core/rcs/glyph.h 1.9 2000/05/11 13:37:44 reggers release $
 *                                                                           *
 *     $Log: glyph.h $
 *     Revision 1.9  2000/05/11 13:37:44  reggers
 *     Added support for outline curve conversion. (Sampo)
 *     Revision 1.8  1999/12/08 15:28:49  reggers
 *     Disable Curve Conversion at runtime
 *     Revision 1.7  1999/10/18 16:59:06  jfatal
 *     Changed all include file names to lower case.
 *     Revision 1.6  1999/09/30 15:11:22  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.5  1999/07/20 14:36:29  sampo
 *     Restored some prototypes for functions in GLYPH2.C
 *     Revision 1.4  1999/07/19 21:50:08  sampo
 *     Removed some experimental prototypes
 *     Revision 1.3  1999/07/16 17:51:58  sampo
 *     Sampo work. Drop #8 July 16, 1999
 *     Revision 1.2  1999/05/17 15:57:01  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/
