//in the Makefile : #define COMPILING_ACCELERANT used in "driver.h"

#include "sisGlobals.h"
#include "accelerant.h"
#include "driver_ioctl.h"
#include "sisCRTC.h"

#if OBJ_SIS5598
#include "sis5598defs.h"
#include "sisBlit.h"
#endif

#if OBJ_SIS6326
#include "sis6326defs.h"
#include "sisBlit.h"
#include "sisOverlay.h"
#endif

#if OBJ_SIS620
#include "sis620defs.h"
#include "sis620_Blit.h"
#include "sisOverlay.h"
#endif

#if OBJ_SIS630
#include "sis630defs.h"
#include "sis630_Blit.h"
#endif

static status_t init (int fd);
static void uninit (void);
static status_t init_clone (void *data);
static void get_clone_info (void *data);
static ssize_t clone_info_size (void);
static status_t deviceinfo (accelerant_device_info *adi);
static sem_id getretracesem (void);

static status_t getdisplaymode (display_mode *current_mode);

static status_t _get_frame_buffer_config (frame_buffer_config *a_frame_buffer);

static uint32 _get_accelerant_mode_count (void);
static status_t calcmodes (void);
static int testbuildmodes (register display_mode *dst);
static status_t _get_mode_list (display_mode *dm);
static status_t _propose_display_mode (display_mode *target,
				display_mode *low,
				display_mode *high);
static status_t propose_video_mode (display_mode *target, const display_mode *low, const display_mode *high);

static status_t _set_display_mode (display_mode *mode_to_set);
static status_t _set_secondary_display_mode (display_mode *mode_to_set);
static status_t _restore_primary_display_mode (void) ;

static status_t _get_display_mode (display_mode *current_mode);
static status_t _get_pixel_clock_limits (display_mode *dm, uint32 *low, uint32 *high);

uint32 getenginecount (void);
status_t acquireengine (uint32 caps,
			       uint32 max_wait,
			       sync_token *st,
			       engine_token **et);
status_t releaseengine (engine_token *et, sync_token *st);
status_t getsynctoken (engine_token *et, sync_token *st);

void rectfill_gen (engine_token *et, register uint32 color, register fill_rect_params *list,register uint32 count);
void blit (engine_token *et,register blit_params *list,register uint32 count);
uint32 colorspacebits (uint32 cs /* a color_space */);


status_t	sis630_Set_Cursor_Shape(uint16 width, uint16 height, uint16 hotX, uint16 hotY,uchar *andMask, uchar *xorMask);
void 		sis630_Move_Cursor(int16 screenX,int16 screenY);
void		sis630_Show_Cursor(bool on);
status_t	sis630_Move_DisplayArea(int16 screenX, int16 screenY);

static uint32 DPMS_Capabilities(void);
static uint32   _ioctl_Get_DPMS_mode(void);
static status_t _ioctl_Set_DPMS_mode(uint32 dpms_flags);

static status_t _ioctl_Set_Cursor_Shape(uint16 width, uint16 height, uint16 hotX, uint16 hotY,uchar *andMask, uchar *xorMask);
static void		_ioctl_Show_Cursor(bool on);
static void 	_ioctl_Move_Cursor(int16 screenX,int16 screenY);

static status_t	_ioctl_Move_Display_Area(uint16 x, uint16 y);
static void		_ioctl_Set_Indexed_Colors (uint count, uint8 first, uint8 *color_data, uint32 flags);

// 3D Init : init3d.c - types from Accelerant.h
status_t	sis620_init3Dengine		( char *device_path, void *glContext );
status_t	sis620_shutdown3Dengine	( char *device_path, void *glContext );
void		sis620_get3DdeviceInfo	( char *device_path, const char **name, uint8 *depth, uint8 *stencil, uint8 *accum );
void		sis620_get3Dmodes		( char *device_path, void (* callback)(video_mode *modes), uint32 min_color, uint32 min_depth, uint32 min_stencil, uint32 min_accum );
//uint32		sis620_get3DbuildID		( char *device_path );

status_t	sis630_init3Dengine		( char *device_path, void *glContext );
status_t	sis630_shutdown3Dengine	( char *device_path, void *glContext );
void		sis630_get3DdeviceInfo	( char *device_path, const char **name, uint8 *depth, uint8 *stencil, uint8 *accum );
void		sis630_get3Dmodes		( char *device_path, void (* callback)(video_mode *modes), uint32 min_color, uint32 min_depth, uint32 min_stencil, uint32 min_accum );
//uint32		sis630_get3DbuildID		( char *device_path );

// Globals : File Descriptors can not be shared by applications

sis_card_info	*ci 	= 0 ;
area_id		ci_areaid	= -1;

int32		driver_fd	= -1;
int32		clone_fd	= -1;

// The token for The engine
engine_token enginetoken = {
	1,					// engine_id
	B_2D_ACCELERATION,	// capability mask
	NULL				// void *opaque
	};

////////////////////////////////////////////////////////
//                                                    //
// -------- Hook Tables for various chipsets -------- //
//                                                    //
////////////////////////////////////////////////////////


