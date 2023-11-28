/*
 * nm2200.c:	Basic rendering acceleration for Neomagic 2100 chips (MagicMedia 256AV).
 *
 * Copyright 2000 Be Incorporated.  Confidential unpublished work.
 * All Rights Reserved.
 */

#include <graphics_p/neomagic/neomagic.h>
#include <graphics_p/neomagic/debug.h>
#include "protos.h"

/*****************************************************************************
 * Globals.
 */
extern neomagic_card_info	*ci;
extern engine_token	enginetoken;

void
nm2200_blit (engine_token *et, blit_params *list, uint32 count)
{
	uint32	bltModeFlags, stride, bltCntlFlags, blitOffsetX, blitOffsetY;
	uint8		pixelWidth;
		
	if (et != &enginetoken)
	{
		dprintf(("neomagic_accel: nm2200_blit - EXIT - no enginetoken\n"));
		return;
	}

	switch (ci->ci_Depth)
	{
		case 8:
			bltModeFlags = NEO_MODE1_DEPTH8;
			pixelWidth = 1;
			break;
		case 15:
		case 16:
			bltModeFlags = NEO_MODE1_DEPTH16;
			pixelWidth = 2;
			break;
	}

  stride = ci->ci_CurDispMode.timing.h_display * pixelWidth;

	/* Initialize for widths */
	switch (ci->ci_CurDispMode.timing.h_display)
	{
		case 640:
			bltModeFlags |= NEO_MODE1_X_640;
			break;
		case 800:
			bltModeFlags |= NEO_MODE1_X_800;
			break;
		case 1024:
			bltModeFlags |= NEO_MODE1_X_1024;
			break;
		case 1152:
			bltModeFlags |= NEO_MODE1_X_1152;
			break;
		case 1280:
			bltModeFlags |= NEO_MODE1_X_1280;
			break;
		case 1600:
			bltModeFlags |= NEO_MODE1_X_1600;
			break;
	}

	bltCntlFlags = 0x0c0000 /*(NEO_BC3_SKIP_MAPPING | neo2200Rop[rop])*/;
	
	/* set blt control */
	WAIT_ENGINE_IDLE(50);
	OUTREG16(NEOREG_BLTMODE, bltModeFlags);
	OUTREG32(NEOREG_PITCH, (stride<<16) | (stride & 0xffff));

	while (count--)
	{
		blitOffsetX = 0;
		blitOffsetY = 0;
		if ((list->dest_top > list->src_top) || ((list->dest_top == list->src_top) && (list->dest_left > list->src_left)))
		{
			blitOffsetX = list->width;
			blitOffsetY = list->height;
			bltCntlFlags |= (NEO_BC0_DST_X_DEC | NEO_BC0_DST_Y_DEC | NEO_BC0_SRC_Y_DEC);
		}

		WAIT_ENGINE_IDLE(100);
		OUTREG32(NEOREG_BLTCNTL, bltCntlFlags);
		OUTREG32(NEOREG_SRCSTARTOFF,
						((list->src_top+blitOffsetY) * stride) + ((list->src_left+blitOffsetX) * pixelWidth));
		OUTREG32(NEOREG_DSTSTARTOFF,
						((list->dest_top+blitOffsetY) * stride) + ((list->dest_left+blitOffsetX) * pixelWidth));
		OUTREG32(NEOREG_XYEXT, ((list->height+1)<<16) | ((list->width+1) & 0xffff));
		list++;
	}
	ci->ci_PrimitivesIssued++;
}

void
nm2200_rectangle_fill (engine_token *et, uint32 color, fill_rect_params *list, uint32 count)
{
	uint32 bltModeFlags;
	
	if (et != &enginetoken)
	{
		dprintf(("neomagic_accel: rectangle_fill - EXIT - no enginetoken\n"));
		return;
	}
	
	switch (ci->ci_Depth)
	{
		case 8:
			bltModeFlags = NEO_MODE1_DEPTH8;
			break;
		case 15:
		case 16:
			bltModeFlags = NEO_MODE1_DEPTH16;
			break;
	}

	/* Initialize for widths */
	switch (ci->ci_CurDispMode.timing.h_display)
	{
		case 640:
			bltModeFlags |= NEO_MODE1_X_640;
			break;
		case 800:
			bltModeFlags |= NEO_MODE1_X_800;
			break;
		case 1024:
			bltModeFlags |= NEO_MODE1_X_1024;
			break;
		case 1152:
			bltModeFlags |= NEO_MODE1_X_1152;
			break;
		case 1280:
			bltModeFlags |= NEO_MODE1_X_1280;
			break;
		case 1600:
			bltModeFlags |= NEO_MODE1_X_1600;
			break;
	}

	/* set blt control */
	WAIT_ENGINE_IDLE(50);
	OUTREG16(NEOREG_BLTMODE, bltModeFlags);
	OUTREG32(NEOREG_BLTCNTL, NEO_BC0_SRC_IS_FG | NEO_BC3_SKIP_MAPPING | NEO_BC3_DST_XY_ADDR | NEO_BC3_SRC_XY_ADDR | 0x0c0000);
	OUTREG32(NEOREG_FGCOLOR, color);
	while (count--)
	{
		uint32 width, height;
		width = list->right - list->left + 1;
		height = list->bottom - list->top + 1;
		WAIT_ENGINE_IDLE(50);
		OUTREG32(NEOREG_DSTSTARTOFF, (list->top << 16) | (list->left & 0xffff));
		OUTREG32(NEOREG_XYEXT, (height << 16) | (width & 0xffff));
		list++;
	}	
	ci->ci_PrimitivesIssued++;
}

