/* :ts=8 bk=0
 *
 * overlay.c: YUV Overlay features.
 *
 * $Id:$
 *
 * Andrew Kimpton					1999.11.02
 *  Derived from matrox driver sources.
 *
 * Copyright 1998 Be Incorporated.  Confidential unpublished work.
 * All Rights Reserved.
 */
#include <BeBuild.h>
#include <support/ByteOrder.h>

#include <graphics_p/video_overlay.h>

#include <add-ons/graphics/Accelerant.h>

#include <graphics_p/3dfx/voodoo4/voodoo4.h>
#include <graphics_p/3dfx/voodoo4/voodoo4_regs.h>
#include <graphics_p/3dfx/common/bena4.h>
#include <graphics_p/3dfx/common/debug.h>

#include "protos.h"

#include "fifo.h"

extern thdfx_card_info *ci;
extern __mem_AreaDef *MemMgr_2D;
extern int devfd;

// Debug assistance routines
static const char *spaceToString (uint32 cs)
{
	const char *s;
	switch (cs)
	{
#define s2s(a) case a: s = #a ; break
		s2s (B_RGB32);
		s2s (B_RGBA32);
		s2s (B_RGB32_BIG);
		s2s (B_RGBA32_BIG);
		s2s (B_RGB16);
		s2s (B_RGB16_BIG);
		s2s (B_RGB15);
		s2s (B_RGBA15);
		s2s (B_RGB15_BIG);
		s2s (B_RGBA15_BIG);
		s2s (B_CMAP8);
		s2s (B_GRAY8);
		s2s (B_GRAY1);
		s2s (B_YCbCr422);
		s2s (B_YCbCr420);
		s2s (B_YUV422);
		s2s (B_YUV411);
		s2s (B_YUV9);
		s2s (B_YUV12);
	default:
		s = "unknown";
		break;
#undef s2s
	}
	return s;
}

// The 'real' code

static uint32 overlay_spaces[] = { B_YCbCr422, B_YCbCr411, B_RGB16, B_NO_COLOR_SPACE };
static uint16 pitch_max[] = { 4092, 4092, 4092, 0 };

uint32 OverlayCount (const display_mode * dm)
{
	return 1;
}

const uint32 *OverlaySupportedSpaces (const display_mode * dm)
{
	return overlay_spaces;
}

uint32 OverlaySupportedFeatures (uint32 a_color_space)
{
	uint32 result;

	switch (a_color_space)
	{
	case B_YCbCr422:
	case B_YCbCr411:
	case B_RGB16:
		result = B_OVERLAY_COLOR_KEY | B_OVERLAY_HORIZONTAL_FITLERING | B_OVERLAY_VERTICAL_FILTERING;
		break;
	default:
		result = 0;
		break;
	}
	return result;
}

const overlay_buffer *AllocateOverlayBuffer (color_space cs, uint16 width, uint16 height)
{
	int i,buf;
	uint32 space;
	uint32 bytes_per_pixel = 0;
	uint32 pitch_mask = 0;
	overlay_buffer *ob;

	if (!width || !height)
		return 0;
	if (cs == B_NO_COLOR_SPACE)
		return 0;

	/* all in use - then fail */
	ob = &(ci->OverlayBuffer[0]);
	buf=0;
	while( ci->AllocOverlayBuffer[buf] )
	{
		buf++;
		ob++;
		if( buf >= MAX_OVERLAY_BUFFERS )
			return 0;
	}

	/* otherwise try and make a buffer */
	/* validate color_space */
	i = 0;
	while (overlay_spaces[i])
	{
		if (overlay_spaces[i] == cs)
			break;
		i++;
	}

	if (overlay_spaces[i] == B_NO_COLOR_SPACE)
	{
		dprintf (("3dfx_accel: Colorspace 0x%x not supported \n", cs ));
		return 0;
	}
	if (width > pitch_max[i])
	{
		dprintf (("3dfx_accel: Width %d too great. \n", width ));
		return 0;
	}

	space = overlay_spaces[i];

	// calculate buffer size
	switch (space)
	{
	case B_RGB16:
		bytes_per_pixel = 2;
		pitch_mask = 3;
		break;
	case B_YCbCr411:
		bytes_per_pixel = 4;
		pitch_mask = 7;			  /* bits which must be clear */
		break;
	case B_YCbCr422:
		bytes_per_pixel = 2;
		pitch_mask = 3;			  /* bits which must be clear */
		break;
	}

	/* make width be a proper multiple for the back-end scaler */
// width = (width + pitch_mask) & ~pitch_mask;
	ob->bytes_per_row = bytes_per_pixel * width;
	ci->AllocOverlayBuffer[buf] = __mem_Allocate (MemMgr_2D, ob->bytes_per_row * height);
	ci->AllocOverlayBuffer[buf]->locked = 255;

	ob->space = space;
	ob->width = width;
	ob->height = height;
	ob->buffer = (void *) ci->AllocOverlayBuffer[buf]->address;
	ob->buffer_dma = (void *) ((uint8 *) ci->BaseAddr1_DMA + ((uint8 *) ob->buffer - (uint8 *) ci->BaseAddr1));
	return ob;
}

