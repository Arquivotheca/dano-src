//=============================================================================
// Description:
//		Software DV codec render engine for RGB32.
//
//
//
//
// Copyright:
//		Copyright (c) 1998 Canopus Co.,Ltd. All Rights Reserved.
//		Developed in San Jose, CA, U.S.A.
// History:
//		09/10/98 Tom > Creaded from RGBT routine.
//
//=============================================================================
#include <windows.h>
#include "csedv.h"
#include "putimage.h"
#include "yuvtorgb.h"

// auto defined for powerpc
//#define	__POWERPC__

#define	SIZE_RGBQ			4
#define	SCALE( d, s )		( ( ( d ) + ( 1 << ( ( s ) - 1 ) ) ) >> ( s ) )
#define	CLIP( rgb )			( ( BYTE )( rgb < 0 ? 0 : rgb > 255 ? 255 : rgb ) )

#define	SIZE_RGB16			2


#ifdef __POWERPC__
#define SCALEUV(X) (X)
#define SCALEY(X) (X)
#else
#define SCALEUV(X) ((X)>>1)
#define SCALEY(X) (((X)>>1)+1024)
#endif

void 	PASCAL	  PutImage525_RGB15( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC )
{
	int		SkipRGB, SkipY, SkipUV;
	short		data, x, y, u, v, Ruv, Guv, Buv, i;

	SkipRGB = Stride - SizeX * SIZE_RGB16;
	SkipY = ( DCT_BUFFER_STRIDE - SizeX ) * 2;
	SizeX >>= 2;
	SkipUV = ( DCT_BUFFER_STRIDE - SizeX ) * 2;

	for( ; SizeY ; SizeY-- )
	{
		for( x = SizeX ; x ; x-- )
		{
			v = SCALEUV(pC[0]);
			u = SCALEUV(pC[8]);

			Ruv = SCALE( v * mmxR_FROM_V               , mmxCOEFF_PRECISION );
			Guv = SCALE( u * mmxG_FROM_U + v * mmxG_FROM_V, mmxCOEFF_PRECISION );
			Buv = SCALE( u * mmxB_FROM_U               , mmxCOEFF_PRECISION );

			for( i = 4 ; i ; i-- )
			{
				int32 v=0;
				y = SCALEY(pY[0]);
				data = SCALE( y + Ruv, DCT_SHIFT+3 );	// red is 5 bits
				v=CLIP(data)<<10;
				data = SCALE( y + Guv, DCT_SHIFT+3 );	// green is 6 bits
				v|=CLIP(data)<<5;
				data = SCALE( y + Buv, DCT_SHIFT+3 );	// blue is 5 bits
				v|=CLIP(data);
				*((uint16*)pImage)=v;
				pImage += SIZE_RGB16;
				pY++;
			}
			pC++;
		}
		( PBYTE )pImage += SkipRGB;
		( PBYTE )pY += SkipY;
		( PBYTE )pC += SkipUV;
	}
}

void 	PASCAL	  PutImage525_RGB16( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC )
{
	int		SkipRGB, SkipY, SkipUV;
	short		data, x, y, u, v, Ruv, Guv, Buv, i;

	SkipRGB = Stride - SizeX * SIZE_RGB16;
	SkipY = ( DCT_BUFFER_STRIDE - SizeX ) * 2;
	SizeX >>= 2;
	SkipUV = ( DCT_BUFFER_STRIDE - SizeX ) * 2;

	for( ; SizeY ; SizeY-- )
	{
		for( x = SizeX ; x ; x-- )
		{
			v=SCALEUV(pC[0]);
			u=SCALEUV(pC[8]);
			Ruv = SCALE( v * mmxR_FROM_V               , mmxCOEFF_PRECISION );
			Guv = SCALE( u * mmxG_FROM_U + v * mmxG_FROM_V, mmxCOEFF_PRECISION );
			Buv = SCALE( u * mmxB_FROM_U               , mmxCOEFF_PRECISION );

			for( i = 4 ; i ; i-- )
			{
				int32 v=0;
				y = SCALEY(pY[0]);
				data = SCALE( y + Ruv, DCT_SHIFT+3 );	// red is 5 bits
				v=CLIP(data)<<11;
				data = SCALE( y + Guv, DCT_SHIFT+2 );	// green is 6 bits
				v|=CLIP(data)<<5;
				data = SCALE( y + Buv, DCT_SHIFT+3 );	// blue is 5 bits
				v|=CLIP(data);
				*((uint16*)pImage)=v;
				pImage += SIZE_RGB16;
				pY++;
			}
			pC++;
		}
		( PBYTE )pImage += SkipRGB;
		( PBYTE )pY += SkipY;
		( PBYTE )pC += SkipUV;
	}
}


