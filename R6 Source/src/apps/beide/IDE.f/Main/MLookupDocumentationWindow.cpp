// ---------------------------------------------------------------------------
/*
	MLookupDocumentationWindow.cpp
	
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			14 January 1999

*/
// ---------------------------------------------------------------------------

#include <string.h>

#include "MLookupDocumentationWindow.h"
#include "MTextView.h"
#include "MKeyFilter.h"
#include "IDEConstants.h"
#include "IDEMessages.h"
#include "MainMenus.h"
#include "Utils.h"

#include <ScrollView.h>
#include <Button.h>
#include <Application.h>

// ---------------------------------------------------------------------------
// Methods for MLookupDocumentationWindow
// ---------------------------------------------------------------------------

MLookupDocumentationWindow::MLookupDocumentationWindow()
	: BWindow(BRect(0, 0, 280, 90),
		"Lookup Documentation",
		B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	BuildWindow();
	CenterWindow(this);
}

// ---------------------------------------------------------------------------

MLookupDocumentationWindow::~MLookupDocumentationWindow()
{
}

// ---------------------------------------------------------------------------

bool
MLookupDocumentationWindow::QuitRequested()
{
	be_app_messenger.SendMessage(msgFindDocumentationClosed);
	Hide();
	
	return false;
}

// ---------------------------------------------------------------------------

void
MLookupDocumentationWindow::BuildWindow()
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
	r.Set(20, top, 165, top + 16.0);
	caption = new BStringView(r, "Open", "Find Documentation for:");
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

void
MLookupDocumentationWindow::MessageReceived(
	BMessage * inMessage)
{
	switch (inMessage->what)
	{
		case msgTextChanged:
			fOKButton->SetEnabled(fTextBox->TextLength() > 0);
			break;

		case msgOK:
			ExtractInfo();
			// Fall through to automatically quit with OK

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

void
MLookupDocumentationWindow::ExtractInfo()
{
	// post a find documentation message back to the IDEApp
	// include the string from the text box
	
	BMessage msg(msgFindDocumentation);
	const char* text = fTextBox->Text();
	msg.AddString(kText, text);
	
	be_app_messenger.SendMessage(&msg);
}
