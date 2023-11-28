//////////////////////////////////////////////////////////////////////////////
// Entry Point
//    This is the main entry point for the accelerant.  It has been split
// into a seperate file for several reasons, not least of which is that it
// should be completely device independant.
//
// Device Dependance: None.
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Includes //////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#include <Accelerant.h>
#include <Drivers.h>
#include <registers.h>
#include <private.h>

#include "debugprint.h"
#include "hooks_2d.h"
#include "hooks_cursor.h"
#include "hooks_dpms.h"
#include "hooks_init.h"
#include "hooks_mode.h"
#include "hooks_overlay.h"
#include "hooks_sync.h"
#include "globals.h"
#include "cardid.h"

//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void *get_accelerant_hook(uint32 feature, void *data)
{
  switch(feature) 
    {
      // Initialization

    case B_INIT_ACCELERANT:             return (void *)Init;
    case B_ACCELERANT_CLONE_INFO_SIZE:  return (void *)CloneInfoSize;
    case B_GET_ACCELERANT_CLONE_INFO:   return (void *)GetCloneInfo;
    case B_CLONE_ACCELERANT:            return (void *)InitClone;
    case B_UNINIT_ACCELERANT:           return (void *)UnInit;
    case B_GET_ACCELERANT_DEVICE_INFO:  return (void *)GetAccelerantDeviceInfo;

      // Mode Hooks

    case B_ACCELERANT_MODE_COUNT:        return (void *)AccelerantModeCount;
    case B_GET_MODE_LIST:                return (void *)GetModeList;
    case B_PROPOSE_DISPLAY_MODE:         return (void *)ProposeVideoMode;
    case B_SET_DISPLAY_MODE:             return (void *)SetVideoMode;
    case B_GET_DISPLAY_MODE:             return (void *)GetVideoMode;
    case B_GET_FRAME_BUFFER_CONFIG:      return (void *)GetFramebufferConfig;
    case B_GET_PIXEL_CLOCK_LIMITS:       return (void *)GetPixelClockLimits;
    case B_MOVE_DISPLAY:                 return (void *)MoveDisplayArea;
    case B_SET_INDEXED_COLORS:           return (void *)SetIndexedColors;
    case B_DPMS_CAPABILITIES:            return (void *)DPMS_Capabilities;
    case B_DPMS_MODE:                    return (void *)DPMS_Mode;
    case B_SET_DPMS_MODE:                return (void *)SetDPMS_Mode;
    case B_ACCELERANT_RETRACE_SEMAPHORE: return (void *)AccelerantRetraceSemaphore;

      // Cursor Hooks

#if 0
    // app_server needs updating to enable turning on/off hardware cursor
    case B_SET_CURSOR_SHAPE:             return (void *)((((display_mode *)data)->flags & B_HARDWARE_CURSOR) ? SetCursorShape : 0);
    case B_MOVE_CURSOR:                  return (void *)((((display_mode *)data)->flags & B_HARDWARE_CURSOR) ? MoveCursor : 0);
    case B_SHOW_CURSOR:                  return (void *)((((display_mode *)data)->flags & B_HARDWARE_CURSOR) ? ShowCursor : 0);
#else
    case B_SET_CURSOR_SHAPE:             return (void *)SetCursorShape;
    case B_MOVE_CURSOR:                  return (void *)MoveCursor;
    case B_SHOW_CURSOR:                  return (void *)ShowCursor;
#endif

      // Synchronization

    case B_ACCELERANT_ENGINE_COUNT:      return (void *)AccelerantEngineCount;
    case B_ACQUIRE_ENGINE:               return (void *)AcquireEngine;
    case B_RELEASE_ENGINE:               return (void *)ReleaseEngine;
    case B_WAIT_ENGINE_IDLE:             return (void *)WaitEngineIdle;
    case B_GET_SYNC_TOKEN:               return (void *)GetSyncToken;
    case B_SYNC_TO_TOKEN:                return (void *)SyncToToken;

      // 2D Acceleration
    case B_SCREEN_TO_SCREEN_BLIT:        return (void *)ScreenToScreenBlit;
    case B_SCREEN_TO_SCREEN_TRANSPARENT_BLIT:        return (void *)ScreenToScreenTransBlit;
    case B_FILL_RECTANGLE:               return (void *)RectangleFill;
    case B_INVERT_RECTANGLE:             return (void *)(si->device_id == MGA_1064S ? 0 : RectangleInvert);
    case B_FILL_SPAN:                    return (void *)SpanFill;

	// private overlay API
	case B_OVERLAY_COUNT:                return (void *)(can_do_overlays ? OVERLAY_COUNT : 0);
	case B_OVERLAY_SUPPORTED_SPACES:     return (void *)(can_do_overlays ? OVERLAY_SUPPORTED_SPACES : 0);
	case B_OVERLAY_SUPPORTED_FEATURES:   return (void *)(can_do_overlays ? OVERLAY_SUPPORTED_FEATURES : 0);
	case B_ALLOCATE_OVERLAY_BUFFER:      return (void *)(can_do_overlays ? ALLOCATE_OVERLAY_BUFFER : 0);
	case B_RELEASE_OVERLAY_BUFFER:       return (void *)(can_do_overlays ? RELEASE_OVERLAY_BUFFER : 0);
	case B_GET_OVERLAY_CONSTRAINTS:      return (void *)(can_do_overlays ? GET_OVERLAY_CONSTRAINTS : 0);
	case B_ALLOCATE_OVERLAY:             return (void *)(can_do_overlays ? ALLOCATE_OVERLAY : 0);
	case B_RELEASE_OVERLAY:              return (void *)(can_do_overlays ? RELEASE_OVERLAY : 0);
	case B_CONFIGURE_OVERLAY:            return (void *)(can_do_overlays ? CONFIGURE_OVERLAY : 0);

    }

  ddprintf(("get_accelerant_hook(%d, 0x%08x) failed!\n", feature, data));
  return 0;
}


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
