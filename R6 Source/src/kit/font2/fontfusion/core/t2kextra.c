/*
 * T2KEXTRA.c
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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "t2k.h"
#include "t2kextra.h"




#ifdef T2K_SCALER 

#ifdef ENABLE_COLORBORDERS


/*
 * Creates a bordered colored antialiased character.
 *
 * This function can be invoked right after T2K_RenderGlyph().
 * You probably should use T2K_RenderGlyph with GrayScale and in T2K_TV_MODE for best results.
 *
 * t:       is a pointer to the T2K scaler object;
 * params:  is a void pointer that poins at T2K_BorderFilerParams
 *
 */
void T2K_CreateBorderedCharacter( T2K *t, void *params )
{
	T2K_BorderFilerParams *myParams = (T2K_BorderFilerParams *)params;
	uint8 greyScaleLevel = myParams->greyScaleLevel;
	int32 dX = myParams->dX;
	int32 dY = myParams->dY;
	uint32 R = myParams->R;
	uint32 G = myParams->G;
	uint32 B = myParams->B;
	uint32 borderR = myParams->borderR;
	uint32 borderG = myParams->borderG;
	uint32 borderB = myParams->borderB;
	register long i, N, j, yDist, xDist;
	register long x, y, widthA, widthB, heightA, heightB, rowBytesA, rowBytesB;
	register uint32 *baseARGB, *ptr32; 
	register uint8 *baseGrey = (uint8 *)t->baseAddr;
	register uint32 result, alpha;
	register int gray = greyScaleLevel != BLACK_AND_WHITE_BITMAP;
	
	assert( R < 256 );
	assert( G < 256 );
	assert( B < 256 );
	assert( borderR < 256 );
	assert( borderG < 256 );
	assert( borderB < 256 );
	assert( dX > 0 && dX < 3 );
	assert( dY > 0 && dY < 3 );
	
	heightA			= t->height;
	widthA			= t->width;
	rowBytesA		= t->rowBytes;
	
	yDist 			= dY + dY;
	xDist 			= dX + dX;
	heightB			= heightA 	+ yDist;
	widthB			= widthA 	+ xDist;
	rowBytesB		= widthB;
	
	t->xAdvanceWidth16Dot16 		+= (xDist<<16);
	t->xLinearAdvanceWidth16Dot16	+= (xDist<<16);
	t->fTop26Dot6 					+= dY << 6; /* added 3/9/99 */
	t->vert_fTop26Dot6 				+= dY << 6;
	t->rowBytes			= rowBytesB;
	t->height			= heightB;
	t->width			= widthB;
	
	N 					= rowBytesB * heightB;
	
	/* Allocate memory from the right place. */
	baseARGB = NULL;
	t->internal_baseARGB = false;
	if ( t->GetCacheMemory != NULL ) {
		baseARGB  = (uint32 *) t->GetCacheMemory( t->theCache,  N * sizeof( uint32 ) ); /* can return NULL */
	}
	if ( baseARGB == NULL ) {
		baseARGB = (uint32 *) tsi_AllocMem( t->mem, N * sizeof( uint32 ) ); assert( baseARGB != NULL );
		t->internal_baseARGB = true;
	}
	t->baseARGB		= baseARGB;
		
	assert( T2K_BLACK_VALUE == 126 );
	
	/* Initialize the Bitmap. */
	for ( i = 0; i < N; i++ ) {
		baseARGB[i] = 0;
	}
	/* Create a smeared background using the border color */
	ptr32 = &baseARGB[(dY+dY) * widthB ]; /* &baseARGB[(y-z1+dY) * widthB + z2 + x + dX ] ); start at z1 = -dY; z2 = -dX; */
	for ( y =  0; y < heightA; y++ ) {
		for ( x = 0; x < widthA; x++ ) {
			if ( gray ) {
				alpha = baseGrey[x];
			} else {
				alpha = (uint32)(baseGrey[x>>3] & (0x80 >> (x&7)) ? T2K_BLACK_VALUE : 0);
			}
			if ( alpha != 0  ) {
				alpha	= alpha + alpha + (alpha>>5); /* map 126 to 255 */
				/* assert(  ptr32 ==  &baseARGB[(y-z1+dY) * widthB + z2 + x + dX ] ); */
				for ( j = yDist; j > 0; j-- ) { /* UP */
					if ( alpha > *ptr32 ) *ptr32 = alpha;
					ptr32 -= widthB;
				}
				for ( j = xDist; j > 0; j-- ) { /* RIGHT */
					if ( alpha > *ptr32 ) *ptr32 = alpha;
					ptr32++;
				}
				for ( j = yDist; j > 0; j-- ) {	/* DOWN */
					if ( alpha > *ptr32 ) *ptr32 = alpha;
					ptr32 += widthB;
				}
				for ( j = xDist; j > 0; j-- ) { /* LEFT */
					if ( alpha > *ptr32 ) *ptr32 = alpha;
					ptr32--;
				}
			}
			ptr32++;
		}
		baseGrey += rowBytesA;
		ptr32 += widthB - widthA;
	}
	{
		register uint32 RGB;
		RGB  = borderR; RGB <<= 8;
		RGB |= borderG; RGB <<= 8;
		RGB |= borderB;
		
		for ( i = 0; i < N; i++ ) {
			result = baseARGB[i];
			result <<= 24;
			result |= RGB;
			baseARGB[i] = result;
		}
	}
	/* Paint in the actual character in the middle on top of the smeared background */
	baseGrey = (uint8 *)t->baseAddr;
	ptr32   = &baseARGB[(0 + dY) * widthB + 0 + dX ];
	for ( y =  0; y < heightA; y++ ) {
		for ( x = 0; x < widthA; x++ ) {
			if ( gray ) {
				alpha = baseGrey[x];
			} else {
				alpha = (uint32)(baseGrey[x>>3] & (0x80 >> (x&7)) ? T2K_BLACK_VALUE : 0);
			}
			if ( alpha != 0 ) {
				uint32 old_alpha, oldR,oldG,oldB;
				
				alpha	= alpha + alpha + (alpha>>5); /* map 126 to 255 */
				/* ptr32   = &baseARGB[(y + dY) * widthB + x + dX ]; */
				result	= *ptr32;
				old_alpha	= (uint32)( result >> 24); 
				oldR		= borderR; /* ==  (long)((result >> 16) & 0xff); */
				oldG		= borderG; /* ==  (long)((result >>  8) & 0xff) */
				oldB		= borderB; /* ==  (long)(result & 0xff);*/
				alpha++; 	/* [1,255] -> [2,256] */
				old_alpha 	= old_alpha + (old_alpha>>7); /* [0,255] -> [0,256] */
				
				oldR 		= oldR + (alpha*( R - oldR ) >> 8); /* (((long)(256-alpha) * oldR + alpha * R	)>>8) & 0xff; */
				oldG 		= oldG + (alpha*( G - oldG ) >> 8);
				oldB 		= oldB + (alpha*( B - oldB ) >> 8);
				alpha 		= old_alpha + (alpha*(256-old_alpha) >> 8) - 1; /* [1,256] -> [0,255] */
				/* alpha		= (alpha + old_alpha); if ( alpha > 255 ) alpha = 255; */
				
				result  = (uint32)alpha;	result <<= 8; 
				result |= oldR; 			result <<= 8;
				result |= oldG; 			result <<= 8;
				result |= oldB;
				
				*ptr32 = result;
			}
			ptr32++;
		}
		baseGrey += rowBytesA;
		ptr32 += widthB - widthA;
	}
	if ( t->baseAddr != NULL && t->internal_baseAddr ) {
		tsi_FastDeAllocN( t->mem, t->baseAddr, T2K_FB_SC_BITMAP ); t->baseAddr = NULL;
	}
}
#endif /* ENABLE_COLORBORDERS */



