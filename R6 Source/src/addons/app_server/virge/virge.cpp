//*****************************************************************************
//
//	File:		 virge.cpp
//
//	Description: add-on for graphics cards using s3 virge chips.
//
//	Copyright 1997, Be Incorporated
//
//*****************************************************************************

// This include just provides prototypes for the few kernel calls usable in the
// add-on (see documentation on graphics add-on development for more infos).
// This add-on has to be linked with a specific library (libkernel.a).
#include <OS.h>
#include <image.h>
#include <Debug.h>

// Graphics card add-ons standard structs and prototypes
#include <GraphicsCard.h>

// Specific stuff for that add-on
#include "virge.h"
#include <priv_syscalls.h>

#if 0
#if 0
#define dprintf printf
#define set_dprintf_enabled
#else
#define set_dprintf_enabled _kset_dprintf_enabled_
//#define dprintf _kdprintf_
#include <stdarg.h>
#include <stdio.h>
static void dprintf(const char *s, ...) {
	va_list ap;
	char buffer[1024];
	va_start(ap, s);
	vsprintf(buffer, s, ap);
	va_end(ap);
	_kdprintf_(buffer);
}
#endif
#else
#define set_dprintf_enabled
#define dprintf 
#endif


#define get_pci(o, s) read_pci_config(ci.pcii.bus, ci.pcii.device, ci.pcii.function, (o), (s))
#define set_pci(o, s, v) write_pci_config(ci.pcii.bus, ci.pcii.device, ci.pcii.function, (o), (s), (v))

#undef TRUE
#define TRUE 1

void DoSelectMode(bool ClearScreen);
int Y2DClipLine(int left, int top, int right, int bottom, long *x1,long *y1,long *x2, long *y2);

//**************************************************************************
//	Private globals of the s3 virge add-on.
//**************************************************************************

static clone_info ci;

//**************************************************************************
//	Private functions
//**************************************************************************

static void init_locks();
static void dispose_locks();
static void lock_io();
static void unlock_io();
static void lock_ge();
static void unlock_ge();
static void delay(bigtime_t);
static void wait_for_fifo(long);
static void wait_for_sync(void);
static void trio_set_clock(ulong setup);
static long trio_encrypt_clock(float freq);
static void do_virge_settings();

//**************************************************************************
//  Standard input/output functions to access io regsters
//**************************************************************************

#define inb(a)		*((vuchar *)(ci.base0 + (a)))
#define outb(a, b)	*((vuchar *)(ci.base0 + (a))) = (b)
#define inw(a)		*((vushort *)(ci.base0 + (a)))
#define outw(a, w)	*((vushort *)(ci.base0 + (a))) = (w)
#define inl(a)		*((vulong *)(ci.base0 + (a)))
#define outl(a, l)	*((vulong *)(ci.base0 + (a))) = (l)


extern "C" int		write_isa_io (int pci_bus, void *addr, int size, uint32 val);
extern "C" int		read_isa_io (int pci_bus, void *addr, int size);
extern "C" long		get_nth_pci_info (long index, pci_info *info);
extern "C" long		read_pci_config (uchar	bus,
												uchar	device,	
												uchar	function,
												long	offset,
												long	size);
extern "C" void		write_pci_config(uchar	bus,
												uchar	device,
												uchar	function,
												long	offset,
												long	size,
												long	value);


#ifdef __INTEL__
#define isa_inb(a)	read_isa_io (0, (char *)(a) - 0x00008000, 1)
#define isa_outb(a, b)	write_isa_io (0, (char *)(a) - 0x00008000, 1, (b))
#else
#define isa_inb(a)	*((vuchar *)(ci.isa_io + (a) - 0x00008000))
#define isa_outb(a, b)	*((vuchar *)(ci.isa_io + (a) - 0x00008000)) = (b)
#endif

//**************************************************************************
//	Tables for registers settings, for all spaces
//**************************************************************************

// ##----> THOSE SETTINGS SHOULD BE COMPATIBLE WITH ALL VGA CHIPS. <----##
// This table contains the settings for vga attribute registers.
// The first 16 values are useless, but at least completly safe.
// The first value is the index of the attribute register (as described in
// every databook), the second the value we set for the register.
static ushort	attribute_table[] =
{
	0x00,	0x00,
	0x01,	0x01,
	0x02,	0x02,
	0x03,	0x03,
	0x04,	0x04,
	0x05,	0x05,
	0x06,	0x14,
	0x07,	0x07,
	0x08,	0x38,
	0x09,	0x39,
	0x0a,	0x3a,
	0x0b,	0x3b,
	0x0c,	0x3c,
	0x0d,	0x3d,
	0x0e,	0x3e,
	0x0f,	0x3f,
	0x10,	0x41,
	0x11,	0x00,
	0x12,	0x0f,
	0x13,	0x00,
	0x14,	0x00,
	0xff,	0xff   // End-of-table
};

// ##----> THOSE SETTINGS SHOULD BE COMPATIBLE WITH ALL VGA CHIPS. <----##
// This table is used to set the vga gcr registers.
// The first value (16 bits) is the adress of the register in the 64K io-space,
// the second (only one significative byte) is the value we set.
// Two pairs are put on the same line, because those registers use indirect
// access (one register with many meanings, indexed by another register). The
// index always comes first.
static ushort	gcr_table[] =
{
	GCR_INDEX,	0x00,	GCR_DATA,	0x00,	// Set/Reset
	GCR_INDEX,	0x01,	GCR_DATA,	0x00,	// Enable Set/Reset
	GCR_INDEX,	0x02,	GCR_DATA,	0x00,	// Color Compare
	GCR_INDEX,	0x03,	GCR_DATA,	0x00,	// Raster Operations/Rotate Counter
	GCR_INDEX,	0x04,	GCR_DATA,	0x00,	// Read Plane Select
	GCR_INDEX,	0x05,	GCR_DATA,	0x40,	// Graphics Controller
	GCR_INDEX,	0x06,	GCR_DATA,	0x05,	// Memory Map Mode
	GCR_INDEX,	0x07,	GCR_DATA,	0x0f,	// Color Don't care
	GCR_INDEX,	0x08,	GCR_DATA,	0xff,	// Bit Mask
	0x000,	0x00                    // End-of-table
};

// ##----> THOSE SETTINGS SHOULD BE COMPATIBLE WITH ALL VGA CHIPS. <----##
// This table is used to set the standard vga sequencer registers.
// The first value (16 bits) is the adress of the register in the 64K io-space,
// the second (only one significative byte) is the value we set.
// Two pairs are put on the same line, because those registers use indirect
// access (one register with many meanings, indexed by another register). The
// index always comes first.
static ushort	sequencer_table[] =
{
	SEQ_INDEX,	0x00,	SEQ_DATA,	0x03,	// Reset
	SEQ_INDEX,	0x01,	SEQ_DATA,	0x01,	// Clocking mode
	SEQ_INDEX,	0x02,	SEQ_DATA,	0x0f,	// Enable write plane
	SEQ_INDEX,	0x03,	SEQ_DATA,	0x00,	// Char Font Select
	SEQ_INDEX,	0x04,	SEQ_DATA,	0x06,	// Memory Mode
	0x000,	0x00                    // End-of-table
};



//**************************************************************************
//	Tables for register settings, for each chip.
//**************************************************************************

//----------------------------------------------------------
// ViRGE
//----------------------------------------------------------
static ushort	vga_virge_table[] =
{
	SEQ_INDEX,  0x09,   SEQ_DATA,  0x80,   // IO map disable
	SEQ_INDEX,  0x0a,   SEQ_DATA,  0xe0, //0x40,   // External bus request (was 0x80/0xc0)
	SEQ_INDEX,  0x0d,   SEQ_DATA,  0x00,   // Extended sequencer 1
	SEQ_INDEX,  0x14,   SEQ_DATA,  0x00,   // CLKSYN
	SEQ_INDEX,  0x18,   SEQ_DATA,  0x40,   // RAMDAC/CLKSYN

	CRTC_INDEX,	0x11,   CRTC_DATA,	0x0c,	// CRTC regs 0-7 unlocked, v_ret_end = 0x0c
	CRTC_INDEX,	0x31,	CRTC_DATA,	0x0c, //0x08,//0x8c,	// Memory config (0x08)
	CRTC_INDEX,	0x32,	CRTC_DATA,	0x00,	// all interrupt disenabled
	CRTC_INDEX,	0x33,	CRTC_DATA,	0x22,	// Backward Compatibility 2
	CRTC_INDEX,	0x34,	CRTC_DATA,	0x00,	// Backward compatibility 3
	CRTC_INDEX,	0x35,	CRTC_DATA,	0x00,	// CRT Register Lock

	CRTC_INDEX,	0x37,	CRTC_DATA,	0xff, /* want 0x19, bios has 0xff : bit 1 is magic :-(
											 Trio32/64 databook says bit 1 is 0 for test, 1 for
											 normal operation.  Must be a documentation error? */
	CRTC_INDEX,	0x66,	CRTC_DATA,	0x01, 	// Extended Miscellaneous Control 1 (enable)
	//?? 0x66 first?
	CRTC_INDEX,	0x3a,	CRTC_DATA,	0x15, //0x95,	// Miscellaneous 1 //95
	//CRTC_INDEX,	0x3c,	CRTC_DATA,	0x40,	// Interlace Retrace Start

	CRTC_INDEX,	0x42,	CRTC_DATA,	0x00,	// Mode Control (non interlace)
	CRTC_INDEX,	0x43,	CRTC_DATA,	0x00,	// Extended Mode
	CRTC_INDEX,	0x45,	CRTC_DATA,	0x00,	// Hardware Graphics Cursor Mode

	//CRTC_INDEX,	0x52,	CRTC_DATA,	0x00,	// Extended BIOS Flag 1
	CRTC_INDEX,	0x53,	CRTC_DATA,	0x08,	// Extended Memory Cont 1
	CRTC_INDEX,	0x54,	CRTC_DATA,	0x02,	// Extended Memory Cont 2
	CRTC_INDEX,	0x55,	CRTC_DATA,	0x00,	// Extended DAC Control
	CRTC_INDEX,	0x56,	CRTC_DATA,	0x00,	// External Sync Cont 1
	//CRTC_INDEX,	0x57,	CRTC_DATA,	0x00,	// External Sync Cont 2
	CRTC_INDEX,	0x5c,	CRTC_DATA,	0x00, 	// General Out Port
	CRTC_INDEX,	0x61,	CRTC_DATA,	0x00,	// Extended Memory Cont 4 pg 18-23
	CRTC_INDEX,	0x63,	CRTC_DATA,	0x00,	// External Sync Delay Adjust High
	CRTC_INDEX,	0x65,	CRTC_DATA,	0x00, 	// Extended Miscellaneous Control 0x24
	CRTC_INDEX,	0x6a,	CRTC_DATA,	0x00,	// Extended System Control 4
	CRTC_INDEX,	0x6b,	CRTC_DATA,	0x00,	// Extended BIOS flags 3
	CRTC_INDEX,	0x6c,	CRTC_DATA,	0x00,	// Extended BIOS flags 4
	0x000,	0x00			        // End-of-table
};

static ushort	vga_virge_dx_gx_table[] =
{
	SEQ_INDEX,  0x1a,   SEQ_DATA,  0x00,	// testing and blank pedestal (disabled)
	SEQ_INDEX,  0x1b,   SEQ_DATA,  0x00,	// testing (disabled)

//	CRTC_INDEX,	0x26,   CRTC_DATA,	0x00,	// write after power-on
	CRTC_INDEX,	0x80,	CRTC_DATA,	0x00,	// memory drive current 8ma
	CRTC_INDEX,	0x85,	CRTC_DATA,	0x00,	// FIFO fetch delay
	CRTC_INDEX,	0x86,	CRTC_DATA,	0x80,	// DAC power saving disabled
	CRTC_INDEX,	0x90,	CRTC_DATA,	0x00,	// diable primary stream prefetch tweaks
	CRTC_INDEX,	0x92,	CRTC_DATA,	0x00	// diable secondary stream prefetch tweaks
};

//**************************************************************************
//  Allowed refresh rate ranges, for each space (in Hertz).
//**************************************************************************

// Minimal values. First line is 8 bits spaces, second 16 bits spaces,
// third 32 bits spaces. Resolutions are columns 1 to 5 : 640x480, 800x600,
// 1024x768, 1280x1024 and 1600x1200.
static float vga_min_rates[32] = {
	56.0, 56.0, 56.0, 56.0, 56.0,
	56.0, 56.0, 56.0, 56.0, 56.0,
	60.0, 60.0, 60.0, 60.0, 60.0,
	56.0, 56.0, 60.0, 60.0, 60.0,
	60.0, 60.0, 60.0, 60.0, 60.0,
	60.0, 60.0, 60.0, 60.0, 60.0,
	60.0, 60.0, 
};

// Maximal values. Same order as minimal values. Be careful with those values.
// High frequency are dangerous for monitors, especially old ones.
static float vga_max_rates[32] = {
	90.0, 90.0, 84.0, 75.0, 70.0,
	90.0, 90.0, 84.0, 75.0, 70.0,
	60.0, 60.0, 60.0, 60.0, 60.0,
	75.0, 75.0, 60.0, 60.0, 60.0,
	60.0, 60.0, 60.0, 60.0, 60.0,
	60.0, 60.0, 60.0, 60.0, 60.0,
	60.0, 60.0,
};



