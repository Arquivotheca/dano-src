/*
 * T2K.c
 * Font Fusion Copyright (c) 1989-2000 all rights reserved by Bitstream Inc.
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

#include "t2k.h"
#include "t2kextra.h"
#include "util.h"
#include "autogrid.h"
#include "t2ksc.h"
#ifdef ENABLE_STRKCONV
#include "strkconv.h"
#endif
#ifdef ENABLE_T2KE
#include "t2kclrsc.h"
#endif
#include "ghints.h"
#include "t2kstrk1.h"

#define T2K_MAGIC1 0x5a1234a5
#define T2K_MAGIC2 0xa5fedc5a


#ifdef T2K_SCALER

#ifdef ENABLE_SBIT
#ifdef REVERSE_SC_Y_ORDER
static void T2K_InvertBitmap(T2K *t);
#endif
#endif

/***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/
#define ENABLE_DROPOUT_ADAPTATION
#ifdef ENABLE_DROPOUT_ADAPTATION

#define MAX_DROPOUTGLYPHS 2048
static dropoutAdaptationClass *New_dropoutAdaptationClass( tsiMemObject *mem, long numGlyphs )
{
	dropoutAdaptationClass *t = (dropoutAdaptationClass *) tsi_AllocMem( mem, sizeof( dropoutAdaptationClass ) );
	t->mem			= mem;
	t->xPPEm 		= t->yPPEm = -1;

	if ( numGlyphs > MAX_DROPOUTGLYPHS ) numGlyphs = MAX_DROPOUTGLYPHS;
	t->length = 2 * ((numGlyphs+7)>>3);
	t->noNeedForXDropout = (uint8 *)tsi_AllocMem( mem, sizeof( uint8 ) * (t->length) );
	t->noNeedForYDropout = &t->noNeedForXDropout[t->length >>1];
	
	return t; /*****/
}

static void setDropoutSize( dropoutAdaptationClass *t, long xPPEm, long yPPEm)
{
	if ( t->xPPEm != xPPEm || t->yPPEm != yPPEm ) {
		register uint8 *p = &t->noNeedForXDropout[0];
		register long i, length = t->length;
		t->xPPEm = xPPEm;
		t->yPPEm = yPPEm;
		
		for ( i = 0; i < length; i++ ) {
			p[i] = 0;
		}
	}
}

/*
 *
 */
static void Delete_dropoutAdaptationClass( dropoutAdaptationClass *t )
{
	if ( t != NULL ) {
		tsi_DeAllocMem( t->mem, t->noNeedForXDropout );
		tsi_DeAllocMem( t->mem, t );
	}
}

/* needXDropout( dropoutAdaptationClass *t, long glyphIndex ) */
#define needXDropout( t, glyphIndex ) ((glyphIndex >= MAX_DROPOUTGLYPHS || (t->noNeedForXDropout[ glyphIndex>>3 ] & (1 << (glyphIndex&7)) ) == 0))
#define needYDropout( t, glyphIndex ) ((glyphIndex >= MAX_DROPOUTGLYPHS || (t->noNeedForYDropout[ glyphIndex>>3 ] & (1 << (glyphIndex&7)) ) == 0))


/* void xDropoutNotUsed( dropoutAdaptationClass *t, long glyphIndex ) */
#define xDropoutNotUsed( t, glyphIndex ) if ( glyphIndex < MAX_DROPOUTGLYPHS ) t->noNeedForXDropout[ glyphIndex>>3 ] = (uint8)(t->noNeedForXDropout[ glyphIndex>>3 ] | (1 << (glyphIndex&7)))
#define yDropoutNotUsed( t, glyphIndex ) if ( glyphIndex < MAX_DROPOUTGLYPHS ) t->noNeedForYDropout[ glyphIndex>>3 ] = (uint8)(t->noNeedForYDropout[ glyphIndex>>3 ] | (1 << (glyphIndex&7)))




#endif /* ENABLE_DROPOUT_ADAPTATION */

/***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/
#ifdef DRIVE_8BIT_LCD
static void SetColorIndexTable( uint8 *arr )
{
	register uint16 r,g,b, rgSum, r169, g13;
	
#ifdef OLD
	uint16 r256, g16
	assert( colorIndexTableSize == 4096 );
	/* Initialize the entire array to gray-scale values */
	for ( r = 0, r256 = 0; r < 16; r++, r256 += 256 ) {
		for ( g = 0, g16 = 0; g < 16; g++, g16 += 16 ) {
			rgSum = (r + g + ((r + g)>>1) + 0 + 2);
			for ( b = 0; b < 16; b++ ) {
				arr[ r256 | g16 | b ] = (rgSum + b) >> 2; /* Avoid a divide */
			}
		}
	}
#endif

	assert( colorIndexTableSize == 2197 );
	assert( 13*13*13 == colorIndexTableSize );
	/* Initialize the entire array to gray-scale values */
	for ( r = 0, r169 = 0; r < 13; r++, r169 += 169 ) {
		for ( g = 0, g13 = 0; g < 13; g++, g13 += 13 ) {
			rgSum = (uint16)(r + g + ((r + g)>>1) + 0 + 2);
			for ( b = 0; b < 13; b++ ) {
				arr[ r169 + g13 + b ] = (uint8)((rgSum + b) >> 2); /* Avoid a divide */
				assert( arr[ r169 | g13 | b ] < 13 );
			}
		}
	}
	/* Now set the values we will mostly use */