#ifdef ENABLE_LCD_OPTION


#ifdef DRIVE_8BIT_LCD
static uint8 distributionTableG[] =
{
	0,0,0,0,0,
	0,1,1,1,0,
	0,2,2,2,0,
	1,2,3,2,1,
	1,3,4,3,1,
	1,4,5,4,1,
	2,4,6,4,2,
	2,5,7,5,2,
	2,6,8,6,2,
	3,6,9,6,3,
	3,7,10,7,3
};


static uint8 distributionTable3B[] =
{
	0,0,0,0,0,
	1,1,1,0,0,
	2,2,2,0,0,
	3,3,3,0,0,
	3,3,4,1,1,
	4,4,5,1,1,
	5,5,6,1,1,
	6,6,7,1,1,
	7,7,8,1,1,
	8,8,9,1,1,
	9,9,10,1,1
};
#endif /* DRIVE_8BIT_LCD */

/* #define SAMPLE_PIXEL_USAGE */

#ifdef SAMPLE_PIXEL_USAGE
extern int pixelUsageCleared;
extern int pixelUsage[];
#endif /* SAMPLE_PIXEL_USAGE */

#define BUFFER_ROW_BYTES_SIZE 128

#ifdef DRIVE_8BIT_LCD
/*
 * This trivial routine just drives the implicit gray-pixels
 * on RGB like displays, where the pixels alternate between
 * the different RGB colors. 
 *
 * This implements the Super LCD Option for FontFusion
 *
 * This is for historical reasons directly called by T2K when the bit-field T2K_LCD_MODE is set.
 * This is why it is intentionally not in T2K_WriteToGrayPixels( T2K *t, void *params) format.
 *
 * for QUANT_VALUE == 4 the routine is optimized to not use an multiplies or divides inside inner loops!!!
 *
 *
 */
