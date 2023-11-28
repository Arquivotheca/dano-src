//=============================================================================
// Description:
//
//
//
//
//
// Copyright:
//		Copyright (c) 1998 CANOPUS Co.,Ltd.
//
// History:
//		07/23/98 Tom > Creaded.
//
//=============================================================================
#include <windows.h>
#include "csedv.h"

//-----------------------------------------------------------------------------
// external valiables
//-----------------------------------------------------------------------------

extern	int		GetImageStride;
extern	PBYTE	GetImageBuffer;

//-----------------------------------------------------------------------------
// define
//-----------------------------------------------------------------------------

#define	TRANS_SHIFT		3						// ‚c‚b‚s—p‚É‚ ‚ç‚©‚¶‚ß‚W”{‚µ‚Ä‚¨‚­
#define	SIZE_YUY2		2

//-----------------------------------------------------------------------------
// Description:                                                  Tom (07/24/98)
//		translate image YUY2 data to DCT buffer for MMX.
//
// Input:
//
// Output:
//
//-----------------------------------------------------------------------------

void PASCAL GetImage525_YUY2( int StartX, int StartY, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pUV )
{
	PBYTE	pSrc;
	int		SkipY, SkipUV, SkipSrc;
	int		x, y, u, v;

	pSrc = GetImageBuffer + GetImageStride * StartY + StartX * 2;

	SkipY = DCT_BUFFER_STRIDE - SizeX;
	SkipUV = DCT_BUFFER_STRIDE - SizeX / 4;
	SkipSrc = GetImageStride - SizeX * 2;

	for( y = SizeY ; y ; y-- )
	{
		for( x = SizeX / 4 ; x ; x--, pUV++ )
		{
			*pY++ = ( ( int )( *pSrc++ )  ) << TRANS_SHIFT;
			u = *pSrc++;
			*pY++ = ( ( int )( *pSrc++ )  ) << TRANS_SHIFT;
			v = *pSrc++;
			*pY++ = ( ( int )( *pSrc++ )  ) << TRANS_SHIFT;
			u += *pSrc++;
			*pY++ = ( ( int )( *pSrc++ )  ) << TRANS_SHIFT;
			v += *pSrc++;
			*( pUV     ) = ( v - 128 * 2 ) << ( TRANS_SHIFT - 1 );
			*( pUV + 8 ) = ( u - 128 * 2 ) << ( TRANS_SHIFT - 1 );
		}
		pY += SkipY;
		pUV += SkipUV;
		pSrc += SkipSrc;
	}
}

//-----------------------------------------------------------------------------
// Description:                                                  Tom (09/09/98)
//
//
// Input:
//
// Output:
//
//-----------------------------------------------------------------------------

void PASCAL GetImage625_YUY2( int StartX, int StartY, PDCT_DATA pY )
{
	PBYTE		pSrcBuffer;
	PDCT_DATA	pUV;
	int			SrcSkip, xs, ys;

	pUV = pY + 16;
	pSrcBuffer = GetImageBuffer + StartY * GetImageStride + StartX * SIZE_YUY2;
	SrcSkip = GetImageStride * 2 - 8 * 2 * SIZE_YUY2;

	for( ys = 8 ; ys ; ys-- )
	{
		for( xs = 8 ; xs ; xs-- )
		{
			*( pY     ) = *( pSrcBuffer             ) << TRANS_SHIFT;
			*( pY + 1 ) = *( pSrcBuffer + SIZE_YUY2 ) << TRANS_SHIFT;
			*( pY + DCT_BUFFER_STRIDE ) = *( pSrcBuffer + GetImageStride ) << TRANS_SHIFT;
			*( pY + DCT_BUFFER_STRIDE + 1 ) = *( pSrcBuffer + GetImageStride + SIZE_YUY2 ) << TRANS_SHIFT;
			*( pUV + 8 ) = ( *( pSrcBuffer + 1 ) + *( pSrcBuffer + GetImageStride + 1 ) - 128 * 2 ) << ( TRANS_SHIFT - 1 );
			*( pUV + 0 ) = ( *( pSrcBuffer + 3 ) + *( pSrcBuffer + GetImageStride + 3 ) - 128 * 2 ) << ( TRANS_SHIFT - 1 );

			pSrcBuffer += SIZE_YUY2 * 2;
			pY += 2;
			pUV++;
		}
		pSrcBuffer += SrcSkip;
		pY += DCT_BUFFER_STRIDE * 2 - 8 * 2;
		pUV += DCT_BUFFER_STRIDE - 8;
	}
}

