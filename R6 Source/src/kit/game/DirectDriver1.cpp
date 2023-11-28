//******************************************************************************
//
//	File:		DirectDriver1.cpp
//
//	Description:	Private class to encapsulate init/dispose of client-side
//					graphic add-on clone.
//
//	Written by:	Pierre Raynaud-Richard
//
//	Revision history
//
//	Copyright 1998, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <Debug.h>

#ifndef _DIRECT_DRIVER1_H
#include <DirectDriver1.h>
#endif

#include <direct_window_priv.h>
#include <vga.h>

/*-------------------------------------------------------------*/

BDirectDriver1::BDirectDriver1(uint32 token) : BDirectDriver(token)
{
	int						i;
	status_t				err;
	direct_driver_info1		info;
	graphics_card_config	config;
	
	drawing_mix = 0x1c;
	addon_state = -1;
	addon_image = B_ERROR;
	
	// get driver infos
	err = GetDriverInfo(&info);
	if (err != B_NO_ERROR)
		return;
	// load add-on
	addon_image = load_add_on(info.addon_path);
	if (addon_image == B_ERROR)
		return;
	// init add-on
	if (get_image_symbol(addon_image,"control_graphics_card",
						 2, (void **)&addon_ctrl_jmp) != B_NO_ERROR)
		if (get_image_symbol(addon_image,"control_onboard_graphics_card",
							 2, (void **)&addon_ctrl_jmp) != B_NO_ERROR)
			return;
	// get available spaces and determine which modes are available.
	err = ControlDriver(dd_token, B_GET_SCREEN_SPACES,
						sizeof(graphics_card_config), (void*)&config);
	available_space = config.space;
	mode_count = 0;
	for (i=0; i<24; i++)
		if (available_space & (1<<i)) {
			mode_list[mode_count] = i+0x200;
			mode_count++;
		}
	
	addon_state = 1;
}

/*-------------------------------------------------------------*/

BDirectDriver1::~BDirectDriver1()
{
	if (addon_state == 1)
		(addon_ctrl_jmp)(B_CLOSE_CLONED_GRAPHICS_CARD, 0L);
	if (addon_image != B_ERROR)
		unload_add_on(addon_image);	
}

/*-------------------------------------------------------------*/

status_t BDirectDriver1::GetHook(driver_hook_token token, graphics_card_hook *hook)
{
	if (addon_state != 1)
		return B_ERROR;
	if (!is_fullscreen && (token < 1024))
		return B_ERROR;	
		
	switch(token) {
	case DD_GET_VIDEO_MODE_AT :
		*hook = (graphics_card_hook)BDirectDriver1::GetVideoModeAt;
		break;
	case DD_GET_VIDEO_MODE_INFO :
		*hook = (graphics_card_hook)BDirectDriver1::GetVideoModeInfo;
		break;
	case DD_SET_VIDEO_MODE :
		*hook = (graphics_card_hook)BDirectDriver1::SetVideoMode;
		break;
	case DD_SET_DISPLAY_START :
		if ((card_info.flags & B_FRAME_BUFFER_CONTROL) == 0)
			return B_ERROR;
		*hook = (graphics_card_hook)BDirectDriver1::SetDisplayStart;
		break;
	case DD_SET_PALETTE_DATA :
		*hook = (graphics_card_hook)BDirectDriver1::SetPaletteData;
		break;
	case DD_SET_CURSOR :
		if (hooks[0] == 0)
			return B_ERROR;
		*hook = (graphics_card_hook)BDirectDriver1::SetCursor;
		break;
	case DD_SET_CURSOR_POS :
		if (hooks[1] == 0)
			return B_ERROR;
		*hook = (graphics_card_hook)BDirectDriver1::SetCursorPos;
		break;
	case DD_SHOW_CURSOR :
		if (hooks[2] == 0)
			return B_ERROR;
		*hook = (graphics_card_hook)BDirectDriver1::ShowCursor;
		break;
	case DD_WAIT_TILL_IDLE :
		if (hooks[10] == 0)
			return B_ERROR;
		*hook = (graphics_card_hook)BDirectDriver1::WaitTillIdle;
		break;
	case DD_SET_MIX :
		*hook = (graphics_card_hook)BDirectDriver1::SetMix;
		break;
	case DD_SET_CLIP_RECT :
		*hook = (graphics_card_hook)BDirectDriver1::SetClipRect;
		break;
	case DD_DRAW_RECT :
		switch (depth) {
		case 8 :
			if (hooks[5] == 0)
				return B_ERROR;
			break;
		case 15 :
		case 16 :
			if (hooks[13] == 0)
				return B_ERROR;
			break;
		case 32 :
			if (hooks[6] == 0)
				return B_ERROR;
			break;
		default :
			return B_ERROR;
		}
		*hook = (graphics_card_hook)BDirectDriver1::DrawRect;
		break;
	case DD_DRAW_LINE :
		switch (depth) {
		case 8 :
			if (hooks[3] == 0)
				return B_ERROR;
			break;
		case 15 :
		case 16 :
			if (hooks[12] == 0)
				return B_ERROR;
			break;
		case 32 :
			if (hooks[4] == 0)
				return B_ERROR;
			break;
		default :
			return B_ERROR;
		}
		*hook = (graphics_card_hook)BDirectDriver1::DrawLine;
		break;
	case DD_BIT_BLT :
		if (hooks[7] == 0)
			return B_ERROR;
		*hook = (graphics_card_hook)BDirectDriver1::BitBlt;
		break;
	}
	
	return B_NO_ERROR;
}

