#include "GlobalData.h"
#include "generic.h"
#include <GraphicsDefs.h>


#define	T_POSITIVE_SYNC	(B_POSITIVE_HSYNC | B_POSITIVE_VSYNC)
#define MODE_FLAGS	(B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS)
#define MODE_COUNT (sizeof (mode_list) / sizeof (display_mode))

static const display_mode mode_list[] = {
  { {  40000,  800,  840,  968, 1056,  600,  601,  605,  628,     T_POSITIVE_SYNC}, B_CMAP8,  800,  600, 0, 0, MODE_FLAGS}, // Vesa_Monitor_@60Hz_(800X600X8.Z1)
//{ { 374, 64, 72, 80, 88, 64, 65, 69, 71, 0}, B_CMAP8, 64, 64, 0, 0, MODE_FLAGS}
};


#if 0
/* create a mask of one "bits" bits wide */
#define MASKFROMWIDTH(bits) ((1 << bits) - 1)

/*
	Validate a target display mode is both
		a) a valid display mode for this device and
		b) falls between the contraints imposed by "low" and "high"
	
	If the mode is not (or cannot) be made valid for this device, return B_ERROR.
	If a valid mode can be constructed, but it does not fall within the limits,
	 return B_BAD_VALUE.
	If the mode is both valid AND falls within the limits, return B_OK.
*/
status_t PROPOSE_DISPLAY_MODE(display_mode *target, const display_mode *low, const display_mode *high) {

	const uint16 h_display_bits = MASKFROMWIDTH(8);
	const uint16 h_sync_bits = MASKFROMWIDTH(5);
	const uint16 h_total_bits = MASKFROMWIDTH(9);
	const uint16 v_display_bits = MASKFROMWIDTH(11);
	const uint16 v_sync_bits = MASKFROMWIDTH(5);
	const uint16 v_total_bits = MASKFROMWIDTH(11);

	status_t
		result = B_OK;
	uint32
		row_bytes,
		limit_clock;
	double
		target_refresh = ((double)target->timing.pixel_clock * 1000.0) / ((double)target->timing.h_total * (double)target->timing.v_total);
	bool
		want_same_width = target->timing.h_display == target->virtual_width,
		want_same_height = target->timing.v_display == target->virtual_height;

	/*
	NOTE:
		Different devices provide different levels of control over the various CRTC values.
		This code should be used as a *GUIDELINE ONLY*.  The device you're controling may
		have very different constraints, and the code below may be insufficient to insure
		that a particular display_mode is valid for your device.  You would do well to
		spend quite a bit of time ensuring that you understand the limitations of your
		device, as setting these values incorrectly can create a display_mode that could
		turn your monitor into a useless slag of molten components, burn up your card,
		lock up the PCI bus, cause partialy or multiply repeating display images, or
		otherwise look just plain wierd.  Honest.
	*/ 
	/*
	NOTE:
		This code doesn't explicitly support interlaced video modes.
	*/

	/* validate horizontal timings */
	{
		/* for most devices, horizontal parameters must be multiples of 8 */
		uint16 h_display = target->timing.h_display >> 3;
		uint16 h_sync_start = target->timing.h_sync_start >> 3;
		uint16 h_sync_end = target->timing.h_sync_end >> 3;
		uint16 h_total = target->timing.h_total >> 3;

		/* ensure reasonable minium display and sequential order of parms */
		if (h_display < (320 >> 3)) h_display = 320 >> 3;
		if (h_display > (2048 >> 3)) h_display = 2048 >> 3;
		if (h_sync_start < (h_display + 2)) h_sync_start = h_display + 2;
		if (h_sync_end < (h_sync_start + 3)) h_sync_end = h_sync_start + 3; /*(0x001f >> 2);*/
		if (h_total < (h_sync_end + 1)) h_total = h_sync_end + 1;
		/* adjust for register limitations: */
		/* h_total is 9 bits */
		if (h_total > h_total_bits) h_total = h_total_bits;
		/* h_display is 8 bits - handled above */
		/* h_sync_start is 9 bits */
		/* h_sync_width is 5 bits */
		if ((h_sync_end - h_sync_start) > h_sync_bits) h_sync_end = h_sync_start + h_sync_bits;

		/* shift back to the full width values */
		target->timing.h_display = h_display << 3;
		target->timing.h_sync_start = h_sync_start << 3;
		target->timing.h_sync_end = h_sync_end << 3;
		target->timing.h_total = h_total << 3;
	}

	/* did we fall out of one of the limits? */
	if (
		(target->timing.h_display < low->timing.h_display) ||
		(target->timing.h_display > high->timing.h_display) ||
		(target->timing.h_sync_start < low->timing.h_sync_start) ||
		(target->timing.h_sync_start > high->timing.h_sync_start) ||
		(target->timing.h_sync_end < low->timing.h_sync_end) ||
		(target->timing.h_sync_end > high->timing.h_sync_end) ||
		(target->timing.h_total < low->timing.h_total) ||
		(target->timing.h_total > high->timing.h_total)
	) result = B_BAD_VALUE;


	/* validate vertical timings */
	{
		uint16 v_display = target->timing.v_display;
		uint16 v_sync_start = target->timing.v_sync_start;
		uint16 v_sync_end = target->timing.v_sync_end;
		uint16 v_total = target->timing.v_total;

		/* ensure reasonable minium display and sequential order of parms */
		/* v_display is 11 bits */
		/* v_total is 11 bits */
		/* v_sync_start is 11 bits */
		/* v_sync_width is 5 bits */
		if (v_display < 200) v_display = 200;
		if (v_display > (v_display_bits - 5)) v_display = (v_display_bits - 5); /* leave room for the sync pulse */
		if (v_sync_start < (v_display + 1)) v_sync_start = v_display + 1;
		if (v_sync_end < v_sync_start) v_sync_end = v_sync_start + 3;
		if (v_total < (v_sync_end + 1)) v_total = v_sync_end + 1;
		/* adjust for register limitations */
		if ((v_sync_end - v_sync_start) > v_sync_bits) v_sync_end = v_sync_start + v_sync_bits;
		target->timing.v_display = v_display;
		target->timing.v_sync_start = v_sync_start;
		target->timing.v_sync_end = v_sync_end;
		target->timing.v_total = v_total;
	}

	/* did we fall out of one of the limits? */
	if (
		(target->timing.v_display < low->timing.v_display) ||
		(target->timing.v_display > high->timing.v_display) ||
		(target->timing.v_sync_start < low->timing.v_sync_start) ||
		(target->timing.v_sync_start > high->timing.h_sync_start) ||
		(target->timing.v_sync_end < low->timing.v_sync_end) ||
		(target->timing.v_sync_end > high->timing.v_sync_end) ||
		(target->timing.v_total < low->timing.v_total) ||
		(target->timing.v_total > high->timing.v_total)
	) result = B_BAD_VALUE;

	/* adjust pixel clock for DAC limits and target refresh rate */
	/*
	We're re-calcuating the pixel_clock here because we might have
	changed the h/v totals above.  If we didn't change anything
	the calculation is wasted, but it's no big deal.
	*/
	target->timing.pixel_clock = target_refresh * ((double)target->timing.h_total) * ((double)target->timing.v_total) / 1000.0;

	/*
	Select the maximum pixel clock based on the color space.  Your
	device may have other constraints.  In this sample driver, we
	calculated the maximum pixel clock for this device in the
	initialization code.
	This is also a convienient place to determine the number of bytes
	per pixel for a later display pitch calculation.
	*/
	switch (target->space & 0x0fff) {
		case B_CMAP8:
			limit_clock = 374;
			row_bytes = 1;
			break;
		case B_RGB15:
		case B_RGB16:
			limit_clock = 374;
			row_bytes = 2;
			break;
		case B_RGB32:
			limit_clock = 374;
			row_bytes = 4;
			break;
		default:
			/* no amount of adjusting will fix not being able to support the pixel format */
			return B_ERROR;
	}
	/* make sure we don't generate more pixel bandwidth than the device can handle */
	if (target->timing.pixel_clock > limit_clock) target->timing.pixel_clock = limit_clock;
	/* we probably ought to check against too low of a pixel rate, but I'm lazy */
	
	/* note if we fell outside the limits */
	if (
		(target->timing.pixel_clock < low->timing.pixel_clock) ||
		(target->timing.pixel_clock > high->timing.pixel_clock)
	) result = B_BAD_VALUE;

	/* validate display vs. virtual */
	if ((target->timing.h_display > target->virtual_width) || want_same_width)
		target->virtual_width = target->timing.h_display;
	if ((target->timing.v_display > target->virtual_height) || want_same_height)
		target->virtual_height = target->timing.v_display;
	if (target->virtual_width > 2048)
		target->virtual_width = 2048;

	/* adjust virtual width for engine limitations */
	target->virtual_width = (target->virtual_width + 7) & ~7;
	if (
		(target->virtual_width < low->virtual_width) ||
		(target->virtual_width > high->virtual_width)
	) result = B_BAD_VALUE;

	/* calculate rowbytes after we've nailed the virtual width */
	row_bytes *= target->virtual_width;

	/* memory requirement for frame buffer */
	if ((row_bytes * target->virtual_height) > si->mem_size)
		target->virtual_height = si->mem_size / row_bytes;

	if (target->virtual_height > 2048)
		target->virtual_height = 2048;

	if (target->virtual_height < target->timing.v_display)
		/* not enough frame buffer memory for the mode */
		return B_ERROR;
	else if (
		(target->virtual_height < low->virtual_height) ||
		(target->virtual_height > high->virtual_height)
	) result = B_BAD_VALUE;

	/*
	Bit Flag Encoding
	
	The way the bit flags works is as follows:

	low high  meaning
	--- ----  -------
	 0   0    Feature must NOT be enabled
	 0   1    Feature MAY be enabled, prefered enabled
	 1   0    Feature MAY be enabled, prefered disabled
	 1   1    Feature MUST be enabled

	*/
	/* MORE WORK REQUIRED HERE.  Current drivers mostly ignore the flags */
	return result;
}
#endif

