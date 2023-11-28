/* 
 *
 * neomagic.c:	The main entry point for the accelerant, establish all the hook
 *   functions and provide a few house-keeping routines
 *
 * Copyright 2000 Be Incorporated.  Confidential unpublished work.
 * All Rights Reserved.
 */
#include <BeBuild.h>
#include <support/ByteOrder.h>
#include <KernelExport.h>

#include <add-ons/graphics/Accelerant.h>

#include <graphics_p/neomagic/neomagic.h>
#include <graphics_p/neomagic/bena4.h>
#include <graphics_p/neomagic/neomagic_ioctls.h>
#include <graphics_p/neomagic/debug.h>

#include "protos.h"

struct device_functions
{
	void * blit_fn;
	void * rectangle_fill_fn;
	void * rectangle_invert_fn;
	void * span_fill_fn;
};

struct device_functions function_ptrs;

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

/****************************************************************************
 * Globals.
 */
neomagic_card_info	*ci;
area_id		ci_areaid = -1;
int32		devfd = -1;
int32		clonefd = -1;
int memtypefd;

/****************************************************************************
 * *The* entry point.
 */
void *
get_accelerant_hook (uint32 feature, void *data)
{

	dprintf(("neomagic_accel: get_accelerant_hook - ENTER, feature = 0x%x\n", feature));
	
	switch (feature) {
	/*  Initialization  */
	case B_INIT_ACCELERANT:
		return (void *) init;
// Clone Hooks are only required for Game Kit Support
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
		if (ci->ci_IRQFlags & IRQF__ENABLED)
			return (void *) getretracesem;
		else
			return(NULL);

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
		return (void *) _get_display_mode;
	case B_GET_FRAME_BUFFER_CONFIG:
		return (void *) _get_frame_buffer_config;
	case B_GET_PIXEL_CLOCK_LIMITS:
		return (void *) _get_pixel_clock_limits;
	case B_MOVE_DISPLAY:
//		return (void *) movedisplayarea;
		return NULL;
	case B_SET_INDEXED_COLORS:
		return (void *) _set_indexed_colors;
	case B_DPMS_CAPABILITIES:
		return (void *) _dpms_capabilities;
	case B_DPMS_MODE:
		return (void *) _dpms_mode;
	case B_SET_DPMS_MODE:
		return (void *) _set_dpms_mode;

	/* cursor managment */
	case B_MOVE_CURSOR:
		return (void *) _move_cursor;
	case B_SET_CURSOR_SHAPE:
		return (void *) _set_cursor_shape;
	case B_SHOW_CURSOR:
		return (void *) _show_cursor;

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
		return (void *) function_ptrs.blit_fn;
	case B_FILL_RECTANGLE:
		return (void *) function_ptrs.rectangle_fill_fn;
	case B_INVERT_RECTANGLE:
		return (void *) function_ptrs.rectangle_invert_fn;
	case B_FILL_SPAN:
		return (void *) function_ptrs.span_fill_fn;

	default:
		return NULL;
	}
	dprintf(("neomagic_accel: get_accelerant_hook(%d, 0x%08x) failed!\n", feature, data));
//	dprintf(("neomagic_accel: get_accelerant_hook - EXIT\n"));
	return 0;
}


/****************************************************************************
 * Initialization/teardown routines.
 */
status_t
init (int the_fd)
{
	neomagic_getglobals	gg;
	status_t	retval;
	uint32 memID, i;

_kset_dprintf_enabled_(1);
dprintf(("neomagic_accel: init - ENTER\n"));
	devfd = the_fd;

	gg.gg_ProtocolVersion = NEOMAGIC_IOCTLPROTOCOL_VERSION;
	if ((retval =
	      ioctl (devfd, NEOMAGIC_IOCTL_GETGLOBALS, &gg, sizeof (gg))) < 0)
	{
		dprintf (("neomagic_accel: init - Failed to get globals.\n"));
		return (retval);
	}

	if ((ci_areaid = clone_area ("neomagic driver data: share",
				     (void **) &ci,
				     B_ANY_ADDRESS,
				     B_READ_AREA | B_WRITE_AREA,
				     gg.gg_GlobalArea)) < 0)
	{
		dprintf (("neomagic_accel: init - Failed to clone global area.\n"));
	}
	// Initialize the list of device specific function pointers
	function_ptrs.blit_fn = NULL;
	function_ptrs.rectangle_fill_fn = NULL;
	function_ptrs.rectangle_invert_fn = NULL;
	function_ptrs.span_fill_fn = NULL;
	
	switch (ci->ci_device_id)
	{
		case DEVICEID_NM2160:
			function_ptrs.blit_fn = nm2097_blit;
			function_ptrs.rectangle_fill_fn = nm2097_rectangle_fill;
			function_ptrs.rectangle_invert_fn = nm2097_rectangle_invert;
			function_ptrs.span_fill_fn = nm2097_span_fill;
			break;
		case DEVICEID_NM2200:
			function_ptrs.blit_fn = nm2200_blit;
			function_ptrs.rectangle_fill_fn = nm2200_rectangle_fill;
			function_ptrs.rectangle_invert_fn = nm2200_rectangle_invert;
			function_ptrs.span_fill_fn = nm2200_span_fill;
			break;
		default:
			break;
	}
	// Initialise the card
dprintf (("neomagic_accel: init - calling neomagic_init()\n"));
	neomagic_init();
	
dprintf(("neomagic_accel: init - returning 0x%x\n", ci_areaid < 0 ?  ci_areaid :  B_OK));
	return (ci_areaid < 0 ?  ci_areaid :  B_OK);
}

static ssize_t
clone_info_size (void)
{
	dprintf (("neomagic_accel: clone_info_size() is %d\n", B_OS_NAME_LENGTH));
	return (B_OS_NAME_LENGTH);
}

static void
get_clone_info (void *data)
{
	dprintf (("neomagic_accel: get_clone_info() dest=0x%08x\n", data));
	strcpy ((char *) data, ci->ci_DevName);
}

static status_t
init_clone (void *data)
{
	status_t	retval;
	char		devname[B_OS_NAME_LENGTH + 6];

	dprintf (("neomagic_accel: init_clone() src=0x%08x\n", data));
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
	dprintf (("neomagic_accel: uninit - ENTER\n"));
	free (ci->ci_DispModes);
	ci->ci_DispModes = NULL;
	ci->ci_NDispModes = 0;

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
	dprintf (("neomagic_accel: uninit - EXIT\n"));
}


/****************************************************************************
 * Human-readable device info.
 */
static status_t
deviceinfo (accelerant_device_info *adi)
{
	dprintf(("neomagic_accel: deviceinfo - ENTER\n"));
	if (adi->version >= 1)
		memcpy (adi, &ci->ci_ADI, sizeof (*adi));
	else
		adi->version = B_ACCELERANT_VERSION;
	dprintf(("neomagic: deviceinfo - EXIT\n"));
	return (B_OK);
}


/****************************************************************************
 * Return vertical retrace syncronization semaphore.
 */
static sem_id
getretracesem (void)
{
		if (ci->ci_IRQFlags & IRQF__ENABLED)
			return (ci->ci_VBlankLock);
		else
			return (B_ERROR);
}
