/*
 * T1.c
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
#include "t1.h"
#include "t1order.h"
#include "util.h"


#ifdef T1_OR_T2_IS_ENABLED
/* T1 or T2 (cff) */

#ifndef DEBUG
#define DEBUG  0
#endif

#if DEBUG
static int debugOn = 1;
#else
static int debugOn = 0;
#endif

/* #define LOG_CMD( s, d ) if (debugOn)  printf("%s %d\n", s, d ) */
/* #define LOG_CMD( s, d ) printf("%s %d\n", s, d ) */
/* #define LOG_CMD( s, d ) NULL */

#define LOG_CMD( s, d )

static short GetGlyphYMax( GlyphClass *glyph )
{
	register int i, limit = glyph->pointCount;
	register short *ooy = glyph->ooy;
	register short ymax = ooy[0];
	
	for ( i = 1; i < limit; i++ ) {
		if ( ooy[i] > ymax ) {
			ymax = ooy[i];
		}
	}
	return ymax; /*****/
}

static short GetGlyphYMin( GlyphClass *glyph )
{
	register int i, limit = glyph->pointCount;
	register short *ooy = glyph->ooy;
	register short ymin = ooy[0];
	
	for ( i = 1; i < limit; i++ ) {
		if ( ooy[i] < ymin ) {
			ymin = ooy[i];
		}
	}
	return ymin; /*****/
}

static short GetGlyphXMin( GlyphClass *glyph )
{
	register int i, limit = glyph->pointCount;
	register short *oox = glyph->oox;
	register short xmin = oox[0];
	
	for ( i = 1; i < limit; i++ ) {
		if ( oox[i] < xmin ) {
			xmin = oox[i];
		}
	}
	return xmin; /*****/
}


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

#endif /* T1_OR_T2_IS_ENABLED */

#ifdef ENABLE_T1

#ifdef ENABLE_MAC_T1
#include <resources.h>
#endif

/* 
TTD:
..
SomeDay start using InputStream....
..

DONE:
Make lenIV variable
Reverse outline direction
Parse composites
T3 scan-conversion

*/

static int T1stringsAreEqual( void *privptr, char *str, uint16 n);
static uint8 *GetNthNameFromEncodingVector( T1Class *t, int n );


/* Black Book */
const unsigned short int c1 = 52845;
const unsigned short int c2 = 22719;

typedef struct {
	unsigned short		appleCode;
	unsigned short		adobeCode;
	unsigned short		appleIndex;
	unsigned short		adobeIndex;		/* SDS 8/17/92 - Added adobeIndex (See AdobeCharOrder.h) */
	char				*name;
} sfnt_CharToName;

#define MISSING_GLYPH_INDEX		((unsigned short)0x0)
#define MISSING_ADOBE_CODE		((unsigned short)0xffff)
#define MISSING_ADOBE_INDEX		((unsigned short)0xffff)

#define MISSING_11				((unsigned short)-11)


/* We need to replace the -11s with real data!!! */
/* SDS - Added adobeIndex portion of table (for Type 1) */
/* We need to replace the -11s with real data!!! */
static const sfnt_CharToName sfnt_CharToNameTable[] = {
	{ 0x00, 0x00, 0x00, MISSING_ADOBE_INDEX, ".notdef" },	/* was .NUL SDS 8/18/92 */
	{ 0x01, MISSING_ADOBE_CODE, MISSING_GLYPH_INDEX, MISSING_ADOBE_INDEX, ".SOH" },
	{ 0x02, MISSING_ADOBE_CODE, MISSING_GLYPH_INDEX, MISSING_ADOBE_INDEX, ".STX" },
	{ 0x03, MISSING_ADOBE_CODE, MISSING_GLYPH_INDEX, MISSING_ADOBE_INDEX, ".ETX" },
	{ 0x04, MISSING_ADOBE_CODE, MISSING_GLYPH_INDEX, MISSING_ADOBE_INDEX, ".EOT" },
	{ 0x05, MISSING_ADOBE_CODE, MISSING_GLYPH_INDEX, MISSING_ADOBE_INDEX, ".ENQ" },
	{ 0x06, MISSING_ADOBE_CODE, MISSING_GLYPH_INDEX, MISSING_ADOBE_INDEX, ".ACK" },
	{ 0x07, MISSING_ADOBE_CODE, MISSING_GLYPH_INDEX, MISSING_ADOBE_INDEX, ".BEL" },
	{ 0x08, MISSING_ADOBE_CODE, MISSING_GLYPH_INDEX, MISSING_ADOBE_INDEX, ".BS" },
	{ 0x09, MISSING_ADOBE_CODE, MISSING_GLYPH_INDEX, MISSING_ADOBE_INDEX, ".HT" },
	{ 0x0a, MISSING_ADOBE_CODE, MISSING_GLYPH_INDEX, MISSING_ADOBE_INDEX, ".LF" },
	{ 0x0b, MISSING_ADOBE_CODE, MISSING_GLYPH_INDEX, MISSING_ADOBE_INDEX, ".VT" },
	{ 0x0c, MISSING_ADOBE_CODE, MISSING_GLYPH_INDEX, MISSING_ADOBE_INDEX, ".FF" },
	{ 0x0d, 0x0d, 0x02, MISSING_ADOBE_INDEX, "CR" },	/* was .CR SDS 8/18/92 */
	{ 0x0e, MISSING_ADOBE_CODE, MISSING_GLYPH_INDEX, MISSING_ADOBE_INDEX, ".SO" },
	{ 0x0f, MISSING_ADOBE_CODE, MISSING_GLYPH_INDEX, MISSING_ADOBE_INDEX, ".SI" },
	{ 0x10, MISSING_ADOBE_CODE, MISSING_GLYPH_INDEX, MISSING_ADOBE_INDEX, ".DLE" },
	{ 0x11, MISSING_ADOBE_CODE, MISSING_GLYPH_INDEX, MISSING_ADOBE_INDEX, ".DC1" },
	{ 0x12, MISSING_ADOBE_CODE, MISSING_GLYPH_INDEX, MISSING_ADOBE_INDEX, ".DC2" },
	{ 0x13, MISSING_ADOBE_CODE, MISSING_GLYPH_INDEX, MISSING_ADOBE_INDEX, ".DC3" },
	{ 0x14, MISSING_ADOBE_CODE, MISSING_GLYPH_INDEX, MISSING_ADOBE_INDEX, ".DC4" },
	{ 0x15, MISSING_ADOBE_CODE, MISSING_GLYPH_INDEX, MISSING_ADOBE_INDEX, ".NAK" },
	{ 0x16, MISSING_ADOBE_CODE, MISSING_GLYPH_INDEX, MISSING_ADOBE_INDEX, ".SYN" },
	{ 0x17, MISSING_ADOBE_CODE, MISSING_GLYPH_INDEX, MISSING_ADOBE_INDEX, ".ETB" },
	{ 0x18, MISSING_ADOBE_CODE, MISSING_GLYPH_INDEX, MISSING_ADOBE_INDEX, ".CAN" },
	{ 0x19, MISSING_ADOBE_CODE, MISSING_GLYPH_INDEX, MISSING_ADOBE_INDEX, ".EM" },
	{ 0x1a, MISSING_ADOBE_CODE, MISSING_GLYPH_INDEX, MISSING_ADOBE_INDEX, ".SUB" },
	{ 0x1b, MISSING_ADOBE_CODE, MISSING_GLYPH_INDEX, MISSING_ADOBE_INDEX, ".ESC" },
	{ 0x1c, MISSING_ADOBE_CODE, MISSING_GLYPH_INDEX, MISSING_ADOBE_INDEX, ".FS" },
	{ 0x1d, MISSING_ADOBE_CODE, 0x01, MISSING_ADOBE_INDEX, "NULL" },	/* was .GS 8/28/92 */
	{ 0x1e, MISSING_ADOBE_CODE, MISSING_GLYPH_INDEX, MISSING_ADOBE_INDEX, ".RS" },
	{ 0x1f, MISSING_ADOBE_CODE, MISSING_GLYPH_INDEX, MISSING_ADOBE_INDEX, ".US" },
	{ 0x20, 0x20, 0x03, kspace, "space" },
	{ 0x21, 0x21, 0x04, kexclam, "exclam" },
	{ 0x22, 0x22, 0x05, kquotedbl, "quotedbl" },
	{ 0x23, 0x23, 0x06, knumbersign, "numbersign" },
	{ 0x24, 0x24, 0x07, kdollar, "dollar" },
	{ 0x25, 0x25, 0x08, kpercent, "percent" },
	{ 0x26, 0x26, 0x09, kampersand, "ampersand" },
	{ 0x27, 0xa9, 0x0a, kquotesingle, "quotesingle" },
	{ 0x28, 0x28, 0x0b, kparenleft, "parenleft" },
	{ 0x29, 0x29, 0x0c, kparenright, "parenright" },
	{ 0x2a, 0x2a, 0x0d, kasterisk, "asterisk" },
	{ 0x2b, 0x2b, 0x0e, kplus, "plus" },
	{ 0x2c, 0x2c, 0x0f, kcomma, "comma" },
	{ 0x2d, 0x2d, 0x10, khyphen, "hyphen" },
	{ 0x2e, 0x2e, 0x11, kperiod, "period" },
	{ 0x2f, 0x2f, 0x12, kslash, "slash" },
	{ 0x30, 0x30, 0x13, kzero, "zero" },
	{ 0x31, 0x31, 0x14, kone, "one" },
	{ 0x32, 0x32, 0x15, ktwo, "two" },
	{ 0x33, 0x33, 0x16, kthree, "three" },
	{ 0x34, 0x34, 0x17, kfour, "four" },
	{ 0x35, 0x35, 0x18, kfive, "five" },
	{ 0x36, 0x36, 0x19, ksix, "six" },
	{ 0x37, 0x37, 0x1a, kseven, "seven" },
	{ 0x38, 0x38, 0x1b, keight, "eight" },
	{ 0x39, 0x39, 0x1c, knine, "nine" },
	{ 0x3a, 0x3a, 0x1d, kcolon, "colon" },
	{ 0x3b, 0x3b, 0x1e, ksemicolon, "semicolon" },
	{ 0x3c, 0x3c, 0x1f, kless, "less" },
	{ 0x3d, 0x3d, 0x20, kequal, "equal" },
	{ 0x3e, 0x3e, 0x21, kgreater, "greater" },
	{ 0x3f, 0x3f, 0x22, kquestion, "question" },
	{ 0x40, 0x40, 0x23, kat, "at" },
	{ 0x41, 0x41, 0x24, kA, "A" },
	{ 0x42, 0x42, 0x25, kB, "B" },
	{ 0x43, 0x43, 0x26, kC, "C" },
	{ 0x44, 0x44, 0x27, kD, "D" },
	{ 0x45, 0x45, 0x28, kE, "E" },
	{ 0x46, 0x46, 0x29, kF, "F" },
	{ 0x47, 0x47, 0x2a, kG, "G" },
	{ 0x48, 0x48, 0x2b, kH, "H" },
	{ 0x49, 0x49, 0x2c, kI, "I" },
	{ 0x4a, 0x4a, 0x2d, kJ, "J" },
	{ 0x4b, 0x4b, 0x2e, kK, "K" },
	{ 0x4c, 0x4c, 0x2f, kL, "L" },
	{ 0x4d, 0x4d, 0x30, kM, "M" },
	{ 0x4e, 0x4e, 0x31, kN, "N" },
	{ 0x4f, 0x4f, 0x32, kO, "O" },
	{ 0x50, 0x50, 0x33, kP, "P" },
	{ 0x51, 0x51, 0x34, kQ, "Q" },
	{ 0x52, 0x52, 0x35, kR, "R" },
	{ 0x53, 0x53, 0x36, kS, "S" },
	{ 0x54, 0x54, 0x37, kT, "T" },
	{ 0x55, 0x55, 0x38, kU, "U" },
	{ 0x56, 0x56, 0x39, kV, "V" },
	{ 0x57, 0x57, 0x3a, kW, "W" },
	{ 0x58, 0x58, 0x3b, kX, "X" },
	{ 0x59, 0x59, 0x3c, kY, "Y" },
	{ 0x5a, 0x5a, 0x3d, kZ, "Z" },
	{ 0x5b, 0x5b, 0x3e, kbracketleft, "bracketleft" },
	{ 0x5c, 0x5c, 0x3f, kbackslash, "backslash" },
	{ 0x5d, 0x5d, 0x40, kbracketright, "bracketright" },
	{ 0x5e, 0x5e, 0x41, kasciicircum, "asciicircum" },
/*	{ 0x5e, 0xc3, 0xd8, "asciicircum" },*/
/*	{ 0x5e, 0xc3, 0xd8, "circumflex" },*/
	{ 0x5f, 0x5f, 0x42, kunderscore, "underscore" },
	{ 0x60, 0xc1, 0x43, kgrave, "grave" },
	{ 0x61, 0x61, 0x44, ka, "a" },
	{ 0x62, 0x62, 0x45, kb, "b" },
	{ 0x63, 0x63, 0x46, kc, "c" },
	{ 0x64, 0x64, 0x47, kd, "d" },
	{ 0x65, 0x65, 0x48, ke, "e" },
	{ 0x66, 0x66, 0x49, kf, "f" },
	{ 0x67, 0x67, 0x4a, kg, "g" },
	{ 0x68, 0x68, 0x4b, kh, "h" },
	{ 0x69, 0x69, 0x4c, ki, "i" },
	{ 0x6a, 0x6a, 0x4d, kj, "j" },
	{ 0x6b, 0x6b, 0x4e, kk, "k" },
	{ 0x6c, 0x6c, 0x4f, kl, "l" },
	{ 0x6d, 0x6d, 0x50, km, "m" },
	{ 0x6e, 0x6e, 0x51, kn, "n" },
	{ 0x6f, 0x6f, 0x52, ko, "o" },
	{ 0x70, 0x70, 0x53, kp, "p" },
	{ 0x71, 0x71, 0x54, kq, "q" },
	{ 0x72, 0x72, 0x55, kr, "r" },
	{ 0x73, 0x73, 0x56, ks, "s" },
	{ 0x74, 0x74, 0x57, kt, "t" },
	{ 0x75, 0x75, 0x58, ku, "u" },
	{ 0x76, 0x76, 0x59, kv, "v" },
	{ 0x77, 0x77, 0x5a, kw, "w" },
	{ 0x78, 0x78, 0x5b, kx, "x" },
	{ 0x79, 0x79, 0x5c, ky, "y" },
	{ 0x7a, 0x7a, 0x5d, kz, "z" },
	{ 0x7b, 0x7b, 0x5e, kbraceleft, "braceleft" },
	{ 0x7c, 0x7c, 0x5f, kbar, "bar" },
	{ 0x7d, 0x7d, 0x60, kbraceright, "braceright" },
	{ 0x7e, 0x7e, 0x61, kasciitilde, "asciitilde" },	/* 61 was d9 */
	{ 0x7f, MISSING_ADOBE_CODE, MISSING_GLYPH_INDEX, MISSING_ADOBE_INDEX, ".DEL" },
	{ 0x80, 0xffff, 0x62, kAdieresis, "Adieresis" },
	{ 0x81, 0xffff, 0x63, kAring, "Aring" },
	{ 0x82, 0xffff, 0x64, kCcedilla, "Ccedilla" },
	{ 0x83, 0xffff, 0x65, kEacute, "Eacute" },
	{ 0x84, 0xffff, 0x66, kNtilde, "Ntilde" },
	{ 0x85, 0xffff, 0x67, kOdieresis, "Odieresis" },
	{ 0x86, 0xffff, 0x68, kUdieresis, "Udieresis" },
	{ 0x87, 0xffff, 0x69, kaacute, "aacute" },
	{ 0x88, 0xffff, 0x6a, kagrave, "agrave" },
	{ 0x89, 0xffff, 0x6b, kacircumflex, "acircumflex" },
	{ 0x8a, 0xffff, 0x6c, kadieresis, "adieresis" },
	{ 0x8b, 0xffff, 0x6d, katilde, "atilde" },
	{ 0x8c, 0xffff, 0x6e, karing, "aring" },
	{ 0x8d, 0xffff, 0x6f, kccedilla, "ccedilla" },
	{ 0x8e, 0xffff, 0x70, keacute, "eacute" },
	{ 0x8f, 0xffff, 0x71, kegrave, "egrave" },
	{ 0x90, 0xffff, 0x72, kecircumflex, "ecircumflex" },
	{ 0x91, 0xffff, 0x73, kedieresis, "edieresis" },
	{ 0x92, 0xffff, 0x74, kiacute, "iacute" },
	{ 0x93, 0xffff, 0x75, kigrave, "igrave" },
	{ 0x94, 0xffff, 0x76, kicircumflex, "icircumflex" },
	{ 0x95, 0xffff, 0x77, kidieresis, "idieresis" },
	{ 0x96, 0xffff, 0x78, kntilde, "ntilde" },
	{ 0x97, 0xffff, 0x79, koacute, "oacute" },
	{ 0x98, 0xffff, 0x7a, kograve, "ograve" },
	{ 0x99, 0xffff, 0x7b, kocircumflex, "ocircumflex" },
	{ 0x9a, 0xffff, 0x7c, kodieresis, "odieresis" },
	{ 0x9b, 0xffff, 0x7d, kotilde, "otilde" },
	{ 0x9c, 0xffff, 0x7e, kuacute, "uacute" },
	{ 0x9d, 0xffff, 0x7f, kugrave, "ugrave" },
	{ 0x9e, 0xffff, 0x80, kucircumflex, "ucircumflex" },
	{ 0x9f, 0xffff, 0x81, kudieresis, "udieresis" },
	{ 0xa0, 0xb2, 0x82, kdagger, "dagger" },
	{ 0xa1, 0xffff, 0x83, kdegree, "degree" },
	{ 0xa2, 0xa2, 0x84, kcent, "cent" },
	{ 0xa3, 0xa3, 0x85, ksterling, "sterling" },
	{ 0xa4, 0xa7, 0x86, ksection, "section" },
	{ 0xa5, 0xb7, 0x87, kbullet, "bullet" },
	{ 0xa6, 0xb6, 0x88, kparagraph, "paragraph" },
	{ 0xa7, 0xfb, 0x89, kgermandbls, "germandbls" },
	{ 0xa8, 0xffff, 0x8a, kregistered, "registered" },
	{ 0xa9, 0xffff, 0x8b, kcopyright, "copyright" },
	{ 0xaa, 0xffff, 0x8c, ktrademark, "trademark" },
	{ 0xab, 0xc2, 0x8d, kacute, "acute" },
	{ 0xac, 0xc8, 0x8e, kdieresis, "dieresis" },
	{ 0xad, 0xad, 0x8f, MISSING_ADOBE_INDEX, "notequal" },
	{ 0xae, 0xe1, 0x90, kAE, "AE" },
	{ 0xaf, 0xe9, 0x91, kOslash, "Oslash" },
	{ 0xb0, MISSING_ADOBE_CODE, 0x92, MISSING_ADOBE_INDEX, "infinity" },
	{ 0xb1, 0xffff, 0x93, kplusminus, "plusminus" },
	{ 0xb2, MISSING_11, 0x94, klessequal, "lessequal" },
	{ 0xb3, MISSING_11, 0x95, kgreaterequal, "greaterequal" },
	{ 0xb4, 0xa5, 0x96, kyen, "yen" },
	{ 0xb5, MISSING_11, 0x97, kmu, "mu" },
	{ 0xb6, MISSING_11, 0x98, MISSING_ADOBE_INDEX, "partialdiff" },	/* was partialdif SDS 8/18/92 */
	{ 0xb7, MISSING_11, 0x99, MISSING_ADOBE_INDEX, "summation" },
	{ 0xb8, MISSING_11, 0x9a, MISSING_ADOBE_INDEX, "product" },
	{ 0xb9, MISSING_11, 0x9b, MISSING_ADOBE_INDEX, "pi" },
	{ 0xba, MISSING_11, 0x9c, MISSING_ADOBE_INDEX, "integral" },
	{ 0xbb, 0xe3, 0x9d, kordfeminine, "ordfeminine" },
	{ 0xbc, 0xeb, 0x9e, kordmasculine, "ordmasculine" },
	{ 0xbd, MISSING_11, 0x9f, MISSING_ADOBE_INDEX, "Omega" },		/* was omega SDS 8/18/92 */
	{ 0xbe, 0xf1, 0xa0, kae, "ae" },
	{ 0xbf, 0xf9, 0xa1, koslash, "oslash" },
	{ 0xc0, 0xbf, 0xa2, kquestiondown, "questiondown" },
	{ 0xc1, 0xa1, 0xa3, kexclamdown, "exclamdown" },
	{ 0xc2, 0xffff, 0xa4, klogicalnot, "logicalnot" },
	{ 0xc3, MISSING_11, 0xa5, MISSING_ADOBE_INDEX, "radical" },
	{ 0xc4, 0xa6, 0xa6, kflorin, "florin" },
	{ 0xc5, MISSING_11, 0xa7, MISSING_ADOBE_INDEX, "approxequal" },
	{ 0xc6, MISSING_11, 0xa8, kdelta, "Delta" },			/* was delta SDS 8/18/92 */
	{ 0xc7, 0xab, 0xa9, kguillemotleft, "guillemotleft" },
	{ 0xc8, 0xbb, 0xaa, kguillemotright, "guillemotright" },
	{ 0xc9, 0xbc, 0xab, kellipsis, "ellipsis" },
	{ 0xca, MISSING_11, 0xac, MISSING_ADOBE_INDEX, "nbspace" }, /* was NON_BREAKING_SPACE SDS 8/18/92 */
	{ 0xcb, 0xffff, 0xad, kAgrave, "Agrave" },
	{ 0xcc, 0xffff, 0xae, kAtilde, "Atilde" },
	{ 0xcd, 0xffff, 0xaf, kOtilde, "Otilde" },
	{ 0xce, 0xea, 0xb0, kOE, "OE" },
	{ 0xcf, 0xfa, 0xb1, koe, "oe" },
/*	{ 0xd0*, -11, -11, "en" },*/
	{ 0xd0, 0xb1, 0xb2, kendash, "endash" },
	{ 0xd1, 0xd0, 0xb3, kemdash, "emdash" },
	{ 0xd2, 0xaa, 0xb4, kquotedblleft, "quotedblleft" },
	{ 0xd3, 0xba, 0xb5, kquotedblright, "quotedblright" },
	{ 0xd4, 0x60, 0xb6, kquoteleft, "quoteleft" },
	{ 0xd5, 0x27, 0xb7, kquoteright, "quoteright" },
	{ 0xd6, 0xffff, 0xb8, kdivide, "divide" },
	{ 0xd7, MISSING_ADOBE_CODE, 0xb9, MISSING_ADOBE_INDEX, "lozenge" },
	{ 0xd8, 0xffff, 0xba, kydieresis, "ydieresis" },
	{ 0xd9, 0xffff, 0xbb, kYdieresis, "Ydieresis" },
	{ 0xda, 0xa4, 0xbc, kfraction, "fraction" },
	{ 0xdb, 0xa8, 0xbd, kcurrency, "currency" },
	{ 0xdc, 0xac, 0xbe, kguilsinglleft, "guilsinglleft" },
	{ 0xdd, 0xad, 0xbf, kguilsinglright, "guilsinglright" },
	{ 0xde, 0xae, 0xc0, kfi, "fi" },
	{ 0xdf, 0xaf, 0xc1, kfl, "fl" },
	{ 0xe0, 0xb3, 0xc2, kdaggerdbl, "daggerdbl" },
	{ 0xe1, 0xb4, 0xc3, kperiodcentered, "periodcentered" },
	{ 0xe2, 0xb8, 0xc4, kquotesinglbase, "quotesinglbase" },
	{ 0xe3, 0xb9, 0xc5, kquotedblbase, "quotedblbase" },
	{ 0xe4, 0xbd, 0xc6, kperthousand, "perthousand" },
	{ 0xe5, 0xffff, 0xc7, kAcircumflex, "Acircumflex" },
	{ 0xe6, 0xffff, 0xc8, kEcircumflex, "Ecircumflex" },
	{ 0xe7, 0xffff, 0xc9, kAacute, "Aacute" },
	{ 0xe8, 0xffff, 0xca, kEdieresis, "Edieresis" },
	{ 0xe9, 0xffff, 0xcb, kEgrave, "Egrave" },
	{ 0xea, 0xffff, 0xcc, kIacute, "Iacute" },
	{ 0xeb, 0xffff, 0xcd, kIcircumflex, "Icircumflex" },
	{ 0xec, 0xffff, 0xce, kIdieresis, "Idieresis" },
	{ 0xed, 0xffff, 0xcf, kIgrave, "Igrave" },
	{ 0xee, 0xffff, 0xd0, kOacute, "Oacute" },
	{ 0xef, 0xffff, 0xd1, kOcircumflex, "Ocircumflex" },
	{ 0xf0, 0xffff, 0xd2, MISSING_ADOBE_INDEX, "apple" },
	{ 0xf1, 0xffff, 0xd3, kOgrave, "Ograve" },
	{ 0xf2, 0xffff, 0xd4, kUacute, "Uacute" },
	{ 0xf3, 0xffff, 0xd5, kUcircumflex, "Ucircumflex" },
	{ 0xf4, 0xffff, 0xd6, kUgrave, "Ugrave" },
	{ 0xf5, 0xf5, 0xd7, kdotlessi, "dotlessi" },				/* SDS 0xf5 was -11 which was wrong!!! */
	{ 0xf6, 0xc3, 0xd8, kcircumflex, "circumflex" },			/* SDS 0xc3 was -11 which was wrong!!! */
	{ 0xf7, 0xc4, 0xd9, ktilde, "tilde" },						/* SDS 0xc4 was -11 which was wrong!!! */
	{ 0xf8, 0xc5, 0xda, kmacron, "macron" },
	{ 0xf9, 0xc6, 0xdb, kbreve, "breve" },
	{ 0xfa, 0xc7, 0xdc, kdotaccent, "dotaccent" },
	{ 0xfb, 0xca, 0xdd, kring, "ring" },
	{ 0xfc, 0xcb, 0xde, kcedilla, "cedilla" },				/* SDS 0xcb was -11 which was wrong!!! */
	{ 0xfd, 0xcd, 0xdf, khungarumlaut, "hungarumlaut" },
	{ 0xfe, 0xce, 0xe0, kogonek, "ogonek" },
	{ 0xff, 0xcf, 0xe1, kcaron, "caron" },
	{ 0xffff, 0xe8, 0xe2, kLslash, "Lslash" },
	{ 0xffff, 0xf8, 0xe3, klslash, "lslash" },
	/*{ 0xffff, 0xffff, 0x97, kmu, "mu" },*/
	{ 0xffff, 0xffff, 0xe4, kScaron, "Scaron" },
	{ 0xffff, 0xffff, 0xe5, kscaron, "scaron" },
	{ 0xffff, 0xffff, 0xe6, kZcaron, "Zcaron" },
	{ 0xffff, 0xffff, 0xe7, kzcaron, "zcaron" },
	{ 0xffff, 0xffff, 0xe8, kbrokenbar, "brokenbar" },
	{ 0xffff, 0xffff, 0xe9, kEth, "Eth" },
	{ 0xffff, 0xffff, 0xea, keth, "eth" },
	{ 0xffff, 0xffff, 0xeb, kYacute, "Yacute" },
	{ 0xffff, 0xffff, 0xec, kYacute, "Yacute" },
	{ 0xffff, 0xffff, 0xed, kThorn, "Thorn" },
	{ 0xffff, 0xffff, 0xee, kthorn, "thorn" },
	{ 0xffff, 0xffff, 0xef, kminus, "minus" },
	{ 0xffff, 0xffff, 0xf0, kmultiply, "multiply" },
	{ 0xffff, 0xffff, 0xf1, konesuperior, "onesuperior" },
	{ 0xffff, 0xffff, 0xf2, ktwosuperior, "twosuperior" },
	{ 0xffff, 0xffff, 0xf3, kthreesuperior, "threesuperior" },
	{ 0xffff, 0xffff, 0xf4, konehalf, "onehalf" },
	{ 0xffff, 0xffff, 0xf5, konequarter, "onequarter" },
	{ 0xffff, 0xffff, 0xf6, kthreequarters, "threequarters" },
	{ 0xffff, 0xffff, 0xf7, kfranc, "franc" },
	{ 0xffff, 0xffff, 0xf8, kGbreve, "Gbreve" },
	{ 0xffff, 0xffff, 0xf9, kgbreve, "gbreve" },
	{ 0xffff, 0xffff, 0xfa, kIdot, "Idot" },
	{ 0xffff, 0xffff, 0xfb, kScedilla, "Scedilla" },
	{ 0xffff, 0xffff, 0xfc, kscedilla, "scedilla" },
	{ 0xffff, 0xffff, 0xfd, kCacute, "Cacute" },
	{ 0xffff, 0xffff, 0xfe, kcacute, "cacute" },
	{ 0xffff, 0xffff, 0xff, kCcaron, "Ccaron" },
	{ 0xffff, 0xffff, 0x100, kccaron, "ccaron" },
	{ 0xffff, 0xffff, 0x101, kdmacron, "dmacron" }
};

