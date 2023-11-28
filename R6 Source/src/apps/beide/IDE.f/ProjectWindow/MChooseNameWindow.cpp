//========================================================================
//	MChooseNameWindow.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
// BDS

#include "MChooseNameWindow.h"
#include "MSectionLine.h"
#include "MTextView.h"
#include "IDEConstants.h"
#include "IDEMessages.h"
#include "Utils.h"
#include <ScrollView.h>
#include <Button.h>
#include <Debug.h>

// ---------------------------------------------------------------------------
//		MChooseNameWindow
// ---------------------------------------------------------------------------
//	Constructor

MChooseNameWindow::MChooseNameWindow(
	BWindow& 		inWindow, 
	MSectionLine&	inSection,
	const char* 	inText)
	: BWindow(
		BRect(0, 0, 240, 90),
		"ChooseNameWindow",
		B_MODAL_WINDOW,
		B_NOT_RESIZABLE),
	fWindow(inWindow),
	fSection(inSection)
{
	BuildWindow(inText);
	CenterWindow(this);
}

// ---------------------------------------------------------------------------
//		~MChooseNameWindow
// ---------------------------------------------------------------------------

MChooseNameWindow::~MChooseNameWindow()
{
}

// ---------------------------------------------------------------------------
//		BuildWindow
// ---------------------------------------------------------------------------

void
MChooseNameWindow::BuildWindow(const char* inText)
{
	BRect			r;
	BMessage*		msg;
	BButton*		button;
	BStringView*	caption;
	BScrollView*	box;
	BView*			topView;

	// Build a special topview so we can have a grey background for
	// the window
	r = Bounds();
	topView = new BView(r, "ourtopview", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	AddChild(topView);
	SetGrey(topView, kLtGray);

	// Static text
	r.Set(20, 10, 190, 26);
	caption = new BStringView(r, "Name", "Name for this Group:"); 
	topView->AddChild(caption);
	caption->SetFont(be_bold_font);
	SetGrey(caption, kLtGray);

	// TextBox
	r.Set(20, 32, 220, 47);
	fTextBox = new MTextView(r, "Name"); 

	box = new BScrollView("box", fTextBox);		// For the border
	topView->AddChild(box);

	fTextBox->MakeFocus();
	fTextBox->SetFont(be_bold_font);
	fTextBox->SetText(inText);
	fTextBox->SelectAll();
	fTextBox->SetMaxBytes(32);

	// OK button
	r.Set(130, 60, 210, 80);
	msg = new BMessage(msgOK);
	button = new BButton(r, "OK", "OK", msg); 
	topView->AddChild(button);
	button->SetTarget(this);
	SetGrey(button, kLtGray);
	SetDefaultButton(button);

	// Cancel button
	r.Set(20, 60, 100, 80);
	msg = new BMessage(msgCancel);
	button = new BButton(r, "Cancel", "Cancel", msg); 
	topView->AddChild(button);
	SetGrey(button, kLtGray);
	button->SetTarget(this);
}

// ---------------------------------------------------------------------------
//		MessageReceived
// ---------------------------------------------------------------------------

void
MChooseNameWindow::MessageReceived(
	BMessage * inMessage)
{
	switch (inMessage->what)
	{
		case msgOK:
			ExtractInfo();
			// fall through

		case msgCancel:
			PostMessage(B_QUIT_REQUESTED);
			break;

		default:
			BWindow::MessageReceived(inMessage);
			break;
	}
}

// ---------------------------------------------------------------------------
//		ExtractInfo
// ---------------------------------------------------------------------------
//	Post a SetSectionName message to the project window.

void
MChooseNameWindow::ExtractInfo()
{
	BMessage		msg(msgSetName);
	const char*		text = fTextBox->Text();
	
	ASSERT(text);

	if (text)
		msg.AddString(kName, text);
	
	msg.AddPointer(kSection, &fSection);

	fWindow.PostMessage(&msg);
}
