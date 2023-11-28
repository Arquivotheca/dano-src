//=============================================================================
// Description:
//		DV video stream decoder main routine.
//
//
//
//
// Copyright:
//		Copyright (c) 1997-1998 CANOPUS Co.,Ltd.
//
// History:
//		07/30/97 Tom > Creaded. Don't change originality!
//
//=============================================================================
#include <windows.h>
#include "dv.h"
#include "table.h"

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void PASCAL InverseDCT( int Mode248, PDCT_DATA src, PDCT_DATA dest );

//------------------------------------------------------------------------------
// For decoder
//------------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

//PDIF_BLOCK	pDif;
DEC_PARAM	DecParam[ 5 ][ 6 ];
extern ENC_PARAM	EncParam[ 5 ][ 6 ];

//-----------------------------------------------------------------------------
// Description:                                                  Tom
// Input:
// Output:
//-----------------------------------------------------------------------------

void PASCAL GetStream( PDEC_PARAM pDecParam )
{
	DWORD	t;

	if( pDecParam->StreamCount >= 16 ) return;
	if( pDecParam->ReadAddress == pDecParam->BottomAddress ) return;

	t  = ( DWORD )*pDecParam->ReadAddress++ << 24;
	t |= ( DWORD )*pDecParam->ReadAddress++ << 16;
	pDecParam->StreamBits |= t >> pDecParam->StreamCount;
	pDecParam->StreamCount += 16;
}

//-----------------------------------------------------------------------------
// Description:                                                  			Tom
//		Decodde pass 1 processing
//
// Input:
//
// Output:
//
//-----------------------------------------------------------------------------

int PASCAL DecodePass1( PDIF_BLOCK	pDif,PDEC_PARAM pDec, PQUANT_DATA *ppQTable )
{
	int	DC0;
	int	run, amp, len, bit, pos;
	int	CompleteBlock;

	for( CompleteBlock = 0 ; ; )
	{
		for( run = 0 ; run < 64 ; run++ ) pDec->DctBuffer[ run ] = 0;
		pDec->ReadAddress = &pDif->ID0 + pDec->ByteTop;
		pDec->BottomAddress = pDec->ReadAddress + pDec->BlockSize;
		DC0 = ( ( ( int )( signed char )( *pDec->ReadAddress ) ) << 8 ) + *( pDec->ReadAddress + 1 );
		pDec->DCTMode = ( DC0 << 1 ) & 0x80;		// DC0's bit 6 is DCT mode flag. And make 0x80 or 0
		pDec->Class = ( DC0 >> 4 ) & 3;				// we don't need Class.
		pDec->pQTable = ppQTable[ ( DC0 >> 4 ) & 7 ];
		pDec->pZigZagTable = pDec->DCTMode ? ZigZag248 : ZigZag88;
		pDec->DctBuffer[ 0 ] = ( ( DC0 >> 7 ) << 2 ) + pDec->DcOffset;	// DC coeff must be muled by 4
		pDec->ReadAddress += 2;
		pDec->StreamBits = DC0 << 28;
		pDec->StreamCount = 4;
		for( pDec->AcIndex = 1 ; ; )
		{
			GetStream( pDec );						// printf( "[%x,%x]", wreg.b.h, wreg.b.l );

			bit = ( pDec->StreamBits >> ( 32 - DEC_VLC_LUP1_BIT ) ) & DEC_VLC_LUP1_MASK;
			run = DecVLCTable[ bit ].Run;
			amp = DecVLCTable[ bit ].Amp;
			len = DecVLCTable[ bit ].Length;
			if( run < 0 )
			{
				bit = amp + ( ( pDec->StreamBits >> ( 32 - DEC_VLC_LUP1_BIT - DEC_VLC_LUP2_BIT ) ) & DEC_VLC_LUP2_MASK );
				run = DecVLCTable[ bit ].Run;
				amp = DecVLCTable[ bit ].Amp;
				len = DecVLCTable[ bit ].Length;
			}
			if( pDec->StreamCount < len )
			{
				pDec->Completed = 0;
				pDec->DataEmpty = 1;
				break;
			}
			pDec->StreamCount -= len;
			pDec->StreamBits <<= len;

			if( run == 64 )
			{
				pDec->Completed = 1;
				pDec->DataEmpty = 0;
				CompleteBlock++;
				if( ! pDec->StreamCount )
				{
					if( pDec->ReadAddress == pDec->BottomAddress )
					{
						pDec->DataEmpty = 1;
					}
				}
				break;
			}
			pDec->AcIndex += run;
			if( pDec->AcIndex >= 64 ) pDec->AcIndex = 63;
			pos = pDec->pZigZagTable[ pDec->AcIndex ];
			pDec->DctBuffer[ pos ] = ( ( amp * pDec->pQTable[ pDec->AcIndex ] ) >> 16 );
			pDec->AcIndex++;
		}
		if( pDec->EndOfBlock ) return( CompleteBlock );
		pDec++;
	}
}

