#include "private.h"
#include "generic.h"


#define	T_POSITIVE_SYNC	(B_POSITIVE_HSYNC | B_POSITIVE_VSYNC)
#define MODE_FLAGS	(B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS | B_SUPPORTS_OVERLAYS | B_DPMS)
#define MODE_COUNT (sizeof (mode_list) / sizeof (display_mode))

#if EMACHINE
static const display_mode mode_list[] = {
#if 0
{ { 70709, 640, 816, 1024, 1116, 480, 486, 490, 528, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS},
{ { 67371, 800, 820, 846, 1192, 600, 601, 604, 628, T_POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS},
{ { 71400, 1024, 1046, 1082, 1190, 768, 770, 774, 800, T_POSITIVE_SYNC}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}
#else
#if 0
{ { 68688, 640, 648, 696, 1080, 480, 481, 485, 530, T_POSITIVE_SYNC}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS},
{ { 65203, 800, 848, 976, 1152, 600, 599, 607, 624, B_POSITIVE_HSYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS},
{ { 65000, 1024, 1048, 1184, 1344, 768, 771, 777, 806, 0}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1024X768X8.Z1) */
#else
{ { 65000, 640, 696, 744, 1024, 480, 481, 485, 530, T_POSITIVE_SYNC}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS},
{ { 65000, 800, 848, 976, 1144, 600, 601, 607, 624, T_POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS},
{ { 80160, 1024, 1064, 1088, 1336, 768, 771, 777, 800, T_POSITIVE_SYNC}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS},
#endif
//{ { 71400, 1024, 1046, 1082, 1190, 768, 770, 774, 800, T_POSITIVE_SYNC}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}
#endif
};
#else
static const display_mode mode_list[] = {
{ { 25175, 640, 656, 752, 800, 480, 490, 492, 525, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(640X480X8.Z1) */
{ { 27500, 640, 672, 768, 864, 480, 488, 494, 530, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* 640X480X60Hz */
{ { 30500, 640, 672, 768, 864, 480, 517, 523, 588, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* SVGA_640X480X60HzNI */
{ { 31500, 640, 664, 704, 832, 480, 489, 492, 520, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70-72Hz_(640X480X8.Z1) */
{ { 31500, 640, 656, 720, 840, 480, 481, 484, 500, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(640X480X8.Z1) */
{ { 36000, 640, 696, 752, 832, 480, 481, 484, 509, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(640X480X8.Z1) */
{ { 38100, 800, 832, 960, 1088, 600, 602, 606, 620, 0}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* SVGA_800X600X56HzNI */
{ { 40000, 800, 840, 968, 1056, 600, 601, 605, 628, T_POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(800X600X8.Z1) */
{ { 49500, 800, 816, 896, 1056, 600, 601, 604, 625, T_POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(800X600X8.Z1) */
{ { 50000, 800, 856, 976, 1040, 600, 637, 643, 666, T_POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70-72Hz_(800X600X8.Z1) */
{ { 56250, 800, 832, 896, 1048, 600, 601, 604, 631, T_POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(800X600X8.Z1) */
{ { 46600, 1024, 1088, 1216, 1312, 384, 385, 388, 404, B_TIMING_INTERLACED}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}, /* SVGA_1024X768X43HzI */
{ { 65000, 1024, 1048, 1184, 1344, 768, 771, 777, 806, 0}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1024X768X8.Z1) */
{ { 75000, 1024, 1048, 1184, 1328, 768, 771, 777, 806, 0}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70-72Hz_(1024X768X8.Z1) */
{ { 78750, 1024, 1040, 1136, 1312, 768, 769, 772, 800, T_POSITIVE_SYNC}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1024X768X8.Z1) */
{ { 94500, 1024, 1072, 1168, 1376, 768, 769, 772, 808, T_POSITIVE_SYNC}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(1024X768X8.Z1) */
{ { 94200, 1152, 1184, 1280, 1472, 864, 865, 868, 914, T_POSITIVE_SYNC}, B_CMAP8, 1152, 864, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70Hz_(1152X864X8.Z1) */
{ { 108000, 1152, 1216, 1344, 1600, 864, 865, 868, 900, T_POSITIVE_SYNC}, B_CMAP8, 1152, 864, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1152X864X8.Z1) */
{ { 121500, 1152, 1216, 1344, 1568, 864, 865, 868, 911, T_POSITIVE_SYNC}, B_CMAP8, 1152, 864, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(1152X864X8.Z1) */
{ { 108000, 1280, 1328, 1440, 1688, 1024, 1025, 1028, 1066, T_POSITIVE_SYNC}, B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1280X1024X8.Z1) */
{ { 135000, 1280, 1296, 1440, 1688, 1024, 1025, 1028, 1066, T_POSITIVE_SYNC}, B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1280X1024X8.Z1) */
{ { 157500, 1280, 1344, 1504, 1728, 1024, 1025, 1028, 1072, T_POSITIVE_SYNC}, B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(1280X1024X8.Z1) */
{ { 162000, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1600X1200X8.Z1) */
{ { 175500, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@65Hz_(1600X1200X8.Z1) */
{ { 189000, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70Hz_(1600X1200X8.Z1) */
{ { 202500, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1600X1200X8.Z1) */
{ { 216000, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@80Hz_(1600X1200X8.Z1) */
{ { 229500, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}  /* Vesa_Monitor_@85Hz_(1600X1200X8.Z1) */
};
#endif

status_t PROPOSE_DISPLAY_MODE(display_mode *target, const display_mode *low, const display_mode *high) {
#if 0
EMACHINE
	/* The EMachine eOne integrated monitor is not tolerant at all, so
	   we will not tolerate any freedom for the sync settings. */
	if (target->timing.h_display == 640) {
#if 0
		target->timing.h_sync_start = 816;
		target->timing.h_sync_end = 1024;
		target->timing.h_total = 1116;
		target->timing.v_display = 480;
		target->timing.v_sync_start = 486;
		target->timing.v_sync_end = 490;
		target->timing.v_total = 528;
		target->timing.pixel_clock = 70709;
#else
		ddprintf(("using mode_list[0].timing for 640x480\n"));
		target->timing = mode_list[0].timing;
#endif
	}
	else if (target->timing.h_display == 800) {
#if 0
		target->timing.h_sync_start = 820;
		target->timing.h_sync_end = 846;
		target->timing.h_total = 1192;
		target->timing.v_display = 600;
		target->timing.v_sync_start = 601;
		target->timing.v_sync_end = 604;
		target->timing.v_total = 628;
		target->timing.pixel_clock = 67371;
#else
		ddprintf(("using mode_list[1].timing for 800x600\n"));
		target->timing = mode_list[1].timing;
#endif
	}
	else if (target->timing.h_display == 1024) {
#if 0
		target->timing.h_sync_start = 1046;
		target->timing.h_sync_end = 1082;
		target->timing.h_total = 1190;
		target->timing.v_display = 768;
		target->timing.v_sync_start = 770;
		target->timing.v_sync_end = 774;
		target->timing.v_total = 800;
		target->timing.pixel_clock = 71400;
#else
		ddprintf(("using mode_list[2].timing for 1024x768\n"));
		target->timing = mode_list[2].timing;
#endif
	}
	else {
		ddprintf(("Ooops: PROPOSE_DISPLAY_MODE failed to match h_display!\n"));
		*target = mode_list[0];
	}
	return B_OK;
#else
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

	/* validate horizontal timings */
	{
		/* horizontal must be multiples of 8 */
		uint16 h_display = target->timing.h_display >> 3;
		uint16 h_sync_start = target->timing.h_sync_start >> 3;
		uint16 h_sync_end = target->timing.h_sync_end >> 3;
		uint16 h_total = target->timing.h_total >> 3;

		/* ensure reasonable minium display and sequential order of parms */
		if (h_display < (320 >> 3)) h_display = 320 >> 3;
		if (h_display > (2048 >> 3)) h_display = 2048 >> 3;
#if 0
		if (h_sync_start < (h_display + 2)) h_sync_start = h_display + 2;
		if (h_sync_end < (h_sync_start + 3)) h_sync_end = h_sync_start + 3; /*(0x001f >> 2);*/
#endif
		if (h_total < (h_sync_end + 1)) h_total = h_sync_end + 1;
		/* adjust for register limitations: */
		/* h_total is 9 bits */
		if (h_total > 0x01ff) h_total = 0x01ff;
		/* h_display is 8 bits - handled above */
		/* h_sync_start is 9 bits */
		/* h_sync_width is 5 bits */
		if ((h_sync_end - h_sync_start) > 0x001f) h_sync_end = h_sync_start + 0x001f;

		target->timing.h_display = h_display << 3;
		target->timing.h_sync_start = h_sync_start << 3;
		target->timing.h_sync_end = h_sync_end << 3;
		target->timing.h_total = h_total << 3;
	}
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
		if (v_display < 200) v_display = 200;
		if (v_display > (0x7ff - 5)) v_display = (0x7ff - 5); /* leave room for the sync pulse */
#if 0
		if (v_sync_start < (v_display + 1)) v_sync_start = v_display + 1;
#endif
		if (v_sync_end < v_sync_start) v_sync_end = v_sync_start + 3;
		if (v_total < (v_sync_end + 1)) v_total = v_sync_end + 1;
		/* adjust for register limitations */
		/* v_display is 11 bits */
		/* v_total is 11 bits */
		/* v_sync_start is 11 bits */
		/* v_sync_width is 5 bits */
		if ((v_sync_end - v_sync_start) > 0x001f) v_sync_end = v_sync_start + 0x001f;
		target->timing.v_display = v_display;
		target->timing.v_sync_start = v_sync_start;
		target->timing.v_sync_end = v_sync_end;
		target->timing.v_total = v_total;
	}
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

#if EMACHINE
	if (target->timing.h_display < 1024)
		target->timing.pixel_clock = 65000;
	else
		target->timing.pixel_clock = 80160;
#else
	/* adjust pixel clock for DAC limits and target refresh rate */
	target->timing.pixel_clock = target_refresh * ((double)target->timing.h_total) * ((double)target->timing.v_total) / 1000.0;
#endif
	switch (target->space & 0x0fff) {
		case B_CMAP8:
			limit_clock = ai->pix_clk_max8;
			row_bytes = 1;
			break;
		case B_RGB15:
		case B_RGB16:
			limit_clock = ai->pix_clk_max16;
			row_bytes = 2;
			break;
		case B_RGB32:
			limit_clock = ai->pix_clk_max32;
			row_bytes = 4;
			break;
		default:
			limit_clock = 0;
			row_bytes = 0;
	}
	if (target->timing.pixel_clock > limit_clock) target->timing.pixel_clock = limit_clock;
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
	if ((row_bytes * target->virtual_height) > ai->mem_size)
		target->virtual_height = ai->mem_size / row_bytes;

	if (target->virtual_height > 2048)
		target->virtual_height = 2048;

	if (target->virtual_height < target->timing.v_display)
		/* not enough frame buffer memory for the mode */
		result = B_ERROR;
	else if (
		(target->virtual_height < low->virtual_height) ||
		(target->virtual_height > high->virtual_height)
	) result = B_BAD_VALUE;
	
	/* flags? */
	return result;
#endif
}

uint32 ACCELERANT_MODE_COUNT(void) {
	/* return the number of 'built-in' display modes */
	return ai->mode_count;
}

status_t GET_MODE_LIST(display_mode *dm) {
	/* copy them to the buffer pointed at by *dm */
	memcpy(dm, my_mode_list, ai->mode_count * sizeof(display_mode));
	return B_OK;
}


void create_mode_list(void) {
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
#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
	color_space spaces[4] = {B_CMAP8, B_RGB15_LITTLE, B_RGB16_LITTLE, B_RGB32_LITTLE};
#else
	color_space spaces[4] = {B_CMAP8, B_RGB15_BIG, B_RGB16_BIG, B_RGB32_BIG};
#endif
	/* figure out how big the list could be, and adjust up to nearest multiple of B_PAGE_SIZE */
	max_size = (((MODE_COUNT * 4) * sizeof(display_mode)) + (B_PAGE_SIZE-1)) & ~(B_PAGE_SIZE-1);
	/* create an area to hold the info */
	ai->mode_list_area = my_mode_list_area =
		create_area("ATI accelerant mode info", (void **)&my_mode_list, B_ANY_ADDRESS, max_size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (my_mode_list_area < B_OK) return;

	/* walk through our predefined list and see which modes fit this device */
	dst = my_mode_list;
	ai->mode_count = 0;
	/* do it once for each depth we want to support */
	for (j = 0; j < (sizeof(spaces) / sizeof(color_space)); j++) {
		src = mode_list;
		for (i = 0; i < MODE_COUNT; i++) {
			/* set ranges for acceptable values */
			low = high = *src;
			/* range is 6.25% of default clock: arbitrarily picked */ 
			pix_clk_range = low.timing.pixel_clock >> 5;
			low.timing.pixel_clock -= pix_clk_range;
			high.timing.pixel_clock += pix_clk_range;
			/* some cards need wider virtual widths for certain modes */
			high.virtual_width = 1920;
			/* set target values */
			*dst = *src;
			/* poke the specific space */
			dst->space = low.space = high.space = spaces[j];
			/* ask for a compatible mode */
			if (PROPOSE_DISPLAY_MODE(dst, &low, &high) != B_ERROR) {
				if (!ai->mode_count || (ai->mode_count && memcmp(dst, dst-1, sizeof(*dst)))) {
					/* count it, and move on to next mode */
					ddprintf(("%dx%d 0x%04lx\n", dst->timing.h_display, dst->timing.v_display, dst->space));
					dst++;
					ai->mode_count++;
				}
			}
			/* advance to next mode */
			src++;
		}
	}
}
