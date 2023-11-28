/*
 * Pfrread.h
 * Font Fusion Copyright (c) 1989-1999 all rights reserved by Bitstream Inc.
 * http://www.bitstream.com/
 * http://www.typesolutions.com/
 * Author: Sampo Kaasila, Robert Eggers, Mike Dewsnap
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


/*                                                                          
 * Header file for the portable font resource examiner                       
 * Migrated from PEEK_PFR.H                                                  
 */                                                                          
 
#ifndef _PFRReader_H
#define _PFRReader_H
#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#ifndef BITOUT
#define BITOUT 0
#endif

#include "fft1hint.h"
/***** Portable Font Resource header definitions *****/
#define PFRHEDFMT  0     /* Format field "PFR0vv<CR><LF>" */
#define PFRHEDSIZ  8     /* Header size field */
#define PFRLFDSIZ 10     /* Logical font directory size field */
#define PFRLFDOFF 12     /* Logical font directory offset field */
#define PFRLFTSZM 14     /* Max logical font size */
#define PFRLFTSZT 16     /* Total size of all logical fonts */
#define PFRLFTFOF 19     /* Offset to first logical font */
#define PFRPFTSZM 22     /* Max physical font size */
#define PFRPFTSZT 24     /* Total size of all physical fonts */
#define PFRPFTFOF 27     /* Offset to first physical font */
#define PFRGPSSZM 30     /* Max glyph program string size */
#define PFRGPSSZT 32     /* Total size of all glyph program strings */
#define PFRGPSFOF 35     /* Offset to first glyph program string */
#define PFRBVLMAX 38     /* Max number of blue values */
#define PFRXCCMAX 39     /* Max number of controlled X coordinates */
#define PFRYCCMAX 40     /* Max number of controlled Y coordinates */

/* Version 0 header extensions */
#define PFRRSRVD0 41     /* First reserved byte */
#define PFRRSRVD1 42     /* Second reserved byte */

/* Version 1 header extensions */
#define PFRPFTSZX 41     /* Extra byte for max physical font size */
#define PFRRSRVD1 42     /* Reserved byte */

/* Version 2 header extensions */
#define PFRPFTSZX 41     /* Extra byte for max physical font size */
#define PFRRSRVD1 42     /* Reserved byte */
#define PFRBCTSZM 43     /* Max size of any bitmap char table (3 bytes)*/
#define PFRBCSSZM 46     /* Max size of any set of bitmap char tables (3 bytes) */
#define PFRPFXSZM 49     /* Max size of any extended physical font (3 bytes)*/

/* Version 3 header extension for physical font count */
#define PFRPFTCNT 52     /* Number of physical font records */

/* Version 3 header extension for stem snap table info */
#define PFRSSVMAX 54     /* Max number of stemSnapV values */
#define PFRSSHMAX 55     /* Max number of stemSnapH values */

/* Version 4 header extension for max number of chars in any font */
#define PFRCHRMAX 56
           
#define PFR_HEADER_SIZE 43 /* Basic header size */


/***** Portable Font Resource font directory definitions *****/

/***** Portable Font Resource logical font record definitions *****/
#define PFRLFTTMX  0     /* Transformation matrix */
#define PFRLFTSFX 12     /* Special effects format byte */
#define LFTMITER    0x00
#define LFTROUND    0x01
#define LFTBEVEL    0x02
#define LFTLINEJOIN 0x03
#define LFTSTROKE   0x04
#define LFTBIGSTR   0x08


/***** FLAG BIT CONSTANTS *****/
#define BIT_0 0x01
#define BIT_1 0x02
#define BIT_2 0x04
#define BIT_3 0x08
#define BIT_4 0x10
#define BIT_5 0x20
#define BIT_6 0x40
#define BIT_7 0x80

#define MAX_ORUS 64    

/* Bitmap flags bit assignments */
#define PFR_BLACK_PIXEL BIT_0    /* Bit representation of black pixel */
#define PFR_INVERT_BMAP BIT_1    /* Set if bitmaps in decreasing Y order */


#define     PFR_MAX_BLUEVALUES 14
#define		PFR_MAX_SNAPS	12


/*  MSB_FIRST defines packing order of pixels within each byte in the cache
	0 = pack pixels starting at the least significant end of each byte
	1 = pack pixels starting at the most significant end of each byte   */
#ifndef MSB_FIRST
#define MSB_FIRST 1
#endif

/* Compound character element record */
typedef struct charElement_tag
    {
    int32 xScale;
    int32 xPosition;
    int32 yScale;
    int32 yPosition;
    uint32 glyphProgStringOffset;
    uint16 glyphProgStringSize;
    uint8 matchSpec;
    } charElement_t;