//-----------------------------------------------------------------------------
// Description:                                                  			Tom
//		Decode pass 2 and 3 processing
//
// Input:
//
// Output:
//
//-----------------------------------------------------------------------------

int PASCAL DecodePass23( PDEC_PARAM pDecParam )
{
	PDEC_PARAM	pDec;
	PDEC_PARAM	pData;
	int		CompleteBlock;
	int		run, amp, len, bit, pos;

	CompleteBlock = 0;
	pDec = pDecParam;
	pData = pDecParam;

	for(  ; ; pData++ )
	{
		if( ! pData->DataEmpty ) break;
		if( pData->EndOfBlock ) return( CompleteBlock );
	}

LoopNotFinishBlock:
	if( ! pDec->Completed )
	{

	LOOP1:
		if( ( pData->StreamCount + pDec->StreamCount ) > 32 )
		{
			pData->ReadAddress -= 2;
			pData->StreamCount -= 16;
			pData->StreamBits &= ~( ( DWORD )0xffffffff >> pData->StreamCount );
		}
		pData->StreamBits >>= pDec->StreamCount;
		pData->StreamBits |= pDec->StreamBits;
		pData->StreamCount += pDec->StreamCount;

		for( ; ; )
		{
			GetStream( pData );

			bit = ( pData->StreamBits >> ( 32 - DEC_VLC_LUP1_BIT ) ) & DEC_VLC_LUP1_MASK;
			run = DecVLCTable[ bit ].Run;
			amp = DecVLCTable[ bit ].Amp;
			len = DecVLCTable[ bit ].Length;
			if( run < 0 )
			{
				bit = amp + ( ( pData->StreamBits >> ( 32 - DEC_VLC_LUP1_BIT - DEC_VLC_LUP2_BIT ) ) & DEC_VLC_LUP2_MASK );
				run = DecVLCTable[ bit ].Run;
				amp = DecVLCTable[ bit ].Amp;
				len = DecVLCTable[ bit ].Length;
			}
			if( pData->StreamCount < len )		// 読み取ったデータがオーバーランしていたら
			{
				pDec->StreamBits = pData->StreamBits;
				pDec->StreamCount = pData->StreamCount;
				pData->DataEmpty = 1;
				for( ; ; )
				{
					if( pData->EndOfBlock ) return( CompleteBlock );
					pData++;
 					if( ! pData->DataEmpty ) break;
				}
				goto LOOP1;
			}
			pData->StreamCount -= len;
			pData->StreamBits <<= len;

			if( run == 64 )
			{
				break;
			}
			pDec->AcIndex += run;
			if( pDec->AcIndex >= 64 ) pDec->AcIndex = 63;
			pos = pDec->pZigZagTable[ pDec->AcIndex ];
			pDec->DctBuffer[ pos ] = ( ( amp * pDec->pQTable[ pDec->AcIndex ] ) >> 16 );
			pDec->AcIndex++;
		}
		pDec->Completed = 1;		// すべてのＤＣＴ係数のデコードが終了した
		CompleteBlock++;

		if( pData->StreamCount == 0 )
		{
			if( pData->ReadAddress == pData->BottomAddress )
			{
				pData->DataEmpty = 1;
				for( ; ; )
				{
					if( pData->EndOfBlock ) return( CompleteBlock );
					pData++;
 					if( ! pData->DataEmpty ) break;
				}
			}
		}
	}
	if( pDec->EndOfBlock )
	{
		return( CompleteBlock );
	}
	pDec++;
	goto LoopNotFinishBlock;
}

