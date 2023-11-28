//*****************************************************************************
//
//	File:		 neomagic/video.c
//
//*****************************************************************************

#include <OS.h>

// Graphics card add-ons standard structs and prototypes
#include <GraphicsCard.h>
#include <PCI.h>
#include "bootscreen.h"

extern int read_isa_io (int bus, long address, int size);
extern int write_isa_io (int bus, long address, int size, int value);

#define inp(addr)  read_isa_io(0, (long)(addr), 1)
#define outp(addr,  val)  write_isa_io(0, (long)(addr), 1, (val))

static void ScreenOff(void);
static void ScreenOn(void);
static void lock_io(void);
static void unlock_io(void);

typedef struct {
	screen 		scr;
	pci_info	pci;
	int32       hotspot_h;
	int32       hotspot_v;
	thread_id	controller_thread;
} board_info;

static board_info	bi;
static ushort		vgaiobase;
static ushort		device_id;				

#define VGAIOBASE	0xff0

#define DEFAULT_REFRESH_RATE    60.1
#define NEOMAGIC_VENDOR	0x10c8
#define NM2070			0x0001	// 128
#define NM2160			0x0004	// 128XD
#define NM2200			0x0005	// 256AV

//----------------------------------------------------------
// global tables (for all depth)
//----------------------------------------------------------

static ushort	vga_table[] =
{
#if 0
	0x3c4,  0x08,   0x3c5,  0x06,
 	VGAIOBASE + 0x4,  	0x11, 	VGAIOBASE + 0x5,  	0x0c, 
#endif
	0x3ce,	0x09,	0x3cf,	0x26,	// Unlock extensions.
#if 0
	0x3ce,	0x0a,	0x3cf,	0x00,	// Unlock

	0x3c4,	0x00,	0x3c5,	0x00,	// Reset
	0x3c4,	0x00,	0x3c5,	0x40,	// Reset

	0x3c3,  0x01,
	0x3c2,	0x63,
#endif

	0x3c4,	0x00,	0x3c5,	0x03, 	// SR0
	0x3c4,	0x01,	0x3c5,	0x01,	// SR1 Clocking mode (reset below)
	0x3c4,	0x02,	0x3c5,	0x0f,	// SR2 Enable write plane
	0x3c4,	0x03,	0x3c5,	0x00,	// SR3 Char Font Select
	0x3c4,	0x04,	0x3c5,	0x0e,	// SR4 Memory Mode
	0x000,	0x00
};

static ushort	vga_table2[] =
{	
    0x3ce,	0x00,	0x3cf,	0x00,	// GR0
	0x3ce,	0x01,	0x3cf,	0x00,	// GR1
	0x3ce,	0x02,	0x3cf,	0x00,	// GR2
	0x3ce,	0x03,	0x3cf,	0x00,	// GR3
	0x3ce,	0x04,	0x3cf,	0x00,	// GR4 Read Plane Select
	0x3ce,	0x05,	0x3cf,	0x00,	// GR5 GC Mode
	0x3ce,	0x06,	0x3cf,	0x05,	// GR6
	0x3ce,	0x07,	0x3cf,	0x0f,	// Color Don't care
	0x3ce,	0x08,	0x3cf,	0xff,	// Bit Mask
#if 0
	0x3ce,  0x11,   0x3cf,  0xC0,	// Enable linear addressing
	0x3ce,  0x90,   0x3cf,  0x01,	// 256 colors
#endif

	0x00,	0x00			// End-of-table
};


static ushort	vga_extra_crt_8[] =
{
	VGAIOBASE + 0x4,	0x1d,	VGAIOBASE + 0x5,	0x00,
	VGAIOBASE + 0x4,	0x1f,	VGAIOBASE + 0x5,	0x00,
	VGAIOBASE + 0x4,	0x21,	VGAIOBASE + 0x5,	0x00,
	VGAIOBASE + 0x4,	0x23,	VGAIOBASE + 0x5,	0x23,
	0x00,	0x00	// End-of-table
};

static ushort	vga_extra_crt_16[] =
{
	VGAIOBASE + 0x4,	0x1d,	VGAIOBASE + 0x5,	0x00,
	VGAIOBASE + 0x4,	0x1f,	VGAIOBASE + 0x5,	0x00,
	VGAIOBASE + 0x4,	0x21,	VGAIOBASE + 0x5,	0x00,
	VGAIOBASE + 0x4,	0x23,	VGAIOBASE + 0x5,	0x34,
	0x00,	0x00	// End-of-table
};


static ushort	vga_extra_gr_8[] = 
{
	0x3ce,	0x0e,	0x3cf,	0x10, // extended crt display addr
	0x3ce,	0x0f,	0x3cf,	0x00, // undocumented
	0x3ce,	0x10,	0x3cf,	0x30, // system interface control 1
	0x3ce,	0x11,	0x3cf,	0x80, // system interface control 2
	0x3ce,	0x15,	0x3cf,	0x00, // single address page register
	0x3ce,	0x16,	0x3cf,	0x00, // dual address page register
	0x3ce,	0x25,	0x3cf,	0x00, // panel display control register 
	0x3ce,	0x2f,	0x3cf,	0x00, // undocumented
	0x3ce,	0x30,	0x3cf,	0x00, // undocumented
	0x3ce,	0x82,	0x3cf,	0x00, // hw cursor control register
	0x3ce,	0x90,	0x3cf,	0x11, // extended color mode select register
//	0x3ce,	0x96,	0x3cf,	0x06, // mclk denominator
//	0x3ce,	0x97,	0x3cf,	0x1a, // mclk numerator
//	0x3ce,	0x98,	0x3cf,	0xd4, // vclk0 numerator
//	0x3ce,	0x99,	0x3cf,	0xd4, // vclk1 numerator
//	0x3ce,	0x9a,	0x3cf,	0xd4, // vclk2 numerator
//	0x3ce,	0x9b,	0x3cf,	0xd4, // vclk3 numerator
//	0x3ce,	0x9c,	0x3cf,	0x09, // vclk0 denominator
//	0x3ce,	0x9d,	0x3cf,	0x09, // vclk1 denominator
//	0x3ce,	0x9e,	0x3cf,	0x09, // vclk2 denominator
//	0x3ce,	0x9f,	0x3cf,	0x09, // vclk3 denominator
	0x3ce,	0xae,	0x3cf,	0x00, // undocumented
	0x3ce,	0xb0,	0x3cf,	0x00, // undocumented
	0x00,	0x00			// End-of-table
};

static ushort	vga_extra_gr_16[] = 
{
	0x3ce,	0x0e,	0x3cf,	0x10, // 0
	0x3ce,	0x0f,	0x3cf,	0x00, 
	0x3ce,	0x10,	0x3cf,	0x30, 
	0x3ce,	0x11,	0x3cf,	0x80, 
	0x3ce,	0x15,	0x3cf,	0x00, 
	0x3ce,	0x16,	0x3cf,	0x00, // 5
	0x3ce,	0x25,	0x3cf,	0x00, 
	0x3ce,	0x2f,	0x3cf,	0x00, 
	0x3ce,	0x30,	0x3cf,	0x00, 
	0x3ce,	0x82,	0x3cf,	0x00, 
	0x3ce,	0x90,	0x3cf,	0x13, // 10
//	0x3ce,	0x96,	0x3cf,	0x06,	
//	0x3ce,	0x97,	0x3cf,	0x1a,
//	0x3ce,	0x98,	0x3cf,	0xd4,
//	0x3ce,	0x99,	0x3cf,	0xd4,
//	0x3ce,	0x9a,	0x3cf,	0xd4, // 15
//	0x3ce,	0x9b,	0x3cf,	0xd4,
//	0x3ce,	0x9c,	0x3cf,	0x09,
//	0x3ce,	0x9d,	0x3cf,	0x09,
//	0x3ce,	0x9e,	0x3cf,	0x09,
//	0x3ce,	0x9f,	0x3cf,	0x09, // 20
	0x3ce,	0xae,	0x3cf,	0x00,
	0x3ce,	0xb0,	0x3cf,	0x00,
	0x00,	0x00			// End-of-table
};

static ushort	vga_extra_gr_16_1024[] = 
{
	0x3ce,	0x0e,	0x3cf,	0x10, // 0
	0x3ce,	0x0f,	0x3cf,	0x01, 
	0x3ce,	0x10,	0x3cf,	0x30, 
	0x3ce,	0x11,	0x3cf,	0x80, 
	0x3ce,	0x15,	0x3cf,	0x00, 
	0x3ce,	0x16,	0x3cf,	0x00, // 5
	0x3ce,	0x25,	0x3cf,	0x00, 
	0x3ce,	0x2f,	0x3cf,	0x00, 
	0x3ce,	0x30,	0x3cf,	0x00, 
	0x3ce,	0x82,	0x3cf,	0x00, 
	0x3ce,	0x90,	0x3cf,	0x13, // 10
//	0x3ce,	0x96,	0x3cf,	0x06,	
//	0x3ce,	0x97,	0x3cf,	0x1a,
//	0x3ce,	0x98,	0x3cf,	0xd4,
//	0x3ce,	0x99,	0x3cf,	0xd4,
//	0x3ce,	0x9a,	0x3cf,	0xd4, // 15
//	0x3ce,	0x9b,	0x3cf,	0xd4,
//	0x3ce,	0x9c,	0x3cf,	0x09,
//	0x3ce,	0x9d,	0x3cf,	0x09,
//	0x3ce,	0x9e,	0x3cf,	0x09,
//	0x3ce,	0x9f,	0x3cf,	0x09, // 20
	0x3ce,	0xae,	0x3cf,	0x00,
	0x3ce,	0xb0,	0x3cf,	0x00,
	0x00,	0x00			// End-of-table
};


