// ---------------------------------------------------------------------------
/*
	MMessageWindow.cpp
	
	Copyright 1995 Metrowerks Corporation, All Rights Reserved.

	Heavily modified to split Errors&Warnings from Information
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			10 June 1999

*/
// ---------------------------------------------------------------------------

#include "MMessageWindow.h"
#include "MInformationMessageWindow.h"
#include "MMessageItem.h"
#include "MDynamicMenuHandler.h"
#include "MDefaultPrefs.h"
#include "MMessageInfoView.h"
#include "MPreferences.h"
#include "MKeyFilter.h"
#include "IDEApp.h"
#include "IDEConstants.h"
#include "IDEMessages.h"
#include "MProjectWindow.h"
#include "MainMenus.h"
#include "ProjectCommands.h"
#include "Utils.h"
#include "MLocker.h"

#include <Autolock.h>
#include <Clipboard.h>
#include <stdlib.h>

BRect sMessageWindowFrame(80.0, 50.0, 280.0, 350.0);

BList MMessageWindow::fgMessageWindowList;
BLocker MMessageWindow::fgMessageWindowListLock("message_window_list_lock");
MMessageWindow* MMessageWindow::fgGeneralMessageWindow = nil;

extern BLocker CursorLock;

const float kInfoViewHeight = 22.0 + 3;
const float kMessageLeft = 0.0 + kBorderWidth;
const float kMessageRight = 640.0 - kBorderWidth;

const int32 kFirstFontMenuItem = 2;

// ---------------------------------------------------------------------------

MMessageWindow::MMessageWindow(const char* title, 
							   const char* infoTitle)
			   : BWindow(sMessageWindowFrame, title, B_DOCUMENT_WINDOW, 0L)
{
	BAutolock listLock(&fgMessageWindowListLock);
	
	fSavePanel = nil;
	
	MMessageWindow::GetPrefs();

	BMessage msg(msgBuildWindow);
	msg.AddString("title", infoTitle);
	
	// start the message window running (and building)	
	MMessageWindow::PostMessage(&msg);
	MMessageWindow::Run();
	
	// Add this message window to our window list (even if it isn't open)
	// since it isn't open, we add it to the front rather than the end
	// ...activation will move it to the end (most recent) position
	fgMessageWindowList.AddItem(this, 0);
}

// ---------------------------------------------------------------------------

MMessageWindow::~MMessageWindow()
{
	BAutolock listLock(&fgMessageWindowListLock);
			
	// Remove the window from its position in the list if it exists
	fgMessageWindowList.RemoveItem(this);

	// Keep our general message window pointer up to date if we are deleting it
	if (this == fgGeneralMessageWindow) {
		fgGeneralMessageWindow = nil;
	}

	delete fSavePanel;
	SetPrefs();

	// Now if this is the last message window sitting around, tell the BeIDE
	if (fgMessageWindowList.CountItems() == 0) {
		be_app_messenger.SendMessage(msg_MessageWindowsAllGone);
	}
}

// ---------------------------------------------------------------------------

