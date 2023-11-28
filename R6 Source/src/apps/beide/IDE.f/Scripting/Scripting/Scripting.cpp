/*	Scripting.cpp
 *	$Id: Scripting.cpp,v 1.1 1996/12/04 11:58:38 hplus Exp elvis $
 *	Constants and functions for the general scripting interface
 */

#include "Scripting.h"
#include "ScriptHandler.h"

#include <Message.h>
#include <Debug.h>


const char kDefaultTargetName[] = "target";
const char kDefaultDataName[] = "data";
const char kDefaultErrorName[] = "error";
const char kDefaultErrorTextName[] = "errortext";
const char kRecordingTargetName[] = "recorder";


static unsigned long sDefaultVerbs[] = {
	kOpenVerb,
	kSaveVerb,
	kCloseVerb,
	kSetVerb,
	kGetVerb,
	kMakeVerb,
	kDeleteVerb,
	kCountVerb,
	0
};

bool
TryScriptMessage(
	BMessage *				message,
	ScriptHandler *			root,
	const unsigned long *	verbs,
	int						numVerbs)
{
	if (!verbs) {
		verbs = sDefaultVerbs;
		numVerbs = -1;
	}
	if (numVerbs < 0)
		for (numVerbs=0; verbs[numVerbs]; numVerbs++)
			/*	nothing	*/;

	for (int ix=0; ix<numVerbs; ix++) {
		if (message->what == verbs[ix]) {
			goto is_verb;
		}
	}

	return false;

is_verb:

	bool wasRePosted = false;

	ScriptHandler *target = root->FindScriptTarget(message, kDefaultTargetName, wasRePosted);

	if (wasRePosted)
		return true;
	
	bool wasDeferred = false;
	long err = B_NO_ERROR;
	BMessage *reply = NULL;
	if (!target)
		err = SCRIPT_NO_PROPERTY;
	else
	{
		err = target->PerformScriptAction(message, reply, wasDeferred);
		target->Done();
	}

	if (!wasDeferred)
	{
		if (!reply)
			reply = new BMessage(kReplyVerb);
		type_code type;
		long count;
		if (err && B_NO_ERROR != reply->GetInfo("error", &type, &count))
			reply->AddInt32("error", err);
	
		message->SendReply(reply);
	}

	return true;
}

