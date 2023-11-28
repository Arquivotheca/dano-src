#include <BeBuild.h>
#include <ByteOrder.h>
#include <Accelerant.h>
#include <image.h>
#include <FindDirectory.h>
#include <PCI.h>
#include <OS.h>
#include <bootscreen.h>

#include <vga.h>
#include <dirent.h>

#include <vga_map.h>

#include <GraphicsCard.h>

static graphics_card_hook	vga_dispatch[B_HOOK_COUNT];
static VGA_CTRL			vga_ctrl_jmp = 0L;
static int					vga_error;
static vga_settings 		thePrefs;
static graphics_card_spec	vgaDesc;
static long				scrnColors;
static image_id				image;
static uint32			spaces;
static display_mode		*mode_list;
static uint32			mode_count;
static int				file_handle;
static int				mapped_card;
static engine_token stub_engine_token = { 1, B_2D_ACCELERATION, NULL };
static area_id			shared_area_id = -1;

typedef struct stub_shared_info {
	display_mode	current_mode;
	bool			is_mac;
	bool			using_supervga;
	bool			using_vesa;
	bool			update_clone;
	vga_map_nth_pci	info;
	uint8			*frame_buffer_dma_base;
	uint8			*frame_buffer_base;
	sem_id			engine_sem;
	int32			engine_ben;
	uint64			fifo_count;
	direct_screen_info	current_device_info;
	uint8			clone_data[0];
} stub_shared_info;

stub_shared_info *ssi = NULL;

VGA_SET_CURSOR_SHAPE    set_cursor_shape_jmp;
VGA_MOVE_CURSOR         move_cursor_jmp;
VGA_SHOW_CURSOR         show_cursor_jmp;
VGA_LINE_8              line_8_jmp;
VGA_LINE_24             line_24_jmp;
VGA_RECT_8              rect_8_jmp;
VGA_RECT_24             rect_24_jmp;
VGA_BLIT                blit_jmp;
VGA_ARRAY_LINE_8        array_line_8_jmp;
VGA_ARRAY_LINE_24       array_line_24_jmp;
VGA_SYNCHRO             synchro_jmp;
VGA_INVERT_RECT_24      invert_rect_24_jmp;

/*	Hack (what else is new?) to use Trey's 16-bit driver, which extends our
	lovely graphics driver "architecture" */
typedef long (*VGA_LINE_15)(long,long,long,long,ushort,bool,short,short,short,short);
typedef long (*VGA_RECT_15)(long,long,long,long,ushort);
#define INDEX_LINE_15	12
#define INDEX_RECT_15	13
VGA_LINE_15             line_15_jmp;
VGA_RECT_15             rect_15_jmp;

#if defined(__cplusplus)
extern "C"
#endif
#define DEBUG 0
#if DEBUG > 0
#if 0
#include <stdio.h>
#define xprintf(a) printf a
#define yprintf(a)
#else
extern void _kdprintf_(const char *, ...);
#define xprintf(a) _kdprintf_ a
#define yprintf(a)
#endif
#else
#define xprintf(a)
#define yprintf(a)
#endif

static status_t init(int fd);
static void uninit(void);

static void dump_display_mode(display_mode *dm);

static void scrn2scrn(engine_token *et, blit_params *list, uint32 count);
static void stub_invert_rect(engine_token *et, fill_rect_params *list, uint32 count);
static void rectfill8(engine_token *et, uint32 color, fill_rect_params *list, uint32 count);
static void rectfill1516(engine_token *et, uint32 color, fill_rect_params *list, uint32 count);
static void rectfill32(engine_token *et, uint32 color, fill_rect_params *list, uint32 count);
static void spanfill8(engine_token *et, uint32 color, uint16 *list, uint32 count);
static void spanfill1516(engine_token *et, uint32 color, uint16 *list, uint32 count);
static void spanfill32(engine_token *et, uint32 color, uint16 *list, uint32 count);

static status_t get_hooks(void);
static uint32 get_screen_spaces(void);
void set_colors(uint count, uint8 first, uint8 *color_data, uint32 flags);
static status_t oldmode2parms(uint32 mode, uint32 *cs, uint16 *width, uint16 *height);
static void parms2oldmode(uint16 width, uint16 height, uint32 cs, uint32 *mode);
static image_id locate_addon(int fd);
static image_id load_graphic_addon (int fd, char *path, bool generic, ulong fbAddr);
static uint32 stub_accelerant_mode_count(void);
static status_t stub_get_mode_list(display_mode *modes);
status_t spaces_to_mode_list(uint32 spaces, uint32 flags);
static void set_standard_timing(display_mode *dm, double refresh);
static status_t stub_set_mode(display_mode *dm);
static status_t stub_get_mode(display_mode *dm);
static status_t stub_get_fb_config(frame_buffer_config *a_frame_buffer);
static status_t stub_set_cursor_shape(uint16 width, uint16 height, uint16 hot_x, uint16 hot_y, uint8 *andMask, uint8 *xorMask);
static void stub_move_cursor(uint16 x, uint16 y);
static void stub_show_cursor(bool is_visible);
static uint32 stub_get_engine_count(void);
static status_t stub_acquire_engine(uint32 caps, uint32 max_wait, sync_token *st, engine_token **et);
static status_t stub_release_engine(engine_token *et, sync_token *st);
static void stub_wait_engine_idle(void);
static status_t stub_get_sync_token(engine_token *et, sync_token *st);
static status_t stub_sync_to_token(sync_token *st);
static status_t stub_get_pixel_clock_limits(display_mode *dm, uint32 *low, uint32 *high);
static uint32 calc_pixel_clock_from_refresh(display_mode *dm, float refresh_rate);
static ssize_t stub_accelerant_clone_info_size(void);
static void stub_get_accelerant_clone_info(void *data);
static status_t stub_clone_accelerant(void *data);
static image_id load_image_and_find_symbol(char *path);
static status_t stub_propose_display_mode(display_mode *target, display_mode *low, display_mode *high);
static status_t stub_move_display_area(uint16 h_display_start, uint16 v_display_start);