/*	List of common hooks for the accelerant
	Including : Initialization hooks, Original SiS mode settings, DPMS
*/

void *get_common_accelerant_hook (uint32 feature, void *data) {

	switch ( feature ) {
		case B_INIT_ACCELERANT:									return (void *) init;
		case B_ACCELERANT_CLONE_INFO_SIZE:						return (void *) clone_info_size;
		case B_GET_ACCELERANT_CLONE_INFO:						return (void *) get_clone_info;
		case B_CLONE_ACCELERANT:								return (void *) init_clone;
		case B_UNINIT_ACCELERANT:								return (void *) uninit;
		case B_GET_ACCELERANT_DEVICE_INFO:						return (void *) deviceinfo;
		case B_ACCELERANT_RETRACE_SEMAPHORE:					return ( NULL ); // Specific

		case B_ACCELERANT_MODE_COUNT:							return (void *) _get_accelerant_mode_count;
		case B_GET_MODE_LIST:									return (void *) _get_mode_list;
		case B_PROPOSE_DISPLAY_MODE:							return (void *) _propose_display_mode;
		case B_SET_DISPLAY_MODE:								return (void *) _set_display_mode;
		case B_GET_DISPLAY_MODE:								return (void *) getdisplaymode;
		case B_GET_FRAME_BUFFER_CONFIG:							return (void *) _get_frame_buffer_config;
		case B_GET_PIXEL_CLOCK_LIMITS:							return (void *) _get_pixel_clock_limits;
		case B_MOVE_DISPLAY:									return ( NULL ) ; // Specific
		case B_SET_INDEXED_COLORS:								return (void *) _ioctl_Set_Indexed_Colors;
		case B_DPMS_CAPABILITIES:								return (void *) DPMS_Capabilities;
		case B_DPMS_MODE:										return (void *) _ioctl_Get_DPMS_mode;
		case B_SET_DPMS_MODE:									return (void *) _ioctl_Set_DPMS_mode;

		case SIS_SET_SECONDARY_DISPLAY_MODE:					return (void *) _set_secondary_display_mode;
		case SIS_RESTORE_PRIMARY_DISPLAY_MODE:					return (void *) _restore_primary_display_mode;
		
		default :
			vvddprintf(("sis: hook 0x%08x not found in common list\n", feature ));
			return ( NULL ) ;
		}

	}

/*	List of the original sis acceleration hooks
	Including : pageflip, cursor, and 2d acceleration
	
	Handles hooks look-up for sis5598
*/
	
void *sis5598_get_accelerant_hook (uint32 feature, void *data) {

	void * hook = get_common_accelerant_hook ( feature, data );
	if (hook) return(hook);
	
	switch ( feature ) {

		case B_MOVE_DISPLAY:									return (void *) _ioctl_Move_Display_Area;
		
		case B_MOVE_CURSOR:										return (void *) _ioctl_Move_Cursor;
		case B_SET_CURSOR_SHAPE:								return (void *) _ioctl_Set_Cursor_Shape;
		case B_SHOW_CURSOR:										return (void *) _ioctl_Show_Cursor ;
		
		case B_ACCELERANT_ENGINE_COUNT:							return (void *) getenginecount;
		case B_ACQUIRE_ENGINE:									return (void *) acquireengine;
		case B_RELEASE_ENGINE:									return (void *) releaseengine;
		case B_WAIT_ENGINE_IDLE:								return (void *) sis_waitengineidle;
		case B_GET_SYNC_TOKEN:									return (void *) getsynctoken;
		case B_SYNC_TO_TOKEN:									return (void *) synctotoken;
		case B_SCREEN_TO_SCREEN_BLIT:							return (void *) sis_screen_to_screen_blit;
		case B_FILL_RECTANGLE:									return (void *) sis_fill_rectangle;
		case B_INVERT_RECTANGLE:								return (void *) sis_invert_rectangle;
		case B_FILL_SPAN:										return ( NULL ); // Unimplemented

		default:
			vvddprintf(("sis : hook 0x%08x not found in sis5598 Pageflip, Cursor and 2D list\n",feature));
			return(NULL);
		}
	}

/*	List of the orginial Overlay hooks
*/

void *get_sisOverlay_hook (uint32 feature, void *data) {
	switch(feature) {

		case B_OVERLAY_COUNT:									return (void *)(OVERLAY_COUNT);
		case B_OVERLAY_SUPPORTED_SPACES:						return (void *)(OVERLAY_SUPPORTED_SPACES);
		case B_OVERLAY_SUPPORTED_FEATURES:						return (void *)(OVERLAY_SUPPORTED_FEATURES);
		case B_ALLOCATE_OVERLAY_BUFFER:							return (void *)(ALLOCATE_OVERLAY_BUFFER);
		case B_RELEASE_OVERLAY_BUFFER:							return (void *)(RELEASE_OVERLAY_BUFFER);
		case B_GET_OVERLAY_CONSTRAINTS:							return (void *)(GET_OVERLAY_CONSTRAINTS);
		case B_ALLOCATE_OVERLAY:								return (void *)(ALLOCATE_OVERLAY);
		case B_RELEASE_OVERLAY:									return (void *)(RELEASE_OVERLAY);
		case B_CONFIGURE_OVERLAY:								return (void *)(CONFIGURE_OVERLAY);
		
		default:
			vvddprintf(("sis : hook 0x%08x not found in Overlay table\n",feature));
			return(NULL);
		} 
	}

