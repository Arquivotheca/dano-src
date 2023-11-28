#include <GraphicsDefs.h>
#include <Accelerant.h>
#include "ati_private.h"
#include <video_overlay.h>
#include <surface/genpool.h>

#define SCREEN_ON	STORM8W(VGA_SEQ_INDEX, VGA_SEQ1); STORM8W(VGA_SEQ_DATA, STORM8(VGA_SEQ_DATA) & 0xdf);
#define SCREEN_OFF	STORM8W(VGA_SEQ_INDEX, VGA_SEQ1); STORM8W(VGA_SEQ_DATA, STORM8(VGA_SEQ_DATA) | 0x20);

#define UNKNOWN_CARD 0xFFFF

typedef struct {
	int	used;	// non-zero if in use
	// these are used to reposition the window during virtual desktop moves
	const overlay_buffer *ob;	// current overlay_buffer for this overlay
	overlay_window ow;	// current overlay_window for this overlay
	overlay_view ov;	// current overlay_view for this overlay
} ati_overlay_token;

typedef struct {
short latency;
short latch;
short trp;
short trcd;
short tcrd;
short tr2w; // new
short phase;    // new
short oe;   // new
short tras;
} mem_cycle_type;


typedef struct {
	uint64
		last_idle_fifo,		/* last fifo slot we *know* the engine was idle after */
		fifo_count,		/* last fifo slot used */
		fifo_limit;		/* slot age after which the command is guaranteed complete */
	int32
		engine_ben;		/* for acceleration benephore */
	uint32
		fifo_mask,		/* bit mask for retrieving FIFO free slot count */
		set_cursor_colors,
		cursor_is_visible,
		mode_count,
		mem_size,
		start_addr,
		bytes_per_pixel,
		pixels_per_row,
		pix_clk_max8,
		pix_clk_max16,
		pix_clk_max32;
	uint16
		hot_x,	/* cursor hot spot */
		hot_y;
	sem_id
		engine_sem;	/* semaphore for engine ownership */
	area_id
		mode_list_area;
	display_mode
		dm;		/* current display mode configuration */
	frame_buffer_config
		fbc;	/* bytes_per_row and start of frame buffer */

	struct {
		float MaxXClk;		// The fastest internal bus clock this card supports.
		float RefClock;		// PLL reference clock frequency, Hz.
		float XClock;		// XCLK clock frequency, Hz.
		float Latency;		// FIFO loop latency parameter.
		float PageSize;		// Page size parameter.
		float MaxVClock;	// Maximum pixel clock supported, Hz.
		float MinXPerQ;		// Minimum number of XClocks per QWord.
		short ram_type;
		mem_cycle_type mem_cycle;
	} Card;
	
	struct {
		// Pointer to cursor image in card memory. Qword-aligned, 1024 bytes.
		void *pCursorBuffer;
		int CursorDefined;	// Indicates whether or not a cursor has been defined.
		int Width;			// Cursor width and height.
		int Height;
		int HotX;			// Cursor hot-spot location.
		int HotY;
		BMemSpec cursor_spec;	// info about cursor memory allocation when using memtypes
	} Cursor;

	uint32
		PanelID;			// on LT cards, the LCD panel type
	uint32 UsingLCD;		// if the LCD is in use for the current display mode
	uint32 poolid;			// memory pool id for frame buffer
	BMemSpec fb_spec;		// frame buffer allocation
	BMemSpec ovl_buffer_specs[4];	// overlay buffer allocations
	overlay_buffer ovl_buffers[4];	// overlay buffers
	ati_overlay_token ovl_tokens[1];// storage for each overlay

} accelerant_info;

typedef struct
{
  unsigned short VendorID;	// The PCI vendor ID.
  unsigned short DeviceID;	// The PCI device ID.
  float MaxXClk;			// The fastest internal bus clock this card supports.
  float HitXClks;			// The number of xclocks per memory fetch on a page hit.
} SUPPORTEDDEVICESTRUCT;

uint32 get_pci(uint32 offset, uint32 size);
void   set_pci(uint32 offset, uint32 size, uint32 value);
void create_mode_list(void);
void calc_max_clocks(void);

extern int fd;
extern int memtypefd;
extern int can_do_overlays;
extern shared_info *si;
extern accelerant_info *ai;
extern vuint32 *regs;
extern area_id	my_mode_list_area;
extern display_mode *my_mode_list;
extern SUPPORTEDDEVICESTRUCT SupportedDevices[];
extern int accelerantIsClone;

#if DEBUG > 0
#include <stdio.h>
#if DEBUG > 1
#include <Debug.h>
#define ddprintf(a) SERIAL_PRINT(a)
#else
#define ddprintf(a) printf a
#endif
#define dump_regs(a) ddump_regs a
extern void ddump_regs(const char *name, bool serial);
#else
#define ddprintf(a)
#define dump_regs(a)
#endif
