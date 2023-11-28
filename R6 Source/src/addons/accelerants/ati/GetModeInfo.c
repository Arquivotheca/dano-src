#include "private.h"
#include "generic.h"

status_t GET_DISPLAY_MODE(display_mode *current_mode) {
	*current_mode = ai->dm;
	return B_OK;
}

status_t GET_FRAME_BUFFER_CONFIG(frame_buffer_config *afb) {
	*afb = ai->fbc;
	return B_OK;
}

status_t GET_PIXEL_CLOCK_LIMITS(display_mode *dm, uint32 *low, uint32 *high) {
#if EMACHINE
#if 0
	/* No freedom as the eOne used a fixed sync monitor */
	if (dm->timing.h_display == 640)
		*low = 70709;
	else if (dm->timing.h_display == 800)
		*low = 67371;
	else
		*low = 71400;
#else
	*low = dm->timing.pixel_clock;
#endif
	*high = *low;
#else
	uint32 total_pix = (uint32)dm->timing.h_total * (uint32)dm->timing.v_total;
	uint32 clock_limit;

	/* max pixel clock is pixel depth dependant */
	switch (dm->space & ~0x3000) {
		case B_RGB32: clock_limit = ai->pix_clk_max32; break;
		case B_RGB15:
		case B_RGB16: clock_limit = ai->pix_clk_max16; break;
		case B_CMAP8: clock_limit = ai->pix_clk_max8; break;
		default:
			clock_limit = 0;
	}
	/* lower limit of about 48Hz vertical refresh */
	*low = (total_pix * 48L) / 1000L;
	if (*low > clock_limit) return B_ERROR;
	*high = clock_limit;
#endif
	return B_OK;
}

sem_id ACCELERANT_RETRACE_SEMAPHORE(void) {
	return si->vblank;
}
