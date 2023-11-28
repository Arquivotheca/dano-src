/*
	Copyright 2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Bitmap.h>
#include <Debug.h>
#include <View.h>
#include <Window.h>
#include "DProxy.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// bitmap dumping addon

extern "C" status_t DumpVariable(DProxy& proxy);


class TmpStorage {
public:
	TmpStorage()
		:	ptr(0)
		{}
	~TmpStorage()
		{ free(ptr); }
		
	void *Use(size_t size)
		{
			free(ptr);
			return malloc(size);
		}

private:
	void *ptr;
};

// some stuff ripped out of BBitmap we will use to get at
// the bitmap fields directly
class BBitmapOverlay : public BArchivable {
public:
	void		*fBasePtr;
	int32		fSize;
	color_space	fType;
	BRect		fBound;
	int32		fRowBytes;
	BWindow		*fWindow;
	int32		fServerToken;
	int32		fToken;
	uint8		unused;
	area_id		fArea;
	area_id		fOrigArea;
	uint32		fFlags;
	status_t	fInitError;
};

struct _offscreen_data_ {
	sem_id	lock;
	void *	bits;
	area_id shadow;
	int32	state;
};

status_t
DumpVariable(DProxy& proxy)
{
	ptr_t addr = proxy.VariableAddress();
	if (proxy.IsPointer()) {
		proxy.Dereference();
		addr = proxy.VariableAddress();
	}
	
	size_t size = proxy.VariableSize();
	if (size != sizeof(BBitmapOverlay)) {
		printf("not a bitmap, size %ld, bitmap size %ld (%ld)\n",
			size, sizeof(BBitmapOverlay), sizeof(BBitmap));
		printf("I'm just going to force the size to be %d for now\n", sizeof(BBitmap));
		size = sizeof(BBitmap);
	}
	
	BBitmapOverlay overlay;
	status_t result = proxy.ReadData(addr, &overlay, size);
	if (result != B_OK) {
		printf("error %s reading bitmap struct\n", strerror(result));
		return result;
	}
	
	overlay.fBound.PrintToStream();

	if (overlay.fSize > 10 * 1024 * 1024) {
		printf("bitmap too large to dump, size %ld\n", overlay.fSize);
		return B_OK;
	}

	BBitmap bitmap(overlay.fBound, overlay.fType);
	result = bitmap.InitCheck();
	if (result != B_OK) {
		printf("error %s creating clone bitmap\n", strerror(result));
		return result;
	}
		
	if (bitmap.Bits()) {
		void *bits = overlay.fBasePtr;
		if (overlay.fBasePtr && (overlay.fFlags & B_BITMAP_IS_OFFSCREEN)) {
			_offscreen_data_ tmp;
			proxy.ReadData((ptr_t)overlay.fBasePtr, (void *)&tmp, sizeof(_offscreen_data_));
			bits = tmp.bits;
		}
		if (bitmap.BitsLength() < overlay.fSize) {
			printf("bitmap wrong size, expected %ld, got %ld\n", 
				bitmap.BitsLength(), overlay.fSize);
			return B_ERROR;
		}
		
		printf("getting bits %p, size %ld\n", bits, overlay.fSize);
		result = proxy.ReadData((ptr_t)bits, bitmap.Bits(), overlay.fSize);
		if (result != B_OK) {
			printf("failed to read bits %s\n", strerror(result));
			return result;
		}
//		bitmap.SetBits(tmp, overlay.fSize, 0, overlay.fType);
//		free(tmp);
	}
	BWindow *window = new BWindow(bitmap.Bounds().OffsetToSelf(100, 100),
		"bitmap", B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE);
	BView *view = new BView(bitmap.Bounds(), "bits", B_FOLLOW_ALL, B_WILL_DRAW);
	window->AddChild(view);
	view->SetViewBitmap(&bitmap, bitmap.Bounds(), bitmap.Bounds());
	window->Show();
	
	return B_OK;
}
