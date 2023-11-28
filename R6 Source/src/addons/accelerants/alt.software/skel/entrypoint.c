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

#include <common_includes.h>
#include <accel_includes.h>


//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void *get_accelerant_hook(uint32 feature, void *data)
{
  ddprintf(("[get_accelerant_hook] Asked for hook %08lx.\n", feature));
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

    case B_SET_CURSOR_SHAPE:             return (void *)SetCursorShape;
    case B_MOVE_CURSOR:                  return (void *)MoveCursor;
    case B_SHOW_CURSOR:                  return (void *)ShowCursor;

      // Synchronization

    case B_ACCELERANT_ENGINE_COUNT:      return (void *)AccelerantEngineCount;
    case B_ACQUIRE_ENGINE:               return (void *)AcquireEngine;
    case B_RELEASE_ENGINE:               return (void *)ReleaseEngine;
    case B_WAIT_ENGINE_IDLE:             return (void *)WaitEngineIdle;
    case B_GET_SYNC_TOKEN:               return (void *)GetSyncToken;
    case B_SYNC_TO_TOKEN:                return (void *)SyncToToken;

      // 2D Acceleration
    case B_SCREEN_TO_SCREEN_BLIT:        return (void *)ScreenToScreenBlit;
    case B_FILL_RECTANGLE:               return NULL;//return (void *)RectangleFill;
    case B_INVERT_RECTANGLE:       		 return NULL;//return (void *)RectangleInvert;

    case B_FILL_SPAN:                    return (void *)SpanFill;
    }

  ddprintf(("get_accelerant_hook(%d, 0x%08x) failed!\n", feature, data));
  return 0;
}


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
