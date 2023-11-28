#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Application.h>
#include <Deskbar.h>
#include <Message.h>
#include <Messenger.h>
#include <Node.h>
#include <Screen.h>
#include <View.h>
#include <Window.h>

#include "roster_private.h"

/*
	Note:  all calls with 'fail before sending' have been tagged as such
	in that they now fail gracefully/immediately in case they are being
	used from a view within Deskbar.  Doing so would result in a deadlock.
	Those calls, the remainder, that do not expect a reply or return value
	now send a message using be_app as the Looper instead of the messenger
	to Deskbar.
	
	To be changed after 4.5, hopefully, somehow.

	The following WILL work even if called from a view on the shelf:
	
		BRect 				Frame() const;
			
		deskbar_location	Location(bool* isExpanded=NULL) const;
		status_t			SetLocation(deskbar_location location, bool expanded=false);
			
		bool				IsExpanded() const;
		status_t			Expand(bool yn);
	
		status_t			RemoveItem(int32 id); 
		status_t			RemoveItem(const char* name);

	The following WILL NOT work when called from a view on the shelf:
	
		status_t			GetItemInfo(int32 id, const char **name) const; 
		status_t			GetItemInfo(const char* name, int32 *id) const; 

	 	bool				HasItem(int32 id) const;
	 	bool				HasItem(const char* name) const;
								
		uint32				CountItems() const; 
			
		status_t			AddItem(BView* archivableView, int32* id=NULL); 

	
	Note: this could be fixed if the children of BarView were named.  Then,
	based on the names, we could iterate through the children and find the
	StatusView and thus the Shelf.
	
	rmc (99.05.27)
*/

BDeskbar::BDeskbar()
{
	fMessenger = new BMessenger(TASK_BAR_MIME_SIG);		
}


BDeskbar::~BDeskbar()
{
	delete fMessenger;
}

//	Finds the window named 'Deskbar'; there should only be two windows in
//	Deskbar as of 4.5 (Twitcher and Deskbar)
//	When found, return the ptr to the window or NULL
//	changes to Deskbar window search code should be mirrored in
//	get_deskbar_frame (GraphicsEnv.cpp)
static BWindow*
DeskbarWindow()
{
	if (!be_app)
		return NULL;
	
	BWindow* window;	
	int32 count = be_app->CountWindows();
	for (int32 windex=0 ; windex<count ; windex++) {
		window = be_app->WindowAt(windex);
		if (window && strcmp(window->Title(), "Deskbar") == 0)
			return window;	
	}
	
	return NULL;
}

BRect 
BDeskbar::Frame() const
{
	BRect rect(0,0,0,0);
	
	if (fMessenger->Team() == be_app_messenger.Team()) {
		//	if we are in the team for Deskbar
		//	get the Deskbar window and return its frame directly
		//	any changes to this functions behavior should also be
		//	mirrored in get_deskbar_frame (GraphicsEnv.cpp)
		BWindow* window = DeskbarWindow();
		if (window)
			rect = window->Frame();
	} else {
		BMessage message(B_GET_PROPERTY);	
		message.AddSpecifier("Frame");
		message.AddSpecifier("Window", "Deskbar");
		
		BMessage reply;
		if (fMessenger->SendMessage(&message, &reply) == B_OK)
			reply.FindRect("result", &rect);
	}	
	return rect;
}

//	top view in Deskbar is BarView
//	if not expanded the children of BarView will be
//		Be menu, shelf
//	if expanded it will be
//		Be menu, shelf, app menubar
//	only applies in topleft and topright mode
static bool
DeskbarIsExpanded(BWindow* window)
{
	if (!window)
		return false;
	
	int32 childcount=0;
	BView* barview = window->FindView("BarView");
	if (barview && window->Lock()) {
		childcount = barview->CountChildren();
		window->Unlock();
	}
	return childcount > 2;
	
}

//	Do some simple compares with respect to the screen
//	to determine the physical location of Deskbar
static deskbar_location
DeskbarLocation(bool* expanded)
{
	// Get screen dimensions
	BScreen theScreen(B_MAIN_SCREEN_ID);
	BRect screenBounds = theScreen.Frame();
	BRect deskbarFrame;

	BWindow* window = DeskbarWindow();
	if (window) {
		deskbarFrame = window->Frame();
	} else
		return B_DESKBAR_RIGHT_BOTTOM;
	
	if (expanded)
		*expanded = false;
	if (deskbarFrame.right < screenBounds.Width()
		&& deskbarFrame.left == 0
		&& deskbarFrame.top == 0) {				
		if (expanded)
			*expanded = DeskbarIsExpanded(window);
		return B_DESKBAR_LEFT_TOP;
	} else if (deskbarFrame.left > 0
		&& deskbarFrame.right == screenBounds.Width()
		&& deskbarFrame.top == 0) {
		if (expanded)
			*expanded = DeskbarIsExpanded(window);
		return B_DESKBAR_RIGHT_TOP;
	} else if (deskbarFrame.top == 0
		&& deskbarFrame.left == 0
		&& deskbarFrame.right == screenBounds.Width()) {
		return B_DESKBAR_TOP;
	} else if (deskbarFrame.bottom == screenBounds.Height()
		&& deskbarFrame.left == 0
		&& deskbarFrame.right < screenBounds.Width()){
		return B_DESKBAR_LEFT_BOTTOM;
	} else if (deskbarFrame.bottom == screenBounds.Height()
		&& deskbarFrame.right == screenBounds.Width()
		&& deskbarFrame.left > 0) {
		return B_DESKBAR_RIGHT_BOTTOM;
	} else if (deskbarFrame.bottom == screenBounds.Height()) {
		return B_DESKBAR_BOTTOM;
	} else
		return B_DESKBAR_BOTTOM;
}

