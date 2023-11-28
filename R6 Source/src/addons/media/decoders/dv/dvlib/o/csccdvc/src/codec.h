//=============================================================================
// Description:
//
//
//
//
//
// Copyright:
//		Copyright (c) 1999 Canopus Co.,Ltd. All Rights Reserved.
//		Developed in San Jose, CA, U.S.A.
// History:
//		97/7  Tom start to develop
//
//=============================================================================

#define CODEC_TYPE		mmioFOURCC( 'C', 'D', 'V', 'C' )	// DV codec DIB biCompression.

//-----------------------------------------------------------------------------
// for short typing some type is defined, but not completed
//-----------------------------------------------------------------------------

#define	DECLARE_POINTER(type)	typedef type *P##type
DECLARE_POINTER( ICOPEN );
DECLARE_POINTER( ICDECOMPRESS );
DECLARE_POINTER( ICDECOMPRESSEX );

//-----------------------------------------------------------------------------
// Don't need this definition. Because DV stream can not control quality.
//-----------------------------------------------------------------------------

#define	QUALITY_DEFAULT	7000

//------------------------------------------------------------------------------
// Externals
//------------------------------------------------------------------------------

LRESULT	PASCAL	DecompressQuery( PBITMAPINFOHEADER lpbiSrc, LPBITMAPINFOHEADER lpbiDst );
LRESULT	PASCAL	Decompress( PICDECOMPRESS lpicd );
LRESULT	PASCAL	DecompressEx( PICDECOMPRESSEX lpicd );
LRESULT	PASCAL	CompressGetSize( PBITMAPINFOHEADER pbiSrc, PBITMAPINFOHEADER pbiDst );
LRESULT	PASCAL	CompressQuery( PBITMAPINFOHEADER pbiSrc, PBITMAPINFOHEADER pbiDst );
LRESULT	PASCAL	CompressGetFormat( PBITMAPINFOHEADER pbiSrc, PBITMAPINFOHEADER pbiDst );
LRESULT	PASCAL	Compress( ICCOMPRESS *icinfo );
LRESULT	PASCAL	CompressGetDefaultQuality( PDWORD lpdwQuality );
LRESULT	PASCAL	CompressSetQuality( DWORD dwQuality );
LRESULT	PASCAL	CompressGetQuality( LPDWORD lpdwQuality );
BOOL	PASCAL	QueryAbout( void );
void	PASCAL	About( HWND hwnd );
BOOL	PASCAL	QueryConfigure( void );
LRESULT PASCAL	Configure( HWND hwnd );
BOOL	PASCAL	DrawQuery( LPBITMAPINFOHEADER lpbiInput );
LRESULT	PASCAL	DrawBegin( ICDRAWBEGIN FAR *icinfo, PDWORD pSurfaceTable );
LRESULT	PASCAL	Draw( ICDRAW *icinfo, DWORD dwDriverID, HDRVR hDriver, PVOID DriverProc );
LRESULT	PASCAL	DrawEnd( void );
LRESULT	PASCAL	DrawWindow( LPRECT lprc );
void	PASCAL	DrawOpen( void );
void	PASCAL	DrawClose( void );


extern	int		CpuType;
extern	HMODULE	ghModule;


