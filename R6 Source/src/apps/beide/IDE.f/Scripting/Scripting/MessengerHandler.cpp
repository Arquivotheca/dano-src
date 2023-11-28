/*	MessengerHandler.h
 *	$Id: MessengerHandler.cpp,v 1.1 1996/12/04 11:58:42 hplus Exp elvis $
 *	How to return a reference to a messenger for a BHandler
 */

#include "MessengerHandler.h"
#include <Handler.h>
#include <Messenger.h>


MessengerHandler::MessengerHandler(
	const char *		name,
	BHandler *			handler) :
	LazyScriptHandler((long)handler, name)
{
	fHandler = handler;
}


status_t
MessengerHandler::PerformScriptAction(
	BMessage *			message,
	BMessage * &		reply,
	bool&				/*wasDeferred*/)
{
	switch (message->what) {
	case kGetVerb:
		return DoGet(message, reply);
	case kSetVerb:
		return SCRIPT_READ_ONLY;
	}
	return SCRIPT_BAD_VERB;
}


long
MessengerHandler::DoGet(
	BMessage *				/*message*/,
	BMessage * &			reply)
{
	if (!reply)
		reply = new BMessage(kReplyVerb);
	BMessenger m(fHandler);
	reply->AddMessenger(kDefaultDataName, m);
	return B_NO_ERROR;
}