static ushort	vga_table_800x600_8[] =
{
#if 0
	0x3ce,	0x0e,	0x3cf,	0x10,
	0x3ce,  0x20,   0x3cf,  0x0a,
	0x3ce,  0x25,   0x3cf,  0x20,
#endif

	VGAIOBASE + 0x4,	0x00,	VGAIOBASE + 0x5,	0x7f,	// Horizontal total
	VGAIOBASE + 0x4,	0x01,	VGAIOBASE + 0x5,	0x63,	// Horizontal display end
	VGAIOBASE + 0x4,	0x02,	VGAIOBASE + 0x5,	0x63,	// Start horizontal blank
	VGAIOBASE + 0x4,	0x03,	VGAIOBASE + 0x5,	0x82,	// End horizontal blank
	VGAIOBASE + 0x4,	0x04,	VGAIOBASE + 0x5,	0x6b,	// Start Horizontal Sync Pos
	VGAIOBASE + 0x4,	0x05,	VGAIOBASE + 0x5,	0x1b,	// End horizontal Sync Pos
	VGAIOBASE + 0x4,	0x06,	VGAIOBASE + 0x5,	0x72,	// vertical total
	VGAIOBASE + 0x4,	0x07,	VGAIOBASE + 0x5,	0xf0,	// CRTC overflow
	VGAIOBASE + 0x4,	0x08,	VGAIOBASE + 0x5,	0x00,	// Preset row scan
	VGAIOBASE + 0x4,	0x09,	VGAIOBASE + 0x5,	0x60,	// Maximum scan line
	VGAIOBASE + 0x4,	0x0a,	VGAIOBASE + 0x5,	0x00,	// Cursor start scan line
	VGAIOBASE + 0x4,	0x0b,	VGAIOBASE + 0x5,	0x00,	// Cursor end scan line
	VGAIOBASE + 0x4,	0x0c,	VGAIOBASE + 0x5,	0x00,	// Start Address High
	VGAIOBASE + 0x4,	0x0d,	VGAIOBASE + 0x5,	0x00,	// Start Address Low
	VGAIOBASE + 0x4,	0x0e,	VGAIOBASE + 0x5,	0x00,	// Start Address Low
	VGAIOBASE + 0x4,	0x0f,	VGAIOBASE + 0x5,	0x00,	// Start Address Low
	VGAIOBASE + 0x4,	0x10,	VGAIOBASE + 0x5,	0x58,	// Vertical retrace Start
//	VGAIOBASE + 0x4,	0x11,	VGAIOBASE + 0x5,	0x8c,	// Vertical retrace End
	VGAIOBASE + 0x4,	0x12,	VGAIOBASE + 0x5,	0x57,	// Vertical Display End
	VGAIOBASE + 0x4,	0x13,	VGAIOBASE + 0x5,	0x64,	// Offset
	VGAIOBASE + 0x4,	0x14,	VGAIOBASE + 0x5,	0x00,	// Underline location
	VGAIOBASE + 0x4,	0x15,	VGAIOBASE + 0x5,	0x57,	// Start vertical blank
	VGAIOBASE + 0x4,	0x16,	VGAIOBASE + 0x5,	0x73,	// End vertical blank
	VGAIOBASE + 0x4,	0x17,	VGAIOBASE + 0x5,	0xe3,	// CRTC Mode Control
	VGAIOBASE + 0x4,	0x18,	VGAIOBASE + 0x5,	0xff,	// Line Compare

	0x00,	0x00			// End-of-table
};

static ushort	vga_table_1024x768_8[] =
{
#if 0
	0x3ce,	0x0e,	0x3cf,	0x10,
	0x3ce,  0x20,   0x3cf,  0x0a,
	0x3ce,  0x25,   0x3cf,  0x20,
#endif

	VGAIOBASE + 0x4,	0x00,	VGAIOBASE + 0x5,	0x98,	// Horizontal total
	VGAIOBASE + 0x4,	0x01,	VGAIOBASE + 0x5,	0x7f,	// Horizontal display end
	VGAIOBASE + 0x4,	0x02,	VGAIOBASE + 0x5,	0x81,	// Start horizontal blank
	VGAIOBASE + 0x4,	0x03,	VGAIOBASE + 0x5,	0x84,	// End horizontal blank
	VGAIOBASE + 0x4,	0x04,	VGAIOBASE + 0x5,	0x83,	// Start Horizontal Sync Pos
	VGAIOBASE + 0x4,	0x05,	VGAIOBASE + 0x5,	0x98,	// End horizontal Sync Pos
	VGAIOBASE + 0x4,	0x06,	VGAIOBASE + 0x5,	0x8e,	// vertical total
	VGAIOBASE + 0x4,	0x07,	VGAIOBASE + 0x5,	0xfd,	// CRTC overflow
	VGAIOBASE + 0x4,	0x08,	VGAIOBASE + 0x5,	0x00,	// Preset row scan
	VGAIOBASE + 0x4,	0x09,	VGAIOBASE + 0x5,	0x60,	// Maximum scan line
	VGAIOBASE + 0x4,	0x0a,	VGAIOBASE + 0x5,	0x00,	// Cursor start scan line
	VGAIOBASE + 0x4,	0x0b,	VGAIOBASE + 0x5,	0x00,	// Cursor end scan line
	VGAIOBASE + 0x4,	0x0c,	VGAIOBASE + 0x5,	0x00,	// Start Address High
	VGAIOBASE + 0x4,	0x0d,	VGAIOBASE + 0x5,	0x00,	// Start Address Low
	VGAIOBASE + 0x4,	0x0e,	VGAIOBASE + 0x5,	0x00,	// Start Address Low
	VGAIOBASE + 0x4,	0x0f,	VGAIOBASE + 0x5,	0x00,	// Start Address Low
	VGAIOBASE + 0x4,	0x10,	VGAIOBASE + 0x5,	0x04,	// Vertical retrace Start
//	VGAIOBASE + 0x4,	0x11,	VGAIOBASE + 0x5,	0x0e,	// 0e Vertical retrace End
	VGAIOBASE + 0x4,	0x12,	VGAIOBASE + 0x5,	0xff,	// Vertical Display End
	VGAIOBASE + 0x4,	0x13,	VGAIOBASE + 0x5,	0x80,	// Offset
	VGAIOBASE + 0x4,	0x14,	VGAIOBASE + 0x5,	0x00,	// Underline location
	VGAIOBASE + 0x4,	0x15,	VGAIOBASE + 0x5,	0x06,	// Start vertical blank
	VGAIOBASE + 0x4,	0x16,	VGAIOBASE + 0x5,	0x74,	// End vertical blank
	VGAIOBASE + 0x4,	0x17,	VGAIOBASE + 0x5,	0xe3,	// CRTC Mode Control
	VGAIOBASE + 0x4,	0x18,	VGAIOBASE + 0x5,	0xff,	// Line Compare

	0x00,	0x00			// End-of-table
};