/* Handles hooks look-up for sis6326
*/

void *sis6326_get_accelerant_hook (uint32 feature, void *data) {

	void *hook = get_common_accelerant_hook (feature, data);
	if (hook != NULL) return(hook) ;

	// Original SiS pageflip, 2d, and Cursor support
	
	hook = sis5598_get_accelerant_hook (feature, data);
	if (hook != NULL) return(hook) ;
	
	hook = get_sisOverlay_hook ( feature, data );
	if (hook != NULL) return(hook) ;

	vvddprintf(("sis6326: --- unhandled hook 0x%08x\n",feature));
	return( NULL );
	}

/* Handles hooks look-up for sis620 and sis530

	Including sis620 2D engine hooks, orginal pageflip and original cursor
	plus 3D hooks
*/

void *sis620_get_accelerant_hook (uint32 feature, void *data) {

	void *hook = get_common_accelerant_hook (feature, data);
	if (hook != NULL) return(hook) ;
	
	switch (feature) {

		case B_MOVE_DISPLAY:									return (void *) _ioctl_Move_Display_Area;
		
		case B_MOVE_CURSOR:										return (void *) _ioctl_Move_Cursor;
		case B_SET_CURSOR_SHAPE:								return (void *) _ioctl_Set_Cursor_Shape;
		case B_SHOW_CURSOR:										return (void *) _ioctl_Show_Cursor ;

		case B_ACCELERANT_ENGINE_COUNT:							return (void *) getenginecount;
		case B_ACQUIRE_ENGINE:									return (void *) acquireengine;
		case B_RELEASE_ENGINE:									return (void *) releaseengine;
		case B_GET_SYNC_TOKEN:									return (void *) getsynctoken;

		case B_WAIT_ENGINE_IDLE:								return (void *) sis620_waitengineidle;
		case B_SYNC_TO_TOKEN:									return (void *) sis620_synctotoken;
		case B_SCREEN_TO_SCREEN_BLIT:							return (void *) sis620_screen_to_screen_blit;
		case B_FILL_RECTANGLE:									return (void *) sis620_fill_rectangle;
		case B_INVERT_RECTANGLE:								return (void *) sis620_invert_rectangle;
		case B_FILL_SPAN:										return ( NULL ); // Unimplemented

		default:
			vvddprintf(("sis530/620: hook 0x%08x not in PageFlip, Cursor and 2D\n",feature));
		}			
			
	if (ci->ci_Device_revision == 0xa2) {
		vvddprintf(("sis530/620: Overlay not supported on this revision (0xa2) of the sis620/530 chipset\n"));
		}
	else {
		hook = get_sisOverlay_hook ( feature, data );
		if (hook != NULL) return(hook) ;
		}
			
	switch( feature ) {
		case B_3D_INIT_ACCELERANT:								return (void *) sis620_init3Dengine;
		case B_3D_GET_DEVICE_INFO:								return (void *) sis620_get3DdeviceInfo;
		case B_3D_GET_MODES:									return (void *) sis620_get3Dmodes;
		
		case SIS_GET_CARD_INFO: // Private
			vddprintf(("sis530/620 : get card info"));
			ci->ci_protos.sis_getsynctoken				= getsynctoken ;
			ci->ci_protos.sis_waitengineidle			= sis620_waitengineidle ;
			ci->ci_protos.sis_synctotoken				= sis620_synctotoken ;
			ci->ci_protos.sis_send_orders				= sis620_send_orders ;
			ci->ci_protos.sis_addcommand_write_synctoken= sis620_addcommand_write_synctoken ;
			ci->ci_protos.sis_addcommand_blit			= sis620_addcommand_blit ;
			ci->ci_protos.sis_addcommand_fill			= sis620_addcommand_fill ;
			ci->ci_protos.sis_addcommand_invert			= sis620_addcommand_invert ;

			return((void*)ci);
			
		default:
			vvddprintf(("sis620 : -- hook 0x%08x not in 3D hooks list\n",feature));
			return(NULL);
		}
	}

/*
	Handles hooks look-up for sis630 ( sis540 ? )
	
	Includes : new Cursor registers, 2d, 3d
	
*/