//------------------------------------------------------------------------------
// Call back function for writing decompressed RGB image data.
//------------------------------------------------------------------------------
void 	PASCAL	  PutImage525_RGBQ( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC )
{
	int		SkipRGB, SkipY, SkipUV;
	short		data, x, y, u, v, Ruv, Guv, Buv, i;
	SkipRGB = Stride - SizeX * SIZE_RGBQ;
	SkipY = ( DCT_BUFFER_STRIDE - SizeX ) * 2;
	SizeX >>= 2;
	SkipUV = ( DCT_BUFFER_STRIDE - SizeX ) * 2;

	for( ; SizeY ; SizeY-- )
	{
		for( x = SizeX ; x ; x-- )
		{
			v=SCALEUV(pC[0]);
			u=SCALEUV(pC[8]);
			Ruv = SCALE(v*mmxR_FROM_V              ,mmxCOEFF_PRECISION);
			Guv = SCALE(u*mmxG_FROM_U+v*mmxG_FROM_V,mmxCOEFF_PRECISION);
			Buv = SCALE(u*mmxB_FROM_U              ,mmxCOEFF_PRECISION);

			for( i = 4 ; i ; i-- )
			{
				y = SCALEY(pY[0]);
				data = SCALE( y + Ruv, DCT_SHIFT );
				*( pImage + 2 ) = CLIP( data );
				data = SCALE( y + Guv, DCT_SHIFT );
				*( pImage + 1 ) = CLIP( data );
				data = SCALE( y + Buv, DCT_SHIFT );
				*( pImage + 0 ) = CLIP( data );
				pImage += SIZE_RGBQ;
				pY++;
			}
			pC++;
		}
		( PBYTE )pImage += SkipRGB;
		( PBYTE )pY += SkipY;
		( PBYTE )pC += SkipUV;
	}
}


void 	PASCAL	  PutImage525_YUY2( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC )
{
	unsigned long	*pDestBuffer=(unsigned long *)pImage;
	int	SkipDest, StrideY, StrideUV;
	int x;
	StrideY = ( DCT_BUFFER_STRIDE - SizeX ) * 2;
	StrideUV = ( DCT_BUFFER_STRIDE - SizeX / 4 ) * 2;
	SkipDest = (Stride - SizeX * 2)>>2;

	for (;SizeY;SizeY--)
	{
		for (x=SizeX/4;x;x--)
		{
			short u,v;
			short y0,y1;
			unsigned long yuyv;
			u=SCALEUV(pC[0]);
			v=SCALEUV(pC[8]);
			u=(u>>3)+128;	// V
			v=(v>>3)+128;	// U
			if (u>255) u=255;
			if (u<0) u=0;
			if (v>255) v=255;
			if (v<0) v=0;

			y0=SCALEY(pY[0])>>3;
			if (y0>255) y0=255;
			if (y0<0) y0=0;
			
			y1=SCALEY(pY[1])>>3;
			if (y1>255) y1=255;
			if (y1<0) y1=0;
			
			yuyv=y0|(v<<8)|(y1<<16)|(u<<24);
			pDestBuffer[0]=yuyv;
		
			y0=SCALEY(pY[2])>>3;
			if (y0>255) y0=255;
			if (y0<0) y0=0;
			
			y1=SCALEY(pY[3])>>3;
			if (y1>255) y1=255;
			if (y1<0) y1=0;
			
			yuyv=y0|(v<<8)|(y1<<16)|(u<<24);
			pDestBuffer[1]=yuyv;
		
			pC++;
			pY+=4;
			pDestBuffer+=2;
		}
		pC+=(StrideUV/2);
		pY+=(StrideY/2);
		pDestBuffer+=SkipDest;
	}
}

