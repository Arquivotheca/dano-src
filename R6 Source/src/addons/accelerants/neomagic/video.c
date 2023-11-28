/* 
 * video.c:	The Very Nasty Stuff Indeed.
 *
 * Copyright 2000 Be Incorporated.  Confidential unpublished work.
 * All Rights Reserved.
 */
#include <kernel/OS.h>
#include <support/Debug.h>
#include <add-ons/graphics/GraphicsCard.h>
#include <add-ons/graphics/Accelerant.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <PCI.h>

#include <graphics_p/neomagic/neomagic.h>
#include <graphics_p/neomagic/neomagic_ioctls.h>
#include <graphics_p/neomagic/debug.h>
#include <graphics_p/neomagic/bittwiddle.h>
#include <graphics_p/video_overlay.h>

#include "protos.h"

/*****************************************************************************
 * #defines
 */
#define MASTERCLK	 14318.18	/*  The frequency of the Beast	kHz*/

#define	MODE_FLAGS	(B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS /*| B_SUPPORTS_OVERLAYS*/)
#define	T_POSITIVE_SYNC	(B_POSITIVE_HSYNC | B_POSITIVE_VSYNC)

#define SCREEN_OFF	write_vga(0x3c4, 0x01); write_vga(0x3c5, 0x21);	snooze(1000);
#define SCREEN_ON	write_vga(0x3c4, 0x01); write_vga(0x3c5, 0x01);	snooze(1000);

/*****************************************************************************
 * Local prototypes.
 */
status_t calcmodes ();
int testbuildmodes (register display_mode *dst);
status_t propose_video_mode (display_mode		*target,const display_mode	*low,const display_mode	*high);
uint32 colorspacebits (uint32 cs);
int32 GetMemSize();
status_t SetupCRTC (register display_mode		*dm);
void CalcVCLK(long freq, uint8 *denominator, uint8 *numerator_high, uint8 *numerator_low);
void write_vga(uint32 offset, uint8 value);
uint8 read_vga(uint32 offset);
void write_vga_crtc(uint8 index, uint8 value);
uint8 read_vga_crtc(uint8 index);
void write_vga_grax(uint8 index, uint8 value);
uint8 read_vga_grax(uint8 index);

/*****************************************************************************
 * Globals.
 */
extern neomagic_card_info	*ci;
extern int devfd;

static uint32 curr_dpms_mode;
static bool		stretching_enabled;
static bool		centering_enabled;
static bool		panel_enabled;

/*
 * This table is formatted for 132 columns, so stretch that window...
 * Stolen from Trey's R4 Matrox driver and reformatted to improve readability
 * (after a fashion).  
 */
