#include <BeBuild.h>
#include <interface/GraphicsDefs.h>
#include "webpad_private.h"

static accelerant_info	*ai;
static shared_info		*si;
static display_mode		*mode_list;
static uint32			mode_count;
static int				file_handle;
static engine_token gxm_engine_token = { 1, B_2D_ACCELERATION, NULL };
static accelerantIsClone = 0;

#if 0
static bool				screen_saver_enable = false;
#endif

// GLOBAL ARRAY OF DISPLAY MODE STRUCTURES

// NOTE: ALL VIDEO MODES ARE SET TO A PITCH OF EITHER 1K OR 2K
// This is to accomodate using the graphics acceleration, which must
// pitch of 1K or 2K bytes to work properly.

DISPLAYMODE DisplayParams[] = {

#if 0
{ 640, 480, 8, 60, 									// display parameters
  0x00006541, 0x0000036F, 0x00003005, 				// gcfg, tcfg, ocfg
  0x00000000, 0x00000280, 0x001FFF00, 				// memory organization
  0x00100100, 0x00008250, 							// line delta, buffer size
  0x031F027F, 0x03170287, 0x02E7028F, 0x02E7028F,	// horizontal timings
  0x020C01DF, 0x020401E7, 0x01EB01E9, 0x01EB01E9,	// vertical timings
  0x7D, 0x47,										// ICS5342 = 50.350 MHz
  0x37058AFF, 										// CX5520 = 50.350 MHz
},
#endif

#if 0
{ 800, 600, 8, 60, 									// display parameters
  0x00006541, 0x0000006F, 0x00003005, 				// gcfg, tcfg, ocfg
  0x00000000, 0x00000320, 0x001FFF00, 				// memory organization
  0x00100100, 0x00006A64, 							// line delta, buffer size
  0x041F031F, 0x04170327, 0x03CF0347, 0x03CF0347,	// horizontal timings
  0x02730257, 0x02730257, 0x025C0258, 0x025C0258,	// vertical timings
  0x79, 0x29,										// ICS5342 = 80.000 MHz
  0x271B1DFF,									    // CX5520 = 80.000 MHz
},
#endif

#if 0
// original working parms
{ 800, 600, 16, 60, 								// display parameters
  0x00007641, 0x0000106F, 0x00003004, 				// gcfg, tcfg, ocfg
  0x00000000, 0x00000700, 0x0012c000, 				// memory organization
  0x00200200, 0x000082CA, 							// line delta, buffer size
  0x041F831F, 0x041F831F, 0x03C78347, 0x03C78347,	// horizontal timings
  0x02738257, 0x02738257, 0x02668262, 0x025B8257,	// vertical timings
  0x79, 0x29,										// ICS5342 = 80.000 MHz
  0x271B1DFF,									    // CX5520 = 80.000 MHz
},
#endif

{ 800, 600, 16, 60, 								// display parameters
  0x00017671, 0x0000106F, 0x00003004, 				// gcfg, tcfg, ocfg
  0x00000000, 0x00000700, 0x0012c000, 				// memory organization
  0x00200200, 0x00007CCA, 							// line delta, buffer size
  0x041F831F, 0x041F831F, 0x03C78347, 0x03C78347,	// horizontal timings
  0x02738257, 0x02738257, 0x02668262, 0x025B8257,	// vertical timings
  0x79, 0x29,										// ICS5342 = 80.000 MHz
  0x271B1DFF,									    // CX5520 = 80.000 MHz
},

#if 0
{ 1024, 768, 8, 60, 								// display parameters
  0x00006581, 0x0000006F, 0x00003005, 				// gcfg, tcfg, ocfg
  0x00000000, 0x000C0000, 0x001FFF00, 				// memory organization
  0x00044100, 0x00008280, 							// line delta, buffer size
  0x053F03FF, 0x053F03FF, 0x049F0417, 0x049F0417,	// horizontal timings
  0x032502FF, 0x032502FF, 0x03080302, 0x03080302,	// vertical timings
  0x77, 0x29,										// ICS5342 = 78.750 MHz
  0x27915BFF,									    // CX5520 = 78.750 MHz
},
#endif

};

#if 0
//----------------------------------------------------------------------
// HARDWARE CONFIGURATION

//static int32	SysTurnonMode = 1;			// Nothing was set by the BIOS
//static int32	SysGXM = 1;					// System contains GXm
//static int32	SysICS5342 = 0;				// System contains ICS5432
static int32	SysFlatPanel = 1;			// System contains flat panel
static int32	SysCX5520 = 0;				// System contains CX5520
#endif


/*----------------------------------------------------------
 driver internal states
----------------------------------------------------------*/

static	volatile uchar		*GXregisters;
static 	volatile uchar		*CX5520registers;

#define DEBUG 0
#if DEBUG > 0
#include <errno.h>
#if 0
#include <stdio.h>
#define xprintf(a) printf a
#else
extern _kdprintf_(char *, ...);
#define xprintf(a) _kdprintf_ a
#define yprintf(a)
#endif
#else
#define xprintf(a)
#define yprintf(a)
#endif

static status_t gxm_init(int fd);
static void gxm_uninit(void);

static void scrn2scrn(engine_token *et, blit_params *list, uint32 count);
static void rectfill16(engine_token *et, uint32 color, fill_rect_params *list, uint32 count);
static void spanfill16(engine_token *et, uint32 color, uint16 *list, uint32 count);


void gxm_set_indexed_colors(uint count, uint8 first, uint8 *color_data, uint32 flags);
static uint32 gxm_accelerant_mode_count(void);
static status_t gxm_get_mode_list(display_mode *modes);
status_t spaces_to_mode_list(uint32 spaces, uint32 flags);
static void set_standard_timing(display_mode *dm, double refresh);
static status_t gxm_set_mode(display_mode *dm);
static status_t gxm_get_mode(display_mode *dm);
static status_t gxm_get_fb_config(frame_buffer_config *a_frame_buffer);
static status_t gxm_move_display_area(uint16 h_display_start, uint16 v_display_start);

static status_t gxm_set_cursor_shape(uint16 width, uint16 height, uint16 hot_x, uint16 hot_y, uint8 *andMask, uint8 *xorMask);
static void gxm_move_cursor(uint16 x, uint16 y);
static void gxm_show_cursor(bool is_visible);
static status_t setcursorcolors (uint32 blackcolor, uint32 whitecolor);

static uint32 gxm_get_engine_count(void);
static status_t gxm_acquire_engine(uint32 caps, uint32 max_wait, sync_token *st, engine_token **et);
static status_t gxm_release_engine(engine_token *et, sync_token *st);
static void gxm_wait_engine_idle(void);
static status_t gxm_get_sync_token(engine_token *et, sync_token *st);
static status_t gxm_sync_to_token(sync_token *st);

static status_t gxm_get_pixel_clock_limits(display_mode *dm, uint32 *low, uint32 *high);
static uint32 calc_pixel_clock_from_refresh(display_mode *dm, float refresh_rate);
static void make_mode_list(void);

#if 0
static bool gxm_set_screen_saver(bool enable);
#endif

#if DEBUG > 0

