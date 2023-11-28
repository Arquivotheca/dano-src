/* :ts=8 bk=0
 *
 * init.c:	Initialization/teardown routines.
 *
 * $Id:$
 *
 * Leo L. Schwab					2000.01.12
 *
 * Copyright 2000 Be Incorporated.
 */
#include <kernel/OS.h>
#include <kernel/image.h>
#include <add-ons/graphics/Accelerant.h>
#include <opengl/GLDefines.h>
#include <opengl/GL/gl.h>
#include <state.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <graphics_p/i810/i810.h>
#include <graphics_p/i810/debug.h>

#include "protos.h"


/****************************************************************************
 * Local prototypes.
 */
static void ctxuninit(__glContext *gc);


/****************************************************************************
 * Globals.
 */
gfx_card_info		*ci;
gfx_card_ctl		*cc;
area_id			ci_areaid;
area_id			cc_areaid;
int32			devfd = -1;
int32			clonefd = -1;

accel_3d_uninit		accel3d_uninit;
glcontext_init		accel3d_ctxinit;
glcontext_uninit	accel3d_ctxuninit;
image_id		accel3d_addon;
int			accel3d_initcount;


/****************************************************************************
 * Initialization/teardown routines.
 */
status_t
init (int the_fd)
{
	gdrv_getglobals	gg;

dprintf ((">>> init()\n"));
	devfd = the_fd;

	gg.gg_ProtocolVersion = I810_IOCTLPROTOCOL_VERSION;
	if (ioctl (devfd, GDRV_IOCTL_GETGLOBALS, &gg, sizeof (gg)) < 0)
	{
		dprintf (("!>> Failed to get globals.\n"));
		return (errno);
	}

	if ((ci_areaid = clone_area ("I810 driver data (RO)",
				     (void **) &ci,
				     B_ANY_ADDRESS,
				     B_READ_AREA,
				     gg.gg_GlobalArea_CI)) < 0)
	{
		dprintf (("!>> Failed to clone read-only global area.\n"));
		return (ci_areaid);
	}
	if ((cc_areaid = clone_area ("I810 driver data (R/W)",
				     (void **) &cc,
				     B_ANY_ADDRESS,
				     B_READ_AREA | B_WRITE_AREA,
				     gg.gg_GlobalArea_CC)) < 0)
	{
		dprintf (("!>> Failed to clone read/write global area.\n"));
		delete_area (ci_areaid);
	}
dprintf ((">>> init() complete.\n"));
	return (cc_areaid < 0 ?  cc_areaid :  B_OK);
}

ssize_t
clone_info_size (void)
{
dprintf ((">>> clone_info_size() is %d\n", sizeof (uint32)));
	return (B_OS_NAME_LENGTH);
}

void
get_clone_info (void *data)
{
dprintf ((">>> get_clone_info() dest=0x%08x\n", data));
	strcpy ((char *) data, ci->ci_DevName);
}

status_t
init_clone (void *data)
{
	status_t	retval;
	char		devname[B_OS_NAME_LENGTH + 6];

dprintf ((">>> init_clone() src=0x%08x\n", data));
	strcpy (devname, "/dev/");
	strcat (devname, (char *) data);
	if ((clonefd = open (devname, B_READ_WRITE)) < 0)
		return (B_ERROR);

	if ((retval = init (clonefd)) < 0)
		uninit ();
	return (retval);
dprintf ((">>> init_clone() complete.\n"));
}

void
uninit (void)
{
dprintf ((">>> uninit()\n"));
	if (cc_areaid > 0) {
		delete_area (cc_areaid);
		cc_areaid = 0;
		cc = NULL;
	}
	if (ci_areaid > 0) {
		delete_area (ci_areaid);
		ci_areaid = 0;
		ci = NULL;
	}
	if (clonefd >= 0) {
		close (clonefd);
		clonefd = -1;
	}
	devfd = -1;
dprintf ((">>> uninit() complete.\n"));
}


/****************************************************************************
 * 3D Initialization/teardown routines.
 */
