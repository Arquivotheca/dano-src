//========================================================================
//	MPopupMenu.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include "MPopupMenu.h"
#include "IDEMessages.h"


// ---------------------------------------------------------------------------
//		MPopupMenu
// ---------------------------------------------------------------------------

MPopupMenu::MPopupMenu(
	const char * inTitle,
	bool		radioMode,
	bool 		autoRename)
	: BPopUpMenu(inTitle, radioMode, autoRename),
	fBottomLeft(0.0, 0.0)
{
	SetFont(be_plain_font);
}

// ---------------------------------------------------------------------------
//		~MPopupMenu
// ---------------------------------------------------------------------------
//	Destructor

MPopupMenu::~MPopupMenu()
{
}

// ---------------------------------------------------------------------------
//		SetBottomLeft
// ---------------------------------------------------------------------------
//	Set the bottom left point of a popup that needs to be popped UP.
//	The point is in screen coordinates.

void
MPopupMenu::SetBottomLeft(
	BPoint	inPoint)
{
	fBottomLeft = inPoint;
}

// ---------------------------------------------------------------------------
//		ScreenLocation
// ---------------------------------------------------------------------------
//	Calculate the topleft of the popup for popups that need to be popped UP.

BPoint
MPopupMenu::ScreenLocation()
{
	if (fBottomLeft.x != 0.0 || fBottomLeft.y != 0.0)
	{
		fBottomLeft.y -= Bounds().Height();
		
		return fBottomLeft;
	}
	else
		return BPopUpMenu::ScreenLocation();
}