/* the standard entry point */
void *	get_accelerant_hook(uint32 feature, void *data) {
	switch (feature) {
		case B_INIT_ACCELERANT:
			return (void *)init;
		case B_ACCELERANT_CLONE_INFO_SIZE:
			return (void *)stub_accelerant_clone_info_size;
		case B_GET_ACCELERANT_CLONE_INFO:
			return (void *)stub_get_accelerant_clone_info;
		case B_CLONE_ACCELERANT:
			return (void *)stub_clone_accelerant;
		case B_UNINIT_ACCELERANT:
			return (void *)uninit;

		case B_ACCELERANT_MODE_COUNT:			/* required */
			return (void *)stub_accelerant_mode_count;
		case B_GET_MODE_LIST:			/* required */
			return (void *)stub_get_mode_list;
		case B_PROPOSE_DISPLAY_MODE:	/* optional */
			return (void *)stub_propose_display_mode;
		case B_SET_DISPLAY_MODE:		/* required */
			return (void *)stub_set_mode;
		case B_GET_DISPLAY_MODE:		/* required */
			return (void *)stub_get_mode;
		case B_GET_FRAME_BUFFER_CONFIG:	/* required */
			return (void *)stub_get_fb_config;
		case B_GET_PIXEL_CLOCK_LIMITS:
			return (void *)stub_get_pixel_clock_limits;
		case B_MOVE_DISPLAY:				/* optional */
			return (void *)stub_move_display_area;
		case B_SET_INDEXED_COLORS:		/* required if driver supports 8bit indexed modes */
			return (void *)set_colors;

		case B_SET_CURSOR_SHAPE:
			return (void *)(set_cursor_shape_jmp ? stub_set_cursor_shape : 0);
		case B_MOVE_CURSOR:
			return (void *)(move_cursor_jmp ? stub_move_cursor : 0);
		case B_SHOW_CURSOR:
			return (void *)(show_cursor_jmp ? stub_show_cursor : 0);

		/* synchronization */
		case B_ACCELERANT_ENGINE_COUNT:
			return (void *)stub_get_engine_count;
		case B_ACQUIRE_ENGINE:
			return (void *)stub_acquire_engine;
		case B_RELEASE_ENGINE:
			return (void *)stub_release_engine;
		case B_WAIT_ENGINE_IDLE:
			return (void *)stub_wait_engine_idle;
		case B_GET_SYNC_TOKEN:
			return (void *)stub_get_sync_token;
		case B_SYNC_TO_TOKEN:
			return (void *)stub_sync_to_token;


		case B_SCREEN_TO_SCREEN_BLIT:
			return (blit_jmp ? (void *)scrn2scrn : 0);

		case B_FILL_RECTANGLE: {
			display_mode *dm = (display_mode *)data;
			if (dm->space == B_CMAP8)
				return (void *)(rect_8_jmp ? rectfill8 : 0);
			if ((dm->space & (uint32)~0x00001000) == B_RGB15)
				return (void *)(rect_15_jmp ? rectfill1516 : 0);
			if ((dm->space & (uint32)~0x00001000) == B_RGB16)
				return (void *)(rect_15_jmp ? rectfill1516 : 0);
			if ((dm->space & (uint32)~0x00001000) == B_RGB32)
				return (void *)(rect_24_jmp ? rectfill32 : 0);
			return (void *)0;
		}
		case B_FILL_SPAN: {
			display_mode *dm = (display_mode *)data;
			if (dm->space == B_CMAP8)
				return (void *)(rect_8_jmp ? spanfill8 : 0);
			if ((dm->space & (uint32)~0x00001000) == B_RGB15)
				return (void *)(rect_15_jmp ? spanfill1516 : 0);
			if ((dm->space & (uint32)~0x00001000) == B_RGB16)
				return (void *)(rect_15_jmp ? spanfill1516 : 0);
			if ((dm->space & (uint32)~0x00001000) == B_RGB32)
				return (void *)(rect_24_jmp ? spanfill32 : 0);
			return (void *)0;
		}
		case B_INVERT_RECTANGLE:
			return (invert_rect_24_jmp ? (void *)stub_invert_rect : 0);		
	}
	return 0;
}

#if DEBUG > 0
static void dump_display_mode(display_mode *dm) {
	xprintf(("timing: %u  %d %d %d %d  ",
		dm->timing.pixel_clock,
		dm->timing.h_display,
		dm->timing.h_sync_start,
		dm->timing.h_sync_end,
		dm->timing.h_total));
	xprintf(("%d %d %d %d  0x%08x\n",
		dm->timing.v_display,
		dm->timing.v_sync_start,
		dm->timing.v_sync_end,
		dm->timing.v_total,
		dm->timing.flags));
	xprintf(("colorspace: 0x%08x\n", dm->space));
	xprintf(("virtual: %d %d\n", dm->virtual_width, dm->virtual_height));
	xprintf(("offset: %d %d\n", dm->h_display_start, dm->v_display_start));
	xprintf(("mode flags: 0x%08x\n", dm->flags));
}
#endif

static ssize_t stub_accelerant_clone_info_size(void) {
	return sizeof(area_id);
}

static void stub_get_accelerant_clone_info(void *data) {
	*(area_id *)data = shared_area_id;
}

static void update_clone_data(void) {
	if (file_handle == -1) {
		/* we're a clone */
		if (ssi->update_clone == TRUE) {
			(vga_ctrl_jmp)(B_SET_CLONED_GRAPHICS_CARD, (void *)ssi->clone_data);
			ssi->update_clone = FALSE;
			get_hooks();
		}
	} else {
		/* we're the original */
		(vga_ctrl_jmp)(B_GET_INFO_FOR_CLONE, (void *)ssi->clone_data);
		ssi->update_clone = TRUE;
	}
}

static status_t stub_clone_accelerant(void *data) {
	status_t result;
	area_id area_to_clone = *(area_id *)data;

	/* mark ourselves as the clone */
	file_handle = -1;

	/* get our shared data */
	result = (status_t)(shared_area_id = clone_area("clone of stub shared info", (void **)&ssi, B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, area_to_clone));
	if (result < 0) goto err0;

	/* load the accelerant into our address space */
	result = (status_t)(image = load_image_and_find_symbol(ssi->current_device_info.path));
	if (result < 0) goto err1;

	/* FIXME: get the display mode list */
	mode_list = NULL;
	mode_count = 0;
	/* clone the underlying driver */
	update_clone_data();

	result = B_OK;
	goto err0;

err2:
	unload_add_on(image);
	image = -1;
err1:
	delete_area(shared_area_id);
	shared_area_id = -1;
err0:
	return result;
}

static uint32 stub_accelerant_mode_count(void) {
	xprintf(("stub_accelerant_mode_count returns %d\n", mode_count));
	return mode_count;
}
static status_t stub_get_mode_list(display_mode *modes) {
	xprintf(("stub_get_mode_list(modes: 0x%08x)\n", modes));
	if (mode_list) {
		memcpy(modes, mode_list, sizeof(display_mode) * mode_count);
		return B_OK;
	}
	return B_ERROR;
}

static status_t stub_get_mode(display_mode *dm) {
	xprintf(("stub_get_mode(dm: 0x%08x)\n", dm));
	*dm = ssi->current_mode;
	return B_OK;
}

