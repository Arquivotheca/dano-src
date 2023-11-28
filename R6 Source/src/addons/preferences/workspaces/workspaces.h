#if ! defined WORKSPACES_INCLUDED
#define WORKSPACES_INCLUDED 1

#include <View.h>

class BMessage;
class BPopUpMenu;
class WSview;

class WorkspacesView : public BView
{
	BPopUpMenu	*wsmenu;
	WSview		*ws;

public:
			WorkspacesView();

	void	AttachedToWindow();
	void	MessageReceived(BMessage *msg);
};

#endif
