//=============================================================================
// Description:
//		Software DV codec.
//		encoder main routine.
//
//
//
// Copyright:
//		Copyright (c) 1998 CANOPUS Co.,Ltd.
//
// History:
//		1996 Tom > revised.
//
//=============================================================================
#include <windows.h>
#include <vfw.h>
#include "codec.h"
#include "csedv.h"
#include "getimage.h"

//-----------------------------------------------------------------------------
// Supported FourCC definitions
//-----------------------------------------------------------------------------

#define	BI_RGBT		mmioFOURCC( 'R', 'G', 'B', 'T' )	// 09/09/98 Tom > added. this is 24bits RGB top-down image.
#define	BI_RGBQ		mmioFOURCC( 'R', 'G', 'B', 'Q' )	// 07/30/98 Tom > added. this is 32bits RGB top-down image.
#define	BI_YUY2		mmioFOURCC( 'Y', 'U', 'Y', '2' )

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

int		GetImageStride;
PBYTE	GetImageBuffer;

//------------------------------------------------------------------------------
// Encode DV frame
//------------------------------------------------------------------------------

int	PASCAL SoftCodecEncodeDV( PBITMAPINFOHEADER lpbiInput, PBYTE lpInput, PBYTE lpOutput )
{
	static PVOID GetImageTableRGBT[] = { GetImage525_RGBT, GetImage625_RGBT };
	static PVOID GetImageTableRGBQ[] = { GetImage525_RGBQ, GetImage625_RGBQ };
	static PVOID GetImageTableYUY2[] = { GetImage525_YUY2, GetImage625_YUY2 };
	int		height;
	PVOID	*GetImageTable;

	GetImageBuffer = lpInput;
	height = lpbiInput->biHeight;			// 注意! アウトプット側の高さは負がある
	if( height < 0 ) height = - height;

	switch( lpbiInput->biCompression )
	{
	case BI_RGB:
		GetImageStride = ( lpbiInput->biWidth * ( lpbiInput->biBitCount / 8 ) + 3 ) & ( ~3 );

		if( lpbiInput->biHeight > 0 )
		{
			GetImageBuffer += ( height - 1 ) * GetImageStride;
			GetImageStride = - GetImageStride;
		}
		if( lpbiInput->biBitCount == 24 )
		{
			GetImageTable = GetImageTableRGBT;
		}
		else
		{
			GetImageTable = GetImageTableRGBQ;
		}
		break;

#ifdef BI_RGBT
	case BI_RGBT:
		GetImageStride = lpbiInput->biWidth * sizeof( RGBTRIPLE );
		GetImageTable = GetImageTableRGBT;
		break;
#endif

#ifdef BI_RGBQ
	case BI_RGBQ:
		GetImageStride = lpbiInput->biWidth * sizeof( RGBQUAD );
		GetImageTable = GetImageTableRGBQ;
		break;
#endif

#ifdef BI_YUY2
	case BI_YUY2:
		GetImageStride = lpbiInput->biWidth * 2;
		GetImageTable = GetImageTableYUY2;
		break;
#endif

	}

	SoftEngineEncodeDV( height == DV_FRAME_HEIGHT_525 ? DV_525_60_SYSTEM : DV_625_50_SYSTEM, lpOutput,
		GetImageTable[ height == DV_FRAME_HEIGHT_625 ? 1 : 0 ] );
	return( height == DV_FRAME_HEIGHT_525 ? 120000 : 144000 );
}

//==============================================================================
//
//  COMPRESSOR
//
//==============================================================================

int	gdwQuality;

//----------------------------------------------------------------------------
//  引数（悪：0～10000：良）
//----------------------------------------------------------------------------

LRESULT	PASCAL CompressGetDefaultQuality( PDWORD lpdwQuality )
{
	*lpdwQuality = QUALITY_DEFAULT;
	return ICERR_OK;
}

//----------------------------------------------------------------------------
//  Entry:
//	dwQuality	画質（0～10000）
//----------------------------------------------------------------------------

LRESULT	PASCAL CompressSetQuality( DWORD dwQuality )
{
	gdwQuality = max( 0, min( 10000, dwQuality ) );
	return ICERR_OK;
}

//----------------------------------------------------------------------------
//  Entry:
//	lpdwQuality	LPDWORD	画質（0～10000）
//  Exit:
//	ICERR_OK		成功。
//	ICERR_UNSUPPORTED	不可。
//----------------------------------------------------------------------------

LRESULT	PASCAL CompressGetQuality( LPDWORD lpdwQuality )
{
	*lpdwQuality = gdwQuality;
	return ICERR_OK;
}

//------------------------------------------------------------------------------
// CompressQuery() handles the ICM_COMPRESSQUERY message.
// This message basically asks, "Can you compress this into this?"
// We look at the input and output bitmap info headers and determine if we can.
//------------------------------------------------------------------------------

