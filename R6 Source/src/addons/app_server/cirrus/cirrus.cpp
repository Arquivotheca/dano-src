//*****************************************************************************
//
//	File:		 cirrus.cpp
//
//	Description: add-on for graphics cards using cirrus chips.
//
//	Copyright 1996, Be Incorporated
//
//*****************************************************************************

// This include just provides prototypes for the few kernel calls usable in the
// add-on (see documentation on graphics add-on development for more infos).
// This add-on has to be linked with a specific library (libkernel.a).
#include <OS.h>
#include <Debug.h>
#include <endian.h>

// Graphics card add-ons standard structs and prototypes
#include <GraphicsCard.h>

// Specific stuff for that add-on
#include "cirrus.h"
#include "priv_syscalls.h"

#define dprintf _sPrintf

void DoSelectMode(bool mode);

//**************************************************************************
//	Chip's available abilities description
//**************************************************************************

// This table describes the available spaces for each chip (without considering
// the available amount of ram on the card).
static ulong vga_res_available[CHIP_COUNT] = {
  // alpine 5434-36
	B_8_BIT_640x480|B_8_BIT_800x600|B_8_BIT_1024x768|B_8_BIT_1152x900|B_8_BIT_1280x1024|
	B_32_BIT_640x480|B_32_BIT_800x600,
  // alpine 5430-40
	B_8_BIT_640x480|B_8_BIT_800x600|B_8_BIT_1024x768,
};

// This table describes the available specific abilities for each chip (this
// is the value return in "flags" in the "graphics_card_info" struct).
static ushort vga_flags[CHIP_COUNT] = {
  // alpine 5434-36
	B_CRT_CONTROL|B_FRAME_BUFFER_CONTROL|B_LAME_ASS_CARD,
  // alpine 5430-40
	B_CRT_CONTROL|B_FRAME_BUFFER_CONTROL|B_LAME_ASS_CARD
};



//**************************************************************************
//	Private globals of the cirrus add-on.
//**************************************************************************

// Id of the vga chip used by the current card (see cirrus.h for codes)
static int              theVGA;

// Available memory (in KB). If the card has more memory than what this add-on
// is able to use, extra memory is ignored.
static int              theMem;

// Offset (in bytes) between the beginning of two lines of the frame buffer.
static int              scrnRowByte;

// Width of the frame buffer (in pixels)
static int		        scrnWidth;

// Height of the frame buffer (in lines)
static int		        scrnHeight;

// Width of the offscreen buffer (in pixels)
static int              offscrnWidth;

// Height of the offscreen buffer (in pixels)
static int              offscrnHeight;

// Offset H and V of the screen in the offscreen (in pixels)
static int              scrnPosH = 0;
static int              scrnPosV = 0;

// Depth of the screen (in bits per pixel)
static int		        scrnColors = 8;

// Base adress of the frame buffer, as mapped in add-on memory space.
static void	            *scrnBase;

// Current refresh rate (in Hertz)
static float            scrnRate = 60.1;

// Current CRT settings (horizontal and vertical positions and sizes).
// See documentation for the meaning of those values.
static short            crtPosH = 50;
static short            crtSizeH = 50;
static short            crtPosV = 50;
static short            crtSizeV = 50;

// Current space code (as described in GraphicsCard.h)
static ulong            scrnResCode = ~0L;

// Index of the current ResCode (index = log2(ResCode)).
static int              scrnResNum;

// Base adress of the cursor shape buffer, as mapped in add-on memory space. 
static uchar            *scrnBufBase;

// Internal code for resolution. 
static long		        scrnRes = vga640x480;

// Mask of available spaces with the current card.
static ulong            available_spaces;

// Position of the hot-spot in the current cursor shape.
static long             hotpt_h = -1000;
static long             hotpt_v = -1000;

// Last settings of the horizontal and vertical totals for CRT registers
static short            lastCrtHT,lastCrtVT;

// Base address of the 64K io-space for chip's registers, as mapped in add-on
// memory space.
static volatile uchar   *isa_IO;



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
	0xff,	0xff    // End-of-table
};

// This table is used for first initialization of the cirrus alpine chip.
// The first value (16 bits) is the adress of the register in the 64K io-space,
// the second (only one significative byte) is the value we set.
// Two pairs are often put on the same line, because many registers use indirect
// access (one register with many meanings, indexed by another register). The
// index always comes first.
static ushort	std_cirrus_vga_table[] =
{
	0x46e8, 0x16,
	0x102,  0x01,
	0x46e8, 0x0e,
	0x3c4,  0x06,
	0x3c5,  0x0f,
	0x3c2,  0x0b,
	0x3c4,  0x06,
	0x3c5,  0x12,
	
	0x3c4,	0x00,	0x3c5,	0x01,	// Reset (not needed)
	0x3c2,	0x2f,	                // Miscellaneous output
	0x3da,	0x00,	                // Feature Control

	0x000,	0x00
};

static ushort	vga_cirrus_table[] =
{
	0x3c4,	0x06,	0x3c5,	0x0F,	// lock registers	
	0x3c2,	0x03,					// clock 0, color, cpu access to videoram ok
	0x3c4,	0x06,	0x3c5,	0x12,	// unlock registers	
	0x3d4,	0x17,	0x3d5,	0x2b,	// CRTC Mode Control
// clock settings
	0x3c4,	0x1f,	0x3c5,	0xa2,	// memory clock = 60.8 Mhz

	0x3ce,	0x05,	0x3cf,	0x40,	// 256 colors
	0x3ce,	0x06,	0x3cf,	0x01,	// graphic mode (not text)
	0x3ce,	0x08,	0x3cf,	0xff,	// write mask disabled
	0x3c0,	0x10,	0x3c0,	0x01,	// graphic mode (not text)
	0x3c4,	0x08,	0x3c5,	0x00,	// EEPROM control
	0x3c4,	0x10,	0x3c5,	0x00,	// graphic cursor X position
	0x3c4,	0x11,	0x3c5,	0x00,	// graphic cursor Y position
	0x3c4,	0x12,	0x3c5,	0x00,	// graphic cursor control (disabled)
	0x3c4,	0x13,	0x3c5,	0x00,	// graphic cursor index selection
	0x3c4,	0x18,	0x3c5,	0x00,	// Signature generator disabled
	0x3ce,	0x09,	0x3cf,	0x00,	// offset register 0
	0x3ce,	0x0a,	0x3cf,	0x00,	// offset regsiter 1
	0x3ce,	0x0b,	0x3cf,	0x20,	// extended write mode disabled (00)
	0x3ce,	0x0e,	0x3cf,	0x00,	// DCLK output divide by 2 off
	0x3ce,	0x30,	0x3cf,	0x00,	// BitBLT mode
	0x000,  0x00
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
	0x3ce,	0x00,	0x3cf,	0x00,	// Set/Reset
	0x3ce,	0x01,	0x3cf,	0x00,	// Enable Set/Reset
	0x3ce,	0x02,	0x3cf,	0x00,	// Color Compare
	0x3ce,	0x03,	0x3cf,	0x00,	// Raster Operations/Rotate Counter
	0x3ce,	0x04,	0x3cf,	0x00,	// Read Plane Select
	0x3ce,	0x05,	0x3cf,	0x40,	// Graphics Controller
	0x3ce,	0x06,	0x3cf,	0x05,	// Memory Map Mode
	0x3ce,	0x07,	0x3cf,	0x0f,	// Color Don't care
	0x3ce,	0x08,	0x3cf,	0xff,	// Bit Mask
	0x000,	0x00
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
	0x3c4,	0x00,	0x3c5,	0x03,	// Reset
	0x3c4,	0x01,	0x3c5,	0x03,	// Clocking mode
	0x3c4,	0x02,	0x3c5,	0x0f,	// Enable write plane
	0x3c4,	0x03,	0x3c5,	0x00,	// Char Font Select
	0x3c4,	0x04,	0x3c5,	0x0e,	// Memory Mode
	0x000,	0x00
};

