//========================================================================
//	MPictureMenuBar.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include "MPictureMenuBar.h"
#include <Picture.h>

// ---------------------------------------------------------------------------
//		MPictureMenuBar
// ---------------------------------------------------------------------------
//	Constructor

MPictureMenuBar::MPictureMenuBar(
	BRect			frame,
	const char *	title,
	BPicture*		inOnPicture,
	BPicture*		inOffPicture,
	uint32 			resizeMask,
	menu_layout 	layout,
	bool 			resizeToFit)
	: BMenuBar(frame, title, resizeMask, layout, resizeToFit),
	fOnPicture(inOnPicture),
	fOffPicture(inOffPicture)
{
}

// ---------------------------------------------------------------------------
//		~MFindWindow
// ---------------------------------------------------------------------------
//	Destructor

MPictureMenuBar::~MPictureMenuBar()
{
	delete fOnPicture;
	delete fOffPicture;
}

// ---------------------------------------------------------------------------
//		Draw
// ---------------------------------------------------------------------------
//	Draw the correct picture depending on whether we're enabled or disabled.

void
MPictureMenuBar::Draw(
	BRect 			inFrame)
{
	if (IsEnabled() && fOnPicture)
	{
		DrawPicture(fOnPicture, B_ORIGIN);
	}
	else
	if (fOffPicture)
		DrawPicture(fOffPicture, B_ORIGIN);
	else	
		FillRect(inFrame);
}

// ---------------------------------------------------------------------------
//		MouseDown
// ---------------------------------------------------------------------------
//	Ignore mousedowns if we're disabled.

void
MPictureMenuBar::MouseDown(
	BPoint		inWhere)
{
	if (IsEnabled())
		BMenuBar::MouseDown(inWhere);
}

// ---------------------------------------------------------------------------
//		GetPictures
// ---------------------------------------------------------------------------

void
MPictureMenuBar::GetPictures(
	BPicture*& inOnPicture,
	BPicture*& inOffPicture)
{
	inOnPicture = fOnPicture;
	inOffPicture = fOffPicture;
}