//------------------------------------------------------------------------------
// decode video section
//------------------------------------------------------------------------------

int PASCAL DecodeVideoBlock( PDIF_BLOCK	pDif,PDEC_PARAM pDecParam )
{
	int	PassFlag;

	PassFlag = DecodePass1( pDif,pDecParam, DecodeQTable[ pDif->video.STA_QNO & 15 ] );
	if( PassFlag != 6 )
	{
		PassFlag += DecodePass23( pDecParam );
	}
	pDif++;
	return( PassFlag );
}

//-----------------------------------------------------------------------------
// Description:
//		07/22/98 Tom modified to support alternate scan(switch even/odd line).
//		and remove dct pattern support
// Input:
//
// Output:
//-----------------------------------------------------------------------------

static int pYuv525x[6]={0,0,0,0,8,8};
static int pYuv525y[6]={0,8,16,24,0,8};

static int pYuv625x[6]={0,0,8,8,0,0};
static int pYuv625y[6]={0,8,0,8,16,24};

void PASCAL BlockDCT525( PDEC_PARAM pd,DCT_DATA YuvBuffer[8*2][DCT_BUFFER_STRIDE])
{
	int	i;
	for( i = 0 ; i < 6 ; i++, pd++ ) InverseDCT( pd->DCTMode, pd->DctBuffer, &YuvBuffer[pYuv525x[i]][pYuv525y[i]]);
}

//-----------------------------------------------------------------------------
// Description:
//		07/22/98 Tom modified to support alternate scan(switch even/odd line).
//		and remove dct pattern support
// Input:
//
// Output:
//-----------------------------------------------------------------------------

void PASCAL BlockDCT625( PDEC_PARAM pd,DCT_DATA YuvBuffer[8*2][DCT_BUFFER_STRIDE])
{
	int	i;

	for( i = 0 ; i < 6 ; i++, pd++ ) InverseDCT( pd->DCTMode, pd->DctBuffer, &YuvBuffer[pYuv625x[i]][pYuv625y[i]]);
}

//------------------------------------------------------------------------------
// main entry for DV decoder.
//------------------------------------------------------------------------------

