// TextStatus.cpp

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <www/util.h> // for find_color() in libwww
#include <Debug.h>

#include "TextStatusContentInstance.h"

#include "TextClockContentInstance.h"

#define SECTOMICROSEC(s)		((long)(1000000l * s))

TextClockContentInstance::TextClockContentInstance(Content *content,
												   GHandler *handler,
												   const BMessage& params)
	: TextStatusContentInstance(content, handler, params)
{
	if (params.HasString(S_TEXT_CLOCK_PARAM_UPDATE_FREQUENCY)) {
		const char *updtStr 
			= params.FindString(S_TEXT_CLOCK_PARAM_UPDATE_FREQUENCY);
		errno = 0;
		fUpdateFrequencySeconds = (float)strtod(updtStr, NULL);
		if (errno != 0 || fUpdateFrequencySeconds == 0)
			fUpdateFrequencySeconds = DEFAULT_UPDATE_FREQUENCY;
	} else {
		fUpdateFrequencySeconds = DEFAULT_UPDATE_FREQUENCY;
	}
	
	PRINT(("[TextClockDisplayView::TextClockDisplayView] constructed with frequency=%0.1f\n",
		fUpdateFrequencySeconds));

	PostDelayedMessage(new BMessage(bmsgUpdateClock), SECTOMICROSEC(fUpdateFrequencySeconds));
}

TextClockContentInstance::~TextClockContentInstance() {

}

void TextClockContentInstance::Notification(BMessage *msg) {
	if (msg->what == bmsgUpdateClock) {
		PRINT(("[TextClockDisplayView] received bmsgUpdateClock\n"));
		InvalidateCachedData();
		PostDelayedMessage(new BMessage(bmsgUpdateClock), SECTOMICROSEC(fUpdateFrequencySeconds));
	} else {
		TextStatusContentInstance::Notification(msg);
	}
}

// TextClockDisplayView::FreshenText
// we run fText through POSIX strftime(3) to get our fDisplayText.
void TextClockContentInstance::FilterText() {
	int32 const 	clock_maxlen(255);
	BString 		formattedString(B_EMPTY_STRING);
	int32 			len(fText.Length()+1);
	char *			formatBuf(NULL);
	time_t 			now(time((time_t*)NULL));
	struct tm		ltime;
	
	localtime_r((const time_t*)&now, &ltime);
	
	formatBuf = formattedString.LockBuffer(clock_maxlen);
	
	len = strftime(formatBuf, (size_t)clock_maxlen,
			fText.String(), (const struct tm *)&ltime);
	PRINT(("[TextClockDisplayView::FreshenText] clock string: \n\tfmt=%s, \n\tout=%s\n",
			fText.String(), formatBuf));
	if (0 == len) { // not enough string space or something
		strcpy(formatBuf, "clock error");
		// len = 1;
	}
	formattedString.UnlockBuffer(-1); // *find* the actual strlen
	fDisplayText.SetTo(formattedString);
	PRINT(("[TextClockDisplayView::FreshenText] fDisplayText.String (%p) = '%s'\n",
		fDisplayText.String(), fDisplayText.String()));
}
