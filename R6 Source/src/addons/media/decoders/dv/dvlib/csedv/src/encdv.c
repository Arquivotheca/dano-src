//==============================================================================
//  DV video portion decoder main kernel routine.
//
//  Copyright(C) 1991-1997 CANOPUS Co.,Ltd.	<<<<< Don't change copyright!
//  Tom created	 1997. 7.30			<<<<< Don't change originality!
//  Tom revised	 1997
//
//  <!!! WARNING !!!>
//  Don't change original comments. It's the morality for programmer.
//==============================================================================
#include <windows.h>
#include "dv.h"
#include "table.h"

#include "csedv.h"

void	PASCAL ForwardDCT248( PDCT_DATA, PDCT_DATA );
void	PASCAL ForwardDCT88( PDCT_DATA, PDCT_DATA );


//-----------------------------------------------------------------------------
// global valiables
//-----------------------------------------------------------------------------

//ENC_PARAM	EncParam[ 5 ][ 6 ];
//PDIF_BLOCK	pEncDif;
//static int	NumberOfDIFSequence;

//------------------------------------------------------------------------------
// Save headers.
//------------------------------------------------------------------------------

void PASCAL EncodeHeader( PDIF_BLOCK pEncDif,int NumberOfDIFSequence,int Dseq )
{
	int	i;

	pEncDif->ID0 = ( SCT_HEADER << 5 ) | 0x1f;
	pEncDif->ID1 = ( Dseq << 4 ) | 0x7;
	pEncDif->ID2 = 0;
	pEncDif->header.ID3 = 0x3f;
	if( NumberOfDIFSequence == 12 ) pEncDif->header.ID3 |= 0x80;
	pEncDif->header.ID4 = 0x68;
	pEncDif->header.ID5 = 0x78;
	pEncDif->header.ID6 = 0x78;
	pEncDif->header.ID7 = 0x78;
	for( i = 0 ; i < 72 ; i++ ) pEncDif->header.Reserved[ i ] = 0xff;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void PASCAL EncodeSubcode( PDIF_BLOCK pEncDif,int Dseq, int DBN )
{
	pEncDif->ID0 = ( SCT_SUBCODE << 5 ) | 0x1f;	// 3f
	pEncDif->ID1 = ( Dseq << 4 ) | 0x7;			// 07,17,27....
	pEncDif->ID2 = DBN;							// 00,01,02
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void PASCAL EncodeVAUX( PDIF_BLOCK pEncDif,int NumberOfDIFSequence,int Dseq )
{
	PDWORD	pdw;
	int	i;
	PBYTE	pbt;

	pdw = ( PDWORD )pEncDif;				// 0xffffffff Ç≈èâä˙âª
	for( i = sizeof( DIF_BLOCK ) * 3 / 4 ; i ; i-- ) *pdw++ = 0xffffffff;

	if( Dseq & 1 )
	{
		pbt = pEncDif->vaux.data;
	}
	else
	{
		pbt = ( pEncDif + 2 )->vaux.data + 45;
	}
	pbt[ 0 ] = 0x60;

	if( NumberOfDIFSequence == 12 )
	{
		pbt[ 3 ] = 0xe0;
	}
	else
	{
		pbt[ 3 ] = 0xc0;
	}
	pbt[ 5 ] = 0x61;
	pbt[ 6 ] = 0x1f;
	pbt[ 7 ] = 0x80;
	pbt[ 8 ] = 0xfc;

	for( i = 0 ; i < 3 ; i++ )
	{
		pEncDif[i].ID0 = ( SCT_VAUX << 5 ) | 0x1f;	// 5f
		pEncDif[i].ID1 = ( Dseq << 4 ) | 0x7;			// 07,17....
		pEncDif[i].ID2 = i;
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void PASCAL EncodeAudio( PDIF_BLOCK pEncDif,int NumberOfDIFSequence,int Dseq, int DBN )
{
	pEncDif->ID0 = ( SCT_AUDIO << 5 ) | ( RSV << 4 );	// 1110000?
	pEncDif->ID1 = ( Dseq << 4 ) | 0x7;
	pEncDif->ID2 = DBN;

	if( ( ( ( Dseq & 1 ) == 0 ) && ( DBN == 3 ) ) || ( ( ( Dseq & 1 ) == 1 ) && ( DBN == 0 ) ) )
	{
		pEncDif->audio.data[ 0 ] = 0x50;
		pEncDif->audio.data[ 1 ] = 0xc0;
		pEncDif->audio.data[ 2 ] = 0x00;
		if( NumberOfDIFSequence == 12 )			// 12 means PAL.
		{
			pEncDif->audio.data[ 3 ] = 0xe0;
		}
		else
		{
			pEncDif->audio.data[ 3 ] = 0xc0;
		}
		pEncDif->audio.data[ 4 ] = 0xc0;
	}
	else if( ( ( ( Dseq & 1 ) == 0 ) && ( DBN == 4 ) ) || ( ( ( Dseq & 1 ) == 1 ) && ( DBN == 1 ) ) )
	{
		pEncDif->audio.data[ 0 ] = 0x51;
		pEncDif->audio.data[ 1 ] = 0x3f;
		pEncDif->audio.data[ 2 ] = 0xcf;
		pEncDif->audio.data[ 3 ] = 0xa0;
		pEncDif->audio.data[ 4 ] = 0xff;
	}
	pEncDif++;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

#define	THRESHOLD_248		( 152 * 8 )		// Ç±ÇÃílÇ≈ÇŸÇ⁄ê≥ÇµÇ¢ÇÊÇ§ÇæÅB

int	PASCAL JudgeDctType( short src[][ DCT_BUFFER_STRIDE ] )
{
	int	asum, sum, j;

	asum = 0;
	for( j = 0 ; j < 8 ; j++ )
	{
		sum  = src[ 0 ][ j ] - src[ 1 ][ j ];
		sum += src[ 2 ][ j ] - src[ 3 ][ j ];
		sum += src[ 4 ][ j ] - src[ 5 ][ j ];
		sum += src[ 6 ][ j ] - src[ 7 ][ j ];
		if( sum < 0 ) sum = -sum;
		asum += sum;				// Ç±ÇÃââéZÇÃåãâ Ç™ THRESHOLD_248 ÇPÇQÇOÇXÇí¥Ç¶ÇÈÇ∆ÇQÇSÇWÇcÇbÇsÇ™ëIëÇ≥ÇÍÇÈÇÊÇ§ÇæÅB
	}
	return( asum >= THRESHOLD_248 ? TRUE : FALSE );
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int	PASCAL JudgeClass( PENC_PARAM pEncParam, PDCT_DATA pZigZagWeighting, int *pZigZag )
{
	int	max, i, p, run, data;

	pEncParam->ZigZagDct[ 0 ] = ( pEncParam->DctBuffer[ 0 ] + 8 ) >> 4;
	for( max = 0, run = 1, i = 1, p = 1 ; i < 64 ; i++ )
	{
		if( data = ( ( ( int )pEncParam->DctBuffer[ pZigZag[ i ] ] * ( int )pZigZagWeighting[ i ] + 32768 ) >> 16 ) )
		{
			pEncParam->ZigZagDct[ p * 2 + 0 ] = run + ( i << 8 );
			pEncParam->ZigZagDct[ p * 2 + 1 ] = data;
			p++;
			if( data < 0 ) data = -data;
			if( data > max ) max = data;
			run = 1;
		}
		else
		{
			run++;
		}
	}
	pEncParam->ZigZagDct[ p * 2 + 0 ] = 64;
	return( max );
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

int	PASCAL CountBits( PENC_PARAM pEnc, int QNo )
{
	PBYTE	pEncQTable;
	int	i, p, r, run, amp, BitCount, BlockBits;
	BYTE	shift;

	for( BitCount = ( 12 + 4 ) * 6 * 5 ; ; pEnc++ )		// add DC,mode,class,EOB's length.
	{
		pEncQTable = EncodeQTable[ QNo ][ pEnc->Class ];// [ QNo:16 ][ Class:4 ]

		BlockBits = 0;
		for( run = 0, i = 1 ; ; i++ )
		{
			p = pEnc->ZigZagDct[ i * 2 + 0 ];
			r = p & 0xff;
			p >>= 8;
			if( r >= 64 ) break;
			run += r;
			amp = pEnc->ZigZagDct[ i * 2 + 1 ];

			if( shift = pEncQTable[ p ] )			// ïKÇ∏ÇOà»äOÇ©É`ÉFÉbÉNÇ∑ÇÈ
			{
				amp = ( amp + ( 1 << ( shift - 1 ) ) ) >> shift;
				if( ! amp ) continue;
			}
			run--;
			if( run >= 16 )
			{
				if( ( amp < -32 ) || ( amp > 31 ) )
				{
					BlockBits += 13 + 16;
				}
				else
				{
				 	BlockBits += 13 + EncVLCLength[ 0 ][ amp + 32 ];
				}
			}
			else if( ( amp < -32 ) || ( amp > 31 ) )
			{
				if( run == 0 )
				{
					BlockBits += 16;
				}
				else
				{
					BlockBits += EncVLCLength[ run - 1 ][ 0 + 32 ] + 16;
				}
			}
			else
			{
				BlockBits += EncVLCLength[ run ][ amp + 32 ];
			}
			run = 0;
		}
		BitCount += BlockBits;

		if( BitCount > VS_BITS_NUMBER ) break;		// over 3040 bits?
		if( pEnc->EndOfBlock ) break;
	}
	return( BitCount );
}

//------------------------------------------------------------------------------
// FDCT block
//------------------------------------------------------------------------------

void PASCAL FDctBlock( PENC_PARAM pEncParam, PDCT_DATA pYuv[] )
{
	int	Block, AcMax;

	for( Block = 0 ; Block < 6 ; Block++, pEncParam++ )
	{
		if( pEncParam->DCTMode = JudgeDctType( ( PVOID )pYuv[ Block ] ) )
		{
			ForwardDCT248( pYuv[ Block ], pEncParam->DctBuffer );
			AcMax = JudgeClass( pEncParam, ZigZagWeighting248DCT, ZigZag248 );
		}
		else
		{
			ForwardDCT88( pYuv[ Block ], pEncParam->DctBuffer );
			AcMax = JudgeClass( pEncParam, ZigZagWeighting88DCT, ZigZag88 );
		}
		if( Block < 4 )					// same as BlueBook
		{
			if( AcMax < 12 )
				pEncParam->Class = 0;
			else if( AcMax < 24 )
				pEncParam->Class = 1;
			else if( AcMax < 36 )
				pEncParam->Class = 2;
			else
				pEncParam->Class = 3;
		}
		else if( Block == 4 )
		{
			if( AcMax < 12 )
				pEncParam->Class = 1;
			else if( AcMax < 24 )
				pEncParam->Class = 2;
			else
				pEncParam->Class = 3;
		}
		else
		{
			if( AcMax < 12 )
				pEncParam->Class = 2;
			else
				pEncParam->Class = 3;
		}
	}
}

//------------------------------------------------------------------------------
// Reconsideration class number if QNo is zero but amount of data bits
// still over 3040 bits
//------------------------------------------------------------------------------

void PASCAL ReconsiderationClass( ENC_PARAM EncParam[5][6] )
{
	PENC_PARAM pEnc;
	int	WorkFlag;

	for( ; ; )
	{
		for( WorkFlag = 0, pEnc = EncParam[ 0 ] ; ; pEnc++ )
		{
			if( pEnc->Class != 3 )
			{
				pEnc->Class++;
				if( CountBits( EncParam[ 0 ], 0 ) <= VS_BITS_NUMBER ) return;
				WorkFlag = 1;
			}
			if( pEnc->EndOfBlock ) break;
		}
		if( ! WorkFlag ) break;
	}
}

//------------------------------------------------------------------------------
// Decide QNo.
// find QNo value by CountBits return value is less than VS_BITS_NUMBER
//------------------------------------------------------------------------------

int	PASCAL EstimateBits( ENC_PARAM EncParam[5][6] )
{
	int	QNo;

	QNo = 8;

	if( CountBits( EncParam[ 0 ], QNo ) <= VS_BITS_NUMBER )	// less than VS_BITS_NUMBER
	{
		for( ; ; )
		{
			if( QNo == 15 ) break;
			if( CountBits( EncParam[ 0 ], QNo + 1 ) > VS_BITS_NUMBER ) break;
			QNo++;
		}
	}
	else
	{
		for( ; ; )					// bigger. So decriment QNo.
		{
			if( ! QNo )
			{
				ReconsiderationClass(EncParam);		// If QNo already zero, reconsider class value.
				break;
			}
			QNo--;
			if( CountBits( EncParam[ 0 ], QNo ) <= VS_BITS_NUMBER ) break;
		}
	}
	return( QNo );
}


//------------------------------------------------------------------------------
// Put bits data to stream
//
//	:<---  current use --->:<------ free area (put new data) ------>:
//	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//	|   StreamCount	       :        |               |               |
//	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//	MSB                                                          LSB
//------------------------------------------------------------------------------

int	PASCAL PutStream( PENC_PARAM pEncParam, DWORD Codewords, int CodeLength )
{
	pEncParam->StreamBits |= Codewords >> pEncParam->StreamCount;
	pEncParam->StreamCount += CodeLength;

	if( pEncParam->StreamCount < 16 ) return( 0 );

	*pEncParam->WriteAddress++ = ( BYTE )( pEncParam->StreamBits >> 24 );
	*pEncParam->WriteAddress++ = ( BYTE )( pEncParam->StreamBits >> 16 );
	pEncParam->StreamBits <<= 16;
	pEncParam->StreamCount -= 16;

	if( pEncParam->StreamCount < 16 )
	{
		if( pEncParam->WriteAddress == pEncParam->BottomAddress )	// écÇË16à»â∫ÇÃèÍçáÇ‡ ﬁØÃßÇ™ÃŸÇ…Ç»Ç¡ÇΩÇ©¡™Ø∏ÇñYÇÍÇ»Ç¢Ç±Ç∆
		{
			pEncParam->StreamFilled = 1;
			return( 1 );
		}
		return( 0 );
	}

	pEncParam->StreamBits |= Codewords << ( CodeLength - pEncParam->StreamCount );

	if( pEncParam->WriteAddress == pEncParam->BottomAddress )
	{
		pEncParam->StreamFilled = 1;
		return( 1 );
	}

	*pEncParam->WriteAddress++ = ( BYTE )( pEncParam->StreamBits >> 24 );
	*pEncParam->WriteAddress++ = ( BYTE )( pEncParam->StreamBits >> 16 );

	pEncParam->StreamCount -= 16;
	pEncParam->StreamBits <<= 16;

	if( pEncParam->WriteAddress == pEncParam->BottomAddress )
	{
		pEncParam->StreamFilled = 1;
		return( 1 );
	}
	return( 0 );
}

//==============================================================================
//  DV decode MCU block
//
//  Copyright(C) 1997 CANOPUS Co.,Ltd.	<<<<< Don't change copyright!
//  Tom created	 1997.12.11		<<<<< Don't change originality!
//  Tom revised	 1997
//
//  <!!! WARNING !!!>
//  Don't change original comments. It's the morality for programmer.
//==============================================================================

//------------------------------------------------------------------------------
// Encode pass 1
//------------------------------------------------------------------------------

void	PASCAL EncodePass1( PENC_PARAM pEncParam, PDIF_BLOCK pEncDif,PBYTE *ppQTable )
{
	int	i, d, r, p, run, amp, len;
	DWORD	cw;
	PBYTE	pEncQTable;
	BYTE	shift;

	for( ; ; pEncParam++ )
	{
		pEncParam->StreamBits = pEncParam->ZigZagDct[ 0 ] - pEncParam->DcOffset;	// (9 bits) DC coeff already dived by 4
		pEncParam->StreamBits <<= 1;
		pEncParam->StreamBits |= pEncParam->DCTMode;
		pEncParam->StreamBits <<= 2;
		pEncParam->StreamBits |= pEncParam->Class;					// (12 bits)
		pEncParam->StreamBits <<= 20;
		pEncParam->StreamCount = 12;

		pEncQTable = ppQTable[ pEncParam->Class ];

		for( i = 1, d = 1, run = 0 ; ; i++ )
		{
			r = pEncParam->ZigZagDct[ i * 2 + 0 ];
			p = r >> 8;
			r &= 0xff;
			if( r >= 64 ) break;
			shift = pEncQTable[ p ];
			amp = ( pEncParam->ZigZagDct[ i * 2 + 1 ] + ( 1 << ( shift - 1 ) ) ) >> shift;
			run += r;
			if( ! amp ) continue;
			pEncParam->ZigZagDct[ d * 2 + 0 ] = run - 1;
			pEncParam->ZigZagDct[ d * 2 + 1 ] = amp;
			d++;
			run = 0;
		}
		pEncParam->ZigZagDct[ d * 2 + 0 ] = 64;

		pEncParam->WriteAddress = &pEncDif->ID0 + pEncParam->ByteTop;
		pEncParam->BottomAddress = pEncParam->WriteAddress + pEncParam->BlockSize;
		pEncParam->StreamFilled = 0;

		for( pEncParam->AcIndex = 1 ; ; )
		{
			run = pEncParam->ZigZagDct[ pEncParam->AcIndex * 2 + 0 ];
			amp = pEncParam->ZigZagDct[ pEncParam->AcIndex * 2 + 1 ];
			if( run >= 64 )
			{
				pEncParam->AcIndex = 65;
				PutStream( pEncParam, 0x60000000, 4 );
				break;
			}
			pEncParam->AcIndex++;
			if( run >= 16 )
			{
				cw = ( ( run - 1 ) + 0x1f80 ) << 19;
				if( ( amp < -32 ) || ( amp > 31 ) )
				{
					amp <<= 1;
					if( amp < 0 ) amp = 1 - amp;
					cw |= ( amp | 0xfe00 ) << 3;
					len = 13 + 16;
				}
				else
				{
				 	cw |= EncVLCCodewords[ 0 ][ amp + 32 ] >> 13;
				 	len = EncVLCLength[ 0 ][ amp + 32 ] + 13;
				}
			}
			else if( ( amp < -32 ) || ( amp > 31 ) )
			{
				if( run == 0 )
				{
					amp <<= 1;
					if( amp < 0 ) amp = 1 - amp;
					cw = ( amp | 0xfe00 ) << 16;
					len = 16;
				}
				else
				{
					cw = EncVLCCodewords[ run - 1 ][ 0 + 32 ];
					len = EncVLCLength[ run - 1 ][ 0 + 32 ] + 16;
					amp <<= 1;
					if( amp < 0 ) amp = 1 - amp;
					cw |= ( amp | 0xfe00 ) << ( 32 - len );
				}
			}
			else
			{
				len = EncVLCLength[ run ][ amp + 32 ];
				cw = EncVLCCodewords[ run ][ amp + 32 ];
			}
			if( PutStream( pEncParam, cw, len ) ) break;
		}
		if( pEncParam->EndOfBlock ) break;
	}
}

//------------------------------------------------------------------------------
// Encode pass 2,3 phase
//------------------------------------------------------------------------------

void	PASCAL EncodePass23( PENC_PARAM pEncParam )
{
	PENC_PARAM	pData;
	int		run, amp, len;
	DWORD		cw;

	pData = pEncParam;
	for( ; ; pData++ )
	{
		if( ! pData->StreamFilled ) break;
DATALOOP:
		if( pData->EndOfBlock ) return;
	}

ENCLOOP:
	if( pEncParam->StreamFilled )
	{
		if( pEncParam->StreamCount )
		{
			if( PutStream( pData, pEncParam->StreamBits, pEncParam->StreamCount ) )
			{
				pEncParam->StreamBits = pData->StreamBits;
				pEncParam->StreamCount = pData->StreamCount;
				pData->StreamCount = 0;
				goto DATALOOP;
			}
			pEncParam->StreamCount = 0;
		}
		if( pEncParam->AcIndex != 65 )
		{
			for( ; ; )
			{
				run = pEncParam->ZigZagDct[ pEncParam->AcIndex * 2 + 0 ];
				amp = pEncParam->ZigZagDct[ pEncParam->AcIndex * 2 + 1 ];

				if( run >= 64 )
				{
					pEncParam->AcIndex = 65;
					if( PutStream( pData, 0x60000000, 4 ) )
					{
						pEncParam->StreamBits = pData->StreamBits;
						pEncParam->StreamCount = pData->StreamCount;
						pData->StreamCount = 0;
						goto DATALOOP;
					}
					break;
				}
				pEncParam->AcIndex++;

				if( run >= 16 )
				{
					cw = ( ( run - 1 ) + 0x1f80 ) << 19;
					if( ( amp < -32 ) || ( amp > 31 ) )
					{
						amp <<= 1;
						if( amp < 0 ) amp = 1 - amp;
						cw |= ( amp | 0xfe00 ) << 3;
						len = 13 + 16;
					}
					else
					{
					 	cw |= EncVLCCodewords[ 0 ][ amp + 32 ] >> 13;
					 	len = EncVLCLength[ 0 ][ amp + 32 ] + 13;
					}
				}
				else if( ( amp < -32 ) || ( amp > 31 ) )
				{
					if( run == 0 )
					{
						amp <<= 1;
						if( amp < 0 ) amp = 1 - amp;
						cw = ( amp | 0xfe00 ) << 16;
						len = 16;
					}
					else
					{
						cw = EncVLCCodewords[ run - 1 ][ 0 + 32 ];
						len = EncVLCLength[ run - 1 ][ 0 + 32 ] + 16;
						amp <<= 1;
						if( amp < 0 ) amp = 1 - amp;
						cw |= ( amp | 0xfe00 ) << ( 32 - len );
					}
				}
				else
				{
					len = EncVLCLength[ run ][ amp + 32 ];
					cw = EncVLCCodewords[ run ][ amp + 32 ];
				}
				if( PutStream( pData, cw, len ) )
				{
					pEncParam->StreamBits = pData->StreamBits;
					pEncParam->StreamCount = pData->StreamCount;
					pData->StreamCount = 0;
					goto DATALOOP;
				}
			}
		}
	}
	if( pEncParam->EndOfBlock )
	{
		return;
	}
	pEncParam++;
	goto ENCLOOP;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void PASCAL EncodeVideoBlock( int Dseq, int VideoNo, int QNo, PENC_PARAM pEncParam,PDIF_BLOCK pEncDif )
{
	pEncDif->ID0 = ( SCT_VIDEO << 5 ) | ( RSV << 4 );
	pEncDif->ID1 = ( Dseq << 4 ) | 0x7;
	pEncDif->ID2 = VideoNo;
	pEncDif->video.STA_QNO = QNo;

	EncodePass1( pEncParam, pEncDif,EncodeQTable[ QNo ] );

	EncodePass23( pEncParam );
//	pEncDif++;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

void PASCAL EncodeFlush( PENC_PARAM pEncParam )
{
	for( ; ; pEncParam++ )
	{
		if( ! pEncParam->StreamFilled )
		{
			if( pEncParam->StreamCount )
			{
				*pEncParam->WriteAddress++ = ( BYTE )( pEncParam->StreamBits >> 24 );
				*pEncParam->WriteAddress++ = ( BYTE )( pEncParam->StreamBits >> 16 );
			}
		}
		if( pEncParam->EndOfBlock ) break;
	}
}

ENC_PARAM	EncParam[ 5 ][ 6 ];

//------------------------------------------------------------------------------
// DV encoder main loop.
//------------------------------------------------------------------------------

int	EXPORT SoftEngineEncodeDV( int mode, PBYTE pStream, PBYTE pImage, int ImageStride, int ImageSize, PVOID GetImage )
{
	int		AudioNo, VideoNo, vo, vi, m, a, b, c, d, e;
	int		QNo;
	int		a_ypos, b_ypos, c_ypos, d_ypos, e_ypos;
	int		DIFSeqNo;
	void 	(PASCAL	*GetImage525)( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC );
	void    (PASCAL *GetImage625)( PBYTE pImage, int Stride, PDCT_DATA pY ); 
	PDIF_BLOCK	pEncDif;
	int NumberOfDIFSequence;
	
	pEncDif = (PDIF_BLOCK)pStream;

	if( mode & DV_525_60_SYSTEM )
	{
		NumberOfDIFSequence = 10;	// NTSC
		GetImage525 = GetImage;
	}
	else
	{
		NumberOfDIFSequence = 12;	// PAL
		GetImage625 = GetImage;
	}

	for( DIFSeqNo = 0 ; DIFSeqNo < NumberOfDIFSequence ; DIFSeqNo++ )
	{
		EncodeHeader(pEncDif,NumberOfDIFSequence,DIFSeqNo );pEncDif++;
		EncodeSubcode( pEncDif,DIFSeqNo, 0 );pEncDif++;
		EncodeSubcode( pEncDif,DIFSeqNo, 1 );pEncDif++;
		EncodeVAUX( pEncDif,NumberOfDIFSequence,DIFSeqNo );pEncDif+=3;

		a = ( DIFSeqNo + 2 ) % NumberOfDIFSequence;
		b = ( DIFSeqNo + 6 ) % NumberOfDIFSequence;
		c = ( DIFSeqNo + 8 ) % NumberOfDIFSequence;
		d = ( DIFSeqNo + 0 ) % NumberOfDIFSequence;
		e = ( DIFSeqNo + 4 ) % NumberOfDIFSequence;
		a_ypos = a * 6 * 8;
		b_ypos = b * 6 * 8;
		c_ypos = c * 6 * 8;
		d_ypos = d * 6 * 8;
		e_ypos = e * 6 * 8;

		AudioNo = 0;
		VideoNo = 0;
		for( m = 0, vo = 0 ; vo < 9 ; vo++ )
		{
			EncodeAudio( pEncDif,NumberOfDIFSequence,DIFSeqNo, AudioNo++ );pEncDif++;

			for( vi = 0 ; vi < 3 ; vi++, m++ )
			{
				if( mode & DV_525_60_SYSTEM )
				{
					unsigned char *pSrcBuffer=pImage + (a_ypos + MBPosition525[2][m].y) * ImageStride + (MBPosition525[2][m].x) * ImageSize;
					GetImage525(pSrcBuffer,ImageStride,32,8,YuvBuffer[0],YuvBuffer[8]);
//					GetImage525( pImage,MBPosition525[2][m].x, a_ypos + MBPosition525[2][m].y, 32, 8, ImageStride,YuvBuffer[0], YuvBuffer[8] );
					FDctBlock( EncParam[ 0 ], pYuv525 );

					pSrcBuffer=pImage + (b_ypos + MBPosition525[1][m].y) * ImageStride + (MBPosition525[1][m].x) * ImageSize;
					GetImage525(pSrcBuffer,ImageStride,32,8,YuvBuffer[0],YuvBuffer[8]);
					FDctBlock( EncParam[ 1 ], pYuv525 );

					pSrcBuffer=pImage + (c_ypos + MBPosition525[3][m].y) * ImageStride + (MBPosition525[3][m].x) * ImageSize;
					GetImage525(pSrcBuffer,ImageStride,32,8,YuvBuffer[0],YuvBuffer[8]);
					FDctBlock( EncParam[ 2 ], pYuv525 );

					pSrcBuffer=pImage + (d_ypos + MBPosition525[0][m].y) * ImageStride + (MBPosition525[0][m].x) * ImageSize;
					GetImage525(pSrcBuffer,ImageStride,32,8,YuvBuffer[0],YuvBuffer[8]);
					FDctBlock( EncParam[ 3 ], pYuv525 );

					if( m < 24 )
					{
						pSrcBuffer=pImage + (e_ypos + MBPosition525[4][m].y) * ImageStride + (MBPosition525[4][m].x) * ImageSize;
						GetImage525(pSrcBuffer,ImageStride,32,8,YuvBuffer[0],YuvBuffer[8]);
					}
					else
					{
						pSrcBuffer=pImage + (e_ypos + MBPosition525[4][m].y) * ImageStride + (MBPosition525[4][m].x) * ImageSize;
						GetImage525(pSrcBuffer,ImageStride,16,8,YuvBuffer[0],YuvBuffer[8]);
						pSrcBuffer=pImage + (e_ypos + MBPosition525[4][m].y+8) * ImageStride + (MBPosition525[4][m].x) * ImageSize;
						GetImage525(pSrcBuffer,ImageStride,16,8,YuvBuffer[0]+16,YuvBuffer[8]+4);
					}
					FDctBlock( EncParam[ 4 ], pYuv525 );
				}
				else
				{
					unsigned char *pSrcBuffer;
					pSrcBuffer=pImage + (a_ypos + MBPosition625[ m ].y) * ImageStride + (16 * 9 * 2 + MBPosition625[ m ].x) * ImageSize;
					GetImage625(pSrcBuffer,ImageStride,YuvBuffer[0]);
					FDctBlock( EncParam[ 0 ], pYuv625 );

					pSrcBuffer=pImage + (b_ypos + MBPosition625[ m ].y) * ImageStride + (16 * 9 * 1 + MBPosition625[ m ].x) * ImageSize;
					GetImage625(pSrcBuffer,ImageStride,YuvBuffer[0]);
					FDctBlock( EncParam[ 1 ], pYuv625 );

					pSrcBuffer=pImage + (c_ypos + MBPosition625[ m ].y) * ImageStride + (16 * 9 * 3 + MBPosition625[ m ].x) * ImageSize;
					GetImage625(pSrcBuffer,ImageStride,YuvBuffer[0]);
					FDctBlock( EncParam[ 2 ], pYuv625 );

					pSrcBuffer=pImage + (d_ypos + MBPosition625[ m ].y) * ImageStride + (16 * 9 * 0 + MBPosition625[ m ].x) * ImageSize;
					GetImage625(pSrcBuffer,ImageStride,YuvBuffer[0]);
					FDctBlock( EncParam[ 3 ], pYuv625 );

					pSrcBuffer=pImage + (e_ypos + MBPosition625[ m ].y) * ImageStride + (16 * 9 * 4 + MBPosition625[ m ].x) * ImageSize;
					GetImage625(pSrcBuffer,ImageStride,YuvBuffer[0]);
					FDctBlock( EncParam[ 4 ], pYuv625  );
				}
				QNo = EstimateBits(EncParam);

				EncParam[ 0 ][ 5 ].EndOfBlock = 1;
				EncParam[ 1 ][ 5 ].EndOfBlock = 1;
				EncParam[ 2 ][ 5 ].EndOfBlock = 1;
				EncParam[ 3 ][ 5 ].EndOfBlock = 1;

				EncodeVideoBlock( DIFSeqNo, VideoNo++, QNo, EncParam[ 0 ],pEncDif );pEncDif++;
				EncodeVideoBlock( DIFSeqNo, VideoNo++, QNo, EncParam[ 1 ],pEncDif );pEncDif++;
				EncodeVideoBlock( DIFSeqNo, VideoNo++, QNo, EncParam[ 2 ],pEncDif );pEncDif++;
				EncodeVideoBlock( DIFSeqNo, VideoNo++, QNo, EncParam[ 3 ],pEncDif );pEncDif++;
				EncodeVideoBlock( DIFSeqNo, VideoNo++, QNo, EncParam[ 4 ],pEncDif );pEncDif++;

				EncParam[ 0 ][ 5 ].EndOfBlock = 0;
				EncParam[ 1 ][ 5 ].EndOfBlock = 0;
				EncParam[ 2 ][ 5 ].EndOfBlock = 0;
				EncParam[ 3 ][ 5 ].EndOfBlock = 0;

				EncodePass23( EncParam[ 0 ] );
				EncodeFlush( EncParam[ 0 ] );
			}
		}
	}
	return( DV_NO_ERROR );
}

//-----------------------------------------------------------------------------
// Initialize encode parameters.
//-----------------------------------------------------------------------------

#define	Y_DC_OFFSET	254

void	InitEncParam( ENC_PARAM EncParam[5][6] )
{
	int	i, l;

	for( i = 0 ; i < 5 ; i++ )
	{
		EncParam[ i ][ 0 ].ByteTop =  4;	// offset to Y0
		EncParam[ i ][ 1 ].ByteTop = 18;
		EncParam[ i ][ 2 ].ByteTop = 32;
		EncParam[ i ][ 3 ].ByteTop = 46;
		EncParam[ i ][ 4 ].ByteTop = 60;
		EncParam[ i ][ 5 ].ByteTop = 70;

		EncParam[ i ][ 0 ].BlockSize = 14;	// block size of Y0
		EncParam[ i ][ 1 ].BlockSize = 14;
		EncParam[ i ][ 2 ].BlockSize = 14;
		EncParam[ i ][ 3 ].BlockSize = 14;
		EncParam[ i ][ 4 ].BlockSize = 10;
		EncParam[ i ][ 5 ].BlockSize = 10;

		EncParam[ i ][ 0 ].DcOffset = Y_DC_OFFSET;
		EncParam[ i ][ 1 ].DcOffset = Y_DC_OFFSET;
		EncParam[ i ][ 2 ].DcOffset = Y_DC_OFFSET;
		EncParam[ i ][ 3 ].DcOffset = Y_DC_OFFSET;
		EncParam[ i ][ 4 ].DcOffset = 0;
		EncParam[ i ][ 5 ].DcOffset = 0;

		for( l = 0 ; l < 6 ; l++ )
		{
			EncParam[ i ][ l ].EndOfBlock = 0;
		}
	}
	EncParam[ 4 ][ 5 ].EndOfBlock = 1;
}