static const char *spaceToString(uint32 cs) {
	const char *s;
	switch (cs) {
#define s2s(a) case a: s = #a ; break
		s2s(B_RGB32);
		s2s(B_RGBA32);
		s2s(B_RGB32_BIG);
		s2s(B_RGBA32_BIG);
		s2s(B_RGB16);
		s2s(B_RGB16_BIG);
		s2s(B_RGB15);
		s2s(B_RGBA15);
		s2s(B_RGB15_BIG);
		s2s(B_RGBA15_BIG);
		s2s(B_CMAP8);
		s2s(B_GRAY8);
		s2s(B_GRAY1);
		s2s(B_YCbCr422);
		s2s(B_YCbCr420);
		s2s(B_YUV422);
		s2s(B_YUV411);
		s2s(B_YUV9);
		s2s(B_YUV12);
		default:
			s = "unknown"; break;
#undef s2s
	}
	return s;
}

static void dump_mode(display_mode *dm) {
	display_timing *t = &(dm->timing);
	xprintf(("  pixel_clock: %ldKHz\n", t->pixel_clock));
	xprintf(("            H: %4d %4d %4d %4d\n", t->h_display, t->h_sync_start, t->h_sync_end, t->h_total));
	xprintf(("            V: %4d %4d %4d %4d\n", t->v_display, t->v_sync_start, t->v_sync_end, t->v_total));
	xprintf((" timing flags:"));
	if (t->flags & B_BLANK_PEDESTAL) xprintf((" B_BLANK_PEDESTAL"));
	if (t->flags & B_TIMING_INTERLACED) xprintf((" B_TIMING_INTERLACED"));
	if (t->flags & B_POSITIVE_HSYNC) xprintf((" B_POSITIVE_HSYNC"));
	if (t->flags & B_POSITIVE_VSYNC) xprintf((" B_POSITIVE_VSYNC"));
	if (t->flags & B_SYNC_ON_GREEN) xprintf((" B_SYNC_ON_GREEN"));
	if (!t->flags) xprintf((" (none)\n"));
	else xprintf(("\n"));
	xprintf((" refresh rate: %4.2f\n", ((double)t->pixel_clock * 1000) / ((double)t->h_total * (double)t->v_total)));
	xprintf(("  color space: %s\n", spaceToString(dm->space)));
	xprintf((" virtual size: %dx%d\n", dm->virtual_width, dm->virtual_height));
	xprintf(("dispaly start: %d,%d\n", dm->h_display_start, dm->v_display_start));

	xprintf(("   mode flags:"));
	if (dm->flags & B_SCROLL) xprintf((" B_SCROLL"));
	if (dm->flags & B_8_BIT_DAC) xprintf((" B_8_BIT_DAC"));
	if (dm->flags & B_HARDWARE_CURSOR) xprintf((" B_HARDWARE_CURSOR"));
	if (dm->flags & B_PARALLEL_ACCESS) xprintf((" B_PARALLEL_ACCESS"));
//	if (dm->flags & B_SUPPORTS_OVERLAYS) xprintf((" B_SUPPORTS_OVERLAYS"));
	if (!dm->flags) xprintf((" (none)\n"));
	else xprintf(("\n"));
}

void paint_for_blit(display_mode *dm, frame_buffer_config *fbc) {
	switch (dm->space & ~0x3000) {
	case B_CMAP8: {
		int16 x, y;
		uint8 *fb = (uint8 *)fbc->frame_buffer;
		xprintf((" frame buffer is 8bpp\n"));
		/* make a checkerboard pattern */
		for (y = 0; y < (dm->virtual_height >> 1); y++) {
			for (x = 0; x < (dm->virtual_width >> 1); x++) {
				fb[x] = 0;
			}
			for (; x < dm->virtual_width; x++) {
				fb[x] = 0xff;
			}
			fb += fbc->bytes_per_row;
		}
		for (; y < dm->virtual_height; y++) {
			for (x = 0; x < (dm->virtual_width >> 1); x++) {
				fb[x] = 0xff;
			}
			for (; x < dm->virtual_width; x++) {
				fb[x] = x;	// 0
			}
			fb += fbc->bytes_per_row;
		}
		fb = (uint8 *)(((uint8 *)fbc->frame_buffer) + fbc->bytes_per_row);
		fb += 1;
		for (y = 0; y < 40; y++) {
			for (x = 0; x < 40; x++) {
				fb[x] = 0x77;
			}
			fb = (uint8 *)(((uint8 *)fb) + fbc->bytes_per_row);
		}
		fb = (uint8 *)(((uint8 *)fbc->frame_buffer) + fbc->bytes_per_row * 11);
		fb += 11; 
		for (y = 0; y < 20; y++) {
			for (x = 0; x < 20; x++) {
				fb[x] = 0;
			}
			fb = (uint8 *)(((uint8 *)fb) + fbc->bytes_per_row);
		}
	} break;
	case B_RGB16_BIG:
	case B_RGB16_LITTLE: {
		int x, y;
		uint16 *fb = (uint16 *)fbc->frame_buffer;
		xprintf((" frame buffer is 16bpp\n"));
		/* make a checkerboard pattern */
		for (y = 0; y < (dm->virtual_height >> 1); y++) {
			for (x = 0; x < (dm->virtual_width >> 1); x++) {
				fb[x] = 0;
			}
			for (; x < dm->virtual_width; x++) {
				fb[x] = 0xffff;
			}
			fb = (uint16 *)(((uint8 *)fb) + fbc->bytes_per_row);
		}
		for (; y < dm->virtual_height; y++) {
			for (x = 0; x < (dm->virtual_width >> 1); x++) {
				fb[x] = 0xffff;
			}
			for (; x < dm->virtual_width; x++) {
				fb[x] = 0;
			}
			fb = (uint16 *)((uint8 *)fb + fbc->bytes_per_row);
		}
		fb = (uint16 *)(((uint8 *)fbc->frame_buffer) + fbc->bytes_per_row);
		fb += 1;
		for (y = 0; y < 40; y++) {
			for (x = 0; x < 40; x++) {
				fb[x] = 0x7777;
			}
			fb = (uint16 *)(((uint8 *)fb) + fbc->bytes_per_row);
		}
		fb = (uint16 *)(((uint8 *)fbc->frame_buffer) + fbc->bytes_per_row * 11);
		fb += 11; 
		for (y = 0; y < 20; y++) {
			for (x = 0; x < 20; x++) {
				fb[x] = 0;
			}
			fb = (uint16 *)(((uint8 *)fb) + fbc->bytes_per_row);
		}
	} break;
	case B_RGB15_BIG:
	case B_RGBA15_BIG:
	case B_RGB15_LITTLE:
	case B_RGBA15_LITTLE: {
		int x, y;
		uint16 *fb = (uint16 *)fbc->frame_buffer;
		uint16 pixel;
		xprintf((" frame buffer is 15bpp\n"));
		/* make a checkerboard pattern */
		for (y = 0; y < (dm->virtual_height >> 1); y++) {
			for (x = 0; x < (dm->virtual_width >> 1); x++) {
				fb[x] = 0;
			}
			for (; x < dm->virtual_width; x++) {
				fb[x] = 0x7fff;
			}
			fb = (uint16 *)(((uint8 *)fb) + fbc->bytes_per_row);
		}
		for (; y < dm->virtual_height; y++) {
			for (x = 0; x < (dm->virtual_width >> 1); x++) {
				fb[x] = 0x7fff;
			}
			for (; x < dm->virtual_width; x++) {
				fb[x] = 0;
			}
			fb = (uint16 *)((uint8 *)fb + fbc->bytes_per_row);
		}
		fb = (uint16 *)(((uint8 *)fbc->frame_buffer) + fbc->bytes_per_row);
		fb += 1;
		for (y = 0; y < 42; y++) {
			pixel = 0x7777;
			if (y != 40)
			for (x = 0; x < 42; x++) {
				if (x != 40) fb[x] = pixel += 0x0011;
			}
			fb = (uint16 *)(((uint8 *)fb) + fbc->bytes_per_row);
		}
		fb = (uint16 *)(((uint8 *)fbc->frame_buffer) + fbc->bytes_per_row * 11);
		fb += 11; 
		for (y = 0; y < 20; y++) {
			for (x = 0; x < 20; x++) {
				fb[x] = 0;
			}
			fb = (uint16 *)(((uint8 *)fb) + fbc->bytes_per_row);
		}
	} break;
	case B_RGB32_BIG:
	case B_RGBA32_BIG:
	case B_RGB32_LITTLE:
	case B_RGBA32_LITTLE: {
		int x, y;
		uint32 *fb = (uint32 *)fbc->frame_buffer;
		xprintf((" frame buffer is 32bpp\n"));
		/* make a checkerboard pattern */
		for (y = 0; y < (dm->virtual_height >> 1); y++) {
			for (x = 0; x < (dm->virtual_width >> 1); x++) {
				fb[x] = 0;
			}
			for (; x < dm->virtual_width; x++) {
				fb[x] = 0xffffffff;
			}
			fb = (uint32 *)((uint8 *)fb + fbc->bytes_per_row);
		}
		for (; y < dm->virtual_height; y++) {
			for (x = 0; x < (dm->virtual_width >> 1); x++) {
				fb[x] = 0xffffffff;
			}
			for (; x < dm->virtual_width; x++) {
				fb[x] = 0;
			}
			fb = (uint32 *)((uint8 *)fb + fbc->bytes_per_row);
		}
		fb = (uint32 *)(((uint8 *)fbc->frame_buffer) + fbc->bytes_per_row);
		fb += 1;
		for (y = 0; y < 40; y++) {
			for (x = 0; x < 40; x++) {
				fb[x] = 0x77777777;
			}
			fb = (uint32 *)(((uint8 *)fb) + fbc->bytes_per_row);
		}
		fb = (uint32 *)(((uint8 *)fbc->frame_buffer) + fbc->bytes_per_row * 11);
		fb += 11; 
		for (y = 0; y < 20; y++) {
			for (x = 0; x < 20; x++) {
				fb[x] = 0;
			}
			fb = (uint32 *)(((uint8 *)fb) + fbc->bytes_per_row);
		}
	} break;
	default:
		xprintf(("YIKES! frame buffer shape unknown!\n"));
	}
}

