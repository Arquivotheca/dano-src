#ifndef _ACCELERANT_INFO_H_
#define _ACCELERANT_INFO_H_

// yuck
#include <surface/genpool.h>
#include <video_overlay.h>

//////////////////////////////////////////////////////////////////////////////
// Accelerant Header
//   This gets smaller and smaller.  Much of the important stuff has moved
// into bios.h recently.  This whole file may go soon.
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Typedefs //////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef struct {
	int	used;	// non-zero if in use
	// these are used to reposition the window during virtual desktop moves
	const overlay_buffer *ob;	// current overlay_buffer for this overlay
	overlay_window ow;	// current overlay_window for this overlay
	overlay_view ov;	// current overlay_view for this overlay
} mga_overlay_token;

//////////////////////////////////////////////////////////////////////////////
// Accelerant Information

typedef struct 
{
  uint64 last_idle_fifo; // last fifo slot we *know* the engine was idle after
  uint64 fifo_count;     // last fifo slot used
  uint64 fifo_limit;     // slot age after which command guaranteed complete

  int32 engine_ben;      // for acceleration benephore

  uint32 poolid;         // memory pool id for frame buffer
  uint8  *cursorData;	 // pointer to cursor data allocated from frame buffer pool
  uint32 fifo_mask;      // bit mask for retrieving FIFO free slot count
  uint32 set_cursor_colors;
  uint32 cursor_is_visible;
  uint32 mode_count;
  uint32 mem_size;
  uint32 YDstOrg;
  uint32 start_addr;
  uint32 bytes_per_pixel;
  uint32 pixels_per_row;
  uint32 pix_clk_max8;
  uint32 pix_clk_max16;
  uint32 pix_clk_max32;
  uint32 interleave;
  uint32 last_fg_color;
  uint32 last_command;

  uint16 hot_x;          // cursor hot spot
  uint16 hot_y;

  sem_id engine_sem;     // semaphore for engine ownership

  area_id mode_list_area;

  display_mode dm;         // current display mode configuration

  frame_buffer_config fbc; // bytes_per_row and start of frame buffer

  BMemSpec fb_spec;	// Ack!
  BMemSpec ovl_buffer_specs[4];
  overlay_buffer ovl_buffers[4];
  mga_overlay_token ovl_tokens[1];

  union 
  {
    BIOS_2064  orig;   // PC Millennium BIOS structure
    BIOS_1064  pins;   // MillenniumII and Mystique
    BIOS_G100  pins31; // Gx00 series cards
  } bios;              // info read from the bios

  // Additional BIOS-derived info extracted for clarity.
  bool block_mode_ok; // Indicates whether or not SGRAM/WRAM block mode functions are available.
} ACCELERANT_INFO;


//////////////////////////////////////////////////////////////////////////////
// Externs ///////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

extern ACCELERANT_INFO *ai;


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#endif