/* Glyph program string execution context */
typedef struct gpsExecContext_tag
    {
    int16 nXorus;
    int16 xOruTable[MAX_ORUS];
    int16 nYorus;
    int16 yOruTable[MAX_ORUS];
    int16 xPrevValue;
    int16 yPrevValue;
    int16 xPrev2Value;
    int16 yPrev2Value;
    } gpsExecContext_t;

/* Extra item action table */
typedef void (*extraItemActions_t[])(uint8 *pByte);

/***** Function Prototypes *****/
void InitGpsExecContext(
    gpsExecContext_t *pGpsExecContext);


void ReadGpsArgs(
    uint8 **ppBuff,
    uint8 format,
    gpsExecContext_t *pGpsExecContext,
    int16 *pX,
    int16 *pY);

int16 ReadByte(
    uint8 *pBuff);

int16 ReadWord(
    uint8 *pBuff);

int32 ReadLongSigned(
    uint8 *pBuff);

int32 ReadLongUnsigned(
    uint8 *pBuff);

void ShowExtraItems(
    uint8 **ppByte,
    extraItemActions_t actionTable);

void ShowPfrStats(void);

typedef struct {
    uint16  charCode;
    uint16  gpsSize;
    long    gpsOffset;
} physCharMap;


typedef struct {
	F16Dot16 m00;
	F16Dot16 m01;
	F16Dot16 m10;
	F16Dot16 m11;
	F16Dot16 xOffset;
	F16Dot16 yOffset;
} tcb_t;


typedef struct bmapCharDir_tag
{
	uint16	charIndex;	/* fill in later */
	uint16	charCode;
	uint16	gpsSize;
	uint32	gpsOffset;
} bmapCharDir_t;	/* bmapCharDirty! */

typedef struct bmapStrike_tag
{
	uint16		xppm;
	uint16		yppm;
	int32		nBmapChars;
	bmapCharDir_t	*bmapDir;
} bmapStrike_t;

typedef struct pfrGlyphInfoData_tag
{
	uint16	glyphIndex;
	uint8 	greyScaleLevel;
	uint16	ppemX;
	uint16	ppemY;
	long 	width, height;
	F16Dot16	horiBearingX;
	F16Dot16	horiBearingY;
	F16Dot16	horiAdvance;
	F16Dot16	vertBearingX;
	F16Dot16	vertBearingY;
	F16Dot16	vertAdvance;
	long bytesPerRow;
	uint8	invertBitmap;
	uint8	*gpsPtr;
	uint16	gpsSize;
	uint32	gpsOffset;
	unsigned char *baseAddr; /* unsigned char baseAddr[N], 	N = t->rowBytes * t->height */
	unsigned char *tempBuffer;
	unsigned char *dst;	/* the place where SetBitmapBits will write */
}pfrGlyphInfoData;

typedef struct {
	/* private */
	tsiMemObject *mem;
	InputStream *in;
	uint8 pfrType;
	physCharMap *charMap;
	uint32 directoryCount;
	uint16  fontNumber;
	uint16	physFontNumber;
	uint16	outlineRes;
	uint16	metricsRes;
	int8 verticalEscapement;
	uint8	*pAuxData;
	long	nAuxBytes;
	F16Dot16	xyScale;
	long	firstGpsOffset;
	long	totalGpsSize;
	uint8	contourOpen;
	unsigned char shortScaleFactors;
	/* bmap stuff: */
	uint8   bmapFlags;
	uint16	nBmapStrikes;
	bmapStrike_t	*bmapStrikes;
	pfrGlyphInfoData	gInfo;
	long lsbx;
	long lsby;
	long awx;
	long awy;
	/* font-wide bounding box: */
	int16   xmin, ymin, xmax, ymax;
	int16   strokeThickness;
	uint16 headerSize, version;
	
	int16   boldThickness;
/*	void *scaler;	*/			/* pointer to the T2K scaler */
	void *sfntClassPtr;	/* pointer to parent sfntClass */
	uint8	pluggedIn;			/* unsigned char, whether style functions plugged in */
	uint8	rendering;			/* unsigned char, whether we are setting up or rendering */

/*	F16Dot16 m00, m01, m10, m11; */
	tcb_t	fontMatrix;
	tcb_t	outputMatrix;
	tcb_t	tcb;
	
	uint8	*fontID;		/* low-level PFR name tag/ID */

	/* public */
	GlyphClass *glyph;
	hmtxClass *hmtx, *noDelete_hmtx;

	
	short NumCharStrings;
	long upem;
	long maxPointCount;
	long ascent;
	long descent;
	long lineGap;
	long advanceWidthMax;
	F16Dot16 italicAngle;
	kernClass *kern;

/*	int16  *BlueValues; */
	long		BlueValues[PFR_MAX_BLUEVALUES];  /* 0 .. (numBlueValues-1) */
	long		numBlueValues;
	long		BlueFuzz;
	F16Dot16	BlueScale;
	long		BlueShift;
	long		StdVW;
	long		StdHW;
	long		nStemSnapV;
	long		nStemSnapH;
/*	int16   *StemSnapH; */
/*	int16	*StemSnapV; */

	long	   StemSnapH[PFR_MAX_SNAPS];
	long	   StemSnapV[PFR_MAX_SNAPS];


	uint32	isFixedPitch;			/* 0 = proportional, non 0 = monospace */
	uint16	firstCharCode;			/* lowest code point, character code in font */
	uint16	lastCharCode;			/* highest code point, character code in font */

	int glyphExists;				/* keep track if default glyph or found */
	
} PFRClass;

