// ======================================================================
// TextClock.h
//
// A text status widget for a Wagner interface that supports special
// text escapes compatible with POSIX strftime(3).
//
// To embed this widget in a Wagner interface, use the appropriate 
// embed tag:
//
// <embed name="plugin_name" value="%D"
// 		src="beos:///?signature=application/x-vnd.Be.TextClock">
//
// Supported plugin parameters are the same as for the text status 
// plugin (see TextStatus.h), plus the additional parameter 
// "update_frequency" (numeric, in seconds).
//
// To change the formatting, send it a message with a what code
// of 'uSTT' and a string field named "text" that contains
// the new formatting.  Example:
// 
// beos.sendMessage(document["plugin_name"], "uSTT", 
// 		"text", "string", "%I");
// ======================================================================

#ifndef TEXT_CLOCK_CONTENT_INSTANCE_H
#define TEXT_CLOCK_CONTENT_INSTANCE_H

#include <Content.h>
#include <String.h>

#include "TextStatusContentInstance.h"

// clock update message
enum {
	bmsgUpdateClock = 'tick' // internal "tick" message
};

#define S_TEXT_CLOCK_PARAM_UPDATE_FREQUENCY "update_frequency"
#define DEFAULT_UPDATE_FREQUENCY		30

using namespace Wagner;

class TextClockContentInstance : public TextStatusContentInstance {
public:
						TextClockContentInstance(Content *content,
												 GHandler *handler,
												 const BMessage& params);
	virtual				~TextClockContentInstance();
	virtual	void		Notification(BMessage *msg);
	virtual void 		FilterText();
protected:
	float 				fUpdateFrequencySeconds;
};

#endif