//------------------------------------------------------------------------------
// Call back function for writing decompressed RGB image data.
//------------------------------------------------------------------------------

void PASCAL PutImage625_RGBQ( PBYTE pImage, int Stride, PDCT_DATA pY )
{
	int	SkipRGB, data, xs, ys, y, u, v, Ruv, Guv, Buv;
	PDCT_DATA pUV;

	pUV = pY + 16;
	SkipRGB = Stride * 2 - 8 * 2 * SIZE_RGBQ;

	for( ys = 8 ; ys ; ys-- )
	{
		for( xs = 8 ; xs ; xs-- )
		{
			v = SCALEUV(pUV[0]);
			u = SCALEUV(pUV[8]);
			Ruv = SCALE( v * mmxR_FROM_V                  , mmxCOEFF_PRECISION );
			Guv = SCALE( u * mmxG_FROM_U + v * mmxG_FROM_V, mmxCOEFF_PRECISION );
			Buv = SCALE( u * mmxB_FROM_U                  , mmxCOEFF_PRECISION );

			y = SCALEY(pY[0]);
			data = SCALE( y + Ruv, DCT_SHIFT );
			*( pImage + 2 ) = CLIP( data );
			data = SCALE( y + Guv, DCT_SHIFT );
			*( pImage + 1 ) = CLIP( data );
			data = SCALE( y + Buv, DCT_SHIFT );
			*( pImage + 0 ) = CLIP( data );

			y = SCALEY(pY[1]);
			data = SCALE( y + Ruv, DCT_SHIFT );
			*( pImage + SIZE_RGBQ + 2 ) = CLIP( data );
			data = SCALE( y + Guv, DCT_SHIFT );
			*( pImage + SIZE_RGBQ + 1 ) = CLIP( data );
			data = SCALE( y + Buv, DCT_SHIFT );
			*( pImage + SIZE_RGBQ + 0 ) = CLIP( data );

			y = SCALEY(pY[DCT_BUFFER_STRIDE]);
			data = SCALE( y + Ruv, DCT_SHIFT );
			*( pImage + Stride + 2 ) = CLIP( data );
			data = SCALE( y + Guv, DCT_SHIFT );
			*( pImage + Stride + 1 ) = CLIP( data );
			data = SCALE( y + Buv, DCT_SHIFT );
			*( pImage + Stride + 0 ) = CLIP( data );

			y = SCALEY(pY[DCT_BUFFER_STRIDE+1]);
			data = SCALE( y + Ruv, DCT_SHIFT );
			*( pImage + Stride + SIZE_RGBQ + 2 ) = CLIP( data );
			data = SCALE( y + Guv, DCT_SHIFT );
			*( pImage + Stride + SIZE_RGBQ + 1 ) = CLIP( data );
			data = SCALE( y + Buv, DCT_SHIFT );
			*( pImage + Stride + SIZE_RGBQ + 0 ) = CLIP( data );

			pImage += SIZE_RGBQ * 2;
			pY += 2;
			pUV++;
		}
		pImage += SkipRGB;
		pY += DCT_BUFFER_STRIDE * 2 - 8 * 2;
		pUV += DCT_BUFFER_STRIDE - 8;
	}
}

//------------------------------------------------------------------------------
// Call back function for writing decompressed RGB image data.
//------------------------------------------------------------------------------

