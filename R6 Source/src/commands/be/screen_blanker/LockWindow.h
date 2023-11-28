#if ! defined LOCKWINDOW_INCLUDED
#define LOCKWINDOW_INCLUDED

#include <Window.h>

class BTextControl;
class BTextView;
class BButton;

class LockWindow : public BWindow
{
public:
			LockWindow(bool *unlocked);
	void	MessageReceived(BMessage *msg);
	void	ScreenChanged(BRect frame, color_space mode);

	int32			net;
	BTextControl	*pass;
	bool			*unlock;
	BButton			*ok;
};

#endif