static void T2K_WriteToGrayPixelsWith8BitCLUT( T2K *t )
{
	register long N;
	register long x, y, widthA, widthB, widthC, heightA, heightB, rowBytesA, rowBytesB;
	register uint8 *baseAddrB, *ptrA;
	register int alpha, state;
	register uint8 *ptrB, *ptrC;
	int internal_baseAddrB;
	int quant = QUANT_VALUE;
	int quant3 = 3*quant;
#ifdef DRIVE_8BIT_LCD
	ff_ColorTableType *colorTable;
#endif
	uint8 buffer[ BUFFER_ROW_BYTES_SIZE ];
	
	
	assert( T2K_BLACK_VALUE == 126 );
	heightA			= t->height;
	widthA			= t->width;
	rowBytesA		= t->rowBytes;
	
	heightB			= heightA;

	widthB			= widthA + 2 + 2;
	widthB 			= (widthB+2)/3;	/* Convert to whole rgb-pixel */
	widthC 			= widthB+widthB+widthB; /* 3*widthB */
#ifndef DRIVE_8BIT_LCD
	widthB			= widthC;
#endif	
	
	rowBytesB		= widthB;
#ifdef MAKE_SC_ROWBYTES_A_4BYTE_MULTIPLE
	rowBytesB               = (rowBytesB+3) & (~3);
#endif	
        t->fLeft26Dot6                  -= 64 + 64;
        t->vert_fLeft26Dot6             -= 64 + 64;

#ifdef DRIVE_8BIT_LCD
		{
			int wError = widthB *3 - (widthA + 2 + 2);      /*  wError >= 0 && wError <= 2 */
			wError <<= 5;  /* div 2 and then mul 64 == mul 32 == << 5 */
			t->fLeft26Dot6							= (t->fLeft26Dot6 + 1 + wError ) / 3;
			t->vert_fLeft26Dot6						= (t->vert_fLeft26Dot6 + 1 + wError ) / 3;
			t->xAdvanceWidth16Dot16					= (t->xAdvanceWidth16Dot16 + 1)/3;
			t->xLinearAdvanceWidth16Dot16			= (t->xLinearAdvanceWidth16Dot16 + 1)/3;
			t->vert_xAdvanceWidth16Dot16			= (t->vert_xAdvanceWidth16Dot16 + 1)/3;
			t->vert_xLinearAdvanceWidth16Dot16		= (t->vert_xLinearAdvanceWidth16Dot16 + 1)/3;

			t->xAdvanceWidth16Dot16 = (t->xAdvanceWidth16Dot16 + 0x8000) >> 16;
			t->xAdvanceWidth16Dot16 <<= 16;
			
			/* This improves the spacing, and is needed because of the *1/3 reduction of the advance width */
			if ((t->fLeft26Dot6 >>6) >= 0 && (widthB + (t->fLeft26Dot6>>6)) >= (t->xAdvanceWidth16Dot16>>16)  ) {
			        t->fLeft26Dot6 -= 64;
			}
		}
#endif
        t->rowBytes                     = rowBytesB;
        t->height                       = heightB;
        t->width                        = widthB;


	
	N 					= rowBytesB * heightB;
	
	assert( LEVEL_COUNT >= (quant3+1) );
	
	/* Allocate memory from the right place. */
	baseAddrB = NULL;
	internal_baseAddrB = false;
	if ( t->GetCacheMemory != NULL  && t->BitmapFilter == NULL) {
		baseAddrB  = (uint8 *) t->GetCacheMemory( t->theCache,  N * sizeof( uint8 ) ); /* can return NULL */
	}
	if ( baseAddrB == NULL ) {
		baseAddrB = (uint8 *) tsi_AllocMem( t->mem, N * sizeof( uint8 ) ); assert( baseAddrB != NULL );
		internal_baseAddrB = true;
	}
	ptrC = widthC <= BUFFER_ROW_BYTES_SIZE ? buffer : (uint8 *) tsi_AllocMem( t->mem, widthC * sizeof( uint8 ) );
	assert( ptrC != NULL );
	
	assert( T2K_BLACK_VALUE == 126 );
#ifdef SAMPLE_PIXEL_USAGE
	if (!pixelUsageCleared) {
		int i;
		for ( i = LEVEL_COUNT * LEVEL_COUNT * LEVEL_COUNT -1; i >= 0; i-- ) {
			pixelUsage[i] = 0;
		}
		pixelUsageCleared  = true;
	}
#endif /* SAMPLE_PIXEL_USAGE */
	
	/* Initialize */
	ptrB = (uint8 *)baseAddrB;
	ptrA = t->baseAddr;
#ifdef DRIVE_8BIT_LCD
	colorTable = FF_GetColorTable();
#endif
	for ( y = 0; y < heightA; y++ ) {
		register int w1, tmp;
		alpha = 0;
		
		ptrC[0]  = 0; 
		ptrC[1]  = 0;
		ptrC[2]  = 0;
		for ( x = 1; x < widthA-2; x += 3) {
			long min;
			
			/* 0->b, 1->r, 2->g, 3->b */
			min = ptrA[x+0];
			tmp = ptrA[x+1]; if ( tmp < min ) min = tmp;
			tmp = ptrA[x+2]; if ( tmp < min ) min = tmp;
			
			/* truncate down in the w1 assignment */
#if (QUANT_VALUE != 4)
			if ( min > 0 && (w1=((min * quant3 )/126)) > 0 ) {	
#else
			if ( min > 10 ) {
				/* *12/126 == *6/63 */
				min += min+min;				/* min is now the 3*original_min */
				/* w1   = (min+min)/63;	*/	/* 6*min/63 == 12*min/126 */
				/* (6*min)/63 is about the same as 65*(6*min+12)/4096 ...,but 126 goes to 11.99 and not 12.. */
				min += min;
				w1 = ((min<<6)+min+12)>>12; /* exactly the same as min/63 for inputs [0,..12] */
#endif			
				ptrC[x+2]  = (uint8)w1; 
				ptrC[x+3]  = (uint8)w1;
				ptrC[x+4]  = (uint8)w1;
#if (QUANT_VALUE != 4)
				min = (w1*126/(quant3));
#else
				assert( w1 >= 0 && w1 <= 12 );
				/* min = map12To126[w1]; */
				min  = (w1+w1); /* 2w1 */
				min += (min+min+min+min); /* 2w1+8w1 = 10w1 */
				min += (w1>>1); /*  126/12 = 10.5 = 8+2+0.5 */
#endif
				ptrA[x+0] = (uint8)(ptrA[x+0] - (uint8)min);
				ptrA[x+1] = (uint8)(ptrA[x+1] - (uint8)min);
				ptrA[x+2] = (uint8)(ptrA[x+2] - (uint8)min);
			} else {
				ptrC[x+2]  = 0; 
				ptrC[x+3]  = 0;
				ptrC[x+4]  = 0;
			}
		}
		/* last index we did in ptrC[] was x+4-3, this means next entry is x+2 */
		for ( x +=2; x < widthC; x++ ) {
			ptrC[x]  = 0; 
		}
		
		/* The state variable maps to 0:red, 1:green, 2:blue */
		/* We start reading at (x+2) == (0+2) == blue */
		for ( x = 0, state = 2; x < widthA; x++, state++ ) {
			if ( state > 2 ) state = 0;
			if ( (alpha = (ptrA[x]+alpha) ) != 0 ) {
				register uint8 *distributionTable;
				
				distributionTable = distributionTable = distributionTableG; 		/* Ok for read and green */
				if ( state == 2 ) {
					distributionTable = distributionTable3B;	/* Blue */
				}
				assert( (x+2) % 3 == state );
				

				w1 = alpha;
				/* assert( t <= 126 ); */
				if ( w1 > 126 ) w1 = 126; else if ( w1 < 0 ) w1 = 0; 
				/* alpha = w1; */
#if (QUANT_VALUE == 4)
				/* approximate, assert( ((w1 + 17)>>5) == (w1 * quant + 126/2)/126 ); */
				w1  = (w1 + 17)>>5; /* 0->0.5, and 126 -> 4.5 */
#else
				w1 = (w1 * quant + 126/2)/126;
#endif
				assert( w1 >= 0 && w1 <= QUANT_VALUE );
				assert( w1 >= 0 && w1 <= QUANT_VALUE );
#if (QUANT_VALUE == 4)
				/*  0->0.5 and 126 -> 4.5 */
				alpha = alpha - (((w1 << 6) - w1) >> 1);	/* 126/4 = 63/2 Keep track of the quantization error */
#else
				alpha = alpha - w1 * 126/quant;		/* Keep track of the quantization error */
#endif

				tmp  = w1 + w1;
				w1  += tmp+tmp; /* w1 *= 5; */
				 
				ptrC[x+0] = (uint8)(ptrC[x+0] + distributionTable[w1+0]);
				ptrC[x+1] = (uint8)(ptrC[x+1] + distributionTable[w1+1]);
				ptrC[x+2] = (uint8)(ptrC[x+2] + distributionTable[w1+2]);
				ptrC[x+3] = (uint8)(ptrC[x+3] + distributionTable[w1+3]);
				ptrC[x+4] = (uint8)(ptrC[x+4] + distributionTable[w1+4]);
			}
		}
		for ( x = 0;  x < widthC; x += 3 ) {
			int r, g, b;
			register uint16 index;
			
			r = ptrC[x+0]; if ( r > quant3 ) r = quant3;
			g = ptrC[x+1]; if ( g > quant3 ) g = quant3;
			b = ptrC[x+2]; if ( b > quant3 ) b = quant3;
			index  = (uint16)r;
#if (QUANT_VALUE == 4)
			/* We could also do the slower no MUL, x *= 13 is the same as x += 12*x which is the same as x += (4x + 4x + 4x); */
			index = (uint16)(index * 13);
			index = (uint16)(index + g);
			index = (uint16)(index * 13);
#else
			index = (uint16)(index * (quant3+1));
			index = (uint16)(index + g);
			index = (uint16)(index * (quant3+1));
#endif
			index = (uint16)(index + b);
#ifdef SAMPLE_PIXEL_USAGE
			pixelUsage[index]++;
#endif
			
			/* We would ideally like to output ((int)[r,g,b]*126/(quant3)) */
#ifdef DRIVE_8BIT_LCD
			assert( index <= colorIndexTableSize);
			*ptrB++ = t->colorIndexTable[index];
#else		
			assert( (index) % (quant3+1) == b );
			assert( (index/(quant3+1) ) % (quant3+1) == g );
			assert( (index/((quant3+1)*(quant3+1)) ) % (quant3+1) == r );
			
			if ( 0 ) {
				register uint32 ARGB;

				/* ARGB = colorTable->ARGB[t->colorIndexTable[index]]; */
				
				ptrB[x+0] = (uint8)(((ARGB >> 16) & 0xFF) * 126/255);
				ptrB[x+1] = (uint8)(((ARGB >>  8) & 0xFF) * 126/255);
				ptrB[x+2] = (uint8)(((ARGB >>  0) & 0xFF) * 126/255);
			} else {
				ptrB[x+0] = (uint8)((int)r*126/(quant3));
				ptrB[x+1] = (uint8)((int)g*126/(quant3));
				ptrB[x+2] = (uint8)((int)b*126/(quant3));
			}
#endif
		}
		
		ptrA += rowBytesA;
#ifdef DRIVE_8BIT_LCD
		ptrB += rowBytesB - widthB;
#else
		ptrB += rowBytesB;
#endif


	}

	if ( ptrC != buffer ) tsi_DeAllocMem( t->mem, ptrC );
	if ( t->baseAddr != NULL && t->internal_baseAddr ) {
		tsi_FastDeAllocN( t->mem, t->baseAddr, T2K_FB_SC_BITMAP );
	}
	t->baseAddr = baseAddrB;
	t->internal_baseAddr = internal_baseAddrB;
}
#endif /* DRIVE_8BIT_LCD */


/*
 * This trivial routine just drives the implicit gray-pixels
 * on RGB like displays, where the pixels alternate between
 * the different RGB colors. The gray-pixel this routine is
 * driving is 5 pixels wide (shape 1,2,3,2,1) and 1 tall.
 * This implements the Gibson LCD Option for T2K
 *
 * This is for historical reasons directly called by T2K when the bit-field T2K_LCD_MODE is set.
 * This is why it is intentionally not in T2K_WriteToGrayPixels( T2K *t, void *params) format.
 *
 */
void T2K_WriteToGrayPixels( T2K *t )
{
	register long N;
	register long x, y, widthA, widthB, heightA, heightB, rowBytesA, rowBytesB;
	register uint8 *baseAddrB, *ptrA;
	register int alpha;
	register uint8 *ptrB;
	int internal_baseAddrB;

#ifdef DRIVE_8BIT_LCD
	T2K_WriteToGrayPixelsWith8BitCLUT( t );
	return; /******/
#endif
	
	heightA			= t->height;
	widthA			= t->width;
	rowBytesA		= t->rowBytes;
	
	heightB			= heightA;
	widthB			= widthA + 2 + 2;
	rowBytesB		= widthB;
	
	t->fLeft26Dot6 			-= 64 + 64;
	t->vert_fLeft26Dot6		-= 64 + 64;
	
	t->rowBytes			= rowBytesB;
	t->height			= heightB;
	t->width			= widthB;
	
	N 					= rowBytesB * heightB;
#ifdef OLD
	baseAddrB = (uint8 *)tsi_AllocMem( t->mem, N * sizeof( uint8 ) ); assert( baseAddrB != NULL );
#endif	
	
	/* Allocate memory from the right place. */
	baseAddrB = NULL;
	internal_baseAddrB = false;
	if ( t->GetCacheMemory != NULL  && t->BitmapFilter == NULL) {
		baseAddrB  = (uint8 *) t->GetCacheMemory( t->theCache,  N * sizeof( uint8 ) ); /* can return NULL */
	}
	if ( baseAddrB == NULL ) {
		baseAddrB = (uint8 *) tsi_AllocMem( t->mem, N * sizeof( uint8 ) ); assert( baseAddrB != NULL );
		internal_baseAddrB = true;
	}
	
	assert( T2K_BLACK_VALUE == 126 );
	
	/* Initialize */
	ptrB = (uint8 *)baseAddrB;
	for ( x = 0; x < N; x++ ) {
		ptrB[x] = 0;
	}
	ptrA = t->baseAddr;
	for ( y = 0; y < heightA; y++ ) {
		for ( x = 0; x < widthA; x++ ) {
			if ( (alpha = ptrA[x] ) != 0 ) {
				register int w2, w1;
				/* OLD: w2 = (uint8)(alpha+4)/9; */
				/* OLD: w1 = (uint8)(alpha +1)/3; */
				/* w2: /=9 maps [0,126] to [0, 14] */
				/* w2: below we also get 0->0, and 126->14 */
				w2 = (alpha >>3) - (alpha>>6); /* == 1/8-1/64 == 1/9.142857.. avoids the divide for more SPEED */
				/* w1: /=3 maps [0,126] to [0,42] */
				/* w1: below we also get 0->0, and 126->42  (63 - 16 - 4 - 1 = 42 ) */
				w1 = ((alpha+1)>>1) - ((alpha+4)>>3) - ((alpha+15)>>5) - (alpha>>6); /* avoids the divide for more SPEED */
				ptrB[x+0] = (uint8)(ptrB[x+0] + w2);
				ptrB[x+1] = (uint8)(ptrB[x+1] + w1 - w2);
				ptrB[x+2] = (uint8)(ptrB[x+2] + w1);
				ptrB[x+3] = (uint8)(ptrB[x+3] + w1 - w2);
				ptrB[x+4] = (uint8)(ptrB[x+4] + w2);
			}
		}
		ptrA += rowBytesA;
		ptrB += rowBytesB;
	}
	if ( t->baseAddr != NULL && t->internal_baseAddr ) {
		tsi_FastDeAllocN( t->mem, t->baseAddr, T2K_FB_SC_BITMAP );
	}
	t->baseAddr = baseAddrB;
	t->internal_baseAddr = internal_baseAddrB;
}
#endif /* ENABLE_LCD_OPTION */


#ifdef ENABLE_FLICKER_FILTER
/* #define FAST_FLICKER_FILTER */
#define FAST_FLICKER_FILTER
/* #define USE_WITH_OLD_T2K_SCALER */
/* #define USE_WITH_OLD_T2K_SCALER */
/*
 * A flicker filter example. This code could be further optimized.
 * This is intended to be an example that can be tweaked further on the
 * actual deployment hardware.
 *
 * It implements this filter:
 *   1 4 1
 *   2 4 2
 *   1 4 1
 *
 * OR this filter if FAST_FLICKER_FILTER is defined
 *
 *   0 4 0
 *   2 4 2
 *   0 4 0
 *
 * IF you can not tell the difference then use the fast version
 *
 * This should be tweaked for the deployment hardware. A piece of advice is
 * that you should not blur the image more than necessary.
 */
 void T2K_FlickerFilterExample( T2K *t, void *params )
 {
	 register long N;
	 register long x, y, widthA, widthB, heightA, heightB, rowBytesA, rowBytesB;
	 register uint8 *baseAddrB, *ptrA;
	 register int alpha, *baseAddrBig, *ptrBig;

	 UNUSED( params );
	 heightA	= t->height;
	 widthA		= t->width;
	 rowBytesA	= t->rowBytes;

	 heightB	= heightA + 1 + 1;
	 widthB		= widthA  + 1 + 1;
	 rowBytesB	= widthB;

	 t->fLeft26Dot6			-= 64;
	 t->vert_fLeft26Dot6	-= 64;

	 t->rowBytes	= rowBytesB;
	 t->height		= heightB;
	 t->width		= widthB;

	 N				= rowBytesB * heightB;

	 /* Allocate temporary memory */
	 baseAddrBig = (int *) tsi_FastAllocN( t->mem, N * sizeof ( int ), T2K_FB_FILTER);
	 assert( baseAddrBig != NULL );

	 assert( T2K_BLACK_VALUE == 126 );

	 /* Initialize */
	 ptrBig = baseAddrBig;
	 for ( x = 0; x < N; x++ ) {
		 ptrBig[x] = 0;
	 }
	 ptrA = t->baseAddr;

	 ptrBig += rowBytesB;
	 for ( y = 0; y < heightA; y++ ) {
		 for ( x = 0; x < widthA; x++ ) {
			 if ( (alpha = ptrA[x] ) != 0 ) {
				 register int value4x, *iptr;

				 value4x = alpha<<2;

				 iptr = ptrBig - rowBytesB;
#ifndef FAST_FLICKER_FILTER
				 iptr[x+0] += alpha;
#endif
				 iptr[x+1] += value4x;
#ifndef FAST_FLICKER_FILTER
				 iptr[x+2] += alpha;
#endif

				 iptr += rowBytesB;
				 iptr[x+0] += alpha+alpha;
				 iptr[x+1] += value4x;
				 iptr[x+2] += alpha+alpha;

				 iptr += rowBytesB;
#ifndef FAST_FLICKER_FILTER
				 iptr[x+0] += alpha;
#endif
				 iptr[x+1] += value4x;
#ifndef FAST_FLICKER_FILTER
				 iptr[x+2] += alpha;
#endif
			 }
		 }
		 ptrA	+= rowBytesA;
		 ptrBig	+= rowBytesB;
	 }
#ifdef USE_WITH_OLD_T2K_SCALER
	 if ( t->baseAddr != NULL ) {
		 tsi_FastDeAllocN( t->mem, t->baseAddr, T2K_FB_SC_BITMAP );
	 }
	 baseAddrB = (uint8 *) tsi_FastAllocN( t->mem, N * sizeof ( uint8 ), T2K_FB_SC_BITMAP);
	 assert( baseAddrB != NULL );
#else
	 if ( t->baseAddr != NULL && t->internal_baseAddr ) {
		 tsi_FastDeAllocN( t->mem, t->baseAddr, T2K_FB_SC_BITMAP );
	 }
	 t->internal_baseAddr = false;
	 /* Allocate memory from the right place */
	 baseAddrB = NULL;
	 if ( t->GetCacheMemory != NULL ) {
		 baseAddrB = (uint8 *)t->GetCacheMemory( t->theCache, N * sizeof ( uint8 ) ); /* can return NULL */
	 }
	 if ( baseAddrB == NULL ) {
		 baseAddrB = (uint8 *)tsi_FastAllocN( t->mem, N * sizeof( uint8 ), T2K_FB_SC_BITMAP );
		 assert( baseAddrB != NULL );
		 t->internal_baseAddr = true;
	 }
#endif
	 for ( x = 0; x < N; x++ ) {
#ifdef FAST_FLICKER_FILTER
		 baseAddrB[x] = (uint8)((baseAddrBig[x] + 8 ) >> 4);
#else
		 baseAddrB[x] = (uint8)((baseAddrBig[x] + 10 ) / 20);
#endif
	 }
	 tsi_FastDeAllocN( t->mem, baseAddrBig, T2K_FB_FILTER );
	 t->baseAddr = baseAddrB;
	 }
#endif	/* ENABLE_FLICKER_FILTER */

	 
#endif /*  T2K_SCALER */

/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: R:/src/FontFusion/Source/Core/rcs/t2kextra.c 1.10 2000/05/18 15:31:38 reggers release $
 *                                                                           *
 *     $Log: t2kextra.c $
 *     Revision 1.10  2000/05/18 15:31:38  reggers
 *     Silence STRICT borland warnings.
 *     Revision 1.9  2000/05/09 14:45:59  reggers
 *     Eliminated 3 warnings.
 *     Revision 1.8  2000/04/19 20:15:39  reggers
 *     Round pixel locations in T2K_WriteToGrayPixelsWith8BitCLUT().
 *     Revision 1.7  2000/04/06 16:27:35  reggers
 *     Changes/enhancements for LCD mode. Fixed 2 byte integer problems.
 *     Revision 1.6  2000/03/27 22:17:13  reggers
 *     Updates for new LCD mode and functionality
 *     Revision 1.5  1999/10/19 16:16:47  shawn
 *     Added a Flicker Filter example.
 *     
 *     Revision 1.4  1999/10/18 17:01:30  jfatal
 *     Changed all include file names to lower case.
 *     Revision 1.3  1999/09/30 15:12:07  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.2  1999/05/17 15:58:12  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/

