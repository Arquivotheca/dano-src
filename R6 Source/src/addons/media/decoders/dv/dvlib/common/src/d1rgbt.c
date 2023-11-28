//=============================================================================
// Description:
//		Software DV Codec Render Engine (for RGB24)
//
//
//
//
// Copyright:
//		Copyright (c) 1998 CANOPUS Co.,Ltd.
//
// History:
//		1997 Tom > Creaded.
//
//=============================================================================
#include <windows.h>
#include "csedv.h"
#include "putimage.h"
#include "yuvtorgb.h"

#define	USE_ASSEMBLER		1	// (1 default)

#define	SIZE_RGBT			3
#define	SCALE( d, s )		( ( ( d ) + ( 1 << ( ( s ) - 1 ) ) ) >> ( s ) )
#define	CLIP( rgb )			( ( BYTE )( rgb < 0 ? 0 : rgb > 255 ? 255 : rgb ) )

//-----------------------------------------------------------------------------
// real definition of mmxXFromY[ 4 ]
//-----------------------------------------------------------------------------

DCT_DATA	mmxRFromV[ 4 ] = { mmxR_FROM_V, mmxR_FROM_V, mmxR_FROM_V, mmxR_FROM_V };
DCT_DATA	mmxGFromU[ 4 ] = { mmxG_FROM_U, mmxG_FROM_U, mmxG_FROM_U, mmxG_FROM_U };
DCT_DATA	mmxGFromV[ 4 ] = { mmxG_FROM_V, mmxG_FROM_V, mmxG_FROM_V, mmxG_FROM_V };
DCT_DATA	mmxBFromU[ 4 ] = { mmxB_FROM_U, mmxB_FROM_U, mmxB_FROM_U, mmxB_FROM_U };

//------------------------------------------------------------------------------
// Call back function for writing decompressed RGB image data.
//------------------------------------------------------------------------------

void PASCAL PutImage525_RGBT( int StartX, int StartY, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pUV )
{
	PBYTE	pDestBuffer;
	int		SkipRGB, SkipY, SkipUV;
	int		data, x, y, u, v, Ruv, Guv, Buv, i;

	pDestBuffer = PutImageBuffer + StartY * PutImageStride + StartX * SIZE_RGBT;
	SkipRGB = PutImageStride - SizeX * SIZE_RGBT;
	SkipY = ( DCT_BUFFER_STRIDE - SizeX ) * 2;
	SizeX >>= 2;
	SkipUV = ( DCT_BUFFER_STRIDE - SizeX ) * 2;

	for( ; SizeY ; SizeY-- )
	{
		for( x = SizeX ; x ; x-- )
		{
			v = pUV[ 0 ];
			u = pUV[ 8 ];
			Ruv = SCALE( v * R_FROM_V               , COEFF_PRECISION );
			Guv = SCALE( u * G_FROM_U + v * G_FROM_V, COEFF_PRECISION );
			Buv = SCALE( u * B_FROM_U               , COEFF_PRECISION );

			for( i = 4 ; i ; i-- )
			{
				y = *pY;
				data = SCALE( y + Ruv, DCT_SHIFT );
				pDestBuffer[ 2 ] = CLIP( data );
				data = SCALE( y + Guv, DCT_SHIFT );
				pDestBuffer[ 1 ] = CLIP( data );
				data = SCALE( y + Buv, DCT_SHIFT );
				pDestBuffer[ 0 ] = CLIP( data );
				pDestBuffer += SIZE_RGBT;
				pY++;
			}
			pUV++;
		}
		( PBYTE )pDestBuffer += SkipRGB;
		( PBYTE )pY += SkipY;
		( PBYTE )pUV += SkipUV;
	}
}

//------------------------------------------------------------------------------
// Call back function for writing decompressed RGB image data.
//------------------------------------------------------------------------------

void PASCAL PutImage625_RGBT( int StartX, int StartY, PDCT_DATA pY )
{
	PBYTE	pDestBuffer;
	int	SkipRGB, data, xs, ys, y, u, v, Ruv, Guv, Buv;
	PDCT_DATA pUV;

	pUV = pY + 16;
	pDestBuffer = PutImageBuffer + StartY * PutImageStride + StartX * SIZE_RGBT;
	SkipRGB = PutImageStride * 2 - 8 * 2 * SIZE_RGBT;

	for( ys = 8 ; ys ; ys-- )
	{
		for( xs = 8 ; xs ; xs-- )
		{
			v = pUV[ 0 ];
			u = pUV[ 8 ];
			Ruv = SCALE( v * R_FROM_V               , COEFF_PRECISION );
			Guv = SCALE( u * G_FROM_U + v * G_FROM_V, COEFF_PRECISION );
			Buv = SCALE( u * B_FROM_U               , COEFF_PRECISION );

			y = *pY;
			data = SCALE( y + Ruv, DCT_SHIFT );
			pDestBuffer[ 2 ] = CLIP( data );
			data = SCALE( y + Guv, DCT_SHIFT );
			pDestBuffer[ 1 ] = CLIP( data );
			data = SCALE( y + Buv, DCT_SHIFT );
			pDestBuffer[ 0 ] = CLIP( data );

			y = *( pY + 1 );
			data = SCALE( y + Ruv, DCT_SHIFT );
			pDestBuffer[ SIZE_RGBT + 2 ] = CLIP( data );
			data = SCALE( y + Guv, DCT_SHIFT );
			pDestBuffer[ SIZE_RGBT + 1 ] = CLIP( data );
			data = SCALE( y + Buv, DCT_SHIFT );
			pDestBuffer[ SIZE_RGBT + 0 ] = CLIP( data );

			y = *( pY + DCT_BUFFER_STRIDE );
			data = SCALE( y + Ruv, DCT_SHIFT );
			pDestBuffer[ PutImageStride + 2 ] = CLIP( data );
			data = SCALE( y + Guv, DCT_SHIFT );
			pDestBuffer[ PutImageStride + 1 ] = CLIP( data );
			data = SCALE( y + Buv, DCT_SHIFT );
			pDestBuffer[ PutImageStride + 0 ] = CLIP( data );

			y = *( pY + DCT_BUFFER_STRIDE + 1 );
			data = SCALE( y + Ruv, DCT_SHIFT );
			pDestBuffer[ PutImageStride + SIZE_RGBT + 2 ] = CLIP( data );
			data = SCALE( y + Guv, DCT_SHIFT );
			pDestBuffer[ PutImageStride + SIZE_RGBT + 1 ] = CLIP( data );
			data = SCALE( y + Buv, DCT_SHIFT );
			pDestBuffer[ PutImageStride + SIZE_RGBT + 0 ] = CLIP( data );

			pDestBuffer += SIZE_RGBT * 2;
			pY += 2;
			pUV++;
		}
		pDestBuffer += SkipRGB;
		pY += DCT_BUFFER_STRIDE * 2 - 8 * 2;
		pUV += DCT_BUFFER_STRIDE - 8;
	}
}

