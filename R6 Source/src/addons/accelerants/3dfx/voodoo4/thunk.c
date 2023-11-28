/* :ts=8 bk=0
 *
 * thunk.c:	Stub routines that thunk their functionality out to the
 *		kernel driver.
 *
 * $Id:$
 *
 * Leo L. Schwab					1998.08.20
 *
 * Copyright 1998 Be Incorporated.  Confidential unpublished work.
 * All Rights Reserved.
 */
#include <graphics_p/video_overlay.h>

#include <graphics_p/3dfx/voodoo4/voodoo4.h>
#include <graphics_p/3dfx/voodoo4/voodoo4_regs.h>
#include <graphics_p/3dfx/common/debug.h>

#include <math.h>
#include "protos.h"

#define TILE_WIDTH	64			/* Width of a tile (in Pixels) for Tiled Access mode */

/****************************************************************************
 * Globals
 */
extern thdfx_card_info	*ci;
extern int		devfd;

extern int memtypefd;

static int32 p6FenceVar[32];
#define P6FENCE __asm__ __volatile__ ( \
	"xchg %%eax, p6FenceVar \n" \
	"xchg %%eax, p6FenceVar+32 \n" \
	"xchg %%eax, p6FenceVar+64 \n" \
	"xchg %%eax, p6FenceVar+96 \n" \
	: : :"%eax" );

/****************************************************************************
 * This "thunk" routine is remarkable in that it has to reset the
 * acceleration state after changing modes.
 */
status_t _V5_set_display_mode (display_mode *mode_to_set)
{
	status_t		retval;
//	dprintf(("3dfx_v4_accel: _set_display_mode - ENTER\n"));
	
	/*
	 * We ask the driver to *NOT* release the rendering engine lock, as
	 * we will be dancing on it further.
	 */
	if ((retval = vid_selectmode (mode_to_set, LEAVELOCKF_ENGINE)) == B_OK)
	{
		memcpy (&ci->CurDispMode, mode_to_set,	sizeof (ci->CurDispMode));
		retval = AccelInit (ci);
		/*  Now we're really done; release the lock.  */
		unlockBena4 (&ci->EngineLock);
	}
	else
	{
		/*  We had an error so make sure we release the lock  */
		unlockBena4 (&ci->EngineLock);
	}

//	dprintf(("3dfx_v4_accel: _set_display_mode - EXIT, returning 0x%x\n", retval));
	return (retval);
}


/****************************************************************************
 * These are just straight thunks out to the kernel driver.
 */
uint32 _V5_get_accelerant_mode_count (void)
{
//dprintf(("3dfx_v4_accel: _get_accelerant_mode_count - EXIT returning %d\n", ci->NDispModes));
	return (ci->NDispModes);
}

status_t _V5_get_mode_list (display_mode *dm)
{
	status_t retval;
	
	/*  The size is wrong...  */
//dprintf(("3dfx_v4_accel: _get_mode_list - ENTER\n"));
		memcpy (dm, ci->DispModes,	ci->NDispModes * sizeof (display_mode));
//dprintf(("3dfx_v4_accel: _get_mode_list - EXIT - returning 0x%x\n", retval));
	return (retval);
}

status_t _V5_propose_display_mode (
display_mode *target,
display_mode *low,
display_mode *high
)
{
	status_t retval;
//	dprintf(("3dfx_v4_accel: _propose_display_mode - ENTER\n"));

	retval = propose_video_mode(target, low, high);
//	dprintf(("3dfx_v4_accel: _propose_display_mode - EXIT, returning 0x%x\n", retval));
	return (retval);
}

status_t _V5_get_frame_buffer_config (frame_buffer_config *a_frame_buffer)
{
	status_t	retval = B_OK;
	
//	dprintf(("3dfx_v4_accel: _get_frame_buffer_config -ENTER\n"));

		a_frame_buffer->frame_buffer = ci->FBBase;
		a_frame_buffer->frame_buffer_dma = ci->FBBase_DMA;
		a_frame_buffer->bytes_per_row = ci->BytesPerRow;
		
	return (retval);
}