static ushort	vga_table_1024x768_16[] =
{
#if 0
	0x3ce,	0x0e,	0x3cf,	0x10,
	0x3ce,  0x20,   0x3cf,  0x0a,
	0x3ce,  0x25,   0x3cf,  0x20,
#endif

	VGAIOBASE + 0x4,	0x00,	VGAIOBASE + 0x5,	0x98,	// Horizontal total
	VGAIOBASE + 0x4,	0x01,	VGAIOBASE + 0x5,	0x7f,	// Horizontal display end
	VGAIOBASE + 0x4,	0x02,	VGAIOBASE + 0x5,	0x81,	// Start horizontal blank
	VGAIOBASE + 0x4,	0x03,	VGAIOBASE + 0x5,	0x84,	// End horizontal blank
	VGAIOBASE + 0x4,	0x04,	VGAIOBASE + 0x5,	0x83,	// Start Horizontal Sync Pos
	VGAIOBASE + 0x4,	0x05,	VGAIOBASE + 0x5,	0x98,	// End horizontal Sync Pos
	VGAIOBASE + 0x4,	0x06,	VGAIOBASE + 0x5,	0x8e,	// vertical total
	VGAIOBASE + 0x4,	0x07,	VGAIOBASE + 0x5,	0xfd,	// CRTC overflow
	VGAIOBASE + 0x4,	0x08,	VGAIOBASE + 0x5,	0x00,	// Preset row scan
	VGAIOBASE + 0x4,	0x09,	VGAIOBASE + 0x5,	0x60,	// Maximum scan line
	VGAIOBASE + 0x4,	0x0a,	VGAIOBASE + 0x5,	0x00,	// Cursor start scan line
	VGAIOBASE + 0x4,	0x0b,	VGAIOBASE + 0x5,	0x00,	// Cursor end scan line
	VGAIOBASE + 0x4,	0x0c,	VGAIOBASE + 0x5,	0x00,	// Start Address High
	VGAIOBASE + 0x4,	0x0d,	VGAIOBASE + 0x5,	0x00,	// Start Address Low
	VGAIOBASE + 0x4,	0x0e,	VGAIOBASE + 0x5,	0x00,	// Start Address Low
	VGAIOBASE + 0x4,	0x0f,	VGAIOBASE + 0x5,	0x00,	// Start Address Low
	VGAIOBASE + 0x4,	0x10,	VGAIOBASE + 0x5,	0x04,	// Vertical retrace Start
//	VGAIOBASE + 0x4,	0x11,	VGAIOBASE + 0x5,	0x8e,	// Vertical retrace End
	VGAIOBASE + 0x4,	0x12,	VGAIOBASE + 0x5,	0xff,	// Vertical Display End
	VGAIOBASE + 0x4,	0x13,	VGAIOBASE + 0x5,	0x00,	// Offset
	VGAIOBASE + 0x4,	0x14,	VGAIOBASE + 0x5,	0x00,	// Underline location
	VGAIOBASE + 0x4,	0x15,	VGAIOBASE + 0x5,	0x06,	// Start vertical blank
	VGAIOBASE + 0x4,	0x16,	VGAIOBASE + 0x5,	0x74,	// End vertical blank
	VGAIOBASE + 0x4,	0x17,	VGAIOBASE + 0x5,	0xe3,	// CRTC Mode Control
	VGAIOBASE + 0x4,	0x18,	VGAIOBASE + 0x5,	0xff,	// Line Compare	
	0x00,	0x00			// End-of-table
};


static ushort	vga_table_800x600_16[] =
{
#if 0
	0x3ce,	0x0e,	0x3cf,	0x10,
	0x3ce,  0x20,   0x3cf,  0x0a,
	0x3ce,  0x25,   0x3cf,  0x20,
#endif

	VGAIOBASE + 0x4,	0x00,	VGAIOBASE + 0x5,	0x7f,	// Horizontal total
	VGAIOBASE + 0x4,	0x01,	VGAIOBASE + 0x5,	0x63,	// Horizontal display end
	VGAIOBASE + 0x4,	0x02,	VGAIOBASE + 0x5,	0x63,	// Start horizontal blank
	VGAIOBASE + 0x4,	0x03,	VGAIOBASE + 0x5,	0x82,	// End horizontal blank
	VGAIOBASE + 0x4,	0x04,	VGAIOBASE + 0x5,	0x6b,	// Start Horizontal Sync Pos
	VGAIOBASE + 0x4,	0x05,	VGAIOBASE + 0x5,	0x1b,	// End horizontal Sync Pos
	VGAIOBASE + 0x4,	0x06,	VGAIOBASE + 0x5,	0x72,	// vertical total
	VGAIOBASE + 0x4,	0x07,	VGAIOBASE + 0x5,	0xf0,	// CRTC overflow
	VGAIOBASE + 0x4,	0x08,	VGAIOBASE + 0x5,	0x00,	// Preset row scan
	VGAIOBASE + 0x4,	0x09,	VGAIOBASE + 0x5,	0x60,	// Maximum scan line
	VGAIOBASE + 0x4,	0x0a,	VGAIOBASE + 0x5,	0x00,	// Cursor start scan line
	VGAIOBASE + 0x4,	0x0b,	VGAIOBASE + 0x5,	0x00,	// Cursor end scan line
	VGAIOBASE + 0x4,	0x0c,	VGAIOBASE + 0x5,	0x00,	// Start Address High
	VGAIOBASE + 0x4,	0x0d,	VGAIOBASE + 0x5,	0x00,	// Start Address Low
	VGAIOBASE + 0x4,	0x10,	VGAIOBASE + 0x5,	0x58,	// Vertical retrace Start
//	VGAIOBASE + 0x4,	0x11,	VGAIOBASE + 0x5,	0x8c,	// Vertical retrace End
	VGAIOBASE + 0x4,	0x12,	VGAIOBASE + 0x5,	0x57,	// Vertical Display End
	VGAIOBASE + 0x4,	0x13,	VGAIOBASE + 0x5,	0xc8,	// Offset
	VGAIOBASE + 0x4,	0x14,	VGAIOBASE + 0x5,	0x00,	// Underline location
	VGAIOBASE + 0x4,	0x15,	VGAIOBASE + 0x5,	0xc8,	// Start vertical blank
	VGAIOBASE + 0x4,	0x16,	VGAIOBASE + 0x5,	0x73,	// End vertical blank
	VGAIOBASE + 0x4,	0x17,	VGAIOBASE + 0x5,	0xe3,	// CRTC Mode Control
	VGAIOBASE + 0x4,	0x18,	VGAIOBASE + 0x5,	0xff,	// Line Compare

	0x00,	0x00			// End-of-table
};

static ushort	vga_table_640x480[] =
{
#if 0
	0x3ce,	0x0e,	0x3cf,	0x10,
	0x3ce,  0x20,   0x3cf,  0x0a,
	0x3ce,  0x25,   0x3cf,  0xa5,
	0x3ce,  0x29,   0x3cf,  0x00,
#endif

	VGAIOBASE + 0x4,	0x00,	VGAIOBASE + 0x5,	0x5f,	// Horizontal total
	VGAIOBASE + 0x4,	0x01,	VGAIOBASE + 0x5,	0x4f,	// Horizontal display end
	VGAIOBASE + 0x4,	0x02,	VGAIOBASE + 0x5,	0x50,	// Start horizontal blank
	VGAIOBASE + 0x4,	0x03,	VGAIOBASE + 0x5,	0x80,	// End horizontal blank
	VGAIOBASE + 0x4,	0x04,	VGAIOBASE + 0x5,	0x53,	// Start Horizontal Sync Pos
	VGAIOBASE + 0x4,	0x05,	VGAIOBASE + 0x5,	0x81,	// End horizontal Sync Pos
	VGAIOBASE + 0x4,	0x06,	VGAIOBASE + 0x5,	0xbf,	// vertical total
	VGAIOBASE + 0x4,	0x07,	VGAIOBASE + 0x5,	0x1f,	// CRTC overflow
	VGAIOBASE + 0x4,	0x08,	VGAIOBASE + 0x5,	0x00,	// Preset row scan
	VGAIOBASE + 0x4,	0x09,	VGAIOBASE + 0x5,	0x40,	// Maximum scan line
	VGAIOBASE + 0x4,	0x0a,	VGAIOBASE + 0x5,	0x00,	// Cursor start scan line
	VGAIOBASE + 0x4,	0x0b,	VGAIOBASE + 0x5,	0x00,	// Cursor end scan line
	VGAIOBASE + 0x4,	0x0c,	VGAIOBASE + 0x5,	0x00,	// Start Address High
	VGAIOBASE + 0x4,	0x0d,	VGAIOBASE + 0x5,	0x00,	// Start Address Low
	VGAIOBASE + 0x4,	0x0e,	VGAIOBASE + 0x5,	0x00,	// Start Address Low
	VGAIOBASE + 0x4,	0x0f,	VGAIOBASE + 0x5,	0x00,	// Start Address Low
	VGAIOBASE + 0x4,	0x10,	VGAIOBASE + 0x5,	0x9c,	// Vertical retrace Start
//	VGAIOBASE + 0x4,	0x11,	VGAIOBASE + 0x5,	0x8e,	// Vertical retrace End
	VGAIOBASE + 0x4,	0x12,	VGAIOBASE + 0x5,	0x8f,	// Vertical Display End
	VGAIOBASE + 0x4,	0x13,	VGAIOBASE + 0x5,	0x50,	// Offset
	VGAIOBASE + 0x4,	0x14,	VGAIOBASE + 0x5,	0x00,	// Underline location
	VGAIOBASE + 0x4,	0x15,	VGAIOBASE + 0x5,	0x96,	// Start vertical blank
	VGAIOBASE + 0x4,	0x16,	VGAIOBASE + 0x5,	0xb9,	// End vertical blank
	VGAIOBASE + 0x4,	0x17,	VGAIOBASE + 0x5,	0xE3,	// CRTC Mode Control
	VGAIOBASE + 0x4,	0x18,	VGAIOBASE + 0x5,	0xff,	// Line Compare	

	0x00,	0x00			// End-of-table
};

static uchar attr_val[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x01, 0x00, 0x0f, 0x00, 0x00
};