#define		NumAppleCodes		((sizeof(sfnt_CharToNameTable))/(sizeof(sfnt_CharToName)))

#ifdef ENABLE_MAC_T1
typedef void (*DoFuncPtr) ( unsigned long *length, Handle hPOST, char *pCumulative );

static void ComputeLength( unsigned long *length, Handle hPOST, char *pCumulative )
{
	pCumulative;
	assert( hPOST != 0L );
	assert( *hPOST != 0L );
	
	*length += GetHandleSize( hPOST ) - 2;		/* The first two bytes "don't count" */
}

static void AppendData(unsigned long *length, Handle hPOST, char *pCumulative)
{
	unsigned long	start;
	
	assert( pCumulative != 0L );
	assert( hPOST != 0L );
	assert( *hPOST != 0L );

	start = *length;
	ComputeLength( length, hPOST, 0L );			/* This figures the end of the data */
	
	assert( (*length - start + 2L) == GetHandleSize( hPOST ));

	BlockMove( (*hPOST) + 2L, pCumulative + start, (long)(*length - start) );
}

static void ForAllEncodedPOSTDo( DoFuncPtr fPtr, unsigned long *length, char *pCumulative, short refNum )
{
	short	rID;
	Handle	hRsrc;
	short	curRefNum;
	
	curRefNum = CurResFile(  ); /* save */
	for (rID = 501; rID < 32767; rID++ ) {
		UseResFile( refNum );
		hRsrc = Get1Resource( 'POST', rID );
		if ( hRsrc != NULL ) {
			short postType;
			
			postType = *((short*)*hRsrc);
			if ( postType != 0x0300 ) { /* Type 1 or 2 binary data */
				(*fPtr)( length, hRsrc, pCumulative );
			}
			ReleaseResource( hRsrc );
			if ( postType == 0x0300 ) break; /*****/ /* End of File */
		} else {
			break; /*****/
		}
	}
	UseResFile( curRefNum ); /* restore */
}


/* The caller has to free the returned pointer */
char *ExtractPureT1FromMacPOSTResources( tsiMemObject *mem, short refNum, unsigned long *length )
{
	char *returnData = 0L;
	
	*length = 0;
	ForAllEncodedPOSTDo( ComputeLength, length, returnData, refNum );
	if ( *length != 0 ) {
		returnData = (char *)tsi_AllocMem( mem, *length );
		*length = 0L;
		ForAllEncodedPOSTDo( AppendData, length, returnData, refNum );
	}	
	return returnData; /*****/	
}

/*
short refNum;
Str255 pascalname;

pascalName[0] = strlen( "AVReg.mac" );
strcpy( (char *)&pascalName[1], "AVReg.mac" );
refNum = OpenResFile( Str255 fileName );
assert( ResError() == noErr );
CloseResFile( refNum );
*/

#endif /* ENABLE_MAC_T1 */



/*
 * Returns a non zero error code if the font data is bad.
 */
unsigned char *ExtractPureT1FromPCType1( unsigned char *src, unsigned long *length, int *errCode  )
{
    unsigned char *dest, *base;
    unsigned char b1, b2;
    unsigned long i, segmentLength;

    assert( length != NULL );

    *errCode = T2K_BAD_FONT;
    dest = base = src;

    while ( (unsigned long)(src-base) < *length ) {
        b1 = src[0];
        b2 = src[1];
        if ( b1 != 128 ) return NULL; /* bail out, the .pfb data is bad!!! */
            if ( b2 == 3 ) break; /*****/ /* End of file indication */

            segmentLength = src[5];
            segmentLength <<= 8;
            segmentLength |= src[4];
            segmentLength <<= 8;
            segmentLength |= src[3];
            segmentLength <<= 8;
            segmentLength |= src[2];
            src += 6;

            for ( i = 0; i < segmentLength; i++ ) {
                    *dest++ = *src++;
            }
	}

    *errCode = 0;
    *length = (unsigned long)(dest - base);
    return base;
}


#ifdef ENABLE_DECRYPT
static unsigned char Decrypt( unsigned char cipher, uint16 *r )
{
	unsigned char plain;

	plain	= (unsigned char)(cipher ^ (*r>>8));
	*r		= (uint16)((cipher + *r) * c1 + c2);
	return	plain; /*****/
}
#endif

#ifdef ENABLE_ENCRYPT
static unsigned char Encrypt( unsigned char plain, uint16 *r )
{
	unsigned char cipher;

	cipher	= (unsigned char)(plain ^ (*r>>8));
	*r		= (uint16)((cipher + *r) * c1 + c2);
	return	cipher; /*****/
}
#endif


#ifdef ENABLE_EITHER_CRYPT

static short ATOI( register const uint8 *aData )
{
	register short num = 0;
	register uint8 c;

	for (;;) {
		c = *aData;
		if ( !( (c >= '0' && c <= '9') || c == '-' ) ) {
			aData++;
			continue; /*****/
		}
		if ( c == '-' ) aData++;
		break;/*****/
	}
	while ((*aData >= '0') && (*aData <= '9')) {
		num *= 10;
		num = (short)(num + *aData - '0');
		aData++;
	}
	return (short)(c == '-' ? -num: num); /*****/
}

static short backwardsATOI( register const char *aData )
{
	register short num = 0;
	
	aData++;
	
	while (*aData == ' ') {
		aData--;					/* Skip initial white-space */
	}
	
	while ((*aData >= '0') && (*aData <= '9'))
		aData--;
	
	aData++;
	
	while ((*aData >= '0') && (*aData <= '9')) {
		num *= 10;
		num = (short)(num + *aData - '0');
		aData++;
	}
	
	return num; /*****/
}
#endif

/* parses 10, 10.5, -10.9, .185339E-3 etc.. */
/* We also multiply the result by 10^expAdd  */
static F16Dot16 ATOFixed( register uint8 *aData, long expAdd )
{
	register long inum = 0;
	register long num, denom, exponent;
	register uint8 c;
	F16Dot16 result;

	for (;;) {
		c = *aData;
		if ( !( (c >= '0' && c <= '9') || c == '-' || c == '.' ) ) {
			aData++;
			continue; /*****/
		}
		if ( c == '-' ) aData++;
		break;/*****/
	}
	while ((*aData >= '0') && (*aData <= '9')) {
		inum *= 10;
		inum = inum + (*aData - '0');
		aData++;
	}
	result = inum << 16;
	if ( *aData == '.' ) {
		aData++;
		for ( num = 0, denom = 1; (*aData >= '0') && (*aData <= '9'); ) {
			if ( denom < 100000000L ) {
				num *= 10; denom *= 10;
				num = num + (*aData - '0');
			}
			aData++;
		}
		exponent = ( *aData == 'E' || *aData == 'e' ) ? ATOI( ++aData ) : 0;
		/* printf("inum = %d, num = %d, denom = %d, exponent = %d\n", inum, num, denom, exponent ); */
		/* The value is num/demom * 10 ^ exponent */
		exponent += expAdd; /* Multiply by 10^expAdd */
		for ( ; exponent > 0; exponent-- ) num   *= 10;
		for ( ; exponent < 0; exponent++ ) denom *= 10;
		result += util_FixDiv( (F16Dot16)num, (F16Dot16)denom );
	}
	return (c == '-' ? -result: result); /*****/
}



