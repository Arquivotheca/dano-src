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
//
//	R = Y           + 88/64V = Y            + 1.375000V
//	G = Y -  22/64U - 45/64V = Y - 0.34375U - 0.703125V
//	B = Y + 111/64U          = Y + 1.73438U
//
//	Y=(  77R+150G+29B)/256
//	U=(-44R-87G+131B)/256
//	V=(131R-110G-21B)/256
//

#define	DCT_SHIFT			3
#define	COEFF_PRECISION		16
#define	R_FROM_V			( ( int )(  1.40200 * ( 1 << COEFF_PRECISION ) + 0.5 ) )	// YUV to RGB transform definition
#define	G_FROM_U			( ( int )( -0.34414 * ( 1 << COEFF_PRECISION ) + 0.5 ) )	// R = Y               + 1.40200 * V	CCIR.601.1
#define	G_FROM_V			( ( int )( -0.71414 * ( 1 << COEFF_PRECISION ) + 0.5 ) )	// G = Y - 0.34414 * U - 0.71414 * V
#define	B_FROM_U			( ( int )(  1.77200 * ( 1 << COEFF_PRECISION ) + 0.5 ) )	// B = Y + 1.77200 * U  <--- not use

//------------------------------------------------------------------------------
// Definition of coefficient for MMX
//------------------------------------------------------------------------------

#define	mmxCOEFF_PRECISION	( 16 - DCT_SHIFT )
#define	mmxR_FROM_V			( ( int )(  1.40200 * ( 1 << mmxCOEFF_PRECISION ) + 0.5 ) )	// YUV to RGB transform definition
#define	mmxG_FROM_U			( ( int )( -0.34414 * ( 1 << mmxCOEFF_PRECISION ) + 0.5 ) )	// R = Y               + 1.40200 * V	CCIR.601.1
#define	mmxG_FROM_V			( ( int )( -0.71414 * ( 1 << mmxCOEFF_PRECISION ) + 0.5 ) )	// G = Y - 0.34414 * U - 0.71414 * V
#define	mmxB_FROM_U			( ( int )(  1.77200 * ( 1 << mmxCOEFF_PRECISION ) + 0.5 ) )	// B = Y + 1.77200 * U  <--- not use

extern	DCT_DATA	mmxRFromV[ 4 ];
extern	DCT_DATA	mmxGFromU[ 4 ];
extern	DCT_DATA	mmxGFromV[ 4 ];
extern	DCT_DATA	mmxBFromU[ 4 ];

//-----------------------------------------------------------------------------
// definitions for range compression.
//-----------------------------------------------------------------------------

#define	MINUS_OFFSET	128
extern	BYTE	DecodeBandTable[];

