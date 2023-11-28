/*
 * TRUETYPE.H
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

#ifndef __T2K_TRUETYPE__
#define __T2K_TRUETYPE__
#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */
/*
 * T2K internal font wide metric data type. used here and in T2KSBIT.H
 */
typedef struct {
	int			isValid;			
	int16		Ascender;			
	int16		Descender;			
	int16		LineGap;			
	uint16		maxAW;	
	F16Dot16	caretDx, caretDy;
	
	int16 underlinePosition;	/* Zero, if unknown. */
	int16 underlineThickness;	/* Zero, if unknown. */
} T2K_FontWideMetrics;

#include "tt_prvt.h"
#include "t2ksbit.h"
#include "glyph.h"
#ifdef ALGORITHMIC_STYLES
#include "shapet.h"
#endif
#include "t1.h"
#ifdef ENABLE_T2KE
#include "t2kexec.h"
#endif
#ifdef ENABLE_PFR
#include "pfrread.h"
#endif
#ifdef ENABLE_SPD
#include "spdread.h"
#endif
#ifdef ENABLE_PCL
#include "pclread.h"
#endif

#ifdef ENABLE_NATIVE_TT_HINTS
#include "t2ktt.h"
#endif

#ifdef ENABLE_NATIVE_T1_HINTS 
#include "fft1hint.h"
#endif

typedef void (*StyleFuncPtr)( GlyphClass *glyph, tsiMemObject *mem, short UPEM, F16Dot16 params[] );
typedef void (*StyleMetricsFuncPtr)( hmtxClass *hmtx, tsiMemObject *mem, short UPEM, F16Dot16 params[] );
#define MAX_STYLE_PARAMS 4

/* A private T2K internal structure containing scaling in formation. */
#define T2K_IMULSHIFT	0
#define T2K_IMULDIV		1
#define T2K_FIXMUL		2
typedef struct {
	int16 nScale;
	int16 dShift;
	int32 dScale;
	int32 dScaleDiv2;
	F16Dot16 fixedScale;
	int16 scaleType;
} T2KScaleInfo;

void setT2KScaleFactors( long pixelsPerEm, long UPEM, T2KScaleInfo *si );



/* Only for use use when we do not want to use an hmtx class */
typedef uint16 (*FF_GetAWFuncPtr)( void *param1, uint16 gIndex);
/* Set to either Get_hmtx_AW or Get_Cached_AW or Get_Upem_Width */
#define FF_Set_GetAWFuncPtr_Reference1( sfntClassPtr, funcptr1, param1 ) (sfntClassPtr->GetAWFuncPtr1 = (funcptr1), sfntClassPtr->GetAWParam1 = (void *)(param1) )
/* If above set to Get_Cached_AW then set this to font format reader specific FF_GetAWFuncPtr */
#define FF_Set_GetAWFuncPtr_Reference2( sfntClassPtr, funcptr2, param2 ) (sfntClassPtr->GetAWFuncPtr2 = (funcptr2), sfntClassPtr->GetAWParam2 = (void *)(param2) )
#define FF_AW_CACHE_SIZE 251

#define FF_SET_NATIVE_HINTS( font, bol_value ) ((font)->useNativeHints  = (bol_value))
#define FF_SET_FONT_SCALE( font, xScaleIn, yScaleIn ) ( (font)->xScale = (xScaleIn),  (font)->yScale = (yScaleIn) )
#define FF_SET_STROKING( font, bol_value ) ((font)->strokeGlyph  = (bol_value))


