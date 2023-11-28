/*******************************************************************************
//
//	File:		DirectDriver.h
//
//	Description:	Private class to encapsulate init/dispose of client-side
//					graphic add-on clone.
//
//	Copyright 1998, Be Incorporated, All Rights Reserved.
//
//*****************************************************************************/

#ifndef	_DIRECT_DRIVER_H
#define	_DIRECT_DRIVER_H

#ifndef _DIRECT_WINDOW_H
#include <DirectWindow.h>
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
#include <WindowScreen.h>

/* List of hooks available in the direct driver API */
enum driver_hook_token {
	/* configuration hooks (usable only in full-screen mode, when available). */
	DD_GET_VIDEO_MODE_AT	= 0,
	DD_GET_VIDEO_MODE_INFO	= 1,
	DD_SET_VIDEO_MODE		= 2,
	DD_SET_DISPLAY_START	= 3,
	DD_SET_PALETTE_DATA		= 4,
	DD_SET_CURSOR			= 5,
	DD_SET_CURSOR_POS		= 6,
	DD_SHOW_CURSOR			= 7,

	/* drawing hooks (always usable, when available) */
	DD_WAIT_TILL_IDLE		= 1024,
	DD_SET_MIX				= 1025,
	DD_SET_CLIP_RECT		= 1026,
	DD_DRAW_RECT			= 1027,
	DD_DRAW_LINE			= 1028,
	DD_BIT_BLT				= 1029
};

//		status_t		GetDriverHook(driver_hook_token token, graphics_card_hook *hook) const;
//		status_t		GetDriverCookie(void **cookie) const;

/*******************************************************************************
//
//	Direct Driver API, prototypes and declarations.
//
//*****************************************************************************/

/* direct driver specific types */
typedef struct {
	uint16		Attributes;
	uint16		XResolution;
	uint16		YResolution;
	uint16		BytesPerScanLine;
	uint16		BitsPerPixel;
	uint16		MaxBuffers;
	
	/* RGB pixel format info */
	uint8		RedMaskSize;
	uint8		RedFieldPosition;
	uint8		GreenMaskSize;
	uint8		GreenFieldPosition;
	uint8		BlueMaskSize;
	uint8		BlueFieldPosition;
	uint8		RsvdMaskSize;
	uint8		RsvdFieldPosition;
	
	/* Virtual buffer dimensions */
	uint16		MaxBytesPerScanLine;
	uint16		MaxScanLineWidth;
	
	/* Linear buffer mode information */
	uint16		LinBytesPerScanLine;
	uint8		BnkMaxBuffers;
	uint8		LinMaxBuffers;
	uint8		LinRedMaskSize;
	uint8		LinRedFieldPosition;
	uint8		LinGreenMaskSize;
	uint8		LinGreenFieldPosition;
	uint8		LinBlueMaskSize;
	uint8		LinBlueFieldPosition;
	uint8		LinRsvdMaskSize;
	uint8		LinRsvdFieldPosition;
	
	/* Refresh rate control information */
	uint16		MaxDotClock;
	uint16		DotClockScaleFactor;
	uint16		IntDotClockScaleFactor;
	
	/* Hardware video and 3D information */
	uint32		VideoCapabilities;
	uint32		HW3DCapabilities;
	uint16		VideoMinXScale;
	uint16		VideoMinYScale;
	uint16		VideoMaxXScale;
	uint16		VideoMaxYScale;
	
	uint8		reserved[70];
} DD_modeInfo;

typedef struct {
	uint16		HorizontalTotal;
	uint16		HorizontalSyncStart;
	uint16		HorizontalSyncEnd;
	uint16		VerticalTotal;
	uint16		VerticalSyncStart;
	uint16		VerticalSyncEnd;
	uint8		Flags;
	uint16		PhysicalDotClock;
	uint16		RefreshRate;
	uint16		NumBuffers;
} DD_CRTCInfo;

typedef struct {
	uint8		blue;
	uint8		green;
	uint8		red;
	uint8		alpha;
} DD_palette;

typedef struct {
	uint32		xorMask[32];
	uint32		andMask[32];
	uint32		hotx;
	uint32		hoty;
} DD_monoCursor;

typedef int32 DD_fix32;

/* direct driver specific enums */
enum {
	/* mode info attributes flags */
	DD_HAVE_MULTI_BUFFER	= 0x0001,
	DD_HAVE_VIRTUAL_SCROLL	= 0x0002,
	DD_HAVE_BANKED_BUFFER	= 0x0004,
	DD_HAVE_LINEAR_BUFFER	= 0x0008,
	DD_HAVE_ACCEL_2D		= 0x0010,
	DD_HAVE_DUAL_BUFFERS	= 0x0020,
	DD_HAVE_HW_CURSOR		= 0x0040,
	DD_HAVE_8_BIT_DAC		= 0x0080,
	DD_NON_VGA_MODE			= 0x0100,
	DD_HAVE_DOUBLE_SCAN		= 0x0200,
	DD_HAVE_INTERLACED		= 0x0400,
	DD_HAVE_TRIPLE_BUFFER	= 0x0800,
	DD_HAVE_STEREO			= 0x1000,
	DD_HAVE_ROP3			= 0x2000
};

