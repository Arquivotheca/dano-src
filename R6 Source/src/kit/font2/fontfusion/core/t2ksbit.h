/*
 * T2ksbit.h
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

#ifndef __T2K_SBIT__
#define __T2K_SBIT__
#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */


#ifdef ENABLE_SBIT
/*
 * The fields are 2bytes wide, instead of 1 byte
 * since this allows us to store large rescaled values here. 
 * Otherwise bitmap scaling might overflow these fields.
 * In the font data these fields are stored in single bytes,
 * This is just our internal represenation of that, which
 * can on demand be rescaled to larger sizes.
 */
typedef struct {
	uint16	height;
	uint16	width;
	int16	horiBearingX;
	int16	horiBearingY;
	uint16	horiAdvance;
	int16	vertBearingX;
	int16	vertBearingY;
	uint16	vertAdvance;
} bigGlyphMetrics;


typedef struct {
	uint16 indexFormat;		/* Format of this inexSubTable. */
	uint16 imageFormat;		/* Format of EBDT/bdat image data. */
	uint32 imageDataOffset;	/* Offset to image data in the EBDT/bdat table. */
} indexSubHeader;

typedef struct {
	uint16 firstGlyphIndex;						/* First glyph index in this range. */
	uint16 lastGlyphIndex;						/* Last glyph index in this range (inclusive). */
	uint32 additionalOffsetToIndexSubTable;		/* Add to indexSubTableArryOffset to get offset from beginning of EBLC/bloc table. */ 
} indexSubTableArray;

/* for the hori and vert arrays in bitmapSizeTable */
#define NUM_SBIT_METRICS_BYTES 12
/* for the flags field in bitmapSizeTable */
#define SBIT_SMALL_METRIC_DIRECTION_IS_HORIZONTAL	0x01
#define SBIT_SMALL_METRIC_DIRECTION_IS_VERTICAL		0x02

/* One bitmapSizeTable for each strike */
typedef struct {
	/* private */
	tsiMemObject *mem;
	/* public */
	uint32 indexSubTableArrayOffset;		/* Offset to index subtable from beginning of EBLC/bloc table */
	uint32 indexTableSize;					/* Total size of IndexSubTables and array */
	uint32 numberOfIndexSubTables;			/* Number of IndexSubTables */
	uint32 colorRef;						/* Not used, should be 0 */
	
	uint8 hori[NUM_SBIT_METRICS_BYTES];		/* Line metrics for horizontally rendered text */
	uint8 vert[NUM_SBIT_METRICS_BYTES];		/* Line metrics for vertically  rendered text */
	
	uint16	startGlyphIndex;				/* First glyph Index for this strike. */
	uint16	endGlyphIndex;					/* Last glyph Index for this strike. */
	uint8	ppemX;							/* Horizontal pixels per Em */
	uint8	ppemY;							/* Vertical pixels per Em */
	uint8	bitDepth;						/* 1 for monochrome */
	uint8	flags;							/* Horizontal or Vertical small metrics */
	
	indexSubTableArray *table;  			/* indexSubTableArray table[ numberOfIndexSubTables ] */
} bitmapSizeTable;

typedef struct {
	/* For caching results of GlyphExists() */
	uint32 offsetA, offsetB;					/* Offset to the first byte, and the offset to one pst the last byte. */
	uint16 glyphIndex;							/* The glyph index. */
	uint16 ppemX, ppemY;						/* Requested size. */
	uint16 substitutePpemX, substitutePpemY;	/* Use bitmap of this size instead with scaling, if different from ppemX and ppemY.  */
	uint8	bitDepth;							/* Bit depth, 1 for monochrome data. */
	uint8	flags;								/* Flags that with bitflags for hor. & ver. directional info. */
	uint16	imageFormat;						/* Bitmap image format. */
	bigGlyphMetrics bigM;						/* Combined metrics for both small and larlge metrics types. */
	int		smallMetricsUsed;					/* If true then it implies that we either have only horizontal or vertical metrics. */
	
	/* bitmap data */
	int32 rowBytes;								/* Number of bytes per row. */
	uint8 *baseAddr;							/* The address of the bitmap. */
	/* cache book-keeping data */
	uint32 N;										/* size of baseAddr */
} sbitGlypInfoData;

