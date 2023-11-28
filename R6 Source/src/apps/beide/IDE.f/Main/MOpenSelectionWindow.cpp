//========================================================================
//	MOpenSelectionWindow.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	The window used to enter the file to search for in response to the
//	open selection menu item.  cmd-d
//	BDS

#include <string.h>

#include "MOpenSelectionWindow.h"
#include "MTextView.h"
#include "MDefaultPrefs.h"
#include "MPreferences.h"
#include "MKeyFilter.h"
#include "IDEConstants.h"
#include "IDEMessages.h"
#include "MainMenus.h"
#include "Utils.h"
#include <InterfaceKit.h>
#include <AppKit.h>

class MProjectWindow;

// ---------------------------------------------------------------------------
//		 MOpenSelectionWindow
// ---------------------------------------------------------------------------
//	Constructor

MOpenSelectionWindow::MOpenSelectionWindow(MProjectWindow* inProject)
	: BWindow(
		BRect(0, 0, 280, 90),
		"Open Selection",
		B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	fProject = inProject;
	CenterWindow(this);
	BuildWindow();
	GetPrefs();
}

// ---------------------------------------------------------------------------
//		 ~MOpenSelectionWindow
// ---------------------------------------------------------------------------

MOpenSelectionWindow::~MOpenSelectionWindow()
{
	SetPrefs();
}

// ---------------------------------------------------------------------------
//		 QuitRequested
// ---------------------------------------------------------------------------
//	Determine if it's ok to close.

bool
MOpenSelectionWindow::QuitRequested()
{
	be_app_messenger.SendMessage(msgOpenSelectionClosed);
	Hide();
	
	return false;
}

// ---------------------------------------------------------------------------
//		 BuildWindow
// ---------------------------------------------------------------------------

void
MOpenSelectionWindow::BuildWindow()
{
	BRect			r;
	BMessage*		msg;
	BButton*		button;
	BStringView*	caption;
	BScrollView*	box;
	BCheckBox*		checkBox;
	BView*			topView;
	float			top = 10.0;

	// Build a special topview so we can have a grey background for
	// the window
	r = Bounds();
	topView = new BView(r, "ourtopview", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	AddChild(topView);
	SetGrey(topView, kLtGray);

	// Static text
	r.Set(20, top, 55, top + 16.0);
	caption = new BStringView(r, "Open", "Open:"); 
	topView->AddChild(caption);
	caption->SetFont(be_bold_font);
	SetGrey(caption, kLtGray);

	// TextBox
	r.Set(60, top, 260, top + 15.0);
	fTextBox = new MTextView(r, "Name"); 

	box = new BScrollView("box", fTextBox);		// For the border
	topView->AddChild(box);

	fTextBox->SetFont(be_bold_font);
	fTextBox->SetMaxBytes(63);
	fTextBox->MakeFocus();
	fTextBox->SetTarget(this);
	top += 20.0;

	// System search checkbox
	r.Set(60, top, 260, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "SysSearch", "Search only in the System Tree.", msg); 
	topView->AddChild(checkBox);
	checkBox->SetTarget(this, nil);
	fSystemSearchCB = checkBox;
	SetGrey(fSystemSearchCB, kLtGray);
	top += 20.0;

	// Open button
	r.Set(180, 60, 260, 80);
	msg = new BMessage(msgOK);
	button = new BButton(r, "OK", "Open", msg); 
	topView->AddChild(button);
	button->SetTarget(this);
	SetDefaultButton(button);
	SetGrey(button, kLtGray);
	fOKButton = button;

	// Cancel button
	r.Set(80, 60, 160, 80);
	msg = new BMessage(msgCancel);
	button = new BButton(r, "Cancel", "Cancel", msg); 
	topView->AddChild(button);
	button->SetTarget(this);
	SetGrey(button, kLtGray);

	AddCommonFilter(new MTextKeyFilter(this, kBindingGlobal));
}

// ---------------------------------------------------------------------------
//		 MessageReceived
// ---------------------------------------------------------------------------

void
MOpenSelectionWindow::MessageReceived(
	BMessage * inMessage)
{
	switch (inMessage->what)
	{
		case msgTextChanged:
			fOKButton->SetEnabled(fTextBox->TextLength() > 0);
			break;

		case msgOK:
			ExtractInfo();
			break;

		case msgCancel:
			PostMessage(B_QUIT_REQUESTED);
			break;

		default:
			if (! SpecialMessageReceived(*inMessage, this))
				BWindow::MessageReceived(inMessage);
			break;
	}
}

// ---------------------------------------------------------------------------
//		 ExtractInfo
// ---------------------------------------------------------------------------

void
MOpenSelectionWindow::ExtractInfo()
{
	BMessage		msg(msgDoOpenSelection);
	const char*		text = fTextBox->Text();
	bool			systemInclude = fSystemSearchCB->Value() == 1;
	
	msg.AddString(kFileName, text);				// filename to be opened
	msg.AddBool(kSystemInclude, systemInclude);	// is the file enclosed in brackets
	msg.AddPointer(kProjectMID, fProject);		// in this project

	be_app_messenger.SendMessage(&msg);
}

// ---------------------------------------------------------------------------
//		 WindowActivated
// ---------------------------------------------------------------------------
//	Select the name of the file without the extension.

void
MOpenSelectionWindow::WindowActivated(
	bool inActivated)
{
	BWindow::WindowActivated(inActivated);

	if (inActivated)
	{
		const char*		text = fTextBox->Text();
		char *			dot = strrchr(text, '.');
		
		if (dot)
			fTextBox->Select(0, dot - text);
		// Set the cursor to the hand if it isn't in the textview
		// The textview sets it to the IBeam if the mouse is inside the view
		BPoint 	where;
		uint32	buttons;
		fTextBox->GetMouse(&where, &buttons);
		if (! fTextBox->Bounds().Contains(where))
			be_app->SetCursor(B_HAND_CURSOR);
	}
}

// ---------------------------------------------------------------------------
//		 GetPrefs
// ---------------------------------------------------------------------------
//	Get the current extension from the database and place it in
//	the textbox.
//	Should verify length and version number here.

void
MOpenSelectionWindow::GetPrefs()
{
	size_t					length;
	OpenSelectionPrefs		prefs;

	length = sizeof(OpenSelectionPrefs);
	if (B_NO_ERROR == MPreferences::GetPreference(kOpenSelectionPrefs, kPrefsType, &prefs, length))
		prefs.SwapBigToHost();
	else
		MDefaultPrefs::SetOpenSelectionDefaults(prefs);

	fTextBox->SetText(prefs.extension);
	fTextBox->Select(0, 0);
}

// ---------------------------------------------------------------------------
//		 SetPrefs
// ---------------------------------------------------------------------------
//	Save the current extension in the database.

void
MOpenSelectionWindow::SetPrefs()
{
	int32					length;
	OpenSelectionPrefs		prefs;
	const char *			text = fTextBox->Text();
	const char *			extension = strrchr(text, '.');

	prefs.version = kCurrentVersion;
	if (extension == nil)
		prefs.extension[0] = '\0';
	else
		strcpy(prefs.extension, extension);
	
	length = sizeof(OpenSelectionPrefs);
	prefs.SwapHostToBig();
	(void) MPreferences::SetPreference(kOpenSelectionPrefs, kPrefsType,  &prefs, length);
}

// ---------------------------------------------------------------------------
//		 RemovePreferences
// ---------------------------------------------------------------------------

void
MOpenSelectionWindow::RemovePreferences()
{
	(void) MPreferences::RemovePreference(kOpenSelectionPrefs, kPrefsType);
}