//**************************************************************************
//  Allowed refresh rate ranges, for each space (in Hertz).
//**************************************************************************

// Minimal values. First line is 8 bits spaces, second 16 bits spaces,
// third 32 bits spaces. Resolutions are columns 1 to 5 : 640x480, 800x600,
// 1024x768, 1280x1024 and 1600x1200.
static float vga_min_rates[32] = {
	56.0, 56.0, 56.0, 56.0, 56.0,
	60.0, 60.0, 60.0, 60.0, 60.0,
	56.0, 56.0, 60.0, 60.0, 60.0,
	56.0, 60.0, 60.0, 60.0, 60.0,
	60.0, 60.0, 60.0, 60.0, 60.0,
	60.0, 60.0, 60.0, 60.0, 60.0,
	60.0, 60.0, 
};

// Maximal values. Same order as minimal values. Be careful with those values.
// High frequency are dangerous for monitors, especially old ones.
static float vga_max_rates[32] = {
	90.0, 90.0, 84.0, 75.0, 70.0,
	60.0, 60.0, 60.0, 60.0, 60.0,
	90.0, 80.0, 60.0, 60.0, 60.0,
	75.0, 60.0, 60.0, 60.0, 60.0,
	60.0, 60.0, 60.0, 60.0, 60.0,
	60.0, 60.0, 60.0, 60.0, 60.0,
	60.0, 60.0,
};



//**************************************************************************
//  GraphicsCard Hook Table declaration.
//**************************************************************************

// Prototypes for hook table (see documentation for more informations).
long    set_cursor_shape(uchar *data,uchar *mask,long dim_h,long dim_v,long hot_h,long hot_v);
long    move_cursor(long new_h,long new_v);
long    show_cursor(bool state);
long 	rect_8(long x1, long y1, long x2, long y2,uchar color);
long 	rect_32(long x1, long y1, long x2, long y2,ulong color);
#if INTEL_STATIC_DRIVERS
static
#endif
long 	blit(long x1, long y1, long x2, long y2, long width, long height);
long    invert_rect_32(long x1, long y1, long x2, long y2);

// Hook table in the standard case.
static graphics_card_hook vga_dispatch[B_HOOK_COUNT] = {
// 0
	(graphics_card_hook)set_cursor_shape,
	(graphics_card_hook)move_cursor,
	(graphics_card_hook)show_cursor,
	0L,
	0L,
// 5
	(graphics_card_hook)rect_8,
	(graphics_card_hook)rect_32,
	(graphics_card_hook)blit,
	0L,
	0L,
// 10
	0L,
	(graphics_card_hook)invert_rect_32,
	0L,
	0L,
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
	0L,
	0L,
	0L,
	0L,
	0L,
// 45
	0L,
	0L,
	0L,
};



extern "C" int		write_isa_io (int pci_bus, void *addr, int size, uint32 val);
extern "C" int		read_isa_io (int pci_bus, void *addr, int size);

//**************************************************************************
//  Standard input/output functions to access io regsters
//**************************************************************************

// Write a value in a 8 bits register at a specified address
static void	outp(long address, uchar value)
{
#if __INTEL__
	write_isa_io (0, (char *)address, 1, value);
#else
	*(vuchar *)ISA_ADDRESS(address) = value;
#endif
}

// Write a value in a 16 bits register at a specified address
// (respecting intel byte order).
static void	outpw(long address, ushort value)
{
	ushort	tmp;

#if __INTEL__
	write_isa_io (0, (char *)address, 2, value);
#else
	tmp = ((value & 0xff) << 8) | ((value >> 8) & 0xff);
	*(vushort *)ISA_ADDRESS(address) = tmp;
#endif
}

// Write a value in a 32 bits register at a specified address.
// (don't respect intel byte order, only for memory to register copy)
static void	outpl(long address, ulong value)
{
#if __INTEL__
	write_isa_io (0, (char *)address, 4, value);
#else
	*(vulong *)ISA_ADDRESS(address) = value;
#endif
}

// Read the current value of a 8 bits register at a specified address
static uchar	inp(long address)
{
#if __INTEL__
	return (uchar) read_isa_io (0, (char *)address, 1);
#else
	return(*(vuchar *)ISA_ADDRESS(address));
#endif	
}

// Read the current value of a 16 bits register at a specified adress
// (respecting intel byte order).
static ushort	inpw(long address)
{
	ushort	value;

#if __INTEL__
	return (ushort) read_isa_io (0, (char *)address, 2);
#else
	value = *(vushort *)ISA_ADDRESS (address);
	return(((value & 0xff) << 8) | ((value >> 8) & 0xff));
#endif	
}



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
static long	    io_lock,ge_lock;

// Create the benaphores (if possible)
void init_locks()
{
	if (io_sem == -1) {
		io_lock = 0L;
		io_sem = create_sem(0,"vga io sem");
		ge_lock = 0L;
		ge_sem = create_sem(0,"vga ge sem");
	}
}

// Free the benaphore (if created)
void dispose_locks()
{
	if (ge_sem >= 0)
		delete_sem(ge_sem);
	if (io_sem >= 0)
		delete_sem(io_sem);
}

// Protect the access to the general io-registers by locking a benaphore.
void lock_io()
{
	int	old;

	old = atomic_add (&io_lock, 1);
	if (old >= 1) {
		acquire_sem(io_sem);	
	}	
}

// Release the protection on the general io-registers
void unlock_io()
{
	int	old;

	old = atomic_add (&io_lock, -1);
	if (old > 1) {
		release_sem(io_sem);
	}
}	

// Protect the access to the memory or the graphic engine registers by
// locking a benaphore.
void lock_ge()
{
	int	old;

	old = atomic_add (&ge_lock, 1);
	if (old >= 1) {
		acquire_sem(ge_sem);	
	}	
}

// Release the protection on the memory and the graphic engine registers.
void unlock_ge()
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

