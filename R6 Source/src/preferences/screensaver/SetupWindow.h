#if ! defined SETUPWINDOW_INCLUDED
#define SETUPWINDOW_INCLUDED 1

#include <Window.h>
#include <Rect.h>
#include <Messenger.h>

class BMessage;
class BTabView;

class SetupWindow : public BWindow
{
	BTabView	*tabView;
public:
			SetupWindow(BRect frame, BMessenger roster);
	bool	QuitRequested();
	void	MessageReceived(BMessage *msg);
	void	SaveState();
};

#endif
