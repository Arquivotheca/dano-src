//========================================================================
//	MBoxControl.cpp
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	Just a container to hold the syntax style views to allow navigation to 
//	this view and selection of one of the syntax style views with 
//	the arrow keys.

#include "MBoxControl.h"
#include "MBoxControlChild.h"
#include "IDEConstants.h"

#include <List.h>
#include <Debug.h>

// ---------------------------------------------------------------------------
//		MBoxControl
// ---------------------------------------------------------------------------

MBoxControl::MBoxControl(
	BRect 		inArea,
	const char*	inName)
	: BView(
		inArea,
		inName,
		B_FOLLOW_ALL_SIDES,
		B_WILL_DRAW | B_NAVIGABLE)
{
}

// ---------------------------------------------------------------------------
//		~MBoxControl
// ---------------------------------------------------------------------------

MBoxControl::~MBoxControl()
{
}

// ---------------------------------------------------------------------------
//		Draw
// ---------------------------------------------------------------------------

void
MBoxControl::Draw(
	BRect /*inArea*/)
{
	if (IsFocus())
		DrawBorder(true);
}

// ---------------------------------------------------------------------------
//		DrawBorder
// ---------------------------------------------------------------------------

void
MBoxControl::DrawBorder(
	bool inShowBorder)
{
	rgb_color	highcolor = HighColor();
	if (inShowBorder)
	{
		SetHighColor(keyboard_navigation_color());		// frame color
	}
	else
	{
		rgb_color	color;
		BView*	parent = Parent();
		if (parent)
			color = parent->LowColor();
		SetHighColor(color);			// parent's low color					
	}
	
	StrokeRect(Bounds());
	SetHighColor(highcolor);			// restorecolor					
}

// ---------------------------------------------------------------------------
//		MouseDown
// ---------------------------------------------------------------------------

void
MBoxControl::MouseDown(
	BPoint /*inWhere*/)
{
	MakeFocus(true);
}

// ---------------------------------------------------------------------------
//		KeyDown
// ---------------------------------------------------------------------------
//	Handle the arrow keys if we're the focus.  We don't handle
//	the spacebar or enter keys like some other navigable views
//	since we can't be Invoked.

void
MBoxControl::KeyDown(
	const char *	inBytes, 
	int32 			inNumBytes)
{
	if (IsFocus())
	{
		switch (inBytes[0])
		{
			case B_UP_ARROW:
			case B_DOWN_ARROW:
			case B_LEFT_ARROW:
			case B_RIGHT_ARROW:
				SelectNewChildView(inBytes[0]);
				break;
		}
	}
	
	BView::KeyDown(inBytes, inNumBytes);
}

// ---------------------------------------------------------------------------
//		SelectNewChildView
// ---------------------------------------------------------------------------
//	Select the next or previous child view.  The input key must
//	be either B_UP_ARROW or B_DOWN_ARROW.

void
MBoxControl::SelectNewChildView(
	uint32 inKey)
{
	BList				viewList;
	BView*				view;
	MBoxControlChild*	childView;
	int32				index = 0;

	// Make a list of the syntax views
	while ((view = ChildAt(index++)) != nil)
	{
		childView = dynamic_cast<MBoxControlChild*>(view);
		if (childView != nil)
			viewList.AddItem(view);
	}
	
	ASSERT(viewList.CountItems() != 0);
	
	// Find the currently selected view
	index = 0;
	while (index < viewList.CountItems())
	{
		childView = static_cast<MBoxControlChild*>(viewList.ItemAt(index));
		if (childView->IsEnabled())
		{
			childView->SetEnabled(false);
			break;
		}
		index++;
	}

	// Select the new view
	switch (inKey)
	{
		case B_UP_ARROW:
		case B_LEFT_ARROW:
			if (index == 0)
				index = viewList.CountItems() - 1;
			else
				index--;
			break;

		case B_DOWN_ARROW:
		case B_RIGHT_ARROW:
			if (index >= viewList.CountItems() - 1)
				index = 0;
			else
				index++;
			break;
	}

	ASSERT(index >= 0 && index < viewList.CountItems());

	childView = static_cast<MBoxControlChild*>(viewList.ItemAt(index));
	childView->Invoke();
}

// ---------------------------------------------------------------------------
//		MakeFocus
// ---------------------------------------------------------------------------
//	Show or hide the border when we gain or lose focus.

void
MBoxControl::MakeFocus(
	bool inBecomeFocus)
{
	DrawBorder(inBecomeFocus);			

	BView::MakeFocus(inBecomeFocus);
}

