#include <OS.h>
#include <KernelExport.h>

#include <add-ons/graphics/Accelerant.h>

#include <graphics_p/radeon/defines.h>
#include <graphics_p/radeon/main.h>
#include <graphics_p/radeon/regdef.h>
#include <graphics_p/radeon/CardState.h>
#include <graphics_p/radeon/radeon_ioctls.h>

#include "cursor.h"




extern CardInfo *ci;
extern __mem_AreaDef *primMemMgr;


status_t cursorCreate()
{
	cursorDestroy();
	cursorShow( 0 );

	ci->AllocCursor = __mem_Allocate( primMemMgr, 1024 );
	if( !ci->AllocCursor )
		return B_ERROR;
	ci->AllocCursor->locked = 255;

	ci->Finish(ci);
	Radeon_WaitForFifo( ci, 4 );
	WRITE_REG_32( CUR_OFFSET, ci->AllocCursor->address );
	WRITE_REG_32( CUR_CLR0, 0xffffff );
	WRITE_REG_32( CUR_CLR1, 0 );

	return B_OK;		
}

uint16 hotx, hoty;

void cursorLoad ( uint16 curswide, uint16 curshigh, uint16 hot_x, uint16 hot_y, uint8 *andbits, uint8 *xorbits )
{
	register int		x, y;
	register uint8		*curs;
	uint32				dstoff;
	int					sbpr;
	
	curs = ci->CardMemory + ci->AllocCursor->address;
	
	hotx = hot_x;
	hoty = hot_y;

	/*
	 * Layout of 64x64 2-bit AND/XOR cursors has been empirically
	 * discovered as follows:
	 *
	 * Memory
	 * Offsets	Function
	 * -----------	--------
	 * 0x00 - 0x07	Line 0, plane 0 (AND)
	 * 0x08 - 0x0f	Line 0, plane 1 (XOR)
	 * 0x10 - 0x17	Line 1, plane 0 (AND)
	 * 0x18 - 0x1f	Line 1, plane 1 (XOR)
	 * 0x20 - 0x27	Line 2, plane 0 (AND)
	 * 0x28 - 0x2f	Line 2, plane 1 (XOR)
	 * 0x30 - 0x37	Line 3, plane 0 (AND)
	 * 0x38 - 0x3f	Line 3, plane 1 (XOR)
	 *  etc.
	 *
	 * Pixels within the cursor area appear as follows:
	 *
	 * AND  XOR	Appearance
	 * ---  ---	----------
	 *  0    0	Solid color; hwCurC0
	 *  0    1	Solid color; hwCurC1
	 *  1    0	Transparent (current Screen Color)
	 *  1    1	Invert underlying framebuffer pixel (NOT Screen Color)
	 */

	sbpr = 0;	
	for(y=0; y < 64; y++)
	{
		for(x=0; x < 8; x++)
		{
			if(x < ((curswide + 7) / 8) && y < curshigh)
			{
				curs[y * 16 + x    ] = *(andbits + sbpr);
				curs[y * 16 + x + 8] = *(xorbits + sbpr);
				sbpr++;
			}
			else
			{
				curs[y * 16 + x    ] = 0xff;
				curs[y * 16 + x + 8] = 0x00;
			}
		}
	}
}

void cursorMove( uint16 x, uint16 y )
{
	int32 previous;
	int32 ix, iy;


	ix = x - hotx;
	iy = y - hoty;
	
	previous = atomic_add( &ci->benEngineInt, 1 );
	if( previous >= 1 )
		acquire_sem( ci->benEngineSem );

	if( (ix>=0) && (iy>=0) )
	{
		Radeon_WriteRegFifo( ci, CUR_HORZ_VERT_OFF, 0 );
		Radeon_WriteRegFifo( ci, CUR_HORZ_VERT_POSN, (iy & 0xfff) | ((ix & 0xfff)<<16) );
		Radeon_WriteRegFifo( ci, CUR_OFFSET, ci->AllocCursor->address );
	}
	else
	{
		int32 nx =0;
		int32 ny =0;
		
		if( ix < 0 )
		{
			nx = -ix;
			ix = 0;
		}

		if( iy < 0 )
		{
			ny = -iy;
			iy = 0;
		}

		Radeon_WriteRegFifo( ci, CUR_HORZ_VERT_OFF, (ny & 0x3f) | ((nx & 0x3f)<<16) );
		Radeon_WriteRegFifo( ci, CUR_HORZ_VERT_POSN, (iy & 0xfff) | ((ix & 0xfff)<<16) );
		Radeon_WriteRegFifo( ci, CUR_OFFSET, &((uint8 *)ci->AllocCursor->address)[ny * 16] );
	}

	previous = atomic_add( &ci->benEngineInt, -1 );
	if( previous > 1 )
		release_sem( ci->benEngineSem );
}

void cursorShow( uint8 show )
{
	int32 previous;
	uint32 dat = READ_REG_32( CRTC_GEN_CNTL );
	
	if( show )
		dat |= CRTC_GEN_CNTL__CRTC_CUR_EN;
	else
		dat &= ~CRTC_GEN_CNTL__CRTC_CUR_EN;
		
	ci->showCursor = show;
	
	previous = atomic_add( &ci->benEngineInt, 1 );
	if( previous >= 1 )
		acquire_sem( ci->benEngineSem );

	Radeon_WriteRegFifo( ci, CRTC_GEN_CNTL, dat );

	previous = atomic_add( &ci->benEngineInt, -1 );
	if( previous > 1 )
		release_sem( ci->benEngineSem );
}

void cursorDestroy()
{
	cursorShow( 0 );
	if ( ci->AllocCursor )
	{
		__mem_Free( primMemMgr, ci->AllocCursor );
		ci->AllocCursor = 0;
	}
}
