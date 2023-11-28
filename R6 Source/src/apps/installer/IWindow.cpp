//******************************************************************************
//
//	File:			IWindow.cpp
//
//	Description:	Be Installer window.
//
//	Written by:		Steve Horowitz
//
//	Copyright 1994, Be Incorporated. All Rights Reserved.
//
//******************************************************************************

#include <Debug.h>
#include <Alert.h>
#include <Button.h>
#include <Roster.h>
#include <Screen.h>

#include "IApp.h"
#include "IWindow.h"
#include "IView.h"


#include <FindDirectory.h>
#include <Path.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


const BRect windRect(0.0, 0.0, 332.0, 160.0);

const int32 M_LAUNCH_TERMINAL = 'trmn';

TIWindow::TIWindow(TEngine* engine)

	 :	BWindow(windRect, "Installer", B_TITLED_WINDOW,
				  B_NOT_RESIZABLE | B_NOT_ZOOMABLE),
		fEngine(engine),
		m_install_file_size(0),
		m_install_attr_size(0),
		m_install_num(0)
{
	initParams.completionSemaphore = create_sem(0, "waitForInitialize");
	
	AddShortcut('T', B_SHIFT_KEY | B_CONTROL_KEY, 
		new BMessage(M_LAUNCH_TERMINAL));
	
	AddShortcut('D', B_SHIFT_KEY | B_CONTROL_KEY, 
		new BMessage(LAUNCH_DRIVE_SETUP));
	
	BRect bounds(Bounds());
	bounds.right += 1.0;
	bounds.bottom += 1.0;
	TIView *view = new TIView(bounds);
	AddChild(view);
	
	BScreen screen(this);
	MoveTo((screen.Frame().Width() - Frame().Width()) / 2.0, (screen.Frame().Height() - (Frame().Height() + EXPANDED_DELTA)) / 2.0);
	
	Show();
}

TIWindow::~TIWindow()
{
	delete_sem(initParams.completionSemaphore);
}

bool
TIWindow::QuitRequested()
{
	BAlert*	alert;
	
	switch (fEngine->State()) {

		case LOADING:
			break;

		case IDLE:
		case DONE:
			be_app->PostMessage(B_QUIT_REQUESTED); 
			return true;

		case QUITTING:
			return true;

		case LAUNCH_DRIVE_SETUP:
			alert = new BAlert("", "Please close the DriveSetup window before"
				" closing the Installer window.", "OK");
			alert->Go();
			break;

		case FORMAT_VOLUME:
			alert = new BAlert("", "Please wait until the disk is formatted before"
				" closing the Installer window.", "OK");
			alert->Go();
			break;

		case INSTALL_FROM:
			alert = new BAlert("", "Please click the \"Stop\" button to"
				" stop the installation"
				" before closing the Installer window.", "OK");
			alert->Go();
			break;
	}

	return false;
}


