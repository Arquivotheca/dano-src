// ---------------------------------------------------------------------------
/*
	MDynamicMenuHandler.cpp
	
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			5 January 1999

	Taken from...
	MWindowsMenu.cpp
	(Copyright 1995 Metrowerks Corporation, All Rights Reserved.)
	
	Static class for handling window and project lists.
*/
// ---------------------------------------------------------------------------

#include <string.h>

#include "MDynamicMenuHandler.h"
#include "MainMenus.h"
#include "MTextWindow.h"
#include "MProjectWindow.h"
#include "MPreferencesWindow.h"
#include "MFindWindow.h"
#include "MKeyBindingManager.h"
#include "IDEApp.h"
#include "IDEMessages.h"
#include "MDefaultPrefs.h"
#include "ProjectCommands.h"
#include "MLocker.h"
#include <SupportDefs.h>
#include <Menu.h>
#include <Message.h>
#include <Node.h>
#include <NodeMonitor.h>
#include <unistd.h>

BLocker					MDynamicMenuHandler::fLocker("dynamicmenu");
MList<WindowRec*>		MDynamicMenuHandler::fWindowList;
MList<entry_ref*>		MDynamicMenuHandler::fRecentProjectList;
MList<entry_ref*>		MDynamicMenuHandler::fRecentWindowList;
bool					MDynamicMenuHandler::fShortcuts[10];

// this should come from preferences some day

const int32 kMaxRecentProjects = 10;
const int32 kMaxRecentWindows = 15;

// ---------------------------------------------------------------------------
//	class WindowRec - 
// ---------------------------------------------------------------------------

class WindowRec {
public:
	BWindow*							sWindow;
	int32								sShortcut;
	MDynamicMenuHandler::EWindowKind	sKind;
};

// ---------------------------------------------------------------------------
// MDynamicMenuHandler static member functions
// ---------------------------------------------------------------------------

MDynamicMenuHandler::MDynamicMenuHandler()
{
	for (int i = 0; i < 10; i++)
		fShortcuts[i] = false;
}

// ---------------------------------------------------------------------------

MDynamicMenuHandler::~MDynamicMenuHandler()
{
	// Destructor allows cleanup of the window recs.  Only a single object
	// should exist.  It should be an included object in the app object.
	
	int				count = 0;
	WindowRec*		rec;
	entry_ref*		proj;
	entry_ref*		win;
	
	while (fWindowList.GetNthItem(rec, count++)) {
		delete rec;
	}
	
	count = 0;
	while (fRecentProjectList.GetNthItem(proj, count++)) {
		delete proj;
	}

	count = 0;
	while (fRecentWindowList.GetNthItem(win, count++)) {
		delete win;
	}

}

// ---------------------------------------------------------------------------

void
MDynamicMenuHandler::TextWindowOpened(MTextWindow* inWindow)
{
	// A text window has opened.
	MDynamicMenuHandler::DoWindowOpened(inWindow, kText);	
}

// ---------------------------------------------------------------------------

void
MDynamicMenuHandler::MessageWindowOpened(BWindow* inWindow)
{
	// A result window has opened
	MDynamicMenuHandler::DoWindowOpened(inWindow, kMessage);	
}

// ---------------------------------------------------------------------------

void
MDynamicMenuHandler::DoWindowOpened(BWindow* inWindow, EWindowKind whichKind)
{
	// Keep track of the window in our list
	// and assign a shortcut if possible
	
	MLocker<BLocker> lock(fLocker);
	WindowRec* rec = new WindowRec;

	fWindowList.AddItem(rec);

	rec->sWindow = inWindow;
	rec->sShortcut = 0;
	rec->sKind = whichKind;
	if (whichKind == kText) {	
		rec->sShortcut = GetNextShortcut();
		fShortcuts[rec->sShortcut] = true;
	}
}

// ---------------------------------------------------------------------------

void
MDynamicMenuHandler::TextWindowClosed(MTextWindow* inWindow)
{
	// A text window was closed,
	// Remove it from the list, recycle shortcut
	// and add it to our most recent list

	// (grab the lock here although we get it again in DoWindowClosed,
	// so that we don't unlock in the middle of this method)
	MLocker<BLocker> lock(fLocker);

	MDynamicMenuHandler::DoWindowClosed(inWindow);
	

	// Add the window to our recent window list 
	// (but only if it has a valid ref and isn't in there already)
	entry_ref ref;
	if (inWindow->GetRef(ref) == B_OK 
				&& MDynamicMenuHandler::FindRecent(fRecentWindowList, ref) == NULL) {
		if (fRecentWindowList.CountItems() >= kMaxRecentWindows) {
			entry_ref* oldest = fRecentWindowList.RemoveItemAt(0);
			delete oldest;
		}
		MDynamicMenuHandler::AddRecent(fRecentWindowList, new entry_ref(ref));
	}

	MFindWindow::GetFindWindow().PostMessage(msgTextWindowClosed);

	if (fWindowList.CountItems() == 0)
		be_app_messenger.SendMessage(msgWindowsAllGone);
}

