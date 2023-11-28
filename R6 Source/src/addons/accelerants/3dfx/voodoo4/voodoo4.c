/* :ts=8 bk=0
 *
 * voodoo4.c:	Main Voodoo 4 (and Voodoo 5) accelerant.  Works extremely closely
 *		with the kernel driver of the same name, so make sure you're
 *		updating both.
 *
 * $Id:$
 *
 * Andrew Kimpton					1999.03.10
 *  Hacked from i740 driver sources.
 *
 * Copyright 1998 Be Incorporated.  Confidential unpublished work.
 * All Rights Reserved.
 */
#include <BeBuild.h>
#include <support/ByteOrder.h>
#include <KernelExport.h>

#include <graphics_p/video_overlay.h>

#include <add-ons/graphics/Accelerant.h>

#include <graphics_p/3dfx/voodoo4/voodoo4.h>
#include <graphics_p/3dfx/common/bena4.h>
//#include <graphics_p/3dfx/common/debug.h>
#define dprintf(x) _kdprintf_ x

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
static status_t movedisplayarea (uint16 x, uint16 y);

/****************************************************************************
 * Globals.
 */
thdfx_card_info	*ci;
area_id		ci_areaid = -1;
int32		devfd = -1;
int32		clonefd = -1;
void * MemMgr_2D;

int32 isClone;

/****************************************************************************
 * *The* entry point.
 */
void *
get_accelerant_hook (uint32 feature, void *data)
{

//	dprintf(("3dfx_v4_accel: get_accelerant_hook - ENTER, feature = 0x%x\n", feature));
	
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
		if (ci->IRQFlags & IRQF__ENABLED)
			return (void *) getretracesem;
		else
			return(NULL);

	/* mode configuration */
	case B_ACCELERANT_MODE_COUNT:
		return (void *) _V5_get_accelerant_mode_count;
	case B_GET_MODE_LIST:
		return (void *) _V5_get_mode_list;
	case B_PROPOSE_DISPLAY_MODE:
		return (void *) _V5_propose_display_mode;
	case B_SET_DISPLAY_MODE:
		return (void *) _V5_set_display_mode;
	case B_GET_DISPLAY_MODE:
		return (void *) getdisplaymode;
	case B_GET_FRAME_BUFFER_CONFIG:
		return (void *) _V5_get_frame_buffer_config;
	case B_GET_PIXEL_CLOCK_LIMITS:
		return (void *) _V5_get_pixel_clock_limits;
	case B_MOVE_DISPLAY:
		return (void *) movedisplayarea;
	case B_SET_INDEXED_COLORS:
		return (void *) _V5_set_indexed_colors;
	case B_DPMS_CAPABILITIES:
		return (void *) _V5_dpms_capabilities;
	case B_DPMS_MODE:
		return (void *) _V5_dpms_mode;
	case B_SET_DPMS_MODE:
		return (void *) _V5_set_dpms_mode;

	/* cursor managment */
	case B_MOVE_CURSOR:
		return (void *) _V5_move_cursor;
	case B_SET_CURSOR_SHAPE:
		return (void *) _V5_set_cursor_shape;
	case B_SHOW_CURSOR:
		return (void *) _V5_show_cursor;

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
		return (void *) rectangle_invert;
	case B_FILL_SPAN:
		return (void *) span_fill;

	// private overlay API
	case B_OVERLAY_COUNT:
		return (void *) OverlayCount;
	case B_OVERLAY_SUPPORTED_SPACES:
		return (void *) OverlaySupportedSpaces;
	case B_OVERLAY_SUPPORTED_FEATURES:
		return (void *) OverlaySupportedFeatures;
	case B_ALLOCATE_OVERLAY_BUFFER:
		return (void *) AllocateOverlayBuffer;
	case B_RELEASE_OVERLAY_BUFFER:
		return (void *) ReleaseOverlayBuffer;
	case B_GET_OVERLAY_CONSTRAINTS:
		return (void *) GetOverlayConstraints;
	case B_ALLOCATE_OVERLAY:
		return (void *) AllocateOverlay;
	case B_RELEASE_OVERLAY:
		return (void *) ReleaseOverlay;
	case B_CONFIGURE_OVERLAY:
		return (void *) ConfigureOverlay;

	case B_3D_INIT_ACCELERANT:
		return (void *) Init3DEngine;
	case B_3D_SHUTDOWN:
		return (void *) Shutdown3DEngine;
	case B_3D_GET_DEVICE_INFO:
		return (void *) get_device_info;
	case B_3D_GET_MODES:
		return (void *) get_3d_modes;

	default:
		return NULL;
	}
	dprintf(("3dfx_v4_accel: get_accelerant_hook(%d, 0x%08x) failed!\n", feature, data));
//	dprintf(("3dfx_v4_accel: get_accelerant_hook - EXIT\n"));
	return 0;
}



/****************************************************************************
 * Initialization/teardown routines.
 */