static uint32 calc_pixel_clock_from_refresh(display_mode *dm, float refresh_rate) {
	double pc;
	pc = (double)dm->timing.h_total * (double)dm->timing.v_total * (double)refresh_rate;
	return (uint32)(pc / 1000.0);
}

static status_t stub_get_pixel_clock_limits(display_mode *dm, uint32 *low, uint32 *high) {
	refresh_rate_info rri;
	
	/* in theory, we should handle any display_mode.  In practice, we only need to
		handle the current display mode (it's the only thing the old drivers can handle anyways */
	/* get the refresh rate ranges from the old driver */
	vga_error = (vga_ctrl_jmp)(B_GET_REFRESH_RATES, (void *)&rri);
	/* convert to pixel clocks given the current display_mode parameters */
	*low = calc_pixel_clock_from_refresh(dm, rri.min);
	*high = calc_pixel_clock_from_refresh(dm, rri.max);
	return vga_error;
}

static status_t stub_get_fb_config(frame_buffer_config *a_frame_buffer) {
	graphics_card_info gci;

	xprintf(("stub_get_fb_config(fbi: 0x%08x)\n", a_frame_buffer));	
	vga_error = (vga_ctrl_jmp)(B_GET_GRAPHICS_CARD_INFO, (void *)&gci);
	xprintf(("stub_get_fb_config - vga_error: 0x%08x\n", vga_error));
	if (vga_error != B_OK) return B_ERROR;
	a_frame_buffer->frame_buffer = gci.frame_buffer;
	if (ssi->using_supervga) {
		ssi->frame_buffer_dma_base = NULL;
		ssi->frame_buffer_base = gci.frame_buffer;
	} else if (!ssi->frame_buffer_dma_base) {
		int i;
		for (i = 0; i < 6; i++) {
			if (((uint32)gci.frame_buffer >= ssi->info.pcii.u.h0.base_registers[i]) &&
				((uint32)gci.frame_buffer < (ssi->info.pcii.u.h0.base_registers[i] + ssi->info.pcii.u.h0.base_register_sizes[i]))) {
				ssi->frame_buffer_dma_base = (uint8*)ssi->info.pcii.u.h0.base_registers_pci[i];
				ssi->frame_buffer_base = (uint8*)ssi->info.pcii.u.h0.base_registers[i];
				goto calc_fb_dma;
			}
		}
		xprintf(("Aieeee!  Can't find frame buffer dma base!\n"));
	}
calc_fb_dma:
	a_frame_buffer->frame_buffer_dma = ssi->frame_buffer_dma_base + ((uint8*)gci.frame_buffer - ssi->frame_buffer_base);
	a_frame_buffer->bytes_per_row = gci.bytes_per_row;
	xprintf(("stub_get_fb_config - fb: 0x%08x, rowbytes: 0x%08x (%d)\n", a_frame_buffer->frame_buffer, a_frame_buffer->bytes_per_row, a_frame_buffer->bytes_per_row));
	return B_OK;
}

static status_t stub_set_cursor_shape(uint16 width, uint16 height, uint16 hot_x, uint16 hot_y, uint8 *andMask, uint8 *xorMask) {
	status_t result = set_cursor_shape_jmp(xorMask, andMask, width, height, hot_x, hot_y);
	update_clone_data();
	return result;
}

static void stub_move_cursor(uint16 x, uint16 y) {
	move_cursor_jmp(x, y);
	update_clone_data();
}

static void stub_show_cursor(bool is_visible) {
	show_cursor_jmp(is_visible);
	update_clone_data();
}


static status_t get_hooks(void) {
	vga_error = (vga_ctrl_jmp)(B_GET_GRAPHICS_CARD_HOOKS, (void *)vga_dispatch);
	if (vga_error != B_NO_ERROR)
		return vga_error;
	
	set_cursor_shape_jmp = (VGA_SET_CURSOR_SHAPE)vga_dispatch[INDEX_SET_CURSOR_SHAPE];
	move_cursor_jmp = (VGA_MOVE_CURSOR)vga_dispatch[INDEX_MOVE_CURSOR];
	show_cursor_jmp = (VGA_SHOW_CURSOR)vga_dispatch[INDEX_SHOW_CURSOR];
	line_8_jmp = (VGA_LINE_8)vga_dispatch[INDEX_LINE_8];
	line_24_jmp = (VGA_LINE_24)vga_dispatch[INDEX_LINE_24];
	rect_8_jmp = (VGA_RECT_8)vga_dispatch[INDEX_RECT_8];
	rect_24_jmp = (VGA_RECT_24)vga_dispatch[INDEX_RECT_24];
	blit_jmp = (VGA_BLIT)vga_dispatch[INDEX_BLIT];
	array_line_8_jmp = (VGA_ARRAY_LINE_8)vga_dispatch[INDEX_ARRAY_LINE_8];
	array_line_24_jmp = (VGA_ARRAY_LINE_24)vga_dispatch[INDEX_ARRAY_LINE_24];
	synchro_jmp = (VGA_SYNCHRO)vga_dispatch[INDEX_SYNCHRO];
	invert_rect_24_jmp = (VGA_INVERT_RECT_24)vga_dispatch[INDEX_INVERT_RECT];
	line_15_jmp = (VGA_LINE_15)vga_dispatch[INDEX_LINE_16];
	rect_15_jmp = (VGA_RECT_15)vga_dispatch[INDEX_RECT_16];
	
	return B_OK;
}

