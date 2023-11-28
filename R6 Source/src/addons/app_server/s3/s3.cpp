//*****************************************************************************
//
//	File:		 s3.cpp
//
//	Description: add-on for graphics cards using s3 chips.
//
//	Copyright 1996, Be Incorporated
//
//*****************************************************************************

// This include just provides prototypes for the few kernel calls usable in the
// add-on (see documentation on graphics add-on development for more infos).
// This add-on has to be linked with a specific library (libkernel.a).
#include <OS.h>
#include <Debug.h>

// Graphics card add-ons standard structs and prototypes
#include <GraphicsCard.h>

// Specific stuff for that add-on
#include "s3.h"
#include "priv_syscalls.h"

#define ddprintf _sPrintf

void DoSelectMode(void);

//**************************************************************************
//	Chip's available abilities description
//**************************************************************************

// This table describes the available spaces for each chip (without considering
// the available amount of ram on the card).
static ulong vga_res_available[CHIP_COUNT] = {
  // 964
	B_8_BIT_640x480|B_8_BIT_800x600|B_8_BIT_1024x768|B_8_BIT_1280x1024|B_8_BIT_1600x1200|
	B_32_BIT_640x480|B_32_BIT_800x600|B_8_BIT_1152x900|B_32_BIT_1024x768|B_32_BIT_1152x900,
  // 864
	B_8_BIT_640x480|B_8_BIT_800x600|B_8_BIT_1024x768|B_8_BIT_1280x1024|
	B_32_BIT_640x480|B_32_BIT_800x600|B_8_BIT_1152x900,
  // trio 32
	B_8_BIT_640x480|B_8_BIT_800x600|B_8_BIT_1024x768|B_8_BIT_1280x1024|
	B_32_BIT_640x480|B_32_BIT_800x600|B_8_BIT_1152x900,
  // trio 64
	B_8_BIT_640x480|B_8_BIT_800x600|B_8_BIT_1024x768|B_8_BIT_1280x1024|
	B_32_BIT_640x480|B_32_BIT_800x600|B_8_BIT_1152x900,
  // trio 64v+
	B_8_BIT_640x480|B_8_BIT_800x600|B_8_BIT_1024x768|B_8_BIT_1280x1024|
	B_32_BIT_640x480|B_32_BIT_800x600|B_8_BIT_1152x900|B_32_BIT_1024x768|B_32_BIT_1152x900,
  // virge
	B_8_BIT_640x480|B_8_BIT_800x600|B_8_BIT_1024x768|B_8_BIT_1152x900|B_8_BIT_1280x1024,

};

// This table describes the available specific abilities for each chip (this
// is the value return in "flags" in the "graphics_card_info" struct).
static ushort vga_flags[CHIP_COUNT] = {
  // 964
	B_CRT_CONTROL|B_FRAME_BUFFER_CONTROL|B_LAME_ASS_CARD,
  // 864
	B_CRT_CONTROL|B_FRAME_BUFFER_CONTROL|B_LAME_ASS_CARD,
  // trio 32
	B_CRT_CONTROL|B_FRAME_BUFFER_CONTROL|B_LAME_ASS_CARD,
  // trio 64
	B_CRT_CONTROL|B_FRAME_BUFFER_CONTROL|B_LAME_ASS_CARD,
  // trio 64v+
	B_CRT_CONTROL|B_FRAME_BUFFER_CONTROL|B_LAME_ASS_CARD,
  // virge
	B_CRT_CONTROL|B_FRAME_BUFFER_CONTROL|B_LAME_ASS_CARD,
};



//**************************************************************************
//	Private globals of the s3 add-on.
//**************************************************************************

// Id of the vga chip used by the current card (see s3.h for codes)
static int				theVGA;

// Id of the DAC used by the current card (see s3.h for codes)
static int              theDAC;

// Available memory (in KB). If the card has more memory than what this add-on
// is able to use, extra memory is ignored.
static int              theMem;

// Offset (in bytes) between the beginning of two lines of the frame buffer.
static int				scrnRowByte;

// Width of the frame buffer (in pixels)
static int				scrnWidth;

// Height of the frame buffer (in lines)
static int				scrnHeight;

// Width of the offscreen buffer (in pixels)
static int              offscrnWidth;

// Height of the offscreen buffer (in pixels)
static int              offscrnHeight;

// Offset H and V of the screen in the offscreen (in pixels)
static int              scrnPosH = 0;
static int              scrnPosV = 0;

// Depth of the screen (in bits per pixel)
static int				scrnColors = 8;

// Base adress of the frame buffer, as mapped in add-on memory space.
static void 			*scrnBase;

// Current refresh rate (in Hertz)
static float            scrnRate = 60.1;

// Current CRT settings (horizontal and vertical positions and sizes).
// See documentation for the meaning of those values.
static short            crtPosH = 50;
static short            crtSizeH = 50;
static short            crtPosV = 50;
static short            crtSizeV = 50;

// Current space code (as described in GraphicsCard.h)
static ulong            scrnResCode = 0xFFFFFFFFL;

// Index of the current ResCode (index = log2(ResCode)).
static int              scrnResNum = 0;

// Base adress of the cursor shape buffer, as mapped in add-on memory space. 
static uchar            *scrnBufBase;

// Internal code for resolution. 
static long				scrnRes = vga640x480;

// Mask of available spaces with the current card.
static ulong            available_spaces;

// Position of the hot-spot in the current cursor shape.
static int              hotpt_h = -1000;
static int              hotpt_v = -1000;

// Last settings of the horizontal and vertical totals for CRT registers
static short            lastCrtHT, lastCrtVT;

// Base adress of the 64K io-space for chip's registers, as mapped in add-on
// memory space.
static volatile uchar   *isa_IO;

// Hardware cursor mode management
static int              CursorMode;



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

// This table is used for first initialization of the s3 chip.
// The first value (16 bits) is the adress of the register in the 64K io-space,
// the second (only one significative byte) is the value we set.
// Two pairs are often put on the same line, because many registers use indirect
// access (one register with many meanings, indexed by another register). The
// index always comes first.
static ushort	s3_table_first[] =
{
	0x3c4,  0x00,   0x3c5,  0x00,   // reset (not necessary)
	0x3c4,  0x00,   0x3c5,  0x03,

	0x46e8,	0x10,                   // go into setup mode
	0x102,	0x01,                   // turn chip enabled
	0x46e8,	0x08,                   // go back into normal mode

	0x000,	0x00                    // End-of-table
};

static ushort   hack_trio64vp[] =
{
	0x3b4,  0x38,   0x3b5,  0x48,
	0x3b4,  0x39,   0x3b5,  0xa5,
	0x3c4,  0x08,   0x3c5,  0x06,
	0x3b4,  0x37,   0x3b5,  0x0f,
	0x3c4,  0x14,   0x3c5,  0x03,
	0x3c4,  0x14,   0x3c5,  0x00,
	0x3c2,  0x0c,

    0x000,  0x00,
};

static ushort	s3_table[] =
{
	0x3c2,	0x2f,                   // enable memory access and color
	
	0x3d4,  0x38,   0x3d5,  0x48,   // unlock all protected registers...
	0x3d4,  0x39,   0x3d5,  0xa5,   // (all those unlocks are not necessary)
	0x3d4,  0x40,   0x3d5,  0x11,
	0x3d4,  0x55,   0x3d5,  0x00,
	0x3c4,  0x08,   0x3c5,  0x06,
	0x3d4,  0x11,   0x3d5,  0x0c,
	0x3d4,  0x33,   0x3d5,  0x02,
	0x3d4,  0x34,   0x3d5,  0x10,
	0x3d4,  0x35,   0x3d5,  0x00,
	0x3d8,  0x0b,   0x3d9,  0xf4,   // last unlock
	
	0x4ae8,	0x11,                   // turn chip enabled
	
    0x3b8,  0xff,                   // standard setting (probably useless)
	0x3bf,  0x03,                   // graphic mode (not text) 
	0x3da,	0x00,                   // standard setting (perhaps useless)
	
	0x000,	0x00                    // End-of-table
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
	0x3c4,	0x00,	0x3c5,	0x03,	// Reset
	0x3c4,	0x01,	0x3c5,	0x01,	// Clocking mode
	0x3c4,	0x02,	0x3c5,	0x0f,	// Enable write plane
	0x3c4,	0x03,	0x3c5,	0x00,	// Char Font Select
	0x3c4,	0x04,	0x3c5,	0x06,	// Memory Mode
	0x000,	0x00                    // End-of-table
};



//**************************************************************************
//	Tables for register settings, for each chip.
//**************************************************************************

//----------------------------------------------------------
// 964 table
//----------------------------------------------------------

static ushort	vga_964_table[] =
{
	0x3c4,	0x0d,	0x3c5,	0x00,	// Screen saving

	0x3d4,	0x31,	0x3d5,	0x8c,	// Memory config
	0x3d4,	0x32,	0x3d5,	0x00,	// Backward Compatibility 1
	0x3d4,	0x33,	0x3d5,	0x28,	// Backward Compatibility 2
	0x3d4,	0x34,	0x3d5,	0x10,	// Backward Compatibility 3
	0x3d4,	0x35,	0x3d5,	0x00,	// CRT Register Lock
	0x3d4,	0x37,	0x3d5,	0xff,	// Configuration 2
	0x3d4,	0x3a,	0x3d5,	0x15,	// Miscellaneous 1
	0x3d4,	0x3c,	0x3d5,	0x40,	// Interlace Retrace Start

	0x3d4,	0x40,	0x3d5,	0x11,	// System Configuration (?)
	0x3d4,	0x41,	0x3d5,	0x00,	// Bios Flag
	0x3d4,	0x42,	0x3d5,	0x02,	// Mode Control
	0x3d4,	0x43,	0x3d5,	0x00,	// Extended Mode
	0x3d4,	0x45,	0x3d5,	0x00,	// Hardware Graphics Cursor Mode

	0x3d4,	0x52,	0x3d5,	0x00,	// Extended BIOS Flag 1
	0x3d4,	0x53,	0x3d5,	0x00,	// Extended Memory Cont 1
	0x3d4,	0x55,	0x3d5,	0x00,	// Extended DAC Control
	0x3d4,	0x56,	0x3d5,	0x00,	// External Sync Cont 1
	0x3d4,	0x57,	0x3d5,	0x00,	// External Sync Cont 2
	0x3d4,	0x5b,	0x3d5,	0x88,	// Extended BIOS Flag 2
	0x3d4,	0x5c,	0x3d5,	0x22,	// General Out Port
	0x3d4,	0x5f,	0x3d5,	0x00,	// Bus Grant Termination Position
	0x3d4,	0x63,	0x3d5,	0x00,	// External Sync Delay Adjust High
	0x3d4,	0x64,	0x3d5,	0x00,	// Genlocking Adjustment
	0x3d4,	0x65,	0x3d5,	0x82,	// Extended Miscellaneous Control
	0x3d4,	0x66,	0x3d5,	0x03,	// Extended Miscellaneous Control 1
	0x3d4,	0x68,	0x3d5,	0xff,	// Configuration 3
	0x3d4,	0x6a,	0x3d5,	0x00,	// Extended System Control 4
	0x3d4,	0x6b,	0x3d5,	0x00,	// Extended BIOS flags 3
	0x3d4,	0x6c,	0x3d5,	0x00,	// Extended BIOS flags 4
	0x3d4,	0x6d,	0x3d5,	0x00,	// Extended Miscellaneous Control
	0x000,	0x00                    // End-of-table
};



//----------------------------------------------------------
// 864 table
//----------------------------------------------------------

