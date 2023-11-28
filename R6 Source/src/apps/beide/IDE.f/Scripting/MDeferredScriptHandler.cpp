//========================================================================
//	MDeferredScriptHandler.cpp
//	Copyright 1996Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include <Message.h>

#include "MDeferredScriptHandler.h"
#include "Scripting.h"
#include "IDEConstants.h"

#include <Looper.h>
#include <Debug.h>

// ---------------------------------------------------------------------------
//		MDeferredScriptHandler
// ---------------------------------------------------------------------------

MDeferredScriptHandler::MDeferredScriptHandler(
	const uint32			inId,
	const char *			inName,
	BLooper &				inLooper)
	: ScriptHandler(inId, inName),
	fMessage(nil),
	fReply(nil),
	fLooper(inLooper)
{
}

// ---------------------------------------------------------------------------
//		~MDeferredScriptHandler
// ---------------------------------------------------------------------------

MDeferredScriptHandler::~MDeferredScriptHandler()
{
	delete fMessage;
	delete fReply;
}

// ---------------------------------------------------------------------------
//		SendReply
// ---------------------------------------------------------------------------
//	Call this function after the deferred task has completed.
//	Pass in an error number and/or error text.

void
MDeferredScriptHandler::SendReply(	
	status_t		inError,
	const char*		inErrorText)

{
	if (fReply == nil)
		fReply = new BMessage(kReplyVerb);

	if (inError != 0)
	{
		type_code		type;
		int32			count;

		if (B_NO_ERROR != fReply->GetInfo("error", &type, &count) ||
			type != B_INT32_TYPE)
			fReply->AddInt32("error", inError);
		else
			fReply->ReplaceInt32("error", inError);
	}

	if (inErrorText != nil)
		fReply->AddString("error", inErrorText);
	
	fMessage->SendReply(fReply);

	delete fMessage;
	fMessage = nil;
	fReply = nil;
}

// ---------------------------------------------------------------------------
//		PerformScriptAction
// ---------------------------------------------------------------------------
//	Call this function from your subclass' PerformScriptAction
//	function to save the message and reply.

status_t
MDeferredScriptHandler::PerformScriptAction(
	BMessage *			inMessage,
	BMessage * &		inReply,
	bool&				outWasDeferred)
{
	ASSERT(inMessage != nil);

	fMessage = inMessage;
	if (fMessage != nil)
		fLooper.DetachCurrentMessage();

	fReply = inReply;

	outWasDeferred = true;

	return B_NO_ERROR;
}
