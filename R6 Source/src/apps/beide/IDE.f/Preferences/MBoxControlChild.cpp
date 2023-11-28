//========================================================================
//	MBoxControlChild.cpp
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include "MBoxControlChild.h"

// ---------------------------------------------------------------------------
//		MBoxControlChild
// ---------------------------------------------------------------------------

MBoxControlChild::MBoxControlChild(
	BRect 		inArea,
	const char*	inName)
	: BView(
		inArea,
		inName,
		B_FOLLOW_LEFT | B_FOLLOW_BOTTOM,
		B_WILL_DRAW),
	MTargeter(0)
{
}

// ---------------------------------------------------------------------------
//		~MBoxControlChild
// ---------------------------------------------------------------------------

MBoxControlChild::~MBoxControlChild()
{
}

// ---------------------------------------------------------------------------
//		DrawFrame
// ---------------------------------------------------------------------------

void
MBoxControlChild::DrawFrame()
{
	const BRect		bounds = Bounds();
	
	if (fEnabled)
	{
		rgb_color	highColor = HighColor();
		SetHighColor(keyboard_navigation_color());		// frame color
		StrokeRect(Bounds());
		SetHighColor(highColor);
	}
}

// ---------------------------------------------------------------------------
//		MouseDown
// ---------------------------------------------------------------------------

void
MBoxControlChild::MouseDown(
	BPoint /*inWhere*/)
{
	if (! fEnabled)
	{
		Invoke();
	}
}

// ---------------------------------------------------------------------------
//		Invoke
// ---------------------------------------------------------------------------

void
MBoxControlChild::Invoke()
{
	SetEnabled(true);
	PostMessageToTarget();
}

// ---------------------------------------------------------------------------
//		SetEnabled
// ---------------------------------------------------------------------------

void
MBoxControlChild::SetEnabled(
	bool inEnable)
{
	if (inEnable != fEnabled)
	{
		const BRect		bounds = Bounds();

		fEnabled = inEnable;

		rgb_color	highColor = HighColor();
		
		if (inEnable)
		{
			SetHighColor(keyboard_navigation_color());		// frame color
		}
		else
		{
			rgb_color	parentViewColor;
			BView*		parent = Parent();
		
			if (parent)
				parentViewColor = parent->ViewColor();
			else
			{
				// If no parent, default is white
				parentViewColor.red = parentViewColor.green = 
				parentViewColor.blue = parentViewColor.alpha = 255;
			}

			SetHighColor(parentViewColor);
		}

		StrokeRect(bounds);
		SetHighColor(highColor);
	}
}

// ---------------------------------------------------------------------------
//		MakeFocus
// ---------------------------------------------------------------------------
//	We never become the focus.  We always make our parent the focus.

void
MBoxControlChild::MakeFocus(
	bool	inBecomeFocus)
{
	if (inBecomeFocus)
	{
		if (inBecomeFocus != Parent()->IsFocus())
			Parent()->MakeFocus(inBecomeFocus);
		if (! fEnabled)
			Invoke();
	}
}
