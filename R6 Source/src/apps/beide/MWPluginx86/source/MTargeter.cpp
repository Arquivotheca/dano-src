//==================================================================
//	MTargeter.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//==================================================================
//	Simple utility class that keeps track of a target handler and posts a message
//	to the target when requested to.

#include "MTargeter.h"

// ---------------------------------------------------------------------------
//		¥ MTargeter
// ---------------------------------------------------------------------------

MTargeter::MTargeter(
	ulong		inMessageType)
{
	if (inMessageType != 0)
		fMessage = new BMessage(inMessageType);
	else
		fMessage = nil;

	fTarget = nil;
	fLooper = nil;
}

// ---------------------------------------------------------------------------
//		¥ ~MTargeter
// ---------------------------------------------------------------------------

MTargeter::~MTargeter()
{
	delete fMessage;
}

// ---------------------------------------------------------------------------
//		¥ PostMessageToTarget
// ---------------------------------------------------------------------------
//	Post the current message to the target.

void
MTargeter::PostMessageToTarget()
{
	if (fMessage != nil)
	{
		if (fLooper != nil || B_NO_ERROR == SetTarget(fTarget))
			fLooper->PostMessage(new BMessage(fMessage), fTarget);
	}
}

// ---------------------------------------------------------------------------
//		¥ SetTarget
// ---------------------------------------------------------------------------
//	Set the target the message will be posted to when the view changes.

long
MTargeter::SetTarget(
	BHandler*	inTarget)
{
	long		err = B_ERROR;

	fTarget = inTarget;

	if (inTarget)
	{
		BLooper*	looper = inTarget->Looper();

		if (looper)
		{
			fLooper = looper;
			err = B_NO_ERROR;
		}
	}

	if (err == B_ERROR)
	{
		fLooper = nil;
	}
	
	return err;
}

// ---------------------------------------------------------------------------
//		¥ SetMessage
// ---------------------------------------------------------------------------
//	Takes ownership of the message that's passed in.

void
MTargeter::SetMessage(
	BMessage*	inMessage)
{
	delete fMessage;
	
	fMessage = inMessage;
}
