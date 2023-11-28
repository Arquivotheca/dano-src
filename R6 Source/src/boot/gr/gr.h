/* ++++++++++
	FILE:	gr.h
	REVS:	$Revision: 1.11 $
	NAME:	herold
	DATE:	Fri Aug 18 14:10:21 PDT 1995
	Copyright (c) 1995 by Be Incorporated.  All Rights Reserved.
+++++ */

#ifndef _GR_H
#define _GR_H

#include <SupportDefs.h>
#include <bootscreen.h>

/* -----
	types
----- */

typedef struct {
	long	firstchar;
	long	lastchar;
	long	width_max;
	long	font_height;
	long	ascent;
	long	descent;
	long	leading;
	long	bm_rowbyte;
	long	default_width;
	uchar	*bitmap;
	short	*offsets;
	char	proportional;
	char	filler1;
} font_desc;

typedef	struct {
	short	left;
	short	top;
	short	right;
	short	bottom;
} rect;

/* -----
	definitions
----- */

typedef enum {
	
	BLACK = 0,
	DARK_GRAY = 9,
	LIGHT_GRAY = 22,
	WHITE = 31,

#if MAC
	RED = 43,
	BLUE = 94
#else
	RED = 20 + 32,
	BLUE = 20 + 64
#endif

} bt_color;


/* -----
	interface
----- */

extern uchar	*unpack(uchar *packed, long *size);
extern long		text (
					long		h,
					long		v,
					char		*str,
					font_desc	*desc,
					bt_color    fore_color,
					bt_color    back_color
				);

extern void		fill_rect (rect r, bt_color color);
extern void		invert_rect (rect r);
extern void		blit (uchar *data, long xs, long ys, long x, long y);
extern void		move_rect (rect src, int dst_x, int dst_y);
extern void		h_line (long x1, long x2, long y1, bt_color color);
extern void		v_line (long x1, long y1, long y2, bt_color color);
extern void		gr_clear_screen (void);
extern screen	*get_main_screen (void);

extern font_desc Kate;

#if MAC
extern uint32 color_table[256];
#endif

#endif

