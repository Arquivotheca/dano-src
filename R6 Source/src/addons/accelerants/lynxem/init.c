/* :ts=8 bk=0
 *
 * init.c:	Initialization/teardown routines.
 *
 * $Id:$
 *
 * Leo L. Schwab					2000.07.14
 *
 * Copyright 2000 Be Incorporated.
 */
#include <kernel/OS.h>
#include <kernel/image.h>
#include <add-ons/graphics/Accelerant.h>
//#include <opengl/GLDefines.h>
//#include <opengl/GL/gl.h>
//#include <state.h>
#include <errno.h>

#include <graphics_p/lynxem/lynxem.h>
#include <graphics_p/lynxem/debug.h>

#include "protos.h"


/****************************************************************************
 * Local prototypes.
 */
//static void ctxuninit(__glContext *gc);


/****************************************************************************
 * Globals.
 */
gfx_card_info		*ci;
gfx_card_ctl		*cc;
area_id			ci_areaid;
area_id			cc_areaid;
int32			devfd = -1;
int32			clonefd = -1;

//glcontext_init		accel3d_ctxinit;
//glcontext_uninit	accel3d_ctxuninit;
//image_id		accel3d_addon;
//int			accel3d_initcount;


/****************************************************************************
 * Initialization/teardown routines.
 */
status_t
init (int the_fd)
{
	gdrv_getglobals	gg;

	devfd = the_fd;

	gg.gg_ProtocolVersion = LYNXEM_IOCTLPROTOCOL_VERSION;
	if (ioctl (devfd, GDRV_IOCTL_GETGLOBALS, &gg, sizeof (gg)) < 0)
	{
		dprintf (("!>> Failed to get globals.\n"));
		return (errno);
	}

	if ((ci_areaid = clone_area ("LynxEM driver data (RO)",
				     (void **) &ci,
				     B_ANY_ADDRESS,
				     B_READ_AREA,
				     gg.gg_GlobalArea_CI)) < 0)
	{
		dprintf (("!>> Failed to clone read-only global area.\n"));
		return (ci_areaid);
	}
	if ((cc_areaid = clone_area ("LynxEM driver data (R/W)",
				     (void **) &cc,
				     B_ANY_ADDRESS,
				     B_READ_AREA | B_WRITE_AREA,
				     gg.gg_GlobalArea_CC)) < 0)
	{
		dprintf (("!>> Failed to clone read/write global area.\n"));
		delete_area (ci_areaid);
	}
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
}


#if 0
/****************************************************************************
 * 3D Initialization/teardown routines.
 */
status_t
init3d (const char *devpath, void *arg)
{
	__glContext	*gc;
	status_t	retval;
	status_t	(*init)(union init3d *i3d) = 0;

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
		i3d.init.i_GetPacketSpace	= getpacketspace;
		i3d.init.i_ExecutePackets	= executepackets;
		i3d.init.i_DevFD		= devfd;
		if ((retval = (*init) (&i3d)) < 0) {
			uninit3d (devpath, gc);
			return (retval);
		}

		accel3d_ctxinit = i3d.reply.r_GLContextInit;
		accel3d_ctxuninit = i3d.reply.r_GLContextUninit;
	}
	accel3d_initcount++;

	if ((retval = (accel3d_ctxinit) (gc)) == B_OK)
		gc->procs.shutdown = ctxuninit;

	return (retval);
}

void
uninit3d (const char *devpath, void *gc)
{
	if (accel3d_addon) {
		unload_add_on (accel3d_addon);
		accel3d_addon = 0;
		accel3d_ctxinit = NULL;
		accel3d_ctxuninit = NULL;
	}
}


static void
ctxuninit (__glContext *gc)
{
	(accel3d_ctxuninit) (gc);
	gc->procs.shutdown = NULL;
	accel3d_initcount--;
//	if (!accel3d_initcount)
//		uninit3d ();
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
	static int32	widths[5] =  {640, 800, 1024, 1152, 1280};
	static int32	heights[5] = {480, 600,  768,  864, 1024};
	int32		ct;
	video_mode	m;

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
	m.color = (min_color & BGL_COLOR_BUFFER_MASK) | BGL_16_BIT;
	if (min_depth == BGL_NONE)
		m.depth = BGL_NONE;
	else
		m.depth = BGL_16_BIT;

	m.width		= 0;
	m.height	= 0;
	m.refresh	= 0;
	m.stencil	= BGL_NONE;
	m.accum		= BGL_NONE;
	callback (&m);
	for (ct = 0;  ct < 5;  ct++) {
		m.width		= widths[ct];
		m.height	= heights[ct];
		m.refresh	= 60;
		callback (&m);
		m.refresh	= 70;
		callback (&m);
		m.refresh	= 75;
		callback (&m);
		m.refresh	= 85;
		callback (&m);
	}
}
#endif