void *sis630_get_accelerant_hook ( uint32 feature, void *data ) {
	void *hook = get_common_accelerant_hook (feature, data);
	if (hook != NULL) return(hook) ;

	switch ( feature ) {
		case B_MOVE_DISPLAY:									return (void *) sis630_Move_DisplayArea ;

		case B_MOVE_CURSOR:										return (void *) sis630_Move_Cursor ;
		case B_SET_CURSOR_SHAPE:								return (void *) sis630_Set_Cursor_Shape ;
		case B_SHOW_CURSOR:										return (void *) sis630_Show_Cursor ;

		case B_ACCELERANT_ENGINE_COUNT:							return (void *) getenginecount;
		case B_ACQUIRE_ENGINE:									return (void *) acquireengine;
		case B_RELEASE_ENGINE:									return (void *) releaseengine;
		case B_GET_SYNC_TOKEN:									return (void *) getsynctoken;

		case B_WAIT_ENGINE_IDLE:								return (void *) sis630_waitengineidle;
		case B_SYNC_TO_TOKEN:									return (void *) sis630_synctotoken;
		case B_SCREEN_TO_SCREEN_BLIT:							return (void *) sis630_screen_to_screen_blit;
		case B_FILL_RECTANGLE:									return (void *) sis630_fill_rectangle;
		case B_INVERT_RECTANGLE:								return (void *) sis630_invert_rectangle;
		case B_FILL_SPAN:										return ( NULL ); // Unimplemented

		case B_3D_INIT_ACCELERANT:								return (void *) sis630_init3Dengine;
		case B_3D_GET_DEVICE_INFO:								return (void *) sis630_get3DdeviceInfo;
		case B_3D_GET_MODES:									return (void *) sis630_get3Dmodes;
		
		case SIS_GET_CARD_INFO:									// Private
			vddprintf(("sis630 : get card info"));
			ci->ci_protos.sis_getsynctoken				= getsynctoken ;
			ci->ci_protos.sis_waitengineidle			= sis630_waitengineidle ;
			ci->ci_protos.sis_synctotoken				= sis630_synctotoken ;
			ci->ci_protos.sis_send_orders				= sis630_send_orders ;
			ci->ci_protos.sis_addcommand_write_synctoken= sis630_addcommand_write_synctoken ;
			ci->ci_protos.sis_addcommand_blit			= sis630_addcommand_blit ;
			ci->ci_protos.sis_addcommand_fill			= fixed_sis630_addcommand_fill ;
			ci->ci_protos.sis_addcommand_invert			= sis630_addcommand_invert ;
			return((void*)ci);
			
		default:
			vvddprintf(("sis630: hook 0x%08x not found in hooks list\n",feature));
			return( NULL );
		}
	}


///////////////////////////////////////////////////////////////
//                                                           //
// -------- THE ENTRY POINT : get_accelerant_hook() -------- //
//                                                           //
///////////////////////////////////////////////////////////////
		
void *get_accelerant_hook (uint32 feature, void *data) {

	void *hook = get_common_accelerant_hook (feature, data);
	if (hook != NULL) return(hook) ;

	if (!ci) {
		ddprintf(("sis: *** error : hook 0x%08x is chipset specific but accelerant is not yet initialized\n",feature));
		return( NULL );
		}
		
	switch( ci->ci_DeviceId ) {
		case SIS5598_DEVICEID:
			hook = sis5598_get_accelerant_hook ( feature, data ) ;
			break ;
		case SIS6326_DEVICEID:
			hook = sis6326_get_accelerant_hook ( feature, data ) ;
			break ;
		case SIS620_DEVICEID:
			hook = sis620_get_accelerant_hook ( feature, data ) ;
			break ;
		case SIS630_DEVICEID:
			hook = sis630_get_accelerant_hook ( feature, data ) ;
			break ;
		default:
			ddprintf(("sis: *** error in get_accelerant_hook : unknown devideid 0x%04x\n",ci->ci_DeviceId));
			return(NULL) ;
		}
		
	if (hook) return(hook);
	
	vddprintf(("sis : -- Unhandled hook : 0x%08x\n",feature));
	return(NULL);
	}

  
status_t allocDesktopVideoMem() {
	status_t retval ;
	int size = ci->ci_BytesPerRow * ci->ci_CurDispMode.virtual_height ;

	if ( size == ci->ci_FBMemSpec.ms_MR.mr_Size ) {
		vddprintf(("sis: Desktop memory size is similar -> using the same memory allocation\n"));
		return(B_OK);
		}
		
	if (ci->ci_FBMemSpec.ms_MR.mr_Size !=0 ) {
		vddprintf(("sis: freeing older Desktop from video mem\n"));
		BFreeByMemSpec (&ci->ci_FBMemSpec) ;
		}

	memset(&ci->ci_FBMemSpec, 0, sizeof(ci->ci_FBMemSpec));
	ci->ci_FBMemSpec.ms_PoolID			= ci->ci_PoolID;
	ci->ci_FBMemSpec.ms_AddrCareBits	= 7 ;
	ci->ci_FBMemSpec.ms_AddrStateBits	= 0 ;
	ci->ci_FBMemSpec.ms_MR.mr_Size		= size ;
	ci->ci_FBMemSpec.ms_AllocFlags		= 0 ;

	if ((retval = BAllocByMemSpec (&ci->ci_FBMemSpec)) < 0) {
		ddprintf(("sis : error couldn't allocate %d kb of memory for the framebuffer\n", (int)(ci->ci_FBMemSpec.ms_MR.mr_Size>>10) ));
		ddprintf(("sis : BAllocByMemSpec returned code %d\n",(int)retval));
		vddprintf(("sis : PoolID was %d\n",ci->ci_FBMemSpec.ms_PoolID ));
		ci->ci_FBBase = 0;
		ci->ci_FBBase_offset = 0;
		ci->ci_FBBase_DMA = 0 ;
		return (B_ERROR);
		}

	ci->ci_FBBase_offset = (uint32)ci->ci_FBMemSpec.ms_MR.mr_Offset ;
	ci->ci_FBBase		= (void*) ( (uint8*)ci->ci_BaseAddr0 		+ ci->ci_FBBase_offset );
	ci->ci_FBBase_DMA	= (void*) ( (uint8*)ci->ci_BaseAddr0_DMA	+ ci->ci_FBBase_offset );
	vddprintf(("sis: DesktopScreen FrameBuffer allocated successfully at 0x%08x\n",(uint32)ci->ci_FBBase_offset));
	return(B_OK);
	}