//**************************************************************************
//  GraphicsCard Hook Table declaration.
//**************************************************************************

// Prototypes for hook table (see documentation for more informations).

long   v_set_cursor_shape(uchar *data,uchar *mask,long dim_h,long dim_v,long hot_h,long hot_v);
long   v_move_cursor(long new_h,long new_v);
long   v_show_cursor(bool state);
long   v_line_8(long x1, long x2, long y1, long y2, uchar color,
			   bool useClip, short clipLeft, short clipTop, short clipRight, short clipBottom);
long   v_line_16(long x1, long x2, long y1, long y2, ushort color,
			   bool useClip, short clipLeft, short clipTop, short clipRight, short clipBottom);
long   array_line_8_bit(indexed_color_line *lines, long count, bool clip, short cxl, short cyt, short cxr, short cyb);
long   v_rect_8(long x1, long y1, long x2, long y2, uchar color);
long   v_rect_16(long x1, long y1, long x2, long y2, ushort color);
long   v_blit(long x1, long y1, long x2, long y2, long width, long height);
long   v_invert_rect(long x1, long y1, long x2, long y2);

// the hooks for the virge graphic engine
static graphics_card_hook vga_dispatch_virge[B_HOOK_COUNT] = {
// 0
	(graphics_card_hook)v_set_cursor_shape,
	(graphics_card_hook)v_move_cursor,
	(graphics_card_hook)v_show_cursor,
	(graphics_card_hook)v_line_8,
	0L, //(graphics_card_hook)v_line_32,
// 5
	(graphics_card_hook)v_rect_8,
	0L, // (graphics_card_hook)v_rect_32,
	(graphics_card_hook)v_blit,
	(graphics_card_hook)array_line_8_bit, // array line 8
	0L, // array line 32
// 10
	0L, // sync
	(graphics_card_hook)v_invert_rect,
	(graphics_card_hook)v_line_16,
	(graphics_card_hook)v_rect_16,
	0L,
// 15
	0L,
	0L,
	0L,
	0L,
	0L,
// 20
	0L,
	0L,
	0L,
	0L,
	0L,
// 25
	0L,
	0L,
	0L,
	0L,
	0L,
// 30
	0L,
	0L,
	0L,
	0L,
	0L,
// 35
	0L,
	0L,
	0L,
	0L,
	0L,
// 40
	(graphics_card_hook)v_line_16,
	(graphics_card_hook)v_rect_16,
	0L,
	0L,
	0L,
// 45
	0L,
	0L,
	0L,
};



//**************************************************************************
//  Card io access protection.
//
//  Those calls are use to guarantee that two threads are not trying to
//  access the memory and the general io-register (io_...) or the graphic
//  engine registers (ge_...) at the same time.
//  This protection is done by a atomic lock (very fast) and a semaphore
//  only when necessary (much slower), to put a thread in waiting state.
//  That two levels protection method allows very good performances. The
//  Be-implementation is currently called "Benaphore", as an extension of
//  semaphore, first use by Benoit Schillings.
//**************************************************************************

// Benaphore description (one long for the atomic lock, and an sem_id for
// the semaphore).
static sem_id	io_sem = -1;
static sem_id   ge_sem = -1;
static long	    io_lock, ge_lock;

// Create the benaphores (if possible)
static void init_locks()
{
	if (io_sem == -1) {
		io_lock = 0L;
		io_sem = create_sem(0,"vga io sem");
		ge_lock = 0L;
		ge_sem = create_sem(0,"vga ge sem");
	}
}

// Free the benaphore (if created)
static void dispose_locks()
{
	if (ge_sem >= 0)
		delete_sem(ge_sem);
	if (io_sem >= 0)
		delete_sem(io_sem);
}

// Protect the access to the general io-registers by locking a benaphore.
static void lock_io()
{
	int	old;

	old = atomic_add (&io_lock, 1);
	if (old >= 1) {
		acquire_sem(io_sem);	
	}	
}

// Release the protection on the general io-registers
static void unlock_io()
{
	int	old;

	old = atomic_add (&io_lock, -1);
	if (old > 1) {
		release_sem(io_sem);
	}
}	

// Protect the access to the memory or the graphic engine registers by
// locking a benaphore.
static void lock_ge()
{
	int	old;

	old = atomic_add (&ge_lock, 1);
	if (old >= 1) {
		acquire_sem(ge_sem);	
	}	
}

// Release the protection on the memory and the graphic engine registers.
static void unlock_ge()
{
	int	old;

	old = atomic_add (&ge_lock, -1);
	if (old > 1) {
		release_sem(ge_sem);
	}
}	



//**************************************************************************
//  Accelerated specific stuff.
//
//  Here is all the code for the hook functions (see documentation for hook
//  functions definitions).
//**************************************************************************

// Stupid delay loop for very short times (snooze is not working in those
// cases and it's better to do some stupid loop into the processor than
// always accessing the bus to look for the end of the current process).
static void delay(bigtime_t i)
{
	bigtime_t start = system_time();
	while(system_time() - start < i);
}

// This table is used to expand a byte xxxx0123 into the byte 00112233.
static uchar expand2[16] = {
	0x00,0x03,0x0c,0x0f,
	0x30,0x33,0x3c,0x3f,
	0xc0,0xc3,0xcc,0xcf,
	0xf0,0xf3,0xfc,0xff
};

// Wait for n*0x100 empty FIFO slots.
static void wait_for_fifo(long value) {
	if ((inl(SUBSYS_STAT) & 0x1f00) < value) {
		//do delay(40);
		while ((inl(SUBSYS_STAT) & 0x1f00) < value);
	}
}

static void wait_for_sync() {
	if ((inl(SUBSYS_STAT) & 0x2000) == 0) {
		//do delay(40);
		while ((inl(SUBSYS_STAT) & 0x2000) == 0);
	}
}

/// W Adam's stuff
// Clip the line to the bounds rectangle
int Y2DClipLine(int left, int top, int right, int bottom, 
	long *x1,long *y1,long *x2, long *y2)

{

int point_1 = 0, point_2 = 0;  // tracks if each end point is visible or invisible

int clip_always = 0;           // used for clipping override

int xi,yi;                     // point of intersection



int right_edge=0,              // which edges are the endpoints beyond
    left_edge=0,
    top_edge=0,
    bottom_edge=0;





int success = 0;               // was there a successfull clipping



float dx,dy;                   // used to holds slope deltas



// test if line is completely visible



if ( (*x1>=left) && (*x1<=right) &&
     (*y1>=top) && (*y1<=bottom) )
     point_1 = 1;





if ( (*x2>=left) && (*x2<=right) &&
     (*y2>=top) && (*y2<=bottom) )
     point_2 = 1;


	// test endpoints
	if (point_1==1 && point_2==1)
		return(1);



	// test if line is completely invisible
	if (point_1==0 && point_2==0)
	{
		// must test to see if each endpoint is on the same side of one of
		// the bounding planes created by each clipping region boundary

		if ( ((*x1<left) && (*x2<left)) || // to the left
        ((*x1>right) && (*x2>right)) || // to the right
        ((*y1<top) && (*y2<top)) || // above
        ((*y1>bottom) && (*y2>bottom)) )  // below
        {
        	// No need to draw line
			return(0);
		} // end if invisible


		// if we got here we have the special case where the line cuts into and
		// out of the clipping region

		clip_always = 1;

	} // end if test for invisibly


	// take care of case where either endpoint is in clipping region
	if (( point_1==1) || (point_1==0 && point_2==0) )
	{
		// compute deltas
		dx = *x2 - *x1;
		dy = *y2 - *y1;

		// compute what boundary line need to be clipped against
		if (*x2 > right)
		{
			// flag right edge
			right_edge = 1;

			// compute intersection with right edge
			if (dx!=0)
				yi = (int)(.5 + (dy/dx) * (right - *x1) + *y1);
			else
				yi = -1;  // invalidate intersection
		} // end if to right

		else  if (*x2 < left)
		{
			// flag left edge
			left_edge = 1;

			// compute intersection with left edge
			if (dx!=0)
				yi = (int)(.5 + (dy/dx) * (left - *x1) + *y1);
			else
				yi = -1;  // invalidate intersection
		} // end if to left

		// horizontal intersections
		if (*y2 > bottom)
		{
			// flag bottom edge
			bottom_edge = 1;

			// compute intersection with right edge
			if (dy!=0)
				xi = (int)(.5 + (dx/dy) * (bottom - *y1) + *x1);
			else
				xi = -1;  // invalidate inntersection

		} // end if bottom
		else
		if (*y2 < top)
		{
			// flag top edge
			top_edge = 1;

			// compute intersection with top edge
			if (dy!=0)
				xi = (int)(.5 + (dx/dy) * (top - *y1) + *x1);
			else
				xi = -1;  // invalidate intersection
		} // end if top


		// now we know where the line passed thru
		// compute which edge is the proper intersection

		if (right_edge==1 && (yi>=top && yi<=bottom) )
		{
			*x2 = right;
			*y2 = yi;
			success = 1;
		} // end if intersected right edge
		else
		if (left_edge==1 && (yi>=top && yi<=bottom) )
		{
			*x2 = left;
			*y2 = yi;

			success = 1;
		} // end if intersected left edge



		if (bottom_edge==1 && (xi>=left && xi<=right) )
		{
			*x2 = xi;
			*y2 = bottom;
			success = 1;
		} // end if intersected bottom edge
		else
		if (top_edge==1 && (xi>=left && xi<=right) )
		{
			*x2 = xi;
			*y2 = top;
			success = 1;
		} // end if intersected top edge
	} // end if point_1 is visible


	// reset edge flags
	right_edge = left_edge= top_edge = bottom_edge = 0;



	// test second endpoint
	if ( (point_2==1) || (point_1==0 && point_2==0))
	{
		// compute deltas
		dx = *x1 - *x2;
		dy = *y1 - *y2;

		// compute what boundary line need to be clipped against
		if (*x1 > right)

      {

      // flag right edge
      right_edge = 1;

      // compute intersection with right edge
      if (dx!=0)
         yi = (int)(.5 + (dy/dx) * (right - *x2) + *y2);
      else
         yi = -1;  // invalidate inntersection
      } // end if to right
   else
   if (*x1 < left)
      {
      // flag left edge
      left_edge = 1;

      // compute intersection with left edge
      if (dx!=0)
         yi = (int)(.5 + (dy/dx) * (left - *x2) + *y2);
      else
         yi = -1;  // invalidate intersection
      } // end if to left


   // horizontal intersections
   if (*y1 > bottom)
      {
      // flag bottom edge

      bottom_edge = 1;

      // compute intersection with right edge
      if (dy!=0)
         xi = (int)(.5 + (dx/dy) * (bottom - *y2) + *x2);
      else
         xi = -1;  // invalidate inntersection

		} // end if bottom

		else
		if (*y1 < top)
		{
			// flag top edge
			top_edge = 1;

			// compute intersection with top edge
			if (dy!=0)
				xi = (int)(.5 + (dx/dy) * (top - *y2) + *x2);
			else
				xi = -1;  // invalidate inntersection
		} // end if top


		// now we know where the line passed thru
		// compute which edge is the proper intersection
		if (right_edge==1 && (yi>=top && yi<=bottom) )
		{
			*x1 = right;
			*y1 = yi;

			success = 1;
		} // end if intersected right edge

		else

		if (left_edge==1 && (yi>=top && yi<=bottom) )
		{
			*x1 = left;
			*y1 = yi;

			success = 1;

		} // end if intersected left edge


		if (bottom_edge==1 && (xi>=left && xi<=right) )
		{
			*x1 = xi;
			*y1 = bottom;

			success = 1;
		} // end if intersected bottom edge
		else
		if (top_edge==1 && (xi>=left && xi<=right) )
		{
			*x1 = xi;
			*y1 = top;

			success = 1;
		} // end if intersected top edge
	} // end if point_2 is visible

	return(success);
}