void
MMessageWindow::BuildWindow(const char* infoTitle)
{
	float menuBarHeight = 16.0;

	// Build the menu bar
	BRect	r = Bounds();
	r.right++;
	r.bottom = r.top + menuBarHeight;
	fMenuBar = MakeMessageWindowMenus(r);
	AddChild(fMenuBar);
	menuBarHeight = fMenuBar->Bounds().Height();

	// Add the View menu item to our menu bar
	// Since it is all hierarchical, and doesn't have global commands
	// we build it locally here...
	
	fFontMenu = new BMenu("View");
	fSizeMenu = new BMenu("Size");

	::FillFontSizeMenu(fSizeMenu);
	fFontMenu->AddItem(new BMenuItem(fSizeMenu));
	::MarkCurrentFontSize(fSizeMenu, fFontPrefs.pFontSize);
	
	fFontMenu->AddSeparatorItem();

	::FillFontFamilyMenu(fFontMenu);
	::MarkCurrentFont(fFontMenu, fFontPrefs.pFontFamily, fFontPrefs.pFontStyle, kFirstFontMenuItem);
	
	fFontMenu->SetTargetForItems(this);
	fMenuBar->AddItem(new BMenuItem(fFontMenu));
	
	// Save window menu
	BMenuItem* item = fMenuBar->FindItem(cmd_ErrorMessageWindow);
	ASSERT(item);
	fWindowMenu = item->Menu();
	ASSERT(fWindowMenu);

	// Disable all these edit menu items
	EnableItem(fMenuBar, cmd_Undo, false);
	EnableItem(fMenuBar, cmd_Cut, false);
	EnableItem(fMenuBar, cmd_Paste, false);
	EnableItem(fMenuBar, cmd_Clear, false);
	EnableItem(fMenuBar, cmd_SelectAll, false);

	// Build the info view and contents
	r = Bounds();
	r.top = menuBarHeight + 1.0;
	r.bottom = menuBarHeight + 1.0 + kInfoViewHeight;
	this->AddChild(this->CreateInfoView(r, infoTitle));
	this->BuildInfoView();
	
	// Build the message view
	r = Bounds();
	r.top = menuBarHeight + 1.0 + kInfoViewHeight;
	r.right -= B_V_SCROLL_BAR_WIDTH;
	r.bottom -= B_H_SCROLL_BAR_HEIGHT;
	r.left--;
	fMessageView = new MMessageView(r, "Message View");

	// Build the scrolling view and add the message view to it
	BScrollView* scrollView = new BScrollView("scroller",
											  fMessageView, 
											  B_FOLLOW_ALL_SIDES, 
											  B_WILL_DRAW, 
											  true, 
											  true, 
											  B_NO_BORDER);
	this->AddChild(scrollView);

	BFont font;
	font.SetFamilyAndStyle(fFontPrefs.pFontFamily, fFontPrefs.pFontStyle);
	font.SetSize(fFontPrefs.pFontSize);
	fMessageView->SetFont(&font);
	fMessageView->MoveBy(1.0, 1.0);
	fMessageView->ResizeBy(-1.0, -1.0);

	BScrollBar* scrollBar = scrollView->ScrollBar(B_HORIZONTAL);
	scrollBar->SetSteps(10, 50);

	AddCommonFilter(new MTextKeyFilter(this, kBindingGlobal));
	AddCommonFilter(new MWindowMenuFilter);

	fMessageView->MakeFocus();
	SetSizeLimits(100.0, 1000.0, 100.0, 2000.0);
	SetZoomLimits(640.0, 1000.0);
}

// ---------------------------------------------------------------------------

bool
MMessageWindow::QuitRequested()
{
	return true;
}

// ---------------------------------------------------------------------------

void
MMessageWindow::MessageReceived(
	BMessage * inMessage)
{
	switch (inMessage->what) {
		case cmd_SaveCopyAs:
			ShowSavePanel();
			break;

		// Edit menu
		case cmd_Copy:
			DoCopy();
			break;

		// File menu
		case cmd_CloseAll:
		{
			// close all the message (and Error message) windows
			BMessage msg(cmd_Close);
			MMessageWindow::MessageToAllMessageWindows(msg);
			break;
		}
		
		// Window menu
		case msgWindowMenuClicked:
		case msgOpenRecent:
		case cmd_Stack:
		case cmd_Tile:
		case cmd_TileVertical:
			be_app_messenger.SendMessage(inMessage);
			break;

		case cmd_ErrorMessageWindow:
		case cmd_ShowProjectWindow:
		{
			MProjectWindow* projectWindow = this->GetAssociatedProject();
			if (projectWindow) {
				projectWindow->PostMessage(inMessage);
			}
			break;
		}
		
		case cmd_PrevMessage:
		case cmd_NextMessage:
			fMessageView->MessageReceived(inMessage);
			break;
		
		case msgShowAndActivate:
		{
			// when msgShowAndActivate is called during compiles/links/finds
			// bring the window up in its current workspace, don't switch 
			// workspaces or bring the message window to the user
			// this is then turned off so that the deskbar or the menu item
			// Window/Message Window can be used to switch to its workspace
			// (all menu activation of window uses cmd_ErrorMessageWindow,
			// or calls Activate() directly - see MDynamicMenuHandler::MenuClicked.)
			uint32 flags = this->Flags();
			uint32 noActivateFlags = flags | B_NO_WORKSPACE_ACTIVATION;
			this->SetFlags(noActivateFlags);	
			ShowAndActivate();
			// Now clear the "don't activate workspace flag"
			this->SetFlags(flags);
			break;
		}
		
		case msgBuildWindow:
			this->BuildWindow(inMessage->FindString("title"));
			break;
		
		case msgClearMessages:
			ClearMessages(*inMessage);
			break;

		case msgUpdateMenus:
			UpdateMessageWindowMenus(fMenuBar);
			break;

		// Hilite color changed
		case msgColorsChanged:
			fMessageView->HiliteColorChanged();
			break;

		// The save panel
		case B_SAVE_REQUESTED:
		{
			entry_ref dir;
			inMessage->FindRef("directory", &dir);

			const char *name = NULL;
			inMessage->FindString("name", &name);

			SaveRequested(&dir, name);
			delete fSavePanel;
			fSavePanel = nil;
			break;
		}

		// from the Font menu...
		case msgSizeChosen:
		{
			BMenuItem *menu = nil;
			inMessage->FindPointer("source", (void **)&menu);
			this->ChangeFontSize(atof(menu->Label()));
			break;
		}
		
		case msgStyleChosen:
		{
			BMenuItem *menu = nil;
			inMessage->FindPointer("source", (void **)&menu);
			this->ChangeFont(menu->Menu()->Superitem()->Label(), menu->Label());
			break;
		}
		
		case msgFontChosen:
		{
			BMenuItem *menu = nil;
			inMessage->FindPointer("source", (void **)&menu);
			this->ChangeFont(menu->Label(), nil);
			break;
		}
		
		default:
			if (! SpecialMessageReceived(*inMessage, this))
				BWindow::MessageReceived(inMessage);
			break;
	}
}