// ---------------------------------------------------------------------------

void
MDynamicMenuHandler::MessageWindowClosed(BWindow* inWindow)
{
	MDynamicMenuHandler::DoWindowClosed(inWindow);
}

// ---------------------------------------------------------------------------

void
MDynamicMenuHandler::DoWindowClosed(BWindow* inWindow)
{
	MLocker<BLocker> lock(fLocker);
	WindowRec* rec = GetItem(inWindow);

	// if the window is in our shortcut list, remove it
	if (rec) {
		fWindowList.RemoveItem(rec);
	
		fShortcuts[rec->sShortcut] = false;
	
		delete rec;
	}
}

// ---------------------------------------------------------------------------

void
MDynamicMenuHandler::ProjectOpened(MProjectWindow* inProject)
{
	// A project opened (called from the app)
	// Notify any clients who need to know about the new project.

	MLocker<BLocker> lock(fLocker);

	BMessage msg(msgProjectOpened);
	msg.AddPointer(kProjectMID, inProject);
	
	MFindWindow::GetFindWindow().PostMessage(&msg);
	MDynamicMenuHandler::MessageToAllTextWindows(msg);

	MDynamicMenuHandler::DoWindowOpened(inProject, kProject);	
}

// ---------------------------------------------------------------------------

void
MDynamicMenuHandler::ProjectClosed(MProjectWindow* inProject)
{
	// When a project closes - do a bunch of cleanup
	// (mostly we only worry about when we aren't quiting
	// however, do the recent projects no matter what)

	// look before doing anything...
	MLocker<BLocker> lock(fLocker);

	MDynamicMenuHandler::DoWindowClosed(inProject);
	
	// add the project to our list of recent projects
	entry_ref ref;
	inProject->GetRef(ref);
	if (MDynamicMenuHandler::FindRecent(fRecentProjectList, ref) == NULL) {
		if (fRecentProjectList.CountItems() >= kMaxRecentProjects) {
			entry_ref* oldest = fRecentProjectList.RemoveItemAt(0);
			delete oldest;
		}
		MDynamicMenuHandler::AddRecent(fRecentProjectList, new entry_ref(ref));
	}

	// If we aren't quiting - do the rest
	// Notify...
	//	the app itself
	//	the Find window
	// 	all the text windows
	
	if (IDEApp::BeAPP().IsQuitting() == false)
	{
		BMessage msg(msgProjectClosed);
		msg.AddPointer(kProjectMID, inProject);

		IDEApp::BeAPP().ProjectClosed(inProject);
		MFindWindow::GetFindWindow().PostMessage(&msg);	
		MessageToAllTextWindows(msg);
	}
}

// ---------------------------------------------------------------------------

entry_ref*
MDynamicMenuHandler::FindRecent(const MList<entry_ref*>& recentList, const entry_ref& alias)
{
	// Use "alias" as a key to look up the corresponding entry_ref in "recentList"
	
	entry_ref* found = NULL;
	entry_ref* nthRef;
	int i = 0;
	
	for (int32 i = 0; recentList.GetNthItem(nthRef, i); i++) {
		if (*nthRef == alias) {
			found = nthRef;
			break;
		}
	}
	
	return found;
}

// ---------------------------------------------------------------------------

int32
MDynamicMenuHandler::FindDeletedNode(const MList<entry_ref*>& recentList, const node_ref& node)
{
	// Look for the entry_ref that matches the node
	// return the index of our entry_ref
	
	// There are two ways to find the deleted node
	// 1. match the node
	// 2. when the conversion BEntry->node_ref fails 
	// Since I don't keep a BFile/BEntry around, the conversion
	// normally fails and I never get to test for equality
	
	int32 index = -1;
	entry_ref* nthRef;
	node_ref nref;
	int i = 0;
	
	for (int32 i = 0; recentList.GetNthItem(nthRef, i); i++) {
		BEntry entry(nthRef);
		if (entry.GetNodeRef(&nref) != B_OK || nref == node) {
			index = i;
			break;
		}
	}
	return index;
}

// ---------------------------------------------------------------------------

