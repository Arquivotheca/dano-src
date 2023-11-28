#include <GL/gl.h>
#include <malloc.h>

#include "mgl.h"
#include "stdio.h"
#include "fixed.h"


// Get a __mglScanPointsPool structure.  Allocate a new one if needed.
// Also clears the id codes to 0.
static __mglScanPoints * getScanPoints( __mglContext *con )
{
	int ct;
	__mglScanPoints *p;
	
	if( con->poly.pool_first )
	{
		p = con->poly.pool_first;
		con->poly.pool_first = p->next;
	}
	else
	{
		p = (__mglScanPoints *) malloc( sizeof(__mglScanPoints) );
	}

	for( ct=0; ct<8; ct++ )
		p->id[ct] = 0;
	p->next = 0;
	return p;
}

static void releaseScanPoints( __mglContext *con, __mglScanPoints *p )
{
	p->next = con->poly.pool_first;
	con->poly.pool_first = p;
}

static void addPoint( __mglContext *con, int32 x, int32 y1, int32 y2, int32 id )
{
	int32 y = y1 >> 16;
	int index;
	
	__mglScanPoints *p = con->poly.points_headers[y];
//printf( "addPoint id=%i   y1=%x  y2=%x \n", id, y1, y2 );
	
	if( !p )
	{
		p = getScanPoints( con );
		con->poly.points_headers[y] = p;
	}
	
//printf( "addPoint 1\n" );
	index = 0;
	while(1)
	{
//printf( "addPoint p=%p  index=%i \n", p, index );
		if( !p->id[index] )
			break;
		
		index++;
		if( index >= 8 )
		{
			__mglScanPoints *next = p->next;
			index = 0;
			
			if( !next )
			{
				next = getScanPoints( con );
				p->next = next;
			}
			p = next;
		}
	}
	
	p->coords_y1[index] = y1;
	p->coords_y2[index] = y2;
	p->coords_x[index] = x;
	p->id[index] = id;


	{
		uint32 *d = (uint32 *)con->color.data;
		d[y*con->color.width + (x>>16)] = 0xffffffff;
	}
}

static int32 calcSlope( int32 x1, int32 y1, int32 x2, int32 y2 )
{
	int32 dx = x2-x1;
	int32 dy = y2-y1;
	
	int32 dxdy = floatToFixed_16_16(((float)dx) / ((float)dy));
	return dxdy;
}


static void generateEdge( __mglContext *con, int32 x1, int32 y1, int32 x2, int32 y2, int32 id )
{
	int32 dxdy;
	int32 y;
	int32 x;
	int32 y2i;


	// Make sure we always generate in the same direction 
	if( y2 < y1 )
	{
		int t;
		
		t = y1;
		y1 = y2;
		y2 = t;

		t = x1;
		x1 = x2;
		x2 = t;
	}

	dxdy = calcSlope( x1, y1, x2, y2 );
	y = y1;
	x = x1;
	y2i = y2 & 0xffff0000;

//printf( "dxdy = %x \n", dxdy );
	
	// Check for fractional start
	if( y & 0xffff )
	{
		int32 yn = (y + 0x10000) & 0xffff0000;
		
		if( yn > y2 )
		{
			// We have a line that starts and stops on fractional y values within a scanline
			addPoint( con, x, y, y2, id );
		}
		else
		{
			// We start on a fractional value and continue into another line.
			addPoint( con, x, y, yn, id );
		}
		x += fixedMul_16_16( y & 0xffff, dxdy );
		y = yn;
	}

	// Fill in the complete points
	while( y < y2i )
	{
		addPoint( con, x, y, y+0x10000, id );
		
	
		x += dxdy;	
		y += 0x10000;
	}

	// Fill in the fractional end	
	if( y < y2 )
	{
		addPoint( con, x, y, y2, id );
	}
	
}


void __mglBeginRegion( __mglContext *con )
{
	con->poly.region_id = 1;
	
	if( con->poly.points_header_count < con->color.height )
	{
		if( con->poly.points_headers )
			free( con->poly.points_headers );
		con->poly.points_headers = malloc( con->color.height * sizeof( __mglScanPoints * ) );
		con->poly.points_header_count = con->color.height;
	}
}