// Stroke a line in 8 bits mode. (see documentation for more informations)
long v_line_8(
	long   x1,         // Coordinates of the two extremities
	long   x2,         //
	long   y1,         //
	long   y2,         //
	uchar  color,      // Indexed color
	bool   useClip,    // Clipping enabling
	short  clipLeft,   // Definition of the cliping rectangle if 
	short  clipTop,    // cliping enabled
	short  clipRight,  //
	short  clipBottom) //
{
	int         XMajor;
	long        dx, dy, tmp, Xdelta;

	// clip first (the app_server is sending us negative co-ordinates!)
	if (useClip)
		// if the line is clipped out
		if (Y2DClipLine(clipLeft, clipTop, clipRight, clipBottom, &x1, &y1, &x2, &y2) == 0)
			// we're finished
			return B_NO_ERROR;

	//dprintf("v_line_8(%ld,%ld,%ld,%ld)\n", x1,x2,y1,y2);
// This call is using the graphic engine and access video memory, so we need to
// get the lock graphic-engine benaphore.
	lock_ge();

	// reset unit
	wait_for_fifo(0x400);
	outl(0xa900, 0x78000000);
	outl(0xa8d4, 0);
	outl(0xa8d8, 0);
	outl(0xa8e4, ci.scrnRowByte | (ci.scrnRowByte << 16));
	//dprintf("scrnRowByte: %d 0x%08x\n", ci.scrnRowByte, ci.scrnRowByte);
	
// ViRGE needs:
//
// 0,0
//     x2,y2
//      /
//     /
//    /
// x1,y1
//
// or
//
// 0,0
//  x2,y2
//    \ 
//     \ 
//      \ 
//     x1,y1

// Invert vector if incorrect y order
// fix so that y1 >= y2
	if (y1 < y2) {
		tmp = x1;
		x1 = x2;
		x2 = tmp;
		tmp = y1;
		y1 = y2;
		y2 = tmp;
	}
	
// Calculate vector coordinates...
	dx = x1-x2;
	dy = y1-y2;

// process Xdelta
	if (dy) Xdelta = (-(dx << 20)) / dy;
	else Xdelta = 0;
	if ((dx > dy) || (-dx > dy)) XMajor = 1;
	else XMajor = 0;
	//dprintf("dx,dy: %d,%d\n", dx, dy);
	//dprintf("Xdelta: 0x%08x %d ", Xdelta, Xdelta);
	wait_for_fifo(0x700);
	outl(0xa970, Xdelta);
	
// Xstart
	tmp = (x1<<20);
	if (XMajor) {
		tmp += (Xdelta/2);
		if (Xdelta < 0)
			tmp +=((1<<20)-1);
	}
	//dprintf("Xstart: %d 0x%08x\n", tmp, tmp);
	outl(0xa974, tmp);

// Ystart
	outl(0xa978, y1);

// Xends
	outl(0xa96c, (x1<<16)|x2);

// color
	outl(0xa8f4, color);
	
// go !
	//dprintf("line %d,%d to %d,%d\n", x1, y1, x2,y2);
//	dprintf(" Xstart: %f\n", (float)tmp / (float)(1<<20));
//	dprintf(" Xdelta: %f\n", (float)Xdelta / (float)(1<<20));
	tmp = 0x19e00121;
#if 1
	if (useClip) {
		//dprintf(" with clipping to %d,%d %d,%d\n", clipLeft, clipTop, clipRight, clipBottom);
		outl(0xa8e0, ((clipTop & 0x7ff)<<16) | (clipBottom & 0x7ff));
		outl(0xa8dc, ((clipLeft & 0x7ff)<<16) | (clipRight & 0x7ff));
		tmp |= 2;
		wait_for_fifo(0x200);
	}
#endif
	outl(0xa900, tmp);

// Ycount
	if (x1 >= x2)
		//outl(0xa97c, y1-y2+1);
		outl(0xa97c, dy+1);
	else
		//outl(0xa97c, (y1-y2+1) | 0x80000000);
		outl(0xa97c, (dy+1) | 0x80000000);

	wait_for_sync();

// Release the benaphore of the graphic engine
	unlock_ge();
	return B_NO_ERROR;
}

// Stroke a line in 8 bits mode. (see documentation for more informations)
long v_line_16(
	long   x1,         // Coordinates of the two extremities
	long   x2,         //
	long   y1,         //
	long   y2,         //
	ushort color,      // Indexed color
	bool   useClip,    // Clipping enabling
	short  clipLeft,   // Definition of the cliping rectangle if 
	short  clipTop,    // cliping enabled
	short  clipRight,  //
	short  clipBottom) //
{
	int         XMajor;
	long        dx, dy, tmp, Xdelta;

	// clip first (the app_server is sending us negative co-ordinates!)
	if (useClip)
		// if the line is clipped out
		if (Y2DClipLine(clipLeft, clipTop, clipRight, clipBottom, &x1, &y1, &x2, &y2) == 0)
			// we're finished
			return B_NO_ERROR;

	//dprintf("v_line_8(%ld,%ld,%ld,%ld)\n", x1,x2,y1,y2);
// This call is using the graphic engine and access video memory, so we need to
// get the lock graphic-engine benaphore.
	lock_ge();

	// reset unit
	wait_for_fifo(0x400);
	outl(0xa900, 0x78000000);
	outl(0xa8d4, 0);
	outl(0xa8d8, 0);
	outl(0xa8e4, ci.scrnRowByte | (ci.scrnRowByte << 16));
	//dprintf("scrnRowByte: %d 0x%08x\n", ci.scrnRowByte, ci.scrnRowByte);
	
// ViRGE needs:
//
// 0,0
//     x2,y2
//      /
//     /
//    /
// x1,y1
//
// or
//
// 0,0
//  x2,y2
//    \ 
//     \ 
//      \ 
//     x1,y1

// Invert vector if incorrect y order
// fix so that y1 >= y2
	if (y1 < y2) {
		tmp = x1;
		x1 = x2;
		x2 = tmp;
		tmp = y1;
		y1 = y2;
		y2 = tmp;
	}
	
// Calculate vector coordinates...
	dx = x1-x2;
	dy = y1-y2;

// process Xdelta
	if (dy) Xdelta = (-(dx << 20)) / dy;
	else Xdelta = 0;
	if ((dx > dy) || (-dx > dy)) XMajor = 1;
	else XMajor = 0;
	//dprintf("dx,dy: %d,%d\n", dx, dy);
	//dprintf("Xdelta: 0x%08x %d ", Xdelta, Xdelta);
	wait_for_fifo(0x700);
	outl(0xa970, Xdelta);
	
// Xstart
	tmp = (x1<<20);
	if (XMajor) {
		tmp += (Xdelta/2);
		if (Xdelta < 0)
			tmp +=((1<<20)-1);
	}
	//dprintf("Xstart: %d 0x%08x\n", tmp, tmp);
	outl(0xa974, tmp);

// Ystart
	outl(0xa978, y1);

// Xends
	outl(0xa96c, (x1<<16)|x2);

// color
	outl(0xa8f4, color);
	
// go !
	//dprintf("line %d,%d to %d,%d\n", x1, y1, x2,y2);
//	dprintf(" Xstart: %f\n", (float)tmp / (float)(1<<20));
//	dprintf(" Xdelta: %f\n", (float)Xdelta / (float)(1<<20));
	tmp = 0x19e00125;
#if 1
	if (useClip) {
		//dprintf(" with clipping to %d,%d %d,%d\n", clipLeft, clipTop, clipRight, clipBottom);
		outl(0xa8e0, ((clipTop & 0x7ff)<<16) | (clipBottom & 0x7ff));
		outl(0xa8dc, ((clipLeft & 0x7ff)<<16) | (clipRight & 0x7ff));
		tmp |= 2;
		wait_for_fifo(0x200);
	}
#endif
	outl(0xa900, tmp);

// Ycount
	if (x1 >= x2)
		//outl(0xa97c, y1-y2+1);
		outl(0xa97c, dy+1);
	else
		//outl(0xa97c, (y1-y2+1) | 0x80000000);
		outl(0xa97c, (dy+1) | 0x80000000);

	wait_for_sync();

// Release the benaphore of the graphic engine
	unlock_ge();
	return B_NO_ERROR;
}

long array_line_8_bit(indexed_color_line *lines, long count, bool clip, short cxl, short cyt, short cxr, short cyb) {
	long x1, y1, x2, y2, color, newcolor;
	int  XMajor;
	long dx, dy, tmp, Xdelta;
	
// This call is using the graphic engine and access video memory, so we need to
// get the lock graphic-engine benaphore.
	lock_ge();

	// reset unit
	wait_for_fifo(0x600);
	outl(0xa900, 0x78000000);
	outl(0xa8d4, 0);
	outl(0xa8d8, 0);
	outl(0xa8e4, ci.scrnRowByte | (ci.scrnRowByte << 16));
	// and clipping
	tmp = 0x19e00121;
#if 1
	if (clip) {
		//dprintf(" with clipping to %d,%d %d,%d\n", clipLeft, clipTop, clipRight, clipBottom);
		outl(0xa8e0, ((cyt & 0x7ff)<<16) | (cyb & 0x7ff));
		outl(0xa8dc, ((cxl & 0x7ff)<<16) | (cxr & 0x7ff));
		tmp |= 2;
		wait_for_fifo(0x100);
	}
#endif
	outl(0xa900, tmp);

	
	//dprintf("scrnRowByte: %d 0x%08x\n", ci.scrnRowByte, ci.scrnRowByte);
	color = -1;
	while (count--) {
		x1 = lines->x1;
		y1 = lines->y1;
		x2 = lines->x2;
		y2 = lines->y2;
		newcolor = lines->color;
		lines++;

		if (clip)
			// if the line is clipped out
			if (Y2DClipLine(cxl, cyt, cxr, cyb, &x1, &y1, &x2, &y2) == 0)
				// we're finished
				continue;

// ViRGE needs:
//
// 0,0
//     x2,y2
//      /
//     /
//    /
// x1,y1
//
// or
//
// 0,0
//  x2,y2
//    \ 
//     \ 
//      \ 
//     x1,y1

// Invert vector if incorrect y order
// fix so that y1 >= y2
	if (y1 < y2) {
		tmp = x1;
		x1 = x2;
		x2 = tmp;
		tmp = y1;
		y1 = y2;
		y2 = tmp;
	}
	
// Calculate vector coordinates...
	dx = x1-x2;
	dy = y1-y2;

// process Xdelta
	if (dy) Xdelta = (-(dx << 20)) / dy;
	else Xdelta = 0;
	if ((dx > dy) || (-dx > dy)) XMajor = 1;
	else XMajor = 0;
	//dprintf("dx,dy: %d,%d\n", dx, dy);
	//dprintf("Xdelta: 0x%08x %d ", Xdelta, Xdelta);
	wait_for_fifo(0x600);
	outl(0xa970, Xdelta);
	
// Xstart
	tmp = (x1<<20);
	if (XMajor) {
		tmp += (Xdelta/2);
		if (Xdelta < 0)
			tmp +=((1<<20)-1);
	}
	//dprintf("Xstart: %d 0x%08x\n", tmp, tmp);
	outl(0xa974, tmp);

// Ystart
	outl(0xa978, y1);

// Xends
	outl(0xa96c, (x1<<16)|x2);

// color
	if (newcolor != color) outl(0xa8f4, color = newcolor);
	
// go !
// Ycount
	if (x1 >= x2)
		//outl(0xa97c, y1-y2+1);
		outl(0xa97c, dy+1);
	else
		//outl(0xa97c, (y1-y2+1) | 0x80000000);
		outl(0xa97c, (dy+1) | 0x80000000);
	}
	
	// don't return before we're finished
	wait_for_sync();

// Release the benaphore of the graphic engine
	unlock_ge();
	return B_NO_ERROR;
}

// Fill a rect in 8 bits (see documentation for more informations)
long v_rect_8(long  x1,    // The rect to fill. Call will always respect
			long  y1,    // x1 <= x2, y1 <= y2 and the rect will be
			long  x2,    // completly in the current screen space (no
			long  y2,    // cliping needed).
			uchar color) // Indexed color.
{
// This call is using the graphic engine and access video memory, so we need to
// get the lock graphic-engine benaphore.
	lock_ge();

	wait_for_fifo(0x800);
	outl(0xa500, 0x78000000);
	outl(0xa4d4, 0);	// source base addr
	outl(0xa4d8, 0);	// dest base addr
	outl(0xa4e4, ci.scrnRowByte | (ci.scrnRowByte << 16));	// source/dest bytes per row
	outl(0xa4f4, color);
	outl(0xa504, ((x2-x1)<<16)+(y2-y1+1));
	outl(0xa50c, (x1<<16)+y1);
	outl(0xa500, 0x17e00120);
	wait_for_sync();

// Release the benaphore of the graphic engine
	unlock_ge();
	return B_NO_ERROR;
}

// Fill a rect in 16 bits (see documentation for more informations)
long v_rect_16(long  x1,    // The rect to fill. Call will always respect
			long  y1,    // x1 <= x2, y1 <= y2 and the rect will be
			long  x2,    // completly in the current screen space (no
			long  y2,    // cliping needed).
			ushort color) // Indexed color.
{
// This call is using the graphic engine and access video memory, so we need to
// get the lock graphic-engine benaphore.
	lock_ge();

	wait_for_fifo(0x800);
	outl(0xa500, 0x78000000);
	outl(0xa4d4, 0);	// source base addr
	outl(0xa4d8, 0);	// dest base addr
	outl(0xa4e4, ci.scrnRowByte | (ci.scrnRowByte << 16));	// source/dest bytes per row
	outl(0xa4f4, color);
	outl(0xa504, ((x2-x1)<<16)+(y2-y1+1));
	outl(0xa50c, (x1<<16)+y1);
	outl(0xa500, 0x17e00124);
	wait_for_sync();

// Release the benaphore of the graphic engine
	unlock_ge();
	return B_NO_ERROR;
}