// Change the bitmap shape of the hardware cursor (see documentation for
// more informations).
long set_cursor_shape(uchar *data,uchar *mask,long dim_h,long dim_v,long hot_h,long hot_v)
{
	uchar   *buf;
	uchar   prev;
	int     i,j,dim_h0;
	int     init;

	lock_io();
	if (hotpt_h == -1000)
		init = 1;
	else
		init = 0;
	hotpt_h = hot_h;
	hotpt_v = hot_v;
	dim_h0 = dim_h>>3;
	buf = (uchar*)scrnBase+1024*theMem-256;

	for (i=0;i<256;i++)
		buf[i] = 0x00;
	
	for (i=0;i<dim_v;i++)
		for (j=0;j<dim_h0;j++) {
			buf[0+i*4+j] = data[j+i*dim_h0];
			buf[128+i*4+j] = mask[j+i*dim_h0]^0xff;
		}

	/* set cursor data pointer */
	outp(0x3c4,0x12);
	prev = inp(0x3c5)&1;
	outp(0x3c5,0x02|prev);
	
	outp(0x3c8,0x0f);
	outp(0x3c9,0x00);
	outp(0x3c9,0x00);
	outp(0x3c9,0x00);
			
	outp(0x3c8,0x00);
	outp(0x3c9,0xff);
	outp(0x3c9,0xff);
	outp(0x3c9,0xff);

	if (init) {
		outp(0x3c4,0x10);
		outp(0x3c5,0);
		outp(0x3c4,0x11);
		outp(0x3c5,0);
		outp(0x3c4,0x12);
		outp(0x3c5,0x00);
	}
	outp(0x3c4,0x13);
	outp(0x3c5,0x3f);
	outp(0x3c4,0x12);
	outp(0x3c5,prev);
	unlock_io();
	
	return B_NO_ERROR;
}

// Move the hardware cursor to a new position of the hot_spot (see documentation
// for more informations).
long move_cursor(long new_h,long new_v)
{
	long h,v;

	lock_io();
	h = new_h-hotpt_h;
	v = new_v-hotpt_v;
	if (h < 0) h = 0;
	if (v < 0) v = 0;

	outp(0x3c4,0x10+((h<<5)&0xe0));
	outp(0x3c5,(h>>3)&255);
	outp(0x3c4,0x11+((v<<5)&0xe0));
	outp(0x3c5,(v>>3)&255);
	unlock_io();
	
	return B_NO_ERROR;
}

// This function is used to show or hide the hardware cursor (see documentation
// for more informations).
long show_cursor(bool state)
{
	lock_io();
	if (state == 0) {
		outp(0x3c4,0x12);
		outp(0x3c5,0x00);
	}
	else {
		outp(0x3c4,0x12);
		outp(0x3c5,0x01);
	}
	unlock_io();
	return B_NO_ERROR;
}

// Fill a rect in 8 bits (see documentation for more informations)
long rect_8(long x1, long y1, long x2, long y2, uchar color)
{
	long     y,adr1,height,width;
	char     oper;

	lock_ge();
	outp(0x3ce,0x31);
	outp(0x3cf,4);
	
	adr1 = y1*scrnRowByte+x1;
	height = y2-y1;
	width = x2-x1;
	
	outp(0x3ce,0x00);
	outp(0x3cf,color);

	outp(0x3ce,0x01);
	outp(0x3cf,color);

	outp(0x3ce,0x20);
	outp(0x3cf,width&255);
	outp(0x3ce,0x21);
	outp(0x3cf,(width>>8)&255);

	outp(0x3ce,0x22);
	outp(0x3cf,height&255);
	outp(0x3ce,0x23);
	outp(0x3cf,(height>>8)&255);

	outp(0x3ce,0x24);
	outp(0x3cf,scrnRowByte&255);
	outp(0x3ce,0x25);
	outp(0x3cf,(scrnRowByte>>8)&255);

	outp(0x3ce,0x26);
	outp(0x3cf,0);
	outp(0x3ce,0x27);
	outp(0x3cf,0);

	outp(0x3ce,0x28);
	outp(0x3cf,adr1&255);
	outp(0x3ce,0x29);
	outp(0x3cf,(adr1>>8)&255);
	outp(0x3ce,0x2a);
	outp(0x3cf,(adr1>>16)&255);

	outp(0x3ce,0x2c);
	outp(0x3cf,(long)scrnBase&255);
	outp(0x3ce,0x2d);
	outp(0x3cf,((long)scrnBase>>8)&255);
	outp(0x3ce,0x2e);
	outp(0x3cf,((long)scrnBase>>16)&255);
	
	outp(0x3ce,0x2f);
	outp(0x3cf,0);
	
	outp(0x3ce,0x30);
	outp(0x3cf,0x80);

	outp(0x3ce,0x32);
	outp(0x3cf,0x0d);

	outp(0x3ce,0x31);
	while ((inp(0x3cf) & 8) == 8) {};
	
	outp(0x3ce,0x31);
	outp(0x3cf,2);
	
	outp(0x3ce,0x31);
	while ((inp(0x3cf) & 8) == 8) {};
	unlock_ge();
	
	return B_NO_ERROR;
}

// Fill a rect in 32 bits (see documentation for more informations)
long rect_32(long x1, long y1, long x2, long y2, ulong color)
{
	long     y,adr1,height,width;
	char     oper;

	lock_ge();
	outp(0x3ce,0x31);
	outp(0x3cf,4);
	
	x1 <<= 2;
	x2 = (x2<<2)+3;
	
	adr1 = y1*scrnRowByte+x1;
	height = y2-y1;
	width = x2-x1;
	
	outp(0x3ce,0x00);
	outp(0x3cf,(color>>24)&255);
	outp(0x3ce,0x01);
	outp(0x3cf,(color>>24)&255);
	outp(0x3ce,0x10);
	outp(0x3cf,(color>>16)&255);
	outp(0x3ce,0x11);
	outp(0x3cf,(color>>16)&255);
	outp(0x3ce,0x12);
	outp(0x3cf,(color>>8)&255);
	outp(0x3ce,0x13);
	outp(0x3cf,(color>>8)&255);
	outp(0x3ce,0x14);
	outp(0x3cf,(color>>0)&255);
	outp(0x3ce,0x15);
	outp(0x3cf,(color>>0)&255);

	outp(0x3ce,0x20);
	outp(0x3cf,width&255);
	outp(0x3ce,0x21);
	outp(0x3cf,(width>>8)&255);

	outp(0x3ce,0x22);
	outp(0x3cf,height&255);
	outp(0x3ce,0x23);
	outp(0x3cf,(height>>8)&255);

	outp(0x3ce,0x24);
	outp(0x3cf,scrnRowByte&255);
	outp(0x3ce,0x25);
	outp(0x3cf,(scrnRowByte>>8)&255);

	outp(0x3ce,0x26);
	outp(0x3cf,0);
	outp(0x3ce,0x27);
	outp(0x3cf,0);

	outp(0x3ce,0x28);
	outp(0x3cf,adr1&255);
	outp(0x3ce,0x29);
	outp(0x3cf,(adr1>>8)&255);
	outp(0x3ce,0x2a);
	outp(0x3cf,(adr1>>16)&255);

	outp(0x3ce,0x2c);
	outp(0x3cf,(long)scrnBase&255);
	outp(0x3ce,0x2d);
	outp(0x3cf,((long)scrnBase>>8)&255);
	outp(0x3ce,0x2e);
	outp(0x3cf,((long)scrnBase>>16)&255);
	
	outp(0x3ce,0x2f);
	outp(0x3cf,0);
	
	outp(0x3ce,0x30);
	outp(0x3cf,0xb0);

	outp(0x3ce,0x32);
	outp(0x3cf,0x0d);

	outp(0x3ce,0x31);
	while ((inp(0x3cf) & 8) == 8) {};
	
	outp(0x3ce,0x31);
	outp(0x3cf,2);
	
	outp(0x3ce,0x31);
	while ((inp(0x3cf) & 8) == 8) {};
	unlock_ge();
	
	return B_NO_ERROR;
}