static status_t init(int fd) {
	status_t result;
	graphics_card_info gci;
	size_t shared_size = B_PAGE_SIZE + (((sizeof(stub_shared_info)) + (B_PAGE_SIZE-1)) & ~(B_PAGE_SIZE-1));

	/* create an area for the shared info */
	shared_area_id = create_area("stub accelerant shared info", (void **)&ssi, B_ANY_ADDRESS, shared_size, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (shared_area_id < 0) {
		result = shared_area_id;
		goto err0;
	}

	/* simple init */
	ssi->update_clone = FALSE;

	/* find the add-on */
	result = image = locate_addon(fd);
	if (image < 0) goto err1;

	/* convert supported resolutions to display_mode list */
	result = (vga_ctrl_jmp)(B_GET_SCREEN_SPACES, (void *)&spaces);
	if (result < 0) goto err1;
	if (!spaces) {
		xprintf(("stub_init() spaces == 0\n"));
		result = B_ERROR;
		goto err2;
	}
	result = get_hooks();
	if (result < 0) goto err1;
	result = (vga_ctrl_jmp)(B_GET_GRAPHICS_CARD_INFO, (void *)&gci);
	if (result < 0) goto err2;
	result = spaces_to_mode_list(spaces, gci.flags);
	if (result < 0) goto err2;
	ssi->engine_sem = create_sem(1, "stub.accelerant.engine");
	if (ssi->engine_sem < 0) {
		result = ssi->engine_sem;
		goto err2;
	}
	ssi->engine_ben = 0;
	ssi->fifo_count = 0;
	/* save for uninit */
	file_handle = fd;
	update_clone_data();
	goto err0;

err2:
	unload_add_on(image);
err1:
	delete_area(shared_area_id);
	shared_area_id = -1;
err0:
	xprintf(("stub_init() completes with result %d\n", result));
	return result;
}

void uninit(void) {
	xprintf(("stub_uninit()\n"));
	if (image >= 0) {
		xprintf(("vga_ctrl_jmp is 0x%08x\n", vga_ctrl_jmp));
		/* uninit-R3 add-on */
		if (file_handle == -1)
			(vga_ctrl_jmp)(B_CLOSE_CLONED_GRAPHICS_CARD, NULL);
		else
			(vga_ctrl_jmp)(B_CLOSE_GRAPHICS_CARD, NULL);
		/* unload the image */
		unload_add_on(image);
		xprintf(("Image unloaded.\n"));
	}
	/* if we're the add-on that owns the file handle */
	if (file_handle != -1) {
		/* un-map whatever card we have mapped */
		ioctl(file_handle, VGA_MAP_UNMAP_NTH, &mapped_card, sizeof(mapped_card));
		/* clean up benaphore */
		delete_sem(ssi->engine_sem);
		ssi->engine_sem = -1;
		file_handle = -1;
	}
	/* FIXME */
	if (mode_list) free(mode_list);
	mode_list = 0;
	mode_count = 0;
	if (file_handle == -1) {
		delete_area(shared_area_id);
		shared_area_id = -1;
	}
}

static uint32 bits_for_color_space(uint32 cs) {
	uint32 bits = 0;
	switch(cs & 0x0fff) {
		case B_CMAP8: bits = 8; break;
		case B_RGB15: bits = 15; break;
		case B_RGB16: bits = 16; break;
		case B_RGB32: bits = 32; break;
	}
	return bits;
}

static status_t stub_propose_display_mode(display_mode *target, display_mode *low, display_mode *high) {
	/*
		Only works if target is "close" to the current display mode.
		This is a limitation of the R3 and earlier driver API.
		Only intended to be used for GameKit/BWindowScreen support.
	*/
	display_mode	*cm = &(ssi->current_mode);
	status_t		result = B_ERROR;
	frame_buffer_info	fbi;

	/* is target "close" to the current mode? */
	if (
		(cm->timing.h_display != target->timing.h_display) ||
		(cm->timing.v_display != target->timing.v_display) ||
//		(cm->timing.pixel_clock != target->timing.pixel_clock) ||
		(cm->space != target->space)
	) goto err0;

	fbi.bits_per_pixel = bits_for_color_space(target->space);
	if (!fbi.bits_per_pixel) goto err0;

	fbi.width = target->virtual_width;
	fbi.display_width = target->timing.h_display;
	fbi.display_height = target->timing.v_display;
	result = (vga_ctrl_jmp)(B_PROPOSE_FRAME_BUFFER, (void *)&fbi);
	if (result == B_OK) {
		if (fbi.height >= target->virtual_height) {
			goto err0;
		} else if (fbi.height >= low->virtual_height) {
			target->virtual_height = fbi.height;
		} else result = B_ERROR;
	}
err0:
	return result;
}

static status_t stub_move_display_area(uint16 h_display_start, uint16 v_display_start) {
	frame_buffer_info fbi;
	status_t result;

	fbi.display_x = h_display_start;
	fbi.display_y = v_display_start;
	result = (vga_ctrl_jmp)(B_MOVE_DISPLAY_AREA, (void *)&fbi);
	update_clone_data();
	return result;
}

static status_t stub_set_mode(display_mode *dm) {
	display_mode std;
	double target_rate;
	graphics_card_config gcc;
	graphics_card_info gci;
	refresh_rate_info rri;


	xprintf(("stub_set_mode(dm: 0x%08x)\n", dm));
#if DEBUG > 0
	dump_display_mode(dm);
#endif
	if(ssi->using_vesa) {
		if(ssi->current_mode.space != dm->space ||
		   ssi->current_mode.timing.h_display != dm->timing.h_display ||
		   ssi->current_mode.timing.v_display != dm->timing.v_display) {
			xprintf(("vesa only support one mode\n"));
			return B_ERROR;
		}
		return B_OK;
	}
	std.timing.h_display = dm->timing.h_display;
	std.timing.v_display = dm->timing.v_display;
	std.space = dm->space;
	xprintf(("color_space is 0x%08x\n", std.space));
	xprintf(("h/v totals: %d %d\n", dm->timing.h_total, dm->timing.v_total));
	xprintf(("pixel_clock: %d\n", dm->timing.pixel_clock));
	target_rate = ((double)dm->timing.pixel_clock * 1000) / ((double)dm->timing.h_total * dm->timing.v_total);
	xprintf(("target_rate is %lf\n", target_rate));
	if (target_rate > 125.0) {
		target_rate = 70.0;
		xprintf(("target_rate re-set to reasonable value\n"));
	}
	set_standard_timing(&std, target_rate);
	gcc.h_size = 50 + ((std.timing.h_total - dm->timing.h_total) >> 3);
	gcc.h_position = 50 + ((std.timing.h_sync_start - dm->timing.h_sync_start) >> 3);
	gcc.v_size = 50 + (std.timing.v_total - dm->timing.v_total);
	gcc.v_position = 50 + (std.timing.v_sync_start - dm->timing.v_sync_start);
	gcc.refresh_rate = (float)target_rate;
	parms2oldmode(dm->timing.h_display, dm->timing.v_display, dm->space, &(gcc.space));
	xprintf(("Converted %dx%dx0x%08x to 0x%08x\n", dm->timing.h_display, dm->timing.v_display, dm->space, gcc.space));
	if ((gcc.space & spaces) == 0) {
		xprintf(("Space 0x%08x not supported by R3 driver\n", gcc.space));
		return B_ERROR;
	}
	vga_error = (vga_ctrl_jmp)(B_CONFIG_GRAPHICS_CARD, (void *)&gcc);
	if (vga_error != B_OK) {
		xprintf(("stub_set_mode() failed B_CONFIG_GRAPHICS_CARD: %d (0x%08x)\n", vga_error, vga_error));
		return vga_error;
	}
	/* adjust for 15/16 wierdness */
	vga_error = (vga_ctrl_jmp)(B_GET_GRAPHICS_CARD_INFO, (void *)&gci);
	if (vga_error != B_OK) {
		xprintf(("stub_set_mode() failed B_GET_GRAPHICS_CARD_INFO: %d (0x%08x)\n", vga_error, vga_error));
		return vga_error;
	}

	dm->flags = 0;
	if (gci.flags & B_FRAME_BUFFER_CONTROL) dm->flags |= B_SCROLL;
	if (gci.flags & B_PARALLEL_BUFFER_ACCESS) dm->flags |= B_PARALLEL_ACCESS;
	if (gci.flags & B_LAME_ASS_CARD) dm->flags |= B_IO_FB_NA;
	if (show_cursor_jmp) dm->flags |= B_HARDWARE_CURSOR;

	ssi->current_mode = *dm;

	/* game kit support */
	if (
		(dm->virtual_width != dm->timing.h_display) ||
		(dm->virtual_height != dm->timing.v_display)
	) {
		/* build up a frame_buffer_info that describes the current mode */
		frame_buffer_info fbi;
		fbi.width = dm->virtual_width;
		fbi.height = dm->virtual_height;
		fbi.bits_per_pixel = gci.bits_per_pixel;
		fbi.display_width = dm->timing.h_display;
		fbi.display_height = dm->timing.v_display;
		fbi.display_x = dm->h_display_start;
		fbi.display_y = dm->v_display_start;
		vga_error = (vga_ctrl_jmp)(B_PROPOSE_FRAME_BUFFER, (void *)&fbi);
		if (vga_error != B_OK) return vga_error;
		if (fbi.height < dm->virtual_height) return B_ERROR;
		fbi.height = dm->virtual_height;
		vga_error = (vga_ctrl_jmp)(B_SET_FRAME_BUFFER, (void *)&fbi);
		if (vga_error != B_OK) return vga_error;
	}
	if (gci.bits_per_pixel == 15) ssi->current_mode.space = B_RGB15_LITTLE;
	else if (gci.bits_per_pixel == 16) ssi->current_mode.space = B_RGB16_LITTLE;
	if (ssi->is_mac && (ssi->current_mode.space != B_CMAP8)) ssi->current_mode.space |= (uint32)0x1000;
	/* set the refresh rate accordingly */
	vga_error = (vga_ctrl_jmp)(B_GET_REFRESH_RATES, (void *)&rri);
	if (vga_error != B_OK) {
		xprintf(("stub_set_mode() failed B_GET_REFRESH_RATES: %d (0x%08x)\n", vga_error, vga_error));
		return vga_error;
	}
	/* convert to pixel clocks given the current display_mode parameters */
	ssi->current_mode.timing.pixel_clock = calc_pixel_clock_from_refresh(dm, rri.current);
	update_clone_data();
	xprintf(("stub_set_mode() completes\n"));
	return B_OK;
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
	dm->timing.pixel_clock = (uint32)(((double)dm->timing.h_total * dm->timing.v_total * refresh) / 1000.0);
	dm->h_display_start = dm->v_display_start = 0;
	dm->flags = 0;
}

status_t spaces_to_mode_list(uint32 spaces, uint32 flags) {
	display_mode *dm;
	uint32 space_mask = 1;
	

	if(ssi->using_supervga &&
	   spaces != (B_8_BIT_640x480 | B_8_BIT_800x600 | B_8_BIT_1024x768 |
	              B_8_BIT_1152x900 | B_8_BIT_1280x1024 | B_8_BIT_1600x1200)) {
		graphics_card_info gci;
		status_t err;

		xprintf(("spaces_to_mode_list() using_vesa\n"));
		ssi->using_vesa = true;

		err = (vga_ctrl_jmp)(B_GET_GRAPHICS_CARD_INFO, (void *)&gci);
		if(err != B_OK)
			return err;
		dm = mode_list = (display_mode *)calloc(1, sizeof(display_mode));
		if (!dm)
			return B_NO_MEMORY; /* no memory? */
			
		switch(gci.bits_per_pixel) {
			case 8: 	dm->space = B_CMAP8; break;
			case 15:	dm->space = B_RGB15; break;
			case 16:	dm->space = B_RGB16; break;
			case 32:	dm->space = B_RGB32; break;
			default:
				xprintf(("spaces_to_mode_list() bits_per_pixel %d\n", gci.bits_per_pixel));
				return B_ERROR;
		}
		dm->timing.h_display = gci.width;
		dm->timing.v_display = gci.height;
		
		set_standard_timing(dm, 60.0);
		if (flags & B_FRAME_BUFFER_CONTROL) dm->flags |= B_SCROLL;
		if (flags & B_PARALLEL_BUFFER_ACCESS) dm->flags |= B_PARALLEL_ACCESS;
		if (flags & B_LAME_ASS_CARD) dm->flags |= B_IO_FB_NA;
		if (show_cursor_jmp) dm->flags |= B_HARDWARE_CURSOR;
		/* bump mode count */
		mode_count++;
		ssi->current_mode = *dm;

		return B_OK;
	}

	/* alloc one for each possible mode */
	dm = mode_list = (display_mode *)calloc(32, sizeof(display_mode));
	if (!dm) return B_ERROR; /* no memory? */
	xprintf(("spaces: 0x%08x, flags: 0x%04x\n", spaces, flags));
	while (space_mask) {
		/* does driver support this mode */
		if (spaces & space_mask) {
			xprintf(("space_mask 0x%08x supported\n", space_mask));
			/* if a valid mode */
			if (oldmode2parms(space_mask, &(dm->space), &(dm->timing.h_display), &(dm->timing.v_display)) == B_OK) {
				xprintf(("successfully converted to %dx%d 0x%08x\n", dm->timing.h_display, dm->timing.v_display, dm->space));
				/* make a 60Hz mode with 50/50 size/centering */
				set_standard_timing(dm, 60.0);
				xprintf((" with a pixel_clock of %d\n", dm->timing.pixel_clock));
				if (flags & B_FRAME_BUFFER_CONTROL) dm->flags |= B_SCROLL;
				if (flags & B_PARALLEL_BUFFER_ACCESS) dm->flags |= B_PARALLEL_ACCESS;
				if (flags & B_LAME_ASS_CARD) dm->flags |= B_IO_FB_NA;
				if (show_cursor_jmp) dm->flags |= B_HARDWARE_CURSOR;
				/* bump mode count */
				mode_count++;
				/* bump mode pointer */
				dm++;
			}
		}
		/* next mode */
		space_mask <<= 1;
	}
	/* no matching spaces? */
	if (dm == mode_list) {
		xprintf(("YIKES!: no matching spaces!!!\n"));
		free(mode_list);
		dm = mode_list = NULL;
	}
	return (dm == NULL) ? B_ERROR : B_OK;
}

static char stub_name[] = "supervga";
/* initialize the accelerant */
static image_id locate_addon(int fd) {
	image_id		image;
	DIR				*d;
	struct dirent	*e;
	int				len, i;
	status_t		result = B_ERROR;
	bool
		use_stub,
		stub_exists;
	system_info		si;
	uint32			fbAddr = 0;
	char path[PATH_MAX];
	directory_which dirs[] = {
		B_COMMON_ADDONS_DIRECTORY,
		B_BEOS_ADDONS_DIRECTORY
	};
	long _kget_default_screen_info_( screen *sinfo );
	screen sinfo;

	/* check use stub? */
	use_stub = false;
	ssi->using_supervga = false;
	ssi->using_vesa = false;
	/* determine the platform */
	get_system_info(&si);
	ssi->is_mac = (si.platform_type == B_MAC_PLATFORM);

	/* Find the base address so if we're booting a Mac
	   we can boot onto the main monitor */
	/* On PCs, check if we should boot into safe mode */
	if( _kget_default_screen_info_( &sinfo ) == B_OK) {
		if (ssi->is_mac) fbAddr = (ulong)sinfo.base;
		use_stub = sinfo.use_stub;
		if (use_stub && !ssi->is_mac) ssi->using_supervga = true;
	}
	/* the stub name */
	if (ssi->is_mac) strcpy(stub_name, "stub");

	xprintf(("fbAddr is 0x%08x\n", fbAddr));
	image = B_ERROR;

	/* iterate through the graphic drivers and find the first one that accepts the job */
	for (i = 0; i < (sizeof (dirs) / sizeof(dirs[0])); i++) {
		/* get the path for the add-on directory */
#if 1
		if (find_directory (dirs[i], -1, false, path, PATH_MAX) != B_OK)
			continue;
		strcat (path, "/app_server/");
#else
		strcpy(path, "/boot/home/config/add-ons/app_server/saveme/");
#endif
		len = strlen(path);
		d = opendir(path);
		if (!d) continue;

		xprintf(("Looking for app_server add-ons in %s\n", path));
		/* ---
			try all files in the directory
		--- */
		stub_exists = false;

		while ((e = readdir(d)) != NULL) {
			if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
				continue;
			strcpy(&path[len], e->d_name);

			/* is it the stub driver? */
			if (!strcmp(e->d_name, stub_name)) {
				stub_exists = true;
				continue;
			}
			
			if (!use_stub) image = load_graphic_addon(fd, path, false, fbAddr);
			else image = -1;
			if (image >= 0) break;
		}
		/* clean up */
		closedir(d);
		if (image >= 0) break;
		
		if (stub_exists) {
			strcpy(&path[len], stub_name);
			image = load_graphic_addon(fd, path, true, fbAddr);
			if (image >= 0) break;
		}
	}
	if (image < 0) return image;
	
	return image >= 0 ? B_OK : B_ERROR;
}

static image_id load_image_and_find_symbol(char *path) {
	int			sym;
	image_id	image;
	static char	*symbols[] = {
		"control_onboard_graphics_card",	/* MUST BE FIRST! */
		"control_onboard_graphics_card__FUlPv",
		"control_graphics_card",
		"control_graphics_card__FUlPv"
	};
	
	/* load the add-on */
	image = load_add_on (path);
	if (image < 0) goto err0;

	/* ---
		check for our beloved control call
	--- */

	for (sym = sizeof (symbols)/sizeof(symbols[0]) - 1; sym >= 0; sym--)
		if (get_image_symbol (image, symbols[sym], 2, (void *)&vga_ctrl_jmp) == B_NO_ERROR)
			break;

	if (sym < 0) {
		unload_add_on(image);
		image = B_ERROR;
	}

err0:
	return image;
}

static image_id load_graphic_addon (int fd, char *path, bool generic, ulong fbAddr)
{
	long				err;
	int					i;
	int32				vga_devices = STUB_MAP_MAGIC;
#if !defined(__INTEL__)
	area_id		aid;
	area_info	ainfo;
#endif
	image_id			image;
	static char			area_name[] = "pci_bus255__isa_ioxxxxxxxx";

	/* prevent unused warning */
	fbAddr;
	image = load_image_and_find_symbol(path);
	if (image < 0) goto err0;

	xprintf(("Try to hire %s %s as graphic driver...\n",
		generic ? "generic add-on" : "add-on", path));


	err = ioctl(fd, VGA_MAP_GET_COUNT, &vga_devices, sizeof(vga_devices));
	if (err != B_OK) goto err1;
	/* skip over PCI slots for Macs when using the stub driver, because we map it generically */
	ssi->info.nth = (generic ? vga_devices - 1 : 0);
	for (; ssi->info.nth < vga_devices; ssi->info.nth++) {
		status_t ioctl_result;
		/* do the area locking thing */
		//Area::GlobalAreaLock();
		/* cause the nth PCI graphics card to be mapped */
		ioctl_result = ioctl(fd, VGA_MAP_MAP_NTH, &ssi->info, sizeof(ssi->info));
		if (ioctl_result != B_OK) {
			/* just unlock */
			//Area::GlobalAreaUnlock();
			continue;
		}
		/* unlock and rescan */
		//Area::GlobalAreaUnlockRescan();
		xprintf(("ioctl(VGA_MAP_MAP_NTH) completed with B_OK\n"));

#if !defined(__INTEL__)	
		// Another hack: we need to ensure we use the same
		// screen that the Mac launcher set up for us.  We will
		// ignore any PCI cards that don't contain fbAddr.  -- SAB
		/*
		More Hack Info: we now compare physical addresses, as those are the only ones
		that will remain constant while we map and unmap areas.  Also note that we
		fake a pci_info structure for the motherboard video devices.
		*/
		xprintf(("app_server - fbAddr: 0x%08x\n", fbAddr));
		if( fbAddr ) {
			bool cont = TRUE;
			for( i=0; i<6; i++ ) {
				ulong hbase = ssi->info.pcii.u.h0.base_registers_pci[i];
				ulong hsize = ssi->info.pcii.u.h0.base_register_sizes[i];
				xprintf(("  0x%08x <= 0x%08x <= 0x%08x ?\n", hbase, fbAddr, hbase + hsize));
				if( fbAddr >= hbase && fbAddr < (hbase + hsize) ) {
					// we found the card!  Let us use it.
					cont = FALSE;
					xprintf(("    YES!\n"));
					break;
				}
				xprintf(("    NO.\n"));
			}
			if( cont ) goto unmap;
		}
#endif
		vgaDesc.screen_base = (void*)ssi->info.pcii.u.h0.base_registers[0];
		vgaDesc.device_id = ssi->info.pcii.device_id;
		vgaDesc.vendor_id = ssi->info.pcii.vendor_id;
		xprintf(("Passing 0x%08x as screen_base.\n", vgaDesc.screen_base));

		/* locate ISA-compatible i/o space, in case driver needs it.
		   In the future (DR9 and later), we will revise the pci_info
		   structure to have the isa_io space address, so all this
		   hacky code goes away. */

		/* hack: busses downstream from a pci-pci bridge do not
		   have an isa area set up, as they share with the host
		   bridge from whence they originate.  In true hacking style,
		   we then just look up the chain.  I can't wait for DR9... */
#if !defined(__INTEL__)
		/* in case some of the area mapping code fails */
		err = B_ERROR;
		for (i = ssi->info.pcii.bus; i >= 0; i--) {
			sprintf (area_name, "pci_bus%d_isa_io", i);
			aid = find_area(area_name);
			if (aid >= 0)
				break;
		}
		/* more hacking for older kernels */
		if (aid < 0)
			aid = find_area("isa_io");
		if (aid < 0)
			aid = find_area("pci_io");
		if (aid < 0)
			break;

		if (get_area_info(aid,&ainfo) < 0)
			break;
		vgaDesc.io_base = (uchar*)ainfo.address;
		ssi->current_device_info.io_area = aid;
#endif

		err = (vga_ctrl_jmp)(B_OPEN_GRAPHICS_CARD,(void*)&vgaDesc);
		
		/* bail if we got a good one */
		if (err == B_OK) break;
unmap:
		xprintf(("undoing %dth mapping\n", ssi->info.nth));
		/* undo a previous mapping */
		/* attempting to unmap the built-in video is an error, but we ignore it */
		/* undoing a mapping we never did isn't a problem either */
		ioctl(fd, VGA_MAP_UNMAP_NTH, &ssi->info.nth, sizeof(ssi->info.nth));
	}

	if (err == B_NO_ERROR) {
		// get a copy of the pathname to the add-on.
		for (i=0; i<64+B_FILE_NAME_LENGTH; i++) {
			ssi->current_device_info.path[i] = path[i];
			if (path[i] == 0) break;
		}
		ssi->current_device_info.path[63+B_FILE_NAME_LENGTH] = 0;
		xprintf(("%s is the new graphic driver...\n",path));
		mapped_card = ssi->info.nth;
		return image;
	}

err1:
	unload_add_on(image);
err0:
	xprintf(("%s refuse the job\n",path));
	return -1;
}


static void scrn2scrn(engine_token *et, blit_params *list, uint32 count) {
	if ((file_handle == -1) && (ssi->update_clone)) update_clone_data();
	/* call the blit function for the loaded driver */
	while (count--) {
		(blit_jmp)(list->src_left, list->src_top, list->dest_left, list->dest_top, list->width, list->height);
		list++;
	}
	ssi->fifo_count++;
	return;
	/* supress unused warning */
	et ;
}

static void stub_invert_rect(engine_token *et, fill_rect_params *list, uint32 count) {
	if ((file_handle == -1) && (ssi->update_clone)) update_clone_data();
	/* call the blit function for the loaded driver */
	while (count--) {
		(invert_rect_24_jmp)(list->left, list->top, list->right, list->bottom);
		list++;
	}
	ssi->fifo_count++;
	return;
	/* supress unused warning */
	et ;
}

static void rectfill8(engine_token *et, uint32 color, fill_rect_params *list, uint32 count) {
	while (count--) {
		(rect_8_jmp)(list->left, list->top, list->right, list->bottom, (uchar)color);
		list++;
	}
	ssi->fifo_count++;
	return;
	/* supress unused warning */
	et ;
}

static void rectfill1516(engine_token *et, uint32 color, fill_rect_params *list, uint32 count) {
	uint16 scolor = color;

	if ((file_handle == -1) && (ssi->update_clone)) update_clone_data();

	while (count--) {
		(rect_15_jmp)(list->left, list->top, list->right, list->bottom, scolor);
		list++;
	}
	ssi->fifo_count++;
	return;
	/* supress unused warning */
	et ;
}

static void rectfill32(engine_token *et, uint32 color, fill_rect_params *list, uint32 count) {
	uint32 lcolor = color;

	if ((file_handle == -1) && (ssi->update_clone)) update_clone_data();

	while (count--) {
		(rect_24_jmp)(list->left, list->top, list->right, list->bottom, lcolor);
		list++;
	}
	ssi->fifo_count++;
	return;
	/* supress unused warning */
	et ;
}

static void spanfill8(engine_token *et, uint32 color, uint16 *list, uint32 count) {
	if ((file_handle == -1) && (ssi->update_clone)) update_clone_data();
	while (count--) {
		uint32 y = (uint32)(*list++);
		uint32 left = (uint32)(*list++);
		uint32 right = (uint32)(*list++);
		(rect_8_jmp)(left, y, right, y, (uchar)color);
	}
	ssi->fifo_count++;
	return;
	/* supress unused warning */
	et ;
}

static void spanfill1516(engine_token *et, uint32 color, uint16 *list, uint32 count) {
	uint16 scolor = color;

	if ((file_handle == -1) && (ssi->update_clone)) update_clone_data();

	while (count--) {
		uint32 y = (uint32)(*list++);
		uint32 left = (uint32)(*list++);
		uint32 right = (uint32)(*list++);
		(rect_15_jmp)(left, y, right, y, scolor);
	}
	ssi->fifo_count++;
	return;
	/* supress unused warning */
	et ;
}

static void spanfill32(engine_token *et, uint32 color, uint16 *list, uint32 count) {
	uint32 lcolor = color;

	if ((file_handle == -1) && (ssi->update_clone)) update_clone_data();

	while (count--) {
		uint32 y = (uint32)(*list++);
		uint32 left = (uint32)(*list++);
		uint32 right = (uint32)(*list++);
		(rect_24_jmp)(left, y, right, y, lcolor);
	}
	ssi->fifo_count++;
	return;
	/* supress unused warning */
	et ;
}

static uint32 stub_get_engine_count(void) {
	xprintf(("stub_get_engine_count()\n"));
	return 1;
}

static status_t stub_acquire_engine(uint32 caps, uint32 max_wait, sync_token *st, engine_token **et) {
	/* acquire the shared benaphore */
	int32 old = atomic_add(&ssi->engine_ben, 1);
	if (old >= 1) acquire_sem(ssi->engine_sem);

	yprintf(("stub_acquire_engine(st:0x%08x, et:0x%08x)\n", st, et));
	/* sync if required */
	if (st) stub_sync_to_token(st);

	/* return an engine_token */
	*et = &stub_engine_token;
	return B_OK;
	caps ; max_wait ;
}

static status_t stub_release_engine(engine_token *et, sync_token *st) {
	int32 old;

	yprintf(("stub_release_engine(et:0x%08x, st:0x%08x)\n", et, st));
	if (!et) {
		yprintf(("ackthp! stub_release_engine() called with null engine_token!\n"));
		return B_ERROR;
	}
	/* update the sync token, if any */
	if (st) {
		yprintf(("updating sync token - id: %d, fifo %Ld\n", et->engine_id, ssi->fifo_count));
		st->engine_id = et->engine_id;
		st->counter = ssi->fifo_count;
	}

	/* release the shared benaphore */
	old = atomic_add(&ssi->engine_ben, -1);
	if (old > 1) release_sem(ssi->engine_sem);
	yprintf(("stub_release_engine() completes\n"));
	return B_OK;
}

static void stub_wait_engine_idle(void) {
	yprintf(("stub_wait_engine_idle()\n"));
	if (synchro_jmp) synchro_jmp();
}

static status_t stub_get_sync_token(engine_token *et, sync_token *st) {
	yprintf(("stub_get_sync_token(et:0x%08x, st:0x%08x)\n", et, st));
	st->engine_id = et->engine_id;
	st->counter = ssi->fifo_count;
	yprintf(("stub_get_sync_token() completes\n"));
	return B_OK;
}

static status_t stub_sync_to_token(sync_token *st) {
	yprintf(("stub_sync_to_token(st: 0x%08x)\n", st));
	stub_wait_engine_idle();
	yprintf(("stub_sync_to_token() completes\n"));
	return B_OK;
	/* prevent unused warning */
	st;
}

void set_colors(uint count, uint8 first, uint8 *color_data, uint32 flags) {
	/* call the add-on for each color */
	indexed_color	thePEL;

	if ((file_handle == -1) && (ssi->update_clone)) update_clone_data();

	xprintf(("set_indexed_colors(%d, %d, 0x%08x, 0x%08x)\n", count, first, color_data, flags));
	thePEL.index = first;
	while (count--) {
		thePEL.color.red = *color_data++;
		thePEL.color.green = *color_data++;
		thePEL.color.blue = *color_data++;
		(vga_ctrl_jmp)(B_SET_INDEXED_COLOR, (void *)&thePEL);
		thePEL.index++;
	}
	xprintf(("set_indexed_colors() completes\n"));
	return;
	/* prevent unused warning */
	flags;
}

/* need a way to convert from display_mode to old mode setup info */
static void parms2oldmode(uint16 width, uint16 height, uint32 cs, uint32 *mode) {
	int32
		area_diff,
		best_diff;
	uint32
		t_mode = 1,
		area = (uint32)width * (uint32)height;
	uint32
		t_cs;
	uint16
		t_width,
		t_height;

	/* fail by default */
	*mode = 0;
	best_diff = 10000000;
	/* ignore endianness */
	cs &= (uint32)~0x00001000;
	/* try each mode */
	while (t_mode) {
		if (oldmode2parms(t_mode, &t_cs, &t_width, &t_height) == B_OK) {
			area_diff = area - ((uint32)t_width * (uint32)t_height);
			if (area_diff < 0) area_diff = -area_diff;
			if ((area_diff <= best_diff) && (cs == (t_cs & ~0x00001000))
			    && width <= t_width && height <= t_height) {
				best_diff = area_diff;
				*mode = t_mode;
			}
		}
		t_mode <<= 1;
	}
}

static status_t oldmode2parms(uint32 mode, uint32 *cs, uint16 *width, uint16 *height) {
	switch (mode) {
		case B_8_BIT_640x480:
		case B_8_BIT_800x600:
		case B_8_BIT_1024x768:
		case B_8_BIT_1152x900:
		case B_8_BIT_1280x1024:
		case B_8_BIT_1600x1200:
			*cs = B_CMAP8;
			break;
		case B_15_BIT_640x480:
		case B_15_BIT_800x600:
		case B_15_BIT_1024x768:
		case B_15_BIT_1152x900:
		case B_15_BIT_1280x1024:
		case B_15_BIT_1600x1200:
			*cs = B_RGB15;
			break;
		case B_16_BIT_640x480:
		case B_16_BIT_800x600:
		case B_16_BIT_1024x768:
		case B_16_BIT_1152x900:
		case B_16_BIT_1280x1024:
		case B_16_BIT_1600x1200:
			*cs = B_RGB16;
			break;
		case B_32_BIT_640x480:
		case B_32_BIT_800x600:
		case B_32_BIT_1024x768:
		case B_32_BIT_1152x900:
		case B_32_BIT_1280x1024:
		case B_32_BIT_1600x1200:
			*cs = B_RGB32;
			break;
		default:
			return B_ERROR;
	}
	/* big endian only on macs and when not indexed */
	if (ssi->is_mac & (*cs != B_CMAP8)) *cs |= (uint32)0x1000;
	switch (mode) {
		case B_8_BIT_640x480:
		case B_15_BIT_640x480:
		case B_16_BIT_640x480:
		case B_32_BIT_640x480:
			*width = 640;
			*height = 480;
			break;
		case B_8_BIT_800x600:
		case B_15_BIT_800x600:
		case B_16_BIT_800x600:
		case B_32_BIT_800x600:
			*width = 800;
			*height = 600;
			break;
		case B_8_BIT_1024x768:
		case B_15_BIT_1024x768:
		case B_16_BIT_1024x768:
		case B_32_BIT_1024x768:
			*width = 1024;
			*height = 768;
			break;
		case B_8_BIT_1152x900:
		case B_15_BIT_1152x900:
		case B_16_BIT_1152x900:
		case B_32_BIT_1152x900:
			*width = 1152;
			*height = 900;
			break;
		case B_8_BIT_1280x1024:
		case B_15_BIT_1280x1024:
		case B_16_BIT_1280x1024:
		case B_32_BIT_1280x1024:
			*width = 1280;
			*height = 1024;
			break;
		case B_8_BIT_1600x1200:
		case B_15_BIT_1600x1200:
		case B_16_BIT_1600x1200:
		case B_32_BIT_1600x1200:
			*width = 1600;
			*height = 1200;
			break;
	}
	return B_OK;
}
