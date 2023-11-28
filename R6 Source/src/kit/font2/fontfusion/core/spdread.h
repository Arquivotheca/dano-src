/*
 * SPDREAD.h
 * Font Fusion Copyright (c) 1989-2000 all rights reserved by Bitstream Inc.
 * http://www.bitstream.com/
 * http://www.typesolutions.com/
 * Author: Robert Eggers
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

/************************** S P D R E A D . H  *******************************
 *                                                                           *
 * Header file for SpeedoM binary font file examiner                         *
 *                                                                           *
 ****************************************************************************/

#ifndef _SPDRead_H
#define _SPDRead_H
#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

/***** PUBLIC FONT HEADER OFFSET CONSTANTS  *****/
#define  FH_FMVER    0      /* U   D4.0 CR LF NULL NULL  8 bytes            */
#define  FH_FNTSZ    8      /* U   Font size (bytes) 4 bytes                */
#define  FH_FBFSZ   12      /* U   Min font buffer size (bytes) 4 bytes     */
#define  FH_CBFSZ   16      /* U   Min char buffer size (bytes) 2 bytes     */
#define  FH_HEDSZ   18      /* U   Header size (bytes) 2 bytes              */
#define  FH_FNTID   20      /* U   Source Font ID  2 bytes                  */
#define  FH_SFVNR   22      /* U   Source Font Version Number  2 bytes      */
#define  FH_FNTNM   24      /* U   Source Font Name  70 bytes               */
#define  FH_MDATE   94      /* U   Manufacturing Date  10 bytes             */
#define  FH_LAYNM  104      /* U   Layout Name  70 bytes                    */
#define  FH_CPYRT  174      /* U   Copyright Notice  78 bytes               */
#define  FH_NCHRL  252      /* U   Number of Chars in Layout  2 bytes       */
#define  FH_NCHRF  254      /* U   Total Number of Chars in Font  2 bytes   */
#define  FH_FCHRF  256      /* U   Index of first char in Font  2 bytes     */
#define  FH_NKTKS  258      /* U   Number of kerning tracks in font 2 bytes */
#define  FH_NKPRS  260      /* U   Number of kerning pairs in font 2 bytes  */
#define  FH_FLAGS  262      /* U   Font flags 1 byte:                       */
                            /*       Bit 0: Extended font                   */
                            /*       Bit 1: not used                        */
                            /*       Bit 2: not used                        */
                            /*       Bit 3: not used                        */
                            /*       Bit 4: not used                        */
                            /*       Bit 5: not used                        */
                            /*       Bit 6: not used                        */
                            /*       Bit 7: not used                        */
#define  FH_CLFGS  263      /* U   Classification flags 1 byte:             */
                            /*       Bit 0: Italic                          */
                            /*       Bit 1: Monospace                       */
                            /*       Bit 2: Serif                           */
                            /*       Bit 3: Display                         */
                            /*       Bit 4: not used                        */
                            /*       Bit 5: not used                        */
                            /*       Bit 6: not used                        */
                            /*       Bit 7: not used                        */
#define  FH_FAMCL  264      /* U   Family Classification 1 byte:            */
                            /*       0:  Don't care                         */
                            /*       1:  Serif                              */
                            /*       2:  Sans serif                         */
                            /*       3:  Monospace                          */
                            /*       4:  Script or calligraphic             */
                            /*       5:  Decorative                         */
                            /*       6-255: not used                        */
#define  FH_FRMCL  265      /* U   Font form Classification 1 byte:         */
                            /*       Bits 0-3 (width type):                 */
                            /*         0-3:   not used                      */
                            /*         4:     Condensed                     */
                            /*         5:     not used                      */
                            /*         6:     Semi-condensed                */
                            /*         7:     not used                      */
                            /*         8:     Normal                        */
                            /*         9:     not used                      */
                            /*        10:     Semi-expanded                 */
                            /*        11:     not used                      */
                            /*        12:     Expanded                      */
                            /*        13-15:  not used                      */
                            /*       Bits 4-7 (Weight):                     */
                            /*         0:   not used                        */
                            /*         1:   Thin                            */
                            /*         2:   Ultralight                      */
                            /*         3:   Extralight                      */
                            /*         4:   Light                           */
                            /*         5:   Book                            */
                            /*         6:   Normal                          */
                            /*         7:   Medium                          */
                            /*         8:   Semibold                        */
                            /*         9:   Demibold                        */
                            /*         10:  Bold                            */
                            /*         11:  Extrabold                       */
                            /*         12:  Ultrabold                       */
                            /*         13:  Heavy                           */
                            /*         14:  Black                           */
                            /*         15-16: not used                      */