typedef struct {
	/* private */
	sfnt_OffsetTable *offsetTable0;

	FF_GetAWFuncPtr GetAWFuncPtr1;
	void *GetAWParam1;
	FF_GetAWFuncPtr GetAWFuncPtr2;
	void *GetAWParam2;
	uint16 *awCache_hashKey;
	uint16 *awCache_aw; /* Points at  &awCache_hashKey[FF_AW_CACHE_SIZE] */
	uint16 upem;
	
#ifdef ENABLE_T1
	T1Class *T1;
#endif
#ifdef ENABLE_CFF
	CFFClass *T2;
#endif
#ifdef ENABLE_T2KE
	T2KEClass *T2KE;
#endif
#ifdef ENABLE_PFR
	PFRClass *PFR;
#endif
#ifdef ENABLE_SPD
	SPDClass *SPD;
#endif
#ifdef ENABLE_PCL
	PCLClass *PCLeo;
#endif
#ifdef ENABLE_SBIT
	blocClass *bloc;
	ebscClass *ebsc;
	uint32 bdatOffset;
#endif

#ifdef ENABLE_NATIVE_TT_HINTS
	T2KTTClass *t2kTT;
#endif

#ifdef ENABLE_NATIVE_T1_HINTS
	FFT1HintClass *ffhint;
#endif


	ttcfClass *ttcf;
	
	headClass *head;
	maxpClass *maxp;
	locaClass *loca;
#ifdef ENABLE_T2KS
	slocClass *sloc;
	ffstClass *ffst;
	ffhmClass *ffhm;
#endif
	gaspClass *gasp;
	hheaClass *hhea;
	hheaClass *vhea;
	hmtxClass *hmtx;
	hmtxClass *vmtx;
	cmapClass *cmap;
	kernClass *kern;
	uint16 preferedPlatformID, preferedPlatformSpecificID;

	int16 post_underlinePosition;	/* Zero, if unknown. */
	int16 post_underlineThickness;	/* Zero, if unknown. */
	
	uint32	isFixedPitch;			/* 0 = proportional, non-0 = monospace */
	uint16	firstCharCode;			/* lowest code point, character code in font */
	uint16	lastCharCode;			/* highest code point, character code in font */
	
	long	xPPEm, yPPEm;

	void *globalHintsCache;
	StyleFuncPtr StyleFunc;
	StyleMetricsFuncPtr StyleMetricsFunc;
	F16Dot16 params[MAX_STYLE_PARAMS];
        
	/* For Stroke Font Hints */
	F16Dot16 xScale, yScale;
	int useNativeHints;
	int strokeGlyph; /* ENABLE_STRKCONV */
	int greyScaleLevel;
#ifdef ENABLE_T2KS
	F16Dot16 currentCoordinate[2];
#endif
	
	
	InputStream *in;		/* Primary InputStream */
	InputStream *in2;		/* Secondary InputStream, normally == NULL, except for Type 1 fonts with kerning */
	OutputStream *out;
	tsiMemObject *mem;
	
	/* OrionModelClass *model; */
	void *model;
	/* public */
	long numGlyphs;
	long numberOfLogicalFonts;	/* Number of logical fonts inside, normally == 1. */
} sfntClass;

hmtxClass *New_hmtxEmptyClass( tsiMemObject *mem, int32 numGlyphs, int32 numberOfHMetrics );
void Delete_hmtxClass( hmtxClass *t );

/* Some useful getter methods */
uint16 GetUPEM( sfntClass *t);
int GetMaxPoints( sfntClass *t);

void GetFontWideOutlineMetrics( sfntClass *font, T2K_FontWideMetrics *hori, T2K_FontWideMetrics *vert );


typedef struct {
	StyleFuncPtr StyleFunc;
	StyleMetricsFuncPtr StyleMetricsFunc;
	F16Dot16 params[MAX_STYLE_PARAMS];
} T2K_AlgStyleDescriptor;

/* Caller does something like in = New_InputStream3( t->mem, data, length ); */
#define FONT_TYPE_1 1
#define FONT_TYPE_2 22
#define FONT_TYPE_TT_OR_T2K 2
#define FONT_TYPE_PFR	3
#define FONT_TYPE_PCL	4
#define FONT_TYPE_PCLETTO	5
#define FONT_TYPE_SPD	6

/* Next two for backwards compatibility. */
#define New_sfntClassLogical( mem, fontType, fontNum, in, styling, errCode ) FF_New_sfntClass( mem, fontType, fontNum, in, NULL, styling, errCode )

#ifdef ENABLE_PCLETTO
typedef struct
{
	uint8	lsbSet;
	uint16	lsb;
	uint8	awSet;
	uint16	aw;
	uint8	tsbSet;
	uint16	tsb;
}HPXL_MetricsInfo_t;

/* callback for getting outline char string pointer from application environment: */
int tt_get_char_data(	long cCode,
						uint8 cmd,
						uint8 **pCharData,
						uint16 *dataSize,
						uint16 *gIndex,
						HPXL_MetricsInfo_t	*metricsInfo
					);