status_t
init3d (const char *devpath, void *arg)
{
	__glContext	*gc;
	status_t	retval;
	status_t	(*init)(union init3d *i3d) = 0;

dprintf ((">>> init3d(); devpath = \"%s\"\n", devpath));
	gc = arg;
	if (!accel3d_initcount) {
		union init3d	i3d;

		if ((retval = load_add_on ("accelerants/i810.3da")) < 0)
		{
			return (retval);
		}
		accel3d_addon = retval;

		if ((retval = get_image_symbol (accel3d_addon,
						"i810_accel3d_init",
						B_SYMBOL_TYPE_TEXT,
						(void **) &init)) < 0)
		{
			uninit3d (devpath, gc);
			return (retval);
		}

		i3d.init.i_ci			= ci;
		i3d.init.i_cc			= cc;
		i3d.init.i_DevicePath		= devpath;
		i3d.init.i_GAH			= get_accelerant_hook;
		i3d.init.i_RectFill		= rectfill_gen;
		i3d.init.i_FlushDropCookie	= flushdropcookie;
		i3d.init.i_WritePacket		= writepacket;
		i3d.init.i_DevFD		= devfd;
		if ((retval = (*init) (&i3d)) < 0) {
			uninit3d (devpath, gc);
			return (retval);
		}

		accel3d_uninit = i3d.reply.r_Accel3DUninit;
		accel3d_ctxinit = i3d.reply.r_GLContextInit;
		accel3d_ctxuninit = i3d.reply.r_GLContextUninit;
	}
	accel3d_initcount++;

	if ((retval = (accel3d_ctxinit) (gc)) == B_OK)
		gc->procs.shutdown = ctxuninit;

dprintf ((">>> init3d() complete.\n"));
	return (retval);
}

void
uninit3d (const char *devpath, void *gc)
{
	(void) devpath;
	(void) gc;

dprintf ((">>> uninit3d()\n"));
	if (accel3d_addon) {
		if (accel3d_uninit)	(accel3d_uninit) ();
		unload_add_on (accel3d_addon);
		accel3d_addon = 0;
		accel3d_uninit = NULL;
		accel3d_ctxinit = NULL;
		accel3d_ctxuninit = NULL;
	}
dprintf ((">>> uninit3d() complete.\n"));
}


static void
ctxuninit (__glContext *gc)
{
	(accel3d_ctxuninit) (gc);
	gc->procs.shutdown = NULL;
	accel3d_initcount--;
	if (!accel3d_initcount)
		uninit3d (NULL, gc);
}


void
get_device_info3d (
const char	*devpath,
const char	**name,
uint8		*depth,
uint8		*stencil,
uint8		*accum
)
{
	(void) devpath;

	*name = "Intel 810";
	*depth = TRUE;
	*stencil = FALSE;
	*accum = FALSE;
}

void
get_3d_modes (
const char	*devpath,
void		(*callback)(video_mode *modes),
uint32		min_color,
uint32		min_depth,
uint32		min_stencil,
uint32		min_accum
)
{
	register int	i;
	display_mode	*modelist, *dm;
	video_mode	vm;
	uint32		nmodes, siz;

	(void) devpath;

	if (min_stencil > BGL_NONE)
		return;
	if (min_accum > BGL_NONE)
		return;
	if (!((min_color & BGL_BIT_DEPTH_MASK) < BGL_8_BIT  ||
	      (min_color & BGL_BIT_DEPTH_MASK) == BGL_16_BIT))
		return;
	if (!(min_depth < BGL_8_BIT  ||  min_depth == BGL_16_BIT))
		return;

	// The user asked for a color and depth mode we can do.
	vm.color = (min_color & BGL_COLOR_BUFFER_MASK) | BGL_16_BIT;
	if (min_depth == BGL_NONE)
		vm.depth = BGL_NONE;
	else
		vm.depth = BGL_16_BIT;

	/*
	 * Grab a copy of the modelist from the driver.
	 */
	if (ioctl (devfd, MAXIOCTL_GDRV + B_ACCELERANT_MODE_COUNT,
	           &nmodes, sizeof (nmodes)) < 0)
		return;

	siz = sizeof (display_mode) * nmodes;
	if (!(modelist = malloc (siz)))
		return;

	if (ioctl (devfd, MAXIOCTL_GDRV + B_GET_MODE_LIST, modelist, siz) < 0)
		goto ackphft;	/*  Look down  */

	/*
	 * Report windowed support.
	 */
	vm.width	=
	vm.height	=
	vm.refresh	= 0;
	vm.stencil	=
	vm.accum	= BGL_NONE;
	callback (&vm);

	/*
	 * Now walk the modelist and translate compatible modes to video_modes.
	 */
	for (dm = modelist, i = nmodes;  --i >= 0;  dm++) {
		if (dm->space == B_RGB16_LITTLE  &&
		    !(dm->timing.flags & B_TIMING_INTERLACED))
		{
			float f;

			f = ((double) dm->timing.pixel_clock * 1000.0)
			  / ((double) dm->timing.h_total
			   * (double) dm->timing.v_total);
			vm.width	= dm->timing.h_display;
			vm.height	= dm->timing.v_display;
			vm.refresh	= (f + 0.5);
			callback (&vm);
		}
	}
ackphft:
	free (modelist);
}
