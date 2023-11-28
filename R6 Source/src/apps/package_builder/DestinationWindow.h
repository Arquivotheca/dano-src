// DestinationWindow.h

#include "DestinationList.h"
#include "ChildWindow.h"

class DestListView;
class PackWindow;

class DestinationWindow : public ChildWindow
{
public:
	DestinationWindow(const char *title,DestList *,PackWindow *_pw);
	virtual			~DestinationWindow();

	virtual void	MessageReceived(BMessage *m);
	virtual bool	QuitRequested();
	
	BMessenger		settingsViewMessenger;
	bool			windowDirty;
	BView			*scrollList;
};
