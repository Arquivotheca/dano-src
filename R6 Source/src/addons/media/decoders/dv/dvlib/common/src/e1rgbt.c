//=============================================================================
// Description:
//		Software DV codec color space transform routine
//		from RGB-24bits(8:8:8) to YUV for encoder DCT buffer.
//
//
// Copyright:
//		Copyright (c) 1998 CANOPUS Co.,Ltd.
//
// History:
//		12/09/97 Tom > Creaded.
//
//=============================================================================
#include <windows.h>
#include "csedv.h"
#include "rgbtoyuv.h"

#define	USE_ASSEMBLER		1	// (1 default)

extern	int		GetImageStride;
extern	PBYTE	GetImageBuffer;

#define	SIZE_RGBT		3

//-----------------------------------------------------------------------------
// this valiables are used by other e1RGxx_n.c
//-----------------------------------------------------------------------------
/*
short	UnpackB[ 4 ] = { 0, 0, 0, 0 };
short	UnpackG[ 4 ] = { 0, 0, 0, 0 };
short	UnpackR[ 4 ] = { 0, 0, 0, 0 };

short	mmxYFromR[ 4 ] = { Y_FROM_R, Y_FROM_R, Y_FROM_R, Y_FROM_R };
short	mmxYFromG[ 4 ] = { Y_FROM_G, Y_FROM_G, Y_FROM_G, Y_FROM_G };
short	mmxYFromB[ 4 ] = { Y_FROM_B, Y_FROM_B, Y_FROM_B, Y_FROM_B };
short	mmxUFromR[ 4 ] = { U_FROM_R, U_FROM_R, U_FROM_R, U_FROM_R };
short	mmxUFromG[ 4 ] = { U_FROM_G, U_FROM_G, U_FROM_G, U_FROM_G };
short	mmxUFromB[ 4 ] = { U_FROM_B, U_FROM_B, U_FROM_B, U_FROM_B };
short	mmxVFromR[ 4 ] = { V_FROM_R, V_FROM_R, V_FROM_R, V_FROM_R };
short	mmxVFromG[ 4 ] = { V_FROM_G, V_FROM_G, V_FROM_G, V_FROM_G };
short	mmxVFromB[ 4 ] = { V_FROM_B, V_FROM_B, V_FROM_B, V_FROM_B };
//short	Y_Offset[ 4 ]  = {-Y_CENTER,-Y_CENTER,-Y_CENTER,-Y_CENTER };
*/
//------------------------------------------------------------------------------
// Get image data from PowerView buffer
//------------------------------------------------------------------------------

void PASCAL GetImage525_RGBT( int StartX, int StartY, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pUV )
{
	PBYTE	pSrcBuffer;
	int	SrcSkip, SkipY, SkipUV;
	int	i, x, y, r, g, b, ba, ga, ra;

	pSrcBuffer = GetImageBuffer + StartY * GetImageStride + StartX * SIZE_RGBT;
	SrcSkip = GetImageStride - SizeX * SIZE_RGBT;
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
			for( i = 0 ; i < 4 ; i++, pSrcBuffer += SIZE_RGBT )
			{
				b = *( pSrcBuffer + 0 );
				g = *( pSrcBuffer + 1 );
				r = *( pSrcBuffer + 2 );
				*pY++ = ( Y_FROM_R * r + Y_FROM_G * g + Y_FROM_B * b ) >> ( COEFF_PRECISION - DCT_SHIFT );
				ba += b;
				ga += g;
				ra += r;
			}
			*( pUV + 0 ) = ( ra * V_FROM_R + ga * V_FROM_G + ba * V_FROM_B ) >> ( COEFF_PRECISION - DCT_SHIFT + 2 );
			*( pUV + 8 ) = ( ra * U_FROM_R + ga * U_FROM_G + ba * U_FROM_B ) >> ( COEFF_PRECISION - DCT_SHIFT + 2 );
			pUV++;
		}
		( PBYTE )pSrcBuffer += SrcSkip;
		( PBYTE )pY += SkipY;
		( PBYTE )pUV += SkipUV;
	}
}

//------------------------------------------------------------------------------
// Get image data from source buffer for 625 system
//------------------------------------------------------------------------------

void PASCAL GetImage625_RGBT( int StartX, int StartY, PDCT_DATA pY )
{
	PBYTE	pSrcBuffer;
	int	SrcSkip;
	int	xs, ys, r, g, b, ba, ga, ra;
	PDCT_DATA pUV;

	pUV = pY + 16;
	pSrcBuffer = GetImageBuffer + StartY * GetImageStride + StartX * SIZE_RGBT;
	SrcSkip = GetImageStride * 2 - 8 * 2 * SIZE_RGBT;

	for( ys = 8 ; ys ; ys-- )
	{
		for( xs = 8 ; xs ; xs-- )
		{
			ba = *( pSrcBuffer + 0 );
			ga = *( pSrcBuffer + 1 );
			ra = *( pSrcBuffer + 2 );
			*pY = ( Y_FROM_R * ra + Y_FROM_G * ga + Y_FROM_B * ba ) >> ( COEFF_PRECISION - DCT_SHIFT );

			b = *( pSrcBuffer + SIZE_RGBT + 0 );
			g = *( pSrcBuffer + SIZE_RGBT + 1 );
			r = *( pSrcBuffer + SIZE_RGBT + 2 );
			*( pY + 1 ) = ( Y_FROM_R * r + Y_FROM_G * g + Y_FROM_B * b ) >> ( COEFF_PRECISION - DCT_SHIFT );
			ba += b;
			ga += g;
			ra += r;

			b = *( pSrcBuffer + GetImageStride + 0 );
			g = *( pSrcBuffer + GetImageStride + 1 );
			r = *( pSrcBuffer + GetImageStride + 2 );
			*( pY + DCT_BUFFER_STRIDE ) = ( Y_FROM_R * r + Y_FROM_G * g + Y_FROM_B * b ) >> ( COEFF_PRECISION - DCT_SHIFT );
			ba += b;
			ga += g;
			ra += r;

			b = *( pSrcBuffer + GetImageStride + SIZE_RGBT + 0 );
			g = *( pSrcBuffer + GetImageStride + SIZE_RGBT + 1 );
			r = *( pSrcBuffer + GetImageStride + SIZE_RGBT + 2 );
			*( pY + DCT_BUFFER_STRIDE + 1 ) = ( Y_FROM_R * r + Y_FROM_G * g + Y_FROM_B * b ) >> ( COEFF_PRECISION - DCT_SHIFT );
			ba += b;
			ga += g;
			ra += r;

			*( pUV + 0 ) = ( ra * V_FROM_R + ga * V_FROM_G + ba * V_FROM_B ) >> ( COEFF_PRECISION - DCT_SHIFT + 2 );
			*( pUV + 8 ) = ( ra * U_FROM_R + ga * U_FROM_G + ba * U_FROM_B ) >> ( COEFF_PRECISION - DCT_SHIFT + 2 );

			pSrcBuffer += SIZE_RGBT * 2;
			pY += 2;
			pUV++;
		}
		pSrcBuffer += SrcSkip;
		pY += DCT_BUFFER_STRIDE * 2 - 8 * 2;
		pUV += DCT_BUFFER_STRIDE - 8;
	}
}