////////////////////////////////////////
//                                    //
// -------- SET_DISPLAY_MODE -------- //
//                                    //
////////////////////////////////////////

status_t _restore_primary_display_mode (void) {
	return(
		ioctl( driver_fd, SIS_IOCTL_RESTORE_PRIMARY_DISPLAY_MODE, 0, 0 )
		) ;
	}

status_t _set_secondary_display_mode (display_mode *m) { // mode = mode to set
	ulong old_dot_clock, same_depth, same_crt_config ;
	display_mode low,high,target;
	ulong pix_clk_range ;
	struct data_ioctl_sis_CRT_settings *sis_CRT_settings ;
	uint32 t;
	status_t retval ;
	
	vddprintf(("sis: -> _set_display_mode()\n"));

	// --- Test Mode Validity
	
	low = high = target = *m;
	pix_clk_range			 = low.timing.pixel_clock >> 5;
	low.timing.pixel_clock	-= pix_clk_range;
	high.timing.pixel_clock	+= pix_clk_range;
	
	if (propose_video_mode (&target, &low, &high) != B_OK) {
		ddprintf(("sis: *** refuse to set this video mode\n"));
		return(B_ERROR);
		}

return(
		ioctl( driver_fd, SIS_IOCTL_SET_SECONDARY_DISPLAY_MODE, (void*)m, sizeof(*m) )
		) ;

	}
	
