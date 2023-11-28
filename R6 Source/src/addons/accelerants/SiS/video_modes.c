#include "sisGlobals.h"
#include "accelerant.h"
#include "sis5598defs.h"
#include "sis6326defs.h"
#include "sis620defs.h"
#include "sis630defs.h"


#define T_POSITIVE_SYNC (B_POSITIVE_HSYNC | B_POSITIVE_VSYNC)
#define MODE_FLAGS      ( B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS | B_DPMS | B_SUPPORTS_OVERLAYS )

const display_mode standard_display_modes[] =
{
	// Sync polarities swiped from Trey's mode tables.
	// CRTC parameters calculated using our formulae.
// this one in first place because it may be the nicest so it is set as default mode
  { {  56250,  800,  832,  896, 1048,  600,  601,  604,  631,     T_POSITIVE_SYNC}, B_CMAP8,  800,  600, 0, 0, MODE_FLAGS}, // Vesa_Monitor_@85Hz_(800X600X8.Z1)
  { {  25175,  640,  656,  752,  800,  480,  490,  492,  525,                   0}, B_CMAP8,  640,  480, 0, 0, MODE_FLAGS}, // Vesa_Monitor_@60Hz_(640X480X8.Z1)

  { {  27500,  640,  672,  768,  864,  480,  488,  494,  530,                   0}, B_CMAP8,  640,  480, 0, 0, MODE_FLAGS}, // 640X480X60Hz
  { {  30500,  640,  672,  768,  864,  480,  517,  523,  588,                   0}, B_CMAP8,  640,  480, 0, 0, MODE_FLAGS}, // SVGA_640X480X60HzNI
  { {  31500,  640,  664,  704,  832,  480,  489,  492,  520,                   0}, B_CMAP8,  640,  480, 0, 0, MODE_FLAGS}, // Vesa_Monitor_@70-72Hz_(640X480X8.Z1)
  { {  31500,  640,  656,  720,  840,  480,  481,  484,  500,                   0}, B_CMAP8,  640,  480, 0, 0, MODE_FLAGS}, // Vesa_Monitor_@75Hz_(640X480X8.Z1)
  { {  36000,  640,  696,  752,  832,  480,  481,  484,  509,                   0}, B_CMAP8,  640,  480, 0, 0, MODE_FLAGS}, // Vesa_Monitor_@85Hz_(640X480X8.Z1)
  { {  38100,  800,  832,  960, 1088,  600,  602,  606,  620,                   0}, B_CMAP8,  800,  600, 0, 0, MODE_FLAGS}, // SVGA_800X600X56HzNI
  { {  40000,  800,  840,  968, 1056,  600,  601,  605,  628,     T_POSITIVE_SYNC}, B_CMAP8,  800,  600, 0, 0, MODE_FLAGS}, // Vesa_Monitor_@60Hz_(800X600X8.Z1)
  { {  49500,  800,  816,  896, 1056,  600,  601,  604,  625,     T_POSITIVE_SYNC}, B_CMAP8,  800,  600, 0, 0, MODE_FLAGS}, // Vesa_Monitor_@75Hz_(800X600X8.Z1)
  { {  50000,  800,  856,  976, 1040,  600,  637,  643,  666,     T_POSITIVE_SYNC}, B_CMAP8,  800,  600, 0, 0, MODE_FLAGS}, // Vesa_Monitor_@70-72Hz_(800X600X8.Z1)

  { {  65000, 1024, 1048, 1184, 1344,  768,  771,  777,  806,                   0}, B_CMAP8, 1024,  768, 0, 0, MODE_FLAGS}, // Vesa_Monitor_@60Hz_(1024X768X8.Z1)
  { {  75000, 1024, 1048, 1184, 1328,  768,  771,  777,  806,                   0}, B_CMAP8, 1024,  768, 0, 0, MODE_FLAGS}, // Vesa_Monitor_@70-72Hz_(1024X768X8.Z1)
  { {  78750, 1024, 1040, 1136, 1312,  768,  769,  772,  800,     T_POSITIVE_SYNC}, B_CMAP8, 1024,  768, 0, 0, MODE_FLAGS}, // Vesa_Monitor_@75Hz_(1024X768X8.Z1)
  { {  94500, 1024, 1072, 1168, 1376,  768,  769,  772,  808,     T_POSITIVE_SYNC}, B_CMAP8, 1024,  768, 0, 0, MODE_FLAGS}, // Vesa_Monitor_@85Hz_(1024X768X8.Z1)
  {	{  94200, 1152, 1184, 1280, 1472,  864,  865,  868,  914,     T_POSITIVE_SYNC},	B_CMAP8, 1152,  864, 0, 0, MODE_FLAGS}, 
  {	{ 108000, 1152, 1216, 1344, 1600,  864,  865,  868,  900,     T_POSITIVE_SYNC},	B_CMAP8, 1152,  864, 0, 0, MODE_FLAGS},
  {	{ 121500, 1152, 1216, 1344, 1568,  864,  865,  868,  911,     T_POSITIVE_SYNC},	B_CMAP8, 1152,  864, 0, 0, MODE_FLAGS},
  { { 108000, 1280, 1328, 1440, 1688, 1024, 1025, 1028, 1066,     T_POSITIVE_SYNC}, B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS}, // Vesa_Monitor_@60Hz_(1280X1024X8.Z1)
  { { 135000, 1280, 1296, 1440, 1688, 1024, 1025, 1028, 1066,     T_POSITIVE_SYNC}, B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS}, // Vesa_Monitor_@75Hz_(1280X1024X8.Z1)
  { { 157500, 1280, 1344, 1504, 1728, 1024, 1025, 1028, 1072,     T_POSITIVE_SYNC}, B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS}, // Vesa_Monitor_@85Hz_(1280X1024X8.Z1)
  {	{ 162000, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250,     T_POSITIVE_SYNC},	B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS},
  {	{ 175500, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250,     T_POSITIVE_SYNC},	B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS},
  {	{ 189000, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250,     T_POSITIVE_SYNC},	B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS},
  {	{ 202500, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250,     T_POSITIVE_SYNC},	B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS},
  {	{ 216000, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250,     T_POSITIVE_SYNC},	B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS},
  {	{ 229500, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250,     T_POSITIVE_SYNC},	B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}
};