/* Microsoft calls this table EBLC, Apple calls this table bloc */
typedef struct {
	/* private */
	tsiMemObject *mem;
	uint32	startOffset;
	int fontIsSbitOnly;
	
	/* glyph Info*/
	sbitGlypInfoData gInfo;
	
	/* public */
	F16Dot16	version; /* Initially set to 0x20000 */
	uint32		nTables; /* number of strikes */
	
	bitmapSizeTable **table; /* bitmapSizeTable *table[ nTables ] */
} blocClass;


typedef struct {
	uint8 hori[NUM_SBIT_METRICS_BYTES];		/* Line metrics for horizontally rendered text */
	uint8 vert[NUM_SBIT_METRICS_BYTES];		/* Line metrics for vertically  rendered text */
	uint8 ppemX;							/* Target horizantal pixels per Em. */
	uint8 ppemY;							/* Target vertical pixels per Em. */
	uint8 substitutePpemX;					/* Use a bitmap of this size before scaling in the x direction. */
	uint8 substitutePpemY;					/* Use a bitmap of this size before scaling in the y direction. */
} bitmapScaleEntry;

/* Embedded bitmap scaling table */
typedef struct {
	/* private */
	tsiMemObject *mem;
	uint32	startOffset;
	
	/* public */
	F16Dot16	version; 	/* Initially set to 0x20000 */
	uint32		numSizes;	/* number of sizes */
	
	bitmapScaleEntry *table; /* bitmapScaleEntry table[ nTables ] */
} ebscClass;

/*
 * The ebscClass class contructor (EBSC table)
 */
ebscClass *New_ebscClass( tsiMemObject *mem, InputStream *in );

/*
 * The ebscClass class destructor (EBSC table)
 */
void Delete_ebscClass( ebscClass *t );


/*
 * The blocClass class contructor (EBLC/bloc table)
 */
blocClass *New_blocClass( tsiMemObject *mem, int fontIsSbitOnly, InputStream *in );
/*
 * The blocClass class destructor (EBLC/bloc table)
 */
void Delete_blocClass( blocClass *t );


/*
 * Returns scaled font wide sbit metrics
 */
void GetFontWideSbitMetrics( blocClass *bloc, ebscClass *ebsc, uint16 ppemX, uint16 ppemY,
							T2K_FontWideMetrics *hori, T2K_FontWideMetrics *vert );

/*
 * Returns true if the glyph exists, and false otherwise
 * Caches some internal results so that we can get to the bits faster when we need them next.
 */
int FindGlyph_blocClass( blocClass *t, ebscClass *ebsc, InputStream *in, uint16 glyphIndex, uint16 ppemX, uint16 ppemY, sbitGlypInfoData *result );

/* This is here so that bad data can not cause an infinite recursion crash */
#define MAX_SBIT_RECURSION_DEPTH 16
/*
 * Gets the bits.
 */
void ExtractBitMap_blocClass( blocClass *t, ebscClass *ebsc, sbitGlypInfoData *gInfo, InputStream *in, uint32 bdatOffset, uint8 greyScaleLevel, int recursionLevel );

#endif /* ENABLE_SBIT */

#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif /* __T2K_SBIT__ */
/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: R:/src/FontFusion/Source/Core/rcs/t2ksbit.h 1.5 2001/04/19 17:38:10 reggers Exp $
 *                                                                           *
 *     $Log: t2ksbit.h $
 *     Revision 1.5  2001/04/19 17:38:10  reggers
 *     Minor data type change.
 *     Revision 1.4  2000/10/26 17:00:43  reggers
 *     Changes for SBIT: use cache based on setting of
 *     scaler->okForBitCreationToTalkToCache. Able to handle caching
 *     SBITs now.
 *     Revision 1.3  1999/09/30 15:12:10  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.2  1999/05/17 15:58:21  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/
