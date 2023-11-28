/* :ts=8 bk=0
 *
 * trident.c:	Main Trident accelerant.  Works extremely closely
 *		with the kernel driver of the same name, so make sure you're
 *		updating both.
 *
 * $Id:$
 *
 * Leo L. Schwab					1999.10.12
 *  Hacked from I740 driver sources.
 *
 * Copyright 1998 Be Incorporated.  Confidential unpublished work.
 * All Rights Reserved.
 */
#include <BeBuild.h>
#include <support/ByteOrder.h>
#include <add-ons/graphics/Accelerant.h>
#include <errno.h>

#include <graphics_p/trident/trident.h>
#include <graphics_p/trident/debug.h>

#include "protos.h"


/****************************************************************************
 * Prototypes.
 */
static status_t init (int fd);
static ssize_t clone_info_size (void);
static void get_clone_info (void *data);
static status_t init_clone (void *data);
static void uninit (void);

static status_t deviceinfo (accelerant_device_info *adi);
static sem_id getretracesem (void);

static status_t getdisplaymode (display_mode *current_mode);
static void movecursor (int16 x, int16 y);
static status_t movedisplayarea (uint16 x, uint16 y);
static uint32 DPMScapabilities (void);

#if _SUPPORTS_FEATURE_CURSOR_COLORS
static status_t setcursorcolors (uint32 blackcolor, uint32 whitecolor);
#endif


/****************************************************************************
 * Globals.
 */
tri_card_info	*ci;
tri_card_ctl	*cc;
area_id		ci_areaid = -1;
area_id		cc_areaid = -1;
int32		devfd = -1;
int32		clonefd = -1;


/****************************************************************************
 * *The* entry point.
 */
void *
get_accelerant_hook (uint32 feature, void *data)
{
	switch (feature) {
	/*  Initialization  */
	case B_INIT_ACCELERANT:
		return (void *) init;
	case B_ACCELERANT_CLONE_INFO_SIZE:
		return (void *) clone_info_size;
	case B_GET_ACCELERANT_CLONE_INFO:
		return (void *) get_clone_info;
	case B_CLONE_ACCELERANT:
		return (void *) init_clone;
	case B_UNINIT_ACCELERANT:
		return (void *) uninit;
	case B_GET_ACCELERANT_DEVICE_INFO:
		return (void *) deviceinfo;
	case B_ACCELERANT_RETRACE_SEMAPHORE:
//		return (void *) getretracesem;
		return (NULL);

	/* mode configuration */
	case B_ACCELERANT_MODE_COUNT:
		return (void *) _get_accelerant_mode_count;
	case B_GET_MODE_LIST:
		return (void *) _get_mode_list;
	case B_PROPOSE_DISPLAY_MODE:
		return (void *) _propose_display_mode;
	case B_SET_DISPLAY_MODE:
		return (void *) _set_display_mode;
	case B_GET_DISPLAY_MODE:
		return (void *) getdisplaymode;
	case B_GET_FRAME_BUFFER_CONFIG:
		return (void *) _get_frame_buffer_config;
	case B_GET_PIXEL_CLOCK_LIMITS:
		return (void *) _get_pixel_clock_limits;
	case B_MOVE_DISPLAY:
		return (void *) movedisplayarea;
	case B_SET_INDEXED_COLORS:
		return (void *) _set_indexed_colors;
	case B_DPMS_CAPABILITIES:
		return (void *) DPMScapabilities;
	case B_DPMS_MODE:
		return (void *) _get_dpms_mode;
	case B_SET_DPMS_MODE:
		return (void *) _set_dpms_mode;

	/* cursor managment */
	case B_MOVE_CURSOR:
		return (void *) movecursor;
	case B_SET_CURSOR_SHAPE:
		return (void *) _set_cursor_shape;
	case B_SHOW_CURSOR:
		return (void *) _show_cursor;
#if _SUPPORTS_FEATURE_CURSOR_COLORS
	case B_SET_CURSOR_COLORS:
		return (void *) setcursorcolors;
#endif

	/* synchronization */
	case B_ACCELERANT_ENGINE_COUNT:
		return (void *) getenginecount;
	case B_ACQUIRE_ENGINE:
		return (void *) acquireengine;
	case B_RELEASE_ENGINE:
		return (void *) releaseengine;
	case B_WAIT_ENGINE_IDLE:
		return (void *) waitengineidle;
	case B_GET_SYNC_TOKEN:
		return (void *) getsynctoken;
	case B_SYNC_TO_TOKEN:
		return (void *) synctotoken;

	/* 2D acceleration */
	case B_SCREEN_TO_SCREEN_BLIT:
		return (void *) blit;
	case B_FILL_RECTANGLE:
		return (void *) rectfill_gen;
	case B_INVERT_RECTANGLE:
//		return (void *)mga_rectangle_invert;
	case B_FILL_SPAN:
//		return (void *)mga_span_fill;

	default:
		return NULL;
	}
	dprintf(("get_accelerant_hook(%d, 0x%08x) failed!\n", feature, data));
	return 0;
}


/****************************************************************************
 * Initialization/teardown routines.
 */
