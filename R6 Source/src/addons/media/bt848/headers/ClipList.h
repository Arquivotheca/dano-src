/*
	
	ClipList.h
	
	Copyright 1997 Be Incorporated, All Rights Reserved.
	
*/

#ifndef _CLIP_LIST_H
#define _CLIP_LIST_H

#include <OS.h>
#include <Rect.h>
#include <malloc.h>
#include <Region.h>
#include <string.h>
#include <bt848_driver.h>

#include "VideoDefs.h"

status_t			ClipListFromRegion(BRegion *clip_region, BRect frame, bt848_cliplist clip_list);

#endif
