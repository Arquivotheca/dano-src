//=============================================================================
// Description:
//
//
//
//
//
// Copyright:
//		Copyright (c) 1998 CANOPUS Corporation. Co.,Ltd.
//
// History:
//		Tom > Creaded.
//
//=============================================================================
#include <windows.h>
#include "dv.h"

//------------------------------------------------------------------------------
// Definition of DCT
//------------------------------------------------------------------------------

#define	DCT_COEFF_PRECISION	14				// DCTåWêîÇÃç≈ëÂílÇ™16ÀﬁØƒà»ì‡Ç≈ï\ÇÌÇπÇÈÀﬁØƒêî(16Ç≈Ç‡MMXà»äOÇ≈ÇÕìÆçÏâ¬)

//------------------------------------------------------------------------------
// DCT coefficient
//------------------------------------------------------------------------------

#define	DCT_COEFF_1			( ( int )( 1.387039845 * ( 1 << DCT_COEFF_PRECISION ) + 0.5 ) )
#define	DCT_COEFF_2			( ( int )( 1.306562965 * ( 1 << DCT_COEFF_PRECISION ) + 0.5 ) )
#define	DCT_COEFF_3			( ( int )( 1.175875602 * ( 1 << DCT_COEFF_PRECISION ) + 0.5 ) )
#define	DCT_COEFF_5			( ( int )( 0.785694958 * ( 1 << DCT_COEFF_PRECISION ) + 0.5 ) )
#define	DCT_COEFF_6			( ( int )( 0.541196100 * ( 1 << DCT_COEFF_PRECISION ) + 0.5 ) )
#define	DCT_COEFF_7			( ( int )( 0.275899379 * ( 1 << DCT_COEFF_PRECISION ) + 0.5 ) )