arr[((0) * 169)	+ ((0) * 13)	+ ((0) * 1)] = 0;
arr[((1) * 169)	+ ((1) * 13)	+ ((1) * 1)] = 1;
arr[((2) * 169)	+ ((2) * 13)	+ ((2) * 1)] = 2;
arr[((3) * 169)	+ ((3) * 13)	+ ((3) * 1)] = 3;
arr[((4) * 169)	+ ((4) * 13)	+ ((4) * 1)] = 4;
arr[((5) * 169)	+ ((5) * 13)	+ ((5) * 1)] = 5;
arr[((6) * 169)	+ ((6) * 13)	+ ((6) * 1)] = 6;
arr[((7) * 169)	+ ((7) * 13)	+ ((7) * 1)] = 7;
arr[((8) * 169)	+ ((8) * 13)	+ ((8) * 1)] = 8;
arr[((9) * 169)	+ ((9) * 13)	+ ((9) * 1)] = 9;
arr[((10) * 169)	+ ((10) * 13)	+ ((10) * 1)] = 10;
arr[((11) * 169)	+ ((11) * 13)	+ ((11) * 1)] = 11;
arr[((12) * 169)	+ ((12) * 13)	+ ((12) * 1)] = 12;
arr[((0) * 169)	+ ((0) * 13)	+ ((1) * 1)] = 13;
arr[((0) * 169)	+ ((0) * 13)	+ ((2) * 1)] = 14;
arr[((0) * 169)	+ ((0) * 13)	+ ((3) * 1)] = 15;
arr[((0) * 169)	+ ((1) * 13)	+ ((3) * 1)] = 16;
arr[((1) * 169)	+ ((0) * 13)	+ ((0) * 1)] = 17;
arr[((1) * 169)	+ ((1) * 13)	+ ((0) * 1)] = 18;
arr[((1) * 169)	+ ((1) * 13)	+ ((2) * 1)] = 19;
arr[((1) * 169)	+ ((1) * 13)	+ ((3) * 1)] = 20;
arr[((1) * 169)	+ ((2) * 13)	+ ((4) * 1)] = 21;
arr[((1) * 169)	+ ((2) * 13)	+ ((5) * 1)] = 22;
arr[((2) * 169)	+ ((1) * 13)	+ ((0) * 1)] = 23;
arr[((2) * 169)	+ ((2) * 13)	+ ((0) * 1)] = 24;
arr[((2) * 169)	+ ((2) * 13)	+ ((1) * 1)] = 25;
arr[((2) * 169)	+ ((2) * 13)	+ ((3) * 1)] = 26;
arr[((2) * 169)	+ ((2) * 13)	+ ((4) * 1)] = 27;
arr[((2) * 169)	+ ((3) * 13)	+ ((4) * 1)] = 28;
arr[((2) * 169)	+ ((3) * 13)	+ ((5) * 1)] = 29;
arr[((2) * 169)	+ ((3) * 13)	+ ((6) * 1)] = 30;
arr[((3) * 169)	+ ((2) * 13)	+ ((0) * 1)] = 31;
arr[((3) * 169)	+ ((2) * 13)	+ ((1) * 1)] = 32;
arr[((3) * 169)	+ ((3) * 13)	+ ((0) * 1)] = 33;
arr[((3) * 169)	+ ((3) * 13)	+ ((1) * 1)] = 34;
arr[((3) * 169)	+ ((3) * 13)	+ ((2) * 1)] = 35;
arr[((3) * 169)	+ ((3) * 13)	+ ((4) * 1)] = 36;
arr[((3) * 169)	+ ((3) * 13)	+ ((5) * 1)] = 37;
arr[((3) * 169)	+ ((3) * 13)	+ ((6) * 1)] = 38;
arr[((3) * 169)	+ ((4) * 13)	+ ((5) * 1)] = 39;
arr[((3) * 169)	+ ((4) * 13)	+ ((6) * 1)] = 40;
arr[((3) * 169)	+ ((4) * 13)	+ ((7) * 1)] = 41;
arr[((3) * 169)	+ ((4) * 13)	+ ((8) * 1)] = 42;
arr[((4) * 169)	+ ((3) * 13)	+ ((1) * 1)] = 43;
arr[((4) * 169)	+ ((3) * 13)	+ ((2) * 1)] = 44;
arr[((4) * 169)	+ ((4) * 13)	+ ((2) * 1)] = 45;
arr[((4) * 169)	+ ((4) * 13)	+ ((3) * 1)] = 46;
arr[((4) * 169)	+ ((4) * 13)	+ ((5) * 1)] = 47;
arr[((4) * 169)	+ ((4) * 13)	+ ((6) * 1)] = 48;
arr[((4) * 169)	+ ((5) * 13)	+ ((7) * 1)] = 49;
arr[((4) * 169)	+ ((5) * 13)	+ ((8) * 1)] = 50;
arr[((5) * 169)	+ ((3) * 13)	+ ((1) * 1)] = 51;
arr[((5) * 169)	+ ((4) * 13)	+ ((1) * 1)] = 52;
arr[((5) * 169)	+ ((4) * 13)	+ ((2) * 1)] = 53;
arr[((5) * 169)	+ ((4) * 13)	+ ((3) * 1)] = 54;
arr[((5) * 169)	+ ((5) * 13)	+ ((3) * 1)] = 55;
arr[((5) * 169)	+ ((5) * 13)	+ ((4) * 1)] = 56;
arr[((5) * 169)	+ ((5) * 13)	+ ((6) * 1)] = 57;
arr[((5) * 169)	+ ((5) * 13)	+ ((7) * 1)] = 58;
arr[((5) * 169)	+ ((6) * 13)	+ ((5) * 1)] = 59;
arr[((5) * 169)	+ ((6) * 13)	+ ((6) * 1)] = 60;
arr[((5) * 169)	+ ((6) * 13)	+ ((7) * 1)] = 61;
arr[((5) * 169)	+ ((6) * 13)	+ ((8) * 1)] = 62;
arr[((5) * 169)	+ ((6) * 13)	+ ((9) * 1)] = 63;
arr[((5) * 169)	+ ((6) * 13)	+ ((10) * 1)] = 64;
arr[((5) * 169)	+ ((7) * 13)	+ ((8) * 1)] = 65;
arr[((5) * 169)	+ ((7) * 13)	+ ((9) * 1)] = 66;
arr[((5) * 169)	+ ((7) * 13)	+ ((10) * 1)] = 67;
arr[((6) * 169)	+ ((5) * 13)	+ ((2) * 1)] = 68;
arr[((6) * 169)	+ ((5) * 13)	+ ((3) * 1)] = 69;
arr[((6) * 169)	+ ((5) * 13)	+ ((4) * 1)] = 70;
arr[((6) * 169)	+ ((6) * 13)	+ ((3) * 1)] = 71;
arr[((6) * 169)	+ ((6) * 13)	+ ((4) * 1)] = 72;
arr[((6) * 169)	+ ((6) * 13)	+ ((5) * 1)] = 73;
arr[((6) * 169)	+ ((6) * 13)	+ ((7) * 1)] = 74;
arr[((6) * 169)	+ ((7) * 13)	+ ((6) * 1)] = 75;
arr[((6) * 169)	+ ((7) * 13)	+ ((7) * 1)] = 76;
arr[((6) * 169)	+ ((7) * 13)	+ ((8) * 1)] = 77;
arr[((6) * 169)	+ ((8) * 13)	+ ((10) * 1)] = 78;
arr[((7) * 169)	+ ((6) * 13)	+ ((3) * 1)] = 79;
arr[((7) * 169)	+ ((6) * 13)	+ ((4) * 1)] = 80;
arr[((7) * 169)	+ ((6) * 13)	+ ((5) * 1)] = 81;
arr[((7) * 169)	+ ((7) * 13)	+ ((4) * 1)] = 82;
arr[((7) * 169)	+ ((7) * 13)	+ ((5) * 1)] = 83;
arr[((7) * 169)	+ ((7) * 13)	+ ((6) * 1)] = 84;
arr[((7) * 169)	+ ((7) * 13)	+ ((8) * 1)] = 85;
arr[((7) * 169)	+ ((7) * 13)	+ ((9) * 1)] = 86;
arr[((7) * 169)	+ ((8) * 13)	+ ((7) * 1)] = 87;
arr[((7) * 169)	+ ((8) * 13)	+ ((8) * 1)] = 88;
arr[((7) * 169)	+ ((8) * 13)	+ ((9) * 1)] = 89;
arr[((8) * 169)	+ ((7) * 13)	+ ((3) * 1)] = 90;
arr[((8) * 169)	+ ((7) * 13)	+ ((4) * 1)] = 91;
arr[((8) * 169)	+ ((7) * 13)	+ ((5) * 1)] = 92;
arr[((8) * 169)	+ ((7) * 13)	+ ((6) * 1)] = 93;
arr[((8) * 169)	+ ((8) * 13)	+ ((4) * 1)] = 94;
arr[((8) * 169)	+ ((8) * 13)	+ ((5) * 1)] = 95;
arr[((8) * 169)	+ ((8) * 13)	+ ((6) * 1)] = 96;
arr[((8) * 169)	+ ((8) * 13)	+ ((7) * 1)] = 97;
arr[((8) * 169)	+ ((8) * 13)	+ ((9) * 1)] = 98;
arr[((8) * 169)	+ ((9) * 13)	+ ((6) * 1)] = 99;
arr[((8) * 169)	+ ((9) * 13)	+ ((8) * 1)] = 100;
arr[((8) * 169)	+ ((9) * 13)	+ ((9) * 1)] = 101;
arr[((8) * 169)	+ ((9) * 13)	+ ((10) * 1)] = 102;
arr[((9) * 169)	+ ((8) * 13)	+ ((7) * 1)] = 103;
arr[((9) * 169)	+ ((9) * 13)	+ ((7) * 1)] = 104;
arr[((9) * 169)	+ ((9) * 13)	+ ((8) * 1)] = 105;
arr[((9) * 169)	+ ((9) * 13)	+ ((10) * 1)] = 106;
arr[((9) * 169)	+ ((10) * 13)	+ ((7) * 1)] = 107;
arr[((9) * 169)	+ ((10) * 13)	+ ((9) * 1)] = 108;
arr[((9) * 169)	+ ((11) * 13)	+ ((12) * 1)] = 109;
arr[((10) * 169)	+ ((9) * 13)	+ ((8) * 1)] = 110;
arr[((10) * 169)	+ ((10) * 13)	+ ((8) * 1)] = 111;
arr[((10) * 169)	+ ((10) * 13)	+ ((9) * 1)] = 112;
arr[((10) * 169)	+ ((10) * 13)	+ ((11) * 1)] = 113;
arr[((10) * 169)	+ ((10) * 13)	+ ((12) * 1)] = 114;
arr[((10) * 169)	+ ((11) * 13)	+ ((12) * 1)] = 115;
arr[((11) * 169)	+ ((11) * 13)	+ ((9) * 1)] = 116;
arr[((11) * 169)	+ ((11) * 13)	+ ((10) * 1)] = 117;
arr[((11) * 169)	+ ((11) * 13)	+ ((12) * 1)] = 118;
arr[((11) * 169)	+ ((12) * 13)	+ ((12) * 1)] = 119;
arr[((12) * 169)	+ ((12) * 13)	+ ((10) * 1)] = 120;
arr[((12) * 169)	+ ((12) * 13)	+ ((11) * 1)] = 121;
/* Map colors we are missing to the closest available color. */
arr[((0) * 169)	+ ((1) * 13)	+ ((2) * 1)] = 14; /* 0-1-2 => 0-0-2 at clut[14] */
arr[((0) * 169)	+ ((1) * 13)	+ ((4) * 1)] = 16; /* 0-1-4 => 0-1-3 at clut[16] */
arr[((1) * 169)	+ ((0) * 13)	+ ((2) * 1)] = 14; /* 1-0-2 => 0-0-2 at clut[14] */
arr[((1) * 169)	+ ((0) * 13)	+ ((3) * 1)] = 15; /* 1-0-3 => 0-0-3 at clut[15] */
arr[((1) * 169)	+ ((1) * 13)	+ ((4) * 1)] = 20; /* 1-1-4 => 1-1-3 at clut[20] */
arr[((1) * 169)	+ ((2) * 13)	+ ((3) * 1)] = 20; /* 1-2-3 => 1-1-3 at clut[20] */
arr[((2) * 169)	+ ((1) * 13)	+ ((3) * 1)] = 20; /* 2-1-3 => 1-1-3 at clut[20] */
arr[((2) * 169)	+ ((1) * 13)	+ ((4) * 1)] = 27; /* 2-1-4 => 2-2-4 at clut[27] */
arr[((2) * 169)	+ ((2) * 13)	+ ((5) * 1)] = 27; /* 2-2-5 => 2-2-4 at clut[27] */
arr[((2) * 169)	+ ((4) * 13)	+ ((4) * 1)] = 28; /* 2-4-4 => 2-3-4 at clut[28] */
arr[((3) * 169)	+ ((2) * 13)	+ ((4) * 1)] = 27; /* 3-2-4 => 2-2-4 at clut[27] */
arr[((3) * 169)	+ ((3) * 13)	+ ((7) * 1)] = 38; /* 3-3-7 => 3-3-6 at clut[38] */
arr[((3) * 169)	+ ((4) * 13)	+ ((2) * 1)] = 35; /* 3-4-2 => 3-3-2 at clut[35] */
arr[((4) * 169)	+ ((2) * 13)	+ ((1) * 1)] = 32; /* 4-2-1 => 3-2-1 at clut[32] */
arr[((4) * 169)	+ ((2) * 13)	+ ((2) * 1)] = 44; /* 4-2-2 => 4-3-2 at clut[44] */
arr[((4) * 169)	+ ((3) * 13)	+ ((0) * 1)] = 33; /* 4-3-0 => 3-3-0 at clut[33] */
arr[((4) * 169)	+ ((3) * 13)	+ ((5) * 1)] = 37; /* 4-3-5 => 3-3-5 at clut[37] */
arr[((4) * 169)	+ ((3) * 13)	+ ((6) * 1)] = 38; /* 4-3-6 => 3-3-6 at clut[38] */
arr[((4) * 169)	+ ((4) * 13)	+ ((1) * 1)] = 45; /* 4-4-1 => 4-4-2 at clut[45] */
arr[((4) * 169)	+ ((4) * 13)	+ ((7) * 1)] = 48; /* 4-4-7 => 4-4-6 at clut[48] */
arr[((4) * 169)	+ ((4) * 13)	+ ((8) * 1)] = 58; /* 4-4-8 => 5-5-7 at clut[58] */
arr[((4) * 169)	+ ((5) * 13)	+ ((2) * 1)] = 45; /* 4-5-2 => 4-4-2 at clut[45] */
arr[((4) * 169)	+ ((5) * 13)	+ ((3) * 1)] = 46; /* 4-5-3 => 4-4-3 at clut[46] */
arr[((4) * 169)	+ ((5) * 13)	+ ((6) * 1)] = 48; /* 4-5-6 => 4-4-6 at clut[48] */
arr[((4) * 169)	+ ((5) * 13)	+ ((9) * 1)] = 50; /* 4-5-9 => 4-5-8 at clut[50] */
arr[((4) * 169)	+ ((6) * 13)	+ ((4) * 1)] = 56; /* 4-6-4 => 5-5-4 at clut[56] */
arr[((4) * 169)	+ ((6) * 13)	+ ((5) * 1)] = 59; /* 4-6-5 => 5-6-5 at clut[59] */
arr[((4) * 169)	+ ((6) * 13)	+ ((6) * 1)] = 60; /* 4-6-6 => 5-6-6 at clut[60] */
arr[((4) * 169)	+ ((6) * 13)	+ ((7) * 1)] = 49; /* 4-6-7 => 4-5-7 at clut[49] */
arr[((5) * 169)	+ ((3) * 13)	+ ((2) * 1)] = 44; /* 5-3-2 => 4-3-2 at clut[44] */
arr[((5) * 169)	+ ((3) * 13)	+ ((3) * 1)] = 54; /* 5-3-3 => 5-4-3 at clut[54] */
arr[((5) * 169)	+ ((4) * 13)	+ ((6) * 1)] = 48; /* 5-4-6 => 4-4-6 at clut[48] */
arr[((5) * 169)	+ ((4) * 13)	+ ((7) * 1)] = 58; /* 5-4-7 => 5-5-7 at clut[58] */
arr[((5) * 169)	+ ((4) * 13)	+ ((8) * 1)] = 58; /* 5-4-8 => 5-5-7 at clut[58] */
arr[((5) * 169)	+ ((5) * 13)	+ ((2) * 1)] = 55; /* 5-5-2 => 5-5-3 at clut[55] */
arr[((5) * 169)	+ ((5) * 13)	+ ((8) * 1)] = 58; /* 5-5-8 => 5-5-7 at clut[58] */
arr[((5) * 169)	+ ((5) * 13)	+ ((9) * 1)] = 58; /* 5-5-9 => 5-5-7 at clut[58] */
arr[((5) * 169)	+ ((6) * 13)	+ ((2) * 1)] = 55; /* 5-6-2 => 5-5-3 at clut[55] */
arr[((5) * 169)	+ ((6) * 13)	+ ((3) * 1)] = 55; /* 5-6-3 => 5-5-3 at clut[55] */
arr[((5) * 169)	+ ((6) * 13)	+ ((4) * 1)] = 56; /* 5-6-4 => 5-5-4 at clut[56] */
arr[((5) * 169)	+ ((7) * 13)	+ ((5) * 1)] = 59; /* 5-7-5 => 5-6-5 at clut[59] */
arr[((5) * 169)	+ ((7) * 13)	+ ((6) * 1)] = 60; /* 5-7-6 => 5-6-6 at clut[60] */
arr[((5) * 169)	+ ((7) * 13)	+ ((7) * 1)] = 61; /* 5-7-7 => 5-6-7 at clut[61] */
arr[((6) * 169)	+ ((4) * 13)	+ ((1) * 1)] = 52; /* 6-4-1 => 5-4-1 at clut[52] */
arr[((6) * 169)	+ ((4) * 13)	+ ((2) * 1)] = 53; /* 6-4-2 => 5-4-2 at clut[53] */
arr[((6) * 169)	+ ((4) * 13)	+ ((3) * 1)] = 54; /* 6-4-3 => 5-4-3 at clut[54] */
arr[((6) * 169)	+ ((4) * 13)	+ ((4) * 1)] = 70; /* 6-4-4 => 6-5-4 at clut[70] */
arr[((6) * 169)	+ ((5) * 13)	+ ((7) * 1)] = 58; /* 6-5-7 => 5-5-7 at clut[58] */
arr[((6) * 169)	+ ((5) * 13)	+ ((8) * 1)] = 58; /* 6-5-8 => 5-5-7 at clut[58] */
arr[((6) * 169)	+ ((6) * 13)	+ ((8) * 1)] = 74; /* 6-6-8 => 6-6-7 at clut[74] */
arr[((6) * 169)	+ ((6) * 13)	+ ((9) * 1)] = 86; /* 6-6-9 => 7-7-9 at clut[86] */
arr[((6) * 169)	+ ((6) * 13)	+ ((10) * 1)] = 86; /* 6-6-10 => 7-7-9 at clut[86] */
arr[((6) * 169)	+ ((7) * 13)	+ ((3) * 1)] = 71; /* 6-7-3 => 6-6-3 at clut[71] */
arr[((6) * 169)	+ ((7) * 13)	+ ((4) * 1)] = 72; /* 6-7-4 => 6-6-4 at clut[72] */
arr[((6) * 169)	+ ((7) * 13)	+ ((5) * 1)] = 73; /* 6-7-5 => 6-6-5 at clut[73] */
arr[((6) * 169)	+ ((7) * 13)	+ ((9) * 1)] = 77; /* 6-7-9 => 6-7-8 at clut[77] */
arr[((6) * 169)	+ ((7) * 13)	+ ((10) * 1)] = 64; /* 6-7-10 => 5-6-10 at clut[64] */
arr[((6) * 169)	+ ((7) * 13)	+ ((11) * 1)] = 64; /* 6-7-11 => 5-6-10 at clut[64] */
arr[((6) * 169)	+ ((8) * 13)	+ ((6) * 1)] = 75; /* 6-8-6 => 6-7-6 at clut[75] */
arr[((6) * 169)	+ ((8) * 13)	+ ((7) * 1)] = 76; /* 6-8-7 => 6-7-7 at clut[76] */
arr[((6) * 169)	+ ((8) * 13)	+ ((8) * 1)] = 77; /* 6-8-8 => 6-7-8 at clut[77] */
arr[((6) * 169)	+ ((8) * 13)	+ ((9) * 1)] = 78; /* 6-8-9 => 6-8-10 at clut[78] */
arr[((6) * 169)	+ ((8) * 13)	+ ((11) * 1)] = 78; /* 6-8-11 => 6-8-10 at clut[78] */
arr[((7) * 169)	+ ((5) * 13)	+ ((2) * 1)] = 68; /* 7-5-2 => 6-5-2 at clut[68] */
arr[((7) * 169)	+ ((5) * 13)	+ ((3) * 1)] = 69; /* 7-5-3 => 6-5-3 at clut[69] */
arr[((7) * 169)	+ ((5) * 13)	+ ((4) * 1)] = 70; /* 7-5-4 => 6-5-4 at clut[70] */
arr[((7) * 169)	+ ((5) * 13)	+ ((5) * 1)] = 81; /* 7-5-5 => 7-6-5 at clut[81] */
arr[((7) * 169)	+ ((6) * 13)	+ ((8) * 1)] = 85; /* 7-6-8 => 7-7-8 at clut[85] */
arr[((7) * 169)	+ ((7) * 13)	+ ((3) * 1)] = 82; /* 7-7-3 => 7-7-4 at clut[82] */
arr[((7) * 169)	+ ((7) * 13)	+ ((10) * 1)] = 86; /* 7-7-10 => 7-7-9 at clut[86] */
arr[((7) * 169)	+ ((7) * 13)	+ ((11) * 1)] = 86; /* 7-7-11 => 7-7-9 at clut[86] */
arr[((7) * 169)	+ ((8) * 13)	+ ((4) * 1)] = 82; /* 7-8-4 => 7-7-4 at clut[82] */
arr[((7) * 169)	+ ((8) * 13)	+ ((5) * 1)] = 83; /* 7-8-5 => 7-7-5 at clut[83] */
arr[((7) * 169)	+ ((8) * 13)	+ ((6) * 1)] = 84; /* 7-8-6 => 7-7-6 at clut[84] */
arr[((7) * 169)	+ ((8) * 13)	+ ((10) * 1)] = 89; /* 7-8-10 => 7-8-9 at clut[89] */
arr[((7) * 169)	+ ((8) * 13)	+ ((11) * 1)] = 102; /* 7-8-11 => 8-9-10 at clut[102] */
arr[((7) * 169)	+ ((8) * 13)	+ ((12) * 1)] = 102; /* 7-8-12 => 8-9-10 at clut[102] */
arr[((7) * 169)	+ ((9) * 13)	+ ((8) * 1)] = 88; /* 7-9-8 => 7-8-8 at clut[88] */
arr[((7) * 169)	+ ((9) * 13)	+ ((9) * 1)] = 89; /* 7-9-9 => 7-8-9 at clut[89] */
arr[((7) * 169)	+ ((9) * 13)	+ ((10) * 1)] = 102; /* 7-9-10 => 8-9-10 at clut[102] */
arr[((7) * 169)	+ ((9) * 13)	+ ((11) * 1)] = 102; /* 7-9-11 => 8-9-10 at clut[102] */
arr[((7) * 169)	+ ((9) * 13)	+ ((12) * 1)] = 102; /* 7-9-12 => 8-9-10 at clut[102] */
arr[((8) * 169)	+ ((6) * 13)	+ ((3) * 1)] = 79; /* 8-6-3 => 7-6-3 at clut[79] */
arr[((8) * 169)	+ ((6) * 13)	+ ((4) * 1)] = 80; /* 8-6-4 => 7-6-4 at clut[80] */
arr[((8) * 169)	+ ((6) * 13)	+ ((5) * 1)] = 81; /* 8-6-5 => 7-6-5 at clut[81] */
arr[((8) * 169)	+ ((6) * 13)	+ ((6) * 1)] = 93; /* 8-6-6 => 8-7-6 at clut[93] */
arr[((8) * 169)	+ ((7) * 13)	+ ((9) * 1)] = 86; /* 8-7-9 => 7-7-9 at clut[86] */
arr[((8) * 169)	+ ((7) * 13)	+ ((10) * 1)] = 86; /* 8-7-10 => 7-7-9 at clut[86] */
arr[((8) * 169)	+ ((8) * 13)	+ ((10) * 1)] = 98; /* 8-8-10 => 8-8-9 at clut[98] */
arr[((8) * 169)	+ ((8) * 13)	+ ((11) * 1)] = 106; /* 8-8-11 => 9-9-10 at clut[106] */
arr[((8) * 169)	+ ((9) * 13)	+ ((5) * 1)] = 95; /* 8-9-5 => 8-8-5 at clut[95] */
arr[((8) * 169)	+ ((9) * 13)	+ ((7) * 1)] = 97; /* 8-9-7 => 8-8-7 at clut[97] */
arr[((8) * 169)	+ ((9) * 13)	+ ((11) * 1)] = 102; /* 8-9-11 => 8-9-10 at clut[102] */
arr[((8) * 169)	+ ((9) * 13)	+ ((12) * 1)] = 102; /* 8-9-12 => 8-9-10 at clut[102] */
arr[((8) * 169)	+ ((10) * 13)	+ ((9) * 1)] = 101; /* 8-10-9 => 8-9-9 at clut[101] */
arr[((8) * 169)	+ ((10) * 13)	+ ((10) * 1)] = 102; /* 8-10-10 => 8-9-10 at clut[102] */
arr[((8) * 169)	+ ((10) * 13)	+ ((11) * 1)] = 102; /* 8-10-11 => 8-9-10 at clut[102] */
arr[((8) * 169)	+ ((10) * 13)	+ ((12) * 1)] = 109; /* 8-10-12 => 9-11-12 at clut[109] */
arr[((9) * 169)	+ ((7) * 13)	+ ((4) * 1)] = 91; /* 9-7-4 => 8-7-4 at clut[91] */
arr[((9) * 169)	+ ((7) * 13)	+ ((5) * 1)] = 92; /* 9-7-5 => 8-7-5 at clut[92] */
arr[((9) * 169)	+ ((7) * 13)	+ ((6) * 1)] = 93; /* 9-7-6 => 8-7-6 at clut[93] */
arr[((9) * 169)	+ ((7) * 13)	+ ((7) * 1)] = 103; /* 9-7-7 => 9-8-7 at clut[103] */
arr[((9) * 169)	+ ((8) * 13)	+ ((4) * 1)] = 94; /* 9-8-4 => 8-8-4 at clut[94] */
arr[((9) * 169)	+ ((8) * 13)	+ ((5) * 1)] = 95; /* 9-8-5 => 8-8-5 at clut[95] */
arr[((9) * 169)	+ ((8) * 13)	+ ((6) * 1)] = 96; /* 9-8-6 => 8-8-6 at clut[96] */
arr[((9) * 169)	+ ((8) * 13)	+ ((10) * 1)] = 106; /* 9-8-10 => 9-9-10 at clut[106] */
arr[((9) * 169)	+ ((9) * 13)	+ ((5) * 1)] = 95; /* 9-9-5 => 8-8-5 at clut[95] */
arr[((9) * 169)	+ ((9) * 13)	+ ((6) * 1)] = 104; /* 9-9-6 => 9-9-7 at clut[104] */
arr[((9) * 169)	+ ((9) * 13)	+ ((11) * 1)] = 106; /* 9-9-11 => 9-9-10 at clut[106] */
arr[((9) * 169)	+ ((9) * 13)	+ ((12) * 1)] = 114; /* 9-9-12 => 10-10-12 at clut[114] */
arr[((9) * 169)	+ ((10) * 13)	+ ((6) * 1)] = 107; /* 9-10-6 => 9-10-7 at clut[107] */
arr[((9) * 169)	+ ((10) * 13)	+ ((8) * 1)] = 105; /* 9-10-8 => 9-9-8 at clut[105] */
arr[((9) * 169)	+ ((10) * 13)	+ ((11) * 1)] = 113; /* 9-10-11 => 10-10-11 at clut[113] */
arr[((9) * 169)	+ ((10) * 13)	+ ((12) * 1)] = 114; /* 9-10-12 => 10-10-12 at clut[114] */
arr[((9) * 169)	+ ((11) * 13)	+ ((11) * 1)] = 109; /* 9-11-11 => 9-11-12 at clut[109] */
arr[((10) * 169)	+ ((8) * 13)	+ ((5) * 1)] = 95; /* 10-8-5 => 8-8-5 at clut[95] */
arr[((10) * 169)	+ ((8) * 13)	+ ((6) * 1)] = 103; /* 10-8-6 => 9-8-7 at clut[103] */
arr[((10) * 169)	+ ((8) * 13)	+ ((7) * 1)] = 103; /* 10-8-7 => 9-8-7 at clut[103] */
arr[((10) * 169)	+ ((8) * 13)	+ ((8) * 1)] = 110; /* 10-8-8 => 10-9-8 at clut[110] */
arr[((10) * 169)	+ ((9) * 13)	+ ((5) * 1)] = 95; /* 10-9-5 => 8-8-5 at clut[95] */
arr[((10) * 169)	+ ((9) * 13)	+ ((6) * 1)] = 104; /* 10-9-6 => 9-9-7 at clut[104] */
arr[((10) * 169)	+ ((9) * 13)	+ ((7) * 1)] = 104; /* 10-9-7 => 9-9-7 at clut[104] */
arr[((10) * 169)	+ ((9) * 13)	+ ((11) * 1)] = 113; /* 10-9-11 => 10-10-11 at clut[113] */
arr[((10) * 169)	+ ((9) * 13)	+ ((12) * 1)] = 114; /* 10-9-12 => 10-10-12 at clut[114] */
arr[((10) * 169)	+ ((10) * 13)	+ ((6) * 1)] = 104; /* 10-10-6 => 9-9-7 at clut[104] */
arr[((10) * 169)	+ ((10) * 13)	+ ((7) * 1)] = 111; /* 10-10-7 => 10-10-8 at clut[111] */
arr[((10) * 169)	+ ((11) * 13)	+ ((7) * 1)] = 107; /* 10-11-7 => 9-10-7 at clut[107] */
arr[((11) * 169)	+ ((9) * 13)	+ ((7) * 1)] = 110; /* 11-9-7 => 10-9-8 at clut[110] */
arr[((11) * 169)	+ ((9) * 13)	+ ((8) * 1)] = 110; /* 11-9-8 => 10-9-8 at clut[110] */
arr[((11) * 169)	+ ((9) * 13)	+ ((9) * 1)] = 110; /* 11-9-9 => 10-9-8 at clut[110] */
arr[((11) * 169)	+ ((10) * 13)	+ ((6) * 1)] = 111; /* 11-10-6 => 10-10-8 at clut[111] */
arr[((11) * 169)	+ ((10) * 13)	+ ((7) * 1)] = 111; /* 11-10-7 => 10-10-8 at clut[111] */
arr[((11) * 169)	+ ((10) * 13)	+ ((8) * 1)] = 111; /* 11-10-8 => 10-10-8 at clut[111] */
arr[((11) * 169)	+ ((10) * 13)	+ ((9) * 1)] = 112; /* 11-10-9 => 10-10-9 at clut[112] */
arr[((11) * 169)	+ ((10) * 13)	+ ((12) * 1)] = 114; /* 11-10-12 => 10-10-12 at clut[114] */
arr[((11) * 169)	+ ((11) * 13)	+ ((7) * 1)] = 111; /* 11-11-7 => 10-10-8 at clut[111] */
arr[((11) * 169)	+ ((11) * 13)	+ ((8) * 1)] = 116; /* 11-11-8 => 11-11-9 at clut[116] */
arr[((11) * 169)	+ ((12) * 13)	+ ((10) * 1)] = 117; /* 11-12-10 => 11-11-10 at clut[117] */
arr[((12) * 169)	+ ((10) * 13)	+ ((8) * 1)] = 116; /* 12-10-8 => 11-11-9 at clut[116] */
arr[((12) * 169)	+ ((10) * 13)	+ ((9) * 1)] = 116; /* 12-10-9 => 11-11-9 at clut[116] */
arr[((12) * 169)	+ ((11) * 13)	+ ((8) * 1)] = 116; /* 12-11-8 => 11-11-9 at clut[116] */
arr[((12) * 169)	+ ((11) * 13)	+ ((9) * 1)] = 116; /* 12-11-9 => 11-11-9 at clut[116] */
arr[((12) * 169)	+ ((11) * 13)	+ ((10) * 1)] = 117; /* 12-11-10 => 11-11-10 at clut[117] */
arr[((12) * 169)	+ ((12) * 13)	+ ((9) * 1)] = 120; /* 12-12-9 => 12-12-10 at clut[120] */

}
#endif	/* DRIVE_8BIT_LCD */
/***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/

/*
 *
 */
