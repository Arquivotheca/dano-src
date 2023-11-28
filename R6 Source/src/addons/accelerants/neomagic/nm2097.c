/*
 * nm2097.c:	Basic rendering acceleration for NeoMagic 2097 & 2160 (aka 128).
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
nm2097_blit (engine_token *et, blit_params *list, uint32 count)
{
	uint32	bltModeFlags, bltCntlFlags, blitOffsetX, blitOffsetY;
		
	if (et != &enginetoken)
	{
		dprintf(("neomagic_accel: nm2097_blit - EXIT - no enginetoken\n"));
		return;
	}

	switch (ci->ci_Depth)
	{
		case 8:
			bltCntlFlags = NEO_BC1_DEPTH8;
			break;
		case 15:
		case 16:
			bltCntlFlags = NEO_BC1_DEPTH16;
			break;
	}

	/* Initialize for widths */
	switch (ci->ci_CurDispMode.timing.h_display)
	{
		case 640:
			bltCntlFlags |= NEO_BC1_X_640;
			break;
		case 800:
			bltCntlFlags |= NEO_BC1_X_800;
			break;
		case 1024:
			bltCntlFlags |= NEO_BC1_X_1024;
			break;
	}

	bltCntlFlags |= (0x0c0000 /* copy */ | NEO_BC3_SKIP_MAPPING | NEO_BC3_DST_XY_ADDR | NEO_BC3_SRC_XY_ADDR);
	
	/* set blt control */
	WAIT_ENGINE_IDLE(50);

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
						((list->src_top+blitOffsetY) << 16) | ((list->src_left+blitOffsetX) & 0xffff));
		OUTREG32(NEOREG_DSTSTARTOFF,
						((list->dest_top+blitOffsetY)<< 16) | ((list->dest_left+blitOffsetX) & 0xffff));
		OUTREG32(NEOREG_XYEXT, ((list->height+1)<<16) | ((list->width+1) & 0xffff));
		list++;
	}
	ci->ci_PrimitivesIssued++;
}

void
nm2097_rectangle_fill (engine_token *et, uint32 color, fill_rect_params *list, uint32 count)
{
	uint32 bltCntlFlags, colorShiftAmt;
	
	if (et != &enginetoken)
	{
		dprintf(("neomagic_accel: rectangle_fill - EXIT - no enginetoken\n"));
		return;
	}
	
	switch (ci->ci_Depth)
	{
		case 8:
			bltCntlFlags = NEO_BC1_DEPTH8;
			colorShiftAmt = 8;
			break;
		case 15:
		case 16:
			bltCntlFlags = NEO_BC1_DEPTH16;
			colorShiftAmt = 0;
			break;
	}

	/* Initialize for widths */
	switch (ci->ci_CurDispMode.timing.h_display)
	{
		case 640:
			bltCntlFlags |= NEO_BC1_X_640;
			break;
		case 800:
			bltCntlFlags |= NEO_BC1_X_800;
			break;
		case 1024:
			bltCntlFlags |= NEO_BC1_X_1024;
			break;
	}

	/* set blt control */
	WAIT_ENGINE_IDLE(50);
	bltCntlFlags |= (NEO_BC0_SRC_IS_FG | NEO_BC3_SKIP_MAPPING | NEO_BC3_DST_XY_ADDR | NEO_BC3_SRC_XY_ADDR | 0x0c0000);
	OUTREG32(NEOREG_BLTCNTL, bltCntlFlags);
	OUTREG32(NEOREG_FGCOLOR, color |= (color << colorShiftAmt));
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
nm2097_rectangle_invert (engine_token *et, fill_rect_params *list, uint32 count)
{
	uint32 bltCntlFlags;
	
	if (et != &enginetoken)
	{
		dprintf(("neomagic_accel: rectangle_invert - EXIT - no enginetoken\n"));
		return;
	}
	
	switch (ci->ci_Depth)
	{
		case 8:
			bltCntlFlags = NEO_BC1_DEPTH8;
			break;
		case 15:
		case 16:
			bltCntlFlags = NEO_BC1_DEPTH16;
			break;
	}

	/* Initialize for widths */
	switch (ci->ci_CurDispMode.timing.h_display)
	{
		case 640:
			bltCntlFlags |= NEO_BC1_X_640;
			break;
		case 800:
			bltCntlFlags |= NEO_BC1_X_800;
			break;
		case 1024:
			bltCntlFlags |= NEO_BC1_X_1024;
			break;
	}
	
	/* set blt control */
	WAIT_ENGINE_IDLE(50);
	bltCntlFlags |= (NEO_BC3_SKIP_MAPPING | NEO_BC3_DST_XY_ADDR | NEO_BC3_SRC_XY_ADDR | 0x050000);
	OUTREG32(NEOREG_BLTCNTL, bltCntlFlags);
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
nm2097_span_fill (engine_token *et, uint32 color, uint16 *list, uint32 count)
{
	uint32 bltCntlFlags, colorShiftAmt;
	
//dprintf(("neomagic_accel: span_fill - ENTER\n"));
	if (et != &enginetoken)
	{
		dprintf(("neomagic_accel: span_fill - EXIT - no enginetoken\n"));
		return;
	}
	
	switch (ci->ci_Depth)
	{
		case 8:
			bltCntlFlags = NEO_BC1_DEPTH8;
			colorShiftAmt = 8;
			break;
		case 15:
		case 16:
			bltCntlFlags = NEO_BC1_DEPTH16;
			colorShiftAmt = 0;
			break;
	}

	/* Initialize for widths */
	switch (ci->ci_CurDispMode.timing.h_display)
	{
		case 640:
			bltCntlFlags |= NEO_BC1_X_640;
			break;
		case 800:
			bltCntlFlags |= NEO_BC1_X_800;
			break;
		case 1024:
			bltCntlFlags |= NEO_BC1_X_1024;
			break;
	}
	
	/* set blt control */
	WAIT_ENGINE_IDLE(50);
	bltCntlFlags |= (NEO_BC0_SRC_IS_FG | NEO_BC3_SKIP_MAPPING | NEO_BC3_DST_XY_ADDR | NEO_BC3_SRC_XY_ADDR | 0x0c0000);
	OUTREG32(NEOREG_BLTCNTL, bltCntlFlags);
	OUTREG32(NEOREG_FGCOLOR, color |= (color << colorShiftAmt));
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