#endif

#define SET_AI_FROM_SI(si) ((accelerant_info *)(si + 1))

static ssize_t ACCELERANT_CLONE_INFO_SIZE(void) {
	return MAX_GXM_DEVICE_NAME_LENGTH;
}

static void GET_ACCELERANT_CLONE_INFO(void *data) {
	/* a null terminated string with the device name */
	gxm_device_name dn;
	status_t result;
	
	dn.magic = GXM_PRIVATE_DATA_MAGIC;
	dn.name = (char *)data;
	result = ioctl(file_handle, GXM_DEVICE_NAME, &dn, sizeof(dn));
	xprintf(("GET_ACCELERANT_CLONE_INFO - ioctl(GXM_DEVICE_NAME) returns %ld\n", result));
}

static status_t gxm_init_common(int fd)
{
	status_t result;
	gxm_get_private_data gpd;

	/* memorise the file descriptor */
	file_handle = fd;
	/* set the magic number so the driver knows we're for real */
	gpd.magic = GXM_PRIVATE_DATA_MAGIC;
	/* contact driver and get a pointer to the registers and shared data */
	result = ioctl(fd, GXM_GET_PRIVATE_DATA, &gpd, sizeof(gpd));
	xprintf(("gxm_init_common - ioctl(GXM_GET_PRIVATE_DATA) returns %d\n", result));
	if (result != B_OK) return result;
	
	/* transfer the info to our globals */
	si = gpd.si;
	ai = SET_AI_FROM_SI(si);
	GXregisters = si->regs;
	CX5520registers = si->dac;
	return B_OK;
}

static status_t gxm_init(int fd) {
	status_t result;

#if 0
	//if (screen_saver_enable) return B_ERROR;
#endif
	result = gxm_init_common(fd);
	if (result != B_OK) goto err0;

	/* make mode list from display table */
	make_mode_list();

	/* a little more initialization */
	INIT_BEN(ai->engine);
	if (ai->engine.sem < 0) {
		result = ai->engine.sem;
		free(mode_list);
		mode_list = 0;
		mode_count = 0;
	}
	ai->fifo_count = 0;
	setcursorcolors(0x00000000, 0x00ffffff);
{
	xprintf(("gcfg: %.8lx\n", READ_REG32(GXregisters, DC_GENERAL_CFG)));
	xprintf(("tcfg: %.8lx\n", READ_REG32(GXregisters, DC_TIMING_CFG)));
	xprintf(("ocfg: %.8lx\n", READ_REG32(GXregisters, DC_OUTPUT_CFG)));
	xprintf(("dcfg: %.8lx\n", READ_REG32(CX5520registers, CX5520_DISPLAY_CONFIG)));
	xprintf(("fb_offset: %.8lx\n", READ_REG32(GXregisters, DC_FB_ST_OFFSET)));
	xprintf(("cb_offset: %.8lx\n", READ_REG32(GXregisters, DC_CB_ST_OFFSET)));
	xprintf(("curs_offset: %.8lx\n", READ_REG32(GXregisters, DC_CURS_ST_OFFSET)));
	xprintf(("line_delta: %.8lx\n", READ_REG32(GXregisters, DC_LINE_DELTA)));
	xprintf(("buffer_size: %.8lx\n", READ_REG32(GXregisters, DC_BUF_SIZE)));
	xprintf(("htiming1: %.8lx\n", READ_REG32(GXregisters, DC_H_TIMING_1)));
	xprintf(("htiming2: %.8lx\n", READ_REG32(GXregisters, DC_H_TIMING_2)));
	xprintf(("htiming3: %.8lx\n", READ_REG32(GXregisters, DC_H_TIMING_3)));
	xprintf(("fp_htiming: %.8lx\n", READ_REG32(GXregisters, DC_FP_H_TIMING)));
	xprintf(("vtiming1: %.8lx\n", READ_REG32(GXregisters, DC_V_TIMING_1)));
	xprintf(("vtiming2: %.8lx\n", READ_REG32(GXregisters, DC_V_TIMING_2)));
	xprintf(("vtiming3: %.8lx\n", READ_REG32(GXregisters, DC_V_TIMING_3)));
	xprintf(("fp_vtiming: %.8lx\n", READ_REG32(GXregisters, DC_FP_V_TIMING)));
	xprintf(("cx5520_clock: %.8lx\n", READ_REG32(CX5520registers, 0x0E14)));
}

err0:
	xprintf(("gxm_init() completes with result %d\n", result));
	return result;
}

void gxm_uninit(void) {
	xprintf(("gxm_uninit()\n"));
	if (accelerantIsClone == 0)
	{
		xprintf(("primary accelerant deleting benaphore\n"));
		DELETE_BEN(ai->engine);
		if (mode_list) free(mode_list);
		mode_list = 0;
		mode_count = 0;
	}
	xprintf(("closing our driver handle\n"));
	close(file_handle);
	xprintf(("gxm_uninit() completes OK\n"));
}

