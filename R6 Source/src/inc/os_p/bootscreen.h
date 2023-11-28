/* ++++++++++
	FILE:	bootscreen.h
	REVS:	$Revision$
	NAME:	herold
	DATE:	Thu Feb 20 11:06:16 PST 1997
	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.
+++++ */

#ifndef _BOOTSCREEN_H
#define _BOOTSCREEN_H

#include <SupportDefs.h>

/* for maximum compatibility, add new fields to the end of the structure */

typedef struct {
	void		*base;		/* -> screen */
	long		width;		/* screen # pixels/line */
	long		height;		/* screen # rows */
	long		rowbyte;	/* screen row bytes */
	float 		refresh;	/* refresh rate */
	long		depth;		/* # bit/pixel */
	bool		use_stub;	/* flag: use stub graphics driver */
} screen;

#endif
