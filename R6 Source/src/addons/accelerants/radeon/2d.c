// Much of this file comes from the rage128 driverv


#include <OS.h>
#include <KernelExport.h>

#include <graphics_p/video_overlay.h>

#include <graphics_p/radeon/defines.h>
#include <graphics_p/radeon/main.h>
#include <graphics_p/radeon/regdef.h>

#include <graphics_p/radeon/CardState.h>
#include <graphics_p/radeon/radeon_ioctls.h>

#include <add-ons/graphics/Accelerant.h>

#include "proto.h"
#include "cursor.h"

extern CardInfo *ci;
	
void Radeon_screen_to_screen_blits(engine_token *et, blit_params *list, uint32 count)
{
	int32 index = 0;
	uint32 buf[256];
	int32 i = 0;
	
//dprintf (("a"));
//dprintf(("Radeon_screen_to_screen_blits count=%ld \n", count ));
	while( index < count )
	{
		blit_params *bp = &list[index];

//dprintf(("Radeon_screen_to_screen_blits index=%ld \n", index ));
	
		if( !i )
		{
			// Write packet header
			buf[i++] = CCE_PACKET3_CNTL_BITBLT;
			buf[i++] = CCE_GC_BRUSH_NONE | CCE_GC_SRC_DSTCOLOR | ROP3_SRCCOPY |
					CCE_GC_DP_SRC_RECT | (ci->hwBppNum << 8);
		}

		buf[i++] = (bp->src_left << 16) + bp->src_top;
		buf[i++] = (bp->dest_left << 16) + bp->dest_top;
		buf[i++] = ((bp->width + 1) << 16) + (bp->height + 1);
		
		if( (i+7) > 256 )
		{
			// Overflow soon so submit and start over;
//dprintf(("Radeon_screen_to_screen_blits i=%ld  Radeon_CPSubmitPackets=%p\n", i, Radeon_CPSubmitPackets ));
			buf[0] |= ((i - 2) << 16);
			ci->CPSubmitPackets (ci, buf, i);
			i=0;
		}

		index++;
	}
	
	if( i )
	{
//dprintf(("Radeon_screen_to_screen_blits i=%ld  Radeon_CPSubmitPackets=%p\n", i, Radeon_CPSubmitPackets ));
		buf[0] |= ((i - 2) << 16);
		ci->CPSubmitPackets (ci, buf, i);
	}
	
	ci->primitivesIssued++;
//dprintf (("A"));
}

void Radeon_fill_rectangle(engine_token *et, uint32 color, fill_rect_params *list, uint32 count)
{
	int32 index = 0;
	uint32 buf[256];
	int32 i = 0;
	
//dprintf (("b"));
//dprintf(("Radeon_fill_rectangle count=%ld \n", count ));
	while( index < count )
	{
		fill_rect_params *frp = &list[index];

//dprintf(("Radeon_fill_rectangle index=%ld \n", index ));
	
		if( !i )
		{
			// Write packet header
			buf[i++] = CCE_PACKET3_CNTL_PAINT_MULTI;
			buf[i++] = CCE_GC_BRUSH_SOLIDCOLOR | CCE_GC_SRC_DSTCOLOR | ROP3_PATCOPY | (ci->hwBppNum << 8);
			buf[i++] = color;
		}

		buf[i++] = (frp->left << 16) + frp->top;
		buf[i++] = ((frp->right - frp->left +1) << 16) + (frp->bottom - frp->top +1);
		
		if( (i+7) > 256 )
		{
			// Overflow soon so submit and start over;
//dprintf(("Radeon_fill_rectangle i=%ld  Radeon_CPSubmitPackets=%p\n", i, Radeon_CPSubmitPackets ));
			buf[0] |= ((i - 2) << 16);
			ci->CPSubmitPackets (ci, buf, i);
			i=0;
		}

		index++;
	}
	
	if( i )
	{
//dprintf(("Radeon_fill_rectangle i=%ld  Radeon_CPSubmitPackets=%p\n", i, Radeon_CPSubmitPackets ));
		buf[0] |= ((i - 2) << 16);
		ci->CPSubmitPackets (ci, buf, i);
	}
	
	ci->primitivesIssued++;
//dprintf (("B"));
}