status_t
init (int the_fd)
{
	trident_getglobals	gg;

	devfd = the_fd;

	gg.gg_ProtocolVersion = TRIDENT_IOCTLPROTOCOL_VERSION;
	if (ioctl (devfd, TRIDENT_IOCTL_GETGLOBALS, &gg, sizeof (gg)) < 0)
	{
		dprintf (("!>> Failed to get globals.\n"));
		return (errno);
	}

	if ((ci_areaid = clone_area ("Trident driver data (RO)",
				     (void **) &ci,
				     B_ANY_ADDRESS,
				     B_READ_AREA,
				     gg.gg_GlobalArea_CI)) < 0)
	{
		dprintf (("!>> Failed to clone read-only global area.\n"));
		return (ci_areaid);
	}
	if ((cc_areaid = clone_area ("Trident driver data (R/W)",
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

static ssize_t
clone_info_size (void)
{
dprintf ((">>> clone_info_size() is %d\n", sizeof (uint32)));
	return (B_OS_NAME_LENGTH);
}

static void
get_clone_info (void *data)
{
dprintf ((">>> get_clone_info() dest=0x%08x\n", data));
	strcpy ((char *) data, ci->ci_DevName);
}

static status_t
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

static void
uninit (void)
{
dprintf ((">>> uninit()\n"));
	if (cc_areaid >= 0) {
		delete_area (cc_areaid);
		cc_areaid = -1;
		cc = NULL;
	}
	if (ci_areaid >= 0) {
		delete_area (ci_areaid);
		ci_areaid = -1;
		ci = NULL;
	}
	if (clonefd >= 0) {
		close (clonefd);
		clonefd = -1;
	}
	devfd = -1;
}


/****************************************************************************
 * Human-readable device info.
 */
static status_t
deviceinfo (accelerant_device_info *adi)
{
	if (adi->version >= 1)
		memcpy (adi, &ci->ci_ADI, sizeof (*adi));
	else
		adi->version = B_ACCELERANT_VERSION;
	return (B_OK);
}


/****************************************************************************
 * Return vertical retrace syncronization semaphore.
 */
static sem_id
getretracesem (void)
{
	return (ci->ci_VBlankLock);
}


/****************************************************************************
 * Return current display mode (should there be locking here?)
 */
static status_t
getdisplaymode (display_mode *current_mode)
{
	memcpy (current_mode, &ci->ci_CurDispMode, sizeof (ci->ci_CurDispMode));
	return (B_OK);
}


/****************************************************************************
 * Cursor handling.
 */
static void
movecursor (int16 x, int16 y)
{
	cc->cc_MousePosX = x;
	cc->cc_MousePosY = y;
	atomic_or (&cc->cc_IRQFlags, IRQF_MOVECURSOR);
		/*  FIXME: Stopgap until we get interrupts working...  */
		ioctl (devfd, MAXIOCTL_TRIDENT + B_MOVE_CURSOR, &x, sizeof (x));
}

#if _SUPPORTS_FEATURE_CURSOR_COLORS
static status_t
setcursorcolors (uint32 blackcolor, uint32 whitecolor)
{
	cc->cc_CursColorBlk = blackcolor;
	cc->cc_CursColorWht = whitecolor;
	ioctl (devfd, MAXIOCTL_TRIDENT + B_SET_CURSOR_COLORS, NULL, 0);

	return (B_OK);
}
#endif


/****************************************************************************
 * Display movement/panning.
 */
static status_t
movedisplayarea (uint16 x, uint16 y)
{
	register display_mode	*dm;
	uint32			addr;
	int			bytespp;

	dm = &ci->ci_CurDispMode;

	if (x + dm->timing.h_display > dm->virtual_width  ||
	    y + dm->timing.v_display > dm->virtual_height)
		return (B_ERROR);

	acquire_sem (ci->ci_CRTCLock);

	bytespp = (ci->ci_Depth + 7) >> 3;
	addr = ci->ci_FBBase + (y * dm->virtual_width + x) * bytespp;
//	cc->cc_ATTR[AR_HPAN] = (addr & 3) << 1;	// Pixel panning

	addr >>= 3;	// 64-bit quantization
	cc->cc_CRTC[CR_FBBASE_9_2]	= Set8Bits (addr);
	cc->cc_CRTC[CR_FBBASE_17_10]	= SetBitField (addr, 15:8, 7:0);
	cc->cc_CRTC[CR_VSYNCEXTRA]	&= FIELDMASK
					    (CR_VSYNCEXTRA, FBBASE_19_17);
	cc->cc_CRTC[CR_VSYNCEXTRA]	|= SetBitField
					    (addr,
					     18:16,
					     CR_VSYNCEXTRA_FBBASE_19_17);

	atomic_or (&cc->cc_IRQFlags, IRQF_SETFBBASE);
	release_sem (ci->ci_CRTCLock);

	return (B_OK);
}


/****************************************************************************
 * DPMS Support.
 */
static uint32
DPMScapabilities (void)
{
	return (B_DPMS_ON       |
		B_DPMS_STAND_BY |
		B_DPMS_SUSPEND  |
		B_DPMS_OFF      );
}