static ushort	vga_864_table[] =
{
	0x3c4,	0x0d,	0x3c5,	0x00,	// Screen saving

	0x3d4,	0x31,	0x3d5,	0x8c,	// Memory config
	0x3d4,	0x32,	0x3d5,	0x00,	// Backward Compatibility 1
	0x3d4,	0x33,	0x3d5,	0x02,	// Backward Compatibility 2
	0x3d4,	0x34,	0x3d5,	0x10,	// Backward compatibility 3
	0x3d4,	0x35,	0x3d5,	0x00,	// CRT Register Lock
	0x3d4,	0x37,	0x3d5,	0xfb,	// Configuration 2
	0x3d4,	0x3a,	0x3d5,	0x95,	// Miscellaneous 1
	0x3d4,	0x3c,	0x3d5,	0x40,	// Interlace Retrace Start

	0x3d4,	0x40,	0x3d5,	0xd1,	// (d1)System Configuration (?)
	0x3d4,	0x42,	0x3d5,	0x03,	// (02)Mode Control
	0x3d4,	0x43,	0x3d5,	0x00,	// Extended Mode
	0x3d4,	0x45,	0x3d5,	0x04,	// (00)Hardware Graphics Cursor Mode

	0x3d4,	0x52,	0x3d5,	0x00,	// Extended BIOS Flag 1
	0x3d4,	0x53,	0x3d5,	0x00,	// Extended Memory Cont 1
	0x3d4,	0x55,	0x3d5,	0x40,	// (00)Extended DAC Control
	0x3d4,	0x56,	0x3d5,	0x10,	// (00)External Sync Cont 1
	0x3d4,	0x57,	0x3d5,	0x00,	// External Sync Cont 2
	0x3d4,	0x5b,	0x3d5,	0x88, 	// (00)Extended BIOS Flag 2
	0x3d4,	0x5c,	0x3d5,	0x03, 	// General Out Port
	0x3d4,	0x5f,	0x3d5,	0x00,	// Bus Grant Termination Position
	0x3d4,	0x63,	0x3d5,	0x00,	// External Sync Delay Adjust High
	0x3d4,	0x64,	0x3d5,	0x00,	// Genlocking Adjustment
	0x3d4,	0x65,	0x3d5,	0x02, 	// (00)Extended Miscellaneous Control
	0x3d4,	0x66,	0x3d5,	0x00, 	// Extended Miscellaneous Control 1
	0x3d4,	0x6a,	0x3d5,	0x00,	// Extended System Control 4
	0x3d4,	0x6b,	0x3d5,	0x00,	// Extended BIOS flags 3
	0x3d4,	0x6c,	0x3d5,	0x00,	// Extended BIOS flags 4
	0x3d4,	0x6d,	0x3d5,	0x02,	// Extended Miscellaneous Control
	0x000,	0x00	           		// End-of-table
};



//----------------------------------------------------------
// trio32
//
// Those settings are also used for the trio64. A few modifica-
// tions have been done to get that working with both chips.
// There is still problems with the trio32.
//----------------------------------------------------------

