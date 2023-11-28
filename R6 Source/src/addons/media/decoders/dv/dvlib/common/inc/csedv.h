#ifndef CSEDV_H
#define CSEDV_H
//=============================================================================
// Description:
//		Software DV decoder/encoder common header file.
//
//
//
//
// Copyright:
//		Copyright (c) 1997-1998 CANOPUS Co.,Ltd.
//
// History:
//		10/01/97 Tom > Creaded.
//=============================================================================

#if defined (__BEOS__) && !defined (__POWERPC__)
#define EXPORT
#else
#define	EXPORT	__declspec(dllexport) PASCAL
#endif

//------------------------------------------------------------------------------
// Data type definition
//------------------------------------------------------------------------------

typedef short		DCT_DATA;
typedef DCT_DATA *	PDCT_DATA;
typedef int			QUANT_DATA;
typedef QUANT_DATA *PQUANT_DATA;


#define	DCT_BUFFER_STRIDE	( 8 * 4 )

#define DIF_SEQUENCE_SIZE	12000

//------------------------------------------------------------------------------
// DV frame image extent
//------------------------------------------------------------------------------

#define	DV_FRAME_WIDTH		720
#define	DV_FRAME_HEIGHT_525	480
#define	DV_FRAME_HEIGHT_625	576

//------------------------------------------------------------------------------
// DV command/mode definitions
//------------------------------------------------------------------------------

#define	DV_DECODE_1_2		0x00000001
#define	DV_DECODE_1_8		0x00000004

#define	DV_DECODE_FIELD		0x00000010
#define	DV_ORDER_ODD_EVEN	0x00000020

#define	DV_525_60_SYSTEM	0x00010000
#define	DV_625_50_SYSTEM	0x00020000
#define	DV_IGNORE_PASS2		0x00100000
#define	DV_IGNORE_PASS3		0x00200000
#define	DV_GET_EXTENT		0x80000000

//------------------------------------------------------------------------------
// definition of hi-speed 1/8 decoder
//------------------------------------------------------------------------------

#define	DIV8_525_WIDTH			88
#define	DIV8_525_HEIGHT			60
#define	DIV8_525_Y_CR_OFFSET	96
#define	DIV8_525_CR_CB_OFFSET	24
#define	DIV8_525_STRIDE			( 96 + 24 * 2 )

#define	DIV8_625_WIDTH			90
#define	DIV8_625_HEIGHT			72
#define	DIV8_625_Y_CR_OFFSET	( 96 * 72 )
#define	DIV8_625_CR_CB_OFFSET	48
#define	DIV8_625_STRIDE			96

//------------------------------------------------------------------------------
// DV error code
//------------------------------------------------------------------------------

#define	DV_NO_ERROR					0	// エラーなし
#define	DV_ERROR_BREAK				5	// 処理を中止しました。abort encode/decoding
#define	DV_ERROR_IMAGE				6	// イメージバッファアクセスでエラー発生
#define	DV_ERROR_STREAM				7	// ストリームアクセスでエラー発生

//#define	DV_ERROR_COMPONENT			1	// サポートしていないコンポーネントの数
//#define	DV_ERROR_SCANRATE			2	// サポートしていないスキャンレート
//#define	DV_ERROR_HUFFMAN_OVERRUN	3	// ハフマンテーブルテーブルが足りません。over run huffman hush table
//#define	DV_ERROR_PREFIX				4	// プレフィックスが見つからない。not found prefix.
//#define	DV_ERROR_UNSUPPORTED		8	// サポートしていないＪＰＥＧ形式
//#define	DV_ERROR_QTABLE_OVERRUN		9	// 量子化テーブルがオーバーしました。


typedef struct {
	BYTE		Completed;
	BYTE		DataEmpty;
	BYTE		EndOfBlock;
	BYTE		dummy1;
	int			DCTMode;
	int			Class;
	int			ByteTop;
	int			BlockSize;
	DWORD		StreamBits;
	int			StreamCount;
	PBYTE		ReadAddress;
	PBYTE		BottomAddress;
	int			DcOffset;
	int			AcIndex;
	int			*pZigZagTable;
	PQUANT_DATA	pQTable;
	DCT_DATA	DctBuffer[ 64 ];
} DEC_PARAM, *PDEC_PARAM;

//------------------------------------------------------------------------------
// For encoder
//------------------------------------------------------------------------------

typedef struct {
	BYTE		EndOfBlock;
	BYTE		StreamFilled;
	BYTE		dmy1;
	BYTE		dmy2;
	int			DCTMode;
	int			Class;
	int			ByteTop;
	int			BlockSize;
	DWORD		StreamBits;
	int			StreamCount;
	PBYTE		WriteAddress;
	PBYTE		BottomAddress;
	int			DcOffset;
	int			AcIndex;
	DCT_DATA	DctBuffer[ 64 ];
	DCT_DATA	ZigZagDct[ 64 * 2 + 2 ];
} ENC_PARAM, *PENC_PARAM;

void	PASCAL SetupCodec( void );
int		PASCAL SoftEngineDecodeDV( int mode, PBYTE pStream, PBYTE pImage, int ImageStride, int ImageSize, PVOID PutImage );
int		PASCAL SoftEngineEncodeDV( int mode, PBYTE pStream, PBYTE pImage, int ImageStride, int ImageSize, PVOID GetImage );

#endif

