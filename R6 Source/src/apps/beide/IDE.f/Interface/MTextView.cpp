//==================================================================
//	MTextView.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//==================================================================
//	A text view to be used in dialog boxes.  It posts a message
//	when it's changed.

#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include <Message.h>

#include "MTextView.h"
#include "IDEMessages.h"
#include "Utils.h"

// ---------------------------------------------------------------------------
//		MTextView
// ---------------------------------------------------------------------------
//	Constructor

MTextView::MTextView(
	BRect			inFrame,
	const char*		inName,
	uint32 			resizeMask,
	uint32 			flags)
	: BTextView(
		inFrame,
		inName,
		GetTextRect(inFrame),
		resizeMask,
		flags),
	MTargeter(msgTextChanged)
{
	fDirty = false;

	DisallowInvalidChars(*this);
}

// ---------------------------------------------------------------------------
//		GetTextRect
// ---------------------------------------------------------------------------

BRect
MTextView::GetTextRect(
	const BRect & area)
{
	BRect r = area;

	r.OffsetTo(B_ORIGIN);
	r.InsetBy(2.0, 2.0);

	return r;
}

// ---------------------------------------------------------------------------
//		~MTextView
// ---------------------------------------------------------------------------
//	Destructor

MTextView::~MTextView()
{
}

// ---------------------------------------------------------------------------
//		SetDirty
// ---------------------------------------------------------------------------

void
MTextView::SetDirty(
	bool	inDirty)
{
	fDirty = inDirty;
	
	PostMessageToTarget();
}

// ---------------------------------------------------------------------------
//		MessageReceived
// ---------------------------------------------------------------------------

void
MTextView::MessageReceived(
	BMessage * message)
{
	switch (message->what)
	{
		case B_SIMPLE_DATA:
			if (message->HasData("text/plain", B_MIME_TYPE))
			{
				SetDirty();
			}
			// Fall through

		default:
			BTextView::MessageReceived(message);
			break;
	}
}

// ---------------------------------------------------------------------------
//		OneCharSelected
// ---------------------------------------------------------------------------
//	Utility for determining if the current keystroke will change the
//	text without changing the textlength.

inline bool
MTextView::OneCharSelected()
{
	int32	selStart;
	int32	selEnd;

	// Deal with case of one char selected and replaced
	GetSelection(&selStart, &selEnd);
	return (selEnd - selStart == 1);
}

inline int32
MTextView::SelectionSize()
{
	int32	selStart;
	int32	selEnd;

	GetSelection(&selStart, &selEnd);
	return (selEnd - selStart);
}


// ---------------------------------------------------------------------------
//		KeyDown
// ---------------------------------------------------------------------------

void
MTextView::KeyDown(
	const char *	inBytes, 
	int32 			inNumBytes)
{
	int32		textLength = TextLength();
	bool		changed = false;

	switch (inBytes[0])
	{
		// BTextView handles these correctly
		case B_LEFT_ARROW:
		case B_RIGHT_ARROW:
		case B_UP_ARROW:
		case B_DOWN_ARROW:
			BTextView::KeyDown(inBytes, inNumBytes);
			break;

		// Ignore these
		case B_HOME:
		case B_END:
		case B_PAGE_UP:
		case B_PAGE_DOWN:
			break;

		case B_BACKSPACE:
			BTextView::KeyDown(inBytes, inNumBytes);
			break;

		case B_DELETE:		// Forward delete
			int32	selStart;
			int32	selEnd;

			GetSelection(&selStart, &selEnd);
			if (selStart == selEnd)
				Select(selStart, selStart + 1);
			BTextView::Delete();
			break;
		
		case B_TAB:
		// If we're navigable and it's not an option-tab pass the tab up to to BView
		if ((Flags() & B_NAVIGABLE) != 0 && (modifiers() & B_OPTION_KEY) == 0)
		{
			BView::KeyDown(inBytes, inNumBytes);
			break;
		}
		// fall through on purpose
		case B_RETURN:
			changed = OneCharSelected();
			BTextView::KeyDown(inBytes, inNumBytes);
			break;
			
		default:			// Normal input
			{
				changed = OneCharSelected() || SelectionSize() == inNumBytes;
				BTextView::KeyDown(inBytes, inNumBytes);
			}		
			break;
	}
	
	if (changed || textLength != TextLength())
		SetDirty();
}

// ---------------------------------------------------------------------------
//		Cut
// ---------------------------------------------------------------------------

void
MTextView::Cut(
	BClipboard *clip)
{
	BTextView::Cut(clip);
	SetDirty();
}

// ---------------------------------------------------------------------------
//		Paste
// ---------------------------------------------------------------------------

void
MTextView::Paste(
	BClipboard *clip)
{
	BTextView::Paste(clip);
	SetDirty();
}

// ---------------------------------------------------------------------------
//		MakeFocus
// ---------------------------------------------------------------------------

void
MTextView::MakeFocus(
	bool	inFocus)
{
	if (inFocus)
		SelectAll();

	BTextView::MakeFocus(inFocus);
}

// ---------------------------------------------------------------------------
//		GetValue
// ---------------------------------------------------------------------------
//	Return a value for the text in this textview.  Not guaranteed to work
//	if the text isn't all digits.

int32
MTextView::GetValue()
{
	int32		value = 0;

	sscanf(Text(), "%ld", &value);

	return value;
}