T2K *NewT2K( tsiMemObject *mem, sfntClass *font, int *errCode  )
{
	assert( errCode != NULL );
	if ( mem == NULL ) {
		*errCode = T2K_ERR_MEM_IS_NULL;
	} else if ( (*errCode = setjmp(mem->env)) == 0 ) {
		/* try */
		register T2K *t = (T2K *)tsi_AllocMem( mem, sizeof( T2K ) );
		t->mem = mem;
		
		t->stamp1 = T2K_MAGIC1;
		t->font	  = font;
		t->numberOfLogicalFonts = font->numberOfLogicalFonts; /* pass through. */
		t->stamp2 = T2K_MAGIC2;
		
		
		t->glyph 			= NULL;
		t->glyphIndex 		= 0;
		t->hintHandle 		= NULL;
		t->baseAddr 		= NULL;
		t->baseARGB 		= NULL;
		t->internal_baseAddr = false;
		t->internal_baseARGB = false;
		t->nameString8  	= NULL;
		t->nameString16 	= NULL;
		t->theCache 		= NULL;
		t->GetCacheMemory	= NULL;
		t->theFM 			= NULL;
		t->FMRenderGlyph 	= NULL;
		t->BitmapFilter		= NULL;
		t->filterParamsPtr	= NULL;
		t->bitRange255		= false;
		t->remapBits		= NULL;
		
		t->ag_xPixelsPerEm = t->ag_yPixelsPerEm = -1;
		/* t->globalHintsCache = NULL; */

		t->isFixedPitch		= font->isFixedPitch;
		t->firstCharCode		= font->firstCharCode;
		t->lastCharCode		= font->lastCharCode;
		
		
#ifdef LAYOUT_CACHE_SIZE
		{
			int i;
			
			for ( i = 0; i < LAYOUT_CACHE_SIZE; i++ ) {
				t->tag[i] = 0xffffffff;
			}
		}
#endif
		t->font->preferedPlatformID 			= 0xffff;
		t->font->preferedPlatformSpecificID 	= 0xffff;
#ifdef ENABLE_PFR
		if (t->font->PFR != NULL ) {
			/* t->font->PFR->scaler = t; */
			t->font->PFR->sfntClassPtr = font;
		}
#endif
		t->tAdapt = NULL;
#ifdef DRIVE_8BIT_LCD
		SetColorIndexTable( t->colorIndexTable );
#endif	/* DRIVE_8BIT_LCD */
		return t; /*****/
	} else {
		/* catch */
		tsi_EmergencyShutDown( mem );
	}

	return NULL; /*****/
}

static void T2KDoGriddingSetUp( T2K *t )
{

#ifdef ENABLE_AUTO_GRIDDING_CORE
	int err;

	if ( t->hintHandle == NULL ) {
		int maxPointCount;
		uint16 unitsPerEm;

		maxPointCount = GetMaxPoints( t->font );
		unitsPerEm	  = t->upem;
		
		err = ag_HintInit( t->mem, maxPointCount, (short)unitsPerEm, &t->hintHandle );
		tsi_Assert( t->mem, err == 0, err );
		t->fontCategory = GetNumGlyphs_sfntClass( t->font ) < 1000 ? ag_ROMAN : ag_KANJI; /* ag_ROMAN/ag_KANJI guess */
		if ( t->font->globalHintsCache == NULL ) {
			InputStream *in = NULL;
			ag_GlobalDataType globalHints;
			
#			ifdef ENABLE_T1
			if ( t->font->T1 != NULL ) {
				; /* Always do recomputation for T1 */
			} else 
#			endif
#			ifdef ENABLE_CFF
			if ( t->font->T2 != NULL ) {
				; /* Always do recomputation for T2 */
			} else 
#			endif
#			ifdef ENABLE_PFR
			if ( t->font->PFR != NULL ) {
				; /* Always do recomputation for PFR */
			} else 
#			endif
#			ifdef ENABLE_SPD
			if ( t->font->SPD != NULL ) {
				; /* Always do recomputation for Speedo */
			} else 
#			endif
#			ifdef ENABLE_PCL
			if ( t->font->PCLeo != NULL ) {
				; /* Always do recomputation for PCL */
			} else 
#			endif
			/* Force recomputation if algorithmic styling is on */
			if ( t->font->StyleFunc == NULL ) {
				in = GetStreamForTable( t->font, tag_T2KG );
			}
			if ( in != NULL ) {
				t2k_ReadGHints( &globalHints, in);
				Delete_InputStream( in, NULL );
#ifdef OLD_TEST
				if ( true ) {
					ag_GlobalDataType globalHints2;
					int i;
					
					t2k_ComputeGlobalHints( t->font, t->hintHandle, &globalHints2 );
					for ( i = 0; i < ag_MAX_HEIGHTS_IN; i++ ) {
						assert( globalHints.heights[i].flat == globalHints2.heights[i].flat );
						assert( globalHints.heights[i].round == globalHints2.heights[i].round );
						assert( globalHints.heights[i].overLap == globalHints2.heights[i].overLap );
					}
					for ( i = 0; i < ag_MAXWEIGHTS; i++ ) {
						assert( globalHints.xWeight[i] == globalHints2.xWeight[i] );
						assert( globalHints.yWeight[i] == globalHints2.yWeight[i] );
					}
					printf("OK\n");
				}
#endif /* OLD_TEST */
			} else {
				t2k_ComputeGlobalHints( t->font, t->hintHandle, &globalHints, t->fontCategory == ag_KANJI  );
			}
			t->font->globalHintsCache = tsi_AllocMem( t->mem, sizeof(ag_GlobalDataType) );
			memcpy(t->font->globalHintsCache, &globalHints, sizeof(ag_GlobalDataType));
		}
		err = ag_SetHintInfo( t->hintHandle, (ag_GlobalDataType *)t->font->globalHintsCache, t->fontCategory ); 
		tsi_Assert( t->mem, err == 0, err );
	}
#endif /*  ENABLE_AUTO_GRIDDING_CORE */
	
	if ( ((t->ag_xPixelsPerEm != t->xPixelsPerEm) || (t->ag_yPixelsPerEm != t->yPixelsPerEm))  ) {
#ifdef ENABLE_AUTO_GRIDDING_CORE
			err = ag_SetScale( t->hintHandle, t->xPixelsPerEm, t->yPixelsPerEm, &t->xWeightIsOne );
			tsi_Assert( t->mem, err == 0, err );
#endif	
#ifdef ENABLE_NATIVE_TT_HINTS
			if ( t->font->t2kTT != NULL ) {
				t->xWeightIsOne = false;
				SetScale_T2KTTClass( (T2KTTClass *)(t->font->t2kTT), t->xPixelsPerEm, t->yPixelsPerEm );
			}
#endif

		t->ag_xPixelsPerEm = t->xPixelsPerEm;
		t->ag_yPixelsPerEm = t->yPixelsPerEm;
	}	
}




static void scalePoints( register int16 *ooz, register F26Dot6 *z, register int n, T2KScaleInfo *si )
{
	register int i;
	register F26Dot6 tmp32;
	register int16 nScale 		= si->nScale;
	register int32 dScaleDiv2	= si->dScaleDiv2;
	

	switch ( si->scaleType ) {
	case T2K_IMULSHIFT:
	{
		register int16 dShift 		= si->dShift;
		for ( i = 0; i < n; i++ ) {
			tmp32   = ooz[i];
			tmp32  *= nScale;
			tmp32  += dScaleDiv2;
			tmp32 >>= dShift;
			z[i]    = tmp32;
		}
	}
	break; /*****/
	case T2K_IMULDIV:
	{
		register int32 dScale		= si->dScale;
		for ( i = 0; i < n; i++ ) {
			tmp32   = ooz[i];
			tmp32  *= nScale;
			if ( tmp32 >= 0 ) {
				tmp32 += dScaleDiv2; tmp32 /= dScale;
			} else {
				tmp32 = -tmp32;
				tmp32 += dScaleDiv2; tmp32 /= dScale;
				tmp32 = -tmp32;
			}
			z[i]    = tmp32;
		}
	}
	break; /*****/
	case T2K_FIXMUL:
	{
		register F16Dot16 fixedScale = si->fixedScale;
		for ( i = 0; i < n; i++ ) {
			z[i] = util_FixMul( fixedScale, ooz[i] );
		}
	}
	break; /*****/
	
	}
}

/*
 *
 */
static void T2K_NewTransformationInternal( T2K *t, int doSetUpNow, long xPixelsPerEm, long yPixelsPerEm, T2K_TRANS_MATRIX *trans )
{
	t->t00 = trans->t00;
	t->t01 = trans->t01;
	t->t10 = trans->t10;
	t->t11 = trans->t11;
	t->is_Identity = 	t->t00 == ONE16Dot16	&& t->t01 == 0 &&
		 				t->t10 == 0				&& t->t11 == ONE16Dot16;
	t->xPixelsPerEm = xPixelsPerEm;
	t->yPixelsPerEm = yPixelsPerEm;
	
	if (doSetUpNow) {
		T2KDoGriddingSetUp(t);
	}
}

void T2K_TransformXFunits( T2K *t, short xValueInFUnits, F16Dot16 *x, F16Dot16 *y )
{
	F16Dot16 x16Dot16, y16Dot16;
	F16Dot16 tmpX;

	x16Dot16 = xValueInFUnits; x16Dot16 <<= 16;
	y16Dot16 = 0;
	
	x16Dot16 = util_FixMul( x16Dot16, t->xMul );
	if ( !t->is_Identity ) {
		tmpX = x16Dot16; /* tmpY = 0; */
		x16Dot16 = util_FixMul( t->t00, tmpX ) /* + util_FixMul( t->t01, tmpY ) */;
		y16Dot16 = util_FixMul( t->t10, tmpX ) /* + util_FixMul( t->t11, tmpY ) */;
	}
	*x = x16Dot16;
	*y = y16Dot16;
}

void T2K_TransformYFunits( T2K *t, short yValueInFUnits, F16Dot16 *x, F16Dot16 *y )
{
	F16Dot16 x16Dot16, y16Dot16;
	F16Dot16 tmpY;

	x16Dot16 = 0;
	y16Dot16 = yValueInFUnits; y16Dot16 <<= 16;
	
	y16Dot16 = util_FixMul( y16Dot16, t->yMul );
	if ( !t->is_Identity ) {
		tmpY = y16Dot16; /* tmpX = 0; */
		x16Dot16 = /* util_FixMul( t->t00, tmpX ) */ + util_FixMul( t->t01, tmpY );
		y16Dot16 = /* util_FixMul( t->t10, tmpX ) */ + util_FixMul( t->t11, tmpY );
	}
	*x = x16Dot16;
	*y = y16Dot16;
}

/* Added function that returns the transformation matrix
 * between the default font matrix and the font matrix described in the
 * font stream.  For most cases this is the identity matrix
 */

static void ConcatFontMatrix( sfntClass *t, T2K_TRANS_MATRIX *trans )
{
#if !defined ENABLE_T1 && !defined ENABLE_CFF
	UNUSED(t);
	UNUSED(trans);
#endif
#ifdef ENABLE_T1
	if ( t->T1 != NULL ) {
	trans->t00 = util_FixMul( trans->t00, t->T1->m00 ) + 
	             util_FixMul( trans->t10, t->T1->m01 );
	trans->t10 = util_FixMul( trans->t00, t->T1->m10 ) + 
	             util_FixMul( trans->t10, t->T1->m11 );
	trans->t01 = util_FixMul( trans->t01, t->T1->m00 ) + 
	             util_FixMul( trans->t11, t->T1->m01 );
	trans->t11 = util_FixMul( trans->t01, t->T1->m10 ) + 
	             util_FixMul( trans->t11, t->T1->m11 );
	}
#endif
#ifdef ENABLE_CFF
	if ( t->T2 != NULL ) {
	trans->t00 = util_FixMul( trans->t00, t->T2->topDictData.m00 ) + 
	             util_FixMul( trans->t10, t->T2->topDictData.m01 );
	trans->t10 = util_FixMul( trans->t00, t->T2->topDictData.m10 ) + 
	             util_FixMul( trans->t10, t->T2->topDictData.m11 );
	trans->t01 = util_FixMul( trans->t01, t->T2->topDictData.m00 ) + 
	             util_FixMul( trans->t11, t->T2->topDictData.m01 );
	trans->t11 = util_FixMul( trans->t01, t->T2->topDictData.m10 ) + 
	             util_FixMul( trans->t11, t->T2->topDictData.m11 );
	} 
#endif
}

/*
 *
 */
void T2K_NewTransformation( T2K *t, int doSetUpNow, long xRes, long yRes, T2K_TRANS_MATRIX *trans, int enableSbits, int *errCode )
{
	F16Dot16 xPointSize, yPointSize;
	F16Dot16 xResRatio, yResRatio;
	long xPixelsPerEm, yPixelsPerEm;
	register uint16 UPEM;
	
	assert( errCode != NULL );
	if ( (*errCode = setjmp(t->mem->env)) == 0 ) {
		/* try */
		tsi_Assert( t->mem, trans != NULL, T2K_ERR_TRANS_IS_NULL );
		tsi_Assert( t->mem, xRes > 0 && yRes > 0, T2K_ERR_RES_IS_NOT_POS );
		
		/* LCroft 19-OCT-000: transform the input matrix by the non-standard
		 * font matrix.  In most cases this matrix will be the identity indicating
		 * that the font uses the default scaling matrix.
		 */
		ConcatFontMatrix( t->font, trans );
	        
		t->upem = UPEM = GetUPEM( t->font );
		
		xPointSize = util_EuclidianDistance( trans->t00, trans->t10 );
		yPointSize = util_EuclidianDistance( trans->t01, trans->t11 );
		
		xResRatio = (xRes << 16) / 72;
		yResRatio = (yRes << 16) / 72;
		
		xPixelsPerEm = util_FixMul( xPointSize, xResRatio );
		yPixelsPerEm = util_FixMul( yPointSize, yResRatio );
		t->xPixelsPerEm16Dot16 = xPixelsPerEm;
		t->yPixelsPerEm16Dot16 = yPixelsPerEm;
		t->xMul = util_FixDiv( t->xPixelsPerEm16Dot16, ((long)(UPEM)) << 16);
		t->yMul = util_FixDiv( t->yPixelsPerEm16Dot16, ((long)(UPEM)) << 16);
		/* Round to integer ppem */
		xPixelsPerEm += 0x00008000; xPixelsPerEm >>= 16;
		yPixelsPerEm += 0x00008000; yPixelsPerEm >>= 16;

		/* Now remove the point size from the matrix */
		if ( xPixelsPerEm > 0 && yPixelsPerEm > 0 ) {
			trans->t00 = util_FixDiv( trans->t00, xPointSize );
			trans->t10 = util_FixDiv( trans->t10, xPointSize );
			trans->t11 = util_FixDiv( trans->t11, yPointSize );
			trans->t01 = util_FixDiv( trans->t01, yPointSize );
		} else {
			trans->t00 = 0;
			trans->t10 = 0;
			trans->t11 = 0;
			trans->t01 = 0;
		}

		if (t->font != NULL) {
			t->font->xPPEm = xPixelsPerEm;
			t->font->yPPEm = yPixelsPerEm;
		}

		T2K_NewTransformationInternal( t, doSetUpNow, xPixelsPerEm, yPixelsPerEm, trans );
		setT2KScaleFactors(xPixelsPerEm, UPEM, &t->xScale);
		setT2KScaleFactors(yPixelsPerEm, UPEM, &t->yScale);
		assert( t != NULL && t->font != NULL );
		
		if ( t->tAdapt == NULL ) {
			t->tAdapt = New_dropoutAdaptationClass( t->mem, T2K_GetNumGlyphsInFont(t) );
		}
		setDropoutSize( t->tAdapt, xPixelsPerEm, xPixelsPerEm);
		{
			long minPixelsPerEm = xPixelsPerEm;
			if ( yPixelsPerEm < minPixelsPerEm ) minPixelsPerEm = yPixelsPerEm;
			t->oneHalfFUnit = (minPixelsPerEm << 6) / (UPEM + UPEM);
		}
		
		t->horizontalFontMetricsAreValid 	= false;
		t->verticalFontMetricsAreValid		= false;
#ifdef ENABLE_SBIT
#ifdef ENABLE_PFR
		if ( t->font->PFR != NULL ) {
			t->enableSbits = enableSbits && t->font->PFR->nBmapStrikes && t->is_Identity;
		}
		else
#endif /* ENABLE_PFR */
		{
			t->enableSbits = enableSbits && T2K_FontSbitsExists(t) && t->is_Identity;
		}
#else		
		UNUSED(enableSbits);
		t->enableSbits = false;
#endif /* ENABLE_SBIT */
		{
			T2K_FontWideMetrics hori, vert;
			int usedOutlines = false;
			
#ifdef ENABLE_SBIT
			if ( t->enableSbits 
#ifdef ENABLE_PFR
					&& (t->font->PFR == NULL)
#endif
								) {
				hori.underlinePosition	= t->font->post_underlinePosition;
				hori.underlineThickness	= t->font->post_underlinePosition;
				vert.underlinePosition	= 0;
				vert.underlineThickness	= 0;
				GetFontWideSbitMetrics( t->font->bloc, t->font->ebsc, (uint16)xPixelsPerEm, (uint16)yPixelsPerEm, &hori, &vert );
				t->horizontalFontMetricsAreValid	= hori.isValid;
				t->verticalFontMetricsAreValid		= vert.isValid;
			}
#endif /* ENABLE_SBIT */
			if ( !t->horizontalFontMetricsAreValid && !t->verticalFontMetricsAreValid ){
				GetFontWideOutlineMetrics( t->font, &hori, &vert );
				usedOutlines = true;
			}
			if ( hori.isValid ) {
				t->yAscender 	= hori.Ascender;	t->yAscender	<<= 16;
				t->xAscender 	= 0;
				t->yDescender	= hori.Descender;	t->yDescender	<<= 16;
				t->xDescender	= 0;
				t->yLineGap		= hori.LineGap;		t->yLineGap		<<= 16;
				t->xLineGap		= 0;
				t->xMaxLinearAdvanceWidth	= hori.maxAW;	t->xMaxLinearAdvanceWidth		<<= 16;
				t->yMaxLinearAdvanceWidth	= 0;
				t->caretDx		= hori.caretDx;
				t->caretDy		= hori.caretDy;
				
				t->horizontalFontMetricsAreValid 	= true;
				if ( usedOutlines ) {
					/* We need to scale */
					if ( !t->is_Identity ) {
						F16Dot16 tmpX = t->caretDx;
						F16Dot16 tmpY = t->caretDy;
						t->caretDx = util_FixMul( t->t00, tmpX ) + util_FixMul( t->t01, tmpY );
						t->caretDy = util_FixMul( t->t10, tmpX ) + util_FixMul( t->t11, tmpY );
					}
					T2K_TransformYFunits( t, hori.Ascender,		&t->xAscender,				&t->yAscender);
					T2K_TransformYFunits( t, hori.Descender,	&t->xDescender,				&t->yDescender);
					T2K_TransformYFunits( t, hori.LineGap,		&t->xLineGap,				&t->yLineGap);
					T2K_TransformXFunits( t, (short)hori.maxAW,	&t->xMaxLinearAdvanceWidth,	&t->yMaxLinearAdvanceWidth );
					
					/* Only for the horizontal direction. */
					T2K_TransformYFunits( t, hori.underlinePosition,  &t->xUnderlinePosition,  &t->yUnderlinePosition);
					T2K_TransformYFunits( t, hori.underlineThickness, &t->xUnderlineThickness, &t->yUnderlineThickness);
				}
			}
			if ( vert.isValid ) {
				t->vert_xAscender 	= vert.Ascender;	t->vert_xAscender	<<= 16;
				t->vert_yAscender 	= 0;
				t->vert_xDescender	= vert.Descender;	t->vert_xDescender	<<= 16;
				t->vert_yDescender	= 0;
				t->vert_xLineGap	= vert.LineGap;		t->vert_xLineGap	<<= 16;
				t->vert_yLineGap	= 0;
				t->vert_yMaxLinearAdvanceWidth	= vert.maxAW;	t->vert_yMaxLinearAdvanceWidth		<<= 16;
				t->vert_xMaxLinearAdvanceWidth	= 0;
				t->vert_caretDx		= vert.caretDx;
				t->vert_caretDy		= vert.caretDy;
				
				t->verticalFontMetricsAreValid 	= true;
				if ( usedOutlines ) {
					/* We need to scale */
					if ( !t->is_Identity ) {
						F16Dot16 tmpX = t->vert_caretDx;
						F16Dot16 tmpY = t->vert_caretDy;
						t->vert_caretDx = util_FixMul( t->t00, tmpX ) + util_FixMul( t->t01, tmpY );
						t->vert_caretDy = util_FixMul( t->t10, tmpX ) + util_FixMul( t->t11, tmpY );
					}
					T2K_TransformXFunits( t, vert.Ascender,	&t->vert_xAscender,					&t->vert_yAscender);
					T2K_TransformXFunits( t, vert.Descender,&t->vert_xDescender,				&t->vert_yDescender);
					T2K_TransformXFunits( t, vert.LineGap, 	&t->vert_xLineGap,					&t->vert_yLineGap);
					T2K_TransformYFunits( t, (short)vert.maxAW, &t->vert_xMaxLinearAdvanceWidth,	&t->vert_yMaxLinearAdvanceWidth );
				}
			}
		}
	} else {
		/* catch */
		tsi_EmergencyShutDown( t->mem );
	}
}

	
static F26Dot6 scaleValue( T2K *t, F26Dot6 value, long ppem )
{
	register int16 UPEM = (int16)t->upem;
	
	value *= ppem*64;
	value += UPEM>>1;
	value /= UPEM;
	return value;
}