void
TIWindow::MessageReceived(BMessage* msg)
{
	BWindow *wind;
	
	switch (msg->what) {
		case M_LAUNCH_TERMINAL:
			if (fEngine->State() == IDLE) {
				BPath path;
				if (find_directory(B_BEOS_APPS_DIRECTORY, &path) == B_OK) {
					path.Append("Terminal &");
					PRINT(("launch terminal %s\n", path.Path()));
					system(path.Path());
				}
			}
			return;

		case DONE_INITIALIZING:
			initParams.cancelOrFail = true;
			if (msg->HasBool("result")) {
				initParams.cancelOrFail = !msg->FindBool("result");
			} else {
				PRINT(("didn't get any result from addon! \n"));
			}
			thread_id looperThreadID;
			status_t junk;
			if (msg->FindInt32("part_looper_thread", &looperThreadID) == B_OK) {
				// this is an updated add-on that puts the thread ID in the msg				
				wait_for_thread(looperThreadID, &junk);
			} else if (msg->FindPointer("part_window", (void **)&wind) == B_OK) {
				// this is an older add-on that just gives us the window pointer
				if (wind->Lock()) {
					looperThreadID = wind->Thread();
					wind->Unlock();
					wait_for_thread(looperThreadID, &junk);
				}
			}
			release_sem(initParams.completionSemaphore);
			return;

		case DISPLAY_MESSAGE:
			{
				const char *statusMessage;
				if (msg->FindString("statusMessage", &statusMessage) == B_OK)
					SetMessage(statusMessage);
			}
			return;
			
		case SHOW_BARBER_POLE:
			SetBarberPoleVisible(true);
			return;
	
		case HIDE_BARBER_POLE:
			SetBarberPoleVisible(false);
			return;

	}
	/*if (fEngine->State() == INSTALL_FROM
		&& msg->what != '_MEX') {	// ignore mouse up

		SetMessage("Stopping installation...");
		fEngine->StopInstall();
	} else {*/
		TIView *iview;
		switch (msg->what) {
			case OPTIONS_TOGGLE_SWITCH:
				iview = (TIView *)ChildAt(0);
				ASSERT(iview != NULL);
				// resize the window's height by EXPANDED_DELTA
				if (iview->IsExpanded()) {
					// currently expanded, so shrink window
					iview->ToggleOptionsExpanded();
					ResizeBy(0, -EXPANDED_DELTA);
				} else {
					// currently shrunk, so expand window
					ResizeBy(0, EXPANDED_DELTA);
					iview->ToggleOptionsExpanded();
				}
				break;
			case 'BUTN':
				if (fEngine->State() == INSTALL_FROM)
				{
					SetMessage("Stopping installation...");
					fEngine->StopInstall();
					break;
				}
				// fall through

			case INIT_ENGINE: 					// fall through
			case COMMAND_SELECTED:				// fall through
			case SOURCE_VOLUME_SELECTED:		// fall through
			case DESTINATION_VOLUME_SELECTED:	// fall through
			case LAUNCH_DRIVE_SETUP:			// fall through
			case OPTION_CHECKED:				// fall through
			case OPTION_MOUSED_OVER:			// fall through
			case 'OPTI':
				fEngine->PostMessage(msg);
				break;
		}
	//}
	inherited::MessageReceived(msg);
}


void
TIWindow::Minimize(const bool a_minimize)
{
	// We need to override this function to disallow the
	// minimizing of the Installer window when the Deskbar
	// is not running (although sometimes I wonder if this
	// shouldn't be the default behavior for a BWindow)
	
	if (be_roster->IsRunning("application/x-vnd.Be-TSKB"))
	{
		inherited::Minimize(a_minimize);
	}
}

void
TIWindow::SetMessage(const char* text)
{
	TIView *view;
	BTextView *textview;

	if (Lock()) {
		if ((view = dynamic_cast<TIView *>(FindView("IView"))) != 0) {
			textview = view->StatusText();
			if (text != NULL)
				textview->SetText(text);
			else {
				char *default_txt = view->BuildDefaultText();
				textview->SetText(default_txt);
				free(default_txt);
			}
		}
		Unlock();
	}
}


void
TIWindow::SetButton(const char* text)
{
	BButton* button;

	Lock();
	if ((button = dynamic_cast<BButton *>(FindView("Button"))) != 0) {
		button->SetLabel(text);
		button->SetEnabled(TRUE);
	}
	Unlock();
}

void
TIWindow::SetSizeBarVisible(bool on)
{
	TIView *view;
	
	if ((view = dynamic_cast<TIView *>(FindView("IView"))) == NULL)
	{
		return;
	}
	
	if (InstallFileSize() <= 0
		|| InstallNum() <= 0)
	{
		view->SetSizeBarVisible(on, true);
		return;
	}
	
	view->SetSizeBarVisible(on, false);
}

void
TIWindow::SetSizeBarMaxValue()
{
	TIView *view;
	
	if (InstallFileSize() <= 0
		|| InstallNum() <= 0
		|| (view = dynamic_cast<TIView *>(FindView("IView"))) == NULL)
	{
		return;
	}
	
	view->SetSizeBarMaxValue();
}

void 
TIWindow::SetBarberPoleVisible(bool on)
{
	TIView *view = dynamic_cast<TIView *>(FindView("IView"));
	if (view)
		view->SetSizeBarVisible(on, true);
}


void
TIWindow::Update(float a_delta)
{
	TIView *view;
	
	if (InstallFileSize() <= 0
		|| InstallNum() <= 0
		|| (view = dynamic_cast<TIView *>(FindView("IView"))) == NULL)
	{
		return;
	}
	
	view->SizeBar()->Update(a_delta);
}
