// ===========================================================================
//	MScriptUtils.cpp
// ===========================================================================
//	Copyright 1996 Metrowerks Corporation. All rights reserved.

#include "MScriptUtils.h"
#include "Scripting.h"

#include <Message.h>

// ---------------------------------------------------------------------------
//		NewScriptID
// ---------------------------------------------------------------------------

uint32
NewScriptID()
{
	static int32		id = 0;
	
	return atomic_add(&id, 1L);
}

// ---------------------------------------------------------------------------
//		AddErrorToReply
// ---------------------------------------------------------------------------

void
AddErrorToReply(
	BMessage * &	reply,
	status_t		inError,
	const char*		inErrorText)
{
	type_code	type;
	int32		count;

	if (reply == nil)
		reply = new BMessage(kReplyVerb);

	if (inError != 0)
	{
		if (B_NO_ERROR !=  reply->GetInfo("error", &type, &count) ||
			type != B_INT32_TYPE)
			reply->AddInt32("error", inError);
		else
			reply->ReplaceInt32("error", inError);
	}

	if (inErrorText != nil)
		reply->AddString("error", inErrorText);
}