enum {
	/* video capabilities flags */
	DD_VIDEO_X_INTERP	= 0x00000001,
	DD_VIDEO_Y_INTERP	= 0x00000002,
	DD_VIDEO_RGB_332	= 0x00000004,
	DD_VIDEO_RGB_555	= 0x00000008,
	DD_VIDEO_RGB_565	= 0x00000010,
	DD_VIDEO_RGB_888	= 0x00000020,
	DD_VIDEO_ARGB_8888	= 0x00000040,
	DD_VIDEO_YUV_9		= 0x00000080,
	DD_VIDEO_YUV_12		= 0x00000100,
	DD_VIDEO_YUV_411	= 0x00000200,
	DD_VIDEO_YUV_422	= 0x00000400,
	DD_VIDEO_YUV_444	= 0x00000800,
	DD_VIDEO_YCRCB_422	= 0x00001000,
	DD_VIDEO_YUYV		= 0x10000000,
	DD_VIDEO_YVYU		= 0x20000000,
	DD_VIDEO_UYVY		= 0x40000000,
	DD_VIDEO_VYUY		= 0x80000000
};

enum {
	/* 3d capabilities flags */
	DD_3D_SMOOTH_SHADE		= 0x0001,
	DD_3D_SMOOTH_TEXTURE	= 0x0002,
	DD_3D_PERSPECTIVE		= 0x0004,
	DD_3D_POINT_SAMPLE		= 0x0008,
	DD_3D_BILINEAR			= 0x0010,
	DD_3D_TRILINEAR			= 0x0020,
	DD_3D_MIPMAP			= 0x0040,
	DD_3D_ZBUFFER_16		= 0x0080,
	DD_3D_ZBUFFER_32		= 0x0100
};

enum {
	/* video mode setting flags */
	DD_DONT_CLEAR 		= 0x8000,
	DD_LINEAR_BUFFER 	= 0x4000,
	DD_MULTI_BUFFER 	= 0x2000,
	DD_VIRTUAL_SCROLL 	= 0x1000,
	DD_REFRESH_CTRL 	= 0x0800
};

enum {
	/* list of mix drawing mode */
	DD_REPLACE_MIX		= 0x00,
	DD_AND_MIX			= 0x01,
	DD_OR_MIX			= 0x02,
	DD_XOR_MIX			= 0x03,
	DD_NOP_MIX			= 0x04,
	DD_R2_BLACK			= 0x10,
	DD_R2_NOT_MERGE_SRC	= 0x11,
	DD_R2_MASK_NOT_SRC	= 0x12,
	DD_R2_NOT_COPY_SRC	= 0x13,
	DD_R2_MASK_SRC_NOT	= 0x14,
	DD_R2_NOT			= 0x15,
	DD_R2_XOR_SRC		= 0x16,
	DD_R2_NOT_MASK_SRC	= 0x17,
	DD_R2_MASK_SRC		= 0x18,
	DD_R2_NOT_XOR_SRC	= 0x19,
	DD_R2_NOP			= 0x1a,
	DD_R2_MERGE_NOT_SRC	= 0x1b,
	DD_R2_COPY_SRC		= 0x1c,
	DD_R2_MERGE_SRC_NOT	= 0x1d,
	DD_R2_MERGE_SRC		= 0x1e,
	DD_R2_WHITE			= 0x1f
};

/* direct driver hooks prototype */
typedef int32	(*DD_GetVideoModeAt)(	void		*cookie,
										uint32		index,
										uint16		*mode);

typedef int32	(*DD_GetVideoModeInfo)(	void		*cookie,
										uint16		mode,
										DD_modeInfo	*modeInfo);

typedef int32	(*DD_SetVideoMode)(	void		*cookie,
									uint16		mode,
									int32		virtualX,
									int32		virtualY,
									int32		*bytesPerLine,
									int32		numBuffers,
									DD_CRTCInfo	*crtc);

typedef void	(*DD_SetDisplayStart)(	void		*cookie,
										int32		x,
										int32		y,
										int32		waitVRT);

typedef void	(*DD_SetPaletteData)(	void		*cookie,
										DD_palette	*pal,
										int32		num,
										int32		index,
										int32		waitVRT);
										
typedef void	(*DD_SetCursor)(void			*cookie,
								DD_monoCursor	*cursor,
								uint8 			red,
								uint8			green,
								uint8			blue);
								
typedef void	(*DD_SetCursorPos)(	void		*cookie,
									int32		x,
									int32		y);
									
typedef void	(*DD_ShowCursor)(	void		*cookie,
									int32		visible);					

typedef void	(*DD_WaitTillIdle)(	void		*cookie);

typedef void	(*DD_SetMix)(	void		*cookie,
								int32		mix);

typedef void	(*DD_SetClipRect)(	void		*cookie,
									int32		minx,
									int32		miny,
									int32		maxx,
									int32		maxy);

typedef void	(*DD_DrawRect)(	void		*cookie,
								uint32		color,
								int32		left,
								int32		top,
								int32		width,
								int32		height);

typedef void	(*DD_DrawLine)(	void		*cookie,
								uint32		color,
								DD_fix32	x1,
								DD_fix32	y1,
								DD_fix32	x2,
								DD_fix32	y2,
								int32		drawLast);

typedef void	(*DD_BitBlt)(	void		*cookie,
								int32		left,
								int32		top,
								int32		width,
								int32		height,
								int32		dstLeft,
								int32		dstTop,
								int32		op);

/*-------------------------------------------------------------*/

class BDirectDriver {
public:
		BDirectDriver(uint32 token);
virtual	~BDirectDriver();
virtual	status_t	GetHook(driver_hook_token token, graphics_card_hook *hook);
virtual	status_t	GetCookie(void **cookie);
virtual	status_t	Sync(bool fullscreen);
		status_t	GetDriverInfo(void *info);
		status_t	GetSyncInfo(void **info);

	uint32		dd_token;
};
#endif
