/* :ts=8 bk=0
 *
 * video.c:	The Very Nasty Stuff Indeed.
 *
 * $Id:$
 *
 * Andrew Kimpton					1999.03.13
 */
#include <kernel/OS.h>
#include <support/Debug.h>
#include <add-ons/graphics/GraphicsCard.h>
#include <add-ons/graphics/Accelerant.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <PCI.h>

#include <graphics_p/3dfx/voodoo4/voodoo4.h>
#include <graphics_p/3dfx/voodoo4/voodoo4_regs.h>
#include <graphics_p/3dfx/common/debug.h>
#include <graphics_p/video_overlay.h>

#include "plltable.h"

#include "protos.h"

extern __mem_Area *memArea;

/*****************************************************************************
 * #defines
 */
#define MASTERCLK	 14318.18	  /*  The frequency of the Beast   kHz */

#define	MODE_FLAGS	(B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS | B_SUPPORTS_OVERLAYS)
#define	T_POSITIVE_SYNC	(B_POSITIVE_HSYNC | B_POSITIVE_VSYNC)


/*****************************************************************************
 * Local prototypes.
 */
static status_t calcmodes ();
static int testbuildmodes (register display_mode * dst);
static uint16 *FindVideoMode (uint32 xRes, uint32 yRes, uint32 refresh);
static uint32 GetMemSize ();
static void InitVga (uint32 legacyDecode);	// 1=enable VGA decode, 0=disable

/*****************************************************************************
 * Globals.
 */
extern thdfx_card_info *ci;
extern int devfd;
extern __mem_AreaDef *MemMgr_2D;

/*
 * This table is formatted for 132 columns, so stretch that window...
 * Stolen from Trey's R4 Matrox driver and reformatted to improve readability
 * (after a fashion).  Some entries have been updated with Intel's "official"
 * numbers.
 */