// Blit a rect from screen to screen (see documentation for more informations)
long v_blit(long  x1,     // top-left point of the source
		  long  y1,     //
		  long  x2,     // top-left point of the destination
		  long  y2,     //
		  long  width,  // size of the rect to move (from border included to
		  long  height) // opposite border included).
{
	ulong	command;

// check for zero height/width blits :-(
	//if ((height == 0) || (width == 0)) return B_NO_ERROR;

// Check degenerated blit (source == destination)
	if ((x1 == x2) && (y1 == y2)) return B_NO_ERROR;

	//dprintf("blit: from %d,%d to %d,%d size %d,%d depth %d\n", x1,y1, x2,y2, width, height, ci.scrnColors);
// This call is using the graphic engine and access video memory, so we need to
// get the lock graphic-engine benaphore.
	lock_ge();

	command = 0x07980020;

	if (x1 < x2) {
		x1 += width;// - 1;
		x2 += width;// - 1;
		command &= ~0x02000000;
	}
	if (y1 < y2) {
		y1 += height;// - 1;
		y2 += height;// - 1;
		command &= ~0x04000000;
	}
	// 16 bit mode
	if (ci.scrnColors == 16) command |= 0x00000004;
	
	wait_for_fifo(0x700);
	outl(0xa500, 0x78000000); // turn off auto-execute
	outl(0xa4d4, 0);	// source base addr
	outl(0xa4d8, 0);	// dest base addr
	outl(0xa4e4, ci.scrnRowByte | (ci.scrnRowByte << 16));	// source/dest bytes per row
	outl(0xa504, ((width)<<16)|(height+1));
	outl(0xa508, (x1<<16)|y1);
	outl(0xa50c, (x2<<16)|y2);
	outl(0xa500, command);
	//dprintf("command was: 0x%08x\n", command);
	wait_for_sync();
	
// Release the benaphore of the graphic engine
	unlock_ge();
	return B_NO_ERROR;
}


// Change the bitmap shape of the hardware cursor (see documentation for
// more informations).
long v_set_cursor_shape(uchar *data, // XorMask
					  uchar *mask, // AndMask
					  long  dim_h, // Dimensions of the cursor (in pixel)
					  long  dim_v, //
					  long  hot_h, // Position of the hot_spot in the cursor
					  long  hot_v) //
{
	int     i, j, dim_h0;
	bool    init;
	uchar   *buf;
	uchar   d0, m0, d1, m1;

// This call is using the io-register and the video memory. So we need to get
// both the lock io-register benaphore and the graphic engine benaphore.
// NB : When both benaphores are taken, they must be taken always in the same
// order to avoid any stupid deadlock.
	lock_io();
	lock_ge();
	
// Width of the bitmap in bytes
	dim_h0 = (dim_h+7)>>3;

// Check if it's the first cursor definition
	if (ci.hotpt_h == -1000) init = TRUE;
	else init = FALSE;

// Memorize the new hot-spot.
	ci.hotpt_h = hot_h;
	ci.hotpt_v = hot_v;

// Initialise the bitmap hardware cursor as completly transparent
	buf = (uchar*)ci.scrnBufBase;
	for (i=0;i<1024;i+=4) {
		//buf[i] = 0x00; // and bits
		buf[i+0] = buf[i+1] = 0xff; // and bits
		buf[i+2] = buf[i+3] = 0x00; // xor bits
	}

// Select the ci.CursorMode depending of the chip and the space
	for (i=0;i<dim_v;i++)
		for (j=0;j<dim_h0;j++) {
			buf[2+i*16+j+(j&6)] = data[j+i*dim_h0];
			buf[0+i*16+j+(j&6)] = mask[j+i*dim_h0];
		}

	outb(CRTC_INDEX,0x0e);
	outb(CRTC_DATA,0xff);
	outb(CRTC_INDEX,0x0f);
	outb(CRTC_DATA,0x00);
	
// Set the two significant colors (black and white)
	outb(CRTC_INDEX,0x45);
	vuchar t = inb(CRTC_DATA);
	outb(CRTC_INDEX,0x4b);
	outb(CRTC_DATA,0xff);
	outb(CRTC_DATA,0xff);
	outb(CRTC_DATA,0xff);
	outb(CRTC_INDEX,0x45);
	t = inb(CRTC_DATA);
	outb(CRTC_INDEX,0x4a);
	outb(CRTC_DATA,0x00);
	outb(CRTC_DATA,0x00);
	outb(CRTC_DATA,0x00);
	
// Set the pointer to the hardware cursor data.
	buf = (uchar *)((ci.theMem - 1) * 1024);
	//dprintf("cursor offset in vid RAM: 0x%08x\n", buf);
	outb(CRTC_INDEX,0x4c);
	outb(CRTC_DATA,((long)buf>>18)&0x0f);
	//outb(CRTC_DATA, 0);
	outb(CRTC_INDEX,0x4d);
	outb(CRTC_DATA,((long)buf>>10)&255);
	//outb(CRTC_DATA, 0);

// If it's the first definition of the cursor shape, initialise the default
// position and the default visibility mode.
	if (init) {
		outb(CRTC_INDEX,0x46);
		outb(CRTC_DATA,0);
		outb(CRTC_INDEX,0x47);
		outb(CRTC_DATA,0);
		outb(CRTC_INDEX,0x49);
		outb(CRTC_DATA,0);
		outb(CRTC_INDEX,0x4e);
		outb(CRTC_DATA,0);
		outb(CRTC_INDEX,0x4f);
		outb(CRTC_DATA,0);
		outb(CRTC_INDEX,0x48);
		outb(CRTC_DATA,0);
		outb(CRTC_INDEX,0x45);
		outb(CRTC_DATA,0x00);
	}
	
// Release the graphic engine benaphore and the benaphore of the io-registers.
	unlock_ge();
	unlock_io();
	return B_NO_ERROR;
}
	

// Move the hardware cursor to a new position of the hot_spot (see documentation
// for more informations).
long v_move_cursor(long new_h, // New hot_spot coordinates
				 long new_v) //
{
	long h, v, dh, dv;

// This call is using only the io-register (not the graphic engine). So we need
// to get only the lock io-register benaphore.
	lock_io();

// Calculate the part of the cursor masked by the left border of the screen
	if (ci.hotpt_h > new_h) {
		dh = ci.hotpt_h-new_h;
		h = 0;
	}
	else {
		dh = 0;
		h = new_h-ci.hotpt_h;
	}

// Calculate the part of the cursor masked by the top border of the screen	
	if (ci.hotpt_v > new_v) {
		dv = ci.hotpt_v-new_v;
		v = 0;
	}
	else {
		dv = 0;
		v = new_v-ci.hotpt_v;
	}

// Set position and offset (for top-left border cliping) of the hardware cursor
// The order of the writing to the register (the high part of v at the end) is
// necessary to get a smooth move. In other orders, you could get a bad position
// during on frame from time to time. Don't ask why.
	outb(CRTC_INDEX,0x46);
	outb(CRTC_DATA,(h>>8)&7);
	outb(CRTC_INDEX,0x47);
	outb(CRTC_DATA,(h)&255);
	outb(CRTC_INDEX,0x49);
	outb(CRTC_DATA,(v)&255);
	outb(CRTC_INDEX,0x4e);
	outb(CRTC_DATA,dh);
	outb(CRTC_INDEX,0x4f);
	outb(CRTC_DATA,dv);
	outb(CRTC_INDEX,0x48);
	outb(CRTC_DATA,(v>>8)&7);
	
// Release the benaphore of the io-registers.
	unlock_io();
	return B_NO_ERROR;
}


// This function is used to show or hide the hardware cursor (see documentation
// for more informations).
long v_show_cursor(bool state)
{
// This call is using only the io-register (not the graphic engine). So we need
// to get the lock io-register benaphore.
	lock_io();

// Show the cursor
	if (state != 0) {
		outb(CRTC_INDEX,0x45);
		outb(CRTC_DATA,0x01);
	}
	
// Hide the cursor
	else {
		outb(CRTC_INDEX,0x45);
		outb(CRTC_DATA,0x00);
	}
	
// Release the benaphore of the io-registers.
	unlock_io();
	return B_NO_ERROR;
}


// This function is use to inverse a rectangle in all modes bits mode.
long v_invert_rect(long x1, long y1, long x2, long y2)
{
	//ulong command = 0x10660120;
	ulong command = 0x16aa0120;
	// 16 bit mode
	if (ci.scrnColors == 16) command |= 0x00000004;
// This call is using the graphic engine and access video memory, so we need to
// get the lock graphic-engine benaphore.
	lock_ge();

	wait_for_fifo(0x700);
	outl(0xa500, 0x78000000);
	outl(0xa4d4, 0);	// source base addr
	outl(0xa4d8, 0);	// dest base addr
	outl(0xa4e4, ci.scrnRowByte | (ci.scrnRowByte << 16));	// source/dest bytes per row
	outl(0xa504, ((x2-x1)<<16)|(y2-y1+1));
	outl(0xa50c, (x1<<16)|y1);
	outl(0xa500, command);
	wait_for_sync();

// Release the benaphore of the graphic engine
	unlock_ge();
	return B_NO_ERROR;
}



//**************************************************************************
//  Global In/Out routines
//
//  Those functions are used to do the setting of many registers in one call,
//  typically from one of the big tables in the first half of that source.
//**************************************************************************


// One set of vga register need a specific process to be set. That function
// is used to set those attribute registers.
static void	set_attr_table(ushort *ptr)
{
	ushort	p1;
	ushort	p2;
	vuchar	v;
	vuchar	t;

// ATTR_REG is the only register used at the same time as index and data register.
// So we've to be careful to stay synchronize (the card switch between index
// and data at each write. Reading INPUT_STATUS_1 give us a simple way to stay well-
// synchronized.
	//dprintf("set_attr_table() begins\n");
	t = inb(INPUT_STATUS_1);
// Save the initial index set in ATTR_REG
	v = inb(ATTR_REG);

	while (TRUE) {
		p1 = *ptr++;
		p2 = *ptr++;
		if (p1 == 0xff && p2 == 0xff) {
			t = inb(INPUT_STATUS_1);
  // Restore the initial index set in ATTR_REG
			outb(ATTR_REG, v | 0x20);
		//	dprintf("set_attr_table() ends\n");
			return;
		}
		t = inb(INPUT_STATUS_1);
		outb(ATTR_REG, p1);
		outb(ATTR_REG, p2);
	}
}


static void	isa_set_table(ushort *ptr)
{
	ushort	p1;
	ushort	p2;

	while (TRUE) {
		p1 = *ptr++;
		p2 = *ptr++;
		if (p1 == 0 && p2 == 0) {
			return;
		}
		isa_outb(p1, p2);
	}
}


// All the other table settings are done with that very simple function.
static void	set_table(ushort *ptr)
{
	ushort	p1;
	ushort	p2;

	while (TRUE) {
		p1 = *ptr++;
		p2 = *ptr++;
		if (p1 == 0 && p2 == 0) {
			return;
		}
		outb(p1, p2);
	}
}

static void noisy_set_table(ushort *ptr)
{
	ushort	p1;
	ushort	p2;

	while (TRUE) {
		p1 = *ptr++;
		p2 = *ptr++;
		if (p1 == 0 && p2 == 0) {
			return;
		}
		outb(p1, p2);
		dprintf("0x%04x (0x%02x) was ", p1, p2);
		p1 = *ptr++;
		p2 = *ptr++;
		dprintf("0x%02x, now 0x%02x\n", inb(p1), p2);
		outb(p1, p2);
		//snooze(1000000);
	}
}


//**************************************************************************
//  DAC mode setting
//
//  This part is old enough to be probably compatible with all the RamDAC.
//**************************************************************************


// ##----> THOSE SETTINGS SHOULD BE COMPATIBLE WITH ALL VGA CHIPS. <----##
// Set the DAC in the standard 8 bits (indexed color) mode. This mode is the
// default one (00).
static void	setup_dac(void)
{
//	dprintf("setup_dac()\n");
#if 0
// To set the mode, we need to access the DAC hidden register using multiple
// access to DAC_ADR_MASK to unlock it. 
	inb(DAC_ADR_MASK);
	inb(DAC_ADR_MASK);
	inb(DAC_ADR_MASK);
	inb(DAC_ADR_MASK);
	outb(DAC_ADR_MASK, 0x00);

// After that, we reset the DAC_ADR_MASK counter protecting the hidden register.	
	inb(DAC_WR_INDEX);

// And then we disable the write mask.
#endif
	outb(DAC_ADR_MASK, 0xff);
//	dprintf("setup_dac() finished\n");
}

//**************************************************************************
//  S3 trio internal DAC specific stuff.
//
//  On trio chips, the DAC and the clock generators are integrated into the
//  graphic chip. Consequently, the interface is pretty simple.
//**************************************************************************


// This table is used to calculate the master video frenquency needed to get
// a selected refresh rate. First line is 8 bits spaces, second 16 bits
// spaces, third 32 bits spaces. Resolutions are columns 1 to 5 : 640x480,
// 800x600, 1024x768, 1280x1024 and 1600x1200 (see GraphicsCard.h for space
// definition).
// 
// See att498_ScreenClockStep for more informations and example of how
// calculate those values.
static float trio_ScreenClockStep[32] =
{
	8.0, 8.0, 8.0, 8.0, 8.0,
	8.0, 8.0, 8.0, 8.0, 8.0,
	0.0, 0.0, 0.0, 0.0, 0.0,
	8.0, 8.0, 0.0, 0.0, 0.0,
	0.0, 0.0, 0.0, 0.0, 0.0,
	0.0, 0.0, 0.0, 0.0, 0.0,
	0.0, 0.0,
};


