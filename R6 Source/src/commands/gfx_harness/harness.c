#include <FindDirectory.h>
#include <OS.h>
#include <image.h>
#include <graphic_driver.h>
#include <Accelerant.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <malloc.h>
#include <errno.h>
#include <video_overlay.h>
#include <tv_out.h>

static uint8 my_hand_cursor_xor[] = {
#if 0
	0xf0, 0x0f,
	0x80, 0x01,
	0x80, 0x01,
	0x80, 0x01,
	0x00, 0x00,
	0x00, 0x00,
	0x0f, 0xf0,
	0x08, 0x10,
	0x08, 0x10,
	0x0f, 0xf0,
	0x00, 0x00,
	0x00, 0x00,
	0x80, 0x01,
	0x80, 0x01,
	0x80, 0x01,
	0xf0, 0x0f
#else
0x0,0x0,0x0,0x0,0x38,0x0,0x24,0x0,0x24,0x0,0x13,0xe0,0x12,0x5c,0x9,
0x2a,0x8,0x1,0x3c,0x1,0x4c,0x1,0x42,0x1,0x30,0x1,0xc,0x1,0x2,0x0,
0x1,0x0
#endif

};
static uint8 my_hand_cursor_and[] = {
#if 0
	0xf0, 0x0f,
	0x80, 0x01,
	0x80, 0x01,
	0x80, 0x01,
	0x00, 0x00,
	0x00, 0x00,
	0x0f, 0xf0,
	0x08, 0x10,
	0x08, 0x10,
	0x0f, 0xf0,
	0x00, 0x00,
	0x00, 0x00,
	0x80, 0x01,
	0x80, 0x01,
	0x80, 0x01,
	0xf0, 0x0f
#else
0x0,0x0,0x0,0x0,0x38,0x0,0x3c,0x0,0x3c,0x0,0x1f,0xe0,0x1f,0xfc,0xf,
0xfe,0xf,0xff,0x3f,0xff,0x7f,0xff,0x7f,0xff,0x3f,0xff,0xf,0xff,0x3,0xfe,
0x1,0xf8
#endif
};

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

int space_bits_per_pixel(uint32 cs) {
	switch (cs) {
		case B_RGB32:
		case B_RGBA32:
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
			return 32;
		case B_RGB16:
		case B_RGB16_BIG:
			return 16;
		case B_RGB15:
		case B_RGBA15:
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
			return 15;
		case B_CMAP8:
		case B_GRAY8:
			return 8;
	}
	return 0;
}

int pick_from_list(const char *msg, char **list) {
	int count = 1;
	int choice;
	puts(msg);
	while (*list) {
		printf(" %2.0d) %s\n", count, *list);
		list++;
		count++;
	}
	while (1) {
		printf("Enter [1-%d]: ", count-1); fflush(stdout);
		if (scanf("%d", &choice) != 1) fgetc(stdin);
		else if ((choice >= 0) && (choice < count)) break;
	}
	return choice - 1;
}

/* return a file descriptor of the desired device, or -1 on failure */
int pick_device(const char *apath) {
	int count = 0;
	int choice;
	DIR				*d;
	struct dirent	*e;
	char
		name_buf[1024],
		message[256],
		*names[16],
		*name = name_buf;

	/* open directory apath */
	d = opendir(apath);
	if (!d) return B_ERROR;
	/* get a list of devices, filtering out . and .. */
	while ((e = readdir(d)) != NULL) {
		if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
			continue;
		strcpy(name, e->d_name);
		names[count++] = name;
		name += strlen(name)+1;
	}
	closedir(d);
	names[count] = NULL;		
	/* if there are no devices, return error */
	if (count == 0) return B_ERROR;
	/* prompt user for which device */
	sprintf(message, "Choose from these devices found in %s:", apath);
	choice = pick_from_list(message, names);
	if (choice >= 0) {
		sprintf(message, "%s/%s", apath, names[choice]);
		return open(message, B_READ_WRITE);
	}
	return B_ERROR;
}

image_id load_accelerant(int fd, GetAccelerantHook *hook) {
	status_t result;
	image_id image = -1; /* fail by default */
	uint32 i;
	char
		signature[1024],
		path[PATH_MAX * 2];
	struct stat st;
	static const directory_which vols[] = {
		B_USER_ADDONS_DIRECTORY,
		B_COMMON_ADDONS_DIRECTORY,
		B_BEOS_ADDONS_DIRECTORY
	};

	/* get signature from driver */
	result = ioctl(fd, B_GET_ACCELERANT_SIGNATURE, &signature, sizeof(signature));
	if (result != B_OK) goto done;
	printf("B_GET_ACCELERANT_SIGNATURE returned ->%s<-\n", signature);

	for(i=0; i < sizeof (vols) / sizeof (vols[0]); i++) {

		/* ---
			compute directory path to common or beos addon directory on
			floppy or boot volume
		--- */

		printf("attempting to get path for %lu (%u)\n", i, vols[i]);
		if (find_directory (vols[i], -1, false, path, PATH_MAX) != B_OK) {
			printf("find directory failed\n");
			continue;
		}

		strcat (path, "/accelerants/");
		strcat (path, signature);

		printf("about to stat(%s)\n", path);
		// don't try to load non-existant files
		if (stat(path, &st) != 0) continue;
		printf("Trying to load accelerant: %s\n", path);
		// load the image
		image = load_add_on(path);
		if (image >= 0) {
			printf("Accelerant loaded!\n");
			// get entrypoint from accelerant
			result = get_image_symbol(image, B_ACCELERANT_ENTRY_POINT,
#if defined(__ELF__)
				B_SYMBOL_TYPE_ANY,
#else
				B_SYMBOL_TYPE_TEXT,
#endif
				(void **)hook);
			if (result == B_OK) {
				init_accelerant ia;
				printf("Entry point %s() found\n", B_ACCELERANT_ENTRY_POINT);
				ia = (init_accelerant)(*hook)(B_INIT_ACCELERANT, NULL);
				printf("init_accelerant is 0x%08lx\n", (uint32)ia);
				if (ia && ((result = ia(fd)) == B_OK)) {
					// we have a winner!
					printf("Accelerant %s accepts the job!\n", path);
					break;
				} else {
					printf("init_accelerant refuses the the driver: %ld\n", result);
				}
			} else {
				printf("Couldn't find the entry point.\n");
			}
			// unload the accelerant, as we must be able to init!
			unload_add_on(image);
		}
		if (image < 0) printf("image failed to load %ld (%.8lx) %s\n", image, image, strerror(image));
		// mark failure to load image
		image = -1;
	}

	printf("Add-on image id: %ld\n", image);

done:
	return image;
}

typedef struct {
		uint32	opcode;
		char	*command_name;
} command;
#define MK_CMD(x) { x, #x }
#define MK_SEC(s) { 0, s }

enum {
	T_PICK_A_MODE = B_ACCELERANT_PRIVATE_START+10000,
	T_TWEAK_MODE,
	T_SET_OVERLAY_SCALING_FACTOR,
	T_SET_OVERLAY_H_POSITION,
	T_SET_OVERLAY_V_POSITION,
	T_SET_OVERLAY_FEATURES,
	T_CLEAR_OVERLAY_FEATURES,
	T_SET_OVERLAY_OFFSETS,
	T_SET_FILL_COLOR,
	T_TWEAK_TV_OUT_ADJUSTMENTS
};