void
nm2200_rectangle_invert (engine_token *et, fill_rect_params *list, uint32 count)
{
	uint32 bltModeFlags;
	
	if (et != &enginetoken)
	{
		dprintf(("neomagic_accel: rectangle_invert - EXIT - no enginetoken\n"));
		return;
	}
	
	switch (ci->ci_Depth)
	{
		case 8:
			bltModeFlags = NEO_MODE1_DEPTH8;
			break;
		case 15:
		case 16:
			bltModeFlags = NEO_MODE1_DEPTH16;
			break;
	}

	/* Initialize for widths */
	switch (ci->ci_CurDispMode.timing.h_display)
	{
		case 640:
			bltModeFlags |= NEO_MODE1_X_640;
			break;
		case 800:
			bltModeFlags |= NEO_MODE1_X_800;
			break;
		case 1024:
			bltModeFlags |= NEO_MODE1_X_1024;
			break;
		case 1152:
			bltModeFlags |= NEO_MODE1_X_1152;
			break;
		case 1280:
			bltModeFlags |= NEO_MODE1_X_1280;
			break;
		case 1600:
			bltModeFlags |= NEO_MODE1_X_1600;
			break;
	}
	
	/* set blt control */
	WAIT_ENGINE_IDLE(50);
	OUTREG16(NEOREG_BLTMODE, bltModeFlags);
	OUTREG32(NEOREG_BLTCNTL, NEO_BC3_SKIP_MAPPING | NEO_BC3_DST_XY_ADDR | NEO_BC3_SRC_XY_ADDR | 0x050000);
	while (count--)
	{
		uint32 width, height;
		width = list->right - list->left + 1;
		height = list->bottom - list->top + 1;
		WAIT_ENGINE_IDLE(50);
		OUTREG32(NEOREG_DSTSTARTOFF, (list->top << 16) | (list->left & 0xffff));
		OUTREG32(NEOREG_XYEXT, (height << 16) | (width & 0xffff));
		list++;
	}	
	ci->ci_PrimitivesIssued++;
}

void
nm2200_span_fill (engine_token *et, uint32 color, uint16 *list, uint32 count)
{
	uint32 bltModeFlags;
	
//dprintf(("neomagic_accel: span_fill - ENTER\n"));
	if (et != &enginetoken)
	{
		dprintf(("neomagic_accel: span_fill - EXIT - no enginetoken\n"));
		return;
	}
	
	switch (ci->ci_Depth)
	{
		case 8:
			bltModeFlags = NEO_MODE1_DEPTH8;
			break;
		case 15:
		case 16:
			bltModeFlags = NEO_MODE1_DEPTH16;
			break;
	}

	/* Initialize for widths */
	switch (ci->ci_CurDispMode.timing.h_display)
	{
		case 640:
			bltModeFlags |= NEO_MODE1_X_640;
			break;
		case 800:
			bltModeFlags |= NEO_MODE1_X_800;
			break;
		case 1024:
			bltModeFlags |= NEO_MODE1_X_1024;
			break;
		case 1152:
			bltModeFlags |= NEO_MODE1_X_1152;
			break;
		case 1280:
			bltModeFlags |= NEO_MODE1_X_1280;
			break;
		case 1600:
			bltModeFlags |= NEO_MODE1_X_1600;
			break;
	}

	/* set blt control */
	WAIT_ENGINE_IDLE(50);
	OUTREG16(NEOREG_BLTMODE, bltModeFlags);
	OUTREG32(NEOREG_BLTCNTL, NEO_BC0_SRC_IS_FG | NEO_BC3_SKIP_MAPPING | NEO_BC3_DST_XY_ADDR | NEO_BC3_SRC_XY_ADDR | 0x0c0000);
	OUTREG32(NEOREG_FGCOLOR, color);
	while (count--)
	{
		uint32 width = list[2] - list[1] + 1;
		WAIT_ENGINE_IDLE(10);
		OUTREG32(NEOREG_DSTSTARTOFF, (list[0] << 16) | (list[1] & 0xffff));
		OUTREG32(NEOREG_XYEXT, (1 << 16) | (width & 0xffff));
		list += 3;
	}	
	ci->ci_PrimitivesIssued++;
}
