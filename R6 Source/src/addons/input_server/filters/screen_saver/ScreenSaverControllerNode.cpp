#include "ScreenSaverControllerNode.h"
#include "command_constants.h"

#include <Autolock.h>
#include <Debug.h>

#define BYLINE "ScreenSaverControllerNode"

// property names
const char* kQuitBlankerProp = "QuitBlanker";
const char* kIdleTimeProp = "idle_time";
const char* kIsBlankingProp = "is_blanking";

// NOTE: these properties need to stay in alphabetical order.  If a new property
// is inserted in this list, code to handle it should be added to ReadProperty().
const char* kPubProperties[] = {
						kIdleTimeProp,
						kIsBlankingProp
};

const int32	kNumPubProperties = (sizeof(kPubProperties)
	/ sizeof(kPubProperties[0]));
	
// message field names
const char* kIdleTimeField = "idle_time";
const char* kQuitBlankerResultField = "quit_blanker_result";
const char* kIsBlankingField = "is_blanking";
extern const char* kEventMaskField = "event_mask";
extern const char* kPropertyNameField = "property_name";

ScreenSaverControllerNode::ScreenSaverControllerNode(BMessenger target)
	: m_target(target), m_locker(BYLINE " Locker")
{
	SERIAL_PRINT((BYLINE ": %s\n", __PRETTY_FUNCTION__));
}

ScreenSaverControllerNode::~ScreenSaverControllerNode(void)
{
}

status_t ScreenSaverControllerNode::OpenProperties(void **cookie, void *copyCookie)
{
	store_cookie *c = new store_cookie;
	if (copyCookie) {
		*c = *((store_cookie*)copyCookie);
	} else {
		c->index = 0;
	}
	*cookie = c;
	
	return B_OK;
}

status_t ScreenSaverControllerNode::NextProperty(void *cookie, char *nameBuf, int32 *len)
{
	status_t err = ENOENT;
	store_cookie *c = (store_cookie*)cookie;

	if (c->index < kNumPubProperties) {	
		const char *name = kPubProperties[c->index];
		strncpy(nameBuf, name, *len);
		*len = strlen(name);
		c->index++;
		err = B_OK;
	}
	
	return err;
}

status_t ScreenSaverControllerNode::CloseProperties(void *cookie)
{
	store_cookie *c = (store_cookie*)cookie;
	delete c;

	return B_OK;
}

put_status_t ScreenSaverControllerNode::WriteProperty(const char *, const property &)
{
	return put_status_t(B_ERROR, true);
}

get_status_t ScreenSaverControllerNode::ReadProperty(const char *name, property &prop,
	const property_list &/*args*/)
{
	status_t err;

	SERIAL_PRINT((BYLINE ": %s, property is %s\n", __PRETTY_FUNCTION__, name));

	prop.Undefine();

	if (strcmp(name, kIdleTimeProp) == 0) {
		BMessage msg(kGetIdleTime);
		
		err = m_target.SendMessage(&msg, &msg);
		if (err == B_OK && msg.what != B_NO_REPLY) {
			bigtime_t idleTime;
			
			err = msg.FindInt64(kIdleTimeField, &idleTime);
			if (err == B_OK) {
				prop = idleTime/1000000.0; // convert to secs.
			}
		}
				
		return get_status_t(err, false);
	}
	else if (strcmp(name, kQuitBlankerProp) == 0) {
		BMessage msg(kQuitBlanker);
		
		err = m_target.SendMessage(&msg, &msg);
		if (err == B_OK && msg.what != B_NO_REPLY) {
			status_t result;
			
			err = msg.FindInt32(kQuitBlankerResultField, &result);
			if (err == B_OK) {
				if (result != B_OK) {
					SERIAL_PRINT((BYLINE ": attempt to quit blanker returned "
						"%s\n", strerror(result)));
					prop = "false";
				} else {
					prop = "true";
				}
			}
		}

		return get_status_t(err, false);
	}
	else if (strcmp(name, kIsBlankingProp) == 0) {
		BMessage msg(kGetIsBlanking);
		
		err = m_target.SendMessage(&msg, &msg);
		if (err == B_OK && msg.what != B_NO_REPLY) {
			bool isBlanking;
			
			err = msg.FindBool(kIsBlankingField, &isBlanking);
			if (err == B_OK) {
				prop = (isBlanking) ? "true" : "false";
			}
		}

		return get_status_t(err, false);
	}

	return ENOENT;
}

status_t ScreenSaverControllerNode::HandleMessage(BMessage* message)
{
	switch (message->what) {
		case kNotifyListeners: {
			int32 eventMask = 0xFFFFFFFF;
			const char* propertyName;
			
			if (message->FindInt32(kEventMaskField, eventMask) == B_OK
					&& message->FindString(kPropertyNameField,
					&propertyName) == B_OK) {
				NotifyListeners(static_cast<uint32>(eventMask), propertyName);
			}
			
			break;
		}
			
		default:
			SERIAL_PRINT((BYLINE ": Unexpected message with code %ld received\n",
				message->what));
			break;
	}
	
	return B_OK;
}
