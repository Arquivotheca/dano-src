//------------------------------------------------------------------------------
// YUV buffer
//------------------------------------------------------------------------------

extern	DCT_DATA	YuvBuffer[ 8 * 2 ][ DCT_BUFFER_STRIDE ];

extern	PDCT_DATA	pYuv525[ 6 ];
extern	PDCT_DATA	pYuv625[ 6 ];
extern	int	ZigZag88[];
extern	int	ZigZag248[];
//------------------------------------------------------------------------------
// Weighting table
//------------------------------------------------------------------------------

extern	short	ZigZagWeighting88DCT[ 64 ];
extern	short	ZigZagWeighting248DCT[ 64 ];


//------------------------------------------------------------------------------
// Macro block position table
//------------------------------------------------------------------------------

extern	POINT	MBPosition525[ 5 ][ 32 ];
extern	POINT	MBPosition625[ 32 ];

//------------------------------------------------------------------------------
// Quantization table with weighting.
//------------------------------------------------------------------------------

extern	PQUANT_DATA	DecodeQTable[ 16 ][ 8 ];
extern	PBYTE		EncodeQTable[ 16 ][ 4 ];

//------------------------------------------------------------------------------
// VLC code for decoder
//------------------------------------------------------------------------------

#define	DEC_VLC_LUP1_BIT	12
#define	DEC_VLC_LUP2_BIT	4
#define	DEC_VLC_LUP1_MASK	( ( 1 << DEC_VLC_LUP1_BIT ) - 1 )
#define	DEC_VLC_LUP2_MASK	( ( 1 << DEC_VLC_LUP2_BIT ) - 1 )

typedef	struct {
	char	Run;
	char	Length;
	short	Amp;
} DEC_VLC;
extern	DEC_VLC DecVLCTable[ 5376 ];

//------------------------------------------------------------------------------
// VLC code for encoder
//------------------------------------------------------------------------------

extern	BYTE	EncVLCLength[ 16 ][ 64 ];
extern	DWORD	EncVLCCodewords[ 16 ][ 64 ];
