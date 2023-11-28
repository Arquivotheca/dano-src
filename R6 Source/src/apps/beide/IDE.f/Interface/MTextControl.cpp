//==================================================================
//	MTextControl.cpp
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//==================================================================

#include <stdio.h>

#include "MTextControl.h"
#include "MTextControlFilter.h"

// ---------------------------------------------------------------------------
//		MTextControl
// ---------------------------------------------------------------------------
//	Constructor

MTextControl::MTextControl(
	BRect 			inFrame,
	const char *	inName,
	const char *	inLabel, 
	const char *	inInitialText, 
	BMessage *		inMessage,
	uint32 			inResizeMask,
	uint32 			inFlags)
	: BTextControl(
		inFrame,
		inName,
		inLabel,
		inInitialText,
		inMessage,
		inResizeMask,
		inFlags)
{
	new MTextControlFilter(this);
}

// ---------------------------------------------------------------------------
//		~MTextControl
// ---------------------------------------------------------------------------
//	Destructor

MTextControl::~MTextControl()
{
}

// ---------------------------------------------------------------------------
//		GetValue
// ---------------------------------------------------------------------------
//	Return a value for the text in this textview.  Not guaranteed to work
//	if the text isn't all digits.

int32
MTextControl::GetValue()
{
	int32		value = 0;
	BTextView*	view = (BTextView*) ChildAt(0);

	sscanf(view->Text(), "%ld", &value);

	return value;
}