int	EXPORT SoftEngineDecodeDV( int mode,
	PBYTE pStream,
	PBYTE pImage,
	int ImageStride,
	int ImageSize,
	PVOID PutImage )
{
	int		vo, vi, m;
	int		a_ypos, b_ypos, c_ypos, d_ypos, e_ypos;
	int		CompletedBlockNumber;
	int		DIFSequenceNumber;
	int		NumberOfDIFSequence = 10;	// 10:NTSC, 12:PAL
	void 	(PASCAL *PutImage525)( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC );
	void    (PASCAL *PutImage625)( PBYTE pImage, int Stride, PDCT_DATA pY ); 
	PVOID	pParam;
	PDIF_BLOCK	pDif = (PDIF_BLOCK)pStream;
	DCT_DATA YuvBuffer[8*2][DCT_BUFFER_STRIDE];
	if( mode & DV_625_50_SYSTEM )
	{
		NumberOfDIFSequence = 12;	// 625-50 System
		PutImage625 = PutImage;
	}
	else
	{
		NumberOfDIFSequence = 10;	// 525-60 System
		PutImage525 = PutImage;
	}

	for( DIFSequenceNumber = 0 ; DIFSequenceNumber < NumberOfDIFSequence ; DIFSequenceNumber++ )
	{
		pDif += 6;
		a_ypos = ( ( DIFSequenceNumber + 2 ) % NumberOfDIFSequence ) * 6 * 8;
		b_ypos = ( ( DIFSequenceNumber + 6 ) % NumberOfDIFSequence ) * 6 * 8;
		c_ypos = ( ( DIFSequenceNumber + 8 ) % NumberOfDIFSequence ) * 6 * 8;
		d_ypos = ( ( DIFSequenceNumber + 0 ) % NumberOfDIFSequence ) * 6 * 8;
		e_ypos = ( ( DIFSequenceNumber + 4 ) % NumberOfDIFSequence ) * 6 * 8;

		for( m = 0, vo = 0 ; vo < 9 ; vo++ )
		{
			pDif++;
			for( vi = 0 ; vi < 3 ; vi++, m++ )
			{
				DecParam[ 0 ][ 5 ].EndOfBlock = 1;
				DecParam[ 1 ][ 5 ].EndOfBlock = 1;
				DecParam[ 2 ][ 5 ].EndOfBlock = 1;
				DecParam[ 3 ][ 5 ].EndOfBlock = 1;

				CompletedBlockNumber  = DecodeVideoBlock( pDif,DecParam[ 0 ] );pDif++;
				CompletedBlockNumber += DecodeVideoBlock( pDif,DecParam[ 1 ] );pDif++;
				CompletedBlockNumber += DecodeVideoBlock( pDif,DecParam[ 2 ] );pDif++;
				CompletedBlockNumber += DecodeVideoBlock( pDif,DecParam[ 3 ] );pDif++;
				CompletedBlockNumber += DecodeVideoBlock( pDif,DecParam[ 4 ] );pDif++;

				DecParam[ 0 ][ 5 ].EndOfBlock = 0;
				DecParam[ 1 ][ 5 ].EndOfBlock = 0;
				DecParam[ 2 ][ 5 ].EndOfBlock = 0;
				DecParam[ 3 ][ 5 ].EndOfBlock = 0;

				if( CompletedBlockNumber != 30 ) DecodePass23( DecParam[ 0 ] );

				if( NumberOfDIFSequence == 10 )			// NTSC
				{
					unsigned char *pDestBuffer;
					BlockDCT525( DecParam[ 0 ],YuvBuffer);
					pDestBuffer = pImage + (a_ypos+MBPosition525[2][m].y) * ImageStride + (MBPosition525[2][m].x) * ImageSize;
					PutImage525( pDestBuffer,ImageStride, 32, 8,YuvBuffer[ 0 ], YuvBuffer[ 8 ]);

					BlockDCT525( DecParam[ 1 ],YuvBuffer);
					pDestBuffer = pImage + (b_ypos+MBPosition525[1][m].y) * ImageStride + (MBPosition525[1][m].x) * ImageSize;
					PutImage525( pDestBuffer,ImageStride, 32, 8,YuvBuffer[ 0 ], YuvBuffer[ 8 ]);

					BlockDCT525( DecParam[ 2 ],YuvBuffer);
					pDestBuffer = pImage + (c_ypos+MBPosition525[3][m].y) * ImageStride + (MBPosition525[3][m].x) * ImageSize;
					PutImage525( pDestBuffer,ImageStride, 32, 8,YuvBuffer[ 0 ], YuvBuffer[ 8 ]);

					BlockDCT525( DecParam[ 3 ],YuvBuffer);
					pDestBuffer = pImage + (d_ypos+MBPosition525[0][m].y) * ImageStride + (MBPosition525[0][m].x) * ImageSize;
					PutImage525( pDestBuffer,ImageStride, 32, 8,YuvBuffer[ 0 ], YuvBuffer[ 8 ]);

					BlockDCT525( DecParam[ 4 ],YuvBuffer);
					if( m < 24 )
					{
						pDestBuffer = pImage + (e_ypos+MBPosition525[4][m].y) * ImageStride + (MBPosition525[4][m].x) * ImageSize;
						PutImage525( pDestBuffer,ImageStride, 32, 8,YuvBuffer[ 0 ], YuvBuffer[ 8 ]);
					}
					else
					{
						pDestBuffer = pImage + (e_ypos+MBPosition525[4][m].y) * ImageStride + (MBPosition525[4][m].x) * ImageSize;
						PutImage525( pDestBuffer,ImageStride, 16, 8,YuvBuffer[ 0 ], YuvBuffer[ 8 ]);
						pDestBuffer = pImage + (e_ypos+MBPosition525[4][m].y+8) * ImageStride + (MBPosition525[4][m].x) * ImageSize;
						PutImage525( pDestBuffer,ImageStride, 16, 8,YuvBuffer[ 0 ]+16, YuvBuffer[ 8 ]+4);
				    }
				}
				else
				{
					unsigned char *pDestBuffer;
					pDestBuffer = pImage + (a_ypos + MBPosition625[ m ].y) * ImageStride + (16 * 9 * 2 + MBPosition625[ m ].x) * ImageSize;
					BlockDCT625( DecParam[ 0 ],YuvBuffer);
					PutImage625(pDestBuffer,ImageStride,YuvBuffer[0]);

					BlockDCT625( DecParam[ 1 ],YuvBuffer);
					pDestBuffer = pImage + (b_ypos + MBPosition625[ m ].y) * ImageStride + (16 * 9 * 1 + MBPosition625[ m ].x) * ImageSize;
					PutImage625(pDestBuffer,ImageStride,YuvBuffer[0]);

					BlockDCT625( DecParam[ 2 ],YuvBuffer);
					pDestBuffer = pImage + (c_ypos + MBPosition625[ m ].y) * ImageStride + (16 * 9 * 3 + MBPosition625[ m ].x) * ImageSize;
					PutImage625(pDestBuffer,ImageStride,YuvBuffer[0]);

					BlockDCT625( DecParam[ 3 ],YuvBuffer);
					pDestBuffer = pImage + (d_ypos + MBPosition625[ m ].y) * ImageStride + (16 * 9 * 0 + MBPosition625[ m ].x) * ImageSize;
					PutImage625(pDestBuffer,ImageStride,YuvBuffer[0]);

					BlockDCT625( DecParam[ 4 ],YuvBuffer);
					pDestBuffer = pImage + (e_ypos + MBPosition625[ m ].y) * ImageStride + (16 * 9 * 4 + MBPosition625[ m ].x) * ImageSize;
					PutImage625(pDestBuffer,ImageStride,YuvBuffer[0]);
				}
			}
		}
	}
	return( DV_NO_ERROR );
}

