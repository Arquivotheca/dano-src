// ---------------------------------------------------------------------------
/*
	MPreferencesWindow.cpp
	
	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	
	Heavily modified to separate preferences from project settings
	This became the base class as a generic preference window
		
	John R. Dance
	12 May 1999

*/
// ---------------------------------------------------------------------------

#include <string.h>

#include "MPreferencesWindow.h"
#include "MPrefsListView.h"
#include "MPreferencesView.h"
#include "Utils.h"
#include "IDEConstants.h"
#include "IDEMessages.h"
#include "MAlert.h"
#include "MainMenus.h"
#include "MFocusBox.h"
#include "MKeyFilter.h"

#include <Application.h>
#include <Autolock.h>

BRect sPrefsWindowFrame(100, 25, 650, 375);	// DR9

int32 MPreferencesWindow::fgLeftMargin = 135;
int32 MPreferencesWindow::fgTopMargin = 10;
int32 MPreferencesWindow::fgRightMargin = 535;
int32 MPreferencesWindow::fgBottomMargin = 310;

const int32 kFirstSelectibleView = 1;

// ---------------------------------------------------------------------------
// MPreferencesWindow member functions
// ---------------------------------------------------------------------------

MPreferencesWindow::MPreferencesWindow(const char* title)
	: BWindow(sPrefsWindowFrame, title, B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	CenterWindow(this);

	fCurrentView = nil;
	fListViewIndex = 0;
	fWindowIsBuilt = false;

	// The width changed for DR8 and height and width changed for DR9
	BRect bounds = Bounds();
	if (bounds.Width() != sPrefsWindowFrame.Width() || 
			bounds.Height() != sPrefsWindowFrame.Height()) {
		ResizeTo(sPrefsWindowFrame.Width(), sPrefsWindowFrame.Height());
	}

	PostMessage(msgBuildWindow);	// defer building the window
	Run();							// because we start out hidden
}

// ---------------------------------------------------------------------------

MPreferencesWindow::~MPreferencesWindow()
{
}

// ---------------------------------------------------------------------------

void
MPreferencesWindow::RemovePreferences()
{
}

// ---------------------------------------------------------------------------

bool
MPreferencesWindow::OKToQuit()
{
	// If we have pending changes, activate the window
	// so the user can see what is in there, and then ask about saving
	
	bool quitOK = true;
	if (fCurrentView != nil && fCurrentView->IsDirty()) {
		this->Activate();
		BAutolock lock(this);
		quitOK = this->AttemptCloseView();
	}
	
	return quitOK;
}

// ---------------------------------------------------------------------------

bool
MPreferencesWindow::QuitRequested()
{
	// We never really quit preference windows,
	// we just hide them (always return false).
	
	// However, only hide the window once was ask the user
	// about any pending change

	if (this->AttemptCloseView()) {
		if (fCurrentView) {
			fCurrentView->LastCall();
		}
		if (this->IsHidden() == false) {
			this->Hide();
		}
	}
	
	return false;
}

// ---------------------------------------------------------------------------

void
MPreferencesWindow::ShowAndActivate()
{
	BAutolock lock(this);

	// Move to the current workspace
	BRect frame = Frame();
	SetWorkspaces(B_CURRENT_WORKSPACE);
	if (frame != Frame()) {
		this->MoveTo(frame.left, frame.top);
	}
	if (IsHidden()) {
		this->Show();
	}
	this->Activate();
	
	// make sure we're onscreen (with potentially new workspace resolution)
	ValidateWindowFrame(*this);
}

// ---------------------------------------------------------------------------

void
MPreferencesWindow::Show()
{
	BAutolock lock(this);

	this->GetDataForCurrentView();
	this->PrefsViewModified();

	BWindow::Show();
}

// ---------------------------------------------------------------------------