static const display_mode	mode_list[] = {
/* Vesa_Monitor_@60Hz_(640X480X8.Z1) */
 {	{  25175,  640,  656,  752,  800,  480,  490,  492,  525, 0},			B_CMAP8,  640,  480, 0, 0, MODE_FLAGS},
/* 640X480X60Hz */
 {	{  27500,  640,  672,  768,  800,  480,  488,  494,  530, 0},			B_CMAP8,  640,  480, 0, 0, MODE_FLAGS},
/* SVGA_640X480X60HzNI */
 {	{  30500,  640,  672,  768,  800,  480,  517,  523,  588, 0},			B_CMAP8,  640,  480, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@70-72Hz_(640X480X8.Z1) */
 {	{  31500,  640,  672,  768,  800,  480,  489,  492,  520, 0},			B_CMAP8,  640,  480, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@75Hz_(640X480X8.Z1) */
 {	{  31500,  640,  672,  736,  840,  480,  481,  484,  500, 0},			B_CMAP8,  640,  480, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@85Hz_(640X480X8.Z1) */
 {	{  36000,  640,  712,  768,  832,  480,  481,  484,  509, 0},			B_CMAP8,  640,  480, 0, 0, MODE_FLAGS},
/* SVGA_800X600X56HzNI */
 {	{  38100,  800,  832,  960, 1088,  600,  602,  606,  620, 0},			B_CMAP8,  800,  600, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@60Hz_(800X600X8.Z1) */
 {	{  40000,  800,  856,  984, 1056,  600,  601,  605,  628, T_POSITIVE_SYNC},	B_CMAP8,  800,  600, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@75Hz_(800X600X8.Z1) */
 {	{  49500,  800,  832,  912, 1056,  600,  601,  604,  625, T_POSITIVE_SYNC},	B_CMAP8,  800,  600, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@70-72Hz_(800X600X8.Z1) */
 {	{  50000,  800,  832,  912, 1056,  600,  637,  643,  666, T_POSITIVE_SYNC},	B_CMAP8,  800,  600, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@85Hz_(800X600X8.Z1) */
 {	{  56250,  800,  848,  912, 1048,  600,  601,  604,  631, T_POSITIVE_SYNC},	B_CMAP8,  800,  600, 0, 0, MODE_FLAGS},
/* SVGA_1024X768X43HzI */
 {	{  46600, 1024, 1088, 1216, 1312,  384,  385,  388,  404, B_TIMING_INTERLACED},	B_CMAP8, 1024,  768, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@60Hz_(1024X768X8.Z1) */
 {	{  65000, 1024, 1064, 1200, 1344,  768,  771,  777,  806, 0},			B_CMAP8, 1024,  768, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@70-72Hz_(1024X768X8.Z1) */
 {	{  75000, 1024, 1048, 1184, 1328,  768,  771,  777,  806, 0},			B_CMAP8, 1024,  768, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@75Hz_(1024X768X8.Z1) */
 {	{  78750, 1024, 1056, 1152, 1312,  768,  769,  772,  800, T_POSITIVE_SYNC},	B_CMAP8, 1024,  768, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@85Hz_(1024X768X8.Z1) */
 {	{  94500, 1024, 1088, 1184, 1376,  768,  769,  772,  808, T_POSITIVE_SYNC},	B_CMAP8, 1024,  768, 0, 0, MODE_FLAGS},
};
#define	NMODES		(sizeof (mode_list) / sizeof (display_mode))

static uint8 attribute_table[] =
{
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
	0x01, 0x00, 0x0f, 0x00, 0x00
};

/*****************************************************************************
 * Initialization sequence.
 */
status_t
neomagic_init ()
{
	status_t		retval = B_OK;
	neomagic_readwrite_vgareg vgareg;
	
	dprintf(("neomagic_accel: neomagic_init - ENTER\n"));

	/*  Determine available framebuffer memory  */

	ci->ci_MemSize = GetMemSize() /* kilobytes */ << 10;	
	ci->ci_MasterClockKHz = MASTERCLK;

	// We initialize the framebuffer to be at the beginning of the card
	ci->ci_FBBase		= (void *) ((uint8 *) ci->ci_BaseAddr0);
	ci->ci_FBBase_DMA	= (void *) ((uint8 *) ci->ci_base_registers_pci[0]);
	
	strcpy (ci->ci_ADI.name, "Neomagic board");
	strcpy (ci->ci_ADI.chipset, "Neomagic");
	ci->ci_ADI.dac_speed = CLOCK_MAX / 1000;
	ci->ci_ADI.memory = ci->ci_MemSize;
	ci->ci_ADI.version = B_ACCELERANT_VERSION;

	// Set the current DPMS Mode
	curr_dpms_mode = B_DPMS_ON;
	
	// Set the flags for stretching, centering & panel use
	if (getenv("NEOMAGIC_STRETCHING_ENABLED"))
		stretching_enabled = true;
	else
		stretching_enabled = false;
		
	if (getenv("NEOMAGIC_CENTERING_DISABLED"))
		centering_enabled = false;
	else
		centering_enabled = true;
	panel_enabled = true;
	
	retval = calcmodes();
	dprintf(("neomagic_accel: neomagic_init - EXIT, returning 0x%x\n", retval));
	return (retval);
}


status_t
calcmodes ()
{
	dprintf(("neomagic_accel: calcmodes - ENTER\n"));
	/*
	 * First count the modes so we know how much memory to allocate.
	 */
	if ( (ci->ci_NDispModes = testbuildmodes (NULL)) ) {
		/*
		 * Okay, build the list for real this time.
		 */
		if ( (ci->ci_DispModes = malloc ((ci->ci_NDispModes + 1) *
					       sizeof (display_mode))) )
		{
			testbuildmodes (ci->ci_DispModes);
			dprintf(("neomagic_accel: calcmodes - EXIT, returning B_OK\n"));
			return (B_OK);
		}
	}
	dprintf(("neomagic_accel: calcmodes - EXIT, returning B_ERROR\n"));
	return (B_ERROR);
}

int
testbuildmodes (display_mode *dst)
{
	const display_mode	*src;
	display_mode		low, high, try;
	uint32			i, j, pix_clk_range;
	int			count;

#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
	static color_space spaces[] =
		{ B_CMAP8, B_RGB15_LITTLE, B_RGB16_LITTLE /*, B_RGB32_LITTLE*/ };
#else
	static color_space spaces[] =
		{ B_CMAP8, B_RGB15_BIG, B_RGB16_BIG /*, B_RGB32_BIG*/ };
#endif
#define	NSPACES		(sizeof (spaces) / sizeof (color_space))

	dprintf(("neomagic_accel: testbuildmodes - ENTER\n"));
	/*
	 * Walk through our predefined list and see which modes fit this
	 * device.
	 */
	src = mode_list;
	count = 0;
	for (i = 0;  i < NMODES;  i++) {
		/* set ranges for acceptable values */
		low = high = *src;
		/* range is 6.25% of default clock: arbitrarily picked */ 
		pix_clk_range = low.timing.pixel_clock >> 5;
		low.timing.pixel_clock -= pix_clk_range;
		high.timing.pixel_clock += pix_clk_range;

		/*
		 * Test each color_space we support.
		 */
		for (j = 0;  j < NSPACES;  j++) {
			try = *src;
			try.space = low.space = high.space = spaces[j];
			if (propose_video_mode (&try, &low, &high) == B_OK)
			{
				/*  It's good; record it if there's storage  */
				if (dst)
				{
					*dst = try;
				  /* Advance to the next slot in the list */
					dst++;
				}
				count++;
			}
		}
		/* advance to next mode */
		src++;
	}

	dprintf(("neomagic_accel: testbuildmodes - EXIT, returning count = %d\n", count));
	return (count);
}


status_t
propose_video_mode (display_mode		*target, const display_mode	*low, const display_mode	*high)
{
	status_t	result = B_OK;
	double		target_refresh;
	uint32		row_bytes, pixel_bytes, limit_clock_lo;
	uint16		width, height, dispwide, disphigh;
	bool		want_same_width, want_same_height;
	
	//	dprintf(("neomagic_accel: propose_video_mode - ENTER\n"));
	
	
	target_refresh = ((double) target->timing.pixel_clock * 1000.0) /
		((double) target->timing.h_total *
		(double) target->timing.v_total);
	want_same_width = target->timing.h_display == target->virtual_width;
	want_same_height = target->timing.v_display == target->virtual_height;
	width = height = dispwide = disphigh = 0;
	
	/* 
	* Quantization to other hardware limits (performed later) may nudge
	* previously validated values, so we need to be able to re-validate.
	*/
	revalidate:
	{
		/*
		* Validate horizontal timings.
		* Horizontal must be multiples of 8.
		*/
		uint16 h_display	= ((target->timing.h_display + 31) & ~31) >> 3;
		uint16 h_sync_start	= target->timing.h_sync_start >> 3;
		uint16 h_sync_end	= target->timing.h_sync_end >> 3;
		uint16 h_total		= target->timing.h_total >> 3;
		
		/*  Ensure reasonable minium display and sequential order of parms  */
		if (h_display < (320 >> 3))		h_display = 320 >> 3;
		if (h_display > (2048 >> 3))		h_display = 2048 >> 3;
		if (h_sync_start < (h_display + 2))	h_sync_start = h_display + 2;
		if (h_sync_end < (h_sync_start + 3))	h_sync_end = h_sync_start + 3;
		if (h_total < (h_sync_end + 1))		h_total = h_sync_end + 1;
		
		/*  Adjust for register limitations  */
		if (h_total - h_display > 0x007f)
		{
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
	{
		//			dprintf(("neomagic_accel: propose_video_mode: horizontal timing out of range - returning B_BAD_VALUE\n"));
		result = B_BAD_VALUE;
	}

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
		if (v_sync_end < v_sync_start)		v_sync_end = v_sync_start +	(0x000f >> 2);
		if (v_total < (v_sync_end + 1))		v_total = v_sync_end + 1;
		
		/*  Adjust for register limitations  */
		if (v_total - v_display > 0x00ff)
		{
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
	{
		//			dprintf(("neomagic_accel: propose_video_mode: vertical timing out of range - returning B_BAD_VALUE\n"));
		result = B_BAD_VALUE;
	}

	/*
	* Validate display vs. virtual.
	* Save current settings so we can test later if they changed.
	*/
	width = target->virtual_width;
	height = target->virtual_height;
	dispwide = target->timing.h_display;
	disphigh = target->timing.v_display;
	
	if (target->timing.h_display > target->virtual_width  || want_same_width)
		target->virtual_width = target->timing.h_display;
	if (target->timing.v_display > target->virtual_height  ||	want_same_height)
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
	{
		//		dprintf(("neomagic_accel: propose_video_mode: virtual_width out of range - returning B_BAD_VALUE\n"));
		result = B_BAD_VALUE;
	}
	/*  Memory requirement for frame buffer  */
	if (row_bytes * target->virtual_height > ci->ci_MemSize)
		target->virtual_height = ci->ci_MemSize / row_bytes;
	
	if (target->virtual_height < target->timing.v_display)
	/* not enough frame buffer memory for the mode */
	{
		//		dprintf(("neomagic_accel: propose_video_mode: insufficient frame buffer memory - returning B_ERROR\n"));
		result = B_ERROR;
	}
	else if (target->virtual_height < low->virtual_height  ||
						target->virtual_height > high->virtual_height)
	{
		//		dprintf(("neomagic_accel: propose_video_mode: virtual_height out of range - returning B_BAD_VALUE\n"));
		result = B_BAD_VALUE;
	}
	
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
															limit_clock_lo = 48.0 *	/*  Until a monitors database does it...  */
															((double) target->timing.h_total) *
															((double) target->timing.v_total) /
															1000.0;
	if (target->timing.pixel_clock > CLOCK_MAX)
		target->timing.pixel_clock = CLOCK_MAX;
	if (target->timing.pixel_clock < limit_clock_lo)
		target->timing.pixel_clock = limit_clock_lo;
	if (target->timing.pixel_clock < low->timing.pixel_clock  ||
			target->timing.pixel_clock > high->timing.pixel_clock)
	{
		//		dprintf(("neomagic_accel: propose_video_mode: pixel_clock out of range - returning B_BAD_VALUE\n"));
		result = B_BAD_VALUE;
	}	
	
	//	dprintf(("neomagic_accel: propose_video_mode - EXIT, returning 0x%x\n", result));
	return (result);
}

//**************************************************************************
//  Space selection, on the fly...
//
//  This function is able to set any configuration available on the add-on
//  without restarting the machine. Until now, all cards works pretty well
//  with that. As Window's 95 is not really doing the same, we're still a
//  bit afraid to have a bad surprize some day... We'll see.
//  The configuration include space (depth and resolution), CRT_CONTROL and
//  refresh rate.
//**************************************************************************

status_t
vid_selectmode (register display_mode *dm,uint32 lockflags)
{
	status_t	retval;

//	dprintf(("neomagic_accel: vid_selectmode - ENTER\n"));
	/*
	 * We are about to Change The World, so grab everything.
	 */
	lockBena4 (&ci->ci_CRTCLock);
	lockBena4 (&ci->ci_CLUTLock);
	lockBena4 (&ci->ci_EngineLock);

	retval = SetupCRTC (dm);

	/*
	 * (Yes, there is a better way to do this thing with the locks.
	 * Howver, it was the first approach I thought of, and it works,
	 * and since we're in Beta cycle at the moment, I'm not going to
	 * fiddle with it right now and possibly break it...)
	 */
	if (!(lockflags & LEAVELOCKF_ENGINE))
		unlockBena4 (&ci->ci_EngineLock);
	if (!(lockflags & LEAVELOCKF_CLUT))
		unlockBena4 (&ci->ci_CLUTLock);
	if (!(lockflags & LEAVELOCKF_CRTC))
		unlockBena4 (&ci->ci_CRTCLock);

//	dprintf(("neomagic_accel: vid_selectmode - EXIT, returning 0x%x\n", retval));
	return (retval);
}

/*----------------------------------------------------------------------
Function name:  GetMemSize

Description:    Return the size of memory in KBs.
                
Information:

Return:         uint32   The size of memory in KBs.
----------------------------------------------------------------------*/
int32
GetMemSize()
{
	int32 memSize = 0;
	
	switch(ci->ci_device_id)
	{
		case DEVICEID_NM2070:
			memSize = 896;
			break;
		case DEVICEID_NM2090:
		case DEVICEID_NM2093:
		case DEVICEID_NM2097:
			memSize = 1152;
			break;
		case DEVICEID_NM2160:
			memSize = 2048;
			break;
		case DEVICEID_NM2200:
			memSize = 2560;
			break;
		case DEVICEID_NM256ZV:
			memSize = 4096;
			break;
	}
  return (memSize);
}

/*****************************************************************************
 * Display manipulation.
 */
status_t
SetupCRTC (register display_mode		*dm)
{
	int				bytespp, bytesperrow;
	int				refreshrate;
	int				hdisp, hsyncstart, hsyncend, htotal;
	int				vdisp, vsyncstart, vsyncend, vtotal;

	uint8			temp, tempPanelDispCntlReg1, tempPanelDispCntlReg2, tempPanelDispCntlReg3, tempColorModeSelect, idx, val, v;
	uint32		pixelFormat=0, stride, vidProcCfg, fb_offset, i, hoffset, voffset;
	bool			useClut=0;
	
	dprintf(("neomagic_accel: SetupCRTC - ENTER\n"));

	/* Convert the BeOS timing structure values into archaic VGA character clocks */
	hdisp = dm->timing.h_display >> 3;
	hsyncstart = dm->timing.h_sync_start >> 3;
	hsyncend = dm->timing.h_sync_end >> 3;
	htotal = dm->timing.h_total >> 3;

	dprintf(("neomagic_accel: SetupCRTC - hdisp = %d, hsyncstart = %d, hsyncend = %d, htotal = %d\n",
		hdisp, hsyncstart, hsyncend, htotal));

	vdisp = dm->timing.v_display;
	vsyncstart = dm->timing.v_sync_start;
	vsyncend = dm->timing.v_sync_end;
	vtotal = dm->timing.v_total;

	dprintf(("neomagic_accel: SetupCRTC - vdisp = %d, vsyncstart = %d, vsyncend = %d, vtotal = %d\n",
		vdisp, vsyncstart, vsyncend, vtotal));
		
	bytespp		= (colorspacebits (dm->space) + 7) >> 3;
	bytesperrow	= (dm->virtual_width >> 3) * bytespp;
	
	refreshrate = ((dm->timing.pixel_clock * 1000) / (dm->timing.h_total * dm->timing.v_total)) + 1;
	
	ci->ci_BytesPerRow = dm->virtual_width * bytespp;
	ci->ci_Depth = colorspacebits (dm->space);

	dprintf(("neomagic_accel: SetupCRTC - ci->ci_BytesPerRow  = %d, ci->ci_Depth = %d\n", ci->ci_BytesPerRow, ci->ci_Depth));
	
	// Unlock CRTC CR0-7
	temp = read_vga_crtc(0x11);
	write_vga_crtc(0x11, temp & 0x7f);
	
	// Unlock the extension registers
	write_vga_grax(0x09, 0x26);

	// Force to bank 0
	write_vga_grax(0x15, 0x00);
	
	SCREEN_OFF;

	/* Setup shadow register locking. */
	switch (0x02 /*Panel/CRT Switch*/)
	{
		case 0x01 : /* External CRT only mode: */
			write_vga_grax(NEO_GENERALLOCKREG, 0x00);
			/* We need to program the VCLK for external display only mode. */
			//new->ProgramVCLK = TRUE;
			break;
		case 0x02 : /* Internal LCD only mode: */
		case 0x03 : /* Simultaneous internal/external (LCD/CRT) mode: */
			write_vga_grax(NEO_GENERALLOCKREG, 0x01);
			/* Don't program the VCLK when using the LCD. */
			// new->ProgramVCLK = FALSE;
			break;
	}
	
	/*
	* Disable horizontal and vertical graphics and text expansions so
	* that vgaHWRestore works properly.
	*/
	temp = read_vga_grax(0x25);
	temp &= 0x39;				// Clear Character & Text Expansion modes (both horiz, & vertical)
	write_vga_grax(0x25, temp);
	/*
	* Sleep for 200ms to make sure that the two operations above have
	* had time to take effect.
	*/
	snooze(200000);
	
	// Set the standard VGA Registers
	// Write the Sequence Registers
	write_vga(0x3c4, 0x00); write_vga(0x3c5, 0x03);		// Reset
	
	SCREEN_OFF;
	
//	write_vga(0x3c4, 0x01); write_vga(0x3c5, 0x01);		// Characters are 9 clocks wide
	write_vga(0x3c4, 0x02); write_vga(0x3c5, 0x0f);		// Disable memory planes 0-3
	write_vga(0x3c4, 0x03); write_vga(0x3c5, 0x00);		// Select Charcater map 0
	write_vga(0x3c4, 0x04); write_vga(0x3c5, 0x0e);		// Extended Memory, normal addressing mode, 

	// Setup DAC
	temp = read_vga(0x3c6);
	temp = read_vga(0x3c6);
	temp = read_vga(0x3c6);
	temp = read_vga(0x3c6);
	write_vga(0x3c6, 0x00);
	temp = read_vga(0x3c8);
	write_vga(0x3c6, 0xff);

	// Set to use CLK2 (the parameterised clock)
	temp = read_vga(0x3cc);
	write_vga(0x3c2, (temp & 0xf3) | 0x08);

	// Write the Attribute Registers
	temp = read_vga(0x3da);
	v = read_vga(0x3c0);
	write_vga(0x3c0, v & ~0x20);		// Allow CPU access to Attribute registers
	for (i =0; i < sizeof(attribute_table); i++)
	{
		temp = read_vga(0x3da);		// Setup to write Attribute
		write_vga(0x3c0, i);			// Write the Index
		write_vga(0x3c0, attribute_table[i]);
	}
	temp = read_vga(0x3da);
	write_vga(0x3c0, v | 0x20);		// Disallow CPU access to Attribute registers
		
	//Write the GRAX registers
	write_vga_grax(0x00, 0x00);
	write_vga_grax(0x01, 0x00);
	write_vga_grax(0x02, 0x00);
	write_vga_grax(0x03, 0x00);
	write_vga_grax(0x04, 0x00);
	write_vga_grax(0x05, 0x40);
	write_vga_grax(0x06, 0x05);
	write_vga_grax(0x07, 0x0f);
	write_vga_grax(0x08, 0xff);

	// Write out the CRT Controller settings
	write_vga_crtc(0x00, Set8Bits (htotal - 5));		// Horizontal Total
	write_vga_crtc(0x01, Set8Bits (hdisp - 1));		// Horizontal Display End
	write_vga_crtc(0x02, Set8Bits (hdisp - 1));		// Horizontal Blank Start
	write_vga_crtc(0x03, SetBitField (htotal - 1, 4:0, 4:0) | SetBit (7));	// Horizontal Blank End
	write_vga_crtc(0x04, Set8Bits (hsyncstart));	// Horizontal Sync Start
	write_vga_crtc(0x05, SetBitField (htotal - 1, 5:5, 7:7) | SetBitField (hsyncend, 4:0, 4:0));	// Horizontal Sync End
	write_vga_crtc(0x06, SetBitField (vtotal - 2, 7:0, 7:0));		// Vertical Total
	write_vga_crtc(0x07, SetBitField (vtotal - 2, 8:8, 0:0) |
					  SetBitField (vdisp - 1, 8:8, 1:1) |
					  SetBitField (vsyncstart, 8:8, 2:2) |
						SetBitField (vdisp, 8:8, 3:3) |
					  SetBit (4) |
					  SetBitField (vtotal - 2, 9:9, 5:5) |
					  SetBitField (vdisp - 1, 9:9, 6:6) |
					  SetBitField (vsyncstart, 9:9, 7:7));			// Overflow
	write_vga_crtc(0x09, SetBitField (vdisp, 9:9, 5:5) | SetBit (6));	// Maximum Line Scan
	write_vga_crtc(0x10, Set8Bits (vsyncstart));				// Vertical Sync Start
	write_vga_crtc(0x11, SetBitField (vsyncend, 3:0, 3:0) | SetBit (5));		// Vertical Sync End
	write_vga_crtc(0x12, Set8Bits (vdisp - 1));		// Vertical Display End
	write_vga_crtc(0x15, Set8Bits (vdisp - 1));		// Vertical Blank Start
	write_vga_crtc(0x16, Set8Bits (vtotal - 1));	// Vetical Blank End
	write_vga_crtc(0x17, 0xe3);										// Mode Control
	write_vga_crtc(0x18, 0xff);										// Line Compare
	
	write_vga_grax(NEO_EXTCRTDISPADDR, 0x10);

	switch (ci->ci_Depth)
	{
		case  8 :
			dprintf(("neomagic_accel: SetupCRTC - programming for 8bpp, h_display = %d\n", dm->timing.h_display));
			write_vga_grax(NEO_EXTCRTOFFSET, dm->timing.h_display >> 11);
			tempColorModeSelect = 0x11;
			write_vga_crtc(0x13, dm->timing.h_display >> 3);
			break;
		case 15 :
			/* 15bpp */
			write_vga(0x3c8, 0);
			for (i = 0; i < 0x40; i++)
			{
				write_vga(0x3c9, i << 1);
				write_vga(0x3c9, i << 1);
				write_vga(0x3c9, i << 1);
			}
			tempColorModeSelect = 0x12;
			write_vga_crtc(0x13, dm->timing.h_display >> 2);
			write_vga_grax(NEO_EXTCRTOFFSET, dm->timing.h_display >> 10);
			break;	
		case 16 :
			/* 16bpp */
			write_vga(0x3c8, 0);
			for (i = 0;  i < 0x40;  i++) {
				write_vga(0x3c9, i << 1);
				write_vga(0x3c9, i);
				write_vga(0x3c9, i << 1);
			}
			write_vga_grax(NEO_EXTCRTOFFSET, dm->timing.h_display >> 10);
			tempColorModeSelect = 0x13;
			write_vga_crtc(0x13, dm->timing.h_display >> 2);
			break;
		case 24 :
			/* 24 bpp */
			write_vga(0x3c8, 0);
			for (i = 0; i < 256; i++)
			{
				write_vga(0x3c9, i);
				write_vga(0x3c9, i);
				write_vga(0x3c9, i);
			}
			write_vga_crtc(0x13, dm->timing.h_display >> 3);
			write_vga_grax(NEO_EXTCRTOFFSET, dm->timing.h_display >> 11);
			tempColorModeSelect = 0x14;
			break;
		default :
			break;
	}
	// Read the current value to make sure we only change the important bits
	temp = read_vga_grax(NEO_EXTCOLORMODESELECT);
	switch (ci->ci_device_id)
	{
		case DEVICEID_NM2070 :
			temp &= 0xF0; /* Save bits 7:4 */
			temp |= (tempColorModeSelect & ~0xf0);
			break;
		case DEVICEID_NM2090 :
		case DEVICEID_NM2093 :
		case DEVICEID_NM2097 :
		case DEVICEID_NM2160 :
		case DEVICEID_NM2200 :
			temp &= 0x70; /* Save bits 6:4 */
			temp |= (tempColorModeSelect & ~0x70);
			break;
	}
	write_vga_grax(NEO_EXTCOLORMODESELECT, temp);

	/* Enable read/write bursts. */
	temp = read_vga_grax(NEO_SYSIFACECNTL1);
	temp &= 0x0F; /* Save bits 3:0 */
	temp |= (0x30 & ~0x0F);		
	write_vga_grax(NEO_SYSIFACECNTL1,temp); 

	/* If they are used, enable linear addressing and/or enable MMIO. */
	temp = SetBit(7) | SetBit(6);
	write_vga_grax(NEO_SYSIFACECNTL2, temp);

    /* Determine panel width */
  temp = read_vga_grax(0x20);
	switch ((temp & 0x18) >> 3)
	{
		case 0x00 :
			ci->ci_PanelWidth  = 640;
			ci->ci_PanelHeight = 480;
			break;
    case 0x01 :
			ci->ci_PanelWidth  = 800;
			ci->ci_PanelHeight = 600;
			break;
    case 0x02 :
			ci->ci_PanelWidth  = 1024;
			ci->ci_PanelHeight = 768;
			break;
#if 0
    case 0x03 :
      /* 1280x1024 panel support needs to be added */
			ci->ci_PanelWidth  = 1280;
			ci->ci_PanelHeight = 1024;
			break;
#endif
    default :
			ci->ci_PanelWidth  = 640;
			ci->ci_PanelHeight = 480;
			break;
	}

	/* Default to internal (i.e., LCD) only. */
	if (panel_enabled)
		tempPanelDispCntlReg1 = 0x02;
	else
		tempPanelDispCntlReg1 = 0x02;
		
	/* If we are using a fixed mode, then tell the chip we are. */
	switch (dm->timing.h_display)
	{
		case 1280:
			tempPanelDispCntlReg1 |= 0x60;
			break;
		case 1024:
			tempPanelDispCntlReg1 |= 0x40;
			break;
		case 800:
			tempPanelDispCntlReg1 |= 0x20;
			break;
		case 640:
		default:
			break;
	}

	temp = read_vga_grax(NEO_PANELDISPCNTLREG1);
	switch (ci->ci_device_id)
	{
		case DEVICEID_NM2070 :
			temp &= 0xfc; /* Save bits 7:2 */
			temp |= (tempPanelDispCntlReg1 & ~0xfc);
			break;
		case DEVICEID_NM2090 :
		case DEVICEID_NM2093 :
		case DEVICEID_NM2097 :
		case DEVICEID_NM2160 :
			temp &= 0xdc; /* Save bits 7:6,4:2 */
			temp |= (tempPanelDispCntlReg1 & ~0xdc);
			break;
		case DEVICEID_NM2200 :
		case DEVICEID_NM256ZV :
			temp &= 0x98; /* Save bits 7,4:3 */
			temp |= (tempPanelDispCntlReg1 & ~0x98);
			break;
	}
	write_vga_grax(NEO_PANELDISPCNTLREG1, temp);
	
	/*
 	 * If the screen is to be stretched, turn on stretching for the
	 * various modes.
	 */
	tempPanelDispCntlReg2 = 0x00;
	tempPanelDispCntlReg3 = 0x00;
	if (stretching_enabled && panel_enabled)
	{
		if (dm->timing.h_display == ci->ci_PanelWidth)
		{
			/*
			* No stretching required when the requested display width
			* equals the panel width.
			*/
		}
		else
		{
			switch (dm->timing.h_display)
			{
				case  640 :
				case  800 :
				case 1024 :
					tempPanelDispCntlReg2 |= 0xC6;
					break;
				default   :
					/* No stretching in these modes. */
					break;
			}
		}
	}

	temp = read_vga_grax(NEO_PANELDISPCNTLREG2);
	temp &= 0x38; /* Save bits 5:3 */
	temp |= (tempPanelDispCntlReg2 & ~0x38);
	write_vga_grax(NEO_PANELDISPCNTLREG2, temp);
	
	tempPanelDispCntlReg3 = 0x00;
	if (ci->ci_device_id != DEVICEID_NM2070)
	{
		temp = read_vga_grax(NEO_PANELDISPCNTLREG3);
		temp &= 0xEF; /* Save bits 7:5 and bits 3:0 */
		temp |= (tempPanelDispCntlReg3 & ~0xEF);
		write_vga_grax(NEO_PANELDISPCNTLREG3, temp);
	}

	/*
	* If the screen is to be centered, turn on the centering for the
	* various modes.
	*/
	write_vga_grax(NEO_PANELVERTCENTERREG1, 0x00);
	write_vga_grax(NEO_PANELVERTCENTERREG2, 0x00);
	write_vga_grax(NEO_PANELVERTCENTERREG3, 0x00);
	write_vga_grax(NEO_PANELVERTCENTERREG4, 0x00);
	write_vga_grax(NEO_PANELVERTCENTERREG5, 0x00);

	write_vga_grax(NEO_PANELHORIZCENTERREG1, 0x00);
	write_vga_grax(NEO_PANELHORIZCENTERREG2, 0x00);
	write_vga_grax(NEO_PANELHORIZCENTERREG3, 0x00);
	write_vga_grax(NEO_PANELHORIZCENTERREG4, 0x00);
	write_vga_grax(NEO_PANELHORIZCENTERREG5, 0x00);
	
	if (centering_enabled && panel_enabled && (dm->timing.h_display != ci->ci_PanelWidth))
	{
		temp = read_vga_grax(NEO_PANELDISPCNTLREG2);
		temp |= SetBit(0);
		write_vga_grax(NEO_PANELDISPCNTLREG2, temp);
		temp = read_vga_grax(NEO_PANELDISPCNTLREG3);
		temp |= SetBit(4);
		write_vga_grax(NEO_PANELDISPCNTLREG3, temp);
		
		/* Calculate the horizontal and vertical offsets. */
		if (!stretching_enabled)
		{
			hoffset = ((ci->ci_PanelWidth - dm->timing.h_display) >> 4) - 1;
			voffset = ((ci->ci_PanelHeight - dm->timing.v_display) >> 1) - 2;
		}
		else
		{
			/* Stretched modes cannot be centered. */
			hoffset = 0;
			voffset = 0;
		}
		
		switch (dm->timing.h_display)
		{
			case  640 :
				write_vga_grax(NEO_PANELHORIZCENTERREG1, hoffset);
				write_vga_grax(NEO_PANELVERTCENTERREG3, voffset);
				break;
			case  800 :
				if (ci->ci_device_id != DEVICEID_NM2070)
				{
					write_vga_grax(NEO_PANELHORIZCENTERREG2, hoffset);
					write_vga_grax(NEO_PANELVERTCENTERREG4, voffset);
				}
				break;
			case 1024 :
				if ((ci->ci_device_id == DEVICEID_NM2200) || (ci->ci_device_id == DEVICEID_NM256ZV))
				{
					write_vga_grax(NEO_PANELHORIZCENTERREG5, hoffset);
					write_vga_grax(NEO_PANELVERTCENTERREG5, voffset);
				}
				break;
			case 1280 :
			default   :
				/* No centering in these modes. */
				break;
		}
	}
	
	/*
	* Calculate the VCLK that most closely matches the requested dot
	* clock.
	*/
	{
		uint8	denominator, numerator_low, numerator_high;
		CalcVCLK(dm->timing.pixel_clock, &denominator, &numerator_high, &numerator_low);
		write_vga_grax(0x9b, numerator_low);
		if (ci->ci_device_id == DEVICEID_NM2200)
		{
			temp = read_vga_grax(0x8f);
			temp &= 0x0f;
			temp |= (numerator_high & ~0x0f);
			write_vga_grax(0x8f, temp);
		}
		write_vga_grax(0x9f, denominator);
	}
	
	/* Vertical Extension */
	temp = (((dm->timing.v_total -2) & 0x400) >> 10 )
											| (((dm->timing.v_display -1) & 0x400) >> 9 )
											| (((dm->timing.v_sync_start) & 0x400) >> 8 )
											| (((dm->timing.v_sync_start) & 0x400) >> 7 );
	write_vga_crtc(0x70, temp);

	/* Write the Start address */
	write_vga_crtc(0x0c, 0 /* offset lo-word*/);
	write_vga_crtc(0x0d, 0 /* offset hi-word*/);
	
	SCREEN_ON;
	dprintf(("neomagic_accel: SetupCRTC - EXIT\n"));
	return (B_OK);
}

/*
 * CalcVCLK --
 *
 * Determine the closest clock frequency to the one requested.
 */
#define REF_FREQ 14.31818
#define MAX_N 127
#define MAX_D 31
#define MAX_F 1

void CalcVCLK(long freq, uint8 *denominator, uint8 *numerator_high, uint8 *numerator_low)
{
	int n, d, f;
	double f_out;
	double f_diff;
	int n_best = 0, d_best = 0, f_best = 0;
	double f_best_diff = 999999.0;
	double f_target = freq/1000.0;
	
	for (f = 0; f <= MAX_F; f++)
		for (n = 0; n <= MAX_N; n++)
			for (d = 0; d <= MAX_D; d++)
			{
				f_out = (n+1.0)/((d+1.0)*(1<<f))*REF_FREQ;
				f_diff = abs(f_out-f_target);
				if (f_diff < f_best_diff)
				{
					f_best_diff = f_diff;
					n_best = n;
					d_best = d;
					f_best = f;
				}
			}
	
	if (ci->ci_device_id == DEVICEID_NM2200)
	{
		/* NOT_DONE:  We are trying the full range of the 2200 clock.
		We should be able to try n up to 2047 */
		*numerator_low  = n_best;
		*numerator_high = (f_best << 7);
	}
	else
	{
		*numerator_low  = n_best | (f_best << 7);
	}
	*denominator = d_best;
}

void
_set_indexed_colors (uint count, uint8 first, uint8 *color_data, uint32 flags)
{
//	dprintf(("neomagic_accel: _set_indexed_colors - ENTER\n"));
	if (ci->ci_CurDispMode.space == B_CMAP8)
	{
		uint8 *color;
		uint idx;
		
		lockBena4 (&ci->ci_CLUTLock);

		color = color_data;
		idx = first;
		while (count--)
		{
			write_vga(0x3c8, idx);
			write_vga(0x3c9, color[0] >> 2);		// Red
			write_vga(0x3c9, color[1] >> 2);		// Green
			write_vga(0x3c9, color[2] >> 2);		// Blue
			color += 3;
			idx++;
		}

		unlockBena4 (&ci->ci_CLUTLock);
	}

//	dprintf(("neomagic_accel: _set_indexed_colors - EXIT\n"));
}

uint32
colorspacebits (uint32 cs /* a color_space */)
{
//	dprintf(("neomagic_accel: colorspacebits - ENTER, cs = %d\n", cs));
	switch (cs)
	{
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		case B_RGB32_LITTLE:
		case B_RGBA32_LITTLE:
	//	dprintf(("neomagic_accel: colorspacebits - EXIT, returning 32\n"));
			return (32);
		case B_RGB24_BIG:
		case B_RGB24_LITTLE:
	//	dprintf(("neomagic_accel: colorspacebits - EXIT, returning 24\n"));
			return (24);
		case B_RGB16_BIG:
		case B_RGB16_LITTLE:
	//	dprintf(("neomagic_accel: colorspacebits - EXIT, returning 16\n"));
			return (16);
		case B_RGB15_BIG:
		case B_RGB15_LITTLE:
		case B_RGBA15_BIG:
		case B_RGBA15_LITTLE:
	//	dprintf(("neomagic_accel: colorspacebits - EXIT, returning 15\n"));
			return (15);
		case B_CMAP8:
	//	dprintf(("neomagic_accel: colorspacebits - EXIT, returning 8\n"));
			return (8);
	}
//	dprintf(("neomagic_accel: colorspacebits - EXIT, returning 0\n"));
	return (0);
}
status_t _set_dpms_mode(uint32 dpms_flags)
{
	status_t err = B_OK;
	uint8 SEQ01, LogicPowerMgmt, LCDOn;
	
	switch(dpms_flags)
	{
		case B_DPMS_ON:	// H: on, V: on, Screen: On
//			dprintf(("neomagic_accel: _set_dpms_mode - DPMS_ON\n"));
			SEQ01 = 0x00;
			LogicPowerMgmt = 0x00;
			if (1 /* Using Panel*/)
				LCDOn = 0x02;
			else
				LCDOn = 0x00;
			break;
		case B_DPMS_STAND_BY: // H: off, V: on, display off
//			dprintf(("neomagic_accel: _set_dpms_mode - DPMS_STAND_BY\n"));
			SEQ01 = 0x20;
			LogicPowerMgmt = 0x10;
			LCDOn = 0x00;
			break;
		case B_DPMS_SUSPEND: // H: on, V: off, display off
//			dprintf(("neomagic_accel: _set_dpms_mode - DPMS_SUSPEND\n"));
			SEQ01 = 0x20;
			LogicPowerMgmt = 0x20;
			LCDOn = 0x00;
			break;
		case B_DPMS_OFF: // H: off, V: off, display off
//			dprintf(("neomagic_accel: _set_dpms_mode - DPMS_OFF\n"));
			SEQ01 = 0x20;
			LogicPowerMgmt = 0x30;
			LCDOn = 0x00;
			break;
		default:
			err = B_ERROR;
			break;
	}
	
	/* Turn the screen on/off */
	write_vga(0x3c4, 0x01);
	SEQ01 |= read_vga(0x3c5) & ~0x20;
	write_vga(0x3c5, SEQ01);
	
	/* Turn the LCD on/off */
	LCDOn |= read_vga_grax(0x20) & ~0x02;
	write_vga_grax(0x20, LCDOn);
	
	/* Set the DPMS mode */
	LogicPowerMgmt |= 0x80;
	LogicPowerMgmt |= read_vga_grax(0x01) & ~0xF0;
	write_vga_grax(0x01, LogicPowerMgmt);

	curr_dpms_mode = dpms_flags;
	return err;
}

uint32 _dpms_capabilities(void)
{
	return 	B_DPMS_ON | B_DPMS_OFF | B_DPMS_STAND_BY | B_DPMS_SUSPEND;
}

uint32 _dpms_mode(void)
{
	return curr_dpms_mode;
}

#if 0
/*
 * move the display area, so that the GameKit works
 */

status_t
movedisplay (uint16 x,uint16 y)
{
	uint32 fb_offset, stride;
	uint32 regBase;

	if (x!=0) {
		return B_ERROR;
	}
	regBase = (uint32) ci->ci_base_registers_pci[2];
	fb_offset = (uint32)ci->ci_FBBase - (uint32) ci->ci_BaseAddr1;
	stride = ci->ci_BytesPerRow;
	fb_offset += y*stride;
	write_pci_32(SSTIOADDR(vidDesktopStartAddr), (fb_offset & SST_VIDEO_START_ADDR) <<
				SST_VIDEO_START_ADDR_SHIFT);
	return B_OK;
}
#endif

void write_vga(uint32 offset, uint8 value)
{
	neomagic_readwrite_vgareg vgareg;
	
	vgareg.reg = offset;
	vgareg.value = value;
	ioctl(devfd, NEOMAGIC_IOCTL_WRITE_VGAREG, &vgareg);
}

uint8 read_vga(uint32 offset)
{
	neomagic_readwrite_vgareg vgareg;
	
	vgareg.reg = offset;
	ioctl(devfd, NEOMAGIC_IOCTL_READ_VGAREG, &vgareg);
	return vgareg.value;
}

void write_vga_crtc(uint8 index, uint8 value)
{
	write_vga(0x3d4, index);
	write_vga(0x3d5, value);
}

uint8 read_vga_crtc(uint8 index)
{
	write_vga(0x3d4, index);
	return read_vga(0x3d5);
}
void write_vga_grax(uint8 index, uint8 value)
{
	write_vga(0x3ce, index);
	write_vga(0x3cf, value);
}

uint8 read_vga_grax(uint8 index)
{
	write_vga(0x3ce, index);
	return read_vga(0x3cf);
}
