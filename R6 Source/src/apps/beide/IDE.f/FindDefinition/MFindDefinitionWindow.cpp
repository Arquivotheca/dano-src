//========================================================================
//	MFindDefinitionWindow.cpp
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
// BDS

#include <string.h>

#include "MFindDefinitionWindow.h"
#include "MTextView.h"
#include "MProjectWindow.h"
#include "MKeyFilter.h"
#include "IDEConstants.h"
#include "IDEMessages.h"
#include "MainMenus.h"
#include "Utils.h"

// ---------------------------------------------------------------------------
//		MFindDefinitionWindow
// ---------------------------------------------------------------------------
//	Constructor

MFindDefinitionWindow::MFindDefinitionWindow(
	MProjectWindow&	inProjectView)
	: BWindow(
		BRect(0, 0, 280, 90),
		"Find Definition",
		B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE),
	fProject(inProjectView)
{
	BuildWindow();
	CenterWindow(this);
}

// ---------------------------------------------------------------------------
//		~MFindDefinitionWindow
// ---------------------------------------------------------------------------

MFindDefinitionWindow::~MFindDefinitionWindow()
{
}

// ---------------------------------------------------------------------------
//		QuitRequested
// ---------------------------------------------------------------------------
//	Determine if it's ok to close.

bool
MFindDefinitionWindow::QuitRequested()
{
	fProject.PostMessage(msgFindDefinitionClosed);
	Hide();
	
	return false;
}

// ---------------------------------------------------------------------------
//		BuildWindow
// ---------------------------------------------------------------------------

void
MFindDefinitionWindow::BuildWindow()
{
	BRect			r;
	BMessage*		msg;
	BButton*		button;
	BStringView*	caption;
	BScrollView*	box;
	BView*			topView;
	float			top = 10.0;

	// Build a special topview so we can have a grey background for
	// the window
	r = Bounds();
	topView = new BView(r, "ourtopview", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	AddChild(topView);
	SetGrey(topView, kLtGray);

	// Static text
	r.Set(20, top, 150, top + 16.0);
	caption = new BStringView(r, "Open", "Find Definition for:");
	topView->AddChild(caption);
	caption->SetFont(be_bold_font);
	SetGrey(caption, kLtGray);
	top += 20.0;

	// TextBox
	r.Set(20, top, 260, top + 15.0);
	fTextBox = new MTextView(r, "Name");

	box = new BScrollView("box", fTextBox);		// For the border
	topView->AddChild(box);

	fTextBox->SetFont(be_plain_font);
	fTextBox->SetMaxBytes(63);
	fTextBox->Select(0, 0);
	fTextBox->MakeFocus();
	fTextBox->SetTarget(this);
	top += 20.0;

	// Find button
	r.Set(180, 60, 260, 80);
	msg = new BMessage(msgOK);
	button = new BButton(r, "OK", "Find", msg);
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
//		MessageReceived
// ---------------------------------------------------------------------------

void
MFindDefinitionWindow::MessageReceived(
	BMessage * inMessage)
{
	switch (inMessage->what)
	{
		case msgTextChanged:
			fOKButton->SetEnabled(fTextBox->TextLength() > 0);
			break;

		case msgOK:
			ExtractInfo();
			// Fall through on purpose

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
//	Post a FindDefinition message to the project window.

void
MFindDefinitionWindow::ExtractInfo()
{
	const char*		text = fTextBox->Text();
	BMessage		msg (msgFindDefinition);
	msg.AddString(kText, text);
	
	fProject.PostMessage(&msg);
}