//------------------------------------------------------------------------------
// initialize
//------------------------------------------------------------------------------

void	InitDecParam( DEC_PARAM pd[5][6] )
{
	int	i, l;

	for( i = 0 ; i < 5 ; i++ )
	{
		pd[ i ][ 0 ].DcOffset = 1024;
		pd[ i ][ 1 ].DcOffset = 1024;
		pd[ i ][ 2 ].DcOffset = 1024;
		pd[ i ][ 3 ].DcOffset = 1024;
		pd[ i ][ 4 ].DcOffset = 0;
		pd[ i ][ 5 ].DcOffset = 0;

		pd[ i ][ 0 ].ByteTop =  4;
		pd[ i ][ 1 ].ByteTop = 18;
		pd[ i ][ 2 ].ByteTop = 32;
		pd[ i ][ 3 ].ByteTop = 46;
		pd[ i ][ 4 ].ByteTop = 60;
		pd[ i ][ 5 ].ByteTop = 70;

		pd[ i ][ 0 ].BlockSize = 14;
		pd[ i ][ 1 ].BlockSize = 14;
		pd[ i ][ 2 ].BlockSize = 14;
		pd[ i ][ 3 ].BlockSize = 14;
		pd[ i ][ 4 ].BlockSize = 10;
		pd[ i ][ 5 ].BlockSize = 10;

		for( l = 0 ; l < 6 ; l++ )
		{
			pd[ i ][ l ].EndOfBlock = 0;
		}
	}
	pd[ 4 ][ 5 ].EndOfBlock = 1;
}

void PASCAL SetupCodec( void )
{
	InitDecParam(DecParam);
	InitEncParam(EncParam);
}
