#include <stdlib.h>
#include <math.h>
#include <OS.h>
#include <KernelExport.h>

#include <graphics_p/video_overlay.h>

#include <graphics_p/radeon/defines.h>
#include <graphics_p/radeon/main.h>
#include <graphics_p/radeon/regdef.h>

#include <graphics_p/radeon/CardState.h>
#include <graphics_p/radeon/radeon_ioctls.h>

#include <add-ons/graphics/Accelerant.h>

#include "proto.h"
#include "cursor.h"


/****************************************************************************
 * Prototypes.
 */
static ssize_t clone_info_size (void);
static void get_clone_info (void *data);
//static sem_id getretracesem (void);
//static status_t getdisplaymode (display_mode *current_mode);
//static status_t movedisplayarea (uint16 x, uint16 y);

/****************************************************************************
 * Globals.
 */

extern CardInfo *ci;
extern int32 primDevFD;
extern __mem_AreaDef *primMemMgr;


/***************************************************************************
* Simple static vido functions.
* Most just call the kernel driver
*/

static void lock( CardInfo *ci )
{
	int32 previous = atomic_add( &ci->benEngineInt, 1 );
	if( previous >= 1 )
		acquire_sem( ci->benEngineSem );
}

static void unlock( CardInfo *ci )
{
	int32 previous = atomic_add( &ci->benEngineInt, -1 );
	if( previous > 1 )
		release_sem( ci->benEngineSem );
}

static uint32 _get_accelerant_mode_count()
{
	uint32 count = 0;
	if( ioctl (primDevFD, RADEON_IOCTL_MODE_COUNT, &count, sizeof (count)) < B_OK )
	{
		dprintf(("Radeon_accel: RADEON_IOCTL_MODE_COUNT FAILED ! \n" ));
	}
dprintf(("Radeon_accel: _get_accelerant_mode_count returning 0x%x \n", count));
	return count;
}

static status_t _get_mode_list (display_mode *dm)
{
	/*  The size is wrong...  */
dprintf(("Radeon_accel: _get_mode_list \n" ));
	if ( ioctl (primDevFD, RADEON_IOCTL_GET_MODE_LIST, dm, sizeof (*dm)) < B_OK)
	{
		dprintf(("Radeon_accel: RADEON_IOCTL_GET_MODE_LIST FAILED ! \n" ));
		return B_ERROR;
	}

	return B_OK;
}

/******************************************************************************
 * Radeon_InitGamma                                                             *
 *  Function: This function initializes the gamma ramp for hi colour modes    *
 *    Inputs: NONE                                                            *
 *   Outputs: NONE                                                            *
 ******************************************************************************/
static void _InitGamma ( float gammaR, float gammaG, float gammaB )
{
	int i;
	
	WRITE_REG_8 (PALETTE_INDEX, 0 );
	
	for (i = 0; i < 256; i++)
	{
		uint32 data;
		uint32 r, g, b;
		r = (uint32)((pow(i/255.0F, 1.0F/gammaR)) * 255.0F + 0.5F);
		g = (uint32)((pow(i/255.0F, 1.0F/gammaG)) * 255.0F + 0.5F);
		b = (uint32)((pow(i/255.0F, 1.0F/gammaB)) * 255.0F + 0.5F);

		data = ((r & 0xff) << PALETTE_DATA__PALETTE_DATA_R__SHIFT)
			| ((g & 0xff) << PALETTE_DATA__PALETTE_DATA_G__SHIFT)
			| (b & 0xff);
		WRITE_REG_32 (PALETTE_DATA, data);
	} // for
} // Radeon_InitGamma


