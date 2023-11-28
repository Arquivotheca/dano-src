/*//////////////////////////////////////////////////////////////////////*/
/*                                                                      */
/*              INTEL CORPORATION PROPRIETARY INFORMATION               */
/*                                                                      */
/*      This software is supplied under the terms of a license          */
/*      agreement or nondisclosure agreement with Intel Corporation     */
/*      and may not be copied or disclosed except in accordance         */
/*      with the terms of that agreement.                               */
/*                                                                      */
/*//////////////////////////////////////////////////////////////////////*/
/*                                                                      */
/* Copyright (C) 1994-1997 Intel Corp.  All Rights Reserved.            */
/*                                                                      */
/*//////////////////////////////////////////////////////////////////////*/

/* $Id: yvu2rgb.h,v 1.12 1994/08/11 00:52:41 chrisb Exp $ */

#if !defined __YVU2RGB_H__
#define __YVU2RGB_H__

void DPF( char *format, ... );

#define YVU_IS_8BIT		/* builds tables for 8-bit YVU pels @ color conv time */
#if defined YVU_IS_8BIT
#define YVU_MAX 256L
#define UV_TABLE_SIZE (1024L)
#define SHIFT8(x) ((x)>>1)
#else
#define SHIFT8(x) (x)
#define YVU_MAX 128L
#define UV_TABLE_SIZE (512L)
#endif /* YVU_IS_8BIT */

#include <QuickDraw.h>

typedef struct {
	PU8 pYdata;         /* Pointer to start of decompressed Y-plane data    */
	PU8 pUdata;         /* Pointer to start of decompressed U-plane data    */
	PU8 pVdata;         /* Pointer to start of decompressed V-plane data    */
	U16 xres;           /* # of pixels per line                             */
	U16 inputStride;	/* stride of input data	(Y-plane)					*/
	PU8 baseAddr;       /* Start of destination pixMap                      */
	int lines;          /* # of lines to output                             */
	short rowBytes;     /* Output pixMap pitch                              */
	void *OtherData;	/* Pointer to any other data the converter needs	*/
} colorConvertArgs;

typedef struct {
	PIA_ENUM		CurrentConverterTables;	/* NONE, RGB8, RGB16 or RGB24 */
	Boo				bTablesInitialized;		/* true tables are initialized */
	PU8				pPaletteTables;			/* Active Palette dynamic CLUT */
	U16				u16ActivePaletteEntries;/* number of entries in AP */
	PTR_BGR_ENTRY	pActivePalette;			/* current Active Palette */
	PU8				pRGB16Tables;			/* RGB16 conversion tables */
	PU8				pRGB32Tables;			/* RGB24 conversion tables */
} PRIVATE_DATA;
typedef PRIVATE_DATA * PTR_PRIVATE_DATA;
	
#define ACTIVE_PALETTE_SIZE (256*sizeof(BGR_ENTRY))
#define PALETTE_TABLE_SIZE (0x10000L + UV_TABLE_SIZE*2)
#define RGB16_TABLE_SIZE (sizeof(U16)*(32L*32L*64L+ UV_TABLE_SIZE*2))
#define RGB32_TABLE_SIZE (0x10000L * 4)

#define COCF_VALID_CONTROL_FLAGS (COUT_RESIZING | COUT_FLAGS_VALID)

#define COUT_TABLES_NONE	0
#define COUT_TABLES_RGB8	1
#define COUT_TABLES_RGB16	2
#define COUT_TABLES_RGB32	3

/* 256-GRAY COLOR CONVERSION
 */
extern int isGrayScalePixMap( CTabHandle p );
extern void ToGray8( colorConvertArgs *args );
extern void ToGray8x2( colorConvertArgs *args );

/* 8-BIT COLOR CONVERSION
 */
extern Handle AllocateDynamicCLUT( void );
extern void ComputeDynamicClut( PU8 table, PTR_BGR_ENTRY p, U16 ncol);
extern void ToRGB8( colorConvertArgs *args );
extern void ToRGB8WithSkips( colorConvertArgs *args );
extern void ToRGB8x2( colorConvertArgs *args );

/* 16-BIT COLOR CONVERSION
 */
extern void MakeRGB16CCTables( PU16 table );
extern void ToRGB16( colorConvertArgs *args );
extern void ToRGB16x2( colorConvertArgs *args );

/* 32-BIT COLOR CONVERSION
 */
void MakeRGB32Table( PU32 pTable );
#ifdef RGB32_PALETTE
extern void ToRGB32( colorConvertArgs *args );
extern void ToRGB32x2( colorConvertArgs *args );
#else
extern void ToRGB32( colorConvertArgs *args );
extern void ToRGB32x2( colorConvertArgs *args );
#endif

/* Miscellaneous color conversion
 */
extern void ToIF09( colorConvertArgs *args );
extern void ToPlanarYVU9( colorConvertArgs *args );
extern void StrideCopy(
	PU8	outPtr,			/* destination base address			*/
	U16	outStride,		/* destination stride				*/
	PU8	inPtr,			/* source base address				*/
	U16	inStride,		/* source stride					*/
	U16	bytesPerLine,	/* number of bytes to copy per line	*/
	U16	numLines		/* number of lines to copy			*/
);

#endif /* __YVU2RGB_H__ */
