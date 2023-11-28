//==================================================================
//	MTextView.cpp
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//==================================================================
//	A text view to be used in dialog boxes.  It posts a message
//	when it's changed.

#include <string.h>
#include <ctype.h>

#include "MTextView.h"

#include <Message.h>
#include <Looper.h>
#include <Debug.h>

// ---------------------------------------------------------------------------
//		MTextView
// ---------------------------------------------------------------------------
//	Constructor

MTextView::MTextView(
	BRect			inFrame,
	const char*		inName,
	ulong 			resizeMask,
	ulong 			flags)
	: BTextView(
		inFrame,
		inName,
		GetTextRect(inFrame),
		resizeMask,
		flags)
{
	fDirty = false;
	fTarget = NULL;
	fOurLooper = NULL;
	fIBeam = false;

	fMessage = new BMessage(msgTextChanged);

	DisallowChar(B_ESCAPE);
	DisallowChar(B_INSERT);
	DisallowChar(B_FUNCTION_KEY);
	DisallowChar(B_PRINT_KEY);
	DisallowChar(B_SCROLL_KEY);
	DisallowChar(B_PAUSE_KEY);
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
	delete fMessage;
}

// ---------------------------------------------------------------------------
//		SetDirty
// ---------------------------------------------------------------------------

void
MTextView::SetDirty(
	bool	inDirty)
{
	fDirty = inDirty;
	
	if (fOurLooper != NULL && fMessage != NULL)
		fOurLooper->PostMessage(fMessage, fTarget);
}

// ---------------------------------------------------------------------------
//		SetTarget
// ---------------------------------------------------------------------------
//	Set the target the message will be posted to when the view changes.

long
MTextView::SetTarget(
	BHandler*	inTarget)
{
	long		err = B_ERROR;

	if (inTarget)
	{
		BLooper*	looper = inTarget->Looper();

		if (looper)
		{
			fOurLooper = looper;
			fTarget = inTarget;
			err = B_NO_ERROR;
		}
	}

	if (err == B_ERROR)
	{
		fTarget = NULL;
		fOurLooper = NULL;
	}
	
	return err;
}

// ---------------------------------------------------------------------------
//		SetMessage
// ---------------------------------------------------------------------------

void
MTextView::SetMessage(
	BMessage*	inMessage)
{
	delete fMessage;
	
	fMessage = inMessage;
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
			if (message->HasData("text", B_ASCII_TYPE))
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
	long	selStart;
	long	selEnd;

	// Deal with case of one char selected and replaced
	GetSelection(&selStart, &selEnd);
	return (selEnd - selStart == 1);
}

// ---------------------------------------------------------------------------
//		KeyDown
// ---------------------------------------------------------------------------

void
MTextView::KeyDown(
	const char *	inBytes, 
	int32 			inNumBytes)
{
	long		textLength = TextLength();
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
			long	selStart;
			long	selEnd;

			GetSelection(&selStart, &selEnd);
			if (selStart == selEnd)
				Select(selStart, selStart + 1);
			BTextView::Delete();
			break;

		case B_RETURN:
		case B_TAB:
			changed = OneCharSelected();
			BTextView::KeyDown(inBytes, inNumBytes);
			break;
			
		default:			// Normal input
			if (inBytes[0] >= B_SPACE)		// simpleminded filter
			{
				changed = OneCharSelected();
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
//		GetValue
// ---------------------------------------------------------------------------
//	Return a value for the text in this textview.  Not guaranteed to work
//	if the text isn't all digits.

long
MTextView::GetValue()
{
	long		value = 0;

	sscanf(Text(), "%ld", &value);

	return value;
}