static status_t _set_display_mode(display_mode *mode_to_set)
{
	lock( ci );
	ci->Finish(ci);
	
//dprintf(("Radeon_accel: _set_display_mode \n" ));
	if ( ioctl (primDevFD, RADEON_IOCTL_SET_DISPLAY_MODE, mode_to_set, sizeof (*mode_to_set)) < B_OK)
	{
		dprintf(("Radeon_accel: RADEON_IOCTL_SET_DISPLAY_MODE FAILED ! \n" ));
		unlock( ci );
		return B_ERROR;
	}
	
	if ( ci->AllocFrameBuffer )
	{
		__mem_Free( primMemMgr, ci->AllocFrameBuffer );
		ci->AllocFrameBuffer = 0;
	}
	
	ci->AllocFrameBuffer = __mem_Allocate( primMemMgr, mode_to_set->virtual_height * ci->FBStride );
	ci->AllocFrameBuffer->locked = 255;
	WRITE_REG_32( CRTC_OFFSET, ci->AllocFrameBuffer->address );

	ci->xres = mode_to_set->virtual_width;
	ci->yres = mode_to_set->virtual_height;
	switch( mode_to_set->space )
	{
	default:
		return 0;
	case B_RGB32_BIG:
	case B_RGBA32_BIG:
	case B_RGB32_LITTLE:
	case B_RGBA32_LITTLE:
		ci->bitpp = 32;
		ci->bytepp = 4;
		ci->hwBppNum = 6;
		break;
		
	case B_RGB16_LITTLE:
	case B_RGB16_BIG:
		ci->bitpp = 16;
		ci->bytepp = 2;
		ci->hwBppNum = 4;
		break;

	case B_RGB15_LITTLE:
	case B_RGBA15_LITTLE:
	case B_RGB15_BIG:
	case B_RGBA15_BIG:
		ci->bitpp = 15;
		ci->bytepp = 2;
		ci->hwBppNum = 3;
		break;
	}

	Radeon_InitEngine( ci );

	Radeon_WaitForFifo( ci, 4 );
	WRITE_REG_32( DST_PITCH_OFFSET, ((ci->AllocFrameBuffer->address >>10) & 0x3fffff) |
		((ci->FBStride >>6) << 22) | (0<<30) );
	WRITE_REG_32( SRC_PITCH_OFFSET, ((ci->AllocFrameBuffer->address >>10) & 0x3fffff) |
		((ci->FBStride >>6) << 22) | (0<<30) );
	WRITE_REG_32( DEFAULT_PITCH_OFFSET, ((ci->AllocFrameBuffer->address >>10) & 0x3fffff) |
		((ci->FBStride >>6) << 22) | (0<<30) );
		
	
	{
		float g = 1.0;
		char *env = getenv( "APP_GAMMA" );
		if( env )
		{
			sscanf( env, "%f", &g );
		}
		_InitGamma( g, g, g );
	}	

	unlock( ci );

	return B_OK;
}

static status_t _get_display_mode(display_mode *mode_to_get)
{
dprintf(("Radeon_accel: _get_display_mode \n" ));
	if ( ioctl (primDevFD, RADEON_IOCTL_GET_DISPLAY_MODE, mode_to_get, sizeof (*mode_to_get)) < B_OK)
		return B_ERROR;
	return B_OK;
}

status_t _get_frame_buffer_config (frame_buffer_config *a_frame_buffer)
{
//	dprintf(("Radeon_accel: _get_frame_buffer_config -ENTER\n"));
//	dprintf(("Radeon_accel: _get_frame_buffer_config afb=%p  ci=%p \n", a_frame_buffer, ci ));
	a_frame_buffer->frame_buffer = ci->CardMemory + ci->AllocFrameBuffer->address;
	a_frame_buffer->frame_buffer_dma = ci->DMA_Base + ci->AllocFrameBuffer->address;
	a_frame_buffer->bytes_per_row = ci->FBStride;
	
	return B_OK;
}

status_t _get_pixel_clock_limits (display_mode *dm, uint32 *low, uint32 *high)
{
//dprintf(("Radeon_accel: _get_pixel_clock_limits \n" ));
	*low = (640*480 * 60L) / 1000L;
	*high = 206000.0;
	return B_OK;
}

void _set_indexed_colors (uint count, uint8 first, uint8 *color_data, uint32 flags)
{
dprintf(("Radeon_accel: _set_indexed_colors \n" ));
}

/****************************************************************************
* The Sync code
*/

static engine_token	enginetoken = {
	1, B_2D_ACCELERATION, NULL
};


/*****************************************************************************
 * Engine and sync_token management.
 */

status_t synctotoken (sync_token *st);

uint32 getenginecount (void)
{
	dprintf (("Radeon_accel: getenginecount - ENTER\n"));
	return (1);
}

static void writeSyncToken( CardInfo *ci )
{
	uint32 buf[2];
	buf[0] = (SCRATCH_REG0 >>2) & 0x7fff;
	buf[1] = ci->primitivesIssued;
	ci->CPSubmitPackets (ci, buf, 2);
}

status_t getsynctoken (engine_token *et, sync_token *st)
{
//dprintf (("Radeon_accel: getsynctoken(et:0x%08x, st:0x%08x) - ENTER\n", et, st));
	
	writeSyncToken( ci );

	if (et) {
		writeSyncToken( ci );
		st->engine_id	= et->engine_id;
		st->counter	= ci->primitivesIssued;
		return (B_OK);
	} else
		return (B_ERROR);
}