/* Not for external use */
static void T2K_PurgeMemoryInternal( register T2K *t, int level )
{
	Delete_GlyphClass( t->glyph ); t->glyph = NULL;
	/* tsi_DeAllocMem( t->mem, t->x ); t->x = NULL; t->y = NULL; */
	
	if ( level > 0 ) {
		if ( t->baseAddr != NULL && t->internal_baseAddr ) {
			tsi_FastDeAllocN( t->mem, t->baseAddr, T2K_FB_SC_BITMAP ); t->baseAddr = NULL;
			if ( level > 99 && tsi_FastSizeN(t->mem, T2K_FB_SC_BITMAP) > 256 ) {
				tsi_FastReleaseN( t->mem, T2K_FB_SC_BITMAP );
			}
		}
		if ( t->baseARGB != NULL && t->internal_baseARGB ) {
			tsi_DeAllocMem( t->mem, t->baseARGB ); t->baseARGB = NULL;
		}

#ifdef ENABLE_AUTO_GRIDDING_CORE
			if ( level > 1 ) {
				int err;
				err = ag_HintEnd( t->hintHandle ); t->hintHandle = NULL;
				t->ag_xPixelsPerEm = t->ag_yPixelsPerEm = -1;
				tsi_Assert( t->mem, err == 0, err  );
			}
#endif
	}
}

void T2K_PurgeMemory( register T2K *t, int level, int *errCode )
{
	assert( errCode != NULL );
	if ( (*errCode = setjmp(t->mem->env)) == 0 ) {
		/* try */
		T2K_PurgeMemoryInternal( t, level );
	} else {
		/* catch */
		tsi_EmergencyShutDown( t->mem );
	}
}





/*
 *
 */