#endif

#ifdef ENABLE_PCL
/* callback for getting outline char string pointer from application environment: */
int eo_get_char_data(	long cCode,
						uint8 cmd,
						uint8 **pCharData,
						uint16 *dataSize,
						uint16 *charCode,
						uint16 *gIndex
					);
#endif

#define New_sfntClass( mem, fontType, in, styling, errCode ) New_sfntClassLogical( mem, fontType, 0, in, styling, errCode )

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
sfntClass *FF_New_sfntClass( tsiMemObject *mem, short fontType, long fontNum, InputStream *in1, InputStream *in2, T2K_AlgStyleDescriptor *styling, int *errCode );
#define CMD_GRID 2
#define CMD_TT_TO_T2K 3
#define CMD_T2K_TO_TT 4
#define CMD_HINT_ROMAN 5
#define CMD_HINT_OTHER 6
#define CMD_TT_TO_T2KE 7

sfntClass *New_sfntClass2( sfntClass *sfnt0, int cmd, int param );

void WriteToFile_sfntClass( sfntClass *t, const char *fname );
void Purge_cmapMemory( sfntClass *t );
void ff_LoadCMAP( sfntClass *t );

#define tag_T2KG        		0x54324B47        /* 'T2KG' */
#define tag_T2KC        		0x54324B43        /* 'T2KC' */
sfnt_DirectoryEntry *GetTableDirEntry_sfntClass( sfntClass *t, long tag );
/* caller need to do Delete_InputStream on the stream */
InputStream *GetStreamForTable( sfntClass *t, long tag  );

#define Delete_sfntClass( t, errCode ) FF_Delete_sfntClass( t, errCode )
void FF_Delete_sfntClass( sfntClass *t, int *errCode );

/*
 * Maps a PostScript Name to character code
 */
uint16 SfntClassPSNameTocharCode( sfntClass *t, char *PSName );

/*
 * Maps the character code to glyphIndex
 */
uint16 GetSfntClassGlyphIndex( sfntClass *t, uint16 charCode );

/*
 * Returns a glyph given the glyphIndex
 */
GlyphClass *GetGlyphByIndex( sfntClass *t, long index, char readHints, uint16 *aWidth, uint16 *aHeight );
/*
 * Returns a glyph given the character code.
 */
#define GetGlyphByCharCode( t, charCode, readHints, aw, ah ) GetGlyphByIndex( t, GetSfntClassGlyphIndex(t,charCode), readHints, aw, ah )

int IsFigure( sfntClass *t, uint16 gIndex );

#ifdef ENABLE_KERNING
void GetSfntClassKernValue( sfntClass *t, uint16 leftGIndex, uint16 rightGIndex, int16 *xKern, int16 *yKern );
#endif /* ENABLE_KERNING */



long GetNumGlyphs_sfntClass( sfntClass *t );

void GetTTNameProperty( sfntClass *font, uint16 languageID, uint16 nameID, uint8 **p8, uint16 **p16 );

void ff_KernShellSort(kernPair0Struct *pairs, int num_pair);

/*
 * Internal list characters function
*/
void T2K_SfntListChars(void *userArg, sfntClass *t, void *ctxPtr, int ListCharsFn(void *userArg, void *p, uint16 code), int *errCode);

#ifdef ENABLE_GASP_TABLE_SUPPORT
/*
 * IN: t, ppem
 * OUT: useGridFitting, useGrayScaleRendering
 * returns true if information found
 */
int Read_gasp( gaspClass *t, int ppem, int *useGridFitting, int *useGrayScaleRendering );
#endif /* ENABLE_GASP_TABLE_SUPPORT */


#ifdef ENABLE_NATIVE_TT_HINTS
#include "t2ktt.h"
#endif