deskbar_location 
BDeskbar::Location(bool* expanded) const
{
	deskbar_location location=B_DESKBAR_RIGHT_BOTTOM;
	if (fMessenger->Team() == be_app_messenger.Team()) {
		//	if in the Deskbar team
		//	figure out where the window is and return directly
		location = DeskbarLocation(expanded);
	} else {
		BMessage message('gloc');
		BMessage replyMsg;
		if (fMessenger->SendMessage(&message, &replyMsg) != B_OK)
			return B_DESKBAR_TOP;
		
		int32 loc;	
		if (replyMsg.FindInt32("location", &loc) != B_OK)
			return B_DESKBAR_TOP;
		
		if (expanded) {
			if (replyMsg.FindBool("expanded", expanded) != B_OK)
				*expanded = false;
		}
			
		location = (deskbar_location)loc;
	}
	return location;
}

status_t 
BDeskbar::SetLocation(deskbar_location location, bool expand)
{
	BMessage message('sloc');
	int32 loc = (int32)location;
	message.AddInt32("location", loc);
	message.AddBool("expand", expand);

	if (fMessenger->Team() == be_app_messenger.Team())
		return be_app->PostMessage(&message);			// see note above
	else {
		BMessage replyMsg;
		return fMessenger->SendMessage(&message, &replyMsg);
	}
}

bool
BDeskbar::IsExpanded() const
{
	if (fMessenger->Team() == be_app_messenger.Team()) {
		//	see note above in Location
		BWindow* window = DeskbarWindow();
		if (window) {
			bool expanded = DeskbarIsExpanded(window);
			return expanded;
		} else
			return false;
	} else {
		BMessage message('gexp');	
		BMessage replyMsg;
		if (fMessenger->SendMessage(&message, &replyMsg) == B_OK) {
			bool expanded;
			replyMsg.FindBool("expanded", &expanded);
			return expanded;
		} else
			return false;
	}
}

status_t
BDeskbar::Expand(bool expand)
{
	BMessage message('sexp');
	message.AddBool("expand", expand);
	if (fMessenger->Team() == be_app_messenger.Team()) {
		return be_app->PostMessage(&message);			// see note above
	} else {
		BMessage replyMsg;
		return fMessenger->SendMessage(&message, &replyMsg);
	}
}

status_t
BDeskbar::GetItemInfo(int32 id, const char **name) const
{
	if (!*name)
		return B_BAD_VALUE;

	status_t err=B_OK;
	if (fMessenger->Team() == be_app_messenger.Team()) {
		*name = NULL;			//	fail before sending, see note above
		err = B_ERROR;
	} else {
		BMessage message('info');
		message.AddInt32("id", id);
		
		BMessage reply;
		if ((err = fMessenger->SendMessage(&message, &reply)) != B_OK)
			return err;
	
		const char* tmp;
		if ((err = reply.FindString("name", &tmp)) != B_OK)
			return err;
		*name = strdup(tmp);
	}		
	return err;
}

status_t
BDeskbar::GetItemInfo(const char* name, int32 *id) const
{
	if (!name || !id)
		return B_BAD_VALUE;

	status_t err=B_OK;
	if (fMessenger->Team() == be_app_messenger.Team()) {
		err = B_ERROR;			//	fail before sending, see not above
		*id = 0;
	} else {
		BMessage message('info');
		message.AddString("name", name);
		
		BMessage reply;
		if ((err = fMessenger->SendMessage(&message, &reply)) != B_OK)
			return err;	
	
		if ((err = reply.FindInt32("id", id)) != B_OK)
			return err;
	}	
	return err;
}

bool 
BDeskbar::HasItem(int32 id) const
{
	if (id <= 0)
		return false;
		
	bool exists=false;	
	if (fMessenger->Team() == be_app_messenger.Team()) {
		exists = false;			//	fail before sending, see note above
	} else {
		BMessage message('exst');
		message.AddInt32("id", id);
			
		BMessage reply;
		if (fMessenger->SendMessage(&message, &reply) != B_OK)
			return false;
	
		if (reply.FindBool("exists", &exists) != B_OK)
			exists = false;
	}	
	return exists;
}