void
MDynamicMenuHandler::WindowActivated(BWindow* inWindow)
{
	// Text files notify us when they are activated so we can keep our window 
	// list in order.

	MLocker<BLocker> lock(fLocker);
	WindowRec* rec = GetItem(inWindow);

	if (rec) {
		// Move this window to the front of the list
		fWindowList.MoveItemTo(rec, 0);
	}
}

// ---------------------------------------------------------------------------

void
MDynamicMenuHandler::MessageToAllTextWindows(BMessage& inMessage)
{
	// Post a copy of the message to all text windows.
	// (only do the text windows)
	
	MLocker<BLocker> lock(fLocker);
	int count = 0;
	WindowRec* rec;

	while (fWindowList.GetNthItem(rec, count++)) {
		if (rec->sKind == kText) {
			rec->sWindow->PostMessage(&inMessage);
		}
	}
}

// ---------------------------------------------------------------------------
//		CloseAllTextWindows
// ---------------------------------------------------------------------------
//	Ask all text windows to close. (only text windows)

bool
MDynamicMenuHandler::CloseAllTextWindows()
{
	Lock();
	MList<WindowRec*>		windowList(fWindowList);
	Unlock();

	int						count = 0;
	bool					result = true;
	bool					okToQuit;
	WindowRec*				rec;
	BMessage				msg(msgCustomQuitRequested);
	BMessage				reply;

	while (windowList.GetNthItem(rec, count++)) {
		if (rec->sKind == kText) {
			BMessenger window(rec->sWindow);
			msg.MakeEmpty();
			reply.MakeEmpty();
	
			if (B_OK == window.SendMessage(&msg, &reply) &&
				B_OK == reply.FindBool(kOKToQuit, &okToQuit)) {
				if (! okToQuit) {
					result = false;
					break;			
				}
				else {
					if (rec->sWindow->Lock()) {
						rec->sWindow->Quit();
					}
				}	
			}
		}
	}
	
	return result;
}

// ---------------------------------------------------------------------------

bool
MDynamicMenuHandler::SaveAllTextWindows()
{
	// Tell all text windows to save.
	Lock();
	MList<WindowRec*> windowList(fWindowList);
	Unlock();

	int						count = 0;
	bool					result = true;
	bool					savedWindows = false;
	WindowRec*				rec;
	
	while (windowList.GetNthItem(rec, count++))
	{
		if (rec->sKind == kText) {
			MTextWindow* theTextWindow = (MTextWindow*) rec->sWindow;
			MLocker<BLooper> lock(theTextWindow);
	
			if (lock.IsLocked()) {
				bool	saved = theTextWindow->DoSave(false);
				savedWindows |= saved;
	
				if (saved == false) {
					result = false;
					break;
				}
			}
		}
	}

	if (savedWindows) {
		sync();		// flush to disk
	}
	
	return result;
}

// ---------------------------------------------------------------------------

MTextWindow*
MDynamicMenuHandler::GetFrontTextWindow()
{
	MLocker<BLocker> lock(fLocker);
	MTextWindow* textWindow = nil;
	WindowRec* rec = nil;
	int32 count = 0;
	
	// We need to loop because the front window(s) could be message windows
	
	while (fWindowList.GetNthItem(rec, count++)) {
		if (rec->sKind == kText) {
			textWindow = (MTextWindow*) rec->sWindow;
			break;
		}
	}
	
	return textWindow;
}

// ---------------------------------------------------------------------------
//		FindTextWindowForFile
// ---------------------------------------------------------------------------

MTextWindow *
MDynamicMenuHandler::FindTextWindowForFile(
	BEntry * 	inFile)
{
	MLocker<BLocker> lock(fLocker);
	MTextWindow* result = nil;
	int32 count = 0;
	WindowRec* rec;
	entry_ref entry;

	if (B_NO_ERROR == inFile->GetRef(&entry))
	{
		while (fWindowList.GetNthItem(rec, count++)) {
			if (rec->sKind == kText && ((MTextWindow*)(rec->sWindow))->IsMatch(entry)) {
				result = (MTextWindow*) rec->sWindow;
				break;
			}
		}
	}
	
	return result;
}

// ---------------------------------------------------------------------------