static const display_mode mode_list[] = {
/* Vesa_Monitor_@60Hz_(640X480X8.Z1) */
	{{25175, 640, 656, 752, 800, 480, 490, 492, 525, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS},
/* 640X480X60Hz */
	{{27500, 640, 672, 768, 800, 480, 488, 494, 530, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS},
/* SVGA_640X480X60HzNI */
	{{30500, 640, 672, 768, 800, 480, 517, 523, 588, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@70-72Hz_(640X480X8.Z1) */
	{{31500, 640, 672, 768, 800, 480, 489, 492, 520, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@75Hz_(640X480X8.Z1) */
	{{31500, 640, 672, 736, 840, 480, 481, 484, 500, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@85Hz_(640X480X8.Z1) */
	{{36000, 640, 712, 768, 832, 480, 481, 484, 509, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS},
/* SVGA_800X600X56HzNI */
	{{38100, 800, 832, 960, 1088, 600, 602, 606, 620, 0}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@60Hz_(800X600X8.Z1) */
	
		{{40000, 800, 856, 984, 1056, 600, 601, 605, 628, T_POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0,
	 MODE_FLAGS},
/* Vesa_Monitor_@75Hz_(800X600X8.Z1) */
	
		{{49500, 800, 832, 912, 1056, 600, 601, 604, 625, T_POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0,
	 MODE_FLAGS},
/* Vesa_Monitor_@70-72Hz_(800X600X8.Z1) */
	
		{{50000, 800, 832, 912, 1056, 600, 637, 643, 666, T_POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0,
	 MODE_FLAGS},
/* Vesa_Monitor_@85Hz_(800X600X8.Z1) */
	
		{{56250, 800, 848, 912, 1048, 600, 601, 604, 631, T_POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0,
	 MODE_FLAGS},
/* SVGA_1024X768X43HzI */
	
		{{46600, 1024, 1088, 1216, 1312, 384, 385, 388, 404, B_TIMING_INTERLACED}, B_CMAP8, 1024, 768,
	 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@60Hz_(1024X768X8.Z1) */
	{{65000, 1024, 1064, 1200, 1344, 768, 771, 777, 806, 0}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@70-72Hz_(1024X768X8.Z1) */
	{{75000, 1024, 1048, 1184, 1328, 768, 771, 777, 806, 0}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@75Hz_(1024X768X8.Z1) */
	
		{{78750, 1024, 1056, 1152, 1312, 768, 769, 772, 800, T_POSITIVE_SYNC}, B_CMAP8, 1024, 768, 0,
	 0, MODE_FLAGS},
/* Vesa_Monitor_@85Hz_(1024X768X8.Z1) */
	
		{{94500, 1024, 1088, 1184, 1376, 768, 769, 772, 808, T_POSITIVE_SYNC}, B_CMAP8, 1024, 768, 0,
	 0, MODE_FLAGS},
/* Vesa_Monitor_@70Hz_(1152X864X8.Z1) */
	
		{{94200, 1152, 1184, 1280, 1472, 864, 865, 868, 914, T_POSITIVE_SYNC}, B_CMAP8, 1152, 864, 0,
	 0, MODE_FLAGS},
/* Vesa_Monitor_@75Hz_(1152X864X8.Z1) */
	
		{{108000, 1152, 1216, 1344, 1600, 864, 865, 868, 900, T_POSITIVE_SYNC}, B_CMAP8, 1152, 864, 0,
	 0, MODE_FLAGS},
/* Vesa_Monitor_@85Hz_(1152X864X8.Z1) */
	
		{{121500, 1152, 1216, 1344, 1568, 864, 865, 868, 911, T_POSITIVE_SYNC}, B_CMAP8, 1152, 864, 0,
	 0, MODE_FLAGS},
/* Vesa_Monitor_@60Hz_(1280X1024X8.Z1) */
	
		{{108000, 1280, 1344, 1456, 1688, 1024, 1025, 1028, 1066, T_POSITIVE_SYNC}, B_CMAP8, 1280,
	 1024, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@75Hz_(1280X1024X8.Z1) */
	
		{{135000, 1280, 1312, 1456, 1688, 1024, 1025, 1028, 1066, T_POSITIVE_SYNC}, B_CMAP8, 1280,
	 1024, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@85Hz_(1280X1024X8.Z1) */
	
		{{157500, 1280, 1344, 1504, 1728, 1024, 1025, 1028, 1072, T_POSITIVE_SYNC}, B_CMAP8, 1280,
	 1024, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@60Hz_(1600X1200X8.Z1) */
	
		{{162000, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600,
	 1200, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@65Hz_(1600X1200X8.Z1) */
	
		{{175500, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600,
	 1200, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@70Hz_(1600X1200X8.Z1) */
	
		{{189000, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600,
	 1200, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@75Hz_(1600X1200X8.Z1) */
	
		{{202500, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600,
	 1200, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@80Hz_(1600X1200X8.Z1) */
	
		{{216000, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600,
	 1200, 0, 0, MODE_FLAGS},
/* Vesa_Monitor_@85Hz_(1600X1200X8.Z1) */
	
		{{229500, 1600, 1680, 1872, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600,
	 1200, 0, 0, MODE_FLAGS}
};
#define	NMODES		(sizeof (mode_list) / sizeof (display_mode))


static void write_phys_8 (offset, val)
{
	thdfx_readwritepci32 buf;
	buf.val = val;
	buf.offset = offset;

	ioctl (devfd, THDFX_IOCTL_WRITE_PHYS_8, &buf, sizeof (buf));
}

static uint8 read_phys_8 (offset)
{
	thdfx_readwritepci32 buf;
	buf.offset = offset;

	ioctl (devfd, THDFX_IOCTL_READ_PHYS_8, &buf, sizeof (buf));
	return buf.val;
}

static void check_for_room ()
{
	ioctl (devfd, THDFX_IOCTL_CHECKFORROOM, 0, 0);
}

/*****************************************************************************
 * Initialization sequence.
 */
status_t thdfx_init ()
{
// register vgaregs  *vregs;
	status_t retval = B_OK;
// uint8       tmp;

	dprintf (("3dfx_accel: thdfx_init - ENTER\n"));

	/*  Determine available framebuffer memory  */

	ci->MemSize = GetMemSize () /* Megabytes */  << 20;
	ci->MasterClockKHz = MASTERCLK;
// dprintf (("3dfx_accel: thdfx_init - ci_MemSize: %d\n", ci->MemSize));

	// We initialize the framebuffer to be at the beginning of the card - in reality
	// setting the video mode will move the framebuffer to the end of the card.
	ci->FBBase = (void *) ((uint8 *) ci->BaseAddr1);
	ci->FBBase_DMA = (void *) ((uint8 *) ci->base_registers_pci[1]);
// ci->frame_buffer_spec.ms_Size = 0;
	ci->AllocFrameBuffer = 0;

	// We also store at this time the base address for DMA purposes of the card
	ci->BaseAddr1_DMA = (void *) ((uint8 *) ci->base_registers_pci[1]);

// dprintf(("3dfx_accel: thdx_init - ci->Regs = 0x%x\n", ci->Regs));

	//InitVga((uint32) ci->base_registers_pci[2], 1);         // 1=enable VGA decode, 0=disable
	InitVga (1);					  // 1=enable VGA decode, 0=disable

	strcpy (ci->ADI.name, "Voodoo Banshee-based board");
	strcpy (ci->ADI.chipset, "Banshee");
	ci->ADI.dac_speed = CLOCK_MAX / 1000;
	ci->ADI.memory = ci->MemSize;
	ci->ADI.version = B_ACCELERANT_VERSION;

	retval = calcmodes ();
	dprintf (("3dfx_accel: thdfx_init - EXIT, returning 0x%x\n", retval));
	return (retval);
}

static status_t calcmodes ()
{
// dprintf(("3dfx_accel: calcmodes - ENTER\n"));
	/*
	 * First count the modes so we know how much memory to allocate.
	 */
	if ((ci->NDispModes = testbuildmodes (NULL)))
	{
		/*
		 * Okay, build the list for real this time.
		 */
		if ((ci->DispModes = malloc ((ci->NDispModes + 1) * sizeof (display_mode))))
		{
			testbuildmodes (ci->DispModes);
//       dprintf(("3dfx_accel: calcmodes - EXIT, returning B_OK\n"));
			return (B_OK);
		}
	}
// dprintf(("3dfx_accel: calcmodes - EXIT, returning B_ERROR\n"));
	return (B_ERROR);
}

static int testbuildmodes (display_mode * dst)
{
	const display_mode *src;
	display_mode low, high, try;
	uint32 i, j, pix_clk_range;
	int count;

#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
	static color_space spaces[] = { B_CMAP8, B_RGB15_LITTLE, B_RGB16_LITTLE, B_RGB32_LITTLE };
#else
	static color_space spaces[] = { B_CMAP8, B_RGB15_BIG, B_RGB16_BIG, B_RGB32_BIG };
#endif
#define	NSPACES		(sizeof (spaces) / sizeof (color_space))

// dprintf(("3dfx_accel: testbuildmodes - ENTER\n"));
	/*
	 * Walk through our predefined list and see which modes fit this
	 * device.
	 */
	src = mode_list;
	count = 0;
	for (i = 0; i < NMODES; i++)
	{
		/* set ranges for acceptable values */
		low = high = *src;
		/* range is 6.25% of default clock: arbitrarily picked */
		pix_clk_range = low.timing.pixel_clock >> 5;
		low.timing.pixel_clock -= pix_clk_range;
		high.timing.pixel_clock += pix_clk_range;

		/*
		 * Test each color_space we support.
		 */
		for (j = 0; j < NSPACES; j++)
		{
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

// dprintf(("3dfx_accel: testbuildmodes - EXIT, returning count = %d\n", count));
	return (count);
}

status_t propose_video_mode (display_mode * target, const display_mode * low, const display_mode * high)
{
	status_t result = B_OK;
	double target_refresh;
	uint32 row_bytes, pixel_bytes, limit_clock_lo;
	uint16 width, height, dispwide, disphigh;
	bool want_same_width, want_same_height;

// dprintf(("3dfx_accel: propose_video_mode - ENTER\n"));

	target_refresh = ((double) target->timing.pixel_clock * 1000.0) /
		((double) target->timing.h_total * (double) target->timing.v_total);
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
		 * FIXME: RIVA 128 restrictions:
		 * Visible width must be quantized to 32 pixels.
		 */
		uint16 h_display = ((target->timing.h_display + 31) & ~31) >> 3;
		uint16 h_sync_start = target->timing.h_sync_start >> 3;
		uint16 h_sync_end = target->timing.h_sync_end >> 3;
		uint16 h_total = target->timing.h_total >> 3;

		/*  Ensure reasonable minium display and sequential order of parms  */
		if (h_display < (320 >> 3))
			h_display = 320 >> 3;
		if (h_display > (2048 >> 3))
			h_display = 2048 >> 3;
		if (h_sync_start < (h_display + 2))
			h_sync_start = h_display + 2;
		if (h_sync_end < (h_sync_start + 3))
			h_sync_end = h_sync_start + 3;
		/*(0x001f >> 2); */
		if (h_total < (h_sync_end + 1))
			h_total = h_sync_end + 1;

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

		target->timing.h_display = h_display << 3;
		target->timing.h_sync_start = h_sync_start << 3;
		target->timing.h_sync_end = h_sync_end << 3;
		target->timing.h_total = h_total << 3;
	}
	if (target->timing.h_display < low->timing.h_display ||
		 target->timing.h_display > high->timing.h_display ||
		 target->timing.h_sync_start < low->timing.h_sync_start ||
		 target->timing.h_sync_start > high->timing.h_sync_start ||
		 target->timing.h_sync_end < low->timing.h_sync_end ||
		 target->timing.h_sync_end > high->timing.h_sync_end ||
		 target->timing.h_total < low->timing.h_total ||
		 target->timing.h_total > high->timing.h_total)
	{
//       dprintf(("3dfx_accel: propose_video_mode: horizontal timing out of range - returning B_BAD_VALUE\n"));
		result = B_BAD_VALUE;
	}
	{
		/*  Validate vertical timings  */
		uint16 v_display = target->timing.v_display;
		uint16 v_sync_start = target->timing.v_sync_start;
		uint16 v_sync_end = target->timing.v_sync_end;
		uint16 v_total = target->timing.v_total;

		/*  Ensure reasonable minium display and sequential order of parms  */
		if (v_display < 200)
			v_display = 200;
		if (v_display > 2048)
			v_display = 2048;		  /* ha! */
		if (v_sync_start < (v_display + 1))
			v_sync_start = v_display + 1;
		if (v_sync_end < v_sync_start)
			v_sync_end = v_sync_start + (0x000f >> 2);
		if (v_total < (v_sync_end + 1))
			v_total = v_sync_end + 1;

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

		target->timing.v_display = v_display;
		target->timing.v_sync_start = v_sync_start;
		target->timing.v_sync_end = v_sync_end;
		target->timing.v_total = v_total;
	}
	if (target->timing.v_display < low->timing.v_display ||
		 target->timing.v_display > high->timing.v_display ||
		 target->timing.v_sync_start < low->timing.v_sync_start ||
		 target->timing.v_sync_start > high->timing.h_sync_start ||
		 target->timing.v_sync_end < low->timing.v_sync_end ||
		 target->timing.v_sync_end > high->timing.v_sync_end ||
		 target->timing.v_total < low->timing.v_total ||
		 target->timing.v_total > high->timing.v_total)
	{
//       dprintf(("3dfx_accel: propose_video_mode: vertical timing out of range - returning B_BAD_VALUE\n"));
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

	if (target->timing.h_display > target->virtual_width || want_same_width)
		target->virtual_width = target->timing.h_display;
	if (target->timing.v_display > target->virtual_height || want_same_height)
		target->virtual_height = target->timing.v_display;
	if (target->virtual_width > 2048)
		target->virtual_width = 2048;

	/*
	 * FIXME:
	 * RIVA restriction: Quantize virtual_width to 8 bytes (should fall
	 * right out of the 32-pixel quantization earlier, but do it anyway).
	 */
	pixel_bytes = (colorspacebits (target->space) + 7) >> 3;
	row_bytes = (pixel_bytes * target->virtual_width + 7) & ~7;
	target->virtual_width = row_bytes / pixel_bytes;
	if (want_same_width)
		target->timing.h_display = target->virtual_width;

	/*  Adjust virtual width for engine limitations  */
	if (target->virtual_width < low->virtual_width || target->virtual_width > high->virtual_width)
	{
//    dprintf(("3dfx_accel: propose_video_mode: virtual_width out of range - returning B_BAD_VALUE\n"));
		result = B_BAD_VALUE;
	}
	/*  Memory requirement for frame buffer  */
	if (row_bytes * target->virtual_height > ci->MemSize)
		target->virtual_height = ci->MemSize / row_bytes;

	if (target->virtual_height < target->timing.v_display)
		/* not enough frame buffer memory for the mode */
	{
//    dprintf(("3dfx_accel: propose_video_mode: insufficient frame buffer memory - returning B_ERROR\n"));
		result = B_ERROR;
	}
	else if (target->virtual_height < low->virtual_height ||
				target->virtual_height > high->virtual_height)
	{
//    dprintf(("3dfx_accel: propose_video_mode: virtual_height out of range - returning B_BAD_VALUE\n"));
		result = B_BAD_VALUE;
	}

	if (width != target->virtual_width ||
		 height != target->virtual_height ||
		 dispwide != target->timing.h_display || disphigh != target->timing.v_display)
		/*  Something changed; we have to re-check.  */
		goto revalidate;			  /*  Look up  */

	/*
	 * Adjust pixel clock for DAC limits and target refresh rate
	 * pixel_clock is recalculated because [hv]_total may have been nudged.
	 */
	target->timing.pixel_clock = target_refresh *
		((double) target->timing.h_total) * ((double) target->timing.v_total) / 1000.0 + 0.5;
	limit_clock_lo = 48.0 *		  /*  Until a monitors database does it...  */
		((double) target->timing.h_total) * ((double) target->timing.v_total) / 1000.0;
	if (target->timing.pixel_clock > CLOCK_MAX)
		target->timing.pixel_clock = CLOCK_MAX;
	if (target->timing.pixel_clock < limit_clock_lo)
		target->timing.pixel_clock = limit_clock_lo;
	if (target->timing.pixel_clock < low->timing.pixel_clock ||
		 target->timing.pixel_clock > high->timing.pixel_clock)
	{
//    dprintf(("3dfx_accel: propose_video_mode: pixel_clock out of range - returning B_BAD_VALUE\n"));
		result = B_BAD_VALUE;
	}

	/*  Banshee doesn't handle 15-bpp at all well.  */
	if (colorspacebits (target->space) == 15)
		result = B_ERROR;

	/* Check to see if this mode specification is valid for our card */
	if (FindVideoMode (target->timing.h_display, target->timing.v_display, target_refresh) == NULL)
		result = B_BAD_VALUE;

// dprintf(("3dfx_accel: propose_video_mode - EXIT, returning 0x%x\n", result));
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

status_t vid_selectmode (register display_mode * dm, uint32 lockflags)
{
	status_t retval;
	uint32 temp;

// dprintf(("3dfx_accel: vid_selectmode - ENTER\n"));
	/*
	 * We are about to Change The World, so grab everything.
	 */
	lockBena4 (&ci->CRTCLock);
	lockBena4 (&ci->CLUTLock);
	lockBena4 (&ci->EngineLock);

	if (ci->IRQFlags & IRQF__ENABLED)
	{
		temp = _V5_ReadReg( ci, V5_2D_INTR_CTRL );
		temp &= ~(1 << 2);	/* Disable Vertical Sync Interrupts */
		_V5_WriteReg_NC( ci, V5_2D_INTR_CTRL, temp );
	}
	retval = SetupCRTC (dm);

	temp = _V5_ReadReg( ci, V5_2D_INTR_CTRL );
	temp &= ~(1 << 8);	/*  Clear Vertical Sync (Rising Edge) Interrupt generated flag  */
	temp |= 1 << 31;	/*  Write '1' to Extern pin pci_inta (active  low) to clear external PCI Interrupt  */
	_V5_WriteReg_NC( ci, V5_2D_INTR_CTRL, temp );

	if (ci->IRQFlags & IRQF__ENABLED)
	{
		/* Enable Vertical Sync (Rising Edge) Interrupts */ ;
		temp = _V5_ReadReg( ci, V5_2D_INTR_CTRL );
		temp |= 1 << 2;
		_V5_WriteReg_NC( ci, V5_2D_INTR_CTRL, temp );
	}

	/*
	 * (Yes, there is a better way to do this thing with the locks.
	 * Howver, it was the first approach I thought of, and it works,
	 * and since we're in Beta cycle at the moment, I'm not going to
	 * fiddle with it right now and possibly break it...)
	 */
	if (!(lockflags & LEAVELOCKF_ENGINE))
		unlockBena4 (&ci->EngineLock);
	if (!(lockflags & LEAVELOCKF_CLUT))
		unlockBena4 (&ci->CLUTLock);
	if (!(lockflags & LEAVELOCKF_CRTC))
		unlockBena4 (&ci->CRTCLock);

// dprintf(("3dfx_accel: vid_selectmode - EXIT, returning 0x%x\n", retval));
	return (retval);
}

/*----------------------------------------------------------------------
Function name:  InitPlls

Description:    Initialize the Banshee Plls.
                
Information:

Return:         VOID
----------------------------------------------------------------------*/
void InitPlls (uint32 regBase,  // init iegister base
					uint32 grxSpeedInMHz,	// desired clock frequency (MHz)
					uint32 memSpeedInMHz) // desired clock frequency (MHz)--------------------------------------*/
{

	// set the graphics clock
	//
	if (grxSpeedInMHz < MIN_PLL_FREQ || grxSpeedInMHz > MAX_PLL_FREQ)
	{
//      dprintf(("3dfx_accel: InitPlls - Figure out how to set GRX clock to %d MHz\n", grxSpeedInMHz));
	}
	else
	{
//      dprintf(("3dfx_accel: InitPlls - Setting Graphics Clock (%d MHz)\n",grxSpeedInMHz));
		_V5_WriteReg_NC( ci, V5_VID_PLL_CTRL_1, pllTable[grxSpeedInMHz]);
	}

	// set the memory clock
	//
	if (memSpeedInMHz < MIN_PLL_FREQ || memSpeedInMHz > MAX_PLL_FREQ)
	{
//      dprintf(("3dfx_accel: InitPlls - Figure out how to set MEM clock to %d MHz\n", memSpeedInMHz));
	}
	else
	{
//      dprintf(("3dfx_accel: InitPlls - Setting Memory Clock (%d MHz)\n",memSpeedInMHz));
		_V5_WriteReg_NC( ci, V5_VID_PLL_CTRL_2, pllTable[memSpeedInMHz]);
	}
}										  // h3InitPlls

/*----------------------------------------------------------------------
Function name:  InitVga

Description:    Initialize the VGA.
                
Information:

Return:         VOID
----------------------------------------------------------------------*/
void InitVga (uint32 legacyDecode) // 1=enable VGA decode, 0=disable-------------------------------------*/
{
	uint32 vgaInit0 = 0, miscInit1;
	thdfx_readwritepci32 buf;

	vgaInit0 |= SST_VGA0_EXTENSIONS;
	vgaInit0 |= SST_WAKEUP_3C3 << SST_VGA0_WAKEUP_SELECT_SHIFT;

	if (legacyDecode)
		vgaInit0 |= SST_VGA0_ENABLE_DECODE << SST_VGA0_LEGACY_DECODE_SHIFT;
	else
		vgaInit0 |= SST_VGA0_DISABLE_DECODE << SST_VGA0_LEGACY_DECODE_SHIFT;

	vgaInit0 |= SST_ENABLE_ALT_READBACK << SST_VGA0_CONFIG_READBACK_SHIFT;

	check_for_room ();
	_V5_WriteReg_NC( ci, V5_VID_VGA_INIT_0, vgaInit0);

	check_for_room ();
	_V5_WriteReg_NC( ci, V5_VID_VGA_INIT_1, 0);

	check_for_room ();
	write_phys_8 (0xc3, 0x1);

	miscInit1 = _V5_ReadReg( ci, V5_VID_MISC_INIT_1 );
	miscInit1 |= BIT (0);
	check_for_room ();

	_V5_WriteReg_NC( ci, V5_VID_MISC_INIT_1, miscInit1);

}										  // h3InitVga

/*----------------------------------------------------------------------
Function name:  CalcPLL

Description:    Return the value for the PLL Register
                
Information:

Return:         
----------------------------------------------------------------------*/
int CalcPLL (int freq, int *f_out)
{
	int m, n, k, best_m, best_n, best_k, f_cur, best_error;

//  dprintf(("CalcPLL start, freq = %d KHz\n", freq));
	best_error = freq;
	best_n = best_m = best_k = 0;
	for (n = 1; n < 256; n++)
	{
		f_cur = MASTERCLK * (n + 2);
		if (f_cur < freq)
		{
			f_cur = f_cur / 3;
			if (freq - f_cur < best_error)
			{
				best_error = freq - f_cur;
				best_n = n;
				best_m = 1;
				best_k = 0;
				continue;
			}
		}
		for (m = 1; m < 64; m++)
		{
			for (k = 1; k < 3; k++)
			{
				f_cur = MASTERCLK * (n + 2) / (m + 2) / (1 << k);
				if (abs (f_cur - freq) < best_error)
				{
					best_error = abs (f_cur - freq);
					best_n = n;
					best_m = m;
					best_k = k;
				}
			}
		}
	}
	n = best_n;
	m = best_m;
	k = best_k;
	*f_out = MASTERCLK * (n + 2) / (m + 2) / (1 << k);
//dprintf(("n = %d, m = %d, k = %d\n", n, m, k));
	return (n << 8) | (m << 2) | k;
}

/*----------------------------------------------------------------------
Function name:  GetMemSize

Description:    Return the size of memory in MBs.
                
Information:

Return:         uint32   The size of memory in MBs.
----------------------------------------------------------------------*/
uint32 GetMemSize ()
{
	uint32 memType,				  // SGRAM or SDRAM
		partSize,					  // size of SGRAM chips in Mbits
		memSize,						  // total size of memory in MBytes
		nChips,						  // # of chips of SDRAM/SGRAM
		dramInit1_strap, dramInit0_strap;

	// determine memory type: SDRAM or SGRAM
	memType = MEM_TYPE_SGRAM;
	dramInit1_strap = _V5_ReadReg( ci, V5_VID_DRAM_INIT_1 );
	dramInit1_strap &= SST_MCTL_TYPE_SDRAM;
	if (dramInit1_strap & SST_MCTL_TYPE_SDRAM)
		memType = MEM_TYPE_SDRAM;

	// determine memory size from strapping pins (dramInit0 and dramInit1)
	dramInit0_strap = _V5_ReadReg( ci, V5_VID_DRAM_INIT_0 );
	dramInit0_strap &= SST_SGRAM_TYPE | SST_SGRAM_NUM_CHIPSETS;

	if (memType == MEM_TYPE_SDRAM)
	{
		nChips = 8;
		partSize = 16;
		memSize = 16;

		// avoid minor performance hit when using SDRAM
		dramInit0_strap &= ~SST_SGRAM_NUM_CHIPSETS;

	}
	else
	{
		nChips = ((dramInit0_strap & SST_SGRAM_NUM_CHIPSETS) == 0) ? 4 : 8;

		if ((dramInit0_strap & SST_SGRAM_TYPE) == SST_SGRAM_TYPE_8MBIT)
		{
			partSize = 8;
		}
		else if ((dramInit0_strap & SST_SGRAM_TYPE) == SST_SGRAM_TYPE_16MBIT)
		{
			partSize = 16;
		}
		else
		{
//      dprintf(("3dfx_accel: GetMemSize: invalid sgram type = 0x%x", (dramInit0_strap & SST_SGRAM_TYPE) << SST_SGRAM_TYPE_SHIFT ));
			return 0;
		}
	}

	memSize = (nChips * partSize) / 8;	// in MBytes

	return (memSize);
}

uint16 mode_table[][24] = {
#include "modetable.h"
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}

};

uint8 vgaattr[] = { 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x00, 0x0F, 0x00
};

/*----------------------------------------------------------------------
Function name:  FindVideoMode

Description:    Find the video mode in the mode table based on the
                xRes, yRes, and refresh rate.
Information:

Return:         (uint16 *)   Ptr to the entry in the mode table,
                            NULL if failure.
----------------------------------------------------------------------*/
uint16 *FindVideoMode (uint32 xRes, uint32 yRes, uint32 refresh)
{
	int i = 0;

	while (mode_table[i][0] != 0)
	{
		if ((mode_table[i][0] == xRes) && (mode_table[i][1] == yRes) && (mode_table[i][2] == refresh))
		{
			return &mode_table[i][3];
		}
		if ((mode_table[i][0] == xRes) &&
			 (mode_table[i][1] == yRes) &&
			 (mode_table[i + 1][2] > refresh) && (mode_table[i][2] < refresh))
		{
			// We may have a partial match where the refresh is not 
			// exactly identical but 'would do'. For example if the 3dfx
			// card supports 70 or 75 Hz but we were asked for 72 Hz then
			// 70 'would do' so return that
//            dprintf(("3dfx_accel: FindVideoMode(%d, %d, %d), returning entry %d, real xRes = %d, yRes = %d, refresh = %d\n", xRes, yRes, refresh, i, mode_table[i][0],mode_table[i][1],mode_table[i][2]));
			return &mode_table[i][3];
		}
		i += 1;
	}

	return NULL;
}

/*----------------------------------------------------------------------
Function name:  SetVideoMode

Description:    Set the video mode.

Information:

Return:         FxBool  FXTRUE  if success or,
                        FXFALSE if failure.
----------------------------------------------------------------------*/
bool SetVideoMode (uint32 regBase,	// regBase of this card
						 uint32 xRes, // xResolution in pixels
						 uint32 yRes, // yResolution in pixels
						 uint32 refresh,	// refresh rate in Hz
						 uint32 loadClut)	// really a bool, should we load the lookup table
{
	thdfx_setvideomode buf;
	buf.xRes = xRes;
	buf.yRes = yRes;
	buf.refresh = refresh;
	buf.loadClut = loadClut;
	buf.isPrimary = 1;

	if (ioctl (devfd, THDFX_IOCTL_SET_VIDEO_MODE, &buf, sizeof (thdfx_setvideomode)) < 0)
		return false;
	return true;
}										  // h3InitSetVideoMode

/*----------------------------------------------------------------------
Function name:  SetVideoDesktopSurface

Description:    Set the width/height/start address/stride (position,
                stretch, filter, etc. for overlay) parameters of
                these surfaces.
Information:

Return:         VOID
----------------------------------------------------------------------*/
void SetVideoDesktopSurface (uint32 regBase, uint32 enable,	// 1=enable desktop surface (DS), 1=disable
									  uint32 tiled,	// 0=DS linear, 1=tiled
									  uint32 pixFmt,	// pixel format of DS
									  uint32 clutBypass,	// bypass clut for DS?
									  uint32 clutSelect,	// 0=lower 256 CLUT entries, 1=upper 256
									  uint32 startAddress,	// board address of beginning of DS
									  uint32 stride)	// distance between scanlines of the DS, in
// units of bytes for linear DS's and tiles for// tiled DS's----------*/
{
	uint32 reg_vidProcCfg = _V5_ReadReg( ci, V5_VID_PROC_CFG );
	uint32 reg_vidDesktopOverlayStride;

// dprintf(("3dfx_accel: SetVideoDesktopSurface - ENTER\n"));
// dprintf(("3dfx_accel: SetVideoDesktopSurface - vidProcCfg = 0x%x\n", vidProcCfg));

	reg_vidProcCfg &= ~( SST_DESKTOP_EN | SST_DESKTOP_TILED_EN | SST_DESKTOP_PIXEL_FORMAT |
							SST_DESKTOP_CLUT_BYPASS | SST_DESKTOP_CLUT_SELECT);
	if (enable)
		reg_vidProcCfg |= SST_DESKTOP_EN;

	if (tiled)
		reg_vidProcCfg |= ~SST_DESKTOP_TILED_EN;

	reg_vidProcCfg |= pixFmt;

//	if (clutBypass)
//		reg_vidProcCfg |= SST_DESKTOP_CLUT_BYPASS;

//	if (clutSelect)
//		reg_vidProcCfg |= SST_DESKTOP_CLUT_SELECT;

// dprintf(("3dfx_accel: SetVideoDesktopSurface - new vidProcCfg = 0x%x\n", vidProcCfg));
	_V5_WriteReg_NC( ci, V5_VID_PROC_CFG, reg_vidProcCfg);

	_V5_WriteReg_NC( ci, V5_VID_DESKTOP_START_ADDR, 
					  (startAddress & SST_VIDEO_START_ADDR) << SST_VIDEO_START_ADDR_SHIFT);

	// change only the desktop portion of the vidDesktopOverlayStride register
	//
	reg_vidDesktopOverlayStride = _V5_ReadReg( ci, V5_VID_DESKTOP_OVERLAY_STRIDE );

//    dprintf(("3dfx_accel: SetVideoDesktopSurface - stride = 0x%x\n", doStride));
	reg_vidDesktopOverlayStride &= ~(SST_DESKTOP_LINEAR_STRIDE | SST_DESKTOP_TILE_STRIDE);
	stride <<= SST_DESKTOP_STRIDE_SHIFT;
	if (tiled)
		stride &= SST_DESKTOP_TILE_STRIDE;
	else
		stride &= SST_DESKTOP_LINEAR_STRIDE;
	reg_vidDesktopOverlayStride |= stride;

//    dprintf(("3dfx_accel: SetVideoDesktopSurface - new stride = 0x%x\n", doStride));
	_V5_WriteReg_NC( ci, V5_VID_DESKTOP_OVERLAY_STRIDE, reg_vidDesktopOverlayStride );
}

/*****************************************************************************
 * Display manipulation.
 */
status_t SetupCRTC (register display_mode * dm)
{
	int hdisp, hsyncstart, hsyncend, htotal;
	int vdisp, vsyncstart, vsyncend, vtotal;
	int htotalpix, vtotalpix;
	int bytespp, bytesperrow;
	int refreshrate;
	uint8 sync_pol;
	uint32 pixelFormat = 0, stride, fb_offset, PLLReg;
	uint32 regBase = (uint32) ci->base_registers_pci[2];
	bool useClut = 0;
	int fout;
	uint32 temp;

// dprintf(("3dfx_accel: SetupCRTC - ENTER\n"));

	htotalpix = dm->timing.h_total;
	hdisp = dm->timing.h_display >> 3;
	hsyncstart = dm->timing.h_sync_start >> 3;
	hsyncend = dm->timing.h_sync_end >> 3;
	htotal = htotalpix >> 3;

	vtotalpix = dm->timing.v_total;
	vdisp = dm->timing.v_display;
	vsyncstart = dm->timing.v_sync_start;
	vsyncend = dm->timing.v_sync_end;
	vtotal = vtotalpix;

	bytespp = (colorspacebits (dm->space) + 7) >> 3;
	bytesperrow = (dm->virtual_width >> 3) * bytespp;

	refreshrate = ((dm->timing.pixel_clock * 1000) / (dm->timing.h_total * dm->timing.v_total)) + 1;

// dprintf(("3dfx_accel: SetupCRTC - pixel_clock = %d, h_total = %d, v_total = %d\n", dm->timing.pixel_clock, dm->timing.h_total, dm->timing.v_total));

	ci->BytesPerRow = dm->virtual_width * bytespp;
	ci->Depth = colorspacebits (dm->space);

// dprintf(("3dfx_accel: SetupCRTC - ci->BytesPerRow  = %d, ci->Depth = %d\n", ci->BytesPerRow, ci->Depth));

	switch (ci->Depth)
	{
	case 8:
		pixelFormat = SST_DESKTOP_PIXEL_PAL8;
		useClut = 1;
		break;
	case 15:
//    dprintf(("3dfx_accel: SetupCRTC - 15 BPP Mode Not Supported !!\n"));
		break;
	case 16:
		pixelFormat = SST_DESKTOP_PIXEL_RGB565;
		useClut = 0;
		break;
	case 32:
		pixelFormat = SST_DESKTOP_PIXEL_RGB32;
		useClut = 0;
		break;
	default:
//    dprintf(("3dfx_accel: SetupCRTC - Unknown colordepth = %d !\n", ci->Depth));
		break;
	}

// dprintf(("3dfx_accel: SetupCRTC - Disabling VGA Decode\n"));   
	InitVga (0);					  // 1=enable VGA decode, 0=disable
	stride = ci->BytesPerRow;

	temp = _V5_ReadReg( ci, V5_VID_PROC_CFG );
	temp |= 0x01;  // Enable Video Processor
	_V5_WriteReg_NC( ci, V5_VID_PROC_CFG, temp );

// dprintf(("3dfx_accel: SetupCRTC - calling SetVideoMode(x= %d, y = %d, refresh = %d)\n", dm->timing.h_display, dm->timing.v_display, refreshrate));
	if (!SetVideoMode
		 ((uint32) ci->base_registers_pci[2], dm->timing.h_display, dm->timing.v_display,
		  refreshrate, 0))
	{
//    dprintf(("3dfx_accel: SetupCRTC - SetVideoMode() failed !\n"));
		return B_ERROR;
	}

// dprintf(("3dfx_v4_accel: SetupCRTC - calling CalcPLL(freq = %d KHz)\n", dm->timing.pixel_clock));
	PLLReg = CalcPLL (dm->timing.pixel_clock, &fout);
// dprintf(("3dfx_v4_accel: SetupCRTC - called CalcPLL(freq = %d KHz), fout = %d KHz, PLLReg = 0x%x\n", dm->timing.pixel_clock, fout, PLLReg));
	_V5_WriteReg_NC( ci, V5_VID_PLL_CTRL_0, PLLReg);
	ci->prim_PLLReg0 = PLLReg;

	temp = _V5_ReadReg( ci, V5_VID_PROC_CFG );
	temp &= 0xfffffffe;	// Disable Video Processor
	_V5_WriteReg_NC( ci, V5_VID_PROC_CFG, temp );
	temp |= 0x01;  // Enable Video Processor
	_V5_WriteReg_NC( ci, V5_VID_PROC_CFG, temp );

	// Set up the Framebuffer using the surface manager/allocator
	dprintf (("3dfx_accel: SetupCRTC - ci_MemSize: 0x%x\n", ci->MemSize));
	dprintf (
				("3dfx_accel: SetupCRTC - frame buffer requires (%d * %d * %d) = 0x%x\n",
				 dm->timing.h_display, dm->timing.v_display, bytespp,
				 (dm->timing.h_display * dm->timing.v_display * bytespp)));
	dprintf (("3dfx_accel: ci->AllocFrameBuffer 0x%x\n", ci->AllocFrameBuffer));
	if (ci->AllocFrameBuffer)
	{
		__mem_Free (MemMgr_2D, ci->AllocFrameBuffer);
		ci->AllocFrameBuffer = 0;
	}
	ci->AllocFrameBuffer =
		__mem_Allocate (MemMgr_2D, (dm->timing.h_display * dm->timing.v_display * bytespp));
	while (!ci->AllocFrameBuffer)
	{
		uint32 buf = MEM_COMMAND_FREE_MEMORY;
		// The alloc failed so we need to free locked memory.
		dprintf (("3dfx_accel: Framebuffer alloc failed! Will attempt to free locked memeory \n"));
		ioctl (devfd, MEM_IOCTL_RELEASE_SEMS, &buf, sizeof (buf));
		dprintf (("3dfx_accel: Trying framebuffer alloc again after ioctl \n"));
		ci->AllocFrameBuffer =
			__mem_Allocate (MemMgr_2D, (dm->timing.h_display * dm->timing.v_display * bytespp));
		dprintf (("3dfx_accel: New ci->AllocFrameBuffer 0x%x \n", ci->AllocFrameBuffer));
	}

	ci->AllocFrameBuffer->locked = 255;
	dprintf (("3dfx_accel: ci->AllocFrameBuffer 0x%x\n", ci->AllocFrameBuffer));
	fb_offset = (uint8 *) ci->AllocFrameBuffer->address - (uint8 *) ci->BaseAddr1;
	dprintf (("3dfx_accel: fb_offset 0x%x\n", fb_offset));

	dprintf (("3dfx_accel: SetupCRTC - fb_offset = 0x%x\n", fb_offset));
	dprintf (
				("3dfx_accel: SetupCRTC - calling SetDesktopSurface(regs = 0x%x, 1, 0, pixelFormat = 0x%x, clutBypass = %d, 0, fb_offset = 0x%x, stride = %d, 0x%x)\n",
				 ci->base_registers_pci[2], pixelFormat, !useClut, fb_offset, stride, stride));
	SetVideoDesktopSurface ((uint32) ci->base_registers_pci[2], 1, 0, pixelFormat, !useClut, 0,
									fb_offset, stride);

	if( !useClut )
	{
		float gamma = 2.5;
		char *env = getenv( "APP_GAMMA" );
		if( env )
		{
			sscanf( env, "%f", &gamma );
		}
		
		_V5_SetGamma( gamma, gamma, gamma );
	}

	ci->FBBase = (void *) ((uint8 *) ci->BaseAddr1 + fb_offset);
	ci->FBBase_DMA = (void *) ((uint8 *) ci->base_registers_pci[1] + fb_offset);

	// We need to store the Sync polarity settings locally too
	// that way we can use them later if we need to adjust the
	// DPMS settings.
	ci->CurDispMode.timing.flags |= (B_POSITIVE_VSYNC | B_POSITIVE_HSYNC);
	sync_pol = read_phys_8 (0xcc);
	if ((sync_pol & 0x80) == 0x80)
	{
		ci->CurDispMode.timing.flags &= ~B_POSITIVE_VSYNC;
	}
	if ((sync_pol & 0x40) == 0x40)
	{
		ci->CurDispMode.timing.flags &= ~B_POSITIVE_HSYNC;
	}
// dprintf(("3dfx_accel: SetupCRTC - EXIT\n"));
	return (B_OK);
}

uint32 colorspacebits (uint32 cs /* a color_space */ )
{
// dprintf(("3dfx_accel: colorspacebits - ENTER, cs = %d\n", cs));
	switch (cs)
	{
	case B_RGB32_BIG:
	case B_RGBA32_BIG:
	case B_RGB32_LITTLE:
	case B_RGBA32_LITTLE:
// dprintf(("3dfx_accel: colorspacebits - EXIT, returning 32\n"));
		return (32);
	case B_RGB24_BIG:
	case B_RGB24_LITTLE:
// dprintf(("3dfx_accel: colorspacebits - EXIT, returning 24\n"));
		return (24);
	case B_RGB16_BIG:
	case B_RGB16_LITTLE:
// dprintf(("3dfx_accel: colorspacebits - EXIT, returning 16\n"));
		return (16);
	case B_RGB15_BIG:
	case B_RGB15_LITTLE:
	case B_RGBA15_BIG:
	case B_RGBA15_LITTLE:
// dprintf(("3dfx_accel: colorspacebits - EXIT, returning 15\n"));
		return (15);
	case B_CMAP8:
// dprintf(("3dfx_accel: colorspacebits - EXIT, returning 8\n"));
		return (8);
	}
// dprintf(("3dfx_accel: colorspacebits - EXIT, returning 0\n"));
	return (0);
}

/*
 * move the display area, so that the GameKit works
 */

status_t movedisplay (uint16 x, uint16 y)
{
	uint32 fb_offset, stride;
	uint32 regBase;

	if (x != 0)
	{
		return B_ERROR;
	}
	regBase = (uint32) ci->base_registers_pci[2];
	fb_offset = (uint32) ci->FBBase - (uint32) ci->BaseAddr1;
	stride = ci->BytesPerRow;
	fb_offset += y * stride;
	_V5_WriteReg_NC( ci, V5_VID_DESKTOP_START_ADDR, (fb_offset & SST_VIDEO_START_ADDR) << SST_VIDEO_START_ADDR_SHIFT);
	return B_OK;
}
