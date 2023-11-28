//========================================================================
//	MSaveFileSetWindow.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
// BDS

#include <string.h>

#include "MSaveFileSetWindow.h"
#include "MTextView.h"
#include "MFindWindow.h"
#include "MKeyFilter.h"
#include "MAlert.h"
#include "IDEConstants.h"
#include "IDEMessages.h"
#include "MainMenus.h"
#include "Utils.h"


// ---------------------------------------------------------------------------
//		MSaveFileSetWindow
// ---------------------------------------------------------------------------
//	Constructor

MSaveFileSetWindow::MSaveFileSetWindow(
	BPopUpMenu&	inFileSetMenuBar,
	bool		inHasProject)
	: BWindow(
		BRect(0, 0, 280, 130),
		"Save A File Set",
		B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE),
		fPopup(inFileSetMenuBar)
{
	BuildWindow(inHasProject);
	CenterWindow(this);
	
	Show();
}

// ---------------------------------------------------------------------------
//		~MSaveFileSetWindow
// ---------------------------------------------------------------------------

MSaveFileSetWindow::~MSaveFileSetWindow()
{
	MFindWindow::GetFindWindow().PostMessage(msgSaveFileSetWindowClosed);
}

// ---------------------------------------------------------------------------
//		BuildWindow
// ---------------------------------------------------------------------------

void
MSaveFileSetWindow::BuildWindow(
	bool	inHasProject)
{
	BRect			r;
	BMessage*		msg;
	BButton*		button;
	BStringView*	caption;
	BScrollView*	box;
	BRadioButton*	radioButton;
	BView*			topView;
	float			top = 10.0;
	float			left = 20.0;

	// Build a special topview so we can have a grey background for
	// the window
	r = Bounds();
	topView = new BView(r, "ourtopview", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	AddChild(topView);
	SetGrey(topView, kLtGray);

	// Static text
	r.Set(left, top, left + 200.0, top + 16.0);
	caption = new BStringView(r, "save", "Save this file set as:"); 
	topView->AddChild(caption);
	caption->SetFont(be_bold_font);
	top += 20.0;
	SetGrey(caption, kLtGray);

	// TextBox
	r.Set(left, top, 260, top + 16.0);
	fTextBox = new MTextView(r, "Name"); 

	box = new BScrollView("box", fTextBox);		// For the border
	topView->AddChild(box);

	fTextBox->SetFont(be_bold_font);
	fTextBox->SetMaxBytes(63);
	fTextBox->Select(0, 0);
	fTextBox->MakeFocus();
	fTextBox->SetTarget(this);
	top += 20.0;

	// Radio buttons
	// Specific to this project
	r.Set(left, top, 250, top + 16);
	msg = new BMessage(msgRadioButtonClicked);
	radioButton = new BRadioButton(r, "rb1", "Specific to this project", msg);
	topView->AddChild(radioButton);
	radioButton->SetFont(be_bold_font);
	radioButton->SetTarget(this);
	fSpecific = radioButton;
	SetGrey(radioButton, kLtGray);
	top += 20;

	// Global, for all projects
	r.Set(left, top, 250, top + 16);
	msg = new BMessage(msgRadioButtonClicked);
	radioButton = new BRadioButton(r, "rb1", "Global, for all projects", msg);
	topView->AddChild(radioButton);
	radioButton->SetFont(be_bold_font);
	radioButton->SetTarget(this);
	SetGrey(radioButton, kLtGray);
	fGlobal = radioButton;
	top += 30;

	// Open button
	r.Set(180, top, 260, top + 20);
	msg = new BMessage(msgOK);
	button = new BButton(r, "OK", "Save", msg); 
	topView->AddChild(button);
	button->SetTarget(this);
	SetGrey(button, kLtGray);
	SetDefaultButton(button);
	fOKButton = button;

	// Cancel button
	r.Set(80, top, 160, top + 20);
	msg = new BMessage(msgCancel);
	button = new BButton(r, "Cancel", "Cancel", msg); 
	topView->AddChild(button);
	SetGrey(button, kLtGray);
	button->SetTarget(this);
	
	// Initialize controls
	if (inHasProject)
	{
		fSpecific->SetValue(B_CONTROL_ON);
	}
	else
	{
		fGlobal->SetValue(B_CONTROL_ON);
		fSpecific->SetEnabled(false);
	}
	
	fOKButton->SetEnabled(false);

	AddCommonFilter(new MTextKeyFilter(this, kBindingGlobal));
}

// ---------------------------------------------------------------------------
//		MessageReceived
// ---------------------------------------------------------------------------

void
MSaveFileSetWindow::MessageReceived(
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
//		ExtractInfo
// ---------------------------------------------------------------------------

void
MSaveFileSetWindow::ExtractInfo()
{
	BWindow&		findWind = MFindWindow::GetFindWindow();
	
	// Check if the name is already taken
	const char*		text = fTextBox->Text();

	findWind.Lock();
	BMenuItem*		item = fPopup.FindItem(text);
	findWind.Unlock();
	
	if (item)
	{
		MAlert		alert("Sorry, that name is already taken");
		alert.Go();
	}
	else
	{
		BMessage		msg(msgSaveFileSet);
		bool			isInProject = fSpecific->Value() == B_CONTROL_ON;

		msg.AddString(kFileName, text);			// name of file set
		msg.AddBool(kIsInProject, isInProject);	// is the file set project specific

		findWind.PostMessage(&msg);
		PostMessage(B_QUIT_REQUESTED);
	}
}