status_t _set_display_mode (display_mode *m) { // mode = mode to set
	ulong old_dot_clock, same_depth, same_crt_config ;
	display_mode low,high,target;
	ulong pix_clk_range ;
	struct data_ioctl_sis_CRT_settings *sis_CRT_settings ;
	uint32 t;
	status_t retval ;
	
	vddprintf(("sis: -> _set_display_mode()\n"));

	// --- Test Mode Validity
	
	low = high = target = *m;
	pix_clk_range			 = low.timing.pixel_clock >> 5;
	low.timing.pixel_clock	-= pix_clk_range;
	high.timing.pixel_clock	+= pix_clk_range;
	
	if (propose_video_mode (&target, &low, &high) != B_OK) {
		ddprintf(("sis: *** refuse to set this video mode\n"));
		return(B_ERROR);
		}

return(
		ioctl( driver_fd, SIS_IOCTL_SET_PRIMARY_DISPLAY_MODE, (void*)m, sizeof(*m) )
		) ;

	
	// --- Get locks on all graphics hardware and wait until graphics engine is idle
	
	lockBena4 (&ci->ci_CRTCLock);
	lockBena4 (&ci->ci_CLUTLock);
	lockBena4 (&ci->ci_EngineLock);
	lockBena4 (&ci->ci_SequencerLock);

	vvddprintf(("sis: waiting until engine is idle...\n"));

	switch(ci->ci_DeviceId) {
#if OBJ_SIS620
		case SIS620_DEVICEID:
			sis620_waitengineidle();
			break;
#endif
#if ( OBJ_SIS5598 | OBJ_SIS6326 )
		case SIS5598_DEVICEID:
		case SIS6326_DEVICEID:
			sis_waitengineidle();
			break;
#endif
		}
		
	vvddprintf(("sis: ...engine is now idle\n"));
	
	// --- change Depth
	// fixme : save old depth
	t = colorspacebits(m->space);
	if (ci->ci_Depth!=t) {
		ci->ci_Depth=t;
		same_depth = 0;
		}
	else same_depth=1;
	
	// --- change CRT configuration
	// fixme : save old crt configuration
	same_crt_config = 1 ;
	ci->ci_BytesPerPixel = (ci->ci_Depth+7)>>3 ; 
	t=m->virtual_width*ci->ci_BytesPerPixel ;
	if (ci->ci_BytesPerRow!=t) {
		ci->ci_BytesPerRow=t;
		same_crt_config = 0;
		}
	if ((ci->ci_CurDispMode.virtual_width!=m->virtual_width) ||
	    (ci->ci_CurDispMode.virtual_height!=m->virtual_height)) same_crt_config = 0 ;
	
	old_dot_clock = ci->ci_CurDispMode.timing.pixel_clock;

	// --- writes Current-Display-Mode into CardInfo structure
	memcpy(&ci->ci_CurDispMode,m,sizeof(ci->ci_CurDispMode));

	sis_CRT_settings = prepare_CRT(m,0,m->virtual_width); 

	// Frame Buffer Memory Allocation

	if (allocDesktopVideoMem() != B_OK ) {
		ddprintf(("sis: *** error *** allocDesktopVideoMem() failed, cannot set display mode\n"));
		return(B_ERROR);
		}

	if ((!same_depth)||(!same_crt_config)) {
		int i;
		ioctl( driver_fd, SIS_IOCTL_SCREEN_OFF, 0, 0);
		// clear screen
		for(i=0;i<((ci->ci_BytesPerRow * ci->ci_CurDispMode.virtual_height)>>2);i++)
			((uint32*)ci->ci_FBBase)[i]= 0x00000000;
		}
	vvddprintf(("sis: screen cleared\n"));
		
	if (((100*(ci->ci_CurDispMode.timing.pixel_clock - old_dot_clock))/old_dot_clock) > 2 ) {
		uint32 clock_value = ci->ci_CurDispMode.timing.pixel_clock ;
		// setClock(ci->ci_CurDispMode.timing.pixel_clock);
		ioctl (driver_fd, SIS_IOCTL_SETCLOCK, &clock_value, sizeof (clock_value) ) ;
		}
	else vddprintf(("sis: set_display_mode/setClock not executed : clock remains almost the same"));

	if (!same_depth) {
		uint32 depth = ci->ci_Depth ;
		ioctl( driver_fd, SIS_IOCTL_SET_COLOR_MODE, &depth, sizeof (depth) );
		}
	else vddprintf(("sis : _set_display_mode : depth is still %d\n",ci->ci_Depth));
	
	if (!same_crt_config) {
		ioctl( driver_fd, SIS_IOCTL_SET_CRT, (void*)sis_CRT_settings, sizeof (*sis_CRT_settings) );
		}
	else vddprintf(("sis : _set_display_mode : CRT config (resolution didn't change\n"));

	ioctl( driver_fd, SIS_IOCTL_INIT_CRT_THRESHOLD, & ci->ci_CurDispMode.timing.pixel_clock, sizeof (ci->ci_CurDispMode.timing.pixel_clock) );
	
	ioctl( driver_fd, SIS_IOCTL_SETUP_DAC, 0, 0);

	ioctl( driver_fd, SIS_IOCTL_RESTART_DISPLAY, 0, 0);
	
	unlockBena4 (&ci->ci_SequencerLock);
	unlockBena4 (&ci->ci_EngineLock);
	unlockBena4 (&ci->ci_CLUTLock);
	unlockBena4 (&ci->ci_CRTCLock);
   
	return(B_OK);
	}

status_t _get_frame_buffer_config (frame_buffer_config *fbc) {
	vddprintf(("sis_accelerant: B_GET_FRAME_BUFFER_CONFIG\n"));
	fbc->frame_buffer = ci->ci_FBBase;
	fbc->frame_buffer_dma = ci->ci_FBBase_DMA;
	fbc->bytes_per_row = ci->ci_BytesPerRow;
	
	vddprintf(("sis_accelerant:     FBBase=0x%08x\n",fbc->frame_buffer));
	vddprintf(("sis_accelerant:     FB_DMA=0x%08x\n",fbc->frame_buffer_dma));
	vddprintf(("sis_accelerant:     BytesPerRow= %d\n",fbc->bytes_per_row));
	
	return(B_OK);
	}

status_t _get_pixel_clock_limits (display_mode *dm, uint32 *low, uint32 *high) {
	int max;
	vddprintf(("sis_accelerant: B_GET_PIXEL_CLOCK_LIMIT\n"));
	*low = ( 59.0 * dm->timing.h_total * dm->timing.v_total) / 1000.0 + 0.5;
	max = ( 88.0 * dm->timing.h_total * dm->timing.v_total) / 1000.0 + 0.5;
	if (max>ci->ci_Clock_Max) *high=ci->ci_Clock_Max;
	else *high=max;
	
	return (B_OK);
	}

static status_t getdisplaymode (display_mode *current_mode) {
	vddprintf(("sis_accelerant: getdisplaymode()\n"));
	memcpy (current_mode, &ci->ci_CurDispMode, sizeof (ci->ci_CurDispMode));
	vddprintf(("sis :     get_current_mode timing.pixel_clock=%d\n",current_mode->timing.pixel_clock));
	vddprintf(("sis :     get_current_mode space=%d\n",current_mode->space));
	vddprintf(("sis :     get_current_mode virtual_width=%d\n",current_mode->virtual_width));
	vddprintf(("sis :     get_current_mode virtual_height=%d\n",current_mode->virtual_height));
	vddprintf(("sis :     get_current_mode flags=0x%08x\n",current_mode->flags));
	return (B_OK);
	}

  