static status_t CLONE_ACCELERANT(void *data) {
	status_t result = B_OK;
	gxm_get_private_data gpd;
	char path[MAXPATHLEN];
	int fd;

	/* the data is the device name */
	strcpy(path, "/dev/");
	strcat(path, (const char *)data);
	xprintf(("CLONE_ACCELERANT: opening %s\n", path));
	/* open the device, the permissions aren't important */
	fd = open(path, B_READ_WRITE);
	if (fd < 0) {
		result = fd;
		xprintf(("Open failed: %d/%d (%s)\n", result, errno, strerror(errno)));
		goto error0;
	}

	/* note that we're a clone accelerant */
	accelerantIsClone = 1;

	result = gxm_init_common(fd);
	if (result == B_OK) goto error0;
	
error1:
	close(fd);
error0:
	xprintf(("CLONE_ACCELERANT returns %.8lx\n", result));
	return result;
}

/* the standard entry point */
void *	get_accelerant_hook(uint32 feature, void *data) {
	switch (feature) {
		case B_INIT_ACCELERANT:
			return (void *)gxm_init;
		case B_ACCELERANT_CLONE_INFO_SIZE:
			return (void *)ACCELERANT_CLONE_INFO_SIZE;
		case B_GET_ACCELERANT_CLONE_INFO:
			return (void *)GET_ACCELERANT_CLONE_INFO;
		case B_CLONE_ACCELERANT:
			return (void *)CLONE_ACCELERANT;
		case B_UNINIT_ACCELERANT:
			return (void *)gxm_uninit;

		case B_ACCELERANT_MODE_COUNT:			/* required */
			return (void *)gxm_accelerant_mode_count;
		case B_GET_MODE_LIST:			/* required */
			return (void *)gxm_get_mode_list;
		case B_PROPOSE_DISPLAY_MODE:	/* optional */
			return (void *)0;
		case B_SET_DISPLAY_MODE:		/* required */
			return (void *)gxm_set_mode;
		case B_GET_DISPLAY_MODE:		/* required */
			return (void *)gxm_get_mode;
		case B_GET_FRAME_BUFFER_CONFIG:	/* required */
			return (void *)gxm_get_fb_config;
		case B_GET_PIXEL_CLOCK_LIMITS:
			return (void *)gxm_get_pixel_clock_limits;
		case B_MOVE_DISPLAY:				/* optional */
			return (void *)gxm_move_display_area;
		case B_SET_INDEXED_COLORS:		/* required if driver supports 8bit indexed modes */
			return (void *)gxm_set_indexed_colors;
#if 0
		case 1000000000:
			return (void*)gxm_set_screen_saver;
#endif

		case B_SET_CURSOR_SHAPE:
			return (void *)gxm_set_cursor_shape;
		case B_MOVE_CURSOR:
			return (void *)gxm_move_cursor;
		case B_SHOW_CURSOR:
			return (void *)gxm_show_cursor;
#if _SUPPORTS_FEATURE_CURSOR_COLORS
		case B_SET_CURSOR_COLORS:
			return (void *) setcursorcolors;
#endif

		/* synchronization */
		case B_ACCELERANT_ENGINE_COUNT:
			return (void *)gxm_get_engine_count;
		case B_ACQUIRE_ENGINE:
			return (void *)gxm_acquire_engine;
		case B_RELEASE_ENGINE:
			return (void *)gxm_release_engine;
		case B_WAIT_ENGINE_IDLE:
			return (void *)gxm_wait_engine_idle;
		case B_GET_SYNC_TOKEN:
			return (void *)gxm_get_sync_token;
		case B_SYNC_TO_TOKEN:
			return (void *)gxm_sync_to_token;

		case B_SCREEN_TO_SCREEN_BLIT:
			return scrn2scrn;
		case B_FILL_RECTANGLE:
			return (void *)rectfill16;
		case B_FILL_SPAN:
				return (void *)spanfill16;
	}
	return 0;
	if (data);
}


/*----------------------------------------------------------
 OS dependent functions
----------------------------------------------------------*/

void delay_milleseconds(uint32 milleseconds) {
	snooze(milleseconds*1000);
}

#if 0
/*----------------------------------------------------------
 various ramdac related functions
----------------------------------------------------------*/

void write_to_external_ramdac(uint32 value) {
	WRITE_REG32(GXregisters, MC_RAMDAC_ACC, value);
}

void set_external_ramdac_register_select(uint32 index)
{
	uint32			unlock, value;

	// UNLOCK THE DISPLAY CONTROLLER REGISTERS
	unlock = READ_REG32(GXregisters, DC_UNLOCK);
	WRITE_REG32(GXregisters, DC_UNLOCK, DC_UNLOCK_VALUE);

	// BEFORE CHANGING THE RS LINES, THE RAMDAC DATA WRITE BUFFER MUST BE
	// FLUSHED.  THIS IS DONE BY READING FROM THE MC RAMDAC INTERFACE.
	READ_REG32(GXregisters, MC_RAMDAC_ACC);

	// SET THE REGISTER SELECT LINES
	value = READ_REG32(GXregisters, DC_GENERAL_CFG);
    value = (value & ~DC_GCFG_DAC_RS_MASK) | (index << DC_GCFG_DAC_RS_POS);
	WRITE_REG32(GXregisters, DC_GENERAL_CFG, value);

	// RESTORE LOCK OF DC_UNLOCK
	WRITE_REG32(GXregisters, DC_UNLOCK, unlock);
}
#endif

/*----------------------------------------------------------
 various clock generator related functions
----------------------------------------------------------*/

// CX5520 ROUTINES
//
// PROGRAM_CX5520_FREQUENCY
// This routine programs the CX5520 dot clock frequency.

void program_cx5520_frequency(uint32 value) {
	// SET THE RESET BIT
	WRITE_REG32(CX5520registers, 0x0E14, value | 0x40000000);

	// PROGRAM THE CLOCK FREQUENCY
	WRITE_REG32(CX5520registers, 0x0E14, value);
}

/*----------------------------------------------------------
 mode setting.
----------------------------------------------------------*/

static int last_video_mode;

