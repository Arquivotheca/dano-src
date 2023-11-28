//	ScriptHandler.cpp

#include <string.h>

#include "ScriptHandler.h"
#include "Scripting.h"
#include "Coercion.h"
#include "LazyPropHandler.h"

#include <Message.h>
#include <Looper.h>


Coercions * ScriptHandler::sCoercions;

ScriptHandler::ScriptHandler(
	const uint32			id,
	const char *			name,
	BLooper *				looper)
	: fId(id),
	fLooper(looper)
{
	AllocName(name);
	if (!sCoercions) {
		sCoercions = new Coercions;
		sCoercions->AddCoercionHandler(new StandardNumberCoercions);
		sCoercions->AddCoercionHandler(new StandardGraphicsCoercions);
		sCoercions->AddCoercionHandler(new StandardTextCoercions);
	}
}


ScriptHandler::~ScriptHandler()
{
	FreeName();
}


void
ScriptHandler::FreeName()
{
	delete[] fName;
}


void
ScriptHandler::AllocName(
	const char *		name)
{
	fName = new char[strlen(name)+1];
	strcpy(fName, name);
}


void
ScriptHandler::SetName(
	const char *		name)
{
	FreeName();
	AllocName(name);
}


ScriptHandler *
ScriptHandler::Reference()
{
	return this;
}


void
ScriptHandler::Done()
{
	//	do nothing
}


ScriptHandler *
ScriptHandler::FindScriptTarget(
	BMessage *				message,
	const char *			parameter,
	bool&					wasRePosted)
{
	if (!parameter) {
		parameter = kDefaultTargetName;
	}
	type_code type;
	int32 count = 0;
	status_t	err;

	err = message->GetInfo(parameter, &type, &count);
	if (err != B_NO_ERROR && err != B_NAME_NOT_FOUND)
		return NULL;

	ScriptHandler *handler = this->Reference();
	BLooper * looper = GetLooper();
	PropertyItem*	item;
	for (int ix=count-1; ix>=0; ix--) {
		ssize_t size;
		err = message->FindData(parameter, PROPERTY_TYPE, ix, (const void**)&item, &size);
		if (err != B_NO_ERROR)
			return NULL;
		ScriptHandler *oldFriend = handler;
		handler = handler->GetSubHandler(item->property, item->form, item->data);
		oldFriend->Done();	//	good-bye, old friend!
		if (!handler)
			break;
		BLooper* newLooper = handler->GetLooper();
		if (looper != NULL && newLooper != NULL &&
			looper != newLooper)
		{
			PostMessageToNewLooper(message, newLooper, parameter, ix);
			handler = NULL;
			wasRePosted = true;
			break;
		}
		looper = newLooper;
	}
	return handler;
}


void
ScriptHandler::PostMessageToNewLooper(
	BMessage *			message,
	BLooper *			looper,
	const char *		parameter,
	int32				index)
{
	type_code type;
	int32 count;
	ssize_t size;
	status_t	err;

	if (B_NO_ERROR == message->GetInfo(parameter, &type, &count)) 
	{
		PropertyItem *items = new PropertyItem[count];
		
		for (int32 ix = 0; ix < count; ix++)
		{
			PropertyItem *item;
			err = message->FindData(parameter, PROPERTY_TYPE, ix, (const void**)&item, &size);

			items[ix] = *item;
		}
		message->RemoveName(parameter);
		size = sizeof(PropertyItem);
		for (int32 ix = 0; ix < index; ix++)
		{
			message->AddData(parameter, PROPERTY_TYPE, &items[ix], size);
		}
		
		looper->PostMessage(message);
	}
}


status_t
ScriptHandler::PerformScriptAction(
	BMessage *				/*message*/,
	BMessage * &			/*reply*/,
	bool&					/*wasDeferred*/)
{
	return SCRIPT_BAD_VERB;
}


ScriptHandler *
ScriptHandler::GetSubHandler(
	const char *			propertyName,
	EForm					form,
	const SData &			data)
{
/*	Since we may be a lazy property, and reference ourselves with another lazy property, 
 *	we tell the lazy properties to reference us so we don't die ahead of time
 */
	LazyPropHandler *lprop = NULL;

	int propCount = -1;
/*	This test is especially marked; see below about enumerating properties
 */
	if (!strcmp(propertyName, "property") && (form == formIndex)) {
		propCount = data.index;
	}
	if (!strcmp(propertyName, "name") || (0 == --propCount)) {
		const char *name = GetName();
		lprop = new LazyPropHandler("name", (void *)name, strlen(name), B_ASCII_TYPE, false);
		lprop->SetHandler(this);
		return lprop;
	}
	if (!strcmp(propertyName, "id") || (0 == --propCount)) {
		lprop = new LazyPropHandler("id", &fId, sizeof(fId), B_INT32_TYPE, false);
		lprop->SetHandler(this);
		return lprop;
	}
/*	This is a neat thing. A script can say:
 *	target.property("name") as well as target.name
 *	This allows languages with strong typing to work with properties as first-class objects
 *	To support enumeration of properties, we also support indexing. This requires the 
 *	subclasses to test for this case (like the marked test above) and deduct for their number 
 *	of properties.
 *	target.property(2).name would return "id" for the base ScriptHandler, whereas 
 *	target.property(2) would return the actual ID.
 *	Notice that with lazy handlers, we have to think about life-span too! That's why we 
 *	build a chain of handlers to dispose in FindScriptTarget instead of disposing directly.
 */
	if (!strcmp(propertyName, "property")) {
		if (form != formName)
			return NULL;
		return GetSubHandler(data.name, formDirect, data);
	}
	return NULL;
}

