#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <bitflinger.h>

#include "Bitflinger.h"
#include "Cache.h"
#include "Cores.h"

#ifdef __cplusplus
extern "C" {
#endif
void _init();
void _fini();
#ifdef __cplusplus
}
#endif

/*
void _init()
{
	printf( "init \n ");
}

void _fini()
{
	printf( "uninit \n ");
}
*/

static void cvInit()
{
	__cvMakeIntTables();
	__cvMakeMMXTables();
}

static void cvShutdown()
{
}

static void resetCache( cvContext *con )
{
	con->exCurrent = -1;
	con->stripCurrent = -1;
}

void * cvCreateContext()
{
	void *alloc;
	cvContext *con;
	
	alloc = malloc( sizeof(cvContext) +15 );
	if( !alloc )
		return 0;
		
	con = (cvContext *)((((uint32)alloc) + 15) & 0xfffffff0 );
	memset( con, 0, sizeof( cvContext ));
	con->alloc = alloc;

	con->transferMode.ScaleR = 1.0;
	con->transferMode.ScaleG = 1.0;
	con->transferMode.ScaleB = 1.0;
	con->transferMode.ScaleA = 1.0;
	con->transferMode.BiasR = 0.0;
	con->transferMode.BiasG = 0.0;
	con->transferMode.BiasB = 0.0;
	con->transferMode.BiasA = 0.0;
	con->transferMode.isSimple = 1;

	con->op.logicFunc = GL_COPY;

	cvCacheInit( con, &con->CacheExtractor );
	cvCacheInit( con, &con->CacheStrips );
	resetCache( con );
	
	return con;
}


void cvDestroyContext( void *context )
{
	int32 ct;
	cvContext *con = (cvContext *)context;

	for( ct=0; ct<10; ct++ )
	{
		if( con->pixelMap[ct].Map )
			free( con->pixelMap[ct].Map );
	}

	free( con->alloc );
}

