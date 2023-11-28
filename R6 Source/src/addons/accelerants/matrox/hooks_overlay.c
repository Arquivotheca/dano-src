#include <OS.h>
#include <unistd.h>

#include "registers.h"
#include "mga_bios.h"
#include "accelerant_info.h"
#include "hooks_overlay.h"
#include "private.h"
#include "globals.h"

#if 0
#include <stdio.h>
#define ddprintf(a) printf a

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
#define ddprintf(a)
#endif

static uint32 overlay_spaces[] = { B_YCbCr422, B_NO_COLOR_SPACE };
static uint16      pitch_max[] = { 4092,       0 };

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
			result =
				B_OVERLAY_COLOR_KEY | 
				B_OVERLAY_HORIZONTAL_FITLERING |
				B_OVERLAY_VERTICAL_FILTERING |
				B_OVERLAY_HORIZONTAL_MIRRORING;
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

	ddprintf(("ALLOCATE_OVERLAY_BUFFER(%s, %d, %d)\n", spaceToString(cs), width, height));

	if (!width || !height) return 0;
	if (cs == B_NO_COLOR_SPACE) return 0;

	/* find an empty overlay buffer */
	for (index = 0; index < MAX_BUFFERS; index++)
		if (ai->ovl_buffers[index].space == B_NO_COLOR_SPACE)
			break;

	ddprintf(("%d == %ld : %s\n", index, MAX_BUFFERS, index == MAX_BUFFERS ? "true" : "false"));

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
	
	ddprintf(("overlay_spaces[%d] == B_NO_COLOR_SPACE: %s\n", i, overlay_spaces[i] == B_NO_COLOR_SPACE ? "true" : "false"));
	ddprintf(("%d > %d : %s\n", width, pitch_max[i], width > pitch_max[i] ? "true" : "false"));

	if (overlay_spaces[i] == B_NO_COLOR_SPACE) return 0;
	if (width > pitch_max[i]) return 0;

	space = overlay_spaces[i];

	// calculate buffer size
	switch (space) {
	case B_YCbCr422:
		bytes_per_pixel = 2;
		pitch_mask = 3;	/* bits which must be clear */
		break;
	}
	/* make width be a proper multiple for the back-end scaler */
	width = (width + pitch_mask) & ~pitch_mask;
	ob->bytes_per_row = bytes_per_pixel * width;
	ms = &(ai->ovl_buffer_specs[index]);
	ms->ms_MR.mr_Size = ob->bytes_per_row * height;
	ms->ms_AddrCareBits = 7;
	ms->ms_AddrStateBits = 0;
	ms->ms_PoolID = ai->poolid;
	ddprintf(("PoolID: %ld\n", ai->poolid));
	if (ioctl(memtypefd, B_IOCTL_ALLOCBYMEMSPEC, ms, sizeof(*ms)) < 0) {
		ddprintf(("ioctl(B_IOCTL_MALLOCBYMEMSPEC) for %ld bytes failed\n", ms->ms_Size));
		return 0;
	}

	ob->space = space;
	ob->width = width;
	ob->height = height;
	ob->buffer = (void *)((uint8 *) si->framebuffer + ms->ms_MR.mr_Offset);
	ob->buffer_dma = (void *)((uint8 *)si->framebuffer_pci + ms->ms_MR.mr_Offset);
	ddprintf(("ALLOCATE_OVERLAY_BUFFER: returning overlay_buffer %p\n", ob));
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
		ddprintf(("RELEASE_OVERLAY_BUFFER: bad buffer pointer\n"));
		return B_ERROR;
	}
	if (ob->space == B_NO_COLOR_SPACE) {
		ddprintf(("RELEASE_OVERLAY_BUFFER: releasing unallocated buffer\n"));
		return B_ERROR;
	}

	ms = &(ai->ovl_buffer_specs[index]);
	/* free the memory */
	ioctl(memtypefd, B_IOCTL_FREEBYMEMSPEC, ms, sizeof(*ms));
	/* mark as empty */
	ob->space = B_NO_COLOR_SPACE;
	ddprintf(("RELEASE_OVERLAY_BUFFER: OK\n"));
	return B_OK;
}