/*-------------------------------------------------------------*/

status_t BDirectDriver1::GetCookie(void **cookie)
{
	*cookie = (void*)this;
	return B_NO_ERROR;
}

/*-------------------------------------------------------------*/

status_t BDirectDriver1::Sync(bool fullscreen)
{
	int				i;
	status_t		err;

	is_fullscreen = fullscreen;
	// grab current hook table.
	if ((addon_ctrl_jmp)(B_GET_GRAPHICS_CARD_HOOKS,(void*)hooks) != B_NO_ERROR)
		for (i=0; i<B_HOOK_COUNT; i++)
			hooks[i] = 0L;
	// get add-on infos
	err = ControlDriver(dd_token, B_GET_GRAPHICS_CARD_INFO,
						sizeof(graphics_card_info), (void*)&card_info);
	if (err != B_NO_ERROR)
		return B_ERROR;
	depth = card_info.bits_per_pixel;
	
	return B_ERROR;
}

/*-------------------------------------------------------------*/

status_t BDirectDriver1::ControlDriver(uint32 token, int32 cmd, int32 size, void *buf) {
	status_t          	error;
	_BAppServerLink_	link;
	
	link.session->swrite_l(GR_CTRL_GRAPHICS_CARD);
	link.session->swrite_l(token);
	link.session->swrite_l(cmd);
	link.session->swrite_l(size);
	link.session->swrite(size, buf);
	link.session->flush();

	link.session->sread(size, buf);
	link.session->sread(4, &error);
	return error;
}

/*-------------------------------------------------------------*/

status_t BDirectDriver1::SetSpace(	uint32		token,
									uint32		space,
									uint32		virtualX,
									uint32		virtualY) {
	_BAppServerLink_ link;
	long             error;

	link.session->swrite_l(GR_SET_STANDARD_SPACE);
	link.session->swrite_l(token);
	link.session->swrite_l(space);
	link.session->swrite_l(virtualX);
	link.session->swrite_l(virtualY);
	link.session->flush();

	link.session->sread(4, &error);
	return error;
}

/*-------------------------------------------------------------*/

int32 BDirectDriver1::GetVideoModeAt(	void		*cookie,
										uint32		index,
										uint16		*mode)
{
	BDirectDriver1	*dd;
	
	dd = (BDirectDriver1*)cookie;
	if (!dd->is_fullscreen)
		return -1;
	
	if ((index < 0) || (index >= dd->mode_count))
		return -1;
		
	*mode = dd->mode_list[index];
	return 0;
}

/*-------------------------------------------------------------*/

