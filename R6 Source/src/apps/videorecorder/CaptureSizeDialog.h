#ifndef CAPTURESIZEDIALOG_H
#define CAPTURESIZEDIALOG_H

#include <Window.h>

class BTextControl;

class CaptureSizeDialog : public BWindow {
	public:
		CaptureSizeDialog(BWindow *target, int width, int height);
		void MessageReceived(BMessage*);
		bool QuitRequested();
		
	private:
		BWindow			*fTarget;
		BTextControl	*fWidth, *fHeight;
};

#endif

