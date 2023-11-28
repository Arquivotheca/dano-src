
//******************************************************************************
//
//	File:		PictureTranslation.h
//
//	Description:	Deals with translation between new and old
//				picture formats.  Also, structures common to
//				both Picture.cpp and PictureTranslation.cpp
//				are here.
//	
//	Written by:	George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#ifndef PICTURE_TRANSLATION
#define PICTURE_TRANSLATION

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif

#include "shared_support.h"

class BPicture;

struct _BPictureExtent_ {
	void *				data;
	int32				size;
	void *				oldData;
	int32				oldSize;
	BArray<BPicture*>	pictureLib;
};

void swap_data(void *ptr, int32 size);

void convert_old_to_new(
	void *oldPtr, int32 oldSize,
	void **newPtr, int32 *newSize);

void convert_new_to_old(
	void *newPtr, int32 newSize,
	void **oldPtr, int32 *oldSize);

status_t do_playback(
	void *ptr, int32 size, BArray<BPicture*> &pictureLib,
	void **callbacks, int32 callbackCount,
	void *userData);

#endif
