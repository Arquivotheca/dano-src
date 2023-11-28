// ---------------------------------------------------------------------------
/*
	FilterWindow.cpp
	
	Copyright (c) 2001 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			17 May 2001

	Allow the user to pick from a set of common filters
	
*/
// ---------------------------------------------------------------------------

#include "FilterWindow.h"

#include <Screen.h>
#include <View.h>
#include <StringView.h>
#include <Button.h>
#include <ListView.h>
#include <TextControl.h>
#include <ScrollView.h>
#include <Alert.h>
#include <String.h>

#include <stdio.h>

const uint32 msgOK			= 'DoIt';
const uint32 msgEdit		= 'Edit';
const uint32 msgCancel		= 'Quit';
const uint32 msgRemove		= 'Remv';

const uint32 msgInvokeItem	= 'Invk';
const uint32 msgSelectItem	= 'Slct';

// ---------------------------------------------------------------------------
//	A few nice window utilities
// ---------------------------------------------------------------------------

void
CenterWindow(BWindow* inWindow)
{
	BScreen screen(inWindow);
	BRect frame = screen.Frame();
	BRect windowFrame = inWindow->Frame();
	const float screenCenterWid = frame.Width() / 2.0;
	const float screenCenterHeight = frame.Height() / 2.0;

	inWindow->MoveBy(screenCenterWid - (windowFrame.Width() / 2.0) - windowFrame.left, 
						screenCenterHeight - (windowFrame.Height() / 2.0) - windowFrame.top);
}

// ---------------------------------------------------------------------------

const rgb_color kLtGray = { 235, 235, 235, 255 };

void
SetGray(BView* view)
{
	view->SetViewColor(kLtGray);
	view->SetLowColor(kLtGray);
}

// ---------------------------------------------------------------------------