// ---------------------------------------------------------------------------

void
MMessageWindow::Zoom(BPoint /*inPosition*/, float /*inWidth*/, float /*inHeight*/)
{
	// Calculate the system state rect
	BRect			windFrame = Frame();
	BScreen			screen(this);
	BRect			frame = screen.Frame();
	float			rowHeight;
	
	if (fMessageView->CountRows() > 0) {
		rowHeight = fMessageView->GetRowHeight(0);
	}
	else {
		rowHeight = 20.0;
	}
	
	const float contentHeight = (rowHeight * 5) +
			fMenuBar->Bounds().Height() + B_H_SCROLL_BAR_HEIGHT + kInfoViewHeight;

	BRect sysFrame(kMessageLeft, frame.bottom - kBorderWidth - contentHeight, kMessageRight, frame.bottom - kBorderWidth);

	// If the window is in user state or the frame has changed since the last
	// zoom then go to system state
	if (fUserState || sysFrame != windFrame) {		// going to system state
		fUserRect = windFrame;

		MoveTo(sysFrame.LeftTop());
		ResizeTo(sysFrame.Width(), sysFrame.Height());	
		fUserState = false;
	}
	else {
		// Restore the user state frame
		MoveTo(fUserRect.LeftTop());
		ResizeTo(fUserRect.Width(), fUserRect.Height());	
		fUserState = true;
	}
}

// ---------------------------------------------------------------------------

void
MMessageWindow::PostErrorMessage(const ErrorNotificationMessage& inError, bool inShowAndActivate)
{
	BMessage msg(msgAddToMessageWindow);
	int32 size = strlen(inError.errorMessage) + 1 + offsetof(ErrorNotificationMessage, errorMessage);
	msg.AddData(kErrorMessageName, kErrorType, &inError, size);

	this->PostMessage(&msg);
	if (inShowAndActivate) {
		this->PostMessage(msgShowAndActivate);
	}
}

// ---------------------------------------------------------------------------

void
MMessageWindow::PostErrorMessage(const ErrorNotificationMessageShort& inError, bool inShowAndActivate)
{
	BMessage msg(msgAddToMessageWindow);
	int32 size = strlen(inError.errorMessage) + 1 + offsetof(ErrorNotificationMessage, errorMessage);
	msg.AddData(kErrorMessageName, kErrorType, &inError, size);

	this->PostMessage(&msg);
	if (inShowAndActivate) {
		this->PostMessage(msgShowAndActivate);
	}
}

// ---------------------------------------------------------------------------

void
MMessageWindow::ShowAndActivate()
{	
	// Move to the current workspace
	BRect frame = Frame();
	if (frame != Frame()) {
		this->MoveTo(frame.left, frame.top);
	}
	if (this->IsHidden()) {
		this->Show();
	}
	this->Activate();
	ValidateWindowFrame(*this);		// make sure that we're onscreen
}

// ---------------------------------------------------------------------------

