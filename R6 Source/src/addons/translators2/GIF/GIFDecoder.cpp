//******************************************************************************
//
//	File:			GIFDecoder.cpp
//
//	Copyright 1999, Be Incorporated, All Rights Reserved.
//  Copyright 1998-1999, Daniel Switkin
//
//******************************************************************************

#include "GIFDecoder.h"
#include "GIFLoad.h"
#include <stdio.h>

GIFDecoder::GIFDecoder(color_space colors) {
	input = new BMallocIO();
	gifload = NULL;
	bitmap = NULL;
	if (colors != B_RGBA15 && colors != B_RGBA32) this->colors = B_RGBA32;
	else this->colors = colors;
	
	is_animated = is_transparent = needs_background_repainted = false;
	duration = 0;
}

GIFDecoder::~GIFDecoder() {
	if (bitmap) delete bitmap;
	if (gifload) delete gifload;
	if (input) delete input;
}

void GIFDecoder::AppendData(char *buf, size_t len, bool more) {
	if (!input) return;
	input->Write(buf, len);
	if (!more) {
		gifload = new GIFLoad(input, this);
		// Clean up single frame GIFs immediately
		if (gifload->fatalerror) {
			UpdateCachedState();
			delete input;
			input = NULL;
			delete gifload;
			gifload = NULL;
		}
	}
}

bool GIFDecoder::DecodeNextFrame() {
	if (!gifload) return false;
	bool result = gifload->DecodeNextFrame();
	// Clean up multiple frame GIFs as soon as possible
	if (!result) {
		UpdateCachedState();
		delete input;
		input = NULL;
		delete gifload;
		gifload = NULL;
	}
	return result;
}

BBitmap *GIFDecoder::CurrentFrame() const {
	return bitmap;
}

bigtime_t GIFDecoder::Duration() {
	if (gifload) duration = gifload->duration;
	return duration;
}

size_t GIFDecoder::Size() const {
	size_t result = 0;
	if (bitmap) result = bitmap->BitsLength();
	if (input) result += input->BufferLength();
	if (gifload) result += sizeof(GIFLoad);
	return result;
}

bool GIFDecoder::IsAnimated() {
	if (gifload) is_animated = gifload->animated;
	return is_animated;
}

bool GIFDecoder::IsTransparent() {
	if (gifload) is_transparent = gifload->IsTransparent();
	return is_transparent;
}

bool GIFDecoder::NeedsBackgroundRepainted() {
	if (gifload) needs_background_repainted = gifload->NeedsBackgroundRepainted();
	return needs_background_repainted;
}

color_space GIFDecoder::ColorSpace() const {
	return colors;
}

void GIFDecoder::UpdateCachedState() {
	Duration();
	IsAnimated();
	IsTransparent();
	NeedsBackgroundRepainted();
}
