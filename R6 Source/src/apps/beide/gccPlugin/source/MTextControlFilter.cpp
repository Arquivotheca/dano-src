//========================================================================
//	MTextControlFilter.cpp
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	Meant to be added to a BTextControl so that it posts its message
//	on every keydown, cut, and paste.

#include <Looper.h>
#include <TextControl.h>
#include <TextView.h>

#include "MTextControlFilter.h"
#include "IDEConstants.h"

// ---------------------------------------------------------------------------
//		MTextControlFilter
// ---------------------------------------------------------------------------
//	Constructor

MTextControlFilter::MTextControlFilter(
	BTextControl*	inControl)
	: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE)
{
	SetControl(inControl);
}

// ---------------------------------------------------------------------------
//		Filter
// ---------------------------------------------------------------------------
//	When getting a keydown, cut or paste, post the textcontrol's
//	modification message to its target.

filter_result
MTextControlFilter::Filter(
	BMessage *inMessage, 
	BHandler **/*inTarget*/)
{
	switch (inMessage->what)
	{
		case B_KEY_DOWN:
		case B_CUT:
		case B_PASTE:
		{
			BHandler*		target = fControl->Target();
			BMessage*		msg = fControl->Message();
			if (target != nil && msg != nil)
			{
				BLooper*	looper = target->Looper();
				
				if (looper != nil)
					looper->PostMessage(msg, target);
			}
		}
		break;
	}

	return B_DISPATCH_MESSAGE;
}

// ---------------------------------------------------------------------------
//		SetControl
// ---------------------------------------------------------------------------
//	Save the TextControl for this filter.  Add the filter
//	to the textview inside the textcontrol.

void
MTextControlFilter::SetControl(
	BTextControl*	inControl)
{
	fControl = inControl;

	if (inControl != nil)
	{
		BTextView*	textView = (BTextView*) inControl->ChildAt(0);

		textView->AddFilter(this);
	}
}


