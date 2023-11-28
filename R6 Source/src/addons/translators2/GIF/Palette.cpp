//******************************************************************************
//
//	File:			Palette.cpp
//
//	Copyright 1999, Be Incorporated, All Rights Reserved.
//  Copyright 1998-1999, Daniel Switkin
//
//******************************************************************************

#include "Palette.h"

Palette::Palette() {
	pal = new rgb_color[256];
	backgroundindex = 0;
	usetransparent = false;
	transparentindex = 0;
	size = size_in_bits = 0;
}

// Bring over important state but don't copy the palette itself
Palette::Palette(const Palette &copy) {
	pal = new rgb_color[256];
	backgroundindex = copy.backgroundindex;
	usetransparent = copy.usetransparent;
	transparentindex = copy.transparentindex;
	size = size_in_bits = 0;
}

// Never index into pal directly - this function is safe
rgb_color Palette::ColorForIndex(int index) {
	if (index >= 0 && index <= size) {
		if (usetransparent && index == transparentindex) return B_TRANSPARENT_32_BIT;
		else return pal[index];
	} else {
		rgb_color color = {0, 0, 0, 0xff};
		return color;
	}
}

Palette::~Palette() {
	delete [] pal;
}