//	LazyPropHandler.cpp

#include "LazyPropHandler.h"
#include "Coercion.h"

#include <Message.h>

long LazyPropHandler::sId;

LazyPropHandler::LazyPropHandler(
	const char *		name,
	void *				prop,
	int					size,
	unsigned long		type,
	bool				modify) :
	LazyScriptHandler(atomic_add(&sId, 1), name)
{
	fProp = prop;
	fSize = size;
	fType = type;
	fModifiable = modify;
	fHandler = NULL;
}


LazyPropHandler::~LazyPropHandler()
{
	if (fHandler)
		fHandler->Done();
}


void
LazyPropHandler::SetHandler(
	ScriptHandler *		h)
{
/*	We only support one handler at a time - since we only reference one property at a time
 */
	if (fHandler)
		fHandler->Done();
	fHandler = h ? h->Reference() : NULL;
}


status_t
LazyPropHandler::PerformScriptAction(
	BMessage *			message,
	BMessage * &		reply,
	bool&				/*wasDeferred*/)
{
	switch (message->what) {
	case kSetVerb:
		if (!fModifiable)
			return SCRIPT_READ_ONLY;
		return DoSet(message, reply);
		break;
	case kGetVerb:
		return DoGet(message, reply);
		break;
	default:
		return SCRIPT_BAD_VERB;
	}

	return 0;	//	not reached
}


long
LazyPropHandler::DoSet(
	BMessage *			message,
	BMessage * &		/*reply*/)
{
	bool ok = false;
	long l;

	switch (fType) {
	case B_UINT32_TYPE:
	case B_INT32_TYPE:
		ok = GetCoercions().GetLong((*(long*)fProp), 
				message, kDefaultDataName);
		break;
	case B_UINT16_TYPE:
	case B_INT16_TYPE:
		ok = GetCoercions().GetLong(l, message, 
				kDefaultDataName);
		*(short*)fProp = l;
		break;
	case B_CHAR_TYPE:
	case B_UINT8_TYPE:
	case B_BOOL_TYPE:
		ok = GetCoercions().GetLong(l, message, 
				kDefaultDataName);
		*(char*)fProp = l;
		break;
	case B_FLOAT_TYPE:
		ok = GetCoercions().GetFloat((*(float *)fProp),
				message, kDefaultDataName);
		break;
	case B_DOUBLE_TYPE:
		ok = GetCoercions().GetDouble((*(double *)fProp),
				message, kDefaultDataName);
		break;
	case B_RECT_TYPE:
		ok = GetCoercions().GetRect((*(BRect *)fProp),
				message, kDefaultDataName);
		break;
	case B_POINT_TYPE:
		ok = GetCoercions().GetPoint((*(BPoint *)fProp),
				message, kDefaultDataName);
		break;
	case B_RGB_COLOR_TYPE:
		ok = GetCoercions().GetColor((*(rgb_color *)fProp),
				message, kDefaultDataName);
		break;
	case B_STRING_TYPE:
	case B_ASCII_TYPE:
		return SCRIPT_READ_ONLY;
	}

	return ok ? B_NO_ERROR : SCRIPT_BAD_TYPE;
}


long
LazyPropHandler::DoGet(
	BMessage *			/* message */,
	BMessage * &		reply)
{
	//	ignore message
	if (!reply) {
		reply = new BMessage(kReplyVerb);
	}
	reply->AddData(kDefaultDataName, fType, fProp, fSize);
	return B_NO_ERROR;
}