PFRClass *tsi_NewPFRClass( tsiMemObject *mem, InputStream *in, int32 fontNum );
void tsi_DeletePFRClass( PFRClass *t );
GlyphClass *tsi_PFRGetGlyphByIndex( PFRClass *t, uint16 index, uint16 *aWidth, uint16 *aHeight, FFT1HintClass *ffhint);
uint16 tsi_PFRGetGlyphIndex( PFRClass *t, uint16 charCode );
uint8 *GetPFRNameProperty( PFRClass *t, uint16 languageID, uint16 nameID );
#ifdef ENABLE_SBIT
int PFR_GlyphSbitsExists( void *p, uint16 glyphIndex, uint8 greyScaleLevel, int *errCode  );
int PFR_GetSbits(void *p, long code, uint8 greyScaleLevel, uint16 cmd);
#endif

void tsi_PFRListChars(void *userArg, PFRClass *t, void *ctxPtr, int ListCharsFn(void *userArg, void *p, uint16 code));

#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif /* _PFRReader_H */
/* eof PFRREAD.H */
/********************* R E V I S I O N   H I S T O R Y **********************
 *                                                                           *
 *  1) 14 Dec 98  R. Eggers  Created                                         *
 *     $Header: R:/src/FontFusion/Source/Core/rcs/pfrread.h 1.21 2001/05/01 18:30:56 reggers Exp $
 *                                                                           *
 *     $Log: pfrread.h $
 *     Revision 1.21  2001/05/01 18:30:56  reggers
 *     Added support for GlyphExists()
 *     Revision 1.20  2000/06/07 15:19:48  mdewsnap
 *     Changes made for dynamic storage
 *     Revision 1.19  2000/05/30 20:43:33  reggers
 *     gcc warning removal
 *     Revision 1.18  2000/05/19 14:39:50  mdewsnap
 *     Converted variables to long from short.
 *     Revision 1.17  2000/05/17 14:06:36  reggers
 *     Corrected C++ style comments
 *     Revision 1.16  2000/05/16 18:44:57  mdewsnap
 *     Added hint related fields.
 *     Revision 1.15  2000/03/10 19:18:10  reggers
 *     Enhanced for enumeration of character codes in font.
 *     Revision 1.14  2000/02/25 17:45:50  reggers
 *     STRICT warning cleanup.
 *     Revision 1.13  2000/02/18 18:56:08  reggers
 *     Added Speedo processor capability.
 *     Revision 1.12  2000/01/07 19:45:05  reggers
 *     Get rid of boolean.
 *     Revision 1.11  2000/01/06 21:58:18  reggers
 *     Some removal of legacy bits types.
 *     Revision 1.10  1999/11/04 20:20:26  reggers
 *     Added code for getting fixed/proportional setting, firstCharCode and
 *     lastCharCode.
 *     Revision 1.9  1999/09/30 15:11:34  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.8  1999/07/22 20:49:52  sampo
 *     Added eof comment and newline.
 *     Revision 1.7  1999/07/13 21:01:03  sampo
 *     Prototypes for Sbits functions.
 *     Revision 1.6  1999/07/09 21:16:21  sampo
 *     Improved bitmap metrics precision.
 *     Revision 1.5  1999/07/02 19:01:49  sampo
 *     Changes for embedded bitmap support.
 *     Revision 1.4  1999/06/22 15:04:46  mdewsnap
 *     Added in kern data parsing field
 *     Revision 1.3  1999/06/15 20:07:56  sampo
 *     Got rid of winFaceName.
 *     Revision 1.2  1999/06/02 16:58:19  sampo
 *     Added GetPFRNameProptery
 *     Revision 1.1  1999/04/28 15:47:03  reggers
 *     Initial revision
 *                                                                           *
 ****************************************************************************/
