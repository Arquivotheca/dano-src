//========================================================================
//	MGoToLineWindow.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	A modeless window that allows the user to move the insertion point
//	to a particular line.
// BDS

#include "MGoToLineWindow.h"
#include "MTextView.h"
#include "IDEConstants.h"
#include "IDEMessages.h"
#include "Utils.h"
#include <InterfaceKit.h>
#include <SupportKit.h>
#include <AppKit.h>
#include <Debug.h>

// ---------------------------------------------------------------------------
//		MGoToLineWindow
// ---------------------------------------------------------------------------
//	Constructor

MGoToLineWindow::MGoToLineWindow(
	BWindow&	 	inWindow,
	BPoint			inTopLeft)
	: BWindow(
		BRect(0, 0, 210, 70),
		"GoToLine Window",
		B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE),
		fWindow(inWindow)
{
	BuildWindow();
	MoveBy(inTopLeft.x, inTopLeft.y);
}

// ---------------------------------------------------------------------------
//		~MGoToLineWindow
// ---------------------------------------------------------------------------

MGoToLineWindow::~MGoToLineWindow()
{
}

// ---------------------------------------------------------------------------
//		QuitRequested
// ---------------------------------------------------------------------------
//	Determine if it's ok to close.

bool
MGoToLineWindow::QuitRequested()
{
	fWindow.PostMessage(msgGoToWindowClosed);

	Hide();
	
	return false;
}

// ---------------------------------------------------------------------------
//		BuildWindow
// ---------------------------------------------------------------------------

void
MGoToLineWindow::BuildWindow()
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
	r.Set(10, 10, 140, 26);
	caption = new BStringView(r, "Text", "Go To Line Number:"); 
	topView->AddChild(caption);
	caption->SetFont(be_bold_font);
	SetGrey(caption, kLtGray);

	// TextBox
	r.Set(160, 10, 200, 25);
	fTextBox = new MTextView(r, "Name"); 

	box = new BScrollView("box", fTextBox);		// For the border
	topView->AddChild(box);

	fTextBox->MakeFocus();
	fTextBox->SetFont(be_bold_font);
	fTextBox->SetText("1");
	fTextBox->SelectAll();
	fTextBox->SetMaxBytes(5);
	DisallowInvalidChars(*fTextBox);

	// Cancel button
	r.Set(10, 40, 90, 60);
	msg = new BMessage(msgCancel);
	button = new BButton(r, "Cancel", "Cancel", msg); 
	topView->AddChild(button);
	button->SetTarget(this);
	SetGrey(button, kLtGray);

	// OK button
	r.Set(120, 40, 200, 60);
	msg = new BMessage(msgOK);
	button = new BButton(r, "OK", "OK", msg); 
	topView->AddChild(button);
	SetGrey(button, kLtGray);
	button->SetTarget(this);
	SetDefaultButton(button);
}

// ---------------------------------------------------------------------------
//		MessageReceived
// ---------------------------------------------------------------------------

void
MGoToLineWindow::MessageReceived(
	BMessage * inMessage)
{
	switch (inMessage->what)
	{
		case msgOK:
			ExtractInfo();
			break;

		case msgCancel:
			PostMessage(B_QUIT_REQUESTED);
			break;

		default:
			BWindow::MessageReceived(inMessage);
			break;
	}
}

// ---------------------------------------------------------------------------
//		WindowActivated
// ---------------------------------------------------------------------------

void
MGoToLineWindow::WindowActivated(bool inActive)
{
	BWindow::WindowActivated(inActive);
	if (inActive)
	{
		fTextBox->SelectAll();
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
//		ExtractInfo
// ---------------------------------------------------------------------------
//	Post a GoToLine message to the text window.

void
MGoToLineWindow::ExtractInfo()
{
	const char*		text = fTextBox->Text();
	
	ASSERT(text);

	if (text)
	{
		int32		lineNumber;

		// use scanf to get thelinenumber from the text
		if (1 == sscanf(text, "%ld", &lineNumber))
		{
			BMessage		msg(msgGoToLine);

			lineNumber--;

			msg.AddInt32(kLineNumber, lineNumber);
		
			fWindow.PostMessage(&msg);
		}
		else
			beep();
	}
}
