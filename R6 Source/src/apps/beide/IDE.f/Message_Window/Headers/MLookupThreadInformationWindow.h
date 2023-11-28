// ---------------------------------------------------------------------------
/*
	MLookupThreadInformationWindow.h
	
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			17 June 1999


	This is a message window that is attached to a thread that is doing some
	sort of lookup.  The reason we need this type of extension is because the 
	window can be closed during the lookup.  If the window is closed during
	lookup, the thread should be cancelled.  However, even if the thread is
	cancelled, additional messages can be posted until the thread responds
	to the cancel.
	
	The creator of this window needs to pass in a BMessenger to be used
	to send a cmd_Cancel to (to cancel the thread) or nil if a independent
	thread isn't be used for the lookup.  The creator must also tell the
	window when it is done.  The BMessenger is adopted.
	The creator must also tell the window when it is done.
	
	Given these two constraints, this window can handle the following:
	1. After the window is closed, additional posts can be handled with no action
	2. The act of closing the window will attempt to cancel the lookup thread
	3. The act of closing the window when the thread is long gone will do nothing
	(See MessageReceived and QuitRequested)
	
*/
// ---------------------------------------------------------------------------

#ifndef _MLOOKUPTHREADINFORMATIONWINDOW_H
#define _MLOOKUPTHREADINFORMATIONWINDOW_H

#include "MInformationMessageWindow.h"

class BMessenger;

// ---------------------------------------------------------------------------
// class MLookupThreadInformationWindow
// ---------------------------------------------------------------------------

class MLookupThreadInformationWindow : public MInformationMessageWindow
{
public:
								MLookupThreadInformationWindow(const char* title, 
															   const char* infoTitle,
															   BMessenger* adoptedMessenger);
	virtual						~MLookupThreadInformationWindow();

	virtual	void				MessageReceived(BMessage * message);
	virtual bool				QuitRequested();

private:
	BMessenger*					fThreadHandler;
	
	enum EThreadState			{ kThreadRunning, kThreadDone, kThreadCancelled };
	EThreadState				fState;
};

#endif