// Blit a rect from screen to screen (see documentation for more informations)
long blit(long x1, long y1, long x2, long y2, long width, long height)
{
	long     adr1,adr2;
	char     mode;

	lock_ge();
	outp(0x3ce,0x31);
	outp(0x3cf,4);

	if (scrnColors == 32) {
		x1 <<= 2;
		x2 <<= 2;
		width = (width<<2)+3;
	}
	
	adr1 = y1*scrnRowByte+x1;
	adr2 = y2*scrnRowByte+x2;
	if (adr1 < adr2){
		mode = 1;
		adr1 += scrnRowByte*height+width;
		adr2 += scrnRowByte*height+width;
	}
	else mode = 0;
	
	outp(0x3ce,0x20);
	outp(0x3cf,width&255);
	outp(0x3ce,0x21);
	outp(0x3cf,(width>>8)&255);

	outp(0x3ce,0x22);
	outp(0x3cf,height&255);
	outp(0x3ce,0x23);
	outp(0x3cf,(height>>8)&255);

	outp(0x3ce,0x24);
	outp(0x3cf,scrnRowByte&255);
	outp(0x3ce,0x25);
	outp(0x3cf,(scrnRowByte>>8)&255);

	outp(0x3ce,0x26);
	outp(0x3cf,scrnRowByte&255);
	outp(0x3ce,0x27);
	outp(0x3cf,(scrnRowByte>>8)&255);

	outp(0x3ce,0x28);
	outp(0x3cf,adr2&255);
	outp(0x3ce,0x29);
	outp(0x3cf,(adr2>>8)&255);
	outp(0x3ce,0x2a);
	outp(0x3cf,(adr2>>16)&255);
	
	outp(0x3ce,0x2c);
	outp(0x3cf,adr1&255);
	outp(0x3ce,0x2d);
	outp(0x3cf,(adr1>>8)&255);
	outp(0x3ce,0x2e);
	outp(0x3cf,(adr1>>16)&255);

	outp(0x3ce,0x2f);
	outp(0x3cf,0);
	
	outp(0x3ce,0x30);
	outp(0x3cf,mode);

	outp(0x3ce,0x32);
	outp(0x3cf,0x0d);

	outp(0x3ce,0x31);
	while ((inp(0x3cf) & 8) == 8) {};
	
	outp(0x3ce,0x31);
	outp(0x3cf,2);
	
	outp(0x3ce,0x31);
	while ((inp(0x3cf) & 8) == 8) {};
	unlock_ge();
	
	return B_NO_ERROR;
}

// This function is use to inverse a rectangle in 32 bits mode.
long invert_rect_32(long x1, long y1, long x2, long y2)
{
	long     y,adr1,height,width;
	char     oper;

	lock_ge();
	outp(0x3ce,0x31);
	outp(0x3cf,4);
	
	x1 <<= 2;
	x2 = (x2<<2)+3;
	
	adr1 = y1*scrnRowByte+x1;
	height = y2-y1;
	width = x2-x1;
	
	outp(0x3ce,0x20);
	outp(0x3cf,width&255);
	outp(0x3ce,0x21);
	outp(0x3cf,(width>>8)&255);

	outp(0x3ce,0x22);
	outp(0x3cf,height&255);
	outp(0x3ce,0x23);
	outp(0x3cf,(height>>8)&255);

	outp(0x3ce,0x24);
	outp(0x3cf,scrnRowByte&255);
	outp(0x3ce,0x25);
	outp(0x3cf,(scrnRowByte>>8)&255);

	outp(0x3ce,0x26);
	outp(0x3cf,0);
	outp(0x3ce,0x27);
	outp(0x3cf,0);

	outp(0x3ce,0x28);
	outp(0x3cf,adr1&255);
	outp(0x3ce,0x29);
	outp(0x3cf,(adr1>>8)&255);
	outp(0x3ce,0x2a);
	outp(0x3cf,(adr1>>16)&255);

	outp(0x3ce,0x2c);
	outp(0x3cf,(long)scrnBase&255);
	outp(0x3ce,0x2d);
	outp(0x3cf,((long)scrnBase>>8)&255);
	outp(0x3ce,0x2e);
	outp(0x3cf,((long)scrnBase>>16)&255);
	
	outp(0x3ce,0x2f);
	outp(0x3cf,0);
	
	outp(0x3ce,0x30);
	outp(0x3cf,0xb0);

	outp(0x3ce,0x32);
	outp(0x3cf,0x0b);

	outp(0x3ce,0x31);
	while ((inp(0x3cf) & 8) == 8) {};
	
	outp(0x3ce,0x31);
	outp(0x3cf,2);
	
	outp(0x3ce,0x31);
	while ((inp(0x3cf) & 8) == 8) {};
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
	uchar	v;
	
// 0x3c0 is the only register used at the same time as index and data register.
// So we've to be careful to stay synchronize (the card switch between index
// and data at each write. Reading 0x3da give us a simple way to stay well-
// synchronized.
	inp(0x3da);

// Save the initial index set in 0x3c0
	v = inp(0x3c0);

	while(1) {
		p1 = *ptr++;
		p2 = *ptr++;
		if (p1 == 0xff && p2 == 0xff) {
			inp(0x3da);
  // Restore the initial index set in 0x3c0
			outp(0x3c0, v | 0x20);
			return;
		}
		inp(0x3da);
		outp(0x3c0, p1);
		outp(0x3c0, p2);
	}
}


