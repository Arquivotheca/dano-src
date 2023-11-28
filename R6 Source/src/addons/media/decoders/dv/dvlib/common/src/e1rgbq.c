//=============================================================================
// Description:
//		Software DV codec color space transform routine
//		from RGB-32bits(8:8:8:?) to YUV for encoder DCT buffer.
//
//
//
// Copyright:
//		Copyright (c) 1998 Canopus Co.,Ltd. All Rights Reserved.
//		Developed in San Jose, CA, U.S.A.
// History:
//		09/10/98 Tom > Creaded.
//
//=============================================================================
#include <windows.h>
#include "csedv.h"
#include "rgbtoyuv.h"
#include "getimage.h"

#define	USE_ASSEMBLER		1	// (1 default)

#define	SIZE_RGBQ		4
#define	SIZE_RGB16		2
#define	SIZE_RGB15		2

// auto defined for powerpc
//#define __POWERPC__

#ifdef __POWERPC__
#define SCALEUV(X) (X)
#define SCALEY(X) (X)
#else
#define SCALEUV(X) ((X)<<1)
#define SCALEY(X) (((X)-1024)<<1)
#endif


//------------------------------------------------------------------------------
// Get image data from PowerView buffer
//------------------------------------------------------------------------------

void 	PASCAL	  GetImage525_RGBQ( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC )
{
	int		i, x, y, r, g, b, ba, ga, ra, SrcSkip, SkipY, SkipUV;
	SrcSkip = Stride - SizeX * SIZE_RGBQ;
	SkipY = ( DCT_BUFFER_STRIDE - SizeX ) * 2;
	SizeX >>= 2;
	SkipUV = ( DCT_BUFFER_STRIDE - SizeX ) * 2;

	for( y = 0 ; y < SizeY ; y++ )
	{
		for( x = SizeX ; x ; x-- )
		{
			ba = 0;
			ga = 0;
			ra = 0;
			for( i = 0 ; i < 4 ; i++, pImage += SIZE_RGBQ )
			{
				b = *( pImage + 0 );
				g = *( pImage + 1 );
				r = *( pImage + 2 );
				*pY++ = SCALEY(( mmxY_FROM_R * r + mmxY_FROM_G * g + mmxY_FROM_B * b ) >> ( mmxCOEFF_PRECISION - DCT_SHIFT ));
				ba += b;
				ga += g;
				ra += r;
			}
			*( pC + 0 ) = SCALEUV(( ra * mmxV_FROM_R + ga * mmxV_FROM_G + ba * mmxV_FROM_B ) >> ( mmxCOEFF_PRECISION - DCT_SHIFT + 2 ));
			*( pC + 8 ) = SCALEUV(( ra * mmxU_FROM_R + ga * mmxU_FROM_G + ba * mmxU_FROM_B ) >> ( mmxCOEFF_PRECISION - DCT_SHIFT + 2 ));
			pC++;
		}
		( PBYTE )pImage += SrcSkip;
		( PBYTE )pY += SkipY;
		( PBYTE )pC += SkipUV;
	}
}

