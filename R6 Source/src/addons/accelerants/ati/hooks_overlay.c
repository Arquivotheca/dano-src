#include <OS.h>
#include <unistd.h>

#include "hooks_overlay.h"
#include "private.h"

#if 0
#include <stdio.h>
#define xdprintf(a) printf a

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

#else
#define xdprintf(a)
#endif

static uint32 overlay_spaces[] = { B_YCbCr422, B_RGB32_LITTLE, B_NO_COLOR_SPACE };
static uint16      pitch_max[] = { 4092,       4092,           0 };

uint32 
OVERLAY_COUNT(const display_mode *dm)
{
	return 1;
}

const uint32 *
OVERLAY_SUPPORTED_SPACES(const display_mode *dm)
{
	return overlay_spaces;
}

uint32
OVERLAY_SUPPORTED_FEATURES(uint32 a_color_space)
{
	uint32 result;

	switch (a_color_space) {
		case B_YCbCr422:
		case B_RGB32_LITTLE:
			result =
				B_OVERLAY_COLOR_KEY |
				B_OVERLAY_CHROMA_KEY |
				B_OVERLAY_HORIZONTAL_FITLERING |
				B_OVERLAY_VERTICAL_FILTERING;
			break;
		default:
			result = 0;
			break;
	}
	return result;
}

#define MAX_BUFFERS (sizeof(ai->ovl_buffers) / sizeof(ai->ovl_buffers[0]))

const overlay_buffer *
ALLOCATE_OVERLAY_BUFFER(color_space cs, uint16 width, uint16 height)
{
	int i, index;
	uint32 space;
	uint32 bytes_per_pixel = 0;
	uint32 pitch_mask = 0;
	BMemSpec *ms;
	overlay_buffer *ob;

	xdprintf(("ALLOCATE_OVERLAY_BUFFER(%s, %d, %d)\n", spaceToString(cs), width, height));

	if (!width || !height) return 0;
	if (cs == B_NO_COLOR_SPACE) return 0;

	/* find an empty overlay buffer */
	for (index = 0; index < MAX_BUFFERS; index++)
		if (ai->ovl_buffers[index].space == B_NO_COLOR_SPACE)
			break;

	xdprintf(("%d == %ld : %s\n", index, MAX_BUFFERS, index == MAX_BUFFERS ? "true" : "false"));

	/* all in use - then fail */
	if (index == MAX_BUFFERS) return 0;
	ob = &(ai->ovl_buffers[index]);

	/* otherwise try and make a buffer */

	/* validate color_space */
	i = 0;
	while (overlay_spaces[i]) {
		if (overlay_spaces[i] == cs) break;
		i++;
	}
	
	xdprintf(("overlay_spaces[%d] == B_NO_COLOR_SPACE: %s\n", i, overlay_spaces[i] == B_NO_COLOR_SPACE ? "true" : "false"));
	xdprintf(("%d > %d : %s\n", width, pitch_max[i], width > pitch_max[i] ? "true" : "false"));

	if (overlay_spaces[i] == B_NO_COLOR_SPACE) return 0;
	if (width > pitch_max[i]) return 0;

	space = overlay_spaces[i];

	// calculate buffer size
	switch (space) {
	case B_YCbCr422:
		bytes_per_pixel = 2;
		pitch_mask = 3;	/* bits which must be clear */
		break;
	case B_RGB32_LITTLE:
		bytes_per_pixel = 4;
		pitch_mask = 1;	/* bits which must be clear */
		break;
	}
	/* make width be a proper multiple for the back-end scaler */
	width = (width + pitch_mask) & ~pitch_mask;
	ob->bytes_per_row = bytes_per_pixel * width;
	ms = &(ai->ovl_buffer_specs[index]);
	ms->ms_MR.mr_Size = ob->bytes_per_row * height;
	ms->ms_PoolID = ai->poolid;
	ms->ms_AddrCareBits = 7;
	ms->ms_AddrStateBits = 0;
	xdprintf(("PoolID: %ld\n", ai->poolid));
	if (ioctl(memtypefd, B_IOCTL_ALLOCBYMEMSPEC, ms, sizeof(*ms)) < 0) {
		xdprintf(("ioctl(B_IOCTL_ALLOCBYMEMSPEC) for %ld bytes failed\n", ms->ms_Size));
		return 0;
	}

	ob->space = space;
	ob->width = width;
	ob->height = height;
	ob->buffer = (void *)((uint8 *) si->framebuffer + ms->ms_MR.mr_Offset);
	ob->buffer_dma = (void *)((uint8 *)si->framebuffer_pci + ms->ms_MR.mr_Offset);
	xdprintf(("ALLOCATE_OVERLAY_BUFFER: returning overlay_buffer %p\n", ob));
	return ob;
}