void
MMessageWindow::WindowActivated(bool inActive)
{
	// deal with the list of message windows and workaround for cursor
	
	BWindow::WindowActivated(inActive);

	if (inActive) {
		// different blocks so I don't have to worry about deadlock
		// the locks locked/unlocked separately and independently
		{
			// Keep track of the last message window activated...
			// We need to keep a list so that when one goes away, we can use the next
			BAutolock listLock(&fgMessageWindowListLock);
			
			// Remove the window from its position in the list if it exists
			fgMessageWindowList.RemoveItem(this);
		
			// Now add it on the end...
			fgMessageWindowList.AddItem(this);
		}
		
		{	
			// Workaround for the problem with cursors...
			BAutolock lock(&CursorLock);
			if (lock.IsLocked()) {
				if (be_app->IsCursorHidden())		// just paranoid
					be_app->ShowCursor();
				be_app->SetCursor(B_HAND_CURSOR);
			}
		}
		
	}
}

// ---------------------------------------------------------------------------

void
MMessageWindow::MenusBeginning()
{
	bool hasContents = fMessageView->CountRows() > 0;

	// Set up the menu items to match the current modifier keys, if any
	UpdateMenuItemsForModifierKeys(fMenuBar);

	EnableItem(fMenuBar, cmd_Copy, hasContents);
	EnableItem(fMenuBar, cmd_SaveCopyAs, hasContents);
	
	// Upate the window/project list menus
	MDynamicMenuHandler::UpdateDynamicMenus(*fMenuBar, this->GetAssociatedProject());
}

// ---------------------------------------------------------------------------

void
MMessageWindow::ClearMessages(bool inSync)
{
	//	Clear messages from the window and its list in preparation for
	//	replacing these messages with new ones.
	//	Post a message to the message window telling it to clear the indicated
	//	messagetype.  This function can be called from any other thread at any
	//	time.  The inSync parameter specifies whether to wait until the task
	//	is completed before returning.  Typically you only need to call this
	//	function syncronously if you are about to start a task that will send
	//	potentially many messages to the message window.  In that case it's
	//	important to wait until the window is clear, which could take a while,
	//	before posting it lots of messages.  The message queue can only hold 
	//	100 messages at a time.

	BMessage msg(msgClearMessages);
	if (inSync) {
		BMessage reply;
		BMessenger messagewindow(this);
		messagewindow.SendMessage(&msg, &reply);
	}
	else {
		this->PostMessage(&msg);
	}
}


// ---------------------------------------------------------------------------

void
MMessageWindow::DoCopy()
{
	//	Copy all the text in the window to the clipboard.

	String text;
	
	fMessageView->CopyAllText(text);

	// Put text on clipboard
	if (be_clipboard->Lock()) {
		be_clipboard->Clear();
		be_clipboard->Data()->AddData(kTextPlain, B_MIME_TYPE, text, text.GetLength());
		be_clipboard->Commit();
		be_clipboard->Unlock();
	}
}

// ---------------------------------------------------------------------------

bool
MMessageWindow::SavePanelIsRunning()
{
	//	Is it visible?
	return fSavePanel != nil;
}

// ---------------------------------------------------------------------------

void
MMessageWindow::ShowSavePanel()
{
	if (SavePanelIsRunning()) {
		::ShowAndActivate(fSavePanel->Window());	// call the one in Utils.h
	}
	else {
		BMessenger me(this);
		BWindow* panel;

		fSavePanel = new BFilePanel(B_SAVE_PANEL, &me);

		panel = fSavePanel->Window();

		panel->SetTitle("Save A Copy As");
		fSavePanel->SetSaveText(this->Title());

		fSavePanel->Show();
	}
}

// ---------------------------------------------------------------------------

void
MMessageWindow::SaveRequested(entry_ref* 	directory, const char*	name)
{
	// The save panel requested that we save so we need to create the file
	// object and specify it.

	BDirectory dir(directory);					// Specify directory
	BFile file;
	status_t err = dir.CreateFile(name, &file);	// Specify file object and create it on disk
	
	if (err == B_NO_ERROR) {
		BEntry entry;
		err = dir.FindEntry(name, &entry);
		
		if (err == B_NO_ERROR) {
			String text;
			
			fMessageView->CopyAllText(text);
			
			BFile openFile(&entry, B_READ_WRITE);
			
			err = openFile.InitCheck();
			
			// Write the data out
			if (err == B_NO_ERROR) {
				file.Write(text, text.GetLength());
				BNodeInfo mime(&openFile);
				mime.SetType(kIDETextMimeType);
			}
		}
	}
}

