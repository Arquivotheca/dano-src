#include "GlobalData.h"
#include "generic.h"

/*
	Return the current display mode.  The only time you might return an
	error is if a mode hasn't been set.
*/
status_t GET_DISPLAY_MODE(display_mode *current_mode) {
	/* easy for us, we return the last mode we set */
	*current_mode = si->dm;
	return B_OK;
}

/*
	Return the frame buffer configuration information.
*/
status_t GET_FRAME_BUFFER_CONFIG(frame_buffer_config *afb) {
	/* easy again, as the last mode set stored the info in a convienient form */
	*afb = si->fbc;
	return B_OK;
}

/*
	Return the maximum and minium pixel clock limits for the specified mode.
*/
status_t GET_PIXEL_CLOCK_LIMITS(display_mode *dm, uint32 *low, uint32 *high) {
	uint32 total_pix = (uint32)dm->timing.h_total * (uint32)dm->timing.v_total;
	/* lower limit of about 60Hz vertical refresh */
	*high = *low = (total_pix * 60L) / 1000L;
	return B_OK;
}