// All the other table settings are done with that very simple function.
static void	set_table(ushort *ptr)
{
	ushort	p1;
	ushort	p2;

	while(1) {
		p1 = *ptr++;
		p2 = *ptr++;
		if (p1 == 0 && p2 == 0) {
			return;
		}
		outp(p1, p2);
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
static void	setup_dac()
{
	inp(0x3c6);
	inp(0x3c6);
	inp(0x3c6);
	inp(0x3c6);
	outp(0x3c6, 0x00);
	inp(0x3c8);
	outp(0x3c6, 0xff);
}

// ##----> THOSE SETTINGS SHOULD BE COMPATIBLE WITH ALL VGA CHIPS. <----##
// Set the DAC in the standard 32 bits (rgb color) mode.
static void	setup_dac32()
{
	inp(0x3c6);
	inp(0x3c6);
	inp(0x3c6);
	inp(0x3c6);
	outp(0x3c6, 0xc5);
	inp(0x3c8);
	outp(0x3c6, 0xff);
}



//**************************************************************************
//  cirrus alpine internal PLL and clock generator stuff
//**************************************************************************

static float ScreenClockStep[32] =
{
	8.0, 8.0, 8.0, 0.0, 0.0,
	0.0, 0.0, 0.0, 0.0, 0.0,
	8.0, 8.0, 0.0, 0.0, 0.0,
	0.0, 0.0, 0.0, 0.0, 0.0,
	0.0, 0.0, 0.0, 0.0, 0.0,
	0.0, 0.0, 0.0, 0.0, 0.0,
	0.0, 0.0,
};

void select_clock()
{
	float    freq,fr,fr0,fr2;
	int      p,q,p2,q2;

	freq = ScreenClockStep[scrnResNum] * scrnRate *
		   (float)lastCrtHT * (float)lastCrtVT * 1e-6;

// calcul p and q parameters (method 1)
	fr0 = freq/TI_REF_FREQ;
	fr2 = -2.0;
	for (q=1;q<=31;q++) {
		p = (int)(fr0*((float)q)+0.5);
		fr = ((float)p)/((float)q);
		if (((fr0-fr)*(fr0-fr) < (fr0-fr2)*(fr0-fr2)) && (p>1) && (p<128)) {
			p2 = p;
			q2 = 2*q+0;
			fr2 = fr;
		}
	}

// calcul p and q parameters (method 2)
	fr0 = freq/(TI_REF_FREQ/2);
	for (q=1;q<=63;q++) {
		p = (int)(fr0*((float)q)+0.5);
		fr = ((float)p)/((float)q);
		if (((fr0-fr)*(fr0-fr) < (fr0-fr2)*(fr0-fr2)) && (p>1) && (p<128)) {
			p2 = p;
			q2 = 2*q+1;
			fr2 = fr;
		}
	}

	outp(0x3c4,0x06);
	outp(0x3c5,0x12);
	outp(0x3c4,0x0b);
	outp(0x3c5,p2);
	outp(0x3c4,0x1b);
	outp(0x3c5,q2);
}



//**************************************************************************
//  CRT_CONTROL stuff for cirrus alpine.
//
//  The purpose of CRT_CONTROL is to allow the user to modify the horizontal
//  and the vertical positions of the display on the screen. The width and
//  height can be modified too, but the result is clearly dependant of the
//  monitor abilities. The technic used to do that consists in modifying the
//  position of the horizontal and vertical synchro for one part, and
//  modifying the proportion between display size (h or v) and total size
//  (h or v). We do that by bilinear interpolation between four states for each
//  direction (4 for horizontal settings, 4 for vertical settings).
//**************************************************************************


// This is the index of the 26 crt registers and extended registers (all access
// through 3c5 index by 3c4) modified by CRT_CONTROL. After that, those
// registers will just be refered as number 0 to 25.

static uchar crt_index[] =
{
  0x00, 0x1d, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
  0x07, 0x09, 0x10, 0x11, 0x12, 0x15, 0x16, 0x1a,
  0x1b,
  0x08, 0x0a, 0x0b, 0x0c, 0x0d, 0x13, 0x14, 0x17,
  0x18
};


// Values to set we need to set into the 26 previous registers to get the
// selected CRT_CONTROL settings.
static 	uchar     settings[26];


// Calculate the settings of the 26 registers to get the positions and sizes
// described in crtPosH, crtPosV, crtSizeH and crtSizeV (value from 0 to 100).
static void prepare_crt()
{
	int       i,par1,par2,par3;
	uchar     Double;
	ulong     Hset[6],Vset[6];
	ulong     Eset[2];
	
// Calculate the value of the 6 basic horizontal crt parameters depending of
// crtPosH and crtSizeH, and the resolution.
//   - 0 : Horizontal total number of clock character per line.
//   - 1 : Horizontal number of clock character in the display part of a line.
//   - 2 : Start of horizontal blanking.
//   - 3 : End of horizontal blanking.
//   - 4 : Start of horizontal synchro position.
//   - 5 : End of horizontal synchro position.
	Hset[1] = scrnWidth/8;
	Hset[0] = (Hset[1]*(700-crtSizeH)+250)/500;
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
	Hset[4] = Hset[2]+(par2*(100-crtPosH)+50)/100;
	Hset[5] = Hset[4]+par3;
	if (par3>63) {
		Hset[5] -= (par3-63)/2;
		Hset[4] += (par3-62)/2;
	}
	
// Calculate the value of the 6 basic vertical crt parameters depending of
// crtPosV and crtSizeV, and the resolution.
//   - 0 : Vertical total number of counted lines per frame
//   - 1 : Start of vertical blanking (line number).
//   - 2 : End of vertical blanking (line number).
//   - 3 : Vertical number of lines in the display part of a frame.
//   - 4 : Start of vertical synchro position (line number).
//   - 5 : End of vertical synchro position (line number).
	Vset[3] = scrnHeight;
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
	Vset[1] = Vset[4]+((100-crtPosV)*par2+50)/100;
	Vset[2] = Vset[1]+par3;

// Calculate extended parameters (rowByte base adress of the display buffer.
	Eset[0] = ((scrnPosH*scrnColors)/8+scrnPosV*scrnRowByte)/4;
	Eset[1] = scrnRowByte/8;
	if (scrnColors == 32) Eset[1] >>= 1;
	
// Memorize the last setting for horizontal and vertical total for use to set
// the clock frequency (see write_pll for more informations).
	lastCrtHT = Hset[0];
	lastCrtVT = Vset[0];
	if (lastCrtVT < 300) {
		Double = 0x80;
		lastCrtVT *= 2;
	}
	else
		Double = 0x00;

// 5 of those 14 parameters have to be adjust by a specific offset before to be
// encoded in the appropriate registers.
	Hset[0] -= 5;
	Hset[1] -= 1;
	Vset[0] -= 2;
	Vset[3] -= 1;
	Vset[4] -= 1;

// 2 of those registers are encoded in a bit complicated way (see databook for
// more information)
	Hset[3] = (Hset[3]&63)+((Hset[3]-Hset[2])&64);
	Hset[5] = (Hset[5]&31)+((Hset[5]-Hset[4])&32);

// Those 2 parameters need to be trunc respectively to 4 and 8 bits.
	Vset[2] = (Vset[2]&15);
	Vset[5] = (Vset[5]&255);

// Those 12 parameters are encoded through the 26 8-bits registers. That code
// is doing the conversion, even inserting a few more significant bit depending
// of the space setting.
	settings[0] = Hset[0]&255;
	settings[1] = (Eset[0]&0x80000)>>12;
	settings[2] = Hset[1]&255;
	settings[3] = Hset[2]&255;
	settings[4] = Hset[3]&31 | 128;
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
	settings[15] = ((Vset[2]&768)>>2) | ((Hset[3]&192)>>2);
	settings[16] = ((Eset[1]&256)>>4) | ((Eset[0]&0x60000)>>15) |
		           2 | ((Eset[0]&0x10000)>>16);
	settings[17] = 0x00;
	settings[18] = 0x20;
	settings[19] = 0x00;
	settings[20] = (Eset[0]&0xff00)>>8;
	settings[21] = (Eset[0]&0xff);
	settings[22] = (Eset[1]&0xff);
	settings[23] = 0x60;
	settings[24] = 0xeb;
	settings[25] = 0xff;
}


// Set the CRT_CONTROL by writing the preprocessed values in the 26 registers.
static void vid_select_crt()
{
	int      i;
	volatile long  j;
	
	for (i=0;i<26;i++) {
		outp(0x3d4, crt_index[i]);
		outp(0x3d5, settings[i]);
		for (j=0;j<1000;j++) {;}
	}
}

// Modification of the base adress of the display buffer
static void vid_selectpos()
{
	prepare_crt();
	
	outp(0x3d4, crt_index[20]);
	outp(0x3d5, settings[20]);
	outp(0x3d4, crt_index[21]);
	outp(0x3d5, settings[21]);
	outp(0x3d4, crt_index[16]);
	outp(0x3d5, settings[16]);
	outp(0x3d4, crt_index[1]);
	outp(0x3d5, settings[1]);
}



//**************************************************************************
//  Complete specific color, width, mem and performance settings for the
//  cirrus alpine.
//**************************************************************************

static uchar index_cirrus_settings[5] =
{
	0x07, 0x0f,
	0x17, 0x19, 0x1a,
};

void do_cirrus_settings()
{
	int      i;
	uchar    value[5];
	
	switch (scrnColors) {
	case 8 :
		value[0] = 0x11;
		value[1] = 0x30;
		break;
	case 32 :
		value[0] = 0x19;
		value[1] = 0x38;
		break;
	}
	if (scrnWidth < 800) {
		value[2] = 0xab;
		value[3] = 0x31;
		value[4] = 0x00;
	}
	else {
		value[2] = 0xe3;
		value[3] = 0x46;
		value[4] = 0x00;
	}

	for (i=0;i<2;i++) {
		outp(0x3c4,index_cirrus_settings[i]);
		outp(0x3c5,value[i]);
	}
	for (i=2;i<5;i++) {
		outp(0x3d4,index_cirrus_settings[i]);
		outp(0x3d5,value[i]);
	}
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

static void vid_checkmem(void)
{
	bool      test;
	long      i,j;
	bigtime_t time;
	
	outp(0x3c4, 0x01);
	outp(0x3c5, 0x21);

// test 2 MB configuration
	theMem = 2048;
	scrnColors = 32;
	scrnWidth = 640;
	scrnHeight = 480;
	scrnRowByte = 640*4;
	scrnRes = vga640x480;

	DoSelectMode(FALSE);

	scrnColors = 0;
	scrnRes = -1;

	outp(0x3c4, 0x0f);
	if ((inp(0x3c5)&0x18) == 0x18)
/*
// Write 32 bytes pattern every 256K
	for (i=0;i<8;i++)
		for (j=0;j<32;j++)
			((uchar*)scrnBase)[(i<<18)+j] = (i<<5)+j;

// Check if the pattern are all the same (no wrapping).
	test = TRUE;
	for (i=0;i<8;i++)
		for (j=0;j<32;j++)
			if (((uchar*)scrnBase)[(i<<18)+j] != (i<<5)+j)
				test = FALSE;
	if (test)*/
		theMem = 2048;
	else
		theMem = 1024;

// Display available amount of memory	
	dprintf("Graphic memory available : %d Ko\n",theMem);

// Measure the writing bandwidth to the videocard on a 800K buffer, using 64
// bits writing access.
	time = system_time();
	for (j=0;j<2;j++)
		for (i=0;i<51200;i++)
			((double*)scrnBase)[i] = 0L;
	time = system_time()-time;

// Display available writing bandwidth
	dprintf("Memory card bandwidth : %d Kb/s\n",(long)(800000000/time));

// The spaces available depend of the amount of video memory available...	
	available_spaces = B_8_BIT_640x480 | B_8_BIT_800x600;
	if (theMem >= 1024)
		available_spaces |= B_8_BIT_1024x768|B_8_BIT_1152x900;
	if (theMem >= 1536)
		available_spaces |= B_32_BIT_640x480|B_8_BIT_1280x1024;
	if (theMem >= 2048)
		available_spaces |= B_32_BIT_800x600;
	
// ...and the specific abilities of the add-on depending of the chip sets.
	available_spaces &= vga_res_available[theVGA];
}


/* ----------
	vid_selectrate : set current refresh rate
----- */

static void vid_selectrate (void)
{
	lock_io();
	lock_ge();

	outp(0x3c4, 0x01);
	outp(0x3c5, 0x21);		// blank the screen

	prepare_crt();
	select_clock();
	vid_select_crt();

	outp(0x3c4, 0x01);
	outp(0x3c5, 0x01);		// blank the screen

	unlock_ge();
	unlock_io();
}

/* ----------
	vid_selectmode : set current depth and resolution
----- */

void DoSelectMode(bool mode) {
	int      i,j;
	uchar    *base;
	ulong    *base2;

	lock_io();
	lock_ge();
	
	outp(0x3c4, 0x01);
	outp(0x3c5, 0x21);		// blank the screen

	set_table(vga_cirrus_table);
	do_cirrus_settings();
	set_table(sequencer_table);
	set_attr_table(attribute_table);

	if (scrnColors == 8)
		setup_dac();
	else
		setup_dac32();

	set_table(gcr_table);
	prepare_crt();
	select_clock();
	vid_select_crt();

	if (mode) {
		if (scrnColors == 8) {
			base = (uchar*)scrnBase;
			for (i = 0; i < scrnHeight / 2; i++) {
				for (j = 0; j < scrnWidth / 2; j++) {
					*(base++) = 0xff;
					*(base++) = 0x00;
				}
				for (j = 0; j < scrnWidth / 2; j++) {
					*(base++) = 0x00;
					*(base++) = 0xff;
				}
			}
		}
		else {
			base2 = (ulong*)scrnBase;
			for (i = 0; i < scrnHeight / 2; i++) {
				for (j = 0; j < scrnWidth / 2; j++) {
					*(base2++) = 0xffffffff;
					*(base2++) = 0x00000000;
				}
				for (j = 0; j < scrnWidth / 2; j++) {
					*(base2++) = 0x00000000;
					*(base2++) = 0xffffffff;
				}
			}
		}
	}
	
	scrnBufBase = (uchar*)(((long)scrnBase+scrnRowByte*scrnHeight+1023)&0xFFFFFC00);
	
	outp(0x3c4, 0x01);
	outp(0x3c5, 0x01);		//unblank the screen
		
	outp(0x3d4, 0x29);
	outp(0x3d5, 0x84);
	outp(0x217a, 0x34);		// disable hardware cursor
	outp(0x217e, 0x00);

	unlock_ge();
	unlock_io();
}
	
static void vid_selectmode (void)
{
	DoSelectMode(TRUE);	
}



//**************************************************************************
//  The last one : the main entry of the Graphics add-on.
//
//  When the application server load this add-on, this is its only entry
//  point, and so the only symbol he looks for (see the documentation for
//  more informations about command, parameters buffer and return value).
//**************************************************************************

#if INTEL_STATIC_DRIVERS
extern "C"
long cirrus_control_graphics_card(ulong message,void *buf)
#else
long control_graphics_card(ulong message,void *buf)
#endif	
{
	int        i,err;
	ulong      code;
	
// By default, anybody is supposed to return B_NO_ERROR if nothing specific
// goes wrong.
	err = B_NO_ERROR;

// Message dispatcher. Other message will be added in future version. So you
// need to define the default case and then return B_ERROR.
	switch (message) {
		
// Use by the application server to get information about the frame buffer and
// part of the abilities of the add-on itself.
	case B_GET_GRAPHICS_CARD_INFO :

  // Current height of the frame buffer in lines. 
	    ((graphics_card_info*)buf)->height = offscrnHeight;
  // Current width of the frame buffer in pixels
		((graphics_card_info*)buf)->width = offscrnWidth;
  // Current depth of the screen in bits per pixel	
		((graphics_card_info*)buf)->bits_per_pixel = scrnColors;
  // Pointer on the beginning of the current frame buffer
		((graphics_card_info*)buf)->frame_buffer = (void*)scrnBase;
  // Offset (in byte) between the beginning of two lines
		((graphics_card_info*)buf)->bytes_per_row = scrnRowByte;
  // Define if some optional abilities are available (see documentation for
  // more informations).
		((graphics_card_info*)buf)->flags = vga_flags[theVGA];
  // Define the order in which the 4 channels are stored in video memory in 32
  // bits depth.
		*((long*)&((graphics_card_info*)buf)->rgba_order) = (long)('bgra');
  // Version of the add-on. The new version support by the release 8 is 2.
		((graphics_card_info*)buf)->version = 2;
  // Id reserved for internal use by combined library/add-on in future version.
  // The application server doesn't use that at all.
		((graphics_card_info*)buf)->id = 0;
		break;
		
	case B_GET_GRAPHICS_CARD_HOOKS :
#if BYTE_ORDER == __LITTLE_ENDIAN
		for (i=0;i<B_HOOK_COUNT;i++) {
			((graphics_card_hook*)buf)[i] = 0L;
		}
		((graphics_card_hook*)buf)[7] = vga_dispatch[7];
#else
		for( i=0; i<B_HOOK_COUNT; i++ ) {
			((graphics_card_hook*)buf)[i] = vga_dispatch[i];
		}
#endif
		if (scrnColors != 32)
			((graphics_card_hook*)buf)[11] = 0L;
		break;
		
	case B_OPEN_GRAPHICS_CARD :
		scrnBase = ((graphics_card_spec*)buf)->screen_base;
		isa_IO = (uchar*)((graphics_card_spec*)buf)->io_base;

		if (((graphics_card_spec*)buf)->vendor_id != 0x1013) {
			err = B_ERROR;
			break;
		}

		set_table(std_cirrus_vga_table);
		
		switch (((graphics_card_spec*)buf)->device_id) {
		case 0x00a0:
		case 0x00ac:
			theVGA = cirrus_5430;
			break;
		case 0x00a8:
			theVGA = cirrus_5434;
			break;
		default:
			theVGA = -1;
			break;
		}
		if (theVGA < 0) {
			err = B_ERROR;
			break;
		}

		init_locks();
		
		vid_checkmem();

		break;
		
	case B_CLOSE_GRAPHICS_CARD :
//		outp(0x3c4, 0x01);
//		outp(0x3c5, 0x21);
		dispose_locks();
		break;
		
	case B_SET_INDEXED_COLOR:
		lock_io();
		
		outp(0x3c6, 0xff);
		inp(0x3c7);
		outp(0x3c8, ((indexed_color*)buf)->index);
		inp(0x3c7);
		
		outp(0x3c9, (((indexed_color*)buf)->color.red) >> 2);
		inp(0x3c7);
		outp(0x3c9, (((indexed_color*)buf)->color.green) >> 2);
		inp(0x3c7);
		outp(0x3c9, (((indexed_color*)buf)->color.blue) >> 2);
		inp(0x3c7);

		unlock_io();
		break;
		
	case B_GET_SCREEN_SPACES:
		((graphics_card_config*)buf)->space = available_spaces;
		break;
		
	case B_CONFIG_GRAPHICS_CARD:
		if ((((graphics_card_config*)buf)->space & available_spaces) == 0) {
			err = B_ERROR;
			break;
		}
		crtPosH = ((graphics_card_config*)buf)->h_position;
		crtSizeH = ((graphics_card_config*)buf)->h_size;
		crtPosV = ((graphics_card_config*)buf)->v_position;
		crtSizeV = ((graphics_card_config*)buf)->v_size;
		if (((graphics_card_config*)buf)->space != scrnResCode) {
			scrnResCode = ((graphics_card_config*)buf)->space;
			scrnResNum = 0;
			code = scrnResCode;
			while (code > 1) {
				scrnResNum++;
				code >>= 1;
			}
			scrnRate = ((graphics_card_config*)buf)->refresh_rate;
			if (scrnRate < vga_min_rates[scrnResNum])
				scrnRate = vga_min_rates[scrnResNum];
			if (scrnRate > vga_max_rates[scrnResNum])
				scrnRate = vga_max_rates[scrnResNum];
			switch (scrnResCode) {
			case B_8_BIT_640x480:
				scrnColors = 8;
				scrnRes = vga640x480;
				scrnWidth = 640;
				scrnHeight = 480;
				scrnRowByte = 640;
			set_res:
				offscrnWidth = scrnWidth;
				offscrnHeight = scrnHeight;
				scrnPosH = 0;
				scrnPosV = 0;
				DoSelectMode(TRUE);
				break;
			case B_8_BIT_800x600:
				scrnColors = 8;
				scrnRes = vga800x600;
				scrnWidth = 800;
				scrnHeight = 600;
				scrnRowByte = 800;
				goto set_res;
			case B_8_BIT_1024x768:
				scrnColors = 8;
				scrnRes = vga1024x768;
				scrnWidth = 1024;
				scrnHeight = 768;
				scrnRowByte = 1024;
				goto set_res;
			case B_8_BIT_1152x900:
				scrnColors = 8;
				scrnRes = vga1152x900;
				scrnWidth = 1152;
				scrnHeight = 900;
				scrnRowByte = 1152;
				goto set_res;
			case B_8_BIT_1280x1024:
				scrnColors = 8;
				scrnRes = vga1280x1024;
				scrnWidth = 1280;
				scrnHeight = 1024;
				scrnRowByte = 1280;
				goto set_res;
			case B_8_BIT_1600x1200:
				scrnColors = 8;
				scrnRes = vga1600x1200;
				scrnWidth = 1600;
				scrnHeight = 1200;
				scrnRowByte = 1600;
				goto set_res;
			case B_32_BIT_640x480:
				scrnColors = 32;
				scrnRes = vga640x480;
				scrnWidth = 640;
				scrnHeight = 480;
				scrnRowByte = 2560;
				goto set_res;
			case B_32_BIT_800x600:
				scrnColors = 32;
				scrnRes = vga800x600;
				scrnWidth = 800;
				scrnHeight = 600;
				scrnRowByte = 3200;
				goto set_res;
  // Space unsupported (this protection is not necessary)
			default :
				err = B_ERROR;
			}
		}
		else {
			scrnRate = ((graphics_card_config*)buf)->refresh_rate;
			if (scrnRate < vga_min_rates[scrnResNum])
				scrnRate = vga_min_rates[scrnResNum];
			if (scrnRate > vga_max_rates[scrnResNum])
				scrnRate = vga_max_rates[scrnResNum];
			vid_selectrate();
		}
		break;
	case B_GET_REFRESH_RATES :
		((refresh_rate_info*)buf)->min = vga_min_rates[scrnResNum];
		((refresh_rate_info*)buf)->max = vga_max_rates[scrnResNum];
		((refresh_rate_info*)buf)->current = scrnRate;
		break;

// Give the size of the buffer needed to discribe the current internal addon setting
	case B_GET_INFO_FOR_CLONE_SIZE :
	
		*((long*)buf) = sizeof(clone_info);
		break;

// Send back a copy of the current internal addon setting
	case B_GET_INFO_FOR_CLONE :
		((clone_info*)buf)->theVGA = theVGA;
		((clone_info*)buf)->theMem = theMem;
		((clone_info*)buf)->scrnRowByte = scrnRowByte;
		((clone_info*)buf)->scrnWidth = scrnWidth;
		((clone_info*)buf)->scrnHeight = scrnHeight;
		((clone_info*)buf)->offscrnWidth = offscrnWidth;
		((clone_info*)buf)->offscrnHeight = offscrnHeight;
		((clone_info*)buf)->scrnPosH = scrnPosH;
		((clone_info*)buf)->scrnPosV = scrnPosV;
		((clone_info*)buf)->scrnColors = scrnColors;
		((clone_info*)buf)->scrnBase = scrnBase;
		((clone_info*)buf)->scrnRate = scrnRate;
		((clone_info*)buf)->crtPosH = crtPosH;
		((clone_info*)buf)->crtSizeH = crtSizeH;
		((clone_info*)buf)->crtPosV = crtPosV;
		((clone_info*)buf)->crtSizeV = crtSizeV;
		((clone_info*)buf)->scrnResCode = scrnResCode;
		((clone_info*)buf)->scrnResNum = scrnResNum;
		((clone_info*)buf)->scrnBufBase = scrnBufBase;
		((clone_info*)buf)->scrnRes = scrnRes;
		((clone_info*)buf)->available_spaces = available_spaces;
		((clone_info*)buf)->hotpt_h = hotpt_h;
		((clone_info*)buf)->hotpt_v = hotpt_v;
		((clone_info*)buf)->lastCrtHT = lastCrtHT;
		((clone_info*)buf)->lastCrtVT = lastCrtVT;
		((clone_info*)buf)->isa_IO = (uchar *) isa_IO;	/* FIXME: Is there a problem here with volatile? -fnf */
		break;

// Update the current internal addon settings
	case B_SET_CLONED_GRAPHICS_CARD :
		theVGA = ((clone_info*)buf)->theVGA;
		theMem = ((clone_info*)buf)->theMem;
		scrnRowByte = ((clone_info*)buf)->scrnRowByte;
		scrnWidth = ((clone_info*)buf)->scrnWidth;
		scrnHeight = ((clone_info*)buf)->scrnHeight;
		offscrnWidth = ((clone_info*)buf)->offscrnWidth;
		offscrnHeight = ((clone_info*)buf)->offscrnHeight;
		scrnPosH = ((clone_info*)buf)->scrnPosH;
		scrnPosV = ((clone_info*)buf)->scrnPosV;
		scrnColors = ((clone_info*)buf)->scrnColors;
		scrnBase = ((clone_info*)buf)->scrnBase;
		scrnRate = ((clone_info*)buf)->scrnRate;
		crtPosH = ((clone_info*)buf)->crtPosH;
		crtSizeH = ((clone_info*)buf)->crtSizeH;
		crtPosV = ((clone_info*)buf)->crtPosV;
		crtSizeV = ((clone_info*)buf)->crtSizeV;
		scrnResCode = ((clone_info*)buf)->scrnResCode;
		scrnResNum = ((clone_info*)buf)->scrnResNum;
		scrnBufBase = ((clone_info*)buf)->scrnBufBase;
		scrnRes = ((clone_info*)buf)->scrnRes;
		available_spaces = ((clone_info*)buf)->available_spaces;
		hotpt_h = ((clone_info*)buf)->hotpt_h;
		hotpt_v = ((clone_info*)buf)->hotpt_v;
		lastCrtHT = ((clone_info*)buf)->lastCrtHT;
		lastCrtVT = ((clone_info*)buf)->lastCrtVT;
		isa_IO = ((clone_info*)buf)->isa_IO;
		init_locks();
		break;

// Close an cloned addon.
	case B_CLOSE_CLONED_GRAPHICS_CARD :
		dispose_locks();
		break;

// Ask for a set of (Depth, Width) what are the rowByte and the maximal height.
	case B_PROPOSE_FRAME_BUFFER :
		{
			long   mem, row;

			((frame_buffer_info*)buf)->bytes_per_row = -1;
			((frame_buffer_info*)buf)->height = -1;
			i = ((frame_buffer_info*)buf)->bits_per_pixel;
			row = ((frame_buffer_info*)buf)->width;
			if ((i != 8) && (i != 32)) {
				err = B_ERROR;
				break;
			}
			row = (row+7)&0xff8;
			((frame_buffer_info*)buf)->bytes_per_row = row;
			mem = theMem*8192L;
			mem /= row*i;
			if (mem > 2048) mem = 2048;
			((frame_buffer_info*)buf)->height = mem;
		}
		break;

// Set frame rectangle size
	case B_SET_FRAME_BUFFER :
		{
			long    col, fw, fh, row, x, y, dw, dh;
			
			col = ((frame_buffer_info*)buf)->bits_per_pixel;
			fw = ((frame_buffer_info*)buf)->width;
			fh = ((frame_buffer_info*)buf)->height;
			dw = ((frame_buffer_info*)buf)->display_width;
			dh = ((frame_buffer_info*)buf)->display_height;
			row = ((frame_buffer_info*)buf)->bytes_per_row;
			x = ((frame_buffer_info*)buf)->display_x;
			y = ((frame_buffer_info*)buf)->display_y;
			
			if ((col != 8) && (col != 32))
				goto bad_setting;
			if ((fw > row) || (dw > fw) || (dh > fh))
				goto bad_setting;
			if ((row & 7) != 0) 
				goto bad_setting;
			if ((row*fh*col) > (theMem*8192L))
				goto bad_setting;
			if ((x < 0) || (y < 0) || (y+dh > fh) || (x+dw > fw))
				goto bad_setting;
			
			scrnRes = vga_specific;
			scrnResCode = ~0L;
			scrnColors = col;
			offscrnWidth = fw;
			offscrnHeight = fh;
			scrnWidth = dw;
			scrnHeight = dh;
			scrnRowByte = row;
			scrnPosH = x;
			scrnPosV = y;
			DoSelectMode(TRUE);
			break;
			
		bad_setting:
			err = B_ERROR;
			break;
		}

// Set display rectangle size
	case B_SET_DISPLAY_AREA :
		{
			long    x, y, dw, dh;
			
			dw = ((frame_buffer_info*)buf)->display_width;
			dh = ((frame_buffer_info*)buf)->display_height;
			x = ((frame_buffer_info*)buf)->display_x;
			y = ((frame_buffer_info*)buf)->display_y;
			
			if ((dw > offscrnWidth) || (dh > offscrnHeight) ||
				(x < 0) || (y < 0) ||
				(y+dh > offscrnHeight) || (x+dw > offscrnWidth)) {
				err = B_ERROR;
				break;
			}
			
			scrnWidth = dw;
			scrnHeight = dh;
			scrnPosH = x;
			scrnPosV = y;
		    vid_selectrate();
			break;
		}

// Set the origin of the display rectangle in the frame rectangle
	case B_MOVE_DISPLAY_AREA :
		{
			long    x, y;
			
			x = ((frame_buffer_info*)buf)->display_x;
			y = ((frame_buffer_info*)buf)->display_y;
			
			if ((x+scrnWidth > offscrnWidth) || (x < 0) ||
				(y+scrnHeight > offscrnHeight) || (y < 0)) {
				err = B_ERROR;
				break;
			}
			
			scrnPosH = x;
			scrnPosV = y;
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
