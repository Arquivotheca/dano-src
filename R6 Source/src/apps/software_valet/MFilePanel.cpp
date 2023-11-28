#include "MFilePanel.h"

#define MODULE_DEBUG 0
#include "MyDebug.h"
#include <Message.h>
#include <Handler.h>
#include <MessageFilter.h>
#include <Messenger.h>
#include <Autolock.h>
#include <string.h>
#include <View.h>
#include <Window.h>
#include <Screen.h>

BLocker		MFilePanel::dataLock;
entry_ref	MFilePanel::saveLoc;
entry_ref	MFilePanel::openLoc;
BRect		MFilePanel::openRect(0,0,0,0);
BRect		MFilePanel::saveRect(0,0,0,0);
// RList<BFilePanel *>		MFilePanel::panelList;

const char		*MFilePanel::kOpenLabel = "Open";
filter_result	checkmsg(BMessage *msg, BHandler **, BMessageFilter *);

MFilePanel::MFilePanel(file_panel_mode mode,
				BMessenger *target,
				entry_ref *start_directory,
				bool directory_selection,
				bool allow_multiple_selection,
				uint32 message,
				BRefFilter* filter,
				const char *openLabel,
				bool autoClose)
	:	BFilePanel(mode,
				target,
				start_directory,
				directory_selection ? B_DIRECTORY_NODE : B_FILE_NODE,
				allow_multiple_selection,
				new BMessage(message),
				filter,
				false),
		fAutoClose(autoClose),
		fTarget(target),
		fOpenLabel(NULL)
{
	//snooze(500*1000);	// let the display update
	{
		BAutolock	l(dataLock);
		BAutolock	w(Window());

		PRINT(("data locked %d ,window locked %d\n",(int)l.IsLocked(),(int)w.IsLocked()));

		PRINT(("MFilePanel::MFilePanel START\n"));
		//panelList.AddItem(this);
		PRINT(("MFilePanel::MFilePanel added to lyst\n"));
		
		/// label stuff
		if (openLabel) {
			fOpenLabel = strdup(openLabel);
			SetButtonLabel(B_DEFAULT_BUTTON,fOpenLabel);
		}
			
		fUpdtButton = (mode == B_OPEN_PANEL && !directory_selection && fOpenLabel);
			
		/// panel mode			
		if (mode == B_OPEN_PANEL) {
			SetPanelDirectory(&openLoc);
			SetWindowRect(openRect);
		}
		else {
			SetPanelDirectory(&saveLoc);
			SetWindowRect(saveRect);
			
			BMessage msg(B_MOUSE_DOWN);
			
			// hack to fix some sort of activating problem
			msg.AddPoint("where",BPoint(0,20));
			msg.AddInt32("clicks",0);
			Window()->PostMessage(&msg,Window()->ChildAt(0));
		}
		
		// Window()->AddFilter(new BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, checkmsg));
	}
}

MFilePanel::~MFilePanel()
{
	PRINT(("MFilePanel::~MFilePanel START\n"));
	free(fOpenLabel);
	delete fTarget;

	// remove ourselves from the master list	
	dataLock.Lock();
	BAutolock w(Window());
	
	if (PanelMode() == 	B_OPEN_PANEL) {
		GetPanelDirectory(&openLoc);
		openRect = Window()->Frame();	
	}
	else {
		GetPanelDirectory(&saveLoc);
		saveRect = Window()->Frame();
	}
	
	//panelList.RemoveItem(this);
	dataLock.Unlock();
	PRINT(("MFilePanel::~MFilePanel END\n"));
}

// called directly by Window()::QuitRequested
void	MFilePanel::WasHidden()
{

	// DEBUGGER("WAS HIDDEN");
	BFilePanel::WasHidden();

#if 0
	if (Window()->Lock()) {
		PRINT(("MFilePanel::WasHidden START\n"));
		// if WasHidden is being called from a thread other than the window
		// thread, then
		// QuitRequested is being called from the app
		// in that case we don't delete ourselves because
		// there was a crashing problem
		// this leaks mem. but then the app heap is destroyed
		if (Window()->Thread() != find_thread(NULL)) {
			Window()->PostMessage(B_QUIT_REQUESTED);
			Window()->Unlock();
			return;
		}
		PRINT(("MFilePanel::WasHidden END\n"));
		Window()->Unlock();
	}
#endif
	
	delete this;
}

/** not sure what this is here for?? ***/
/** this is to implement proper auto-closing behavior even
when the message is something other than B_REFS_RECEIVED
I think this was fixed in PR2 **/
void	MFilePanel::SendMessage(const BMessenger *msngr, BMessage *msg)
{
	bool closeMe = false;
	if (msg->what != B_REFS_RECEIVED && fAutoClose)
		closeMe = true;
	
	BFilePanel::SendMessage(msngr,msg);	
	
	if (closeMe)
		Hide();
}

void	MFilePanel::SetWindowRect(BRect r)
{
	PRINT(("MFilePanel::SetWindowRect ENTER\n"));
	if (r.left <= 0 || r.top <= 0)
		return;

	BScreen	screen(Window());
	
	if (screen.Frame().Intersects(r)) {
		PRINT(("Setting file panel window size\n"));

		Window()->MoveTo(r.LeftTop());
		Window()->ResizeTo(r.Width(),r.Height());
	}
	PRINT(("MFilePanel::SetWindowRect EXIT\n"));
}

void	MFilePanel::SelectionChanged()
{
	BFilePanel::SelectionChanged();
	
	if (fUpdtButton) {
		entry_ref	ref;
		BEntry		entry;
		int			count = 0;
		
		Rewind();
		while (GetNextSelectedRef(&ref) == B_NO_ERROR) {
			count++;
			if (count > 1)
				break;
			entry.SetTo(&ref);
		}
		if (count == 1 && entry.IsDirectory()) {
			SetButtonLabel(B_DEFAULT_BUTTON,kOpenLabel);
		}
		else {
			SetButtonLabel(B_DEFAULT_BUTTON,fOpenLabel);
		}
	}
}