static void	set_attr_table()
{
	uchar	p1,v;
	
	inp(0x3da);
	v = inp(0x3c0);
	
	for (p1=0; p1<sizeof(attr_val); p1++) {
		inp(0x3da);
		outp(0x3c0, p1);
		outp(0x3c0, attr_val[p1]);
	}

	inp(0x3da);
	outp(0x3c0, v | 0x20);
}

static void	set_table(ushort vgaiobase, ushort *ptr)
{
	ushort	p1;
	ushort	p2;

	while(1) {
		p1 = *ptr++;
		if ((p1 & 0xff0) == VGAIOBASE) {
			p1 = vgaiobase + (p1 & 0x00f);
		}
		p2 = *ptr++;
		if (p1 == 0 && p2 == 0) {
			return;
		}
		outp(p1, p2);
	}
}

static void	setup_dac()
{
#if 1
	inp(0x3c6);
	inp(0x3c6);
	inp(0x3c6);
	inp(0x3c6);
	outp(0x3c6, 0x00);
	inp(0x3c8);
	outp(0x3c6, 0xff);
#endif
}


#if 0
static status_t set_screen_saver_mode(bool save)
{
	unsigned char SEQ01;
	unsigned char LogicPowerMgmt;
	unsigned char LCD_on;

	if (save) {
		/* Screen: Off; HSync: Off, VSync: Off */
		SEQ01 = 0x20;
		LogicPowerMgmt = 0x30;
		LCD_on = 0x00;
	}
	else {
		/* Screen: On; HSync: On, VSync: On */
		SEQ01 = 0x00;
		LogicPowerMgmt = 0x00;
		LCD_on = 0x02;
	}
	
	/* Turn the screen on/off */
	outp(0x3C4, 0x01);
	SEQ01 |= inp(0x3C5) & ~0x20;
	outp(0x3C5, SEQ01);
	
	/* Turn the LCD on/off */
	outp(0x3ce, 0x20);
	LCD_on |= inp(0x3ce + 1) & ~0x02;
	outp(0x3ce + 1, LCD_on);
	
	/* Set the DPMS mode */
	LogicPowerMgmt |= 0x80;
	outp(0x3ce, 0x01);
	LogicPowerMgmt |= inp(0x3ce + 1) & ~0xF0;
	outp(0x3ce + 1, LogicPowerMgmt);

	return (B_NO_ERROR);
}
#endif


static void SetGraphicMode()
{
	int     i;
	uchar	temp;

	ScreenOff();

	lock_io();

	vgaiobase = (inp(0x3cc) & 0x01) ? 0x3d0 : 0x3b0; 

	outp(vgaiobase + 0x4,  0x11);
	temp = inp(vgaiobase + 0x5);
	outp(vgaiobase + 0x5, temp & 0x7f);

	set_table(vgaiobase, vga_table);		
	setup_dac();

	temp = inp(0x3cc);
	outp(0x3c2, (temp & 0xf3) | 0x08);

	set_attr_table();			

	set_table(vgaiobase, vga_table2);

	if (bi.scr.width == 640) {
		set_table(vgaiobase, vga_table_640x480);
	} else if (bi.scr.width == 800) {
		if (bi.scr.depth == 8) {
			set_table(vgaiobase, vga_table_800x600_8);
		} else {
			set_table(vgaiobase, vga_table_800x600_16);
		}
	} else if (bi.scr.width == 1024) {
		if (bi.scr.depth == 8) {
			set_table(vgaiobase, vga_table_1024x768_8);
		} else {
			set_table(vgaiobase, vga_table_1024x768_16);
		}
	}

	if (bi.scr.depth == 8) {
		set_table(vgaiobase, vga_extra_gr_8);
		set_table(vgaiobase, vga_extra_crt_8);
	} else {
		if (bi.scr.width == 1024)
			set_table(vgaiobase, vga_extra_gr_16_1024);
		else
			set_table(vgaiobase, vga_extra_gr_16);
		set_table(vgaiobase, vga_extra_crt_16);
	}
	
	unlock_io();

	ScreenOn();
}

static long
FindNeomagic(uchar *registers)
{
	int i, found;
	pci_info *pcii;

	pcii = &bi.pci;
	
	// call get_nth_pci_info() until registers matches
	found = !B_ERROR;
	for (i = 0; found != B_ERROR; i++) {
		found = get_nth_pci_info(i, pcii);
		if ((pcii->vendor_id == NEOMAGIC_VENDOR)  &&
			(pcii->u.h0.base_registers[0] == (ulong)registers)) {
			break;
		}
	}

	// on error, bail out after cleaning up
	if (found == B_ERROR)
		return B_ERROR;

	device_id = pcii->device_id;

	write_pci_config (pcii->bus, pcii->device, pcii->function,
					  PCI_command, 2, 0x0003);

	return B_NO_ERROR;
}

static void
ScreenOff(void)
{
	lock_io();
	outp(0x3c4, 0x01);
	outp(0x3c5, 0x21);		// blank the screen
	unlock_io();
}

static void
ScreenOn(void)
{
	lock_io();
	outp(0x3c4, 0x01);
	outp(0x3c5, 0x01);		//unblank the screen
	unlock_io();
}

long control_graphics_card(ulong message,void *buf);


long   kset_cursor_shape_2160(uchar *data,uchar *mask,long dim_h,
						 long dim_v,long hot_h,long hot_v);
long   kmove_cursor_2160(long new_h,long new_v);
long   kshow_cursor_2160(bool state);
long   krect8_2160(long x1, long y1, long x2, long y2, uchar color);
long   krect16_2160(long x1, long y1, long x2, long y2, uint16 color);
long   kblit_2160(long x1, long y1, long x2, long y2, long width, long height);
long   kset_cursor_shape_2070(uchar *data,uchar *mask,long dim_h,
						 long dim_v,long hot_h,long hot_v);
long   kmove_cursor_2070(long new_h,long new_v);
long   kshow_cursor_2070(bool state);
long   krect8_2070(long x1, long y1, long x2, long y2, uchar color);
long   kblit_2070(long x1, long y1, long x2, long y2, long width, long height);


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
static long	    io_lock;

// Create the benaphores (if possible)
static void init_locks(void)
{
	if (io_sem == -1) {
		io_lock = 0L;
		io_sem = create_sem(0,"vga io sem");
	}
}

// Free the benaphore (if created)
static void dispose_locks(void)
{
	if (io_sem >= 0)
		delete_sem(io_sem);
}

// Protect the access to the general io-registers by locking a benaphore.
static void lock_io(void)
{
	int	old;

	old = atomic_add (&io_lock, 1);
	if (old >= 1)
		acquire_sem(io_sem);	
}

// Release the protection on the general io-registers
static void unlock_io(void)
{
	int	old;

	old = atomic_add (&io_lock, -1);
	if (old > 1)
		release_sem(io_sem);
}	

long kset_cursor_shape_2200(uchar *data, // XorMask
					  uchar *mask, // AndMask
					  long  dim_h, // Dimensions of the cursor (in pixel)
					  long  dim_v, //
					  long  hot_h, // Position of the hot_spot in the cursor
					  long  hot_v) //
{
	int     i, j, k,dim_h0;
	bool    init;
	uchar   *buf;

	lock_io();
	
	dim_h0 = (dim_h+7)>>3;

	bi.hotspot_h = hot_h;
	bi.hotspot_v = hot_v;

	for (k=0x7f0; k<0x800; k++) {
		buf = ((uchar*)bi.scr.base) + 1024 * k;
		for (i=0;i<1024;i++) 
		  buf[i] = 0x00;	
		
		for (i=0;i<dim_v;i++)
		  for (j=0;j<dim_h0;j++) {
			buf[i*16+j] = data[j+i*dim_h0]/* & (mask[j+i*dim_h0]^0xff)*/;
			buf[i*16+j+8] = (mask[j+i*dim_h0]^0xff);
		  }
		buf += 1024;
	}

	outp(0x3ce, 0x85); outp(0x3cf, 0x7f);
	outp(0x3ce, 0x86); outp(0x3cf, 0x00);

	outp(0x3ce, 0x74); outp(0x3cf, 0x00);
	outp(0x3ce, 0x75); outp(0x3cf, 0x00);
	outp(0x3ce, 0x76); outp(0x3cf, 0x00);
	outp(0x3ce, 0x77); outp(0x3cf, 0xff);
	outp(0x3ce, 0x78); outp(0x3cf, 0xff);
	outp(0x3ce, 0x79); outp(0x3cf, 0xff);

	unlock_io();

	return B_NO_ERROR;
}