void set_video_mode(int mode) {
	uint32			unlock;
	uint32			gcfg, tcfg, ocfg, dcfg;
	DISPLAYMODE		*pMode;
	
	last_video_mode = mode;
	
	pMode = &DisplayParams[mode];
	
	// UNLOCK THE DISPLAY CONTROLLER REGISTERS
	unlock = READ_REG32(GXregisters, DC_UNLOCK);
	WRITE_REG32(GXregisters, DC_UNLOCK, DC_UNLOCK_VALUE);

	// READ THE CURRENT GX VALUES
    gcfg = READ_REG32(GXregisters, DC_GENERAL_CFG);
    tcfg = READ_REG32(GXregisters, DC_TIMING_CFG);
    ocfg = READ_REG32(GXregisters, DC_OUTPUT_CFG);

#if 0
	// READ THE CURRENT CX5520 VALUES AND BLANK THE CX5520 DISPLAY
    if (SysCX5520) {
    	dcfg = READ_REG32(CX5520registers, CX5520_DISPLAY_CONFIG);
		dcfg &= ~(unsigned long) CX5520_DCFG_DAC_BL_EN;
    	WRITE_REG32(CX5520registers, CX5520_DISPLAY_CONFIG, dcfg);
	}
#endif

    // BLANK THE DISPLAY
    tcfg &= ~(unsigned long)DC_TCFG_BLKE;
    WRITE_REG32(GXregisters, DC_TIMING_CFG, tcfg);

    // DISABLE THE TIMING GENERATOR
    tcfg &= ~(unsigned long)DC_TCFG_TGEN;
    WRITE_REG32(GXregisters, DC_TIMING_CFG, tcfg);

	// DELAY: WAIT FOR PENDING MEMORY REQUESTS
	// This delay is used to make sure that all pending requests to the
	// memory controller have completed before disabling the FIFO load.
	delay_milleseconds(1);

    // DISABLE DISPLAY FIFO LOAD AND DISABLE COMPRESSION
    gcfg &= ~(unsigned long)(DC_GCFG_DFLE | DC_GCFG_CMPE | DC_GCFG_DECE);
    WRITE_REG32(GXregisters, DC_GENERAL_CFG, gcfg);

	// CLEAR THE "DCLK_MUL" FIELD=20
	gcfg &= ~(unsigned long)(DC_GCFG_DDCK | DC_GCFG_DPCK); // FFB - not for GXLV | DC_GCFG_DFCK);
	gcfg &= ~(unsigned long)DC_GCFG_DCLK_MASK;
	WRITE_REG32(GXregisters, DC_GENERAL_CFG, gcfg);

#if 0
	// SET THE DOT CLOCK FREQUENCY
	//if (SysICS5342) program_ics5342_ramdac(pMode);
	if (SysCX5520) program_cx5520_frequency(pMode->cx5520_clock);
#endif

	// DELAY: WAIT FOR THE PLL TO SETTLE
	// This allows the dot clock frequency that was just set to settle.
	delay_milleseconds(1);

	// SET THE "DCLK_MUL" FIELD OF DC_GENERAL_CFG
	// The GX hardware divides the dot clock, so 2x really means that the
	// internal dot clock equals the external dot clock.
	gcfg |= pMode->gcfg & DC_GCFG_DCLK_MASK;
	WRITE_REG32(GXregisters, DC_GENERAL_CFG, gcfg);

	// DELAY: WAIT FOR THE ADL TO LOCK
	// This allows the clock generatation within GX to settle.  This is
	// needed since some of the register writes that follow require that
	// clock to be present.
	delay_milleseconds(1);

	// SET THE DISPLAY CONTROLLER PARAMETERS
	WRITE_REG32(GXregisters, DC_FB_ST_OFFSET, pMode->fb_offset);
	WRITE_REG32(GXregisters, DC_CB_ST_OFFSET, pMode->cb_offset);
	WRITE_REG32(GXregisters, DC_CURS_ST_OFFSET, pMode->curs_offset);
	WRITE_REG32(GXregisters, DC_LINE_DELTA, pMode->line_delta);
	WRITE_REG32(GXregisters, DC_BUF_SIZE, pMode->buffer_size);
	WRITE_REG32(GXregisters, DC_H_TIMING_1, pMode->htiming1);
	WRITE_REG32(GXregisters, DC_H_TIMING_2, pMode->htiming2);
	WRITE_REG32(GXregisters, DC_H_TIMING_3, pMode->htiming3);
	WRITE_REG32(GXregisters, DC_FP_H_TIMING, pMode->fp_htiming);
	WRITE_REG32(GXregisters, DC_V_TIMING_1, pMode->vtiming1);
	WRITE_REG32(GXregisters, DC_V_TIMING_2, pMode->vtiming2);
	WRITE_REG32(GXregisters, DC_V_TIMING_3, pMode->vtiming3);
	WRITE_REG32(GXregisters, DC_FP_V_TIMING, pMode->fp_vtiming);

	// COPY NEW GX CONFIGURATION VALUES
	gcfg = pMode->gcfg;
	tcfg = pMode->tcfg;
	ocfg = pMode->ocfg;

#if 0
	// MODIFY GX CONFIGURATION FOR CX5520, IF NEEDED
	if (SysCX5520) {
		// SET SYNC POLARITIES
		// For CX5520 systems, the GX is always programmed so that it
		// gemerates positive sync polarities (pulse from low to high).
		// The CX5520 then reverses the polarities, if needed.
		dcfg &= ~(unsigned long) CX5520_DCFG_CRT_HSYNC_POL;
		dcfg &= ~(unsigned long) CX5520_DCFG_CRT_VSYNC_POL;
		dcfg &= ~(unsigned long) CX5520_DCFG_FP_HSYNC_POL;
		dcfg &= ~(unsigned long) CX5520_DCFG_FP_VSYNC_POL;
		if (tcfg & DC_TCFG_CHSP) dcfg |= CX5520_DCFG_CRT_HSYNC_POL;
		if (tcfg & DC_TCFG_CVSP) dcfg |= CX5520_DCFG_CRT_VSYNC_POL;
		if (tcfg & DC_TCFG_FHSP) dcfg |= CX5520_DCFG_FP_HSYNC_POL;
		if (tcfg & DC_TCFG_FVSP) dcfg |= CX5520_DCFG_FP_VSYNC_POL;
		tcfg &= ~(unsigned long) (DC_TCFG_CHSP | DC_TCFG_CVSP);
		tcfg &= ~(unsigned long) (DC_TCFG_FHSP | DC_TCFG_FVSP);
	}
#endif

	// PROGRAM THE CONFIGURATION REGISTERS (MUST BE IN THIS SEQUENCE)
	WRITE_REG32(GXregisters, DC_OUTPUT_CFG, pMode->ocfg);
	WRITE_REG32(GXregisters, DC_TIMING_CFG, pMode->tcfg);
	WRITE_REG32(GXregisters, DC_GENERAL_CFG, pMode->gcfg);
#if 0
	if (SysCX5520) {
		// ENABLE DISPLAY
		dcfg |= CX5520_DCFG_DIS_EN;
		dcfg |= CX5520_DCFG_HSYNC_EN;
		dcfg |= CX5520_DCFG_VSYNC_EN;
		
		// ENABLE CRT OUTPUT
		dcfg |= CX5520_DCFG_DAC_BL_EN;
		dcfg |= CX5520_DCFG_DAC_PWDNX;

		// ENABLE FLAT PANEL OUTPUT
		if (SysFlatPanel) {
			dcfg |= CX5520_DCFG_FP_PWR_EN;
			dcfg |= CX5520_DCFG_FP_DATA_EN;
		}

		// PICK WHICH DATA STREAM GOES THROUGH THE PALETTE
		// For this file, this is always the video stream.
		dcfg |= CX5520_DCFG_GV_PAL_BYP;
		WRITE_REG32(CX5520registers, CX5520_DISPLAY_CONFIG, dcfg);
	}
#endif

	// RESTORE LOCK OF DC_UNLOCK
	WRITE_REG32(GXregisters, DC_UNLOCK, unlock);

	// init internal states
	ai->fbc.bytes_per_row = (pMode->line_delta & 0x7ff)*4;
	ai->fbc.frame_buffer = si->framebuffer;
	ai->fbc.frame_buffer_dma = si->framebuffer_dma;
	ai->cursor.data = (uint32 *)(si->framebuffer + pMode->curs_offset);
{
	xprintf(("gcfg: %.8lx\n", READ_REG32(GXregisters, DC_GENERAL_CFG)));
	xprintf(("tcfg: %.8lx\n", READ_REG32(GXregisters, DC_TIMING_CFG)));
	xprintf(("ocfg: %.8lx\n", READ_REG32(GXregisters, DC_OUTPUT_CFG)));
	xprintf(("dcfg: %.8lx\n", READ_REG32(CX5520registers, CX5520_DISPLAY_CONFIG)));
	xprintf(("fb_offset: %.8lx\n", READ_REG32(GXregisters, DC_FB_ST_OFFSET)));
	xprintf(("cb_offset: %.8lx\n", READ_REG32(GXregisters, DC_CB_ST_OFFSET)));
	xprintf(("curs_offset: %.8lx\n", READ_REG32(GXregisters, DC_CURS_ST_OFFSET)));
	xprintf(("line_delta: %.8lx\n", READ_REG32(GXregisters, DC_LINE_DELTA)));
	xprintf(("buffer_size: %.8lx\n", READ_REG32(GXregisters, DC_BUF_SIZE)));
	xprintf(("htiming1: %.8lx\n", READ_REG32(GXregisters, DC_H_TIMING_1)));
	xprintf(("htiming2: %.8lx\n", READ_REG32(GXregisters, DC_H_TIMING_2)));
	xprintf(("htiming3: %.8lx\n", READ_REG32(GXregisters, DC_H_TIMING_3)));
	xprintf(("fp_htiming: %.8lx\n", READ_REG32(GXregisters, DC_FP_H_TIMING)));
	xprintf(("vtiming1: %.8lx\n", READ_REG32(GXregisters, DC_V_TIMING_1)));
	xprintf(("vtiming2: %.8lx\n", READ_REG32(GXregisters, DC_V_TIMING_2)));
	xprintf(("vtiming3: %.8lx\n", READ_REG32(GXregisters, DC_V_TIMING_3)));
	xprintf(("fp_vtiming: %.8lx\n", READ_REG32(GXregisters, DC_FP_V_TIMING)));
	xprintf(("cx5520_clock: %.8lx\n", READ_REG32(CX5520registers, 0x0E14)));
}
	return;
}