static uint16 XResolution[24] = {
	640,	800,	1024,	1280,	1600,	640,	800,	1024,
	1280,	1600,	640,	800,	1024,	1280,	1600,	1152,
	1152,	1152,	640,	800,	1024,	1280,	1600,	1152
};
static uint16 YResolution[24] = {
	480,	600,	768,	1024,	1200,	480,	600,	768,
	1024,	1200,	480,	600,	768,	1024,	1200,	900,
	900,	900,	480,	600,	768,	1024,	1200,	900
};
static uint16 BitsPerPixel[24] = {
	8,		8,		8,		8,		8,		16,		16,		16,
	16,		16,		32,		32,		32,		32,		32,		8,
	16,		32,		16,		16,		16,		16,		16,		16
};
static uint16 RedMaskSize[24] = {
	0,		0,		0,		0,		0,		5,		5,		5,
	5,		5,		8,		8,		8,		8,		8,		0,
	5,		8,		5,		5,		5,		5,		5,		5
};
static uint16 RedFieldPosition[24] = {
	0,		0,		0,		0,		0,		11,		11,		11,
	11,		11,		16,		16,		16,		16,		16,		0,
	11,		16,		10,		10,		10,		10,		10,		10
};
static uint16 GreenMaskSize[24] = {
	0,		0,		0,		0,		0,		6,		6,		6,
	6,		6,		8,		8,		8,		8,		8,		0,
	5,		8,		5,		5,		5,		5,		5,		5
};
static uint16 GreenFieldPosition[24] = {
	0,		0,		0,		0,		0,		5,		5,		5,
	5,		5,		8,		8,		8,		8,		8,		0,
	5,		8,		5,		5,		5,		5,		5,		5
};
static uint16 BlueMaskSize[24] = {
	0,		0,		0,		0,		0,		5,		5,		5,
	5,		5,		8,		8,		8,		8,		8,		0,
	5,		8,		5,		5,		5,		5,		5,		5
};
static uint16 BlueFieldPosition[24] = {
	0,		0,		0,		0,		0,		0,		0,		0,
	0,		0,		0,		0,		0,		0,		0,		0,
	0,		0,		0,		0,		0,		0,		0,		0
};
static uint16 RsvdMaskSize[24] = {
	0,		0,		0,		0,		0,		0,		0,		0,
	0,		0,		8,		8,		8,		8,		8,		0,
	0,		8,		1,		1,		1,		1,		1,		1
};
static uint16 RsvdFieldPosition[24] = {
	0,		0,		0,		0,		0,		0,		0,		0,
	0,		0,		24,		24,		24,		24,		24,		0,
	0,		24,		15,		15,		15,		15,		15,		15
};

/*-------------------------------------------------------------*/

int32 BDirectDriver1::GetVideoModeInfo(	void		*cookie,
										uint16		mode,
										DD_modeInfo	*modeInfo)
{
	int				i;
	int32			index;
	BDirectDriver1	*dd;
	
	dd = (BDirectDriver1*)cookie;
	if (!dd->is_fullscreen)
		return -1;
		
	for (i=0; i<dd->mode_count; i++)
		if (dd->mode_list[i] == mode)
			goto valid_mode;
	return -1;
	
valid_mode:
	index = mode-0x200;	
	modeInfo->Attributes = 0x0002 | 0x0008;
	if (dd->hooks[5] || dd->hooks[7])
		modeInfo->Attributes |= 0x0010;
	if (dd->hooks[0] && dd->hooks[1] && dd->hooks[2])
		modeInfo->Attributes |= 0x0040;
	modeInfo->XResolution = XResolution[index];
	modeInfo->YResolution = YResolution[index];
	modeInfo->BytesPerScanLine = 0;
	modeInfo->BitsPerPixel = BitsPerPixel[index];
	modeInfo->MaxBuffers = 1;
	
	modeInfo->RedMaskSize = RedMaskSize[index];
	modeInfo->RedFieldPosition = RedFieldPosition[index];
	modeInfo->GreenMaskSize = GreenMaskSize[index];
	modeInfo->GreenFieldPosition = GreenFieldPosition[index];
	modeInfo->BlueMaskSize = BlueMaskSize[index];
	modeInfo->BlueFieldPosition = BlueFieldPosition[index];
	modeInfo->RsvdMaskSize = RsvdMaskSize[index];
	modeInfo->RsvdFieldPosition = RsvdFieldPosition[index];
	
	modeInfo->MaxBytesPerScanLine = 0;
	modeInfo->MaxScanLineWidth = 0;
	
	modeInfo->LinBytesPerScanLine;
	modeInfo->BnkMaxBuffers = 1;
	modeInfo->LinMaxBuffers = 1;
	modeInfo->LinRedMaskSize = RedMaskSize[index];
	modeInfo->LinRedFieldPosition = RedFieldPosition[index];
	modeInfo->LinGreenMaskSize = GreenMaskSize[index];
	modeInfo->LinGreenFieldPosition = GreenFieldPosition[index];
	modeInfo->LinBlueMaskSize = BlueMaskSize[index];
	modeInfo->LinBlueFieldPosition = BlueFieldPosition[index];
	modeInfo->LinRsvdMaskSize = RsvdMaskSize[index];
	modeInfo->LinRsvdFieldPosition = RsvdFieldPosition[index];
	
	modeInfo->MaxDotClock = 0;
	modeInfo->DotClockScaleFactor = 0;
	modeInfo->IntDotClockScaleFactor = 0;
	
	modeInfo->VideoCapabilities = 0;
	if (dd->available_space & 0x00801f)
		modeInfo->VideoCapabilities |= 0x04;
	if (dd->available_space & 0xfc0000)
		modeInfo->VideoCapabilities |= 0x08;
	if (dd->available_space & 0x0103e0)
		modeInfo->VideoCapabilities |= 0x10;
	if (dd->available_space & 0x027c00)
		modeInfo->VideoCapabilities |= 0x40;
	modeInfo->HW3DCapabilities = 0;
	modeInfo->VideoMinXScale = 1;
	modeInfo->VideoMinYScale = 1;
	modeInfo->VideoMaxXScale = 1;
	modeInfo->VideoMaxYScale = 1;
	
	for (i=0; i<70; i++)
		modeInfo->reserved[i] = 0;
	return 0;
}