status_t cvSetType( void *context, int32 dir, int32 type, int32 format, uint8 swap_endian, uint8 lsb_first )
{
	cvContext *con = (cvContext *)context;
	PixelMode *pmode = 0;
	
	switch( dir )
	{
	case B_BIT_IN:
//printf( "cvSetType  in   type =%x   format=%x \n", type, format );
		pmode = &con->in;
		break;
	case B_BIT_OUT:
//printf( "cvSetType  out  type =%x   format=%x \n", type, format );
		pmode = &con->out;
		break;
	};
	
	if( !pmode )
		return B_ERROR;
		
	switch( format )
	{
		case GL_COLOR_INDEX:
		//case GL_STENCIL_INDEX:
		//case GL_DEPTH_COMPONENT:
		case GL_RED:
		case GL_GREEN:
		case GL_BLUE:
		case GL_ALPHA:
		case GL_RGB:
		case GL_RGBA:
		case GL_LUMINANCE:
		case GL_LUMINANCE_ALPHA:
		case GL_BGR:
		case GL_BGRA:
		case GL_ABGR_EXT:
			break;
		default:
			return B_ERROR;
	}
		
	switch( type )
	{
		case GL_UNSIGNED_BYTE_3_3_2:
		case GL_UNSIGNED_BYTE_2_3_3_REV:
		case GL_UNSIGNED_SHORT_5_6_5:
		case GL_UNSIGNED_SHORT_5_6_5_REV:
		case GL_UNSIGNED_SHORT_4_4_4_4:
		case GL_UNSIGNED_SHORT_4_4_4_4_REV:
		case GL_UNSIGNED_SHORT_5_5_5_1:
		case GL_UNSIGNED_SHORT_1_5_5_5_REV:
		case GL_UNSIGNED_INT_8_8_8_8:
		case GL_UNSIGNED_INT_8_8_8_8_REV:
		case GL_UNSIGNED_INT_10_10_10_2:
		case GL_UNSIGNED_INT_2_10_10_10_REV:
		case GL_BYTE:
		case GL_UNSIGNED_BYTE:
		case GL_SHORT:
		case GL_UNSIGNED_SHORT:
		case GL_INT:
		case GL_UNSIGNED_INT:
		case GL_FLOAT:
		case GL_DOUBLE:
			break;
		default:
			return B_ERROR;
	}

	resetCache( con );
	
	pmode->Format = format;
	pmode->Type = type;

	pmode->isSigned = 0;
	
	pmode->Components = 0;
	pmode->R_Bits = 0;
	pmode->G_Bits = 0;
	pmode->B_Bits = 0;
	pmode->A_Bits = 0;
	pmode->R_HBit = 0;
	pmode->G_HBit = 0;
	pmode->B_HBit = 0;
	pmode->A_HBit = 0;
	pmode->bytes = 0;

	switch( type )
	{
		case GL_UNSIGNED_BYTE_3_3_2:
			if( format != GL_RGB )
				return B_ERROR;
			pmode->Components = 3;
			pmode->R_Bits = 3;
			pmode->G_Bits = 3;
			pmode->B_Bits = 2;
			pmode->R_HBit = 7;
			pmode->G_HBit = 4;
			pmode->B_HBit = 1;
			pmode->bytes = 1;
			break;
		case GL_UNSIGNED_BYTE_2_3_3_REV:
			if( format != GL_RGB )
				return B_ERROR;
			pmode->Components = 3;
			pmode->R_Bits = 3;
			pmode->G_Bits = 3;
			pmode->B_Bits = 2;
			pmode->R_HBit = 2;
			pmode->G_HBit = 5;
			pmode->B_HBit = 7;
			pmode->bytes = 1;
			break;
		case GL_UNSIGNED_SHORT_5_6_5:
			if( format != GL_RGB )
				return B_ERROR;
			pmode->Components = 3;
			pmode->R_Bits = 5;
			pmode->G_Bits = 6;
			pmode->B_Bits = 5;
			pmode->R_HBit = 15;
			pmode->G_HBit = 10;
			pmode->B_HBit = 4;
			pmode->bytes = 2;
			break;
		case GL_UNSIGNED_SHORT_5_6_5_REV:
			if( format != GL_RGB )
				return B_ERROR;
			pmode->Components = 3;
			pmode->R_Bits = 5;
			pmode->G_Bits = 6;
			pmode->B_Bits = 5;
			pmode->R_HBit = 4;
			pmode->G_HBit = 10;
			pmode->B_HBit = 15;
			pmode->bytes = 2;
			break;
		case GL_UNSIGNED_SHORT_4_4_4_4:
			pmode->Components = 4;	
			pmode->bytes = 2;
			pmode->R_Bits = 4;
			pmode->G_Bits = 4;
			pmode->B_Bits = 4;
			pmode->A_Bits = 4;
			switch( format )
			{
				case GL_RGBA:
					pmode->R_HBit = 15;
					pmode->G_HBit = 11;
					pmode->B_HBit = 7;
					pmode->A_HBit = 3;
					break;
				case GL_BGRA:
					pmode->R_HBit = 7;
					pmode->G_HBit = 11;
					pmode->B_HBit = 15;
					pmode->A_HBit = 3;
					break;
				default:
					return B_ERROR;
			}
			break;
		case GL_UNSIGNED_SHORT_4_4_4_4_REV:
			pmode->Components = 4;
			pmode->R_Bits = 4;
			pmode->G_Bits = 4;
			pmode->B_Bits = 4;
			pmode->A_Bits = 4;
			pmode->bytes = 2;
			switch( format )
			{
				case GL_RGBA:
					pmode->R_HBit = 3;
					pmode->G_HBit = 7;
					pmode->B_HBit = 11;
					pmode->A_HBit = 15;
					break;
				case GL_BGRA:
					pmode->R_HBit = 11;
					pmode->G_HBit = 7;
					pmode->B_HBit = 3;
					pmode->A_HBit = 15;
					break;
				default:
					return B_ERROR;
			}
			break;
		case GL_UNSIGNED_SHORT_5_5_5_1:
			pmode->Components = 4;
			pmode->R_Bits = 5;
			pmode->G_Bits = 5;
			pmode->B_Bits = 5;
			pmode->A_Bits = 1;
			pmode->bytes = 2;
			switch( format )
			{
				case GL_RGBA:
					pmode->R_HBit = 15;
					pmode->G_HBit = 10;
					pmode->B_HBit = 5;
					pmode->A_HBit = 0;
					break;
				case GL_BGRA:
					pmode->R_HBit = 5;
					pmode->G_HBit = 10;
					pmode->B_HBit = 15;
					pmode->A_HBit = 0;
					break;
				default:
					return B_ERROR;
			}
			break;
		case GL_UNSIGNED_SHORT_1_5_5_5_REV:
			pmode->Components = 4;
			pmode->R_Bits = 5;
			pmode->G_Bits = 5;
			pmode->B_Bits = 5;
			pmode->A_Bits = 1;
			pmode->bytes = 2;
			switch( format )
			{
				case GL_RGBA:
					pmode->R_HBit = 14;
					pmode->G_HBit = 9;
					pmode->B_HBit = 4;
					pmode->A_HBit = 15;
					break;
				case GL_BGRA:
					pmode->R_HBit = 4;
					pmode->G_HBit = 9;
					pmode->B_HBit = 14;
					pmode->A_HBit = 15;
					break;
				default:
					return B_ERROR;
			}
			break;
		case GL_UNSIGNED_INT_8_8_8_8:
			pmode->Components = 4;
			pmode->R_Bits = 8;
			pmode->G_Bits = 8;
			pmode->B_Bits = 8;
			pmode->A_Bits = 8;
			pmode->bytes = 4;
			switch( format )
			{
				case GL_RGBA:
					pmode->R_HBit = 31;
					pmode->G_HBit = 23;
					pmode->B_HBit = 15;
					pmode->A_HBit = 7;
					break;
				case GL_BGRA:
					pmode->R_HBit = 15;
					pmode->G_HBit = 23;
					pmode->B_HBit = 31;
					pmode->A_HBit = 7;
					break;
				default:
					return B_ERROR;
			}
			break;
		case GL_UNSIGNED_INT_8_8_8_8_REV:
			pmode->Components = 4;
			pmode->R_Bits = 8;
			pmode->G_Bits = 8;
			pmode->B_Bits = 8;
			pmode->A_Bits = 8;
			pmode->bytes = 4;
			switch( format )
			{
				case GL_RGBA:
					pmode->R_HBit = 7;
					pmode->G_HBit = 15;
					pmode->B_HBit = 23;
					pmode->A_HBit = 31;
					break;
				case GL_BGRA:
					pmode->R_HBit = 23;
					pmode->G_HBit = 15;
					pmode->B_HBit = 7;
					pmode->A_HBit = 31;
					break;
				default:
					return B_ERROR;
			}
			break;
		case GL_UNSIGNED_INT_10_10_10_2:
			pmode->Components = 4;
			pmode->R_Bits = 10;
			pmode->G_Bits = 10;
			pmode->B_Bits = 10;
			pmode->A_Bits = 2;
			pmode->bytes = 4;
			switch( format )
			{
				case GL_RGBA:
					pmode->R_HBit = 31;
					pmode->G_HBit = 21;
					pmode->B_HBit = 11;
					pmode->A_HBit = 1;
					break;
				case GL_BGRA:
					pmode->R_HBit = 11;
					pmode->G_HBit = 21;
					pmode->B_HBit = 31;
					pmode->A_HBit = 1;
					break;
				default:
					return B_ERROR;
			}
			break;
		case GL_UNSIGNED_INT_2_10_10_10_REV:
			pmode->Components = 4;
			pmode->R_Bits = 10;
			pmode->G_Bits = 10;
			pmode->B_Bits = 10;
			pmode->A_Bits = 2;
			pmode->bytes = 4;
			switch( format )
			{
				case GL_RGBA:
					pmode->R_HBit = 9;
					pmode->G_HBit = 19;
					pmode->B_HBit = 29;
					pmode->A_HBit = 31;
					break;
				case GL_BGRA:
					pmode->R_HBit = 29;
					pmode->G_HBit = 19;
					pmode->B_HBit = 9;
					pmode->A_HBit = 31;
					break;
				default:
					return B_ERROR;
			}
			break;

		case GL_BYTE:
			pmode->isSigned = 1;
		case GL_UNSIGNED_BYTE:
		{
			switch( format )
			{
				case GL_RGBA:
					pmode->Components = 4;
					pmode->R_Bits = 8;
					pmode->G_Bits = 8;
					pmode->B_Bits = 8;
					pmode->A_Bits = 8;
					pmode->R_HBit = 7;
					pmode->G_HBit = 15;
					pmode->B_HBit = 23;
					pmode->A_HBit = 31;
					pmode->bytes = 4;
					break;
				case GL_BGRA:
					pmode->Components = 4;
					pmode->R_Bits = 8;
					pmode->G_Bits = 8;
					pmode->B_Bits = 8;
					pmode->A_Bits = 8;
					pmode->R_HBit = 23;
					pmode->G_HBit = 15;
					pmode->B_HBit = 7;
					pmode->A_HBit = 31;
					pmode->bytes = 4;
					break;
				case GL_ABGR_EXT:
					pmode->Components = 4;
					pmode->R_Bits = 8;
					pmode->G_Bits = 8;
					pmode->B_Bits = 8;
					pmode->A_Bits = 8;
					pmode->R_HBit = 31;
					pmode->G_HBit = 23;
					pmode->B_HBit = 15;
					pmode->A_HBit = 7;
					pmode->bytes = 4;
					break;
				case GL_RGB:
					pmode->Components = 3;
					pmode->R_Bits = 8;
					pmode->G_Bits = 8;
					pmode->B_Bits = 8;
					pmode->R_HBit = 7;
					pmode->G_HBit = 15;
					pmode->B_HBit = 23;
					pmode->bytes = 3;
					break;
				case GL_BGR:
					pmode->Components = 3;
					pmode->R_Bits = 8;
					pmode->G_Bits = 8;
					pmode->B_Bits = 8;
					pmode->R_HBit = 23;
					pmode->G_HBit = 15;
					pmode->B_HBit = 7;
					pmode->bytes = 3;
					break;
				case GL_INTENSITY:
					if( dir == B_BIT_IN )
					{
						pmode->A_Bits = 8;
						pmode->A_HBit = 7;
					}
				case GL_LUMINANCE:
					if( dir == B_BIT_IN )
					{
						pmode->G_Bits = 8;
						pmode->G_HBit = 7;
						pmode->B_Bits = 8;
						pmode->B_HBit = 7;
					}
				case GL_RED:
					pmode->Components = 1;
					pmode->R_Bits = 8;
					pmode->R_HBit = 7;
					pmode->bytes = 1;
					break;
				case GL_GREEN:
					pmode->Components = 1;
					pmode->G_Bits = 8;
					pmode->G_HBit = 7;
					pmode->bytes = 1;
					break;
				case GL_BLUE:
					pmode->Components = 1;
					pmode->B_Bits = 8;
					pmode->B_HBit = 7;
					pmode->bytes = 1;
					break;
				case GL_ALPHA:
					pmode->Components = 1;
					pmode->A_Bits = 8;
					pmode->A_HBit = 7;
					pmode->bytes = 1;
					break;
				case GL_LUMINANCE_ALPHA:
					pmode->Components = 2;
					if( dir == B_BIT_IN )
					{
						pmode->G_Bits = 8;
						pmode->G_HBit = 15;
						pmode->B_Bits = 8;
						pmode->B_HBit = 15;
					}
					pmode->R_Bits = 8;
					pmode->R_HBit = 15;
					pmode->A_Bits = 8;
					pmode->A_HBit = 7;
					pmode->bytes = 2;
					break;
				default:
					return B_ERROR;
			}
			break;
		}

		case GL_SHORT:
			pmode->isSigned = 1;
		case GL_UNSIGNED_SHORT:
		{
			switch( format )
			{
				case GL_RGBA:
					pmode->Components = 4;
					pmode->R_Bits = 16;
					pmode->G_Bits = 16;
					pmode->B_Bits = 16;
					pmode->A_Bits = 16;
					pmode->R_HBit = 15;
					pmode->G_HBit = 31;
					pmode->B_HBit = 47;
					pmode->A_HBit = 63;
					pmode->bytes = 8;
					break;
				case GL_ABGR_EXT:
					pmode->Components = 4;
					pmode->R_Bits = 16;
					pmode->G_Bits = 16;
					pmode->B_Bits = 16;
					pmode->A_Bits = 16;
					pmode->R_HBit = 63;
					pmode->G_HBit = 47;
					pmode->B_HBit = 31;
					pmode->A_HBit = 15;
					pmode->bytes = 8;
					break;
				case GL_RGB:
					pmode->Components = 3;
					pmode->R_Bits = 16;
					pmode->G_Bits = 16;
					pmode->B_Bits = 16;
					pmode->R_HBit = 15;
					pmode->G_HBit = 31;
					pmode->B_HBit = 47;
					pmode->bytes = 6;
					break;
				case GL_INTENSITY:
					if( dir == B_BIT_IN )
					{
						pmode->A_Bits = 16;
						pmode->A_HBit = 15;
					}
				case GL_LUMINANCE:
					if( dir == B_BIT_IN )
					{
						pmode->G_Bits = 16;
						pmode->G_HBit = 15;
						pmode->B_Bits = 16;
						pmode->B_HBit = 15;
					}
				case GL_RED:
					pmode->Components = 1;
					pmode->R_Bits = 16;
					pmode->R_HBit = 15;
					pmode->bytes = 2;
					break;
				case GL_GREEN:
					pmode->Components = 1;
					pmode->G_Bits = 16;
					pmode->G_HBit = 15;
					pmode->bytes = 2;
					break;
				case GL_BLUE:
					pmode->Components = 1;
					pmode->B_Bits = 16;
					pmode->B_HBit = 15;
					pmode->bytes = 2;
					break;
				case GL_ALPHA:
					pmode->Components = 1;
					pmode->A_Bits = 16;
					pmode->A_HBit = 15;
					pmode->bytes = 2;
					break;
				case GL_LUMINANCE_ALPHA:
					pmode->Components = 2;
					if( dir == B_BIT_IN )
					{
						pmode->G_Bits = 16;
						pmode->G_HBit = 31;
						pmode->B_Bits = 16;
						pmode->B_HBit = 31;
					}
					pmode->R_Bits = 16;
					pmode->R_HBit = 31;
					pmode->A_Bits = 16;
					pmode->A_HBit = 15;
					pmode->bytes = 4;
					break;
				default:
					return B_ERROR;
			}
			break;
		}
		
		case GL_INT:
			pmode->isSigned = 1;
		case GL_FLOAT:		// This is not marks as signed because it doesn't need special treatment.
		case GL_UNSIGNED_INT:
		{
			switch( format )
			{
				case GL_RGBA:
					pmode->Components = 4;
					pmode->R_Bits = 32;
					pmode->G_Bits = 32;
					pmode->B_Bits = 32;
					pmode->A_Bits = 32;
					pmode->R_HBit = 31;
					pmode->G_HBit = 63;
					pmode->B_HBit = 95;
					pmode->A_HBit = 127;
					pmode->bytes = 16;
					break;
				case GL_ABGR_EXT:
					pmode->Components = 4;
					pmode->R_Bits = 32;
					pmode->G_Bits = 32;
					pmode->B_Bits = 32;
					pmode->A_Bits = 32;
					pmode->R_HBit = 127;
					pmode->G_HBit = 95;
					pmode->B_HBit = 63;
					pmode->A_HBit = 31;
					pmode->bytes = 16;
					break;
				case GL_RGB:
					pmode->Components = 3;
					pmode->R_Bits = 32;
					pmode->G_Bits = 32;
					pmode->B_Bits = 32;
					pmode->R_HBit = 31;
					pmode->G_HBit = 63;
					pmode->B_HBit = 95;
					pmode->bytes = 12;
					break;
				case GL_INTENSITY:
					if( dir == B_BIT_IN )
					{
						pmode->A_Bits = 32;
						pmode->A_HBit = 31;
					}
				case GL_LUMINANCE:
					if( dir == B_BIT_IN )
					{
						pmode->G_Bits = 32;
						pmode->G_HBit = 31;
						pmode->B_Bits = 32;
						pmode->B_HBit = 31;
					}
				case GL_RED:
					pmode->Components = 1;
					pmode->R_Bits = 32;
					pmode->R_HBit = 31;
					pmode->bytes = 4;
					break;
				case GL_GREEN:
					pmode->Components = 1;
					pmode->G_Bits = 32;
					pmode->G_HBit = 31;
					pmode->bytes = 4;
					break;
				case GL_BLUE:
					pmode->Components = 1;
					pmode->B_Bits = 32;
					pmode->B_HBit = 31;
					pmode->bytes = 4;
					break;
				case GL_ALPHA:
					pmode->Components = 1;
					pmode->A_Bits = 32;
					pmode->A_HBit = 31;
					pmode->bytes = 4;
					break;
				case GL_LUMINANCE_ALPHA:
					pmode->Components = 2;
					if( dir == B_BIT_IN )
					{
						pmode->G_Bits = 32;
						pmode->G_HBit = 63;
						pmode->B_Bits = 32;
						pmode->B_HBit = 63;
					}
					pmode->R_Bits = 32;
					pmode->R_HBit = 63;
					pmode->A_Bits = 32;
					pmode->A_HBit = 31;
					pmode->bytes = 8;
					break;
				default:
					return B_ERROR;
			}
			break;
		}
		default:
			return B_ERROR;
	}
	
	return B_OK;
}