void PASCAL PutImage625_RGB16( PBYTE pImage, int Stride, PDCT_DATA pY )
{
	int	SkipRGB, data, xs, ys, y, u, v, Ruv, Guv, Buv;
	PDCT_DATA pUV;

	pUV = pY + 16;
	SkipRGB = Stride * 2 - 8 * 2 * SIZE_RGB16;

	for( ys = 8 ; ys ; ys-- )
	{
		for( xs = 8 ; xs ; xs-- )
		{
			int32 o=0;

			v = SCALEUV(pUV[0]);
			u = SCALEUV(pUV[8]);
			Ruv = SCALE( v * mmxR_FROM_V                  , mmxCOEFF_PRECISION );
			Guv = SCALE( u * mmxG_FROM_U + v * mmxG_FROM_V, mmxCOEFF_PRECISION );
			Buv = SCALE( u * mmxB_FROM_U                  , mmxCOEFF_PRECISION );

			y = SCALEY(pY[0]);
			data = SCALE( y + Ruv, DCT_SHIFT );	// red is 5 bits
			o=(CLIP(data)&0xf8)<<(11-3);
			data = SCALE( y + Guv, DCT_SHIFT );	// green is 6 bits
			o|=(CLIP(data)&0xfc)<<(5-2);
			data = SCALE( y + Buv, DCT_SHIFT );	// blue is 5 bits
			o|=((CLIP(data)&0xf8)>>3);

			y = SCALEY(pY[1]);
			data = SCALE( y + Ruv, DCT_SHIFT);
			o|=(CLIP(data)&0xf8)<<(27-3);
			data = SCALE( y + Guv, DCT_SHIFT);
			o|=(CLIP(data)&0xfc)<<(21-2);
			data = SCALE( y + Buv, DCT_SHIFT);
			o|=(CLIP(data)&0xf8)<<(16-3);
			*((uint32*)(pImage))=o;
	
			y = SCALEY(pY[DCT_BUFFER_STRIDE]);
			data = SCALE( y + Ruv, DCT_SHIFT );	// red is 5 bits
			o=(CLIP(data)&0xf8)<<(11-3);
			data = SCALE( y + Guv, DCT_SHIFT );	// green is 6 bits
			o|=(CLIP(data)&0xfc)<<(5-2);
			data = SCALE( y + Buv, DCT_SHIFT );	// blue is 5 bits
			o|=((CLIP(data)&0xf8)>>3);
			*((uint16*)(pImage+Stride))=o;
	
			y = SCALEY(pY[DCT_BUFFER_STRIDE+1]);
			data = SCALE( y + Ruv, DCT_SHIFT );	// red is 5 bits
			o=(CLIP(data)&0xf8)<<(11-3);
			data = SCALE( y + Guv, DCT_SHIFT );	// green is 6 bits
			o|=(CLIP(data)&0xfc)<<(5-2);
			data = SCALE( y + Buv, DCT_SHIFT );	// blue is 5 bits
			o|=((CLIP(data)&0xf8)>>3);
			*((uint16*)(pImage+Stride+SIZE_RGB16))=o;

			pImage += SIZE_RGB16 * 2;
			pY += 2;
			pUV++;
		}
		pImage += SkipRGB;
		pY += DCT_BUFFER_STRIDE * 2 - 8 * 2;
		pUV += DCT_BUFFER_STRIDE - 8;
	}
}
//------------------------------------------------------------------------------
// Call back function for writing decompressed RGB image data.
//------------------------------------------------------------------------------