/////////////////////////////////////////////
// -------- ACCELERANT MANAGEMENT -------- //
/////////////////////////////////////////////

status_t init (int the_fd) {
	accelerant_getglobals	gg;
	status_t				retval;
	struct BPoolInfo		*pi ;
	
	driver_fd = the_fd;
	vddprintf(("sis_accelerant: B_INIT_ACCELERANT\n"));

	// Connect to the driver
	
	gg.gg_ProtocolVersion = 0;
	if ((retval = ioctl (driver_fd, SIS_IOCTL_GETGLOBALS, &gg, sizeof (gg))) < 0) {
		ddprintf(("sis_accelerant: !>> Failed to get globals.\n"));
		return (retval);
		}
	
	// Clone Card-Info area
	
	if ((ci_areaid = clone_area ("sis accelerant-card-info",
				     (void **) &ci,
				     B_ANY_ADDRESS,
				     B_READ_AREA | B_WRITE_AREA,
				     gg.gg_GlobalArea)) < 0) {
		ddprintf (("sis_accelerant: !>> Failed to clone global area.\n"));
		return(ci_areaid);
		}
	else vddprintf (("sis_accelerant cloned shared area successfully\n"));
	
	// Clone the VideoMemory Pool
	
	pi = BAllocPoolInfo() ;
	pi->pi_PoolID = ci->ci_PoolID ;
	if (BClonePool(pi) != B_OK) {
		ddprintf(("sis: could not clone pool %d\n",pi->pi_PoolID));
		return(B_ERROR);
		}
	vddprintf(("sis: pool %d cloned\n",pi->pi_PoolID));
	BFreePoolInfo(pi);
	return(B_OK);
	}

static ssize_t clone_info_size (void) {
	vddprintf (("sis_accelerant: clone_info_size() is %d\n", B_OS_NAME_LENGTH));
	return (B_OS_NAME_LENGTH);
	}

static void get_clone_info (void *data) {
	vddprintf (("sis_accelerant: get_clone_info() dest=0x%08x\n", data));
	vddprintf (("sis: going to write <%s> there\n",ci->ci_DevName));
	strcpy ((char *) data, ci->ci_DevName);
	vddprintf (("sis_accelerant: REAL get_clone_info() device name is <%s>\n", data));	
	}

static status_t init_clone (void *data) {
	status_t	retval;
	char		devname[B_OS_NAME_LENGTH + 6];

	vddprintf (("sis_accelerant: init_clone() src=0x%08x\n", data));
	strcpy (devname, "/dev/");
	strcat (devname, (char *) data);
	vddprintf (("sis: init_clone() device name =<%s>\n", (char*)data));	
	if ((clone_fd = open (devname, B_READ_WRITE)) < 0) {
		ddprintf(("sis: error in clone_accelerant - couldn't open device %s\n",(char*)data));
		return (B_ERROR);
		}
	if ((retval = init (clone_fd)) < 0) {
		ddprintf(("sis: error in clone_accelerant - init() failed\n"));
		uninit ();
		}
	return (retval);
	}

static void uninit (void) {
	vddprintf (("sis_accelerant: uninit()\n"));
	if (ci_areaid >= 0) {
		delete_area (ci_areaid);
		ci_areaid = -1;
		ci = NULL;
		}
	if (clone_fd >= 0) { // it's a clone
		close (clone_fd);
		clone_fd = -1;
		}
	if (driver_fd>=0) { // it's the main accelerant
		driver_fd = -1;
		}
	}


static status_t deviceinfo (accelerant_device_info *adi) {
	vddprintf(("sis_accelerant: B_GET_ACCELERANT_DEVICE_INFO\n"));
	if (adi->version >= 1)
		memcpy (adi, &ci->ci_ADI, sizeof (*adi));
	else
		adi->version = B_ACCELERANT_VERSION;
	return (B_OK);
	}

static sem_id getretracesem (void) {
	return (ci->ci_VBlankLock);
	}

//////////
// VOID //
//////////

uint32 getenginecount (void) {
  vddprintf(("sis_accelerant: B_ACCELERANT_ENGINE_COUNT\n"));
  return(1); // only 1 graphics engine. This is now the ONLY engine_id possible
  }
  
status_t acquireengine (uint32 caps,
			       uint32 max_wait,
			       sync_token *st,
			       engine_token **et) {
	vvddprintf(("sis_accelerant: B_ACQUIRE_ENGINE\n"));
	//(void) max_wait; // ?

	lockBena4 (&ci->ci_EngineLock);
	//  Sync if required
	if (st) {

		switch(ci->ci_DeviceId) {
#if ( OBJ_SIS620 | OBJ_SIS630 )
			case SIS620_DEVICEID:
			case SIS630_DEVICEID:
				sis620_synctotoken(st);
				break;
#endif
#if ( OBJ_SIS5598 | OBJ_SIS6326 )
			case SIS5598_DEVICEID:
			case SIS6326_DEVICEID:
				synctotoken (st);
				break;
#endif
			}	
		}
		
	//  Return an engine_token
	*et = &enginetoken;
	vvddprintf (("sis_accelerant: >>> acquireengine() completes\n"));
	return (B_OK);
	}
  
