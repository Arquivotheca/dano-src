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
#include <math.h>
 
#include <graphics_p/video_overlay.h>


#include <graphics_p/3dfx/banshee/banshee.h>
#include <graphics_p/3dfx/banshee/banshee_regs.h>
#include <graphics_p/3dfx/common/debug.h>

#include "protos.h"

/****************************************************************************
 * Globals
 */
extern thdfx_card_info	*ci;

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
status_t _set_display_mode (display_mode *mode_to_set)
{
	status_t		retval;
//	dprintf(("3dfx_accel: _set_display_mode - ENTER\n"));
	
	/*
	 * We ask the driver to *NOT* release the rendering engine lock, as
	 * we will be dancing on it further.
	 */
	if ((retval = vid_selectmode (mode_to_set, LEAVELOCKF_ENGINE)) == B_OK)
	{
		memcpy (&ci->CurDispMode, mode_to_set,	sizeof (ci->CurDispMode));
		retval = B_OK;
		/*  Now we're really done; release the lock.  */
		unlockBena4 (&ci->EngineLock);
	}
	else
	{
		/*  We had an error so make sure we release the lock  */
		unlockBena4 (&ci->EngineLock);
	}

	if ((mode_to_set->timing.h_display == 640) && (mode_to_set->timing.v_display == 480))
	{
		enable_tvout(true);
	}
	else
	{
		enable_tvout(false);
	}
	
	return (retval);
}


/****************************************************************************
 * These are just straight thunks out to the kernel driver.
 */
uint32 _get_accelerant_mode_count (void)
{
	return (ci->NDispModes);
}

status_t _get_mode_list (display_mode *dm)
{
	/*  The size is wrong...  */
	memcpy (dm, ci->DispModes, ci->NDispModes * sizeof (display_mode));
	return B_OK;
}

status_t _get_frame_buffer_config (frame_buffer_config *a_frame_buffer)
{
	a_frame_buffer->frame_buffer = ci->FBBase;
	a_frame_buffer->frame_buffer_dma = ci->FBBase_DMA;
	a_frame_buffer->bytes_per_row = ci->BytesPerRow;
	return B_OK;
}

status_t _get_pixel_clock_limits (display_mode *dm, uint32 *low, uint32 *high)
{
	/*
	 * Constrain low-end to 48 Hz, until a monitors database
	 * shows up...
	 */
	uint32 total_pix = dm->timing.h_total * dm->timing.v_total;
	*low = (total_pix * 48L) / 1000L;
	*high = CLOCK_MAX;

	return B_OK;
}

static void _set_palette_entry( uint32 entry, uint32 r, uint32 g, uint32 b )
{
	uint32 data;
	uint32 t[64];

	atomic_add( &t[0], 1 );
	atomic_add( &t[8], 1 );
	atomic_add( &t[16], 1 );
	atomic_add( &t[24], 1 );
	atomic_add( &t[32], 1 );
	data = ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff);

	_V3_WriteReg_NC( ci, V3_VID_DAC_ADDR, entry );
	P6FENCE;

	atomic_add( &t[0], 1 );
	atomic_add( &t[8], 1 );
	atomic_add( &t[16], 1 );
	atomic_add( &t[24], 1 );
	atomic_add( &t[32], 1 );
	_V3_WriteReg_NC( ci, V3_VID_DAC_DATA, data);
	P6FENCE;
}

void _V3_SetGamma( float gammaR, float gammaG, float gammaB)
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

void _set_indexed_colors (uint count, uint8 first, uint8 *color_data, uint32 flags)
{
	uint8 *color;
	int32 idx = first;
	
dprintf(( "_set_indexed_colors Depth=%ld\n", ci->Depth  ));
	
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

status_t _set_dpms_mode(uint32 dpms_flags)
{
	uint32 LTemp;

	LTemp = _V3_ReadReg( ci, V3_VID_DAC_MODE );
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

	_V3_WriteReg_NC( ci, V3_VID_DAC_MODE, LTemp );
	
	return B_OK;
}

uint32 _dpms_capabilities(void)
{
	return 	B_DPMS_ON | B_DPMS_OFF | B_DPMS_STAND_BY | B_DPMS_SUSPEND;
}

uint32 _dpms_mode(void)
{
	uint32 LTemp = _V3_ReadReg( ci, V3_VID_DAC_MODE );

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
