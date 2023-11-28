#ifndef __KalWindow
#define __KalWindow

#ifndef _WINDOW_H
#include <Window.h>
#endif

class KalDraw;
class BMenuBar;

class KalWindow : public BWindow {

public:				KalWindow(BRect frame);
virtual	void		FrameResized(float width, float height);
virtual	bool		QuitRequested(void);
virtual	void		MessageReceived(BMessage* msg);

		KalDraw		*fDrawThread;

private:
		BMenuBar	*menuBar;
};

#endif
