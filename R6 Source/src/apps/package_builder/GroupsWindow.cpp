#include <Be.h>
// GroupsWindow.cpp

#include "GroupsWindow.h"
#include "GroupsView.h"
#include "PackMessages.h"
#include "GroupsListView.h"
#include "PackWindow.h"

#include "STextField.h"

#include "Util.h"

#include "MyDebug.h"


GroupsWindow::GroupsWindow(const char *title,GroupList *sharedList,BWindow *parWin)
	:	ChildWindow(	BRect(0,0,260,400),
						title,
						B_TITLED_WINDOW,B_NOT_ZOOMABLE,
						parWin),
		pw(parWin),
		helpHidden(FALSE)
{
	Lock();
	
	PositionWindow(this,0.5,0.333);
			
	////////////////////////////////////////////////////
	//////       Do the main view

	BRect r = Bounds();
	r.top += 14;
	gv = new GroupsView(r,sharedList);
	AddChild(gv);
			
	////////////////////////////////////////////////////
	BView *scrollList = FindView("grouplisting");
	if (scrollList == NULL)
		doError("Failed to find grouplist view");
	
	r = Bounds();
	r.bottom = r.top + 14;
	
	BMenuBar *mbar = new BMenuBar(r,"menubar");
	
	BMenu *menu = new BMenu("Groups"); 
	BMenuItem *item;
	
	item = new BMenuItem("Add Group",new BMessage(M_NEW_GROUP),'A');
	item->SetTarget(scrollList);
	menu->AddItem(item);
	item = new BMenuItem("Add Separator",new BMessage(M_NEW_SEPARATOR));
	item->SetTarget(scrollList);
	menu->AddItem(item);
	item = new BMenuItem("Delete",new BMessage(M_REMOVE_ITEMS),'D');
	item->SetTarget(scrollList);
	menu->AddItem(item);
	
	mbar->AddItem(menu);
	AddChild(mbar);
	
	SetSizeLimits(Bounds().Width(),8192.0,200,8192.0);

	SetDisplayHelp(FALSE);	
	Show();
	Unlock();
}

GroupsWindow::~GroupsWindow()
{
}

void GroupsWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case M_NEW_GROUP:
			DetachCurrentMessage();
			PostMessage(msg,FindView("grouplisting"));
			break;
	}
}

void GroupsWindow::DoSave()
{
	GroupsListView *listView = (GroupsListView *)FindView("grouplisting");
	listView->NameGroup(); 				// dirties attributes
	listView->SetDescription();
	listView->SetHelp();
	
	SetParentDirty();
}

void GroupsWindow::SetParentDirty()
{
	((PackWindow *)pw)->attribDirty = TRUE;
}

bool GroupsWindow::QuitRequested()
{
	if (Dirty())
		DoSave();

	return ChildWindow::QuitRequested();
}

void GroupsWindow::WindowActivated(bool state)
{
	if (!state && Dirty()) {
		DoSave();
	}
	ChildWindow::WindowActivated(state);
}

void GroupsWindow::SetDisplayHelp(bool state)
{
	STextField 	*tf = (STextField *)FindView("grpshelp");
	long height = tf->Frame().Height();
	
	if (state) {
		tf->Show();
		if (helpHidden) {
			ulong saveMode = gv->ResizingMode();
			gv->SetResizingMode(B_FOLLOW_NONE);
			ResizeBy(0,height);
			gv->SetResizingMode(saveMode);		
		}
		helpHidden = FALSE;
	}
	else {
		tf->Hide();
		if (!helpHidden) {
			ulong saveMode = gv->ResizingMode();
			gv->SetResizingMode(B_FOLLOW_NONE);
			ResizeBy(0,-height);
			gv->SetResizingMode(saveMode);
		}
		helpHidden = TRUE;
	}
}