status_t
GET_OVERLAY_CONSTRAINTS(const display_mode *dm, const overlay_buffer *ob, overlay_constraints *oc)
{
	/* minimum size of view */
	oc->view.width.min = 1;
	oc->view.height.min = 1;
	/* view may not be larger than the buffer or 1024, which ever is lower */
	oc->view.width.max = ob->width > 1024 ? 1024 : ob->width;
	oc->view.height.max = ob->height > 1024 ? 1024 : ob->height;
	/* view alignment */
	oc->view.h_alignment = 0;
	oc->view.v_alignment = 0;
	oc->view.width_alignment = 0;
	oc->view.height_alignment = 0;
	
	/* minium size of window */
	oc->window.width.min = 1;
	oc->window.height.min = 1;
	/* upper usefull size is limited by screen realestate */
	oc->window.width.max = 4096;
	oc->window.height.max = 4096;
	/* window alignment */
	oc->window.h_alignment = 0;
	oc->window.v_alignment = 0;
	oc->window.width_alignment = 0;
	oc->window.height_alignment = 0;
	
	/* overall scaling factors */
	oc->h_scale.min = 1.0 / 32.0;
	oc->v_scale.min = 1.0 / 32.0;
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
		ddprintf(("ALLOCATE_OVERLAY: no more overlays\n"));
		return 0;
	}

	/* mark it in use */
	ai->ovl_tokens[index].used++;
	ddprintf(("ALLOCATE_OVERLAY: returning %p\n", &(ai->ovl_tokens[index])));
	return (overlay_token)&(ai->ovl_tokens[index]);
}

status_t 
RELEASE_OVERLAY(overlay_token ot)
{
	status_t result;
	mga_overlay_token *mot = (mga_overlay_token *)ot;
	int index = mot - &(ai->ovl_tokens[0]);

	if ((index < 0) || (index >= MAX_OVERLAYS)) {
		ddprintf(("RELEASE_OVERLAY: bad overlay_token\n"));
		return B_ERROR;
	}
	if (!ai->ovl_tokens[index].used) {
		ddprintf(("RELEASE_OVERLAY: releasing unused overlay\n"));
		return B_ERROR;
	}
	/* de-configure overlay */
	result = CONFIGURE_OVERLAY(ot, NULL, NULL, NULL);
	/* mark unused */
	ai->ovl_tokens[index].used = 0;
	ddprintf(("RELEASE_OVERLAY: returning %ld\n", result));
	return result;
}

