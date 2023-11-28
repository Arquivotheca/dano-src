//-----------------------------------------------------------------------------
//
// RWindow.cpp
//
// Reusable window class.
//
// Adds following features to BWindow:
//
// Changes to hand cursor on activates.
// Maintains a window list in front-to-back order.
//
//-----------------------------------------------------------------------------

#include "RWindow.h"
#include "MyDebug.h"

BList RWindow::windowList;
BLocker RWindow::windowListLock;

///RWindow
//-----------------------------------------------------------------------------

RWindow::RWindow(BRect frame, const char *title, 
	window_type type, ulong flags)
	: BWindow(frame, title, type, flags)
{
	//be_app->SetCursor(B_HAND_CURSOR);

	// Add new window to list.

	windowListLock.Lock();
	windowList.AddItem(this);
	windowListLock.Unlock();
}

///WindowActivated
//-----------------------------------------------------------------------------

void RWindow::WindowActivated(bool active)
{
	//if (active) be_app->SetCursor(B_HAND_CURSOR);
	
	// Move newly activated window to front of list.
	
	windowListLock.Lock();
	windowList.RemoveItem(this);
	windowList.AddItem(this, 0);
	windowListLock.Unlock();
	
	BWindow::WindowActivated(active);
}

///Quit
//-----------------------------------------------------------------------------

void RWindow::Quit()
{
	// Remove dead window from the list.
	PRINT(("\n@@@@@@@@@@@@@@@@@@@@@@\nThis is RWindow quit requested\n"));


	windowListLock.Lock();
	windowList.RemoveItem(this);
	windowListLock.Unlock();
	
	BWindow::Quit();
	PRINT(("returned from BWindow::Quit()\n"));
}

///WindowList
//-----------------------------------------------------------------------------

BList RWindow::WindowList()
{
	// Returns a copy of the current windowlist. This is just an unlocked 
	// snapshot. Real windows can continue to be created and deleted while you 
	// use the list. In particular, be certain to lock any windows in the list
	// before you try to use them. E.g.:
	//
	//	BList windowList = RWindow::WindowList();
	//	for (long i = 0; RWindow *w = (RWindow*)windowList.ItemAt(i); i++) {
	//		if (w->Lock()) {
	//			// ... work with window w
	//			w->Unlock();
	//		}
	//	}

	windowListLock.Lock();
	// does a shallow copy of windowList
	BList result = windowList;
	windowListLock.Unlock();
	return result;
}

///Front
//-----------------------------------------------------------------------------

RWindow* RWindow::Front()
{
	// Returns the front window, or NULL if the window list is empty.
	// Lock the result before using it!
	
	windowListLock.Lock();
	RWindow *result = (RWindow*)windowList.ItemAt(0);
	windowListLock.Unlock();
	return result;
}

///CountWindows
//-----------------------------------------------------------------------------

long RWindow::CountWindows()
{
	// Returns the number of RWindows currently in our list.
	
	windowListLock.Lock();
	long result = windowList.CountItems();
	windowListLock.Unlock();
	return result;
}