status_t acquireengine ( uint32 caps, uint32 max_wait,
	sync_token *st, engine_token **et )
{
	lock( ci );

	ci->Finish(ci);

	/*  Sync if required  */
	if (st)
		synctotoken (st);

	/*  Return an engine_token  */
	*et = &enginetoken;
	return (B_OK);
}

status_t releaseengine (engine_token *et, sync_token *st)
{
//dprintf(("Radeon_accel: releaseengine(et:0x%08x, st:0x%08x) - ENTER\n", et, st));

	writeSyncToken( ci );

	ci->Finish(ci);

	
	if (!et)
		return (B_ERROR);

	/*  Update the sync token, if any  */
	if (st) {
		st->engine_id = et->engine_id;
		st->counter	= ci->primitivesIssued;
	}

	unlock( ci );

	return (B_OK);
}

void waitengineidle (void)
{
#if 0
	int32 previous;

//	if( !lockCount )
//	{
//		previous = atomic_add( &ci->benEngineInt, 1 );
//		if( previous >= 1 )
//			acquire_sem( ci->benEngineSem );
//	}

	Radeon_WaitForIdle( ci );

//	if( !lockCount )
//	{
//		previous = atomic_add( &ci->benEngineInt, -1 );
//		if( previous > 1 )
//			release_sem( ci->benEngineSem );
//	}
#endif
}

/*
 * This does The Cool Thing (!) with the sync_token values, and waits for the
 * requested primitive to complete, rather than waiting for the whole engine
 * to drop to idle.  BEWARE - Do not actually write to the FIFO in this
 * routine - the engine lock is not currently held and you'll corrupt the FIFO
 * command stream.
 */
status_t synctotoken (sync_token *st)
{
	uint32 c = st->counter;
	
	int32 diff = c - ci->scratch[0];

int32 t = 0;
	while( diff > 0 )
	{
		snooze( 500 );
		diff = c - ci->scratch[0];

t+=500;
if (t>1000000) {
	dprintf(("synctotoken %ld, %ld \n", c, ci->scratch[0] ));
	break;
}

	}
	return (B_OK);
}

/**********************
DPMS
*/

static status_t _set_dpms_mode(uint32 dpms_flags)
{
	uint32 mode;
	status_t err = B_OK;

	switch(dpms_flags)
	{
		case B_DPMS_ON:
			mode = 0; break;
		case B_DPMS_STAND_BY:
			mode = 1; break;
		case B_DPMS_SUSPEND:
			mode = 2; break;
		case B_DPMS_OFF:
			mode = 3; break;
		default:
			return B_ERROR;
	}

dprintf(( "_set_dpms_mode %ld \n", mode ));	
	if ( ioctl (primDevFD, RADEON_IOCTL_SET_DPMS_MODE, &mode, sizeof (mode)) < B_OK)
	{
dprintf(( "ioctl failed  \n" ));	
		return B_ERROR;
	}
dprintf(( "ioctl ok  \n" ));	
	return B_OK;
}

static uint32 _dpms_capabilities(void)
{
	return 	B_DPMS_ON | B_DPMS_OFF | B_DPMS_STAND_BY | B_DPMS_SUSPEND;
}

static uint32 _dpms_mode(void)
{
	uint32 mode;
	const static uint32 tbl[4] = {B_DPMS_ON, B_DPMS_STAND_BY, B_DPMS_SUSPEND, B_DPMS_OFF };

	if ( ioctl (primDevFD, RADEON_IOCTL_GET_DPMS_MODE, &mode, sizeof (mode)) < B_OK)
		return B_ERROR;
		
	return tbl[mode];
}


/****************************************************************************
 * *The* entry point.
 */