static void T2K_RenderGlyphInternal( T2K *t, long aCode, int depth, int8 xFracPenDelta, int8 yFracPenDelta, uint8 greyScaleLevel, uint16 cmd )
{
	F26Dot6 *xPtr, *yPtr;
	int pointCount;
	long i, n;
	register GlyphClass *glyph;
	int isFigure = false;
	uint16 aw, ah;
	register uint16 UPEM;
	int doNativeHints;
	int doAutoHints;
	short agCmd;
	int activate_droputcontrol 	= true;
	int smart_droput 			= true;
	int include_stubs 			= false;
	int isTTfont = 0;	/* These variables refer to either TT or T1-like native hints */
	int isT1font = 0;
	int isPFRfont = 0;
	int isStrokeFont = 0;
	

	doNativeHints = cmd & T2K_NAT_GRID_FIT;
	if ( doNativeHints ) {
		isTTfont = !(t->font->head == NULL ||
					(t->font->head->glyphDataFormat >= 2000 &&
					t->font->head->glyphDataFormat <= 2002));
#ifdef ENABLE_T1
		isT1font = (t->font->T1 != NULL);	/* Add CFF(T2) and PFR later */
#endif
#ifdef ENABLE_PFR
		isPFRfont = (t->font->PFR != NULL);	
#endif
		isStrokeFont = (t->font->head != NULL) && (t->font->head->glyphDataFormat == 2002);
		
		cmd &= ~T2K_GRID_FIT; /* Turn off run time-auto-hints */
		if (isTTfont) {
#ifdef ENABLE_NATIVE_TT_HINTS
            int lowestRecPPEM = t->font->head->lowestRecPPEM;

            if ( t->xPixelsPerEm < lowestRecPPEM || t->yPixelsPerEm < lowestRecPPEM ) {
                    doNativeHints = false;
                    cmd &= ~T2K_NAT_GRID_FIT;
                    include_stubs = true;
            }
#else
            include_stubs = true;
            doNativeHints = false;          /* Ooops code not compiled in :-( */
            cmd &= ~T2K_NAT_GRID_FIT;       /* We can't do native hints, */
#ifdef ENABLE_AUTO_GRIDDING_CORE
            cmd |= T2K_GRID_FIT;            /* but we can do run-time auto-hints instead */
#endif
#endif
		} else if (isT1font || isPFRfont ) {
			include_stubs = true;
#ifdef ENABLE_NATIVE_T1_HINTS
			; /* OK We can do native hints :-) (in pixel space ) */
#else
			doNativeHints = false;		/* Ooops code not compiled in :-( */
			cmd &= ~T2K_NAT_GRID_FIT;	/* We can't do native hints, */
#ifdef ENABLE_AUTO_GRIDDING_CORE
			cmd |= T2K_GRID_FIT;		/* but we can do run-time auto-hints instead */
#endif
#endif
		} else if ( isStrokeFont ) {
			doNativeHints = false; /* Stroke font native hints are done elsewere since they are different and done in the original domain and not pixel space  */
			cmd &= ~T2K_NAT_GRID_FIT; /* Don't attempt any native hint in the pixel-space */
			assert( !(cmd & T2K_GRID_FIT )); /* We also do not want to do run-time auto-hints for this format. */
		} else {
			doNativeHints = false;
			cmd &= ~T2K_NAT_GRID_FIT;	/* We can't do native hints, */
#ifdef ENABLE_AUTO_GRIDDING_CORE
			cmd |= T2K_GRID_FIT;		/* but we can do run-time auto-hints instead */
#endif
		}
	}

#ifdef ENABLE_AUTO_GRIDDING_CORE
	doAutoHints = !doNativeHints && (cmd & (T2K_GRID_FIT | T2K_NAT_GRID_FIT | T2K_Y_ALIGN ));
	agCmd = (short)(cmd & T2K_Y_ALIGN ? CMD_AUTOGRID_YHEIGHTS : CMD_AUTOGRID_ALL);
#else
	assert(( cmd & (T2K_Y_ALIGN | T2K_GRID_FIT )) == 0 ); /* Please fix CONFIG.H if you hit this assert. */
	agCmd = 0;
	doAutoHints   = false;
#endif
	
	tsi_Assert( t->mem, /* greyScaleLevel >= BLACK_AND_WHITE_BITMAP && */ greyScaleLevel <= GREY_SCALE_BITMAP_EXTREME_QUALITY, T2K_ERR_BAD_GRAY_CMD  );
/*	tsi_Assert( t->mem, xFracPenDelta >= 0 && xFracPenDelta < 64 && yFracPenDelta >= 0 && yFracPenDelta < 64, T2K_ERR_BAD_FRAC_PEN  ); */

	
	UPEM = t->upem;
	T2K_PurgeMemoryInternal( t, 1 );
	FF_SET_FONT_SCALE( t->font, t->xScale.fixedScale, t->yScale.fixedScale ); /* For Stroke Font Hints */

	
	if ( cmd & T2K_CODE_IS_GINDEX ) {
		glyph = GetGlyphByIndex( t->font, aCode, (char)doNativeHints, &aw, &ah );
		isFigure = IsFigure( t->font, (unsigned short)aCode );
		if ( depth == 0 ) t->glyphIndex = (unsigned short)aCode;
	} else {
		glyph = GetGlyphByCharCode( t->font, ((uint16)aCode), (char)doNativeHints, &aw, &ah );
		if ( depth == 0 ) t->glyphIndex = glyph->myGlyphIndex;
		if ( aCode >= '0' && aCode <= '9' ) isFigure = true;
	}

	tsi_Assert( t->mem, glyph != NULL, T2K_ERR_GOT_NULL_GLYPH  );
	t->glyph = glyph;
	if ( glyph->contourCount < 0 ) {
		GlyphClass *base = NULL;
		uint16 comp_cmd;
		uint16 flags, oredFlags = 0;
		T2K_TRANS_MATRIX save, newbie;
		long save_XPPEM, save_YPPEM;
		int newT;
		short *componentData	= glyph->componentData; /* Grab the componentData! */
		uint8 *hintFragment		= glyph->hintFragment; /* Grab the hintFragment! */
		long hintLength			= glyph->hintLength;
		
		
		glyph->componentData	= NULL;
		glyph->hintFragment		= NULL;
		glyph->hintLength		= 0;
		
		save.t00 = t->t00;
		save.t01 = t->t01;
		save.t10 = t->t10;
		save.t11 = t->t11;
		save_XPPEM = t->xPixelsPerEm;
		save_YPPEM = t->yPixelsPerEm;
		comp_cmd = (unsigned char)(cmd & (T2K_GRID_FIT | T2K_Y_ALIGN | T2K_NAT_GRID_FIT | T2K_USE_FRAC_PEN) );
		comp_cmd |= T2K_RETURN_OUTLINES | T2K_CODE_IS_GINDEX;
/* #define KOREA_EXPERIMENT */
#define KOREA_EXPERIMENT
#ifdef KOREA_EXPERIMENT
	if ( t->fontCategory == ag_KANJI || (agCmd == CMD_AUTOGRID_YHEIGHTS) ) {
		comp_cmd &= ~(T2K_GRID_FIT | T2K_Y_ALIGN);
	}
#endif
		i = 0;
		do {
			long gIndex;
			long arg1, arg2;
			
			flags  = (unsigned short)componentData[i++]; oredFlags |= flags;
			gIndex = (uint16)componentData[i++];
 			if ( (flags & ARG_1_AND_2_ARE_WORDS) != 0 ) {
				arg1 = componentData[i++];
				arg2 = componentData[i++];
 			} else {
				arg1 = componentData[i++];
				/* Signed/unsigned bug fixed Dec 8, 1998 ---Sampo */
 				if ( flags & ARGS_ARE_XY_VALUES ) {
	 				arg2 = (long)((int8)(arg1 & 0xff));
	 				arg1 >>= 8;
 				} else {
	 				arg2 = arg1 & 0xff;
	 				arg1 >>= 8;
 					arg1 &= 0xff;
 				}
 			}

 			/*newT = false;*/
 			if ( flags & ARGS_ARE_XY_VALUES ) {
 				arg1 = scaleValue( t, arg1, t->xPixelsPerEm );
 				arg2 = scaleValue( t, arg2, t->yPixelsPerEm );
 			}
 			if ( (flags & WE_HAVE_A_SCALE) != 0 ) {
				newbie.t00 = ((F16Dot16)componentData[i]) << 2;
				newbie.t01 = 0;
				newbie.t10 = 0;
				newbie.t11 = ((F16Dot16)componentData[i]) << 2;
				i++;
				newT = true;
 			} else if ( (flags & WE_HAVE_AN_X_AND_Y_SCALE) != 0 ) {
				newbie.t00 = ((F16Dot16)componentData[i++]) << 2;
				newbie.t01 = 0;
				newbie.t10 = 0;
				newbie.t11 = ((F16Dot16)componentData[i++]) << 2;
				newT = true;
 			} else if ( (flags & WE_HAVE_A_TWO_BY_TWO) != 0 ) {
				newbie.t00 = ((F16Dot16)componentData[i++]) << 2;
				newbie.t01 = ((F16Dot16)componentData[i++]) << 2;
				newbie.t10 = ((F16Dot16)componentData[i++]) << 2;
				newbie.t11 = ((F16Dot16)componentData[i++]) << 2;
				newT = true;
 			} else { /* New, May 20 1999 */
				newbie.t00 = ONE16Dot16;
				newbie.t01 = 0;
				newbie.t10 = 0;
				newbie.t11 = ONE16Dot16;
				newT = true;
 			}
 			if ( newT ) {
 				T2K_NewTransformationInternal( t, false, t->xPixelsPerEm, t->yPixelsPerEm, &newbie );
 			}
			/* T2K_RenderGlyph( t, gIndex, xFracPenDelta, yFracPenDelta, 0, comp_cmd, NULL ); */
			T2K_RenderGlyphInternal( t, gIndex, depth+1, xFracPenDelta, yFracPenDelta, 0, comp_cmd );

				
 			if ( newT ) {
 				T2K_NewTransformationInternal( t, false, save_XPPEM, save_YPPEM, &save );
 			}
 			
			Add_GlyphClass( &base, t->glyph, flags, arg1, arg2 );
			if ( base != t->glyph ) {
				Delete_GlyphClass( t->glyph );
			}
			t->glyph = NULL;
		} while ( flags & MORE_COMPONENTS );
		t->glyph = glyph = base;
				
		tsi_FastDeAllocN( t->mem, glyph->hintFragment, T2K_FB_HINTS );
		glyph->hintFragment		= NULL;
		glyph->hintLength		= 0;
		glyph->hintFragment		= hintFragment; /* Put back the top-level composite hints! */
		glyph->hintLength		= hintLength;
		tsi_DeAllocMem( t->mem, componentData );
		xPtr = glyph->x;
		yPtr = glyph->y;
		pointCount = glyph->pointCount; n = pointCount + SbPtCount;

		if ( !(oredFlags & USE_MY_METRICS) ) {
			glyph->oox[glyph->pointCount + 0] = 0;
			glyph->oox[glyph->pointCount + 1] = (short)(glyph->oox[t->glyph->pointCount + 0] + aw);
			glyph->x[glyph->pointCount + 1]   = scaleValue( t, glyph->oox[glyph->pointCount + 1], t->xPixelsPerEm );
			if (cmd & (T2K_GRID_FIT | T2K_NAT_GRID_FIT) ) {
				glyph->x[glyph->pointCount + 1] += 32;
				glyph->x[glyph->pointCount + 1] &= ~63;
			}
			glyph->x[glyph->pointCount + 0]   = 0; /* == scaleValue(0) */
		}
		
		
#ifdef KOREA_EXPERIMENT
		if ( (cmd & T2K_NAT_GRID_FIT) || (agCmd == CMD_AUTOGRID_YHEIGHTS) || (t->fontCategory == ag_KANJI && doAutoHints ) ) {
			/* inverse-scale glyph */
			register F26Dot6 tmpX, tmpY;
			register F16Dot16 xMul = util_FixDiv( UPEM, t->xPixelsPerEm*64 );
			register F16Dot16 yMul = util_FixDiv( UPEM, t->yPixelsPerEm*64 );
			
			pointCount = glyph->pointCount;
			n    = pointCount + SbPtCount;
			for ( i = 0; i < n; i++ ) {
				tmpX = xPtr[i];
				tmpY = yPtr[i];
				tmpX = util_FixMul( tmpX, xMul );
				tmpY = util_FixMul( tmpY, yMul );
				glyph->oox[i] = (short)tmpX;
				glyph->ooy[i] = (short)tmpY;
			}
#ifdef ENABLE_NATIVE_TT_HINTS
				if ( doNativeHints && isTTfont) {
					GridOutline_T2KTTClass( (T2KTTClass *)t->font->t2kTT, glyph );
				}
#endif
#ifdef ENABLE_AUTO_GRIDDING_CORE
				if ( doAutoHints ) {
					int err;
					ag_ElementType elem;

					T2KDoGriddingSetUp( t );
					elem.contourCount   = glyph->contourCount;
					tsi_Assert( t->mem, pointCount <= 32000, T2K_ERR_TOO_MANY_POINTS  );
					
					elem.pointCount 	= (short)glyph->pointCount;
					elem.sp 			= glyph->sp;
					elem.ep 			= glyph->ep;
					elem.oox 			= glyph->oox;
					elem.ooy 			= glyph->ooy;
					elem.onCurve 		= glyph->onCurve;
					elem.x 				= xPtr;
					elem.y 				= yPtr;
					/* justYAlign */
					err = ag_AutoGridOutline( t->hintHandle, &elem, agCmd, (short)isFigure, glyph->curveType, (short)(greyScaleLevel > BLACK_AND_WHITE_BITMAP), SbPtCount );
					if ( err == CMD_AUTOGRID_YHEIGHTS_DISABLED ) {
						err = 0;
						scalePoints(glyph->ooy, yPtr, n, &t->yScale );
					}
					tsi_Assert( t->mem, err == 0, err  );
				}
#endif /* ENABLE_AUTO_GRIDDING_CORE */
		}
#endif /* KOREA_EXPERIMENT */
	} else {
		pointCount = glyph->pointCount;
		n    = pointCount + SbPtCount;
		/*
		xPtr = (long*) tsi_AllocMem( t->mem, (n) * 2 * sizeof( long ) );
		yPtr = &xPtr[ n ];
		glyph->x = xPtr;
		glyph->y = yPtr;
		*/
		xPtr = glyph->x;
		yPtr = glyph->y;
		
#ifdef ENABLE_T2KE
		if ( t->font != NULL && t->font->T2KE != NULL && t->font->T2KE->properties[T2KE_PROP_HINTS_DISABLED] != 0 ) {
			cmd &= ~(T2K_GRID_FIT | T2K_Y_ALIGN | T2K_NAT_GRID_FIT);
		}
#endif		
		if ( (doNativeHints || doAutoHints) && !(cmd & T2K_USE_FRAC_PEN) && pointCount > 0 ) {
			;
#ifdef ENABLE_NATIVE_TT_HINTS
				if ( doNativeHints && isTTfont) {
					/* Scale the glyph */
					scalePoints(glyph->oox, xPtr, n, &t->xScale );
					scalePoints(glyph->ooy, yPtr, n, &t->yScale );
					GridOutline_T2KTTClass( (T2KTTClass *)t->font->t2kTT, glyph );
				}
#endif



#ifdef ENABLE_NATIVE_T1_HINTS
#ifdef ENABLE_T1
				if ( doNativeHints && isT1font) {

					/* Scale the glyph */
					SetScale_FFT1HintClass( t->font->ffhint, t->xPixelsPerEm, t->yPixelsPerEm );



					SetupGlobalHints( t->font->ffhint, t->font->T1->numBlueValues, 
										t->font->T1->BlueFuzz,
										t->font->T1->BlueScale,
										t->font->T1->BlueShift,
										t->font->T1->BlueValues,
										t->font->T1->StdVW,
										t->font->T1->StdHW,
										t->font->T1->StemSnapV,
										t->font->T1->StemSnapH,
										t->font->T1->numStemSnapV,
										t->font->T1->numStemSnapH
										);



					ApplyHints_FFT1HintClass( t->font->ffhint, (short)pointCount, (short)SbPtCount, glyph );

				}
#endif
#endif

#ifdef ENABLE_NATIVE_T1_HINTS
#ifdef ENABLE_PFR
				if ( doNativeHints && isPFRfont) {


					SetScale_FFT1HintClass( t->font->ffhint, t->xPixelsPerEm, t->yPixelsPerEm );




					SetupGlobalHints( t->font->ffhint, t->font->PFR->numBlueValues, 
										t->font->PFR->BlueFuzz,
										t->font->PFR->BlueScale,
										t->font->PFR->BlueShift,
										t->font->PFR->BlueValues,
										t->font->PFR->StdVW,
										t->font->PFR->StdHW,
										t->font->PFR->StemSnapV,
										t->font->PFR->StemSnapH,
										t->font->PFR->nStemSnapV,
										t->font->PFR->nStemSnapH
										);


					ApplyHints_FFT1HintClass( t->font->ffhint, (short)pointCount, (short)SbPtCount, glyph );

				}
#endif
#endif

#ifdef ENABLE_AUTO_GRIDDING_CORE
				if ( doAutoHints ) {
					int err;
					ag_ElementType elem;
					
					T2KDoGriddingSetUp( t );
					elem.contourCount   = glyph->contourCount;
					tsi_Assert( t->mem, pointCount <= 32000, T2K_ERR_TOO_MANY_POINTS  );
		
					elem.pointCount 	= (short)pointCount;
					elem.sp 			= glyph->sp;
					elem.ep 			= glyph->ep;
					elem.oox 			= glyph->oox;
					elem.ooy 			= glyph->ooy;
					elem.onCurve 		= glyph->onCurve;
					elem.x 				= xPtr;
					elem.y 				= yPtr;
					/* Scale the glyph */
					scalePoints(glyph->oox, xPtr, n, &t->xScale );
					if ( agCmd != CMD_AUTOGRID_YHEIGHTS ) scalePoints(glyph->ooy, yPtr, n, &t->yScale );
					err = ag_AutoGridOutline( t->hintHandle, &elem, agCmd, (short)isFigure, glyph->curveType, (short)(greyScaleLevel > BLACK_AND_WHITE_BITMAP), SbPtCount );
					if ( err == CMD_AUTOGRID_YHEIGHTS_DISABLED ) {
						err = 0;
						scalePoints(glyph->ooy, yPtr, n, &t->yScale );
					}
					
					tsi_Assert( t->mem, err == 0, err  );
				}
#endif /* ENABLE_AUTO_GRIDDING_CORE */
		} else  {
			/* Scale the glyph */
			scalePoints(glyph->oox, xPtr, n, &t->xScale );
			scalePoints(glyph->ooy, yPtr, n, &t->yScale );
			if ( cmd & T2K_USE_FRAC_PEN ) {
				/* Apply FracPenDelta, but not to the side bearing points */
				if ( xFracPenDelta != 0 ) {
					for ( i = 0; i < pointCount; i++ ) {
						xPtr[i] += xFracPenDelta;
					}
				}
				if ( yFracPenDelta != 0 ) {
					for ( i = 0; i < pointCount; i++ ) {
						yPtr[i] -= yFracPenDelta;
					}
				}
			}
		}
		
	}
	t->xLinearAdvanceWidth16Dot16 = aw;			t->xLinearAdvanceWidth16Dot16 <<= 16;
	t->yLinearAdvanceWidth16Dot16 = 0;

	t->vert_xLinearAdvanceWidth16Dot16 = 0;
	t->vert_yLinearAdvanceWidth16Dot16 = ah;	t->vert_yLinearAdvanceWidth16Dot16 <<= 16;
	
#ifdef ENABLE_T1
	if ( t->font->T1 != NULL && t->font->T1->m01 != 0 ) {
		F16Dot16 skew = t->font->T1->m01;
		for ( i = 0; i < n; i++ ) {
			xPtr[i] += util_FixMul( skew, yPtr[i] );
		}
	} 
#endif
#ifdef ENABLE_CFF
	if ( t->font->T2 != NULL && t->font->T2->topDictData.m01 != 0 ) {
		F16Dot16 skew = t->font->T2->topDictData.m01;
		for ( i = 0; i < n; i++ ) {
			xPtr[i] += util_FixMul( skew, yPtr[i] );
		}
	} 
#endif
	if ( depth == 0 ) {
		int xMicroPosition = (cmd & (T2K_TV_MODE | T2K_LCD_MODE)) && !doNativeHints;
		{
			register F26Dot6 error = xPtr[pointCount]; /* The lsb point */
			if ( error != 0 ) {
				for ( i = 0; i < n; i++ ) {
					xPtr[i] -= error;
				}		
			}
			error = yPtr[pointCount]; /* The lsb point */
			if ( error != 0 ) {
				for ( i = 0; i < n; i++ ) {
					yPtr[i] -= error;
				}		
			}
		}

		if ( !(cmd & T2K_USE_FRAC_PEN) ) {
			/* To ensure integer spacing for no gridfitting, and something like the space character with zero points */
			F26Dot6 x1Old, x1New;
			F26Dot6 x2Old, x2New;
			F26Dot6 wOld, wNew;
			x1Old 	= xPtr[pointCount];
			x2Old 	= xPtr[pointCount+1];
			wOld	= x2Old - x1Old;
			
			x1New	= x1Old;					/* Initialize. */
			wNew	= wOld;
			
			x1New	+=  32; x1New &= ~63; 		/* Integralize. */
			wNew	+=  32; wNew &= ~63;
			x2New	= x1New + wNew;
			
			xPtr[pointCount] 	= x1New;		/* Write out. */
			xPtr[pointCount+1]  = x2New;		/* Write out. */
			
			if ( xMicroPosition && pointCount > 0 ) {
				F26Dot6 xShift = (x1New - x1Old + x2New - x2Old) >> 1; /* Do this, 10/19/98 ---Sampo */;
								
				if ( xShift != 0 ) {
					for ( i = 0; i < pointCount; i++ ) {
						xPtr[i] += xShift;
					}
				}
			}
		}
	}
	
	{
		/*
		F16Dot16 mul;
		
		mul = util_FixDiv( t->xPixelsPerEm16Dot16, ((long)(UPEM)) << 16);
		*/
		t->xLinearAdvanceWidth16Dot16		= util_FixMul( t->xLinearAdvanceWidth16Dot16, t->xMul );
		t->vert_yLinearAdvanceWidth16Dot16	= util_FixMul( t->vert_yLinearAdvanceWidth16Dot16, t->yMul );
		
		/* go to 16.16 for the side bearing points */
		xPtr[pointCount] <<= 10;
		yPtr[pointCount] <<= 10;
		xPtr[pointCount+1] <<= 10;
		yPtr[pointCount+1] <<= 10;
		
#if SbPtCount >= 4
		xPtr[pointCount+2] <<= 10;
		yPtr[pointCount+2] <<= 10;
		xPtr[pointCount+3] <<= 10;
		yPtr[pointCount+3] <<= 10;
#endif
	}
	
	if ( !t->is_Identity  ) {
		F16Dot16 t00 = t->t00;
		F16Dot16 t01 = t->t01;
		F16Dot16 t10 = t->t10;
		F16Dot16 t11 = t->t11;
		register F26Dot6 tmpX, tmpY;
		
		if ( t01 == 0 && t10 == 0 ) {
			for ( i = 0; i < n; i++ ) {
				tmpX = xPtr[i]; tmpY = yPtr[i];
				xPtr[i] = util_FixMul( t00, tmpX ) + 0;
				yPtr[i] = 0 + 						 util_FixMul( t11, tmpY );
			}
		} else {
			for ( i = 0; i < n; i++ ) {
				tmpX = xPtr[i]; tmpY = yPtr[i];
				xPtr[i] = util_FixMul( t00, tmpX ) + util_FixMul( t01, tmpY );
				yPtr[i] = util_FixMul( t10, tmpX ) + util_FixMul( t11, tmpY );
			}
		}
		tmpX = t->xLinearAdvanceWidth16Dot16; /*tmpY = 0;*/
		t->xLinearAdvanceWidth16Dot16 = util_FixMul( t00, tmpX ) /* + util_FixMul( t01, tmpY ) */;
		t->yLinearAdvanceWidth16Dot16 = util_FixMul( t10, tmpX ) /* + util_FixMul( t11, tmpY ) */;

		/*tmpX = 0*/; tmpY = t->vert_yLinearAdvanceWidth16Dot16;
		t->vert_xLinearAdvanceWidth16Dot16 = /* util_FixMul( t00, tmpX ) + */ util_FixMul( t01, tmpY );
		t->vert_yLinearAdvanceWidth16Dot16 = /* util_FixMul( t10, tmpX ) + */ util_FixMul( t11, tmpY );
	}
	t->xAdvanceWidth16Dot16	= xPtr[pointCount+1] - xPtr[pointCount];
	t->yAdvanceWidth16Dot16	= yPtr[pointCount+1] - yPtr[pointCount];
#if SbPtCount >= 4
	t->vert_xAdvanceWidth16Dot16	= -(xPtr[pointCount+3] - xPtr[pointCount+2]);
	t->vert_yAdvanceWidth16Dot16	= -(yPtr[pointCount+3] - yPtr[pointCount+2]);
#endif

	/* Translate back to 26.6 from 16.16 */
	xPtr[pointCount] >>= 10;
	yPtr[pointCount] >>= 10;
	xPtr[pointCount+1] >>= 10;
	yPtr[pointCount+1] >>= 10;
#if SbPtCount >= 4
	xPtr[pointCount+2] >>= 10;
	yPtr[pointCount+2] >>= 10;
	xPtr[pointCount+3] >>= 10;
	yPtr[pointCount+3] >>= 10;
#endif
	
	/* Initialize */
	t->baseARGB = NULL;
	
	t->baseAddr = NULL;	t->rowBytes = 0;
	t->width = 0; 		t->height = 0;
	t->fTop26Dot6 = 0;	t->fLeft26Dot6 = 0;
	t->vert_fTop26Dot6 = 0;	t->vert_fLeft26Dot6 = 0;
	
	glyph->dropOutControl = true;
	{
		long maxSize = 255;
#ifdef ENABLE_NATIVE_TT_HINTS
		if ( doNativeHints  && isTTfont) {
			if ( t->is_Identity ) {
				if ( (t->font->t2kTT->globalGS.localParBlock.scanControl & 0xff ) != 0xff ) {
					maxSize = t->font->t2kTT->globalGS.localParBlock.scanControl & 0xff;
				}
				/* See if bits 12 or 13 are set. */
				if ( t->font->t2kTT->globalGS.localParBlock.scanControl & (2<<12) ) {
					maxSize = 0;
				}
			}
			activate_droputcontrol	= (t->font->t2kTT->globalGS.localParBlock.scanControl & ACTIVATE_DROPOUTCONTROL) ? true : false;
			smart_droput 			= (t->font->t2kTT->globalGS.localParBlock.scanControl & SMART_DROPOUT) ? true : false;
			include_stubs 			= (t->font->t2kTT->globalGS.localParBlock.scanControl & INCLUDE_STUBS) ? true : false;
			glyph->dropOutControl   = (char)((int)glyph->dropOutControl && activate_droputcontrol);

		}
#endif
		if ( t->xPixelsPerEm > maxSize ) {
			glyph->dropOutControl = false;
		}
	}
	if ( cmd & T2K_SCAN_CONVERT && glyph->pointCount > 0 ) {
#ifdef ENABLE_STRKCONV
		ffStrkConv *sk = NULL;
#endif
		tsiScanConv *sc = NULL;
#ifdef ENABLE_T2KE
		tsiColorScanConv *scc = NULL;
#endif

		char xWeightIsOne = (char)(t->xWeightIsOne && (cmd & T2K_GRID_FIT) && t->ag_xPixelsPerEm <= 24);
	
		if ( glyph->colorPlaneCount > 0 ) {
#ifdef ENABLE_T2KE
			assert( t->font->T2KE != NULL );
			scc = tsi_NewColorScanConv( t->mem, (void *)t->font->T2KE, (void*)t, glyph->contourCount, glyph->sp, glyph->ep,
								 	glyph->colors, glyph->colorPlaneCount,
				                  	xPtr, yPtr, (char *)glyph->onCurve, greyScaleLevel, glyph->oox, glyph->ooy, t->upem, t->oneHalfFUnit );
			MakeColorBits( scc, greyScaleLevel, xWeightIsOne, (char)(cmd & T2K_SKIP_SCAN_BM), t->GetCacheMemory, t->theCache );
			t->internal_baseARGB = scc->internal_baseARGB;
			t->width  		= scc->right  - scc->left;
			t->height 		= scc->bottom - scc->top;
			t->fTop26Dot6	= scc->fTop26Dot6;
			t->fLeft26Dot6  = scc->fLeft26Dot6;
			t->vert_fTop26Dot6	 = scc->fTop26Dot6;
			t->vert_fLeft26Dot6  = scc->fLeft26Dot6;
			t->rowBytes 	= scc->rowBytes;
#else
			assert( false );
#endif
		} else {
			char xDropOutControl, yDropOutControl;
			uint16 gIndex = T2K_GetGlyphIndexFast(t);
			
			xDropOutControl = yDropOutControl = glyph->dropOutControl;
			if ( greyScaleLevel == 0 && t->is_Identity  && xDropOutControl ) {
				if ( !needXDropout( t->tAdapt, gIndex )) xDropOutControl = false;
				if ( !needYDropout( t->tAdapt, gIndex )) yDropOutControl = false;
			}
#ifdef ENABLE_STRKCONV
			if ( t->font->head != NULL && t->font->head->glyphDataFormat == 2002 && !t->font->strokeGlyph )  {
				long xRadius, yRadius, radius;
				
				assert( t->font->ffst != NULL );
				radius = t->font->ffst->minRadius + util_FixMul( t->font->currentCoordinate[0], t->font->ffst->maxRadius - t->font->ffst->minRadius ); 
				xRadius = util_FixMul( radius, t->font->xScale );
				yRadius = util_FixMul( radius, t->font->yScale );
				sk = ff_NewStrkConv( t->mem, glyph->contourCount, glyph->sp, glyph->ep,
								xPtr, yPtr, (char *)glyph->onCurve );
				if ( t->okForBitCreationToTalkToCache ) {
					MakeStrkBits( sk, (char)(cmd & T2K_SKIP_SCAN_BM), t->GetCacheMemory, t->theCache, t->bitRange255, t->remapBits, xRadius, yRadius );
				} else {
					MakeStrkBits( sk, (char)(cmd & T2K_SKIP_SCAN_BM), NULL, NULL ,t->bitRange255, t->remapBits, xRadius, yRadius);
				}
				t->internal_baseAddr = sk->internal_baseAddr;
				t->width  		= sk->right  - sk->left;
				t->height 		= sk->bottom - sk->top;
				t->fTop26Dot6	= sk->fTop26Dot6;
				t->fLeft26Dot6  = sk->fLeft26Dot6;
				t->vert_fTop26Dot6	= sk->fTop26Dot6;
				t->vert_fLeft26Dot6	= sk->fLeft26Dot6;
				t->rowBytes 	= sk->rowBytes;
			} else
#endif
			{
#ifdef TEST_CURVE_CONVERSION_CODE
				glyph = FF_ConvertGlyphSplineTypeInternal( glyph, 2 );
				xPtr = glyph->x;yPtr = glyph->y;pointCount = glyph->pointCount;
				assert( false );
#endif
				sc = tsi_NewScanConv( t->mem, glyph->contourCount, glyph->sp, glyph->ep,
					                  xPtr, yPtr, (char *)glyph->onCurve, greyScaleLevel, (char)glyph->curveType, xDropOutControl, yDropOutControl, smart_droput, include_stubs, t->oneHalfFUnit );
				if (t->okForBitCreationToTalkToCache) {
					MakeBits( sc, xWeightIsOne, (char)(cmd & T2K_SKIP_SCAN_BM), t->GetCacheMemory, t->theCache, t->bitRange255, t->remapBits );
				} else {
					MakeBits( sc, xWeightIsOne, (char)(cmd & T2K_SKIP_SCAN_BM), NULL, NULL ,t->bitRange255, t->remapBits );
				}
				t->internal_baseAddr = sc->internal_baseAddr;
				t->width  		= sc->right  - sc->left;
				t->height 		= sc->bottom - sc->top;
				t->fTop26Dot6	= sc->fTop26Dot6;
				t->fLeft26Dot6  = sc->fLeft26Dot6;
				t->vert_fTop26Dot6	= sc->fTop26Dot6;
				t->vert_fLeft26Dot6	= sc->fLeft26Dot6;
				t->rowBytes 	= sc->rowBytes;
	            if ( greyScaleLevel == 0 && t->is_Identity && !(cmd & T2K_SKIP_SCAN_BM) ) {
					if ( !sc->weDidXDropouts ) xDropoutNotUsed( t->tAdapt, gIndex );
					if ( !sc->weDidYDropouts ) yDropoutNotUsed( t->tAdapt, gIndex );
				}
			}
		}
		{
			/* Center the character for vertical layout */
			t->vert_fLeft26Dot6	 = t->vert_fLeft26Dot6 - xPtr[pointCount+2];	
			t->vert_fTop26Dot6	 = t->vert_fTop26Dot6  - yPtr[pointCount+2];
		}


		
		if ( glyph->colorPlaneCount > 0 ) {
#ifdef ENABLE_T2KE
			t->baseARGB = scc->baseARGB; scc->baseARGB = NULL; /* We take over and deallocate t->baseARGB */
			t->baseAddr = NULL;
#else
			assert( false );
#endif
		} else {
#ifdef ENABLE_STRKCONV
			if ( sk != NULL ) {
				t->baseAddr = sk->baseAddr; sk->baseAddr = NULL; /* We take over and deallocate t->baseAddr */
			} else
#endif
			{
				t->baseAddr = sc->baseAddr; sc->baseAddr = NULL; /* We take over and deallocate t->baseAddr */
			}
			t->baseARGB = NULL;
		}
		
		/* Compensate if the lsb_point != 0 */
		if ( (i=xPtr[pointCount]) != 0 ) {
			t->fLeft26Dot6 -= i;
		}
		if ( (i=yPtr[pointCount]) != 0 ) {
			t->fTop26Dot6 -= i;
		}
		
		t->fLeft26Dot6 &= ~63;
		t->fTop26Dot6  &= ~63;
		
#ifdef ENABLE_T2KE
		tsi_DeleteColorScanConv( scc );
#endif
		tsi_DeleteScanConv( sc );
#ifdef ENABLE_STRKCONV
		ff_DeleteStrkConv( sk );
#endif
	}
	
	if ( cmd & T2K_RETURN_OUTLINES ) {
		;
	} else {
		T2K_PurgeMemoryInternal( t, 0 );
	}
}