static uint32 gxm_accelerant_mode_count(void) {
	xprintf(("gxm_accelerant_mode_count returns %d\n", mode_count));
	return mode_count;
}
static status_t gxm_get_mode_list(display_mode *modes) {
	xprintf(("gxm_get_mode_list(modes: 0x%08x)\n", modes));
	if (mode_list) {
		memcpy(modes, mode_list, sizeof(display_mode) * mode_count);
		return B_OK;
	}
	return B_ERROR;
}

static status_t gxm_get_mode(display_mode *dm) {
	xprintf(("gxm_get_mode(dm: 0x%08x)\n", dm));
	*dm = ai->current_mode;
	return B_OK;
}

static uint32 calc_pixel_clock_from_refresh(display_mode *dm, float refresh_rate) {
	double pc;
	pc = (double)dm->timing.h_total * (double)dm->timing.v_total * (double)refresh_rate;
	return (uint32)(pc / 1000);
}

static status_t gxm_get_pixel_clock_limits(display_mode *dm, uint32 *low, uint32 *high) {
	/* convert to pixel clocks given the current display_mode parameters */
	*low = *high = calc_pixel_clock_from_refresh(dm, 60.0);
	return B_OK;
}

static status_t gxm_get_fb_config(frame_buffer_config *a_frame_buffer) {
	*a_frame_buffer = ai->fbc;
	xprintf(("gxm_get_fb_config - fb: 0x%08x, rowbytes: 0x%08x (%d)\n", a_frame_buffer->frame_buffer, a_frame_buffer->bytes_per_row, a_frame_buffer->bytes_per_row));
	return B_OK;
}

status_t 
gxm_move_display_area(uint16 h_display_start, uint16 v_display_start)
{
	uint32 unlock;
	uint32 offset = (si->framebuffer - ai->fbc.frame_buffer);
	uint32 old_offset;
	uint32 gcfg;

	xprintf(("gxm_move_display_area(%d,%d)\n", h_display_start, v_display_start));
	/* assuming 2 byte per pixel */
	offset += (v_display_start * ai->fbc.bytes_per_row) + (h_display_start * 2);
	/* unlock access */
	unlock = READ_REG32(GXregisters, DC_UNLOCK);
	WRITE_REG32(GXregisters, DC_UNLOCK, DC_UNLOCK_VALUE);

#if 0
	/* read */
	gcfg = READ_REG32(GXregisters, DC_GENERAL_CFG);
	/* enabled compression */
	gcfg |= (DC_GCFG_CMPE | DC_GCFG_DECE);
	WRITE_REG32(GXregisters, DC_GENERAL_CFG, gcfg);
#endif
#if 0
	/* disable compression */
	gcfg &= ~(DC_GCFG_CMPE | DC_GCFG_DECE);
	WRITE_REG32(GXregisters, DC_GENERAL_CFG, gcfg);
	/* if the lower 4 bits of the offset changed, wait for a vblank */
	//if ((READ_REG32(GXregisters, DC_FB_ST_OFFSET) & 0x0f) != (offset & 0x0f))
	{
		uint32 count = 10000;
		while (count & (!(READ_REG32(GXregisters, DC_TIMING_CFG) & DC_TCFG_VNA)))
			count--;
	}
#endif
	/* wait for end of this frame */
	while (!(READ_REG32(GXregisters, DC_TIMING_CFG) & DC_TCFG_VNA))
		/* do nothing */;
	/* write offset */
	WRITE_REG32(GXregisters, DC_FB_ST_OFFSET, offset);
#if 0
	/* wait for start of next frame */
	while (READ_REG32(GXregisters, DC_TIMING_CFG) & DC_TCFG_VNA)
		/* do nothing */;
	/* wait for end of this frame */
	while (!(READ_REG32(GXregisters, DC_TIMING_CFG) & DC_TCFG_VNA))
		/* do nothing */;
#endif

#if 1
	/* mark the frame dirty */
	{
	volatile uint16 *p;
	uint16 v;
	int i;
	p = (uint16 *)(si->framebuffer + offset);
	for (i = 0; i < 600; i++)
	{
		v = *p;	*p = 0; *p = 1; *p = v;
		p += 1024;
	}
	}
#endif

	/* restore lock status */
	WRITE_REG32(GXregisters, DC_UNLOCK, unlock);
	/* update info */
	ai->current_mode.h_display_start = h_display_start;
	ai->current_mode.v_display_start = v_display_start;
	return B_OK;
}


static status_t gxm_set_cursor_shape(uint16 width, uint16 height, uint16 hot_x, uint16 hot_y, uint8 *andMask, uint8 *xorMask)
{
	int i;
	uint32 *data = ai->cursor.data;
	uint32 value;

	/* build the new cursor */
	for (i = 0; i < height; i++)
	{
		/* first 16 pixels */
		value = *andMask++;
		value <<= 8;
		value |= (width > 8) ? *andMask++ : 0xff;
		value <<= 8;
		value |= *xorMask++;
		value <<= 8;
		value |= (width > 8) ? *xorMask++ : 0x00;
		*data++ = value;
		/* second 16 pixels */
		value = (width > 16) ? *andMask++ : 0xff;
		value <<= 8;
		value |= (width > 24) ? *andMask++ : 0xff;
		value <<= 8;
		value |= (width > 16) ? *xorMask++ : 0x00;
		value <<= 8;
		value |= (width > 24) ? *xorMask++ : 0x00;
		yprintf(("second 16 pixels: %.8lx\n", value));
		*data++ = value;
	}
	for ( ;i < 32; i++)
	{
		*data++ = 0xffff0000;
		*data++ = 0xffff0000;
	}
	ai->cursor.hot_x = hot_x;
	ai->cursor.hot_y = hot_y;
	return B_OK;
}

static void gxm_move_cursor(uint16 x, uint16 y) {
	/* make the hot spot at x, y */
	int16 delta_x, delta_y;
	uint32 hardware_x = 0, hardware_y = 0;

	delta_x = x - ai->cursor.hot_x;
	delta_y = y - ai->cursor.hot_y;
	if (delta_x < 0) hardware_x = (int32)-delta_x << 11;
	else hardware_x = delta_x;
	if (delta_y < 0) hardware_y = (int32)-delta_y << 11;
	else hardware_y = delta_y;
	/* write */
	WRITE_REG32(GXregisters, DC_CURSOR_X, hardware_x);
	WRITE_REG32(GXregisters, DC_CURSOR_Y, hardware_y);
}