static ushort	vga_t32_table[] =
{
	0x3c4,  0x09,   0x3c5,  0x00,   // IO map enable
	0x3c4,  0x0a,   0x3c5,  0x80,   // External bus request
	0x3c4,  0x0d,   0x3c5,  0x00,   // Extended sequencer 1

	0x3d4,	0x31,	0x3d5,	0x8c,	// Memory config
	0x3d4,	0x32,	0x3d5,	0x00,	// all interrupt disenabled
	0x3d4,	0x33,	0x3d5,	0x02,	// Backward Compatibility 2
	0x3d4,	0x34,	0x3d5,	0x10,	// Backward compatibility 3
	0x3d4,	0x35,	0x3d5,	0x00,	// CRT Register Lock
	0x3d4,	0x37,	0x3d5,	0xfb,	// Configuration 2
	0x3d4,	0x3a,	0x3d5,	0x95,	// Miscellaneous 1
	0x3d4,	0x3c,	0x3d5,	0x40,	// Interlace Retrace Start

	0x3d4,	0x40,	0x3d5,	0x11,	// System Configuration
	0x3d4,	0x42,	0x3d5,	0x00,	// Mode Control (non interlace)
	0x3d4,	0x43,	0x3d5,	0x00,	// Extended Mode
	0x3d4,	0x45,	0x3d5,	0x00,	// Hardware Graphics Cursor Mode

	0x3d4,	0x52,	0x3d5,	0x00,	// Extended BIOS Flag 1
	0x3d4,	0x53,	0x3d5,	0x00,	// Extended Memory Cont 1
	0x3d4,	0x55,	0x3d5,	0x00,	// Extended DAC Control
	0x3d4,	0x56,	0x3d5,	0x00,	// External Sync Cont 1
	0x3d4,	0x57,	0x3d5,	0x00,	// External Sync Cont 2
	0x3d4,	0x5c,	0x3d5,	0x00, 	// General Out Port
	0x3d4,	0x63,	0x3d5,	0x00,	// External Sync Delay Adjust High
	0x3d4,	0x65,	0x3d5,	0x00, 	// Extended Miscellaneous Control
	0x3d4,	0x66,	0x3d5,	0x00, 	// Extended Miscellaneous Control 1
	0x3d4,	0x6a,	0x3d5,	0x00,	// Extended System Control 4
	0x3d4,	0x6b,	0x3d5,	0x00,	// Extended BIOS flags 3
	0x3d4,	0x6c,	0x3d5,	0x00,	// Extended BIOS flags 4
	0x000,	0x00			        // End-of-table
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
long   set_cursor_shape(uchar *data,uchar *mask,long dim_h,long dim_v,long hot_h,long hot_v);
long   move_cursor(long new_h,long new_v);
long   show_cursor(bool state);
long   line_8(long x1, long x2, long y1, long y2, uchar color,
			   bool useClip, short clipLeft, short clipTop, short clipRight, short clipBottom);
long   line_32(long x1, long x2, long y1, long y2, ulong color,
				bool useClip, short clipLeft, short clipTop, short clipRight, short clipBottom);
long   rect_8(long x1, long y1, long x2, long y2, uchar color);
long   rect_32(long x1, long y1, long x2, long y2, ulong color);
long   blit(long x1, long y1, long x2, long y2, long width, long height);
long   invert_rect_32(long x1, long y1, long x2, long y2);

// Hook table in the standard case (for 864). As most of the accelerated
// functions are compatible with every s3 chips (except cursor calls), we
// have only one hook table that we reduce or modify when we copy it in
// the application server table (with the GET_GRAPHICS_HOOKS call).
static graphics_card_hook vga_dispatch[B_HOOK_COUNT] = {
// 0
	(graphics_card_hook)set_cursor_shape,
	(graphics_card_hook)move_cursor,
	(graphics_card_hook)show_cursor,
	(graphics_card_hook)line_8,
	(graphics_card_hook)line_32,
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

//**************************************************************************
//  Standard input/output functions to access io regsters
//**************************************************************************

// Write a value in a 8 bits register at a specified address
static void	outp(long address, uchar value)
{
	volatile    int i;

	for (i=0;i<6;i++);
#if __INTEL__
	write_isa_io (0, (char *)address, 1, value);
#else
	*(vuchar *)ISA_ADDRESS(address) = value;
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

// Write a value in a 16 bits register at a specified address
// (respecting intel byte order).
static void	outpw(long address, ushort value)
{
	ushort	tmp;
	volatile    int i;

	for (i=0;i<6;i++);
#if __INTEL__
	write_isa_io (0, (char *)address, 2, value);
#else
	tmp = ((value & 0xff) << 8) | ((value >> 8) & 0xff);
	*(vushort *)ISA_ADDRESS(address) = tmp;
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

// Write a value in a 32 bits register at a specified address.
// (don't respect intel byte order, only for memory to register copy)
static void	outpl(long address, ulong value)
{
	volatile    int i;

	for (i=0;i<6;i++);
#if __INTEL__
	write_isa_io (0, (char *)address, 4, value);
#else
	*(vulong *)ISA_ADDRESS(address) = value;
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
static long	    io_lock, ge_lock;

// Create the benaphores (if possible)
void init_locks()
{
	// turn on debuging
//	set_dprintf_enabled(1);

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

// Stupid delay loop for very short times (snooze is not working in those
// cases and it's better to do some stupid loop into the processor than
// always accessing the bus to look for the end of the current process).
void delay(volatile long i)
{
	while(i--);
}


// For long delays, it's better to use snooze. 2000 is already less than the
// current minimal amount of time managed by standard scheduling.
void wait_vga_hw_big(void)
{
	do {
		snooze(2000);	
	} while (inpw(GP_STAT) & 0x200);
}


// Use the previous delay() function for fast operation. Those functions are
// used to wait for terminaison of current hardware operation.
void wait_vga_hw(void)
{
	while (inpw(GP_STAT) & 0x200)
		delay(40);	
}


// Stroke a line in 8 bits mode. (see documentation for more informations)
long line_8(long   x1,         // Coordinates of the two extremities
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
	short		command;
	short		min, max;
	long        abs_dx, abs_dy;

// This call is using the graphic engine and access video memory, so we need to
// get the lock graphic-engine benaphore.
	lock_ge();
	
// Refer to any s3 chip databook (864, 964, trio32, trio64 and more) to get
// more information about specific graphic engine operation.
// The cliping rectangle is discribed with 12 bits values.
	if (useClip) {
		outpw(DATA_REG, SCISSORS_T | (clipTop & 0xfff));
		outpw(DATA_REG, SCISSORS_L | (clipLeft & 0xfff));
		outpw(DATA_REG, SCISSORS_B | (clipBottom & 0xfff));
		outpw(DATA_REG, SCISSORS_R | (clipRight & 0xfff));
	}
	else {
		outpw(DATA_REG, SCISSORS_T | 0);
		outpw(DATA_REG, SCISSORS_L | 0);
		outpw(DATA_REG, SCISSORS_B | 0x7ff);
		outpw(DATA_REG, SCISSORS_R | 0x7ff);
	}

// Standard comand for x and y negative directions, and x major axis.
	command = 0x2413;

// Check for positive y direction.
	if (y1 < y2)
		command |= 0x80;
	
// Calculate vector coordinates...
	abs_dx = abs(x2-x1);
	abs_dy = abs(y2-y1);
	
// ...and check orientation.
	if (abs_dx > abs_dy) {
		max = abs_dx;
		min = abs_dy;
	}
	else {
		max = abs_dy;
		min = abs_dx;
  // Command correction when y is the major axis.
		command |= 0x40;
	}

// Wait for 3 empty FIFO slots.
	if (inpw(CMD) & 0x20)
		do {
			delay(40);
		} while (inpw(CMD) & 0x20);

// Select mode code (copy), color and reset pixel control.
	outpw(FRGD_MIX, 0x27);
	outp(FRGD_COLOR, color);
	outpw(DATA_REG, PIX_CNTL);

// Wait for 7 empty FIFO slots.
	if (inpw(CMD) & 0x02)
		do {
			delay(40);
		} while (inpw(CMD) & 0x02);

// Send geometric description of the line in graphic engine format
	outpw(CUR_X, x1);
	outpw(CUR_Y, y1);
	outpw(MAJ_AXIS_PCNT, max);
	outpw(DESTX_DIASTP, 2 * (min - max));
	outpw(DESTY_AXSTP, 2 * min);
	if ((x2 - x1) > 0) {
		outpw(ERR_TERM, 2 * min - max);
  // Command correction when x direction is positive.
		command |= 0x20;
	}
	else
		outpw(ERR_TERM, 2 * min - max - 1);
	
// Run the operation
	outpw(CMD, command);

// Wait for terminaison (this operation is synchronous in the current
// implementation).  
	wait_vga_hw();

// Release the benaphore of the graphic engine
	unlock_ge();
	return B_NO_ERROR;
}


// Stroke a line in 32 bits mode. (see documentation for more informations)
// NB: this call will probably be used in future version for 16 bits line too.
long line_32(long   x1,         // Coordinates of the two extremities
			 long   x2,         //
			 long   y1,         //
			 long   y2,         //
			 ulong  color,      // RGB color
			 bool   useClip,    // Clipping enabling
			 short  clipLeft,   // Definition of the cliping rectangle if 
			 short  clipTop,    // cliping enabled
			 short  clipRight,  //
			 short  clipBottom) //
{
	short		command;
	short		min, max;
	long        abs_dx, abs_dy;

// This call is using the graphic engine and access video memory, so we need to
// get the lock graphic-engine benaphore.
	lock_ge();

// Refer to any s3 chip databook (864, 964, trio32, trio64 and more) to get
// more information about specific graphic engine operation.
// The cliping rectangle is discribed with 12 bits values.
	if (useClip) {
		outpw(DATA_REG, SCISSORS_T | (clipTop & 0xfff));
		outpw(DATA_REG, SCISSORS_L | (clipLeft & 0xfff));
		outpw(DATA_REG, SCISSORS_B | (clipBottom & 0xfff));
		outpw(DATA_REG, SCISSORS_R | (clipRight & 0xfff));
	}
	else {
		outpw(DATA_REG, SCISSORS_T | 0);
		outpw(DATA_REG, SCISSORS_L | 0);
		outpw(DATA_REG, SCISSORS_B | 0x7ff);
		outpw(DATA_REG, SCISSORS_R | 0x7ff);
	}
	
// Standard comand for x and y negative directions, and x major axis.
	command = 0x2413;
	
// Check for positive y direction.
	if (y1 < y2)
		command |= 0x80;

// Calculate vector coordinates...
	abs_dx = abs(x2-x1);
	abs_dy = abs(y2-y1);

// ...and check orientation.
	if (abs_dx > abs_dy) {
		max = abs_dx;
		min = abs_dy;
	}
	else {
		max = abs_dy;
		min = abs_dx;
  // Command correction when y is the major axis.
		command |= 0x40;
	}
	
// Wait for 3 empty FIFO slots.
	if (inpw(CMD) & 0x20)
		do {
			delay(40);
		} while (inpw(CMD) & 0x20);

// Select mode code (copy), color and reset pixel control.
//
// NB : Be careful about color definition : the four channels (Blue, Gree, Red
// and Alpha) are put in memory in the order described in the rgba_order in
// graphics_card_info, and so the ulong you receive is ordered so that writting
// it into memory would put the bytes in the good order. So the order of the
// channels in the long depends of the endianness of the processor.
// For example, on the PPC Bebox, if rgba_order is "rgba", then the red channel
// is coded in the highest byte of the ulong (0x##000000), as we're using
// big-endian. 
	outpw(FRGD_MIX, 0x27);
	outpl(FRGD_COLOR, color);
	outpw(DATA_REG, PIX_CNTL);

// Wait for 7 empty FIFO slots.
	if (inpw(CMD) & 0x02)
		do {
			delay(40);
		} while (inpw(CMD) & 0x02);
	
// Send geometric description of the line in graphic engine format
	outpw(CUR_X, x1);
	outpw(CUR_Y, y1);
	outpw(MAJ_AXIS_PCNT, max);
	outpw(DESTX_DIASTP, 2 * (min - max));
	outpw(DESTY_AXSTP, 2 * min);
	if ((x2 - x1) > 0) {
		outpw(ERR_TERM, 2 * min - max);
  // Command correction when x direction is positive.
		command |= 0x20;
	}
	else
		outpw(ERR_TERM, 2 * min - max - 1);
	
// Run the operation
	outpw(CMD, command);
	
// Wait for terminaison (this operation is synchronous in the current
// implementation).  
	wait_vga_hw();

// Release the benaphore of the graphic engine
	unlock_ge();
	return B_NO_ERROR;
}


// Fill a rect in 8 bits (see documentation for more informations)
long rect_8(long  x1,    // The rect to fill. Call will always respect
			long  y1,    // x1 <= x2, y1 <= y2 and the rect will be
			long  x2,    // completly in the current screen space (no
			long  y2,    // cliping needed).
			uchar color) // Indexed color.
{
// This call is using the graphic engine and access video memory, so we need to
// get the lock graphic-engine benaphore.
	lock_ge();

// Refer to any s3 chip databook (864, 964, trio32, trio64 and more) to get
// more information about specific graphic engine operation.
// Disable the cliping rectangle.
	outpw(DATA_REG, SCISSORS_T | 0);
	outpw(DATA_REG, SCISSORS_L | 0);
	outpw(DATA_REG, SCISSORS_B | 0x7ff);
	outpw(DATA_REG, SCISSORS_R | 0x7ff);
	
// Wait for 3 empty FIFO slots.
	if (inpw(CMD) & 0x20)
	  do {
		delay(40);
	  } while (inpw(CMD) & 0x20);
	
// Select mode code (copy), color and reset pixel control.
	outpw(FRGD_MIX, 0x27);
	outp(FRGD_COLOR, color);
	outpw(DATA_REG, PIX_CNTL);

// Wait for 5 empty FIFO slots.
	if (inpw(CMD) & 0x08)
		do {
			delay(40);
		} while (inpw(CMD) & 0x08);
	
// Send geometric description of the rect in graphic engine format
	outpw(CUR_X, x1);
	outpw(CUR_Y, y1);
	outpw(MAJ_AXIS_PCNT, x2 - x1);
	outpw(DATA_REG, MIN_AXIS_PCNT | (y2 - y1));

// Run the operation
	outpw(CMD, 0x40b3);
	
// Wait for terminaison (this operation is synchronous in the current
// implementation).  
	wait_vga_hw();
	
// Release the benaphore of the graphic engine
	unlock_ge();
	return B_NO_ERROR;
}


// Fill a rect in 32 bits (see documentation for more informations)
long rect_32(long  x1,    // The rect to fill. Call will always respect
			 long  y1,    // x1 <= x2, y1 <= y2 and the rect will be
			 long  x2,    // completly in the current screen space (no
			 long  y2,    // cliping needed).
			 ulong color) // Rgb color.
{
// This call is using the graphic engine and access video memory, so we need to
// get the lock graphic-engine benaphore.
	lock_ge();
	
// Refer to any s3 chip databook (864, 964, trio32, trio64 and more) to get
// more information about specific graphic engine operation.
// Disable the cliping rectangle.
	outpw(DATA_REG, SCISSORS_T | 0);
	outpw(DATA_REG, SCISSORS_L | 0);
	outpw(DATA_REG, SCISSORS_B | 0x7ff);
	outpw(DATA_REG, SCISSORS_R | 0x7ff);
	
// Wait for 3 empty FIFO slots.
	if (inpw(CMD) & 0x20)
		do {
			delay(40);
		} while (inpw(CMD) & 0x20);
	
// Select mode code (copy), color and reset pixel control.
//
// NB : Be careful about color definition : the four channels (Blue, Gree, Red
// and Alpha) are put in memory in the order described in the rgba_order in
// graphics_card_info, and so the ulong you receive is ordered so that writting
// it into memory would put the bytes in the good order. So the order of the
// channels in the long depend of the endianness of the processor.
// For example, on the PPC Bebox, if rgba_order is "rgba", then the red channel
// is coded in the highest byte of the ulong (0x##000000), as we're using
// big-endian. 
	outpw(FRGD_MIX, 0x27);
	outpl(FRGD_COLOR, color);
	outpw(DATA_REG, PIX_CNTL);

// Wait for 5 empty FIFO slots.
	if (inpw(CMD) & 0x08)
		do {
			delay(40);
		} while (inpw(CMD) & 0x08);
	
// Send geometric description of the rect in graphic engine format
	outpw(CUR_X, x1);
	outpw(CUR_Y, y1);
	outpw(MAJ_AXIS_PCNT, x2 - x1);
	outpw(DATA_REG, MIN_AXIS_PCNT | (y2 - y1));

// Run the operation
	outpw(CMD, 0x40b3);
	
// Wait for terminaison (this operation is synchronous in the current
// implementation).  
	wait_vga_hw();

// Release the benaphore of the graphic engine
	unlock_ge();
	return B_NO_ERROR;
}


// Blit a rect from screen to screen (see documentation for more informations)
long blit(long  x1,     // top-left point of the source
		  long  y1,     //
		  long  x2,     // top-left point of the destination
		  long  y2,     //
		  long  width,  // size of the rect to move (from border included to
		  long  height) // opposite border included).
{
	short	command;
	short	srcx, srcy, destx, desty;

// Check degenerated blit (source == destination)
	if ((x1 == x2) && (y1 == y2)) return B_NO_ERROR;

// This call is using the graphic engine and access video memory, so we need to
// get the lock graphic-engine benaphore.
	lock_ge();

// Refer to any s3 chip databook (864, 964, trio32, trio64 and more) to get
// more information about specific graphic engine operation.
// Standard command for negative x, negative y 
	command = 0xc013;

// Convert application server width and height (as for BRect object) into
// real width and height
	width += 1;
	height += 1;

// Check if source and destination are not linked 
	if ((x2 > (x1 + width)) ||
		((x2 + width) < x1) ||
		(y2 > (y1 + height) ||
		 (y2 + height) < y1)) {
		srcx = x1;
		srcy = y1;
		destx = x2;
		desty = y2;
  // Command correction when positive x and positive y.
		command |= 0xa0;
	}
// In the other case, copy in the correct order
	else {
		if (x1 > x2) {
  // Command correction when positive x.
			command |= 0x20;
			srcx = x1;
			destx = x2;
		}
		else {
			srcx = x1 + width - 1;
			destx = x2 + width - 1;
		}
		if (y1 > y2) {
  // Command correction when positive y.
			command |= 0x80;
			srcy = y1;
			desty = y2;
		}
		else {
			srcy = y1 + height - 1;
			desty = y2 + height - 1;
		}
	}
	
// Disable the cliping rectangle.
	outpw(DATA_REG, SCISSORS_T | 0);
	outpw(DATA_REG, SCISSORS_L | 0);
	outpw(DATA_REG, SCISSORS_B | 0x7ff);
	outpw(DATA_REG, SCISSORS_R | 0x7ff);
	
// Wait for 2 empty FIFO slots.
	if (inpw(CMD) & 0x40)
		do {
			delay(40);
		} while (inpw(CMD) & 0x40);
	
// Select mode code (display memory source) and reset pixel control.
	outpw(FRGD_MIX, 0x67);
	outpw(DATA_REG, PIX_CNTL);
	
// Wait for 7 empty FIFO slots.
	if (inpw(CMD) & 0x02)
		do {
			delay(40);
		} while (inpw(CMD) & 0x02);
	
// Send geometric description of the blit in graphic engine format
	outpw(CUR_X, srcx);
	outpw(CUR_Y, srcy);
	outpw(DESTX_DIASTP, destx);
	outpw(DESTY_AXSTP, desty);
	outpw(MAJ_AXIS_PCNT, width - 1);
	outpw(DATA_REG, MIN_AXIS_PCNT | ((height - 1) & 0xfff));

// Run the operation
	outpw(CMD, command);
	
// Wait for terminaison (this operation is synchronous in the current
// implementation). For big blit, the delay function is calling snooze to
// free some CPU time.
	if ((width*height) > 25000)
		wait_vga_hw_big();
	else
		wait_vga_hw();

// Release the benaphore of the graphic engine
	unlock_ge();
	return B_NO_ERROR;
}


// This table is used to expand a byte xxxx0123 into the byte 00112233.
static uchar expand2[16] = {
	0x00,0x03,0x0c,0x0f,
	0x30,0x33,0x3c,0x3f,
	0xc0,0xc3,0xcc,0xcf,
	0xf0,0xf3,0xfc,0xff
};


// Change the bitmap shape of the hardware cursor (see documentation for
// more informations).
long set_cursor_shape(uchar *data, // XorMask
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
	if (hotpt_h == -1000) init = TRUE;
	else init = FALSE;

// Memorize the new hot-spot.
	hotpt_h = hot_h;
	hotpt_v = hot_v;

// Initialise the bitmap hardware cursor as completly transparent
	buf = (uchar*)scrnBufBase;
	for (i=0;i<1024;i+=4) {
		buf[i+2] = buf[i+3] = 0x00;
		buf[i+0] = buf[i+1] = 0xff;
	}

// Select the CursorMode depending of the chip and the space
	if (scrnColors == 8) {
		switch (theVGA) {
//		case s3_868:
//			break;
//		case s3_964:
//			break;
//		case s3_968:
//			break;
		case s3_864:
			for (i=0;i<dim_v;i++)
				for (j=0;j<dim_h0;j++) {
  // As the invert mode is not currently working (because of the current value
  // of the color map), the Invert display color mode is mapped on black color.
  // NB : be careful, old databook are wrong about the arrangement of the two
  // planes describing the hardware cursor shape for a S3 chip.
					buf[2+i*16+j+(j&6)] = data[j+i*dim_h0];
					buf[0+i*16+j+(j&6)] = mask[j+i*dim_h0]&(0xff^data[j+i*dim_h0]);
				}
			outp(0x3d4,0x0e);
			outp(0x3d5,0x00);
			outp(0x3d4,0x0f);
			outp(0x3d5,0xff);
			break;
		case s3_trio32:
		case s3_trio64:
		case s3_trio64vp:
			for (i=0;i<dim_v;i++)
				for (j=0;j<dim_h0;j++) {
  // As the invert mode is not currently working (because of the current value
  // of the color map), the Invert display color mode is mapped on black color.
  // NB : be careful, old databook are wrong about the arrangement of the two
  // planes describing the hardware cursor shape for a S3 chip.
					buf[2+i*16+j+(j&6)] = data[j+i*dim_h0];
					buf[0+i*16+j+(j&6)] = mask[j+i*dim_h0]&(0xff^data[j+i*dim_h0]);
				}
			outp(0x3d4,0x0e);
			outp(0x3d5,0xff);
			outp(0x3d4,0x0f);
			outp(0x3d5,0x00);
			break;
		}
		CursorMode = 0;
	} else if (scrnColors == 32) {
		switch (theVGA) {
		case s3_864:
			hotpt_h *= 2;
			for (i=0;i<dim_v;i++)
				for (j=0;j<dim_h0;j++) { 
  // The invert mode is working well in 32 bits, but for reason of cooherency,
  // this mode is mapped to black color as in 8 bits.
  // NB : big surprise on s3 chips : when you set the hardware cursor in 32 bits 
  // mode, you get some strange thing in 16 bits !! (perhaps this setting is
  // incorrect, but we don't know why...) So we have to duplicate each pixel
  // and trick the color definition to get approximatly the good shape...
					d0 = data[j+i*dim_h0];
					m0 = mask[j+i*dim_h0]&(0xff^data[j+i*dim_h0]);
					d1 = expand2[d0&15];
					d0 = expand2[d0>>4];
					m1 = expand2[m0&15];
					m0 = expand2[m0>>4];
					buf[2+i*16+j*4] = d0;
					buf[0+i*16+j*4] = m0;
					buf[3+i*16+j*4] = d1;
					buf[1+i*16+j*4] = m1;
				}
			CursorMode = 1;
			break;
//		case s3_868:
//			break;
//		case s3_964:
//			break;
//		case s3_968:
//			break;
		case s3_trio32:
		case s3_trio64:
		case s3_trio64vp:
			for (i=0;i<dim_v;i++)
				for (j=0;j<dim_h0;j++) {
  // As the invert mode is not currently working (because of the current value
  // of the color map), the Invert display color mode is mapped on black color.
  // NB : be careful, old databook are wrong about the arrangement of the two
  // planes describing the hardware cursor shape for a S3 chip.
					buf[2+i*16+j+(j&6)] = data[j+i*dim_h0];
					buf[0+i*16+j+(j&6)] = mask[j+i*dim_h0]&(0xff^data[j+i*dim_h0]);
				}
			CursorMode = 0;
			break;
		}
	}
	
// Set the two significant colors (black and white)
	outp(0x3d4,0x45);
	inp(0x3d5);
	outp(0x3d4,0x4b);
	outp(0x3d5,0xff);
	outp(0x3d4,0x4b);
	outp(0x3d5,0xff);
	outp(0x3d4,0x4b);
	outp(0x3d5,0xff);
	outp(0x3d4,0x4b);
	outp(0x3d5,0xff);
	outp(0x3d4,0x45);
	inp(0x3d5);
	outp(0x3d4,0x4a);
	outp(0x3d5,0);
	outp(0x3d4,0x4a);
	outp(0x3d5,0);
	outp(0x3d4,0x4a);
	outp(0x3d5,0);
	outp(0x3d4,0x4a);
	outp(0x3d5,0);
	
// Set the pointer to the hardware cursor data.
	outp(0x3d4,0x4c);
	outp(0x3d5,(((char*)scrnBufBase-(char*)scrnBase)>>18)&7);
	outp(0x3d4,0x4d);
	outp(0x3d5,(((char*)scrnBufBase-(char*)scrnBase)>>10)&255);

// If it's the first definition of the cursor shape, initialise the default
// position and the default visibility mode.
	if (init) {
		outp(0x3d4,0x46);
		outp(0x3d5,0);
		outp(0x3d4,0x47);
		outp(0x3d5,0);
		outp(0x3d4,0x49);
		outp(0x3d5,0);
		outp(0x3d4,0x4e);
		outp(0x3d5,0);
		outp(0x3d4,0x4f);
		outp(0x3d5,0);
		outp(0x3d4,0x48);
		outp(0x3d5,0);
		outp(0x3d4,0x45);
		outp(0x3d5,0x00);
	}
	
// Release the graphic engine benaphore and the benaphore of the io-registers.
	unlock_ge();
	unlock_io();
	return B_NO_ERROR;
}
	

// Move the hardware cursor to a new position of the hot_spot (see documentation
// for more informations).
long move_cursor(long new_h, // New hot_spot coordinates
				 long new_v) //
{
	long h, v, dh, dv;

// This call is using only the io-register (not the graphic engine). So we need
// to get only the lock io-register benaphore.
	lock_io();

// Because of the silly bug with 32/16 bits hardware cursor, in 32 bits we need
// to multiply the horizontal coordinate by 2 (as the display cursor is really
// insert in 16 bits for a 32 bits display).
	if (CursorMode == 1)
		new_h *= 2;

// Calculate the part of the cursor masked by the left border of the screen
	if (hotpt_h > new_h) {
		dh = hotpt_h-new_h;
		h = 0;
	}
	else {
		dh = 0;
		h = new_h-hotpt_h;
	}

// Calculate the part of the cursor masked by the top border of the screen	
	if (hotpt_v > new_v) {
		dv = hotpt_v-new_v;
		v = 0;
	}
	else {
		dv = 0;
		v = new_v-hotpt_v;
	}

// Set position and offset (for top-left border cliping) of the hardware cursor
// The order of the writing to the register (the high part of v at the end) is
// necessary to get a smooth move. In other orders, you could get a bad position
// during on frame from time to time. Don't ask why.
	outp(0x3d4,0x46);
	outp(0x3d5,(h>>8)&7);
	outp(0x3d4,0x47);
	outp(0x3d5,(h)&255);
	outp(0x3d4,0x49);
	outp(0x3d5,(v)&255);
	outp(0x3d4,0x4e);
	outp(0x3d5,dh);
	outp(0x3d4,0x4f);
	outp(0x3d5,dv);
	outp(0x3d4,0x48);
	outp(0x3d5,(v>>8)&7);
	
// Release the benaphore of the io-registers.
	unlock_io();
	return B_NO_ERROR;
}


// This function is used to show or hide the hardware cursor (see documentation
// for more informations).
long show_cursor(bool state)
{
// This call is using only the io-register (not the graphic engine). So we need
// to get the lock io-register benaphore.
	lock_io();

// Show the cursor
	if (state != 0) {
		outp(0x3d4,0x45);
		outp(0x3d5,0x01);
	}
	
// Hide the cursor
	else {
		outp(0x3d4,0x45);
		outp(0x3d5,0x00);
	}
	
// Release the benaphore of the io-registers.
	unlock_io();
	return B_NO_ERROR;
}


// This function is use to inverse a rectangle in 32 bits mode.
long invert_rect_32(long x1, long y1, long x2, long y2)
{
// This call is using the graphic engine and access video memory, so we need to
// get the lock graphic-engine benaphore.
	lock_ge();
	
// Refer to any s3 chip databook (864, 964, trio32, trio64 and more) to get
// more information about specific graphic engine operation.
// Disable the cliping rectangle.
	outpw(DATA_REG, SCISSORS_T | 0);
	outpw(DATA_REG, SCISSORS_L | 0);
	outpw(DATA_REG, SCISSORS_B | 0x7ff);
	outpw(DATA_REG, SCISSORS_R | 0x7ff);
	
// Wait for 2 empty FIFO slots.
	if (inpw(CMD) & 0x20)
		do {
			delay(40);
		} while (inpw(CMD) & 0x20);
	
// Select mode code (invert) and reset pixel control.
	outpw(FRGD_MIX, 0x00);
	outpw(DATA_REG, PIX_CNTL);

// Wait for 5 empty FIFO slots.
	if (inpw(CMD) & 0x08)
		do {
			delay(40);
		} while (inpw(CMD) & 0x08);
	
// Send geometric description of the rect in graphic engine format
	outpw(CUR_X, x1);
	outpw(CUR_Y, y1);
	outpw(MAJ_AXIS_PCNT, x2 - x1);
	outpw(DATA_REG, MIN_AXIS_PCNT | (y2 - y1));

// Run the operation
	outpw(CMD, 0x40b3);
	
// Wait for terminaison (this operation is synchronous in the current
// implementation).  
	wait_vga_hw();

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
	uchar	v;

// 0x3c0 is the only register used at the same time as index and data register.
// So we've to be careful to stay synchronize (the card switch between index
// and data at each write. Reading 0x3da give us a simple way to stay well-
// synchronized.
	inp(0x3da);
	
// Save the initial index set in 0x3c0
	v = inp(0x3c0);

	while (TRUE) {
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

	while (TRUE) {
		p1 = *ptr++;
		p2 = *ptr++;
		if (p1 == 0 && p2 == 0) {
			return;
		}
		outp(p1, p2);
	}
}


// All the other table settings are done with that very simple function.
static void	set_table_slow(ushort *ptr)
{
	ushort	p1;
	ushort	p2;
	volatile int i,j;

	while (TRUE) {
		p1 = *ptr++;
		p2 = *ptr++;
		if (p1 == 0 && p2 == 0) {
			return;
		}
		j = 0;
		for (i=0;i<2000;i++) j++;
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
static void	setup_dac(void)
{
// To set the mode, we need to access the DAC hidden register using multiple
// access to 0x3c6 to unlock it. 
	inp(0x3c6);
	inp(0x3c6);
	inp(0x3c6);
	inp(0x3c6);
	outp(0x3c6, 0x00);

// After that, we reset the 0x3c6 counter protecting the hidden register.	
	inp(0x3c8);

// And then we disable the write mask.	
	outp(0x3c6, 0xff);
}


// ##----> THOSE SETTINGS SHOULD BE COMPATIBLE WITH ALL VGA CHIPS. <----##
// Set the DAC in the standard 32 bits (rgb color) mode.
static void	setup_dac32(void)
{
// To set the mode, we need to access the DAC hidden register using multiple
// access to 0x3c6 to unlock it. 
	inp(0x3c6);
	inp(0x3c6);
	inp(0x3c6);
	inp(0x3c6);
	outp(0x3c6, 0x56);

// After that, we reset the 0x3c6 counter protecting the hidden register.	
	inp(0x3c8);

// And then we disable the write mask.	
	outp(0x3c6, 0xff);
}



//**************************************************************************
//  TVP 3025 specific stuff.
//
//  This is a DAC and clock generator used with 964 chips.
//**************************************************************************


static float tvp_ScreenClockStep[32] =
{
	8.0, 8.0, 8.0, 8.0, 8.0,
	0.0, 0.0, 0.0, 0.0, 0.0,
	8.0, 8.0, 0.0, 0.0, 0.0,
	8.0, 0.0, 0.0, 0.0, 0.0,
	0.0, 0.0, 0.0, 0.0, 0.0,
	0.0, 0.0, 0.0, 0.0, 0.0,
	0.0, 0.0,
};


// Read an 8 bits value from a specific TVP 3025 registers, known by its index
// in the TVP 3025 register table (see the TVP 3025 databook and any standard
// s3 chip databook for more informations).
static uchar tvp_in(uchar reg)
{
	uchar	tmp, tmp1, ret;

	outp(0x3d4, 0x55);
	tmp = inp(0x3d5) & 0xfc;
	outp(0x3d5, tmp | 0x01);
	tmp1 = inp(TI_INDEX_REG);
	outp(TI_INDEX_REG, reg);
	ret = inp(TI_DATA_REG);
	outp(TI_INDEX_REG, tmp1);
	outp(0x3d5, tmp);
	return(ret);
}


// Write bits in a specific TVP 3025 registers, known by is index. The value is
// in data and the insertion mask in mask. If (mask == 0), the whole byte is
// written (see the TVP 3025 databook and any standard s3 chip databook for
// more informations).
static void	tvp_out(uchar reg, uchar mask, uchar data)
{
	uchar	tmp, tmp1, tmp2;

	tmp2 = 0;

	outp(0x3d4, 0x55);
	tmp = inp(0x3d5) & 0xfc;
	outp(0x3d5, tmp | 0x01);
	tmp1 = inp(TI_INDEX_REG);
	outp(TI_INDEX_REG, reg);

	if (mask != 0x00) {
		tmp2 = inp(TI_DATA_REG) & mask;
	}
	outp(TI_DATA_REG, tmp2 | data);

	outp(TI_INDEX_REG, tmp1);
	outp(0x3d5, tmp);
}


long encrypt_tvp_clock(float freq);
// Set the frequency of the video clock generator of the TVP3025. And modify
// the pixel-clock generator for adjustement (see the TVP3025 databook for
// more information about pixel-clock generator).
void tvp_set_clock(float clock)
{
	uchar	tmp;
	ulong	setup;
	float	lclk_adjust;

	ddprintf("tvp_set_clock(%f)\n", clock);
// Unlock S3 extended registers (probably not necessary).	
	outp(0x3d4, 0x38);
	outp(0x3d5, 0x48);
	
	outp(0x3d4, 0x39);
	outp(0x3d5, 0xa5);

// Prepare use of register 0x5c to communicate with TVP3025
	outp(0x3d4, 0x5c);
	tmp = inp(0x3d5);
	outp(0x3d5, tmp & 0xdf);

// Standard configuration of TVP3025 (depth dependent)
	if (scrnColors == 8) {
		//tvp_out(TI_MUX_CONTROL_1, 0x00, 0xc6);
		tvp_out(TI_MUX_CONTROL_1, 0x00, 0x80);
		tvp_out(TI_GENERAL_IO_CONTROL, 0x00, 0x1f);
		tvp_out(TI_TRUE_COLOR_CONTROL, 0x00, 0x00);
		//tvp_out(TI_TRUE_COLOR_CONTROL, 0x00, 0x1f);
		//tvp_out(TI_OUTPUT_CLOCK_SELECT, 0x00, 0x92);
		tvp_out(TI_OUTPUT_CLOCK_SELECT, 0x00, 0x13);
		//tvp_out(TI_MISC_CONTROL, 0x00, 0xc4);
		tvp_out(TI_MISC_CONTROL, 0x00, 0xc4);
		lclk_adjust = 8;
	}
	else {
		//tvp_out(TI_MUX_CONTROL_1, 0x00, 0x0f);
		tvp_out(TI_MUX_CONTROL_1, 0x00, 0x06);
		tvp_out(TI_GENERAL_IO_CONTROL, 0x00, 0x00);
		tvp_out(TI_TRUE_COLOR_CONTROL, 0x00, 0xce);
		//tvp_out(TI_TRUE_COLOR_CONTROL, 0x00, 0x00);
		tvp_out(TI_OUTPUT_CLOCK_SELECT, 0x00, 0x80);
		tvp_out(TI_MISC_CONTROL, 0x00, 0xcc);
		lclk_adjust = 32;
	}
		
// Standard configuration of TVP3025 (beginning)
	tvp_out(TI_MUX_CONTROL_2, 0x00, 0x1c);
	
	tvp_out(TI_INPUT_CLOCK_SELECT, 0x00, 0x01);
	tvp_out(TI_GENERAL_CONTROL, 0x00, 0x00);
	tvp_out(TI_AUXILLARY_CONTROL, 0x00, 0x01);
	
	tvp_out(TI_GENERAL_IO_DATA, 0x00, 0x00);

// Main video clock setting.
	if (clock > 135000000.0) clock = 135000000.0;
	setup = encrypt_tvp_clock(clock * 1e-6);
	ddprintf("NMP: 0x%06x\n", setup);
	tvp_out(TI_PLL_CONTROL, 0x00, 0x00);
	tvp_out(TI_PIXEL_CLOCK_PLL_DATA, 0x00, (setup>>16)&127);
	tvp_out(TI_PIXEL_CLOCK_PLL_DATA, 0x00, (setup>>8)&127);
	tvp_out(TI_PIXEL_CLOCK_PLL_DATA, 0x00, (setup&0x0b)|0x88);

// Memory clock standard setting (equivalent to 60 Mhz standard setting for
// other DAC).
	tvp_out(TI_MCLK_PLL_DATA, 0x00, 0x01);
	tvp_out(TI_MCLK_PLL_DATA, 0x00, 0x01);
	tvp_out(TI_MCLK_PLL_DATA, 0x00, 0x80);

// Pixel-clock setting and adjustement.
	//setup = encrypt_tvp_clock(clock * 1e-6 / lclk_adjust);
	//ddprintf(("NMP: 0x%06x\n", setup));
	tvp_out(TI_LOOP_CLOCK_PLL_DATA, 0x00, 0x01);
	tvp_out(TI_LOOP_CLOCK_PLL_DATA, 0x00, 0x01);
	tvp_out(TI_LOOP_CLOCK_PLL_DATA, 0x00, (setup&0x03)|0x80);
	//tvp_out(TI_LOOP_CLOCK_PLL_DATA, 0x00, (setup>>16)&127);
	//tvp_out(TI_LOOP_CLOCK_PLL_DATA, 0x00, (setup>>8)&127);
	//tvp_out(TI_LOOP_CLOCK_PLL_DATA, 0x00, (setup&0x0b)|0x80);
	/*
	tvp_out(TI_LOOP_CLOCK_PLL_DATA, 0x00, 0x01);
	tvp_out(TI_LOOP_CLOCK_PLL_DATA, 0x00, 0x01);
	tvp_out(TI_LOOP_CLOCK_PLL_DATA, 0x00, (setup>>24)&0x0b);
	*/

// Standard configuration of TVP3025 (end).
	tvp_out(TI_SENSE_TEST, 0x00, 0x00);
	
	tvp_out(TI_CURS_CONTROL, 0x00, 0x80);
}


// Convert a clock frenquency in Mhz into the tvp clock generator format (see
// the TVP 3025 databook for more information). Those clock generator provide
// Fpll = (1/2^P)*Fref*((M+2)*8)/(N+2) frequencies, with N and M described by
// 7 bits and P by 2 bits.
long encrypt_tvp_clock(float freq)
{
	float    fr, fr0, fr2;
	long     p, m, n, m2, n2, nmax;

	ddprintf("encrypt_tvp_clock(%f)\n", freq);
// Fpll*2^P has to be > 110.0 Mhz. 
	p = 0;
	while (freq < 110.0) {
		freq *= 2.0;
		p++;
	}

// Look for the best choice in all the allowed (M,N) couples (see TVP 3025
// databook for specific conditions).
	fr0 = freq/(8.0*TI_REF_FREQ);
	fr2 = -2.0;
	nmax = (int)TI_REF_FREQ;
	for (n=3;n<=nmax;n++) {
  // Check all allowed values of N+2, calculate the best choice for M+2
		m = (int)(fr0*((float)n)+0.5);
		fr = ((float)m)/((float)n);
  // Keep the most accurate choice.
		if (((fr0-fr)*(fr0-fr) < (fr0-fr2)*(fr0-fr2)) && (m >= 3) && (m < 130)) {
			m2 = m;
			n2 = n;
			fr2 = fr;
		}
	}
	
// We were using M+2 and N+2, we need to encrypt M and N
	m = m2-2;
	n = n2-2;

// Return the 3 values in 3 bytes of a long.
	//return (n<<16)+(m<<8)+8+p;
	return (n<<16)+(m<<8)+p;
}



//**************************************************************************
//  ATT21C498 specific stuff.
//
//  This is a DAC used with 864 chips. Most of the time, the clock
//  generator used with that DAC uses a very simple standard serial interface.
//  We've taken the ICD2061A as an reference, but many other clock generator
//  are compatible with that interface.
//**************************************************************************


// This table is used to calculate the master video frenquency needed to get
// a selected refresh rate. First line is 8 bits spaces, second 16 bits
// spaces, third 32 bits spaces. Resolutions are columns 1 to 5 : 640x480,
// 800x600, 1024x768, 1280x1024 and 1600x1200 (see GraphicsCard.h for space
// definition).
//
// As we're already using CRT_CONTROL for other chips than 964 and 968, you'll
// not find a equivalent of the tvp_ScreenClockSize table. In place, we use
// a table providing for each space the number of clock cycles needed to
// display the equivalent of one horizontal unit of a CRT horizontal counter.
//
// For example, on the 864, in 8 bits, 640x480, the horizontal CRT width is 80
// as the CRT counter display 8 bytes per count (it's the problem of
// compatibily with the old text mode, using character 8 pixels width). For the
// same space, the ATT21C498 is reading one byte per cycle. So finally the
// ATT21C498 read 8 bytes in 8 cycles. So one increment of the horizontal CRT
// counter is equivalent to 8 cycles of the  master video clock (and so
// att498_ScreenClockStep[640x480, 8bits] = 8).
// For 640x480, 32 bits, the counter still count 8 bytes per increment, and the
// ATT21C498 read 16 bits per cycle, so we get a setting of 4.
//
// After that, we just have to multiple that by the horizontal and the vertical
// size of the screen (as described by the CRT counters) to get the number of
// cycles needed to display one frame, which is the equivalent of the data
// stored in the tvp_ScreenClockSize.
static float att498_ScreenClockStep[32] =
{
	8.0, 8.0, 8.0, 8.0, 8.0,
	0.0, 0.0, 0.0, 0.0, 0.0,
	4.0, 4.0, 0.0, 0.0, 0.0,
	8.0, 0.0, 0.0, 0.0, 0.0,
	0.0, 0.0, 0.0, 0.0, 0.0,
	0.0, 0.0, 0.0, 0.0, 0.0,
	0.0, 0.0,
};


// Write one bit to the serial input of the ICD2061A (see the ICD2061A databook
// for more information about critical timing). One bit of data (bit 3), one
// bit of clock (bit 2).
//
// As the interface used to communicate with the ICD2061A clock generator is
// pretty simple, there's no way to read back any informations... As they say
// in the databook, if you want to know if the setting succeed, use the VBL
// interrupt and check the display frequency !!!
static void wrt_clk_bit(unsigned int value)
{
	int j;

	outp(0x3c2, value);
	for (j = 2; --j; )
		inp(0x200);
}


// Send a 24 bits configuration word to the ICD2061A to program the frequency
// of one of the clock generator (see ICD2061A databook for description of
// the protocol used).
static void icd_set_clock(ulong setup)
{
	uchar		tmp;
	ushort		oldclk;
	ushort		bitval;
	ushort		restore42;
	uchar		c;
	uchar		nclk[2], clk[2];
	int			i,ii;

// Unlock CRT registers (probably not necessary)
	outp(0x3d4, 0x11);
	tmp = inp(0x3d5);
	outp(0x3d5, tmp & ~0x80);

// Unlock S3 extended registers (probably not necessary)
	outpw(0x3d4, 0x4a38);
	outpw(0x3d4, 0xa039);

// Send the configuration word three times. As we don't have any easy way to
// know if the setting is succesful, and as it's really timing sensitive, it'll
// fail if this thread is suspend during the setting (and as we're not a part
// of the kernel, we're not allowed to inhibit interruption).
// By sending it 3 times, we know that at least one of the three will be send
// in a complete form, with a good initialization and without being interrupt
// (except in case of an incredible bad luck). If possible, don't hesitate to
// do cleaner yourself...
	for (ii=0;ii<3;ii++) {
  // initialize the serial input of the ICD2061A
		oldclk = inp(0x3cc);
		outp(0x3d4, 0x42);		
		restore42 = inp(0x3d5);
		
		outpw(0x3c4, 0x0100);
		
		outp(0x3c4, 0x01);		
		c = inp(0x3c5);		   
		outp(0x3c5, 0x20 | c); 
		outp(0x3d4, 0x42);	   
		outp(0x3d5, 0x03);
		outpw(0x3c4, 0x0300);

  // Prepare the 4 values needed to program any value of the clock bit and the
  // data bit.
		nclk[0] = oldclk & 0xf3;
		nclk[1] = nclk[0] | 0x08;
		clk[0] = nclk[0] | 0x04;
		clk[1] = nclk[0] | 0x0c;
		
		outpw(0x3d4, 0x0100);

  // Specific unlock sequence
		wrt_clk_bit(oldclk | 0x08);
		wrt_clk_bit(oldclk | 0x0c);
		for (i = 0; i < 5; i++) {
			wrt_clk_bit(nclk[1]);
			wrt_clk_bit(clk[1]);
		}
		wrt_clk_bit(nclk[1]);
		wrt_clk_bit(nclk[0]);
		wrt_clk_bit(clk[0]);
		wrt_clk_bit(nclk[0]);
		wrt_clk_bit(clk[0]);

  // Send the 24 bits word.
		for (i = 0; i < 24; i++) {
			bitval = setup & 0x01;
			setup >>= 1;
			wrt_clk_bit(clk[1 - bitval]);
			wrt_clk_bit(nclk[1 - bitval]);
			wrt_clk_bit(nclk[bitval]);
			wrt_clk_bit(clk[bitval]);
		}
		wrt_clk_bit(clk[1]);
		wrt_clk_bit(nclk[1]);
		wrt_clk_bit(clk[1]);

  // Put everything back in order.		
		outp(0x3c4, 0x01);
		c = inp(0x3c5);
		outp(0x3c5, 0xdf & c);
		
		outp(0x3d4, 0x42);
		outp(0x3d5, restore42);
		
		outp(0x3c2, oldclk);
		
		outpw(0x3c4,0x0300);
	}
}


// Convert a clock frenquency in Mhz into the ICD clock generator format (see
// the ICD2061A databook for more information). Those clock generator provide
// Fpll = (1/2^M)*Fref*(P+3)/(Q+2) with P and Q described by 7 bits values, and
// M by 3 bits.
long icd_encrypt_clock(float freq)
{
	float    fr, fr0, fr2;
	long     p, q, p2, q2, m, i, qmin, qmax;

// Fpll*2^M has to be > 50.0 Mhz. The top limit is 120 Mhz, so we add a 1%
// margin to increase precision near 50 Mhz.
	m = 0;
	while (freq < 50.5) {
		freq *= 2.0;
		m++;
	}

// Look for the best choice in all the allowed (P+3,Q+2) couples (see ICD2061A
// databook for specific conditions).
	fr0 = freq/(2.0*TI_REF_FREQ);
	fr2 = -2.0;
	qmin = ((int)TI_REF_FREQ)+1;
	qmax = (int)(5.0*TI_REF_FREQ);
	if (qmax > 129) qmax = 129;
	for (q=qmin;q<=qmax;q++) {
  // Check all allowed values of Q+2, calculate the best choice for P+3
		p = (int)(fr0*((float)q)+0.5);
		fr = ((float)p)/((float)q);
  // Keep the most accurate choice.
		if (((fr0-fr)*(fr0-fr) < (fr0-fr2)*(fr0-fr2)) && (p >= 4) && (p < 131)) {
			p2 = p;
			q2 = q;
			fr2 = fr;
		}
	}
	
// The ICD2061A need one more parameter describing the range of Fpll*2^M. This
// is the I parameter and the correspondance is only empirical (as described in
// the databook).
	freq = 2.0*TI_REF_FREQ*fr2;
	if (freq < 51.0) i = 0;
	else if (freq < 53.2) i = 1;
	else if (freq < 58.5) i = 2;
	else if (freq < 60.7) i = 3;
	else if (freq < 64.4) i = 4;
	else if (freq < 66.8) i = 5;
	else if (freq < 73.5) i = 6;
	else if (freq < 75.6) i = 7;
	else if (freq < 80.9) i = 8;
	else if (freq < 83.2) i = 9;
	else if (freq < 91.5) i = 10;
	else if (freq < 100.0) i = 11;
	else i = 12;

// We were using P+3 and Q+2, we need to encrypt P and Q
	p = p2-3;
	q = q2-2;

// Return the 4 parameters packed into a 21 bits word. 3 more bits will be add
// at the head of the word to specify which clock generator we want to program.
	return ((i<<17)+(p<<10)+(m<<7)+(q));
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
	0.0, 0.0, 0.0, 0.0, 0.0,
	8.0, 8.0, 0.0, 0.0, 0.0,
	8.0, 0.0, 0.0, 0.0, 0.0,
	0.0, 0.0, 0.0, 0.0, 0.0,
	0.0, 0.0, 0.0, 0.0, 0.0,
	0.0, 0.0,
};


// Send a 32 bits configuration word to the trio DAC to program the frequency
// of the two clock generators (see trio32/64 databook for more informations).
// Each clock settings is packed into 2 bytes, using each 7 bits.
void trio_set_clock(ulong setup)
{
// Program the memory clock frenquency
	outp(0x3c4, 0x15);
	outp(0x3c5, 0x00);

	outp(0x3c4, 0x10);
	outp(0x3c5, (setup>>24)&127);
	outp(0x3c4, 0x11);
	outp(0x3c5, (setup>>16)&127);
	
	outp(0x3c4, 0x15);
	outp(0x3c5, 0x01);

// Program main video clock frenquency
	outp(0x3c4, 0x15);
	outp(0x3c5, 0x00);

	outp(0x3c4, 0x12);
	outp(0x3c5, (setup>>8)&127);
	outp(0x3c4, 0x13);
	outp(0x3c5, setup&127);
	
	outp(0x3c4, 0x15);
	outp(0x3c5, 0x02);

// All done
	outp(0x3c4, 0x15);
	outp(0x3c5, 0x00);
}


// Convert a clock frenquency in Mhz into the trio clock generator format (see
// the trio32/64 databook for more information). Those clock generator provide
// Fpll = (1/2^R)*Fref*(M+2)/(N+2) with M described by 7 bits, N by 5 bits and
// R by 2 bits.
long trio_encrypt_clock(float freq)
{
	float    fr,fr0,fr2;
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
	
	switch (theDAC) {
	case tvp3025_DAC :
		
// As the CRT_CONTROL is not implemented with TVP3025 (it could be easily with
// enough time), the video clock depends only of the length of a display frame
// multiplied by the selected refresh rate.
		clock = tvp_ScreenClockStep[scrnResNum] * scrnRate *
			    (float)lastCrtHT * (float)lastCrtVT;
#if 0
		setup = encrypt_tvp_clock(clock * 1e-6);

// Some tricky adjustement needed for high resolution. As the TVP3025 is a VRam
// DAC, you'll find a lot of complicated optimization available.
		if (scrnRes <= vga800x600)
			setup |= 0x02000000;
		else
			setup |= 0x01000000;
#endif
// Set the video clock to the chosen frenquency. The setting of the memory
// clock is hard-coded in tvp_set_clock.
		//tvp_set_clock(setup);
		tvp_set_clock(clock);
		break;

	case att21c498_DAC :
		
// Set the memory clock to the standard frequency, 60 Mhz.
		setup = icd_encrypt_clock(60.0) + 0x600000;
		icd_set_clock(setup);

// As the ATT21C498 uses CRT_CONTROL, the clock frequency depends of the
// horizntal crt counter increment multiplied by the horizontal and vertical
// sizes of the frame (as described by crt registers), multiplied by the
// selected refresh rate.
		clock = att498_ScreenClockStep[scrnResNum] * scrnRate *
			    (float)lastCrtHT * (float)lastCrtVT;
		setup = icd_encrypt_clock(clock * 1e-6) + 0x400000;
		icd_set_clock(setup);
		break;

	case s3trio_DAC :

// Set the memory clock to the standard frequency, 60 Mhz.
		setup = trio_encrypt_clock(60.0);

// As the trio DAC uses CRT_CONTROL, the clock frequency depends of the
// horizntal crt counter increment multiplied by the horizontal and vertical
// sizes of the frame (as described by crt registers), multiplied by the
// selected refresh rate.
		clock = trio_ScreenClockStep[scrnResNum] * scrnRate *
			    (float)lastCrtHT * (float)lastCrtVT;

// Packed definition of both clock settings (memory and video)
		setup = (setup<<16)+trio_encrypt_clock(clock * 1e-6);
		trio_set_clock(setup);
		break;
	}
}



//**************************************************************************
//  DAC identification.
//
//  Test some specific sequences to identify the DAC we're able to drive.
//  As there's no real standard to do that, people generally use a lot of
//  different trick. Databooks generally provide good way to recognize their
//  chips.
//**************************************************************************

static bool check_DAC()
{
	theDAC = 0;

// For 964 try to recognize TVP3025 DAC.
	if (theVGA == s3_964) {
		unsigned char saveCR55, saveCR5C, saveTIndx, saveTIndx2, saveTIdata;

		outp(0x3d4, 0x55);	// EX_DAC_CT
		saveCR55 = inp(0x3d5);
		outp(0x3d5, (saveCR55 & 0xfc) | 0x01);

		saveTIndx = inp(TI_INDEX_REG);

		outp(0x3d4, 0x5c);	// GOUT_PORT
		saveCR5C = inp(0x3d5);
		outp(0x3d5, saveCR5C & 0xdf);
		saveTIndx2 = inp(TI_INDEX_REG);
		outp(TI_INDEX_REG, TI_CURS_CONTROL);
		saveTIdata = inp(TI_DATA_REG);
		outp(TI_DATA_REG, saveTIdata & 0x7f);
		outp(TI_INDEX_REG, TI_ID);
		if (inp(TI_DATA_REG) == TI_VIEWPOINT25_ID)
			theDAC = tvp3025_DAC;
		outp(TI_INDEX_REG, TI_CURS_CONTROL);
		outp(TI_DATA_REG, saveTIdata);
		outp(TI_INDEX_REG, saveTIndx2);
		outp(0x3d4, 0x5c);	// GOUT_PORT
		outp(0x3d5, saveCR5C);

		outp(TI_INDEX_REG, saveTIndx);
		outp(0x3d4, 0x55);	// EX_DAC_CT
		outp(0x3d5, saveCR55);
	}

// For 864 try to recognize ATT21C498 DAC	
	if (theVGA == s3_864) {
		int dir, mir;

  // Use signature stored in hidden register.
		inp(0x3c8);
		inp(0x3c6);
		inp(0x3c6);
		inp(0x3c6);
		inp(0x3c6);
		inp(0x3c6);
		mir = inp(0x3c6);
		dir = inp(0x3c6);
		inp(0x3c8);

		if ((mir == 0x84) && (dir == 0x98))
			theDAC = att21c498_DAC;
	}

// Recognize between the trio32 and the trio64 (just for happiness as we're
// driving both the same way for now).
	if (theVGA == s3_trio00) {
		int    i;

		outp(0x3d4,0x2e);
		i = inp(0x3d5);
		if (i == 0xff) {
			outp(0x3c3, 1);
			inp(0x3cc);
			inp(0x3cc);
			inp(0x3cc);
			set_table_slow(hack_trio64vp);
			inp(0x3cc);
			outp(0x3d4,0x2e);
			i = inp(0x3d5);
		}
		if (i == 0x11) {
			outp(0x3d4,0x2f);
			i = inp(0x3d5);
			if ((i&0x40) == 0x40)
				theVGA = s3_trio64vp;
			else
				theVGA = s3_trio64;
		} else
			theVGA = s3_trio32;
		theDAC = s3trio_DAC;
	}

// return FALSE if no known DAC has been detected
	return (theDAC != 0);
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


// Values to set we need to set into the 28 previous registers to get the
// selected CRT_CONTROL settings.
static 	uchar     settings[28];


// Calculate the settings of the 28 registers to get the positions and sizes
// described in crtPosH, crtPosV, crtSizeH and crtSizeV (value from 0 to 100).
static void prepare_crt (long pixel_per_unit)
{
	int       i,par1,par2,par3;
	uchar     Double;
	ulong     Eset[2];
	ulong     Hset[7],Vset[6];
	
// Calculate the value of the 7 basic horizontal crt parameters depending of
// crtPosH and crtSizeH. Those 7 parameters are (in the same order as refered
// in crt_864_640x480 for example) :
//   - 0 : Horizontal total number of clock character per line.
//   - 1 : Horizontal number of clock character in the display part of a line.
//   - 2 : Start of horizontal blanking.
//   - 3 : End of horizontal blanking.
//   - 4 : Start of horizontal synchro position.
//   - 5 : End of horizontal synchro position.
//   - 6 : fifo filling optimization parameter
	while (TRUE) {
		Hset[1] = scrnWidth/pixel_per_unit;
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
		if (theVGA == s3_964)
			Hset[6] = (Hset[0]+Hset[4])/2;
		else
			Hset[6] = Hset[0] - 5;
		
		if (Hset[6] > 505) {
			crtSizeH++;
			continue;
		}
		
		break;
	}
	
// Calculate the value of the 6 basic vertical crt parameters depending of
// crtPosV and crtSizeV. Those 6 parameters are (in the same order as refered
// in crt_864_640x480 for example) :
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

// Calculate extended parameters (rowByte and base adress of the frameBuffer
	Eset[0] = ((scrnPosH*scrnColors)/8+scrnPosV*scrnRowByte)/4;
	Eset[1] = scrnRowByte/8;
	
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

// 5 of those 15 parameters have to be adjust by a specific offset before to be
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

// Those 12 parameters are encoded through the 28 8-bits registers. That code
// is doing the conversion, even inserting a few more significant bit depending
// of the space setting.
	settings[0] = Hset[0]&255;
	settings[1] = Hset[6]&255;
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
}


// Set the CRT_CONTROL by writing the preprocessed values in the 28 registers.
static void vid_select_crt()
{
	int      i;
	volatile long  j;
	
	for (i=0;i<28;i++) {
		outp(0x3d4, crt_index_864[i]);
		for (j=0;j<6;j++) {;}
		outp(0x3d5, settings[i]);
		for (j=0;j<6;j++) {;}
	}
}


// Just move the frame_buffer base adress
static void vid_select_crt_pos()
{
	outp(0x3d4, crt_index_864[20]);
	outp(0x3d5, settings[20]);
	outp(0x3d4, crt_index_864[21]);
	outp(0x3d5, settings[21]);
	outp(0x3d4, crt_index_864[27]);
	outp(0x3d5, settings[27]);
}



//**************************************************************************
//  CRT settings global management.
//
//  This part is doing the complete settings of all crt registers (including
//  CRT_CONTROL when necessary), for all depth and resolutions. Spaces are
//  decomposed by depth because depth allow common parts between different
//  resolutions. The opposite is not always true.
//**************************************************************************

// Depending of the video chip used, preprocess the CRT_CONTROL if necessary.
// We need to know the current horizontal and vertical total before setting the
// video clock frequency, but we also need to set the clock before setting
// all the crt register. That's the resaon why we first preprocess the CRT_
// CONTROL parameters. This is the code for 8 bits depth.
static void vid_prepare_crt_8 (void)
{
	prepare_crt(8);
}

// Depending of the video chip used, preprocess the CRT_CONTROL if necessary.
// The same as vid_prepare_crt_8, but for 32 bits depth.
static void vid_prepare_crt_32 (void)
{
	if (theVGA == s3_864)
		prepare_crt(2);
	else
		prepare_crt(8);
}



//**************************************************************************
//  Complete specific color, width, mem and performance settings for 964.
//**************************************************************************

static uchar index_964_settings[8] =
{
	0x36, 0x50, 0x54, 0x58, 0x60, 0x61, 0x62, 0x66
};

void do_964_settings()
{
	int      i;
	long     width;
	uchar    value[8];
	
	switch (scrnColors) {
	case 8 :
		value[1] = 0x00;
		value[7] = 0x03;
		break;
	case 32 :
		value[1] = 0x30;
		value[7] = 0x01;
		break;
	}

	if (theMem == 1024) {
		value[0] = 0xde;
		value[3] = 0x91;
	}
	else if (theMem == 2048) {
		value[0] = 0x9e;
		value[3] = 0x92;
	}
	else {
		value[0] = 0x1e;
		value[3] = 0x93;
	}

	i = offscrnWidth;
	if (i<=640) i = 0x40;
	else if (i<=800) i = 0x80;
	else if (i<=1024) i = 0x00;
	else if (i<=1152) i = 0x01;
	else if (i<=1280) i = 0xc0;
	else i = 0x81;
	value[1] |= i;

	value[2] = 0x00;
	value[4] = 0xff;
	width = (scrnWidth*scrnColors)/64;
	value[5] = 0x80|(width>>8);
	value[6] = width&255;
	
	for (i=0;i<8;i++) {
		outp(0x3d4,index_964_settings[i]);
		outp(0x3d5,value[i]);
	}
}



//**************************************************************************
//  Complete specific color, width, mem and performance settings for 864.
//**************************************************************************

static uchar index_864_settings[8] =
{
	0x36, 0x50, 0x54, 0x58, 0x60, 0x61, 0x62, 0x67
};

void do_864_settings()
{
	int      i;
	long     width;
	uchar    value[8];
	
	switch (scrnColors) {
	case 8 :
		value[1] = 0x00;
		value[7] = 0x00;
		break;
	case 32 :
		value[1] = 0x30;
		value[7] = 0x70;
		break;
	}

	if (theMem == 1024) {
		value[0] = 0xde;
		value[3] = 0x11;
	}
	else {
		value[0] = 0x9e;
		value[3] = 0x12;
	}

	i = offscrnWidth;
	if (i<=640) i = 0x40;
	else if (i<=800) i = 0x80;
	else if (i<=1024) i = 0x00;
	else if (i<=1152) i = 0x01;
	else if (i<=1280) i = 0xc0;
	else i = 0x81;
	value[1] |= i;

	value[2] = 0x00;
	value[4] = 0xff;
	if (theMem == 2048)
		width = (scrnWidth*scrnColors)/64;
	else
		width = (scrnWidth*scrnColors)/32;
	value[5] = 0x80|(width>>8);
	value[6] = width&255;
	
	for (i=0;i<8;i++) {
		outp(0x3d4,index_864_settings[i]);
		outp(0x3d5,value[i]);
	}
}



//**************************************************************************
//  Complete specific color, width, mem and performance settings for trio.
//**************************************************************************

static uchar index_t32_settings[7] =
{
	0x0b, 0x36, 0x50, 0x54, 0x58, 0x60, 0x67
};

void do_t32_settings()
{
	int      i;
	uchar    value[7];
	
	switch (scrnColors) {
	case 8 :
		value[0] = 0x00;
		value[2] = 0x00;
		value[6] = 0x00;
		break;
	case 32 :
		value[0] = 0xd0;
		value[2] = 0x30;
		value[6] = 0xd0;
		break;
	}

	if (theMem == 1024) {
		value[1] = 0xde;
		value[4] = 0x11;
	}
	else if (theMem == 2048) {
		value[1] = 0x9e;
		value[4] = 0x12;
	}
	else {
		value[1] = 0x1e;
		value[4] = 0x13;
	}
	

	i = offscrnWidth;
	if (i<=640) i = 0x40;
	else if (i<=800) i = 0x80;
	else if (i<=1024) i = 0x00;
	else if (i<=1152) i = 0x01;
	else if (i<=1280) i = 0xc0;
	else i = 0x81;
	value[2] |= i;

	value[3] = 0x00;
	value[5] = 0xff;
	
	outp(0x3c4,index_t32_settings[0]);
	outp(0x3c5,value[0]);
	for (i=1;i<7;i++) {
		outp(0x3d4,index_t32_settings[i]);
		outp(0x3d5,value[i]);
	}
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

	snooze(10000);

// Initialize the s3 chip in a reasonnable start configuration. Turn the chip
// on and unlock all the registers we need to access (and even more...). Do a
// few standard vga config.
	set_table(s3_table);

// The first big thing : the setting of the good space using all the extended
// registers. That's the most difficult part in all the process because there's
// so many small differences between chips (and so many bugs so well documented)
	switch(theVGA) {

// Those tables have been set for the 964, and work not too bad.
	case s3_964:
		set_table(vga_964_table);
		do_964_settings();
		break;
			
// Those tables have been set for the 864, and work pretty well.
	case s3_864:
		set_table(vga_864_table);
		do_864_settings();
		break;
			
// We're just working on those tables. They come from the ones for the 864,
// with a few small differencies. They've been test for the trio32, and work
// already in a few cases. We'll probably have to modify them a little for
// the trio64.
	case s3_trio32:
	case s3_trio64:
	case s3_trio64vp:
		set_table(vga_t32_table);
		do_t32_settings();
		break;
	}

// After that, we program two sets of standard vga registers
	set_table(sequencer_table);
	set_attr_table(attribute_table);

// It's time to turn the dac on in the good mode (that's a vga standard part)
	if (scrnColors == 8)
		setup_dac();
	else
		setup_dac32();
	
// Just another set of vga standard registers
	set_table(gcr_table);

// Now, we've to preprocess the CRT_CONTROL (if available)
	if (scrnColors == 8)
		vid_prepare_crt_8();
	else
		vid_prepare_crt_32();
	
// After that, we now know the real size of the display and so we can set
// set the selected refresh rate.
	write_pll();

// After the clock setting, we can program the crt registers
	vid_select_crt();		

// Better to initialize the frame buffer with a standard black and white
// pattern. We don't know how long will it take for the application server
// to decide to refresh all the screen. Better to get that than the previous
// buffer in the bad mode. I know, this is not really optimized...
	snooze(10000);

	if (ClearScreen) {
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
			draw = (ulong*)scrnBase;
			for (i = 0; i < scrnHeight / 2; i++) {
				for (j = 0; j < scrnWidth / 2; j++) {
					*(draw++) = 0xffffffff;
					*(draw++) = 0x00000000;
				}
				for (j = 0; j < scrnWidth / 2; j++) {
					*(draw++) = 0x00000000;
					*(draw++) = 0xffffffff;
				}
			}
		}
	}

  // A thing we've better not forget to do : disable the read and write mask
  // that protect every access to the video memory through the graphic engine
  // (by using a full mask). If you want some fun, try with other values...
	if (scrnColors == 8) {
		outpw(DATA_REG, MULT_MISC | 0x000);
		outpw(RD_MASK, 0xffff);
		outpw(RD_MASK, 0xffff);
		outpw(WRT_MASK, 0xffff);
		outpw(WRT_MASK, 0xffff);
	}
	else {
		outpw(DATA_REG, MULT_MISC | 0x200);
		outpl(RD_MASK, 0xffffffff);
		outpl(WRT_MASK, 0xffffffff);
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

static void vid_checkmem (void)
{
	bool      test;
	long      i, j;
	bigtime_t time;
	
	outp(0x3c4, 0x01);
	outp(0x3c5, 0x21);

// test 4 MB configuration
	if ((theVGA == s3_trio64) ||
		(theVGA == s3_trio64vp) ||
		(theVGA == s3_964) ||
		(theVGA == s3_virge)) {
		theMem = 4096;
		scrnColors = 32;
		scrnWidth = 1024;
		scrnHeight = 768;
		scrnRowByte = 1024*4;
		scrnRes = vga1024x768;

		DoSelectMode(FALSE);
		
		scrnColors = 0;
		scrnRes = -1;
		
		// Write 32 bytes pattern every 512K
		for (i=0;i<8;i++)
			for (j=0;j<32;j++)
				((uchar*)scrnBase)[(i<<19)+j] = (i<<5)+j;
		
		// Check if the pattern are all the same (no wrapping).
		test = TRUE;
		for (i=0;i<8;i++)
			for (j=0;j<32;j++) {
				if (((uchar*)scrnBase)[(i<<19)+j] != (i<<5)+j)
					test = FALSE;
				}
		if (!test) theMem = 1024;
	}

// test 2 MB configuration
	if (theMem <= 2048) {
		theMem = 2048;
		scrnColors = 32;
		scrnWidth = 640;
		scrnHeight = 480;
		scrnRowByte = 640*4;
		scrnRes = vga640x480;
		
		DoSelectMode(FALSE);
		
		scrnColors = 0;
		scrnRes = -1;
		
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
		if (!test) theMem = 1024;
	}
		
// The spaces available depend of the amount of video memory available...	
	available_spaces = B_8_BIT_640x480 | B_8_BIT_800x600;
	if (theMem >= 1024)
		available_spaces |= B_8_BIT_1024x768|B_8_BIT_1152x900;
	if (theMem >= 1536)
		available_spaces |= B_32_BIT_640x480|B_8_BIT_1280x1024;
	if (theMem >= 2048)
		available_spaces |= B_32_BIT_800x600|B_8_BIT_1600x1200;
	if (theMem >= 4096)
		available_spaces |= B_32_BIT_1024x768|B_32_BIT_1152x900;
	
// ...and the specific abilities of the add-on depending of the chip sets.
	available_spaces &= vga_res_available[theVGA];
	ddprintf("theMem: %d, spaces: 0x%08x\n", theMem, available_spaces);
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
	outp(0x3c4, 0x01);
	outp(0x3c5, 0x21);

// Select the good mode...
	DoSelectMode(ClearScreen);
	
// We need a buffer of 1024 bytes to store the cursor shape. This buffer has to
// be aligned on a 1024 bytes boundary. We put it just after the frame buffer.
// But if you want, you're free to put it at the beginning of the video memory,
// and then to offset the frame buffer by 1024 bytes.
	scrnBufBase = (uchar*)(((long)scrnBase + scrnRowByte * scrnHeight
							+ 1023) & 0xFFFFFC00);

// Unblank the screen now that all the very bad things are done.
	outp(0x3c4, 0x01);
	outp(0x3c5, 0x01);

// Disable the cliping of the graphic engine one more time. Probably useless now
// as everything else is already protected.
	outpw(DATA_REG, SCISSORS_T | 0x000);
	outpw(DATA_REG, SCISSORS_L | 0x000);
	outpw(DATA_REG, SCISSORS_B | 0xfff);
	outpw(DATA_REG, SCISSORS_R | 0xfff);

// Set the graphic engine in enhanced mode
	outpw(ADVFUNC_CNTL, 0x11);

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
	if (theVGA == s3_964) {
		vid_selectmode(FALSE);
		return;
	}
	
// As we are doing very bad things, it's better to protect everything. So we
// need to get both io-register and graphic engine benaphores.
// NB : When both benaphores are taken, they must be taken always in the same
// order to avoid any stupid deadlock.
	lock_io();
	lock_ge();

// Blank the screen. Not doing that is too dangerous (you risk to definitively
// confuse some monitors, putting them in a strange state from which they will
// never come back, except turn off/turn on).
	outp(0x3c4, 0x01);
	outp(0x3c5, 0x21);

// Initialize the s3 chip in a reasonnable start configuration. Turn the chip
// on and unlock all the registers we need to access (and even more...). Do a
// few standard vga config.
	snooze(10000);
	set_table(s3_table);

// Preprocess the CRT_CONTROL, set the new video clock frequency and then
// set the crt registers. 
	if (scrnColors == 8)
		vid_prepare_crt_8();
	else
		vid_prepare_crt_32();

	write_pll();
	vid_select_crt();
	snooze(10000);

// Unblank the screen now that all the bad things are done.
	outp(0x3c4, 0x01);
	outp(0x3c5, 0x01);

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
	if (scrnColors == 8)
		vid_prepare_crt_8();
	else
		vid_prepare_crt_32();

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
		
  // Current height of the frame buffer (not only the display part) in lines. 
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
		
// The aplication use a set of hooks function to accelerate some part of the
// user interface. This function copy the links to the available abilities of
// this add-on, depending of the current space setting.
    case B_GET_GRAPHICS_CARD_HOOKS :
	
		switch (theVGA) {
  // Full support is provided for 864 and compatible. Use with trio chips has
  // not been really tested.
		case s3_trio32:
		case s3_trio64:
		case s3_trio64vp:
		case s3_864:
			for (i=0;i<B_HOOK_COUNT;i++)
				((graphics_card_hook*)buf)[i] = vga_dispatch[i];
			break;
  // For 964 and compatible, the hardware cursor doesn't work at all although
  // the databook describes completly how to use it. But the VRam DACs
  // provide their own hardware cursor support, so it's no a problem, except
  // that hardware cursor support has not been implemented for now.
  // Consequently, we disable all the hardware cursor hooks.
		case s3_964:
			for (i=0;i<B_HOOK_COUNT;i++)
				((graphics_card_hook*)buf)[i] = vga_dispatch[i];
			for (i=0;i<3;i++)
				((graphics_card_hook*)buf)[i] = 0L;
			break;
		}
		if (scrnColors != 32)
			((graphics_card_hook*)buf)[11] = 0L;
		break;

// This is the first command a add-on will ever get. The aim is to know if this
// add-on is able to drive the card described by the application server.
	case B_OPEN_GRAPHICS_CARD :
	
  // This is the pointer to the beginning of the video memory, as mapped in
  // the add-on memory adress space. This add-on puts the frame buffer just at
  // the beginning of the video memory.
	    scrnBase = ((graphics_card_spec*)buf)->screen_base;

  // This is the pointer to the beginning of the 64K area used for io registers,
  // as mapped in the add-on memory adress space.
		isa_IO = (uchar*)((graphics_card_spec*)buf)->io_base;
		ddprintf("S3 B_OPEN_GRAPHICS_CARD isa_IO is 0x%08X\n", isa_IO);
  // All PCI chips provide the vendor Id of the graphic chip. First, we need to
  // verify we're speaking to a S3 one (for distract people, 0x5333 = 'S3').
		if (((graphics_card_spec*)buf)->vendor_id != 0x5333) {
			err = B_ERROR;
			break;
		}

  // All PCI chips provide also an device_id, used by each manufacturer to
  // identify all their different chips.
		switch (((graphics_card_spec*)buf)->device_id) {
		case 0x88d0:
			theVGA = s3_964;
			break;			
  // Two ids for the 864, don't ask why...
		case 0x88c0:
		case 0x88c1:
			theVGA = s3_864;
			break;
  // Trio32 and Trio64 have the same id. We first select an undefined trio
  // chip, and after that the DAC identification code will get a look to some
  // specific register to make the difference.
		case 0x8811:
			theVGA = s3_trio00;
			break;
  // S3 has a lot of others chips...
		default:
  // We set a error code, then quit that first switch level...
			theVGA = -1;
			break;
		}
  // ... and then check the error code and quit the second switch level.
		if (theVGA < 0) {
			err = B_ERROR;
			break;
		}

// Initialize the s3 chip in a reasonable configuration. Turn the chip on and
// unlock all the registers we need to access (and even more...). Do a few
// standard vga config. Then we're ready to identify the DAC.
//	    set_table(s3_table_first);

	    outp(0x46e8,0x10);
	    i = inp(0x102);
	    outp(0x102,0);
	    outp(0x46e8,0x08);

	    snooze(10000);
	
	    outp(0x46e8,0x10);
	    outp(0x102,1);
	    outp(0x46e8,0x08);
	    outp(0x4ae8,0x11);

	    if ((i&1) == 1)
			outp(0x3c3,1);
	
	    set_table(s3_table);
		
// We need to check that the chip we've identified is used with a DAC we know.
// In the other case, we can't drive that card.
	    if (!check_DAC()) {
			err = B_ERROR;
			break;
		}

// Arrived at that point, we now know we're able to drive that card. So we need
// to initialise what we need here, especially the benaphore protection that
// will allow concurent use of the add-on.
		init_locks();

// We finally need to test the size of the available memory, and we can test
// the available writing bandwidth, just for information.

		vid_checkmem();
		break;

// When the add-on is not able to drive any of the available cards, he will be
// unload soon. But first, this command will be call. As the application server
// never stop, except by crash or at reboot, this call will never be used to
// close an active add-on. Nevertheless, it's better to free here all the
// stuff you could be using.
	case B_CLOSE_GRAPHICS_CARD :
	
  // Blank the screen. The add-on prefers dying in the darkness...
		outp(0x3c4, 0x01);
		outp(0x3c5, 0x21);
  // Free the benaphore ressources (if necessary).
		dispose_locks();
		break;

// That command set an entry of the color_map in 8 bits depth (see documentation
// for more informations).
	case B_SET_INDEXED_COLOR:
	
// This call is using only the io-register (not the graphic engine). So we need
// to get the lock io-register benaphore.
		lock_io();

// ##----> THOSE SETTINGS SHOULD BE COMPATIBLE WITH ALL VGA CHIPS. <----##
// Set the selected entry of the color table of the DAC. This code use only
// standard vga registers
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

// Release the benaphore of the io-registers.
		unlock_io();
		break;

// This command is used by the application server to know what spaces are
// available on the card drive by this add-on.
	case B_GET_SCREEN_SPACES:
	
// All the work has already be done by vid_checkmem.
		((graphics_card_config*)buf)->space = available_spaces;
		break;

// This command is called to change any parameters of the current configuration.
// The add-on is supposed to check itself what parameters have changed since
// the previous, and then to change the card settings as smoothly as possible.
	case B_CONFIG_GRAPHICS_CARD:
	
// First, check that the selected space is available
		if ((((graphics_card_config*)buf)->space & available_spaces) == 0) {
			err = B_ERROR;
			break;
		}

// Change the CRT_CONTROL parameters (they've to be reset everytime).
		crtPosH = ((graphics_card_config*)buf)->h_position;
		crtSizeH = ((graphics_card_config*)buf)->h_size;
		crtPosV = ((graphics_card_config*)buf)->v_position;
		crtSizeV = ((graphics_card_config*)buf)->v_size;

// If the space has changed, we need to completly reconfigure the card.
		if (((graphics_card_config*)buf)->space != scrnResCode) {
  // Modify the space setting
			scrnResCode = ((graphics_card_config*)buf)->space;
			scrnResNum = 0;
			code = scrnResCode;
			while (code > 1) {
				scrnResNum++;
				code >>= 1;
		    }
  // Change the refresh rate (this parameter has to be reset everytime).
			scrnRate = ((graphics_card_config*)buf)->refresh_rate;
			if (scrnRate < vga_min_rates[scrnResNum])
				scrnRate = vga_min_rates[scrnResNum];
			if (scrnRate > vga_max_rates[scrnResNum])
				scrnRate = vga_max_rates[scrnResNum];
  // Do a full configuration and change the description of the current frame
  // buffer.
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
				ddprintf("Setting mode: %dx%dx%d@%f\n", scrnWidth, scrnHeight, scrnColors, scrnRate);
				vid_selectmode(TRUE);
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

// When the space is unchanged, we just need to reset the clock frenquency and
// the crt settings.
		else {
  // Change the refresh rate (this parameter has to be reset everytime).
			scrnRate = ((graphics_card_config*)buf)->refresh_rate;
			if (scrnRate < vga_min_rates[scrnResNum])
				scrnRate = vga_min_rates[scrnResNum];
			if (scrnRate > vga_max_rates[scrnResNum])
				scrnRate = vga_max_rates[scrnResNum];
  // Reset the refresh rate and the crt settings
			ddprintf("Setting refresh rate: %f\n", scrnRate);
		    vid_selectrate();
		}
		break;

// The refresh rate limits are under control of the add-on. The application
// is free to ask for any refresh rate, but the add-on is free to selected only
// allowed values. To limit misunderstanding, the application server uses that
// function to ask the available refresh rate range for a given space, and the
// real current setting as it doesn't know if what it asked for has been done.
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
		((clone_info*)buf)->theDAC = theDAC;
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
		((clone_info*)buf)->isa_IO = isa_IO;
		((clone_info*)buf)->CursorMode = CursorMode;
		break;

// Update the current internal addon settings
	case B_SET_CLONED_GRAPHICS_CARD :
	
		theVGA = ((clone_info*)buf)->theVGA;
		theDAC = ((clone_info*)buf)->theDAC;
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
		CursorMode = ((clone_info*)buf)->CursorMode;
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

		// default return values in case of failure
			((frame_buffer_info*)buf)->bytes_per_row = -1;
			((frame_buffer_info*)buf)->height = -1;
		// check that the depth ask for is available
			i = ((frame_buffer_info*)buf)->bits_per_pixel;
			row = ((frame_buffer_info*)buf)->width;
			if ((i != 8) && (i != 32)) {
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
			((frame_buffer_info*)buf)->bytes_per_row = row;
		// calculate how many lines can be used with that rowByte
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

		// check safety conditions between the values of the
		// different parameters.
			if ((col != 8) && (col != 32))
				goto bad_setting;
			if ((fw > row) || (dw > fw) || (dh > fh))
				goto bad_setting;
			if ((row != 640) && (row != 800) && (row != 1024) &&
				(row != 1152) && (row != 1280) && (row != 1600))
				goto bad_setting;
			if ((row*fh*col) > (theMem*8192L))
				goto bad_setting;
			if ((x < 0) || (y < 0) || (y+dh > fh) || (x+dw > fw))
				goto bad_setting;
			
			scrnRes = vga_specific;
			scrnResCode = -1;
			scrnColors = col;
			offscrnWidth = fw;
			offscrnHeight = fh;
			scrnWidth = dw;
			scrnHeight = dh;
			scrnRowByte = row;
			scrnPosH = x;
			scrnPosV = y;
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
			
			dw = ((frame_buffer_info*)buf)->display_width;
			dh = ((frame_buffer_info*)buf)->display_height;
			x = ((frame_buffer_info*)buf)->display_x;
			y = ((frame_buffer_info*)buf)->display_y;
			
		// check safety conditions between the values of the
		// different parameters.
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
			
		// check safety conditions between the values of the
		// different parameters.
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
