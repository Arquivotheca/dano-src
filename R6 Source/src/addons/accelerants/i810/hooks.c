/* :ts=8 bk=0
 *
 * hooks.c:	Main Intel I810 accelerant.  Works extremely closely
 *		with the kernel driver of the same name, so make sure you're
 *		updating both.
 *
 * $Id:$
 *
 * Leo L. Schwab					2000.01.12
 *  Hacked from I740 driver sources.
 *
 * Copyright 2000 Be Incorporated.
 */
#include <kernel/OS.h>
#include <add-ons/graphics/Accelerant.h>
#include <unistd.h>
#include <errno.h>

#include <graphics_p/i810/i810.h>
#include <graphics_p/i810/debug.h>

#include "protos.h"


/****************************************************************************
 * Prototypes.
 */
static status_t deviceinfo (accelerant_device_info *adi);
static sem_id getretracesem (void);

static status_t getdisplaymode (display_mode *current_mode);
static void movecursor (int16 x, int16 y);
static status_t movedisplayarea (uint16 x, uint16 y);
static uint32 DPMScapabilities (void);


/****************************************************************************
 * Globals.
 */
extern gfx_card_info	*ci;
extern gfx_card_ctl	*cc;
extern int32		devfd;


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
		return (void *) (ci  &&  ci->ci_IRQEnabled  ?
				 getretracesem  :
				 NULL);

	/* mode configuration */
	case B_ACCELERANT_MODE_COUNT:
		return (void *) _get_accelerant_mode_count;
	case B_GET_MODE_LIST:
		return (void *) _get_mode_list;
	case B_PROPOSE_DISPLAY_MODE:
		return (void *) _propose_display_mode;
	case B_SET_DISPLAY_MODE:
		return (void *) setdisplaymode;
	case B_GET_DISPLAY_MODE:
		return (void *) getdisplaymode;
	case B_GET_FRAME_BUFFER_CONFIG:
		return (void *) getframebufferconfig;
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
		return (void *) rectfill;
	case B_INVERT_RECTANGLE:
		return (void *) rectinvert;
	case B_FILL_SPAN:
//		return (void *) spanfill;
		return NULL;

	/*  3D acceleration init  */
	case B_3D_INIT_ACCELERANT:
		return (void *) init3d;
	case B_3D_GET_DEVICE_INFO:
		return (void *) get_device_info3d;
	case B_3D_GET_MODES:
		return (void *) get_3d_modes;

	/*  Overlay management  */
	case B_OVERLAY_COUNT:
		return (void *) overlaycount;
	case B_OVERLAY_SUPPORTED_SPACES:
		return (void *) overlaysupportedspaces;
	case B_OVERLAY_SUPPORTED_FEATURES:
		return (void *) overlaysupportedfeatures;
	case B_ALLOCATE_OVERLAY_BUFFER:
		return (void *) allocateoverlaybuffer;
	case B_RELEASE_OVERLAY_BUFFER:
		return (void *) releaseoverlaybuffer;
	case B_GET_OVERLAY_CONSTRAINTS:
		return (void *) getoverlayconstraints;
	case B_ALLOCATE_OVERLAY:
		return (void *) allocateoverlay;
	case B_RELEASE_OVERLAY:
		return (void *) releaseoverlay;
	case B_CONFIGURE_OVERLAY:
		return (void *) configureoverlay;

	default:
		return NULL;
	}
	dprintf(("get_accelerant_hook(%d, 0x%08x) failed!\n", feature, data));
	return 0;
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
	memcpy (current_mode,
	        &ci->ci_DispConfig.dc_DispMode,
	        sizeof (display_mode));
	return (B_OK);
}


/****************************************************************************
 * Cursor movement.
 */
static void
movecursor (int16 x, int16 y)
{
	cc->cc_CursorState.cs_X = x;
	cc->cc_CursorState.cs_Y = y;
	if (ci->ci_IRQEnabled)
		atomic_or (&cc->cc_IRQFlags, IRQF_MOVECURSOR);
	else
		ioctl (devfd, MAXIOCTL_GDRV + B_MOVE_CURSOR, &x, sizeof (x));
}


/****************************************************************************
 * Display movement/panning.
 */
static status_t
movedisplayarea (uint16 x, uint16 y)
{
#if 1
	gdrv_move_display	md;

	md.md_HDisplayStart = x;
	md.md_VDisplayStart = y;

//synclog (('mov>', ci->ci_Regs->DISP_SL));
	if (ioctl (devfd, MAXIOCTL_GDRV + B_MOVE_DISPLAY,
	           &md, sizeof (md)) < 0)
		return (errno);
//synclog (('mov<', ci->ci_Regs->DISP_SL));
	return (B_OK);

#elif 0
	/*  The old way  */
	register display_mode	*dm;
	moderegs		*mr;
	uint32			addr;
	int			bytespp;

	dm = &ci->ci_CurDispMode;
	mr = &cc->cc_ModeRegs;

	if (x + dm->timing.h_display > dm->virtual_width  ||
	    y + dm->timing.v_display > dm->virtual_height)
		return (B_ERROR);

	acquire_sem (ci->ci_ModeRegsLock);

	bytespp = (ci->ci_Depth + 7) >> 3;
	addr = ci->ci_FBBase + (y * dm->virtual_width + x) * bytespp;
	mr->mr_ATTR[AR_HPAN] = (addr & 3) << 1;	// Pixel panning

//	addr >>= 2;	// 32-bit quantization
	mr->mr_CRTC[CR_FBBASE_9_2]	= SetBitField (addr, 9:2, 7:0);
	mr->mr_CRTC[CR_FBBASE_17_10]	= SetBitField (addr, 17:10, 7:0);
	mr->mr_CRTC[CR_FBBASE_23_18]	= SetBitField (addr, 23:18, 5:0) |
					  MASKEXPAND (CR_FBBASE_23_18_LOADADDR);
	mr->mr_CRTC[CR_FBBASE_31_24]	= SetBitField (addr, 31:24, 7:0);

	atomic_or (&cc->cc_IRQFlags, IRQF_SETFBBASE);
	release_sem (ci->ci_ModeRegsLock);

	return (B_OK);
#elif 0
	/*  What if I use an instruction?  */
	register display_mode	*dm;
	uint32			addr;

	dm = &ci->ci_DispConfig.dc_DispMode;

	if (x + dm->timing.h_display > dm->virtual_width  ||
	    y + dm->timing.v_display > dm->virtual_height)
		return (B_ERROR);

	addr = y * cc->cc_DispBuf.gb_BytesPerRow
	     + x * cc->cc_DispBuf.gb_PixelBytes
	     + cc->cc_DispBuf.gb_BaseOffset;
	setfbbase (addr);

	return (B_OK);
#endif
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
