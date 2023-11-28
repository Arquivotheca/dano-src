/* ++++++++++
	FILE:	platform.h
	REVS:	$Revision$
	NAME:	herold
	DATE:	Thu Jun 13 11:41:06 PDT 1996
	Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.
+++++ */

#ifndef _PLATFORM_H
#define _PLATFORM_H

typedef enum {
	B_PLATFORM_BEBOX = 0,
	B_PLATFORM_MAC,
	B_PLATFORM_ATCLONE
} platform_type;

extern platform_type platform();

#endif

