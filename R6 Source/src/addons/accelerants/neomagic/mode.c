/*
 * mode.c:	Graphics mode setting, retrieval and configureation.
 *
 * Copyright 2000 Be Incorporated.  Confidential unpublished work.
 * All Rights Reserved.
 */
#include <graphics_p/video_overlay.h>

#include <graphics_p/neomagic/neomagic.h>
#include <graphics_p/neomagic/debug.h>

#include "protos.h"

/****************************************************************************
 * Globals
 */
extern neomagic_card_info	*ci;
extern int		devfd;

/****************************************************************************
 * This routine is remarkable in that it has to reset the
 * acceleration state after changing modes.
 */
status_t
_set_display_mode (display_mode *mode_to_set)
{
	status_t		retval;
	dprintf(("neomagic_accel: _set_display_mode - ENTER\n"));
	
	/*
	 * We ask the driver to *NOT* release the rendering engine lock, as
	 * we will be dancing on it further.
	 */
	if ((retval = vid_selectmode (mode_to_set, LEAVELOCKF_ENGINE)) == B_OK)
	{
		memcpy (&ci->ci_CurDispMode, mode_to_set,	sizeof (ci->ci_CurDispMode));
		retval = AccelInit (ci);
		/*  Now we're really done; release the lock.  */
		unlockBena4 (&ci->ci_EngineLock);
	}
	else
	{
		/*  We had an error so make sure we release the lock  */
		unlockBena4 (&ci->ci_EngineLock);
	}
	
	dprintf(("neomagic_accel: _set_display_mode - EXIT, returning 0x%x\n", retval));
	return (retval);
}

uint32
_get_accelerant_mode_count (void)
{
dprintf(("neomagic_accel: _get_accelerant_mode_count - EXIT returning %d\n", ci->ci_NDispModes));
	return (ci->ci_NDispModes);
}

status_t
_get_mode_list (display_mode *dm)
{
	status_t retval;
	
	/*  The size is wrong...  */
dprintf(("neomagic_accel: _get_mode_list - ENTER\n"));
		memcpy (dm, ci->ci_DispModes,	ci->ci_NDispModes * sizeof (display_mode));
dprintf(("neomagic_accel: _get_mode_list - EXIT - returning 0x%x\n", retval));
	return (retval);
}

status_t
_propose_display_mode (display_mode *target, display_mode *low,display_mode *high)
{
	status_t retval = B_OK;
	dprintf(("neomagic_accel: _propose_display_mode - ENTER\n"));

//	retval = propose_video_mode(target, low, high);
	dprintf(("neomagic_accel: _propose_display_mode - EXIT, returning 0x%x\n", retval));
	return (retval);
}

status_t
_get_frame_buffer_config (frame_buffer_config *a_frame_buffer)
{
	status_t	retval = B_OK;
	
	dprintf(("neomagic_accel: _get_frame_buffer_config -ENTER\n"));

		a_frame_buffer->frame_buffer = ci->ci_FBBase;
		a_frame_buffer->frame_buffer_dma = ci->ci_FBBase_DMA;
		a_frame_buffer->bytes_per_row = ci->ci_BytesPerRow;
		
	return (retval);
}

status_t
_get_pixel_clock_limits (display_mode *dm, uint32 *low, uint32 *high)
{
	status_t		retval = B_OK;
	uint32 total_pix;
	
	dprintf(("neomagic_accel: _get_pixel_clock_limits - ENTER\n"));

	/*
	 * Constrain low-end to 48 Hz, until a monitors database
	 * shows up...
	 */
	total_pix = dm->timing.h_total * dm->timing.v_total;
	*low = (total_pix * 48L) / 1000L;
	*high = CLOCK_MAX;

	dprintf(("neomagic_accel: _get_pixel_clock_limits - EXIT, returning 0x%x\n", retval));
	return (retval);
}

/****************************************************************************
 * Return current display mode (should there be locking here?)
 */
status_t
_get_display_mode (display_mode *current_mode)
{
	dprintf(("neomagic_accel: getdisplaymode - ENTER\n"));
	memcpy (current_mode, &ci->ci_CurDispMode, sizeof (ci->ci_CurDispMode));
	dprintf(("neomagic_accel: getdisplaymode - EXIT\n"));
	return (B_OK);
}


