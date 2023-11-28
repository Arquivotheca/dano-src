/*******************************************************************************
//
//	File:		DirectDriver1.h
//
//	Description:	Private class to encapsulate init/dispose of client-side
//					graphic add-on clone.
//
//	Copyright 1998, Be Incorporated, All Rights Reserved.
//
//*****************************************************************************/

#ifndef	_DIRECT_DRIVER1_H
#define	_DIRECT_DRIVER1_H

#ifndef _DIRECT_DRIVER_H
#include <DirectDriver.h>
#endif

#ifndef _APP_DEFS_PRIVATE_H
#include <AppDefsPrivate.h>
#endif
#ifndef _MESSAGES_H
#include <messages.h>
#endif
#ifndef _TOKEN_SPACE_H
#include <token.h>
#endif
#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _MESSAGE_UTIL_H
#include <message_util.h>
#endif
#ifndef _SESSION_H
#include <session.h>
#endif

#include <OS.h>
#include <image.h>

/*-------------------------------------------------------------*/

class BDirectDriver1 : public BDirectDriver {
public:
		BDirectDriver1(uint32 token);
virtual	~BDirectDriver1();
virtual	status_t	GetHook(driver_hook_token token, graphics_card_hook *hook);
virtual	status_t	GetCookie(void **cookie);
virtual	status_t	Sync(bool fullscreen);

private:
typedef int32 (*add_on_control)(uint32,void *);
static status_t	ControlDriver(uint32 token, int32 cmd, int32 size, void *buf);
static status_t	SetSpace(	uint32		token,
							uint32		space,
							uint32		virtualX,
							uint32		virtualY);
static int32	GetVideoModeAt(	void		*cookie,
								uint32		index,
								uint16		*mode);
static int32	GetVideoModeInfo(	void		*cookie,
									uint16		mode,
									DD_modeInfo	*modeInfo);
static int32	SetVideoMode(	void		*cookie,
								uint16		mode,
								int32		virtualX,
								int32		virtualY,
								int32		*bytesPerLine,
								int32		numBuffers,
								DD_CRTCInfo	*crtc);
static void		SetDisplayStart(void		*cookie,
								int32		x,
								int32		y,
								int32		waitVRT);
static void		SetPaletteData(	void		*cookie,
								DD_palette	*pal,
								int32		num,
								int32		index,
								int32		waitVRT);
static void		SetCursor(	void			*cookie,
							DD_monoCursor	*cursor,
							uint8 			red,
							uint8			green,
							uint8			blue);
static void		SetCursorPos(	void		*cookie,
								int32		x,
								int32		y);
static void		ShowCursor(	void		*cookie,
							int32		visible);					
static void		WaitTillIdle(	void		*cookie);
static void		SetMix(	void		*cookie,
						int32		mix);
static void		SetClipRect(void		*cookie,
							int32		minx,
							int32		miny,
							int32		maxx,
							int32		maxy);
static void		DrawRect(	void		*cookie,
							uint32		color,
							int32		left,
							int32		top,
							int32		width,
							int32		height);
static void		DrawLine(	void		*cookie,
							uint32		color,
							DD_fix32	x1,
							DD_fix32	y1,
							DD_fix32	x2,
							DD_fix32	y2,
							int32		drawLast);
static void		BitBlt(	void		*cookie,
						int32		left,
						int32		top,
						int32		width,
						int32		height,
						int32		dstLeft,
						int32		dstTop,
						int32		op);
						
		char					addon_state;
		bool					is_fullscreen;
		uint16					drawing_mix;
		uint16					depth;
		uint16					mode_list[24];
		uint32					mode_count;
		uint32					available_space;		
		image_id				addon_image;
		clipping_rect			clip_rect;
		add_on_control			addon_ctrl_jmp;
		graphics_card_info		card_info;
		graphics_card_hook		hooks[B_HOOK_COUNT]; 
};
#endif