long kset_cursor_shape_2160(uchar *data, // XorMask
					  uchar *mask, // AndMask
					  long  dim_h, // Dimensions of the cursor (in pixel)
					  long  dim_v, //
					  long  hot_h, // Position of the hot_spot in the cursor
					  long  hot_v) //
{
	int     i, j, k,dim_h0;
	bool    init;
	uchar   *buf;

	lock_io();
	
	dim_h0 = (dim_h+7)>>3;

	bi.hotspot_h = hot_h;
	bi.hotspot_v = hot_v;

	buf = ((uchar*)bi.scr.base) + 1024 * 0x7fc;

	for (i=0;i<1024;i++) 
	  buf[i] = 0x00;	
	
	for (i=0;i<dim_v;i++)
	  for (j=0;j<dim_h0;j++) {
		buf[i*16+j] = data[j+i*dim_h0]/* & (mask[j+i*dim_h0]^0xff)*/;
		buf[i*16+j+8] = (mask[j+i*dim_h0]^0xff);
	  }

	outp(0x3ce, 0x85); outp(0x3cf, 0x7f);
	outp(0x3ce, 0x86); outp(0x3cf, 0x00);

	outp(0x3ce, 0x74); outp(0x3cf, 0x00);
	outp(0x3ce, 0x75); outp(0x3cf, 0x00);
	outp(0x3ce, 0x76); outp(0x3cf, 0x00);
	outp(0x3ce, 0x77); outp(0x3cf, 0xff);
	outp(0x3ce, 0x78); outp(0x3cf, 0xff);
	outp(0x3ce, 0x79); outp(0x3cf, 0xff);

	unlock_io();

	return B_NO_ERROR;
}
	
long kmove_cursor(long new_h, long new_v)
{
	lock_io();

	new_h -= bi.hotspot_h;
	new_v -= bi.hotspot_v;  
	if (new_h < 0)
	  new_h = 0;
	if (new_v < 0)
	  new_v = 0;

	outp(0x3ce, 0x70); outp(0x3cf, new_h & 0xff);
	outp(0x3ce, 0x71); outp(0x3cf, new_h >> 8);
	outp(0x3ce, 0x72); outp(0x3cf, new_v & 0xff);
	outp(0x3ce, 0x73); outp(0x3cf, new_v >> 8);
	
	unlock_io();

	return B_NO_ERROR;
}

long kshow_cursor(bool state)
{
	lock_io();

// Show the cursor
	if (state != 0) {
	  outp(0x3ce, 0x82); outp(0x3cf, 0x01);
	}
// Hide the cursor
	else {
	  outp(0x3ce, 0x82); outp(0x3cf, 0x00);
	}
	
	unlock_io();
	return B_NO_ERROR;
}

static void  wait_for_blit()
{
	bigtime_t t0, t1;

	t0 = system_time();
	do t1 = system_time();
	while ((t1-t0) < 50);

	outp(0x3ce, 0x40);		// status
	while (inp(0x3cf) & 1);
}

long kblit_2200(long  x1,     // top-left point of the source
		  long  y1,     //
		  long  x2,     // top-left point of the destination
		  long  y2,     //
		  long  width,  // size of the rect to move (from border included to
		  long  height) // opposite border included).
{
/*
	int32       cmd;
	int32		LTemp;

	if ((x1 == x2) && (y1 == y2))
	  return B_NO_ERROR;

	lock_io();

	width++;
	height++;

	// blt mode	
	LTemp = 0x1000 | 0x0200;
	outp(0x3ce, 0x42); outp(0x3cf, LTemp & 0xff);	
	outp(0x3ce, 0x43); outp(0x3cf, LTemp >> 8);

	cmd = 0x80000000 | 0x0c0000;
	// control/status cmd
	LTemp = cmd;
	outp(0x3ce, 0x44); outp(0x3cf, LTemp & 0xff);	
	outp(0x3ce, 0x45); outp(0x3cf, (LTemp >> 8)  & 0xff);	
	outp(0x3ce, 0x46); outp(0x3cf, (LTemp >> 16) & 0xff);		
	outp(0x3ce, 0x47); outp(0x3cf, (LTemp >> 24) & 0xff);

	// pitch
	outp(0x3ce, 0x54); outp(0x3cf, bi.scr.rowbyte & 0xff);
	outp(0x3ce, 0x55); outp(0x3cf, bi.scr.rowbyte >> 8);
	outp(0x3ce, 0x56); outp(0x3cf, bi.scr.rowbyte & 0xff);
	outp(0x3ce, 0x57); outp(0x3cf, bi.scr.rowbyte >> 8);


	wait_for_blit();

    if ((y2 < y1) || ((y2 == y1) && (x2 < x1)))
		cmd = 0x80000000 | 0x0c0000;
	else
		cmd = (0x80000000 | 0x0c0000) | 0x00000002 | 0x00000001 | 0x00000010;

	// control/status cmd
	LTemp = cmd;
	outp(0x3ce, 0x44); outp(0x3cf, LTemp & 0xff);	
	outp(0x3ce, 0x45); outp(0x3cf, (LTemp >> 8)  & 0xff);	
	outp(0x3ce, 0x46); outp(0x3cf, (LTemp >> 16) & 0xff);		
	outp(0x3ce, 0x47); outp(0x3cf, (LTemp >> 24) & 0xff);

	// high color
//	outp(0x3ce, 0x4c); outp(0x3cf, 0x00);
//	outp(0x3ce, 0x4d); outp(0x3cf, 0x00);

	// src start off
    if ((y2 < y1) || ((y2 == y1) && (x2 < x1))) 
		LTemp = (y1 * bi.scr.rowbyte) + (x1 * 2);
	else
		LTemp = ((y1 + height - 1) * bi.scr.rowbyte) + ((x1 + width - 1) * 2);

	outp(0x3ce, 0x60); outp(0x3cf, 0x00);	
	outp(0x3ce, 0x61); outp(0x3cf, 0x00);		
	outp(0x3ce, 0x62); outp(0x3cf, 0x00);
	outp(0x3ce, 0x63); outp(0x3cf, 0x00);

#if 1
	outp(0x3ce, 0x64); outp(0x3cf, LTemp & 0xff);		
	outp(0x3ce, 0x65); outp(0x3cf, (LTemp >> 8)  & 0xff);		
	outp(0x3ce, 0x66); outp(0x3cf, (LTemp >> 16) & 0xff);
	outp(0x3ce, 0x67); outp(0x3cf, (LTemp >> 24) & 0xff);
#else
	outp(0x3ce, 0x60); outp(0x3cf, LTemp & 0xff);		
	outp(0x3ce, 0x61); outp(0x3cf, (LTemp >> 8)  & 0xff);		
	outp(0x3ce, 0x62); outp(0x3cf, (LTemp >> 16) & 0xff);
	outp(0x3ce, 0x63); outp(0x3cf, (LTemp >> 24) & 0xff);
#endif

	// dst start off
    if ((y2 < y1) || ((y2 == y1) && (x2 < x1))) 
		LTemp = (y2 * bi.scr.rowbyte) + (x2 * 2);
	else 
		LTemp = ((y2 + height - 1) * bi.scr.rowbyte) + ((x2 + width - 1) * 2);
#if 1
	outp(0x3ce, 0x6c); outp(0x3cf, LTemp & 0xff);
	outp(0x3ce, 0x6d); outp(0x3cf, (LTemp >> 8)  & 0xff);
	outp(0x3ce, 0x6e); outp(0x3cf, (LTemp >> 16) & 0xff);
	outp(0x3ce, 0x6f); outp(0x3cf, (LTemp >> 24) & 0xff);
#else
	outp(0x3ce, 0x64); outp(0x3cf, LTemp & 0xff);
	outp(0x3ce, 0x65); outp(0x3cf, (LTemp >> 8)  & 0xff);
	outp(0x3ce, 0x66); outp(0x3cf, (LTemp >> 16) & 0xff);
	outp(0x3ce, 0x67); outp(0x3cf, (LTemp >> 24) & 0xff);
#endif

	// xy ext
	LTemp = (height << 16) | (width & 0xffff);
#if 1
	outp(0x3ce, 0x70); outp(0x3cf, LTemp & 0xff);
	outp(0x3ce, 0x71); outp(0x3cf, (LTemp >> 8)  & 0xff);
	outp(0x3ce, 0x72); outp(0x3cf, (LTemp >> 16) & 0xff);
	outp(0x3ce, 0x73); outp(0x3cf, (LTemp >> 24) & 0xff);
#else
	outp(0x3ce, 0x69); outp(0x3cf, LTemp & 0xff);
	outp(0x3ce, 0x6a); outp(0x3cf, (LTemp >> 8)  & 0xff);
	outp(0x3ce, 0x6b); outp(0x3cf, (LTemp >> 16) & 0xff);
	outp(0x3ce, 0x6c); outp(0x3cf, (LTemp >> 24) & 0xff);
#endif

	outp(0x3ce, 0x68); outp(0x3cf, 0x00);
	outp(0x3ce, 0x69); outp(0x3cf, 0x00);
	outp(0x3ce, 0x6a); outp(0x3cf, 0x00);
	outp(0x3ce, 0x6b); outp(0x3cf, 0x00);

	wait_for_blit();

	unlock_io();

	return B_NO_ERROR;
*/
	int32       cmd, src, dst, LTemp;

	if ((x1 == x2) && (y1 == y2))
	  return B_NO_ERROR;

	lock_io();

	src = y1*bi.scr.rowbyte + x1*(bi.scr.depth>>3);
	dst = y2*bi.scr.rowbyte + x2*(bi.scr.depth>>3);

	if (src < dst) {
	  src += bi.scr.rowbyte*height+width*(bi.scr.depth>>3);
	  dst += bi.scr.rowbyte*height+width*(bi.scr.depth>>3);
	  cmd = 0x13;
    }
	else
	  cmd = 0x00;

	width++;
	height++;

	LTemp = 0x1000 | (0x0100<<(bi.scr.depth>>4));
	outp(0x3ce, 0x42); outp(0x3cf, LTemp & 0xff);	
	outp(0x3ce, 0x43); outp(0x3cf, LTemp >> 8);

	outp(0x3ce, 0x44); outp(0x3cf, cmd);		// control/status cmd
	outp(0x3ce, 0x45); outp(0x3cf, bi.scr.depth>>3);		// bit depth
	outp(0x3ce, 0x46); outp(0x3cf, 0x0c);		// rop
	outp(0x3ce, 0x47); outp(0x3cf, 0x00);	// bits 01 == 00

	outp(0x3ce, 0x4c); outp(0x3cf, 0x00);
	outp(0x3ce, 0x4d); outp(0x3cf, 0x00);

	outp(0x3ce, 0x54); outp(0x3cf, bi.scr.rowbyte & 0xff); // dst pitch
	outp(0x3ce, 0x55); outp(0x3cf, bi.scr.rowbyte >> 8);
	outp(0x3ce, 0x56); outp(0x3cf, bi.scr.rowbyte & 0xff); // src pitch
	outp(0x3ce, 0x57); outp(0x3cf, bi.scr.rowbyte >> 8);

	outp(0x3ce, 0x60); outp(0x3cf, src & 0xff);		// dst base addr
	outp(0x3ce, 0x61); outp(0x3cf, (src>>8) & 0xff);
	outp(0x3ce, 0x62); outp(0x3cf, src>>16);
	outp(0x3ce, 0x63); outp(0x3cf, 0x00);		// ?? dst bit offset

	outp(0x3ce, 0x64); outp(0x3cf, dst & 0xff);		// dst base addr
	outp(0x3ce, 0x65); outp(0x3cf, (dst>>8) & 0xff);
	outp(0x3ce, 0x66); outp(0x3cf, dst>>16);
	outp(0x3ce, 0x67); outp(0x3cf, 0x00);		// ?? dst bit offset

	outp(0x3ce, 0x68); outp(0x3cf, width & 0xff);
	outp(0x3ce, 0x69); outp(0x3cf, width >> 8);
	outp(0x3ce, 0x6a); outp(0x3cf, height & 0xff);
	outp(0x3ce, 0x6b); outp(0x3cf, height >> 8);

	wait_for_blit();

	unlock_io();

	return B_NO_ERROR;
}

