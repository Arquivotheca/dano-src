//========================================================================
//	MHiliteColor.cpp
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include "MHiliteColor.h"

// rgb_color hilite_color = { 216, 216, 216, 255 };
rgb_color hilite_color = { 0, 0, 255, 0 };

// ---------------------------------------------------------------------------
//		HiliteColor
// ---------------------------------------------------------------------------
//	rgb_color is a 4 byte struct and is read and written atomically so
//	doesn't need to be protected by a semaphore.

rgb_color
HiliteColor()
{
	return hilite_color;
}

// ---------------------------------------------------------------------------
//		SetHiliteColor
// ---------------------------------------------------------------------------

void
SetHiliteColor(
	rgb_color inHiliteColor)
{
	hilite_color = inHiliteColor;
}