#define	NMODES		(sizeof (standard_display_modes) / sizeof (display_mode))


/////////////////////////
// PROPOSE_DISPLAY_MODE
/////////////////////////


status_t propose_video_mode (
	display_mode		*target,
	const display_mode	*low,
	const display_mode	*high
	)
	{
	status_t	result = B_OK;
	double		target_refresh;
	uint32		row_bytes, pixel_bytes, limit_clock_lo;
	uint16		width, height, dispwide, disphigh;
	bool		want_same_width, want_same_height;

	// first check color mode
	if ((target->space == B_CMAP8) || (target->space == B_RGB16_LITTLE)) goto color_mode_ok;
	if (target->space == B_RGB32_LITTLE ) {
		if (ci->ci_DeviceId == SIS620_DEVICEID) goto color_mode_ok;
		if (ci->ci_DeviceId == SIS630_DEVICEID) goto color_mode_ok;
		}
	vvddprintf(("sis: color mode(%d) can not be accepted\n",target->space));
	return(B_ERROR);
	
color_mode_ok:
		
	target_refresh = ((double) target->timing.pixel_clock * 1000.0) /
			 ((double) target->timing.h_total *
			  (double) target->timing.v_total);
	vvddprintf(("sis: _propose_video_mode target_refresh=%d\n",(int)target_refresh));
	
	want_same_width = target->timing.h_display == target->virtual_width;
	want_same_height = target->timing.v_display == target->virtual_height;
	width = height = dispwide = disphigh = 0;

revalidate:
      {
	uint16 h_display	= ((target->timing.h_display + 31) & ~31) >> 3;
	uint16 h_sync_start	= target->timing.h_sync_start >> 3;
	uint16 h_sync_end	= target->timing.h_sync_end >> 3;
	uint16 h_total		= target->timing.h_total >> 3;

	/*  Ensure reasonable minium display and sequential order of parms  */
	if (h_display < (320 >> 3))		h_display = 320 >> 3;
	if (h_display > (2048 >> 3))		h_display = 2048 >> 3;
	if (h_sync_start < (h_display + 2))	h_sync_start = h_display + 2;
	if (h_sync_end < (h_sync_start + 3))	h_sync_end = h_sync_start + 3;
						/*(0x001f >> 2);*/
	if (h_total < (h_sync_end + 1))		h_total = h_sync_end + 1;

	/*  Adjust for register limitations  */
	if (h_total - h_display > 0x007f) {
		h_total = h_display + 0x007f;
		if (h_sync_end > h_total - 1)
			h_sync_end = h_total - 1;
		if (h_sync_start > h_sync_end)
			h_sync_start = h_sync_end - 0x001f;
	}
	if (h_sync_end - h_sync_start > 0x001f)
		h_sync_end = h_sync_start + 0x001f;

	target->timing.h_display	= h_display << 3;
	target->timing.h_sync_start	= h_sync_start << 3;
	target->timing.h_sync_end	= h_sync_end << 3;
	target->timing.h_total		= h_total << 3;
      }
	if (target->timing.h_display < low->timing.h_display  ||
	    target->timing.h_display > high->timing.h_display  ||
	    target->timing.h_sync_start < low->timing.h_sync_start  ||
	    target->timing.h_sync_start > high->timing.h_sync_start  ||
	    target->timing.h_sync_end < low->timing.h_sync_end  ||
	    target->timing.h_sync_end > high->timing.h_sync_end  ||
	    target->timing.h_total < low->timing.h_total  ||
	    target->timing.h_total > high->timing.h_total)
		result = B_BAD_VALUE;

      {
	/*  Validate vertical timings  */
	uint16 v_display	= target->timing.v_display;
	uint16 v_sync_start	= target->timing.v_sync_start;
	uint16 v_sync_end	= target->timing.v_sync_end;
	uint16 v_total		= target->timing.v_total;

	/*  Ensure reasonable minium display and sequential order of parms  */
	if (v_display < 200)			v_display = 200;
	if (v_display > 2048)			v_display = 2048; /* ha! */
	if (v_sync_start < (v_display + 1))	v_sync_start = v_display + 1;
	if (v_sync_end < v_sync_start)		v_sync_end = v_sync_start +
							      (0x000f >> 2);
	if (v_total < (v_sync_end + 1))		v_total = v_sync_end + 1;

	/*  Adjust for register limitations  */
	if (v_total - v_display > 0x00ff) {
		v_total = v_display + 0x00ff;
		if (v_sync_end > v_total - 1)
			v_sync_end = v_total - 1;
		if (v_sync_start > v_sync_end)
			v_sync_start = v_sync_end - 0x000f;
	}
	if (v_sync_end - v_sync_start > 0x000f)
		v_sync_end = v_sync_start + 0x000f;

	target->timing.v_display	= v_display;
	target->timing.v_sync_start	= v_sync_start;
	target->timing.v_sync_end	= v_sync_end;
	target->timing.v_total		= v_total;
      }
	if (target->timing.v_display < low->timing.v_display  ||
	    target->timing.v_display > high->timing.v_display  ||
	    target->timing.v_sync_start < low->timing.v_sync_start  ||
	    target->timing.v_sync_start > high->timing.h_sync_start  ||
	    target->timing.v_sync_end < low->timing.v_sync_end  ||
	    target->timing.v_sync_end > high->timing.v_sync_end  ||
	    target->timing.v_total < low->timing.v_total  ||
	    target->timing.v_total > high->timing.v_total)
		result = B_BAD_VALUE;

	vvddprintf(("sis: propose_mode: timings %s\n",(result ? "FAILED" : "OK")));

	width = target->virtual_width;
	height = target->virtual_height;
	dispwide = target->timing.h_display;
	disphigh = target->timing.v_display;

	if (target->timing.h_display > target->virtual_width  ||
	    want_same_width)
		target->virtual_width = target->timing.h_display;
	if (target->timing.v_display > target->virtual_height  ||
	    want_same_height)
		target->virtual_height = target->timing.v_display;
	if (target->virtual_width > 2048)
		target->virtual_width = 2048;

	pixel_bytes = (colorspacebits (target->space) + 7) >> 3;

	row_bytes = (pixel_bytes * target->virtual_width + 7) & ~7;
	target->virtual_width = row_bytes / pixel_bytes;
	if (want_same_width)
		target->timing.h_display = target->virtual_width;

	/*  Adjust virtual width for engine limitations  */
	if (target->virtual_width < low->virtual_width  ||
	    target->virtual_width > high->virtual_width)
		result = B_BAD_VALUE;

	vvddprintf(("sis: propose_mode: virtual size %s\n",(result ? "FAILED" : "OK")));

	/*  Memory requirement for frame buffer  */
	vvddprintf(("sis: rowbyte=%d virtual_height=%d mem=%d\n",row_bytes,target->virtual_height,ci->ci_MemSize));
	if (row_bytes * target->virtual_height >
	    (ci->ci_MemSize<<10)) {
	    target->virtual_height = (ci->ci_MemSize<<10) / row_bytes;
	    // or else : result = B_BAD_VALUE;
	    }
		
	if (target->virtual_height < target->timing.v_display)
		/* not enough frame buffer memory for the mode */
		result = B_ERROR;
	else if (target->virtual_height < low->virtual_height  ||
		 target->virtual_height > high->virtual_height)
		result = B_BAD_VALUE;

	vvddprintf(("sis: propose_mode: memory depending calculated height %s\n",(result ? "FAILED" : "OK")));

	if (width != target->virtual_width  ||
	    height != target->virtual_height  ||
	    dispwide != target->timing.h_display  ||
	    disphigh != target->timing.v_display)
		/*  Something changed; we have to re-check.  */
		goto revalidate;	/*  Look up  */

	/*
	 * Adjust pixel clock for DAC limits and target refresh rate
	 * pixel_clock is recalculated because [hv]_total may have been nudged.
	 */
	target->timing.pixel_clock = target_refresh *
				     ((double) target->timing.h_total) *
				     ((double) target->timing.v_total) /
				     1000.0 + 0.5;
	vvddprintf(("sis: _propose_video_mode / target->timing.pixel_clock = %d\n",target->timing.pixel_clock));
	limit_clock_lo = 48.0 *	/*  Until a monitors database does it...  */
			 ((double) target->timing.h_total) *
			 ((double) target->timing.v_total) /
			 1000.0;
	if (target->timing.pixel_clock > ci->ci_Clock_Max)
		target->timing.pixel_clock = ci->ci_Clock_Max;
	if (target->timing.pixel_clock < limit_clock_lo)
		target->timing.pixel_clock = limit_clock_lo;
	if (target->timing.pixel_clock < low->timing.pixel_clock  ||
	    target->timing.pixel_clock > high->timing.pixel_clock)
		result = B_BAD_VALUE;

	target->flags |= MODE_FLAGS ;	
	vvddprintf(("sis: propose_mode: clock %s\n",(result ? "FAILED" : "OK")));

	/*  flags?  */
	return (result);
	}