// Send a 32 bits configuration word to the trio DAC to program the frequency
// of the two clock generators (see trio32/64 databook for more informations).
// Each clock settings is packed into 2 bytes, using each 7 bits.
void trio_set_clock(ulong setup)
{
//	dprintf("trio_set_clock(0x%08x)\n", setup);
// Program the memory clock frenquency
	outb(SEQ_INDEX, 0x15);
	outb(SEQ_DATA, 0x00);

	outb(SEQ_INDEX, 0x10);
	outb(SEQ_DATA, (setup>>24)&127);
	outb(SEQ_INDEX, 0x11);
	outb(SEQ_DATA, (setup>>16)&127);
	
	outb(SEQ_INDEX, 0x15);
	outb(SEQ_DATA, 0x01);

// Program main video clock frenquency
	outb(SEQ_INDEX, 0x15);
	outb(SEQ_DATA, 0x00);

	outb(SEQ_INDEX, 0x12);
	outb(SEQ_DATA, (setup>>8)&127);
	outb(SEQ_INDEX, 0x13);
	outb(SEQ_DATA, setup&127);
	
	outb(SEQ_INDEX, 0x15);
	outb(SEQ_DATA, 0x02);

// All done
	outb(SEQ_INDEX, 0x15);
	outb(SEQ_DATA, 0x80);

//	dprintf("trio_set_clock(0x%08x) finished\n", setup);
}


// Convert a clock frenquency in Mhz into the trio clock generator format (see
// the trio32/64 databook for more information). Those clock generator provide
// Fpll = (1/2^R)*Fref*(M+2)/(N+2) with M described by 7 bits, N by 5 bits and
// R by 2 bits.
long trio_encrypt_clock(float freq)
{
	float    fr,fr0,fr2, orig = freq;
	long     m,n,m2,n2,r;

// Fpll*2^R has to be > 135.0 Mhz.
	r = 0;
	while (freq < 135.0) {
		freq *= 2.0;
		r++;
	}

// Look for the best choice in all the allowed (M,N) couples (see trio32/64
// databook for specific conditions).
	fr0 = freq*(1.0/TI_REF_FREQ);
	fr2 = -2.0;
	for (n=1;n<=31;n++) {
  // Check all allowed values of N, calculate the best choice for M
		m = (int)(fr0*(float)(n+2)-1.5);
		fr = ((float)(m+2))/((float)(n+2));
  // Keep the most accurate choice.
		if (((fr0-fr)*(fr0-fr) < (fr0-fr2)*(fr0-fr2)) && (m >= 1) && (m <= 127)) {
			m2 = m;
			n2 = n;
			fr2 = fr;
		}
	}

//	dprintf("trio_encrypt_clock(%f) r: %d, n: %d, m: %d\n", orig, r, n2, m2);
// Return the 3 parameters packed into a 15 bits word.
	return ((r<<13)+(n2<<8)+m2);
}



//**************************************************************************
//  Clock generator global setting
//
//  For each DAC available, program the good memory and video clock frenquency
//  depending of the space (resolution and depth), the refresh rate selected
//  and the crt settings when CRT_CONTROL is available.
//**************************************************************************

static void	write_pll()
{
	ulong		setup;
	float       clock;

//	dprintf("write_pll()\n");	
// Set the memory clock to the standard frequency, 60 Mhz.
	setup = trio_encrypt_clock(60.0);

// As the trio DAC uses CRT_CONTROL, the clock frequency depends of the
// horizntal crt counter increment multiplied by the horizontal and vertical
// sizes of the frame (as described by crt registers), multiplied by the
// selected refresh rate.
	clock = trio_ScreenClockStep[ci.scrnResNum] * ci.scrnRate *
		    (float)ci.lastCrtHT * (float)ci.lastCrtVT;
	clock = ci.dot_clock * 1000;
	
// Packed definition of both clock settings (memory and video)
//	dprintf("clocks: %f, %f\n", clock, clock * 1e-6);
	setup = (setup<<16)+trio_encrypt_clock(clock * 1e-6);
//	dprintf("about to trio_set_clock(0x%08x)\n", setup);
	trio_set_clock(setup);
//	dprintf("write_pll() finished\n");	
}


//**************************************************************************
//  CRT_CONTROL stuff for 864 and trio.
//
//  The purpose of CRT_CONTROL is to allow the user to modify the horizontal
//  and the vertical positions of the display on the screen. The width and
//  height can be modified too, but the result is clearly dependant of the
//  monitor abilities. The technic used to do that consists in modifying the
//  position of the horizontal and vertical synchro for one part, and
//  modifying the proportion between display size (h or v) and total size
//  (h or v). We do that by bilinear interpolation between four states for each
//  direction (4 for horizontal settings, 4 for vertical settings). Get a look
//  to crt_864_640x480_map for an example. More explanation are available
//  with the code below.
//**************************************************************************


// This is the index of the 28 crt registers and extended registers (all access
// through 3d5 index by 3d4) modified by CRT_CONTROL. After that, those
// registers will just be refered as number 0 to 27.
static uchar crt_index_864[] =
{ 0x00, 0x3b, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
  0x07, 0x09, 0x10, 0x11, 0x12, 0x15, 0x16, 0x5d,
  0x5e,
  0x08, 0x0a, 0x0b, 0x0c, 0x0d, 0x13, 0x14, 0x17,
  0x18,
  0x51, 0x69 };

typedef struct {
	float	refresh;
	ulong	dot_clock_KHz;
	ushort h_disp, h_front_porch, h_sync_width, h_back_porch, h_sync_pol;
	ushort v_disp, v_front_porch, v_sync_width, v_back_porch, v_sync_pol;
} VESA_Timings;

static VESA_Timings monitor_timings[] = {
	{60.0,  25175,  640, 16,  96,  48, 0,  480, 10, 2, 33, 0},
	{60.0,  40000,  800, 40, 128,  88, 1,  600,  1, 4, 23, 1},
	{60.0,  65000, 1024, 24, 136, 160, 0,  768,  3, 6, 29, 0},
	{70.0,  94200, 1152, 32,  96, 192, 1,  900,  1, 3, 46, 1},
	{60.0, 108000, 1280, 48, 112, 256, 1, 1024,  1, 3, 38, 1},
	{ 0.0,      0,    0,  0,   0,   0, 0,    0,  0, 0,  0, 0}
};

// Values to set we need to set into the 28 previous registers to get the
// selected CRT_CONTROL settings.
static 	uchar     settings[28];


// Calculate the settings of the 28 registers to get the positions and sizes
// described in ci.crtPosH, ci.crtPosV, ci.crtSizeH and ci.crtSizeV (value from 0 to 100).
static void prepare_crt (long pixel_per_unit)
{
	int       i,par1,par2,par3;
	uchar     Double;
	ulong     Eset[2];
	ulong     Hset[7],Vset[6];
	ulong	h_d, h_f_p, h_s_w, h_b_p;

//	dprintf("prepare_crt(%d)\n", pixel_per_unit);

#if 0
	/* Find a setting in the VESA_Timing table */
	VESA_Timings *pVT = monitor_timings;
	while (pVT->refresh) {
		if (ci.scrnWidth == pVT->h_disp) break;
		pVT++;
	}
	h_d = pVT->h_disp / pixel_per_unit;
	h_f_p = pVT->h_front_porch / pixel_per_unit;
	h_s_w = pVT->h_sync_width / pixel_per_unit;
	h_b_p = pVT->h_back_porch / pixel_per_unit;
	ci.dot_clock = pVT->dot_clock_KHz;
	
	Hset[0] = h_d + h_f_p + h_s_w + h_b_p;
	Hset[1] = h_d;
	Hset[2] = Hset[1];
	Hset[3] = h_d + h_f_p + h_s_w;
	Hset[4] = h_d + h_f_p;
	Hset[5] = Hset[3];

	Vset[0] = pVT->v_disp + pVT->v_front_porch + pVT->v_sync_width + pVT->v_back_porch;
	Vset[1] = pVT->v_disp;
	Vset[2] = Vset[0];
	Vset[3] = pVT->v_disp;
	Vset[4] = pVT->v_disp + pVT->v_front_porch;
	Vset[5] = Vset[4] + pVT->v_sync_width;
#else
// Calculate the value of the 7 basic horizontal crt parameters depending of
// ci.crtPosH and ci.crtSizeH. Those 7 parameters are (in the same order as refered
// in crt_864_640x480 for example) :
//   - 0 : Horizontal total number of clock character per line.
//   - 1 : Horizontal number of clock character in the display part of a line.
//   - 2 : Start of horizontal blanking.
//   - 3 : End of horizontal blanking.
//   - 4 : Start of horizontal synchro position.
//   - 5 : End of horizontal synchro position.
//   - 6 : fifo filling optimization parameter
	while (TRUE) {
		Hset[1] = ci.scrnWidth/pixel_per_unit;
		Hset[0] = (Hset[1]*(700-ci.crtSizeH)+250)/500;
		par1 = (3*Hset[1]+100)/200;
		if (par1 < 2) par1 = 2;
		Hset[2] = Hset[1]+par1;
		Hset[3] = Hset[0]-par1;
		i = Hset[3]-Hset[2]-127;
		if (i>0) {
			Hset[3] -= i/2;
			Hset[2] += (i+1)/2;
		}

		par3 = (Hset[1]+5)/10;
		par2 = Hset[0]-Hset[1]-2*par1-par3;
		if (par2 < 0) {
			par3 += par2; 
			par2 = 0;
		}
		Hset[4] = Hset[2]+(par2*(100-ci.crtPosH)+50)/100;
		Hset[5] = Hset[4]+par3;
		if (par3>63) {
			Hset[5] -= (par3-63)/2;
			Hset[4] += (par3-62)/2;
		}

#if 0
		if (theVGA == s3_964)
			Hset[6] = (Hset[0]+Hset[4])/2;
		else
			Hset[6] = Hset[0] - 5;
#endif
		Hset[6] = Hset[0] - 10; // typically 5 less than what will be programmed into CR0, 10 less than Hset[0]
		if (Hset[6] > Hset[3])
			Hset[6] = Hset[3] - 1; // must lie in the blanking interval
		if (Hset[6] > 505) {
			ci.crtSizeH++;
			continue;
		}
		
		break;
	}
	
// Calculate the value of the 6 basic vertical crt parameters depending of
// ci.crtPosV and ci.crtSizeV. Those 6 parameters are (in the same order as refered
// in crt_864_640x480 for example) :
//   - 0 : Vertical total number of counted lines per frame
//   - 1 : Start of vertical blanking (line number).
//   - 2 : End of vertical blanking (line number).
//   - 3 : Vertical number of lines in the display part of a frame.
//   - 4 : Start of vertical synchro position (line number).
//   - 5 : End of vertical synchro position (line number).
	Vset[3] = ci.scrnHeight;
	Vset[0] = (Vset[3]*108+50)/100;
	par1 = (Vset[3]*5+250)/500;
	if (par1 < 2) par1 = 1;
	Vset[4] = Vset[3]+par1;
	Vset[5] = Vset[0]-par1;
	par3 = (Vset[3]+80)/160;
	if (par3 < 2) par3 = 2;
	par2 = Vset[0]-Vset[3]-2*par1-par3;
	if (par2 < 0) {
		par3 += par2; 
		par2 = 0;
	}
	Vset[1] = Vset[4]+((100-ci.crtPosV)*par2+50)/100;
	Vset[2] = Vset[1]+par3;

#endif
// Calculate extended parameters (rowByte and base adress of the frameBuffer
	Eset[0] = ((ci.scrnPosH*ci.scrnColors)/8+ci.scrnPosV*ci.scrnRowByte)/4;
	Eset[1] = ci.scrnRowByte/8;
	
// Memorize the last setting for horizontal and vertical total for use to set
// the clock frequency (see write_pll for more informations).
	ci.lastCrtHT = Hset[0];
	ci.lastCrtVT = Vset[0];
	if (ci.lastCrtVT < 300) {
		Double = 0x80;
		ci.lastCrtVT *= 2;
	}
	else
		Double = 0x00;

	ci.dot_clock = (ulong)(ci.lastCrtHT * ci.lastCrtVT * pixel_per_unit * (ci.scrnRate / 1000.0));
//	dprintf("H: %04x %04x %04x %04x %04x %04x, FIFO: %04x\n",
//	Hset[1], Hset[2], Hset[4], Hset[5], Hset[3], Hset[0], Hset[6]);
// 5 of those 15 parameters have to be adjust by a specific offset before to be
// encoded in the appropriate registers.
	Hset[0] -= 5;
	Hset[1] -= 1;
	Vset[0] -= 2;
	Vset[3] -= 1;
	Vset[4] -= 1;

//	dprintf("H: %04x %04x %04x %04x %04x %04x, FIFO: %04x\n",
//	Hset[1], Hset[2], Hset[4], Hset[5], Hset[3], Hset[0], Hset[6]);
// 2 of those registers are encoded in a bit complicated way (see databook for
// more information)
	Hset[3] = (Hset[3] & 0x03f); //(Hset[3]&63)+((Hset[3]-Hset[2])&64);
	Hset[5] = (Hset[5] & 0x01f); //(Hset[5]&31)+((Hset[5]-Hset[4])&32);
//	dprintf("H: %04x %04x %04x %04x %04x %04x, FIFO: %04x\n",
//	Hset[1], Hset[2], Hset[4], Hset[5], Hset[3], Hset[0], Hset[6]);

// Those 2 parameters need to be trunc respectively to 4 and 8 bits.
	Vset[2] = (Vset[2]&15);
	Vset[5] = (Vset[5]&255);

// Those 12 parameters are encoded through the 28 8-bits registers. That code
// is doing the conversion, even inserting a few more significant bit depending
// of the space setting.
	settings[0] = Hset[0]&255;
	settings[1] = Hset[6]&255;
	settings[2] = Hset[1]&255;
	settings[3] = Hset[2]&255;
	settings[4] = Hset[3]&31 | 128; // EGA/VGA guide: bit 7 always 1.
	settings[5] = Hset[4]&255;
	settings[6] = Hset[5]&31 | ((Hset[3]&32)<<2);
	settings[7] = Vset[0]&255;
	settings[8] = ((Vset[0]&256)>>8) | ((Vset[3]&256)>>7) |
		          ((Vset[1]&256)>>6) | 0x10 |
				  ((Vset[4]&256)>>5) | ((Vset[0]&512)>>4) |
				  ((Vset[3]&512)>>3) | ((Vset[1]&512)>>2);
	settings[9] = Double | 0x40 | ((Vset[4]&512)>>4);
	settings[10] = Vset[1]&255;
	settings[11] = Vset[2]&15;
	settings[12] = Vset[3]&255;
	settings[13] = Vset[4]&255;
	settings[14] = Vset[5]&255;
	settings[15] = ((Hset[0]&256)>>8) | ((Hset[1]&256)>>7) |
		           ((Hset[2]&256)>>6) | ((Hset[3]&64)>>3) |
				   ((Hset[4]&256)>>4) | (Hset[5]&32) | ((Hset[6]&256)>>2);
	settings[16] = ((Vset[0]&1024)>>10) | ((Vset[3]&1024)>>9) |
		           ((Vset[4]&1024)>>8) | ((Vset[1]&1024)>>6);
	settings[17] = 0x00;
	settings[18] = 0x20;
	settings[19] = 0x00;
	settings[20] = (Eset[0]&0xff00)>>8;
	settings[21] = (Eset[0]&0xff);
	settings[22] = (Eset[1]&0xff);
	settings[23] = 0x60;
	settings[24] = 0xeb;
	settings[25] = 0xff;
	settings[26] = (Eset[1]&0x300)>>4;
	settings[27] = (Eset[0]&0xf0000)>>16;

//	dprintf("prepare_crt(%d) finished\n", pixel_per_unit);
}


