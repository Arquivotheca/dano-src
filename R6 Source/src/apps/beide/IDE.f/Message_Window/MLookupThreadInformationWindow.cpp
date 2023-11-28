// ---------------------------------------------------------------------------
/*
	MLookupThreadInformationWindow.cpp
	
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			17 June 1999

*/
// ---------------------------------------------------------------------------

#include "MLookupThreadInformationWindow.h"
#include "IDEMessages.h"
#include "ProjectCommands.h"

#include <Messenger.h>
#include <Autolock.h>

MLookupThreadInformationWindow::MLookupThreadInformationWindow(const char* title, 
															   const char* infoTitle,
															   BMessenger* adoptedMessenger)
						 	   : MInformationMessageWindow(title, infoTitle)
{
	fThreadHandler = adoptedMessenger;
	fState = kThreadRunning;
}

// ---------------------------------------------------------------------------

MLookupThreadInformationWindow::~MLookupThreadInformationWindow()
{
	delete fThreadHandler;	
}

// ---------------------------------------------------------------------------
	
void
MLookupThreadInformationWindow::MessageReceived(BMessage* inMessage)
{
	// If we get messages after the lookup thread has been cancelled,
	// just ignore them


	switch (inMessage->what) {
		case msgDoneWithMessageWindow:
			if (fState == kThreadRunning) {
				fState = kThreadDone;
			}
			else if (fState == kThreadCancelled) {
				fState = kThreadDone;
				this->Quit();
			}
			break;

		// user pressed cmd-. 
		// Don't shut down the window, but do stop the lookup thread
		case cmd_Cancel:
			if (fState == kThreadRunning && fThreadHandler) {
				fThreadHandler->SendMessage(cmd_Cancel);
			}
			break;
			
		// Find/definition/documentation Lookup (check thread state)
		case msgAddInfoToMessageWindow:
		case msgAddDefinitionToMessageWindow:
		case msgAddDocInfoToMessageWindow:
		case msgShowAndActivate:
			if (fState == kThreadRunning) {
				MInformationMessageWindow::MessageReceived(inMessage);
			}
			break;

		default:
			MInformationMessageWindow::MessageReceived(inMessage);
			break;
	}
}

// ---------------------------------------------------------------------------

bool
MLookupThreadInformationWindow::QuitRequested()
{
	// If the task is not running, then allow normal quit
	// If the task is still running, then simulate the quit
	// by hidding the window and cancelling the thread.
	// (if cancelling is possible).
	// In this state, quit the window when we get the
	// "done" flag.
	
	// make sure we are locked
	BAutolock lock(this);
		
	if (fState == kThreadDone) {
		return true;
	}
	
	fState = kThreadCancelled;
	if (fThreadHandler) {
		fThreadHandler->SendMessage(cmd_Cancel);
	}
	this->Hide();
	return false;
}