#define	DCT_DATA_SIZE	2
#define	DCT_SRC_STRIDE	(8*2)
#define	DCT_DEST_STRIDE	(32*2)
#define	SCALE( d, s )	( ( ( d ) + ( 1 << ( ( s ) - 1 ) ) ) >> ( s ) )

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void PASCAL IDCT_X88( short *buff )
{
	int	y, x0, x1, y0, y1, z0, z1, z2, z3;

	for( y = 0 ; y < 8 ; y++ )
	{
		x0 = ( buff[ 0 ] + buff[ 4 ] ) << DCT_COEFF_PRECISION;
		x1 = ( buff[ 0 ] - buff[ 4 ] ) << DCT_COEFF_PRECISION;
		y0 = DCT_COEFF_2 * buff[ 2 ] + DCT_COEFF_6 * buff[ 6 ];
		y1 = DCT_COEFF_6 * buff[ 2 ] - DCT_COEFF_2 * buff[ 6 ];
		z0 = DCT_COEFF_1 * buff[ 1 ] + DCT_COEFF_3 * buff[ 3 ] + DCT_COEFF_5 * buff[ 5 ] + DCT_COEFF_7 * buff[ 7 ];
		z1 = DCT_COEFF_3 * buff[ 1 ] - DCT_COEFF_7 * buff[ 3 ] - DCT_COEFF_1 * buff[ 5 ] - DCT_COEFF_5 * buff[ 7 ];
		z2 = DCT_COEFF_5 * buff[ 1 ] - DCT_COEFF_1 * buff[ 3 ] + DCT_COEFF_7 * buff[ 5 ] + DCT_COEFF_3 * buff[ 7 ];
		z3 = DCT_COEFF_7 * buff[ 1 ] - DCT_COEFF_5 * buff[ 3 ] + DCT_COEFF_3 * buff[ 5 ] - DCT_COEFF_1 * buff[ 7 ];

		buff[ 0 ] = SCALE( x0 + y0 + z0, DCT_COEFF_PRECISION );
		buff[ 1 ] = SCALE( x1 + y1 + z1, DCT_COEFF_PRECISION );
		buff[ 2 ] = SCALE( x1 - y1 + z2, DCT_COEFF_PRECISION );
		buff[ 3 ] = SCALE( x0 - y0 + z3, DCT_COEFF_PRECISION );
		buff[ 4 ] = SCALE( x0 - y0 - z3, DCT_COEFF_PRECISION );
		buff[ 5 ] = SCALE( x1 - y1 - z2, DCT_COEFF_PRECISION );
		buff[ 6 ] = SCALE( x1 + y1 - z1, DCT_COEFF_PRECISION );
		buff[ 7 ] = SCALE( x0 + y0 - z0, DCT_COEFF_PRECISION );
		buff += 8;
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void PASCAL IDCT_Y88( short src[][ 8 ], short dest[][ 32 ] )
{
	int	x, x0, x1, y0, y1, z0, z1, z2, z3;

	for( x = 0 ; x < 8 ; x++ )
	{
		x0 = ( src[ 0 ][ x ] + src[ 4 ][ x ] ) << DCT_COEFF_PRECISION;
		x1 = ( src[ 0 ][ x ] - src[ 4 ][ x ] ) << DCT_COEFF_PRECISION;
		y0 = DCT_COEFF_2 * src[ 2 ][ x ] + DCT_COEFF_6 * src[ 6 ][ x ];
		y1 = DCT_COEFF_6 * src[ 2 ][ x ] - DCT_COEFF_2 * src[ 6 ][ x ];
		z0 = DCT_COEFF_1 * src[ 1 ][ x ] + DCT_COEFF_3 * src[ 3 ][ x ] + DCT_COEFF_5 * src[ 5 ][ x ] + DCT_COEFF_7 * src[ 7 ][ x ];
		z1 = DCT_COEFF_3 * src[ 1 ][ x ] - DCT_COEFF_7 * src[ 3 ][ x ] - DCT_COEFF_1 * src[ 5 ][ x ] - DCT_COEFF_5 * src[ 7 ][ x ];
		z2 = DCT_COEFF_5 * src[ 1 ][ x ] - DCT_COEFF_1 * src[ 3 ][ x ] + DCT_COEFF_7 * src[ 5 ][ x ] + DCT_COEFF_3 * src[ 7 ][ x ];
		z3 = DCT_COEFF_7 * src[ 1 ][ x ] - DCT_COEFF_5 * src[ 3 ][ x ] + DCT_COEFF_3 * src[ 5 ][ x ] - DCT_COEFF_1 * src[ 7 ][ x ];

		dest[ 0 ][ x ] = SCALE( x0 + y0 + z0, DCT_COEFF_PRECISION/* + 3*/ );
		dest[ 1 ][ x ] = SCALE( x1 + y1 + z1, DCT_COEFF_PRECISION/* + 3*/ );
		dest[ 2 ][ x ] = SCALE( x1 - y1 + z2, DCT_COEFF_PRECISION/* + 3*/ );
		dest[ 3 ][ x ] = SCALE( x0 - y0 + z3, DCT_COEFF_PRECISION/* + 3*/ );
		dest[ 4 ][ x ] = SCALE( x0 - y0 - z3, DCT_COEFF_PRECISION/* + 3*/ );
		dest[ 5 ][ x ] = SCALE( x1 - y1 - z2, DCT_COEFF_PRECISION/* + 3*/ );
		dest[ 6 ][ x ] = SCALE( x1 + y1 - z1, DCT_COEFF_PRECISION/* + 3*/ );
		dest[ 7 ][ x ] = SCALE( x0 + y0 - z0, DCT_COEFF_PRECISION/* + 3*/ );
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void PASCAL IDCT_Y248( short src[][ 8 ], short dest[][ 32 ] )
{
	int	x, x0, x1, x2, x3, y0, y1;

	for( x = 0 ; x < 8 ; x++ )
	{
		x0 = ( src[ 0 ][ x ] + src[ 4 ][ x ] ) << DCT_COEFF_PRECISION;
		x1 = ( src[ 1 ][ x ] + src[ 5 ][ x ] );
		x2 = ( src[ 2 ][ x ] + src[ 6 ][ x ] ) << DCT_COEFF_PRECISION;
		x3 = ( src[ 3 ][ x ] + src[ 7 ][ x ] );

		y0 = DCT_COEFF_2 * x1 + DCT_COEFF_6 * x3;
		y1 = DCT_COEFF_6 * x1 - DCT_COEFF_2 * x3;

		dest[ 0 ][ x ] = SCALE( x0 + x2 + y0, DCT_COEFF_PRECISION );
		dest[ 2 ][ x ] = SCALE( x0 - x2 + y1, DCT_COEFF_PRECISION );
		dest[ 4 ][ x ] = SCALE( x0 - x2 - y1, DCT_COEFF_PRECISION );
		dest[ 6 ][ x ] = SCALE( x0 + x2 - y0, DCT_COEFF_PRECISION );

		x0 = ( src[ 0 ][ x ] - src[ 4 ][ x ] ) << DCT_COEFF_PRECISION;
		x1 = ( src[ 1 ][ x ] - src[ 5 ][ x ] );
		x2 = ( src[ 2 ][ x ] - src[ 6 ][ x ] ) << DCT_COEFF_PRECISION;
		x3 = ( src[ 3 ][ x ] - src[ 7 ][ x ] );

		y0 = DCT_COEFF_2 * x1 + DCT_COEFF_6 * x3;
		y1 = DCT_COEFF_6 * x1 - DCT_COEFF_2 * x3;

		dest[ 1 ][ x ] = SCALE( x0 + x2 + y0, DCT_COEFF_PRECISION );
		dest[ 3 ][ x ] = SCALE( x0 - x2 + y1, DCT_COEFF_PRECISION );
		dest[ 5 ][ x ] = SCALE( x0 - x2 - y1, DCT_COEFF_PRECISION );
		dest[ 7 ][ x ] = SCALE( x0 + x2 - y0, DCT_COEFF_PRECISION );
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void PASCAL InverseDCT( int Mode248, short src[][ 8 ], short dest[][ 32 ] )
{
	IDCT_X88( src[ 0 ] );
	if( Mode248 )
	{
		IDCT_Y248( src, dest );
	}
	else
	{
		IDCT_Y88( src, dest );
	}
}


//------------------------------------------------------------------------------
// FDCT coefficient
//------------------------------------------------------------------------------

#define	SHIFT_X		2
#define	SHIFT_Y		2

void PASCAL FDCT_X( short src[][ 32 ], short temp[][ 8 ] )
{
	int	y, x0, x1, x2, x3;

	for( y = 0 ; y < 8 ; y++ )
	{
		x0 = src[ y ][ 0 ] - src[ y ][ 7 ];
		x1 = src[ y ][ 1 ] - src[ y ][ 6 ];
		x2 = src[ y ][ 2 ] - src[ y ][ 5 ];
		x3 = src[ y ][ 3 ] - src[ y ][ 4 ];

		temp[ y ][ 1 ] = SCALE( DCT_COEFF_1 * x0 + DCT_COEFF_3 * x1 + DCT_COEFF_5 * x2 + DCT_COEFF_7 * x3, DCT_COEFF_PRECISION + SHIFT_X );
		temp[ y ][ 3 ] = SCALE( DCT_COEFF_3 * x0 - DCT_COEFF_7 * x1 - DCT_COEFF_1 * x2 - DCT_COEFF_5 * x3, DCT_COEFF_PRECISION + SHIFT_X );
		temp[ y ][ 5 ] = SCALE( DCT_COEFF_5 * x0 - DCT_COEFF_1 * x1 + DCT_COEFF_7 * x2 + DCT_COEFF_3 * x3, DCT_COEFF_PRECISION + SHIFT_X );
		temp[ y ][ 7 ] = SCALE( DCT_COEFF_7 * x0 - DCT_COEFF_5 * x1 + DCT_COEFF_3 * x2 - DCT_COEFF_1 * x3, DCT_COEFF_PRECISION + SHIFT_X );

		x0 = src[ y ][ 0 ] + src[ y ][ 7 ];
		x1 = src[ y ][ 1 ] + src[ y ][ 6 ];
		x2 = src[ y ][ 2 ] + src[ y ][ 5 ];
		x3 = src[ y ][ 3 ] + src[ y ][ 4 ];

		temp[ y ][ 0 ] = SCALE( x0 + x3 + x1 + x2, SHIFT_X );
		temp[ y ][ 4 ] = SCALE( x0 + x3 - x1 - x2, SHIFT_X );
		x0 -= x3;
		x1 -= x2;
		temp[ y ][ 2 ] = SCALE( DCT_COEFF_2 * x0 + DCT_COEFF_6 * x1, DCT_COEFF_PRECISION + SHIFT_X );
		temp[ y ][ 6 ] = SCALE( DCT_COEFF_6 * x0 - DCT_COEFF_2 * x1, DCT_COEFF_PRECISION + SHIFT_X );
	}
}



void PASCAL F88DCT_Y( short temp[][ 8 ], short dest[][ 8 ] )
{
	int	x, x0, x1, x2, x3;

	for( x = 0 ; x < 8 ; x++ )
	{
		x0 = temp[ 0 ][ x ] - temp[ 7 ][ x ];
		x1 = temp[ 1 ][ x ] - temp[ 6 ][ x ];
		x2 = temp[ 2 ][ x ] - temp[ 5 ][ x ];
		x3 = temp[ 3 ][ x ] - temp[ 4 ][ x ];

		dest[ 1 ][ x ] = SCALE( DCT_COEFF_1 * x0 + DCT_COEFF_3 * x1 + DCT_COEFF_5 * x2 + DCT_COEFF_7 * x3, DCT_COEFF_PRECISION + SHIFT_Y );
		dest[ 3 ][ x ] = SCALE( DCT_COEFF_3 * x0 - DCT_COEFF_7 * x1 - DCT_COEFF_1 * x2 - DCT_COEFF_5 * x3, DCT_COEFF_PRECISION + SHIFT_Y );
		dest[ 5 ][ x ] = SCALE( DCT_COEFF_5 * x0 - DCT_COEFF_1 * x1 + DCT_COEFF_7 * x2 + DCT_COEFF_3 * x3, DCT_COEFF_PRECISION + SHIFT_Y );
		dest[ 7 ][ x ] = SCALE( DCT_COEFF_7 * x0 - DCT_COEFF_5 * x1 + DCT_COEFF_3 * x2 - DCT_COEFF_1 * x3, DCT_COEFF_PRECISION + SHIFT_Y );

		x0 = temp[ 0 ][ x ] + temp[ 7 ][ x ];
		x1 = temp[ 1 ][ x ] + temp[ 6 ][ x ];
		x2 = temp[ 2 ][ x ] + temp[ 5 ][ x ];
		x3 = temp[ 3 ][ x ] + temp[ 4 ][ x ];

		dest[ 0 ][ x ] = SCALE( x0 + x3 + x1 + x2, SHIFT_Y );
		dest[ 4 ][ x ] = SCALE( x0 + x3 - x1 - x2, SHIFT_Y );
		x0 -= x3;
		x1 -= x2;
		dest[ 2 ][ x ] = SCALE( DCT_COEFF_2 * x0 + DCT_COEFF_6 * x1, DCT_COEFF_PRECISION + SHIFT_Y );
		dest[ 6 ][ x ] = SCALE( DCT_COEFF_6 * x0 - DCT_COEFF_2 * x1, DCT_COEFF_PRECISION + SHIFT_Y );
	}
}


void PASCAL F248DCT_Y( short temp[][ 8 ], short dest[][ 8 ] )
{
	int	x, x0, x1, x2, x3;

	for( x = 0 ; x < 8 ; x++ )
	{
		x0 = temp[ 0 ][ x ] + temp[ 1 ][ x ];
		x1 = temp[ 2 ][ x ] + temp[ 3 ][ x ];
		x2 = temp[ 4 ][ x ] + temp[ 5 ][ x ];
		x3 = temp[ 6 ][ x ] + temp[ 7 ][ x ];

		dest[ 0 ][ x ] = SCALE( x0 + x3 + x1 + x2, SHIFT_Y );
		dest[ 2 ][ x ] = SCALE( x0 + x3 - x1 - x2, SHIFT_Y );
		x0 -= x3;
		x1 -= x2;
		dest[ 1 ][ x ] = SCALE( DCT_COEFF_2 * x0 + DCT_COEFF_6 * x1, DCT_COEFF_PRECISION + SHIFT_Y );
		dest[ 3 ][ x ] = SCALE( DCT_COEFF_6 * x0 - DCT_COEFF_2 * x1, DCT_COEFF_PRECISION + SHIFT_Y );

		x0 = temp[ 0 ][ x ] - temp[ 1 ][ x ];
		x1 = temp[ 2 ][ x ] - temp[ 3 ][ x ];
		x2 = temp[ 4 ][ x ] - temp[ 5 ][ x ];
		x3 = temp[ 6 ][ x ] - temp[ 7 ][ x ];

		dest[ 4 ][ x ] = SCALE( x0 + x3 + x1 + x2, SHIFT_Y );
		dest[ 6 ][ x ] = SCALE( x0 + x3 - x1 - x2, SHIFT_Y );
		x0 -= x3;
		x1 -= x2;
		dest[ 5 ][ x ] = SCALE( DCT_COEFF_2 * x0 + DCT_COEFF_6 * x1, DCT_COEFF_PRECISION + SHIFT_Y );
		dest[ 7 ][ x ] = SCALE( DCT_COEFF_6 * x0 - DCT_COEFF_2 * x1, DCT_COEFF_PRECISION + SHIFT_Y );
	}
}


void PASCAL ForwardDCT248( short src[][ 32 ], short dest[][ 8 ] )
{
	short	temp[ 8 ][ 8 ];

	FDCT_X( src, temp );
	F248DCT_Y( temp, dest );
}



void PASCAL ForwardDCT88( short src[][ 32 ], short dest[][ 8 ] )
{
	short	temp[ 8 ][ 8 ];

	FDCT_X( src, temp );
	F88DCT_Y( temp, dest );
}