status_t ReleaseOverlayBuffer (const overlay_buffer * _ob)
{
	overlay_buffer *ob = (overlay_buffer *) _ob;
	int32 index = (ob - (&ci->OverlayBuffer[0]));

	if (ob->space == B_NO_COLOR_SPACE)
	{
		dprintf (("3dfx_accel: ReleaseOverlayBuffer - releasing unallocated buffer\n"));
		return B_ERROR;
	}

	/* free the memory */
	__mem_Free (MemMgr_2D, ci->AllocOverlayBuffer[index]);
	ci->AllocOverlayBuffer[index] = 0;
	
	/* mark as empty */
	ob->space = B_NO_COLOR_SPACE;
	return B_OK;
}

status_t GetOverlayConstraints (const display_mode * dm, const overlay_buffer * ob, overlay_constraints * oc)
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
	oc->h_scale.min = 1.0;
	oc->v_scale.min = 1.0;
	oc->h_scale.max = 50.0;		  /* a lie, but a convienient one */
	oc->v_scale.max = 50.0;		  /* being accurate requires knowing either the window or the view size exactly */

	return B_OK;
}

#define MAX_OVERLAYS (sizeof(ci->OverlayTokens) / sizeof(ci->OverlayTokens[0]))


static uint32 OverlayToken = 1;

overlay_token AllocateOverlay (void)
{
	if( ci->OverlayOwnerID >= 0 )
	{
		dprintf (("3dfx_accel: AllocateOverlay - no more overlays\n"));
		return 0;
	}
	
	ci->OverlayOwnerID = MemMgr_2D->memID;
	return &OverlayToken;	// Not sure if this is good.
}

status_t ReleaseOverlay (overlay_token ot)
{
	status_t result;
	thdfx_overlay_token *tot = (thdfx_overlay_token *) ot;

	if ( ot != &OverlayToken )
	{
		dprintf (("3dfx_accel: ReleaseOverlay - bad overlay_token \n" ));
		return B_ERROR;
	}

	/* de-configure overlay */
	result = ConfigureOverlay (ot, NULL, NULL, NULL);

	/* mark unused */
	ci->OverlayOwnerID = -1;
	
	return result;
}