static void sortScanline( __mglContext *con, int y )
{
	__mglScanPoints *ph = con->poly.points_headers[y];
	__mglScanPoints *p;
	int count=0;


	if( !ph )
		return;
	
//printf( "sortScanline ph=%p \n", ph );
	// Count them
	p = ph;
	while( p )
	{
		int index;
		for( index=0; index<8; index++ )
		{
			if( p->id[index] )
				count++;
			else
				break;
		}
		p = p->next;
	}
	
	// Sort them
	while( count-- )
	{
		int i1,i2;
		__mglScanPoints *p2;

		p = ph;
		p2 = p;
		i1 = 0;
		i2 = 1;

		while( p2 && p2->id[i2] )
		{
			int32 t;
//printf( "sortScanline p=%p p2=%p   i1=%i  i2=%i\n", p, p2, i1, i2 );
			if( p2->coords_x[i2] < p->coords_x[i1] )
			{
				t = p2->coords_x[i2];
				p2->coords_x[i2] = p->coords_x[i1];
				p->coords_x[i1] = t;
			}
			
			i1=i2;
			p = p2;
			if( (++i2) > 7 )
			{
				i2 = 0;
				p2 = p2->next;
			}
//printf( "sortScanline XX p=%p p2=%p   i1=%i  i2=%i\n", p, p2, i1, i2 );
		}


		
	}
//printf( "sortScanline done \n" );
	
}

static void drawScan( __mglContext *con, int32 x1, int32 y1, int32 x2, int32 y2 )
{
#if 0
	int y = y1 >> 16;
	int w = (x2>>16) - (x1>>16);
	uint32 *d = & ((uint32 *)con->color.data)[y*con->color.width + (x1>>16)];
	
	if( y2 & 0xffff )
		return;	// Must end in .0 to be rendered non AA
	
	while( w )
	{
		*d = 0x7f7f7f7f;
		d++;
		w--;
	}
#else
	con->shade.xLeftFixed = x1;
	con->shade.xRightFixed = x2;
	con->shade.yBottom = y1 >> 16;
	
	con->go_s( con );
#endif
}

void __mglEndRegion( __mglContext *con, bool compress )
{
	int y;
	
	for( y=0; y<con->color.height; y++ )
	{
		__mglScanPoints *p = con->poly.points_headers[y];
		int32 wind = 0;
			
		if( p )
		{
			int i1,i2;
			__mglScanPoints *p2;
	
			p2 = p;
			i1 = 0;
			i2 = 1;

			sortScanline( con, y );

			while( p2 && p2->id[i2] )
			{
				drawScan( con, p->coords_x[i1], p->coords_y1[i1], p2->coords_x[i2], p->coords_y2[i1] );
				
				i1=i2;
				p = p2;
				if( (++i2) > 7 )
				{
					i2 = 0;
					p2 = p2->next;
				}
			}
						
			
			p = con->poly.points_headers[y];
			while( p )
			{
				p2 = p->next;
				releaseScanPoints( con, p );
				p = p2;
			}
			con->poly.points_headers[y] = 0;
			
		}
	}

	// Lets fill it.
	
	
	
	
}


void __mglBeginPoly( __mglContext *con )
{
	con->poly.poly_id = 0;
}

void __mglEndPoly( __mglContext *con )
{
	generateEdge( con,
		con->poly.point_first_x, con->poly.point_first_y,
		con->poly.point_prev_x, con->poly.point_prev_y,
		con->poly.region_id );
}


void __mglPoint2x( __mglContext *con, int32 x, int32 y )
{
	if( con->poly.poly_id )
	{
		generateEdge( con, con->poly.point_prev_x, con->poly.point_prev_y, x, y, con->poly.region_id );
	}
	else
	{
		con->poly.point_first_x = x;
		con->poly.point_first_y = y;
	}
	
	con->poly.poly_id++;
	con->poly.region_id++;
	con->poly.point_prev_x = x;
	con->poly.point_prev_y = y;
}

void __mglPoint2f( __mglContext *con, float x, float y )
{
	__mglPoint2x( con, floatToFixed_16_16(x), floatToFixed_16_16(y) );
}