int32 T2K_GetNumAxes(T2K *t)
{
	sfntClass *font;
	int32 numAxes = 0;
	
	font = t->font;
	assert( font != NULL );
	
#ifdef ENABLE_T1
	if ( font->T1 != NULL ) {
		numAxes = font->T1->numAxes;
	}
#endif
#ifdef ENABLE_CFF
	if ( font->T2 != NULL ) {
		numAxes = font->T2->topDictData.numAxes;
	}
#endif
#ifdef ENABLE_T2KE
	if ( font->T2KE != NULL ) {
		numAxes = font->T2KE->numAxes;
	}
#endif
#ifdef ENABLE_T2KS
	if ( t->font->head != NULL && t->font->head->glyphDataFormat == 2002 ) {
		assert( t->font->ffst != NULL );
		numAxes = t->font->ffst->numAxes;
	}
#endif
	/* granularity;  location; */
	return numAxes; /*****/
}

F16Dot16 T2K_GetAxisGranularity(T2K *t, int32 n)
{
	F16Dot16 granularity = ONE16Dot16;
	sfntClass *font = t->font;

	assert( font != NULL );
	assert( n >= 0 && n < T2K_GetNumAxes( t ) );
	
#ifdef ENABLE_T2KE
	if ( font->T2KE != NULL ) {
		granularity = font->T2KE->granularity[n];
	}
#else
	UNUSED( n );
#endif
	return granularity; /*****/
}

F16Dot16 T2K_GetCoordinate(T2K *t, int32 n )
{
	F16Dot16 coordinate;
	sfntClass *font = t->font;

	assert( font != NULL );
	assert( n >= 0 && n < T2K_GetNumAxes( t ) );
	
#ifdef ENABLE_T2KE
	if ( font->T2KE != NULL ) {
		coordinate = font->T2KE->currentCoordinate[n];
	}
#else
#ifdef ENABLE_T2KS
		coordinate = font->currentCoordinate[n];
#else
		UNUSED( n );
		coordinate = 0;
#endif
#endif
	return coordinate; /*****/
}

void T2K_SetCoordinate(T2K *t, int32 n, F16Dot16 value )
{
	sfntClass *font = t->font;

	assert( font != NULL );
	assert( n >= 0 && n < T2K_GetNumAxes( t ) );
	
#ifdef ENABLE_T2KE
	if ( font->T2KE != NULL ) {
		font->T2KE->currentCoordinate[n] = value;
	}
#else
#ifdef ENABLE_T2KS
		font->currentCoordinate[n] = value;
#else
		UNUSED( n );
		UNUSED( value );
#endif
#endif
}


#ifdef ENABLE_SBIT
/*
 * Query method to see if a particular glyph exists in sbit format for the current size.
 * If you need to use characterCode then map it to glyphIndex by using T2K_GetGlyphIndex() first.
 */
int T2K_GlyphSbitsExists( T2K *t, uint16 glyphIndex, int *errCode  )
{
	volatile int result = false; /* Initialize */
	
	uint16 ppemX = (uint16)t->xPixelsPerEm;
	uint16 ppemY = (uint16)t->yPixelsPerEm;
	assert( errCode != NULL );
	if ( (*errCode = setjmp(t->mem->env)) == 0 ) {
		/* try */
		tsi_Assert( t->mem, t->mem->state == T2K_STATE_ALIVE, T2K_ERR_USE_PAST_DEATH );
		/* See if we have an indentity transformation and if the bloc and bdat tables exist */
		if ( t->is_Identity && t->font->bloc != NULL && t->font->bdatOffset != 0 ) {
			result = FindGlyph_blocClass( t->font->bloc, t->font->ebsc, t->font->in, glyphIndex, ppemX, ppemY, &(t->font->bloc->gInfo) );
		}
	} else {
		/* catch */
		tsi_EmergencyShutDown( t->mem );
	}	
	return result; /*****/
}


/*
 * Gets the embeded bitmap
 */
static int T2K_GetSbits(T2K *scaler, long aCode, uint8 greyScaleLevel, uint16 cmd)
{
	int result = false;	/* Initialize */
	blocClass *bloc = scaler->font->bloc;
	ebscClass *ebsc = scaler->font->ebsc;
	
	/* See if we have an indentity transformation and if the bloc and bdat tables exist */
	if ( scaler->is_Identity && bloc != NULL && scaler->font->bdatOffset != 0 ) {
		uint16 ppemX = (uint16)scaler->xPixelsPerEm;
		uint16 ppemY = (uint16)scaler->yPixelsPerEm;
		sbitGlypInfoData *gInfo = &(bloc->gInfo);
		uint16 glyphIndex;
		
		glyphIndex = (uint16)((cmd & T2K_CODE_IS_GINDEX) ? aCode :
						GetSfntClassGlyphIndex(scaler->font, (uint16)aCode ));
		
		result = gInfo->glyphIndex == glyphIndex && ppemX == gInfo->ppemX && ppemY == gInfo->ppemY && gInfo->offsetA != 0;
		if ( !result ) {
			FindGlyph_blocClass( bloc, ebsc, scaler->font->in, glyphIndex, ppemX, ppemY, gInfo );
			result = gInfo->glyphIndex == glyphIndex && ppemX == gInfo->ppemX && ppemY == gInfo->ppemY && gInfo->offsetA != 0;
		}
		if ( result ) {
			ExtractBitMap_blocClass( bloc, ebsc, gInfo, scaler->font->in, scaler->font->bdatOffset, greyScaleLevel, 0 );
                
			scaler->baseAddr        = gInfo->baseAddr;      gInfo->baseAddr = NULL; /* Hand over the pointer. */
			scaler->internal_baseAddr = true;
			scaler->glyphIndex = glyphIndex;
			if (scaler->okForBitCreationToTalkToCache)
			{
				unsigned char *baseAddr;
				baseAddr = scaler->GetCacheMemory(scaler->theCache, gInfo->N);
				if (baseAddr != NULL)
				{
					register uint32 i, N = gInfo->N;
					for (i = 0; i < N; i++)
					{
						baseAddr[i] = scaler->baseAddr[i];
					}
					tsi_DeAllocMem(scaler->mem, scaler->baseAddr);
					scaler->baseAddr = baseAddr;
					scaler->internal_baseAddr = false;
				}
			}
			if ( scaler->baseAddr != NULL ) {
				scaler->xAdvanceWidth16Dot16 = gInfo->bigM.horiAdvance;
				/* scaler->yAdvanceWidth16Dot16 = gInfo->bigM.vertAdvance; */
				scaler->yAdvanceWidth16Dot16 = 0;
				
				scaler->rowBytes		= gInfo->rowBytes;	gInfo->rowBytes	= 0;
				scaler->width			= gInfo->bigM.width;	
				scaler->height			= gInfo->bigM.height;
				
				/* Set horizontal metrics. */
				scaler->horizontalMetricsAreValid = true;
				scaler->fLeft26Dot6		= gInfo->bigM.horiBearingX;	scaler->fLeft26Dot6		<<= 6;
				scaler->fTop26Dot6		= gInfo->bigM.horiBearingY; scaler->fTop26Dot6		<<= 6;
				scaler->xAdvanceWidth16Dot16		= gInfo->bigM.horiAdvance;
				scaler->xAdvanceWidth16Dot16 		<<= 16;
				scaler->xLinearAdvanceWidth16Dot16	= scaler->xAdvanceWidth16Dot16;
				scaler->yAdvanceWidth16Dot16 		= scaler->yLinearAdvanceWidth16Dot16 = 0;
				/* Set vertical metrics. */
				scaler->verticalMetricsAreValid = true;
				scaler->vert_fLeft26Dot6	= gInfo->bigM.vertBearingX;	scaler->vert_fLeft26Dot6	<<= 6;
				scaler->vert_fTop26Dot6		= gInfo->bigM.vertBearingY; scaler->vert_fTop26Dot6		<<= 6;
				scaler->vert_yAdvanceWidth16Dot16		= gInfo->bigM.vertAdvance;
				scaler->vert_yAdvanceWidth16Dot16 		<<= 16;
				scaler->vert_yLinearAdvanceWidth16Dot16	= scaler->vert_yAdvanceWidth16Dot16;
				scaler->vert_xAdvanceWidth16Dot16 		= scaler->vert_xLinearAdvanceWidth16Dot16 = 0;
				
				if ( gInfo->smallMetricsUsed ) {
					if ( !(gInfo->flags & SBIT_SMALL_METRIC_DIRECTION_IS_HORIZONTAL) ) {
						scaler->horizontalMetricsAreValid = false;
					}
					if ( !(gInfo->flags & SBIT_SMALL_METRIC_DIRECTION_IS_VERTICAL) ) {
						scaler->verticalMetricsAreValid = false;
					}
				}
			} else {
				result = false;
			}
		}
	}
	return result; /*****/
}
#endif /* ENABLE_SBIT */

/*
 * Modifies the greyScaleLevel and cmdIn paramers for T2K_RenderGlyph
 * according to the wishes of the gasp table if it exists and also dependend on
 * if ENABLE_GASP_TABLE_SUPPORT is defined. You can use it like this:
 * T2K_GaspifyTheCmds( scaler, &greyScaleLevel, &cmd );
 * T2K_RenderGlyph( scaler, charCode, 0, 0, greyScaleLevel, cmd, &errCode );
 */
void T2K_GaspifyTheCmds( T2K *t, uint8 *greyScaleLevelPtr, uint16 *cmdInPtr )
{
	uint8 greyScaleLevel	= *greyScaleLevelPtr;
	uint16 cmdIn 			= *cmdInPtr;
#ifdef ENABLE_GASP_TABLE_SUPPORT
	int useGridFitting, useGrayScaleRendering;
	if ( t->font && !(cmdIn & (T2K_LCD_MODE|T2K_TV_MODE)) &&
	     Read_gasp( t->font->gasp, t->xPixelsPerEm, &useGridFitting, &useGrayScaleRendering ) ) {
		if ( !useGrayScaleRendering && (cmdIn & (T2K_GRID_FIT | T2K_NAT_GRID_FIT)) ) {
			greyScaleLevel = BLACK_AND_WHITE_BITMAP;
		}
		if ( !useGridFitting ) {
			cmdIn &= ~(T2K_GRID_FIT | T2K_NAT_GRID_FIT);
		}
	}
#else
	UNUSED(t);
#endif /* ENABLE_GASP_TABLE_SUPPORT */
	*greyScaleLevelPtr	= greyScaleLevel;
	*cmdInPtr 			= cmdIn;
}

/*
 *
 */
void T2K_RenderGlyph( T2K *t, long aCode, int8 xFracPenDelta, int8 yFracPenDelta, uint8 greyScaleLevel, uint16 cmdIn, int *errCode )
{
	volatile uint16 cmd = cmdIn;
	assert( errCode != NULL );
	if ( (*errCode = setjmp(t->mem->env)) == 0 ) {
		/* try */
		tsi_Assert( t->mem, t->mem->state == T2K_STATE_ALIVE, T2K_ERR_USE_PAST_DEATH );
		FF_SET_NATIVE_HINTS( t->font, cmd & T2K_NAT_GRID_FIT);
#ifdef ENABLE_STRKCONV
		FF_SET_STROKING( t->font,
				!(greyScaleLevel != 0 && (t->xPixelsPerEm <= ENABLE_STRKCONV || t->yPixelsPerEm <= ENABLE_STRKCONV)) );
#endif
		t->font->greyScaleLevel = greyScaleLevel;
#ifndef ENABLE_AUTO_GRIDDING_CORE
		cmd &= ~T2K_GRID_FIT; /* Turn of GRID_FIT if code not enabled */
#endif
		assert( !( (cmd & (T2K_GRID_FIT | T2K_NAT_GRID_FIT) ) && (cmd & T2K_TV_MODE)) ); /* If you turn on T2K_TV_MODE, then please turn off T2K_GRID_FIT */

		t->okForBitCreationToTalkToCache = (int)((t->GetCacheMemory != NULL) && !( t->BitmapFilter || (cmd & T2K_LCD_MODE ) ));
#ifdef ENABLE_SBIT
#ifdef ENABLE_PFR
		if ( t->font->PFR != NULL && t->enableSbits && (cmd & T2K_SCAN_CONVERT) &&
							PFR_GetSbits(t, aCode, greyScaleLevel, cmd)) {
			t->embeddedBitmapWasUsed	= true;
#ifdef REVERSE_SC_Y_ORDER
			T2K_InvertBitmap(t);
#endif
		} else
#endif /* ENABLE_PFR */
		if ( t->enableSbits && (cmd & T2K_SCAN_CONVERT) &&
					T2K_GetSbits( t, aCode, greyScaleLevel, cmd )  ) {
			t->embeddedBitmapWasUsed 		= true;
#ifdef REVERSE_SC_Y_ORDER
			T2K_InvertBitmap(t);
#endif
		} else
#endif /* ENABLE_SBIT */
		{
			t->embeddedBitmapWasUsed		= false;
			T2K_RenderGlyphInternal( t, aCode, 0, xFracPenDelta, yFracPenDelta, greyScaleLevel, cmd );
			t->horizontalMetricsAreValid	= true;
			t->verticalMetricsAreValid		= false;
		}
#ifdef ENABLE_LCD_OPTION
		if ( cmd & T2K_LCD_MODE )  {
			T2K_WriteToGrayPixels( t );
		}
#endif
		if ( t->BitmapFilter != NULL ) {
			/* Filter */
			t->BitmapFilter( t, t->filterParamsPtr );
		}
	} else {
		/* catch */
		tsi_EmergencyShutDown( t->mem );
	}
}

#ifdef ENABLE_FF_CURVE_CONVERSION
/*
 * Call right after T2K_RenderGlyph used with the T2K_RETURN_OUTLINES bit set,
 * and before you call T2K_PurgeMemory().
 * This method creates a t->glyph with outlines of type curveTypeOut
 * independent of what the original curve type is.
 *
 * Valid values for curveTypeOut out are:
 * 1 : This returns a glyph composed of just straight line segments (a polyline).
 * 2 : This returns a glyph composed of 2nd order B-splines (parabolas) (and straight lines).
 * 3 : This returns a glyph composed of 3rd order Beziers (qubics) (and straight lines).
 *
 * The resulting outlines are in glyph->x/y/onCurve/sp/ep/contourCount and not in oox/ooy.
 *
 */
void T2K_ConvertGlyphSplineType( T2K *t, short curveTypeOut, int *errCode )
{
	if ( (*errCode = setjmp(t->mem->env)) == 0 ) {
		/* try */
		assert( t->glyph != NULL );
		t->glyph = FF_ConvertGlyphSplineTypeInternal( t->glyph, curveTypeOut);
	} else {
		/* catch */
		tsi_EmergencyShutDown( t->mem );
	}
}
#endif /* ENABLE_FF_CURVE_CONVERSION */

void T2K_SetNameString( T2K *t, uint16 languageID, uint16 nameID, int *errCode )
{
	sfntClass *font = t->font;
	
	if ( (*errCode = setjmp(t->mem->env)) == 0 ) {
			/* try */
			tsi_DeAllocMem( t->mem, t->nameString8); 	t->nameString8 = NULL;
			tsi_DeAllocMem( t->mem, t->nameString16 );	t->nameString16 = NULL;
#ifdef ENABLE_T1
			if ( font->T1 != NULL ) {
				t->nameString8		= GetT1NameProperty( font->T1, languageID, nameID );
				return; /*****/
			}
#endif
#ifdef ENABLE_CFF
			if ( font->T2 != NULL ) {
				t->nameString8		= GetT2NameProperty( font->T2, languageID, nameID );
				return; /*****/
			}
#endif
#ifdef ENABLE_PFR
			if ( font->PFR != NULL ) {
				t->nameString8 = GetPFRNameProperty( font->PFR, languageID, nameID );
				return; /*****/
			}
#endif
#ifdef ENABLE_SPD
			if ( font->SPD != NULL ) {
				t->nameString8 = GetSPDNameProperty( font->SPD, languageID, nameID );
				return; /*****/
			}
#endif
#ifdef ENABLE_PCL
			if ( font->PCLeo != NULL ) {
				t->nameString8 = GetPCLNameProperty( font->PCLeo, languageID, nameID );
				return; /*****/
			}
#endif
			GetTTNameProperty( font, languageID, nameID, &t->nameString8, &t->nameString16);
	} else {
			/* catch */
		tsi_EmergencyShutDown(t->mem);
	}
}

/*
 *
 */
void DeleteT2K( T2K *t, int *errCode )
{
	assert( errCode != NULL );
	if ( (*errCode = setjmp(t->mem->env)) == 0 ) {
		/* try */
		tsi_Assert( t->mem, t->stamp1 == (long) T2K_MAGIC1 && t->stamp2 == (long) T2K_MAGIC2, T2K_ERR_BAD_T2K_STAMP  );

		T2K_PurgeMemoryInternal( t, 2 );

		tsi_DeAllocMem( t->mem, t->font->globalHintsCache );
		t->font->globalHintsCache = NULL;
		
		tsi_DeAllocMem( t->mem, t->nameString8);
		tsi_DeAllocMem( t->mem, t->nameString16 );
		
		Delete_dropoutAdaptationClass( t->tAdapt );
		tsi_FreeFastMemBlocks( t->mem );
		tsi_DeAllocMem( t->mem, t );
	} else {
		/* catch */
		tsi_EmergencyShutDown( t->mem );
	}
}



uint16 T2K_GetGlyphIndex( T2K *t, uint16 charCode, int *errCode )
{
	if ( (*errCode = setjmp(t->mem->env)) == 0) {
		/* try */
		return GetSfntClassGlyphIndex( t->font, charCode ); /*****/
	} else {
		/* catch */
		tsi_EmergencyShutDown(t->mem);
		return 0;
	}
}

#ifdef ENABLE_LINE_LAYOUT

