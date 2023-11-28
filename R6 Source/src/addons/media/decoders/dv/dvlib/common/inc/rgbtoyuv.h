//=============================================================================
// Description:
//
//
//
//
//
// Copyright:
//		Copyright (c) 1998 Canopus Co.,Ltd. All Rights Reserved.
//		Developed in San Jose, CA, U.S.A.
// History:
//		09/10/98 Tom > Separated.
//
//=============================================================================
//------------------------------------------------------------------------------
// YUV to RGB transform definition
//------------------------------------------------------------------------------
//
//	16 <= Y <= 235		16 <= CR,CB <= 240
//	(126 center)		(128 center)
//	 16 (-110=Black)	16 (-112)
//	 17 (-109)			17 (-111)
//	¥¥¥¥				¥¥¥¥
//	125 (-1)			127 (-1)
//	126 (0)				128 (0)
//	127 (1)				129 (1)
//	¥¥¥¥				¥¥¥¥
//	234 (108)			239 (111)
//	235 (109=White)		240 (112)
//
//	Y = (  0.29900 * R + 0.58700 * G + 0.11400 * B ) * 220 / 256 + 16
//	U = ( -0.16874 * R - 0.33126 * G + 0.50000 * B ) * 225 / 256 + 128
//	V = (  0.50000 * R - 0.41869 * G - 0.08131 * B ) * 225 / 256 + 128
//
//	( Y -  16 ) * 256 / 220 =  0.29900 * R + 0.58700 * G + 0.11400 * B
//	( U - 128 ) * 256 / 225 = -0.16874 * R - 0.33126 * G + 0.50000 * B
//	( V - 128 ) * 256 / 225 =  0.50000 * R - 0.41869 * G - 0.08131 * B

#define	COEFF_PRECISION	15
#define	DCT_SHIFT		3						// DCT—p‚É‚ ‚ç‚©‚¶‚ß8”{‚µ‚Ä‚¨‚­
#define	COEFF_SCALE_Y	( 254.0 / 256.0 )
#define	COEFF_SCALE_U	( 256.0 / 256.0 )
#define	COEFF_SCALE_V	( 256.0 / 256.0 )
#define	Y_FROM_R		( ( int )(  0.29900 * ( 1 << COEFF_PRECISION ) * COEFF_SCALE_Y + 0.5 ) )	// RGB to YUV transform definition CCIR.601.1
#define	Y_FROM_G		( ( int )(  0.58700 * ( 1 << COEFF_PRECISION ) * COEFF_SCALE_Y + 0.5 ) )
#define	Y_FROM_B		( ( int )(  0.11400 * ( 1 << COEFF_PRECISION ) * COEFF_SCALE_Y + 0.5 ) )	// Y =  0.29900 * R + 0.58700 * G + 0.11400 * B
#define	U_FROM_R		( ( int )( -0.16874 * ( 1 << COEFF_PRECISION ) * COEFF_SCALE_U + 0.5 ) )	// U = -0.16874 * R - 0.33126 * G + 0.50000 * B
#define	U_FROM_G		( ( int )( -0.33126 * ( 1 << COEFF_PRECISION ) * COEFF_SCALE_U + 0.5 ) )	// V =  0.50000 * R - 0.41869 * G - 0.08131 * B
#define	U_FROM_B		( ( int )(  0.50000 * ( 1 << COEFF_PRECISION ) * COEFF_SCALE_U + 0.5 ) )
#define	V_FROM_R		( ( int )(  0.50000 * ( 1 << COEFF_PRECISION ) * COEFF_SCALE_V + 0.5 ) )
#define	V_FROM_G		( ( int )( -0.41869 * ( 1 << COEFF_PRECISION ) * COEFF_SCALE_V + 0.5 ) )
#define	V_FROM_B		( ( int )( -0.08131 * ( 1 << COEFF_PRECISION ) * COEFF_SCALE_V + 0.5 ) )