/*-------------------------------------------------------------*/

int32 BDirectDriver1::SetVideoMode(	void		*cookie,
									uint16		mode,
									int32		virtualX,
									int32		virtualY,
									int32		*bytesPerLine,
									int32		numBuffers,
									DD_CRTCInfo	*crtc)
{
	int				i;
	status_t		err;
	BDirectDriver1	*dd;
	
	dd = (BDirectDriver1*)cookie;
	if (!dd->is_fullscreen)
		return -1;
		
	for (i=0; i<dd->mode_count; i++)
		if (dd->mode_list[i] == mode)
			goto valid_mode;
	return -1;
	
valid_mode:
	err = dd->SetSpace(dd->dd_token, 1<<(mode-0x200), virtualX, virtualY);
	if (err != B_NO_ERROR)
		return -1;
	
	err = dd->ControlDriver(dd->dd_token, B_GET_GRAPHICS_CARD_INFO,
							sizeof(graphics_card_info), (void*)&dd->card_info);
	if (err != B_NO_ERROR)
		return -1;
	*bytesPerLine = dd->card_info.bytes_per_row;
	dd->depth = dd->card_info.bits_per_pixel;
	return 0;
}

/*-------------------------------------------------------------*/

void BDirectDriver1::SetDisplayStart(void		*cookie,
									int32		x,
									int32		y,
									int32		waitVRT)
{
	BDirectDriver1		*dd;
	frame_buffer_info	info;
	
	dd = (BDirectDriver1*)cookie;
	if (!dd->is_fullscreen)
		return;
		
	info.display_x = x;
	info.display_y = y;
	dd->ControlDriver(dd->dd_token, B_MOVE_DISPLAY_AREA,
					  sizeof(frame_buffer_info), (void*)&info);
}

/*-------------------------------------------------------------*/

void BDirectDriver1::SetPaletteData(void		*cookie,
									DD_palette	*pal,
									int32		num,
									int32		index,
									int32		waitVRT)
{
	int				i, from, to;
	indexed_color   col;       
	BDirectDriver1	*dd;
	
	dd = (BDirectDriver1*)cookie;
	if (!dd->is_fullscreen)
		return;
		
	from = num;
	to = num+index;
	if (from < 0)
		from = 0;
	if (to > 256)
		to = 256;
	if (from >= to)
		return;
		
	if (dd->addon_state == 1) {
		for (i=from; i<to; i++)
			col.index = i;
			col.color.red = pal[from-num].red;
			col.color.green = pal[from-num].green;
			col.color.blue = pal[from-num].blue;
			col.color.alpha = pal[from-num].alpha;
			(dd->addon_ctrl_jmp)(B_SET_INDEXED_COLOR, (void*)&col);
		return;
	}
	else {
		_BAppServerLink_	link;
		
		link.session->swrite_l(GR_SET_COLOR_MAP);
		link.session->swrite_l(dd->dd_token);
		link.session->swrite_l(from);
		link.session->swrite_l(to);
		for (i=from; i<to; i++) {
			col.index = i;
			col.color.red = pal[from-num].red;
			col.color.green = pal[from-num].green;
			col.color.blue = pal[from-num].blue;
			col.color.alpha = pal[from-num].alpha;
			link.session->swrite(sizeof(indexed_color), (void*)&col);
		}
		link.session->flush();
	}
}

/*-------------------------------------------------------------*/

void BDirectDriver1::SetCursor(	void			*cookie,
								DD_monoCursor	*cursor,
								uint8 			red,
								uint8			green,
								uint8			blue)
{
	BDirectDriver1	*dd;
	
	dd = (BDirectDriver1*)cookie;
	if (!dd->is_fullscreen)
		return;
	((VGA_SET_CURSOR_SHAPE)dd->hooks[0])
		((uchar*)cursor->xorMask, (uchar*)cursor->andMask,
		 32, 32, cursor->hotx, cursor->hoty);
}