/* Returns the total pixel width fast, and computes the kern values */
uint32 T2K_MeasureTextInX(T2K *t, const uint16 *text, int16 *xKernValuesInFUnits, uint32 numChars )
{
	uint32 i, totalWidth, thisWidth;
	uint16 glyphIndex, prevGlyphIndex;
	uint16 charCode, prevCharCode;
	/* uint16 *awArr; */
#ifdef LAYOUT_CACHE_SIZE
	uint32 cachePos, cacheTag;
#endif
#ifdef ENABLE_KERNING
	int16 xKern, yKern;
#endif
	
	assert( t != NULL );
	assert( t->font != NULL );
	assert( xKernValuesInFUnits != NULL );
	totalWidth		= 0;
	/* assert( t->font->hmtx != NULL ); */
	/* awArr			= t->font->hmtx->aw; */
	assert( t->font->GetAWFuncPtr1 != NULL );
	/*glyphIndex 		= 0xffff;*/
	prevCharCode	= 32; /* space character */
	prevGlyphIndex	= 0xffff;
	for ( i = 0; i < numChars; i++ ) {
		charCode = text[i];
		
#ifdef LAYOUT_CACHE_SIZE
		cachePos   = prevCharCode;
		cachePos <<= 4;
		cachePos  ^= charCode;
		cachePos  %= LAYOUT_CACHE_SIZE;
		
		cacheTag   = prevCharCode;
		cacheTag <<= 16;
		cacheTag  |= charCode;
		if ( t->tag[cachePos] == cacheTag ) {
			thisWidth	= t->kernAndAdvanceWidth[ cachePos ];
#ifdef ENABLE_KERNING
			xKern		= t->kern[ cachePos ];
#endif
			glyphIndex	= 0xffff;
		} else {
#endif	/* LAYOUT_CACHE_SIZE */	
			glyphIndex	= GetSfntClassGlyphIndex( t->font, charCode );
			/* thisWidth	= awArr[ glyphIndex ]; */
			thisWidth	= t->font->GetAWFuncPtr1( t->font->GetAWParam1, glyphIndex );

#ifdef ENABLE_KERNING
			{
				if ( prevGlyphIndex == 0xffff ) {
					prevGlyphIndex = GetSfntClassGlyphIndex( t->font, prevCharCode );
				}
				GetSfntClassKernValue( t->font, prevGlyphIndex, glyphIndex, &xKern, &yKern );
				thisWidth += xKern;
			}
#endif /* ENABLE_KERNING */	
#ifdef LAYOUT_CACHE_SIZE
			/* cache the results */
			t->tag[cachePos]					= cacheTag;
			t->kernAndAdvanceWidth[ cachePos ]	= (short)thisWidth;
#ifdef ENABLE_KERNING
			t->kern[ cachePos ]					= xKern;
#endif
		}
#endif	/* LAYOUT_CACHE_SIZE */
#ifdef ENABLE_KERNING
		xKernValuesInFUnits[i]	= xKern; /* in fUnits */
#else
		xKernValuesInFUnits[i]	= 0; 
#endif
		totalWidth		+= thisWidth;
		prevGlyphIndex	 = glyphIndex;
		prevCharCode	 = charCode;
	}
	/* now scale to pixels */
	totalWidth = (uint32)util_FixMul( (F16Dot16)totalWidth, t->xMul );
	return totalWidth; /*****/
}


void T2K_GetIdealLineWidth( T2K *t, const T2KCharInfo cArr[], long lineWidth[], T2KLayout out[] )
{
	int i;
	const T2KCharInfo *cd;
	long totalIntWidthX, totalIntWidthY;
	F16Dot16 totSumX, totSumY;
	uint16 prevIndex = 0;

#ifndef ENABLE_KERNING
	UNUSED(t);
#endif

	totalIntWidthX = totalIntWidthY = 0;
	totSumX = totSumY = 0;
	for ( i = 0; (cd = &cArr[ i ])->charCode != 0 ; i++ ) {
		totSumX += cd->LinearAdvanceWidth16Dot16[ T2K_X_INDEX ];
		totSumY += cd->LinearAdvanceWidth16Dot16[ T2K_Y_INDEX ];
		/* Initialize to the non-linear metrics */
		out[i].BestAdvanceWidth16Dot16[ T2K_X_INDEX ] = cd->AdvanceWidth16Dot16[ T2K_X_INDEX ];
		out[i].BestAdvanceWidth16Dot16[ T2K_Y_INDEX ] = cd->AdvanceWidth16Dot16[ T2K_Y_INDEX ];
#ifdef ENABLE_KERNING
		if ( i != 0 ) {
			int16 xKern, yKern;
			F16Dot16 delta;
			GetSfntClassKernValue( t->font, prevIndex, cd->glyphIndex, &xKern, &yKern );
			if ( xKern != 0 ) {
				delta = xKern; delta <<= 16;
				delta = util_FixMul( delta, t->xMul );
				totSumX += delta;
				/* truncate */
				delta &= 0xffff0000;
				out[i].BestAdvanceWidth16Dot16[ T2K_X_INDEX ] += delta;
			}
			if ( yKern != 0 ) {
				delta = yKern; delta <<= 16;
				delta = util_FixMul( delta, t->yMul );
				totSumY += delta;
				/* truncate */
				delta &= 0xffff0000;
				out[i].BestAdvanceWidth16Dot16[ T2K_Y_INDEX ] += delta;
			}
		}
#endif /* ENABLE_KERNING */		
		totalIntWidthX += totSumX >> 16; totSumX &= 0x0000ffff;
		totalIntWidthY += totSumY >> 16; totSumY &= 0x0000ffff;
		prevIndex = cd->glyphIndex;
	}
	lineWidth[ T2K_X_INDEX ] = totalIntWidthX;
	lineWidth[ T2K_Y_INDEX ] = totalIntWidthY;
}


void T2K_LayoutString( const T2KCharInfo cArr[], const long LineWidthGoal[], T2KLayout out[] )
{
	int i, j, MY_INDEX;
	const T2KCharInfo *cd;
	long totalIntWidth;
	F16Dot16 fracSum, spaceAdvance = 0;
	long error, goal, delta, deltaI, spaceCount, strLen;

	
	
	/* Set to the dimension that we will tweak, the other we will just scale */
	if ( LineWidthGoal[ T2K_X_INDEX ] >= LineWidthGoal[ T2K_Y_INDEX ] ) {
		goal = LineWidthGoal[ T2K_X_INDEX ];
		MY_INDEX = T2K_X_INDEX;
	} else {
		goal = LineWidthGoal[ T2K_Y_INDEX ];
		MY_INDEX = T2K_Y_INDEX;
	}
	totalIntWidth = 0; fracSum = 0;
	for ( spaceCount = 0, strLen = 0, i = 0; (cd = &cArr[ i ])->charCode != 0; i++ ) {
		/*
		 already done
		out[i].BestAdvanceWidth16Dot16[ T2K_X_INDEX ] = cd->AdvanceWidth16Dot16[ T2K_X_INDEX ];
		out[i].BestAdvanceWidth16Dot16[ T2K_Y_INDEX ] = cd->AdvanceWidth16Dot16[ T2K_Y_INDEX ];
		*/
		fracSum += out[i].BestAdvanceWidth16Dot16[ MY_INDEX ];
		totalIntWidth += fracSum >> 16; fracSum &= 0x0000ffff;
		if ( cd->charCode == 32 ) {
			spaceCount++;
			spaceAdvance = cd->LinearAdvanceWidth16Dot16[ MY_INDEX ];
		}
		strLen++;
	}
	error = totalIntWidth - goal;
	if ( strLen == 0 ) return; /*****/
	
	
	if ( error > 0 ) {
		deltaI = -1;
		delta  = -ONE16Dot16;
	} else {
		deltaI = 1;
		delta  = ONE16Dot16;
	}
	if ( spaceCount > 0 ) {
		long tmp 		= spaceAdvance;
		long minSpace	= spaceAdvance/2 + 1;
		long maxSpace	= spaceAdvance*4;
		while ( error != 0 && tmp >= minSpace && tmp <= maxSpace) {
			for ( i = 0; i < strLen; i++ ) {
				cd = &cArr[ i ];
				if ( cd->charCode == 32 ) {
					tmp = out[i].BestAdvanceWidth16Dot16[ MY_INDEX ] + delta;
					if ( tmp < minSpace || tmp > maxSpace ) break; /*****/
					out[i].BestAdvanceWidth16Dot16[ MY_INDEX ] = tmp;
					error += deltaI;
					if ( error == 0 ) break; /*****/
				}
			}
		}
	}
	if ( error >= strLen || error <= -strLen ) {
		int mul = error/strLen;
		if ( mul < 0 ) mul = -mul;
		for ( i = 0; i < strLen; i++ ) {
			cd = &cArr[ i ];
			out[i].BestAdvanceWidth16Dot16[ MY_INDEX ] += delta*mul;
			error += deltaI*mul;
		}
	}
	/* assert( error < strLen && error > -strLen ); */

	if ( error != 0 ) {
		int absError = error > 0 ? error : -error;
		int inc = strLen / ( absError + 1 );
		
		
		for ( i = inc>>1; error != 0; i = i % strLen ) {
			cd = &cArr[ i ];
			if ( out[i].BestAdvanceWidth16Dot16[ MY_INDEX ] > 0 ) {
				out[i].BestAdvanceWidth16Dot16[ MY_INDEX ] += delta;
				error += deltaI;
				i += inc;
			} else {
				i++;
			}
		}
	}
	
	/* Scale the other dimension(s) */
	for ( j = 0; j < T2K_NUM_INDECES; j++ ) {
		if ( j == MY_INDEX ) continue; /*****/
		for ( i = 0; i < strLen; i++ ) {
			F16Dot16 scaleFactor;
			scaleFactor = cd->AdvanceWidth16Dot16[ j ] > 0 ? util_FixDiv( out[i].BestAdvanceWidth16Dot16[ MY_INDEX ], cd->AdvanceWidth16Dot16[ MY_INDEX ] ) : 0;
			out[i].BestAdvanceWidth16Dot16[ j ] = util_FixMul( out[i].BestAdvanceWidth16Dot16[ j ], scaleFactor );
		}
	}
}
#endif /* ENABLE_LINE_LAYOUT */


#ifdef ENABLE_KERNING
/*
 * Return value is a pointer to T2K_KernPair with *pairCountPtr entries.
 * The entries consist of all kern pairs between the the character with
 * the charCode character code combined with itself and all the members
 * of baseSet. (A character should only appear once in baseSet)
 * The caller *has* to deallocate the pointer, if != NULL, with
 * tsi_DeAllocMem( t->mem, pointer );
 */
T2K_KernPair *T2K_FindKernPairs( T2K *t, uint16 *baseSet, int baseLength, uint16 charCode, int *pairCountPtr )
{
	register T2K_KernPair *pairs;
	register sfntClass *font = t->font;
	register int i, pairCount = 0;
	int16 xKern, yKern;
	uint16 glyphIndexA, glyphIndexB;
	
#ifdef ENABLE_PFR
	if (t->font->PFR != NULL)
		glyphIndexA = charCode;
	else
#endif
		glyphIndexA	= GetSfntClassGlyphIndex( font, charCode );

	GetSfntClassKernValue( font, glyphIndexA, glyphIndexA, &xKern, &yKern ); /* Check charCode-charCode */
	if ( xKern != 0 || yKern != 0 ) {
		pairCount++;
	}
	/* Allocate memory for worst (largest) case */
	pairs = (T2K_KernPair *)tsi_AllocMem( t->mem, sizeof(T2K_KernPair) * (baseLength+baseLength+pairCount) );
	
	if ( pairCount != 0 ) {
		pairs[0].left  = charCode;
		pairs[0].right = charCode;
		pairs[0].xKern = xKern;
		pairs[0].yKern = yKern;
	}
	
	for ( i = 0; i < baseLength; i++ ) {

#ifdef ENABLE_PFR
		if (t->font->PFR != NULL)
			glyphIndexB = baseSet[i];
		else 
#endif
			glyphIndexB = GetSfntClassGlyphIndex( font, baseSet[i] );

		GetSfntClassKernValue( font, glyphIndexA, glyphIndexB, &xKern, &yKern );
		if ( xKern != 0 || yKern != 0 ) {
			pairs[pairCount].left  = charCode;
			pairs[pairCount].right = baseSet[i] ;
			pairs[pairCount].xKern = xKern;
			pairs[pairCount].yKern = yKern;
			pairCount++;
		}
		GetSfntClassKernValue( font, glyphIndexB, glyphIndexA, &xKern, &yKern );
		if ( xKern != 0 || yKern != 0 ) {
			pairs[pairCount].left  = baseSet[i];
			pairs[pairCount].right = charCode;
			pairs[pairCount].xKern = xKern;
			pairs[pairCount].yKern = yKern;
			pairCount++;
		}
	}
	if ( pairCount != 0 ) {
		pairs = (T2K_KernPair *)tsi_ReAllocMem( t->mem, pairs, sizeof(T2K_KernPair) * pairCount );
	} else {
		tsi_DeAllocMem( t->mem, pairs );
		pairs = NULL;
	}
	
	*pairCountPtr = pairCount;
	return pairs; /*****/
}
#endif /* ENABLE_KERNING */		
#ifdef DRIVE_8BIT_LCD
/* This is a  READ ONLY table, so it is OK to make it a static table */
static ff_ColorTableType ff_ColorTable = {122,
{
0x0, /* 0-0-0 clut[0] */
0x151515, /* 1-1-1 clut[1] */
0x2b2b2b, /* 2-2-2 clut[2] */
0x404040, /* 3-3-3 clut[3] */
0x555555, /* 4-4-4 clut[4] */
0x6a6a6a, /* 5-5-5 clut[5] */
0x808080, /* 6-6-6 clut[6] */
0x959595, /* 7-7-7 clut[7] */
0xaaaaaa, /* 8-8-8 clut[8] */
0xbfbfbf, /* 9-9-9 clut[9] */
0xd5d5d5, /* 10-10-10 clut[10] */
0xeaeaea, /* 11-11-11 clut[11] */
0xffffff, /* 12-12-12 clut[12] */
0x15, /* 0-0-1 clut[13] */
0x2b, /* 0-0-2 clut[14] */
0x40, /* 0-0-3 clut[15] */
0x1540, /* 0-1-3 clut[16] */
0x150000, /* 1-0-0 clut[17] */
0x151500, /* 1-1-0 clut[18] */
0x15152b, /* 1-1-2 clut[19] */
0x151540, /* 1-1-3 clut[20] */
0x152b55, /* 1-2-4 clut[21] */
0x152b6a, /* 1-2-5 clut[22] */
0x2b1500, /* 2-1-0 clut[23] */
0x2b2b00, /* 2-2-0 clut[24] */
0x2b2b15, /* 2-2-1 clut[25] */
0x2b2b40, /* 2-2-3 clut[26] */
0x2b2b55, /* 2-2-4 clut[27] */
0x2b4055, /* 2-3-4 clut[28] */
0x2b406a, /* 2-3-5 clut[29] */
0x2b4080, /* 2-3-6 clut[30] */
0x402b00, /* 3-2-0 clut[31] */
0x402b15, /* 3-2-1 clut[32] */
0x404000, /* 3-3-0 clut[33] */
0x404015, /* 3-3-1 clut[34] */
0x40402b, /* 3-3-2 clut[35] */
0x404055, /* 3-3-4 clut[36] */
0x40406a, /* 3-3-5 clut[37] */
0x404080, /* 3-3-6 clut[38] */
0x40556a, /* 3-4-5 clut[39] */
0x405580, /* 3-4-6 clut[40] */
0x405595, /* 3-4-7 clut[41] */
0x4055aa, /* 3-4-8 clut[42] */
0x554015, /* 4-3-1 clut[43] */
0x55402b, /* 4-3-2 clut[44] */
0x55552b, /* 4-4-2 clut[45] */
0x555540, /* 4-4-3 clut[46] */
0x55556a, /* 4-4-5 clut[47] */
0x555580, /* 4-4-6 clut[48] */
0x556a95, /* 4-5-7 clut[49] */
0x556aaa, /* 4-5-8 clut[50] */
0x6a4015, /* 5-3-1 clut[51] */
0x6a5515, /* 5-4-1 clut[52] */
0x6a552b, /* 5-4-2 clut[53] */
0x6a5540, /* 5-4-3 clut[54] */
0x6a6a40, /* 5-5-3 clut[55] */
0x6a6a55, /* 5-5-4 clut[56] */
0x6a6a80, /* 5-5-6 clut[57] */
0x6a6a95, /* 5-5-7 clut[58] */
0x6a806a, /* 5-6-5 clut[59] */
0x6a8080, /* 5-6-6 clut[60] */
0x6a8095, /* 5-6-7 clut[61] */
0x6a80aa, /* 5-6-8 clut[62] */
0x6a80bf, /* 5-6-9 clut[63] */
0x6a80d5, /* 5-6-10 clut[64] */
0x6a95aa, /* 5-7-8 clut[65] */
0x6a95bf, /* 5-7-9 clut[66] */
0x6a95d5, /* 5-7-10 clut[67] */
0x806a2b, /* 6-5-2 clut[68] */
0x806a40, /* 6-5-3 clut[69] */
0x806a55, /* 6-5-4 clut[70] */
0x808040, /* 6-6-3 clut[71] */
0x808055, /* 6-6-4 clut[72] */
0x80806a, /* 6-6-5 clut[73] */
0x808095, /* 6-6-7 clut[74] */
0x809580, /* 6-7-6 clut[75] */
0x809595, /* 6-7-7 clut[76] */
0x8095aa, /* 6-7-8 clut[77] */
0x80aad5, /* 6-8-10 clut[78] */
0x958040, /* 7-6-3 clut[79] */
0x958055, /* 7-6-4 clut[80] */
0x95806a, /* 7-6-5 clut[81] */
0x959555, /* 7-7-4 clut[82] */
0x95956a, /* 7-7-5 clut[83] */
0x959580, /* 7-7-6 clut[84] */
0x9595aa, /* 7-7-8 clut[85] */
0x9595bf, /* 7-7-9 clut[86] */
0x95aa95, /* 7-8-7 clut[87] */
0x95aaaa, /* 7-8-8 clut[88] */
0x95aabf, /* 7-8-9 clut[89] */
0xaa9540, /* 8-7-3 clut[90] */
0xaa9555, /* 8-7-4 clut[91] */
0xaa956a, /* 8-7-5 clut[92] */
0xaa9580, /* 8-7-6 clut[93] */
0xaaaa55, /* 8-8-4 clut[94] */
0xaaaa6a, /* 8-8-5 clut[95] */
0xaaaa80, /* 8-8-6 clut[96] */
0xaaaa95, /* 8-8-7 clut[97] */
0xaaaabf, /* 8-8-9 clut[98] */
0xaabf80, /* 8-9-6 clut[99] */
0xaabfaa, /* 8-9-8 clut[100] */
0xaabfbf, /* 8-9-9 clut[101] */
0xaabfd5, /* 8-9-10 clut[102] */
0xbfaa95, /* 9-8-7 clut[103] */
0xbfbf95, /* 9-9-7 clut[104] */
0xbfbfaa, /* 9-9-8 clut[105] */
0xbfbfd5, /* 9-9-10 clut[106] */
0xbfd595, /* 9-10-7 clut[107] */
0xbfd5bf, /* 9-10-9 clut[108] */
0xbfeaff, /* 9-11-12 clut[109] */
0xd5bfaa, /* 10-9-8 clut[110] */
0xd5d5aa, /* 10-10-8 clut[111] */
0xd5d5bf, /* 10-10-9 clut[112] */
0xd5d5ea, /* 10-10-11 clut[113] */
0xd5d5ff, /* 10-10-12 clut[114] */
0xd5eaff, /* 10-11-12 clut[115] */
0xeaeabf, /* 11-11-9 clut[116] */
0xeaead5, /* 11-11-10 clut[117] */
0xeaeaff, /* 11-11-12 clut[118] */
0xeaffff, /* 11-12-12 clut[119] */
0xffffd5, /* 12-12-10 clut[120] */
0xffffea, /* 12-12-11 clut[121] */
}};

