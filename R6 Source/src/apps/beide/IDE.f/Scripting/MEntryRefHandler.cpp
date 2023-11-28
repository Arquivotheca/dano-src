//========================================================================
//	MEntryRefHandler.cpp
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include <string.h>

#include "MEntryRefHandler.h"
#include "Coercion.h"
#include "MScripting.h"

#include <Message.h>

// ---------------------------------------------------------------------------
//		MEntryRefHandler
// ---------------------------------------------------------------------------
//	For use with a list of entry_refs.  Add each ref
//	to the list like this:
//	entry_ref*		ref = new entry_refs;
//	SetRecordRef(ref);
//	list->AddItem(ref);

MEntryRefHandler::MEntryRefHandler(
	const char *		inName,
	BList *				inList) //	takes ownership of the list
	: LazyScriptHandler(-1, inName),
	fList(inList)
{
}

// ---------------------------------------------------------------------------
//		MEntryRefHandler
// ---------------------------------------------------------------------------
//	for use with a single ref

MEntryRefHandler::MEntryRefHandler(
	const char *		inName,
	entry_ref			inRef)	 //	for use with a single ref
	: LazyScriptHandler(-1, inName)
{
	fList = new BList;
	entry_ref*		ref = new entry_ref;
	*ref = inRef;
	fList->AddItem(ref);
}

// ---------------------------------------------------------------------------
//		~MEntryRefHandler
// ---------------------------------------------------------------------------

MEntryRefHandler::~MEntryRefHandler()
{
	for (int32 i = 0; i < fList->CountItems(); i++)
	{
		delete (entry_ref*) fList->ItemAtFast(i);
	}
	
	delete fList;
}

// ---------------------------------------------------------------------------
//		GetSubHandler
// ---------------------------------------------------------------------------

ScriptHandler *
MEntryRefHandler::GetSubHandler(
	const char *			propertyName,
	EForm					form,
	const SData &			data)
{
	int propCount = -1;

	if (!strcmp(propertyName, "property") && (form == formIndex)) {
		propCount = data.index;
	}

	// return a reference to ourself
	if (!strcmp(propertyName, kRefProp) && (form == formIndex)) {
		return this;
	}

	if (propCount > 0) {
		SData newData;
		newData.index = propCount;
		return LazyScriptHandler::GetSubHandler(propertyName, formIndex, newData);
	}

	return LazyScriptHandler::GetSubHandler(propertyName, form, data);
}


// ---------------------------------------------------------------------------
//		PerformScriptAction
// ---------------------------------------------------------------------------

status_t
MEntryRefHandler::PerformScriptAction(
	BMessage *			message,
	BMessage * &		reply,
	bool&				/*wasDeferred*/)
{
	switch (message->what) {
	case kGetVerb:
		return DoGet(message, reply);
		break;
	case kSetVerb:
		return DoSet(message, reply);
		break;
	}
	return SCRIPT_BAD_VERB;
}


// ---------------------------------------------------------------------------
//		DoGet
// ---------------------------------------------------------------------------
//	Get the entry refs.

status_t
MEntryRefHandler::DoGet(
	BMessage *				/* message */,
	BMessage * &			reply)
{
	if (!reply)
		reply = new BMessage(kReplyVerb);

	entry_ref*		ref;
	
	for (int32 i = 0; i < fList->CountItems(); i++)
	{
		ref = (entry_ref*) fList->ItemAtFast(i);
		reply->AddRef(kDefaultDataName, ref);
	}

	return B_NO_ERROR;
}


// ---------------------------------------------------------------------------
//		DoSet
// ---------------------------------------------------------------------------
//	We don't support settings the record_refs.

status_t
MEntryRefHandler::DoSet(
	BMessage *				/*message*/,
	BMessage * &			/*reply*/)
{
#if 0
	if (fPtr) {
		if (!GetCoercions().GetFloat(*fPtr, message, kDefaultDataName, 0)) {
			return SCRIPT_BAD_TYPE;
		}
	} else {
		if (!GetCoercions().GetRect(frame, message, kDefaultDataName, 0)) {
			return SCRIPT_BAD_TYPE;
		}
	}
	window->MoveTo(frame.LeftTop());
	window->ResizeTo(frame.Width(), frame.Height());
#endif
	return B_ERROR;
}