status_t releaseengine (engine_token *et, sync_token *st) {
	vvddprintf(("sis_accelerant: >>> releaseengine(et:0x%08x, st:0x%08x)\n", et, st));
	if (!et) {
		ddprintf(("sis_accelerant: >>> ackthp! stub_release_engine() called with null engine_token!\n"));
		return (B_ERROR);
		}
	//  Update the sync token, if any
	if (st) {
		vvddprintf(("sis_accelerant: >>> updating sync token - id: %d, count %Ld\n",et->engine_id, ci->ci_PrimitivesIssued));
		st->engine_id	= et->engine_id;
		st->counter		= ci->ci_PrimitivesIssued;
		}
	unlockBena4 (&ci->ci_EngineLock);
	vvddprintf (("sis_accelerant: >>> releaseengine() completes\n"));
	return(B_OK);
	}

status_t getsynctoken (engine_token *et, sync_token *st) {
	vvddprintf (("sis_accelerant: >>> getsynctoken(et:0x%08x, st:0x%08x)\n", et, st));
	if (st) {
		if (et) st->engine_id = et->engine_id;
		st->counter	= ci->ci_PrimitivesIssued;
		return (B_OK);
		}
	else
		return (B_ERROR);
 	}
  
uint32 colorspacebits (uint32 cs /* a color_space */) {
	switch (cs) {
	case B_RGB32_BIG:
	case B_RGBA32_BIG:
	case B_RGB32_LITTLE:
	case B_RGBA32_LITTLE:
		return (32);
	case B_RGB24_BIG:
	case B_RGB24_LITTLE:
		return (24);
	case B_RGB16_BIG:
	case B_RGB16_LITTLE:
		return (16);
	case B_RGB15_BIG:
	case B_RGB15_LITTLE:
	case B_RGBA15_BIG:
	case B_RGBA15_LITTLE:
		return (15);
	case B_CMAP8:
		return (8);
	}
	return (0);
}


//////////////////////
// DPMS Capabilities
//////////////////////

uint32 DPMS_Capabilities(void) {
	return (B_DPMS_ON       |
			B_DPMS_STAND_BY |
			B_DPMS_SUSPEND  |
			B_DPMS_OFF      );
	}


/////////////////////
// IOCTL Functions //
/////////////////////

static uint32 _ioctl_Get_DPMS_mode(void) {
	uint32 mode ;
	ioctl (driver_fd, SIS_IOCTL_GETDPMS, &mode, sizeof (mode) ) ;
	return(mode);
	}
	

static status_t _ioctl_Set_DPMS_mode(uint32 dpms_flags) {
	return( ioctl (driver_fd, SIS_IOCTL_SETDPMS, &dpms_flags, sizeof (dpms_flags)) ) ;
	}

static status_t _ioctl_Set_Cursor_Shape(uint16 width, uint16 height, uint16 hotX, uint16 hotY,uchar *andMask, uchar *xorMask) {
	data_ioctl_set_cursor_shape data ;
	data.width = width ;
	data.height = height ;
	data.hotX = hotX ;
	data.hotY = hotY ;
	data.andMask = andMask ;
	data.xorMask = xorMask ;
	return( ioctl (driver_fd, SIS_IOCTL_SET_CURSOR_SHAPE, &data, sizeof (data)) ) ;	
	}
	
static void _ioctl_Show_Cursor(bool on) {
	uint32 show ;
	if (on) show=1; else show=0;
	ioctl(driver_fd, SIS_IOCTL_SHOW_CURSOR, &show, sizeof(show) );
	}
	
static void	_ioctl_Move_Cursor(int16 screenX,int16 screenY) {
	data_ioctl_move_cursor data ;
	data.screenX = screenX ;
	data.screenY = screenY ;
	ioctl( driver_fd, SIS_IOCTL_MOVE_CURSOR, &data, sizeof (data) ) ;	
	}

static status_t	_ioctl_Move_Display_Area(uint16 x, uint16 y) {
	struct data_ioctl_move_display_area data ;
	data.x = x;
	data.y = y;
	return(ioctl( driver_fd, SIS_IOCTL_MOVE_DISPLAY_AREA, &data, sizeof (data) )) ;		
	}

static void _ioctl_Set_Indexed_Colors (uint count, uint8 first, uint8 *color_data, uint32 flags) {
	struct data_ioctl_set_indexed_colors data;
	data.count = count ;
	data.first = first ;
	data.color_data = color_data ;
	data.flags = flags ;
	ioctl( driver_fd, SIS_IOCTL_SET_INDEXED_COLORS, &data, sizeof (data) ) ;			
	}


