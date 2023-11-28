//******************************************************************************
//
//	File:			GIFDecoder.h
//
//	Copyright 1999, Be Incorporated, All Rights Reserved.
//  Copyright 1998-1999, Daniel Switkin
//
//******************************************************************************

#ifndef GIFDECODER_H
#define GIFDECODER_H

#include <Bitmap.h>
#include <DataIO.h>
class GIFLoad;

class GIFDecoder {
	friend class GIFLoad;
	
	public:
		GIFDecoder(color_space colors = B_RGBA32);
		~GIFDecoder();
		
		void AppendData(char *buf, size_t len, bool more);
		bool DecodeNextFrame();
		BBitmap *CurrentFrame() const;
		bigtime_t Duration();
		size_t Size() const;
		bool IsAnimated();
		bool IsTransparent();
		bool NeedsBackgroundRepainted();
		color_space ColorSpace() const;

	private:
		void UpdateCachedState();

		BMallocIO *input;
		GIFLoad *gifload;
		BBitmap *bitmap;
		color_space colors;
		bool is_animated, is_transparent, needs_background_repainted;
		int duration;
};

#endif