// Blit a rect from screen to screen (see documentation for more informations)
long kblit_2160(long  x1,     // top-left point of the source
		  long  y1,     //
		  long  x2,     // top-left point of the destination
		  long  y2,     //
		  long  width,  // size of the rect to move (from border included to
		  long  height) // opposite border included).
{
	int32       cmd, src, dst;

	if ((x1 == x2) && (y1 == y2))
	  return B_NO_ERROR;

	lock_io();

	src = y1*bi.scr.rowbyte + x1*(bi.scr.depth>>3);
	dst = y2*bi.scr.rowbyte + x2*(bi.scr.depth>>3);

	if (src < dst) {
	  src += bi.scr.rowbyte*height+width*(bi.scr.depth>>3);
	  dst += bi.scr.rowbyte*height+width*(bi.scr.depth>>3);
	  cmd = 0x13;
    }
	else
	  cmd = 0x00;

	width++;
	height++;

	outp(0x3ce, 0x44); outp(0x3cf, cmd);		// control/status cmd
	outp(0x3ce, 0x45); outp(0x3cf, bi.scr.depth>>3);		// bit depth
	outp(0x3ce, 0x46); outp(0x3cf, 0x0c);		// rop
	outp(0x3ce, 0x47); outp(0x3cf, 0x00);	// bits 01 == 00

	outp(0x3ce, 0x4c); outp(0x3cf, 0x00);
	outp(0x3ce, 0x4d); outp(0x3cf, 0x00);

	outp(0x3ce, 0x54); outp(0x3cf, bi.scr.rowbyte & 0xff); // dst pitch
	outp(0x3ce, 0x55); outp(0x3cf, bi.scr.rowbyte >> 8);
	outp(0x3ce, 0x56); outp(0x3cf, bi.scr.rowbyte & 0xff); // src pitch
	outp(0x3ce, 0x57); outp(0x3cf, bi.scr.rowbyte >> 8);

	outp(0x3ce, 0x60); outp(0x3cf, 0x00);		// ?? dst bit offset
	outp(0x3ce, 0x61); outp(0x3cf, src & 0xff);		// dst base addr
	outp(0x3ce, 0x62); outp(0x3cf, (src>>8) & 0xff);
	outp(0x3ce, 0x63); outp(0x3cf, src>>16);

	outp(0x3ce, 0x64); outp(0x3cf, 0x00);		// ?? dst bit offset
	outp(0x3ce, 0x65); outp(0x3cf, dst & 0xff);		// dst base addr
	outp(0x3ce, 0x66); outp(0x3cf, (dst>>8) & 0xff);
	outp(0x3ce, 0x67); outp(0x3cf, dst>>16);

	outp(0x3ce, 0x69); outp(0x3cf, width & 0xff);
	outp(0x3ce, 0x6a); outp(0x3cf, width >> 8);
	outp(0x3ce, 0x6b); outp(0x3cf, height & 0xff);
	outp(0x3ce, 0x6c); outp(0x3cf, height >> 8);

	wait_for_blit();

	unlock_io();

	return B_NO_ERROR;
}


long krect8_2200(long x1, long y1, long x2, long y2, uchar color)
{
	int32       dst, width, height, LTemp;

	lock_io();

	width = x2-x1+1;
	height =  y2-y1+1;
	dst = y1*bi.scr.rowbyte + x1;

	LTemp = 0x1000 | 0x0100;
	outp(0x3ce, 0x42); outp(0x3cf, LTemp & 0xff);	
	outp(0x3ce, 0x43); outp(0x3cf, LTemp >> 8);

	outp(0x3ce, 0x44); outp(0x3cf, 0x08);		// control/status cmd
	outp(0x3ce, 0x45); outp(0x3cf, 0x01);		// bit depth
	outp(0x3ce, 0x46); outp(0x3cf, 0x0c);		// rop ??

	outp(0x3ce, 0x47); outp(0x3cf, 0x00);	// bits 01 == 00

	outp(0x3ce, 0x4c); outp(0x3cf, color);
	outp(0x3ce, 0x4d); outp(0x3cf, color);

	outp(0x3ce, 0x56); outp(0x3cf, bi.scr.rowbyte & 0xff); // dst pitch
	outp(0x3ce, 0x57); outp(0x3cf, bi.scr.rowbyte >> 8);

	outp(0x3ce, 0x64); outp(0x3cf, dst & 0xff);		// dst base addr
	outp(0x3ce, 0x65); outp(0x3cf, (dst>>8) & 0xff);
	outp(0x3ce, 0x66); outp(0x3cf, dst>>16);
	outp(0x3ce, 0x67); outp(0x3cf, 0x00);		// ?? dst bit offset

	outp(0x3ce, 0x68); outp(0x3cf, width & 0xff);
	outp(0x3ce, 0x69); outp(0x3cf, width >> 8);
	outp(0x3ce, 0x6a); outp(0x3cf, height & 0xff);
	outp(0x3ce, 0x6b); outp(0x3cf, height >> 8);

	wait_for_blit();

	unlock_io();

	return B_NO_ERROR;
}	

