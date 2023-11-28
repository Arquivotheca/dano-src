// GroupsWindow.h

#include "GroupList.h"
#include "ChildWindow.h"

class GroupsView;

class GroupsWindow : public ChildWindow
{
public:
	GroupsWindow(const char *title,GroupList *,BWindow *);
	
	virtual			~GroupsWindow();
	
		void		SetParentDirty();
		
		void		DoSave();
virtual bool		QuitRequested();	
virtual void		MessageReceived(BMessage *msg);
virtual void		WindowActivated(bool state);
		void		SetDisplayHelp(bool state);
	
	BMessenger 		settingsViewMessenger;
	BWindow			*pw;
private:

	bool			helpHidden;
	GroupsView		*gv;
};
