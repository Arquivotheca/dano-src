/* ++++++++++
	FILE:	macscreen.h
	REVS:	$Revision$
	NAME:	bronson
	DATE:	Wed Oct 30 14:48:35 PST 1996
	Copyright (c) 1996-1997 by Be Incorporated.  All Rights Reserved.
+++++ */

#ifndef _MACSCREEN_H
#define _MACSCREEN_H


typedef struct {
	void		*screen_base;	/* -> screen */
	long		screen_row;		/* screen row bytes */
	long		screen_height;	/* screen # rows */
	long		screen_width;	/* screen # pixels/line */
	float 		refresh;
	long		bit_depth;		/* # bit/pixel */
} default_screen_info;

#endif