void * get_accelerant_hook (uint32 feature, void *data)
{
	dprintf(("Radeon_accel: get_accelerant_hook - ENTER, feature = 0x%x\n", feature));
	
	switch (feature)
	{
	/*  Initialization  */
	case B_INIT_ACCELERANT:
		return (void *) __radeon_Init;
	case B_ACCELERANT_CLONE_INFO_SIZE:
		return (void *) clone_info_size;
	case B_GET_ACCELERANT_CLONE_INFO:
		return (void *) get_clone_info;
	case B_CLONE_ACCELERANT:
		return (void *) Radeon_init_clone;
	case B_UNINIT_ACCELERANT:
		return (void *) __radeon_Uninit;
	case B_GET_ACCELERANT_DEVICE_INFO:
		return (void *) __radeon_DeviceInfo;
	case B_ACCELERANT_RETRACE_SEMAPHORE:
		return(NULL);

	/* mode configuration */
	case B_ACCELERANT_MODE_COUNT:
		return (void *) _get_accelerant_mode_count;
	case B_GET_MODE_LIST:
		return (void *) _get_mode_list;
	case B_PROPOSE_DISPLAY_MODE:
		return (void *) 0;//_propose_display_mode;
	case B_SET_DISPLAY_MODE:
		return (void *) _set_display_mode;
	case B_GET_DISPLAY_MODE:
		return (void *) _get_display_mode;
	case B_GET_FRAME_BUFFER_CONFIG:
		return (void *) _get_frame_buffer_config;
	case B_GET_PIXEL_CLOCK_LIMITS:
		return (void *) _get_pixel_clock_limits;
	case B_MOVE_DISPLAY:
		return (void *) 0;//movedisplayarea;
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
		return (void *) cursorMove;
	case B_SET_CURSOR_SHAPE:
		return (void *) cursorLoad;
	case B_SHOW_CURSOR:
		return (void *) cursorShow;

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
		return (void *) Radeon_screen_to_screen_blits;
	case B_FILL_RECTANGLE:
		return (void *) Radeon_fill_rectangle;
	case B_INVERT_RECTANGLE:
		return (void *) 0;//rectangle_invert;
	case B_FILL_SPAN:
		return (void *) 0;//span_fill;

	// private overlay API
	case B_OVERLAY_COUNT:
		return (void *) 0;//OverlayCount;
	case B_OVERLAY_SUPPORTED_SPACES:
		return (void *) 0;//OverlaySupportedSpaces;
	case B_OVERLAY_SUPPORTED_FEATURES:
		return (void *) 0;//OverlaySupportedFeatures;
	case B_ALLOCATE_OVERLAY_BUFFER:
		return (void *) 0;//AllocateOverlayBuffer;
	case B_RELEASE_OVERLAY_BUFFER:
		return (void *) 0;//ReleaseOverlayBuffer;
	case B_GET_OVERLAY_CONSTRAINTS:
		return (void *) 0;//GetOverlayConstraints;
	case B_ALLOCATE_OVERLAY:
		return (void *) 0;//AllocateOverlay;
	case B_RELEASE_OVERLAY:
		return (void *) 0;//ReleaseOverlay;
	case B_CONFIGURE_OVERLAY:
		return (void *) 0;//ConfigureOverlay;

	case B_3D_INIT_ACCELERANT:
		return (void *) Radeon_Init3DEngine;
	case B_3D_SHUTDOWN:
		return (void *) Radeon_Shutdown3DEngine;
	case B_3D_GET_DEVICE_INFO:
		return (void *) Radeon_get_device_info;
	case B_3D_GET_MODES:
		return (void *) Radeon_get_3d_modes;

	default:
		return NULL;
	}
	dprintf(("Radeon: get_accelerant_hook(%d, 0x%08x) failed!\n", feature, data));
	return 0;
}



static ssize_t clone_info_size (void)
{
	dprintf (("3dfx_accel: clone_info_size() is %d\n", B_OS_NAME_LENGTH));
	return (B_OS_NAME_LENGTH);
}

static void get_clone_info (void *data)
{
	dprintf (("3dfx_accel: get_clone_info() dest=0x%08x\n", data));
	strcpy ((char *) data, ci->devname);
}




#if 0


/****************************************************************************
 * Return current display mode (should there be locking here?)
 */
static status_t getdisplaymode (display_mode *current_mode)
{
//	dprintf(("3dfx_accel: getdisplaymode - ENTER\n"));
	memcpy (current_mode, &ci->ci_CurDispMode, sizeof (ci->ci_CurDispMode));
//dprintf(("3dfx_accel: getdisplaymode - EXIT\n"));
	return (B_OK);
}


/****************************************************************************
 * Display movement/panning.
 */
static status_t movedisplayarea (uint16 x, uint16 y)
{
	if (x!=0) return B_ERROR;
	if (ci->ci_IRQFlags & IRQF__ENABLED) { // card supports interrupts. notify the interrupt handler
		lockBena4 (&ci->ci_CRTCLock);
		ci->ci_DisplayAreaX=x;	
		ci->ci_DisplayAreaY=y;	
		atomic_or (&ci->ci_IRQFlags, IRQF_SETFBBASE);
		unlockBena4 (&ci->ci_CRTCLock);
		return B_OK;
	} else { // card doesn't support interrupts. Do stuff instantly.
		return movedisplay(x, y);
	}
}
#endif