void 	PASCAL	  GetImage525_RGB16( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC )
{
	int		i, x, y, r, g, b, ba, ga, ra, SrcSkip, SkipY, SkipUV;

	SrcSkip = Stride - SizeX * SIZE_RGB16;
	SkipY = ( DCT_BUFFER_STRIDE - SizeX ) * 2;
	SizeX >>= 2;
	SkipUV = ( DCT_BUFFER_STRIDE - SizeX ) * 2;

	for( y = 0 ; y < SizeY ; y++ )
	{
		for( x = SizeX ; x ; x-- )
		{
			ba = 0;
			ga = 0;
			ra = 0;
			for( i = 0 ; i < 4 ; i++, pImage += SIZE_RGB16 )
			{
				short v=*((short*)pImage);
				r=((v>>11)&0x1f)<<3;
				g=((v>>5)&0x3f)<<2;
				b=((v>>0)&0x1f)<<3;
				*pY++ = SCALEY(( mmxY_FROM_R * r + mmxY_FROM_G * g + mmxY_FROM_B * b ) >> ( mmxCOEFF_PRECISION - DCT_SHIFT ));
				ba += b;
				ga += g;
				ra += r;
			}
			*( pC + 0 ) = SCALEUV(( ra * mmxV_FROM_R + ga * mmxV_FROM_G + ba * mmxV_FROM_B ) >> ( mmxCOEFF_PRECISION - DCT_SHIFT + 2 ));
			*( pC + 8 ) = SCALEUV(( ra * mmxU_FROM_R + ga * mmxU_FROM_G + ba * mmxU_FROM_B ) >> ( mmxCOEFF_PRECISION - DCT_SHIFT + 2 ));
			pC++;
		}
		( PBYTE )pImage += SrcSkip;
		( PBYTE )pY += SkipY;
		( PBYTE )pC += SkipUV;
	}

}
void 	PASCAL	  GetImage525_RGB15( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC )
{
	int		i, x, y, r, g, b, ba, ga, ra, SrcSkip, SkipY, SkipUV;

	SrcSkip = Stride - SizeX * SIZE_RGB16;
	SkipY = ( DCT_BUFFER_STRIDE - SizeX ) * 2;
	SizeX >>= 2;
	SkipUV = ( DCT_BUFFER_STRIDE - SizeX ) * 2;

	for( y = 0 ; y < SizeY ; y++ )
	{
		for( x = SizeX ; x ; x-- )
		{
			ba = 0;
			ga = 0;
			ra = 0;
			for( i = 0 ; i < 4 ; i++, pImage += SIZE_RGB16 )
			{
				short v=*((short*)pImage);
				r=((v>>10)&0x1f)<<3;
				g=((v>>5)&0x1f)<<3;
				b=((v>>0)&0x1f)<<3;
				*pY++ = SCALEY(( mmxY_FROM_R * r + mmxY_FROM_G * g + mmxY_FROM_B * b ) >> ( mmxCOEFF_PRECISION - DCT_SHIFT ));
				ba += b;
				ga += g;
				ra += r;
			}
			*( pC + 0 ) = SCALEUV(( ra * mmxV_FROM_R + ga * mmxV_FROM_G + ba * mmxV_FROM_B ) >> ( mmxCOEFF_PRECISION - DCT_SHIFT + 2 ));
			*( pC + 8 ) = SCALEUV(( ra * mmxU_FROM_R + ga * mmxU_FROM_G + ba * mmxU_FROM_B ) >> ( mmxCOEFF_PRECISION - DCT_SHIFT + 2 ));
			pC++;
		}
		( PBYTE )pImage += SrcSkip;
		( PBYTE )pY += SkipY;
		( PBYTE )pC += SkipUV;
	}
}

//------------------------------------------------------------------------------
// Get image data from source buffer for 625 system
//------------------------------------------------------------------------------

void    PASCAL    GetImage625_RGBQ( PBYTE pImage, int Stride, PDCT_DATA pY )
{
	PDCT_DATA	pUV;
	int			SrcSkip, xs, ys, r, g, b, ba, ga, ra;

	pUV = pY + 16;
	SrcSkip = Stride * 2 - 8 * 2 * SIZE_RGBQ;

	for( ys = 8 ; ys ; ys-- )
	{
		for( xs = 8 ; xs ; xs-- )
		{
			ba = *( pImage + 0 );
			ga = *( pImage + 1 );
			ra = *( pImage + 2 );
			*pY = SCALEY(( Y_FROM_R * ra + Y_FROM_G * ga + Y_FROM_B * ba ) >> ( COEFF_PRECISION - DCT_SHIFT ));

			b = *( pImage + SIZE_RGBQ + 0 );
			g = *( pImage + SIZE_RGBQ + 1 );
			r = *( pImage + SIZE_RGBQ + 2 );
			*( pY + 1 ) = SCALEY(( Y_FROM_R * r + Y_FROM_G * g + Y_FROM_B * b ) >> ( COEFF_PRECISION - DCT_SHIFT ));
			ba += b;
			ga += g;
			ra += r;

			b = *( pImage + Stride + 0 );
			g = *( pImage + Stride + 1 );
			r = *( pImage + Stride + 2 );
			*( pY + DCT_BUFFER_STRIDE ) = SCALEY(( Y_FROM_R * r + Y_FROM_G * g + Y_FROM_B * b ) >> ( COEFF_PRECISION - DCT_SHIFT ));
			ba += b;
			ga += g;
			ra += r;

			b = *( pImage + Stride + SIZE_RGBQ + 0 );
			g = *( pImage + Stride + SIZE_RGBQ + 1 );
			r = *( pImage + Stride + SIZE_RGBQ + 2 );
			*( pY + DCT_BUFFER_STRIDE + 1 ) = SCALEY(( Y_FROM_R * r + Y_FROM_G * g + Y_FROM_B * b ) >> ( COEFF_PRECISION - DCT_SHIFT ));
			ba += b;
			ga += g;
			ra += r;

			*( pUV + 0 ) = SCALEUV(( ra * V_FROM_R + ga * V_FROM_G + ba * V_FROM_B ) >> ( COEFF_PRECISION - DCT_SHIFT + 2 ));
			*( pUV + 8 ) = SCALEUV(( ra * U_FROM_R + ga * U_FROM_G + ba * U_FROM_B ) >> ( COEFF_PRECISION - DCT_SHIFT + 2 ));

			pImage += SIZE_RGBQ * 2;
			pY += 2;
			pUV++;
		}
		pImage += SrcSkip;
		pY += DCT_BUFFER_STRIDE * 2 - 8 * 2;
		pUV += DCT_BUFFER_STRIDE - 8;
	}
}