void PASCAL PutImage625_RGB15( PBYTE pImage, int Stride, PDCT_DATA pY )
{
	int	SkipRGB, data, xs, ys, y, u, v, Ruv, Guv, Buv;
	PDCT_DATA pUV;

	pUV = pY + 16;
	SkipRGB = Stride * 2 - 8 * 2 * SIZE_RGB16;

	for( ys = 8 ; ys ; ys-- )
	{
		for( xs = 8 ; xs ; xs-- )
		{
			int32 o=0;

			v = SCALEUV(pUV[0]);
			u = SCALEUV(pUV[8]);
			Ruv = SCALE( v * mmxR_FROM_V                  , mmxCOEFF_PRECISION );
			Guv = SCALE( u * mmxG_FROM_U + v * mmxG_FROM_V, mmxCOEFF_PRECISION );
			Buv = SCALE( u * mmxB_FROM_U                  , mmxCOEFF_PRECISION );

			y = SCALEY(pY[0]);
			data = SCALE( y + Ruv, DCT_SHIFT );	// red is 5 bits
			o=(CLIP(data)&0xf8)<<(10-3);
			data = SCALE( y + Guv, DCT_SHIFT );	// green is 6 bits
			o|=(CLIP(data)&0xf8)<<(5-3);
			data = SCALE( y + Buv, DCT_SHIFT );	// blue is 5 bits
			o|=((CLIP(data)&0xf8)>>3);

			y = SCALEY(pY[1]);
			data = SCALE( y + Ruv, DCT_SHIFT);
			o|=(CLIP(data)&0xf8)<<(26-3);
			data = SCALE( y + Guv, DCT_SHIFT);
			o|=(CLIP(data)&0xf8)<<(21-3);
			data = SCALE( y + Buv, DCT_SHIFT);
			o|=(CLIP(data)&0xf8)<<(16-3);
			*((uint32*)(pImage))=o;
	
			y = SCALEY(pY[DCT_BUFFER_STRIDE]);
			data = SCALE( y + Ruv, DCT_SHIFT );	// red is 5 bits
			o=(CLIP(data)&0xf8)<<(10-3);
			data = SCALE( y + Guv, DCT_SHIFT );	// green is 6 bits
			o|=(CLIP(data)&0xf8)<<(5-3);
			data = SCALE( y + Buv, DCT_SHIFT );	// blue is 5 bits
			o|=((CLIP(data)&0xf8)>>3);
			*((uint16*)(pImage+Stride))=o;
	
			y = SCALEY(pY[DCT_BUFFER_STRIDE+1]);
			data = SCALE( y + Ruv, DCT_SHIFT );	// red is 5 bits
			o=(CLIP(data)&0xf8)<<(10-3);
			data = SCALE( y + Guv, DCT_SHIFT );	// green is 6 bits
			o|=(CLIP(data)&0xf8)<<(5-3);
			data = SCALE( y + Buv, DCT_SHIFT );	// blue is 5 bits
			o|=((CLIP(data)&0xf8)>>3);
			*((uint16*)(pImage+Stride+SIZE_RGB16))=o;

			pImage += SIZE_RGB16 * 2;
			pY += 2;
			pUV++;
		}
		pImage += SkipRGB;
		pY += DCT_BUFFER_STRIDE * 2 - 8 * 2;
		pUV += DCT_BUFFER_STRIDE - 8;
	}
}

void PASCAL PutImage625_YUY2( PBYTE pImage, int Stride, PDCT_DATA pY )
{
	unsigned long *	pDestBuffer;
	int		DestSkip;
	int		LoopXCount, LoopYCount;
	short	*pUV;
	
	DestSkip = ( Stride * 2 ) - 16 * 2;
	pDestBuffer = (unsigned long *)pImage;//(PutImageBuffer + StartY * Stride + StartX * 2);
	pUV=&pY[16];
	
	for (LoopYCount=8;LoopYCount;LoopYCount--)
	{
		for (LoopXCount=8;LoopXCount;LoopXCount--)
		{
			short u=SCALEUV(pUV[0]>>3);
			short v=SCALEUV(pUV[8]>>3);
			unsigned long yuyv;
			short y0,y1;
			
			if (v<0) v=0;
			if (v>255) v=255;
			if (u<0) u=0;
			if (u>255) u=255;

			y0=SCALEY(pY[0]>>3);
			y1=SCALEY(pY[1]>>3);
			if (y0<0) y0=0;
			if (y0>255) y0=255;
			if (y1<0) y1=0;
			if (y1>255) y1=255;
			
			yuyv=y0|(v<<8)|(y1<<16)|(u<<24);
			pDestBuffer[0]=yuyv;

			y0=SCALEY(pY[DCT_BUFFER_STRIDE]>>3);
			y1=SCALEY(pY[DCT_BUFFER_STRIDE+1]>>3);
			if (y0<0) y0=0;
			if (y0>255) y0=255;
			if (y1<0) y1=0;
			if (y1>255) y1=255;

			yuyv=y0|(v<<8)|(y1<<16)|(u<<24);			
			pDestBuffer[Stride/4]=yuyv;
			
			pUV++;
			pY+=2;
			pDestBuffer++;
		}
		pUV+=(DCT_BUFFER_STRIDE-8);
		pY+=(DCT_BUFFER_STRIDE*2-8*2);
		pDestBuffer+=(DestSkip/4);
	}
}


