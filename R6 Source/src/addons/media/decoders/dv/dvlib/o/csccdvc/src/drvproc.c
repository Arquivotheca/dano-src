//=============================================================================
// Description:
//		Main entry routine for codec driver
//
//
//
//
// Copyright:
//		Copyright (c) 1999 Canopus Co.,Ltd. All Rights Reserved.
//		Developed in San Jose, CA, U.S.A.
// History:
//		01/28/99 Tom > Creaded.
//
//=============================================================================
#include <windows.h>
#include <vfw.h>
#include "codec.h"
#include "csedv.h"
#include "cputype.h"

#define	BOGUS_DRIVER_ID		0xffffffff	// invalid pointer to use bogus ID

//------------------------------------------------------------------------------
// Globals
//------------------------------------------------------------------------------

HMODULE	ghModule;
int		CpuType;

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

LRESULT	PASCAL OnDrvOpen( PICOPEN lpico )
{
	if( lpico->dwSize < sizeof( *lpico ) ) return( ( LRESULT )NULL );
	if( lpico->fccType != ICTYPE_VIDEO ) return( ( LRESULT )NULL );
	if( lpico->dwFlags & ICMODE_DRAW ) return( ( LRESULT )NULL );		// this is NOT rendering driver
	if( ( lpico->dwFlags & ( ICMODE_DECOMPRESS | ICMODE_COMPRESS ) ) == 0 ) return( ( LRESULT)NULL );	// we are only codec
	lpico->dwError = ICERR_OK;
	return( ( LRESULT )&ghModule );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

LRESULT PASCAL OnDrvClose( void )
{
	return( TRUE );
}

//==============================================================================
// DECOMPRESS routines
//==============================================================================
//------------------------------------------------------------------------------
// ICM_DECOMPRESS_BEGIN
//------------------------------------------------------------------------------

LRESULT PASCAL DecompressBegin( PBITMAPINFOHEADER lpbiSrc, PBITMAPINFOHEADER lpbiDst )
{
	LRESULT	lrc;
	if( lrc = DecompressQuery( lpbiSrc, lpbiDst ) ) return( lrc );
	return( ICERR_OK );
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

LRESULT PASCAL DecompressGetFormat( PBITMAPINFOHEADER pbiSrc, PBITMAPINFOHEADER pbiDst )
{
	LRESULT	lrc;

	if( lrc = DecompressQuery( pbiSrc, NULL ) ) return( lrc );
	if( pbiDst == NULL ) return( sizeof( BITMAPINFOHEADER ) );	// if pbiDst == NULL then, return the size required to hold a output format
	pbiDst->biSize = sizeof( BITMAPINFOHEADER );
	pbiDst->biWidth = pbiSrc->biWidth;
	pbiDst->biHeight = pbiSrc->biHeight;
	pbiDst->biPlanes = 1;
	pbiDst->biBitCount = 24;
	pbiDst->biCompression = BI_RGB;
	pbiDst->biSizeImage = ( ( ( pbiDst->biWidth * 24 + 31 ) & ( ~31 ) ) >> 3 ) * pbiDst->biHeight;
	pbiDst->biXPelsPerMeter = 0;
	pbiDst->biYPelsPerMeter = 0;
	pbiDst->biClrUsed = 0;
	pbiDst->biClrImportant = 0;
	return( ICERR_OK );
}

//------------------------------------------------------------------------------
// ICM_DECOMPRESS_END
//------------------------------------------------------------------------------

LRESULT PASCAL DecompressEnd( void )
{
	return( ICERR_OK );
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

DWORD PASCAL GetInfo( ICINFO *icinfo, DWORD dwSize )
{
	#define	NUMELMS( aa )	( sizeof( aa ) / sizeof( ( aa )[ 0 ] ) )

	if( icinfo == NULL ) return sizeof( ICINFO );
	if( dwSize < sizeof( ICINFO ) ) return 0;

	icinfo->dwSize = sizeof( ICINFO );
	icinfo->fccType = ICTYPE_VIDEO;
	icinfo->fccHandler = CODEC_TYPE;
	icinfo->dwFlags = 0;

	icinfo->dwVersion = ICVERSION;
	icinfo->dwVersionICM = ICVERSION;			// ICMのバージョン

	MultiByteToWideChar( CP_ACP, 0, "Software DV Codec"        , -1, icinfo->szName       , NUMELMS( icinfo->szName ) );
	MultiByteToWideChar( CP_ACP, 0, "Canopus Software DV Codec", -1, icinfo->szDescription, NUMELMS( icinfo->szDescription ) );
	return sizeof( ICINFO );
}

//----------------------------------------------------------------------------
// CompressBegin() implements ICM_COMPRESSBEGIN.
// We're about to start compressing, initialize coprocessor, etc.
//----------------------------------------------------------------------------

LRESULT PASCAL CompressBegin( PBITMAPINFOHEADER pbiSrc, PBITMAPINFOHEADER pbiDst )
{
	return( CompressQuery( pbiSrc, pbiDst ) );			// 11/05/98 Tom > これをやらないと変なｻｲｽﾞがそのままｴﾝｼﾞﾝにいくことがある
}

//----------------------------------------------------------------------------
// CompressEnd() is called on ICM_COMPRESS_END.
// This function is a chance to flush buffers, deinit hardware, etc.
// after compressing a single frame.
//----------------------------------------------------------------------------

LRESULT PASCAL CompressEnd( void )
{
	return ICERR_OK;
}

//==============================================================================
// DRIVER PROC routines
//==============================================================================

LRESULT PASCAL DriverProc( DWORD dwDriverID, HDRVR hDriver, UINT uiMessage, LPARAM lParam1, LPARAM lParam2 )
{
	switch( uiMessage )
	{

	//----------------------------------------------------------------------
	// standard driver messages
	//----------------------------------------------------------------------

	case DRV_LOAD:
		return( TRUE );

	case DRV_FREE:
		return( TRUE );

	case DRV_OPEN:						// if being opened with no open struct, then return a non-zero value without actually opening
		return( lParam2 ? OnDrvOpen( ( PICOPEN )lParam2 ) : BOGUS_DRIVER_ID );

	case DRV_CLOSE:
		return( dwDriverID != BOGUS_DRIVER_ID ? OnDrvClose() : TRUE );

	case DRV_ENABLE:
		return( 1 );

	case DRV_DISABLE:
		return( 1 );

	case DRV_INSTALL:
		return( ( LRESULT )DRV_OK );

	case DRV_REMOVE:
		return( ( LRESULT )DRV_OK );

	case DRV_QUERYCONFIGURE:
		return( FALSE );

	case DRV_CONFIGURE:
		return( ICERR_OK );

	//----------------------------------------------------------------------
	// state messages (ICMｲﾝﾀｰﾌｪｰｽに関するﾒｯｾｰｼﾞ
	//----------------------------------------------------------------------

	case ICM_CONFIGURE:
		if( lParam1 == -1 ) return( ICERR_UNSUPPORTED );
		return( ICERR_OK );

	case ICM_ABOUT:
		if( lParam1 == -1 ) return( ICERR_UNSUPPORTED );	// 情報ダイアログボックスの有無を問い合わせる。
		return( ICERR_OK );

	case ICM_GETSTATE:
		return ICERR_OK;

	case ICM_SETSTATE:
		return ICERR_OK;

	case ICM_GETINFO:												// This message is set to a video compression driver to have it
		return GetInfo( ( ICINFO * )lParam1, ( DWORD )lParam2 );	// return information describing the driver.

	//----------------------------------------------------------------------
	// Configuration Messages for Compression Quality
	//----------------------------------------------------------------------
	//----------------------------------------------------------------------
	// This message is sent to a video compression driver to request
	// that the driver return its default quality setting.
	// dwParam1 specifies a far pointer to a DWORD used by the driver
	// to return its default quality. dwParam2 is not used.
	//----------------------------------------------------------------------

	case ICM_GETDEFAULTQUALITY:
		if( ! lParam1 ) break;
		return( CompressGetDefaultQuality( ( LPDWORD )lParam1 ) );

	//----------------------------------------------------------------------
	// This message is sent to a video compression driver to set the
	// quality level for compression.
	// dwParam1 specifies the new quality value. dwParam2 is not used.
	//----------------------------------------------------------------------

	case ICM_SETQUALITY:
		return( CompressSetQuality( ( DWORD )lParam1 ) );

	//----------------------------------------------------------------------
	// This message is sent to a video compression driver to request
	// that its current quality setting.
	//
	// dwParam1 specifies a far pointer to a DWORD used by the driver
	// to return the current quality value. dwParam2 is not used.
	//----------------------------------------------------------------------

	case ICM_GETQUALITY:
		return( CompressGetQuality( ( PDWORD )lParam1 ) );

	//----------------------------------------------------------------------
	// compression messages
	//----------------------------------------------------------------------

	case ICM_COMPRESS_QUERY:
		return( CompressQuery( ( LPBITMAPINFOHEADER )lParam1, ( PBITMAPINFOHEADER )lParam2 ) );

	case ICM_COMPRESS_BEGIN:
		return( CompressBegin( ( LPBITMAPINFOHEADER )lParam1, ( PBITMAPINFOHEADER )lParam2 ) );

	case ICM_COMPRESS_GET_FORMAT:
		return( CompressGetFormat( ( LPBITMAPINFOHEADER )lParam1, ( PBITMAPINFOHEADER )lParam2 ) );

	case ICM_COMPRESS_GET_SIZE:
		return( CompressGetSize( ( LPBITMAPINFOHEADER )lParam1, ( PBITMAPINFOHEADER )lParam2 ) );

	case ICM_COMPRESS:
		return( Compress( ( ICCOMPRESS * )lParam1 ) );

	case ICM_COMPRESS_END:
		return( CompressEnd() );

	//----------------------------------------------------------------------
	// decompress messages
	//----------------------------------------------------------------------

	case ICM_DECOMPRESS_QUERY:
		return( DecompressQuery( ( PBITMAPINFOHEADER )lParam1, ( LPBITMAPINFOHEADER )lParam2 ) );

	case ICM_DECOMPRESS_BEGIN:
		return( DecompressBegin( ( PBITMAPINFOHEADER )lParam1, ( PBITMAPINFOHEADER )lParam2 ) );

	case ICM_DECOMPRESS_GET_FORMAT:
		return( DecompressGetFormat( ( PBITMAPINFOHEADER )lParam1, ( LPBITMAPINFOHEADER )lParam2 ) );

	case ICM_DECOMPRESS_GET_PALETTE:
		return( ICERR_UNSUPPORTED );

	case ICM_DECOMPRESSEX_QUERY:
		if( ! lParam1 ) return( ICERR_BADFORMAT );
		return( DecompressQuery( ( ( PICDECOMPRESSEX )lParam1 )->lpbiSrc, ( ( PICDECOMPRESSEX )lParam1 )->lpbiDst ) );

	case ICM_DECOMPRESSEX_BEGIN:
		return( DecompressBegin( ( ( PICDECOMPRESSEX )lParam1 )->lpbiSrc, ( ( PICDECOMPRESSEX )lParam1 )->lpbiDst ) );

	case ICM_DECOMPRESSEX:
	case ICM_DECOMPRESS:
		return( DecompressEx( ( PICDECOMPRESSEX )lParam1 ) );

	case ICM_DECOMPRESSEX_END:
	case ICM_DECOMPRESS_END:
		return( DecompressEnd() );

	}
	return( uiMessage < DRV_USER ? DefDriverProc( dwDriverID, hDriver, uiMessage, lParam1, lParam2 ) : ICERR_UNSUPPORTED );
}

//==============================================================================
// DLL entry
//==============================================================================

BOOL PASCAL DllMain( HINSTANCE hModule, ULONG Reason, LPVOID pv )
{
	switch( Reason )
	{
	case DLL_PROCESS_ATTACH:
		ghModule = hModule;
		DisableThreadLibraryCalls( hModule );
		CpuType = GetCpuType();
		break;

	case DLL_PROCESS_DETACH:
		break;
	}
	return( TRUE );
}