status_t cvSetTransferParam( void *context, int32 name, float value)
{
	cvContext *con = (cvContext *)context;

	resetCache( con );
//printf( "cvSetTransferParam %x  %f \n", name, value );

	switch (name)
	{
	case GL_RED_SCALE:
		con->transferMode.ScaleR = value;
		break;
	case GL_GREEN_SCALE:
		con->transferMode.ScaleG = value;
		break;
	case GL_BLUE_SCALE:
		con->transferMode.ScaleB = value;
		break;
	case GL_ALPHA_SCALE:
		con->transferMode.ScaleA = value;
		break;
	case GL_DEPTH_SCALE:
		con->transferMode.ScaleD = value;
		break;
	case GL_RED_BIAS:
		con->transferMode.BiasR = value;
		break;
	case GL_GREEN_BIAS:
		con->transferMode.BiasG = value;
		break;
	case GL_BLUE_BIAS:
		con->transferMode.BiasB = value;
		break;
	case GL_ALPHA_BIAS:
		con->transferMode.BiasA = value;
		break;
	case GL_DEPTH_BIAS:
		con->transferMode.BiasD = value;
		break;
	case GL_INDEX_SHIFT:
		con->transferMode.IndexShift = (int32) value;
		break;
	case GL_INDEX_OFFSET:
		con->transferMode.IndexOffset = (int32) value;
		break;
	case GL_MAP_COLOR:
		con->transferMode.MapColor = (value != 0);
		break;
	case GL_MAP_STENCIL:
		con->transferMode.MapStencil = (value != 0);
		break;
	default:
		return B_BAD_VALUE;
	}

	if( (fabs(con->transferMode.ScaleR - 1.0) < 0.0001) &&
		(fabs(con->transferMode.ScaleG - 1.0) < 0.0001) &&
		(fabs(con->transferMode.ScaleB - 1.0) < 0.0001) &&
		(fabs(con->transferMode.ScaleA - 1.0) < 0.0001) &&
		(fabs(con->transferMode.BiasR) < 0.0001) &&
		(fabs(con->transferMode.BiasG) < 0.0001) &&
		(fabs(con->transferMode.BiasB) < 0.0001) &&
		(fabs(con->transferMode.BiasA) < 0.0001) &&
		(con->transferMode.MapColor) )
	{
		con->transferMode.isSimple = 1;
	}
	else
	{
		con->transferMode.isSimple = 0;
	}

	return B_OK;
}