void    PASCAL    GetImage625_RGB16( PBYTE pImage, int Stride, PDCT_DATA pY )

{
	PDCT_DATA	pUV;
	int			SrcSkip, xs, ys, r, g, b, ba, ga, ra;

	pUV = pY + 16;
	SrcSkip = Stride * 2 - 8 * 2 * SIZE_RGB16;

	for( ys = 8 ; ys ; ys-- )
	{
		for( xs = 8 ; xs ; xs-- )
		{
			short v=*((short*)pImage);
			ra=((v>>11)&0x1f)<<3;
			ga=((v>>5)&0x3f)<<2;
			ba=((v>>0)&0x1f)<<3;
			*pY = SCALEY(( Y_FROM_R*ra+Y_FROM_G*ga+Y_FROM_B*ba)>>(COEFF_PRECISION-DCT_SHIFT ));

			v=*((short*)(pImage+SIZE_RGB16));
			r=((v>>11)&0x1f)<<3;
			g=((v>>5)&0x3f)<<2;
			b=((v>>0)&0x1f)<<3;
			*( pY + 1 ) = SCALEY(( Y_FROM_R * r + Y_FROM_G * g + Y_FROM_B * b ) >> ( COEFF_PRECISION - DCT_SHIFT ));
			ba += b;
			ga += g;
			ra += r;

			v=*((short*)(pImage+Stride));
			r=((v>>11)&0x1f)<<3;
			g=((v>>5)&0x3f)<<2;
			b=((v>>0)&0x1f)<<3;
			*( pY + DCT_BUFFER_STRIDE ) = SCALEY(( Y_FROM_R * r + Y_FROM_G * g + Y_FROM_B * b ) >> ( COEFF_PRECISION - DCT_SHIFT ));
			ba += b;
			ga += g;
			ra += r;

			v=*((short*)(pImage+Stride+SIZE_RGB16));
			r=((v>>11)&0x1f)<<3;
			g=((v>>5)&0x3f)<<2;
			b=((v>>0)&0x1f)<<3;
			*( pY + DCT_BUFFER_STRIDE + 1 ) = SCALEY(( Y_FROM_R * r + Y_FROM_G * g + Y_FROM_B * b ) >> ( COEFF_PRECISION - DCT_SHIFT ));
			ba += b;
			ga += g;
			ra += r;

			*( pUV + 0 ) = SCALEUV(( ra * V_FROM_R + ga * V_FROM_G + ba * V_FROM_B ) >> ( COEFF_PRECISION - DCT_SHIFT + 2 ));
			*( pUV + 8 ) = SCALEUV(( ra * U_FROM_R + ga * U_FROM_G + ba * U_FROM_B ) >> ( COEFF_PRECISION - DCT_SHIFT + 2 ));

			pImage += SIZE_RGB16 * 2;
			pY += 2;
			pUV++;
		}
		pImage += SrcSkip;
		pY += DCT_BUFFER_STRIDE * 2 - 8 * 2;
		pUV += DCT_BUFFER_STRIDE - 8;
	}
}
void    PASCAL    GetImage625_RGB15( PBYTE pImage, int Stride, PDCT_DATA pY )
{
	PDCT_DATA	pUV;
	int			SrcSkip, xs, ys, r, g, b, ba, ga, ra;

	pUV = pY + 16;
	SrcSkip = Stride * 2 - 8 * 2 * SIZE_RGB16;

	for( ys = 8 ; ys ; ys-- )
	{
		for( xs = 8 ; xs ; xs-- )
		{
			short v=*((short*)pImage);
			ra=((v>>10)&0x1f)<<3;
			ga=((v>>5)&0x1f)<<3;
			ba=((v>>0)&0x1f)<<3;
			*pY = SCALEY(( mmxY_FROM_R*ra+mmxY_FROM_G*ga+mmxY_FROM_B*ba)>>(mmxCOEFF_PRECISION-DCT_SHIFT ));

			v=*((short*)(pImage+SIZE_RGB16));
			r=((v>>10)&0x1f)<<3;
			g=((v>>5)&0x1f)<<3;
			b=((v>>0)&0x1f)<<3;
			*( pY + 1 ) = SCALEY(( mmxY_FROM_R * r + mmxY_FROM_G * g + mmxY_FROM_B * b ) >> ( mmxCOEFF_PRECISION - DCT_SHIFT ));
			ba += b;
			ga += g;
			ra += r;

			v=*((short*)(pImage+Stride));
			r=((v>>10)&0x1f)<<3;
			g=((v>>5)&0x1f)<<3;
			b=((v>>0)&0x1f)<<3;
			*( pY + DCT_BUFFER_STRIDE ) = SCALEY(( mmxY_FROM_R * r + mmxY_FROM_G * g + mmxY_FROM_B * b ) >> ( mmxCOEFF_PRECISION - DCT_SHIFT ));
			ba += b;
			ga += g;
			ra += r;

			v=*((short*)(pImage+Stride+SIZE_RGB16));
			r=((v>>10)&0x1f)<<3;
			g=((v>>5)&0x1f)<<3;
			b=((v>>0)&0x1f)<<3;
			*( pY + DCT_BUFFER_STRIDE + 1 ) = SCALEY(( Y_FROM_R * r + Y_FROM_G * g + Y_FROM_B * b ) >> ( mmxCOEFF_PRECISION - DCT_SHIFT ));
			ba += b;
			ga += g;
			ra += r;

			*( pUV + 0 ) = SCALEUV(( ra * V_FROM_R + ga * V_FROM_G + ba * V_FROM_B ) >> ( COEFF_PRECISION - DCT_SHIFT + 2 ));
			*( pUV + 8 ) = SCALEUV(( ra * U_FROM_R + ga * U_FROM_G + ba * U_FROM_B ) >> ( COEFF_PRECISION - DCT_SHIFT + 2 ));

			pImage += SIZE_RGB16 * 2;
			pY += 2;
			pUV++;
		}
		pImage += SrcSkip;
		pY += DCT_BUFFER_STRIDE * 2 - 8 * 2;
		pUV += DCT_BUFFER_STRIDE - 8;
	}
}