#define  FH_SFNTN  266      /* U   Short Font Name  32 bytes                */
#define  FH_SFACN  298      /* U   Short Face Name  16 bytes                */
#define  FH_FNTFM  314      /* U   Font form 14 bytes                       */
#define  FH_ITANG  328      /* U   Italic angle 2 bytes (1/256th deg)       */
#define  FH_ORUPM  330      /* U   Number of ORUs per em  2 bytes           */
#define  FH_WDWTH  332      /* U   Width of Wordspace  2 bytes              */
#define  FH_EMWTH  334      /* U   Width of Emspace  2 bytes                */
#define  FH_ENWTH  336      /* U   Width of Enspace  2 bytes                */
#define  FH_TNWTH  338      /* U   Width of Thinspace  2 bytes              */
#define  FH_FGWTH  340      /* U   Width of Figspace  2 bytes               */
#define  FH_FXMIN  342      /* U   Font-wide min X value  2 bytes           */
#define  FH_FYMIN  344      /* U   Font-wide min Y value  2 bytes           */
#define  FH_FXMAX  346      /* U   Font-wide max X value  2 bytes           */
#define  FH_FYMAX  348      /* U   Font-wide max Y value  2 bytes           */
#define  FH_ULPOS  350      /* U   Underline position 2 bytes               */
#define  FH_ULTHK  352      /* U   Underline thickness 2 bytes              */
#define  FH_SMCTR  354      /* U   Small caps transformation 6 bytes        */
#define  FH_DPSTR  360      /* U   Display sups transformation 6 bytes      */
#define  FH_FNSTR  366      /* U   Footnote sups transformation 6 bytes     */
#define  FH_ALSTR  372      /* U   Alpha sups transformation 6 bytes        */
#define  FH_CMITR  378      /* U   Chemical infs transformation 6 bytes     */
#define  FH_SNMTR  384      /* U   Small nums transformation 6 bytes        */
#define  FH_SDNTR  390      /* U   Small denoms transformation 6 bytes      */
#define  FH_MNMTR  396      /* U   Medium nums transformation 6 bytes       */
#define  FH_MDNTR  402      /* U   Medium denoms transformation 6 bytes     */
#define  FH_LNMTR  408      /* U   Large nums transformation 6 bytes        */
#define  FH_LDNTR  414      /* U   Large denoms transformation 6 bytes      */
                            /*     Transformation data format:              */
                            /*       Y position 2 bytes                     */
                            /*       X scale 2 bytes (1/4096ths)            */
                            /*       Y scale 2 bytes (1/4096ths)            */
#define  FH_METRS  420      /* U   Metrics resolution  2 bytes              */
#define  FH_PVHSZ  422      /* U   Private header size (bytes) 2 bytes      */
#define  FH_NDSNA  424      /* U   Number of design axes 1 byte             */

/***** PRIVATE FONT HEADER OFFSET CONSTANTS  *****/
#define  FH_ORUMX    0      /* U   Max ORU value  2 bytes                   */
#define  FH_PIXMX    2      /* U   Max Pixel value  2 bytes                 */
#define  FH_CUSNR    4      /* U   Customer Number  2 bytes                 */
#define  FH_OFFCD    6      /* E   Offset to Char Directory  3 bytes        */
#define  FH_OFCNS    9      /* E   Offset to Constraint Data  3 bytes       */
#define  FH_OFFTK   12      /* E   Offset to Track Kerning  3 bytes         */
#define  FH_OFFPK   15      /* E   Offset to Pair Kerning  3 bytes          */
#define  FH_OCHRD   18      /* E   Offset to Character Data  3 bytes        */
#define  FH_NBYTE   21      /* E   Number of Bytes in File  3 bytes         */
#define  FH_OFIRN   24      /* E   Offset to int range names  3 bytes       */
#define  FH_OFITM   27      /* E   Offset to int term mask  3 bytes         */
#define  FH_OFCAD   30      /* E   Offset to constr adj data  3 bytes       */
#define  FH_OFTAD   33      /* E   Offset to track kern adj data  3 bytes   */
#define  FH_OFPAD   36      /* E   Offset to pair kern adj data  3 bytes    */


/***** DECRYPTION KEY CONSTANTS (PC Platform) *****/
#define KEY0    0                  /* Decryption key 0 */
#define KEY1   72                  /* Decryption key 1 */
#define KEY2  123                  /* Decryption key 2 */
#define KEY3    1                  /* Decryption key 3 */
#define KEY4  222                  /* Decryption key 4 */
#define KEY5  194                  /* Decryption key 5 */
#define KEY6  113                  /* Decryption key 6 */
#define KEY7  119                  /* Decryption key 7 */
#define KEY8   52                  /* Decryption key 8 */

#define  BIT0           0x01
#define  BIT1           0x02
#define  BIT2           0x04
#define  BIT3           0x08
#define  BIT4           0x10
#define  BIT5           0x20
#define  BIT6           0x40
#define  BIT7           0x80


/****** TYPE DEFINITIONS *****/
typedef struct point_tag
    {
    int16   x;                     /* X coord of point     */
    int16   y;                     /* Y coord of point     */
    }
point_t;                           /* Point                */


typedef struct {
	uint16	charID;
	uint16	charIndex;
} spdCharDir_t;

typedef struct {
	uint16	gpsSize;
	uint32	gpsOffset;
#if 1
	uint16	charID;
#endif
} spdLocDir_t;

typedef struct {
	uint8   start_edge;
	uint8   end_edge;
	int16   constr_nr;
} controlZone_t;

