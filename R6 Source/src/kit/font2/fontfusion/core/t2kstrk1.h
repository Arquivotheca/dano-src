/*
 * T2Kstrk1.h
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

#ifndef __T2K_STRK1__
#define __T2K_STRK1__
#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

/*
 * glyph1 is the input GlyphClass;
 * radius   possible values:   2,3,4,...
 * joinType possible values:   MITER_JOIN, ROUND_JOIN, BEVEL_JOIN 
 * capType  possible values:   BUTT_CAP, ROUND_CAP, SQUARE_CAP, SQUARE_50PERCENT_CAP 
 * open     possible values:   true, or false
 */
GlyphClass *ff_glyph_StrokeGlyph( GlyphClass *glyph1, int radius, int joinType, int capType, int open );

/*
 * Creates a GlyphClass object for the glyph corresponding to index.
 */
GlyphClass *ff_New_GlyphClassT2KS( sfntClass *font, GlyphClass *glyph, long index, uint16 *aWidth, uint16 *aHeight, void *model, int depth );

void ff_Read2Numbers( InputStream *in, uint16 arr[] );
void ff_Read4Numbers( InputStream *in, uint16 arr[] );

#ifdef ENABLE_WRITE
	/* Not part of outside releases, so we do not need the ff_ pre-fix */
	int  Write2Numbers( uint16 arrIn[], OutputStream *out );
	int  Write4Numbers( uint16 arrIn[], OutputStream *out );
	void WriteLowUnsignedNumber( OutputStream *out, unsigned long n );
#endif

#ifdef ENABLE_T2KS
ffstClass *FF_New_ffstClass( tsiMemObject *mem, InputStream *in, unsigned long length );
void FF_Delete_ffstClass( ffstClass *t );
#endif

#ifdef ENABLE_WRITE
/*
 *
 */
void FF_Write_ffstClass( OutputStream *out, int gIndexFirstRoman, int gIndexLastRoman );
#endif /* ENABLE_WRITE*/


/*
 * The ffhmClass constructor
 */
ffhmClass *FF_New_ffhmClass( tsiMemObject *mem, InputStream *in );

/*
 * The ffhmClass Get-Advance-Width method
 */
uint16 FF_GetAW_ffhmClass( void *param1, register uint16 index );

/*
 * The ffhmClass destructor
 */
void FF_Delete_ffhmClass( ffhmClass *t );

#ifdef ENABLE_WRITE
/*
 * Writes out the ffhm image into the out stream
 */
void FF_Write_ffhmClass( OutputStream *out, int32 numGlyphs, uint16 *aw, uint16 defaultWidth );
#endif /* ENABLE_WRITE*/


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* __T2K_STRK1__ */

/*********************** R E V I S I O N   H I S T O R Y **********************
 *                                                                            *
 *     $Header: R:/src/FontFusion/Source/Core/rcs/t2kstrk1.h 1.6 2000/01/07 19:45:57 reggers release $
 *                                                                            *
 *     $Log: t2kstrk1.h $
 *     Revision 1.6  2000/01/07 19:45:57  reggers
 *     Sampo enhancements for FFS fonts.
 *     Revision 1.5  1999/09/30 15:10:49  jfatal
 *     Added correct Copyright notice.
 *                                                                            *
 ******************************************************************************/