status_t
init (int the_fd)
{
	thdfx_getglobals	gg;
	status_t	retval;

	isClone = 0;
dprintf(("3dfx_v4_accel: init - ENTER\n"));
	devfd = the_fd;

	gg.gg_ProtocolVersion = THDFX_IOCTLPROTOCOL_VERSION;
	if ((retval =
	      ioctl (devfd, THDFX_IOCTL_GETGLOBALS, &gg, sizeof (gg))) < 0)
	{
//		dprintf (("3dfx_v4_accel: init - Failed to get globals.\n"));
		return (retval);
	}

	if ((ci_areaid = clone_area ("3dfx driver data: share",
				     (void **) &ci,
				     B_ANY_ADDRESS,
				     B_READ_AREA | B_WRITE_AREA,
				     gg.gg_GlobalArea)) < 0)
	{
//		dprintf (("3dfx_v4_accel: init - Failed to clone global area.\n"));
	}

	MemMgr_2D = __mem_InitClient( devfd );
dprintf (("3dfx_v4_accel: MemMgr_2D %p\n", MemMgr_2D));
dprintf (("3dfx_v4_accel: init - InitClient.\n"));

	// Initialise the card
dprintf (("3dfx_v4_accel: init - calling thdfx_init()\n"));
	thdfx_init();
	
	// Initialise the memory layout, overlay information
  memset(&(ci->AllocOverlayBuffer), 0, sizeof(ci->AllocOverlayBuffer));
  memset(&(ci->OverlayBuffer), 0, sizeof(ci->OverlayBuffer));


	// Allocate the command fifo (for 3D)
dprintf(("3dfx_v4_accel: init - Allocating 3D Fifo\n"));
	ci->CmdTransportInfo[0].fifoSize = 512 * 1024;
dprintf(("3dfx_v4_accel: init - MemMgr_2D %p\n", MemMgr_2D ));
	ci->AllocFifo[0] = __mem_Allocate( MemMgr_2D, ci->CmdTransportInfo[0].fifoSize );
	ci->AllocFifo[0]->locked = 255;
	ci->CmdTransportInfo[0].fifoOffset = (uint8 *)ci->AllocFifo[0]->address - (uint8 *)ci->BaseAddr1;
dprintf(("3dfx_v4_accel: Fifo Alloc 0 index = %ld \n", ci->AllocFifo[0]->index ));
dprintf(("3dfx_v4_accel: Fifo Alloc 0 start = %ld \n", ci->AllocFifo[0]->start ));
dprintf(("3dfx_v4_accel: Fifo Alloc 0 end = %ld \n", ci->AllocFifo[0]->end ));
	

	// Allocate the command fifo (for 2D)
dprintf(("3dfx_v4_accel: init - Allocating 2D Fifo\n"));
	ci->CmdTransportInfo[1].fifoSize = SIZEOF_CMDFIFO;
	ci->AllocFifo[1] = __mem_Allocate( MemMgr_2D, SIZEOF_CMDFIFO );	// Space for fifo's
	ci->AllocFifo[1]->locked = 255;
	ci->CmdTransportInfo[1].fifoOffset = (uint8 *)ci->AllocFifo[1]->address - (uint8 *)ci->BaseAddr1;
dprintf(("3dfx_v4_accel: Fifo Alloc 1 index = %ld \n", ci->AllocFifo[1]->index ));
dprintf(("3dfx_v4_accel: Fifo Alloc 1 start = %ld \n", ci->AllocFifo[1]->start ));
dprintf(("3dfx_v4_accel: Fifo Alloc 1 end = %ld \n", ci->AllocFifo[1]->end ));


	ci->PrimitivesCompleted = 0;
	ci->PrimitivesIssued = 0;
	ci->AllocSentinal = __mem_Allocate( MemMgr_2D, 64 );		// Space for ci_AllocSentinal
	ci->AllocSentinal->locked = 255;
dprintf(("3dfx_v4_accel: ci_AllocSentinal index = %ld \n", ci->AllocSentinal->index ));
dprintf(("3dfx_v4_accel: ci_AllocSentinal start = %ld \n", ci->AllocSentinal->start ));
dprintf(("3dfx_v4_accel: ci_AllocSentinal end = %ld \n", ci->AllocSentinal->end ));


	ci->AllocCursor = __mem_Allocate( MemMgr_2D, SIZEOF_CURSORPAGE );		// Space for cursor
	ci->AllocCursor->locked = 255;
	ci->CursorBase = ci->AllocCursor->address;
dprintf(("3dfx_v4_accel: ci_AllocCursor index = %ld \n", ci->AllocCursor->index  ));
dprintf(("3dfx_v4_accel: ci_AllocCursor start = %ld \n", ci->AllocCursor->start ));
dprintf(("3dfx_v4_accel: ci_AllocCursor end = %ld \n", ci->AllocCursor->end ));

	ci->OverlayOwnerID = -1;	
	
	// Initialize the Fifo's
	if (InitFifo(1) != B_OK)
	{
		dprintf(("3dfx_v4_accel: init - InitFifo(1) failed\n"));
	}

	if (InitFifo(0) != B_OK)
	{
		dprintf(("3dfx_v4_accel: init  - InitFifo(0) failed\n"));
	}

dprintf(("3dfx_v4_accel: init - returning 0x%x\n", ci_areaid < 0 ?  ci_areaid :  B_OK));
	return (ci_areaid < 0 ?  ci_areaid :  B_OK);
}