// Set the CRT_CONTROL by writing the preprocessed values in the 28 registers.
static void vid_select_crt()
{
	int      i;
	volatile long  j;
//	dprintf("vid_select_crt()\n");
	for (i=0;i<28;i++) {
		outb(CRTC_INDEX, crt_index_864[i]);
		//for (j=0;j<6;j++) {;}
		outb(CRTC_DATA, settings[i]);
		//for (j=0;j<6;j++) {;}
		//dprintf("CR%02x = 0x%02x (%d)\n", crt_index_864[i], settings[i], settings[i]);
		//snooze(1000000);
	}
//	dprintf("vid_select_crt() finished\n");
}


// Just move the frame_buffer base adress
static void vid_select_crt_pos()
{
//	dprintf("vid_select_crt_pos()\n");
	outb(CRTC_INDEX, crt_index_864[20]);
	outb(CRTC_DATA, settings[20]);
	outb(CRTC_INDEX, crt_index_864[21]);
	outb(CRTC_DATA, settings[21]);
	outb(CRTC_INDEX, crt_index_864[27]);
	outb(CRTC_DATA, settings[27]);
//	dprintf("vid_select_crt_pos() finished\n");
}



//**************************************************************************
//  CRT settings global management.
//
//  This part is doing the complete settings of all crt registers (including
//  CRT_CONTROL when necessary), for all depth and resolutions. Spaces are
//  decomposed by depth because depth allow common parts between different
//  resolutions. The opposite is not always true.
//**************************************************************************

//**************************************************************************
//  Complete specific color, width, mem and performance settings for virge.
//**************************************************************************

static uchar index_virge_settings[4] =
{
	0x0b, 0x36, 0x58, 0x67
};


void do_virge_settings()
{
	int      i;
	uchar    value[4];
	
//	dprintf("do_virge_settings()\n");
	if (ci.scrnColors == 8) {
		value[0] = 0x00;
		value[3] = 0x00; //0x0c;
	 } else {
		value[0] = 0x50; //0x50;
	 	value[3] = 0x50; //0x50;
	 }

	if (ci.theMem == 2048) {
		value[1] = 0x8a; //0x8e; //0x8e;
		value[2] = 0x12; //0x12
	} else { // 4MB
		value[1] = 0x0a; //0x0e; // 0x0e
		value[2] = 0x13; // 0x13
	}
	
	outb(SEQ_INDEX,index_virge_settings[0]);
	outb(SEQ_DATA,value[0]);
	for (i=1;i<4;i++) {
		outb(CRTC_INDEX,index_virge_settings[i]);
		outb(CRTC_DATA,value[i]);
	}
#if 0
	if (ci.scrnColors == 8) {
		outl(0x8180, 0x00000000);
		outl(0x8190, 0x00000000);
	} else {
		outl(0x8180, 0x05000000);
		outl(0x8190, 0x05000000);
	}
	outl(0x8184, 0x00000000);
	outl(0x8194, 0x00000000);
	outl(0x8198, 0x00000000);
	outl(0x81a0, 0x01000000);
	outl(0x81c0, 0x00000000);
	outl(0x81c4, 0x00000000);
	outl(0x81c8, ci.scrnRowByte);
	outl(0x81cc, 0x00000000);
	outl(0x81d0, 0x00000000);
	outl(0x81d4, 0x00000000);
	outl(0x81d8, 0x00000001);
	outl(0x81dc, 0x40000000);
	outl(0x81e0, 0x00000000);
	outl(0x81e4, 0x00000000);
	outl(0x81e8, 0x00000000);
	outl(0x81f0, 0x00010001);
	outl(0x81f4, (ci.scrnHeight << 16) | (ci.scrnWidth-1));
	outl(0x81f8, 0x07ff07ff);
	outl(0x81fc, 0x00010001);
	outl(0x8200, 0x00006088);
#endif
//	dprintf("do_virge_settings() finished\n");
}


//**************************************************************************
//  Mode selection.
//
//  Do the configuration for a specific mode, without the glue.
//**************************************************************************
void DoSelectMode(bool ClearScreen)
{
	int			i, j;
	uchar 		*base;
	ulong       *draw;

	// snooze(10000);
//	dprintf("DoSelectMode(%d)\n", (int)ClearScreen);

	// not sure this is required.
	//setup_dac();

	// streams fifo ctrl
	outl(0x8200, 0x0000c000);
	// disable streams
	outb(CRTC_INDEX, 0x67);
	outb(CRTC_DATA, inb(CRTC_DATA) & ~0x0c);

// Now, we've to preprocess the CRT_CONTROL (if available)
	prepare_crt(64 / ci.scrnColors);
	
// After that, we now know the real size of the display and so we can set
// set the selected refresh rate.
	write_pll();
// After the clock setting, we can program the crt registers
	vid_select_crt();		

	do_virge_settings();

	vuchar t = inb(MISC_OUT_R);
	t &= 0x3f;	// clear top bits, we'll set them below
	if      (ci.scrnHeight < 400)
		t |= 0x40; /* +vsync -hsync : 350 lines */
	else if (ci.scrnHeight < 480)
		t |= 0x80; /* -vsync +hsync : 400 lines */
	else if (ci.scrnHeight < 768)
		t |= 0xC0; /* -vsync -hsync : 480 lines */
	else
		t |= 0x00; /* +vsync +hsync : other */
	outb(MISC_OUT_W, t);

// Better to initialize the frame buffer with a standard black and white
// pattern. We don't know how long will it take for the application server
// to decide to refresh all the screen. Better to get that than the previous
// buffer in the bad mode. I know, this is not really optimized...
	snooze(10000);

#if 0
	if (ClearScreen) {
		if (ci.scrnColors == 8) {
			base = (uchar*)ci.scrnBase;
			for (i = 0; i < ci.scrnHeight / 2; i++) {
				for (j = 0; j < ci.scrnWidth / 2; j++) {
					*(base++) = 0xff;
					*(base++) = 0x00;
				}
				for (j = 0; j < ci.scrnWidth / 2; j++) {
					*(base++) = 0x00;
					*(base++) = 0xff;
				}
			}
		}
		else {
			draw = (ulong*)ci.scrnBase;
			for (i = 0; i < ci.scrnHeight / 2; i++) {
				for (j = 0; j < ci.scrnWidth / 2; j++) {
					*(draw++) = 0xffffffff;
					*(draw++) = 0x00000000;
				}
				for (j = 0; j < ci.scrnWidth / 2; j++) {
					*(draw++) = 0x00000000;
					*(draw++) = 0xffffffff;
				}
			}
		}
	}
#endif
}
	


//**************************************************************************
//  Check the amount of available memory
//
//  First, we're supposing that every card will have at least 1MB of memory.
//  After that, we test all the different interesting configuration one by
//  one. We can't just test memory from bottom to top because the mapping
//  can depend of the memory size set (chips can use from 16 bits to 64 bits
//  bus depending of the amount of available memory). So we've to set a
//  memory size, then test it to see if there isn't any wrapping.
//  Nowadays, 2MB is the only other size tested.
//
//  After that, this function determined the available spaces.
//**************************************************************************