status_t 
CONFIGURE_OVERLAY(overlay_token ot, const overlay_buffer *ob, const overlay_window *ow, const overlay_view *ov)
{
	status_t result;
	mga_overlay_token *mot = (mga_overlay_token *)ot;
	int index = mot - &(ai->ovl_tokens[0]);

	int use_h_filtering;
	int use_v_filtering;
	int use_h_mirroring;

	uint32 pitch;
	uint32 base_address;
	uint32 origin_address;
	uint32 srcwidth;
	uint32 destwidth;
	uint32 srcheight;
	uint32 intrep;
	uint32 ifactor;
	uint32 ifactorbetter;
	uint32 roundoff;
	uint32 beshzoom;
	uint32 beshiscal;
	uint32 besviscal;
	uint32 beshsrcst;
	uint32 beshsrcend;
	uint32 acczoom;
	uint32 dataformat;
	uint32 offsetleft;
	uint32 offsettop;
	uint32 offsetright;
	uint32 offsetbot;
	uint32 cropleft;
	uint32 croptop;
	uint32 cropright;
	uint32 cropbot;
	int32 besleft;
	int32 bestop;
	int32 besright;
	int32 besbot;
	int32 besv1srclst;
	int32 besv1wght;
	int32 deskleft, desktop, deskright, deskbot;
	int32 ow_right, ow_bot;
	

	/* validate the overlay token */
	if ((index < 0) || (index >= MAX_OVERLAYS)) {
		ddprintf(("CONFIGURE_OVERLAY: invalid overlay_token\n"));
		return B_ERROR;
	}
	if (!mot->used) {
		ddprintf(("CONFIGURE_OVERLAY: trying to configure un-allocated overlay\n"));
		return B_ERROR;
	}

	if ((ob == 0) || (ow == 0) || (ov == 0)) {
		/* disable overlay */
		BES32W(BES_CTL, 0);
		/* note disabled state for virtual desktop sliding */
		mot->ob = 0;
		ddprintf(("CONFIGURE_OVERLAY: disabling overlay\n"));
		return B_OK;
	}

	/* check scaling limits, etc. */
	/* make sure the view fits in the buffer */
	if (((ov->width + ov->h_start) > ob->width) ||
		((ov->height + ov->v_start) > ob->height)) {
		ddprintf(("CONFIGURE_OVERLAY: overlay_view does not fit in overlay_buffer\n"));
		return B_ERROR;
	}

	use_h_filtering = ow->flags & B_OVERLAY_HORIZONTAL_FITLERING ? 1 : 0;
	use_v_filtering = ow->flags & B_OVERLAY_VERTICAL_FILTERING ? 1 : 0;
	use_h_mirroring = ow->flags & B_OVERLAY_HORIZONTAL_MIRRORING ? 1 : 0;
	
	/* offseting: window in visible virtual desktop */
	deskleft = (int32)ai->dm.h_display_start;
	desktop =  (int32)ai->dm.v_display_start;
	deskright = deskleft + (int32)ai->dm.timing.h_display - 1;
	deskbot   = desktop  + (int32)ai->dm.timing.v_display - 1;
	ddprintf(("desktop: %ld,%ld %ld,%ld\n", deskleft, desktop, deskright, deskbot));

	ow_right = ((int32)ow->h_start + (int32)ow->width) -  1;
	ow_bot   = ((int32)ow->v_start + (int32)ow->height) - 1;
	ddprintf(("overlay window: %d,%d %ld,%ld\n", ow->h_start, ow->v_start, ow_right, ow_bot));
	
	/* make sure the window is at least partialy in the desktop */
	if ((ow->h_start > deskright) ||
		(ow_right < deskleft) ||
		(ow->v_start > deskbot) ||
		(ow_bot < desktop)) {
		ddprintf(("CONFIGURE_OVERLAY: overlay_window does show on desktop\n"));
		/* disable overlay */
		BES32W(BES_CTL, 0);
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

	pitch = ob->bytes_per_row / 2 /* aka: bytes_per_pixel(ob->space) */ ;
	base_address = (uint8 *)ob->buffer - (uint8 *)si->framebuffer;	/* offset into frame buffer of overlay buffer */


	/* source and destination widths, mostly for scaling */
	destwidth = ow->width;
	srcheight = ov->height; // ? ob/ov ?

	/* horizontal positioning and scaling */
	if (use_h_filtering) {
		if ((ov->width == ow->width) || (ow->width < 2)) intrep = 0;
		else intrep = 1;
	} else {
		if ((ow->width >= ov->width) || (ow->width < 2)) intrep = 0;
		else intrep = 1;
	}

	ifactor = (((ov->width - intrep) << 14) / ((ow->width - intrep) << 14)) << 14;
	ifactorbetter = ((((ov->width - intrep) << 20) / ((ow->width - intrep) << 20)) << 20) + intrep;

	ifactor = (((ov->width - intrep) << 14) / ((ow->width - intrep) << 0)) << 0;
	ifactorbetter = ((((ov->width - intrep) << 20) / ((ow->width - intrep) << 0)) << 0) + intrep;
	ddprintf(("raw ifactors: %ld, %ld\n", ifactor, ifactorbetter));
	ddprintf(("horizontal ifactors: %f, %f\n", (double)ifactor / (double)(1 << 14), (double)ifactorbetter / (double)(1 << 20)));

	if (use_h_filtering) {
		roundoff = 0;
	} else {
		uint32 dwm1 = ow->width - 1;
		if (((ifactorbetter * (dwm1 << 20)) >> 20) > ((ifactor * (dwm1 << 14)) >> 14)) roundoff = 1;
		else roundoff = 0;
	}
	
	beshzoom = (ai->dm.timing.pixel_clock > 135000) ? 1 : 0;
	if (beshzoom) acczoom = 2;
	else acczoom = 1;

	beshiscal = (acczoom * ifactor) + roundoff;
	/* (srcwidth / 16384) <= beshiscal < 32 */    /*        14 */
	if ((beshiscal < (((uint32)ov->width << 14) / (16384 << 0))) || (beshiscal >= (32 << 14))) {
		ddprintf(("CONFIGURE_OVERLAY: horizontal scale out of range: %f\n", (double)beshiscal / (double)(1 << 14)));
		return B_ERROR;
	}

	if (use_h_mirroring) {
		beshsrcst = ((cropright << 14) + ((offsetleft << 0) * (ifactor + roundoff))) >> 0; // 14
	} else {
		beshsrcst = ((cropleft << 14) + ((offsetleft << 0) * (ifactor + roundoff))) >> 0; // 14
	}
	beshsrcend = beshsrcst +
		(((((destwidth - offsetleft - offsetright - 1) << 14) / acczoom) & ~((1 << 14) - 1)) *
		(acczoom * ifactor + roundoff));
	beshsrcend = beshsrcst +
		(((destwidth - offsetleft - offsetright - 1) / acczoom) *
		(acczoom * ifactor + roundoff));

	/* vertical positioning and scaling */
	if (use_v_filtering) {
		if ((ov->height == ow->height) || (ow->height < 2)) intrep = 0;
		else intrep = 1;
	} else {
		if ((ow->height >= ov->height) || (ow->height < 2)) intrep = 0;
		else intrep = 1;
	}

	ifactor = (((srcheight - intrep) << 14) / ((ow->height - intrep) << 14)) << 14;
	ifactorbetter = ((((srcheight - intrep) << 20) / ((ow->height - intrep) << 20)) << 20) + intrep;

	ifactor = (((srcheight - intrep) << 14) / ((ow->height - intrep) << 0)) << 0;
	ifactorbetter = ((((srcheight - intrep) << 20) / ((ow->height - intrep) << 0)) << 0) + intrep;
	ddprintf(("raw ifactors: %ld, %ld\n", ifactor, ifactorbetter));
	ddprintf(("vertical ifactors: %f, %f\n", (double)ifactor / (double)(1 << 14), (double)ifactorbetter / (double)(1 << 20)));
	
	if (use_v_filtering) {
		roundoff = 0;
	} else {
		uint32 dhm1 = ow->height - 1;
		if (((ifactorbetter * (dhm1 << 20)) >> 20) > ((ifactor * (dhm1 << 14)) >> 14)) roundoff = 1;
		else roundoff = 0;
	}
	
	besviscal = ifactor + roundoff;
	/* (srcheight / 16384) <= besviscal < 32 */  /* 14 */
	if ((besviscal < ((srcheight << 14) / (16384 << 0))) || (besviscal >= (32 << 14))) {
		ddprintf(("CONFIGURE_OVERLAY: vertical scale out of range: %f\n", (double)besviscal / (double)(1 << 14)));
		return B_ERROR;
	}

	dataformat = 2; /* dataformat_for(ob->space) */
	origin_address = (croptop + (((offsettop << 14) * besviscal) >> 14)) * pitch * dataformat +
		base_address + (use_h_mirroring * (srcheight - 1) * dataformat);

	origin_address = (croptop + (((offsettop << 0) * besviscal) >> 14)) * pitch * dataformat +
		base_address + (use_h_mirroring * (ov->width - 1) * dataformat);

	/* Add chroma_plane_origin here if we ever support 4:2:0 planar */

	besv1wght = offsettop * besviscal;
	
	besv1srclst = srcheight - 1 - (((croptop << 14) + (offsettop * besviscal)) >> 14);
	
	/* position in the *displayed* desktop of the destination window */
	besleft = ow->h_start + ow->offset_left - ai->dm.h_display_start;
	if (besleft < 0) besleft = 0;
	bestop = ow->v_start + ow->offset_top - ai->dm.v_display_start;
	if (bestop < 0) bestop = 0;
	besright = (ow->h_start + ow->width - 1 - ow->offset_right) - ai->dm.h_display_start;
	if (besright > ai->dm.timing.h_display) besright = ai->dm.timing.h_display - 1;
	besbot = (ow->v_start + ow->height - 1 - ow->offset_bottom) - ai->dm.v_display_start;
	if (besbot > ai->dm.timing.v_display) besbot = ai->dm.timing.v_display - 1;

	/* stuff the registers */
	ddprintf((" bes coords: %ld,%ld %ld,%ld\n", besleft, bestop, besright, besbot));
	ddprintf(("    offsets: %ld,%ld %ld,%ld\n", offsetleft, offsettop, offsetright, offsetbot));
	ddprintf(("  addresses: base - %ld, origin - %ld\n", base_address, origin_address));
	ddprintf(("   besv1wght: %f\n", (double)besv1wght / (double)(1 << 14)));
	ddprintf(("  besviscal: %f\n", (double)besviscal / (double)(1 << 14)));
	ddprintf(("  beshiscal: %f\n", (double)beshiscal / (double)(1 << 14)));
	ddprintf(("  beshsrcst: %f\n", (double)beshsrcst / (double)(1 << 14)));
	ddprintf((" beshsrcend: %f\n", (double)beshsrcend / (double)(1 << 14)));
	ddprintf(("besv1srclst: %ld\n", besv1srclst));

	{
	int i;
	static uint32 besregs[] = {
		BES_A1ORG,
		BES_PITCH,
		BES_HCOORD,
		BES_VCOORD,
		BES_HISCAL,
		BES_VISCAL,
		BES_HSRCST,
		BES_HSRCEND,
		BES_V1WGHT,
		BES_HSRCLST,
		BES_V1SRCLST,
		BES_GLOBCTL,
		BES_CTL,
		0xff
	};
#define BES_REG_COUNT (sizeof(besregs) / sizeof(besregs[0])) 
	uint32 besvalues[BES_REG_COUNT];
	besvalues[0] = origin_address;
	besvalues[1] = pitch;
	besvalues[2] = ((besleft & 0x7ff) << 16) | (besright & 0x7ff);
	besvalues[3] = ((bestop  & 0x7ff) << 16) | (besbot   & 0x7ff);
	besvalues[4] = (beshiscal & 0x7ffff) << 2;
	besvalues[5] = (besviscal & 0x7ffff) << 2;
	besvalues[6] = (beshsrcst  & 0x00ffffff) << 2;
	besvalues[7] = (beshsrcend & 0x00ffffff) << 2;
	besvalues[8] = (besv1wght & 0x3fff) << 2; // I think the sign will always be zero
	besvalues[9] = ((uint32)(ob->width - 1)) << 16;
	besvalues[10] = (besv1srclst & 0x3ff);
	besvalues[11] =
		beshzoom |
		((uint32)ai->dm.timing.v_display << 16) | // besvcnt
		((use_h_filtering && beshzoom ? 1 : 0) << 1) |	// beshzoomf
		0;
	besvalues[12] =
		1 | // besen
		((((besv1wght >> 14) + croptop) & 1) << 6) | // besv1srcstp
		((use_h_filtering & 1) << 10) | // beshfen
		((use_v_filtering & 1) << 11) | // besvfen
		((use_h_mirroring & 1) << 19) | // beshmir
		0;
	for (i = 0; besregs[i] != 0xff; i++) {
		BES32W(besregs[i], besvalues[i]);
		ddprintf(("BES32W(%02lx, %08lx)\n", besregs[i], besvalues[i]));
	}
	if (ow->flags & B_OVERLAY_COLOR_KEY) {
		/* do color keying */
		DAC8IW(MID_XKEYOPMODE, 1);
		DAC8IW(MID_XCOLMSK0RED, ow->red.mask);
		DAC8IW(MID_XCOLKEY0RED, ow->red.value);
		DAC8IW(MID_XCOLMSK0GREEN, ow->green.mask);
		DAC8IW(MID_XCOLKEY0GREEN, ow->green.value);
		DAC8IW(MID_XCOLMSK0BLUE, ow->blue.mask);
		DAC8IW(MID_XCOLKEY0BLUE, ow->blue.value);
	} else {
		/* do exclusive (non-keying) overlay */
		DAC8IW(MID_XKEYOPMODE, 0);
	}
	}
save_info:
	mot->ob = ob;
	mot->ow = *ow;
	mot->ov = *ov;
	return B_OK;
}