status_t cvSetMapUIV( void *context, uint32 map, uint32 mapSize, uint32 *values )
{
	cvContext *con = (cvContext *)context;
	int32 index = map - GL_PIXEL_MAP_I_TO_I;

	if (mapSize < 1)
		return B_BAD_VALUE;

	resetCache( con );
	
	switch (map)
	{
	case GL_PIXEL_MAP_I_TO_I:
	case GL_PIXEL_MAP_S_TO_S:
		if ((mapSize & (mapSize - 1)))
		{
			/*
			   ** Maps indexed by color or stencil index must be sized
			   ** to a power of two.
			 */
			return B_BAD_VALUE;
		}
		
		if( con->pixelMap[index].Map )
		{
			free( con->pixelMap[index].Map );
			con->pixelMap[index].Map = 0;
		}
		
		con->pixelMap[index].Map = malloc ( mapSize * sizeof (GLint));
		if (!con->pixelMap[index].Map)
		{
			return B_ERROR;
		}
		
		con->pixelMap[index].Size = mapSize;
		while (--mapSize > 0)
		{
			((GLint *)con->pixelMap[index].Map)[mapSize] = values[mapSize];
		}
		break;
		
	case GL_PIXEL_MAP_I_TO_R:
	case GL_PIXEL_MAP_I_TO_G:
	case GL_PIXEL_MAP_I_TO_B:
	case GL_PIXEL_MAP_I_TO_A:
		if ((mapSize & (mapSize - 1)))
		{
			/*
			   ** Maps indexed by color or stencil index must be sized
			   ** to a power of two.
			 */
			return B_ERROR;
		}
		// Fallthrough is intentional.	
	
	case GL_PIXEL_MAP_R_TO_R:
	case GL_PIXEL_MAP_G_TO_G:
	case GL_PIXEL_MAP_B_TO_B:
	case GL_PIXEL_MAP_A_TO_A:
		if( con->pixelMap[index].Map )
		{
			free( con->pixelMap[index].Map );
			con->pixelMap[index].Map = 0;
		}
		
		con->pixelMap[index].Map = malloc ( mapSize * sizeof (float));
		if (!con->pixelMap[index].Map)
		{
			return B_ERROR;
		}
		
		con->pixelMap[index].Size = mapSize;
		do
		{
			mapSize--;
			((float *)con->pixelMap[index].Map) [mapSize] = ((float)values[mapSize]) / (double) 4294965000u;
		} while (mapSize);
		break;
		
	default:
		return B_ERROR;
	}

	return B_OK;
}