#ifdef ENABLE_DECRYPT
static int IsHex( register char c )
{
	return ( (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f') );
}

static uint8 MapHex( register uint8 c )
{
	if ( c <= '9' ) {
		return (uint8)(c - '0');
	} else if ( c <= 'F' ) {
		return (uint8)(c - 'A' + 10);
	} else {
		return (uint8)(c - 'a' + 10);
	}
}

static long DecryptData( uint8 *aData, long dataLen )
{
	long	i;
	uint16	r1, r2, byteCount;
	uint8   c_0, c_1, c_2, c_3;
	
	assert( dataLen >= 4 );
	
	r1			= 55665;	/* Magic starting decryption number - p.63 Black Book */
	byteCount	= 0;

	c_2 = c_1 = c_0 = 0;
	
	if ( IsHex( (char)aData[0] ) && IsHex( (char)aData[1] ) && IsHex( (char)aData[2] ) && IsHex( (char)aData[3] ) ) {
		long j = 0;
		uint8 binData, h1, h2;
		/* ascii form form */
		for ( i = 0; i < dataLen; ) {
			do { /* get data and skip white space */
				h1 = aData[i++];
			} while ( h1 == ' ' || h1 == '\n' || h1 == '\r' || h1 == '\t' );
			do { /* get data and skip white space */
				h2 = aData[i++];
			} while ( h2 == ' ' || h2 == '\n' || h2 == '\r' || h2 == '\t' );
			binData = MapHex( h1 );
			binData <<= 4;
			binData |= MapHex( h2 );
			
			c_3 = c_2; c_2 = c_1; c_1 = c_0;
			if ( byteCount ) {			/* Decrypt a byte run which is doubly encoded */
				aData[j] = c_0 = Decrypt(Decrypt(binData, &r1), &r2);
				byteCount--;
			} else {
				aData[j] = c_0 = Decrypt(binData, &r1);
				if ( c_3 == ' ' && c_0 == ' ' && ((c_2 == 'R' && c_1 == 'D') || (c_2 == '-' && c_1 == '|')) ) {
					byteCount	= (unsigned short)backwardsATOI( (char*)&aData[j-4] );
					r2			= 4330;	/* charstring encryption has this initial R - p.64 Black Book */
				}
			}
			j++;
		}
		dataLen = i;
	} else {
		/* binary form */
		for ( i = 0; i < dataLen; i++ ) {
			c_3 = c_2; c_2 = c_1; c_1 = c_0;
			if ( byteCount ) {			/* Decrypt a byte run which is doubly encoded */
				aData[i] = c_0 = Decrypt(Decrypt(aData[i], &r1), &r2);
				byteCount--;
			} else {
				aData[i] = c_0 = Decrypt(aData[i], &r1);
				if ( c_3 == ' ' && c_0 == ' ' && ((c_2 == 'R' && c_1 == 'D') || (c_2 == '-' && c_1 == '|')) ) {
					byteCount	= (unsigned short)backwardsATOI( (char*)&aData[i-4] );
					r2			= 4330;	/* charstring encryption has this initial R - p.64 Black Book */
				}
			}
		}
		assert( byteCount == 0);
		assert( i == dataLen );
	}
	return dataLen; /*****/
}
#endif

#ifdef ENABLE_ENCRYPT
/* Caller has to deallocate the returned pointer !!! */
static uint8 *EncryptData( register T1Class *t, uint8 *decryptedData, long dataLen )
{
	uint8	*aData;
	long	i;
	uint16	r1, r2, byteCount;
	
	assert( dataLen >= 4 );
	aData = (uint8 *)tsi_AllocMem(t->mem, dataLen);
	
	r1 			= 55665;	/* Magic starting decryption number - p.63 Black Book */
	byteCount	= 0;

	for ( i = 0; i < dataLen; i++ ) {
		if ( byteCount ) {			/* Copying a byte run which is doubly encoded */
			aData[i] = Encrypt(Encrypt(decryptedData[i], &r2), &r1);
			byteCount--;
		} else {
			aData[i] = Encrypt(decryptedData[i], &r1);
			if ( i >= 4 && (decryptedData[i-3] == ' ') && (decryptedData[i] == ' ') ) {
				uint8 c_2 = decryptedData[i-2];
				uint8 c_1 = decryptedData[i-1];
				if ( ((c_2 == 'R') && (c_1 == 'D')) || ((c_2 == '-') && (c_1 == '|')) ) {
					byteCount	= backwardsATOI( (char*)&decryptedData[i-4] );
					r2			= 4330;	/* charstring encryption has this initial R - p.64 Black Book */
				}
			}
		}
	}
	assert( byteCount == 0);
	assert( i == dataLen );
	return aData; /*****/
}
#endif

/* Searches [start, limit[ */
/* We might eventually want to use a Boyer-Moore Search for greater speed. ---Sampo */
static uint8 *tsi_T1Find( T1Class *t, const uint8 *param, long start, long limit )
{
	register size_t j, length;
	register int i;
	register uint8 c0, *result, *p = t->decryptedData;
	assert( t->decryptedData != NULL );
	
	
	result = NULL;
	assert( p != NULL );
	assert( param != NULL );
	length = (size_t)strlen( (const char *)param );
	c0 = param[0];
	for ( i = start; i < limit; i++ ) {
		if ( c0 == p[i] ) {
			int match = true;
			for ( j = 1; j < length; j++ ) {
				if ( p[i+j] != param[j] ) {
					match = false;
					break; /*****/
				}
			}
			if ( match ) {
				result = &p[i+j];
				break;/*****/
			}
		}
	}
	return result; /*****/
}


#define transform_decrypt 20
#define transform_encrypt 21
/*
 *
 */
static void TransformData( register T1Class *t, int tranform, uint8 *aData, long dataLen )
{
	uint8  *p;
	
	t->dataInPtr		= aData;
	t->decryptedData	= aData;
	t->dataLen			= dataLen;
#ifdef ROM_BASED_T1
	if ( tranform == transform_decrypt ) {
		assert( t->decryptedData != NULL );
		t->decryptedData	= (uint8 *)tsi_AllocMem(t->mem, (size_t)dataLen);
		t->dataLen			= dataLen;
		memcpy( t->decryptedData, aData, (size_t)dataLen );
	}
#endif
		
	t->eexecGO = dataLen;
	/* scan for (curre)ntfile eexec */
	p = tsi_T1Find( t, (uint8 *)"ntfile eexec", 0, t->eexecGO );
	assert( p != NULL );
	t->eexecGO = 0;
	if ( p != NULL ) {
		while ( *p == '\r' || *p == '\n' ) {
			p++;
		}
		t->eexecGO = p - t->decryptedData;
	}
	if ( tranform == transform_decrypt ) {
		if ( t->eexecGO > 0 ) {
			/* ExtractPureT1FromPCType1 eliminates the need for this PC specific hack */
#ifdef OLD
			t->eexecGO += 6; /* skip the 6 header bytes in the pfb data */
#endif
			t->dataLen = DecryptData( &t->decryptedData[t->eexecGO], dataLen - t->eexecGO ) + t->eexecGO;
		}
	}

}

/*	
 *  PSNameToAppleCode
 *  Description:  Takes an PostScript name and returns
 *                an Apple Character Code
 *  Input:	 Postscript Name
 *  Output:  Apple character Code, 0xFFFF if not found
 */
static uint16 PSNameToAppleCode( register uint8 *PSName, uint16 *adobeCode, uint16 *tableIdx )
{
	register unsigned int i;
	register const sfnt_CharToName *CharToNameTableP;

	*adobeCode = 0xffff;
	CharToNameTableP = sfnt_CharToNameTable;
	for ( i = 0; i < NumAppleCodes; i++ ) {
		if ( !strcmp(CharToNameTableP[i].name, (char *)PSName) ) {
			*adobeCode = CharToNameTableP[i].adobeCode;
			*tableIdx = (uint16)i;
			return( CharToNameTableP[i].appleCode ); /*****/
		}
	}
	return 0xffff; /*****/
}


static uint16 PSNameToCodeFromEncodingVector( T1Class *t, register uint8 *PSName, uint16 *encodingIdx )
{
	register uint8 *p;	
	register int i, j, count, length, match;
	uint16 aCode;
	
	count	= ATOI( t->encoding );
	length	= (int)strlen( (const char *)PSName );
	p		= t->encoding;

	for ( i = 0; i < count; i++ ) {
		/* dup 32 /space put */
		/* ... */
		/* dup 254 /bracerightbt put */
		p = tsi_T1Find( t, (uint8 *)"dup ", p - t->decryptedData, t->dataLen );
		if ( p == NULL ) {
			return 0xffff; /*****/
		}
		assert( p != NULL );
		aCode = (uint16)ATOI( p );
		for ( j = 0; *p != '/' && j < 32;  ) p++;
		p++; /* skip the '/' */
		if ( p[length] != ' ' ) continue; /*****/
		match = true;
		for ( j = 0; j < length; j++ ) {
			if ( PSName[j] != p[j] ) {
				match = false;
				break; /*****/
			}
		}
		if ( match ) {
			*encodingIdx = (uint16)i;
			return aCode; /*****/
		}

	}
	return 0xFFFF; /*****/
}

static uint8 *ppp;
static uint8 *GetNthNameFromEncodingVector( T1Class *t, int n )
{
	register uint8 *p;	
	register int i, j, count;
	uint16 aCode;
	
	ppp = NULL;
	count	= ATOI( t->encoding );
	p		= t->encoding;

	for ( i = 0; i < count; i++ ) {
		/* dup 32 /space put */
		/* ... */
		/* dup 254 /bracerightbt put */
		p = tsi_T1Find( t, (uint8 *)"dup ", p - t->decryptedData, t->dataLen );
		if ( p == NULL ) {
			return ppp; /*****/
		}
		assert( p != NULL );
		aCode = (uint16)ATOI( p );
		for ( j = 0; *p != '/' && j < 32;  ) p++;
		p++; /* skip the '/' */
		if ( i == n ) {
			return ppp = p; /*****/
		}

	}
	return NULL; /*****/
}



static void SetMetrics( T1Class *t, long lsbx, long lsby, long awx, long awy )
{
	t->lsbx = lsbx;
	t->lsby = lsby;
	t->awx 	= awx;
	t->awy 	= awy;
}


/*
 *
 */
static uint16 tsi_T1GetGlyphIndexFromAdobeCode( T1Class *t, uint16 charCode )
{
	long i, limit = t->NumCharStrings;
	uint16 *adobeCode = t->adobeCode;
	for ( i = 0; i < limit; i++ ) {
		if ( adobeCode[i] == charCode ) break; /*****/
	}
	return (uint16)i; /*****/
}

#define STACK_BOTTOM 0


/* CasloBooBEReg */


static void Type1BuildChar( T1Class *t, uint8 *p, int byteCount, int nestingLevel, FFT1HintClass *ffhint, long gppm)
{
	register int i, v1, v2;
	register long num;
	long x = t->x; /* cache x and y in local variables */
	long y = t->y;
#ifdef ENABLE_NATIVE_T1_HINTS
	long flexppm;
	long temp_x, temp_y;
	int16 temp;
	int iii;
	int bcount;
#endif
	for ( i = 0; i < byteCount; ) {
		v1 = p[i++];
		/* printf("%d : %d\n", i, (int)v1 ); */
/* LOG_CMD( "byte #", i ); 			*/
/* LOG_CMD( "byte v1", (int)v1 ); 	*/
		if ( v1 < 32 ) {
			/* A command */
/* LOG_CMD( "CMD v1", (int)v1 );	*/

			switch( v1 ) {
			case  0:
				break;
			case  1:	/* hstem */
				LOG_CMD( "hstem", t->gNumStackValues );
				assert( t->gNumStackValues >= 2 );

#ifdef ENABLE_NATIVE_T1_HINTS
				if (ffhint != NULL)
				{
					if ((ffhint->numyOrus + 2) >= ffhint->yOrus_num_ml)
					{
						ffhint->yOrus_num_ml = (int16)(ffhint->yOrus_num_ml + WIDTH_CHUNK);
					   	ffhint->yOrus_ptr = (int16 *)tsi_ReAllocMem( ffhint->mem, ffhint->yOrus_ptr , (ffhint->yOrus_num_ml) * sizeof(int16) );
						ffhint->ygcount_ml = (int16)(ffhint->ygcount_ml + WIDTH_CHUNK);
						ffhint->ygcount_ptr = (int16 *)tsi_ReAllocMem( ffhint->mem, ffhint->ygcount_ptr , (ffhint->ygcount_ml) * sizeof(int16) );
					}
	
					ffhint->yOrus_ptr[ffhint->numyOrus] = (int16)t->gStackValues[ STACK_BOTTOM + 0 ];
					ffhint->yOrus_ptr[ffhint->numyOrus] += (short)t->lsby;
					ffhint->ygcount_ptr[ffhint->numyOrus] = t->glyph->pointCount;
					ffhint->numyOrus = (short)(ffhint->numyOrus + 1);
					
					ffhint->yOrus_ptr[ffhint->numyOrus] = (int16)(ffhint->yOrus_ptr[ffhint->numyOrus - 1] + (int16)t->gStackValues[ STACK_BOTTOM + 1 ]);
					ffhint->ygcount_ptr[ffhint->numyOrus] = t->glyph->pointCount;
					if (ffhint->yOrus_ptr[ffhint->numyOrus] < ffhint->yOrus_ptr[ffhint->numyOrus - 1])
					{
						temp = ffhint->yOrus_ptr[ffhint->numyOrus - 1];
						ffhint->yOrus_ptr[ffhint->numyOrus - 1] = ffhint->yOrus_ptr[ffhint->numyOrus];
						ffhint->yOrus_ptr[ffhint->numyOrus] = temp;
					}
					ffhint->numyOrus = (short)(ffhint->numyOrus + 1);
				}
#endif
				t->gNumStackValues = 0; /* clear the stack */
				break;
			case  2:
				break;
			case  3:	/* vstem */
				LOG_CMD( "vstem", t->gNumStackValues );
				assert( t->gNumStackValues >= 2 );
#ifdef ENABLE_NATIVE_T1_HINTS
				if (ffhint != NULL)
				{
					if ((ffhint->numxOrus + 2) >= ffhint->xOrus_num_ml)
					{
						ffhint->xOrus_num_ml = (int16)(ffhint->xOrus_num_ml + WIDTH_CHUNK);
					   	ffhint->xOrus_ptr = (int16 *)tsi_ReAllocMem( ffhint->mem, ffhint->xOrus_ptr , (ffhint->xOrus_num_ml) * sizeof(int16) );
						ffhint->xgcount_ml = (int16)(ffhint->xgcount_ml + WIDTH_CHUNK);
						ffhint->xgcount_ptr = (int16 *)tsi_ReAllocMem( ffhint->mem, ffhint->xgcount_ptr , (ffhint->xgcount_ml) * sizeof(int16) );
					}
	
					ffhint->xOrus_ptr[ffhint->numxOrus] = (int16)t->gStackValues[ STACK_BOTTOM + 0 ];
					ffhint->xOrus_ptr[ffhint->numxOrus] += (short)t->lsbx;
					ffhint->xgcount_ptr[ffhint->numxOrus]  = t->glyph->pointCount;
					ffhint->numxOrus = (short)(ffhint->numxOrus + 1);
					ffhint->xOrus_ptr[ffhint->numxOrus] = (int16)(ffhint->xOrus_ptr[ffhint->numxOrus - 1] + (int16)t->gStackValues[ STACK_BOTTOM + 1 ]);
					ffhint->xgcount_ptr[ffhint->numxOrus] = t->glyph->pointCount;
					if (ffhint->xOrus_ptr[ffhint->numxOrus] < ffhint->xOrus_ptr[ffhint->numxOrus - 1])
					{
						temp = ffhint->xOrus_ptr[ffhint->numxOrus - 1];
						ffhint->xOrus_ptr[ffhint->numxOrus - 1] = ffhint->xOrus_ptr[ffhint->numxOrus];
						ffhint->xOrus_ptr[ffhint->numxOrus] = temp;
					}
					ffhint->numxOrus = (short)(ffhint->numxOrus + 1);
				}
#endif
				t->gNumStackValues = 0; /* clear the stack */
				break;
            case  4:        /* vmoveto */
                LOG_CMD( "vmoveto", t->gNumStackValues );
                assert( t->gNumStackValues >= 1 );
                y += t->gStackValues[ STACK_BOTTOM + 0 ];
                t->gNumStackValues = 0; /* clear the stack */
                if ( !t->flexOn ) glyph_CloseContour(t->glyph );
#if DEBUG
				if (debugOn)
				{
					printf("VMOVE	%d, %d\n", x, y);
				}
#endif
                break;
			case  5:	/* rlineto */
				LOG_CMD( "rlineto", t->gNumStackValues );
				assert( t->gNumStackValues >= 2 );
				glyph_StartLine( t->glyph, x, y );
				x += t->gStackValues[ STACK_BOTTOM + 0 ];
				y += t->gStackValues[ STACK_BOTTOM + 1 ];
				glyph_AddPoint( t->glyph, x, y, 1 );
				t->gNumStackValues = 0; /* clear the stack */
#if DEBUG
				if (debugOn)
				{
					printf("RLINE	%d, %d\n", x, y);
				}
#endif
				break;
			case  6:	/* hlineto */
				LOG_CMD( "hlineto", t->gNumStackValues );
				assert( t->gNumStackValues >= 1 );
				glyph_StartLine( t->glyph, x, y );
				x += t->gStackValues[ STACK_BOTTOM + 0 ];
				glyph_AddPoint( t->glyph, x, y, 1 );
				t->gNumStackValues = 0; /* clear the stack */
#if DEBUG
				if (debugOn)
				{
					printf("HLINE	%d, %d\n", x, y);
				}
#endif
				break;
			case  7:	/* vlineto */
				LOG_CMD( "vlineto", t->gNumStackValues );
				assert( t->gNumStackValues >= 1 );
				glyph_StartLine( t->glyph, x, y );
				y += t->gStackValues[ STACK_BOTTOM + 0 ];
				glyph_AddPoint( t->glyph, x, y, 1 );
				t->gNumStackValues = 0; /* clear the stack */
#if DEBUG
				if (debugOn)
				{
					printf("VLINE	%d, %d\n", x, y);
				}
#endif
				break;
			case  8:	/* rrcurveto */
				LOG_CMD( "rrcurveto", t->gNumStackValues );
				assert( t->gNumStackValues >= 6 );
				glyph_StartLine( t->glyph, x, y );
				x += t->gStackValues[ STACK_BOTTOM + 0 ];
				y += t->gStackValues[ STACK_BOTTOM + 1 ];
#if DEBUG
				if (debugOn)
				{
					printf("RRCURVE	%d, %d, ", x, y);
				}
#endif
				glyph_AddPoint( t->glyph, x, y, 0 );
				x += t->gStackValues[ STACK_BOTTOM + 2 ];
				y += t->gStackValues[ STACK_BOTTOM + 3 ];
#if DEBUG
				if (debugOn)
				{
					printf("%d, %d, ", x, y);
				}
#endif
				glyph_AddPoint( t->glyph, x, y, 0 );
				x += t->gStackValues[ STACK_BOTTOM + 4 ];
				y += t->gStackValues[ STACK_BOTTOM + 5 ];
#if DEBUG
				if (debugOn)
				{
					printf("%d, %d\n", x, y);
				}
#endif
				glyph_AddPoint( t->glyph, x, y, 1 );
				t->gNumStackValues = 0; /* clear the stack */
				break;
			case  9:	/* closepath */
				LOG_CMD( "closepath", t->gNumStackValues );
				glyph_CloseContour( t->glyph );
				t->gNumStackValues = 0; /* clear the stack */
#if DEBUG
				if (debugOn)
				{
					printf("CLOSEPATH\n");
				}
#endif
				break;
			case 10:	/* callsubr */
				LOG_CMD( "callsubr", t->gNumStackValues );
				assert( t->gNumStackValues >= 1 );
				t->gNumStackValues--;
				{
					int fnum = t->gStackValues[ t->gNumStackValues + 0 ]; /* topmost element */
					int fByteCount;
					
					if ( fnum >= 0 && fnum < t->numSubrs ) {
						fByteCount	= backwardsATOI( (char *)(t->subrsData[fnum]-5) );
LOG_CMD("***callsubr number =  ", fnum);			
LOG_CMD("***callsubr fByteCount = ", fByteCount);			
LOG_CMD("***BEGIN CALL = ", fnum );			
						/* Type1BuildChar( t, x, y, t->subrsData[fnum], fByteCount ); */
						fByteCount -= t->lenIV;
						if ( fByteCount > 0 && nestingLevel < 10 ) {
							t->x = x;
							t->y = y;
							Type1BuildChar( t, t->subrsData[fnum] + t->lenIV, fByteCount, nestingLevel+1, ffhint, gppm);
							x = t->x;
							y = t->y;
						}
LOG_CMD("***END CALL = ", fnum );			
					}
					assert( t->gNumStackValues >= 0 );
					assert( t->gNumStackValues <= kMaxStackValues );
				}
				/* Do not clear the stack */
				/* t->gNumStackValues = 0; */
				break;
			case 11:	/* return  (from callsubr) */
				LOG_CMD( "return", t->gNumStackValues );
				t->x = x;
				t->y = y;
				return; /*****/
				/* Do not clear the stack */
				/* break; */
			case 12:	/* escape */
				v2 = p[i++];
				switch( v2 ) {
					case 0:	 	/* dotsections */
						LOG_CMD( "dotsections", t->gNumStackValues );
						t->gNumStackValues = 0; /* clear the stack */
						break;
					case 1: 	/* vstem3 */
						LOG_CMD( "vstem3", t->gNumStackValues );
						assert( t->gNumStackValues >= 6 );
#ifdef ENABLE_NATIVE_T1_HINTS
						if (ffhint != NULL)
						{

							if ((ffhint->numxOrus + 6) >= ffhint->xOrus_num_ml)
							{
								ffhint->xOrus_num_ml = (int16)(ffhint->xOrus_num_ml + WIDTH_CHUNK);
					   			ffhint->xOrus_ptr = (int16 *)tsi_ReAllocMem( ffhint->mem, ffhint->xOrus_ptr , (ffhint->xOrus_num_ml) * sizeof(int16) );
								ffhint->xgcount_ml = (int16)(ffhint->xgcount_ml + WIDTH_CHUNK);
								ffhint->xgcount_ptr = (int16 *)tsi_ReAllocMem( ffhint->mem, ffhint->xgcount_ptr , (ffhint->xgcount_ml) * sizeof(int16) );
							}


							for (iii = 0; iii < 3; iii++)
							{
								ffhint->xOrus_ptr[ffhint->numxOrus] = (int16)t->gStackValues[ STACK_BOTTOM + (iii * 2) ];
								ffhint->xOrus_ptr[ffhint->numxOrus] += (short)t->lsbx;
								ffhint->xgcount_ptr[ffhint->numxOrus] = t->glyph->pointCount;
								ffhint->numxOrus = (short)(ffhint->numxOrus + 1);
								ffhint->xOrus_ptr[ffhint->numxOrus] = (int16)(ffhint->xOrus_ptr[ffhint->numxOrus - 1] + (int16)t->gStackValues[ STACK_BOTTOM + 1 + (iii * 2) ]);
								ffhint->xgcount_ptr[ffhint->numxOrus] = t->glyph->pointCount;
								if (ffhint->xOrus_ptr[ffhint->numxOrus] < ffhint->xOrus_ptr[ffhint->numxOrus - 1])
								{
									temp = ffhint->xOrus_ptr[ffhint->numxOrus - 1];
									ffhint->xOrus_ptr[ffhint->numxOrus - 1] = ffhint->xOrus_ptr[ffhint->numxOrus];
									ffhint->xOrus_ptr[ffhint->numxOrus] = temp;
								}
								ffhint->numxOrus = (short)(ffhint->numxOrus + 1);
							}
						}
#endif
						t->gNumStackValues = 0; /* clear the stack */
						break;
					case 2: 	/* hstem3 */
						LOG_CMD( "hstem3", t->gNumStackValues );
						assert( t->gNumStackValues >= 6 );

#ifdef ENABLE_NATIVE_T1_HINTS
						if (ffhint != NULL)
						{
							if ((ffhint->numyOrus + 6) >= ffhint->yOrus_num_ml)
							{
								ffhint->yOrus_num_ml = (int16)(ffhint->yOrus_num_ml + WIDTH_CHUNK);
					   			ffhint->yOrus_ptr = (int16 *)tsi_ReAllocMem( ffhint->mem, ffhint->yOrus_ptr , (ffhint->yOrus_num_ml) * sizeof(int16) );
								ffhint->ygcount_ml = (int16)(ffhint->ygcount_ml + WIDTH_CHUNK);
								ffhint->ygcount_ptr = (int16 *)tsi_ReAllocMem( ffhint->mem, ffhint->ygcount_ptr , (ffhint->ygcount_ml) * sizeof(int16) );
							}

							for (iii = 0; iii < 3; iii++)
							{
								ffhint->yOrus_ptr[ffhint->numyOrus] = (int16)t->gStackValues[ STACK_BOTTOM + (iii * 2) ];
								ffhint->yOrus_ptr[ffhint->numyOrus] += (short)t->lsby;
								ffhint->ygcount_ptr[ffhint->numyOrus] = t->glyph->pointCount;
								ffhint->numyOrus = (short)(ffhint->numyOrus + 1);
								ffhint->yOrus_ptr[ffhint->numyOrus] = (int16)(ffhint->yOrus_ptr[ffhint->numyOrus - 1] + (int16)t->gStackValues[ STACK_BOTTOM + 1  + (iii * 2)]);
								ffhint->ygcount_ptr[ffhint->numyOrus] = t->glyph->pointCount;
								if (ffhint->yOrus_ptr[ffhint->numyOrus] < ffhint->yOrus_ptr[ffhint->numyOrus - 1])
								{
									temp = ffhint->yOrus_ptr[ffhint->numyOrus - 1];
									ffhint->yOrus_ptr[ffhint->numyOrus - 1]= ffhint->yOrus_ptr[ffhint->numyOrus];
									ffhint->yOrus_ptr[ffhint->numyOrus] = temp;
								}
								ffhint->numyOrus = (short)(ffhint->numyOrus + 1);
							}
						}
#endif
						t->gNumStackValues = 0; /* clear the stack */
						break;
					case 6: 	/* seac (standard encoding accented character) */
						{
							long /* asb, */ adx, ady, leftSideBearing;

							uint16 bchar, achar;
							uint16 flags;
							GlyphClass *glyph = t->glyph, *glyph2;
							uint16 aw, ah;


							LOG_CMD( "seac", t->gNumStackValues );
							assert( t->gNumStackValues >= 5 );
							/* asb = t->gStackValues[ STACK_BOTTOM + 0 ]; */
							adx = t->gStackValues[ STACK_BOTTOM + 1 ]; /* The offset of the left side bearing points */
							ady = t->gStackValues[ STACK_BOTTOM + 2 ];
							bchar = (uint16)t->gStackValues[ STACK_BOTTOM + 3 ];
							achar = (uint16)t->gStackValues[ STACK_BOTTOM + 4 ];
							
							LOG_CMD( "achar code: ", (int)achar );
							LOG_CMD( "bchar code: ", (int)bchar );
							/* map to glyph index */
							achar = tsi_T1GetGlyphIndexFromAdobeCode( t, achar );
							bchar = tsi_T1GetGlyphIndexFromAdobeCode( t, bchar );
							LOG_CMD( "achar index: ", (int)achar );
							LOG_CMD( "bchar index: ", (int)bchar );

#if 1
							leftSideBearing = 0;
							if (achar < t->NumCharStrings)
							{
								glyph2 = tsi_T1GetGlyphByIndex( t, achar, &aw, &ah, NULL, -1);
								if (glyph2)
								{
									leftSideBearing = t->lsbx;
									Delete_GlyphClass( glyph2 );
								}
							}
							if (bchar < t->NumCharStrings)
							{
								glyph2 = tsi_T1GetGlyphByIndex( t, bchar, &aw, &ah, NULL, -1);
								if (glyph2)
								{
									leftSideBearing -= t->lsbx;
									Delete_GlyphClass( glyph2 );
								}
							/*	else
									leftSideBearing -= 0; */
							}
						/*	else
								leftSideBearing -= 0;	*/
							t->glyph = glyph;
#else
							leftSideBearing  = achar < t->NumCharStrings ? t->noDelete_hmtx->lsb[achar] : 0; /* Added 7/17/98 ---Sampo */
							leftSideBearing -= bchar < t->NumCharStrings ? t->noDelete_hmtx->lsb[bchar] : 0;
#endif
					 		glyph->componentSizeMax	= 64;
					 		glyph->componentData	= (short*) tsi_AllocMem( t->mem, glyph->componentSizeMax * sizeof(short) );
					 		glyph->componentSize	= 0;
							flags = 0;
							flags |= ARG_1_AND_2_ARE_WORDS;
							flags |= MORE_COMPONENTS;
							flags |= (ARGS_ARE_XY_VALUES | ROUND_XY_TO_GRID);
							glyph->componentData[ glyph->componentSize++] = (short)flags;
							glyph->componentData[ glyph->componentSize++] = (short)bchar; /* base charcode not glyph index */
							glyph->componentData[ glyph->componentSize++] = 0;
							glyph->componentData[ glyph->componentSize++] = 0;
							flags = 0;
							flags |= ARG_1_AND_2_ARE_WORDS;
							flags |= (ARGS_ARE_XY_VALUES | ROUND_XY_TO_GRID);
							glyph->componentData[ glyph->componentSize++] = (short)flags;
							glyph->componentData[ glyph->componentSize++] = (short)achar; /* accent charcode not glyph index */
							glyph->componentData[ glyph->componentSize++] = (short)(adx - leftSideBearing); /* 7/17/98 changed from (short)adx since lsb impact this */
							glyph->componentData[ glyph->componentSize++] = (short)ady;
							
							glyph->contourCount = -1;
						}
						t->gNumStackValues = 0; /* clear the stack */
						break;
					case 7: 	/* sbw */
						{
							long lsbx, lsby, awx, awy;
							LOG_CMD( "sbw", t->gNumStackValues );
							assert( t->gNumStackValues >= 4 );
							lsbx = t->gStackValues[ STACK_BOTTOM + 0 ]; 
							lsby = t->gStackValues[ STACK_BOTTOM + 1 ]; 
							awx  = t->gStackValues[ STACK_BOTTOM + 2 ]; 
							awy  = t->gStackValues[ STACK_BOTTOM + 3 ];
							x = lsbx;
							y = lsby;
							SetMetrics( t, lsbx, lsby, awx, awy );
						}
						t->gNumStackValues = 0; /* clear the stack */
						break;
					case 12: 	/* div */
						LOG_CMD( "div", t->gNumStackValues );
						/* Warning this is different from Scott's old code ---Sampo */
						/* We also simulate div with an integer idiv operation */
						assert( t->gNumStackValues >= 2 );
						t->gStackValues[t->gNumStackValues-2] /= t->gStackValues[t->gNumStackValues-1];
						t->gNumStackValues--;
						/* Do not clear the stack */
						break;
					case 16: 	/* callothersubr */
						{
							long otherNumber, PS_StackArgCount;
							LOG_CMD( "callothersubr", t->gNumStackValues );

	LOG_CMD("stack-top+1 = ", t->gStackValues[ t->gNumStackValues -0 ] );			
	LOG_CMD("stack-top-0 = ", t->gStackValues[ t->gNumStackValues -1 ] );			
	LOG_CMD("stack-top-1 = ", t->gStackValues[ t->gNumStackValues -2 ] );			
	LOG_CMD("stack-top-2 = ", t->gStackValues[ t->gNumStackValues -3 ] );
	LOG_CMD("stack-top-3 = ", t->gStackValues[ t->gNumStackValues -4 ] );
	LOG_CMD("stack-top-4 = ", t->gStackValues[ t->gNumStackValues -5 ] );

							t->gNumStackValues--;
							otherNumber = t->gStackValues[ t->gNumStackValues ]; /* arg1, arg2, # */
							t->gNumStackValues--;
							PS_StackArgCount = t->gStackValues[ t->gNumStackValues ];
							switch ( otherNumber ) {
								
							case 0:
								/* end flex */
								t->flexOn = false;
							
#ifdef ENABLE_NATIVE_T1_HINTS
								flexppm = t->gStackValues[ STACK_BOTTOM + 0 ]; 
#endif							
								t->gNumStackValues = (short)(t->gNumStackValues - 2); /* ppem, x, y */
#ifdef ENABLE_NATIVE_T1_HINTS
								if ((gppm < flexppm) && (ffhint != NULL))
								{
									temp_x = t->glyph->oox[t->glyph->pointCount - 1];
									temp_y = t->glyph->ooy[t->glyph->pointCount - 1];
									t->glyph->pointCount = (short)(t->glyph->pointCount - 6);
									glyph_AddPoint( t->glyph, temp_x, temp_y, 1 );	/* Ask about final parameter */
								}
#endif
								break;/*****/
							case 1:
								/* start flex */
								t->flexOn = true;
								t->flexCount = 0;
								glyph_StartLine( t->glyph, x, y );
								break;/*****/
							case 2:
								if ( t->flexOn ) {
									switch( t->flexCount ) {
									case 0:
										/* The flat mid-point, flex-dist = dist(point(0)-point(3))*/
										break; /*****/
									case 1:
										glyph_AddPoint( t->glyph, x, y, 0 );
										break; /*****/
									case 2:
										glyph_AddPoint( t->glyph, x, y, 0 );
										break; /*****/
									case 3:
										glyph_AddPoint( t->glyph, x, y, 1 );
										break; /*****/
									case 4:
										glyph_AddPoint( t->glyph, x, y, 0 );
										break; /*****/
									case 5:
										glyph_AddPoint( t->glyph, x, y, 0 );
										break; /*****/
									case 6:
										glyph_AddPoint( t->glyph, x, y, 1 );
										break; /*****/
									default:
										assert( false );
										break; /*****/
									}
									t->flexCount++;
								}
								break;/*****/
							case 3:
								/* Hint replacement mechanism */
								/* t->gNumStackValues--; */

								/* This is to reset if original block of hints will be */
								/* superceeded with hint replacement sets */

#ifdef ENABLE_NATIVE_T1_HINTS
								if (ffhint != NULL)
								{
									if ((t->glyph->pointCount == ffhint->ygcount_ptr[ffhint->numyOrus - 1]) ||
										(t->glyph->pointCount == ffhint->xgcount_ptr[ffhint->numxOrus - 1]))
									{
										bcount = ffhint->numxOrus - 1;
										while ((ffhint->xgcount_ptr[bcount] == t->glyph->pointCount) &&
											   (bcount > 0))
											bcount--;
										if (bcount > 0)
											ffhint->numxOrus = (int16)(bcount+1);
										else
											ffhint->numxOrus = 0;
										
									   bcount = ffhint->numyOrus - 1;
										while ((ffhint->ygcount_ptr[bcount] == t->glyph->pointCount) &&
											   (bcount > 0))
											bcount--;
										if (bcount > 0)
											ffhint->numyOrus = (int16)(bcount+1);
										else
											ffhint->numyOrus = 0;
									}
	
								}
#endif
								t->gNumStackValues = (short)(t->gNumStackValues - PS_StackArgCount);
								/* assert( PS_StackArgCount == 1 ); */
								break;/*****/
							/* 12 Counter Control 12, clear stack */
							/* 13 Counter Control 13, clear stack */
							case 12:
							case 13:
								t->gNumStackValues = 0;
								break;/*****/
							/* 14-18 relates to MM fonts */
							case 14: /* Maps numMasters * 1 values to 1 value */
							case 15: /* Maps numMasters * 2 values to 2 values */
							case 16: /* Maps numMasters * 3 values to 3 values */
							case 17: /* Maps numMasters * 4 values to 4 values */
							case 18: /* Maps numMasters * 6 values to 6 values */
								{
									long dNum, *aData, N, loop;
									F16Dot16 result;
									
									N = otherNumber == 18 ? 6 : otherNumber - 13; /* N = [1,2,3,4,6] */
									
									assert( PS_StackArgCount == t->numMasters * N );
									t->gNumStackValues = (short)(t->gNumStackValues - t->numMasters * N);
									assert( t->gNumStackValues >= 0 );
									
									aData = &t->gStackValues[ t->gNumStackValues + N ];
									for ( loop = 0; loop < N; loop++ ) {
										result = t->gStackValues[ t->gNumStackValues ];
										for ( dNum = 1; dNum < t->numMasters; dNum++ ) {
											result += util_FixMul(*aData++, t->WeightVector[dNum] );
										}
										t->gStackValues[ t->gNumStackValues++ ] = result;
									}
									t->gNumStackValues = (short)(t->gNumStackValues - N);
								}
								break;/*****/
							default:
								/* UnKnown */
								t->gNumStackValues = (short)(t->gNumStackValues - PS_StackArgCount);
LOG_CMD("* * * * * UnKnown callothersubr number = ", otherNumber );			
								assert( false );
								break;/*****/
							}
							assert( t->gNumStackValues >= 0 );
							if ( t->gNumStackValues < 0 ) t->gNumStackValues = 0;
LOG_CMD("* * * * * callothersubr number = ", otherNumber );			
LOG_CMD("* * * * * x = ", x );			
LOG_CMD("* * * * * y = ", y );			
						/* calls the PS interpreter (!) */
						}
						/* Do not clear the stack */
/*t->gNumStackValues = 0; */
						break;
					case 17: 	/* pop */
						LOG_CMD( "pop", t->gNumStackValues );
						/* From the PS interpreter */
#ifdef OLD
						t->gStackValues[ t->gNumStackValues ] = 3;
#endif
						t->gNumStackValues++; /* just pass on arguments from callothersubr */
						/* Do not clear the stack */
						break;
					case 33: 	/* setcurrentpoint */
						LOG_CMD( "setcurrentpoint", t->gNumStackValues );
						assert( t->gNumStackValues >= 2 );
						/*
						...Only used with othersubr, => skip it...
						x = t->gStackValues[ STACK_BOTTOM + 0 ];
						y = t->gStackValues[ STACK_BOTTOM + 1 ];
						*/
						glyph_StartLine( t->glyph, x, y );
						t->gNumStackValues = 0; /* clear the stack */
#if DEBUG
						if (debugOn)
						{
							printf("SETCURRENT	%d, %d\n", x, y);
						}
#endif
						break;
					default:
						break;
				}
				break;
			case 13:	/* hsbw */
				{
					long lsbx, lsby, awx, awy;
					LOG_CMD( "hsbw", t->gNumStackValues );
					assert( t->gNumStackValues >= 2 );
					lsbx = t->gStackValues[ STACK_BOTTOM + 0 ]; 
					lsby = 0; 
					awx  = t->gStackValues[ STACK_BOTTOM + 1 ]; 
					awy  = 0;
					x = lsbx;
					y = lsby;
					SetMetrics( t, lsbx, lsby, awx, awy );
					t->gNumStackValues = 0; /* clear the stack */
				}
				break;
			case 14:	/* endchar */
				/* endchar is the last command for normal characters and seac is the last one for accented characters */
				LOG_CMD( "endchar", t->gNumStackValues );
				t->gNumStackValues = 0; /* clear the stack */
#if DEBUG
				if (debugOn)
				{
					printf("ENDCHAR\n");
				}
#endif
				break;
			case 15:
				break;
			case 16:
				break;
			case 17:
				break;
			case 18:
				break;
			case 19:
				break;
			case 20:
				break;
            case 21:        /* rmoveto */
                LOG_CMD( "rmoveto", t->gNumStackValues );
                assert( t->gNumStackValues >= 2 );
                x += t->gStackValues[ STACK_BOTTOM + 0 ];
                y += t->gStackValues[ STACK_BOTTOM + 1 ];
                t->gNumStackValues = 0; /* clear the stack */
                if ( !t->flexOn ) glyph_CloseContour(t->glyph );
#if DEBUG
				if (debugOn)
				{
					printf("RMOVE	%d, %d\n", x, y);
				}
#endif
                break;
             case 22:        /* hmoveto */
                LOG_CMD( "hmoveto", t->gNumStackValues );
                assert( t->gNumStackValues >= 1 );
                x += t->gStackValues[ STACK_BOTTOM + 0 ];
                t->gNumStackValues = 0; /* clear the stack */
                if ( !t->flexOn ) glyph_CloseContour(t->glyph );
#if DEBUG
				if (debugOn)
				{
					printf("HMOVE	%d, %d\n", x, y);
				}
#endif
				break;
			case 23:
				break;
			case 24:
				break;
			case 25:
				break;
			case 26:
				break;
			case 27:
				break;
			case 28:
				break;
			case 29:
				break;
			case 30:	/* vhcurveto */
				LOG_CMD( "vhcurveto", t->gNumStackValues );
				assert( t->gNumStackValues >= 4 );
				glyph_StartLine( t->glyph, x, y );
				x += 0;
				y += t->gStackValues[ STACK_BOTTOM + 0 ];
#if DEBUG
				if (debugOn)
				{
					printf("VHCURVE	%d, %d, ", x, y);
				}
#endif
				glyph_AddPoint( t->glyph, x, y, 0 );
				x += t->gStackValues[ STACK_BOTTOM + 1 ];
				y += t->gStackValues[ STACK_BOTTOM + 2 ];
#if DEBUG
				if (debugOn)
				{
					printf("%d, %d, ", x, y);
				}
#endif
				glyph_AddPoint( t->glyph, x, y, 0 );
				x += t->gStackValues[ STACK_BOTTOM + 3 ];
				y += 0;
#if DEBUG
				if (debugOn)
				{
					printf("%d, %d\n", x, y);
				}
#endif
				glyph_AddPoint( t->glyph, x, y, 1 );
				t->gNumStackValues = 0; /* clear the stack */
				break;
			case 31:	/* hvcurveto */
				LOG_CMD( "hvcurveto", t->gNumStackValues );
				assert( t->gNumStackValues >= 4 );
				glyph_StartLine( t->glyph, x, y );
				x += t->gStackValues[ STACK_BOTTOM + 0 ];
				y += 0;
#if DEBUG
				if (debugOn)
				{
					printf("HVCURVE	%d, %d, ", x, y);
				}
#endif
				glyph_AddPoint( t->glyph, x, y, 0 );
				x += t->gStackValues[ STACK_BOTTOM + 1 ];
				y += t->gStackValues[ STACK_BOTTOM + 2 ];
#if DEBUG
				if (debugOn)
				{
					printf("%d, %d, ", x, y);
				}
#endif
				glyph_AddPoint( t->glyph, x, y, 0 );
				x += 0;
				y += t->gStackValues[ STACK_BOTTOM + 3 ];
#if DEBUG
				if (debugOn)
				{
					printf("%d, %d\n", x, y);
				}
#endif
				glyph_AddPoint( t->glyph, x, y, 1 );
				t->gNumStackValues = 0; /* clear the stack */
				break;
			}
		} else {
			/* >= 32 => A number */
			if ( v1 <= 246 ) {
				num = v1 - 139; 					/* 1 byte: [-107, +107] */
			} else if ( v1 <= 250 ) {
				v2 = p[i++];
				num = ((v1-247)*256) + v2 + 108;	/* 2 bytes: [108, 1131] */
			} else if ( v1 <= 254 ) {
				v2 = p[i++];
				num = -((v1-251)*256) - v2 - 108;	/* 2 bytes: [-108, -1131] */
			} else {
				/* v1 == 255 */					 	/* 5 bytes: +-32 bit signed number  */
				num = p[i++];
				num <<= 8;
				num |= p[i++];
				num <<= 8;
				num |= p[i++];
				num <<= 8;
				num |= p[i++];
			}
#ifdef OLDDBUG
if ( debugOn ) {
	printf("stack: ");
}
#endif			
			if ( t->gNumStackValues < kMaxStackValues ) {
				t->gStackValues[t->gNumStackValues++] = num;
			}
#ifdef OLDDBUG
if ( debugOn ) {
	int k;
	
	for ( k = 0; k < t->gNumStackValues; k++ ) {
		printf(" %d ", t->gStackValues[k] );
	}
	printf("\n");
}
#endif			

		}
	}
	LOG_CMD( "RETURNING FROM Type1BuildChar", t->gNumStackValues );
	t->x = x;
	t->y = y;

}


/*
 *
 */
uint16 tsi_T1GetGlyphIndex( T1Class *t, register uint16 charCode )
{
	long i, limit = t->NumCharStrings;
	register uint16 *charCodePtr = t->charCode;
	
	for ( i = 0; i < limit; i++ ) {
		if ( charCodePtr[i] == charCode ) break; /*****/
	}
	t->glyphExists = (i < limit);
	return (uint16)i; /*****/
}



/*
 *
 */
GlyphClass *tsi_T1GetGlyphByIndex( T1Class *t, uint16 index, uint16 *aWidth, uint16 *aHeight, FFT1HintClass *ffhint, long PPEm)
{
	int byteCount, limit = t->NumCharStrings;
	uint8 *p;
	GlyphClass *glyph;
	
	t->glyph = New_EmptyGlyph( t->mem, 0, 0, 0, 0 );
	t->glyph->curveType = 3;
	t->gNumStackValues = 0;

	if ( (uint16)index < limit ) {
		p = t->charData[index];
		if ( p != NULL ) {
			byteCount = backwardsATOI( (char *)( p-5 ) );
			t->x = 0;
			t->y = 0;
			t->flexOn = false;
			LOG_CMD( "tsi_T1GetGlyphByIndex:", index );

			if (ffhint != NULL)
			{
				Type1BuildChar( t, p + t->lenIV, byteCount - t->lenIV, 0, ffhint, PPEm);
			}
			else
				Type1BuildChar( t, p + t->lenIV, byteCount - t->lenIV, 0, 0, 0);

			glyph_CloseContour( t->glyph );
		}
	}
	glyph = t->glyph;
	
	glyph->ooy[glyph->pointCount + 0] = 0;
	glyph->oox[glyph->pointCount + 0] = 0;
	
	glyph->ooy[glyph->pointCount + 1] = (short)t->awy;
	glyph->oox[glyph->pointCount + 1] = (short)t->awx;
	
	*aWidth = (uint16)t->awx;
	
	{
		uint16 ah;

#		if SbPtCount >= 4
		{
			long xMid = (glyph->oox[glyph->pointCount + 0] + glyph->oox[glyph->pointCount + 1]) >> 1;
			int16 tsb, ymax, *ooy = glyph->ooy;
			int i, limit = glyph->pointCount;
			
			ymax = ooy[0];
			for ( i = 1; i < limit; i++ ) {
				if ( ooy[i] > ymax ) ymax = ooy[i];
			}
			/* Simulate */
			ah 	= (uint16)t->upem;
			tsb = (int16)(ah/10);
			
			ooy[glyph->pointCount + 2] 			= (short)(ymax + tsb);
			glyph->oox[glyph->pointCount + 2]	= (short)xMid;
			
			ooy[glyph->pointCount + 3]			= (short)(ooy[glyph->pointCount + 2] - ah);
			glyph->oox[glyph->pointCount + 3]	= (short)xMid;
		}
#		else
		ah = 0;
#		endif
		*aHeight = ah;
	}	
	t->glyph = NULL;
	FlipContourDirection( glyph );
	return glyph; /*****/
}



/*
 *
 */
long tsi_T1GetParam( T1Class *t, const uint8 *param, long defaultValue )
{
	register uint8 *start;
	long result = defaultValue;

	start = tsi_T1Find( t, param, 0, t->charstringsGO );
	
	if ( start != 0 ) {
		result = ATOI( start );
	}
	return result; /*****/
}





static long tsi_T1GetArray( T1Class *t, const uint8 *param, int maxDim, long *arr )
{
	register uint8 *start, *p;
	long count, limit, numDimension = 0;

	start = tsi_T1Find( t, param, 0, t->charstringsGO );
	
	/* /WeightVector [0.17 0.08 0.52 0.23 ] def */

	if ( (p = start) != NULL ) {
		count = 0; limit = 32;
		for ( ; *p != '[' && count < limit; p++ ) count++; 	p++; 	/* skip the '[' */
		limit = 512;
		for ( numDimension = 0; numDimension < maxDim; ) {
			for ( ; *p == ' ' && count < limit; p++ )
				count++; 	/* skip spaces */
			if ( *p == ']' )
				break; /***** We are Done *****/
			arr[numDimension++] = ATOI( p );
			for ( ; *p != ' ' && count < limit; p++ )
			{
				count++; 	/* skip the number */
				if (*p == ']')
					break;
			}
		}
	}
	return numDimension; /*****/
}




/*
 *
 */
long tsi_T1GetBoolParam( T1Class *t, const uint8 *param, long defaultValue )
{
	register uint8 *start;
	long result = defaultValue;

	start = tsi_T1Find( t, param, 0, t->charstringsGO );
	
	if ( start != 0 ) {
		result = ( start[0] == 't' );
	}
	return result; /*****/
}

/*
 *
 */
F16Dot16 tsi_T1GetFixedParam( T1Class *t, const uint8 *param, F16Dot16 defaultValue )
{
	register uint8 *start;
	F16Dot16 result = defaultValue;

	start = tsi_T1Find( t, param, 0, t->charstringsGO );
	
	if ( start != 0 ) {
		result = ATOFixed( start, 0 );
	}
	return result; /*****/
}

/*
 * returns numDimension
 */
static F16Dot16 tsi_T1GetFixedArray( T1Class *t, const uint8 *param, int maxDim, F16Dot16 *arr )
{
	register uint8 *start, *p;
	long count, limit, numDimension = 0;

	start = tsi_T1Find( t, param, 0, t->charstringsGO );
	
	/* /WeightVector [0.17 0.08 0.52 0.23 ] def */

	if ( (p = start) != NULL ) {
		count = 0; limit = 32;
		for ( ; *p != '[' && count < limit; p++ ) count++; 	p++; 	/* skip the '[' */
		limit = 512;
		for ( numDimension = 0; numDimension < maxDim; ) {
			for ( ; *p == ' ' && count < limit; p++ ) count++; 	/* skip spaces */
			if ( *p == ']' ) break; /***** We are Done *****/
			arr[numDimension++] = ATOFixed( p, 0 );
			for ( ; *p != ' ' && count < limit; p++ ) count++; 	/* skip the number */
		}
	}
	return numDimension; /*****/
}


/*
 *
 */
static void GetT1FontMatrix( T1Class *t )
{
	register uint8 *start, *p;
	long count, limit;
	
	t->m00 = ONE16Dot16;    t->m01 = 0;                             /* Oblique */
	t->m10 = 0;                             t->m11 = ONE16Dot16;
	
	
	   /* LCroft 16-Oct-00: fixed font matrix initialization */
	
	/* /FontMatrix [0.001000 0 .185339E-3 0.001 0 0] readonly def */
	/* /FontMatrix [0.001 0 0 0.001 0 0] readonly def */
	start = tsi_T1Find( t, (const uint8 *)"/FontMatrix ", 0, t->charstringsGO );
	if ( (p = start) != NULL ) {
		count = 0; limit = 256;
		
		for ( ; *p != '[' && count < limit; p++ ) count++;      p++; /* skip the '[' */
		for ( ; *p == ' ' && count < limit; p++ ) count++;      /* skip spaces */
		/* now we point at number 1 */
		if ( count < limit ) {
			t->m00 = ATOFixed( p, 3 ); /* Scale up by a factor of 10^3 == 1000 */
		}
		
		for ( ; *p != ' ' && count < limit; p++ ) count++;      /* skip number 1 */
		for ( ; *p == ' ' && count < limit; p++ ) count++;      /* skip spaces */
		/* now we point at number 2 */
		if ( count < limit ) {
			t->m10 = ATOFixed( p, 3 ); /* Scale up by a factor of 10^3 == 1000 */
		}
		
		for ( ; *p != ' ' && count < limit; p++ ) count++;      /* skip number 2 */
		for ( ; *p == ' ' && count < limit; p++ ) count++;      /* skip spaces */
		/* now we point at number 3 */
		if ( count < limit ) {
			t->m01 = ATOFixed( p, 3 ); /* Scale up by a factor of 10^3 == 1000 */
		}
		
		for ( ; *p != ' ' && count < limit; p++ ) count++;      /* skip number 3 */
		for ( ; *p == ' ' && count < limit; p++ ) count++;      /* skip spaces */
		/* now we point at number 4 */
		if ( count < limit ) {
			t->m11 = ATOFixed( p, 3 ); /* Scale up by a factor of 10^3 == 1000 */
		}
	}
}


uint8 *GetT1NameProperty( T1Class *t, uint16 languageID, uint16 nameID )
{
	register uint8 *start;
	long i, length;
	uint8 *key = NULL, *result = NULL;

	/* Examples: */
	/* /FullName (ITC Avant Garde Gothic Book Oblique) readonly def */
	/* /FontName /AvantGarde-BookOblique def */
	/* /FullName (Times Roman) readonly def */
	/* /FontName /Times-Roman def */
	UNUSED(languageID);
	if ( nameID == 4 ) {
		key = (uint8 *)"/FullName";
	}
	if ( key != NULL ) {
		start = tsi_T1Find( t, key, 0, t->charstringsGO );
		if ( start != 0 ) {
			for ( i = 0; i < 3; i++ ) {
				if ( *start != '(' ) start++;
			}
			if ( *start == '(' ) {
				start++;
				for ( length = i = 0; start[i] != ')' && i < 80; i++ ) {
					length++;
				}
				result = (uint8 *)tsi_AllocMem( t->mem, (unsigned long)(length + 1) );
				for ( i = 0; i < length; i++ ) {
					result[i] = start[i];
				}
				result[i] = 0; /* NULL terminate */
			}
		}
	}
	return result; /*****/
}

static void BuildMetricsEtc( T1Class *t )
{
	uint16 gIndex;
	GlyphClass *glyph;
	uint16 aw, ah;
	
	t->numAxes	  = 0;
	t->numMasters = tsi_T1GetFixedArray( t, (unsigned char *)"/WeightVector", T1_MAX_MASTERS, t->WeightVector );

	t->numBlueValues = (short)tsi_T1GetArray( t, (unsigned char *)"/BlueValues", T1_MAX_BLUEVALUES, (long *)t->BlueValues);

	t->BlueFuzz = (short)tsi_T1GetParam( t, (uint8 *)"/BlueFuzz ", 1 );

	t->BlueScale = tsi_T1GetFixedParam( t, (uint8 *)"/BlueScale ", 0xb35 );

	t->BlueShift = (short)tsi_T1GetParam( t, (uint8 *)"/BlueShift ", 7 );


	t->numStemSnapH = (short)tsi_T1GetArray( t, (unsigned char *)"/StemSnapH", T1_MAX_SNAPS, (long *)t->StemSnapH);
	t->numStemSnapV = (short)tsi_T1GetArray( t, (unsigned char *)"/StemSnapV", T1_MAX_SNAPS, (long *)t->StemSnapV);

	t->StdHW = (short)tsi_T1GetParam( t, (uint8 *)"/StdHW ", 0 );
	t->StdVW = (short)tsi_T1GetParam( t, (uint8 *)"/StdVW ", 0 );



#ifdef OLD
	if ( t->numMasters != 0 ) {
		int i;
		for ( i = 0; i < t->numMasters; i++ ) {
			printf("WeightVector[%d] = %f\n", i, (double)t->WeightVector[i]/65536.0 );
		}
	}
#endif

	t->upem = tsi_T1GetParam( t, (uint8 *)"/em ", 1000 );
	t->maxPointCount = 0;
	t->ascent = tsi_T1GetParam( t, (uint8 *)"/ascent ", 0x7fff );
	t->descent = -tsi_T1GetParam( t, (uint8 *)"/descent ", -0x7fff );
	t->UnderlinePosition = tsi_T1GetParam( t, (uint8 *)"/UnderlinePosition ", 0 );
	t->UnderlineThickness = tsi_T1GetParam( t, (uint8 *)"/UnderlineThickness ", 0 );

	GetT1FontMatrix( t );
	t->italicAngle = tsi_T1GetFixedParam( t, (uint8 *)"/ItalicAngle ", 0 );
	t->isFixedPitch = (uint32)tsi_T1GetBoolParam( t, (uint8 *)"/isFixedPitch ", 0x0000 );;
	
	if ( t->ascent == 0x7fff )
	{
		gIndex = tsi_T1GetGlyphIndex( t, 'f' );
		if (gIndex)
		{
			glyph = tsi_T1GetGlyphByIndex( t, gIndex, &aw, &ah, NULL, -1);
			if (glyph)
			{
				t->ascent = GetGlyphYMax( glyph );
				Delete_GlyphClass( glyph );
			}
		}
	}
	if ( t->descent == 0x7fff )
	{
		gIndex = tsi_T1GetGlyphIndex( t, 'g' );
		if (gIndex)
		{
			glyph = tsi_T1GetGlyphByIndex( t, gIndex, &aw, &ah, NULL, -1);
			if (glyph)
			{
				t->descent = GetGlyphYMin( glyph );
				Delete_GlyphClass( glyph );
			}
		}
	}
	t->advanceWidthMax = t->upem;	/* we just don't really know */
	t->maxPointCount = 32;			/* well, start with that anyway */

	if ( t->ascent == 0x7fff )  t->ascent  =  750;
	if ( t->descent == 0x7fff ) t->descent = -250;
	t->lineGap = t->upem - (t->ascent - t->descent);
	if ( t->lineGap < 0 ) t->lineGap = 0;
	
}


uint16 FF_GetAW_T1Class( void *param1, register uint16 index )
{
	register T1Class *t = (T1Class *)param1;
	register uint16 value = 0;
	uint16 aw, ah;
	GlyphClass *glyph;
	glyph = tsi_T1GetGlyphByIndex( t, index, &aw, &ah, NULL, -1);
	if (glyph)
	{
		value = aw;
		Delete_GlyphClass( glyph );
	}
	return value; /*****/
}

static void BuildSubrs( T1Class *t )
{
	register uint8 *start, *p, *pStartSub;
	int i, fnum, byteCount;
	long limit = t->dataLen;
	
	t->numSubrs		= 0;
	t->subrsData	= NULL;
	
	start = tsi_T1Find( t, (uint8 *)"/Subrs ", t->eexecGO, t->dataLen );
	if ( start == NULL ) return; /*****/
	t->numSubrs = ATOI( start );
	
	t->subrsData = (uint8  **)tsi_AllocMem( t->mem, sizeof(uint8  *) * t->numSubrs );
	for ( i = 0; i < t->numSubrs; i++ ) {
		t->subrsData[i] = NULL;
	}
	for ( p = start, i = 0; i < t->numSubrs; i++ ) {
		/* dup 0 15 RD .....  NP */
		/* ... */
		/* dup 15 9 RD .....  NP */
		/* They are not always consequtive */
		p = tsi_T1Find( t, (uint8 *)"dup ", p - t->decryptedData, limit );
		fnum = ATOI( p );
		pStartSub = tsi_T1Find( t, (uint8 *)"RD ", p - t->decryptedData, p - t->decryptedData + 16 );
		if ( pStartSub == NULL ) {
			pStartSub = tsi_T1Find( t, (uint8 *)"-| ", p - t->decryptedData, p - t->decryptedData + 16 );
		}
		assert( pStartSub != NULL );
		p = pStartSub;
		
		byteCount = backwardsATOI((char *)(p - 5)); 
		assert( fnum >= 0 && fnum < t->numSubrs );
		t->subrsData[fnum] = p; /* byteCount	= backwardsATOI( t->subrsData[index]-5 ); */
		p += byteCount;
/*
printf("%d : Subr, byteCount = %d\n", fnum, byteCount );
*/

	}
	/*****/
}

static int T1stringsAreEqual( void *privptr, char *str, uint16 n)
{
T1Class *t = (T1Class *)privptr;
int isEqual = false;
uint8 *pString;
register const sfnt_CharToName *CharToNameTableP;
	if ( *t->encoding <= '9' && *t->encoding >= '0' )
	{/* Use the encoding vector */
		pString = GetNthNameFromEncodingVector( t, (int) n );
	}
	else
	{/* Standard encoding */
		CharToNameTableP = sfnt_CharToNameTable;
		pString = (uint8 *)CharToNameTableP[n].name;
	}
	isEqual = (strncmp((const char *)str, (const char *)pString, strlen(str)) == 0);
	return isEqual;
}



static void BuildCMAP( T1Class *t )
{
	register uint8 *start, *name,  *p = t->decryptedData;
	register int index, j, byteCount;
	register long i, limit = t->dataLen;
	uint8 c_0, c_1, c_2, c_3;
	uint8 PSName[64];
	uint16 tableIdx, encodingIdx;
	
	
	/* /Encoding StandardEncoding def */
	/* Or, Encoding # array etc.. */
	
	t->encoding = tsi_T1Find( t, (uint8 *)"/Encoding ", 0, t->dataLen );
	start = tsi_T1Find( t, (uint8 *)"/CharStrings ", t->eexecGO, t->dataLen );
	tsi_Assert( t->mem, start != NULL, T2K_BAD_FONT );
	t->charstringsGO = start - t->decryptedData;

	t->NumCharStrings	= ATOI( start );
	t->charCode		= (uint16 *)tsi_AllocMem( t->mem, sizeof(uint16) * t->NumCharStrings );
	t->adobeCode	= (uint16 *)tsi_AllocMem( t->mem, sizeof(uint16) * t->NumCharStrings );
	t->charData		= (uint8 **)tsi_AllocMem( t->mem, sizeof(uint8  *) * t->NumCharStrings );
/* printf("t->NumCharStrings = %d\n", t->NumCharStrings ); */

	t->T1_StringsHash = New_hashClass( t->mem,
										t->NumCharStrings,
										(FF_STRS_ARE_EQUAL_FUNC_PTR) T1stringsAreEqual,
										t );
	
	/* Initialize */
	for ( i = 0; i < t->NumCharStrings; i++ ) {
		t->charCode[i] = 0xffff;
		t->adobeCode[i] = 0xffff;
		t->charData[i] = NULL;
	}
	t->firstCharCode = 0x00;
	t->lastCharCode = 0xff;
	
	c_2 = c_1 = c_0 = 0;
	index = byteCount = 0;

	name = NULL;
	for ( i = start - t->decryptedData; i < limit; i++ ) {
		c_3 = c_2; c_2 = c_1; c_1 = c_0;
		c_0 = p[i];
		
		if ( byteCount ) {
			byteCount--;
			continue; /*****/
		}

		if ( c_0 == '/' ) {
			name = &p[i] + 1;
		} else if ( c_3 == ' ' && c_0 == ' ' && ((c_2 == 'R' && c_1 == 'D') || (c_2 == '-' && c_1 == '|')) ) {
			byteCount	= backwardsATOI( (char*)&p[i-4] );
			assert( index < t->NumCharStrings );

			for ( j = 0; *name != ' ' && j < 63; j++ ) {
				PSName[j] = *name++;
			}
			PSName[j] = 0; /* Zero terminate */
			assert( j < 64 );
			t->charData[index] = &p[i] + 1; /* byteCount	= backwardsATOI( t->charData[index]-5 ); */
			
			if ( *t->encoding <= '9' && *t->encoding >= '0' ) {
				/* Use the encoding vector */
				t->charCode[index] = PSNameToCodeFromEncodingVector( t, PSName, &encodingIdx );
				/* t->adobeCode[index] = t->charCode[index]; */
				PSNameToAppleCode( PSName, &t->adobeCode[index], &tableIdx ); /* 7/16/98 Replaced the above line with this ---Sampo */

				put_hashClass( t->T1_StringsHash, encodingIdx, t->charCode[index], (char *)PSName  );

 /* printf("Encoding vector "); */
			} else {
/* printf("Standard encoding "); */
				/* Standard encoding */
				t->charCode[index] = PSNameToAppleCode( PSName, &t->adobeCode[index], &tableIdx );

				put_hashClass( t->T1_StringsHash, tableIdx, t->charCode[index], (char *)PSName  );

			}
/*
printf("name = <%s> ", PSName);
printf("code = 0x%x ", t->charCode[index]);
printf("adobe-c = 0x%x ", t->adobeCode[index]);
printf("index = %d ", index);
printf("byteCount = %d\n", byteCount);
*/
			index++;
			/* Break to avoid out of bounds writes for corrupt font data */
			if ( index >= t->NumCharStrings ) break; /*****/ 
		}
	}
}




static uint8 *tsi_T1Findtag( uint8 *p, const uint8 *param, int start, int limit )
{
	register size_t j, length;
	register long i;
	register uint8 c0, *result;
	
	result = NULL;
	assert( p != NULL );
	assert( param != NULL );
	length = strlen( (const char *)param );
	c0 = param[0];
	for ( i = start; i < limit; i++ ) {
		if ( c0 == p[i] ) {
			int match = true;
			for ( j = 1; j < length; j++ ) {
				if ( p[i+j] != param[j] ) {
					match = false;
					break; /*****/
				}
			}
			if ( match ) {
				result = &p[i+j];
				break;/*****/
			}
		}
	}
	return result; /*****/
}



kernSubTable0Data *New_T1kernSubTable0Data(T1Class *tp, tsiMemObject *mem, uint8 *aData, long dataLen )
{
	int num_pairs = 0;
	uint8 *start;
	uint8 *current;
	int numx /*, numy */;
	char first_name[63];
	char second_name[63];
	int jj, jjj, count, limit;
	uint16 aCode = 0;
	uint16 bCode = 0;
	uint16 first_code = 0;
	uint16 second_code = 0;
	uint16 dontcare;
	


	kernSubTable0Data *t = (kernSubTable0Data *) tsi_AllocMem( mem, sizeof( kernSubTable0Data ) );

	start = aData;
	t->mem	= mem;
	t->nPairs = 0;
	t->pairs = NULL;
	current = tsi_T1Findtag( start, (const uint8 *)"StartKernPairs", 0, dataLen);
	if (current != NULL)
	{
		num_pairs	= ATOI( current );
		start = current;


	
		t->nPairs			= (uint16)num_pairs;
		t->searchRange		= (uint16)0;
		t->entrySelector	= (uint16)0;
		t->rangeShift		= (uint16)0;
		t->pairs			= (kernPair0Struct*) tsi_AllocMem( mem, t->nPairs * sizeof(kernPair0Struct) );

	
	
		for (count = 0; count < num_pairs; count++)
		{
			/*********   KP case ************/
			jj = 0;
			limit = 80;
			current = tsi_T1Findtag( start, (const uint8 *)"KP ", 0, 80);
			if (current != NULL)
			{
			for ( ; *current == ' ' && jj < limit; current++ ) jj++; 	/* skip spaces */

			for ( jjj = 0; *current != ' ' && jjj < 63; jjj++ ) {
				first_name[jjj] = (char)(*current++);
			}
			first_name[jjj] = 0;

			PSNameToAppleCode( (uint8 *)first_name, &aCode , &dontcare);
			first_code = tsi_T1GetGlyphIndexFromAdobeCode( tp, aCode );


			for ( ; *current == ' ' && jj < limit; current++ ) jj++; 	/* skip spaces */
		
			for ( jjj = 0; *current != ' ' && jjj < 63; jjj++ ) {
				second_name[jjj] = (char)(*current++);
			}
			second_name[jjj] = 0;

			PSNameToAppleCode( (uint8 *)second_name, &aCode , &dontcare);
			second_code = tsi_T1GetGlyphIndexFromAdobeCode( tp, aCode );


			for ( ; *current == ' ' && jj < limit; current++ ) jj++; 	/* skip spaces */
			numx = ATOI(current);

			for ( ; *current != ' ' && count < limit; current++ ) count++; 	/* skip spaces */
			/* numy = ATOI(current); */
			start = current;

			t->pairs[count].leftRightIndex	= (uint32)((first_code << 16) | (second_code));
			t->pairs[count].value 			= (int16)numx;

			}

			/*********   KPH case ************/
			jj = 0;
			limit = 80;
			current = tsi_T1Findtag( start, (const uint8 *)"KPH", 0, 80);
			if (current != NULL)
			{
			for ( ; *current == ' ' && jj < limit; current++ ) jj++; 	/* skip spaces */
			for ( ; *current == '<' && jj < limit; current++ ) jj++; 	/* skip < */
			
			for ( jjj = 0; *current != '>'  && jjj < 63; jjj++ ) {
				first_name[jjj] = (char)(*current++);
			}
			first_name[jjj] = 0;

			if ( first_name[0] <= '9' && first_name[0] >= '0' ) 
				aCode = (uint16)(0 + first_name[0] - '0');
			else if ( first_name[0] <= 'F' && first_name[0] >= 'A' ) 
				aCode = (uint16)(10 + second_name[0] - 'A');
			else if ( first_name[0] <= 'f' && first_name[0] >= 'a' ) 
				aCode = (uint16)(10 + second_name[0] - 'a');


			if ( first_name[1] <= '9' && first_name[1] >= '0' ) 
				bCode = (uint16)(0 + first_name[1] - '0');
			else if ( first_name[1] <= 'F' && first_name[1] >= 'A' ) 
				bCode = (uint16)(10 + first_name[1] - 'A');
			else if ( first_name[1] <= 'f' && first_name[1] >= 'a' ) 
				bCode = (uint16)(10 + first_name[1] - 'a');

			aCode = (uint16)((aCode << 4) | (bCode & 0x0F));
			first_code = tsi_T1GetGlyphIndexFromAdobeCode( tp, aCode );



			for ( ; *current == '>' && jj < limit; current++ ) jj++; 	/* skip > */
			for ( ; *current == ' ' && jj < limit; current++ ) jj++; 	/* skip spaces */
			for ( ; *current == '<' && jj < limit; current++ ) jj++; 	/* skip < */

			for ( jjj = 0; *current != '>'  && jjj < 63; jjj++ ) {
				second_name[jjj] = (char)(*current++);
			}
			second_name[jjj] = 0;


			if ( second_name[0] <= '9' && second_name[0] >= '0' ) 
				aCode = (uint16)(0 + second_name[0] - '0');
			else if ( second_name[0] <= 'F' && second_name[0] >= 'A' ) 
				aCode = (uint16)(10 + second_name[0] - 'A');
			else if ( second_name[0] <= 'f' && second_name[0] >= 'a' ) 
				aCode = (uint16)(10 + second_name[0] - 'a');


			if ( second_name[1] <= '9' && second_name[1] >= '0' ) 
				bCode = (uint16)(0 + second_name[1] - '0');
			else if ( second_name[1] <= 'F' && second_name[1] >= 'A' ) 
				bCode = (uint16)(10 + second_name[1] - 'A');
			else if ( second_name[1] <= 'f' && second_name[1] >= 'a' ) 
				bCode = (uint16)(10 + second_name[1] - 'a');

			aCode = (uint16)((aCode << 4) | (bCode & 0x0F));
			second_code = tsi_T1GetGlyphIndexFromAdobeCode( tp, aCode );

			for ( ; *current == '>' && jj < limit; current++ ) jj++; 	/* skip > */
			for ( ; *current == ' ' && count < limit; current++ ) count++; 	/* skip spaces */

			numx = ATOI(current);

			for ( ; *current != ' ' && count < limit; current++ ) count++; 	/* skip spaces */

			/* numy = ATOI(current); */
			start = current;

			t->pairs[count].leftRightIndex	= (uint32)((first_code << 16) | (second_code));
			t->pairs[count].value 			= (int16)numx;

			}

			/*********   KPX case ************/
			jj = 0;
			limit = 80;

			current = tsi_T1Findtag( start, (const uint8 *)"KPX", 0, 80);
			if (current != NULL)
			{
			for ( ; *current == ' ' && jj < limit; current++ ) jj++; 	/* skip spaces */

			for ( jjj = 0; *current != ' ' && jjj < 63; jjj++ ) {
				first_name[jjj] = (char)(*current++);
			}
			first_name[jjj] = 0;

			PSNameToAppleCode( (uint8 *)first_name, &aCode , &dontcare);
			first_code = tsi_T1GetGlyphIndexFromAdobeCode( tp, aCode );

			for ( ; *current == ' ' && jj < limit; current++ ) jj++; 	/* skip spaces */

			for ( jjj = 0; *current != ' ' && jjj < 63; jjj++ ) {
				second_name[jjj] = (char)(*current++);
			}
			second_name[jjj] = 0;

			PSNameToAppleCode( (uint8 *)second_name, &aCode, &dontcare);
			second_code = tsi_T1GetGlyphIndexFromAdobeCode( tp, aCode );


			for ( ; *current == ' ' && jj < limit; current++ ) jj++; 	/* skip spaces */
			numx = ATOI(current);
			start = current;

			t->pairs[count].leftRightIndex	= (uint32)((first_code << 16) | (second_code));
			t->pairs[count].value 			= (int16)numx;

			}

			/*********   KPY case ************/
			jj = 0;
			limit = 80;

			current = tsi_T1Findtag( start, (const uint8 *)"KPY", 0, 80);
			if (current != NULL)
			{
			for ( ; *current == ' ' && jj < limit; current++ ) jj++; 	/* skip spaces */

			for ( jjj = 0; *current != ' ' && jjj < 63; jjj++ ) {
				first_name[jjj] = (char)(*current++);
			}
			first_name[jjj] = 0;
			for ( ; *current == ' ' && jj < limit; current++ ) jj++; 	/* skip spaces */

			for ( jjj = 0; *current != ' ' && jjj < 63; jjj++ ) {
				second_name[jjj] = (char)(*current++);
			}
			second_name[jjj] = 0;
			for ( ; *current == ' ' && jj < limit; current++ ) jj++; 	/* skip spaces */
			/* numy = ATOI(current); */
			start = current;

			t->pairs[count].leftRightIndex	= (uint32)((first_code << 16) | (second_code));
			t->pairs[count].value 			= 0;

			}
		}
	}
	if (num_pairs != 0)
		ff_KernShellSort(t->pairs, num_pairs);

	return t; /*****/
}



kernSubTable *New_T1kernSubTable(T1Class *tp, tsiMemObject *mem, uint8 *in , 
										 long extraItemSize)
{
	kernSubTable *t = (kernSubTable *) tsi_AllocMem( mem, sizeof( kernSubTable ) );
	t->mem			= mem;

	t->version		= (uint16)0;
	t->length		= (uint16)sizeof(kernSubTable);
	t->coverage		= (uint16)1;

	t->kernData			= NULL;
	if ( t->version == 0 && t->length > 0 ) {
		t->kernData			= New_T1kernSubTable0Data(tp, mem, in , extraItemSize);
	}
	
	return t; /*****/
}

kernClass *New_T1kernClass(T1Class *tp, tsiMemObject *mem, uint8 *in, long extraItemSize )
{
	
	kernClass *t = (kernClass *) tsi_AllocMem( mem, sizeof( kernClass ) );
	t->mem			= mem;
	t->version		= 0;
	t->nTables		= 1;
	
	t->table		= (kernSubTable **) tsi_AllocMem( mem, sizeof( kernSubTable * ) );

	*(t->table) = New_T1kernSubTable(tp, mem, in , extraItemSize);

	
	return t; /*****/
}








T1Class *tsi_NewT1Class( tsiMemObject *mem, uint8 *aData, long dataLen )
{
	register T1Class *t = (T1Class *)tsi_AllocMem( mem, sizeof( T1Class ) );
	t->mem = mem;
	
	
	
	t->glyph = NULL;
	t->decryptedData = NULL; t->dataLen = 0;
	
	
	TransformData( t, transform_decrypt, aData, dataLen );
	/* WriteDataToFile( "AvantGarBooObl.txt", t->decryptedData, (unsigned long) dataLen ); */
	/* WriteDataToFile( "MyriaMM.txt", t->decryptedData, (unsigned long) dataLen ); */
	/* WriteDataToFile( "times.txt", t->decryptedData, (unsigned long) t->dataLen ); */
	
	t->charCode		= NULL;
	t->adobeCode	= NULL;
	t->charData		= NULL;
	t->subrsData	= NULL;
	t->charstringsGO = t->dataLen;
	t->eexecGO = 0;
	BuildCMAP( t );
	BuildSubrs( t );
	t->lenIV = (short)tsi_T1GetParam( t, (uint8 *)"/lenIV ", 4 );
	BuildMetricsEtc( t );
	return t; /*****/
}


void tsi_DeleteT1Class( T1Class *t )
{
	if ( t != NULL ) {
		/* tsi_DeAllocMem( t->mem, t->baseAddr ); */
		Delete_GlyphClass( t->glyph );
		if ( t->decryptedData != t->dataInPtr ) {
			tsi_DeAllocMem( t->mem, (char *)t->decryptedData );
		}
		tsi_DeAllocMem( t->mem, (char *)t->charCode );
		tsi_DeAllocMem( t->mem, (char *)t->adobeCode );
		tsi_DeAllocMem( t->mem, (char *)t->charData );
		tsi_DeAllocMem( t->mem, (char *)t->subrsData );
		Delete_hashClass( t->T1_StringsHash );
		tsi_DeAllocMem( t->mem, (char *)t );

	}
}

void tsi_T1ListChars(void *userArg, T1Class *t, void *ctxPtr, int ListCharsFn(void *userArg, void *p, uint16 code))
{
int ii, checkStop = 0;
	for (ii = 0; !checkStop && (ii < t->NumCharStrings); ii++)
	{
		if (t->charCode[ii] != 0xffff)
			checkStop = ListCharsFn(userArg, ctxPtr, t->charCode[ii]);
	}
}

/*
 * converts PSName to ccode for Type1 fonts
*/
uint16 tsi_T1PSName2CharCode( T1Class *t, char *PSName)
{
int found;
uint16 ccode;

	found = get_using_str_hashClass( t->T1_StringsHash, PSName, &ccode );
	if (!found)
		ccode = 0;
	return ccode;
}

#endif /* ENABLE_T1 */

#ifdef ENABLE_CFF

typedef struct
{
	SIDCode sid;
	char *StdStringName;
} SIDKeyPair;

extern char *StdSIDStrings[nStdStrings]; /* ahead, see end of ENABLE_CFF section */


static void ShellSortT2Cmap(cmap_t *pairs, int num_pair);
static uint8 BSearchCTbl ( CFFClass *t, uint16 *indexPtr, uint16 theValue);
static CFFClass *_tsi_NewCFFClass( tsiMemObject *mem, InputStream *in, int32 fontNum, uint8 isOpenType, uint32 cffOffset);

static int T2stringsAreEqual( void *privptr, char *str, uint16 n);






static uint32 ReadOfffset1( InputStream *in )
{
	return ReadUnsignedByteMacro( in ); /*****/
}

static uint32 ReadOfffset2( InputStream *in )
{
	uint32 offset = ReadUnsignedByteMacro( in );
	offset      <<= 8;
	offset       |= ReadUnsignedByteMacro( in );
	return offset; /*****/
}

static uint32 ReadOfffset3( InputStream *in )
{
	uint32 offset = ReadUnsignedByteMacro( in );
	offset      <<= 8;
	offset       |= ReadUnsignedByteMacro( in );
	offset      <<= 8;
	offset       |= ReadUnsignedByteMacro( in );
	return offset; /*****/
}

static uint32 ReadOfffset4( InputStream *in )
{
	uint32 offset = ReadUnsignedByteMacro( in );
	offset      <<= 8;
	offset       |= ReadUnsignedByteMacro( in );
	offset      <<= 8;
	offset       |= ReadUnsignedByteMacro( in );
	offset      <<= 8;
	offset       |= ReadUnsignedByteMacro( in );
	return offset; /*****/
}

typedef uint32 (*PF_READ_OFFSET) ( InputStream *in );

static PF_READ_OFFSET GetOffsetFunction( OffSize offSize )
{
	PF_READ_OFFSET f;
	
	assert( offSize >= 1 && offSize <= 4 );
	if ( offSize == 1 ) {
		f = ReadOfffset1;
	} else if ( offSize == 2 ) {
		f = ReadOfffset2;
	} else if ( offSize == 3 ) {
		f = ReadOfffset3;
	} else {
		f = ReadOfffset4;
	}
	
	return f;/*****/
}

#define READ_SID(in) ((SIDCode)ReadInt16( in ))
#define IS_OPERATOR( v1 )	(v1 <= 27 || v1 == 31 )
#define IS_NUMBER( v1 )		(v1 >  27 && v1 != 31 )

static F16Dot16 READ_REAL( /* int v1, */ InputStream *in )
{
	unsigned char nibble[2];
	int i;
	int len;
#define MAX_REAL_STRING_LEN 64
	char pRealString[MAX_REAL_STRING_LEN + 1];
	
	/* assert( v1 == 30 ); */
	
	for (len = 0;;) 
    {
		nibble[0] = ReadUnsignedByteMacro( in );
		nibble[1] = (uint8)(nibble[0] & 0x0f);
		nibble[0] = (uint8)((nibble[0] & 0xf0) >> 4);
        for (i = 0; i < 2; i++)
        {
            switch (nibble[i])
            {
            case 0xa: /* .(decimal point) */
                assert( len < MAX_REAL_STRING_LEN );
                pRealString[len++] = '.';
                break;

            case 0xb: /* E */
                assert( len < MAX_REAL_STRING_LEN );
                pRealString[len++] = 'E';
                break;

            case 0xc: /* E- */
                assert( len < MAX_REAL_STRING_LEN - 1 );
                pRealString[len++] = 'E';
                pRealString[len++] = '-';
                break;

            case 0xd: /* reserved */
                break;

            case 0xe: /* -(minus) */
                assert( len < MAX_REAL_STRING_LEN );
                pRealString[len++] = '-';
                break;

            case 0xf: /* done */
                pRealString[len] = '\0';
                return ATOFixed( (unsigned char *)pRealString, 0 );

            default: /* 0-9 */
                assert( len < MAX_REAL_STRING_LEN );
                pRealString[len++] = (char)('0' + (char)nibble[i]);          
            }
        }
	}
	return 0; /* never reached */
}


static int32 READ_INTEGER( int v1, InputStream *in )
{
	int32 num = 0;
	int v2;
	
	if ( v1 == 28 ) {
		num  = ReadUnsignedByteMacro( in );
		num <<= 8;
		num |= ReadUnsignedByteMacro( in );
	} else if ( v1 == 29 ) {
		num  = ReadUnsignedByteMacro( in );
		num <<= 8;
		num |= ReadUnsignedByteMacro( in );
		num <<= 8;
		num |= ReadUnsignedByteMacro( in );
		num <<= 8;
		num |= ReadUnsignedByteMacro( in );
	} else {
		assert( v1 >= 32 );
		
		if ( v1 <= 246 ) {
			num = v1 - 139; 					/* 1 byte: [-107, +107] */
		} else if ( v1 <= 250 ) {
			v2 = ReadUnsignedByteMacro( in );
			num = ((v1-247)*256) + v2 + 108;	/* 2 bytes: [108, 1131] */
		} else if ( v1 <= 254 ) {
			v2 = ReadUnsignedByteMacro( in );
			num = -((v1-251)*256) - v2 - 108;	/* 2 bytes: [-108, -1131] */
		} else {
			assert( false );
		}
	}
	return num; /*****/
}


static CFFIndexClass *tsi_NewCFFIndexClass( tsiMemObject *mem, InputStream *in )
{
	register CFFIndexClass *t = (CFFIndexClass *)tsi_AllocMem( mem, sizeof( CFFIndexClass ) );
	PF_READ_OFFSET ReadOfffset;
	int32 i, max;

	t->mem 			= mem;
	t->offsetArray	= NULL;
	
	t->count 		= (Card16)ReadInt16( in );
	if ( t->count != 0 ) {
		t->offSize 		= ReadUnsignedByteMacro( in );
		t->offsetArray	= (uint32 *)tsi_AllocMem( mem, sizeof( uint32 ) * (t->count + 1) );
		ReadOfffset		= GetOffsetFunction( t->offSize );
		
		max = t->count; /* [0 .. t->count] */
		for ( i = 0; i <= max; i++ ) {
			t->offsetArray[i] = ReadOfffset( in );
		}
		/* Here is the data */
		t->baseDataOffset = Tell_InputStream( in ) - 1;
		/* offsetArray[0] == 1 */
		
		/* Skip past data. */
		Seek_InputStream( in, t->baseDataOffset + t->offsetArray[t->count] );
	}
	return t; /*****/
}

static uint8 *tsi_GetCFFData( CFFIndexClass *t, InputStream *in, long n )
{
	uint8 *p = NULL;
	if ( n >= 0 && n < t->count ) {
		uint32 length;
		uint32 oldPos = Tell_InputStream( in );
		Seek_InputStream(in, t->baseDataOffset + t->offsetArray[n] );
		length =  t->offsetArray[n+1] -  t->offsetArray[n];
		p = (uint8 *)tsi_AllocMem( t->mem, length + 2 );
		ReadSegment( in, p, (long)length);
		p[length+0] = 0;	/* Zero terminate */
		p[length+1] = 0;	
		Seek_InputStream(in, oldPos );
	}
	return p; /*****/
}

static void tsi_DeleteCFFIndexClass( CFFIndexClass *t )
{
	if ( t != NULL ) {
		tsi_DeAllocMem( t->mem, (char *)t->offsetArray );
		tsi_DeAllocMem( t->mem, (char *)t );
	}
}

uint8 *GetT2NameProperty( CFFClass *t, uint16 languageID, uint16 nameID )
{
	UNUSED(languageID);
	if ( nameID == 4 && t->name != NULL ) {
		return  tsi_GetCFFData( t->name, t->in, t->fontNum );

	}
	return NULL; /*****/
}


static void tsi_ParsePrivateDictData( CFFClass *t )
{
	InputStream *in = t->in;
	uint32 /* pos, */ limit, savepos;
	int32 stack[ CFF_MAX_STACK ];
	int32 number, stackCount = 0;
	
	
	savepos = Tell_InputStream( in );
	
	/* Set default values */
	t->privateDictData.Subr = 0;
	t->privateDictData.SubrOffset = 0;
	t->privateDictData.defaultWidthX = 0;
	t->privateDictData.nominalWidthX = 0;
	
	Seek_InputStream( in, t->cffOffset + (uint32)t->topDictData.PrivateDictOffset );
	limit = (uint32)(t->cffOffset + t->topDictData.PrivateDictOffset + t->topDictData.PrivateDictSize);
	while ( (/*pos = */Tell_InputStream( in )) < limit ) {
		uint8 v1 /* ,v2 */;
		
		v1 = ReadUnsignedByteMacro( in );
		if ( IS_NUMBER( v1 ) ) {
			if ( v1 == 30 ) {
				number = READ_REAL( in );
			} else {
				number = READ_INTEGER( v1, in );
			}
			assert( stackCount < CFF_MAX_STACK );
			stack[ stackCount++ ] = number;
		} else {
			if ( v1 == 12 ) {
				/* v2 = ReadUnsignedByteMacro( in ); */
			} else {
				/* v2 = 0; */
				switch ( v1 ) {
					case 19:	t->privateDictData.Subr = stack[0];
						break; /*****/
					case 20:	t->privateDictData.defaultWidthX = stack[0];
						break; /*****/
					case 21:	t->privateDictData.nominalWidthX = stack[0];
						break; /*****/
					default: 
						break; /*****/
				}
			}
			stackCount = 0;
		}
	}
	if ( t->privateDictData.Subr != 0 ) {
		t->privateDictData.SubrOffset = (int32)t->cffOffset + t->topDictData.PrivateDictOffset + t->privateDictData.Subr;
	}
	Seek_InputStream( in, savepos );
}

static void tsi_ParseCFFTopDict( CFFIndexClass *t, InputStream *in, TopDictInfo *topDictData, int32 n )
{
	uint32 /*pos, */limit, savepos;
	int32 stack[ CFF_MAX_STACK ];
	int32 number, stackCount = 0;
	
	
	savepos = Tell_InputStream( in );
	/* Set default values. */
	topDictData->bbox_xmax = 0; topDictData->bbox_xmin = 0;
	topDictData->bbox_ymax = 0; topDictData->bbox_ymin = 0;
	topDictData->italicAngle = 0;
	topDictData->UnderlinePosition = -100;
	topDictData->UnderlineThickness = 50;
	topDictData->m00 = ONE16Dot16;	topDictData->m01 = 0;				/* Oblique */
	topDictData->m10 = 0;			topDictData->m11 = ONE16Dot16;				

	topDictData->charset 			= 0;
	topDictData->Encoding			= 0;
	topDictData->PrivateDictSize	= 0; topDictData->PrivateDictOffset	= 0;
	
	topDictData->numAxes			= 0;
	topDictData->numMasters			= 1;
	topDictData->lenBuildCharArray	= 0;
	topDictData->buildCharArray		= NULL;
	topDictData->isFixedPitch		= 0;
	topDictData->isCIDKeyed			= false;
	topDictData->CharstringType 	= 2;
	
	Seek_InputStream( in, t->baseDataOffset + t->offsetArray[n] );
	
	limit = t->baseDataOffset + t->offsetArray[n+1];
	while ( (/*pos = */Tell_InputStream( in )) < limit ) {
		uint8 v1, v2;
		
		v1 = ReadUnsignedByteMacro( in );
		if ( IS_NUMBER( v1 ) ) {
			if ( v1 == 30 ) {
				number = READ_REAL( in );
			} else {
				number = READ_INTEGER( v1, in );
			}
			assert( stackCount < CFF_MAX_STACK );
			stack[ stackCount++ ] = number;
		} else {
			if ( v1 == 12 ) {
				v2 = ReadUnsignedByteMacro( in );
				switch ( v2 ) {
					int loop;
					case 1:topDictData->isFixedPitch = (uint32)stack[0];
						break; /*****/
					case 2: topDictData->italicAngle = stack[0];
						break; /*****/
					case 3: topDictData->UnderlinePosition = stack[0];
						break; /*****/
					case 4: topDictData->UnderlineThickness = stack[0];
						break; /*****/
					case 6:	topDictData->CharstringType = (uint8)stack[0];
						break;
					case 24: /* MultipleMaster */
						topDictData->numAxes 	= stackCount - 4;
						assert( topDictData->numAxes <= 16 );
						topDictData->numMasters	= stack[0];
						/* UDV Array, stack[1]..stack[1 + numAxes-1], == default instance */
						for ( loop = 0; loop < topDictData->numAxes; loop ++ ) {
							topDictData->defaultWeight[loop] = stack[1+loop];
						}
						topDictData->lenBuildCharArray = stack[1 + topDictData->numAxes + 0];
						topDictData->buildCharArray	   = (F16Dot16 *)tsi_AllocMem( t->mem, topDictData->lenBuildCharArray * sizeof(F16Dot16) );
						topDictData->NDV = (uint16)stack[1 + topDictData->numAxes + 1];
						topDictData->CDV = (uint16)stack[1 + topDictData->numAxes + 2];
						
						
					case 7:  /* FontMatrix */
                        topDictData->m00 = util_FixMul( stack[0], 1000L  << 16  );
                        topDictData->m10 = util_FixMul( stack[1], 1000L  << 16  );
                        topDictData->m01 = util_FixMul( stack[2], 1000L  << 16  );
                        topDictData->m11 = util_FixMul( stack[3], 1000L  << 16  );
						break; /*****/
					case 30:	/* ROS: signals CID-keyed font */
						topDictData->isCIDKeyed = true;
						break;
					default:
						break; /*****/
				}
			} else {
				/* v2 = 0; */
				switch ( v1 ) {
					case 0:		topDictData->version = (uint16)stack[0];
						break; /*****/
					case 1:		topDictData->Notice = (uint16)stack[0];
						break; /*****/
					case 2:		topDictData->FullName = (uint16)stack[0];
						break; /*****/
					case 3:		topDictData->FamilyName = (uint16)stack[0];
						break; /*****/
					case 4:		topDictData->Weight = (uint16)stack[0];
						break; /*****/
					case 5:		topDictData->bbox_xmin = stack[0];
								topDictData->bbox_ymin = stack[1];
								topDictData->bbox_xmax = stack[2];
								topDictData->bbox_ymax = stack[3];
						break; /*****/
					case 13:	topDictData->UniqueId = stack[0];
						break; /*****/
					case 15:	topDictData->charset = stack[0];
						break; /*****/
					case 16:	topDictData->Encoding = stack[0];
						break; /*****/
					case 17:	topDictData->Charstrings = stack[0];
						break; /*****/
					case 18:	topDictData->PrivateDictSize	= stack[0];
								topDictData->PrivateDictOffset	= stack[1];
						break; /*****/
					default:
						break; /*****/
				}
			}
			stackCount = 0;
		}
	}
	
	Seek_InputStream( in, savepos );
}

static void Type2BuildChar( CFFClass *t, InputStream *in, int byteCount, int nestingLevel )
{
	register int /*i, */ pos, v1, v2;
	unsigned long endpos   = (Tell_InputStream( in ) + byteCount);
	
	register F16Dot16 	x = t->x; /* cache x, and y in local variables */
	register F16Dot16 	y = t->y;
	register F16Dot16 	*gStackValues 	= t->gStackValues; /* Cache the "stack"-array in a local variable */
	register long 		gNumStackValues = t->gNumStackValues; /* Cache gNumStackValues */
	
	/* The group of operators that can participate in the width specification are:
	 * [ hstem,vstem,hstemhm,vstemhm,cntrmask,hintmask,hmoveto,vmoveto,rmoveto,endchar]
	 */

	while ( (/*i=(int)*/Tell_InputStream( in )) < endpos  ) {
		v1 = ReadUnsignedByteMacro( in );
		if ( v1 < 32 ) {
			LOG_CMD( "cmd", v1 );
			LOG_CMD( "#stack", gNumStackValues );
			/* if ( debugOn ) printf("pointCount = %d, x,y = (%d,%d)\n", t->glyph->pointCount , x>>16, y>>16 ); */
			switch( v1 ) {
			/* 0 Reserved */
			case  1:	/* hstem */
			case  3:	/* vstem */
				LOG_CMD( (v1 == 1 ? "hstem" : "vstem"), gNumStackValues );
				assert( gNumStackValues >= 2 );
				/* takes 2*n arguments */
				pos = 0;
				if ( !t->widthDone && (gNumStackValues & 1) ) {
					t->widthDone	= true;
					t->awx 			= t->privateDictData.nominalWidthX + (gStackValues[ pos++ ]>>16);
				}
				t->numStemHints += (gNumStackValues-pos)/2;
				gNumStackValues = 0; /* clear the stack */
				break;
			/* 2 Reserved */
			case  4:	/* vmoveto */
				LOG_CMD( "vmoveto", gNumStackValues );
				pos = 0;
				if ( !t->widthDone && gNumStackValues > 1 ) {
					t->widthDone	= true;
					t->awx 			= t->privateDictData.nominalWidthX + (gStackValues[ pos++ ]>>16);
				}
				assert( gNumStackValues > pos );
				y += gStackValues[ pos ];
				gNumStackValues = 0; /* clear the stack */
				if ( t->pointAdded ) {
					glyph_CloseContour( t->glyph );
				}
				break;
			case  5:	/* rlineto */
				LOG_CMD( "rlineto", gNumStackValues );
				assert( gNumStackValues >= 2 );
				glyph_StartLine( t->glyph, x>>16, y>>16 );
				for ( pos = 0; pos < gNumStackValues; ) {
					x += gStackValues[ pos++ ];
					y += gStackValues[ pos++ ];
					glyph_AddPoint( t->glyph, x>>16, y>>16, 1 );
				}
				t->pointAdded   = 1;
				gNumStackValues = 0; /* clear the stack */
				break;
			case  6:	/* hlineto */
				LOG_CMD( "hlineto", gNumStackValues );
				assert( gNumStackValues >= 1 );
				glyph_StartLine( t->glyph, x>>16, y>>16 );
				for ( pos = 0; pos < gNumStackValues; ) {
					x += gStackValues[ pos++ + 0 ];
					glyph_AddPoint( t->glyph, x>>16, y>>16, 1 );
					if ( pos >= gNumStackValues ) break; /*****/
					y += gStackValues[ pos++ + 0 ];
					glyph_AddPoint( t->glyph, x>>16, y>>16, 1 );
				}
				t->pointAdded   = 1;
				gNumStackValues = 0; /* clear the stack */
				break;
			case  7:	/* vlineto */
				LOG_CMD( "vlineto", gNumStackValues );
				assert( gNumStackValues >= 1 );
				glyph_StartLine( t->glyph, x>>16, y>>16 );
				for ( pos = 0; pos < gNumStackValues; ) {
					y += gStackValues[ pos++ + 0 ];
					glyph_AddPoint( t->glyph, x>>16, y>>16, 1 );
					if ( pos >= gNumStackValues ) break; /*****/
					x += gStackValues[ pos++ + 0 ];
					glyph_AddPoint( t->glyph, x>>16, y>>16, 1 );
				}
				t->pointAdded   = 1;
				gNumStackValues = 0; /* clear the stack */
				break;
			case  8:	/* rrcurveto */
				LOG_CMD( "rrcurveto", gNumStackValues );
				assert( gNumStackValues >= 6 );
				assert( gNumStackValues % 6 == 0 );
				glyph_StartLine( t->glyph, x>>16, y>>16 );
				for ( pos = 0; pos < gNumStackValues; ) {
					x += gStackValues[ pos++ ];
					y += gStackValues[ pos++ ];
					glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
					x += gStackValues[ pos++ ];
					y += gStackValues[ pos++ ];
					glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
					x += gStackValues[ pos++ ];
					y += gStackValues[ pos++ ];
					glyph_AddPoint( t->glyph, x>>16, y>>16, 1 );
				}
				t->pointAdded   = 1;
				gNumStackValues = 0; /* clear the stack */
				break;
			/* 9 Reserved */
			case 29:	/* callgsubr */
			case 10:	/* callsubr */
				assert( gNumStackValues >= 1 );
				gNumStackValues -= 1;
				{
					int fnum = gStackValues[ gNumStackValues + 0 ] >> 16; /* topmost element */
					long savepos;
					int fByteCount;
					CFFIndexClass *subr;
					
					if ( v1 == 10 ) {
						LOG_CMD( "callsubr", gNumStackValues );
						fnum = fnum + t->lSubrBias;
						subr = t->lSubr;
					} else {
						LOG_CMD( "callgsubr", gNumStackValues );
						fnum = fnum + t->gSubrBias;
						subr = t->gSubr;
					}
					
					if ( subr != NULL && fnum >= 0 && fnum < subr->count ) {
						
						savepos = (long)Tell_InputStream( in );
						Seek_InputStream( t->in, subr->baseDataOffset + subr->offsetArray[ fnum ] );
						LOG_CMD("***callsubr number =  ", fnum);			
						LOG_CMD("***callsubr fByteCount = ", fByteCount);			
						LOG_CMD("***BEGIN CALL = ", fnum );	
						fByteCount = (int)(subr->offsetArray[ fnum + 1 ] - subr->offsetArray[ fnum ]);
						if ( fByteCount > 0 && nestingLevel < 10 ) {
							/* Save state from registers. */
							t->x = x;
							t->y = y;
							t->gNumStackValues = gNumStackValues;
							Type2BuildChar( t, in, fByteCount, nestingLevel+1 );
							/* Cache state in registers. */
							x = t->x;
							y = t->y;
							gNumStackValues = t->gNumStackValues;
						}
						LOG_CMD("***END CALL = ", fnum );			
						Seek_InputStream( t->in, (uint32)savepos );
					}
					assert( gNumStackValues >= 0 );
					assert( gNumStackValues <= CFF_MAX_STACK );
				}
				/* Do not clear the stack */
				/* gNumStackValues = 0; */
				break;
			case 11:	/* return  (from callsubr) */
				LOG_CMD( "return", gNumStackValues );
				/* Save state from registers. */
				t->x = x;
				t->y = y;
				t->gNumStackValues = gNumStackValues;
				return; /*****/
				/* Do not clear the stack */
				/* break; */
			case 12:	/* escape */
				v2 = ReadUnsignedByteMacro( in );
				LOG_CMD( "v2", v2 );
				switch( v2 ) {
					F16Dot16 dx1, dy1, dx2, dy2, dx3, dy3, dx4, dy4, dx5, dy5, dx6, dy6;
					case 0: /* reserved but happens == old T1 dotsection */
						gNumStackValues = 0; /* clear the stack */
						break;
						
					/* 0,1,2 are reserved */
					case 3: 	/* and */
						LOG_CMD( "and", gNumStackValues );
						assert( gNumStackValues >= 2 );
						gNumStackValues -= 2;
						
						gStackValues[ gNumStackValues + 0 ] = 	gStackValues[ gNumStackValues + 0 ] &&
																gStackValues[ gNumStackValues + 1 ] ? ONE16Dot16 : 0;
						gNumStackValues++;
						break;
					case 4: 	/* or */
						LOG_CMD( "or", gNumStackValues );
						assert( gNumStackValues >= 2 );
						gNumStackValues -= 2;
						
						gStackValues[ gNumStackValues + 0 ] = 	gStackValues[ gNumStackValues + 0 ] ||
																gStackValues[ gNumStackValues + 1 ] ? ONE16Dot16 : 0;
						gNumStackValues++;
						break;
					case 5: 	/* not */
						LOG_CMD( "not", gNumStackValues );
						assert( gNumStackValues >= 1 );
						gNumStackValues--;
						gStackValues[ gNumStackValues + 0 ] =	gStackValues[ gNumStackValues + 0 ] == 0 ? ONE16Dot16 : 0;
						gNumStackValues++;
						break;
					/* 6, 7 are reserved */
					case 8: 	/* store */
						LOG_CMD( "store", gNumStackValues );
						assert( gNumStackValues >= 4 );
						gNumStackValues -= 4;
						assert( t->topDictData.buildCharArray != NULL );
						{
							int index, j, regItem, count, i1;
							
							regItem	= gStackValues[ gNumStackValues + 0 ] >> 16;
							j		= gStackValues[ gNumStackValues + 1 ] >> 16;
							index	= gStackValues[ gNumStackValues + 2 ] >> 16;
							count	= gStackValues[ gNumStackValues + 3 ] >> 16;
							
							assert( index >= 0 && index < t->topDictData.lenBuildCharArray );
							switch ( regItem ) {
							case 0:
								for ( i1 = 0; i1 < count; i1++ ) {
									t->topDictData.reg_WeightVector[j + i1] = t->topDictData.buildCharArray[index + i1];
								}
								break;
							case 1:
								for ( i1 = 0; i1 < count; i1++ ) {
									t->topDictData.reg_NormalizedDesignVector[j + i1] = t->topDictData.buildCharArray[index + i1];
								}
								break;
							case 2:
								for ( i1 = 0; i1 < count; i1++ ) {
									t->topDictData.reg_UserDesignVector[j + i1] = t->topDictData.buildCharArray[index + i1];
								}
								break;
								
							default:
								break;
							
							}
						}
						break;
					case 9: 	/* abs */
						LOG_CMD( "abs", gNumStackValues );
						assert( gNumStackValues >= 1 );
						{
							F16Dot16 value;
							gNumStackValues--;
							
							value = gStackValues[ gNumStackValues + 0 ];
							if ( value < 0 ) {
								gStackValues[ gNumStackValues + 0 ] = -value;
							}
							gNumStackValues++;
						}
						break;
					case 10: 	/* add */
						LOG_CMD( "add", gNumStackValues );
						assert( gNumStackValues >= 2 );
						gNumStackValues -= 2;
						
						gStackValues[ gNumStackValues + 0 ] = 	gStackValues[ gNumStackValues + 0 ] +
																gStackValues[ gNumStackValues + 1 ];
						gNumStackValues++;
						break;
					case 11: 	/* sub */
						LOG_CMD( "sub", gNumStackValues );
						assert( gNumStackValues >= 2 );
						gNumStackValues -= 2;
						
						gStackValues[ gNumStackValues + 0 ] =	gStackValues[ gNumStackValues + 0 ] -
																gStackValues[ gNumStackValues + 1 ];
						gNumStackValues++;
						break;
					case 12: 	/* div */
						LOG_CMD( "div", gNumStackValues );
						assert( gNumStackValues >= 2 );
						gNumStackValues -= 2;
						
						gStackValues[ gNumStackValues + 0 ] = util_FixDiv(	gStackValues[ gNumStackValues + 0 ], 
																			gStackValues[ gNumStackValues + 1 ] );
						gNumStackValues++;
						break;
					case 13: 	/* load */
						LOG_CMD( "load", gNumStackValues );
						assert( gNumStackValues >= 3 );
						gNumStackValues -= 3;
						assert( t->topDictData.buildCharArray != NULL );
						{
							int index, regItem, count, i1;
							
							regItem	= gStackValues[ gNumStackValues + 0 ] >> 16;
							index	= gStackValues[ gNumStackValues + 1 ] >> 16;
							count	= gStackValues[ gNumStackValues + 2 ] >> 16;
							
							assert( index >= 0 && index < t->topDictData.lenBuildCharArray );
							switch ( regItem ) {
							case 0:
								for ( i1 = 0; i1 < count; i1++ ) {
									t->topDictData.buildCharArray[index + i1] = t->topDictData.reg_WeightVector[i1];
								}
								break;
							case 1:
								for ( i1 = 0; i1 < count; i1++ ) {
									t->topDictData.buildCharArray[index + i1] = t->topDictData.reg_NormalizedDesignVector[i1];
								}
								break;
							case 2:
								for ( i1 = 0; i1 < count; i1++ ) {
									t->topDictData.buildCharArray[index + i1] = t->topDictData.reg_UserDesignVector[i1];
								}
								break;
								
							default:
								break;
							
							}
						}
						break;
					
					
					case 14: 	/* neg */
						LOG_CMD( "neg", gNumStackValues );
						assert( gNumStackValues >= 1 );
						gNumStackValues--;
						gStackValues[ gNumStackValues + 0 ] = -gStackValues[ gNumStackValues + 0 ];
						gNumStackValues++;
						break;
					case 15: 	/* eq */
						LOG_CMD( "eq", gNumStackValues );
						assert( gNumStackValues >= 2 );
						gNumStackValues -= 2;
						
						gStackValues[ gNumStackValues + 0 ] = 	gStackValues[ gNumStackValues + 0 ] ==
																gStackValues[ gNumStackValues + 1 ] ? ONE16Dot16 : 0;
						gNumStackValues++;
						break;
						
					/* 16 is reserved */
					/* 17 is not the pop operator and is reserved. */
					
					case 18: 	/* drop */
						LOG_CMD( "drop", gNumStackValues );
						assert( gNumStackValues >= 1 );
						gNumStackValues--;
						break;
						
					/* 19 is reserved */	
					/* t->topDictData.buildCharArray */
					case 20: 	/* put */
						LOG_CMD( "put", gNumStackValues );
						assert( gNumStackValues >= 2 );
						gNumStackValues -= 2;
						assert( t->topDictData.buildCharArray != NULL );
						{
							F16Dot16 value;
							int index;
							
							value = gStackValues[ gNumStackValues + 0 ];
							index = gStackValues[ gNumStackValues + 1 ] >> 16;
							assert( index >= 0 && index < t->topDictData.lenBuildCharArray );
							t->topDictData.buildCharArray[index] = value;
						}
						break;
					
					case 21: 	/* get */
						LOG_CMD( "get", gNumStackValues );
						assert( gNumStackValues >= 1 );
						gNumStackValues--;
						assert( t->topDictData.buildCharArray != NULL );
						{
							F16Dot16 value;
							int index;
							
							index = gStackValues[ gNumStackValues + 0 ] >> 16;
							assert( index >= 0 && index < t->topDictData.lenBuildCharArray );
							value = t->topDictData.buildCharArray[index];
							gStackValues[ gNumStackValues + 0 ] = value;
						}
						gNumStackValues++;
						break;
					case 22: 	/* ifelse */
						LOG_CMD( "ifelse", gNumStackValues );
						assert( gNumStackValues >= 4 );
						gNumStackValues -= 4;
						
						gStackValues[ gNumStackValues + 0 ] =	gStackValues[ gNumStackValues + 2 ] <=
																gStackValues[ gNumStackValues + 3 ] ?
																gStackValues[ gNumStackValues + 0 ] :
																gStackValues[ gNumStackValues + 1 ];
						gNumStackValues++;
						break;
						
					case 23: 	/* random */
						LOG_CMD( "random", gNumStackValues );
						assert( gNumStackValues >= 0 );
						{
							F16Dot16 value;
							
							value   =	util_FixMul( gStackValues[ 0 ], gStackValues[ 1 ] ) ^
										util_FixMul( gStackValues[ 2 ], gStackValues[ 3 ] );
							value  ^= (~(gNumStackValues<<10) ^ gStackValues[ 4 ] );
							t->seed = (uint16)(58653 * t->seed + 13849);
							value  ^= t->seed;
							value  &= 0x0000ffff;
							value++; /* greater than 0 and less than or equal to one */
						
							gStackValues[ gNumStackValues + 0 ] = value;
							gNumStackValues++;
						}
						break;
					case 24: 	/* mul */
						LOG_CMD( "mul", gNumStackValues );
						assert( gNumStackValues >= 2 );
						gNumStackValues -= 2;
						
						gStackValues[ gNumStackValues + 0 ] = util_FixMul(	gStackValues[ gNumStackValues + 0 ], 
																			gStackValues[ gNumStackValues + 1 ] );
						gNumStackValues++;
						break;
					/* 25 is reserved */	
					case 26: 	/* sqrt */
						LOG_CMD( "sqrt", gNumStackValues );
						assert( gNumStackValues >= 1 );
						gNumStackValues--;
						{
							int loop = 0;					
							F16Dot16 square, root, old_root;
							square	= gStackValues[ gNumStackValues + 0 ];
							root 	= square;

							/* A Newton Raphson loop */
							do {
								root = ((old_root = root) + util_FixDiv( square, root ) + 1 ) >> 1;
							} while (old_root != root && loop++ < 10 );
							
							gStackValues[ gNumStackValues + 0 ] = root;
						}
						gNumStackValues++;
						break;
						
					case 27: 	/* dup */
						LOG_CMD( "dup", gNumStackValues );
						assert( gNumStackValues >= 1 );
						gNumStackValues--;
						
						/* gStackValues[ gNumStackValues + 0 ] = gStackValues[ gNumStackValues + 0 ]; */
						gStackValues[ gNumStackValues + 1 ] = gStackValues[ gNumStackValues + 0 ];
						
						gNumStackValues += 2;
						break;
					case 28: 	/* exch */
						LOG_CMD( "exch", gNumStackValues );
						assert( gNumStackValues >= 2 );
						gNumStackValues -= 2;
						{
							F16Dot16 val0, val1;
						
							val0 = gStackValues[ gNumStackValues + 0 ];
							val1 = gStackValues[ gNumStackValues + 1 ];
							gStackValues[ gNumStackValues + 0 ] = val1;
							gStackValues[ gNumStackValues + 1 ] = val0;
						}
							
						gNumStackValues += 2;
						break;
					case 29: 	/* index */
						LOG_CMD( "index", gNumStackValues );
						assert( gNumStackValues >= 2 );
						{
							long index;
							
							index = gStackValues[ gNumStackValues - 1 ] >> 16; /* top element */
							if ( index < 0 ) {
								index = 0;
							} else if ( index > gNumStackValues - 2 ) {
								index = gNumStackValues - 2; /* to avoid out of bounds access */
							}
							gStackValues[ gNumStackValues - 1 ] = gStackValues[ gNumStackValues - 2 - index ];
						}
						break;
					case 30: 	/* roll */
						LOG_CMD( "roll", gNumStackValues );
						assert( gNumStackValues >= 2 );
						gNumStackValues -= 2;
						
						{
							long N, J, i1, i2, tmp;
							
							N = gStackValues[ gNumStackValues + 0 ] >> 16;
							if ( N < 0 ) N = 0;
							J = gStackValues[ gNumStackValues + 1 ] >> 16;
							
							/* numi == gStackValues[ gNumStackValues - 1 - i ]; */
							if ( J >= 0 ) {
								for ( i1 = 0; i1 < J; i1++ ) {
									tmp = gStackValues[ gNumStackValues - 1 - 0];
									for ( i2 = 1; i2 < N; i2++ ) {
										gStackValues[ gNumStackValues - 1 - (i2 - 1)] = gStackValues[ gNumStackValues - 1 - i2];
									}
									gStackValues[ gNumStackValues - 1 - (N-1)] = tmp;
								}
							} else {
								J = -J;
								for ( i1 = 0; i1 < J; i1++ ) {
									tmp = gStackValues[ gNumStackValues - 1 - (N-1)];
									for ( i2 = N-1; i2 > 0; i2-- ) {
										gStackValues[ gNumStackValues - 1 - i2] = gStackValues[ gNumStackValues - 1 - (i2 - 1)];
									}
									gStackValues[ gNumStackValues - 1 - 0] = tmp;
								}
							}
							
						}
						break;
					/* 31 is reserved */	
					/* 32 is reserved */	
					/* 33 is reserved */
					
					case 34: 	/* hflex */
						LOG_CMD( "hflex", gNumStackValues );
						assert( gNumStackValues >= 7 );
						gNumStackValues -= 7;
						glyph_StartLine( t->glyph, x>>16, y>>16 );
						{
							
							dx1 = gStackValues[ gNumStackValues + 0 ];
							dy1 = 0;
							
							dx2 = gStackValues[ gNumStackValues + 1 ];
							dy2 = gStackValues[ gNumStackValues + 2 ];
							
							dx3 = gStackValues[ gNumStackValues + 3 ];
							dy3 = 0;
							
							dx4 = gStackValues[ gNumStackValues + 4 ];
							dy4 = 0;
							
							dx5 = gStackValues[ gNumStackValues + 5 ];
							dy5 = -dy2;
							
							dx6 = gStackValues[ gNumStackValues + 6 ];
							dy6 = 0;

							x += dx1; y += dy1;
							glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
							x += dx2; y += dy2;
							glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
							x += dx3; y += dy3;
							glyph_AddPoint( t->glyph, x>>16, y>>16, 1 );
							x += dx4; y += dy4;
							glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
							x += dx5; y += dy5;
							glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
							x += dx6; y += dy6;
							glyph_AddPoint( t->glyph, x>>16, y>>16, 1 );
							t->pointAdded = 1;
						}
						break;
					case 35: 	/* flex */
						LOG_CMD( "flex", gNumStackValues );
						assert( gNumStackValues >= 13 );
						gNumStackValues -= 13;
						glyph_StartLine( t->glyph, x>>16, y>>16 );
						{
							
							dx1 = gStackValues[ gNumStackValues + 0 ];
							dy1 = gStackValues[ gNumStackValues + 1 ];
							
							dx2 = gStackValues[ gNumStackValues + 2 ];
							dy2 = gStackValues[ gNumStackValues + 3 ];
							
							dx3 = gStackValues[ gNumStackValues + 4 ];
							dy3 = gStackValues[ gNumStackValues + 5 ];
							
							dx4 = gStackValues[ gNumStackValues + 6 ];
							dy4 = gStackValues[ gNumStackValues + 7 ];
							
							dx5 = gStackValues[ gNumStackValues + 8 ];
							dy5 = gStackValues[ gNumStackValues + 9 ];
							
							dx6 = gStackValues[ gNumStackValues + 10 ];
							dy6 = gStackValues[ gNumStackValues + 11 ];
							/* fd = gStackValues[ gNumStackValues + 12 ]; */

							x += dx1; y += dy1;
							glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
							x += dx2; y += dy2;
							glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
							x += dx3; y += dy3;
							glyph_AddPoint( t->glyph, x>>16, y>>16, 1 );
							x += dx4; y += dy4;
							glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
							x += dx5; y += dy5;
							glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
							x += dx6; y += dy6;
							glyph_AddPoint( t->glyph, x>>16, y>>16, 1 );
							t->pointAdded = 1;
						}
						break;
					case 36: 	/* hflex1 */
						LOG_CMD( "hflex1", gNumStackValues );
						assert( gNumStackValues >= 9 );
						gNumStackValues -= 9;
						glyph_StartLine( t->glyph, x>>16, y>>16 );
						{
							
							dx1 = gStackValues[ gNumStackValues + 0 ];
							dy1 = gStackValues[ gNumStackValues + 1 ];
							
							dx2 = gStackValues[ gNumStackValues + 2 ];
							dy2 = gStackValues[ gNumStackValues + 3 ];
							
							dx3 = gStackValues[ gNumStackValues + 4 ];
							dy3 = 0;
							/*---*/
							dx4 = gStackValues[ gNumStackValues + 5 ];
							dy4 = 0;
							
							dx5 = gStackValues[ gNumStackValues + 6 ];
							dy5 = gStackValues[ gNumStackValues + 7 ];
							
							dx6 = gStackValues[ gNumStackValues + 8 ];
							dy6 = -(dy1+dy2+dy5);

							x += dx1; y += dy1;
							glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
							x += dx2; y += dy2;
							glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
							x += dx3; y += dy3;
							glyph_AddPoint( t->glyph, x>>16, y>>16, 1 );
							x += dx4; y += dy4;
							glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
							x += dx5; y += dy5;
							glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
							x += dx6; y += dy6;
							glyph_AddPoint( t->glyph, x>>16, y>>16, 1 );
							t->pointAdded = 1;
						}
						break;
					case 37: 	/* flex1 */
						LOG_CMD( "flex1", gNumStackValues );
						assert( gNumStackValues >= 11 );
						gNumStackValues -= 11;
						glyph_StartLine( t->glyph, x>>16, y>>16 );
						{
							F16Dot16 sumDx, sumDy;
							F16Dot16 sumDxAbs, sumDyAbs;
							
							dx1 = gStackValues[ gNumStackValues + 0 ];
							dy1 = gStackValues[ gNumStackValues + 1 ];
							
							dx2 = gStackValues[ gNumStackValues + 2 ];
							dy2 = gStackValues[ gNumStackValues + 3 ];
							
							dx3 = gStackValues[ gNumStackValues + 4 ];
							dy3 = gStackValues[ gNumStackValues + 5 ];
							
							dx4 = gStackValues[ gNumStackValues + 6 ];
							dy4 = gStackValues[ gNumStackValues + 7 ];
							
							dx5 = gStackValues[ gNumStackValues + 8 ];
							dy5 = gStackValues[ gNumStackValues + 9 ];
							
							sumDx = dx1 + dx2 + dx3 + dx4 + dx5;
							sumDy = dy1 + dy2 + dy3 + dy4 + dy5;
							sumDxAbs = sumDx;
							sumDyAbs = sumDy;
							if ( sumDxAbs < 0 ) sumDxAbs = -sumDxAbs;
							if ( sumDyAbs < 0 ) sumDyAbs = -sumDyAbs;
							
							
							if ( sumDxAbs > sumDyAbs ) {
								/* First and last y are equal */
								dx6 = gStackValues[ gNumStackValues + 10 ];
								dy6 = -sumDy;
							} else {
								/* First and last x are equal */
								dx6 = -sumDx;
								dy6 = gStackValues[ gNumStackValues + 10 ];
							}

							x += dx1; y += dy1;
							glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
							x += dx2; y += dy2;
							glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
							x += dx3; y += dy3;
							glyph_AddPoint( t->glyph, x>>16, y>>16, 1 );
							x += dx4; y += dy4;
							glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
							x += dx5; y += dy5;
							glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
							x += dx6; y += dy6;
							glyph_AddPoint( t->glyph, x>>16, y>>16, 1 );
							t->pointAdded = 1;
						}
						break;
					/* 38 - 255 is reserved */	
					default:
						LOG_CMD( "12 - reserved", (int)v2);
						assert( false );
						break;
				}
				break;
			/* 13 is reserved */
			case 14:	/* endchar */
				/* endchar is the last command for normal characters and seac is the last one for accented characters */
				LOG_CMD( "endchar", gNumStackValues );
				/* Save state from registers. */
				t->x = x; /* set x, and y since this may mean return in a subroutine ! */
				t->y = y;
				t->gNumStackValues = gNumStackValues;
				pos = 0;
				if ( !t->widthDone && gNumStackValues > 0 ) {
					t->widthDone	= true;
					t->awx 			= t->privateDictData.nominalWidthX + (gStackValues[ pos ]>>16);
				}
				gNumStackValues = 0; /* clear the stack */
				break;
			/* 15 is reserved */
			case 16: 	/* blend */
				/* for k master designs produces n interpolated results value(s) from n*k arguments. */
				/* INPUT: k groups of n arguments, n */
				/* The values in the second a subsequent groups are expressed as deltas to the values in the first group. */
				LOG_CMD( "blend", gNumStackValues );
				assert( gNumStackValues >= 2 );
				{
					int k, n, i1, i2;
					
					k = t->topDictData.numMasters;
					
					gNumStackValues--;
					n = gStackValues[ gNumStackValues + 0 ] >> 16;
					gNumStackValues -= k * n;
					assert( gNumStackValues >= 0 );
					
					for ( i1 = 0; i1 < n; i1++ ) {
						F16Dot16 value = gStackValues[ gNumStackValues + i1 + 0 ];
						F16Dot16 weight;
						for ( i2 = 1; i2 < k; i2++ ) {
							weight = t->topDictData.defaultWeight[i2];
							value += util_FixMul( weight, gStackValues[ gNumStackValues + i1 + i2*k ]  );
						}
						gStackValues[ gNumStackValues + i1 + 0 ] = value;
					}
					gNumStackValues += n;
				}
				break;
			/* 17 is reserved */
				
			case 18: /* hstemhm */
			case 23: /* vstemhm */
				LOG_CMD( (v1 == 18 ? "hstemhm" : "vstemhm"), gNumStackValues );
				/* takes 2*n arguments */
				pos = 0;
				if ( !t->widthDone && (gNumStackValues & 1) ) {
					t->widthDone	= true;
					t->awx 			= t->privateDictData.nominalWidthX + (gStackValues[ pos++ ]>>16);
				}
				t->numStemHints += (gNumStackValues-pos)/2;
				gNumStackValues = 0;
				break;
			case 19: /* hintmask */
			case 20: /* cntrmask */
				LOG_CMD( "hintmask/cntrmask", gNumStackValues );
				pos = 0;
				if ( !t->widthDone && gNumStackValues > 0 ) {
					t->widthDone	= true;
					t->awx 			= t->privateDictData.nominalWidthX + (gStackValues[ pos++ ]>>16);
				}
				while ( pos < gNumStackValues ) {
					pos += 2; /* consume vstem hints */
					t->numStemHints++;
				}
				/* Consume the mask multibyte sequence */
				{
					int count = (t->numStemHints + 7) >> 3;
					
					LOG_CMD( "t->numStemHints", t->numStemHints );
					while ( count-- > 0 ) {
#if DEBUG
						v2 = ReadUnsignedByteMacro( in );
						LOG_CMD( "CONSUMED", (int)v2 );
#else
						Seek_InputStream(in, in->pos + 1L);
#endif
					}
				}
				
				gNumStackValues = 0;
				break;
			case 21:	/* rmoveto */
				LOG_CMD( "rmoveto", gNumStackValues );
				pos = 0;
				if ( !t->widthDone && gNumStackValues > 2 ) {
					t->widthDone	= true;
					t->awx 			= t->privateDictData.nominalWidthX + (gStackValues[ pos++ ]>>16);
				}
				assert( gNumStackValues - pos >= 2 );
				x += gStackValues[ pos++ ];
				y += gStackValues[ pos ];
				gNumStackValues = 0; /* clear the stack */
				if ( t->pointAdded ) {
					glyph_CloseContour( t->glyph );
				}
				break;
			case 22:	/* hmoveto */
				LOG_CMD( "hmoveto", gNumStackValues );
				pos = 0;
				if ( !t->widthDone && gNumStackValues > 1 ) {
					t->widthDone	= true;
					t->awx 			= t->privateDictData.nominalWidthX + (gStackValues[ pos++ ]>>16);
				}
				assert( gNumStackValues - pos >= 1 );
				x += gStackValues[ pos ];
				gNumStackValues = 0; /* clear the stack */
				if ( t->pointAdded ) {
					glyph_CloseContour( t->glyph );
				}
				break;
			case 24: /* rcurveline */
				LOG_CMD( "rcurveline", gNumStackValues );
				assert( gNumStackValues >= 8 );
				glyph_StartLine( t->glyph, x>>16, y>>16 );
				for ( pos = 0; pos+6 <= gNumStackValues; ) {
					x += gStackValues[ pos++ ];
					y += gStackValues[ pos++ ];
					glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
					x += gStackValues[ pos++ ];
					y += gStackValues[ pos++ ];
					glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
					x += gStackValues[ pos++ ];
					y += gStackValues[ pos++ ];
					glyph_AddPoint( t->glyph, x>>16, y>>16, 1 );
				}
				x += gStackValues[ pos++ ];
				y += gStackValues[ pos ];
				glyph_AddPoint( t->glyph, x>>16, y>>16, 1 );
				gNumStackValues = 0; /* clear the stack */
				t->pointAdded = 1;
				break;
			case 25: /* rlinecurve */
				LOG_CMD( "rlinecurve", gNumStackValues );
				assert( gNumStackValues >= 8 );
				glyph_StartLine( t->glyph, x>>16, y>>16 );

				for ( pos = 0; pos+6 < gNumStackValues; ) {
					x += gStackValues[ pos++ ];
					y += gStackValues[ pos++ ];
					glyph_AddPoint( t->glyph, x>>16, y>>16, 1 );
				}

				x += gStackValues[ pos++ ];
				y += gStackValues[ pos++ ];
				glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
				x += gStackValues[ pos++ ];
				y += gStackValues[ pos++ ];
				glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
				x += gStackValues[ pos++ ];
				y += gStackValues[ pos ];
				glyph_AddPoint( t->glyph, x>>16, y>>16, 1 );
				
				t->pointAdded   = 1;
				gNumStackValues = 0; /* clear the stack */
				break;
			case 26: /* vvcurveto */
				LOG_CMD( "vvcurveto", gNumStackValues );
				glyph_StartLine( t->glyph, x>>16, y>>16 );
				pos = 0;
				if ( gNumStackValues & 1 ) {
					x += gStackValues[ pos++ ];
				}
				while ( pos+4 <= gNumStackValues ) {
					y += gStackValues[ pos++ ];
					glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
					x += gStackValues[ pos++ ];
					y += gStackValues[ pos++ ];
					glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
					y += gStackValues[ pos++ ];
					glyph_AddPoint( t->glyph, x>>16, y>>16, 1 );
				}
				t->pointAdded   = 1;
				gNumStackValues = 0; /* clear the stack */
				break;
			case 27: /* hhcurveto */
				LOG_CMD( "hhcurveto", gNumStackValues );
				glyph_StartLine( t->glyph, x>>16, y>>16 );
				pos = 0;
				if ( gNumStackValues & 1 ) {
					y += gStackValues[ pos++ ];
				}
				while ( pos+4 <= gNumStackValues ) {
					x += gStackValues[ pos++ ];
					glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
					x += gStackValues[ pos++ ];
					y += gStackValues[ pos++ ];
					glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
					x += gStackValues[ pos++ ];
					glyph_AddPoint( t->glyph, x>>16, y>>16, 1 );
				}
				t->pointAdded   = 1;
				gNumStackValues = 0; /* clear the stack */
				break;
			case 28: /* shortint */
				{
					register F16Dot16 num;
					
					num = ReadUnsignedByteMacro( in );	/* 3 bytes 2 bytes 16.0 number */
					num <<= 8;
					num |= ReadUnsignedByteMacro( in );
					num <<= 16;
					if ( gNumStackValues < CFF_MAX_STACK ) {
						gStackValues[gNumStackValues++] = num;
					}
				}
				break;
				
			/* 29 see above around 10 */
			case 30:	/* vhcurveto */
				LOG_CMD( "vhcurveto", gNumStackValues );
				assert( gNumStackValues >= 4 );
				glyph_StartLine( t->glyph, x>>16, y>>16 );
				for ( pos = 0; pos+4 <= gNumStackValues; ) {
					x += 0;
					y += gStackValues[ pos++ ];
					glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
					x += gStackValues[ pos++ ];
					y += gStackValues[ pos++ ];
					glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
					x += gStackValues[ pos++ ];
					y += 0;
					if ( pos + 1 == gNumStackValues ) {
						y += gStackValues[ pos++ ];
					}
					glyph_AddPoint( t->glyph, x>>16, y>>16, 1 );
					
					if ( pos+4 > gNumStackValues ) break; /*****/
					
					x += gStackValues[ pos++ ];
					y += 0;
					glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
					x += gStackValues[ pos++ ];
					y += gStackValues[ pos++ ];
					glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
					x += 0;
					y += gStackValues[ pos++ ];
					if ( pos + 1 == gNumStackValues ) {
						x += gStackValues[ pos++ ];
					}
					glyph_AddPoint( t->glyph, x>>16, y>>16, 1 );
				
				}
				t->pointAdded   = 1;
				gNumStackValues = 0; /* clear the stack */
				break;
				
			case 31:	/* hvcurveto */
				LOG_CMD( "hvcurveto", gNumStackValues );
				assert( gNumStackValues >= 4 );
				glyph_StartLine( t->glyph, x>>16, y>>16 );
				for ( pos = 0; pos+4 <= gNumStackValues; ) {
					x += gStackValues[ pos++ ];
					y += 0;
					glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
					x += gStackValues[ pos++ ];
					y += gStackValues[ pos++ ];
					glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
					x += 0;
					y += gStackValues[ pos++ ];
					if ( pos + 1 == gNumStackValues ) {
						x += gStackValues[ pos++ ];;
					}
					glyph_AddPoint( t->glyph, x>>16, y>>16, 1 );
					
					if ( pos+4 > gNumStackValues ) break; /*****/
					
					x += 0;
					y += gStackValues[ pos++ ];
					glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
					x += gStackValues[ pos++ ];
					y += gStackValues[ pos++ ];
					glyph_AddPoint( t->glyph, x>>16, y>>16, 0 );
					x += gStackValues[ pos++ ];
					y += 0;
					if ( pos + 1 == gNumStackValues ) {
						y += gStackValues[ pos++ ];
					}
					glyph_AddPoint( t->glyph, x>>16, y>>16, 1 );
					
				}
				t->pointAdded   = 1;
				gNumStackValues = 0; /* clear the stack */
				break;
				
			default:
				LOG_CMD( "reserved cmd", (int)v1);
				assert( false );
				
			}
		} else {
			register F16Dot16 num;
			
			/* v1 == 28 is already handled */
			if ( v1 <= 246 ) {
				/* >= 32 => A number */
				num = v1 - 139; 					/* 1 byte: [-107, +107] */
				num <<= 16;
			} else if ( v1 <= 250 ) {
				v2 = ReadUnsignedByteMacro( in );
				num = ((v1-247)*256) + v2 + 108;	/* 2 bytes: [108, 1131] */
				num <<= 16;
			} else if ( v1 <= 254 ) {
				v2 = ReadUnsignedByteMacro( in );
				num = -((v1-251)*256) - v2 - 108;	/* 2 bytes: [-108, -1131] */
				num <<= 16;
			} else {
				/* v1 == 255 */					 	/* 5 bytes: +-16.16 bit signed number  */
				num = ReadUnsignedByteMacro( in );
				num <<= 8;
				num |= ReadUnsignedByteMacro( in );
				num <<= 8;
				num |= ReadUnsignedByteMacro( in );
				num <<= 8;
				num |= ReadUnsignedByteMacro( in );
			}
#ifdef OLDDBUG
if ( debugOn ) {
	printf("stack: ");
}
#endif			
			if ( gNumStackValues < CFF_MAX_STACK ) {
				gStackValues[gNumStackValues++] = num;
			}
		}
	}
	/* Save state from registers. */
	t->x = x;
	t->y = y;
	t->gNumStackValues = gNumStackValues;
}




uint16 tsi_T2GetGlyphIndex( CFFClass *t, uint16 charCode )
{
	SIDCode stringID;
	uint16 gIndex = 0;
	
	t->glyphExists = true;
	if (!t->topDictData.isCIDKeyed)
	{
		if (charCode < 256)
			stringID = t->charCodeToSID[charCode];
		else
		{
			t->glyphExists = false;
			stringID = 0;	/* .notdef */
		}
			
		if (stringID && !BSearchCTbl ( t, &gIndex, stringID))
		{
			t->glyphExists = false;
			gIndex = 0;	/* .notdef */
		}
	}
	else
		{
			stringID = charCode;
			if (stringID && !BSearchCTbl ( t, &gIndex, stringID))
			{
				t->glyphExists = false;
				gIndex = 0;	/* .notdef */
			}
		}

	return gIndex; /*****/
}

static void BuildT2MetricsEtc( CFFClass *t )
{
	GlyphClass *glyph;
	uint16 aw, ah;
	uint16 fGIndex, gGIndex;
	F16Dot16 m00;

	t->NumCharStrings	= t->CharStrings->count;
	t->upem				= 1000;
	t->upem = util_FixDiv(1000L << 16, t->topDictData.m00) >> 16;
	m00 = t->topDictData.m00;
	t->topDictData.m00 = util_FixDiv(t->topDictData.m00, m00);
	t->topDictData.m01 = util_FixDiv(t->topDictData.m01, m00);
	t->topDictData.m10 = util_FixDiv(t->topDictData.m10, m00);
	t->topDictData.m11 = util_FixDiv(t->topDictData.m11, m00);

	t->maxPointCount	= 0;
	t->ascent			= 0x7fff;
	t->descent			= 0x7fff;
	t->italicAngle = t->topDictData.italicAngle;

	fGIndex = tsi_T2GetGlyphIndex( t, 'f' );
	gGIndex = tsi_T2GetGlyphIndex( t, 'g' );
	if ( t->ascent == 0x7fff )
	{
		if (fGIndex)
		{
			glyph = tsi_T2GetGlyphByIndex( t, fGIndex, &aw, &ah);
			if (glyph)
			{
				t->ascent = GetGlyphYMax( glyph );
				Delete_GlyphClass( glyph );
			}
		}
	}
	if ( t->descent == 0x7fff )
	{
		if (gGIndex)
		{
			glyph = tsi_T2GetGlyphByIndex( t, gGIndex, &aw, &ah);
			if (glyph)
			{
				t->descent = GetGlyphYMin( glyph );
				Delete_GlyphClass( glyph );
			}
		}
	}
	t->advanceWidthMax = t->upem;
	t->maxPointCount = 32;

	if ( t->ascent == 0x7fff )  t->ascent  =  750;
	if ( t->descent == 0x7fff ) t->descent = -250;
	t->lineGap = t->upem - (t->ascent - t->descent);
	if ( t->lineGap < 0 ) t->lineGap = 0;
}

uint16 FF_GetAW_CFFClass( void *param1, register uint16 index )
{
	register CFFClass *t = (CFFClass *)param1;
	register uint16 value = 0;
	uint16 aw, ah;
	GlyphClass *glyph;
	glyph = tsi_T2GetGlyphByIndex( t, index, &aw, &ah);
	if (glyph)
	{
		value = aw;
		Delete_GlyphClass( glyph );
	}
	return value; /*****/
}

/*** Begin Predefined Charsets ***/
/* Maps gIndex -> SID */

static SIDCode ISOAdobeSID[] = {1,228,
							0, 0};
							
static SIDCode ExpertSID[] 	 = {1,	1,
							229,238,
							13,15,
							99,99,
							239,248,
							27,28,
							249,266,
							109,110,
							267,318,
							158,158,
							155,155,
							163,163,
							319,326,
							150,150,
							164,164,
							169,169,
							327,378,
							0, 	0};

static SIDCode ExpertSubsetSID[] = {	1,	1,
									231, 232,
									235, 238,
									13, 15,
									99, 99,
									239, 248,
									27, 28,
									249, 251,
									253, 266,
									109, 110,
									267, 270,
									272, 272,
									300, 302,
									305, 305,
									314, 315,
									158, 158,
									155, 155,
									163, 163,
									320, 326,
									150, 150,
									164, 164,
									169, 169,
									327, 346,
									0, 	0};

/*** End Predefined Charsets ***/
/*** Begin Predefined Encodings ***/
/* Maps charcode to ->SID */
static uint16 standarEncodingData[] = { 0,  31,  0,   0, 	/* charcode: 0-31    maps to the SID: range 0-0 */
										32, 126, 1,   95, 	/* charcode: 32-126  maps to the SID: range 1-95 */
										127,160, 0,   0, 	/* charcode: 127-160 maps to the SID: range 0-0 */
										161,175, 96,  110,	/* charcode: 127-160 maps to the SID: range 0-0 */
										176,176, 0,   0, 
										177,180, 111, 114,
										181,181, 0,   0, 
										182,189, 115, 122, 
										190,190, 0,   0, 
										191,191, 123, 123, 
										192,192, 0,   0, 
										193,200, 124, 131, 
										201,201, 0,   0, 
										202,203, 132, 133, 
										204,204, 0,   0, 
										205,208, 134, 137, 
										209,224, 0,   0, 
										225,225, 138, 138, 
										226,226, 0,   0, 
										227,227, 139, 139, 
										228,231, 0,   0, 
										232,235, 140, 143, 
										236,240, 0,   0, 
										241,241, 144, 144, 
										242,244, 0,   0, 
										245,245, 145, 145, 
										245,245, 145, 145, 
										246,247, 0,    0, 
										248,251, 146,  149, 
										252,255, 0,    0 }; 

static uint16 expertEncodingData[] = {   0, 31, 0,   0, 	/* charcode: 0-31    maps to the SID: range 0-0 */
										32, 32, 1,   1, 	/* charcode: 32-32   maps to the SID: range 1-1 */
										33, 34, 229, 230, 	/* charcode: 33-34   maps to the SID: range 229-230 */
										36, 43, 231, 238, 	
										44, 46,  13, 15, 	
										47, 47,  99, 99, 	
										48, 57, 239, 248, 	
										58, 59,  27,  28, 	
										60, 63,  249, 252,
										64, 64,    0,   0,
										65, 69,  253, 257,
										70, 72,    0,   0,
										73, 73,  258, 258,
										74, 75,    0,   0,
										76, 79,  259, 262,
										80, 81,    0,   0,
										82, 84,  263, 265,
										85, 85,    0,   0,
										86, 86,  266, 266,
										87, 88,  109, 110,
										89, 91,  267, 269,
										92, 92,    0,   0,
										93, 126, 270, 303,
										127, 160,   0,   0,
										161, 163, 304, 306,
										164, 165,   0,   0,
										166, 170, 307, 311,
										171, 171,   0,   0,
										172, 172, 312, 312,
										173, 174,   0,   0,
										175, 175, 313, 313,
										176, 177,   0,   0,
										178, 179, 314, 315,
										180, 181,   0,   0,
										182, 184, 316, 318,
										185, 187,   0,   0,
										188, 188, 158, 158,
										189, 189, 155, 155,
										190, 190, 163, 163,
										191, 197, 319, 325,
										198, 199,   0,   0,
										200, 200, 326, 326,
										201, 201, 150, 150,
										202, 202, 164, 164,
										203, 203, 169, 169,
										204, 255, 327, 378 };
										
										
/*** End Predefined Encodings ***/


static int T2stringsAreEqual( void *privptr, char *str, uint16 n)
{
CFFClass *t = (CFFClass *)privptr;
int isEqual = false;
uint8 *pString;
	/* return true if str == Strings[n] */
	if (n < nStdStrings)
	{
		isEqual = (strcmp(str, StdSIDStrings[n]) == 0);
	}
	else
	{
		pString = tsi_GetCFFData( t->string, t->in, n - nStdStrings );
		if (pString)
		{
			isEqual = (strcmp((const char *)pString, (const char *)str) == 0);
			tsi_DeAllocMem( t->mem, (char *)pString );
		}
	}
	return isEqual;
}

static void BuildT2CMAP( CFFClass *t )
{
	long i, j, k;
	uint8 format, format7F;
	SIDCode	aSID;
	t->NumCharStrings	= t->CharStrings->count;

	t->T2_CMAPTable		= (cmap_t *)tsi_AllocMem( t->mem, sizeof(cmap_t) * t->NumCharStrings );

	for ( i = 0; i < 256; i++ ) {
		t->charCodeToSID[i] = 0xffff; /* impossible code */
	}

/* gIndex To SID */
	if ( t->topDictData.charset < 3 ) {
		SIDCode *range = NULL, first, last;
		
		
		/* A predefined charset */
		switch ( t->topDictData.charset ) {
		case 0:
			range = ISOAdobeSID;		/* 0 = ISOAdobe */
			break;
		case 1:
			range = ExpertSID;			/* 1 = Expert */
			break;
		case 2:
			range = ExpertSubsetSID;	/* 2 = ExpertSubset */ 
			break;
		}
		i = 0;
		t->T2_CMAPTable[i].glyphIndex = (uint16)i;
		t->T2_CMAPTable[i++].IDCode = 0;	/* notdef */

		for ( k = 0; i < t->NumCharStrings; k += 2 ) {
			first = range[k];
			last  = range[k+1];
			assert( last >= first );
			if ( first == 0 && last == 0 ) break; /*****/
			for ( j = first; j <= last && i < t->NumCharStrings; j++ ) {
				t->T2_CMAPTable[i].glyphIndex = (uint16)i;
				t->T2_CMAPTable[i++].IDCode = (uint16)j;
			}
		}
	} else {
		Seek_InputStream( t->in, t->cffOffset + (uint32)t->topDictData.charset );
		format = ReadUnsignedByteMacro( t->in );
			

		i = 0;
		t->T2_CMAPTable[i].glyphIndex = (uint16)i;
		t->T2_CMAPTable[i++].IDCode = 0;	/* notdef */
		if ( format == 0 ) {
			while ( i < t->NumCharStrings ) {
				aSID = (uint16)ReadInt16( t->in );
				t->T2_CMAPTable[i].glyphIndex = (uint16)i;
				t->T2_CMAPTable[i++].IDCode = aSID;
			}
		} else if ( format == 1 || format == 2 ) {
			SIDCode 	first;
			uint16	nLeft; /* could be uint8 or uint16 */
			
			while ( i < t->NumCharStrings ) {
				first = READ_SID( t->in );
				nLeft = (uint16)(format == 1 ? (uint16)ReadUnsignedByteMacro( t->in ) : (uint16)ReadInt16( t->in ));

				for ( j = 0; j <= nLeft && i < t->NumCharStrings ; j++ ) {
					t->T2_CMAPTable[i].glyphIndex = (uint16)i;
					t->T2_CMAPTable[i++].IDCode = (uint16)(first + j);
				}
			}
		} else {
			assert( false );
		}
	}
	
/* ccode -> SID, or gIndex-> charcode */
	if ( !t->topDictData.isCIDKeyed && t->topDictData.Encoding < 2 ) {
		uint16 firstCode, lastCode;
		SIDCode firstSID, lastSID;
		
		uint16 *encodingData = t->topDictData.Encoding == 0 ? standarEncodingData : expertEncodingData;

		i = 0;
		do {
			firstCode = encodingData[i+0];
			lastCode  = encodingData[i+1];
			firstSID  = encodingData[i+2];
			lastSID   = encodingData[i+3]; i += 4;
			
			if ( firstSID == lastSID ) {
				for ( j = firstCode; j <= lastCode; j++ ) {
					/* ccode j maps to SID firstSID */
					t->charCodeToSID[j] = firstSID;
				}
			} else {
				assert( (lastCode-firstCode) == (lastSID-firstSID) );
				for ( k = 0, j = firstCode; j <= lastCode; j++, k++ ) {
					/* ccode j maps to SID firstSID + k */
					t->charCodeToSID[j] = (uint16)(firstSID + k);
				}
			}
		
		} while ( lastCode < 255 );
		
	} else if (!t->topDictData.isCIDKeyed) {
		Seek_InputStream( t->in, t->cffOffset + (uint32)t->topDictData.Encoding );
		format = ReadUnsignedByteMacro( t->in );
		
		format7F = (uint8)(format & 0x7f);
		
		
		
		if ( format7F == 0 ) {
			uint8 nCodes, ccode;
			
			nCodes = ReadUnsignedByteMacro( t->in );
			for ( i = 0; i < nCodes; i++ ) {
				ccode = ReadUnsignedByteMacro( t->in );
				t->charCodeToSID[ccode] = t->T2_CMAPTable[i+1].IDCode;
			}
		} else if ( format7F == 1 ) {
			uint8 nRanges, first, nLeft,ccode;
			
			i = 0;
			nRanges = ReadUnsignedByteMacro( t->in );
			/* t->gIndexToCharCode[0] = 0xffff; */
			for ( j = 0; j < nRanges && i < 255; j++ ) {
				first = ReadUnsignedByteMacro( t->in );
				nLeft = ReadUnsignedByteMacro( t->in );
				for ( k = 0; k <= nLeft && i < 255; k++ ) {
					ccode =  (uint8)(first + k);
					i++;
					t->charCodeToSID[ccode] = t->T2_CMAPTable[i].IDCode;
				}
			}
		} else {
			assert( false );;
		}
		if ( format & 0x80 ) {
			/* supplemental encoding data. */
			uint8 nSups, ccode;
			
			nSups = ReadUnsignedByteMacro( t->in );
			for ( i = 0; i < nSups; i++ ) {
				ccode = ReadUnsignedByteMacro( t->in );
				t->charCodeToSID[ccode] = READ_SID(t->in);
			}
		}
	}
	

	/* now sort the t->T2_CMAPTable[] by IDCode */
	ShellSortT2Cmap((cmap_t *)&t->T2_CMAPTable[0], (int)t->NumCharStrings);

	if (t->topDictData.isCIDKeyed)
	{
		t->firstCharCode = t->T2_CMAPTable[0].IDCode;
		t->lastCharCode = t->T2_CMAPTable[t->NumCharStrings - 1].IDCode;
	}	
}



CFFClass *tsi_NewCFFClassOTF( tsiMemObject *mem, InputStream *in, int32 fontNum , uint32 cffOffest)
{
	return _tsi_NewCFFClass( mem, in, fontNum, true , cffOffest);
}

CFFClass *tsi_NewCFFClass( tsiMemObject *mem, InputStream *in, int32 fontNum )
{
	return _tsi_NewCFFClass( mem, in, fontNum, false , (uint32)0);
}

static CFFClass *_tsi_NewCFFClass( tsiMemObject *mem, InputStream *in, int32 fontNum, uint8 isOpenType, uint32 cffOffset)
{
	register CFFClass *t;
	Card8	tmpCard8;
	int ii;
	uint8 *pString;

	/*** Read the header ***/
	Seek_InputStream( in, cffOffset);
	tmpCard8 = ReadUnsignedByteMacro( in );
	/* See if we understand this version */
	tsi_Assert( mem, tmpCard8 == 1, T2K_UNKNOWN_CFF_VERSION );
	
	t = (CFFClass *)tsi_AllocMem( mem, sizeof( CFFClass ) );
	t->mem		= mem;
	t->major	= tmpCard8;
	t->minor	= ReadUnsignedByteMacro( in );
	t->hdrSize	= ReadUnsignedByteMacro( in );
	t->offSize 	= ReadUnsignedByteMacro( in );
	t->fontNum	= fontNum;
	t->cffOffset = (uint32)cffOffset;
	
	/* Skip data from future formats that we do not understand. */
	Seek_InputStream( in, t->cffOffset + t->hdrSize );
	/*** Done with the header ***/
	
	t->in = in;
	
	t->name		= tsi_NewCFFIndexClass( mem, in );	/* Name Index */
	t->topDict	= tsi_NewCFFIndexClass( mem, in );	/* Top DICT Index */
	t->string	= tsi_NewCFFIndexClass( mem, in );	/* String Index */

	t->gSubr	= tsi_NewCFFIndexClass( mem, in );	/* Global Subr Index */

	if ( t->gSubr->count < 1240 ) {
		t->gSubrBias = 107;
	} else if ( t->gSubr->count < 33900 ) {
		t->gSubrBias = 1131;
	} else {
		t->gSubrBias = 32768;
	}

	t->firstCharCode = 0x00;
	t->lastCharCode = 0xff;
	
	tsi_ParseCFFTopDict( t->topDict, in, &t->topDictData, fontNum );
	
	
	
	
	Seek_InputStream( in, t->cffOffset + (uint32)t->topDictData.Charstrings );

	t->CharStrings = tsi_NewCFFIndexClass( mem, in );	/* CharStrings */

#if 0
	/* Test DecryptData(): file must be in RAM for this to work! Only a test! */
	if (t->topDictData.isCIDKeyed)
		{
		int ii;
		long size;
		uint8 *p;
		for (ii = 0; ii < t->CharStrings->count; ii++)
			{
			p = (uint8 *)&in->privateBase[t->CharStrings->baseDataOffset + t->CharStrings->offsetArray[ii]];
			size = t->CharStrings->offsetArray[ii + 1] - t->CharStrings->offsetArray[ii];
			if (size >= 4)
				{
				DecryptData( p, size);
				DecryptData( p, size);
				}
			p += size;
			}
		}
#endif	
	/* Private DICT, per-font */
	tsi_ParsePrivateDictData( t );
	
	/* Local soubroutines, per-font */

	t->lSubr = NULL;
	t->lSubrBias = 0;
	if ( t->privateDictData.Subr != 0 ) {
		Seek_InputStream( in, (uint32)t->privateDictData.SubrOffset );
		t->lSubr = tsi_NewCFFIndexClass( mem, in );	/* Local Subr Index */
		if ( t->lSubr->count < 1240 ) {
			t->lSubrBias = 107;
		} else if ( t->lSubr->count < 33900 ) {
			t->lSubrBias = 1131;
		} else {
			t->lSubrBias = 32768;
		}
	}
	if (!isOpenType)
	{
		BuildT2CMAP( t );
		BuildT2MetricsEtc( t );
	}
	/* create the table for resolving PSNames to SIDCodes: */
	t->T2_StringsHash = New_hashClass( t->mem,
										nStdStrings + t->string->count,
										(FF_STRS_ARE_EQUAL_FUNC_PTR) T2stringsAreEqual,
										t );
	/* fill 'er up, regular: */
	for (ii = 0; ii < nStdStrings; ii++)
	{
		pString = (uint8 *)StdSIDStrings[ii];
		put_hashClass( t->T2_StringsHash, (uint16)ii, (uint16)ii, (char *)pString  );
	}
	
	/* fill 'er up, custom: */
	for (ii = 0; ii < t->string->count; ii++)
	{
		pString = tsi_GetCFFData( t->string, t->in, ii );
		if (pString)
		{
			put_hashClass( t->T2_StringsHash, (uint16)(ii + nStdStrings), (uint16)(ii + nStdStrings), (char *)pString  );
			tsi_DeAllocMem( t->mem, (char *)pString );
		}
	}
	/* create the table for resolving SIDCodes to CharCodes: */
	t->T2_SIDToCharCodeHash = New_hashClass( t->mem,
										256,
										(FF_STRS_ARE_EQUAL_FUNC_PTR) NULL,
										t );
	/* populate: */
	for (ii = 0; ii < 256; ii++)
	{
		put_hashClass(t->T2_SIDToCharCodeHash, (uint16)t->charCodeToSID[ii], (uint16)ii, NULL);
	}
	
	return t; /*****/
}



void tsi_DeleteCFFClass( CFFClass *t )
{
	if ( t != NULL ) {
		tsi_DeleteCFFIndexClass( t->name );
		
		tsi_DeAllocMem( t->mem, (char *)t->topDictData.buildCharArray );
		tsi_DeleteCFFIndexClass( t->topDict );
		
		tsi_DeleteCFFIndexClass( t->string );
		tsi_DeleteCFFIndexClass( t->gSubr );
		tsi_DeleteCFFIndexClass( t->CharStrings );
		tsi_DeleteCFFIndexClass( t->lSubr );
		Delete_hashClass( t->T2_StringsHash );
		Delete_hashClass( t->T2_SIDToCharCodeHash );
		tsi_DeAllocMem( t->mem, (char *)t->T2_CMAPTable );
		tsi_DeAllocMem( t->mem, (char *)t );
	}
}


/*
 *
 */
GlyphClass *tsi_T2GetGlyphByIndex( CFFClass *t, uint16 index, uint16 *aWidth, uint16 *aHeight )
{
	int byteCount, limit = t->CharStrings->count;
	GlyphClass *glyph;
	
	t->glyph = New_EmptyGlyph( t->mem, 0, 0, 0, 0 );
	t->glyph->curveType = 3;
	t->gNumStackValues = 0;

	if ( /* index >= 0 && */ index < limit ) {
		/* Initialize the Type2BuildChar state. */
		t->awx 	= t->privateDictData.defaultWidthX;
		t->awy 	= 0;
		t->lsbx = 0;
		t->lsby = 0;
		t->x = 0;
		t->y = 0;
		t->numStemHints	= 0;
		t->pointAdded	= 0;
		t->widthDone	= false;
		
		/* Find the data */
		byteCount = (int)(t->CharStrings->offsetArray[index+1] - t->CharStrings->offsetArray[index]);
		Seek_InputStream( t->in, t->CharStrings->baseDataOffset + t->CharStrings->offsetArray[index] );

		/* Go! */
		Type2BuildChar( t, t->in, byteCount, 0 );
		
		/* Wrap up the contour */
		glyph_CloseContour( t->glyph );
		LOG_CMD("ep[contourCount-1] = ", t->glyph->ep[t->glyph->contourCount-1] );
		LOG_CMD("t->glyph->contourCount", t->glyph->contourCount );
		t->lsbx = GetGlyphXMin( t->glyph );
	}
	glyph = t->glyph;
	
	glyph->ooy[glyph->pointCount + 0] = 0;
	glyph->oox[glyph->pointCount + 0] = 0;
	
	glyph->ooy[glyph->pointCount + 1] = (short)t->awy;
	glyph->oox[glyph->pointCount + 1] = (short)t->awx;
	
	*aWidth = (uint16)t->awx;
	{
		uint16 ah;

#		if SbPtCount >= 4
		{
			long xMid = (glyph->oox[glyph->pointCount + 0] + glyph->oox[glyph->pointCount + 1]) >> 1;
			int16 tsb, ymax, *ooy = glyph->ooy;
			int i, limit = glyph->pointCount;
			
			ymax = ooy[0];
			for ( i = 1; i < limit; i++ ) {
				if ( ooy[i] > ymax ) ymax = ooy[i];
			}
			/* Simulate */
			ah 	= (uint16)t->upem;
			tsb = (int16)(ah/10);
			
			ooy[glyph->pointCount + 2] 			= (short)(ymax + tsb);
			glyph->oox[glyph->pointCount + 2]	= (short)xMid;
			
			ooy[glyph->pointCount + 3]			= (short)(ooy[glyph->pointCount + 2] - ah);
			glyph->oox[glyph->pointCount + 3]	= (short)xMid;
		}
#		else
		ah = 0;
#		endif
		*aHeight = ah;
	}	
	t->glyph = NULL;
	FlipContourDirection( glyph );
	/* Return the glyph data */
	return glyph; /*****/
}

static void ShellSortT2Cmap(cmap_t *pairs, int num_pair)
{
	int i, j, incr = num_pair/2;
	uint32 tempindex;
	uint16  tempvalue;

	while (incr > 0)
	{
		for (i = incr; i < num_pair; i++)
		{
			j = i - incr;
			while (j >= 0)
			{
				if (pairs[j].IDCode > pairs[j+incr].IDCode)
				{
					/* swap the two numbers */
					tempindex = pairs[j].IDCode;
					tempvalue = pairs[j].glyphIndex;
	
					pairs[j].IDCode = pairs[j+incr].IDCode;
					pairs[j].glyphIndex = pairs[j+incr].glyphIndex;
	
					pairs[j+incr].IDCode = (uint16)tempindex;
					pairs[j+incr].glyphIndex = tempvalue;
	
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

/*************************************************************************************
*	BSearchCTbl()
*	RETURNS:	true on success, false on failure.
*				If successful, *indexPtr contains index of where found.
*************************************************************************************/
static uint8 BSearchCTbl ( CFFClass *t, uint16 *indexPtr, uint16 theValue)
{
   signed long    left, right, middle;
   int16    result;
   int32 nElements = t->NumCharStrings;

   left = 0;
   right = nElements - 1;

   while ( right >= left )
   {
      middle = (left + right)/2;
 
      if (theValue == t->T2_CMAPTable[middle].IDCode)
      	result = 0;
      else if (theValue < t->T2_CMAPTable[middle].IDCode)
      	result = -1;
      else
      	result = 1;
      	
      if ( result == -1 )
         right = middle - 1;
      else
         left = middle + 1;
      if ( result == 0 )
      {
         *indexPtr = (uint16)t->T2_CMAPTable[middle].glyphIndex;
         return ( true );
      }
   }
if (debugOn)
	{
	int idx, offset = -5;
	printf("Failed to find 0x%4x, here's the neighboorhood:\n", (int)theValue);
	while (offset < 6)
		{
		idx = left + offset;
		printf("t->charMap[%d].charCode = 0x%4x\n", (int)idx, (int)t->T2_CMAPTable[idx].IDCode);
		offset++;
		}
	}
   return ( false );
}

void tsi_T2ListChars(void *userArg, CFFClass *t, void *ctxPtr, int ListCharsFn(void *userArg, void *p, uint16 code))
{
int ii, checkStop = 0;
	for (ii = 0; !checkStop && (ii < 256); ii++)
	{
		if (t->charCodeToSID[ii] != 0xffff)
		{
			checkStop = ListCharsFn(userArg, ctxPtr, (uint16)ii);
		}
	}
}


/*
 * converts PSName to ccode for CFF fonts
*/
uint16 tsi_T2PSName2CharCode( CFFClass *t, char *PSName)
{
uint16 aSid = 0;
int found;
uint16 ccode = 0;

	found = get_using_str_hashClass( t->T2_StringsHash, PSName, &aSid );
	if (found)
		 found = get_using_uint16_hashClass( t->T2_SIDToCharCodeHash, aSid, (uint16 *) &ccode );
	return ccode;
}


static char *StdSIDStrings[nStdStrings] =
{
	".notdef",
	"space",
	"exclam",
	"quotedbl",
	"numbersign",
	"dollar",
	"percent",
	"ampersand",
	"quoteright",
	"parenleft",
	"parenright",
	"asterisk",
	"plus",
	"comma",
	"hyphen",
	"period",
	"slash",
	"zero",
	"one",
	"two",
	"three",
	"four",
	"five",
	"six",
	"seven",
	"eight",
	"nine",
	"colon",
	"semicolon",
	"less",
	"equal",
	"greater",
	"question",
	"at",
	"A",
	"B",
	"C",
	"D",
	"E",
	"F",
	"G",
	"H",
	"I",
	"J",
	"K",
	"L",
	"M",
	"N",
	"O",
	"P",
	"Q",
	"R",
	"S",
	"T",
	"U",
	"V",
	"W",
	"X",
	"Y",
	"Z",
	"bracketleft",
	"backslash",
	"bracketright",
	"asciicircum",
	"underscore",
	"quoteleft",
	"a",
	"b",
	"c",
	"d",
	"e",
	"f",
	"g",
	"h",
	"i",
	"j",
	"k",
	"l",
	"m",
	"n",
	"o",
	"p",
	"q",
	"r",
	"s",
	"t",
	"u",
	"v",
	"w",
	"x",
	"y",
	"z",
	"braceleft",
	"bar",
	"braceright",
	"asciitilde",
	"exclamdown",
	"cent",
	"sterling",
	"fraction",
	"yen",
	"florin",
	"section",
	"currency",
	"quotesingle",
	"quotedblleft",
	"guillemotleft",
	"guilsinglleft",
	"guilsinglright",
	"fi",
	"fl",
	"endash",
	"dagger",
	"daggerdbl",
	"periodcentered",
	"paragraph",
	"bullet",
	"quotesinglbase",
	"quotedblbase",
	"quotedblright",
	"guillemotright",
	"ellipsis",
	"perthousand",
	"questiondown",
	"grave",
	"acute",
	"circumflex",
	"tilde",
	"macron",
	"breve",
	"dotaccent",
	"dieresis",
	"ring",
	"cedilla",
	"hungarumlaut",
	"ogonek",
	"caron",
	"emdash",
	"AE",
	"ordfeminine",
	"Lslash",
	"Oslash",
	"OE",
	"ordmasculine",
	"ae",
	"dotlessi",
	"lslash",
	"oslash",
	"oe",
	"germandbls",
	"onesuperior",
	"logicalnot",
	"mu",
	"trademark",
	"Eth",
	"onehalf",
	"plusminus",
	"Thorn",
	"onequarter",
	"divide",
	"brokenbar",
	"degree",
	"thorn",
	"threequarters",
	"twosuperior",
	"registered",
	"minus",
	"eth",
	"multiply",
	"threesuperior",
	"copyright",
	"Aacute",
	"Acircumflex",
	"Adieresis",
	"Agrave",
	"Aring",
	"Atilde",
	"Ccedilla",
	"Eacute",
	"Ecircumflex",
	"Edieresis",
	"Egrave",
	"Iacute",
	"Icircumflex",
	"Idieresis",
	"Igrave",
	"Ntilde",
	"Oacute",
	"Ocircumflex",
	"Odieresis",
	"Ograve",
	"Otilde",
	"Scaron",
	"Uacute",
	"Ucircumflex",
	"Udieresis",
	"Ugrave",
	"Yacute",
	"Ydieresis",
	"Zcaron",
	"aacute",
	"acircumflex",
	"adieresis",
	"agrave",
	"aring",
	"atilde",
	"ccedilla",
	"eacute",
	"ecircumflex",
	"edieresis",
	"egrave",
	"iacute",
	"icircumflex",
	"idieresis",
	"igrave",
	"ntilde",
	"oacute",
	"ocircumflex",
	"odieresis",
	"ograve",
	"otilde",
	"scaron",
	"uacute",
	"ucircumflex",
	"udieresis",
	"ugrave",
	"yacute",
	"ydieresis",
	"zcaron",
	"exclamsmall",
	"Hungarumlautsmall",
	"dollaroldstyle",
	"dollarsuperior",
	"ampersandsmall",
	"Acutesmall",
	"parenleftsuperior",
	"parenrightsuperior",
	"twodotenleader",
	"onedotenleader",
	"zerooldstyle",
	"oneoldstyle",
	"twooldstyle",
	"threeoldstyle",
	"fouroldstyle",
	"fiveoldstyle",
	"sixoldstyle",
	"sevenoldstyle",
	"eightoldstyle",
	"nineoldstyle",
	"commasuperior",
	"threequartersemdash",
	"periodsuperior",
	"questionsmall",
	"asuperior",
	"bsuperior",
	"centsuperior",
	"dsuperior",
	"esuperior",
	"isuperior",
	"lsuperior",
	"msuperior",
	"nsuperior",
	"osuperior",
	"rsuperior",
	"ssuperior",
	"tsuperior",
	"ff",
	"ffi",
	"ffl",
	"parenleftinferior",
	"parenrightinferior",
	"Circumflexsmall",
	"hyphensuperior",
	"Gravesmall",
	"Asmall",
	"Bsmall",
	"Csmall",
	"Dsmall",
	"Esmall",
	"Fsmall",
	"Gsmall",
	"Hsmall",
	"Ismall",
	"Jsmall",
	"Ksmall",
	"Lsmall",
	"Msmall",
	"Nsmall",
	"Osmall",
	"Psmall",
	"Qsmall",
	"Rsmall",
	"Ssmall",
	"Tsmall",
	"Usmall",
	"Vsmall",
	"Wsmall",
	"Xsmall",
	"Ysmall",
	"Zsmall",
	"colonmonetary",
	"onefitted",
	"rupiah",
	"Tildesmall",
	"exclamdownsmall",
	"centoldstyle",
	"Lslashsmall",
	"Scaronsmall",
	"Zcaronsmall",
	"Dieresissmall",
	"Brevesmall",
	"Caronsmall",
	"Dotaccentsmall",
	"Macronsmall",
	"figuredash",
	"hypheninferior",
	"Ogoneksmall",
	"Ringsmall",
	"Cedillasmall",
	"questiondownsmall",
	"oneeighth",
	"threeeighths",
	"fiveeighths",
	"seveneighths",
	"onethird",
	"twothirds",
	"zerosuperior",
	"foursuperior",
	"fivesuperior",
	"sixsuperior",
	"sevensuperior",
	"eightsuperior",
	"ninesuperior",
	"zeroinferior",
	"oneinferior",
	"twoinferior",
	"threeinferior",
	"fourinferior",
	"fiveinferior",
	"sixinferior",
	"seveninferior",
	"eightinferior",
	"nineinferior",
	"centinferior",
	"dollarinferior",
	"periodinferior",
	"commainferior",
	"Agravesmall",
	"Aacutesmall",
	"Acircumflexsmall",
	"Atildesmall",
	"Adieresissmall",
	"Aringsmall",
	"AEsmall",
	"Ccedillasmall",
	"Egravesmall",
	"Eacutesmall",
	"Ecircumflexsmall",
	"Edieresissmall",
	"Igravesmall",
	"Iacutesmall",
	"Icircumflexsmall",
	"Idieresissmall",
	"Ethsmall",
	"Ntildesmall",
	"Ogravesmall",
	"Oacutesmall",
	"Ocircumflexsmall",
	"Otildesmall",
	"Odieresissmall",
	"OEsmall",
	"Oslashsmall",
	"Ugravesmall",
	"Uacutesmall",
	"Ucircumflexsmall",
	"Udieresissmall",
	"Yacutesmall",
	"Thornsmall",
	"Ydieresissmall",
	"001.000",
	"001.001",
	"001.002",
	"001.003",
	"Black",
	"Bold",
	"Book",
	"Light",
	"Medium",
	"Regular",
	"Roman",
	"Semibold"
};



#endif /* ENABLE_CFF */


/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: R:/src/FontFusion/Source/Core/rcs/t1.c 1.55 2001/05/03 17:20:06 reggers Exp $
 *                                                                           *
 *     $Log: t1.c $
 *     Revision 1.55  2001/05/03 17:20:06  reggers
 *     Warning cleanup
 *     Revision 1.54  2001/05/02 21:24:47  reggers
 *     Warning cleaning.
 *     Revision 1.53  2001/05/02 21:18:30  reggers
 *     Fixed tsi_T2ListChars so calls back with encoding values and
 *     not SID codes.
 *     Revision 1.52  2001/05/01 20:58:54  reggers
 *     Cleanup of extraneous functions and data arrays.
 *     Revision 1.51  2001/05/01 18:29:33  reggers
 *     Added support for GlyphExists()
 *     Revision 1.50  2001/04/26 20:45:10  reggers
 *     Cleanup relative to PSName conversion. Added errCode parameter,
 *     Revision 1.49  2001/04/25 21:57:52  reggers
 *     Moved ROM_BASED_T1 option to config.h.
 *     Improvements of hash table stuff for PSName to ccode conversion.
 *     Revision 1.47  2001/01/22 19:59:52  reggers
 *     Corrected a spelling error of "unsigned"
 *     Fixed BuildT2MetricsEtc to correctly set upem and adjust font matrix
 *     for non-1000 resolution fonts. Fixes bugs for non-standard font matrices.
 *     Revision 1.46  2000/12/15 15:40:59  reggers
 *     Made 'i' unsigned in PSNameToAppleCode().
 *     Revision 1.45  2000/11/30 17:25:09  reggers
 *     Changed an assert to a tsi_Assert in BuildCMAP(). Must find /Charstrings!
 *     Revision 1.44  2000/10/26 17:07:01  reggers
 *     Corrections for reading reals, T1 and T2 font matrices, and SIDs.
 *     Thanks LCroft!
 *     Revision 1.43  2000/06/16 17:18:04  reggers
 *     Made ENABLE_NATIVE_T1_HINTS user config option.
 *     Revision 1.42  2000/06/14 21:38:58  reggers
 *     Removed new warnings!
 *     Revision 1.41  2000/06/14 21:30:18  reggers
 *     ExtractPureT1... added errCode param.
 *     Fix parse in tsi_GetArray().
 *     Hint replacement fix.
 *     Some casting fixes.
 *     Revision 1.40  2000/06/12 20:54:03  reggers
 *     Borland STRICT warning removal.
 *     Revision 1.39  2000/06/07 16:38:40  reggers
 *     Removed C++ style comments.
 *     Revision 1.38  2000/06/07 15:20:07  mdewsnap
 *     Changes made for dynamic storage
 *     Revision 1.37  2000/05/25 20:13:47  reggers
 *     More 2 byte int changes: BuildCMAP(), for Type 1.
 *     Revision 1.36  2000/05/25 19:18:40  reggers
 *     Upgraded tsi_T1Find() and tsi_T1Findtag() for 2 byte integer builds.
 *     Also fixed call from BuildSubrs() to tsi_T1Find(), limit parameter.
 *     Revision 1.35  2000/05/22 16:08:49  reggers
 *     Corrected a mistake with xorus and yorus confusion caused by
 *     hasty cut and paste.
 *     Revision 1.34  2000/05/18 14:54:11  reggers
 *     Silence STRICT Borland warnings.
 *     Revision 1.33  2000/05/12 14:31:22  reggers
 *     Removed C++ style comment
 *     Revision 1.32  2000/05/09 21:44:28  reggers
 *     Removal of dead code.
 *     Addedd Ff_GetAW... functions; get away from hmtx tables.
 *     Faster font open for T1 and T2.
 *     Revision 1.31  2000/04/19 18:59:57  mdewsnap
 *     Added in code to parse and handle T1 hints
 *     Revision 1.30  2000/04/13 18:14:14  reggers
 *     Updated list chars for user argument or context pass through.
 *     Revision 1.29  2000/03/10 19:18:18  reggers
 *     Enhanced for enumeration of character codes in font.
 *     Revision 1.28  2000/02/25 17:46:06  reggers
 *     STRICT warning cleanup.
 *     Revision 1.27  2000/02/01 19:48:57  reggers
 *     Removed extra curly bracket.
 *     Revision 1.26  2000/02/01 19:32:33  reggers
 *     Always call the close contour after Type1BuildChar(). Always! Fixes
 *     last problem with barcode fonts.
 *     Revision 1.25  2000/01/27 17:10:32  reggers
 *     Re-did the change noted last time!
 *     Revision 1.24  2000/01/27 16:39:54  reggers
 *     Automatic close contour on rmoveto, vmoveto and hmoveto. Fixes bug
 *     observed in some bar code fonts discovered by Moore Research.
 *     Revision 1.23  2000/01/06 20:53:02  reggers
 *     Corrections of data types and casts. Cleanup for configurations.
 *     Revision 1.22  2000/01/06 16:28:17  reggers
 *     Moved CFF function prototypes to within ENABLE_CFF block.
 *     Revision 1.21  1999/12/23 22:03:01  reggers
 *     New ENABLE_PCL branches. Rename any 'code' and 'data' symbols.
 *     Revision 1.19  1999/12/10 15:52:02  reggers
 *     Silenced some warnings with casts.
 *     Revision 1.18  1999/12/09 21:45:28  reggers
 *     End to end test of OTF, changes to cffOffset use.
 *     Revision 1.17  1999/12/08 19:00:49  reggers
 *     Read isCIDKeyed from TopdictData.
 *     Dismantled old char mapping structs: SIDToGIndex and gIndexToSID.
 *     Now use sorted cmap table of key value pairs and BSearch to find code
 *     to index mappings.
 *     Scalable for large character set collections like asian CIDKeyed fonts.
 *     Cmapping may be excluded at runtime for the OpenType processing
 *     mode.
 *     New API function for OpenType: tsi_NewCFFClassOTF(), permits
 *     CFF block to *not* be at the head of the InputStream.
 *     Revision 1.16  1999/11/15 20:07:20  reggers
 *     firstCharCode, lastCharCode and isFixedPitch, first pass.
 *     Revision 1.15  1999/10/18 16:57:27  jfatal
 *     Changed all include file names to lower case.
 *     Revision 1.14  1999/10/07 17:44:26  shawn
 *     Added code to double the calculated number of points in a
 *     character's outline.
 *     Revision 1.13  1999/09/30 15:10:35  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.12  1999/09/30 14:39:13  mdewsnap
 *     Initialized the pairs pointer to NULL in KernSubTable0Data 
 *     routine
 *     Revision 1.11  1999/09/29 21:17:24  mdewsnap
 *     Initialized nPairs field in KernSunTable0Data to 0
 *     Revision 1.10  1999/09/29 20:29:22  mdewsnap
 *     Added in mem hook up in KernSubTable0Data routine --
 *     checked for num pairs of zero before sort.
 *     Revision 1.9  1999/07/19 20:21:13  sampo
 *     Error/warning cleanup.
 *     Revision 1.8  1999/07/19 18:49:32  mdewsnap
 *     Cleaned up warnings
 *     Revision 1.7  1999/07/19 16:59:03  sampo
 *     Change shellsort call to new name.
 *     Revision 1.6  1999/07/16 15:56:20  mdewsnap
 *     Added in call to shell sort
 *     Revision 1.5  1999/07/15 13:21:49  mdewsnap
 *     Added in the parse code for Type1 fonts..
 *     Revision 1.4  1999/07/02 19:10:31  sampo
 *     Corrected some casts.
 *     Revision 1.3  1999/07/01 20:43:17  mdewsnap
 *     added T1 kern code
 *     Revision 1.2  1999/05/17 15:57:45  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/