void
MPreferencesWindow::MessageReceived(BMessage* inMessage)
{
	switch (inMessage->what)
	{
		case msgFactorySettings:
			ASSERT(fCurrentView);
			fCurrentView->DoFactorySettings();
			break;
		
		case msgRevertSettings:
			ASSERT(fCurrentView);
			fCurrentView->DoRevert();
			break;
		
		case msgCancelSettings:
			fCurrentView->DoRevert();
			PostMessage(B_QUIT_REQUESTED);
			break;
		
		case msgSaveSettings:
			this->DoSave();
			break;

		case msgPrefsViewModified:
			this->PrefsViewModified();
			break;
				
		case msgListSelectionChanged:
			this->ChangeView();
			break;
		
		case msgBuildWindow:
			this->BuildWindow();
			fWindowIsBuilt = true;
			break;

		// Hilite color changed
		case msgColorsChanged:
			ASSERT(fCurrentView);
			fListView->HiliteColorChanged();
			if (fCurrentView) {
				fCurrentView->Invalidate();
			}
			break;
		
		default:
			if (! SpecialMessageReceived(*inMessage, this))
				BWindow::MessageReceived(inMessage);
			break;
	}
}

// ---------------------------------------------------------------------------

void
MPreferencesWindow::VerifyWindowIsBuilt()
{
	//	Don't return until the window has actually completed building.
	//	Because the window is built asyncronously it's possible for
	//	some static functions to be called very early during app launch.
	//	This will most likely only happen if a project window has been
	//	double-clicked to launch the app.

	while (this->fWindowIsBuilt == false) {
		snooze(150000);
	}
}

// ---------------------------------------------------------------------------

int32
MPreferencesWindow::IndexOf(const char*	inTitle)
{
	//	The titles of the plug-ins can be hierarchical to indicate the
	//	section in the listview that they go into.
	//	ex: "Linker/PEF"
	//	This utility gets the index for one after the last child row after the
	//	collapsable row that matches the title.

	int32 index = -1;
	const char* slash = strchr(inTitle, '/');
	
	if (slash) {
		int32 len = slash - inTitle;
		PrefsRowData* rec;
		index = 0;

		// Find the appropriate section
		while (fDataList.GetNthItem(rec, index++)) {
			if (rec->isSection && 0 == strncmp(inTitle, rec->name, len)) {
				break;
			}
		}
		
		// Find the index after the last child row in this section
		while (fDataList.GetNthItem(rec, index) && ! rec->isSection) {
			index++;
		}
	}

	return index;
}

// ---------------------------------------------------------------------------

int32
MPreferencesWindow::VisibleIndexOf(const char*	inTitle)
{
	//	The titles of the plug-ins can be hierarchical to indicate the
	//	section in the listview that they go into.
	//	ex: "Linker/PEF"
	//	This utility gets the visible index for one after the last child row after the
	//	collapsable row that matches the title.

	int32 index = 0;
	const char* slash = strchr(inTitle, '/');
	BList& list = *fListView->GetList();
	
	if (slash) {
		int32 len = slash - inTitle;
		int32 term = list.CountItems();
		PrefsRowData* rec;

		// Find the appropriate section
		while (index < term) {
			rec = (PrefsRowData*) list.ItemAt(index++);
			if (rec->isSection && 0 == strncmp(inTitle, rec->name, len)) {
				break;
			}
		}
		
		// Find the index after the last child row in this section
		while (index < term) {
			rec = (PrefsRowData*) list.ItemAt(index);
			if (rec->isSection) {
				break;
			}
			index++;
		}
	}

	return index;
}

// ---------------------------------------------------------------------------

void
MPreferencesWindow::AddPreferenceCategory(const char* title)
{
	PrefsRowData* rec = new PrefsRowData(nil, nil, true, title);
	fListView->InsertCollapsableRow(fListView->CountRows(), rec);
	fDataList.AddItem(rec);
}

// ---------------------------------------------------------------------------

void
MPreferencesWindow::AddView(MPreferencesView* inView, void* viewCookie)
{
	fTopView->AddChild(inView);
	inView->Hide();

	fViewList.AddItem(inView);
	PrefsRowData* rec = new PrefsRowData(inView, viewCookie, false, inView->Title());
	fListView->InsertRow(fListView->CountRows(), rec);
	fDataList.AddItem(rec);
}

// ---------------------------------------------------------------------------