status_t cvSetMapUSV( void *context, uint32 map, uint32 mapSize, uint16 *values )
{
	cvContext *con = (cvContext *)context;
	int32 index = map - GL_PIXEL_MAP_I_TO_I;

	if (mapSize < 1)
		return B_BAD_VALUE;

	resetCache( con );
	
	switch (map)
	{
	case GL_PIXEL_MAP_I_TO_I:
	case GL_PIXEL_MAP_S_TO_S:
		if ((mapSize & (mapSize - 1)))
		{
			/*
			   ** Maps indexed by color or stencil index must be sized
			   ** to a power of two.
			 */
			return B_BAD_VALUE;
		}
		
		if( con->pixelMap[index].Map )
		{
			free( con->pixelMap[index].Map );
			con->pixelMap[index].Map = 0;
		}
		
		con->pixelMap[index].Map = malloc ( mapSize * sizeof (GLint));
		if (!con->pixelMap[index].Map)
		{
			return B_ERROR;
		}
		
		con->pixelMap[index].Size = mapSize;
		while (--mapSize > 0)
		{
			((GLint *)con->pixelMap[index].Map)[mapSize] = values[mapSize];
		}
		break;
		
	case GL_PIXEL_MAP_I_TO_R:
	case GL_PIXEL_MAP_I_TO_G:
	case GL_PIXEL_MAP_I_TO_B:
	case GL_PIXEL_MAP_I_TO_A:
		if ((mapSize & (mapSize - 1)))
		{
			/*
			   ** Maps indexed by color or stencil index must be sized
			   ** to a power of two.
			 */
			return B_ERROR;
		}
	case GL_PIXEL_MAP_R_TO_R:
	case GL_PIXEL_MAP_G_TO_G:
	case GL_PIXEL_MAP_B_TO_B:
	case GL_PIXEL_MAP_A_TO_A:
		if( con->pixelMap[index].Map )
		{
			free( con->pixelMap[index].Map );
			con->pixelMap[index].Map = 0;
		}
		
		con->pixelMap[index].Map = malloc ( mapSize * sizeof (float));
		if (!con->pixelMap[index].Map)
		{
			return B_ERROR;
		}
		
		con->pixelMap[index].Size = mapSize;
		do
		{
			mapSize--;
			((float *)con->pixelMap[index].Map)[mapSize] = values[mapSize] / 65535.0;
		} while (mapSize);
		break;
		
	default:
		return B_ERROR;
	}

	return B_OK;
}