static ssize_t
clone_info_size (void)
{
//	dprintf (("3dfx_v4_accel: clone_info_size() is %d\n", B_OS_NAME_LENGTH));
	return (B_OS_NAME_LENGTH);
}

static void
get_clone_info (void *data)
{
//	dprintf (("3dfx_v4_accel: get_clone_info() dest=0x%08x\n", data));
	strcpy ((char *) data, ci->DevName);
}

static status_t
init_clone (void *data)
{
	thdfx_getglobals	gg;
	status_t	retval;
	char		devname[B_OS_NAME_LENGTH + 6];

//	dprintf (("3dfx_v4_accel: init_clone() src=0x%08x\n", data));
	strcpy (devname, "/dev/");
	strcat (devname, (char *) data);
	if ((clonefd = open (devname, B_READ_WRITE)) < 0)
		return (B_ERROR);

	isClone = 1;
	dprintf(("3dfx_v4_accel: init - ENTER\n"));

	gg.gg_ProtocolVersion = THDFX_IOCTLPROTOCOL_VERSION;
	if ((retval = ioctl (clonefd, THDFX_IOCTL_GETGLOBALS, &gg, sizeof (gg))) < 0)
	{
		dprintf (("3dfx_v4_accel: init - Failed to get globals.\n"));
		return (retval);
	}

	if ((ci_areaid = clone_area ("3dfx accel driver data: share",
				     (void **) &ci,
				     B_ANY_ADDRESS,
				     B_READ_AREA | B_WRITE_AREA,
				     gg.gg_GlobalArea)) < 0)
	{
		dprintf (("3dfx_v4_accel: init - Failed to clone global area.\n"));
	}

	dprintf (("3dfx_v4_accel: init - InitClient.\n"));

//	if ((retval = init (clonefd)) < 0)
//		uninit ();
	return (retval);
}

static void
uninit (void)
{
	dprintf (("3dfx_v4_accel: uninit - ENTER\n"));
	
	if(!isClone )
	{
		free (ci->DispModes);
		ci->DispModes = NULL;
		ci->NDispModes = 0;
	
		if (ci_areaid >= 0)
		{
			delete_area (ci_areaid);
			ci_areaid = -1;
			ci = NULL;
		}
		devfd = -1;
	}
	else
	{
		if (clonefd >= 0)
		{
			if( ci_areaid >= 0 )
				delete_area( ci_areaid );
			close (clonefd);
			clonefd = -1;
		}
	}
	dprintf (("3dfx_v4_accel: uninit - EXIT\n"));
}


/****************************************************************************
 * Human-readable device info.
 */
static status_t
deviceinfo (accelerant_device_info *adi)
{
//	dprintf(("3dfx: deviceinfo - ENTER\n"));
	if (adi->version >= 1)
		memcpy (adi, &ci->ADI, sizeof (*adi));
	else
		adi->version = B_ACCELERANT_VERSION;
//	dprintf(("3dfx: deviceinfo - EXIT\n"));
	return (B_OK);
}


/****************************************************************************
 * Return vertical retrace syncronization semaphore.
 */
static sem_id
getretracesem (void)
{
		if (ci->IRQFlags & IRQF__ENABLED)
			return (ci->VBlankLock);
		else
			return (B_ERROR);
}


/****************************************************************************
 * Return current display mode (should there be locking here?)
 */
static status_t
getdisplaymode (display_mode *current_mode)
{
//	dprintf(("3dfx_v4_accel: getdisplaymode - ENTER\n"));
	memcpy (current_mode, &ci->CurDispMode, sizeof (ci->CurDispMode));
//dprintf(("3dfx_v4_accel: getdisplaymode - EXIT\n"));
	return (B_OK);
}


/****************************************************************************
 * Display movement/panning.
 */
static status_t
movedisplayarea (uint16 x, uint16 y)
{
	if (x!=0) return B_ERROR;
	if (ci->IRQFlags & IRQF__ENABLED) { // card supports interrupts. notify the interrupt handler
		lockBena4 (&ci->CRTCLock);
		ci->DisplayAreaX=x;	
		ci->DisplayAreaY=y;	
		atomic_or (&ci->IRQFlags, IRQF_SETFBBASE);
		unlockBena4 (&ci->CRTCLock);
		return B_OK;
	} else { // card doesn't support interrupts. Do stuff instantly.
		return movedisplay(x, y);
	}
}