static command commands[] = {
	/* mode configuration */
	MK_CMD(B_ACCELERANT_MODE_COUNT),			/* required */
	MK_CMD(B_GET_MODE_LIST),			/* required */
	MK_CMD(T_PICK_A_MODE),
	MK_CMD(B_PROPOSE_DISPLAY_MODE),	/* optional */
	MK_CMD(B_SET_DISPLAY_MODE),		/* required */	
	MK_CMD(B_GET_DISPLAY_MODE),		/* required */
	MK_CMD(B_GET_FRAME_BUFFER_CONFIG),	/* required */
	MK_CMD(B_GET_PIXEL_CLOCK_LIMITS),	/* required */
	MK_CMD(B_MOVE_DISPLAY),				/* optional */
	MK_CMD(B_SET_INDEXED_COLORS),		/* required if driver supports 8bit indexed modes */
	MK_CMD(B_ACCELERANT_RETRACE_SEMAPHORE),
	MK_CMD(B_SET_DPMS_MODE),			/* optional */
	MK_CMD(T_TWEAK_MODE),
	MK_SEC("Mode Configuration"),

	/* cursor managment */
	MK_CMD(B_MOVE_CURSOR),				/* optional */
	MK_CMD(B_SET_CURSOR_SHAPE),			/* optional */
	MK_CMD(B_SHOW_CURSOR),				/* optional */
	MK_SEC("Cursor Managment"),

	/* synchronization */
	MK_CMD(B_ACQUIRE_ENGINE),
	MK_CMD(B_RELEASE_ENGINE),
	MK_CMD(B_WAIT_ENGINE_IDLE),

	/* 2D acceleration */
	MK_CMD(B_SCREEN_TO_SCREEN_BLIT),
	MK_CMD(B_FILL_RECTANGLE),
	MK_CMD(B_INVERT_RECTANGLE),
	MK_CMD(B_FILL_SPAN),
#if NEW_BLIT_HOOKS
	MK_CMD(B_SCREEN_TO_SCREEN_TRANSPARENT_BLIT),
	MK_CMD(B_SCREEN_TO_SCREEN_SCALED_FILTERED_BLIT),
#endif
	MK_CMD(T_SET_FILL_COLOR),
	MK_SEC("Acceleration"),

	/* overlay support */
	MK_CMD(B_OVERLAY_COUNT),
	MK_CMD(B_OVERLAY_SUPPORTED_SPACES),
	MK_CMD(B_OVERLAY_SUPPORTED_FEATURES),
	MK_CMD(B_ALLOCATE_OVERLAY_BUFFER),
	MK_CMD(B_RELEASE_OVERLAY_BUFFER),
	MK_CMD(B_GET_OVERLAY_CONSTRAINTS),
	MK_CMD(B_ALLOCATE_OVERLAY),
	MK_CMD(B_RELEASE_OVERLAY),
	MK_CMD(B_CONFIGURE_OVERLAY),
	MK_CMD(T_SET_OVERLAY_SCALING_FACTOR),
	MK_CMD(T_SET_OVERLAY_H_POSITION),
	MK_CMD(T_SET_OVERLAY_V_POSITION),
	MK_CMD(T_SET_OVERLAY_FEATURES),
	MK_CMD(T_CLEAR_OVERLAY_FEATURES),
	MK_CMD(T_SET_OVERLAY_OFFSETS),
	MK_SEC("Overlays"),
	
	/* tv output */
	MK_CMD(B_GET_TV_OUT_ADJUSTMENTS_FOR_MODE),
	MK_CMD(T_TWEAK_TV_OUT_ADJUSTMENTS),
	MK_SEC("TV Out"),
};
#define CMD_SIZE (sizeof(commands) / sizeof(command))

void missing_feature(char *s) {
	printf("Accelerant doesn't implement required feature %s\n", s);
}

void missing_option(char *s) {
	printf("Accelerant doesn't implement optional feature %s\n", s);
}

void failed_with_reason(char *s, status_t r) {
	printf("%s failed with reason %ld (0x%08lx)\n", s, r, r);
}

void failed_string(char *s, char *r) {
	printf("%s failed with reason %s\n", s, r);
}

void dump_overlay_buffer(const overlay_buffer *ob) {
	printf("     color space: %s\n", spaceToString(ob->space));
	printf("            size: %d,%d\n", ob->width, ob->height);
	printf("   bytes_per_row: %ld\n", ob->bytes_per_row);
	printf("    buffer start: %p\n", ob->buffer);
	printf("dma buffer start: %p\n", ob->buffer_dma);
}

void dump_overlay_constraints(const overlay_constraints *oc) {
	printf("view constraints:\n");
	printf(" alignment:\n");
	printf("           h, v: 0x%04x, 0x%04x\n", oc->view.h_alignment, oc->view.v_alignment);
	printf("  width, height: 0x%04x, 0x%04x\n", oc->view.width_alignment, oc->view.height_alignment);
	printf(" limits:\n");
	printf("          width: min %d, max %d\n", oc->view.width.min, oc->view.width.max);
	printf("         height: min %d, max %d\n", oc->view.width.min, oc->view.width.max);
	printf("window constraints:\n");
	printf(" alignment:\n");
	printf("           h, v: 0x%04x, 0x%04x\n", oc->window.h_alignment, oc->window.v_alignment);
	printf("  width, height: 0x%04x, 0x%04x\n", oc->window.width_alignment, oc->window.height_alignment);
	printf(" limits:\n");
	printf("          width: min %d, max %d\n", oc->window.width.min, oc->window.width.max);
	printf("         height: min %d, max %d\n", oc->window.width.min, oc->window.width.max);
	printf("scaling:\n");
	printf("     horizontal: min %f, max %f\n", oc->h_scale.min, oc->h_scale.max);
	printf("       vertical: min %f, max %f\n", oc->v_scale.min, oc->v_scale.max);
	printf("\n");
}

void dump_overlay_features(const char *s, uint32 features) {
	printf("%s:\n", s);
#define PF(a)	if (features & a) printf(" "#a"\n")
	PF(B_OVERLAY_COLOR_KEY);
	PF(B_OVERLAY_CHROMA_KEY);
	PF(B_OVERLAY_HORIZONTAL_FITLERING);
	PF(B_OVERLAY_VERTICAL_FILTERING);
	PF(B_OVERLAY_HORIZONTAL_MIRRORING);
	PF(B_OVERLAY_KEYING_USES_ALPHA);
	PF(B_OVERLAY_FROM_VIP);
#undef PF
	if (!features) printf(" <none>\n");
	printf("\n");
}

void dump_mode(display_mode *dm) {
	display_timing *t = &(dm->timing);
	printf("  pixel_clock: %ldKHz\n", t->pixel_clock);
	printf("            H: %4d %4d %4d %4d\n", t->h_display, t->h_sync_start, t->h_sync_end, t->h_total);
	printf("            V: %4d %4d %4d %4d\n", t->v_display, t->v_sync_start, t->v_sync_end, t->v_total);
	printf(" timing flags:");
	if (t->flags & B_BLANK_PEDESTAL) printf(" B_BLANK_PEDESTAL");
	if (t->flags & B_TIMING_INTERLACED) printf(" B_TIMING_INTERLACED");
	if (t->flags & B_POSITIVE_HSYNC) printf(" B_POSITIVE_HSYNC");
	if (t->flags & B_POSITIVE_VSYNC) printf(" B_POSITIVE_VSYNC");
	if (t->flags & B_SYNC_ON_GREEN) printf(" B_SYNC_ON_GREEN");
	if (!t->flags) printf(" (none)\n");
	else printf("\n");
	printf(" refresh rate: %4.2fKHz H, %4.2f Hz V\n", ((double)t->pixel_clock / (double)t->h_total), ((double)t->pixel_clock * 1000) / ((double)t->h_total * (double)t->v_total));
	printf("  color space: %s\n", spaceToString(dm->space));
	printf(" virtual size: %dx%d\n", dm->virtual_width, dm->virtual_height);
	printf("dispaly start: %d,%d\n", dm->h_display_start, dm->v_display_start);

	printf("   mode flags: %.08lx\n\t", dm->flags);
#define DUMPMASKFLAG(mask, value) if ((dm->flags & (uint32)(mask)) == (uint32)(value)) printf(" "#value);
#define DUMPFLAG(value) DUMPMASKFLAG(value, value)
	DUMPFLAG(B_SCROLL);
	DUMPFLAG(B_8_BIT_DAC);
	DUMPFLAG(B_HARDWARE_CURSOR);
	DUMPFLAG(B_PARALLEL_ACCESS);
	DUMPFLAG(B_SUPPORTS_OVERLAYS);
#define TVOUTFLAG(x) DUMPMASKFLAG(B_TV_OUT_MASK, x)
	TVOUTFLAG(B_TV_OUT_NONE);
	TVOUTFLAG(B_TV_OUT_NTSC);
	TVOUTFLAG(B_TV_OUT_NTSC_J);
	TVOUTFLAG(B_TV_OUT_PAL);
	TVOUTFLAG(B_TV_OUT_PAL_M);
	if (!dm->flags) printf(" (none)\n");
	else printf("\n");
}

display_mode * pick_a_mode(display_mode *mode_list, uint32 mode_count) {
	char **modes = (char **)calloc(sizeof(char *),mode_count+1);
	char buffer[128];
	int mode;
	display_mode *dm = NULL;
	if (!modes) {
		printf("Couldn't allocate enough RAM for pick-a-mode list.\n");
		goto done;
	}
	for (mode = 0; mode < (int)mode_count; mode++) {
		display_mode *dm = mode_list + mode;
		sprintf(buffer, "%dx%d@%4.2f %ld KHz pixel clock (%dx%d virtual) %s", dm->timing.h_display, dm->timing.v_display,
			((double)dm->timing.pixel_clock * 1000) / ((double)dm->timing.h_total * (double)dm->timing.v_total), dm->timing.pixel_clock,
			dm->virtual_width, dm->virtual_height, spaceToString(dm->space));
		modes[mode] = strdup(buffer);
	}
	modes[mode] = NULL;
	mode = pick_from_list("Select a display mode:", modes);
	if (mode >= 0) {
		dm = mode_list + mode;
		dump_mode(dm);
	}
	for (mode = 0; mode < (int)mode_count; mode++)
		free(modes[mode]);
	free(modes);
done:
	return dm;
}