ff_ColorTableType *FF_GetColorTable( void )
{
	return &ff_ColorTable;
}


/*
 * The ff_ColorTableType constructor
 * The foreground, and background color values need to be in the range 0-255
 */
ff_ColorTableType *FF_NewColorTable( tsiMemObject *mem, uint16 Rb, uint16 Gb, uint16 Bb, uint16 Rf, uint16 Gf, uint16 Bf )
{
	register ff_ColorTableType *t;
	register uint32 ARGB;
	register uint16 Ra, Ga, Ba;
	register int i, N;
	ff_ColorTableType *origTable = FF_GetColorTable();
	
	assert( Rb >=0 && Rb <= 255 && Gb >=0 && Gb <= 255 && Bb >=0 && Bb <= 255 );
	assert( Rf >=0 && Rf <= 255 && Gf >=0 && Gf <= 255 && Bf >=0 && Bf <= 255 );
	
	t = (ff_ColorTableType *)tsi_AllocMem( mem, sizeof(ff_ColorTableType) );
	t->N = origTable->N;
	N = (int)t->N;
	
	if ( Rf == 0 && Gf == 0 && Bf == 0 && Rb == 0xff && Gb == 0xff && Bb == 0xff ) {
		/* Special case black on white for speed reasons. */
		for ( i = 0; i < N; i++ ) {
			ARGB = origTable->ARGB[i];
			Ba = (uint16)(ARGB & 0xff); ARGB >>= 8;
			Ga = (uint16)(ARGB & 0xff); ARGB >>= 8;
			Ra = (uint16)(ARGB & 0xff);
				
			ARGB  = (uint32)(255 - Ra); ARGB <<= 8;
			ARGB |= (uint32)(255 - Ga); ARGB <<= 8;
			ARGB |= (uint32)(255 - Ba);
			
			t->ARGB[i] = ARGB;
		}
	} else {
		/* The general case */
		for ( i = 0; i < N; i++ ) {
			ARGB = origTable->ARGB[i];
			Ba = (uint16)(ARGB & 0xff); ARGB >>= 8;
			Ga = (uint16)(ARGB & 0xff); ARGB >>= 8;
			Ra = (uint16)(ARGB & 0xff);
			
			/* Map 0-255 to 0-256 */
			Ba = (uint16)(Ba + ((Ba & 80)>>7));
			Ga = (uint16)(Ga + ((Ga & 80)>>7));
			Ra = (uint16)(Ra + ((Ra & 80)>>7));
			
			
			ARGB  = (((uint32)(256-Ra) * (Rb) 	+ Ra * Rf ) >> 8 );	/* Blend foreground and background colors */
			ARGB <<= 8;
			ARGB |= (((uint32)(256-Ga) * (Gb) 	+ Ga * Gf ) >> 8 );
			ARGB <<= 8;
			ARGB |= (((uint32)(256-Ba) * (Bb) 	+ Ba * Bf ) >> 8 );
		
			t->ARGB[i] = ARGB;
		}
	}
	

	return t; /*****/
}

/*
 * The ff_ColorTableType constructor
 */
void FF_DeleteColorTable( tsiMemObject *mem, ff_ColorTableType *t )
{
	tsi_DeAllocMem( mem, t );
}

#endif /* DRIVE_8BIT_LCD */

/*
 * Enumerates the characters in the font resource
 */
void T2K_ListChars(void * userArg, T2K *scaler, int ListCharsFn(void * userArg, void *scaler, uint16 code), int *errCode)
{
	T2K_SfntListChars(userArg, scaler->font, (void *)scaler, ListCharsFn, errCode);
}

/*
 * Converts PSName to CharCode (Type1 and Type2/CFF fonts only!!)
 */
int FF_PSNameToCharCode(T2K *t, char *PSName, int *errCode)
{
int ret;
	if ( (*errCode = setjmp(t->mem->env)) == 0 ) {
		ret = (int) SfntClassPSNameTocharCode( t->font, PSName);
	} else {
		/* catch */
		tsi_EmergencyShutDown( t->mem );
	}
	return ret;
}


#ifdef ENABLE_SBIT
#ifdef REVERSE_SC_Y_ORDER
static void T2K_InvertBitmap(T2K *t)
{
	uint8 *rowPtr1, *rowPtr2, *scanLine;
	size_t N;
	if (t->height > 1)
	{
		N = t->rowBytes;
		scanLine = (uint8 *)tsi_AllocMem( t->mem, N );
		if (scanLine)
		{
			rowPtr1 = t->baseAddr;
			rowPtr2 = t->baseAddr + (t->height - 1) * N;
			do
			{/* swap top and bottom rows, moving top down, bottom up,
					until meet/cross in middle */
				memcpy ((void *)scanLine, (const void *)rowPtr1, N);
				memcpy ((void *)rowPtr1, (const void *)rowPtr2, N);
				memcpy ((void *)rowPtr2, (const void *)scanLine, N);
				rowPtr1 += N;
				rowPtr2 -= N;
			} while (rowPtr2 > rowPtr1);
			tsi_DeAllocMem( t->mem, scanLine );
		}
	}
}
#endif /* REVERSE_SC_Y_ORDER */
#endif /* ENABLE_SBIT */


/*
 * Returns a pointer to memory buffer containing any arbitrary TrueType table
 */
uint8 *FF_GetTTTablePointer(T2K *t,					/* T2K Class pointer */
							 long tag,					/* table tag, e.g. 'loca' */
							 unsigned char **ppTbl,		/* address of buffer we'll allocate */
							 size_t *bufSize,			/* size of that buffer */
							 int *errCode )				/* pointer to error code integer */
{
InputStream *in = NULL;
unsigned char *pInBuf, *pOutBuf = NULL;
	/* first, return NULL for all types except TrueType: */
	*ppTbl = pOutBuf;
	*bufSize = 0;
#ifdef ENABLE_T1
	if ( t->font->T1 != NULL ) {
		return pOutBuf;
	} else 
#endif
#ifdef ENABLE_CFF
	if ( t->font->T2 != NULL ) {
		return pOutBuf;
	} else 
#endif
#ifdef ENABLE_PFR
	if ( t->font->PFR != NULL ) {
		return pOutBuf;
	} else 
#endif
#ifdef ENABLE_SPD
	if ( t->font->SPD != NULL ) {
		return pOutBuf;
	} else 
#endif
#ifdef ENABLE_PCL
	if ( t->font->PCLeo != NULL ) {
		return pOutBuf;
	} else 
#endif
	if ( (*errCode = setjmp(t->mem->env)) == 0 ) {
		/* try */
		in = GetStreamForTable( t->font, tag );
        pOutBuf = (uint8 *)CLIENT_MALLOC( in->maxPos );
        tsi_Assert( t->mem, pOutBuf != NULL, T2K_ERR_MEM_MALLOC_FAILED );
		*ppTbl = pOutBuf;
		*bufSize = (size_t)in->maxPos;
		pInBuf = GetEntireStreamIntoMemory( in  );
		memcpy(pOutBuf, pInBuf, in->maxPos );
		Delete_InputStream( in, errCode  );
	} else {
		/* catch */
		tsi_EmergencyShutDown( t->mem );
	}

	return pOutBuf;
}


/*
 * Forces the unloading of the current TrueType cmap and loads the cmap
 * currently selected by Set_PlatformID( scaler, ID ), and Set_PlatformSpecificID( scaler, ID ).
 * This function presumes you have already used those setter macros. The values
 * set by those macros will be ignored if a character has previously forced the load of a
 * TrueType cmap, unless you call this function!
 */
void FF_ForceCMAPChange(T2K *t,			/* T2K Class pointer */
						 int *errCode)		/* pointer to error code integer */
{
	/* first, return doing nothing for all types except TrueType: */
#ifdef ENABLE_T1
	if ( t->font->T1 != NULL ) {
		return;
	} else 
#endif
#ifdef ENABLE_CFF
	if ( t->font->T2 != NULL ) {
		return;
	} else 
#endif
#ifdef ENABLE_PFR
	if ( t->font->PFR != NULL ) {
		return;
	} else 
#endif
#ifdef ENABLE_SPD
	if ( t->font->SPD != NULL ) {
		return;
	} else 
#endif
#ifdef ENABLE_PCL
	if ( t->font->PCLeo != NULL ) {
		return;
	} else 
#endif
	if ( (*errCode = setjmp(t->mem->env)) == 0 ) {
		/* try */
		Purge_cmapMemory( t->font );
		ff_LoadCMAP( t->font );

	} else {
		/* catch */
		tsi_EmergencyShutDown( t->mem );
	}

}

/*
 * returns true if the glyph exists, false otherwise
 */
int FF_GlyphExists( T2K *t, long code, uint16 cmd, int *errCode )
{
	int ret;
	uint16 glyphIndex;
	if ( cmd & T2K_CODE_IS_GINDEX )
	{
		if (code < t->font->numGlyphs)
			ret = true;
	}
	else if ( (*errCode = setjmp(t->mem->env)) == 0 )
	{
		/* try */
		glyphIndex = GetSfntClassGlyphIndex( t->font, (uint16)code );
		if (glyphIndex == 0)
		{/* closer scrutiny is needed for PFRs and SPDs: */
			ret = false;
#ifdef ENABLE_T1
			if ( t->font->T1 != NULL ) {
				ret = t->font->T1->glyphExists;
			}
#endif
#ifdef ENABLE_CFF
			if ( t->font->T2 != NULL ) {
				ret = t->font->T2->glyphExists;
			}
#endif
#ifdef ENABLE_PFR
		    if ( t->font->PFR != NULL ) {
				ret = t->font->PFR->glyphExists;
			}
#endif
#ifdef ENABLE_SPD
			if ( t->font->SPD != NULL ) {
				ret = t->font->PFR->glyphExists;
			}
#endif

		}
		else
			ret = true;
	}
	else
	{/* catch */
		tsi_EmergencyShutDown( t->mem );
	}

	return ret;
}

#endif /* T2K_SCALER */



/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: R:/src/FontFusion/Source/Core/rcs/t2k.c 1.71 2001/05/18 20:27:38 reggers Exp reggers $
 *                                                                           *
 *     $Log: t2k.c $
 *     Revision 1.71  2001/05/18 20:27:38  reggers
 *     Added error handling for CMD_AUTOGRID_YHEIGHTS_DISABLED
 *     return from ag_AutoGridOutline() call.
 *     Revision 1.70  2001/05/07 18:22:22  reggers
 *     Fixed warnings when T1 and CFF not enabled.
 *     Revision 1.69  2001/05/04 21:44:20  reggers
 *     Warning cleanup.
 *     Revision 1.68  2001/05/03 20:46:39  reggers
 *     LoadCMAP mapped to ff_LoadCMAP
 *     Revision 1.67  2001/05/03 20:16:08  reggers
 *     Fixed a configuration problem in ConcatFontMatrix.
 *     Revision 1.66  2001/05/03 16:04:56  reggers
 *     Error handling added to GlyphExists
 *     Revision 1.65  2001/05/02 21:25:11  reggers
 *     Map new functions to FF_ prefix rather than T2K_ prefix.
 *     Revision 1.64  2001/05/02 20:31:13  reggers
 *     Only flip the T2K_GRID_FIT bit on when ENABLE_AUTO_GRIDDING_CORE
 *     is set.
 *     Revision 1.63  2001/05/02 17:20:27  reggers
 *     SEAT BELT mode added (Sampo)
 *     Revision 1.62  2001/05/01 18:30:10  reggers
 *     Added T2K_GlyphExists()
 *     Revision 1.61  2001/04/27 20:33:23  reggers
 *     Added new API function T2K_ForceCMAPChange()
 *     Revision 1.60  2001/04/27 19:35:40  reggers
 *     Added new API function GetTTTablePointer for loading any TrueType
 *     Table into RAM memory.
 *     Revision 1.59  2001/04/26 20:45:17  reggers
 *     Cleanup relative to PSName conversion. Added errCode parameter,
 *     Revision 1.58  2001/04/26 19:28:19  reggers
 *     Made PFR kerning index based rather than charCode based.
 *     Revision 1.57  2001/04/24 21:57:06  reggers
 *     Added GASP table support (Sampo).
 *     Revision 1.56  2001/04/19 17:37:22  reggers
 *     First cut: convert PSName to ccode T1 and CFF.
 *     Sampo mod to support improved stroke font hinting.
 *     Revision 1.55  2000/11/02 17:21:37  reggers
 *     Treat maxStackElements as unsigned.
 *     Revision 1.54  2000/10/26 17:03:51  reggers
 *     Changes for SBIT: use cache based on setting of
 *     scaler->okForBitCreationToTalkToCache. Able to handle caching
 *     SBITs now.
 *     Added new matrix concatenation for T1 and T2 fonts. Thanks LCroft!
 *     Revision 1.53  2000/09/13 16:59:36  reggers
 *     Added FF_NewColorTable() and FF_DeleteColorTable() (Sampo)
 *     Revision 1.52  2000/08/09 19:25:04  reggers
 *     Took curly brackets away from # items member of the color table.
 *     Revision 1.51  2000/07/13 14:34:42  reggers
 *     fix for 2 byte integers
 *     Revision 1.50  2000/07/11 17:33:34  reggers
 *     Configuration fix for ENABLE_STRKCONV in RenderGlyph
 *     Revision 1.49  2000/06/16 15:28:13  reggers
 *     Included dropoutControl stubs.
 *     Revision 1.48  2000/06/15 15:15:57  mdewsnap
 *     Added Compile time option protection for section that invokes 
 *     native hint code in T1 and PFRs
 *     Revision 1.47  2000/06/14 21:20:46  reggers
 *     Included dropoutControl stubs for Type1 and PFR.
 *     Revision 1.46  2000/06/13 14:08:28  reggers
 *     Bug fix caused by bad casts for warning silencing related to dropoutControl.
 *     Revision 1.45  2000/06/07 15:22:22  mdewsnap
 *     Removed temp test for hints.
 *     Revision 1.44  2000/05/18 16:11:15  reggers
 *     Silence STRICT Borland warnings.
 *     Revision 1.43  2000/05/12 14:22:36  reggers
 *     Bug fix in micropositioning logic
 *     Revision 1.42  2000/05/11 15:12:38  reggers
 *     Implemented SBIT scaline order reversal both PFR and TTF.
 *     Revision 1.41  2000/05/11 13:36:57  reggers
 *     Added support for outline curve conversion & a few bug fixes
 *     related to native T1/PFR/TT hints. (Sampo)
 *     Revision 1.40  2000/05/08 18:01:51  reggers
 *     Changes for setjmp error code handling.
 *     maxPointCount made dynamic.
 *     Revision 1.39  2000/05/08 16:14:16  mdewsnap
 *     Setup compile options for hinting both T1 and PFRs
 *     Revision 1.38  2000/04/27 21:35:34  reggers
 *     New Stroke convert painting
 *     Revision 1.37  2000/04/19 20:30:27  reggers
 *     Test the ->T1 only when ENABLE_T1 is defined.
 *     Revision 1.36  2000/04/19 19:00:38  mdewsnap
 *     Added in code to deal with T1 hints
 *     Revision 1.35  2000/04/14 17:00:51  reggers
 *     First cut applying selective hints to stroke font glyphs.
 *     Revision 1.34  2000/04/13 18:14:21  reggers
 *     Updated list chars for user argument or context pass through.
 *     Revision 1.33  2000/04/06 16:27:56  reggers
 *     Changes for LCD mode.
 *     Revision 1.32  2000/03/27 22:17:07  reggers
 *     Updates for new LCD mode and functionality
 *     Revision 1.31  2000/03/27 20:53:15  reggers
 *     Changed a cast of (signed) to (long) pertaining to stamp1 and stamp2.
 *     Fixes bug on 2 byte integer environments.
 *     Revision 1.30  2000/03/10 19:18:23  reggers
 *     Enhanced for enumeration of character codes in font.
 *     Revision 1.29  2000/02/25 17:46:09  reggers
 *     STRICT warning cleanup.
 *     Revision 1.28  2000/02/18 18:56:14  reggers
 *     Added Speedo processor capability.
 *     Revision 1.27  2000/01/31 15:24:28  reggers
 *     Spelling of internal_baseAddr
 *     Revision 1.26  2000/01/31 15:01:32  reggers
 *     Fix bug in adaptive dropout control when we were not generating bitmaps.
 *     Revision 1.25  2000/01/31 14:51:12  reggers
 *     Make sure all sBit allocations marked as internal in RenderGlyph.
 *     Revision 1.24  2000/01/19 19:21:22  reggers
 *     Changed all references to PCLClass member PCL to PCLeo to
 *     avoid nasty namespace conflict on Windows builds.
 *     Revision 1.23  2000/01/07 19:45:50  reggers
 *     Sampo enhancements for FFS fonts.
 *     Revision 1.22  1999/12/23 22:03:04  reggers
 *     New ENABLE_PCL branches. Rename any 'code' and 'data' symbols.
 *     Revision 1.21  1999/12/09 21:17:34  reggers
 *     Sampo: multiple TrueType compatibility enhancements (scan convereter).
 *     Revision 1.20  1999/11/17 21:53:10  reggers
 *     Added UNUSED() macro for enableSbits in NewTrans
 *     Revision 1.19  1999/11/04 20:20:29  reggers
 *     Added code for getting fixed/proportional setting, firstCharCode and
 *     lastCharCode.
 *     Revision 1.18  1999/10/29 15:06:48  jfatal
 *     1- Wrap PFR stuff into a #ifdef ENABLE_PFR
 *     2- T2K_GetGlyphIndex() is moved out of #ifdef ENABLE_LINE_LAYOUT
 *     Revision 1.17  1999/10/21 20:41:05  jfatal
 *     1- Fix compile error if ENABLE_KERNING is turned off.
 *     2- Fix in T2K_MeasureTextInX() so that we get the correct
 *         Kerning values for PFRs.
 *     Revision 1.16  1999/10/19 16:23:35  shawn
 *     Initialized some variables to avoid compiler warnings.
 *     
 *     Revision 1.15  1999/10/18 20:28:33  shawn
 *     Modifications to correct dropout control problems.
 *     
 *     Revision 1.14  1999/10/18 16:57:17  jfatal
 *     Changed all include file names to lower case.
 *     Revision 1.13  1999/10/13 19:45:47  mdewsnap
 *     Added check in T2K_FindKernPairs to see if sfnt is a PFR.
 *     Revision 1.12  1999/09/30 15:10:42  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.11  1999/09/27 21:42:14  reggers
 *     Fix for T2K_GetGlyphIndexFast macro.
 *     Revision 1.9  1999/07/29 16:10:33  sampo
 *     First revision for T2KS
 *     Revision 1.7  1999/07/16 19:30:40  sampo
 *     Sbits for PFR.
 *     Revision 1.6  1999/07/16 17:52:01  sampo
 *     Sampo work. Drop #8 July 16, 1999
 *     Revision 1.4  1999/06/08 13:53:28  Krode
 *     Added call to GetPFRNameProperty(). Some newbie stuff, too.
 *     Revision 1.3  1999/05/17 15:57:54  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/

