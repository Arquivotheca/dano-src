#ifndef QUICKRES_ADD_WINDOW_H
#define QUICKRES_ADD_WINDOW_H

#include <View.h>
#include <Window.h>
#include <String.h>

#include <ResourceRoster.h>
#include <ToolTipHook.h>

class BButton;
class BListView;
class BMenuField;
class BTextControl;

enum {
	ADDWIN_ACTIVATE = 'Aact'
};

enum add_types {
	ADD_ATTRIBUTES_ONLY,
	ADD_RESOURCES_ONLY,
	ADD_ANYTHING
};

class AddWindow : public BWindow, public BToolTipFilter
{
public:
	AddWindow(BList* items, add_types types, BMessenger target,
			   BPoint center, const char* doc_name,
			   window_look look = B_TITLED_WINDOW_LOOK,
			   window_feel feel = B_NORMAL_WINDOW_FEEL,
			   uint32 flags = B_ASYNCHRONOUS_CONTROLS,
			   uint32 workspace = B_CURRENT_WORKSPACE);

	virtual void	MessageReceived(BMessage *msg);
	
	virtual void	Quit();
	virtual bool	QuitRequested();
	
private:
	void			UpdateControls();
	
	BString					fDocName;
	add_types				fTypes;
	BMessenger				fTarget;
	BListView*				fList;
	BTextControl*			fID;
	BMenuField*				fType;
	BTextControl*			fName;
	BButton*				fAddButton;
	BButton*				fCloseButton;
};

#endif