void 	PASCAL	  GetImage525_YUY2( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC )
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
			unsigned long yuyv=pDestBuffer[0];
			short y0=((yuyv>>0)&0xff);
			short v=((yuyv>>8)&0xff);
			short y1=((yuyv>>16)&0xff);
			short u=((yuyv>>24)&0xff);
			pC[0]=SCALEUV((u-128)<<3);
			pC[8]=SCALEUV((v-128)<<3);
			pY[0]=SCALEY(y0<<3);
			pY[1]=SCALEY(y1<<3);
			yuyv=pDestBuffer[1];
			y0=((yuyv>>0)&0xff);
			y1=((yuyv>>16)&0xff);
			pY[2]=SCALEY(y0<<3);
			pY[3]=SCALEY(y1<<3);

			pC++;
			pY+=4;
			pDestBuffer+=2;
		}
		pC+=(StrideUV/2);
		pY+=(StrideY/2);
		pDestBuffer+=SkipDest;
	}
}
void    PASCAL    GetImage625_YUY2( PBYTE pImage, int Stride, PDCT_DATA pY )
{
	unsigned long *	pDestBuffer;
	int		DestSkip;
	int		LoopXCount, LoopYCount;
	short	*pUV;
	
	DestSkip = ( Stride * 2 ) - 16 * 2;
	pDestBuffer = (unsigned long *)pImage;
	pUV=&pY[16];
	
	for (LoopYCount=8;LoopYCount;LoopYCount--)
	{
		for (LoopXCount=8;LoopXCount;LoopXCount--)
		{
			unsigned long yuyv=pDestBuffer[0];
			short y0=((yuyv>>0)&0xff);
			short v=((yuyv>>8)&0xff);
			short y1=((yuyv>>16)&0xff);
			short u=((yuyv>>24)&0xff);
			pY[0]=SCALEY(y0<<3);
			pY[1]=SCALEY(y1<<3);
			pUV[0]=SCALEUV((u-128)<<3);
			pUV[8]=SCALEUV((v-128)<<3);
			yuyv=pDestBuffer[Stride/4];
			y0=((yuyv>>0)&0xff);
			y1=((yuyv>>16)&0xff);
			pY[DCT_BUFFER_STRIDE]=SCALEY(y0<<3);
			pY[DCT_BUFFER_STRIDE+1]=SCALEY(y1<<3);

			pUV++;
			pY+=2;
			pDestBuffer++;
		}
		pUV+=(DCT_BUFFER_STRIDE-8);
		pY+=(DCT_BUFFER_STRIDE*2-8*2);
		pDestBuffer+=(DestSkip/4);
	}
}