status_t 
RELEASE_OVERLAY_BUFFER(const overlay_buffer *_ob)
{
	BMemSpec *ms;
	overlay_buffer *ob = (overlay_buffer *)_ob;
	int index = ob - ai->ovl_buffers;

	/* validated buffer */
	if ((index < 0) || (index >= MAX_BUFFERS)) {
		xdprintf(("RELEASE_OVERLAY_BUFFER: bad buffer pointer\n"));
		return B_ERROR;
	}
	if (ob->space == B_NO_COLOR_SPACE) {
		xdprintf(("RELEASE_OVERLAY_BUFFER: releasing unallocated buffer\n"));
		return B_ERROR;
	}

	ms = &(ai->ovl_buffer_specs[index]);
	/* free the memory */
	ioctl(memtypefd, B_IOCTL_FREEBYMEMSPEC, ms, sizeof(*ms));
	ms->ms_MR.mr_Size = 0;
	/* mark as empty */
	ob->space = B_NO_COLOR_SPACE;
	xdprintf(("RELEASE_OVERLAY_BUFFER: OK\n"));
	return B_OK;
}

status_t
GET_OVERLAY_CONSTRAINTS(const display_mode *dm, const overlay_buffer *ob, overlay_constraints *oc)
{
	/* minimum size of view */
	oc->view.width.min = 4;
	oc->view.height.min = 1;
	/* view may not be larger than the buffer or 768x1024, which ever is lower */
	oc->view.width.max = ob->width > 768 ? 768 : ob->width;
	oc->view.height.max = ob->height > 1024 ? 1024 : ob->height;
	/* view alignment */
	oc->view.h_alignment = 3;
	oc->view.v_alignment = 0;
	oc->view.width_alignment = 3;
	oc->view.height_alignment = 0;
	
	/* minium size of window */
	oc->window.width.min = 2;
	oc->window.height.min = 2;
	/* upper usefull size is limited by screen realestate */
	oc->window.width.max = 4096;
	oc->window.height.max = 4096;
	/* window alignment */
	oc->window.h_alignment = 0;
	oc->window.v_alignment = 0;
	oc->window.width_alignment = 3;
	oc->window.height_alignment = 0;
	
	/* overall scaling factors */
	oc->h_scale.min = 1.0 / 16.0;
	oc->v_scale.min = 1.0 / 16.0;
	oc->h_scale.max = 50.0;	/* a lie, but a convienient one */
	oc->v_scale.max = 50.0; /* being accurate requires knowing either the window or the view size exactly */

	return B_OK;
}

#define MAX_OVERLAYS (sizeof(ai->ovl_tokens) / sizeof(ai->ovl_tokens[0]))

overlay_token 
ALLOCATE_OVERLAY(void)
{
	int index;
	/* find an unused overlay token */
	for (index = 0; index < MAX_OVERLAYS; index++)
		if (ai->ovl_tokens[index].used == 0) break;
	if (index >= MAX_OVERLAYS) {
		xdprintf(("ALLOCATE_OVERLAY: no more overlays\n"));
		return 0;
	}

	/* mark it in use */
	ai->ovl_tokens[index].used++;
	xdprintf(("ALLOCATE_OVERLAY: returning %p\n", &(ai->ovl_tokens[index])));
	return (overlay_token)&(ai->ovl_tokens[index]);
}