FilterWindow::FilterWindow(FilterHandler* handler, BList& commands) 
			 : BWindow(BRect(0, 0, 326, 260), "Filter", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	fCurrentSelection = -1;
	fHandler = handler;
	
	::CenterWindow(this);

	// Build a special topview so we can have a grey background for
	// the window
	float top = 10.0;
	BRect r = Bounds();
	BView* topView = new BView(r, "ourtopview", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	this->AddChild(topView);
	::SetGray(topView);

	// Static text
	r.Set(16.0, top, 290.0, top + 16.0);
	BStringView* caption = new BStringView(r, "select", "Select filter:"); 
	topView->AddChild(caption);
	caption->SetFont(be_bold_font);
	::SetGray(caption);
	top += 25.0;

	// List box
	const float kListHeight = 140.;
	r.Set(20.0, top, 290.0, top + kListHeight);
	fListView = new BListView(r, "list", B_SINGLE_SELECTION_LIST);
	fListView->SetTarget(this);
	topView->AddChild(new BScrollView("scroll_list", fListView, B_FOLLOW_LEFT | B_FOLLOW_TOP, 0, false, true));
	top += kListHeight + 10.0;

	// Edit box
	r.Set(16.0, top, 306.0, top + 20.0);
	fEditBox = new BTextControl(r, "edit", "Edit Filter:", "", NULL);
	fEditBox->SetDivider(100);
	fEditBox->SetTarget(this);
	fEditBox->SetModificationMessage(new BMessage(msgEdit));
	topView->AddChild(fEditBox);
	::SetGray(fEditBox);
	top += 30.;
	
	// Remove button
	r.Set(20.0, top, 100.0, top + 15.0);
	fRemoveButton = new BButton(r, "Remove", "Remove", new BMessage(msgRemove));
	topView->AddChild(fRemoveButton);
	fRemoveButton->SetTarget(this);
	::SetGray(fRemoveButton);
	fRemoveButton->SetEnabled(false);
	
	// OK button
	r.Set(210.0, top, 290.0, top + 15.0);
	fOKButton = new BButton(r, "OK", "OK", new BMessage(msgOK)); 
	topView->AddChild(fOKButton);
	fOKButton->SetTarget(this);
	this->SetDefaultButton(fOKButton);
	::SetGray(fOKButton);
	fOKButton->SetEnabled(false);

	// Cancel button
	r.Set(120.0, top, 200.0, top + 15.0);
	BButton* cancelButton = new BButton(r, "Cancel", "Cancel", new BMessage(msgCancel)); 
	topView->AddChild(cancelButton);
	cancelButton->SetTarget(this);
	::SetGray(cancelButton);
	
	// Build the list view contents
	FilterWindow::BuildListView(fListView, commands);
	
	fListView->MakeFocus();
}

// ---------------------------------------------------------------------------

FilterWindow::~FilterWindow()
{
	// The BListView doesn't delete the list items so we need to do it	
	int32 numItems = fListView->CountItems();
	for (int i = 0; i < numItems; i++) {
		BListItem* anItem = fListView->RemoveItem((int32) 0);
		delete anItem;
	}
}

// ---------------------------------------------------------------------------

void
FilterWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
	case msgInvokeItem:
		fCurrentSelection = this->GetSelectedItem(message);
		if (this->IsValidSelection()) {
			// apply the filter selected in the list box
			fHandler->SetCommand(this->GetSelectionText());
			PostMessage(B_QUIT_REQUESTED);
		}
		break;
	
	case msgOK:
		// apply the filter in the edit box
		fHandler->SetCommand(fEditBox->Text());
		PostMessage(B_QUIT_REQUESTED);
		break;
	
	case msgEdit:
		// allow ok if we have some text
		fOKButton->SetEnabled(strlen(fEditBox->Text()) > 0);
		break;
		
	case msgCancel:
		PostMessage(B_QUIT_REQUESTED);
		break;
	
	case msgRemove:
		if (this->IsValidSelection()) {
			BStringItem* unwanted = (BStringItem*) fListView->RemoveItem(fCurrentSelection);
			fHandler->RemoveCommand(unwanted->Text());
			fRemoveButton->SetEnabled(false);
			fOKButton->SetEnabled(false);
			fEditBox->SetText("");
		}
		break;

	case msgSelectItem:
		fCurrentSelection = this->GetSelectedItem(message);
		if (this->IsValidSelection()) {
			// transfer list item to edit box for editing
			fEditBox->SetText(this->GetSelectionText());
			
			// allow remove/ok if we have a current selection
			fRemoveButton->SetEnabled(true);
			fOKButton->SetEnabled(true);
		}
		else {
			// disallow remove/ok with an invalid selection
			fRemoveButton->SetEnabled(false);
			fOKButton->SetEnabled(false);
		}
		break;
	
	default:
		BWindow::MessageReceived(message);
		break;
	}
}

// ---------------------------------------------------------------------------

const char*
FilterWindow::GetSelectionText() const
{
	BStringItem* item = (BStringItem*) fListView->ItemAt(fCurrentSelection);
	return item->Text();
}

// ---------------------------------------------------------------------------

int32
FilterWindow::GetSelectedItem(BMessage* message) const
{
	int32 index = -1;
	message->FindInt32("index", &index);

	return index;
}

// ---------------------------------------------------------------------------

bool
FilterWindow::IsValidSelection() const
{
	if (fCurrentSelection >= 0 && fCurrentSelection < fListView->CountItems()) {
		return true;
	}
	else {
		return false;
	}
}

// ---------------------------------------------------------------------------

void
FilterWindow::BuildListView(BListView* list, BList& commands)
{	
	int32 count = commands.CountItems();
	for (int i = 0; i < count; i++) {
		BString* cmd = (BString*) commands.ItemAt(i);
		list->AddItem(new BStringItem(cmd->String()));
	}

	// when an item is invoked, send this message
	list->SetInvocationMessage(new BMessage(msgInvokeItem));
	
	// send this message when an item is selected/deselected
	list->SetSelectionMessage(new BMessage(msgSelectItem));
}