bool 
BDeskbar::HasItem(const char* name) const
{
	if (!name)
		return false;
	
	bool exists=false;	
	if (fMessenger->Team() == be_app_messenger.Team()) {
		exists = false;			//	fail before sending, see note above
	} else {		
		BMessage message('exst');
		message.AddString("name", name);
		
		BMessage reply;
		if (fMessenger->SendMessage(&message, &reply) != B_OK)
			return false;
	
		if (reply.FindBool("exists", &exists) != B_OK)
			exists = false;
	}	
	return exists;
}

uint32 
BDeskbar::CountItems() const
{
	int32 count=0;
	if (fMessenger->Team() == be_app_messenger.Team()) {
		count = 0;				//	fail before sending, see note above
	} else {
		BMessage message('cwnt');
		BMessage replyMsg;
		if (fMessenger->SendMessage(&message, &replyMsg) == B_OK)
			if (replyMsg.FindInt32("count", &count) != B_OK)
				count = 0;
	}
		
	return count;
}

status_t 
BDeskbar::AddItem(BView* archivableView, int32* id)
{
	if (!archivableView)
		return B_BAD_VALUE;
		
	status_t err=B_OK;
	if (fMessenger->Team() == be_app_messenger.Team()) {
		err = B_ERROR;				//	fail before sending, see note above
	} else {
		BMessage message('icon');
		BMessage data(B_ARCHIVED_OBJECT);
		if ((err = archivableView->Archive(&data)) != B_OK)
			return err;	
		
		message.AddMessage("view", (const BMessage*)&data);
		BMessage reply;
		if ((err = fMessenger->SendMessage(&message, &reply)) != B_OK)
			return err;
		
		if (id) {
			if ((err = reply.FindInt32("id", id)) != B_OK)
				return err;
		}
		reply.FindInt32("error", &err);
	}
	
	return err;
}

status_t 
BDeskbar::AddItem(entry_ref* addon, int32* id)
{
	status_t err=B_ERROR;
	if (!addon)
		return err;
	
	//
	//	add the attribute that Deskbar will look for later on
	//	will be removed later on when RemoveItem is used to remove
	//		the replicant - handled in StatusView::RemoveItem(id)
	//
	BNode node(addon);
	if (node.WriteAttr("be:deskbar_item_status", B_STRING_TYPE,
		0, "enabled", 7) != 7)
		return err;

	//
	//	the entry_ref is sent so that Deskbar can deal
	//	with changes/removal, etc
	//
	BMessage message('adon');	
	message.AddRef("addon", addon);

	if (fMessenger->Team() == be_app_messenger.Team()) {
		//
		//	id is incorrect for now
		//
		if (id)
			*id = -1;
		err = be_app->PostMessage(&message);
	} else {
		BMessage reply;
		if ((err = fMessenger->SendMessage(&message, &reply)) != B_OK)
			return err;
		
		if (id) {
			if ((err = reply.FindInt32("id", id)) != B_OK)
				return err;
		}
		reply.FindInt32("error", &err);
	}
		
	return err;
}

//
//	if AddItem(entry_ref) was used to add the item,
//	a call to RemoveItem(id|name) will remove the attribute
//	that was added
//
status_t 
BDeskbar::RemoveItem(int32 id)
{
	if (id < 0)
		return B_ERROR;
		
	BMessage message('remv');		
	message.AddInt32("id", id);
	
	if (fMessenger->Team() == be_app_messenger.Team()) {
		return be_app->PostMessage(&message);			// see note above
	} else {
		BMessage replyMsg;
		return fMessenger->SendMessage(&message, &replyMsg);
	}
}

status_t 
BDeskbar::RemoveItem(const char *name)
{
	if (!name)
		return B_ERROR;
		
	BMessage message('remv');		
	message.AddString("name", name);
	
	if (fMessenger->Team() == be_app_messenger.Team()) {
		return be_app->PostMessage(&message);			// see note above
	} else {
		BMessage replyMsg;
		return fMessenger->SendMessage(&message, &replyMsg);
	}
}

#if 0
BRect 
BDeskbar::AddOnFrame(int32 id)
{
	BMessenger messenger(TASK_BAR_MIME_SIG);
	if (!messenger.IsValid())
		return BRect(0,0,0,0);

	BMessage message('ifrm');
	message.AddInt32("index", id);
	
	BMessage reply;
	if (messenger.SendMessage(&message, &reply) != B_OK)
		return BRect(0,0,0,0);
	
	BRect frame;
	if (reply.FindRect("frame", &frame) != B_OK)
		return BRect(0,0,0,0);
	
	return frame;
}

BRect 
BDeskbar::AddOnFrame(const char* name)
{
	BMessenger messenger(TASK_BAR_MIME_SIG);
	if (!messenger.IsValid())
		return BRect(0,0,0,0);

	BMessage message('ifrm');
	message.AddString("name", name);
	
	BMessage reply;
	if (messenger.SendMessage(&message, &reply) != B_OK)
		return BRect(0,0,0,0);
	
	BRect frame;
	if (reply.FindRect("frame", &frame) != B_OK)
		return BRect(0,0,0,0);
	
	return frame;
}
#endif
