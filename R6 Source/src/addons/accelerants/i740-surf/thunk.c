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
#include <graphics_p/i740-surf/i740.h>
#include <graphics_p/i740-surf/debug.h>
#include <errno.h>

#include "protos.h"


/****************************************************************************
 * Globals
 */
extern i740_card_info	*ci;
extern i740_card_ctl	*cc;
extern int		devfd;


/****************************************************************************
 * This "thunk" routine is remarkable in that it has to reset the
 * acceleration state after changing modes.
 */
status_t
_set_display_mode (display_mode *mode_to_set)
{
	i740_set_display_mode	sdm;
	status_t		retval;

	/*
	 * We grab the engine lock so confusion doesn't result.
	 */
	sdm.sdm_Mode = mode_to_set;
	BLockBena4 (&cc->cc_EngineLock);
	if ((retval = ioctl (devfd, MAXIOCTL_I740 + B_SET_DISPLAY_MODE,
			     &sdm, sizeof (sdm))) == B_OK)
		retval = AccelInit (ci);
	else
		retval = errno;
	/*  Now we're really done; release the lock.  */
	BUnlockBena4 (&cc->cc_EngineLock);
	return (retval);
}


/****************************************************************************
 * These are just straight thunks out to the kernel driver.
 */
uint32
_get_accelerant_mode_count (void)
{
	uint32	count = 0;

	ioctl (devfd, MAXIOCTL_I740 + B_ACCELERANT_MODE_COUNT,
	       &count, sizeof (count));
	return (count);
}

status_t
_get_mode_list (display_mode *dm)
{
	status_t retval;

	/*  The size is wrong...  */
	if ((retval = ioctl (devfd, MAXIOCTL_I740 + B_GET_MODE_LIST,
			     dm, sizeof (*dm))) < 0)
		retval = errno;

	return (retval);
}


status_t
_propose_display_mode (
display_mode *target,
display_mode *low,
display_mode *high
)
{
	i740_propose_display_mode	pdm;
	status_t			retval;

	pdm.pdm_Target	= target;
	pdm.pdm_Lo	= low;
	pdm.pdm_Hi	= high;
	if ((retval = ioctl (devfd, MAXIOCTL_I740 + B_PROPOSE_DISPLAY_MODE,
			     &pdm, sizeof (pdm))) < 0)
		retval = errno;

	return (retval);
}

status_t
_get_frame_buffer_config (frame_buffer_config *a_frame_buffer)
{
	status_t retval;
	
	if ((retval = ioctl (devfd, MAXIOCTL_I740 + B_GET_FRAME_BUFFER_CONFIG,
			     a_frame_buffer, sizeof (*a_frame_buffer))) < 0)
		return (retval);

	return (retval);
}

status_t
_get_pixel_clock_limits (display_mode *dm, uint32 *low, uint32 *high)
{
	i740_pixel_clock_limits	pcl;
	status_t		retval;

	pcl.pcl_DisplayMode	= dm;
	if ((retval = ioctl (devfd, MAXIOCTL_I740 + B_GET_PIXEL_CLOCK_LIMITS,
			     &pcl, sizeof (pcl))) == B_OK)
	{
		*low = pcl.pcl_Lo;
		*high = pcl.pcl_Hi;
	} else
		retval = errno;
	return (retval);
}

#if 0
status_t
_move_display_area (uint16 h_display_start, uint16 v_display_start)
{
	i740_move_display md;

	md.md_HDisplayStart	= h_display_start;
	md.md_VDisplayStart	= v_display_start;
	return (ioctl (devfd, MAXIOCTL_I740 + B_MOVE_DISPLAY,
		       &md, sizeof (md)));
}
#endif

void
_set_indexed_colors (uint count, uint8 first, uint8 *color_data, uint32 flags)
{
	i740_set_indexed_colors sic;

	sic.sic_Count		= count;
	sic.sic_First		= first;
	sic.sic_ColorData	= color_data;
	sic.sic_Flags		= flags;
	ioctl (devfd, MAXIOCTL_I740 + B_SET_INDEXED_COLORS,
	       &sic, sizeof (sic));
}


uint32
_get_dpms_mode (void)
{
	uint32	mode;

	ioctl (devfd, MAXIOCTL_I740 + B_DPMS_MODE,
	       &mode, sizeof (mode));
	return (mode);
}

status_t
_set_dpms_mode (uint32 dpms_flags)
{
	status_t retval;

	if ((retval = ioctl (devfd, MAXIOCTL_I740 + B_SET_DPMS_MODE,
			     &dpms_flags, sizeof (dpms_flags))) < 0)
		retval = errno;

	return (retval);
}


status_t
_set_cursor_shape (
uint16	width,
uint16	height,
uint16	hot_x,
uint16	hot_y,
uint8	*andMask,
uint8	*xorMask
)
{
	i740_set_cursor_shape	scs;
	status_t		retval;

	scs.scs_Width	= width;
	scs.scs_Height	= height;
	scs.scs_HotX	= hot_x;
	scs.scs_HotY	= hot_y;
	scs.scs_ANDMask	= andMask;
	scs.scs_XORMask	= xorMask;

	if ((retval = ioctl (devfd, MAXIOCTL_I740 + B_SET_CURSOR_SHAPE,
		       &scs, sizeof (scs))) < 0)
		retval = errno;

	return (retval);
}

void
_show_cursor (bool on)
{
	/*
	 * Yeah, I could do this in the accelerant; it's a trivial operation.
	 * But it would have meant opening the PCI bus module and doing I/O
	 * operations, with attendant cleanup code, and I was feeling
	 * supremely lazy.  So I did it this way.  If it's a performance issue,
	 * I'll pull it into the accelerant.
	 */
	ioctl (devfd, MAXIOCTL_I740 + B_SHOW_CURSOR,
	       &on, sizeof (on));
}