static void gxm_show_cursor(bool is_visible) {
	uint32 unlock;
	uint32 gcfg;

	/* unlock access */
	unlock = READ_REG32(GXregisters, DC_UNLOCK);
	WRITE_REG32(GXregisters, DC_UNLOCK, DC_UNLOCK_VALUE);
	/* read */
	gcfg = READ_REG32(GXregisters, DC_GENERAL_CFG);
	if (is_visible)
	{
		gcfg |= DC_GCFG_CURE;
	}
	else
	{
		gcfg &= ~DC_GCFG_CURE;
	}
	/* write */
	WRITE_REG32(GXregisters, DC_GENERAL_CFG, gcfg);
	/* restore lock status */
	WRITE_REG32(GXregisters, DC_UNLOCK, unlock);
	ai->cursor.is_visible = is_visible;
}

static status_t
setcursorcolors (uint32 blackcolor, uint32 whitecolor)
{
	uint32 value =
		((/*  red  */((blackcolor >> 16) & 0xff) >> 2) << 12) |
		((/* green */((blackcolor >> 8) & 0xff) >> 2) << 6) |
		((/* blue  */((blackcolor >> 0) & 0xff) >> 2) << 0);
	WRITE_REG32(GXregisters, DC_PAL_ADDRESS, PAL_CURSOR_COLOR_1);
	WRITE_REG32(GXregisters, DC_PAL_DATA, value);
	value =
		((/*  red  */((whitecolor >> 16) & 0xff) >> 2) << 12) |
		((/* green */((whitecolor >> 8) & 0xff) >> 2) << 6) |
		((/* blue  */((whitecolor >> 0) & 0xff) >> 2) << 0);
	WRITE_REG32(GXregisters, DC_PAL_ADDRESS, PAL_CURSOR_COLOR_0);
	WRITE_REG32(GXregisters, DC_PAL_DATA, value);
	xprintf(("setcursorcolors(%.8lx, %.8lx)\n", blackcolor, whitecolor));
	return (B_OK);
}

static void set_standard_timing(display_mode *dm, double refresh) {
	dm->virtual_width = dm->timing.h_display;
	dm->timing.h_total = dm->timing.h_display + (dm->timing.h_display >> 2);
	dm->timing.h_sync_start = (dm->timing.h_total + dm->timing.h_display) >> 1;
	dm->timing.h_sync_end = dm->timing.h_sync_start + 8;
	dm->virtual_height = dm->timing.v_display;
	dm->timing.v_total = dm->timing.v_display + (dm->timing.v_display >> 2);
	dm->timing.v_sync_start = (dm->timing.v_total + dm->timing.v_display) >> 1;
	dm->timing.v_sync_end = dm->timing.v_sync_start + 1;
	dm->timing.pixel_clock = (uint32)((double)dm->timing.h_total * dm->timing.v_total * refresh) / 1000;
	dm->h_display_start = dm->v_display_start = 0;
	dm->timing.flags = 0;
}


static void parms_to_mode(DISPLAYMODE *parms, display_mode *dm) {
	dm->timing.h_display = parms->xres;
	dm->timing.v_display = parms->yres;
	dm->space = B_RGB16;
	set_standard_timing(dm, (double)parms->hz);
	dm->flags = B_PARALLEL_ACCESS | B_HARDWARE_CURSOR;
}

static void make_mode_list(void) {
	int i;
	mode_count = sizeof(DisplayParams) / sizeof(DisplayParams[0]);
	mode_list = (display_mode *)calloc(mode_count, sizeof(display_mode));
	for (i = 0 ; i < mode_count ; i++) {
		parms_to_mode(&(DisplayParams[i]), (mode_list+i));
	}
}

static int32 display_mode_to_index(display_mode *dm) {
	/* match display_mode to one in our table, or return -1 if no match */
	int i;
	for (i = 0 ; i < mode_count ; i++)
		if ((dm->timing.h_display == mode_list[i].timing.h_display) &&
			(dm->timing.v_display == mode_list[i].timing.v_display) &&
			(dm->space == mode_list[i].space))
			return i;
	return -1;
}

static status_t gxm_set_mode(display_mode *dm) {
	int i;
	
#if DEBUG > 0
	xprintf(("gxm_set_mode() - wanted\n"));
	dump_mode(dm);
#endif

	i = display_mode_to_index(dm);
	if (i < 0) return B_ERROR;
	*dm = ai->current_mode = mode_list[i];
#if DEBUG > 0
	xprintf(("gxm_set_mode() - setting\n"));
	dump_mode(dm);
#endif
	set_video_mode(i);
	gxm_move_display_area(dm->h_display_start, dm->v_display_start);

	{
	uint32 value;
	if (ai->fbc.bytes_per_row > 1024) value = BC_FB_WIDTH_2048;
	else value = 0;
	if (dm->space != B_CMAP8) value |= BC_16BPP;
	WAIT_BUSY(GXregisters);
	WRITE_REG32(GXregisters, GP_BLIT_STATUS, value);
	}

#if DEBUG > 0
	{
	frame_buffer_config fbc;
	gxm_get_fb_config(&fbc);
	paint_for_blit(&(ai->current_mode), &fbc);
	}
#endif
	xprintf(("gxm_set_mode() completes\n"));
	return B_OK;
}

/*----------------------------------------------------------
 acceleration hooks
----------------------------------------------------------*/

// Fill a rect in 8 bits (see documentation for more informations)
long rect_16(long  x1,    // The rect to fill. Call will always respect
			long  y1,    // x1 <= x2, y1 <= y2 and the rect will be
			long  x2,    // completly in the current screen space (no
			long  y2,    // cliping needed).
			uint16 color) // Indexed color.
{
	int32		value;
	
	// WAIT TO PROGRAM THE GRAPHICS PIPELINE
	// The graphics pipeline can be rendering a primitive and have another
	// one "on deck".  Before programming a new graphics primitive, the driver
	// must wait until there is no longer a "pending" primitive.  This is done
	// be spinning until the BS_BLIT_PENDING bit is clear in the
	// BS_BLIT_STATUS register.
	WAIT_PENDING(GXregisters);

#if 0
	value = color;
	value =  value | (value<<8) | (value<<16) | (value<<24);
#endif
	// CLEAR THE FRAMEBUFFER TO BLACK
	// The GP_DST_YCOOR and GP_HEIGHT registers can be written in the upper
	// 16 bits of a 32-bit access...
	WRITE_REG32(GXregisters, GP_DST_XCOOR, x1 | (y1 << 16));
	WRITE_REG32(GXregisters, GP_WIDTH, (x2-x1+1) | ((y2-y1+1) << 16));
	WRITE_REG16(GXregisters, GP_PAT_COLOR_0, color);
	WRITE_REG16(GXregisters, GP_RASTER_MODE, 0xF0);
	WRITE_REG16(GXregisters, GP_BLIT_MODE, BM_WRITE_FB);
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
	int32		value;

// Check degenerated blit (source == destination)
	if ((x1 == x2) && (y1 == y2)) return B_NO_ERROR;
	
	// WAIT TO PROGRAM THE GRAPHICS PIPELINE
	// The graphics pipeline can be rendering a primitive and have another
	// one "on deck".  Before programming a new graphics primitive, the driver
	// must wait until there is no longer a "pending" primitive.  This is done
	// be spinning until the BS_BLIT_PENDING bit is clear in the
	// BS_BLIT_STATUS register.
	WAIT_PENDING(GXregisters);

	// CLEAR THE FRAMEBUFFER TO BLACK
	// The GP_DST_YCOOR and GP_HEIGHT registers can be written in the upper
	// 16 bits of a 32-bit access...
	value = BM_WRITE_FB|BM_READ_SRC_FB;
	if (y1 < y2) {
		value |= BM_REVERSE_Y;
		y1 += height;
		y2 += height;
	}
	WRITE_REG32(GXregisters, GP_SRC_XCOOR, x1 | (y1 << 16));
	WRITE_REG32(GXregisters, GP_DST_XCOOR, x2 | (y2 << 16));
	WRITE_REG32(GXregisters, GP_WIDTH, (width+1) | ((height+1) << 16));
	WRITE_REG32(GXregisters, GP_PAT_COLOR_0, 0xffffffff);
	WRITE_REG16(GXregisters, GP_RASTER_MODE, 0xc0);
	WRITE_REG16(GXregisters, GP_BLIT_MODE, value);
	return B_NO_ERROR;
}

