/*	WindowHandlers.cpp
 *	$Id: WindowFrameHandler.cpp,v 1.1 1996/12/04 11:58:40 hplus Exp elvis $
 *	Implementation of off-the-shelf window manipulators
 */

#pragma once

#include "WindowFrameHandler.h"
#include <Window.h>
#include "LazyPropHandler.h"
#include "Coercion.h"


WindowFrameHandler::WindowFrameHandler(
	const char *		inName,
	BWindow *			inWindow) :
	LazyScriptHandler((long)inWindow, inName)
{
	frame = inWindow->Frame();
	window = inWindow;
	fPtr = NULL;
}


/*	This handler encapsulates both a rectangle and its sub-fields.
 *	That makes some special things have to happen... for instance, it won't work 
 *	if we ever want to add a "parent" property.
 */
ScriptHandler *
WindowFrameHandler::GetSubHandler(
	const char *			propertyName,
	EForm					form,
	const SData &			data)
{
	int propCount = -1;

	if (!strcmp(propertyName, "property") && (form == formIndex)) {
		propCount = data.index;
	}
	/*	fPtr is set once we reference a field of the rectangle. We also change the name 
	 *	of ourselves to reflect our new designation. Since the caller will call Done() on 
	 *	us when we return ourselves in the next step, we return a Reference() to ourself.
	 */
	if (!fPtr) {
		if (!strcmp(propertyName, "left") || (0 == --propCount)) {
			SetName("left");
			fPtr = &frame.left;
			return Reference();
		}
		if (!strcmp(propertyName, "top") || (0 == --propCount)) {
			SetName("top");
			fPtr = &frame.top;
			return Reference();
		}
		if (!strcmp(propertyName, "right") || (0 == --propCount)) {
			SetName("right");
			fPtr = &frame.right;
			return Reference();
		}
		if (!strcmp(propertyName, "bottom") || (0 == --propCount)) {
			SetName("bottom");
			fPtr = &frame.bottom;
			return Reference();
		}
	}

	if (propCount > 0) {
		SData newData;
		newData.index = propCount;
		return LazyScriptHandler::GetSubHandler(propertyName, formIndex, newData);
	}
	return LazyScriptHandler::GetSubHandler(propertyName, form, data);
}


status_t
WindowFrameHandler::PerformScriptAction(
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


long
WindowFrameHandler::DoGet(
	BMessage *				/* message */,
	BMessage * &			reply)
{
	if (!reply)
		reply = new BMessage(kReplyVerb);
	if (fPtr) {
		reply->AddFloat(kDefaultDataName, *fPtr);
	} else {
		reply->AddRect(kDefaultDataName, frame);
	}
	return B_NO_ERROR;
}


long
WindowFrameHandler::DoSet(
	BMessage *				message,
	BMessage * &			/*reply*/)
{
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
	return B_NO_ERROR;
}