status_t cvSetMapFV( void *context, uint32 map, uint32 mapSize, float *values )
{
	cvContext *con = (cvContext *)context;
	int32 index = map - GL_PIXEL_MAP_I_TO_I;

	if (mapSize < 1)
		return B_BAD_VALUE;

	resetCache( con );
	
	switch (map)
	{
	case GL_PIXEL_MAP_I_TO_I:
	case GL_PIXEL_MAP_S_TO_S:
		if ((mapSize & (mapSize - 1)))
		{
			/*
			   ** Maps indexed by color or stencil index must be sized
			   ** to a power of two.
			 */
			return B_BAD_VALUE;
		}
		
		if( con->pixelMap[index].Map )
		{
			free( con->pixelMap[index].Map );
			con->pixelMap[index].Map = 0;
		}
		
		con->pixelMap[index].Map = malloc ( mapSize * sizeof (GLint));
		if (!con->pixelMap[index].Map)
		{
			return B_ERROR;
		}
		
		con->pixelMap[index].Size = mapSize;
		while (--mapSize > 0)
		{
			((GLint *)con->pixelMap[index].Map)[mapSize] = (GLint) (0.5 + values[mapSize]);
		}
		break;
		
	case GL_PIXEL_MAP_I_TO_R:
	case GL_PIXEL_MAP_I_TO_G:
	case GL_PIXEL_MAP_I_TO_B:
	case GL_PIXEL_MAP_I_TO_A:
		if ((mapSize & (mapSize - 1)))
		{
			/*
			   ** Maps indexed by color or stencil index must be sized
			   ** to a power of two.
			 */
			return B_ERROR;
		}
	case GL_PIXEL_MAP_R_TO_R:
	case GL_PIXEL_MAP_G_TO_G:
	case GL_PIXEL_MAP_B_TO_B:
	case GL_PIXEL_MAP_A_TO_A:
		if( con->pixelMap[index].Map )
		{
			free( con->pixelMap[index].Map );
			con->pixelMap[index].Map = 0;
		}
		
		con->pixelMap[index].Map = malloc ( mapSize * sizeof (float));
		if (!con->pixelMap[index].Map)
		{
			return B_ERROR;
		}
		
		con->pixelMap[index].Size = mapSize;
		
		do
		{
			mapSize--;
			((float *)con->pixelMap[index].Map)[mapSize] = values[mapSize];
		} while (mapSize);
		break;
		
	default:
		return B_ERROR;
	}

	return B_OK;
}