void
MPreferencesWindow::AddView(MPreferencesView* inView, void* viewCookie,
							int32 inIndex, const char* inTitle)
{
	fTopView->AddChild(inView);
	inView->Hide();

	fViewList.AddItem(inView, inIndex);

	PrefsRowData* rec = new PrefsRowData(inView, viewCookie, false, inTitle);
	fListView->InsertRow(inIndex, rec);
	fDataList.AddItem(rec, inIndex);
}

// ---------------------------------------------------------------------------

void
MPreferencesWindow::ShowView(MPreferencesView* inView)
{		
	//	Show a new view in response to hitting one of the preferences view buttons.
	ASSERT(inView);

	if (inView != fCurrentView && this->AttemptCloseView()) {
		if (fCurrentView) {
			fCurrentView->Hide();
		}
		fCurrentView = inView;
		this->GetDataForCurrentView();
		inView->Show();
		this->PrefsViewModified();
	}
}

// ---------------------------------------------------------------------------

void
MPreferencesWindow::GetDataForCurrentView()
{
	//	Show a new view in response to hitting one of the preferences view buttons.

	ASSERT(fCurrentView);

	BMessage msg;
	
	fCurrentView->GetData(msg, true);	// What prefs data does it want?
	this->GetTargetData(msg);			// get that data from the target
	fCurrentView->SetData(msg);			// Tell the view the new data
	fCurrentView->DoSave();				// Start clean
}

// ---------------------------------------------------------------------------

bool
MPreferencesWindow::AttemptCloseView()
{
	//	See if the current prefs view will allow itself to be changed.

	bool result = true;
	
	if (fCurrentView != nil && fCurrentView->IsDirty()) {
		MSaveFileAlert alert("Save changes to this preferences panel?", "Save", "Don't Save", "Cancel");
		alert.SetShortcut(MSaveFileAlert::kDontSaveButton, 'd');

		switch (alert.Go()) {
			case MSaveFileAlert::kSaveButton:
				this->DoSave();
				break;

			case MSaveFileAlert::kDontSaveButton:
				fCurrentView->DoRevert();
				break;

			case MSaveFileAlert::kCancelButton:
				result = false;
				break;
		}
	}

	return result;
}

// ---------------------------------------------------------------------------

void
MPreferencesWindow::WindowActivated(
	bool 	inActive)
{
	BWindow::WindowActivated(inActive);
	if (inActive )
	{
		// Fix up the cursor if needed
		BPoint 	where;
		uint32	buttons;

		fTopView->GetMouse(&where, &buttons);
		fTopView->ConvertToScreen(&where);
		ConvertFromScreen(&where);
		BView*	view = FindView(where);
		
		if (view != nil)
		{
			BTextView* textView = dynamic_cast<BTextView*>(view);
			if (textView == nil) {
				be_app->SetCursor(B_HAND_CURSOR);
			}
		}
	}
}

// ---------------------------------------------------------------------------

void
MPreferencesWindow::PrefsViewModified()
{
	//	Update the save and revert
	//	buttons to match the dirty state of the current view.

	bool isDirty = fCurrentView->IsDirty();
	
	// if we can't change settings because of the current project
	// revert the view back to previous settings
	if (isDirty && this->CanChangeSettings() == false) {
		fCurrentView->DoRevert();
		return;
	}
	
	fRevertButton->SetEnabled(isDirty);
	fSaveButton->SetEnabled(isDirty);
}

// ---------------------------------------------------------------------------

void
MPreferencesWindow::ChangeView()
{
	//	Respond to a change in the selection in the list view.
	int32 visibleIndex = -1;
	
	if (fListView->NextSelected(visibleIndex)) {
		bool changed = false;
		int32 wideOpenIndex = fListView->GetWideOpenIndex(visibleIndex);
		PrefsRowData* newrec = (PrefsRowData*) fListView->GetList()->ItemAt(visibleIndex);
		PrefsRowData* prevrec;

		fDataList.GetNthItem(prevrec, fListViewIndex);
		if (wideOpenIndex != fListViewIndex && newrec != nil && prevrec != nil ) {
			if (! newrec->isSection) {
				MPreferencesView* temp = fCurrentView;
				this->ShowView(newrec->view);
				if (fCurrentView == temp && ! prevrec->isSection) {
					// The view didn't change so reset the selection
					// in the list
					fListView->SelectRow(fListViewIndex);
				}
				else {
					changed = true;
				}
			}
		}
			
		if (changed) {
			fListViewIndex = wideOpenIndex;
		}	
	}
}