/*
	Return the number of modes this device will return from GET_MODE_LIST().
*/
uint32 ACCELERANT_MODE_COUNT(void) {
	/* return the number of 'built-in' display modes */
	return si->mode_count;
}

/*
	Copy the list of guaranteed supported video modes to the location
	provided.
*/
status_t GET_MODE_LIST(display_mode *dm) {
	/* copy them to the buffer pointed at by *dm */
	memcpy(dm, my_mode_list, si->mode_count * sizeof(display_mode));
	return B_OK;
}


/*
	Create a list of display_modes to pass back to the caller.
*/
status_t create_mode_list(void) {
	size_t max_size;
	uint32
		i, j,
		pix_clk_range;
	const display_mode
		*src;
	display_mode
		*dst,
		low,
		high;

	/*
	We prefer frame buffers to have the same endianness as the host CPU, but it's
	not required.  You can even return both, although there isn't a way for the
	current Screen preferences panel to allow the user to choose which one to use.
	*/
#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
	static color_space spaces[] = {B_RGB16_LITTLE}; //, B_RGB15_LITTLE, B_RGB16_LITTLE, B_RGB32_LITTLE};
#else
	static color_space spaces[] = {B_CMAP8, B_RGB15_BIG, B_RGB16_BIG, B_RGB32_BIG};
#endif

	/* figure out how big the list could be, and adjust up to nearest multiple of B_PAGE_SIZE */
	max_size = (((MODE_COUNT * 4) * sizeof(display_mode)) + (B_PAGE_SIZE-1)) & ~(B_PAGE_SIZE-1);
	/* create an area to hold the info */
	si->mode_area = my_mode_list_area =
		create_area("SAMPLE accelerant mode info", (void **)&my_mode_list, B_ANY_ADDRESS, max_size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (my_mode_list_area < B_OK) return my_mode_list_area;

	/* walk through our predefined list and see which modes fit this device */
	src = mode_list;
	dst = my_mode_list;
	si->mode_count = 0;
	for (i = 0; i < MODE_COUNT; i++) {
		/* set ranges for acceptable values */
		low = high = *src;
		/* range is 6.25% of default clock: arbitrarily picked */ 
		pix_clk_range = low.timing.pixel_clock >> 5;
		low.timing.pixel_clock -= pix_clk_range;
		high.timing.pixel_clock += pix_clk_range;
		/* some cards need wider virtual widths for certain modes */
		high.virtual_width = 2048;
		/* do it once for each depth we want to support */
		for (j = 0; j < (sizeof(spaces) / sizeof(color_space)); j++) {
			/* set target values */
			*dst = *src;
			/* poke the specific space */
			dst->space = low.space = high.space = spaces[j];
			/* ask for a compatible mode */
			if (1) {
				/* count it, and move on to next mode */
				dst++;
				si->mode_count++;
			}
		}
		/* advance to next mode */
		src++;
	}
	
	return B_OK;
}