long krect16_2200(long x1, long y1, long x2, long y2, uint16 color)
{
	int32       dst, width, height, LTemp;

	lock_io();

	width = x2-x1+1;
	height =  y2-y1+1;
	dst = y1*bi.scr.rowbyte + x1*2;

	LTemp = 0x1000 | 0x0200;
	outp(0x3ce, 0x42); outp(0x3cf, LTemp & 0xff);	
	outp(0x3ce, 0x43); outp(0x3cf, LTemp >> 8);

	outp(0x3ce, 0x44); outp(0x3cf, 0x08);		// control/status cmd
	outp(0x3ce, 0x45); outp(0x3cf, 0x02);		// bit depth
	outp(0x3ce, 0x46); outp(0x3cf, 0x0c);		// rop ??

	outp(0x3ce, 0x47); outp(0x3cf, 0x00);	// bits 01 == 00

	outp(0x3ce, 0x4c); outp(0x3cf, color&0xff);
	outp(0x3ce, 0x4d); outp(0x3cf, color>>8);

	outp(0x3ce, 0x56); outp(0x3cf, bi.scr.rowbyte & 0xff); // dst pitch
	outp(0x3ce, 0x57); outp(0x3cf, bi.scr.rowbyte >> 8);

	outp(0x3ce, 0x64); outp(0x3cf, dst & 0xff);		// dst base addr
	outp(0x3ce, 0x65); outp(0x3cf, (dst>>8) & 0xff);
	outp(0x3ce, 0x66); outp(0x3cf, dst>>16);
	outp(0x3ce, 0x67); outp(0x3cf, 0x00);		// ?? dst bit offset

	outp(0x3ce, 0x68); outp(0x3cf, width & 0xff);
	outp(0x3ce, 0x69); outp(0x3cf, width >> 8);
	outp(0x3ce, 0x6a); outp(0x3cf, height & 0xff);
	outp(0x3ce, 0x6b); outp(0x3cf, height >> 8);

	wait_for_blit();

	unlock_io();

	return B_NO_ERROR;
}	

long krect8_2160(long x1, long y1, long x2, long y2, uchar color)
{
	int32       dst, width, height;

	lock_io();

	width = x2-x1+1;
	height =  y2-y1+1;
	dst = y1*bi.scr.rowbyte + x1;

	outp(0x3ce, 0x44); outp(0x3cf, 0x08);		// control/status cmd
	outp(0x3ce, 0x45); outp(0x3cf, 0x01);		// bit depth
	outp(0x3ce, 0x46); outp(0x3cf, 0x0c);		// rop ??

	outp(0x3ce, 0x47); outp(0x3cf, 0x00);	// bits 01 == 00

	outp(0x3ce, 0x4c); outp(0x3cf, color);
	outp(0x3ce, 0x4d); outp(0x3cf, color);

	outp(0x3ce, 0x56); outp(0x3cf, bi.scr.rowbyte & 0xff); // dst pitch
	outp(0x3ce, 0x57); outp(0x3cf, bi.scr.rowbyte >> 8);

	outp(0x3ce, 0x64); outp(0x3cf, 0x00);		// ?? dst bit offset
	outp(0x3ce, 0x65); outp(0x3cf, dst & 0xff);		// dst base addr
	outp(0x3ce, 0x66); outp(0x3cf, (dst>>8) & 0xff);
	outp(0x3ce, 0x67); outp(0x3cf, dst>>16);

	outp(0x3ce, 0x69); outp(0x3cf, width & 0xff);
	outp(0x3ce, 0x6a); outp(0x3cf, width >> 8);
	outp(0x3ce, 0x6b); outp(0x3cf, height & 0xff);
	outp(0x3ce, 0x6c); outp(0x3cf, height >> 8);

	wait_for_blit();

	unlock_io();

	return B_NO_ERROR;
}	

long krect16_2160(long x1, long y1, long x2, long y2, uint16 color)
{
	int32       dst, width, height;

	lock_io();

	width = x2-x1+1;
	height =  y2-y1+1;
	dst = y1*bi.scr.rowbyte + x1*2;

	outp(0x3ce, 0x44); outp(0x3cf, 0x08);		// control/status cmd
	outp(0x3ce, 0x45); outp(0x3cf, 0x02);		// bit depth
	outp(0x3ce, 0x46); outp(0x3cf, 0x0c);		// rop ??

	outp(0x3ce, 0x47); outp(0x3cf, 0x00);	// bits 01 == 00

	outp(0x3ce, 0x4c); outp(0x3cf, color&0xff);
	outp(0x3ce, 0x4d); outp(0x3cf, color>>8);

	outp(0x3ce, 0x56); outp(0x3cf, bi.scr.rowbyte & 0xff); // dst pitch
	outp(0x3ce, 0x57); outp(0x3cf, bi.scr.rowbyte >> 8);

	outp(0x3ce, 0x64); outp(0x3cf, 0x00);		// ?? dst bit offset
	outp(0x3ce, 0x65); outp(0x3cf, dst & 0xff);		// dst base addr
	outp(0x3ce, 0x66); outp(0x3cf, (dst>>8) & 0xff);
	outp(0x3ce, 0x67); outp(0x3cf, dst>>16);

	outp(0x3ce, 0x69); outp(0x3cf, width & 0xff);
	outp(0x3ce, 0x6a); outp(0x3cf, width >> 8);
	outp(0x3ce, 0x6b); outp(0x3cf, height & 0xff);
	outp(0x3ce, 0x6c); outp(0x3cf, height >> 8);

	wait_for_blit();

	unlock_io();

	return B_NO_ERROR;
}	


long kset_cursor_shape_2070(uchar *data, // XorMask
					  uchar *mask, // AndMask
					  long  dim_h, // Dimensions of the cursor (in pixel)
					  long  dim_v, //
					  long  hot_h, // Position of the hot_spot in the cursor
					  long  hot_v) //
{
	int     i, j, dim_h0;
	bool    init;
	uchar   *buf;

	lock_io();
	
	dim_h0 = (dim_h+7)>>3;

	bi.hotspot_h = hot_h;
	bi.hotspot_v = hot_v;

	buf = ((uchar*)bi.scr.base)+(1<<19)+14*1024;
	for (i=0;i<1024;i++) 
	  buf[i] = 0x00;
	
	for (i=0;i<dim_v;i++)
	  for (j=0;j<dim_h0;j++) {
		buf[i*16+j] = data[j+i*dim_h0]/* & (mask[j+i*dim_h0]^0xff)*/;
		buf[i*16+j+8] = (mask[j+i*dim_h0]^0xff);
	  }

	outp(0x3ce, 0x85); outp(0x3cf, 0x20);
	outp(0x3ce, 0x86); outp(0x3cf, 0x00);

	outp(0x3ce, 0x74); outp(0x3cf, 0x00);
	outp(0x3ce, 0x75); outp(0x3cf, 0x00);
	outp(0x3ce, 0x76); outp(0x3cf, 0x00);
	outp(0x3ce, 0x77); outp(0x3cf, 0x3f);
	outp(0x3ce, 0x78); outp(0x3cf, 0x3f);
	outp(0x3ce, 0x79); outp(0x3cf, 0x3f);

	unlock_io();

	return B_NO_ERROR;
}
	

// Blit a rect from screen to screen (see documentation for more informations)
long kblit_2070(long  x1,     // top-left point of the source
		  long  y1,     //
		  long  x2,     // top-left point of the destination
		  long  y2,     //
		  long  width,  // size of the rect to move (from border included to
		  long  height) // opposite border included).
{
	short	  command;
	int32     src, dst;

	if ((x1 == x2) && (y1 == y2))
	  return B_NO_ERROR;

	lock_io();

	src = y1*bi.scr.rowbyte + x1;
	dst = y2*bi.scr.rowbyte + x2;

	if (src < dst) {
	  src += bi.scr.rowbyte*height+width;
	  dst += bi.scr.rowbyte*height+width;
	  command = 0x13;
    }
	else
	  command = 0x00;

	outp(0x3ce, 0x41); outp(0x3cf, command);
	outp(0x3ce, 0x42); outp(0x3cf, 0x01);
	outp(0x3ce, 0x43); outp(0x3cf, 0x0c);
	outp(0x3ce, 0x50); outp(0x3cf, 0xff);
	outp(0x3ce, 0x51); outp(0x3cf, 0xff);

	outp(0x3ce, 0x54); outp(0x3cf, width & 0xff);
	outp(0x3ce, 0x55); outp(0x3cf, width >> 8);
	outp(0x3ce, 0x56); outp(0x3cf, height & 0xff);
	outp(0x3ce, 0x57); outp(0x3cf, height >> 8);

	outp(0x3ce, 0x58); outp(0x3cf, bi.scr.rowbyte & 0xff);
	outp(0x3ce, 0x59); outp(0x3cf, bi.scr.rowbyte >> 8);
	outp(0x3ce, 0x5c); outp(0x3cf, 0x00);
	outp(0x3ce, 0x5d); outp(0x3cf, src & 0xff);
	outp(0x3ce, 0x5e); outp(0x3cf, (src>>8) & 0xff);
	outp(0x3ce, 0x5f); outp(0x3cf, src>>16);
	outp(0x3ce, 0x60); outp(0x3cf, bi.scr.rowbyte & 0xff);
	outp(0x3ce, 0x61); outp(0x3cf, bi.scr.rowbyte >> 8);
	outp(0x3ce, 0x64); outp(0x3cf, 0x00);
	outp(0x3ce, 0x65); outp(0x3cf, dst & 0xff);
	outp(0x3ce, 0x66); outp(0x3cf, (dst>>8) & 0xff);
	outp(0x3ce, 0x67); outp(0x3cf, dst>>16);

	wait_for_blit();

	unlock_io();

	return B_NO_ERROR;
}

