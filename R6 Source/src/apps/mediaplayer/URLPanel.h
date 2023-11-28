#ifndef _OPEN_URL_PANEL_H
#define _OPEN_URL_PANEL_H

#include <Messenger.h>
#include <Window.h>

class BTextControl;

class URLPanel : public BWindow {
public:

	URLPanel(BPoint position, BMessenger *messenger, const char *text);
	virtual void MessageReceived(BMessage*);
	virtual void WindowActivated(bool active);
	

private:

	BTextControl *fTextControl;
	BMessenger fMessenger;
};

#endif