void paint_for_blit(display_mode *dm, frame_buffer_config *fbc) {
	switch (dm->space & ~0x3000) {
	case B_CMAP8: {
		int16 x, y;
		uint8 *fb = (uint8 *)fbc->frame_buffer;
		printf(" frame buffer is 8bpp\n");
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
		printf(" frame buffer is 16bpp\n");
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
		printf(" frame buffer is 15bpp\n");
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
		printf(" frame buffer is 32bpp\n");
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
		printf("YIKES! frame buffer shape unknown!\n");
	}
}

static int getcmd(void)
{
	char buf[1024];
	char *s = fgets(buf, sizeof(buf), stdin);
	if (s) return *s;
	return 0;
}

static void
tweak_mode(set_display_mode sdc, const display_mode *orig)
{
	display_mode mode = *orig;
	int key;
	bool setting = true;
	char peg_vertical = 'v';
	status_t err;
	float refresh = (float)mode.timing.pixel_clock * 1000.0;
	if (peg_vertical == 'v') refresh /= (float)mode.timing.h_total * mode.timing.v_total;
	else refresh /= (float)mode.timing.h_total;

	fprintf(stdout,
	"Choose:\n"
	" (u)p,(d)own,(l)eft,(r)ight\n"
	" (t)aller,(s)horter,(w)ider,(n)arrower\n"
	" h-sync pulse (,)shorter,(.)longer\n"
	" peg (c)lock,(h)oriz. refresh, (v)ertical refresh,(;)specify rate\n"
	" (x)revert mode, (p)toggle mode setting, (q)uit tweaking  :");
	fflush(stdout);
	
	while ((key = getcmd()) != 'q') {
		bool changed = true;
		switch (key) {
#if 0
			case 'g': // get current mode
				err = screen.GetMode(&mode);
				orig = mode;
				changed = false;
				dump_mode(&mode);
				break;
#endif
			case 'u': // up
				mode.timing.v_sync_start += 1;
				mode.timing.v_sync_end += 1;
				break;
			case 'd': // down
				mode.timing.v_sync_start -= 1;
				mode.timing.v_sync_end -= 1;
				break;
			case 'l': // left
				mode.timing.h_sync_start += 8;
				mode.timing.h_sync_end += 8;
				break;
			case 'r': // right
				mode.timing.h_sync_start -= 8;
				mode.timing.h_sync_end -= 8;
				break;
			case ',': // short h sync pulse
				mode.timing.h_sync_end -=8;
				break;
			case '.': // longer h sync pulse
				mode.timing.h_sync_end +=8;
				break;
			case 'w': // wider
				mode.timing.h_total -= 8;
				break;
			case 'n': // narrower
				mode.timing.h_total += 8;
				break;
			case 't': // taller
				mode.timing.v_total -= 1;
				break;
			case 's': // shorter
				mode.timing.v_total += 1;
				break;
			case ';': // set refresh rate
				{
				float new_refresh;
				fprintf(stdout, "New rate: "); fflush(stdout);
				fscanf(stdin, "%f", &new_refresh);
				// burn the new line
				fgetc(stdin);
				if (new_refresh) {
					if (peg_vertical != 'c') refresh = new_refresh;
					else mode.timing.pixel_clock = (uint32)new_refresh;
				} else changed = false;
				}
				break;
			case 'x': // revert to original mode
				mode = *orig;
				// fall through to recalc refresh rate
			case 'c': // peg pixel clock
			case 'h': // peg horizontal refresh rate
			case 'v': // peg vertical refresh rate (default)
				if (key != 'x') {
					peg_vertical = key;// == 'v';
					changed = false;
				}
				refresh = (float)mode.timing.pixel_clock * 1000.0;
				if (peg_vertical == 'v') refresh /= (float)mode.timing.h_total * mode.timing.v_total;
				else if (peg_vertical == 'h') refresh /= (float)mode.timing.h_total;
				fprintf(stdout, "\nUsing %s refresh rate of %.2f\n", peg_vertical == 'v' ? "vertical" : (peg_vertical == 'h' ? "horizontal" : "clock"), refresh);
				break;
			case 'p': // toggle mode setting after each mode change
				setting = !setting;
				fprintf(stdout, "Mode setting %s\n", setting ? "enabled" : "disabled");
				changed = true;
				break;
			default:
				// barf
				changed = false;
				break;
		}
		if (changed && setting) {
			// recalc pixel clock, pegging the appropriate refresh rate
			if (peg_vertical != 'c')
				mode.timing.pixel_clock = (uint32)(peg_vertical == 'v' ?
					(refresh * mode.timing.h_total * mode.timing.v_total) / 1000.0 :
					(refresh * mode.timing.h_total) / 1000.0);
			printf("Trying to set this mode:\n");
			dump_mode(&mode);
			// set new mode
			err = (sdc)(&mode);
#if 0
			if (err != B_OK) {
				printf("\nCould not set mode.  Getting current mode.\n");
				err = screen.GetMode(&mode);
			}
#endif
			// dump mode info
			//if (dump) dump_mode(&mode);
		}
		fprintf(stdout,
		"Choose:\n"
		" (u)p,(d)own,(l)eft,(r)ight\n"
		" (t)aller,(s)horter,(w)ider,(n)arrower\n"
		" h-sync pulse (,)shorter,(.)longer\n"
		" peg (c)lock,(h)oriz. refresh, (v)ertical refresh,(;)specify rate\n"
		" (x)revert mode, (p)toggle printing,  :");
		fflush(stdout); 
	}
	
}

static tv_out_adjustment_limits gToal;
static uint32 gWhichTvOut = 0;

static void get_and_dump_tv_out_adjustments_for_mode(get_tv_out_adjustments_for_mode toafm, display_mode *dm)
{
	status_t result = toafm(dm, &gWhichTvOut, &gToal);
	if (result != B_OK) return;
	
	if (gWhichTvOut & B_TV_OUT_H_POS)
		printf("B_TV_OUT_H_POS - max: %u, default: %u\n", gToal.h_pos.max_value, gToal.h_pos.default_value);
	if (gWhichTvOut & B_TV_OUT_V_POS)
		printf("B_TV_OUT_V_POS - max: %u, default: %u\n", gToal.v_pos.max_value, gToal.v_pos.default_value);
	if (gWhichTvOut & B_TV_OUT_START_ACTIVE)
		printf("B_TV_OUT_START_ACTIVE - max: %u, default: %u\n", gToal.start_active_video.max_value, gToal.start_active_video.default_value);
	if (gWhichTvOut & B_TV_OUT_BLACK_LEVEL)
		printf("B_TV_OUT_BLACK_LEVEL - max: %u, default: %u\n", gToal.black_level.max_value, gToal.black_level.default_value);
	if (gWhichTvOut & B_TV_OUT_CONTRAST)
		printf("B_TV_OUT_CONTRAST - max: %u, default: %u\n", gToal.contrast.max_value, gToal.contrast.default_value);
}

static tv_out_adjustments gAdjustments;

static void get_dump_tv_out_adj(get_tv_out_adjustments gtoa)
{
	uint32 which;
	gtoa(&which, &gAdjustments);
	if (which & B_TV_OUT_H_POS)
		printf("B_TV_OUT_H_POS: %u\n", gAdjustments.h_pos);
	if (which & B_TV_OUT_V_POS)
		printf("B_TV_OUT_V_POS: %u\n", gAdjustments.v_pos);
	if (which & B_TV_OUT_START_ACTIVE)
		printf("B_TV_OUT_START_ACTIVE: %u\n", gAdjustments.start_active_video);
	if (which & B_TV_OUT_BLACK_LEVEL)
		printf("B_TV_OUT_BLACK_LEVEL: %u\n", gAdjustments.black_level);
	if (which & B_TV_OUT_CONTRAST)
		printf("B_TV_OUT_CONTRAST: %u\n", gAdjustments.contrast);
}

static void tweak_tv_out(set_tv_out_adjustments stoa, get_tv_out_adjustments gtoa)
{
	uint32 h_pos, v_pos, start_active_video, black_level, contrast;
	uint32 which;
	int key;

	get_dump_tv_out_adj(gtoa);
	h_pos = gAdjustments.h_pos;
	v_pos = gAdjustments.v_pos;
	start_active_video = gAdjustments.start_active_video;
	black_level = gAdjustments.black_level;
	contrast = gAdjustments.contrast;
	
	fprintf(stdout,
	"Choose:\n"
	" (u)p,(d)own,(l)eft,(r)ight\n"
	" start active video (,)shorter,(.)longer\n"
	" black level (w)hiter,(b)lacker\n"
	" contrast enhance (a)white,(z)black\n"
	" (x)revert, (q)uit tweaking  :");
	fflush(stdout);

	while ((key = getcmd()) != 'q') {
		which = 0;
		switch (key) {
			case 'u': // up
				v_pos++;
				if (v_pos > gToal.v_pos.max_value) v_pos = 0;
				which = B_TV_OUT_V_POS;
				gAdjustments.v_pos = (uint16)v_pos;
				break;
			case 'd': // down
				v_pos--;
				if (v_pos > gToal.v_pos.max_value) v_pos = gToal.v_pos.max_value;
				which = B_TV_OUT_V_POS;
				gAdjustments.v_pos = (uint16)v_pos;
				break;
			case 'l': // left
				h_pos--;
				if (h_pos > gToal.h_pos.max_value) h_pos = gToal.h_pos.max_value;
				which = B_TV_OUT_H_POS;
				gAdjustments.h_pos = (uint16)h_pos;
				break;
			case 'r': // right
				h_pos++;
				if (h_pos > gToal.h_pos.max_value) h_pos = 0;
				which = B_TV_OUT_H_POS;
				gAdjustments.h_pos = (uint16)h_pos;
				break;
			case ',': // shorter
				start_active_video--;
				if (start_active_video > gToal.start_active_video.max_value) start_active_video = gToal.start_active_video.max_value;
				which = B_TV_OUT_START_ACTIVE;
				gAdjustments.start_active_video = (uint16)start_active_video;
				break;
			case '.': // longer
				start_active_video++;
				if (start_active_video > gToal.start_active_video.max_value) start_active_video = 0;
				which = B_TV_OUT_START_ACTIVE;
				gAdjustments.start_active_video = (uint16)start_active_video;
				break;
			case 'w': // black level whiter
				black_level++;
				if (black_level > gToal.black_level.max_value) black_level = 0;
				which = B_TV_OUT_BLACK_LEVEL;
				gAdjustments.black_level = (uint16)black_level;
				break;
			case 'b': // black level blacker
				black_level--;
				if (black_level > gToal.black_level.max_value) black_level = gToal.black_level.max_value;
				which = B_TV_OUT_BLACK_LEVEL;
				gAdjustments.black_level = (uint16)black_level;
				break;
			case 'a': // contrast enhance white
				contrast++;
				if (contrast > gToal.contrast.max_value) contrast = 0;
				which = B_TV_OUT_CONTRAST;
				gAdjustments.contrast = (uint16)contrast;
				break;
			case 'z': // contrast enhance black
				contrast--;
				if (contrast > gToal.contrast.max_value) contrast = gToal.contrast.max_value;
				which = B_TV_OUT_CONTRAST;
				gAdjustments.contrast = (uint16)contrast;
				break;
		}
		if (which)
		{
			stoa(which, &gAdjustments);
			get_dump_tv_out_adj(gtoa);
		}
		fprintf(stdout,
		"Choose:\n"
		" (u)p,(d)own,(l)eft,(r)ight\n"
		" start active video (,)shorter,(.)longer\n"
		" black level (w)hiter,(b)lacker\n"
		" contrast enhance (a)white,(z)black\n"
		" (x)revert, (q)uit tweaking  :");
		fflush(stdout);
	}
}

void exercise_driver(GetAccelerantHook gah) {
	/* make list of commands for picker */
	char ***cmds;// = (char **)calloc(sizeof(char *), 2 + CMD_SIZE);
	int i;
	status_t
		result = B_ERROR;
	uint32
		mode_count = 0;
	display_mode
		dm,
		*mode_list = NULL;
	engine_token
		*et;
	bool
		valid_mode = FALSE,
		engine_acquired = FALSE;

	const overlay_buffer *ob = 0;
	overlay_view	ov;
	overlay_window	ow;
	uint32	offset_left = 0;
	uint32 offset_top = 0;
	uint32 offset_right = 0;
	uint32 offset_bottom = 0;
	overlay_token	ot = 0;
	float scaling_factor = 1;
	int overlay_h_pos = 0;
	int overlay_v_pos = 0;
	uint32 features = 0;
	char * feature_list[] = {"B_OVERLAY_COLOR_KEY", "B_OVERLAY_CHROMA_KEY", "B_OVERLAY_HORIZONTAL_FITLERING", "B_OVERLAY_VERTICAL_FILTERING", "B_OVERLAY_HORIZONTAL_MIRRORING", "B_OVERLAY_KEYING_USES_ALPHA", "B_OVERLAY_FROM_VIP", "All Features", NULL};
	uint32 feature_flags[] = {B_OVERLAY_COLOR_KEY, B_OVERLAY_CHROMA_KEY, B_OVERLAY_HORIZONTAL_FITLERING, B_OVERLAY_VERTICAL_FILTERING, B_OVERLAY_HORIZONTAL_MIRRORING, B_OVERLAY_KEYING_USES_ALPHA, B_OVERLAY_FROM_VIP, -1};
	char * dpms_list[] = {"B_DPMS_ON", "B_DPMS_STAND_BY", "B_DPMS_SUSPEND", "B_DPMS_OFF", NULL};
	uint32 dpms_flags[] = {B_DPMS_ON, B_DPMS_STAND_BY, B_DPMS_SUSPEND, B_DPMS_OFF, -1};
	uint32 gColor = 0xf800f800;

	int section_count = 0;
	int index;
	int items;
	int *offsets = NULL;

	for (i = 0; i < (int)CMD_SIZE; i++)
		if (commands[i].opcode == 0)
			section_count++;

	cmds = (char ***)calloc(sizeof(char **), section_count+1);
	offsets = (int *)calloc(sizeof(int), section_count);

	index = 0;
	i = 0;
	items = 0;
	while (index < section_count) {
		if (commands[i].opcode == 0) {
			cmds[index] = (char **)calloc(sizeof(char *), items + 1);
			index++;
			items = 0;
		} else {
			items++;
		}
		i++;
	}
	cmds[index] = (char **)calloc(sizeof(char *), section_count + 1);

	index = 0;
	i = 0;
	items = 0;
	while (index < section_count) {
		if (commands[i].opcode == 0) {
			cmds[index][items] = NULL;
			cmds[section_count][index] = commands[i].command_name;
			index++;
			if (index < section_count) offsets[index] = i + 1;
			items = 0;
		} else {
			cmds[index][items] = commands[i].command_name;
			items++;
		}
		i++;
	}

	while ((index = pick_from_list("Pick a command section:", cmds[section_count])) >= 0) {
		i = pick_from_list("Pick a command:", cmds[index]);
		if (i < 0) continue;
		i += offsets[index];
		switch (commands[i].opcode) {
			case B_ACCELERANT_MODE_COUNT: {
				accelerant_mode_count gmc = gah(B_ACCELERANT_MODE_COUNT, NULL);
				if (!gmc) {
					missing_feature(commands[i].command_name);
					break;
				}
				mode_count = gmc();
				printf("Device supports %ld modes\n", mode_count);
			} break;
			case B_GET_MODE_LIST: {
				get_mode_list gml = gah(B_GET_MODE_LIST, NULL);
				if (!gml) {
					missing_feature(commands[i].command_name);
					break;
				}
				if (mode_count == 0) {
					printf("Do B_ACCELERANT_MODE_COUNT first!\n");
					break;
				}
				if (mode_list) free(mode_list);
				mode_list = (display_mode *)calloc(sizeof(display_mode), mode_count);
				if (!mode_list) {
					printf("Couldn't allocate enough RAM for display_mode list.\n");
					break;
				}
				result = gml(mode_list);
				printf("Mode list retrieved with result %ld\n", result);
			} break;
			case T_PICK_A_MODE: {
				if (mode_list) {
					display_mode *adm = pick_a_mode(mode_list, mode_count);
					if (adm) {
						dm = *adm;
						valid_mode = TRUE;
					}
				} else printf("Do B_GET_MODE_LIST first!\n");
			} break;
			case T_TWEAK_MODE: {
				set_display_mode sdc = gah(B_SET_DISPLAY_MODE, NULL);
				if (!sdc) {
					missing_feature("B_SET_DISPLAY_MODE");
					break;
				}
				if (!valid_mode) {
					printf("Do T_PICK_A_MODE or B_PROPOSE_DISPLAY_MODE first!\n");
					break;
				}
				tweak_mode(sdc, &dm);
			} break;
			case B_SET_DISPLAY_MODE: {
				set_display_mode sdc = gah(B_SET_DISPLAY_MODE, NULL);
				if (!sdc) {
					missing_feature(commands[i].command_name);
					break;
				}
				if (!valid_mode) {
					printf("Do T_PICK_A_MODE or B_PROPOSE_DISPLAY_MODE first!\n");
					break;
				}
				result = sdc(&dm);
				printf("set_display_mode() completed with result %ld\n", result);
			} break;
			case B_GET_DISPLAY_MODE: {
				get_display_mode gdc = gah(B_GET_DISPLAY_MODE, NULL);
				if (!gdc) {
					missing_feature(commands[i].command_name);
					break;
				}
				result = gdc(&dm);
				if (result != B_OK) {
					printf("get_display_mode() failed: %ld (0x%08lx)\n", result, result);
					break;
				}
				printf("get_display_mode() suceeded!\n");
				valid_mode = TRUE;
				dump_mode(&dm);
			} break;
			case B_GET_FRAME_BUFFER_CONFIG: {
				frame_buffer_config fbc;
				get_frame_buffer_config gfbc = gah(B_GET_FRAME_BUFFER_CONFIG, NULL);
				if (!gfbc) {
					missing_feature(commands[i].command_name);
					break;
				}
				if (!valid_mode) {
					printf("Do T_PICK_A_MODE or B_PROPOSE_DISPLAY_MODE first!\n");
					break;
				}
				result = gfbc(&fbc);
				if (result != B_OK) {
					printf("get_frame_buffer_config() failed: %ld (0x%08lx)\n", result, result);
					break;
				}
				printf("get_frame_buffer_config() suceeded!\n");
				printf(" frame buffer: 0x%08lx\n", (uint32)fbc.frame_buffer);
				printf("   dma buffer: 0x%08lx\n", (uint32)fbc.frame_buffer_dma);
				printf("bytes per row: %ld\n\n", fbc.bytes_per_row);
				paint_for_blit(&dm, &fbc);
			} break;
			case B_GET_PIXEL_CLOCK_LIMITS: {
				get_pixel_clock_limits gpcl = gah(B_GET_PIXEL_CLOCK_LIMITS, NULL);
				uint32 low, high;
				if (!gpcl) {
					missing_feature(commands[i].command_name);
					break;
				}
				result = gpcl(&dm, &low, &high);
				if (result != B_OK) {
					printf("get_pixel_clock_limits() failed: %ld (0x%08lx)\n", result, result);
					break;
				}
				printf("Minimum pixel clock: %lu\nMaximum pixel clock: %lu\n", low, high);
			} break;
			case B_MOVE_DISPLAY: {
				move_display_area mda = gah(B_MOVE_DISPLAY, NULL);	/* should we be passing the display mode? */
				int top, left;
				if (mda) {
					printf("Enter new coordinates: "); fflush(stdout);
					scanf("%d,%d", &left, &top);
					/* no range checking for now */
					result = mda(left, top);
					printf("move_display_area() returned %ld (0x%08lx)\n", result, result);
				} else {
					missing_option(commands[i].command_name);
				}
			} break;
			case B_SET_INDEXED_COLORS: {
				set_indexed_colors sic = gah(B_SET_INDEXED_COLORS, NULL); /* should we be passing display mode */
				if (sic) {
					char *list[] = {"Red CLUT", "Green CLUT", "Blue CLUT", "Grey CLUT", "Be CLUT", NULL};
					int choice;
					choice = pick_from_list("Select CLUT:", list);
					if (choice >= 0) {
						uint8 color_data[256 * 3], *cd = color_data;
						int j, k;
						for (j = 0; j < 256; j++) {
							for (k = 0; k < 3; k++) {
								*cd = 0;
								if ((k == choice) || (choice == 3)) *cd = (uint8)j;
								cd++;
							}
						}
						sic(256, 0, color_data, 0);
					}
				} else {
					missing_option(commands[i].command_name);
				}
			} break;
			case B_ACCELERANT_RETRACE_SEMAPHORE: {
				accelerant_retrace_semaphore ars = gah(B_ACCELERANT_RETRACE_SEMAPHORE, NULL);
				sem_id sid;
				status_t result;
				if (!ars) {
					missing_feature(commands[i].command_name);
					break;
				}
				sid = ars();
				if (sid < B_OK) {
					printf("Bad semaphore ID returned: %ld\n", sid);
					break;
				}
				if (!valid_mode) {
					printf("No video mode selected, skipping retrace check.\n");
				}
				result = acquire_sem_etc(sid, 1, B_TIMEOUT, 1000000L);
				if (result != B_OK) {
					printf("Semaphore acquisition failed: %ld (0x%08lx)\n", result, result);
					break;
				}
				printf("Retrace semaphore acquired!\n");
			} break;
			case B_SET_DPMS_MODE: {
				set_dpms_mode sdm = gah(B_SET_DPMS_MODE, NULL);
				if (sdm) {
					int choice;
					choice = pick_from_list("Select DPMS mode:", dpms_list);
					if (choice >= 0) {
						result = sdm(dpms_flags[choice]);
						if (result != B_OK) failed_with_reason("B_SET_DPMS_MODE", result);
						else printf("DPMS mode %s set!\n", dpms_list[choice]);
					}
				} else printf("Device does not support DPMS.\n");
			} break;
			case B_SET_CURSOR_SHAPE: {
				if (dm.flags & B_HARDWARE_CURSOR) {
					set_cursor_shape scs = gah(B_SET_CURSOR_SHAPE, NULL);
					if (!scs) {
						missing_option(commands[i].command_name);
						break;
					}
					{
					int i;
					uint8 hand_and[32];
					for (i = 0; i < 32; i++) hand_and[i] = ~my_hand_cursor_and[i];
					result = scs(16, 16, 2, 2, hand_and, my_hand_cursor_xor);
					}
					if (result != B_OK)
						failed_with_reason("B_SET_CURSOR_SHAPE", result);
				} else printf("Mode does not support hardware cursor!\n");
			} break;
			case B_MOVE_CURSOR: {
				if (dm.flags & B_HARDWARE_CURSOR) {
					move_cursor mc = gah(B_MOVE_CURSOR, NULL);
					if (!mc) {
						missing_option(commands[i].command_name);
						break;
					}
					mc((uint16)(dm.timing.h_display >> 1), (uint16)(dm.timing.v_display >> 1));
				} else printf("Mode does not support hardware cursor!\n");
			} break;
			case B_SHOW_CURSOR: {
				if (dm.flags & B_HARDWARE_CURSOR) {
					show_cursor sc = gah(B_SHOW_CURSOR, NULL);
					char *list[] = {"Hide Cursor", "Show Cursor", NULL};
					int choice;
					if (!sc) {
						missing_option(commands[i].command_name);
						break;
					}
					choice = pick_from_list("Set cursor visibility:", list);
					if (choice >= 0) sc(choice == 1);
				} else printf("Mode does not support hardware cursor!\n");
			} break;
			case B_ACQUIRE_ENGINE: {
				acquire_engine ae = gah(B_ACQUIRE_ENGINE, NULL);
				if (!ae) {
					missing_feature(commands[i].command_name);
					break;
				}
				result = ae(0,0, NULL, &et);
				if (result != B_OK) {
					failed_with_reason("acquire_engine()", result);
					et = NULL;
					break;
				}
				engine_acquired = TRUE;
				printf("acceleration engine acquired: 0x%08lx\n", (uint32)et);
			} break;
			case B_RELEASE_ENGINE: {
				release_engine re = gah(B_RELEASE_ENGINE, NULL);
				if (!re) {
					missing_feature(commands[i].command_name);
					break;
				}
				if (!engine_acquired) {
					printf("Do B_ACQUIRE_ENGINE first!\n");
					break;
				}
				result = re(et, NULL);
				if (result != B_OK) {
					failed_with_reason("release_engine()", result);
					break;
				}
				engine_acquired = FALSE;
				printf("acceleration engine released.\n");
			} break;
			case B_WAIT_ENGINE_IDLE: {
				wait_engine_idle wei = gah(B_WAIT_ENGINE_IDLE, NULL);
				if (!wei) {
					missing_feature(commands[i].command_name);
					break;
				}
				if (!engine_acquired) {
					printf("Do B_ACQUIRE_ENGINE first!\n");
					break;
				}
				wei();
				printf("Acceleration engine idle.\n");
			} break;
#if 1
			case B_SCREEN_TO_SCREEN_BLIT: {
				frame_buffer_config fbc;
				get_frame_buffer_config gfbc = gah(B_GET_FRAME_BUFFER_CONFIG, NULL);
				screen_to_screen_blit ssb = gah(B_SCREEN_TO_SCREEN_BLIT, &dm);
				if (!ssb) {
					missing_option(commands[i].command_name);
					break;
				}
				printf("SCREEN_TO_SCREEN_BLIT is %p\n", ssb);
				if (!gfbc) {
					missing_feature(commands[i].command_name);
					break;
				}
				if (!valid_mode) {
					printf("Do T_PICK_A_MODE or B_PROPOSE_DISPLAY_MODE first!\n");
					break;
				}
				if (!engine_acquired) {
					printf("Do B_ACQUIRE_ENGINE first!\n");
					break;
				}
				result = gfbc(&fbc);
				if (result != B_OK) {
					failed_with_reason( "get_frame_buffer_config" , result);
					break;
				}
				// paint the frame buffer with a pattern we can blit
				//paint_for_blit(&dm, &fbc);
				{
				blit_params bp[16];
				int j = 0;
				bp[j].src_left = 0;
				bp[j].src_top = 0;
				bp[j].width = 41;
				bp[j].height = 41;
				bp[j].dest_top = ((dm.timing.v_display - 1) >> (j+2)) * 3;
				bp[j].dest_left = (dm.timing.v_display - 1) >> (j+2);
				ssb(et, bp, 1);
				}
			} break;
#endif
#if NEW_BLIT_HOOKS
			case B_SCREEN_TO_SCREEN_TRANSPARENT_BLIT: {
				frame_buffer_config fbc;
				get_frame_buffer_config gfbc = gah(B_GET_FRAME_BUFFER_CONFIG, NULL);
				screen_to_screen_transparent_blit sstb = gah(B_SCREEN_TO_SCREEN_TRANSPARENT_BLIT, &dm);
				if (!sstb) {
					missing_option(commands[i].command_name);
					break;
				}
				if (!gfbc) {
					missing_feature(commands[i].command_name);
					break;
				}
				if (!valid_mode) {
					printf("Do T_PICK_A_MODE or B_PROPOSE_DISPLAY_MODE first!\n");
					break;
				}
				if (!engine_acquired) {
					printf("Do B_ACQUIRE_ENGINE first!\n");
					break;
				}
				result = gfbc(&fbc);
				if (result != B_OK) {
					failed_with_reason( "get_frame_buffer_config" , result);
					break;
				}
				// paint the frame buffer with a pattern we can blit
				paint_for_blit(&dm, &fbc);
				{
				blit_params bp[16];
				int j = 0;
				bp[j].src_left = 0;
				bp[j].src_top = 0;
				bp[j].width = 41;
				bp[j].height = 41;
				bp[j].dest_top = ((dm.timing.v_display - 1) >> (j+2)) * 3;
				bp[j].dest_left = (dm.timing.v_display - 1) >> (j+2);
				sstb(et, 0, bp, 1);
				}
			} break;
			case B_SCREEN_TO_SCREEN_SCALED_FILTERED_BLIT: {
				frame_buffer_config fbc;
				get_frame_buffer_config gfbc = gah(B_GET_FRAME_BUFFER_CONFIG, NULL);
				screen_to_screen_scaled_filtered_blit sssfb = gah(B_SCREEN_TO_SCREEN_SCALED_FILTERED_BLIT, &dm);
				if (!sssfb) {
					missing_option(commands[i].command_name);
					break;
				}
				if (!gfbc) {
					missing_feature(commands[i].command_name);
					break;
				}
				if (!valid_mode) {
					printf("Do T_PICK_A_MODE or B_PROPOSE_DISPLAY_MODE first!\n");
					break;
				}
				if (!engine_acquired) {
					printf("Do B_ACQUIRE_ENGINE first!\n");
					break;
				}
				result = gfbc(&fbc);
				if (result != B_OK) {
					failed_with_reason( "get_frame_buffer_config" , result);
					break;
				}
				// paint the frame buffer with a pattern we can blit
				paint_for_blit(&dm, &fbc);
				{
				scaled_blit_params sbp[16];
				int j = 0;
#if 1
				sbp[j].src_left = 0;
				sbp[j].src_top = 0;
				sbp[j].src_width = 41;
				sbp[j].src_height = 41;

				sbp[j].dest_top = ((dm.timing.v_display - 1) >> 1) + 10;
				sbp[j].dest_left = 10;
				sbp[j].dest_width = sbp[j].src_width;
				sbp[j].dest_height = sbp[j].src_height;
				j++;
#endif

#if 1
				sbp[j].src_left = 0;
				sbp[j].src_top = 0;
				sbp[j].src_width = 41;
				sbp[j].src_height = 41;

				sbp[j].dest_top = ((dm.timing.v_display - 1) >> 1) + 10;
				sbp[j].dest_left = 200;

#if 1
				sbp[j].dest_width = ((sbp[j].src_width+1) * 6) - 1;
				sbp[j].dest_height = ((sbp[j].src_height+1) * 6) - 1;
#else
				sbp[j].dest_width = sbp[j].src_width;
				sbp[j].dest_height = sbp[j].src_height;
#endif
				j++;
#endif

#if 1
				sbp[j].src_left = 0;
				sbp[j].src_top = 0;
				sbp[j].src_width = 41;
				sbp[j].src_height = 41;

				sbp[j].dest_top =  10;
				sbp[j].dest_left = ((dm.timing.h_display - 1) >> 1) + 10;
				sbp[j].dest_width = sbp[j].src_width;
				sbp[j].dest_height = sbp[j].src_height;
				j++;
#endif

#if 1
				sbp[j].src_left = 1;
				sbp[j].src_top = 1;
				sbp[j].src_width = 41;
				sbp[j].src_height = 41;

				sbp[j].dest_top =  10;
				sbp[j].dest_left = ((dm.timing.h_display - 1) >> 1) + 70;

#if 1
				sbp[j].dest_width = ((sbp[j].src_width+1) * 6) - 1;
				sbp[j].dest_height = ((sbp[j].src_height+1) * 6) - 1;
#else
				sbp[j].dest_width = sbp[j].src_width;
				sbp[j].dest_height = sbp[j].src_height;
#endif
				j++;
#endif
				sssfb(et, sbp, j);
				}
			} break;
#endif
			case B_FILL_RECTANGLE: {
				fill_rectangle rf = gah(B_FILL_RECTANGLE, &dm);
				fill_rect_params rfp[16];
				bigtime_t start, stop;
				int j;
				if (!rf) {
					missing_option(commands[i].command_name);
					break;
				}
				if (!engine_acquired) {
					printf("Do B_ACQUIRE_ENGINE first!\n");
					break;
				}
				rfp[0].left = rfp[0].top = 0;
				rfp[0].right = dm.timing.h_display - 1;
				rfp[0].bottom = dm.timing.v_display - 1;
				start = system_time();
				rf(et, 0, rfp, 1);
#if 0
				rf(et, 0, rfp, 1);
				rf(et, 0, rfp, 1);
				rf(et, 0, rfp, 1);
				rf(et, 0, rfp, 1);
				rf(et, 0, rfp, 1);
				rf(et, 0, rfp, 1);
				rf(et, 0, rfp, 1);
				rf(et, 0, rfp, 1);
				rf(et, 0, rfp, 1);
#endif
				stop = system_time();
				printf("area fill one rect: %Ld\n", stop-start);
#if 1
				for (j = 0; ((dm.timing.h_display-1) >> (j+1)) && ((dm.timing.v_display-1) >> (j+1)); j++) {
					rfp[j].right = (dm.timing.h_display - 1) >> j;
					rfp[j].left = (dm.timing.h_display - 1) >> (j+1);
					rfp[j].bottom = (dm.timing.v_display - 1) >> j;
					rfp[j].top = (dm.timing.v_display - 1) >> (j+1);
					printf("   rfp[%d] = %d,%d to %d,%d\n", j, rfp[j].left, rfp[j].top, rfp[j].right, rfp[j].bottom);
				}
				start = system_time();
				rf(et, 0xffffffff, rfp, (uint32)j);
#if 0
				rf(et, 0xffffffff, rfp, (uint32)j);
				rf(et, 0xffffffff, rfp, (uint32)j);
#endif
				stop = system_time();
				printf("area fill %d rects: %Ld (stop: %Ld, start: %Ld)\n", j, stop-start, stop, start);
				//rf(et, 0xffffffff, rfp, (uint32)2);
#endif
			} break;
			case B_INVERT_RECTANGLE: {
				uint16 left, top, right, bottom, h_delta, v_delta;
				fill_rect_params rfp[16];
				int j;
				invert_rectangle ri = gah(B_INVERT_RECTANGLE, &dm);
				if (!ri) {
					missing_option(commands[i].command_name);
					break;
				}
				if (!engine_acquired) {
					printf("Do B_ACQUIRE_ENGINE first!\n");
					break;
				}
				left = 0; right = dm.timing.h_display - 1;
				top = 0; bottom = dm.timing.v_display - 1;
				h_delta = dm.timing.h_display >> 4;
				v_delta = dm.timing.v_display >> 4;
				for (j = 0; j < 8; j++) {
					rfp[j].left = left;
					rfp[j].top = top;
					rfp[j].right = right;
					rfp[j].bottom = bottom;
					left += h_delta;
					right -= h_delta;
					top += v_delta;
					bottom -= v_delta;
				}
				ri(et, rfp, 8);
			} break;
			case B_FILL_SPAN: {
				fill_span sf = gah(B_FILL_SPAN, &dm);
				uint16 *spans, *sp;
				int j, k, w;
				if (!sf) {
					missing_option(commands[i].command_name);
					break;
				}
				if (!engine_acquired) {
					printf("Do B_ACQUIRE_ENGINE first!\n");
					break;
				}
				sp = spans = (uint16*)calloc(dm.timing.v_display * 3, sizeof(uint16));
				if (!spans) {
					printf("Couldn't allocate RAM for span list :-(\n");
					break;
				}
				for (j = dm.timing.v_display, k = 0, w = dm.timing.h_total >> 1; j > 0; j--) {
					*sp++ = j-1;
					*sp++ = k;
					*sp++ = k + w;
					if (j > dm.timing.v_display / 2) k++;
					else k--;
					if (j & 1) w--;
				}
				sf(et, gColor, spans, dm.timing.v_display);
				free(spans);
			} break;
			case T_SET_FILL_COLOR: {
				printf("Current fill color: 0x%08lx.  Enter new color:\n", gColor); fflush(stdout);
				scanf("%lx", &gColor);
				printf("New color: 0x%08lx\n", gColor);
			} break;
			case B_OVERLAY_COUNT: {
				overlay_count oc = gah(B_OVERLAY_COUNT, 0);
				if (!oc) {
					missing_option(commands[i].command_name);
					break;
				}
				printf("This device supports %ld overlays with the current display mode.\n", oc(&dm));
			} break;
			case B_OVERLAY_SUPPORTED_SPACES: {
				overlay_supported_spaces oss = gah(B_OVERLAY_SUPPORTED_SPACES, 0);
				const uint32 *spaces;
				if (!oss) {
					missing_option(commands[i].command_name);
					break;
				}
				spaces = oss(&dm);
				if (!spaces) {
					failed_string( "B_OVERLAY_SUPPORTED_SPACES" , "null pointer return");
					break;
				}
				printf("Supported overlay spaces:");
				while (*spaces != B_NO_COLOR_SPACE) {
					printf(" %s", spaceToString(*spaces));
					spaces++;
				}
				printf("\n");
			} break;
			
			case B_OVERLAY_SUPPORTED_FEATURES: {
				overlay_supported_features osf = gah(B_OVERLAY_SUPPORTED_FEATURES, 0);
				if (!osf) {
					missing_option(commands[i].command_name);
					break;
				}
				dump_overlay_features("Supported overlay features", osf(ob->space));
			} break;

			case B_ALLOCATE_OVERLAY_BUFFER: {
				char *list[] = {"NTSC color bars (160x90)", "NTSC color bars (320x180)", "Color wash (256x256)", "From VIP", NULL};
				int choice;
				allocate_overlay_buffer aob = gah(B_ALLOCATE_OVERLAY_BUFFER, 0);
				if (!aob) {
					missing_option(commands[i].command_name);
					break;
				}
				choice = pick_from_list("Select overlay contents:", list);
				if ((choice >= 0) && (choice < 2)) {
					int width = 160 << choice;
					int height = 90 << choice;
					int j,k;
					uint8 *p;

					ob = aob(B_YCbCr422, width, height);
					if (!ob) {
						failed_string( "B_ALLOCATE_OVERLAY_BUFFER" , "null pointer return");
						break;
					}
					features &= ~B_OVERLAY_FROM_VIP;
					dump_overlay_buffer(ob);
					/* fill the overlay with colorbars */
					p = ob->buffer;
					for (j = 0; j < ob->height; j++) {
						// white
						for (k = 0; k < ob->width / 16; k++) {
							*p++ = 235; // Y
							*p++ = 128; // Cb
							*p++ = 235; // Y
							*p++ = 128; // Cr
						}
						// yellow
						for (k = 0; k < ob->width / 16; k++) {
							*p++ = 210; // Y
							*p++ = 16; // Cb
							*p++ = 210; // Y
							*p++ = 146; // Cr
						}
						// cyan
						for (k = 0; k < ob->width / 16; k++) {
							*p++ = 170; // Y
							*p++ = 166; // Cb
							*p++ = 170; // Y
							*p++ = 16; // Cr
						}
						// green
						for (k = 0; k < ob->width / 16; k++) {
							*p++ = 145; // Y
							*p++ = 54; // Cb
							*p++ = 145; // Y
							*p++ = 34; // Cr
						}
						// magenta
						for (k = 0; k < ob->width / 16; k++) {
							*p++ = 106; // Y
							*p++ = 202; // Cb
							*p++ = 106; // Y
							*p++ = 222; // Cr
						}
						// red
						for (k = 0; k < ob->width / 16; k++) {
							*p++ = 81; // Y
							*p++ = 90; // Cb
							*p++ = 81; // Y
							*p++ = 240; // Cr
						}
						// blue
						for (k = 0; k < ob->width / 16; k++) {
							*p++ = 41; // Y
							*p++ = 240; // Cb
							*p++ = 41; // Y
							*p++ = 110; // Cr
						}
						// black
						for (k = 0; k < ob->width / 16; k++) {
							*p++ = 16; // Y
							*p++ = 128; // Cb
							*p++ = 16; // Y
							*p++ = 128; // Cr
						}
						// fill in any extra bytes
					}
				}
				else if (choice == 2)
				{
					int red,green,blue;
					uint8 *p;

					ob = aob(B_YCbCr422, 256, 256);
					if (!ob) {
						failed_string( "B_ALLOCATE_OVERLAY_BUFFER" , "null pointer return");
						break;
					}
					features &= ~B_OVERLAY_FROM_VIP;
					dump_overlay_buffer(ob);
					/* fill the overlay with colorbars */
					p = ob->buffer;
					for (red = 0, blue = 255; red < 256; red++, blue--) {
						// 
						for (green = 0; green < 256; green += 2) {
							*p++ = (uint8)(( 0.257 * red) + (0.504 * green) + (0.098 * blue) + 16); // Y
							*p++ = (uint8)((-0.148 * red) - (0.291 * green) + (0.439 * blue) + 128); // Cb
							*p++ = (uint8)(( 0.257 * red) + (0.504 * (green + 1)) + (0.098 * blue) + 16); // Y
							*p++ = (uint8)(( 0.439 * red) - (0.368 * green) - (0.071 * blue) + 128); // Cr
						}
					}
				}
				else
				{
					/* B_OVERLAY_FROM_VIP */
					ob = aob(B_YUV422, 720, 484);
					if (!ob) {
						failed_string( "B_ALLOCATE_OVERLAY_BUFFER" , "null pointer return");
						break;
					}
					features |= B_OVERLAY_FROM_VIP;
					dump_overlay_buffer(ob);
				}
				{
				uint8 *p = ob->buffer;
				uint32 i;
				printf("buffer %p first line contents:\n", p);
				for (i = 0; i < ob->bytes_per_row; i++, p++)
					printf("%.2x ", *p);
				printf("\n");
				}
			} break;
			case B_RELEASE_OVERLAY_BUFFER: {
				release_overlay_buffer rob = gah(B_RELEASE_OVERLAY_BUFFER, 0);
				if (!rob) {
					missing_option(commands[i].command_name);
					break;
				}
				result = rob(ob);
				if (result != B_OK) {
					failed_with_reason( "B_ALLOCATE_OVERLAY_BUFFER" , result);
					break;
				}
				ob = 0;
				printf("overlay_buffer released\n");
			} break;
			case B_GET_OVERLAY_CONSTRAINTS: {
				get_overlay_constraints goc = gah(B_GET_OVERLAY_CONSTRAINTS, 0);
				overlay_constraints oc;
				if (!goc) {
					missing_option(commands[i].command_name);
					break;
				}
				result = goc(&dm, ob, &oc);
				if (result != B_OK) {
					failed_with_reason( "B_GET_OVERLAY_CONSTRAINTS" , result);
					break;
				}
				dump_overlay_constraints(&oc);
			} break;
			case B_ALLOCATE_OVERLAY: {
				allocate_overlay ao = gah(B_ALLOCATE_OVERLAY, 0);
				if (!ao) {
					missing_option(commands[i].command_name);
					break;
				}
				ot = ao();
				if (result != B_OK) {
					failed_string( "B_ALLOCATE_OVERLAY" , "null pointer return");
					break;
				}
				printf("overlay_token %p\n", ot);
			} break;
			case B_RELEASE_OVERLAY: {
				release_overlay ro = gah(B_RELEASE_OVERLAY, 0);
				if (!ro) {
					missing_option(commands[i].command_name);
					break;
				}
				result = ro(ot);
				if (result != B_OK) {
					failed_with_reason( "B_RELEASE_OVERLAY" , result);
					break;
				}
				ot = 0;
				printf("overlay_token released\n");
			} break;
			case B_CONFIGURE_OVERLAY: {
				configure_overlay co = gah(B_CONFIGURE_OVERLAY, 0);
				int bpp = space_bits_per_pixel(dm.space);

				if (!co) {
					missing_option(commands[i].command_name);
					break;
				}
				ow.width = ob->width * scaling_factor; // + (ob->width / 2);
				ow.height = ob->height * scaling_factor; // + (ob->height / 2);
				switch (overlay_h_pos) {
					case -2: ow.h_start = - (ow.width / 2);
					break;
					case -1: ow.h_start = 0;
					break;
					case  0: ow.h_start = (dm.timing.h_display - ow.width) / 2;
					break;
					case  1: ow.h_start = (dm.timing.h_display - ow.width);
					break;
					case  2: ow.h_start = dm.timing.h_display - (ow.width / 2);
					break;
				}
				switch (overlay_v_pos) {
					case -2: ow.v_start = - (ow.height / 2);
					break;
					case -1: ow.v_start = 0;
					break;
					case  0: ow.v_start = (dm.timing.v_display - ow.height) / 2;
					break;
					case  1: ow.v_start = (dm.timing.v_display - ow.height);
					break;
					case  2: ow.v_start = dm.timing.v_display - (ow.height / 2);
					break;
				}
				ow.flags = features;
				if (bpp == 8) {
					ow.red.value = ow.red.mask = 0x01;
					ow.green.value = ow.green.mask = 0x02;
					ow.blue.value = ow.blue.mask = 0x04;
				} else if (bpp == 15) {
					ow.red.value = ow.red.mask = 0x1f;
					ow.green.value = ow.green.mask = 0x1f;
					ow.blue.value = ow.blue.mask = 0x1f;
				} else if (bpp == 16) {
					ow.red.value = ow.red.mask = 0x1f;
					ow.green.value = ow.green.mask = 0x3f;
					ow.blue.value = ow.blue.mask = 0x1f;
				} else if (bpp == 32) {
					ow.red.value = ow.red.mask = 0xff;
					ow.green.value = ow.green.mask = 0xff;
					ow.blue.value = ow.blue.mask = 0xff;
				}
				// TEST THIS BETTER
				ow.offset_top = offset_top;
				ow.offset_left = offset_left;
				ow.offset_bottom = offset_bottom;
				ow.offset_right = offset_right;
				
				ov.width = ob->width;// * 0.75;
				ov.height = ob->height; // * 0.75;
				ov.h_start = (ob->width - ov.width) / 2;// - 3;
				ov.v_start = (ob->height - ov.height) / 2;// - 1;
				result = co(ot, ob, &ow, &ov);
				if (result != B_OK) {
					failed_with_reason( "B_CONFIGURE_OVERLAY" , result);
					break;
				}
				printf("overlay configured\n");
			} break;
			case T_SET_OVERLAY_SCALING_FACTOR: {
				printf("Enter new scaling_factor: "); fflush(stdout);
				scanf("%f", &scaling_factor);
				printf("scaling_factor: %f\n", scaling_factor);
			} break;
			case T_SET_OVERLAY_H_POSITION: {
				char *list[] = {"Off the left", "Left aligned", "Centered", "Right aligned", "Off the right", NULL};
				int choice;
				choice = pick_from_list("Select overlay horizontal position:", list);
				if (choice >= 0) overlay_h_pos = choice - 2;
				else overlay_h_pos = 0;
			} break;
			case T_SET_OVERLAY_V_POSITION: {
				char *list[] = {"Off the top", "Top aligned", "Centered", "Bottom aligned", "Off the bottom", NULL};
				int choice;
				choice = pick_from_list("Select overlay vertical position:", list);
				if (choice >= 0) overlay_v_pos = choice - 2;
				else overlay_v_pos = 0;
			} break;
			case T_SET_OVERLAY_FEATURES: {
				int choice;
				dump_overlay_features("Current features", features);
				choice = pick_from_list("Select overlay feature to set:", feature_list);
				if (choice >= 0) {
					features |= feature_flags[choice];
					dump_overlay_features("New features", features);
				} else {
					printf("Feautures unchanged.\n");
				}
			} break;
			case T_CLEAR_OVERLAY_FEATURES: {
				int choice;
				dump_overlay_features("Current features", features);
				choice = pick_from_list("Select overlay feature to clear:", feature_list);
				if (choice >= 0) {
					features &= ~feature_flags[choice];
					dump_overlay_features("New features", features);
				} else {
					printf("Feautures unchanged.\n");
				}
			} break;
			case T_SET_OVERLAY_OFFSETS: {
				printf("Enter new offsets (left,top right,bottom): "); fflush(stdout);
				scanf("%lu,%lu %lu,%lu", &offset_left, &offset_top, &offset_right, &offset_bottom);
				printf("New offsets: %lu,%lu %lu,%lu\n", offset_left, offset_top, offset_right, offset_bottom);
			} break;
			case B_GET_TV_OUT_ADJUSTMENTS_FOR_MODE: {
				get_tv_out_adjustments_for_mode toafm = gah(B_GET_TV_OUT_ADJUSTMENTS_FOR_MODE, 0);
				if (!toafm)
				{
					missing_option(commands[i].command_name);
					break;
				}
				get_and_dump_tv_out_adjustments_for_mode(toafm, &dm);
			} break;
			case T_TWEAK_TV_OUT_ADJUSTMENTS: {
				set_tv_out_adjustments stoa = gah(B_SET_TV_OUT_ADJUSTMENTS, 0);
				get_tv_out_adjustments gtoa = gah(B_GET_TV_OUT_ADJUSTMENTS, 0);
				if (!stoa || !gtoa) {
					printf("Missing B_SET_TV_OUT_ADJUSTMENTS or B_GET_TV_OUT_ADJUSTMENTS\n");
					break;
				}
				tweak_tv_out(stoa, gtoa);
			}
			default: {
				printf("Ooops: %s not implemented yet.\n", commands[i].command_name);
			} break;
		}
	}
	/* clean up */
	if (cmds) free(cmds);
	if (mode_list) free(mode_list);
}

#define T_GRAPHICS_DEVICE_DIR "/dev/graphics"
int main(int argc, char **argv) {
	int fd;
	GetAccelerantHook gah;
	image_id image;

	(void)argc;(void)argv;

	/* pick a device */
	fd = pick_device(T_GRAPHICS_DEVICE_DIR);
	if (fd < 0) {
		fprintf(stderr, "Can't open device: %s (%s)\n", strerror(fd), strerror(errno));
		return fd;
	}
	/* load the accelerant */
	image = load_accelerant(fd, &gah);
	if (image < 0) goto close_driver;

	exercise_driver(gah);

	/* uninit accelerant */
	{
	uninit_accelerant ua = gah(B_UNINIT_ACCELERANT, NULL);
	if (ua) ua();
	}
	fprintf(stderr, "Unloading add-on...\n");
	/* unload add-on */
	unload_add_on(image);
	fprintf(stderr, "Add-on unloaded.\n");
close_driver:
	/* close the driver */
	close(fd);
	fprintf(stderr, "Device handle closed\n");
	return B_OK;
}