#define	mmxCOEFF_PRECISION	15
#define	mmxY_FROM_R		( ( int )(  0.29900 * ( 1 << mmxCOEFF_PRECISION ) * COEFF_SCALE_Y + 0.5 ) )	// RGB to YUV transform definition CCIR.601.1
#define	mmxY_FROM_G		( ( int )(  0.58700 * ( 1 << mmxCOEFF_PRECISION ) * COEFF_SCALE_Y + 0.5 ) )
#define	mmxY_FROM_B		( ( int )(  0.11400 * ( 1 << mmxCOEFF_PRECISION ) * COEFF_SCALE_Y + 0.5 ) )	// Y =  0.29900 * R + 0.58700 * G + 0.11400 * B
#define	mmxU_FROM_R		( ( int )( -0.16874 * ( 1 << mmxCOEFF_PRECISION ) * COEFF_SCALE_U + 0.5 ) )	// U = -0.16874 * R - 0.33126 * G + 0.50000 * B
#define	mmxU_FROM_G		( ( int )( -0.33126 * ( 1 << mmxCOEFF_PRECISION ) * COEFF_SCALE_U + 0.5 ) )	// V =  0.50000 * R - 0.41869 * G - 0.08131 * B
#define	mmxU_FROM_B		( ( int )(  0.50000 * ( 1 << mmxCOEFF_PRECISION ) * COEFF_SCALE_U + 0.5 ) )
#define	mmxV_FROM_R		( ( int )(  0.50000 * ( 1 << mmxCOEFF_PRECISION ) * COEFF_SCALE_V + 0.5 ) )
#define	mmxV_FROM_G		( ( int )( -0.41869 * ( 1 << mmxCOEFF_PRECISION ) * COEFF_SCALE_V + 0.5 ) )
#define	mmxV_FROM_B		( ( int )( -0.08131 * ( 1 << mmxCOEFF_PRECISION ) * COEFF_SCALE_V + 0.5 ) )
//-----------------------------------------------------------------------------
// only for compression rgb->yuv transform.
//-----------------------------------------------------------------------------

#define	MAX_Y			( ( ( int )( ( (  256 ) << COEFF_PRECISION ) * COEFF_SCALE_Y ) ) >> ( COEFF_PRECISION - DCT_SHIFT ) )
#define	MIN_U			( ( ( int )( ( ( -127 ) << COEFF_PRECISION ) * COEFF_SCALE_U ) ) >> ( COEFF_PRECISION - DCT_SHIFT ) )
#define	MAX_U			( ( ( int )( ( (  126 ) << COEFF_PRECISION ) * COEFF_SCALE_U ) ) >> ( COEFF_PRECISION - DCT_SHIFT ) )
#define	MIN_V			( ( ( int )( ( ( -127 ) << COEFF_PRECISION ) * COEFF_SCALE_V ) ) >> ( COEFF_PRECISION - DCT_SHIFT ) )
#define	MAX_V			( ( ( int )( ( (  126 ) << COEFF_PRECISION ) * COEFF_SCALE_V ) ) >> ( COEFF_PRECISION - DCT_SHIFT ) )

//-----------------------------------------------------------------------------
// external valiables
//-----------------------------------------------------------------------------

extern	short	UnpackB[ 4 ];
extern	short	UnpackG[ 4 ];
extern	short	UnpackR[ 4 ];

extern	short	mmxYFromR[ 4 ];
extern	short	mmxYFromG[ 4 ];
extern	short	mmxYFromB[ 4 ];
extern	short	mmxUFromR[ 4 ];
extern	short	mmxUFromG[ 4 ];
extern	short	mmxUFromB[ 4 ];
extern	short	mmxVFromR[ 4 ];
extern	short	mmxVFromG[ 4 ];
extern	short	mmxVFromB[ 4 ];
//extern	short	Y_Offset[ 4 ];

extern	int		EncodeBandTable[];