status_t cvSetCache( void *context, int32 dir, int32 c )
{
	cvContext *con = (cvContext *)context;

	switch( dir )
	{
	case B_BIT_IN:
	case B_BIT_OUT:
		break;
	default:
		return B_ERROR;
	}

	switch( c )
	{
	case B_BIT_CACHE_L1:
	case B_BIT_CACHE_L2:
	case B_BIT_CACHE_L3:
	case B_BIT_CACHE_RAM:
	case B_BIT_CACHE_WC:
	case B_BIT_CACHE_NC:
	case B_BIT_CACHE_PCI:
	case B_BIT_CACHE_PCI_WC:
		break;
	default:
		return B_ERROR;
	}

	switch( dir )
	{
	case B_BIT_IN:
		con->in.cache = c;
		break;
	case B_BIT_OUT:
		con->out.cache = c;
		break;
	}

	return B_OK;
}

status_t cvSetBlendFunction( void *context, uint32 src, uint32 dst )
{
	cvContext *con = (cvContext *)context;

	switch( src )
	{
	case GL_ZERO:
	case GL_ONE:
	case GL_DST_COLOR:
	case GL_ONE_MINUS_DST_COLOR:
	case GL_SRC_ALPHA:
	case GL_ONE_MINUS_SRC_ALPHA:
	case GL_DST_ALPHA:
	case GL_ONE_MINUS_DST_ALPHA:
	case GL_SRC_ALPHA_SATURATE:
	case GL_CONSTANT_COLOR:
	case GL_ONE_MINUS_CONSTANT_COLOR:
	case GL_CONSTANT_ALPHA:
	case GL_ONE_MINUS_CONSTANT_ALPHA:
		break;
	default:
		return B_ERROR;
	}

	switch( dst )
	{
	case GL_ZERO:
	case GL_ONE:
	case GL_SRC_COLOR:
	case GL_ONE_MINUS_SRC_COLOR:
	case GL_SRC_ALPHA:
	case GL_ONE_MINUS_SRC_ALPHA:
	case GL_DST_ALPHA:
	case GL_ONE_MINUS_DST_ALPHA:
	case GL_CONSTANT_COLOR:
	case GL_ONE_MINUS_CONSTANT_COLOR:
	case GL_CONSTANT_ALPHA:
	case GL_ONE_MINUS_CONSTANT_ALPHA:
		break;
	default:
		return B_ERROR;
	}

	con->op.blendSrcFunc = src;
	con->op.blendDstFunc = dst;
	
	return B_OK;
}

status_t cvSetBlendEquation( void *context, uint32 equ )
{
	cvContext *con = (cvContext *)context;

	switch( equ )
	{
	case GL_FUNC_ADD:
	case GL_FUNC_SUBTRACT:
	case GL_FUNC_REVERSE_SUBTRACT:
	case GL_MIN:
	case GL_MAX:
		break;
	default:
		return B_ERROR;
	}
	
	con->op.blendEqu = equ;
	return B_OK;
}

