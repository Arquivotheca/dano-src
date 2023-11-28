//==================================================================
//	MDLOGListView.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//==================================================================
//	A list view to be used in dialog boxes.  It posts a message
//	when it's changed.
//	BDS

#include "MDLOGListView.h"
#include "MFocusBox.h"
#include "IDEMessages.h"

#include <Looper.h>
#include <Message.h>

// ---------------------------------------------------------------------------
//		MDLOGListView
// ---------------------------------------------------------------------------
//	Constructor

MDLOGListView::MDLOGListView(
	BRect			inFrame,
	const char*		inName,
	uint32 			resizeMask,
	uint32 			flags)
	: MListView(
		inFrame,
		inName,
		resizeMask,
		flags)
{
	fFocusBox = nil;
	fTarget = nil;
}

// ---------------------------------------------------------------------------
//		~MDLOGListView
// ---------------------------------------------------------------------------
//	Destructor

MDLOGListView::~MDLOGListView()
{
}

// ---------------------------------------------------------------------------
//		SetFocusBox
// ---------------------------------------------------------------------------

void
MDLOGListView::SetFocusBox(
	MFocusBox*	inBox)
{
	fFocusBox = inBox;
	if (fFocusBox && fFocusBox->Window())
	{
		fFocusBox->SetPenSize(2.0);
		fHighColor = fFocusBox->HighColor();
	}
}

// ---------------------------------------------------------------------------
//		AttachedToWindow
// ---------------------------------------------------------------------------

void
MDLOGListView::AttachedToWindow()
{
	MListView::AttachedToWindow();
	if (fFocusBox)
	{
		fFocusBox->SetPenSize(2.0);
		fHighColor = fFocusBox->HighColor();
	}
}

// ---------------------------------------------------------------------------
//		DrawRow
// ---------------------------------------------------------------------------

void
MDLOGListView::DrawRow(
	int32 	/*inRow*/,
	void * 	inData,
	BRect	inArea,
	BRect	/*inIntersection*/)
{
	font_height		info;
	
	GetFontHeight(&info);

	MovePenTo(2.0, inArea.bottom - info.descent);
	const char *	text = (const char*) inData;
	if (text)
		DrawString(text);
}

// ---------------------------------------------------------------------------
//		MakeFocus
// ---------------------------------------------------------------------------
//	Adjust the color of the focusBox when we are made the focus or lose the
//	focus.  

void
MDLOGListView::MakeFocus(
	bool	inBecomeFocused)
{
	bool		wasFocus = IsFocus();
	
	if (fFocusBox && wasFocus != inBecomeFocused)
	{
		rgb_color	viewColor;

		if (inBecomeFocused)
		{
			viewColor = fHighColor;
		}
		else
		{
			fHighColor = fFocusBox->HighColor();			
			viewColor = fFocusBox->ParentViewColor();
		}
		
		fFocusBox->SetHighColor(viewColor);
		fFocusBox->Invalidate();
	}

	MListView::MakeFocus(inBecomeFocused);		// Becomes the focus here

	// Post a message to our target indicating that we've
	// become the focus
	if (inBecomeFocused && ! wasFocus)
	{
		PostToTarget(msgListBecameFocus);
	}
}

// ---------------------------------------------------------------------------
//		SelectRow
// ---------------------------------------------------------------------------

void
MDLOGListView::SelectRow(
	int32 	row,
	bool 	keepOld,
	bool 	toSelect)
{
	MListView::SelectRow(row, keepOld, toSelect);
	PostToTarget(msgListSelectionChanged);
}

// ---------------------------------------------------------------------------
//		SelectRows
// ---------------------------------------------------------------------------

void
MDLOGListView::SelectRows(
	int32 	fromRow,
	int32 	toRow,
	bool 	keepOld,
	bool 	toSelect)
{
	MListView::SelectRows(fromRow, toRow, keepOld, toSelect);
	PostToTarget(msgListSelectionChanged);
}

// ---------------------------------------------------------------------------
//		SetTarget
// ---------------------------------------------------------------------------

void
MDLOGListView::SetTarget(
	BHandler *target)
{
	fTarget = target;
}

// ---------------------------------------------------------------------------
//		PostToTarget
// ---------------------------------------------------------------------------

void
MDLOGListView::PostToTarget(
	uint32	inMessageKind)
{
	BHandler*		target = Target();
	
	if (target)
	{
		BLooper*	looper = target->Looper();
		if (looper)
		{
			BMessage		msg(inMessageKind);
			
			msg.AddPointer("source", this);
			
			looper->PostMessage(&msg, target);
		}
	}
}