status_t 
RELEASE_OVERLAY(overlay_token ot)
{
	status_t result;
	ati_overlay_token *aot = (ati_overlay_token *)ot;
	int index = aot - &(ai->ovl_tokens[0]);

	if ((index < 0) || (index >= MAX_OVERLAYS)) {
		xdprintf(("RELEASE_OVERLAY: bad overlay_token\n"));
		return B_ERROR;
	}
	if (!ai->ovl_tokens[index].used) {
		xdprintf(("RELEASE_OVERLAY: releasing unused overlay\n"));
		return B_ERROR;
	}
	/* de-configure overlay */
	result = CONFIGURE_OVERLAY(ot, NULL, NULL, NULL);
	/* mark unused */
	ai->ovl_tokens[index].used = 0;
	xdprintf(("RELEASE_OVERLAY: returning %ld\n", result));
	return result;
}

status_t 
CONFIGURE_OVERLAY(overlay_token ot, const overlay_buffer *ob, const overlay_window *ow, const overlay_view *ov)
{
	ati_overlay_token *aot = (ati_overlay_token *)ot;
	int index = aot - &(ai->ovl_tokens[0]);

	int use_h_filtering;
	int use_v_filtering;

	uint32 pitch;
	uint32 base_address;
	uint32 destwidth;
	uint32 srcheight;
	uint32 offsetleft;
	uint32 offsettop;
	uint32 offsetright;
	uint32 offsetbot;
	uint32 cropleft;
	uint32 croptop;
	uint32 cropright;
	uint32 cropbot;
	uint32 bytes_per_pixel;
	uint32 pixel_alignment_mask;
	uint32 scaler_buf0_offset;
	uint32 horz_inc;
	uint32 vert_inc;
	uint32 scaler_height;
	uint32 scaler_width;
	int32 besleft;
	int32 bestop;
	int32 besright;
	int32 besbot;
	int32 deskleft, desktop, deskright, deskbot;
	int32 ow_right, ow_bot;
	uint32 scaler_clock_divider;	

	/* validate the overlay token */
	if ((index < 0) || (index >= MAX_OVERLAYS)) {
		xdprintf(("CONFIGURE_OVERLAY: invalid overlay_token\n"));
		return B_ERROR;
	}
	if (!aot->used) {
		xdprintf(("CONFIGURE_OVERLAY: trying to configure un-allocated overlay\n"));
		return B_ERROR;
	}

	if ((ob == 0) || (ow == 0) || (ov == 0)) {
		/* disable overlay */
		regs[OVERLAY_SCALE_CNTL] = 0;
		/* note disabled state for virtual desktop sliding */
		aot->ob = 0;
		xdprintf(("CONFIGURE_OVERLAY: disabling overlay\n"));
		return B_OK;
	}

	/* check scaling limits, etc. */
	/* make sure the view fits in the buffer */
	if (((ov->width + ov->h_start) > ob->width) ||
		((ov->height + ov->v_start) > ob->height)) {
		xdprintf(("CONFIGURE_OVERLAY: overlay_view does not fit in overlay_buffer\n"));
		return B_ERROR;
	}

	/* offseting: window in visible virtual desktop */
	deskleft = (int32)ai->dm.h_display_start;
	desktop =  (int32)ai->dm.v_display_start;
	deskright = deskleft + (int32)ai->dm.timing.h_display - 1;
	deskbot   = desktop  + (int32)ai->dm.timing.v_display - 1;
	xdprintf(("desktop: %ld,%ld %ld,%ld\n", deskleft, desktop, deskright, deskbot));

	ow_right = ((int32)ow->h_start + (int32)ow->width) -  1;
	ow_bot   = ((int32)ow->v_start + (int32)ow->height) - 1;
	xdprintf(("overlay window: %d,%d %ld,%ld\n", ow->h_start, ow->v_start, ow_right, ow_bot));
	
	/* make sure the window is at least partialy in the desktop */
	if ((ow->h_start > deskright) ||
		(ow_right < deskleft) ||
		(ow->v_start > deskbot) ||
		(ow_bot < desktop)) {
		xdprintf(("CONFIGURE_OVERLAY: overlay_window does show on desktop\n"));
		/* disable overlay */
		regs[OVERLAY_SCALE_CNTL] = 0;
		goto save_info;
	}

	/* cropping: view within buffer */
	cropleft = ov->h_start;
	cropright = ob->width - ov->width - ov->h_start;
	croptop = ov->v_start;
	cropbot = ob->height - ov->height - ov->v_start;
	
	offsetleft = (((int32)ow->h_start + ow->offset_left) < deskleft) ? deskleft - ((int32)ow->h_start + ow->offset_left) : ow->offset_left;
	offsettop  = (((int32)ow->v_start + ow->offset_top)  < desktop)  ? desktop  - ((int32)ow->v_start + ow->offset_top)  : ow->offset_top;
	offsetright = ((ow_right - ow->offset_right) > deskright) ? ow_right - deskright - ow->offset_right  : ow->offset_right;
	offsetbot =   ((ow_bot - ow->offset_bottom)  > deskbot)   ? ow_bot   - deskbot   - ow->offset_bottom : ow->offset_bottom;

	/* the overlay_buffer defines the contents of the source data */
	/* the overlay_view defines what part of the overlay_buffer is visible in the overlay_window.  I.e. it crops the buffer. */
	/* the overlay_window defines how much space on the desktop the overlay takes and where it's located.  I.e. it defines scaling and positioning. */
	bytes_per_pixel = 2;  /* aka: bytes_per_pixel(ob->space) */
	switch (ob->space) {
		case B_YCbCr422:
			bytes_per_pixel = 2;
			break;
		case B_RGB32_LITTLE:
			bytes_per_pixel = 4;
			break;
		default:
			return B_ERROR;
	}
	pixel_alignment_mask = (8 / bytes_per_pixel) - 1;	/* for QWORD alignment off buffer starts */
	pitch = ob->bytes_per_row / bytes_per_pixel;
	base_address = (uint8 *)ob->buffer - (uint8 *)si->framebuffer;	/* offset into frame buffer of overlay buffer */

	if (ov->width > 768) return B_ERROR;
	//if (offsetleft & pixel_alignment_mask) return B_ERROR;	/* Yes, that really should be logical-and */

	/* source and destination widths, mostly for scaling */
	destwidth = ow->width;
	srcheight = ov->height; // ? ob/ov ?

	use_h_filtering = ow->flags & B_OVERLAY_HORIZONTAL_FITLERING ? 1 : 0;
	use_v_filtering = ow->flags & B_OVERLAY_VERTICAL_FILTERING ? 1 : 0;

	horz_inc = ((uint32)ov->width << 12) / (uint32)ow->width;
	vert_inc = ((uint32)ov->height << 12) / (uint32)ow->height;

	scaler_clock_divider = 0;
	// if the vclk exceeds 135Mhz, adjust scaling factors
	if (ai->dm.timing.pixel_clock > 108000) {
		// inverse scaling factors are multiplied by two
		horz_inc <<= 1;
		// scaled coordinates are divided by two
		offsetleft >>= 1;
		offsetright >>= 1;
#if 0
		vert_inc <<= 1;
		offsettop >>= 1;
		offsetbot >>= 1;
#endif
		// when we set the regs below, we'll set the scaler clock to vclk/2
		scaler_clock_divider++;
	}
#if 0
	// if the vclk exceeds 157.5Mhz, adjust scaling factors
	if (ai->dm.timing.pixel_clock > 162000) {
#if 1
		// inverse scaling factors are multiplied by two
		horz_inc <<= 1;
		vert_inc >>= 1; // FFB - ODD
		// scaled coordinates are divided by two
		offsetleft >>= 1;
		offsetright >>= 1;
		offsettop >>= 1;
		offsetbot >>= 1;
#endif
		// when we set the regs below, we'll set the scaler clock to vclk/2
		scaler_clock_divider++;
	}
#endif

	/* position in the *displayed* desktop of the destination window */
	besleft = ow->h_start + ow->offset_left - ai->dm.h_display_start;
	if (besleft < 0) besleft = 0;
	bestop = ow->v_start + ow->offset_top - ai->dm.v_display_start;
	if (bestop < 0) bestop = 0;
	besright = (ow->h_start + ow->width - 1 - ow->offset_right) - ai->dm.h_display_start;
	if (besright > ai->dm.timing.h_display) besright = ai->dm.timing.h_display - 1;
	besbot = (ow->v_start + ow->height - 1 - ow->offset_bottom) - ai->dm.v_display_start;
	if (besbot > ai->dm.timing.v_display) besbot = ai->dm.timing.v_display - 1;

	// scaler_width is the width of the overlay_view visible on the display
	scaler_width = ov->width - (((offsetleft + offsetright) * horz_inc) >> 12);
	scaler_height = ov->height - (((offsettop + offsetbot) * vert_inc) >> 12);

	//scaler_buf0_offset = ((offsettop * ob->bytes_per_row) + (offsetleft * bytes_per_pixel)) +  base_address;
#if 0
	scaler_buf0_offset = (croptop + ((offsettop * vert_inc) >> 12)) * pitch * bytes_per_pixel +
		base_address +
		((cropleft + ((((offsetleft * horz_inc) >> 12) + 1) & ~1)) * bytes_per_pixel);
#else
#if 1
	scaler_buf0_offset = (croptop + ((offsettop * vert_inc) >> 12)) * pitch * bytes_per_pixel +
		base_address +
		((cropleft + (offsetleft * ((double)horz_inc / (1<<12)))) * bytes_per_pixel);
	//scaler_buf0_offset += 4;
	scaler_buf0_offset &= ~7;
#else
	scaler_buf0_offset = croptop * pitch * bytes_per_pixel +
		base_address +
		cropleft * bytes_per_pixel;
	//scaler_buf0_offset += 4;
	scaler_buf0_offset &= ~7;
#endif
#endif

	/* stuff the registers */
	xdprintf(("crop coords: %ld,%ld %ld,%ld\n", cropleft, croptop, cropright, cropbot));
	xdprintf((" bes coords: %ld,%ld %ld,%ld\n", besleft, bestop, besright, besbot));
	xdprintf(("    offsets: %ld,%ld %ld,%ld\n", offsetleft, offsettop, offsetright, offsetbot));
	xdprintf(("  addresses: base - %ld, offset - %ld\n", base_address, scaler_buf0_offset));
	xdprintf(("   horz_inc: %f\n", (double)horz_inc / (double)(1 << 12)));
	xdprintf(("   vert_inc: %f\n", (double)vert_inc / (double)(1 << 12)));
	xdprintf(("scaler size: width - %ld, height - %ld\n", scaler_width, scaler_height));
	xdprintf(("bytes_per_pixel: %ld\n", bytes_per_pixel));
	xdprintf(("pixel_alignment_mask: 0x%08lx\n", pixel_alignment_mask));
	xdprintf(("pitch: %ld\n", pitch));


	{
	uint32 LTemp;
#if 0
		SCALER_BUF0_OFFSET,
		SCALER_BUF_PITCH,
		SCALER_HEIGHT_WIDTH,
		SCALER_COLOUR_CNTL,
		SCALER_H_COEFF0,
		SCALER_H_COEFF1,
		SCALER_H_COEFF2,
		SCALER_H_COEFF3,
		SCALER_H_COEFF4,
		VIDEO_FORMAT,
		CAPTURE_CONFIG,
		OVERLAY_SCALE_INC,
		OVERLAY_SCALE_CNTL,
		OVERLAY_EXCLUSIVE_HORZ,
		OVERLAY_KEY_CNTL,
		OVERLAY_Y_X_START,
		OVERLAY_Y_X_END,
#endif

#define SETREG(a, b) regs[a] = b; xdprintf(("regs["#a"] = 0x%08lx\n", (uint32)(b)));
	/* adjust scaler clock */
	// Write reference divider.
	LTemp = regs[CLOCK_CNTL];
	LTemp &= 0xFF0001FF;
	LTemp |= 0x05 << 10; // PLL_VCLK_CNTL
	regs[CLOCK_CNTL] = LTemp;	// write, so we can read current contents
	LTemp = regs[CLOCK_CNTL];	// read current contents
	LTemp |= 0x00000200; // enable write mode
	LTemp &= 0xFF0FFFFF; // clear scaler clock selection bits
	LTemp |= scaler_clock_divider << 20; // enable half-speed scaler clock
	SETREG(CLOCK_CNTL, LTemp);
	// Set to read PLL 0 (safe state).
	LTemp = regs[CLOCK_CNTL];
	LTemp &= 0xFF0001FF;
	regs[CLOCK_CNTL] = LTemp;

	SETREG(SCALER_BUF0_OFFSET, scaler_buf0_offset);
	SETREG(SCALER_BUF_PITCH, pitch);
	SETREG(SCALER_HEIGHT_WIDTH, (scaler_width << 16) | scaler_height);
	SETREG(SCALER_COLOUR_CNTL,
		(0) |	// BRIGHTNESS [-64,63] 7-bit signed
		((1 << 4) << 8) |	// SATURATION_U
		((1 << 4) << 16) | // SATURATION_V
		0);
	SETREG(SCALER_H_COEFF0, 0x00002000);
	SETREG(SCALER_H_COEFF1, 0x0d06200d);
	SETREG(SCALER_H_COEFF2, 0x0d0a1c0d);
	SETREG(SCALER_H_COEFF3, 0x0c0e1a0c);
	SETREG(SCALER_H_COEFF4, 0x0c14140c);
	switch (ob->space) {
		case B_YCbCr422:
			SETREG(VIDEO_FORMAT, (11 << 16));
			break;
		case B_RGB32_LITTLE:
			SETREG(VIDEO_FORMAT, (6 << 16));
			break;
	}
	SETREG(CAPTURE_CONFIG, 0);
	SETREG(OVERLAY_SCALE_INC, (horz_inc << 16) | vert_inc);
	SETREG(OVERLAY_EXCLUSIVE_HORZ, 0);
	if (ow->flags & B_OVERLAY_COLOR_KEY) {
		uint32 mask;
		uint32 value;
		/* use color key */
		SETREG(OVERLAY_KEY_CNTL, (5 << 4)); // color key
		// each mode is different
		
		switch (ai->dm.space & 0xfff) {
			case B_RGB32:
				mask = (uint32)(ow->red.mask << 16) | (uint32)(ow->green.mask << 8) | (uint32)(ow->blue.mask);
				value = (uint32)(ow->red.value << 16) | (uint32)(ow->green.value << 8) | (uint32)(ow->blue.value);
				break;
			case B_RGB16:
				mask = (uint32)((ow->red.mask & 0x1f) << 11) | (uint32)((ow->green.mask & 0x3f) << 5) | (uint32)((ow->blue.mask & 0x1f));
				value = (uint32)((ow->red.value & 0x1f) << 11) | (uint32)((ow->green.value & 0x3f) << 5) | (uint32)((ow->blue.value & 0x1f));
				break;
			case B_RGB15:
			case B_RGBA15:
				mask = (uint32)((ow->red.mask & 0x1f) << 10) | (uint32)((ow->green.mask & 0x1f) << 5) | (uint32)((ow->blue.mask & 0x1f));
				value = (uint32)((ow->red.value & 0x1f) << 10) | (uint32)((ow->green.value & 0x1f) << 5) | (uint32)((ow->blue.value & 0x1f));
				break;
			case B_CMAP8:
				mask = (uint32)(ow->red.mask | ow->green.mask | ow->blue.mask);
				value = (uint32)(ow->red.value | ow->green.value | ow->blue.value);
				break;
			default:
				mask = value = 0x00ffffff;	// what's up here?
				break;	
		}
		SETREG(OVERLAY_GRAPHICS_KEY_CLR, value);
		SETREG(OVERLAY_GRAPHICS_KEY_MSK, mask);		
	} else {
		/* excusive mode */
		SETREG(OVERLAY_KEY_CNTL, 1); // overlay always
	}
	SETREG(OVERLAY_Y_X_START, (besleft << 16) | bestop);
	SETREG(OVERLAY_Y_X_END, (besright << 16) | besbot);
	SETREG(OVERLAY_SCALE_CNTL,
		1 |	// SCALE_PIX_EXPAND 0 == zero extend, 1 == dynamic range correct
		(0 << 1) | // SCALE_Y2R_TEMP (red temp.) 0 == 6500K, 1 == 9500K
		((~use_h_filtering & 1) << 2) |
		((~use_v_filtering & 1) << 3) |
		(0 << 4) | // SCALE_SIGNED_UV 0 == unsigned UV, 1 == signed UV
		(0 << 5) | // SCALE_GAMMA_SEL 0 == brightness enable
		(1 << 26) | // SCALE_BANDWIDTH 1 == reset
		(1 << 30) | // OVERLAY_EN 1 == enable overlay
		(1 << 31) | // SCALE_EN 1 == enable scaler
		0);
	}

save_info:
	aot->ob = ob;
	aot->ow = *ow;
	aot->ov = *ov;
	return B_OK;
}