/*-------------------------------------------------------------*/

void BDirectDriver1::SetCursorPos(	void		*cookie,
									int32		x,
									int32		y)
{
	BDirectDriver1	*dd;
	
	dd = (BDirectDriver1*)cookie;
	if (!dd->is_fullscreen)
		return;
	((VGA_MOVE_CURSOR)dd->hooks[1])(x, y);
}

/*-------------------------------------------------------------*/

void BDirectDriver1::ShowCursor(	void		*cookie,
									int32		visible)				
{
	BDirectDriver1	*dd;
	
	dd = (BDirectDriver1*)cookie;
	if (!dd->is_fullscreen)
		return;
	((VGA_SHOW_CURSOR)dd->hooks[2])(visible);
}

/*-------------------------------------------------------------*/

void BDirectDriver1::WaitTillIdle(	void		*cookie)
{
	BDirectDriver1	*dd;
	
	dd = (BDirectDriver1*)cookie;
	((VGA_SYNCHRO)dd->hooks[10])();
}

/*-------------------------------------------------------------*/

void BDirectDriver1::SetMix(	void		*cookie,
								int32		mix)
{
	BDirectDriver1	*dd;
	
	dd = (BDirectDriver1*)cookie;
	dd->drawing_mix = mix;
}

/*-------------------------------------------------------------*/

void BDirectDriver1::SetClipRect(void		*cookie,
								int32		minx,
								int32		miny,
								int32		maxx,
								int32		maxy)
{
	BDirectDriver1	*dd;
	
	dd = (BDirectDriver1*)cookie;
	dd->clip_rect.left = minx;
	dd->clip_rect.top = miny;
	dd->clip_rect.right = maxx;
	dd->clip_rect.bottom = maxy;
}

/*-------------------------------------------------------------*/

void BDirectDriver1::DrawRect(	void		*cookie,
								uint32		color,
								int32		left,
								int32		top,
								int32		width,
								int32		height)
{
	int32			right, bottom;
	BDirectDriver1	*dd;
	
	dd = (BDirectDriver1*)cookie;
	right = left+width-1;
	bottom = top+height-1;
	if (left < dd->clip_rect.left)
		left = dd->clip_rect.left;
	if (top < dd->clip_rect.top)
		top = dd->clip_rect.top;
	if (right > dd->clip_rect.right)
		right = dd->clip_rect.right;
	if (bottom > dd->clip_rect.bottom)
		bottom = dd->clip_rect.bottom;
	if ((left > right) || (top > bottom))
		return;
	switch (dd->depth) {
	case 8 :
		((VGA_RECT_8)dd->hooks[5])(left, top, right, bottom, color);
		break;
	case 15 :
	case 16 :
		((VGA_RECT_16)dd->hooks[13])(left, top, right, bottom, color);
		break;
	case 32 :
		((VGA_RECT_24)dd->hooks[6])(left, top, right, bottom, color);
		break;
	}
}

/*-------------------------------------------------------------*/

void BDirectDriver1::DrawLine(	void		*cookie,
								uint32		color,
								DD_fix32	x1,
								DD_fix32	y1,
								DD_fix32	x2,
								DD_fix32	y2,
								int32		drawLast)
{
	BDirectDriver1	*dd;
	
	dd = (BDirectDriver1*)cookie;
	switch (dd->depth) {
	case 8 :
		((VGA_LINE_8)dd->hooks[3])(x1, y1, x2, y2, color, 1,
					   dd->clip_rect.left, dd->clip_rect.top,
					   dd->clip_rect.right, dd->clip_rect.bottom);
		break;
	case 15 :
	case 16 :
		((VGA_LINE_16)dd->hooks[12])(x1, y1, x2, y2, color, 1,
						dd->clip_rect.left, dd->clip_rect.top,
						dd->clip_rect.right, dd->clip_rect.bottom);
		break;
	case 32 :
		((VGA_LINE_24)dd->hooks[4])(x1, y1, x2, y2, color, 1,
					   dd->clip_rect.left, dd->clip_rect.top,
					   dd->clip_rect.right, dd->clip_rect.bottom);
		break;
	}
}

/*-------------------------------------------------------------*/

void BDirectDriver1::BitBlt(	void		*cookie,
							int32		left,
							int32		top,
							int32		width,
							int32		height,
							int32		dstLeft,
							int32		dstTop,
							int32		op)
{
	BDirectDriver1	*dd;
	
	dd = (BDirectDriver1*)cookie;
	((VGA_BLIT)dd->hooks[7])(left, top, dstLeft, dstTop, width, height);
}
