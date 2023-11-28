/* :ts=8 bk=0
 *
 * overlay.c:	Overlay support.
 *
 * $Id:$
 *
 * Leo L. Schwab					2000.06.26
 */
#include <kernel/OS.h>
#include <unistd.h>
#include <errno.h>

#include <graphics_p/i810/i810.h>
#include <graphics_p/i810/i810defs.h>
#include <graphics_p/i810/debug.h>

#include "protos.h"


/*****************************************************************************
 * By way of explanation:
 *
 * Although the kernel driver is responsible for setting up the overlay
 * parameter buffer, the accelerant does the actual dispatching.  This is
 * because the kernel driver cannot be allowed to try and obtain
 * cc->cc_EngineLock, which is a Bena4 and therefore not safe.  Besides, all
 * the logic for allocating/dispatching command buffers is here in the
 * accelerant, anyway.
 *
 * The driver indicates it needs the overlay command buffer written to
 * hardware by returning a positive value from the ioctl().  A zero return
 * value means success, but no special action need be taken.  A negative
 * value is an error (real error in errno (POSIX me harder)).
 *
 * 2000.06.30: I am temporarily overriding all this by writing the OV0ADDR
 * register in the driver...
 */

/*****************************************************************************
 * Local prototypes.
 */
static void flushoverlay(void);


/*****************************************************************************
 * Globals.
 */
extern gfx_card_info	*ci;
extern gfx_card_ctl	*cc;
extern int		devfd;

static uint32 overlay_spaces[] = {
	B_RGB15,
	B_RGB16,
	B_YCbCr422,
	B_NO_COLOR_SPACE
};

static overlay_constraints	constraints = {
	{	/*  view  */
		0,	/*  h_alignment  */
		0,	/*  v_alignment  */
		7,	/*  width_alignment  (uint64-aligned)  */
		0,	/*  height_alignment  */
		{	/*  width (min, max) */
			1, 2047
		},
		{	/*  height (min, max)  */
			2, 2047
		}
	},
	{	/*  window  */
		0,	/*  h_alignment  */
		0,	/*  v_alignment  */
		0,	/*  width_alignment  */
		0,	/*  height_alignment  */
		{	/*  width (min, max)  */
			1, 2047
		},
		{	/*  height  */
			1, 2047
		}
	},
	{	/*  h_scale (min, max)  */
		1.0 / 2.0, 4096.0
	},
	{	/*  v_scale (min, max)  */
		1.0 / 4.0, 4096.0
	}
};


/*****************************************************************************
 * Overlay code.
 */
uint32
overlaycount (const display_mode *dm)
{
	(void) dm;

	return (1);
}

const uint32 *
overlaysupportedspaces (const display_mode *dm)
{
	(void) dm;

	return (overlay_spaces);
}

uint32
overlaysupportedfeatures (uint32 a_color_space)
{
	(void) a_color_space;

	return (B_OVERLAY_COLOR_KEY |
	        B_OVERLAY_HORIZONTAL_FITLERING |
	        B_OVERLAY_VERTICAL_FILTERING);
}

const overlay_buffer *
allocateoverlaybuffer (color_space cs, uint16 width, uint16 height)
{
	overlay_buffer	parms;

	parms.space	= cs;
	parms.width	= width;
	parms.height	= height;

	ioctl (devfd, MAXIOCTL_GDRV + B_ALLOCATE_OVERLAY_BUFFER,
	       &parms, sizeof (parms));

	/*  Convert overlay_buffer index to local address space.  */
	if (parms.bytes_per_row != ~0U)
		return (&ci->ci_OverlayBuffers[parms.bytes_per_row]);
	else
		return (NULL);
}

status_t
releaseoverlaybuffer (const overlay_buffer *ob)
{
	status_t	retval;
	uint32		idx;

	/*  Convert to index  */
	idx = ob - ci->ci_OverlayBuffers;
	if ((retval = ioctl (devfd, MAXIOCTL_GDRV + B_RELEASE_OVERLAY_BUFFER,
	                     (void *) idx, 0)) < 0)
		retval = errno;

	return (retval);
}

status_t
getoverlayconstraints (
const display_mode	*dm,
const overlay_buffer	*ob,
overlay_constraints	*oc
)
{
	memcpy (oc, &constraints, sizeof (constraints));

	if (oc->view.width.max > ob->width)
		oc->view.width.max = ob->width;
	if (oc->view.height.max > ob->height)
		oc->view.height.max = ob->height;

	if (oc->window.width.max > dm->timing.h_display)
		oc->window.width.max = dm->timing.h_display;
	if (oc->window.height.max > dm->timing.v_display)
		oc->window.height.max = dm->timing.v_display;

	return (B_OK);
}


overlay_token
allocateoverlay (void)
{
	overlay_token retval;
	
	ioctl (devfd, MAXIOCTL_GDRV + B_ALLOCATE_OVERLAY,
	       &retval, sizeof (retval));

	return (retval);
}

status_t
releaseoverlay (overlay_token ot)
{
	status_t retval;

	if ((retval = ioctl (devfd, MAXIOCTL_GDRV + B_RELEASE_OVERLAY,
	                     &ot, sizeof (ot))) < 0)
		retval = errno;
	else if (retval > 0) {
		flushoverlay ();
		retval = B_OK;
	}

	return (retval);
}

status_t
configureoverlay (
overlay_token		ot,
const overlay_buffer	*ob,
const overlay_window	*ow,
const overlay_view	*ov
)
{
	gdrv_configure_overlay	parms;
	status_t		retval;

	memset (&parms, 0, sizeof (parms));
	parms.co_Token	= ot;
	if (ob)	parms.co_Buffer	= *ob;
	if (ow)	parms.co_Window	= *ow;
	if (ov)	parms.co_View	= *ov;

	if ((retval = ioctl (devfd, MAXIOCTL_GDRV + B_CONFIGURE_OVERLAY,
	                     &parms, sizeof (parms))) < 0)
		retval = errno;
	else if (retval > 0) {
		flushoverlay ();
		retval = B_OK;
	}

	return (retval);
}


static void
flushoverlay (void)
{
	GCmd_OverlayFlip	of;

	BLockBena4 (&cc->cc_EngineLock);

	of.gcmd_Instruction	= GFXCMDPARSER_OVERLAY_FLIP;
	of.gcmd_BaseAddr	= ci->ci_OverlayRegs_DMA | (1 << 31);
	writepacket (&of, sizeof (of), TRUE, 0);

	BUnlockBena4 (&cc->cc_EngineLock);
}