// ---------------------------------------------------------------------------

void
MMessageWindow::GetData(BMessage& inOutMessage)
{
	BAutolock lock(this);

	if (inOutMessage.HasData(kFontPrefs, kMWPrefs)) {
		inOutMessage.ReplaceData(kFontPrefs, kMWPrefs, &fFontPrefs, sizeof(fFontPrefs));
	}
}

// ---------------------------------------------------------------------------

void
MMessageWindow::ChangeFontSize(float size)
{
	BAutolock lock(this);

	fFontPrefs.pFontSize = size;
	this->DoSetData();
	::MarkCurrentFontSize(fSizeMenu, fFontPrefs.pFontSize);
}

// ---------------------------------------------------------------------------

const char* kEmptyString = "";

void
MMessageWindow::ChangeFont(const font_family family, const font_style style)
{
	BAutolock lock(this);

	strcpy(fFontPrefs.pFontFamily, family);
	if (style == nil) {
		style = kEmptyString;
	}
	strcpy(fFontPrefs.pFontStyle, style);	
	this->DoSetData();
	::MarkCurrentFont(fFontMenu, fFontPrefs.pFontFamily, fFontPrefs.pFontStyle, kFirstFontMenuItem);
}

// ---------------------------------------------------------------------------

void
MMessageWindow::SetData(BMessage& inOutMessage)
{
	// Extract preferences data of the kind that this window
	// knows about from the BMessage, and set the fields in the view.

	ssize_t length;
	FontPrefs* font;
	MLocker<BLooper> lock(this);

	if (B_NO_ERROR == inOutMessage.FindData(kFontPrefs, kMWPrefs, (const void**) &font, &length)) {
		ASSERT(length == sizeof(FontPrefs));
		fFontPrefs = *font;		
		this->DoSetData();
	}
}

// ---------------------------------------------------------------------------

void
MMessageWindow::DoSetData()
{
	// fFontPrefs contains the new data -
	// save it and adjust our view accordingly

	this->SetPrefs();

	// Resize the height of each cell to reflect the
	// font characteristics
	BFont font;
	
	font.SetFamilyAndStyle(fFontPrefs.pFontFamily, fFontPrefs.pFontStyle);
	font.SetSize(fFontPrefs.pFontSize);
	fMessageView->SetFont(&font);
	fMessageView->AdjustAllRowHeights();	
}

// ---------------------------------------------------------------------------

void
MMessageWindow::GetPrefs()
{
	size_t length;
	BRect frame;

	length = sizeof(fFontPrefs);
	if (B_NO_ERROR == MPreferences::GetPreference(kMessageWindowPrefs, kPrefsType, &fFontPrefs, length)) {
		fFontPrefs.SwapBigToHost();
	}
	else {
		MDefaultPrefs::SetFontDefaults(fFontPrefs);
		if (fFontPrefs.pFontSize < 12.0) {	// The stock size is too small
			fFontPrefs.pFontSize = 12.0;
		}
	}

	length = sizeof(BRect);
	if (B_NO_ERROR == MPreferences::GetPreference(kMessageWindowFrame, B_RECT_TYPE, &frame, length)) {
		SwapRectToBig(frame);
		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), frame.Height());
		ValidateWindowFrame(frame, *this);
	}
}

// ---------------------------------------------------------------------------

void
MMessageWindow::SetPrefs()
{
	size_t				length;
	BRect				frame = Frame();

	length = sizeof(fFontPrefs);
	fFontPrefs.SwapHostToBig();
	(void) MPreferences::SetPreference(kMessageWindowPrefs, kPrefsType, &fFontPrefs, length);
	fFontPrefs.SwapBigToHost();
	length = sizeof(frame);
	SwapRectToBig(frame);
	(void) MPreferences::SetPreference(kMessageWindowFrame, B_RECT_TYPE, &frame, length);
}

// ---------------------------------------------------------------------------

int32
MMessageWindow::Errors()
{
	// Need to lock the window to prevent a race condition
	// with removal of warnings.
	BAutolock lock(this);
	return fMessageView->Errors();
}

// ---------------------------------------------------------------------------

int32
MMessageWindow::Warnings()
{
	// Need to lock the window to prevent a race condition
	// with removal of warnings.
	BAutolock lock(this);
	return fMessageView->Warnings();
}