/***** ***** ***** ***** ***** ***** *****/
locaClass *New_locaClass( tsiMemObject *mem, InputStream *in, short indexToLocFormat, long length );
#ifdef ENABLE_WRITE
void CreateTableIfMissing( sfnt_OffsetTable *t, long tag );
sfnt_OffsetTable *New_sfnt_EmptyOffsetTable( tsiMemObject *mem, short numberOfOffsets );
void Write_sfnt_OffsetTable( sfnt_OffsetTable *t, OutputStream *out );
void Write_locaClass( locaClass *t, OutputStream *out );
void SortTableDirectory( sfnt_OffsetTable *offsetTable );
void CalculateNewCheckSums( sfntClass *sfnt );
void Write_hmtxClass( hmtxClass *t, OutputStream *out);
void Write_maxpClass( maxpClass *t, OutputStream *out );
void Write_headClass( headClass *t, OutputStream *out);
void Write_hheaClass( hheaClass *t, OutputStream *out);
long CalculateCheckSumAdjustment( sfntClass *sfnt );
uint8 *GetTTFPointer( sfntClass *sfnt );
uint32 GetTTFSize( sfntClass *sfnt );



void TEST_T2K_CMAP( InputStream *in, uint16 *charCodeToGIndex, long N, uint16 platformID, uint16 encodingID);
#endif /* ENABLE_WRITE */
/***** ***** ***** ***** ***** ***** *****/

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* __T2K_TRUETYPE__ */
/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: R:/src/FontFusion/Source/Core/rcs/truetype.h 1.27 2001/05/04 21:44:39 reggers Exp $
 *                                                                           *
 *     $Log: truetype.h $
 *     Revision 1.27  2001/05/04 21:44:39  reggers
 *     Warning cleanup
 *     Revision 1.26  2001/05/03 20:46:51  reggers
 *     LoadCMAP mapped to ff_LoadCMAP
 *     Revision 1.25  2001/04/27 20:33:38  reggers
 *     Added new API function T2K_ForceCMAPChange()
 *     Revision 1.24  2001/04/24 21:57:17  reggers
 *     Added GASP table support (Sampo).
 *     Revision 1.23  2001/04/19 17:38:55  reggers
 *     Sampo mod to support improved stroke font hinting.
 *     Revision 1.22  2000/05/17 20:43:39  mdewsnap
 *     Fixed Hint Include file name.
 *     Revision 1.21  2000/04/27 21:35:47  reggers
 *     New Stroke convert painting
 *     Revision 1.20  2000/04/19 19:01:17  mdewsnap
 *     Added in code to deal with T1 hints
 *     Revision 1.19  2000/04/14 17:01:01  reggers
 *     First cut applying selective hints to stroke font glyphs.
 *     Revision 1.18  2000/03/10 19:18:32  reggers
 *     Enhanced for enumeration of character codes in font.
 *     Revision 1.17  2000/02/18 18:56:19  reggers
 *     Added Speedo processor capability.
 *     Revision 1.16  2000/01/20 15:48:26  reggers
 *     Changed MKS comment to correct to ENABLE_PCLETTO
 *     Revision 1.15  2000/01/20 15:47:20  reggers
 *     HPXLMetricsInfo_t now defined only on ENABLE_PCLETTO.
 *     Revision 1.14  2000/01/19 19:21:28  reggers
 *     Changed all references to PCLClass member PCL to PCLeo to
 *     avoid nasty namespace conflict on Windows builds.
 *     Revision 1.13  2000/01/18 20:53:59  reggers
 *     Changes to abstract the character directory and character string
 *     storage to the application environment for ENABLE_PCL.
 *     Revision 1.11  2000/01/07 19:46:04  reggers
 *     Sampo enhancements for FFS fonts.
 *     Revision 1.10  1999/12/23 22:03:19  reggers
 *     New ENABLE_PCL branches. Rename any 'code' and 'data' symbols.
 *     Revision 1.9  1999/11/04 20:20:38  reggers
 *     Added code for getting fixed/proportional setting, firstCharCode and
 *     lastCharCode.
 *     Revision 1.8  1999/10/18 17:02:46  jfatal
 *     Changed all include file names to lower case.
 *     Revision 1.7  1999/09/30 15:12:31  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.6  1999/08/27 20:08:54  reggers
 *     Latest changes from Sampo
 *     Revision 1.5  1999/07/29 16:10:53  sampo
 *     First revision for T2KS
 *     Revision 1.4  1999/07/19 16:59:40  sampo
 *     Prototype of kern shell sort routine moved here from util.h
 *     Revision 1.3  1999/07/16 17:52:12  sampo
 *     Sampo work. Drop #8 July 16, 1999
 *     Revision 1.2  1999/05/17 15:58:45  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/