LRESULT	PASCAL CompressQuery( PBITMAPINFOHEADER pbiSrc, PBITMAPINFOHEADER pbiDst )
{
	if( pbiSrc == NULL ) return ICERR_BADFORMAT;

	switch( pbiSrc->biCompression )		// determine if the input DIB data is in a format we like.
	{
	case BI_RGB:

#ifdef BI_RGBT
		if( pbiSrc->biBitCount == 24 ) break;
#endif

#ifdef BI_RGBQ
		if( pbiSrc->biBitCount == 32 ) break;
#endif
		return( ICERR_BADFORMAT );

#ifdef BI_YUY2
	case BI_YUY2:
		if( pbiSrc->biBitCount == 16 ) break;
		return( ICERR_BADFORMAT );
#endif

	default:
		return( ICERR_BADFORMAT );
	}

	if( pbiDst == NULL ) return ICERR_OK;											// are we being asked to query just the input format?
	if( pbiDst->biCompression != CODEC_TYPE ) return ICERR_BADFORMAT;				// make sure we can handle the format to compress to also.
	if( pbiDst->biBitCount != 24 ) return ICERR_BADFORMAT;
	if( pbiDst->biWidth != DV_FRAME_WIDTH ) return ICERR_BADFORMAT;								// width must be 720
	if( pbiDst->biHeight != DV_FRAME_HEIGHT_525 && pbiDst->biHeight != DV_FRAME_HEIGHT_625 ) return ICERR_BADFORMAT;	// height must be 480 or 576
	if( pbiDst->biWidth != pbiSrc->biWidth || pbiDst->biHeight != pbiSrc->biHeight ) return ICERR_BADFORMAT;			// must not stretch
	return ICERR_OK;
}

//----------------------------------------------------------------------------
//  圧縮されたフレームのサイズ.
//  CompressGetSize() implements ICM_COMPRESS_GET_SIZE.
//  This function returns how much (upper bound) memory a compressed frame will take.
//----------------------------------------------------------------------------

LRESULT	PASCAL CompressGetSize( PBITMAPINFOHEADER pbiSrc, PBITMAPINFOHEADER pbiDst )
{
	if( pbiSrc->biHeight == DV_FRAME_HEIGHT_525 )		// 525-60システム
	{
		return( DIF_SEQUENCE_SIZE * 10 );
	}
	else					// 625-50 system.
	{
		return( DIF_SEQUENCE_SIZE * 12 );
	}
}

//------------------------------------------------------------------------------
//  CompressGetFormat() implements ICM_GETFORMAT.
//  This message asks, "If I gave you this bitmap, how much memory would it be compressed?"
//  If the output bitmap info header is NULL, we just return how big
//  the header would be (header + palette, actually)
//
//  Otherwise, we fill in the header, most importantly the biSizeImage.
//  This field must contain an upper bound on the size of the
//  compressed frame. A value that is too high here will result in
//  inefficient memory allocation at compression time, but will not be
//  reflected to the stored bitmap - the compression algorithm may
//  chop biSizeImage down to the actual amount with no ill effects.
//
//  現在サポートしているのは、１６ビットＲＧＢの入力に対して、２４ビットＪＰＥＧの出力のみ。
//------------------------------------------------------------------------------

LRESULT	PASCAL CompressGetFormat( PBITMAPINFOHEADER pbiSrc, PBITMAPINFOHEADER pbiDst )
{
	LRESULT		dw;

	if( ( dw = CompressQuery( pbiSrc, NULL ) ) != ICERR_OK ) return dw;
	if( pbiDst == NULL ) return( sizeof( BITMAPINFOHEADER ) );	// pbiDstがNULLならｱｳﾄﾌﾟｯﾄﾌｫｰﾏｯﾄのｻｲｽﾞを返す

	pbiDst->biSize			= sizeof( BITMAPINFOHEADER );
	pbiDst->biWidth         = pbiSrc->biWidth;
	pbiDst->biHeight        = pbiSrc->biHeight;
	pbiDst->biPlanes		= 1;
	pbiDst->biBitCount		= 24;
	pbiDst->biCompression	= CODEC_TYPE;
	pbiDst->biXPelsPerMeter	= 0;
	pbiDst->biYPelsPerMeter	= 0;
	pbiDst->biClrUsed		= 0;
	pbiDst->biClrImportant	= 0;
	pbiDst->biSizeImage     = CompressGetSize( pbiSrc, pbiDst );
	return ICERR_OK;
}

//------------------------------------------------------------------------------
//  Compress() implements ICM_COMPRESS.
//  Everything is set up; call the actual compression routine.
//
//  Note:
//  1)	We set the ckid in icinfo to a two-character code indicating
//	how we compressed. This code will be returned to us at decom-
//	press time to allow us to pick a decompression algorithm to
//	match. This is different from icinfo->fccHandler, which tells
//	which driver to use!
//
//  2)	We set the key-frame flag on every frame since we do no tempo-
//	ral (inter-frame) compression.
//------------------------------------------------------------------------------

LRESULT	PASCAL Compress( ICCOMPRESS *icinfo )
{
	if( icinfo->dwQuality == ICQUALITY_DEFAULT ) icinfo->dwQuality = QUALITY_DEFAULT;

	icinfo->lpbiOutput->biSizeImage = SoftCodecEncodeDV( icinfo->lpbiInput, icinfo->lpInput, icinfo->lpOutput );

	if( icinfo->lpckid ) *icinfo->lpckid = aviTWOCC( 'd', 'c' );	// return the chunk id
	if( icinfo->lpdwFlags )							// set the AVI index flags,
	{
		*icinfo->lpdwFlags |= AVIIF_KEYFRAME;		// make it a keyframe
	}
	return ICERR_OK;
}