void
MDynamicMenuHandler::UpdateDynamicMenus(BMenuBar& inMenuBar, MProjectWindow* project)
{
	// Called from MenusBeginning
	// Performs 3 actions
	// step 1 - update the list of active windows in windows menu
	// step 2 - update the project name in windows menu
	// step 3 - enable/disable the errors & warnings item based on open project
	// step 4 - update the list of recent projects and files
		
	MLocker<BLocker> lock(fLocker);
	
	// Get the menus that we are going to be updating (File and Window)
	BMenuItem* fileItem = inMenuBar.FindItem(kFileMenuName);
	BMenu* fileMenu = inMenuBar.SubmenuAt(inMenuBar.IndexOf(fileItem));
	BMenuItem* windowItem = inMenuBar.FindItem(kWindowMenuName);
	BMenu* windowMenu = inMenuBar.SubmenuAt(inMenuBar.IndexOf(windowItem));
	
	// step 1 - handle list of active windows
	int32 numItems = kWinMenuMinItemCount + fWindowList.CountItems();
	int32 existingItems = windowMenu->CountItems();
	int32 index;
	BMenuItem* item;
	BMessage* msg;

	// Remove all the extra text menu items
	for (index = existingItems - 1; index >= kWinMenuMinItemCount; index--) {
		delete windowMenu->RemoveItem(index);
	}	

	// Generate new menu items for each text window
	for (index = kWinMenuMinItemCount; index < numItems; index++) {
		WindowRec*		rec;

		fWindowList.GetNthItem(rec, index - kWinMenuMinItemCount);
		ASSERT(rec);
		int32			shortcut = rec->sShortcut;
		CommandT		command = msgWindowMenuClicked;
		KeyBinding		binding  = kEmptyBinding;
		
		// If this window has a shortcut get the key binding.
		if (shortcut > 0) {
			command = shortcut + cmd_FirstWindowListCmd;
			MKeyBindingManager::Manager().GetBinding(kBindingGlobal, command, binding);
		}

		msg = new BMessage(command);
		item = new BMenuItem(rec->sWindow->Title(), msg, binding.keyCode, binding.modifiers);
		
		windowMenu->AddItem(item);
	}

	// step 2 - handle the project name
	BMenuItem* projectItem = windowMenu->FindItem(cmd_ShowProjectWindow);
	ASSERT(projectItem);

	if (project) {
		projectItem->SetLabel(project->Title());
		projectItem->SetEnabled(true);
	}
	else {
		projectItem->SetLabel("No Owning Project");
		projectItem->SetEnabled(false);
	}

	// step 3 - enable/disable the errors & warnings item based on open project
	BMenuItem* errorMessageItem = windowMenu->FindItem(cmd_ErrorMessageWindow);
	errorMessageItem->SetEnabled(project ? true : false);
	
	// step 4 - update the list of recent projects and files
	item = fileMenu->ItemAt(kOpenRecentMenuItemIndex);
	if (item != nil) {
		BMenu* recentMenu = item->Submenu();
		ASSERT(recentMenu);
		
		int32 projectCount = fRecentProjectList.CountItems();
		int32 windowCount = fRecentWindowList.CountItems();
		
		if (recentMenu) {
			// just clean out the old menu
			for (int32 i = recentMenu->CountItems() - 1; i >= 0; i--) {
				BMenuItem* oldItem = recentMenu->RemoveItem(i);
				delete oldItem;
			}
			
			entry_ref currentProject;
			entry_ref* aRef;
			bool enabled = false;
				
			if (project) {
				project->GetRef(currentProject);
			}

			// add any project items
			for (int32 i = 0; fRecentProjectList.GetNthItem(aRef, i); i++) {
				BMessage* msg = new BMessage(msgOpenRecent);
				BMenuItem* newItem = new BMenuItem(aRef->name, msg);
				recentMenu->AddItem(newItem);
				if (currentProject == *aRef) {
					newItem->SetEnabled(false);
				}
				else {
					newItem->SetEnabled(true);
					enabled = true;
				}
			}
			
			// add the separator if necessary
			if (projectCount > 0 && windowCount > 0) {
				recentMenu->AddSeparatorItem();
			}
			
			// now add the window items
			for (int32 i = 0; fRecentWindowList.GetNthItem(aRef, i); i++) {
				BMessage* msg = new BMessage(msgOpenRecent);
				BMenuItem* newItem = new BMenuItem(aRef->name, msg);
				recentMenu->AddItem(newItem);
				newItem->SetEnabled(true);
				enabled = true;
			}
			
			// when we are all done, if we haven't added any items, add a bogus item
			if (recentMenu->CountItems() == 0) {
				BMenuItem* newItem = new BMenuItem("No Projects or Files", NULL);
				newItem->SetEnabled(false);
				recentMenu->AddItem(newItem);
			}
			
			recentMenu->SetEnabled(enabled);
		}
	}	
}

// ---------------------------------------------------------------------------
 
