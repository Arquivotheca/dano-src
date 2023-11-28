//========================================================================
//	MFocusBox.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include "MFocusBox.h"

// ---------------------------------------------------------------------------
//		MFocusBox
// ---------------------------------------------------------------------------
//	Constructor

MFocusBox::MFocusBox(
	BRect			inFrame,
	const char*		inName,
	uint32 			resizeMask,
	uint32 			flags)
	: BView(inFrame, inName, resizeMask, flags)
{
}

// ---------------------------------------------------------------------------
//		~MFocusBox
// ---------------------------------------------------------------------------
//	Destructor

MFocusBox::~MFocusBox()
{
}

// ---------------------------------------------------------------------------
//		Draw
// ---------------------------------------------------------------------------

void
MFocusBox::Draw(
	BRect /*updateRect*/)
{
	BRect		bounds = Bounds();
	float		offset = PenSize() - 1;

	bounds.top += offset;
	bounds.left += offset;

	StrokeRect(bounds);
}

// ---------------------------------------------------------------------------
//		AttachedToWindow
// ---------------------------------------------------------------------------

void
MFocusBox::AttachedToWindow()
{
	SetPenSize(2.0);
}

// ---------------------------------------------------------------------------
//		ParentViewColor
// ---------------------------------------------------------------------------
//	return the viewcolor of the focus box's parent view.

rgb_color
MFocusBox::ParentViewColor()
{
	rgb_color	viewColor;
	BView*		parent = Parent();

	if (parent)
		viewColor = parent->ViewColor();
	else
	{
		// If no parent, default is white
		viewColor.red = viewColor.green = viewColor.blue = viewColor.alpha = 255;
	}

	return viewColor;
}
