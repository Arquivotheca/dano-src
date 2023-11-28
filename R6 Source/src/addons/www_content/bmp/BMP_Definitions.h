/*****************************************************************************
 *	Filename: BMP_Definitions.h
 *  Copyright 2001 by Be Incorporated.
 *  
 *  Author: Myron W. Walker  >> others... see change log at end of file...
 *
 *  Description:  This file contains the data structure definitions necessary
 *  to decode BMP images.
 *
 *  BMP File Format
 *  --------------------
 *  | BITMAPFILEHEADER |
 *  --------------------------------------------------------
 *  | BITMAPINFOHEADER , BITMAPV4HEADER, OR BITMAPV5HEADER |
 *  --------------------------------------------------------
 *  | RGBQUAD array |
 *  -----------------
 *  | PixelData |
 *  -------------
 *****************************************************************************/

#ifndef __BMP_DEFINITIONS_H__
#define __BMP_DEFINITIONS_H__

#include "FXFP_2_30.h"

typedef struct _RGB_Color { 
  unsigned char blue; 
  unsigned char green; 
  unsigned char red; 
  unsigned char reserved; 
} RGB_Color; 

// BITMAP FILE HEADER
typedef struct _BMP_FileHeader
{
	unsigned short bfType;
	unsigned long bfSize;
	unsigned short bfReserved1;
	unsigned short bfReserved2;
	unsigned long bfOffsetBits;
} BMP_FileHeader;

/* BITMAP INFO HEADER
 *
 * USED PRIOR TO WINDOWS 95 AND NT 4.0
 */
 typedef struct _BMP_BitmapInfo
{
	unsigned long biInfoHeaderSize;
	long biWidth;
	long biHeight;
	unsigned short biPlanes;
	unsigned short biBitCount;
	unsigned long biCompression;
	unsigned long biSizeImage;
	long biXPelsPerMeter;
	long biYPelsPerMeter;
	unsigned long biClrUsed;
	unsigned long biClrImportant;
} BMP_BitmapInfo; 


/* BITMAP INFO PV4 HEADER 
 *
 * USED IN WINDOWS 95 AND NT 4.0
 */
typedef struct _BMP_PV4BitmapInfo
{
	unsigned long biInfoHeaderSize;
	long biWidth;
	long biHeight;
	unsigned short biPlanes;
	unsigned short biBitCount;
	unsigned long biCompression;
	unsigned long biSizeImage;
	long biXPelsPerMeter;
	long biYPelsPerMeter;
	unsigned long biClrUsed;
	unsigned long biClrImportant;
	unsigned long biRedMask;
	unsigned long biGreenMask;
	unsigned long biBlueMask;
	unsigned long biAlphaMask;
	unsigned long biCSType;	
	CIE_XYZTriple biEndPoints;
	unsigned long biGammaRed;
	unsigned long biGammaGreen;
	unsigned long biGammaBlue;
} BMP_PV4BitmapInfo; 

/* BITMAP INFO PV4 HEADER 
 *
 * USED IN WINDOWS 98 AND WINDOWS 2000
 */
typedef struct _BMP_PV5BitmapInfo
{
	unsigned long biInfoHeaderSize;
	long biWidth;
	long biHeight;
	unsigned short biPlanes;
	unsigned short biBitCount;
	unsigned long biCompression;
	unsigned long biSizeImage;
	long biXPelsPerMeter;
	long biYPelsPerMeter;
	unsigned long biClrUsed;
	unsigned long biClrImportant;
	unsigned long biRedMask;
	unsigned long biGreenMask;
	unsigned long biBlueMask;
	unsigned long biAlphaMask;
	unsigned long biCSType;	
	CIE_XYZTriple biEndPoints;
	unsigned long biGammaRed;
	unsigned long biGammaGreen;
	unsigned long biGammaBlue;
	unsigned long biIntent;
	unsigned long biProfileData;
	unsigned long biProfileSize;
	unsigned long biReserved;
} BMP_PV5BitmapInfo;

class BBitmap;

enum BITMAP_VERSION
{
	BMP_WIN3X,
	BMP_PV4,
	BMP_PV5
};

#endif //__BMP_DEFINITIONS_H__

/*****************************************************************************
 *								CHANGE LOG
 *****************************************************************************
 *	 05 Jan 2001	Myron W. Walker
 *
 *****************************************************************************/