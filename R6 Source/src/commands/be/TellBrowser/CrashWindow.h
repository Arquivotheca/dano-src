#ifndef _CRASH_WINDOW_H
#define _CRASH_WINDOW_H

#include <Window.h>

#define SHOW_MESSAGE 'show'
#define HIDE_MESSAGE 'hide'

class CrashWindow : public BWindow {
	public:
						CrashWindow(bool quitenabled);
		void			MessageReceived(BMessage *msg);
};

#endif
