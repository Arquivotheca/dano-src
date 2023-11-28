//*****************************************************************************
//
//	File:		GraphicsDriver.h
//
//	Description:	Interface for kernel graphics drivers.
//	
//	Copyright 1993-97, Be Incorporated, All Rights Reserved.
//
//*****************************************************************************

#ifndef _GRAPHICS_DRIVER_H
#define _GRAPHICS_DRIVER_H

#include <GraphicsDefs.h>

// How about split screen, h and v?


enum {
	
	// void* points to the language code
	// returns an error if language is not supported.
	B_SET_LANGUAGE = 10000,  // B_DEVICE_OP_CODES_END + 1
	
	/*   POST-DR9	
	// void* points to a graphics_capabilities struct
	B_REPORT_CAPABILITES,
	*/
	
	// void* points to a config struct
	B_GET_DISPLAY_CONFIG,
	// SetDislplayConfig MAY fail unless B_GET_PIXEL_FORMATS is
	// used to validate the timing.
	B_SET_DISPLAY_CONFIG,
	
			
	// Returns all information interesting to an application that
	// would draw, i.e. no timing info or anything like that.
	// void* points to a frame_buffer_info struct, filled in on return
	B_GET_FRAME_BUFFER_INFO,

	
	// void* points to an indexed_colors struct
	B_GET_INDEXED_COLORS,	// Uses indexed_color struct (slightly
	B_SET_INDEXED_COLORS,	// different than how Pierre defined it.
	                        // Maybe we should use a long instead?

	// void* points to an rgb_gamma struct
	B_GET_RGB_GAMMA,
	B_SET_RGB_GAMMA,

	
	// These return the timings the driver wants to supply.
	B_COUNT_CUSTOM_TIMINGS,
	B_GET_CUSTOM_TIMINGS,


	// void* points to a timing_color_space structure.  gad.
	B_GET_COLOR_SPACES,

	
	// void* points to a long, returns 1 if monitor is attached
	B_SENSE_MONITOR,	// Tells if a display is attached, if possible
	
	// void* points to a long, returns sense codes
	B_GET_SENSE_CABLE,		// Returns sense pin statuses

	/* DDC stuff is disabled for now.
	// void* points to 128-byte EDID block.  Uhhhh... We should
	// include the block number as well for VESA expansion.
	B_READ_DDC_EDID,				// will be implemented later
	// void* is the same as ReadDDCEdid, except it points to a
	// 64-byte VDIF block.  We need to include the block number.
	B_READ_DDC_VDIF,
	*/

	// void* points to a long: 0=on, 1=standby, 2=suspend, 4=off, 8=reduced
	// If SetPowerMode returns a positive value, it means the call succeeded,
	// but VRAM must be refreshed by the client, zero means no refresh needed.
	B_GET_POWER_MODE,
	B_SET_POWER_MODE,
	
	// Hardware cursor calls:
	B_SET_CURSOR,
	B_MOVE_CURSOR,
	B_SHOW_CURSOR,
	B_HIDE_CURSOR,
	
	// This gets the screen location relative to the desktop, in pixels.
	// These calls will only work if you asked for a larger desktop than
	// screen area in the B_SET_CONFIG call.
	B_GET_SCREEN_LOCATION,
	B_SET_SCREEN_LOCATION,
	
	B_GRAPHICS_DEVICE_OP_CODES_END = 19999	// start of user-defined opcodes
};


// for the display_timing.flags field below:
enum {
	B_TIMING_INTERLACED = 1<<7,
	B_POSITIVE_VSYNC = 1<<2,
	B_POSITIVE_HSYNC = 1<<1
};

typedef struct display_timing {
  // all other values (front porch, h/v frequency, etc.) can
  // be calculated from these values.

  float  pixel_clock;	// Pixel clock            (MHz)
  uint16 h_total;		// Horizontal Total       (pixels)
  uint16 h_display;		// Horizontal Display End (pixels)
  uint16 h_blank_start;	// Horizontal Blank Start (pixels)
  uint16 h_blank_width;	// Horizontal Blank Width (pixels)
  uint16 h_sync_start;	// Horizontal Sync Start  (pixels)
  uint16 h_sync_width;	// Horizontal Sync Width  (pixels)
  uint16 v_total;		// Vertical Total         (lines)
  uint16 v_display;		// Vertical Display End   (lines)
  uint16 v_blank_start;	// Vertical Blank Start   (lines)
  uint16 v_blank_width;	// Vertical Blank Width   (lines)
  uint16 v_sync_start;	// Vertical Sync (retrace) Start (lines)
  uint16 v_sync_width;	// Vertical Sync (retrace) Width (lines)
  uint16 flags;
  
  // uh, do we need a border too?
  uint16 unused1;	// should be set
  uint32 unused2;	// to all zeros.

} display_timing;


typedef struct named_display_timing {
  char name[64];
  display_timing timing;
} named_display_timing;


// NEW!!!
typedef struct timing_color_space {
  display_timing *timing;	// client supplies the timing...
  uint32 space;		// ...driver fills in the supported color spaces.
  // NOTE: space is not declared a color_space, as it is meant to
  // be a bit field of ALL supported color spaces, not just one.
  // Any time only one color space is expected, I'll use color_space.
} timing_color_space;


typedef struct indexed_colors {
  uint32 offset;		// The first color to set, zero based
  uint32 count;			// The number of contiguous colors to set
  rgb_color *colors;	// memory for (count) colors
} indexed_colors;


/*
typedef struct graphics_capabilities {
                        // a 0 means not supported, 1 means supported
  
  // Power management capabilities...
  uint8 dpms_services;	// bit 0=standby, 1=suspend, 2=off, 3=reduced on
  uint8 dpms_version;		// 0x00010000 means VESA DPMS 1.0
  
  // DDC capabilities...
  uint8 ddc_services;	// bit 0=DDC1, 1=DDC2, 2=screen blanked during xfer
  uint8 ddc_edid_time;	// Time, in seconds, rounded up, to xfer 128b EDID blk
                        // However, this may be dependent on current vsync.
} graphics_capabilites;
*/


typedef struct frame_buffer_info {
  void		*base;			// base address
  uint32	bytes_per_row;	// could also be called pitch
  uint32	width;			// desktop (imageable, not viewable) width
  uint32	height;			// ... and height
  color_space space;		// pixel format
  
  // Also add the physical measurements of the screen? 
  // Not sure if this should be driver territory; maybe it requires
  // calibration...?

} frame_buffer_info;


typedef struct display_config {
  
  display_timing	timing;
  color_space		space;


  // option list?
  
  // optional stuff below, not sure if this is the best way to handle it
  // for pan and zoom (0 to disable pan or zoom).  Maybe this stuff
  // should go in an option list...?

  uint32 width;			// Horizontal size of desktop (pixels)
  uint32 height;		// Vertical size of desktop (pixels)
  
  
  // Maybe we should not have a zoom.  If you set the screen
  // to 320x240 and the desktop to 640x480, the driver should
  // automatically realize that a zoom must happen...

  float h_zoom;			// Horizontal zoom factor (2 = zoom in 2X)
  float v_zoom;			// Vertical zoom factor
  
} display_config;


// Interpolation to a 10-bit DAC doesn't take too much effort
// and doesn't result in noticable aliasing (and, of course,
// downsampling to a 6-bit DAC is a no-brainer).  If further
// gamma table formats are required, we'll add them using
// separate ioctl calls.

typedef struct rgb_gamma {
  uchar		red[256];
  uchar		green[256];
  uchar 	blue[256];
} rgb_gamma;

#endif
