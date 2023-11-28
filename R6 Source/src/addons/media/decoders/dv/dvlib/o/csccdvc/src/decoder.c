//
//
//  Software Motion JPEG encoder/decoder
//  VFW I/F layer
//
//  1996/05/09 sus-e modified to support YUY2 format.
//  Copyright (C) 1996,1997 CANOPUS Co., Ltd. All Rights Reserved
//
// codec				| Total3D		| PW GX			| using
// 						| 1024x768x16	| support DD	| DirectVideo
//						| no overlay	| YUY2 surface	|
// ---------------------+---------------+---------------+---------------
// Adaptec DVC.DLL		|  7.10	fps		|  7.10 fps		| hang up
// Canopus CSCDVSD.DLL	| 12.27	fps		| 19.37 fps		| 19.37 fps
//
//
#include <windows.h>
#include <vfw.h>
#include "codec.h"
#include "csedv.h"
#include "putimage.h"

//-----------------------------------------------------------------------------
// FourCC definitions
//-----------------------------------------------------------------------------

#define	BI_RGBT		mmioFOURCC( 'R', 'G', 'B', 'T' )	// 09/09/98 Tom > added. this is 24bits RGB top-down image.
#define	BI_RGBQ		mmioFOURCC( 'R', 'G', 'B', 'Q' )	// 07/30/98 Tom > added. this is 32bits RGB top-down image.
#define	BI_YUY2		mmioFOURCC( 'Y', 'U', 'Y', '2' )

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

int		PutImageStride;
PBYTE	PutImageBuffer;
PBYTE	GetStreamBuffer;

//------------------------------------------------------------------------------
// Decode DV frame
//------------------------------------------------------------------------------

int	PASCAL SoftCodecDecodeDV( PVOID lpInput, PBITMAPINFOHEADER lpbiOutput, PVOID lpOutput )
{
	static PVOID PutImageTableRGBT[][ 2 ] =
	{
		{    PutImage525_RGBT,    PutImage625_RGBT },
		{    PutImage525_RGBT,    PutImage625_RGBT },
		{    PutImage525_RGBT,    PutImage625_RGBT },
	};
	static PVOID PutImageTableRGBQ[][ 2 ] =
	{
		{    PutImage525_RGBQ,    PutImage625_RGBQ },
		{ mmxPutImage525_RGBQ, mmxPutImage625_RGBQ },
		{ mmxPutImage525_RGBQ, mmxPutImage625_RGBQ },
	};
	static PVOID PutImageTableYUY2[][ 2 ] =
	{
		{    PutImage525_YUY2,    PutImage625_YUY2 },
		{ mmxPutImage525_YUY2, mmxPutImage625_YUY2 },
		{ mmxPutImage525_YUY2, mmxPutImage625_YUY2 },
	};
	int		mode, height;
	PVOID   ( *PutImageTable )[ 2 ];

	PutImageBuffer = lpOutput;
	height = lpbiOutput->biHeight;			// 注意! アウトプット側の高さは負がある
	if( height < 0 ) height = - height;

	switch( lpbiOutput->biCompression )
	{
	case BI_RGB:
		PutImageStride = ( lpbiOutput->biWidth * ( lpbiOutput->biBitCount / 8 ) + 3 ) & ( ~3 );

		if( lpbiOutput->biHeight > 0 )
		{
			PutImageBuffer += ( height - 1 ) * PutImageStride;
			PutImageStride = - PutImageStride;
		}
		if( lpbiOutput->biBitCount == 24 )
		{
			PutImageTable = PutImageTableRGBT;
		}
		else
		{
			PutImageTable = PutImageTableRGBQ;
		}
		break;

#ifdef BI_RGBT
	case BI_RGBT:
		PutImageStride = lpbiOutput->biWidth * sizeof( RGBTRIPLE );
		PutImageTable = PutImageTableRGBT;
		break;

#endif

#ifdef BI_RGBQ
	case BI_RGBQ:
		PutImageStride = lpbiOutput->biWidth * sizeof( RGBQUAD );
		PutImageTable = PutImageTableRGBQ;
		break;
#endif

#ifdef BI_YUY2
	case BI_YUY2:
		PutImageStride = lpbiOutput->biWidth * 2;
		PutImageTable = PutImageTableYUY2;
		break;
#endif

	}
	mode = height == DV_FRAME_HEIGHT_525 ? DV_525_60_SYSTEM : DV_625_50_SYSTEM;
	return( SoftEngineDecodeDV( mode, lpInput, PutImageTable[ CpuType ][ mode & DV_625_50_SYSTEM ? 1 : 0 ] ) );
}

//==============================================================================
//
//  DECOMPRESSOR
//
//==============================================================================
//------------------------------------------------------------------------------
//  ICM_DECOMPRESS_QUERY
//------------------------------------------------------------------------------

LRESULT PASCAL DecompressQuery( PBITMAPINFOHEADER pbiSrc, PBITMAPINFOHEADER pbiDst )
{
	int		height;

	if( pbiSrc == NULL ) return ( ICERR_BADFORMAT );
	if( pbiSrc->biSize < sizeof( BITMAPINFOHEADER ) ) return( ICERR_BADFORMAT );

	if( ( pbiSrc->biCompression & 0xdfdfdfdf ) != CODEC_TYPE )
	{
		if( ( pbiSrc->biCompression | 0x20202020 ) != CODEC_TYPE ) return( ICERR_BADFORMAT );
	}
	if( pbiSrc->biBitCount != 24 ) return( ICERR_BADFORMAT );

	//  check output(target) bitmap format
	if( pbiDst == NULL ) return( ICERR_OK );
	if( pbiDst->biPlanes != 1 ) return( ICERR_BADFORMAT );
	height = pbiDst->biHeight < 0 ? - pbiDst->biHeight : pbiDst->biHeight;
	if( pbiDst->biWidth != pbiSrc->biWidth || height != pbiSrc->biHeight ) return( ICERR_BADFORMAT );

	switch( pbiDst->biCompression )
	{
	case BI_RGB:
#ifdef BI_RGBQ
		if( pbiDst->biBitCount == 32 ) break;
#endif
#ifdef BI_RGBT
		if( pbiDst->biBitCount == 24 ) break;
#endif
		return( ICERR_BADFORMAT );

#ifdef BI_YUY2
	case BI_YUY2:
		if( pbiDst->biBitCount != 16 ) return( ICERR_BADFORMAT );
		break;
#endif

	default:
		return( ICERR_BADFORMAT );
	}
	return( ICERR_OK );
}

//-----------------------------------------------------------------------------
// decompress stream to image
//-----------------------------------------------------------------------------

LRESULT	PASCAL	DecompressEx( PICDECOMPRESSEX lpicd )
{
	PBITMAPINFOHEADER	pbiInput;
	int			err;

	pbiInput = lpicd->lpbiSrc;
	if( lpicd->dwFlags & ICDECOMPRESS_HURRYUP )		// because DV frames are key frames we dont need to do any thing if behind.
	{
		return( ICERR_OK );
	}

	err = SoftCodecDecodeDV( lpicd->lpSrc, lpicd->lpbiDst, lpicd->lpDst );
	return( ICERR_OK );
}

