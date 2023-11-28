// ---------------------------------------------------------------------------
/*
	MRemoveFileSetWindow.cpp
	
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			23 April 1999

*/
// ---------------------------------------------------------------------------

#include "MRemoveFileSetWindow.h"
#include "MFileSet.h"
#include "Utils.h"
#include "IDEMessages.h"

#include <StringView.h>
#include <ScrollView.h>
#include <ListView.h>
#include <ListItem.h>
#include <Button.h>


// ---------------------------------------------------------------------------

MRemoveFileSetWindow::MRemoveFileSetWindow(BMessenger* adoptTarget, MFileSetKeeper& fileSetKeeper)
					 : BWindow(BRect(0, 0, 250, 200), 
					 		   "Remove A File Set",
					 		   B_TITLED_WINDOW,
					 		   B_NOT_RESIZABLE | B_NOT_ZOOMABLE),
					   fFileSetKeeper(fileSetKeeper)
{
	fMessenger = adoptTarget;

	::CenterWindow(this);
	MRemoveFileSetWindow::BuildWindow();
	MRemoveFileSetWindow::Show();
}

// ---------------------------------------------------------------------------

MRemoveFileSetWindow::~MRemoveFileSetWindow()
{
	// the BListView does not delete the BStringItems
	// we have to do it ourselves
	
	int32 numItems = fListView->CountItems();
	for (int i = 0; i < numItems; i++) {
		BListItem* anItem = fListView->RemoveItem((int32) 0);
		delete anItem;
	}

	fMessenger->SendMessage(msgRemoveFileSetWindowClosed);
	delete fMessenger;
}

// ---------------------------------------------------------------------------

const float kListHeight = 100.0;

void
MRemoveFileSetWindow::BuildWindow()
{
	BRect bounds = this->Bounds();
	BRect r = bounds;
	BView* topView = new BView(r, "topview", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	this->AddChild(topView);
	SetGrey(topView, kLtGray);

	// Title
	float top = 10.0;
	r.Set(20.0, top, bounds.right-20.0, top+16.0);
	BStringView* title = new BStringView(r, "title", "Select file sets to remove:"); 
	topView->AddChild(title);
	title->SetFont(be_bold_font);
	SetGrey(title, kLtGray);
	top += 25.0;

	// List box
	r.Set(20.0, top, bounds.right-35.0, top+kListHeight);
	fListView = new BListView(r, "listview", B_MULTIPLE_SELECTION_LIST);
	fListView->SetTarget(this);
	
	// scroller
	BScrollView* frame = new BScrollView("frame", fListView, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_FRAME_EVENTS, false, true, B_FANCY_BORDER);		// For the border
	topView->AddChild(frame);
	top += kListHeight + 20.0;

	// Remove button
	r.Set(bounds.right-100.0, top, bounds.right-20.0, top + 15.0);
	fOKButton = new BButton(r, "OK", "Remove", new BMessage(msgOK)); 
	topView->AddChild(fOKButton);
	fOKButton->SetTarget(this);
	SetDefaultButton(fOKButton);
	SetGrey(fOKButton, kLtGray);
	fOKButton->SetEnabled(false);

	// Cancel button
	r.Set(bounds.right-200.0, top, bounds.right-120.0, top + 15.0);
	BButton* button = new BButton(r, "Cancel", "Cancel", new BMessage(msgCancel)); 
	topView->AddChild(button);
	button->SetTarget(this);
	SetGrey(button, kLtGray);
	
	// add all the file sets to the list
	this->BuildListView();
	
	fListView->MakeFocus();
}

// ---------------------------------------------------------------------------

void
MRemoveFileSetWindow::BuildListView()
{
	// iterate through all the system wide and the project specific file sets
	// and add them to the list view
	
	const MFileSet* aSet = NULL;
	for (int32 i = 0; aSet = fFileSetKeeper.GetNthFileSet(i, false); i++) {
		fListView->AddItem(new BStringItem(aSet->fName));
	}
	
	for (int32 i = 0; aSet = fFileSetKeeper.GetNthFileSet(i, true); i++) {
		fListView->AddItem(new BStringItem(aSet->fName));
	}

	// when an item is invoked, send this message
	fListView->SetInvocationMessage(new BMessage(msgRowSelected));
	
	// send this message when an item is selected/deselected
	fListView->SetSelectionMessage(new BMessage(msgSelectionChanged));

}

// ---------------------------------------------------------------------------

void
MRemoveFileSetWindow::MessageReceived(BMessage* inMessage)
{
	switch (inMessage->what) {
		case msgRowSelected:
			this->DoRemoveSet(*inMessage);	
			PostMessage(B_QUIT_REQUESTED);
			break;

		case msgOK:
			this->DoRemoveSet(*inMessage);
			PostMessage(B_QUIT_REQUESTED);
			break;
			
		case msgCancel:
			PostMessage(B_QUIT_REQUESTED);
			break;

		case msgSelectionChanged:
			// Enable/disable the OK button depending on whether we have a selection
			fOKButton->SetEnabled(fListView->CurrentSelection() >= 0);
			break;
		
		default:
			BWindow::MessageReceived(inMessage);
			break;
	}
}

// ---------------------------------------------------------------------------

void
MRemoveFileSetWindow::DoRemoveSet(BMessage& inMessage)
{
	int32 index;

	// if we have "index" then a line was double-clicked
	// otherwise the OK button was pressed
	if (inMessage.FindInt32("index", &index) == B_OK) {
		BStringItem* item = (BStringItem*) fListView->ItemAt(index);
		BMessage msg(msgRemoveFileSet);
		msg.AddString(kFileName, item->Text());
		fMessenger->SendMessage(&msg);
	}
	else {
		// for the OK button, we need to iterate through 
		// all selected items
	   for (int32 i = 0; (index = fListView->CurrentSelection(i)) >= 0; i++) { 
			BStringItem* item = (BStringItem*) fListView->ItemAt(index);
			BMessage msg(msgRemoveFileSet);
			msg.AddString(kFileName, item->Text());
			fMessenger->SendMessage(&msg);
		}		
	}
}