///////////////////////
// MODE CONFIGURATION
///////////////////////

static status_t calcmodes (void) {
	if ((ci->ci_NDispModes = testbuildmodes (NULL))>0) {	// First count the modes so we know how much memory to allocate.
		if ((ci->ci_DispModes = malloc ((ci->ci_NDispModes + 1) * sizeof(display_mode)))!=NULL) {
			testbuildmodes (ci->ci_DispModes);
			return (B_OK);
			}
		}
	return (B_ERROR);
	}

static int testbuildmodes (register display_mode *dst) {
	const display_mode	*src;
	display_mode		low, high, target;
	uint32				i, j, pix_clk_range;
	int					count;

	static color_space sis5598_spaces[] = { B_CMAP8 , B_RGB16_LITTLE };
	static color_space sis6326_spaces[] = { B_CMAP8 , B_RGB16_LITTLE };
	static color_space sis620_spaces[] = { B_CMAP8 , B_RGB16_LITTLE , B_RGB32_LITTLE };
	static color_space sis630_spaces[] = { B_CMAP8 , B_RGB16_LITTLE , B_RGB32_LITTLE };
#define	sis5598_NSPACES		(sizeof (sis5598_spaces) / sizeof (color_space))
#define	sis6326_NSPACES		(sizeof (sis6326_spaces) / sizeof (color_space))
#define	sis620_NSPACES		(sizeof (sis620_spaces) / sizeof (color_space))
#define	sis630_NSPACES		(sizeof (sis630_spaces) / sizeof (color_space))
	color_space *spaces;
	int n_spaces;

	vddprintf(("sis: Creating VideoModes list\n"));	
	switch(ci->ci_DeviceId) {
		case SIS5598_DEVICEID:
		case SIS6326_DEVICEID:
			spaces = sis5598_spaces;
			n_spaces = sis5598_NSPACES;
			break;
		case SIS620_DEVICEID:
			spaces = sis620_spaces;
			n_spaces = sis620_NSPACES;
			break;
		case SIS630_DEVICEID:
			spaces = sis630_spaces;
			n_spaces = sis630_NSPACES;
			break;
		default:
			ddprintf(("sis: space configuration for this chipset not found\n"));
			return(0);
		}
		
	// Walk through our predefined list and see which modes fit this device.
	src = standard_display_modes;
	count = 0;
	for (i = 0;  i < NMODES;  i++) {
		/* set ranges for acceptable values */
		low = high = *src;
		/* range is 6.25% of default clock: arbitrarily picked */ 
		pix_clk_range = low.timing.pixel_clock >> 5;
		low.timing.pixel_clock -= pix_clk_range;
		high.timing.pixel_clock += pix_clk_range;

		vvddprintf(("sis : buildmodes, mode number %d ",i));
		
		// Test each color_space we support.
		for (j = 0;  j < n_spaces;  j++) {
			target = *src;
			target.space = low.space = high.space = spaces[j];
			// debug : target.flags |= B_IO_FB_NA ; // B_LAME_ADD_CARD
			if (propose_video_mode (&target, &low, &high) == B_OK) {
				//  It's good; record it if there's storage
				if (dst) *dst++ = target;
				count++;
				vvddprintf(("accepted\n"));
				}
			else vvddprintf(("refused\n"));
			}
			// advance to next mode
			src++;
		}
	return (count);
	}


uint32 _get_accelerant_mode_count (void) {
	vddprintf(("sis_accelerant: B_ACCELERANT_MODE_COUNT\n"));
	if (ci->ci_NDispModes==-1) calcmodes();
	else vddprintf(("sis: display modes have already been counted\n"));
	vddprintf(("sis_accelerant:     there are %d modes\n",ci->ci_NDispModes));	
	return(ci->ci_NDispModes);
	}

status_t _get_mode_list (display_mode *dm) {
	vddprintf(("sis_accelerant: B_GET_MODE_LIST\n"));
	memcpy (dm,
			ci->ci_DispModes,
			ci->ci_NDispModes*sizeof(display_mode));
	return(B_OK);
	}


status_t _propose_display_mode (display_mode *target, display_mode *low, display_mode *high) {
	status_t retval = propose_video_mode(target, low, high);
	vddprintf(("sis_accelerant: propose_display_mode() : answer is %s\n",(retval ? "OK" : "FAILED")));
	return(retval);
	}

