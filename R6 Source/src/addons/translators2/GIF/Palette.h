//******************************************************************************
//
//	File:			Palette.h
//
//	Copyright 1999, Be Incorporated, All Rights Reserved.
//  Copyright 1998-1999, Daniel Switkin
//
//******************************************************************************

#ifndef PALETTE_H
#define PALETTE_H

#include <GraphicsDefs.h>

class Palette {
	public:
		Palette();
		Palette(const Palette &copy);
		rgb_color ColorForIndex(int index);
		~Palette();

		rgb_color *pal;
		int size, size_in_bits;
		bool usetransparent;
		int backgroundindex, transparentindex;
};

#endif