// ---------------------------------------------------------------------------

bool
MPreferencesWindow::CanChangeSettings() const
{
	return true;
}

// ---------------------------------------------------------------------------

void
MPreferencesWindow::BuildWindow()
{
	BRect			r;
	BButton*		button;
	BMessage*		msg;
	BPopUpMenu*		popup;
	MPrefsListView*	list;
	MFocusBox*		focusBox;
	BScrollView*	frame;
	BMenuField*		menufield;

	// Build a special topview so we can have a grey background for
	// the window
	r = Bounds();
	fTopView = new BView(r, "ourtopview", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	AddChild(fTopView);
	SetGrey(fTopView, kLtGray);

	// List View 
	// (bottom-1.0 gives it a nicer look wrt preference views)
	float top = fgTopMargin;
	r.Set(5, top, 130, fgBottomMargin-1.0);

	focusBox = new MFocusBox(r);						// Focus Box
	fTopView->AddChild(focusBox);
	SetGrey(focusBox, kLtGray);
	focusBox->SetHighColor(kFocusBoxGray);

	r.InsetBy(10.0, 3.0);
	r.OffsetTo(3.0, 3.0);
	list = new MPrefsListView(r, "listview");
	list->SetTarget(this);
	list->SetFocusBox(focusBox);

	frame = new BScrollView("frame", list, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_FRAME_EVENTS, false, true, B_PLAIN_BORDER);		// For the border
	focusBox->AddChild(frame);
	list->SetFont(be_plain_font);
	font_height		info = list->GetCachedFontHeight();
	list->SetDefaultRowHeight(info.ascent + info.descent + info.leading + 1.0);
	fListView = list;

	// Buttons
	// Factory Settings
	top = fgBottomMargin + 7.0;
	r.Set(10, top, 110, top + 20.0);
	msg = new BMessage(msgFactorySettings);
	button = new BButton(r, "FactorySettings", "Factory Settings", msg); 
	fTopView->AddChild(button);
	button->SetTarget(this);
	fFactoryButton = button;

	// Revert Panel
	r.Set(130, top, 230, top + 20.0);
	msg = new BMessage(msgRevertSettings);
	button = new BButton(r, "Revert", "Revert Panel", msg); 
	fTopView->AddChild(button);
	button->SetTarget(this);
	fRevertButton = button;

	// Cancel
	r.Set(250, top, 335, top + 20.0);
	msg = new BMessage(msgCancelSettings);
	button = new BButton(r, "Cancel", "Cancel", msg); 
	fTopView->AddChild(button);
	button->SetTarget(this);
	fCancelButton = button;

	// Save
	r.Set(355, top, 440, top + 20.0);
	msg = new BMessage(msgSaveSettings);
	button = new BButton(r, "Save", "Save", msg); 
	fTopView->AddChild(button);
	button->SetTarget(this);
	fSaveButton = button;
	SetDefaultButton(button);

	AddCommonFilter(new MTextKeyFilter(this, kBindingGlobal));

	// We don't have a key menu bar in this window so make sure 
	// any popups added to the window aren't considered the keymenubar
	SetKeyMenuBar(nil);

	// Finish building the window
	this->BuildViews();
	
	// Now select the first item and show its view
	fListView->MakeFocus();
	fListView->SelectRow(kFirstSelectibleView);
	this->ChangeView();

}

// ---------------------------------------------------------------------------

void
MPreferencesWindow::DoSave()
{
	ASSERT(fCurrentView);
	
	BMessage msg(msgSetData);
	if (this->OKToSave(msg)) {
		// Get the current data from the current view
		fCurrentView->GetData(msg, false);
		
		// Tell the target to save its prefs
		this->SetTargetData(msg);
	
		// Let the current view know that it was saved
		fCurrentView->DoSave();
		
		// Update our buttons
		PrefsViewModified();
	}
}