// ---------------------------------------------------------------------------

int32
MMessageWindow::Infos()
{
	// Need to lock the window to prevent a race condition
	// with removal of warnings.
	BAutolock lock(this);
	return fMessageView->Infos();
}

// ---------------------------------------------------------------------------

void
MMessageWindow::BuildControlGroup(BPoint start,
								  BView* inView,
								  const BBitmap* bitMap,
								  BCheckBox** checkBox,
								  BPictureButton** pictureButton,
								  BStringView** caption)
{
	// Utility helper function 
	// Build a checkbox, picture button and caption
	// Use the bitmap specified for the picture
	// The checkbox is optional.  If the caller
	// doesn't provide the pointer, don't build it
	
	// Check box
	BRect r;
	if (checkBox) {
		r.Set(start.x, start.y, start.x + 15, start.y + 15);
		*checkBox = new BCheckBox(r, "", "", new BMessage(msgCheckBoxChanged));
		inView->AddChild(*checkBox);
		(*checkBox)->SetTarget(this);
		SetGrey(*checkBox, kLtGray);
		start.x += 18;
	}
	
	// Bitmap picture
	r.Set(start.x, start.y, start.x + 16, start.y + 16);
	inView->BeginPicture(new BPicture);
	inView->SetDrawingMode(B_OP_OVER);
	inView->DrawBitmap(bitMap, B_ORIGIN);
	BPicture* picture1 = inView->EndPicture();
	BPicture* picture2 = new BPicture(*picture1);
	
	// Use the two pictures for our button
	*pictureButton = new BPictureButton(r, "", picture1, picture2, new BMessage(msgPictureButtonChanged));
	inView->AddChild(*pictureButton);
	(*pictureButton)->SetTarget(this);
	SetGrey(*pictureButton, kLtGray);
	start.x += 20;

	// Caption for count
	r.Set(start.x, start.y, start.x + 50, start.y + 15);
	*caption = new BStringView(r, "", "0");
	inView->AddChild(*caption);
	SetGrey(*caption, kLtGray);
	(*caption)->SetFont(be_fixed_font);
}

// ---------------------------------------------------------------------------

void
MMessageWindow::RemovePreferences()
{
	(void) MPreferences::RemovePreference(kMessageWindowPrefs, kPrefsType);
	(void) MPreferences::RemovePreference(kMessageWindowFrame, B_RECT_TYPE);
}

// ---------------------------------------------------------------------------

MMessageWindow*
MMessageWindow::GetRecentMessageWindow()
{
	// return the message window most recently activated...
	// (can return nil)
	
	BAutolock listLock(&fgMessageWindowListLock);
	return (MMessageWindow*) fgMessageWindowList.LastItem();
}

// ---------------------------------------------------------------------------

void
MMessageWindow::MessageToAllMessageWindows(BMessage& inMessage)
{
	// Post the message to all the current message windows 
	BAutolock listLock(&fgMessageWindowListLock);
	
	int32 count = fgMessageWindowList.CountItems();
	for (int i = 0; i < count; i++) {
		MMessageWindow* aWindow = (MMessageWindow*) fgMessageWindowList.ItemAt(i);
	 	aWindow->PostMessage(&inMessage);
	}
}

// ---------------------------------------------------------------------------

void
MMessageWindow::MessageToMostRecent(BMessage& inMessage)
{
	MMessageWindow* mostRecent = MMessageWindow::GetRecentMessageWindow();
	if (mostRecent) {
		mostRecent->PostMessage(&inMessage);
	}
}

// ---------------------------------------------------------------------------

MMessageWindow*
MMessageWindow::GetGeneralMessageWindow()
{
	// The general message window is used for general information that isn't
	// a build error/warning or a find result.  It is shared so that multiple
	// messages go into the same window.  However, once it is deleted, it is
	// lost (because it is an information window)

	BAutolock listLock(&fgMessageWindowListLock);
	if (fgGeneralMessageWindow == nil) {
		fgGeneralMessageWindow = new MInformationMessageWindow("Project Messages", "Access Paths, Adding Files, Add-ons");
	}	
	
	return fgGeneralMessageWindow;
}

// ---------------------------------------------------------------------------

int32
MMessageWindow::MessageWindowCount()
{
	BAutolock listLock(&fgMessageWindowListLock);
	return fgMessageWindowList.CountItems();
}
