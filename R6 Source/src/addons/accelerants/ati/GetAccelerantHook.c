#include "generic.h"
#include "hooks_overlay.h"
#include "private.h"

/* the standard entry point */
void *	get_accelerant_hook(uint32 feature, void *data) {
	switch (feature) {
#define HOOK(x) case B_##x: return (void *)x
#define ZERO(x) case B_##x: return (void *)0
		/* initialization */
		HOOK(INIT_ACCELERANT);
		HOOK(ACCELERANT_CLONE_INFO_SIZE);
		HOOK(GET_ACCELERANT_CLONE_INFO);
		HOOK(CLONE_ACCELERANT);
		HOOK(UNINIT_ACCELERANT);
		HOOK(ACCELERANT_RETRACE_SEMAPHORE);

		/* mode configuration */
		HOOK(ACCELERANT_MODE_COUNT);
		HOOK(GET_MODE_LIST);
		HOOK(PROPOSE_DISPLAY_MODE);
		HOOK(SET_DISPLAY_MODE);
		HOOK(GET_DISPLAY_MODE);
		HOOK(GET_FRAME_BUFFER_CONFIG);
		HOOK(GET_PIXEL_CLOCK_LIMITS);
		HOOK(MOVE_DISPLAY);
		HOOK(SET_INDEXED_COLORS);

		HOOK(DPMS_CAPABILITIES);
		HOOK(DPMS_MODE);
		HOOK(SET_DPMS_MODE);

		/* cursor managment */
		HOOK(SET_CURSOR_SHAPE);
		HOOK(MOVE_CURSOR);
		HOOK(SHOW_CURSOR);

		/* synchronization */
		HOOK(ACCELERANT_ENGINE_COUNT);
		HOOK(ACQUIRE_ENGINE);
		HOOK(RELEASE_ENGINE);
		HOOK(WAIT_ENGINE_IDLE);
		HOOK(GET_SYNC_TOKEN);
		HOOK(SYNC_TO_TOKEN);

		/* 2D acceleration */
		HOOK(SCREEN_TO_SCREEN_BLIT);
		HOOK(FILL_RECTANGLE);
		HOOK(INVERT_RECTANGLE);
		HOOK(FILL_SPAN);
		HOOK(SCREEN_TO_SCREEN_TRANSPARENT_BLIT);
		case B_SCREEN_TO_SCREEN_SCALED_FILTERED_BLIT: {
			switch (((display_mode *)data)->space & 0xcfff) {
				case B_RGB32:
				case B_RGB16:
				case B_RGB15:
					return (void *)SCREEN_TO_SCREEN_SCALED_FILTERED_BLIT;
				default:
					return (void *)0;  // we don't support filtering on indexed data
			}
		} break;

		/* video overlays */
		case B_OVERLAY_COUNT:                return (void *)(can_do_overlays ? OVERLAY_COUNT : 0);
		case B_OVERLAY_SUPPORTED_SPACES:     return (void *)(can_do_overlays ? OVERLAY_SUPPORTED_SPACES : 0);
		case B_OVERLAY_SUPPORTED_FEATURES:   return (void *)(can_do_overlays ? OVERLAY_SUPPORTED_FEATURES : 0);
		case B_ALLOCATE_OVERLAY_BUFFER:      return (void *)(can_do_overlays ? ALLOCATE_OVERLAY_BUFFER : 0);
		case B_RELEASE_OVERLAY_BUFFER:       return (void *)(can_do_overlays ? RELEASE_OVERLAY_BUFFER : 0);
		case B_GET_OVERLAY_CONSTRAINTS:      return (void *)(can_do_overlays ? GET_OVERLAY_CONSTRAINTS : 0);
		case B_ALLOCATE_OVERLAY:             return (void *)(can_do_overlays ? ALLOCATE_OVERLAY : 0);
		case B_RELEASE_OVERLAY:              return (void *)(can_do_overlays ? RELEASE_OVERLAY : 0);
		case B_CONFIGURE_OVERLAY:            return (void *)(can_do_overlays ? CONFIGURE_OVERLAY : 0);
#undef HOOK
#undef ZERO
	}
	return 0;
}