status_t _V5_get_pixel_clock_limits (display_mode *dm, uint32 *low, uint32 *high)
{
	status_t		retval = B_OK;

//	dprintf(("3dfx_v4_accel: _get_pixel_clock_limits - ENTER\n"));

	/*
	 * Constrain low-end to 48 Hz, until a monitors database
	 * shows up...
	 */
	uint32 total_pix = dm->timing.h_total * dm->timing.v_total;
	*low = (total_pix * 48L) / 1000L;
	*high = CLOCK_MAX;

//	dprintf(("3dfx_v4_accel: _get_pixel_clock_limits - EXIT, returning 0x%x\n", retval));
	return (retval);
}

static void _set_palette_entry( uint32 entry, uint32 r, uint32 g, uint32 b )
{
	uint32 data;

	data = ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff);

	_V5_WriteReg_NC( ci, V5_VID_DAC_ADDR, entry );
	P6FENCE;

	_V5_WriteReg_NC( ci, V5_VID_DAC_DATA, data);
	P6FENCE;
}

void _V5_SetGamma( float gammaR, float gammaG, float gammaB)
{
	int32 i;
	
	for (i = 0; i < 256; i++)
	{
		uint32 data;
		uint32 r, g, b;
		r = (uint32)((pow(i/255.0F, 1.0F/gammaR)) * 255.0F + 0.5F);
		g = (uint32)((pow(i/255.0F, 1.0F/gammaG)) * 255.0F + 0.5F);
		b = (uint32)((pow(i/255.0F, 1.0F/gammaB)) * 255.0F + 0.5F);
		_set_palette_entry( i, r, g, b );
	}
}

void _V5_set_indexed_colors (uint count, uint8 first, uint8 *color_data, uint32 flags)
{
	uint8 *color;
	int32 idx = first;

	if( ci->Depth > 8 )
		return;	

	color = color_data;

	while (count--)
	{
		lockBena4 (&ci->CLUTLock);
		_set_palette_entry( idx++, color[0], color[1], color[2] );
		color += 3;
		unlockBena4 (&ci->CLUTLock);
	}
}

status_t _V5_set_dpms_mode(uint32 dpms_flags)
{
	uint32 LTemp;

	LTemp = _V5_ReadReg( ci, V5_VID_DAC_MODE );
	LTemp &= ~(SST_DAC_DPMS_ON_VSYNC | SST_DAC_DPMS_ON_HSYNC);	// clear all disable bits

	switch(dpms_flags)
	{
		case B_DPMS_ON:	// H: on, V: on
			// do nothing, bits already clear
			break;
		case B_DPMS_STAND_BY: // H: off, V: on, display off
			LTemp |= SST_DAC_DPMS_ON_VSYNC;
			break;
		case B_DPMS_SUSPEND: // H: on, V: off, display off
			LTemp |= SST_DAC_DPMS_ON_HSYNC;
			break;
		case B_DPMS_OFF: // H: off, V: off, display off
			LTemp |= SST_DAC_DPMS_ON_VSYNC;
			LTemp |= SST_DAC_DPMS_ON_HSYNC;
			break;
		default:
			return B_ERROR;
	}

	_V5_WriteReg_NC( ci, V5_VID_DAC_MODE, LTemp );
	
	return B_OK;
}

uint32 _V5_dpms_capabilities(void)
{
	return 	B_DPMS_ON | B_DPMS_OFF | B_DPMS_STAND_BY | B_DPMS_SUSPEND;
}

uint32 _V5_dpms_mode(void)
{
	uint32 LTemp = _V5_ReadReg( ci, V5_VID_DAC_MODE );

	if (LTemp & SST_DAC_DPMS_ON_HSYNC)
	{
		// H: off, V: on
		return B_DPMS_STAND_BY;
	}
	
	if (LTemp & SST_DAC_DPMS_ON_VSYNC)
	{
		// H: on, V: off
		return B_DPMS_SUSPEND;
	}

	if ((LTemp & SST_DAC_DPMS_ON_VSYNC) && (LTemp & SST_DAC_DPMS_ON_HSYNC))
	{
		// H: off, V: off
		return B_DPMS_OFF;
	}

	return B_DPMS_ON; // Default case - all bits are clear
}