status_t ConfigureOverlay (overlay_token ot, const overlay_buffer * ob, const overlay_window * ow, const overlay_view * ov)
{
	uint32 reg_vidOverlayChroma;
	uint32 reg_vidOverlayDudxOffsetSrcWidth;
	uint32 reg_vidOverlayDudx;
	uint32 reg_vidOverlayDudy;
	uint32 reg_vidOverlayStartCoords;
	uint32 reg_vidOverlayEndScreenCoord;
	uint32 reg_vidProcCfg = ci->reg.r_V5_VID_PROC_CFG;
	uint32 reg_vidDesktopOverlayStride = ci->reg.r_V5_VID_DESKTOP_OVERLAY_STRIDE;

	status_t result;
	thdfx_overlay_token *tot = (thdfx_overlay_token *) ot;
	int32 start_x, start_y, end_x, end_y, desktop_w, desktop_h;
	int32 ov_display_width, ov_display_height;
	int32 use_h_filtering, use_v_filtering, use_h_mirroring;
	float horiz_scale, vert_scale, initial_offset_h, initial_offset_v;
	uint32 base_address;
	static engine_token *et;


	if( ov && ow && ((ov->width > ow->width) || (ov->height > ow->height)) )
	{
		dprintf (("3dfx_v4_accel: ConfigureOverlay - trying to configure overlay with scale < 1\n"));
		return B_ERROR;
	}

	if (!tot->used)
	{
		dprintf (("3dfx_v4_accel: ConfigureOverlay - trying to configure un-allocated overlay\n"));
		return B_ERROR;
	}

	if ((et = (engine_token *)malloc(sizeof(engine_token))) == 0)
	{
		dprintf(("3dfx_v4_accel: ConfigureOverlay - unable to allocate storage for engine token\n"));
		return B_ERROR;
	}

	acquireengine(0, 0, 0, &et);
	waitengineidle();
	if ((ob == 0) || (ow == 0) || (ov == 0))
	{
		/* disable overlay */
		reg_vidProcCfg &= ~( SST_CHROMA_EN | SST_OVERLAY_EN );
		_V5_WriteReg_NC( ci, V5_VID_PROC_CFG, reg_vidProcCfg );

		/* note disabled state for virtual desktop sliding */
		tot->ob = 0;
		releaseengine(et, 0);
		return B_OK;
	}

	/* check scaling limits, etc. */
	/* make sure the view fits in the buffer */
	if (((ov->width + ov->h_start) > ob->width) || ((ov->height + ov->v_start) > ob->height))
	{
		dprintf (("3dfx_accel: ConfigureOverlay - overlay_view does not fit in overlay_buffer\n"));
		releaseengine(et, 0);
		return B_ERROR;
	}

	use_h_filtering = ow->flags & B_OVERLAY_HORIZONTAL_FITLERING ? 1 : 0;
	use_v_filtering = ow->flags & B_OVERLAY_VERTICAL_FILTERING ? 1 : 0;
	use_h_mirroring = ow->flags & B_OVERLAY_HORIZONTAL_MIRRORING ? 1 : 0;

	if (ow->flags & B_OVERLAY_COLOR_KEY)
	{
		/* do color keying */
		if( ci->Depth == 16 )
		{
			reg_vidOverlayChroma =
				(ow->red.value << SST_VIDEO_CHROMA_16BPP_RED_SHIFT) |
				(ow->green.value << SST_VIDEO_CHROMA_16BPP_GREEN_SHIFT) |
				(ow->blue.value << SST_VIDEO_CHROMA_16BPP_BLUE_SHIFT);
		}
		else
		{
			reg_vidOverlayChroma =
				(ow->red.value << SST_VIDEO_CHROMA_32BPP_RED_SHIFT) |
				(ow->green.value << SST_VIDEO_CHROMA_32BPP_GREEN_SHIFT) |
				(ow->blue.value << SST_VIDEO_CHROMA_32BPP_BLUE_SHIFT);
		}
	}

	// Enable Chroma Key and Overlay in the Video Processor
	reg_vidProcCfg &= ~(
			SST_OVERLAY_STEREO_EN |	// Disable stereo
//          SST_INTERLACED_EN             |  // No interlace output
			SST_CHROMA_EN |	  // Enable chromakeying  need this??? -SS
//          SST_CHROMA_INVERT             |  // Disable chromakey result inversion need this??? -SS
			SST_OVERLAY_EN |	  // Fetch overlay
//          SST_VIDEOIN_AS_OVERLAY        |  // Autoflipping overlay enable
			SST_OVERLAY_CLUT_BYPASS |	// No overlay clut bypass
			SST_OVERLAY_CLUT_SELECT |	// Overlay lower clut select
			SST_OVERLAY_HORIZ_SCALE_EN |	// Enable horizontal scaling
			SST_OVERLAY_VERT_SCALE_EN |	// Enable vertical scaling
			SST_OVERLAY_FILTER_MODE |	// Use bilinear scaling
			SST_OVERLAY_PIXEL_FORMAT |	// Select UYVY422
            SST_OVERLAY_TILED_EN          |  // Select overlay linear space
//          SST_OVERLAY_DEINTERLACE_EN    |  // Disable backend deinterlace
			0);

	reg_vidProcCfg |=
			SST_OVERLAY_EN |
			//SST_VIDEOIN_AS_OVERLAY        |
			SST_OVERLAY_CLUT_BYPASS |
			//SST_OVERLAY_CLUT_SELECT       |
			0;

	switch (ob->space)
	{
	case B_RGB16:
		reg_vidProcCfg |= SST_OVERLAY_PIXEL_RGB565U;
		break;
	case B_YCbCr411:
		reg_vidProcCfg |= SST_OVERLAY_PIXEL_YUV411;
		break;
	case B_YCbCr422:
		reg_vidProcCfg |= SST_OVERLAY_PIXEL_YUYV422;
		break;
	}

	if (ow->flags & B_OVERLAY_COLOR_KEY)
		reg_vidProcCfg |= SST_CHROMA_EN;

	// What size is the desktop surface ?
	desktop_w = ci->CurDispMode.timing.h_display;
	desktop_h = ci->CurDispMode.timing.v_display;

	if ((use_h_filtering || use_v_filtering) && (desktop_w <= 1280))	// Filtering not supported for modes above 1280x1024
		reg_vidProcCfg |= SST_OVERLAY_FILTER_BILINEAR;

	// Set the Stride for the overlay
	reg_vidDesktopOverlayStride &= ~(SST_OVERLAY_LINEAR_STRIDE);
	reg_vidDesktopOverlayStride |= ob->bytes_per_row << SST_OVERLAY_STRIDE_SHIFT;

	// Calculate the horizontal & vertical scaling values
	horiz_scale = (float) ov->width / (float) ow->width;
	vert_scale = (float) ov->height / (float) ow->height;

	// Set the OffsetSrcWidth & scaling values
	ov_display_width = ow->width - (0 - ow->h_start);
	if (ov_display_width + ow->h_start > desktop_w)
		ov_display_width = desktop_w - ow->h_start;

	initial_offset_h = 0;
	ov_display_width = (ov_display_width * horiz_scale) + initial_offset_h;
	reg_vidOverlayDudxOffsetSrcWidth = (ov_display_width * 2 /* bpp */ ) << SST_OVERLAY_FETCH_SIZE_SHIFT;
	if (horiz_scale != 1.0)
	{
		reg_vidProcCfg |= SST_OVERLAY_HORIZ_SCALE_EN;
		reg_vidOverlayDudx = (uint32) ((float) (1 << 20) * horiz_scale);
	}
	if (vert_scale != 1.0)
	{
		reg_vidProcCfg |= SST_OVERLAY_VERT_SCALE_EN;
		reg_vidOverlayDudy = (uint32) ((float) (1 << 20) * vert_scale);
	}

	if (ow->h_start < 0)
		reg_vidOverlayDudxOffsetSrcWidth |= ((1 << 19) - 1) & ((uint32) ((float) (1 << 19) * (ow->h_start) * horiz_scale));

	// Set The start coords of the overlay
	start_x = ow->h_start;
	start_y = ow->v_start;
	if (start_x < 0)
		start_x = 0;
	if (start_y < 0)
		start_y = 0;
	reg_vidOverlayStartCoords = ((start_x) << SST_OVERLAY_X_SHIFT) | ((start_y) << SST_OVERLAY_Y_SHIFT);

	// Set The end coords of the overlay
	end_x = ow->h_start + ow->width - 1;
	end_y = ow->v_start + ow->height - 1;
	if (end_x > desktop_w)
		end_x = desktop_w;
	if (end_y > desktop_h)
		end_y = desktop_h;
	reg_vidOverlayEndScreenCoord = (end_x << SST_OVERLAY_X_SHIFT) | (end_y << SST_OVERLAY_Y_SHIFT);

	// Calculate and set the overlay base address
	base_address = (uint8 *) ob->buffer - (uint8 *) ci->BaseAddr1;	/* offset into frame buffer of overlay buffer */

	if (ow->h_start < 0)			  // Left edge offscreen - so shift the start address
		base_address = base_address + ((0 - ow->h_start) * horiz_scale * 2) /* bpp */ ;
	if (ow->v_start < 0)			  // Top edge offscreen - so shift the start address
		base_address = base_address + ((uint32) ((0 - ow->v_start) * vert_scale) * ob->bytes_per_row);

	// Acquire the rendering engine so we can write to the FIFO
	ioctl (devfd, THDFX_IOCTL_CHECKFORROOM, 0, 0);
	_V5_WriteReg_NC( ci, V5_3D_LEFT_OVERLAY_BUF, base_address );
	_V5_WriteReg_NC( ci, V5_3D_SWAPBUFFER_CMD, 1 );

	_V5_WriteReg_NC( ci, V5_VID_CHROMA_MIN, reg_vidOverlayChroma );
	_V5_WriteReg_NC( ci, V5_VID_CHROMA_MAX, reg_vidOverlayChroma );
	_V5_WriteReg_NC( ci, V5_VID_OVERLAY_DUDX_OFFSET_SRC_WIDTH, reg_vidOverlayDudxOffsetSrcWidth );
	_V5_WriteReg_NC( ci, V5_VID_OVERLAY_DUDX, reg_vidOverlayDudx );
	_V5_WriteReg_NC( ci, V5_VID_OVERLAY_DVDY, reg_vidOverlayDudy );
	_V5_WriteReg_NC( ci, V5_VID_OVERLAY_START_COORDS, reg_vidOverlayStartCoords );
	_V5_WriteReg_NC( ci, V5_VID_OVERLAY_END_SCREEN_COORD, reg_vidOverlayEndScreenCoord );
	_V5_WriteReg_NC( ci, V5_VID_PROC_CFG, reg_vidProcCfg );
	_V5_WriteReg_NC( ci, V5_VID_DESKTOP_OVERLAY_STRIDE, reg_vidDesktopOverlayStride );
	
	tot->ob = ob;
	tot->ow = *ow;
	tot->ov = *ov;
	releaseengine(et, 0);
	return B_OK;
}