status_t cvSetBlendColor( void *context, float r, float g, float b, float a )
{
	cvContext *con = (cvContext *)context;

	con->op.blendR = r;	
	con->op.blendG = g;	
	con->op.blendB = b;	
	con->op.blendA = a;
	
	con->opt.blendConstColor.r = (uint16)(con->op.blendR * 0x1000);
	con->opt.blendConstColor.g = (uint16)(con->op.blendG * 0x1000);
	con->opt.blendConstColor.b = (uint16)(con->op.blendB * 0x1000);
	con->opt.blendConstColor.a = (uint16)(con->op.blendA * 0x1000);

	con->opt.blendConstAlpha.r = con->opt.blendConstColor.a;
	con->opt.blendConstAlpha.g = con->opt.blendConstColor.a;
	con->opt.blendConstAlpha.b = con->opt.blendConstColor.a;
	con->opt.blendConstAlpha.a = con->opt.blendConstColor.a;

	con->opt.oneMinusBlendConstColor.r = (uint16)((1 - con->op.blendR) * 0x1000);
	con->opt.oneMinusBlendConstColor.g = (uint16)((1 - con->op.blendG) * 0x1000);
	con->opt.oneMinusBlendConstColor.b = (uint16)((1 - con->op.blendB) * 0x1000);
	con->opt.oneMinusBlendConstColor.a = (uint16)((1 - con->op.blendA) * 0x1000);

	con->opt.oneMinusBlendConstAlpha.r = con->opt.oneMinusBlendConstColor.a;
	con->opt.oneMinusBlendConstAlpha.g = con->opt.oneMinusBlendConstColor.a;
	con->opt.oneMinusBlendConstAlpha.b = con->opt.oneMinusBlendConstColor.a;
	con->opt.oneMinusBlendConstAlpha.a = con->opt.oneMinusBlendConstColor.a;
	
	return B_OK;
}

status_t cvSetWriteMask( void *context, int8 r, int8 g, int8 b, int8 a )
{
	cvContext *con = (cvContext *)context;

	con->op.writeR = r != 0;
	con->op.writeG = g != 0;
	con->op.writeB = b != 0;
	con->op.writeA = a != 0;

	return B_OK;
}

status_t cvSetLogicOp( void *context, uint32 op )
{
	cvContext *con = (cvContext *)context;
	
	switch( op )
	{
	case GL_CLEAR:
	case GL_SET:
	case GL_COPY:
	case GL_COPY_INVERTED:
	case GL_NOOP:
	case GL_INVERT:
	case GL_AND:
	case GL_NAND:
	case GL_OR:
	case GL_NOR:
	case GL_XOR:
	case GL_EQUIV:
	case GL_AND_REVERSE:
	case GL_AND_INVERTED:
	case GL_OR_REVERSE:
	case GL_OR_INVERTED:
		break;
	default:
		return B_ERROR;
	}
	
	con->op.logicFunc = op;
	
	return B_OK;
}

status_t cvSetEnvMode( void *context, uint32 mode )
{
	cvContext *con = (cvContext *)context;
	
	switch( mode )
	{
	case GL_MODULATE:
	case GL_REPLACE:
	case GL_DECAL:
	case GL_BLEND:
		break;
	default:
		return B_ERROR;
	}
	
	con->op.envMode = mode;
	
	return B_OK;
}

status_t cvSetEnvColor( void *context, float r, float g, float b, float a )
{
	cvContext *con = (cvContext *)context;
	
	con->op.envR = r;
	con->op.envG = g;
	con->op.envB = b;
	con->op.envA = a;
	
	con->opt.envColor.r = (uint16)(con->op.envR * 0x1000);
	con->opt.envColor.g = (uint16)(con->op.envG * 0x1000);
	con->opt.envColor.b = (uint16)(con->op.envB * 0x1000);
	con->opt.envColor.a = (uint16)(con->op.envA * 0x1000);
	
	return B_OK;
}

status_t cvSetEnvConstColor( void *context, float r, float g, float b, float a )
{
	cvContext *con = (cvContext *)context;
	
	con->op.envConstR = r;
	con->op.envConstG = g;
	con->op.envConstB = b;
	con->op.envConstA = a;

	con->opt.envConstColor.r = (uint16)(con->op.envConstR * 0x1000);
	con->opt.envConstColor.g = (uint16)(con->op.envConstG * 0x1000);
	con->opt.envConstColor.b = (uint16)(con->op.envConstB * 0x1000);
	con->opt.envConstColor.a = (uint16)(con->op.envConstA * 0x1000);
	
	return B_OK;
}



class AutoInit
{
private:
	AutoInit();
	~AutoInit();
	
	static AutoInit m_theOne;
};

AutoInit  AutoInit::m_theOne;

AutoInit::AutoInit()
{
	cvInit();
}

AutoInit::~AutoInit()
{
	cvShutdown();
}