static void scrn2scrn(engine_token *et, blit_params *list, uint32 count) {
	/* call the blit function for the loaded driver */
	while (count--) {
		blit(list->src_left, list->src_top, list->dest_left, list->dest_top, list->width, list->height);
		list++;
	}
	ai->fifo_count++;
	return;
	/* supress unused warning */
	et ;
}

static void rectfill16(engine_token *et, uint32 color, fill_rect_params *list, uint32 count) {
	while (count--) {
		rect_16(list->left, list->top, list->right, list->bottom, (uint16)color);
		list++;
	}
	ai->fifo_count++;
	return;
	/* supress unused warning */
	et ;
}

static void spanfill16(engine_token *et, uint32 color, uint16 *list, uint32 count) {
	while (count--) {
		uint32 y = (uint32)(*list++);
		uint32 left = (uint32)(*list++);
		uint32 right = (uint32)(*list++);
		rect_16(left, y, right, y, (uint16)color);
	}
	ai->fifo_count++;
	return;
	/* supress unused warning */
	et ;
}

static uint32 gxm_get_engine_count(void) {
	xprintf(("gxm_get_engine_count()\n"));
	return 1;
}

static status_t gxm_acquire_engine(uint32 caps, uint32 max_wait, sync_token *st, engine_token **et) {
	/* acquire the shared benaphore */
	ACQUIRE_BEN(ai->engine);

	yprintf(("gxm_acquire_engine(st:0x%08x, et:0x%08x)\n", st, et));
	/* sync if required */
	if (st) gxm_sync_to_token(st);

	/* return an engine_token */
	*et = &gxm_engine_token;
	return B_OK;
	caps ; max_wait ;
}

static status_t gxm_release_engine(engine_token *et, sync_token *st) {
	int32 old;

	yprintf(("gxm_release_engine(et:0x%08x, st:0x%08x)\n", et, st));
	if (!et) {
		yprintf(("ackthp! gxm_release_engine() called with null engine_token!\n"));
		return B_ERROR;
	}
	/* update the sync token, if any */
	if (st) {
		yprintf(("updating sync token - id: %d, fifo %Ld\n", et->engine_id, ai->fifo_count));
		st->engine_id = et->engine_id;
		st->counter = ai->fifo_count;
	}

	/* release the shared benaphore */
	RELEASE_BEN(ai->engine);
	yprintf(("gxm_release_engine() completes\n"));
	return B_OK;
}

static void gxm_wait_engine_idle(void) {
	yprintf(("gxm_wait_engine_idle()\n"));
	WAIT_BUSY(GXregisters);
}

static status_t gxm_get_sync_token(engine_token *et, sync_token *st) {
	yprintf(("gxm_get_sync_token(et:0x%08x, st:0x%08x)\n", et, st));
	st->engine_id = et->engine_id;
	st->counter = ai->fifo_count;
	yprintf(("gxm_get_sync_token() completes\n"));
	return B_OK;
}

static status_t gxm_sync_to_token(sync_token *st) {
	yprintf(("gxm_sync_to_token(st: 0x%08x)\n", st));
	gxm_wait_engine_idle();
	yprintf(("gxm_sync_to_token() completes\n"));
	return B_OK;
	/* prevent unused warning */
	st;
}

void gxm_set_indexed_colors(uint count, uint8 first, uint8 *color_data, uint32 flags) {
	uint32 red, green, blue;
	uint32 value;
	uint32 unlock;
	/* unlock access */
	unlock = READ_REG32(GXregisters, DC_UNLOCK);
	WRITE_REG32(GXregisters, DC_UNLOCK, DC_UNLOCK_VALUE);

	while (count--) {
		/* write internal dac */
		red = *color_data++;
		green = *color_data++;
		blue = *color_data++;
		value = ((red >> 2) << 12) | ((green >> 2) << 6) | (blue >> 2);
		WRITE_REG32(GXregisters, DC_PAL_ADDRESS, first);
		WRITE_REG32(GXregisters, DC_PAL_DATA, value);
		/* write external dac */
#if 0
		// set index
		set_external_ramdac_register_select(0);
		write_to_external_ramdac(first);
		// set color components
		set_external_ramdac_register_select(1);
		write_to_external_ramdac(red);
		write_to_external_ramdac(green);
		write_to_external_ramdac(blue);
#endif
		/* advance to next color */
		first++;
	}
	/* restore lock status */
	WRITE_REG32(GXregisters, DC_UNLOCK, unlock);

	return;
	/* prevent unused warning */
	flags;
}

static uint32 save_dcfg, save_tcfg, save_gcfg, save_ocfg;

#if 0
bool gxm_set_screen_saver(bool enable) {
	uint32			dcfg, unlock, tcfg, gcfg, ocfg;
	
	screen_saver_enable = enable;
    if (!SysCX5520)
    	return B_ERROR;
	if (enable == true) {
	 	// UNLOCK THE DISPLAY CONTROLLER REGISTERS
		unlock = READ_REG32(GXregisters, DC_UNLOCK);
		WRITE_REG32(GXregisters, DC_UNLOCK, DC_UNLOCK_VALUE);

 	    gcfg = READ_REG32(GXregisters, DC_GENERAL_CFG);
	    tcfg = READ_REG32(GXregisters, DC_TIMING_CFG);
	    ocfg = READ_REG32(GXregisters, DC_OUTPUT_CFG);
   		dcfg = READ_REG32(CX5520registers, CX5520_DISPLAY_CONFIG);

	    // BLANK THE DISPLAY
			dcfg &= ~(unsigned long) CX5520_DCFG_DAC_BL_EN;
	    tcfg &= ~(unsigned long)DC_TCFG_BLKE;
	    tcfg &= ~(unsigned long)DC_TCFG_TGEN;

		WRITE_REG32(GXregisters, DC_OUTPUT_CFG, ocfg);
		WRITE_REG32(GXregisters, DC_TIMING_CFG, tcfg);
		WRITE_REG32(GXregisters, DC_GENERAL_CFG, gcfg);
		WRITE_REG32(CX5520registers, CX5520_DISPLAY_CONFIG, dcfg);

		// RESTORE LOCK OF DC_UNLOCK
		WRITE_REG32(GXregisters, DC_UNLOCK, unlock);
	}
	else
 		set_video_mode(last_video_mode);
	return B_NO_ERROR;
}
#endif