void
MDynamicMenuHandler::AddHierarchalMenuItems(BMenuBar& inBar)
{
	BMenuItem* item = inBar.FindItem(kRecentProjectMenuKey);

	if (item != nil) {
		BMenu* theMenu = item->Menu();
		ASSERT(theMenu);
		if (theMenu) {
			BMenu* recentMenu = new BMenu("Open Recent");
			theMenu->AddItem(recentMenu, kOpenRecentMenuItemIndex);
			recentMenu->SetEnabled(false);
		}
	}
}


// ---------------------------------------------------------------------------
//		GetNextShortcut
// ---------------------------------------------------------------------------
//	Get the next available shortcut.  Valid shortcuts are in the range 1-9.

int32
MDynamicMenuHandler::GetNextShortcut()
{
	int32			result = 0;

	for (int i = 1; i <= 9; i++)
	{
		if (! fShortcuts[i])
		{
			result = i;
			break;
		}
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//		MenuClicked
// ---------------------------------------------------------------------------
//	A window in the window menu or a project in the project submenu was chosen.

void
MDynamicMenuHandler::MenuClicked(
	BMessage& inMessage)
{
	int32				index = -1;

	(void) inMessage.FindInt32("index", &index);

	switch (inMessage.what)
	{
		case msgWindowMenuClicked:
		{
			Lock();

			WindowRec*		rec = nil;
			bool			moveToCurrentWorkSpace = false;

			if (B_OK != inMessage.FindBool(kUseOptionKey, &moveToCurrentWorkSpace))
				moveToCurrentWorkSpace = (modifiers() & (B_OPTION_KEY | B_CONTROL_KEY)) != 0;

			if (fWindowList.GetNthItem(rec, index - kWinMenuMinItemCount))
			{
				Unlock();
				if (rec->sWindow->Lock())
				{
					// (always move message windows to where we are)
					if (moveToCurrentWorkSpace || rec->sKind == kMessage)
					{
						rec->sWindow->SetWorkspaces(B_CURRENT_WORKSPACE);
						ValidateWindowFrame(*rec->sWindow);		// make sure it's onscreen
					}
					rec->sWindow->Activate();	// locks window, deadlocks live here
					rec->sWindow->Unlock();
				}
			}
			else
				Unlock();

			ASSERT(rec);
			break;
		}
		
		case msgOpenRecent:
		{
			// if the index is more than the number of recent projects
			// then it is in the list of recent files...
			MLocker<BLocker> lock(fLocker);
			entry_ref* ref;
			
			int32 projectCount = fRecentProjectList.CountItems();
			MList<entry_ref*>* recentList = NULL;
			if (index < projectCount) {
				recentList = &fRecentProjectList;
			}
			else {
				recentList = &fRecentWindowList;
				if (projectCount > 0) {
					projectCount += 1;		// extra increment for the separator
				}
				index -= projectCount;
			}
			
			// Send the entry off to the IDEApp to open it up
			// while we are at it, verify that the entry exists
			if (recentList->GetNthItem(ref, index)) {
				BMessage msg(B_REFS_RECEIVED);
				msg.AddRef("refs", ref);
				be_app_messenger.SendMessage(&msg);
			}
			break;
		}

		case cmd_Stack:
			Stack();
			break;

		case cmd_Tile:
			Tile(kMatrixTile);
			break;

		case cmd_TileVertical:
			Tile(kVerticalTile);
			break;
	}
}

const float		kWindowMargin = 5.0;
const float		kMaxWindowWidth = 500.0;
const float		kMinWindowWidth = 150.0;

// ---------------------------------------------------------------------------
//		Stack
// ---------------------------------------------------------------------------
//	Stack the windows in groups of 5.  Starting at the left edge of the window
//	move the windows incrementally right and down.

void
MDynamicMenuHandler::Stack()
{
	MLocker<BLocker>		lock(fLocker);

	const uint32			currentWorkspace = 1 << current_workspace();

	// Stack (only text) source windows
	const BPoint	increment(5.0, 20.0);
	WindowRec*		rec;
	BWindow*		wind;
	int32			windowCount = TextWindowsInThisWorkSpace(wind);
	
	if (windowCount > 0)
	{
		BScreen			screen(wind);
		int32			movedCount = 0;
		int32			count = fWindowList.CountItems() - 1;
		BRect			frame = screen.Frame();
		BPoint			start(kWindowMargin, 30.0);
		BPoint			topleft = start;
		const float		kWindowWidth = min(kMaxWindowWidth, (float) (frame.right - start.x - (5.0 * 5.0)));

		while (fWindowList.GetNthItem(rec, count--)) {
			if (rec->sKind == kText) {
				if (rec->sWindow->Lock()) {
					if ((rec->sWindow->Workspaces() & currentWorkspace) != 0) {
						rec->sWindow->MoveTo(topleft);
						rec->sWindow->ResizeTo(kWindowWidth, frame.bottom - topleft.y - kWindowMargin);
		
						if (++movedCount % 5 == 0) {
							topleft = start;
						}
						else {
							topleft += increment;
						}
					}
					
					rec->sWindow->Unlock();
				}
			}
		}
	}
}

// ---------------------------------------------------------------------------
//		Tile
// ---------------------------------------------------------------------------
//	Move the windows into a row and column matrix.

void
MDynamicMenuHandler::Tile(
	TileType	inTileType)
{
	MLocker<BLocker>	lock(fLocker);
	const uint32		currentWorkspace = 1 << current_workspace();
	BWindow*			wind;
	const int32			windowCount = TextWindowsInThisWorkSpace(wind);
	
	// Tile (only text) source windows
	if (windowCount > 0)
	{
		BScreen			screen(wind);
		BRect			frame = screen.Frame();
		int32			rows;
		int32			cols;
	
		switch (inTileType)
		{
			case kMatrixTile:
				rows = (int32) ceil(sqrt((float) windowCount));
				cols = (int32) (ceil((float) windowCount / (float) rows));
				break;
	
			case kVerticalTile:
				rows = 1;
				cols = windowCount;
				break;
		}
	
		const float		kMinWindowHeight = 150.0;
		float			top = 30;
		float			left = kWindowMargin;
		BPoint			topleft(left, top);
		const float		windowHIncrement = floor((frame.right - topleft.x) / cols);
		const float		windowVIncrement = floor((frame.bottom - topleft.y) / rows);
		const float		windowWidth = max(kMinWindowWidth, windowHIncrement - 10.0f);
		const float		windowHeight = max(kMinWindowHeight, windowVIncrement - 10.0f);
		int32			movedCount = 0;
		int32			count = fWindowList.CountItems() - 1;
		WindowRec*		rec;
	
		while (fWindowList.GetNthItem(rec, count--))
		{
			if (rec->sKind == kText) {
				if (rec->sWindow->Lock()) {
					if ((rec->sWindow->Workspaces() & currentWorkspace) != 0) {
						rec->sWindow->MoveTo(topleft);
						rec->sWindow->ResizeTo(windowWidth, windowHeight);
		
						if (++movedCount % rows == 0) {
							left += windowHIncrement;
							topleft.Set(left, top);
						}
						else {
							topleft.y += windowVIncrement;
						}
					}
					
					rec->sWindow->Unlock();
				}
			}
		}
	}
}

static void ActivateIfInThisWorkspace(
	BWindow*	inWindow,
	uint32		inWorkspace)
{
	if (inWindow->Lock())
	{
		if (! inWindow->IsHidden() && (inWindow->Workspaces() & inWorkspace) != 0)
			inWindow->Activate();
		inWindow->Unlock();
	}
}


// ---------------------------------------------------------------------------
//		TextWindowsInThisWorkSpace
// ---------------------------------------------------------------------------
//	return the number of text windows in this workspace and a pointer to one of them.

int32
MDynamicMenuHandler::TextWindowsInThisWorkSpace(
	BWindow*&	outWindow)
{
	WindowRec*		rec;
	int32			count = 0;
	int32			windows = 0;
	const uint32	currentWorkspace = 1 << current_workspace();

	while (fWindowList.GetNthItem(rec, count++)) {
		if (rec->sKind == kText) {
			if (rec->sWindow->Lock()) {
				if ((rec->sWindow->Workspaces() & currentWorkspace) != 0) {
					if (windows == 0) {
						outWindow = rec->sWindow;
					}
					windows++;
				}
				rec->sWindow->Unlock();
			}
		}		
	}
	
	return windows;
}


// ---------------------------------------------------------------------------
//		GetItem
// ---------------------------------------------------------------------------
//	Find a window in the list.

WindowRec*
MDynamicMenuHandler::GetItem(BWindow* inWindow)
{
	int32			count = 0;
	WindowRec*		rec = nil;
	WindowRec*		result = nil;

	while (fWindowList.GetNthItem(rec, count++)) {
		if (rec->sWindow == inWindow) {
			result = rec;
			break;
		}
	}
	
	ASSERT(result);	// should always work

	return result;
}

// ---------------------------------------------------------------------------
//		FindWindowByName
// ---------------------------------------------------------------------------
//	There might be more than one window with the same name...

MTextWindow*
MDynamicMenuHandler::FindWindowByName(const char* inName)
{
	MLocker<BLocker>	lock(fLocker);
	int32				count = 0;
	WindowRec*			rec = nil;
	MTextWindow*		result = nil;

	while (fWindowList.GetNthItem(rec, count++)) {
		if (rec->sKind == kText && strcmp(rec->sWindow->Title(), inName) == 0) {
			result = (MTextWindow*) rec->sWindow;
			break;
		}
	}
	
	return result;
}


// ---------------------------------------------------------------------------
//		FindWindow
// ---------------------------------------------------------------------------
//	Return the window with the specified record ref.  Doesn't check that
//	the ref passed in is valid.

MTextWindow*
MDynamicMenuHandler::FindWindow(entry_ref inRef)
{
	MLocker<BLocker>	lock(fLocker);
	int32				count = 0;
	WindowRec*			rec = nil;
	MTextWindow*		result = nil;

	while (fWindowList.GetNthItem(rec, count++)) {
		if (rec->sKind == kText) {
			entry_ref ref;
			MTextWindow* candidate = (MTextWindow*) rec->sWindow;
			if (candidate->GetRef(ref) == B_OK && ref == inRef) {
				result = (MTextWindow*) rec->sWindow;
				break;
			}
		}
	}

	return result;
}

// ---------------------------------------------------------------------------
//		IndexOf
// ---------------------------------------------------------------------------
//	Return the index of the specified text window.

int32
MDynamicMenuHandler::IndexOf(const BWindow* inWindow)
{
	MLocker<BLocker>	lock(fLocker);
	WindowRec*			rec;
	int32				result = -1;
	int32				count = 0;

	while (fWindowList.GetNthItem(rec, count))
	{
		if (inWindow == rec->sWindow)
		{
			result = count;
			break;
		}
		count++;
	}

	return result;
}

// ---------------------------------------------------------------------------
//		IndexOf
// ---------------------------------------------------------------------------
//	Return the index of the specified text window.

int32
MDynamicMenuHandler::IndexOf(int32 inShortcut)
{
	MLocker<BLocker> lock(fLocker);
	WindowRec* rec;
	int32 result = -1;
	int32 count = 0;

	while (fWindowList.GetNthItem(rec, count))
	{
		if (inShortcut == rec->sShortcut)
		{
			result = count;
			break;
		}
		count++;
	}

	return result;
}

// ---------------------------------------------------------------------------
//		MWindowMenuFilter
// ---------------------------------------------------------------------------

MWindowMenuFilter::MWindowMenuFilter()
: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, B_KEY_DOWN)
{
}

// ---------------------------------------------------------------------------
//		Filter
// ---------------------------------------------------------------------------
//	When getting a keydown, change a cmd-option-number key or cmd-control-number-key
//	key press into a msgWindowMenuClicked message with an additional bool to 
//	indicate that the option key was down.  This saves adding all the shortcuts 
//	for all of these combinations to all windows.

filter_result
MWindowMenuFilter::Filter(
	BMessage *inMessage, 
	BHandler **/*inTarget*/)
{
	switch (inMessage->what)
	{
		case B_KEY_DOWN:
			uint32				modifiers;
			int32				raw_char;
			const uint32		cmdOpt = B_COMMAND_KEY | B_OPTION_KEY;
			const uint32		cmdCntrl = B_COMMAND_KEY | B_CONTROL_KEY;

			ASSERT(B_OK == inMessage->FindInt32("raw_char", &raw_char));	// notify if they ever remove this
	
			if (B_OK == inMessage->FindInt32("modifiers", (int32*) &modifiers) && 
				((modifiers & cmdOpt) == cmdOpt || (modifiers & cmdCntrl) == cmdCntrl) &&
				B_OK == inMessage->FindInt32("raw_char", &raw_char))
			{
				// Project window
				if (raw_char == '0')
				{
					inMessage->MakeEmpty();
					inMessage->what = cmd_ShowProjectWindow;
					inMessage->AddBool(kUseOptionKey, true);				
				}
				else
				// A text window
				if (raw_char >= '0' && raw_char <= '9')
				{
					inMessage->MakeEmpty();
					inMessage->what = msgWindowMenuClicked;
					inMessage->AddBool(kUseOptionKey, true);
					int32		index = MDynamicMenuHandler::IndexOf(raw_char - '0');
					if (index >= 0)
						inMessage->AddInt32("index", index + kWinMenuMinItemCount);
				}
				else
				// Message window
				if (raw_char == 'I')
				{
					inMessage->MakeEmpty();
					inMessage->what = cmd_ErrorMessageWindow;
					inMessage->AddBool(kUseOptionKey, true);				
				}
			}
		break;
	}

	return B_DISPATCH_MESSAGE;
}

// ---------------------------------------------------------------------------
//	Persistent recent document list handling
// ---------------------------------------------------------------------------

void
MDynamicMenuHandler::GetRecentDocumentList(BMessage& outMessage)
{
	// Create a BMessage persistent version of the recent
	// document list using full path names.
	// iterate through both the project list and the window list
	// for each entry_ref, save its path

	MLocker<BLocker> lock(fLocker);
	
	BPath thePath;
	BEntry theEntry;
	entry_ref* nthRef;
	
	outMessage.AddInt32(kVersionName, kCurrentVersion);
	
	for (int32 i = 0; fRecentProjectList.GetNthItem(nthRef, i); i++) {
		if (theEntry.SetTo(nthRef) == B_OK && thePath.SetTo(&theEntry) == B_OK) {
			outMessage.AddString(kRecentProjectName, thePath.Path());
		}
	}
	
	for (int32 i = 0; fRecentWindowList.GetNthItem(nthRef, i); i++) {
		if (theEntry.SetTo(nthRef) == B_OK && thePath.SetTo(&theEntry) == B_OK) {
			outMessage.AddString(kRecentWindowName, thePath.Path());
		}
	}

}

// ---------------------------------------------------------------------------

void
MDynamicMenuHandler::PutRecentDocumentList(const BMessage& inMessage)
{
	// From a BMessage, 
	// ...fill in the fRecentProjectList and fRecentWindowList	
	// (we can ignore the version number until we make a change)
	// If the file no longer exists - don't put it in the menu

	MLocker<BLocker> lock(fLocker);
	
	int32 index = 0;
	const char* path = NULL;
	while (path = inMessage.FindString(kRecentProjectName, index++)) {
		entry_ref ref;
		if (get_ref_for_path(path, &ref) == B_OK) {
			BEntry theEntry(&ref);
			if (theEntry.Exists()) {
				MDynamicMenuHandler::AddRecent(fRecentProjectList, new entry_ref(ref), &theEntry);
			}
		}
	}
	
	// reset index for the window list and loop through all entries
	index = 0;
	while (path = inMessage.FindString(kRecentWindowName, index++)) {
		entry_ref ref;
		if (get_ref_for_path(path, &ref) == B_OK) {
			BEntry theEntry(&ref);
			if (theEntry.Exists()) {
				MDynamicMenuHandler::AddRecent(fRecentWindowList, new entry_ref(ref), &theEntry);
			}
		}
	}	
}

// ---------------------------------------------------------------------------

void        
MDynamicMenuHandler::AddRecent(MList<entry_ref*>& recentList, entry_ref* ref, BEntry* entry)
{
	// Add a new item to recentList
	// node_watch that entry

	recentList.AddItem(ref);

	node_ref nref;
	if (entry) {
		entry->GetNodeRef(&nref); 
	}
	else {
		BEntry theEntry(ref);
		theEntry.GetNodeRef(&nref);
	}

	watch_node(&nref, B_WATCH_NAME, be_app_messenger); 
}

// ---------------------------------------------------------------------------

void
MDynamicMenuHandler::HandleNodeMonitorChange(const BMessage& msg)
{
	// delete the entry from our recent list in either 
	// moved or deleted case
	// (while we could use entry_refs in the case of B_ENTRY_MOVED
	// I just share the code with B_ENTRY_REMOVED where we have to
	// use the node_ref.)
	
	MLocker<BLocker> lock(fLocker);
	int32 opcode;
	msg.FindInt32("opcode", &opcode);
	switch (opcode) {
		case B_ENTRY_MOVED:
		case B_ENTRY_REMOVED:
		{
			// delete the entry from our recent lists
			// unfortunately we have to search by node_ref
			
			// create a node_ref from the message
			node_ref nref;
			msg.FindInt32("device", &nref.device);
			msg.FindInt64("node", &nref.node);

			// look up the bad entry_ref in our recent lists
			MList<entry_ref*>* recentList = &fRecentWindowList;
			int32 index = MDynamicMenuHandler::FindDeletedNode(*recentList, nref);
			if (index == -1) {
				recentList = &fRecentProjectList;
				index = MDynamicMenuHandler::FindDeletedNode(*recentList, nref);
			}

			// delete the entry in our list and stop watching that node
			if (index >= 0) {
				entry_ref* bogus = recentList->RemoveItemAt(index);
				delete bogus;
			}
			watch_node(&nref, B_STOP_WATCHING, be_app_messenger);
			break;
		}
		
		default:
			break;
	}
}