long krect8_2070(long x1, long y1, long x2, long y2, uchar color)
{
	int32     dst, width, height;

	lock_io();

	width = x2-x1;
	height =  y2-y1;
	dst = y1*bi.scr.rowbyte + x1;

	outp(0x3ce, 0x41); outp(0x3cf, 0x08);
	outp(0x3ce, 0x42); outp(0x3cf, 0x01);
	outp(0x3ce, 0x43); outp(0x3cf, 0x0f);
	outp(0x3ce, 0x48); outp(0x3cf, color);
	outp(0x3ce, 0x49); outp(0x3cf, color);
	outp(0x3ce, 0x4c); outp(0x3cf, color);
	outp(0x3ce, 0x4d); outp(0x3cf, color);
	outp(0x3ce, 0x50); outp(0x3cf, 0xff);
	outp(0x3ce, 0x51); outp(0x3cf, 0xff);

	outp(0x3ce, 0x54); outp(0x3cf, width & 0xff);
	outp(0x3ce, 0x55); outp(0x3cf, width >> 8);
	outp(0x3ce, 0x56); outp(0x3cf, height & 0xff);
	outp(0x3ce, 0x57); outp(0x3cf, height >> 8);

	outp(0x3ce, 0x60); outp(0x3cf, bi.scr.rowbyte & 0xff);
	outp(0x3ce, 0x61); outp(0x3cf, bi.scr.rowbyte >> 8);
	outp(0x3ce, 0x64); outp(0x3cf, 0x00);
	outp(0x3ce, 0x65); outp(0x3cf, dst & 0xff);
	outp(0x3ce, 0x66); outp(0x3cf, (dst>>8) & 0xff);
	outp(0x3ce, 0x67); outp(0x3cf, dst>>16);

	wait_for_blit();

	unlock_io();

	return B_NO_ERROR;
}	


static long
OpenGraphicsCard(graphics_card_spec *buf)
{
    int    i;
	screen *scr = &bi.scr;

/*	
	for (i=0; i<256; i++) {
		if ((i & 15) == 0)
			_sPrintf("[0x%02x]: ", i);
		outp(0x3ce, i);
		_sPrintf("%02x ", inp(0x3cf));
		if ((i & 15) == 15)
			_sPrintf("\n");
	}
*/

	if (!buf)
		return B_ERROR;

	if (buf->vendor_id != NEOMAGIC_VENDOR)
		return B_ERROR;

	if (FindNeomagic(buf->screen_base) != B_NO_ERROR)
		return B_ERROR;

	scr->base 	= (void*)(((int32)buf->screen_base));
	scr->width	= 800;			/* XXX how? */
	scr->height	= 600;
	scr->rowbyte= 800;
	scr->refresh= DEFAULT_REFRESH_RATE;
	scr->depth	= 8;
	scr->use_stub= FALSE;

	for (i=0; i<((scr->width*scr->height)>>2); i++)
	  ((uint32*)scr->base)[i] = 0;

	init_locks();
		
	SetGraphicMode();

	return B_NO_ERROR;
}

static void
CloseGraphicsCard(void)
{
	ScreenOff();
	dispose_locks();
}

static long
SetIndexedColor(indexed_color *buf)
{
	outp(0x3c8, buf->index);
	outp(0x3c9, buf->color.red>>2); 
	outp(0x3c9, buf->color.green>>2);
	outp(0x3c9, buf->color.blue>>2);
	return B_NO_ERROR;
}

static long
GetGraphicsCardHooks(graphics_card_hook *buf)
{
	int    i;
	
	memset(buf, 0, sizeof(graphics_card_hook));

	if (device_id == NM2070) {
		buf[0] = kset_cursor_shape_2070;
		buf[1] = kmove_cursor;
		buf[2] = kshow_cursor;
		buf[5] = krect8_2070;
		buf[7] = kblit_2070;
	}
	else if (device_id == NM2160) {
		buf[0] = kset_cursor_shape_2160;
		buf[1] = kmove_cursor;
		buf[2] = kshow_cursor;
		buf[5] = krect8_2160;
		buf[7] = kblit_2160;
		buf[13] = krect16_2160;
	}
	else if (device_id == NM2200) {
		buf[0] = kset_cursor_shape_2200;
		buf[1] = kmove_cursor;
		buf[2] = kshow_cursor;
		buf[5] = krect8_2200;
		buf[7] = kblit_2200;
		buf[13] = krect16_2200;
	}
}

static long
GetGraphicsCardInfo(graphics_card_info *info)
{
	screen *scr = &bi.scr;

	info->version	= 2;
	info->id		= 0;
	
	info->height		= scr->height;
	info->width			= scr->width;
	info->bits_per_pixel= scr->depth;
	info->frame_buffer	= scr->base;
	info->bytes_per_row = scr->rowbyte;
	info->flags			= B_PARALLEL_BUFFER_ACCESS;
	*(long*)&info->rgba_order = 'argb';
	
	return B_NO_ERROR;
}

static long
GetScreenSpaces(graphics_card_config *config)
{
	config->space = 
		B_8_BIT_800x600 | 
		B_16_BIT_800x600 | 
		B_8_BIT_640x480 | 
		B_8_BIT_1024x768 |
		B_16_BIT_1024x768;
	return B_NO_ERROR;
}

static void
InitGamma16()
{
	int i;

	outp(0x3c8, 0x0);
	for (i = 0; i < 0x40; i++) {
		outp(0x3c9, i<<1);
		outp(0x3c9, i);
		outp(0x3c9, i<<1);
	}
	return;
}

static long
ConfigGraphicsCard(graphics_card_config *config)
{
	long err = B_ERROR;

	ulong ourspaces = 
		B_8_BIT_800x600 | 
		B_16_BIT_800x600 | 
		B_8_BIT_640x480 | 
		B_8_BIT_1024x768 | 
		B_16_BIT_1024x768;

/*	config->space = B_8_BIT_800x600;*/
	if ((config->space & ourspaces) == 0)
		goto bail;
	
	err = B_NO_ERROR;
	switch (config->space) {
	case B_8_BIT_640x480 :
		bi.scr.width	= 640;
		bi.scr.height	= 480;
		bi.scr.rowbyte  = 640;
		bi.scr.depth = 8;
		break;

	case B_8_BIT_800x600 :
		bi.scr.width	= 800;
		bi.scr.height	= 600;
		bi.scr.rowbyte  = 800;
		bi.scr.depth = 8;
		break;

	case B_16_BIT_640x480:
		bi.scr.depth = 16;
		bi.scr.width	= 640;
		bi.scr.height	= 480;
		bi.scr.rowbyte  = 640*2;
		InitGamma16();
		break;

	case B_16_BIT_800x600:
		bi.scr.depth = 16;
		bi.scr.width	= 800;
		bi.scr.height	= 600;
		bi.scr.rowbyte  = 800*2;
		InitGamma16();
		break;

	case B_8_BIT_1024x768:
		bi.scr.depth = 8;
		bi.scr.width	= 800;
		bi.scr.height	= 600;
		bi.scr.rowbyte  = 800;
		SetGraphicMode();
		snooze(40000);
		bi.scr.width	= 1024;
		bi.scr.height	= 768;
		bi.scr.rowbyte  = 1024;
		break;

	case B_16_BIT_1024x768:
		bi.scr.depth = 16;
		bi.scr.width	= 1024;
		bi.scr.height	= 768;
		bi.scr.rowbyte  = 1024*2;
		InitGamma16();
		break;

	default:
		err =  B_ERROR;
		goto bail;
	}

	SetGraphicMode();
	
bail:	
	return err;
}

long
control_graphics_card(ulong message, void *buf)
{
	long err = B_NO_ERROR;

	switch (message) {
	case B_OPEN_GRAPHICS_CARD:
		err = OpenGraphicsCard((graphics_card_spec *) buf);
		break;

	case B_CLOSE_GRAPHICS_CARD:
		CloseGraphicsCard();
		break;

	case B_SET_INDEXED_COLOR:
		err = SetIndexedColor((indexed_color *) buf);
		break;

    case B_GET_GRAPHICS_CARD_HOOKS:
		err = GetGraphicsCardHooks((graphics_card_hook *) buf);
		break;

	case B_GET_GRAPHICS_CARD_INFO:
		err = GetGraphicsCardInfo((graphics_card_info*) buf);
		break;
		
	case B_GET_SCREEN_SPACES:
		err = GetScreenSpaces((graphics_card_config*) buf);
		break;
		
	case B_CONFIG_GRAPHICS_CARD:
		err = ConfigGraphicsCard((graphics_card_config*) buf);
		break;

	case B_GET_REFRESH_RATES:
		((refresh_rate_info*)buf)->min = bi.scr.refresh;
		((refresh_rate_info*)buf)->max = bi.scr.refresh;
		((refresh_rate_info*)buf)->current = bi.scr.refresh;
		break;

#if 0
	case 1000000000:
		// screen saver mode
		return ((long)set_screen_saver_mode);
#endif

	default:
		err = B_ERROR;
		break;
	}
	return err;
}