static void vid_checkmem (void)
{
	ulong      i, j;
	vuchar *scrn;
	uchar	x;

//	dprintf("vid_checkmem()\n");
	SCREEN_OFF;

// test 4 MB configuration
	ci.theMem = 4096;
	ci.scrnColors = 8;
	ci.scrnWidth = 1024;
	ci.scrnHeight = 768;
	ci.scrnRowByte = 1024;
	ci.scrnRes = vga1024x768;

	DoSelectMode(FALSE);
	
//	dprintf("clearing video memory...\n");
	// clear video memory
	v_rect_8(0,0,1023,4095,0);
//	dprintf("Done!\n");
	
	// reset for later
	ci.scrnColors = 0;
	ci.scrnRes = -1;
	
	scrn = (vuchar *)ci.scrnBase;
	scrn += 2 * 1024 * 1024;
	*scrn = 0xab;
	scrn -= 2 * 1024 * 1024;
	*scrn = 0xcd;
	scrn += 2 * 1024 * 1024;
	clear_caches(ci.scrnBase, 4 * 1024 * 1024, B_FLUSH_DCACHE);
	dprintf("byte at %08x: 0x%02x\n", scrn, *scrn);
	if (*scrn == 0xab)
		ci.theMem = 4096;
	else {
		/* turn off the P50 pin - the docs say this shouldn't be required :-( */
		outb(SEQ_INDEX, 0x0a); outb(SEQ_DATA, inb(SEQ_DATA) & 0xbf);
		scrn -= 2 * 1024 * 1024;
		dprintf("byte at %08x: 0x%02x\n", scrn, *scrn);
		if (*scrn == 0xcd) ci.theMem = 2048;
		else ci.theMem = 1024; // should never happen
	}

// The spaces available depend of the amount of video memory available...
// Not really, for now, as we always have enough for the 8bit modes.
	ci.available_spaces =
		B_8_BIT_640x480 | B_16_BIT_640x480 |
		B_8_BIT_800x600 | B_16_BIT_800x600 |
		B_8_BIT_1024x768 | B_16_BIT_1024x768 |
		B_8_BIT_1152x900 | // B_16_BIT_1152x900 |
		B_8_BIT_1280x1024; // can do B_16_BIT_1280x1024 if interlaced, but it takes 4MB
		// B_8_BIT_1600x1200;
	dprintf("ci.theMem: %d, spaces: 0x%08x\n", ci.theMem, ci.available_spaces);
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

static void vid_selectmode (bool ClearScreen)
{
// As we are doing very very bad things, it's better to protect everything. So
// we need to get both io-register and graphic engine benaphores.
// NB : When both benaphores are taken, they must be taken always in the same
// order to avoid any stupid deadlock.
	lock_io();
	lock_ge();
	
// Blank the screen. Not doing that is criminal (you risk to definitively
// confuse your monitor, because in that case we're reprogramming most of the
// registers. So at time, we can get very bad intermediate state).
	SCREEN_OFF;
// Select the good mode...
	DoSelectMode(ClearScreen);
	
// We need a buffer of 1024 bytes to store the cursor shape. This buffer has to
// be aligned on a 1024 bytes boundary. We put it just after the frame buffer.
// But if you want, you're free to put it at the beginning of the video memory,
// and then to offset the frame buffer by 1024 bytes.

//	ci.scrnBufBase = (uchar*)(((long)ci.scrnBase + ci.scrnRowByte * ci.scrnHeight + 1023) & 0xFFFFFC00);

// Unblank the screen now that all the very bad things are done.
	SCREEN_ON;

// Release the graphic engine benaphore and the benaphore of the io-registers.
	unlock_ge();
	unlock_io();
}



//**************************************************************************
//  Current refresh rate setting on the fly...
//
//  This part is trying to provide a fast an efficient way to modify the
//  current CRT_CONTROL and refresh rate as fast as possible, without
//  disturbing too much the display. The quality of the result depends of
//  the monitor (brand new multi-sync are really touchy about frenquency
//  modifications).
//**************************************************************************

static void vid_selectrate (void)
{
	
// As we are doing very bad things, it's better to protect everything. So we
// need to get both io-register and graphic engine benaphores.
// NB : When both benaphores are taken, they must be taken always in the same
// order to avoid any stupid deadlock.
	lock_io();
	lock_ge();

// Blank the screen. Not doing that is too dangerous (you risk to definitively
// confuse some monitors, putting them in a strange state from which they will
// never come back, except turn off/turn on).
	SCREEN_OFF;

// Initialize the s3 chip in a reasonnable start configuration. Turn the chip
// on and unlock all the registers we need to access (and even more...). Do a
// few standard vga config.
	snooze(10000);

// Preprocess the CRT_CONTROL, set the new video clock frequency and then
// set the crt registers. 
	prepare_crt(64 / ci.scrnColors);

	write_pll();
	vid_select_crt();
	snooze(10000);

// Unblank the screen now that all the bad things are done.
	SCREEN_ON;

// Release the graphic engine benaphore and the benaphore of the io-registers.
	unlock_ge();
	unlock_io();
}



//**************************************************************************
//  Current position setting on the fly...
//
//  This part is trying to provide a fast an efficient way to modify the
//  current CRT_CONTROL and refresh rate as fast as possible, without
//  disturbing too much the display. The quality of the result depends of
//  the monitor (brand new multi-sync are really touchy about frenquency
//  modifications).
//**************************************************************************

static void vid_selectpos(void)
{
// As we are doing very bad things, it's better to protect everything. So we
// need to get both io-register and graphic engine benaphores.
// NB : When both benaphores are taken, they must be taken always in the same
// order to avoid any stupid deadlock.
	lock_io();
	lock_ge();

// Preprocess the CRT_CONTROL, set the new video clock frequency and then
// set the crt registers. 
	prepare_crt(64 / ci.scrnColors);

	vid_select_crt_pos();

// Release the graphic engine benaphore and the benaphore of the io-registers.
	unlock_ge();
	unlock_io();
}



//**************************************************************************
//  The last one : the main entry of the Graphics add-on.
//
//  When the application server load this add-on, this is its only entry
//  point, and so the only symbol he looks for (see the documentation for
//  more informations about command, parameters buffer and return value).
//**************************************************************************

long control_graphics_card(ulong message,void *buf)
{
	int        i, j, err;
	ulong      code;

// By default, anybody is supposed to return B_NO_ERROR if nothing specific
// goes wrong.
	err = B_NO_ERROR;

// Message dispatcher. Other message will be added in future version. So you
// need to define the default case and then return B_ERROR.
	switch (message) {

// Used by the application server to get information about the frame buffer and
// part of the abilities of the add-on itself.
	case B_GET_GRAPHICS_CARD_INFO :
#define GCI		((graphics_card_info*)buf)
  // Current height of the frame buffer (not only the display part) in lines. 
	    GCI->height = ci.offscrnHeight;
  // Current width of the frame buffer in pixels
		GCI->width = ci.offscrnWidth;
  // Current depth of the screen in bits per pixel	
		GCI->bits_per_pixel = ci.scrnColors;
  // Pointer on the beginning of the current frame buffer
		GCI->frame_buffer = (void*)ci.scrnBase;
  // Offset (in byte) between the beginning of two lines
		GCI->bytes_per_row = ci.scrnRowByte;
  // Define if some optional abilities are available (see documentation for
  // more informations).
		GCI->flags = B_CRT_CONTROL|B_FRAME_BUFFER_CONTROL|B_PARALLEL_BUFFER_ACCESS;
  // Define the order in which the 4 channels are stored in video memory in 32
  // bits depth.
#if defined(__INTEL__)	/* FIXME: This should probably use <endian.h> for the right define */
		{ char *c = GCI->rgba_order;
		*c++ = 'b'; *c++ = 'g'; *c++ = 'r'; *c = 'a';
		}
#else
		*((long*)&(GCI->rgba_order)) = (long)('bgra');
#endif
  // Version of the add-on. The new version support by the release 8 is 2.
		GCI->version = 2;
  // Id reserved for internal use by combined library/add-on in future version.
  // The application server doesn't use that at all.
		GCI->id = 0;
		break;
		
// The aplication use a set of hooks function to accelerate some part of the
// user interface. This function copy the links to the available abilities of
// this add-on, depending of the current space setting.
    case B_GET_GRAPHICS_CARD_HOOKS :
	
		for (i=0;i<B_HOOK_COUNT;i++)
			((graphics_card_hook*)buf)[i] = vga_dispatch_virge[i];
		break;

// This is the first command a add-on will ever get. The aim is to know if this
// add-on is able to drive the card described by the application server.
	case B_OPEN_GRAPHICS_CARD :
		{
		set_dprintf_enabled(true); 
#define GCS ((graphics_card_spec*)buf)
  // All PCI chips provide the vendor Id of the graphic chip. First, we need to
  // verify we're speaking to a S3 ViRGE (for distract people, 0x5333 = 'S3').
		if (GCS->vendor_id != 0x5333) {
			err = B_ERROR;
			break;
		} else {
			switch (GCS->device_id) {
				case 0x5631:	// vanilla ViRGE
				case 0x8a01:	// ViRGE DX
					break;
				default:
					err = B_ERROR;
					break;
			}
			if (err == B_ERROR)
				break;
		}

		// Find the card we were asked to open.  It could be one of many in the machine.
		int index = 0;
		bool found = false;
		while (get_nth_pci_info(index, &ci.pcii) != B_ERROR) {
			if ((ci.pcii.vendor_id == GCS->vendor_id) &&
				(ci.pcii.device_id == GCS->device_id) &&
				(ci.pcii.u.h0.base_registers[0] == (ulong)(GCS->screen_base))) {
				found = true;
				break;
			}
			index++;
		}
		
		// bail on failure
		if (found == false) {
		//	dprintf("Couldn't find a ViRGE!\n");
			err = B_ERROR;
			break;
		}
		// this rate MUST be initialized :-)
		ci.scrnRate = 60.1;
		ci.hotpt_v = ci.hotpt_h = -1000;
	//	dprintf("ViRGE found at pci index %d\n", index);
#if 0
		dprintf("base_registers[0] is 0x%08x\n", ci.pcii.u.h0.base_registers[0]);
		dprintf("base_registers_pci[0] is 0x%08x\n", ci.pcii.u.h0.base_registers_pci[0]);
		dprintf("base_register_sizes[0] is 0x%08x\n", ci.pcii.u.h0.base_register_sizes[0]);
		dprintf("ROMBASE: 0x%08x\n", ci.pcii.u.h0.rom_base);
		dprintf("ROMBASE (pci): 0x%08x\n", ci.pcii.u.h0.rom_base_pci);
		dprintf("ROMBASE size: 0x%08x\n", ci.pcii.u.h0.rom_size);
#endif
		// This is the pointer for memory mapped IO
#ifdef __INTEL__	/* FIXME: This should probably use <endian.h> for the right define */
		ci.base0 = (vuchar *)(GCS->screen_base) + 0x01000000;	// little endian area
#else
		ci.base0 = (vuchar *)(GCS->screen_base) + 0x03000000;	// big endian area
		// and the pointer for isa_io
		ci.isa_io = (vuchar *)GCS->io_base;
		dprintf("ci.base0 is 0x%08x\n", ci.base0);
#endif

  // This is the pointer to the beginning of the video memory, as mapped in
  // the add-on memory adress space. This add-on puts the frame buffer just at
  // the beginning of the video memory.
		ci.scrnBase = (uchar *)GCS->screen_base;

// Initialize the s3 chip in a reasonable configuration. Turn the chip on and
// unlock all the registers we need to access (and even more...). Do a few
// standard vga config. Then we're ready to identify the DAC.

		/*
		I don't know why we need to access this through the isa_io, but we seem to need to.
		If anybody knows a way around it, feel free to fix it.
		*/
		set_pci(0x04, 4, 0x02000003); // enable ISA IO, enable MemMapped IO
		isa_outb(VGA_ENABLE, 0x01); // pg 13-1
		dprintf("VGA_ENABLE: 0x%02x\n", (int)isa_inb(VGA_ENABLE));
		outb(VGA_ENABLE, 0x01); // pg 13-1
		dprintf("VGA_ENABLE: 0x%02x\n", (int)inb(VGA_ENABLE));
		isa_outb(MISC_OUT_W, 0x0f);
		dprintf("MISC_OUT_R: 0x%02x\n", (int)isa_inb(MISC_OUT_R));
		outb(MISC_OUT_W, 0x0f);
		dprintf("MISC_OUT_R: 0x%02x\n", (int)inb(MISC_OUT_R));
#if 1
		vuchar t = isa_inb(MISC_OUT_R); // pg 13-1
		t = inb(MISC_OUT_R); // pg 13-1
		isa_outb(DAC_ADR_MASK, 0xff);
		outb(DAC_ADR_MASK, 0xff);
		isa_outb(SEQ_INDEX, 0x08); // pg 13-2 -- unlock extended Sequencer regs
		isa_outb(SEQ_DATA,  0x06);
		outb(SEQ_INDEX, 0x08); // pg 13-2 -- unlock extended Sequencer regs
		outb(SEQ_DATA,  0x06);
		isa_outb(CRTC_INDEX, 0x38); // pg 13-2 -- unlock extended CRTC 2D->3F
		isa_outb(CRTC_DATA,  0x48);
		outb(CRTC_INDEX, 0x38); // pg 13-2 -- unlock extended CRTC 2D->3F
		outb(CRTC_DATA,  0x48);
		isa_outb(CRTC_INDEX, 0x39); // pg 13-2 -- unlock extended CRTC 40->FF
		isa_outb(CRTC_DATA,  0xa5);
		outb(CRTC_INDEX, 0x39); // pg 13-2 -- unlock extended CRTC 40->FF
		outb(CRTC_DATA,  0xa5);
		isa_outb(CRTC_INDEX, 0x40); // pg 13-2 -- enable access to enhanced programming regs
		t = isa_inb(CRTC_DATA);
		isa_outb(CRTC_DATA,  t | 0x01);
		outb(CRTC_INDEX, 0x40); // pg 13-2 -- enable access to enhanced programming regs
		t = inb(CRTC_DATA);
		outb(CRTC_DATA,  t | 0x01);
		isa_outb(CRTC_INDEX, 0x53);	// Extended memory control 1
		isa_outb(CRTC_DATA,  0x08);
		outb(CRTC_INDEX, 0x53);	// Extended memory control 1
		outb(CRTC_DATA,  0x08);
#else
#endif
		//outb(VGA_ENABLE,1);
		
		// We've got the ViRGE they want, now lets put it in a REASONABLE mode.
		dprintf("Was PCI 0x04: 0x%08x\n", get_pci(0x04, 4));
		set_pci(0x04, 4, 0x02000002); // disable ISA IO, enable MemMapped IO
		dprintf("Now PCI 0x04: 0x%08x\n", get_pci(0x04, 4));
#if 0
		dprintf("Was PCI 0x30: 0x%08x\n", get_pci(0x30, 4));
		set_pci(0x30, 4, 0x00000000); // disable the BIOS
		dprintf("Now PCI 0x30: 0x%08x\n", get_pci(0x30, 4));
#endif

#if 1
		outb(CRTC_INDEX, 0x2d);
		dprintf("Chip ID Hi: 0x%02x\n", (int)inb(CRTC_DATA));
		outb(CRTC_INDEX, 0x2e);
		dprintf("Chip ID Lo: 0x%02x\n", (int)inb(CRTC_DATA));
		outb(CRTC_INDEX, 0x2f);
		dprintf("Chip Revis: 0x%02x\n", (int)inb(CRTC_DATA));
		outb(CRTC_INDEX, 0x59);
		dprintf("CR59: 0x%02x\n", (int)inb(CRTC_DATA));
		outb(CRTC_INDEX, 0x5a);
		dprintf("CR5A: 0x%02x\n", (int)inb(CRTC_DATA));
		outb(CRTC_INDEX, 0x31);
		dprintf("CR31: 0x%02x\n", (int)inb(CRTC_DATA));
		outb(CRTC_INDEX, 0x32);
		dprintf("CR32: 0x%02x\n", (int)inb(CRTC_DATA));
		outb(CRTC_INDEX, 0x35);
		dprintf("CR35: 0x%02x\n", (int)inb(CRTC_DATA));
		outb(CRTC_INDEX, 0x36);
		dprintf("CR36: 0x%02x\n", (int)inb(CRTC_DATA));
		outb(CRTC_INDEX, 0x37);
		dprintf("CR37: 0x%02x\n", (int)inb(CRTC_DATA));
		outb(CRTC_INDEX, 0x68);
		dprintf("CR68: 0x%02x\n", (int)inb(CRTC_DATA));
		outb(CRTC_INDEX, 0x6f);
		dprintf("CR6F: 0x%02x\n", (int)inb(CRTC_DATA));
#endif


// Set the graphic engine in enhanced mode
#if 1
		ulong afc = inl(ADVFUNC_CNTL);
		dprintf("ADVFUNC_CNTL: 0x%08x\n", afc);
		outl(ADVFUNC_CNTL, 0x00000013);
		outl(ADVFUNC_CNTL, 0x00000011);
		afc = inl(ADVFUNC_CNTL);
		dprintf("ADVFUNC_CNTL: 0x%08x\n", afc);
		dprintf("outl(SUBSYS_CNTL, 0x007f)\n");
		outl(SUBSYS_CNTL, 0x0000807f);
		delay(10000);
		outl(SUBSYS_CNTL, 0x00004000);
#endif

		dprintf("set_table(gcr_table)\n");
		set_table(gcr_table);
		dprintf("set_table(sequencer_table)\n");
		set_table(sequencer_table);
		dprintf("set_attr_table(attribute_table)\n");
		set_attr_table(attribute_table);
		delay(10000);
		//dprintf("set_table(vga_virge_table)\n");
		set_table(vga_virge_table);
		// enforce defaults on DX/GX chipsets
		// fixes "display mirroring and probaby some "sparkle" problems
		if (GCS->device_id == 0x8a01) {
			//delay(10000);
			dprintf("set_table(vga_virge_dx_gx_table)\n");
			set_table(vga_virge_dx_gx_table);
		}

// Arrived at that point, we now know we're able to drive that card. So we need
// to initialise what we need here, especially the benaphore protection that
// will allow concurent use of the add-on.
		init_locks();

// We finally need to test the size of the available memory, and we can test
// the available writing bandwidth, just for information.

		vid_checkmem();

		ci.scrnBufBase = (uchar *)GCS->screen_base + (ci.theMem * 1024) - 1024;
		dprintf("   scrnBase: 0x%08x\nscrnBufBase: 0x%08x\n", ci.scrnBase, ci.scrnBufBase);
		v_show_cursor(false);
		}
		break;

// When the add-on is not able to drive any of the available cards, he will be
// unload soon. But first, this command will be call. As the application server
// never stop, except by crash or at reboot, this call will never be used to
// close an active add-on. Nevertheless, it's better to free here all the
// stuff you could be using.
	case B_CLOSE_GRAPHICS_CARD :
	
		// Blank the screen. The add-on prefers dying in the darkness...
		SCREEN_OFF;
		// Free the benaphore ressources (if necessary).
		dispose_locks();
		break;

// That command set an entry of the color_map in 8 bits depth (see documentation
// for more informations).
	case B_SET_INDEXED_COLOR:
#define IC ((indexed_color*)buf)	

// This call is using only the io-register (not the graphic engine). So we need
// to get the lock io-register benaphore.
		lock_io();

// ##----> THOSE SETTINGS SHOULD BE COMPATIBLE WITH ALL VGA CHIPS. <----##
// Set the selected entry of the color table of the DAC. This code use only
// standard vga registers
		outb(DAC_ADR_MASK, 0xff);
		outb(DAC_WR_INDEX, IC->index);
		
		outb(DAC_DATA, (IC->color.red) >> 2);
		outb(DAC_DATA, (IC->color.green) >> 2);
		outb(DAC_DATA, (IC->color.blue) >> 2);

// Release the benaphore of the io-registers.
		unlock_io();
		break;

// this definition may be dangerous
#define GCC	((graphics_card_config*)buf)	

// This command is used by the application server to know what spaces are
// available on the card drive by this add-on.
	case B_GET_SCREEN_SPACES:
	
// All the work has already be done by vid_checkmem.
		GCC->space = ci.available_spaces;
		break;

// This command is called to change any parameters of the current configuration.
// The add-on is supposed to check itself what parameters have changed since
// the previous, and then to change the card settings as smoothly as possible.
	case B_CONFIG_GRAPHICS_CARD:
// First, check that the selected space is available
		if ((GCC->space & ci.available_spaces) == 0) {
			err = B_ERROR;
			break;
		}

// Change the CRT_CONTROL parameters (they've to be reset everytime).
		ci.crtPosH = GCC->h_position;
		ci.crtSizeH = GCC->h_size;
		ci.crtPosV = GCC->v_position;
		ci.crtSizeV = GCC->v_size;

// If the space has changed, we need to completly reconfigure the card.
		if (GCC->space != ci.scrnResCode) {
  // Modify the space setting
			ci.scrnResCode = GCC->space;
			ci.scrnResNum = 0;
			code = ci.scrnResCode;
			while (code > 1) {
				ci.scrnResNum++;
				code >>= 1;
		    }
  // Change the refresh rate (this parameter has to be reset everytime).
			ci.scrnRate = GCC->refresh_rate;
			if (ci.scrnRate < vga_min_rates[ci.scrnResNum])
				ci.scrnRate = vga_min_rates[ci.scrnResNum];
			if (ci.scrnRate > vga_max_rates[ci.scrnResNum])
				ci.scrnRate = vga_max_rates[ci.scrnResNum];
  // Do a full configuration and change the description of the current frame
  // buffer.
			switch (ci.scrnResCode) {
			case B_8_BIT_640x480:
			case B_16_BIT_640x480:
				ci.scrnRes = vga640x480;
				ci.scrnWidth = 640;
				ci.scrnHeight = 480;
				ci.scrnRowByte = 640;
			set_res:
	  			if (ci.scrnResCode & (
	  				B_8_BIT_640x480 | B_8_BIT_800x600 | B_8_BIT_1024x768 | 
	  				B_8_BIT_1152x900 | B_8_BIT_1280x1024 | B_8_BIT_1600x1200)) ci.scrnColors = 8;
	  			else {
	  				ci.scrnColors = 16;
	  				ci.scrnRowByte <<= 1;
	  			};
				ci.offscrnWidth = ci.scrnWidth;
				ci.offscrnHeight = ci.scrnHeight;
				ci.scrnPosH = 0;
				ci.scrnPosV = 0;
				//dprintf("Setting mode: %dx%dx%d@%f\n", ci.scrnWidth, ci.scrnHeight, ci.scrnColors, ci.scrnRate);
				vid_selectmode(TRUE);
				break;
			case B_8_BIT_800x600:
			case B_16_BIT_800x600:
				ci.scrnRes = vga800x600;
				ci.scrnWidth = 800;
				ci.scrnHeight = 600;
				ci.scrnRowByte = 800;
				goto set_res;
			case B_8_BIT_1024x768:
			case B_16_BIT_1024x768:
				ci.scrnRes = vga1024x768;
				ci.scrnWidth = 1024;
				ci.scrnHeight = 768;
				ci.scrnRowByte = 1024;
				goto set_res;
			case B_8_BIT_1152x900:
			case B_16_BIT_1152x900:
				ci.scrnRes = vga1152x900;
				ci.scrnWidth = 1152;
				ci.scrnHeight = 900;
				ci.scrnRowByte = 1152;
				goto set_res;
			case B_8_BIT_1280x1024:
			case B_16_BIT_1280x1024:
				ci.scrnRes = vga1280x1024;
				ci.scrnWidth = 1280;
				ci.scrnHeight = 1024;
				ci.scrnRowByte = 1280;
				goto set_res;
			case B_8_BIT_1600x1200:
			case B_16_BIT_1600x1200:
				ci.scrnRes = vga1600x1200;
				ci.scrnWidth = 1600;
				ci.scrnHeight = 1200;
				ci.scrnRowByte = 1600;
				goto set_res;
  // Space unsupported (this protection is not necessary)
			default :
				err = B_ERROR;
			}
		}

// When the space is unchanged, we just need to reset the clock frenquency and
// the crt settings.
		else {
  // Change the refresh rate (this parameter has to be reset everytime).
			ci.scrnRate = GCC->refresh_rate;
			if (ci.scrnRate < vga_min_rates[ci.scrnResNum])
				ci.scrnRate = vga_min_rates[ci.scrnResNum];
			if (ci.scrnRate > vga_max_rates[ci.scrnResNum])
				ci.scrnRate = vga_max_rates[ci.scrnResNum];
  // Reset the refresh rate and the crt settings
			//dprintf("Setting refresh rate: %f\n", ci.scrnRate);
		    vid_selectrate();
		}
		break;

// The refresh rate limits are under control of the add-on. The application
// is free to ask for any refresh rate, but the add-on is free to selected only
// allowed values. To limit misunderstanding, the application server uses that
// function to ask the available refresh rate range for a given space, and the
// real current setting as it doesn't know if what it asked for has been done.
	case B_GET_REFRESH_RATES :
#define RRI ((refresh_rate_info*)buf)	
		RRI->min = vga_min_rates[ci.scrnResNum];
		RRI->max = vga_max_rates[ci.scrnResNum];
		RRI->current = ci.scrnRate;
		break;

// Give the size of the buffer needed to discribe the current internal addon setting
	case B_GET_INFO_FOR_CLONE_SIZE :
	
		*((long*)buf) = sizeof(clone_info);
		break;

// Send back a copy of the current internal addon setting
	case B_GET_INFO_FOR_CLONE :
		*((clone_info*)buf) = ci;
		break;

// Update the current internal addon settings
	case B_SET_CLONED_GRAPHICS_CARD :
		ci = *((clone_info*)buf);
		init_locks();
		break;

// Close an cloned addon.
	case B_CLOSE_CLONED_GRAPHICS_CARD :

		dispose_locks();
		break;

// Ask for a set of (Depth, Width) what are the rowByte and the maximal height.
	case B_PROPOSE_FRAME_BUFFER :
		{
			long   col, mem, row;
#define FBI	((frame_buffer_info*)buf)
		// default return values in case of failure
			FBI->bytes_per_row = -1;
			FBI->height = -1;
		// check that the depth ask for is available
			col = FBI->bits_per_pixel;
			row = FBI->width;
			if ((col != 8) && (col != 16)) {
				err = B_ERROR;
				break;
			}
		// find the smaller usable rowByte compatble with the hardware and
		// bigger than the selected width.
			if (row <= 640) row = 640;
			else if (row <= 800) row = 800;
			else if (row <= 1024) row = 1024;
			else if (row <= 1152) row = 1152;
			else if (row <= 1280) row = 1280;
			else if (row <= 1600) row = 1600;
			else {
				err = B_ERROR;
				break;
			}
			FBI->bytes_per_row = row * (col / 8);
		// calculate how many lines can be used with that rowByte
			mem = ci.theMem*1024L;
			mem /= FBI->bytes_per_row;
			//if (mem > 2048) mem = 2048;
			FBI->height = mem;
		}
		break;

// Set frame rectangle size
	case B_SET_FRAME_BUFFER :
		{
			long    col, fw, fh, row, x, y, dw, dh;
			
			col = FBI->bits_per_pixel;
			fw = FBI->width;
			fh = FBI->height;
			dw = FBI->display_width;
			dh = FBI->display_height;
			row = FBI->bytes_per_row;
			x = FBI->display_x;
			y = FBI->display_y;

		// check safety conditions between the values of the
		// different parameters.
			if ((col != 8) && (col != 16))
				goto bad_setting;
			if ((fw > (row/(col/8))) || (dw > fw) || (dh > fh))
				goto bad_setting;
			if (((row/(col/8)) != 640) && ((row/(col/8)) != 800) && ((row/(col/8)) != 1024) &&
				((row/(col/8)) != 1152) && ((row/(col/8)) != 1280) && ((row/(col/8)) != 1600))
				goto bad_setting;
			if ((row*fh) > (ci.theMem*8192L))
				goto bad_setting;
			if ((x < 0) || (y < 0) || (y+dh > fh) || (x+dw > fw))
				goto bad_setting;
			
			ci.scrnRes = vga_specific;
			ci.scrnResCode = (ulong)-1;
			ci.scrnColors = col;
			ci.offscrnWidth = fw;
			ci.offscrnHeight = fh;
			ci.scrnWidth = dw;
			ci.scrnHeight = dh;
			ci.scrnRowByte = row;
			ci.scrnPosH = x;
			ci.scrnPosV = y;
			vid_selectmode(TRUE);
			break;
			
		bad_setting:
			err = B_ERROR;
			break;
		}

// Set display rectangle size
	case B_SET_DISPLAY_AREA :
		{
			long    x, y, dw, dh;
			
			dw = FBI->display_width;
			dh = FBI->display_height;
			x = FBI->display_x;
			y = FBI->display_y;
			
		// check safety conditions between the values of the
		// different parameters.
			if ((dw > ci.offscrnWidth) || (dh > ci.offscrnHeight) ||
				(x < 0) || (y < 0) ||
				(y+dh > ci.offscrnHeight) || (x+dw > ci.offscrnWidth)) {
				err = B_ERROR;
				break;
			}
			
			ci.scrnWidth = dw;
			ci.scrnHeight = dh;
			ci.scrnPosH = x;
			ci.scrnPosV = y;
		    vid_selectrate();
			break;
		}

// Set the origin of the display rectangle in the frame rectangle
	case B_MOVE_DISPLAY_AREA :
		{
			long    x, y;
			
			x = FBI->display_x;
			y = FBI->display_y;
			
		// check safety conditions between the values of the
		// different parameters.
			if ((x+ci.scrnWidth > ci.offscrnWidth) || (x < 0) ||
				(y+ci.scrnHeight > ci.offscrnHeight) || (y < 0)) {
				err = B_ERROR;
				break;
			}
			
			ci.scrnPosH = x;
			ci.scrnPosV = y;
		    vid_selectpos();
			break;
		}

// Any unknown command has to return B_ERROR for future compatibility.
	default :
		
		err = B_ERROR;
		break;
	}
	return err;
}