typedef struct {
	uint8   start_edge;
	int16   start_adj;
	uint8   end_edge;
	int16   end_adj;
} interpolationTable_t;

typedef struct {
	F16Dot16 m00;
	F16Dot16 m01;
	F16Dot16 m10;
	F16Dot16 m11;
	F16Dot16 xOffset;
	F16Dot16 yOffset;
} spdtcb_t;


typedef struct {
	/* private */
	tsiMemObject *mem;
	InputStream *in;
	uint16	outlineRes;
	uint16	metricsRes;
	F16Dot16	xyScale;
	int8 verticalEscapement;
	uint8	contourOpen;
	unsigned char shortScaleFactors;
	long lsbx;
	long lsby;
	long awx;
	long awy;
	uint8 weight;
	/******** possible weight values and their meanings : 
					1 = Thin
					2 = Ultralight
					3 = Extra light
					4 = Light
					5 = Book
					6 = Normal
					7 = Medium
					8 = Semibold
					9 = Demibold
					10 = Bold
					11 = Extrabold
					12 = Ultrabold
					13 = Heavy
					14 = Black
	*********/
	uint16 wordSpace;
	uint16 emSpace;
	uint16 enSpace;
	uint16 thinSpace;
	uint16 figureSpace;
	
	/* font bounding box info: */
	int16 minX;
	int16 minY;
	int16 maxX;
	int16 maxY;
	
	int16 uline_pos;
	int16 uline_thickness;
	
/*	F16Dot16 m00, m01, m10, m11; */
	spdtcb_t	fontMatrix;
	spdtcb_t	outputMatrix;
	spdtcb_t	tcb;
	
	uint8	fontName[64];		/* SPD short font name */
	uint16	firstCharIndex;
	uint16	nKernPairs;
	uint32	charDirOffset;
	uint32	pairKernOffset;
	uint32	charDataOffset;
	uint16	NumLayoutChars;

	spdCharDir_t	*spdCodeToIndex;
	spdLocDir_t		*spdIndexToLoc;

	/* oru table */
	uint16  no_X_orus;
	uint16  no_Y_orus;
	int16    Y_edge_org;       /* Index to first controlled Y coord */
	int16    orus[256];        /* Controlled coordinate table */	

	/* control zone table: */
	int16	Y_zone_org;
	controlZone_t controlZoneTbl[256];

	/* interpolation zone table */
	uint8   edge_org;
	int16   no_X_int_zones;
	int16   no_Y_int_zones;
	interpolationTable_t interpolationTable[256];

	int16	x_orus, y_orus;

	/* public */
	GlyphClass *glyph;
	hmtxClass *hmtx;

	
	short NumCharStrings;
	long upem;
	long maxPointCount;
	long ascent;
	long descent;
	long lineGap;
	long advanceWidthMax;
	F16Dot16 italicAngle;
	kernClass *kern;
	
	uint32	isFixedPitch;			/* 0 = proportional, non 0 = monospace */
	uint16	firstCharCode;			/* lowest code point, character code in font */
	uint16	lastCharCode;			/* highest code point, character code in font */

	int glyphExists;				/* keep track if default glyph or found */
	
} SPDClass;

SPDClass *tsi_NewSPDClass( tsiMemObject *mem, InputStream *in, int32 fontNum );
void tsi_DeleteSPDClass( SPDClass *t );
GlyphClass *tsi_SPDGetGlyphByIndex( SPDClass *t, uint16 index, uint16 *aWidth, uint16 *aHeight );
uint16 tsi_SPDGetGlyphIndex( SPDClass *t, uint16 charCode );
uint8 *GetSPDNameProperty( SPDClass *t, uint16 languageID, uint16 nameID );
void tsi_SPDListChars(void *userArg, SPDClass *t, void *ctxPtr, int ListCharsFn(void *userArg, void *p, uint16 code));


#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif /* _PFRReader_H */
/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 * 01/21/00 R. Eggers Created                                                *
 *     $Header: R:/src/FontFusion/Source/Core/rcs/spdread.h 1.8 2001/05/01 18:28:46 reggers Exp $
 *                                                                           *
 *     $Log: spdread.h $
 *     Revision 1.8  2001/05/01 18:28:46  reggers
 *     Added support for GlyphExists()
 *     Revision 1.7  2000/05/09 17:38:47  reggers
 *     Got rid of noDeletehmtx
 *     Revision 1.6  2000/05/08 18:53:03  reggers
 *     Fixed SPDListChars prototype.
 *     Revision 1.5  2000/03/14 16:34:04  reggers
 *     Removed extraneous field, physCharMap from SPDClass.
 *     Revision 1.4  2000/03/10 19:18:15  reggers
 *     Enhanced for enumeration of character codes in font.
 *     Revision 1.3  2000/02/25 17:46:02  reggers
 *     STRICT warning cleanup.
 *     Revision 1.2  2000/02/18 19:52:15  reggers
 *     Warning cleanup- MS Studio
 *                                                                           *
******************************************************************************/
/* eof SPDRead.h */



