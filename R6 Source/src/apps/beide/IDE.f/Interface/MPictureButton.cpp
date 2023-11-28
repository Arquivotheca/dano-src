//========================================================================
//	MPictureButton.cpp
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include "MPictureButton.h"

// ---------------------------------------------------------------------------
//		¥ MPictureButton
// ---------------------------------------------------------------------------

MPictureButton::MPictureButton(
	BRect 			frame,
	const char*		name, 
	BPicture *		off, 			   
	BPicture *		on,
	BMessage *		message, 
	uint32 			behavior,
	uint32 			resizeMask, 
	uint32 			flags)
 :	BPictureButton(
		frame,
		name,
		off,
		on,
		message,
		behavior,
		resizeMask,
		flags)
{
}

// ---------------------------------------------------------------------------
//		¥ ~MPictureButton
// ---------------------------------------------------------------------------

MPictureButton::~MPictureButton()
{
}

// ---------------------------------------------------------------------------
//		¥ Draw
// ---------------------------------------------------------------------------

void
MPictureButton::Draw(
	BRect area)
{
	BPictureButton::Draw(area);
	if ((Flags() & B_NAVIGABLE) != 0 && IsFocus())
		DrawNavigationHilite(area);
}

// ---------------------------------------------------------------------------
//		¥ DrawNavigationHilite
// ---------------------------------------------------------------------------

void
MPictureButton::DrawNavigationHilite(
	BRect area)
{
	BPoint		start(area.left + 2, area.bottom - 2);
	BPoint		end(area.right - 2, area.bottom - 2);

	rgb_color	highColor = HighColor();
	SetHighColor(0, 0, 0, 255);	// Black

	StrokeLine(start, end);

	SetHighColor(highColor);
}

// ---------------------------------------------------------------------------
//		¥ KeyDown
// ---------------------------------------------------------------------------
//	BPictureButton doesn't handle key navigation correctly for
//	a two_state_button.
//	This may be fixed in DR9.  This function isn't called
#if 0
void
MPictureButton::KeyDown(
	ulong key)
{
	if (Behavior() == B_TWO_STATE_BUTTON)
	{
		switch (key)
		{
			case B_SPACE:
			case B_ENTER:
				SetValue(1 - Value());
				Invoke();
				break;

			default:
				BPictureButton::KeyDown(key);
				break;				
		}
	}
	else
		BPictureButton::KeyDown(key);
}
#endif
