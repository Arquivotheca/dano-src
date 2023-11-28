//========================================================================
//	MTextWindow.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	IDE source file text window
//	BDS

#include <ctype.h>
#include <math.h>
#include <string.h>
#include "CString.h"
#include <unistd.h>

#include "IDEApp.h"
#include "MTextWindow.h"
#include "MIDETextView.h"
#include "MTextInfoView.h"
#include "MProjectView.h"
#include "MProjectWindow.h"
#include "MDynamicMenuHandler.h"
#include "MGoToLineWindow.h"
#include "MFindWindow.h"
#include "MDefaultPrefs.h"
#include "MTextAddOn.h"
#include "MAlert.h"
#include "MPopupMenu.h"
#include "MKeyFilter.h"
#include "MainMenus.h"
#include "MLocker.h"
#include "ProjectCommands.h"
#include "Utils.h"
#include "IDEConstants.h"
#include "IDEMessages.h"
#include "Auto.h"

#include <CheckBox.h>
#include <NodeMonitor.h>
#include <Roster.h>

BRect			MTextWindow::fSavePanelFrame;
entry_ref		MTextWindow::fLastDirectory;
BLocker			MTextWindow::fSavePanelLock("savelock");
bool			MTextWindow::fRememberSelections;
bool			MTextWindow::fLastDirectoryGood = false;
InfoStructList	MTextWindow::fGoBackList;
MList<edit_add_on *>	MTextWindow::fEditAddOns;


const int32 kSearchMenuIndex = 2;
const int32 kMaxGoBack = 10;
const float kTextLeft = kBorderWidth;
const float kTextTop = 25.0;
const float kTextZoomRight = 640.0;
const float INFO_VIEW_WIDTH = 200.0;
const float kTabHeight = 19.0;

//BRect sNewTextArea(70, 25, 570, 470);
//BRect sOldTextArea(70, 25, 570, 470);
BRect sNewTextArea(70, 25, 640, 470);

const char* kSyntaxItemLabel = "Syntax Styling";
const char* kSizeItemLabel = "Size";
const char* kFontItemLabel = "Font";

// ---------------------------------------------------------------------------
//		MTextWindow
// ---------------------------------------------------------------------------
//	Constructor for new window

MTextWindow::MTextWindow(
	const char * title) 
	: BWindow(sNewTextArea, title, B_DOCUMENT_WINDOW, 0)
{
	fFile = NULL;

	InitMTextWindow(title);
	MoveBy(10.0, 10.0);
	
	GetDefaultPrefs();
	UpdateTextView();
}

// ---------------------------------------------------------------------------
//		MTextWindow
// ---------------------------------------------------------------------------
//	Constructor for existing file

MTextWindow::MTextWindow(
	BEntry *		file,
	const char *	inTitle) 
	: BWindow(sNewTextArea, inTitle, B_DOCUMENT_WINDOW, 0)
{
	fFile = file;				//	Assume ownership

	InitMTextWindow(inTitle);
	ReadTextFile();
	fTextView->SetDirty(false);
	
	GetPrefs();
	UpdateTextView();

	fChangeFontInfo = false;

	SetSaveDirectory(*fFile);
}

// ---------------------------------------------------------------------------
//		MTextWindow
// ---------------------------------------------------------------------------
//	Constructor for existing file

MTextWindow::MTextWindow(
	const entry_ref& inRef) 
	: BWindow(sNewTextArea, B_EMPTY_STRING, B_DOCUMENT_WINDOW, 0)
{
	fFile = new BEntry(&inRef);
	char		title[B_FILE_NAME_LENGTH] = { '\0' };

	status_t	err = fFile->GetName(title);

	InitMTextWindow(title);
	ReadTextFile();
	fTextView->SetDirty(false);
	
	GetPrefs();
	UpdateTextView();

	fChangeFontInfo = false;

	SetSaveDirectory(*fFile);
}

// ---------------------------------------------------------------------------
//		MTextWindow
// ---------------------------------------------------------------------------
//	Constructor for new window when the text to go into the window already
//	exists.  This is for preprocess results and dissassembly results.

MTextWindow::MTextWindow(
	const char * 	title,
	const char * 	inText,
	size_t			inTextLength) :
	BWindow(sNewTextArea, title, B_DOCUMENT_WINDOW, 0)
{
	fFile = NULL;
	InitMTextWindow(title);
	MoveBy(10.0, 10.0);
	
	GetDefaultPrefs();
	UpdateTextView();

	fTextView->SetText(inText, inTextLength);
	fTextView->Select(0, 0);
	fTextView->SetDirty(false);
	UpdateIfNeeded();
}

// ---------------------------------------------------------------------------
//		~MTextWindow
// ---------------------------------------------------------------------------
//	Destructor

MTextWindow::~MTextWindow()
{
	SetPrefs();
	CloseSavePanel();

	if (fGoToLineWindow && fGoToLineWindow->Lock())
		fGoToLineWindow->Quit();

	// tell the dynamic handler that this window is going away
	// (notice that the MTextWindow is still fully constructed
	// at this point -- don't delete members until after this call)
	
	MDynamicMenuHandler::TextWindowClosed(this);
	
	// now we can delete data members
	delete fFile;
	if(fMonitorFile)
		stop_watching(this);
}

// ---------------------------------------------------------------------------
//		InitMTextWindow
// ---------------------------------------------------------------------------
//	Private init function.

void
MTextWindow::InitMTextWindow(
	const char *	inTitle)
{
	fProjectIsOpen = false;
	fProject = nil;
	fGoToLineWindow = nil;
	fSavingACopyAs = false;
	fNativeTextFormat = kNewLineFormat;
	fUsingPermanentFontPrefs = true;
	fSavePanel = nil;
	fSavePrefs = true;
	fSelStart = fSelEnd = 0;

	if (fFile != nil)
	{
		fWriteableState = FileWriteableState(*fFile);
		fIsWritable = FileIsWriteable(fWriteableState);
	}
	else
	{
		fWriteableState = kUnsavedFile;
		fIsWritable = true;	
	}
	
	fMonitorFile = nil;
	StartMonitor();
	
	BuildWindow(GetSuffixType(inTitle));

	// Turn off these menu items if file is read/only
	if (! fIsWritable)
	{
		WritableStateChanged();
	}

	SetTitle(inTitle);
	MDynamicMenuHandler::TextWindowOpened(this);
}

void MTextWindow::StartMonitor()
{
	if(fMonitorFile)
		stop_watching(this);

	fMonitorFile = fFile;
	if(fMonitorFile)
	{
		node_ref nref; 
		fMonitorFile->GetNodeRef(&nref);
		watch_node(&nref, B_WATCH_STAT, this);
	}
}

// ---------------------------------------------------------------------------
//		WritableStateChanged
// ---------------------------------------------------------------------------

void
MTextWindow::WritableStateChanged()
{
	EnableItem(fMenuBar, cmd_Cut, fIsWritable);
	EnableItem(fMenuBar, cmd_Paste, fIsWritable);
	EnableItem(fMenuBar, cmd_Clear, fIsWritable);
	EnableItem(fMenuBar, cmd_ShiftLeft, fIsWritable);
	EnableItem(fMenuBar, cmd_ShiftRight, fIsWritable);
//	EnableItem(fMenuBar, cmd_Balance, fIsWritable);
	
	fTextView->MakeEditable(fIsWritable);
	fTextView->SetDirty(false);
	fTextView->ClearAllUndo();
}

// ---------------------------------------------------------------------------
//		ChangeWritableState
// ---------------------------------------------------------------------------
//	Change the writable state from its current state.

void
MTextWindow::ChangeWritableState()
{
	status_t		err;
	struct stat		info;

	err = fFile->GetStat(&info);

	switch (fWriteableState)
	{
		case kIsWritable:
			err = fFile->SetPermissions(info.st_mode & ~(S_IWUSR | S_IWGRP | S_IWOTH));
			break;

		case kPermissionLocked:
			err = fFile->SetPermissions(info.st_mode | (S_IWUSR | S_IWGRP | S_IWOTH));
			break;

		default:
			ASSERT(false);
			break;
	}

	fWriteableState = FileWriteableState(*fFile);		
	fIsWritable = FileIsWriteable(fWriteableState);
	WritableStateChanged();
}

// ---------------------------------------------------------------------------
//		SetSaveDirectory
// ---------------------------------------------------------------------------
//	Set the directory that the save panel will open in.

void
MTextWindow::SetSaveDirectory(
	BEntry&	inFile)
{
	if (! fLastDirectoryGood)
	{
		MLocker<BLocker>	lock(fSavePanelLock);
		BDirectory			dir;
		BEntry				dirEntry;
		if (B_NO_ERROR == inFile.GetParent(&dir) &&
			B_NO_ERROR == dir.GetEntry(&dirEntry) &&
			B_NO_ERROR == dirEntry.GetRef(&fLastDirectory))
		{
			fLastDirectoryGood = true;
		}
	}
}

// ---------------------------------------------------------------------------
//		MessageReceived
// ---------------------------------------------------------------------------

void
MTextWindow::MessageReceived(
	BMessage * inMessage)
{
	switch (inMessage->what)
	{
		case B_NODE_MONITOR:
		{
			// We get here on saves also, so don't blindly change call
			// WritableStateChanged since that clears all our undo information
			FileWriteAble newState = FileWriteableState(*fFile);
			if (newState != fWriteableState) {
				fWriteableState = newState;
				fIsWritable = FileIsWriteable(newState);
				WritableStateChanged();
				fInfoView->Invalidate();
			}
			break;
		}
		
		// File menu
		case cmd_Save:
			DoSave();
			break;

		case cmd_SaveAs:
			DoSaveAs();
			break;

		case cmd_SaveCopyAs:
			DoSaveACopyAs();
			break;

		case cmd_Close:
			AttemptClose();
			break;

		case cmd_Revert:
			DoRevert();
			break;

		case cmd_Print:
			fTextView->Print();
			break;
			
		case cmd_PageSetup:
		{
			BPrintJob	job("printjob");
			IDEApp::BeAPP().PageSetup(job);
		}
			break;

		case cmd_OpenSelection:		// from the menu
		case msgDoOpenSelection:	// from the application
			OpenSelection();
			break;

	// Edit menu
	// This is a hack to get around the fact that the keybindingmanager
	// allows keybindings to be used only once.  It changes the command-Z
	// into cmd_Undo always.  This will have to be changed for multiple-undo
		case cmd_Undo:
		case cmd_Redo:
		{
			if (fTextView->UsesMultipleUndo())
			{
				if (inMessage->what == cmd_Undo)
					fTextView->Undo();
				else
					fTextView->Redo();			
			}
			else
			{
				fTextView->AdjustUndoMenuItems(*fUndoItem, *fRedoItem);
	
				if (fUndoItem->Command() == cmd_Undo)
					fTextView->Undo();
				else
					fTextView->Redo();
			}
		}
			break;

		case cmd_Clear:
			fTextView->Clear();
			break;

		case cmd_SelectAll:
			fTextView->SelectAll();
			break;

		case cmd_ShiftRight:
			fTextView->ShiftRight();
			break;

		case cmd_ShiftLeft:
			fTextView->ShiftLeft();
			break;

		case cmd_Balance:
			fTextView->DocBalance();
			break;

		// Project menu
		case cmd_AddWindow:
			AddToProject();
			break;

		case cmd_Make:
		case cmd_BringUpToDate:
		case CMD_LINK:
			if (fProject)
				fProject->PostMessage(inMessage);
			else
				be_app_messenger.SendMessage(inMessage);
			break;

		case cmd_Compile:
			DoCompile();
			break;

		case cmd_Precompile:
			DoSendToProject(msgPreCompileOne);
			break;

		case cmd_Preprocess:
			DoSendToProject(msgPreProcessOne);
			break;

		case cmd_CheckSyntax:
			DoSendToProject(msgCheckSyntaxOne);
			break;

		case cmd_Disassemble:
			DoSendToProject(msgDisassembleOne);
			break;

		// Search menu
		case cmd_Find:
			MFindWindow::GetFindWindow().PostMessage(msgFindFromTextWindow);
			break;

		case cmd_FindNext:
		case cmd_FindPrevious:
			DoFindNext(inMessage->what == cmd_FindNext);
			break;

		case cmd_FindInNextFile:
		case cmd_FindInPrevFile:
			MFindWindow::GetFindWindow().PostMessage(inMessage);
			break;

		case cmd_EnterFindString:
		case cmd_EnterReplaceString:
			DoEnterFindString(inMessage->what == cmd_EnterFindString);
			break;

		case cmd_FindSelection:
		case cmd_FindPrevSelection:
			DoFindSelection(inMessage->what == cmd_FindSelection);
			break;

		case cmd_Replace:
			DoReplace();
			break;

		case cmd_ReplaceAndFindNext:
		case cmd_ReplaceAndFindPrev:
			DoReplaceAndFind(inMessage->what == cmd_ReplaceAndFindNext);
			break;

		case cmd_ReplaceAll:
			DoReplaceAll();
			break;

		case cmd_GotoLine:
			OpenGoToLineWindow();
			break;

		case cmd_FindDefinition:
			FindDefinition();
			break;

		case cmd_FindDocumentation:
			this->FindDocumentation();
			break;

		case cmd_GoBack:
			GoBack();
			break;

		// Window menu
		case msgWindowMenuClicked:
		case msgOpenRecent:
		case cmd_Stack:
		case cmd_Tile:
		case cmd_TileVertical:
			be_app_messenger.SendMessage(inMessage);
			break;

		case cmd_ShowProjectWindow:
		case cmd_ErrorMessageWindow:
			if (fProject) {
				fProject->PostMessage(inMessage);
			}
			break;

		case cmd_Cancel:
			if (fProject) {
				fProject->PostMessage(inMessage);
			}
			else {
				be_app_messenger.SendMessage(inMessage);
			}
			break;

		// Special command keys
		case cmd_AndyFeature:
			DoAndyFeature();
			break;

		// Application
		case msgProjectOpened:
			HandleProjectOpened(*inMessage);
			break;

		case msgProjectClosed:
			HandleProjectClosed(*inMessage);
			break;

		case msgBalanceWhileTyping:
			fTextView->BalanceWhileTyping(*inMessage);
			break;

		case msgCustomQuitRequested:
		{
			ASSERT(inMessage->IsSourceWaiting());
			bool			result = QuitRequested();
			BMessage		reply;

			reply.AddBool(kOKToQuit, result);
			inMessage->SendReply(&reply);
		}
			break;

		case msgUpdateMenus:
			UpdateTextMenus(fMenuBar);
			break;

		case msgColorsChanged:
			SetData(*inMessage, false);
			break;

		// Go To Line window and from IDEApp
		case msgGoToLine:
			DoGoToLine(*inMessage);
			break;

		case msgGoToWindowClosed:
			if (fGoToLineWindow->Lock())
				fGoToLineWindow->Quit();
			fGoToLineWindow = nil;
			break;

		// Set EOLType popup menu
		case msgSetFileType:
			if (fIsWritable && 
				B_NO_ERROR == inMessage->FindInt32(kEOLType, (int32*) &fNativeTextFormat))
			{
				fTextView->SetDirty(true);
			}
			break;

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

		case msgSetSyntaxColoring:
			ToggleSyntaxColoring();
			break;

		// Header popup menu
		case msgOpenSourceFile:
			be_app_messenger.SendMessage(inMessage);	// The app will open any windows
			break;

		case msgTouchFile:
			ASSERT(fProject && fProjectIsOpen);
			fProject->PostMessage(inMessage);	// The project will do the touching
			break;

		// An Add-on
		case CMD_EDIT_ADD_ON:
		{
			status_t	 err = PerformEditAddOn(inMessage);
			if (err == B_NO_ERROR)
				fTextView->SetDirty();
			break;
		}
		
		// The save panel
		case B_SAVE_REQUESTED:
		{
			entry_ref dir;
			inMessage->FindRef("directory", &dir);

			const char *name = NULL;
			inMessage->FindString("name", &name);

			SaveRequested(&dir, name);
			CloseSavePanel();
			break;
		}

		default:
			if (! SpecialMessageReceived(*inMessage, this))
				BWindow::MessageReceived(inMessage);
			break;
	}
}

// ---------------------------------------------------------------------------
//		MenusBeginning
// ---------------------------------------------------------------------------

void
MTextWindow::ScreenChanged(
	BRect 			/*screen_size*/, 
	color_space 	depth)
{
	fTextView->SetColorSpace(depth);
}

// ---------------------------------------------------------------------------
//		MenusBeginning
// ---------------------------------------------------------------------------

void
MTextWindow::MenusBeginning()
{
	// Set up the menu items to match the current modifier keys, if any
	UpdateMenuItemsForModifierKeys(fMenuBar);

	// File menu
	EnableItem(fMenuBar, cmd_Save, fTextView->IsDirty());
	EnableItem(fMenuBar, cmd_Revert, fFile != nil && fTextView->IsDirty());

	// Edit menu
	// Do the Undo/Redo item
	ASSERT(fUndoItem);
	ASSERT(fRedoItem);
	fTextView->AdjustUndoMenuItems(*fUndoItem, *fRedoItem);
	
	// These items are enabled only if there is a selection
	const bool		hasSelection = fTextView->HasSelection();

	EnableItem(fMenuBar, cmd_Copy, hasSelection);

	if (fIsWritable)
	{
		EnableItem(fMenuBar, cmd_Cut, hasSelection);
		EnableItem(fMenuBar, cmd_Clear, hasSelection);
	
		// Is there any text on the clipboard?
		EnableItem(fMenuBar, cmd_Paste, fTextView->CanPaste());
	}

	// Find menu
	FindMenuWillShow();

	// Project menu
	const bool		attachedToProject = fProject != nil;

	EnableItem(fMenuBar, cmd_Compile, attachedToProject);
	EnableItem(fMenuBar, cmd_Make, fProjectIsOpen);
	EnableItem(fMenuBar, cmd_BringUpToDate, fProjectIsOpen);
	EnableItem(fMenuBar, CMD_LINK, fProjectIsOpen);
	EnableItem(fMenuBar, cmd_Disassemble, attachedToProject);
	EnableItem(fMenuBar, cmd_Precompile, fFile != nil && fProjectIsOpen);
	EnableItem(fMenuBar, cmd_Preprocess, fFile != nil && fProjectIsOpen);
	EnableItem(fMenuBar, cmd_CheckSyntax, fFile != nil && fProjectIsOpen);

	EnableItem(fMenuBar, cmd_AddWindow, !attachedToProject && fProjectIsOpen && fFile != nil);

	// Upate the window/projects list menus
	MDynamicMenuHandler::UpdateDynamicMenus(*fMenuBar, fProject);
}

// ---------------------------------------------------------------------------
//		FindMenuWillShow
// ---------------------------------------------------------------------------

void
MTextWindow::FindMenuWillShow()
{
	MFindWindow&	findWindow = MFindWindow::GetFindWindow();
	const bool		hasSelection = fTextView->HasSelection();
	const bool		hasFindString = findWindow.HasFindString();
	const bool		writeable = fIsWritable;
	const BMenu*	searchMenu = fMenuBar->SubmenuAt(kSearchMenuIndex);
	
	EnableItem(fMenuBar, cmd_FindPrevious, hasFindString);
	EnableItem(fMenuBar, cmd_FindInPrevFile, findWindow.CanFindInNextFile());
	EnableItem(fMenuBar, cmd_EnterReplaceString, hasSelection);
	EnableItem(fMenuBar, cmd_FindPrevSelection, hasSelection);
	EnableItem(fMenuBar, cmd_ReplaceAndFindPrev, hasSelection && hasFindString && writeable);
	EnableItem(fMenuBar, cmd_GoBack, CanGoBack());

	EnableItem(fMenuBar, cmd_FindNext, hasFindString);
	EnableItem(fMenuBar, cmd_FindInNextFile, findWindow.CanFindInNextFile());
	EnableItem(fMenuBar, cmd_EnterFindString, hasSelection);
	EnableItem(fMenuBar, cmd_FindSelection, hasSelection);
	EnableItem(fMenuBar, cmd_ReplaceAndFindNext, hasSelection && hasFindString && writeable);
	EnableItem(fMenuBar, cmd_FindDefinition, fProject != nil);

	EnableItem(fMenuBar, cmd_Replace, hasSelection && writeable);
	EnableItem(fMenuBar, cmd_ReplaceAll, hasFindString && writeable && findWindow.CanReplaceAll());
}

// ---------------------------------------------------------------------------
//		BuildWindow
// ---------------------------------------------------------------------------
//	Build the window.

void
MTextWindow::BuildWindow(
	SuffixType	inSuffix)
{
	BRect 			bounds = Bounds();
	BRect 			area = bounds;
	BScrollBar *	vertScrollBar;
	BScrollBar *	horizScrollBar;
	float			menuBarHeight = 26.0;
	
	// Menu bar
	area.bottom = area.top + menuBarHeight;
	area.right++;
	fMenuBar = MakeTextMenus(area);
	AddChild(fMenuBar);
	menuBarHeight = fMenuBar->Bounds().Height();
	

	// Add in the view/font menu
	// syntax styling, font + size
	fViewMenu = new BMenu("View");
	
	// syntax styling
	fViewMenu->AddItem(new BMenuItem(kSyntaxItemLabel, new BMessage(msgSetSyntaxColoring)));
	fViewMenu->AddSeparatorItem();
	
	fSizeMenu = new BMenu(kSizeItemLabel);
	fFontMenu = new BMenu(kFontItemLabel);

	::FillFontSizeMenu(fSizeMenu);
	fViewMenu->AddItem(new BMenuItem(fSizeMenu));
	::FillFontFamilyMenu(fFontMenu);
	fViewMenu->AddItem(new BMenuItem(fFontMenu));
	fViewMenu->SetTargetForItems(this);
	
	fMenuBar->AddItem(new BMenuItem(fViewMenu));

	BMenu* AddOnsMenu = BuildEditAddOnMenu("Add-ons");
	if (AddOnsMenu) {
		fMenuBar->AddItem(AddOnsMenu);
	}
	
	// Save the undo menu item
	fUndoItem = fMenuBar->FindItem(cmd_Undo);
	fRedoItem = fMenuBar->FindItem(cmd_Redo);
	ASSERT(fUndoItem);
	ASSERT(fRedoItem);

	// Save window menu
	BMenuItem* item = fMenuBar->FindItem(cmd_ErrorMessageWindow);
	ASSERT(item);
	fWindowMenu = item->Menu();
	ASSERT(fWindowMenu);

	// Text view
	area = bounds;
	area.top += menuBarHeight + 1.0;
	area.right -= B_V_SCROLL_BAR_WIDTH;
	area.bottom -= B_H_SCROLL_BAR_HEIGHT;
	fTextView = new MIDETextView(area, inSuffix);

	// Scroll bars
	area.left = area.right + 1.0;
	area.right = bounds.right + 1.0;
	area.bottom++;
	area.top--;
	vertScrollBar = new BScrollBar(area, "vert", fTextView, 0, 0, B_VERTICAL);
	AddChild(vertScrollBar);
	vertScrollBar->SetSteps(14, vertScrollBar->Bounds().Height() - 14);	// should use real lineheight

	area = bounds;
	area.top = (area.bottom - B_H_SCROLL_BAR_HEIGHT) + 1.0;
	area.right -= B_V_SCROLL_BAR_WIDTH - 1;
	area.left += INFO_VIEW_WIDTH;
	area.bottom++;
	horizScrollBar = new BScrollBar(area, "horiz", fTextView, 0, 0, B_HORIZONTAL);
	AddChild(horizScrollBar);
	horizScrollBar->SetSteps(10, 50);
	horizScrollBar->SetRange(0, 1000);

	AddChild(fTextView);

	// Info view
	area.right = area.left - 1;
	area.left = 0;
	fInfoView = new MTextInfoView(area, *this, *fTextView);
	AddChild(fInfoView);
	// "Line x" will be in normal font rather than bold
	// fInfoView->SetFont(be_bold_font);
	fInfoView->SetScroller(vertScrollBar);

	// Target the edit commands to the textview
	const uint32 cmdsToLink[] = {
		cmd_Cut, cmd_Copy, cmd_Paste, 0
	};

	for (int ix = 0; cmdsToLink[ix]; ix++)
	{
		BMenuItem* theItem = fMenuBar->FindItem(cmdsToLink[ix]);
		ASSERT(theItem);
		if (theItem)
			theItem->SetTarget(fTextView);
	}

	AddCommonFilter(new MWindowMenuFilter);
	AddCommonFilter(new MKeyFilter(this, kBindingEditor));

	fTextView->MakeFocus();
	
	const float		klimit = INFO_VIEW_WIDTH + B_V_SCROLL_BAR_WIDTH;
	SetSizeLimits(klimit, 2048.0, klimit, 2048.0);
	SetZoomLimits(kTextZoomRight, 1000.0);
}

// ---------------------------------------------------------------------------
//		DispatchMessage
// ---------------------------------------------------------------------------
//	This is a hack to override the BWindow desire to trap all key downs
//	that are command keydowns.  This method directly dispatches a keydown
//	to the focus view, which in this case is the TextView.  The textview
//	will handle command arrow keydowns properly.
//	This could probably also be accomplished by calling SetShortcut for
//	command-arrow keys and then handling the message in messageReceived.
    
void
MTextWindow::DispatchMessage(
	BMessage 	*message, 
	BHandler 	*receiver)
{
	bool		dispatch = true;

	switch (message->what)
	{
		case B_MOUSE_DOWN:
			if (message->HasInt32("modifiers") && message->HasInt32("clicks"))
			{
				// Option double click??
				uint32		modifiers = message->FindInt32("modifiers");
				if (receiver == fTextView && 
					(modifiers & (B_OPTION_KEY | B_CONTROL_KEY | B_COMMAND_KEY)) != 0 && 
					2 == message->FindInt32("clicks"))
				{
//					MLocker<BLooper>		lock(this);	
					BPoint		where = message->FindPoint("be:view_where");								
					fTextView->MouseDown(where);
					// option/control + double click = find definition
					// command + double click = find documentation
					if ((modifiers & (B_OPTION_KEY | B_CONTROL_KEY)) != 0) {				
						this->FindDefinition();
					}
					else {
						this->FindDocumentation();
					}
					dispatch = false;
				}
			}
			break;
	}

	// BWindow ignores the cmd-arrowkeys but if we don't send it the message
	// it hilites the file menu when the cmd key comes up
	if (dispatch)
		BWindow::DispatchMessage(message, receiver);
}

// ---------------------------------------------------------------------------
//		Zoom
// ---------------------------------------------------------------------------
//	Override the BWindow version so we can move the window to a position on the
//	left edge of the screen.

void
MTextWindow::Zoom(
	BPoint			/*inPosition*/,
	float 			/*inWidth*/,
	float 			/*inHeight*/)
{
	// Calculate the system state rect
	BRect			windFrame = Frame();
	BScreen			screen(this);
	BRect			sysFrame(kTextLeft, kTextTop, kTextZoomRight, screen.Frame().bottom - kBorderWidth);

	// If the window is in user state or the frame has changed since the last
	// zoom then go to system state
	if (fUserState || sysFrame != windFrame)		// going to system state
	{
		fUserRect = windFrame;

		MoveTo(sysFrame.LeftTop());
		ResizeTo(sysFrame.Width(), sysFrame.Height());	
		fUserState = false;
	}
	else
	{
		// Restore the user state frame
		MoveTo(fUserRect.LeftTop());
		ResizeTo(fUserRect.Width(), fUserRect.Height());	
		fUserState = true;
	}
}

// ---------------------------------------------------------------------------
//		FindDefinition
// ---------------------------------------------------------------------------

void
MTextWindow::FindDefinition()
{
	// Ask the project to do something about it
	if (fProject)
	{
		int32			selStart;
		int32			selEnd;
		
		fTextView->GetSelection(&selStart, &selEnd);

		String			text(fTextView->Text() + selStart, selEnd - selStart);

		BMessage		msg(msgFindDefinition);
		msg.AddString(kText, text);
		msg.AddPointer(kTextWindow, this);
		
		fProject->PostMessage(&msg);
	}
	else
		beep();
}

// ---------------------------------------------------------------------------

void
MTextWindow::FindDocumentation()
{
	// Get together our selection, and then ask the application
	// to do the work

	BMessage msg(msgFindDocumentation);
	int32 selStart;
	int32 selEnd;
	fTextView->GetSelection(&selStart, &selEnd);
	if (selEnd != selStart) {
		String text(fTextView->Text() + selStart, selEnd - selStart);
		msg.AddString(kText, text);
	}
	be_app_messenger.SendMessage(&msg);
}

// ---------------------------------------------------------------------------
//		GoBack
// ---------------------------------------------------------------------------
//	Pop the top entry off the GoBack list and tell the app to display it.

void
MTextWindow::GoBack()
{
	InfoStruct*			info;
	MLocker<BLocker>	lock(fSavePanelLock);

	if (fGoBackList.GetNthItem(info, fGoBackList.CountItems() - 1))
	{
		BMessage	msg(msgOpenSourceFile);

		msg.AddRef("refs", info->iRef);
		msg.AddData(kTokenIdentifier, kTokenIDType, &info->iToken, sizeof(info->iToken));
		be_app_messenger.SendMessage(&msg);
		
		fGoBackList.RemoveItem(info);
		delete info;
	}
}

// ---------------------------------------------------------------------------
//		PutGoBack
// ---------------------------------------------------------------------------
//	Save an infostruct for this window in the GoBack list.  The user can
//	then go back to this selection later.

void
MTextWindow::PutGoBack()
{
	MLocker<BLocker>	lock(fSavePanelLock);
	Lock();

	InfoStruct*		info;
	if (fGoBackList.CountItems() >= kMaxGoBack)
	{
		int32		num = 0;
		info = (InfoStruct*) fGoBackList.RemoveItemAt(num);	
		delete info;	
	}

	// Build a new InfoStruct and add it to the GoBackList
	int32			selStart;
	int32			selEnd;
	int32			size;
	int32			line = fTextView->CurrentLine();

	fTextView->GetSelection(&selStart, &selEnd);

	info = new InfoStruct;
	info->iTextOnly = false;
	info->iRef = new entry_ref;
	GetRef(*info->iRef);

	info->iLineNumber = line;
	strcpy(info->iFileName, Title());
	
	const char *	text = Text();
	for (size = 0; size < kLineTextLength; size++) 
	{
		if (*(text + selStart + size) == EOL_CHAR) 
			break;
		info->iLineText[size] = *(text + selStart + size);
	}
	info->iLineText[size] = 0;

	// Build the token id struct
	info->iToken.eLineNumber = line;
	info->iToken.eOffset = selStart;
	info->iToken.eLength = selEnd - selStart;
	info->iToken.eSyncLength = min(31L, size);
	info->iToken.eSyncOffset = 0;
	memcpy(info->iToken.eSync, &text[selStart], info->iToken.eSyncLength);
	info->iToken.eSync[info->iToken.eSyncLength] = 0;
	info->iToken.eIsFunction = false;

	fGoBackList.AddItem(info);
	Unlock();
}

// ---------------------------------------------------------------------------
//		CanGoBack
// ---------------------------------------------------------------------------

bool
MTextWindow::CanGoBack()
{
	MLocker<BLocker>	lock(fSavePanelLock);
	
	return fGoBackList.CountItems() > 0;
}

// ---------------------------------------------------------------------------
//		SetTextProperties
// ---------------------------------------------------------------------------
//	Set the text properties for the text view.  This will eventually be
//	in a resource or something.

void
MTextWindow::SetTextProperties()
{
	ASSERT(fTextView);
	
	fTextView->SetFont(be_fixed_font);
	fTextView->SetFontSize(12.0);
	fTextView->SetAutoindent(true);
}

// ---------------------------------------------------------------------------
//		SavePanelIsRunning
// ---------------------------------------------------------------------------
//	Is it visible?

bool
MTextWindow::SavePanelIsRunning()
{
	return fSavePanel != nil;
}

// ---------------------------------------------------------------------------
//		CloseSavePanel
// ---------------------------------------------------------------------------
//	Close it and get all its info.

void
MTextWindow::CloseSavePanel()
{
	if (fSavePanel != nil)
	{
		MLocker<BLocker>	lock(fSavePanelLock);

		fSavePanel->GetPanelDirectory(&fLastDirectory);
		fSavePanelFrame = fSavePanel->Window()->Frame();
		fLastDirectoryGood = true;

		delete fSavePanel;
		fSavePanel = nil;
	}
}

// ---------------------------------------------------------------------------
//		ShowSavePanel
// ---------------------------------------------------------------------------

const float kCheckBoxOffset = 15.0;
const float kCheckBoxWidth = 100.0;
const char* kAddProjectViewName = "AddProject view";

void
MTextWindow::ShowSavePanel(
	const char *	inTitle,
	const char *	inSaveText)
{
	if (SavePanelIsRunning())
	{
		CloseSavePanel();
	}

	BMessenger		me(this);
	BWindow*		panel;

	MLocker<BLocker>	lock(fSavePanelLock);
	// Restore the frame and directory
	if (fLastDirectoryGood)
	{
		fSavePanel = new BFilePanel(B_SAVE_PANEL, &me, &fLastDirectory);
	}
	else
		fSavePanel = new BFilePanel(B_SAVE_PANEL, &me);

	panel = fSavePanel->Window();
	
	if (inTitle != nil)
		panel->SetTitle(inTitle);
	if (inSaveText != nil)
		fSavePanel->SetSaveText(inSaveText);

	if (fLastDirectoryGood && fSavePanelFrame.IsValid())
	{
		panel->MoveTo(fSavePanelFrame.LeftTop());
		panel->ResizeTo(fSavePanelFrame.Width(), fSavePanelFrame.Height());
	}

	// if the file we are saving isn't part of a project
	// (or we are doing a save a copy as so we know it isn't part of the project)
	// give the user a chance to do it here with a checkbox in the save file panel...
	if ((fProject == nil || fSavingACopyAs == true) && fProjectIsOpen && panel->Lock()) {
		BView* background = panel->ChildAt(0);
		BView* textView = panel->FindView("text view");
		if (textView) {
		BRect textFame = textView->Frame();
		BRect r;
		r.Set(textFame.right + kCheckBoxOffset, 
			  textFame.top, 
			  textFame.right + kCheckBoxOffset + kCheckBoxWidth, 
			  textFame.bottom);
		BCheckBox* checkBox = new BCheckBox(r, kAddProjectViewName, "Add to Project", 
											NULL, B_FOLLOW_LEFT + B_FOLLOW_BOTTOM); 
		background->AddChild(checkBox);
		checkBox->SetViewColor(background->ViewColor());
		
		// With the added check box, the file panel has a new minimum width
		// Also, make sure our current size is greater than our new minimum
		float min_h;
		float max_h;
		float min_v;
		float max_v;
		panel->GetSizeLimits(&min_h, &max_h, &min_v, &max_v);
		min_h += kCheckBoxWidth;
		panel->SetSizeLimits(min_h, max_h, min_v, max_v);
		
		BRect currentBounds = panel->Bounds();
		if (currentBounds.Width() < min_h) {
			panel->ResizeTo(min_h, currentBounds.Height());
		}
		}
		else {
			fprintf(stderr, "HEY! We didn't get the text view!\n");
		}
		panel->Unlock();
	}
	
	fSavePanel->Show();
}

// ---------------------------------------------------------------------------

bool
MTextWindow::SavePanelAddProjectChecked()
{
	bool isChecked = false;
	if (fSavePanel) {
		BWindow* panel = fSavePanel->Window();
		BCheckBox* checkBox = (BCheckBox*) panel->FindView(kAddProjectViewName);
		if (checkBox) {
			isChecked = checkBox->Value() == 1;
		}
	}
	
	return isChecked;
}

// ---------------------------------------------------------------------------

void
MTextWindow::AddEntryToProject(const BEntry& entry)
{
	// Given the BEntry - add it to the project
	// (called from a "Save a copy as" when we don't
	// have the text window open)
	entry_ref ref;
	if (entry.GetRef(&ref) == B_OK) {
		BMessage msg(msgAddFiles);
		msg.AddRef("refs", &ref);
		be_app_messenger.SendMessage(&msg);
	}
}

// ---------------------------------------------------------------------------
//		QuitRequested
// ---------------------------------------------------------------------------
//	Determine if it's ok to close.

bool
MTextWindow::QuitRequested()
{
	bool		OKToClose = true;

	if (SavePanelIsRunning())
	{
		BWindow*	w = fSavePanel->Window();
		if (w->Lock())
		{
			if (w->IsHidden())
				CloseSavePanel();
			else
				w->Unlock();
		}
	}

	if (SavePanelIsRunning())
	{
		// If the save panel has already been opened for this window
		// just bring it to the front and prevent the window from closing
		Activate(true);
		fSavePanel->Window()->Activate();
		OKToClose = false;
	}

	if (OKToClose) {
		OKToClose = AskToClose();
	}
	
	return OKToClose;
}

// ---------------------------------------------------------------------------
//		AttemptClose
// ---------------------------------------------------------------------------

void
MTextWindow::AttemptClose()
{
	if (!fTextView->IsDirty() || AskToClose())
	{
		Quit();
	}
}

// ---------------------------------------------------------------------------
//		AskToClose
// ---------------------------------------------------------------------------
//	return true if it's ok to close.

bool
MTextWindow::AskToClose()
{
	bool		result = true;

	if (fTextView->IsDirty())
	{
		Activate();			// Bring this window to the front
		UpdateIfNeeded();	// Redraw it if needed

		String			text = "Save changes to the document "B_UTF8_OPEN_QUOTE;
		
		text += Title();
		text += B_UTF8_CLOSE_QUOTE"?";

		MSaveFileAlert alert(text, "Save", "Don't Save", "Cancel");
		alert.SetShortcut(MSaveFileAlert::kDontSaveButton, 'd');

		switch ( alert.Go() )
		{
			case MSaveFileAlert::kSaveButton:
				result = DoSave();
				break;
	
			case MSaveFileAlert::kDontSaveButton:
				fSavePrefs = false;
				break;

			case MSaveFileAlert::kCancelButton:
				result = false;
				break;
		}
	}

	return result;
}

// ---------------------------------------------------------------------------
//		DoRevert
// ---------------------------------------------------------------------------

void
MTextWindow::DoRevert()
{
	String			text = "Revert to the last saved version of "B_UTF8_OPEN_QUOTE;
	
	text += Title();
	text += B_UTF8_CLOSE_QUOTE"?";

	MAlert			alert(text, "Revert", "Don't Revert");

	if (kOKButton == alert.Go())
	{
		ReadTextFile();
		fTextView->ParseText();
		fTextView->SetDirty(false);
		fTextView->ClearAllUndo();
	}
}

// ---------------------------------------------------------------------------
//		DoSave
// ---------------------------------------------------------------------------

bool
MTextWindow::DoSave(
	bool	inSync)
{
	bool		result = true;

	if (fTextView->IsDirty())
	{
		if (!fFile)
			result = DoSaveAs();
		if (result)
		{
			result = (B_NO_ERROR == WriteDataToFile(inSync));
			if (result)
				SetPrefs();
		}
	}
	StartMonitor();
	return result;
}

// ---------------------------------------------------------------------------
//		WriteDataToFile
// ---------------------------------------------------------------------------

status_t
MTextWindow::WriteDataToFile(
	bool	inSync)
{
	ASSERT(fIsWritable);	// would  be ok for saveas and saveacopyas

	int32 length = fTextView->TextLength();
	char* buffer = (char*) fTextView->Text();
	BEntry tempEntry;
	auto_Entry autoE(&tempEntry);
	
	// Allocate a temp file in the /boot/tmp directory
	MFileUtils::GetTempFile(tempEntry, fFile);

	// If we need to convert the file on output
	// we allocate a temp buffer to hold the text
	if (fNativeTextFormat != kNewLineFormat) {
		MFormatUtils::ConvertToNativeFormat(buffer, length, fTextView->CountLines(), fNativeTextFormat);
	}

	// Write the data out
	status_t err = B_NO_ERROR;
	BDirectory parentDir;

	if (err == B_NO_ERROR)
	{
		BFile temp(&tempEntry, B_READ_WRITE);
		ssize_t writtenlength;
		writtenlength = temp.Write(buffer, length);
		err = writtenlength == length ? B_NO_ERROR : B_ERROR;
		
		if (err == B_NO_ERROR) {
			
			err = fFile->GetParent(&parentDir);
			if (err == B_NO_ERROR) {
				time_t creationTime;
				status_t timeErr;
	
				if (B_NO_ERROR == fFile->GetCreationTime(&creationTime)) {
					timeErr = tempEntry.SetCreationTime(creationTime);
				}
	
				BNode textFile(fFile);
				err = CopyAttrs(textFile, temp);
				
				// If we failed to copy the attributes
				// try to set the type manually
				if (err != B_OK) {
					BNodeInfo mimefile(&temp);
					mimefile.SetType(kIDETextMimeType);
				}
							
				if (inSync)
					sync();		// make sure the file is saved to disk
	
				// If we get this far, make sure err is set appropriately for MoveTo
				// (We don't care about failed attributes at this point.
				// if the bytes of the file have been written then err = B_OK)
				err = B_OK;
			}
		}
	}
	
	// The MoveTo has to be done after the file is closed
	// cifs (and perhaps posix) don't support renaming open files
	if (err == B_OK) {
		err = tempEntry.MoveTo(&parentDir, Title(), true);
		if (err == B_OK) {
			autoE.release();
		}
		fTextView->DocumentSaved();
	}
	
	// Delete the buffer if we allocated one
	if (fNativeTextFormat != kNewLineFormat)
		delete[] buffer;

	// The first time that a file is saved it might not know
	// anything about an open project.  It tells the app
	// that it has been saved and the app will send back
	// a 'project opened' msg if there is an open project.
	if (err == B_NO_ERROR && fProject == nil)
	{
		BMessage msg(msgFileSaved);
		
		msg.AddPointer(kTextWindow, this);

		be_app_messenger.SendMessage(&msg);
	}
	
	if (err != B_NO_ERROR)
	{
		String		text = "An error ocurred while saving the file "B_UTF8_OPEN_QUOTE;
		text += Title();
		text += B_UTF8_CLOSE_QUOTE".";
		text += err;

		MAlert	alert(text);
		alert.Go();
	}
	
	return err;
}

// ---------------------------------------------------------------------------
//		WriteFileToBlock
// ---------------------------------------------------------------------------
//	Copy the contents of the file to the memory block.  ioSize
//	specifies the size of the block on input and the size of the
//	contents on output.  return B_NO_ERROR if no problems
//	occur.

status_t
MTextWindow::WriteToBlock(
	char*		inBlock,
	off_t&		inSize)
{
	Lock();

	int32		length = fTextView->TextLength() + 1;
	status_t	err = B_ERROR;

	// If the file isn't new line terminated then we generate
	// a version with the correct eol format
	if (length < inSize)
	{
		if (fNativeTextFormat == kNewLineFormat)
		{	
			memcpy(inBlock, fTextView->Text(), length);
		}
		else
		{
			int32			length = fTextView->TextLength();
			char *			buffer = (char*) fTextView->Text();
			MFormatUtils::ConvertToNativeFormat(buffer, length, fTextView->CountLines(), fNativeTextFormat);
		
			memcpy(inBlock, buffer, length);
			
			delete [] buffer;
		}

		inBlock[length] = 0;
		inSize = length;
		
		err = B_OK;
	}

	Unlock();

	return err;
}

// ---------------------------------------------------------------------------
//		DoSaveAs
// ---------------------------------------------------------------------------
//	Put up the save panel to SaveAs.

bool
MTextWindow::DoSaveAs()
{
	FileNameT	 	docTitle;

	GetDocTitle(docTitle);

	String		windowTitle = "Save "B_UTF8_OPEN_QUOTE;
	windowTitle += Title();
	windowTitle += B_UTF8_CLOSE_QUOTE;
	
	fSavingACopyAs = false;
	ShowSavePanel(windowTitle, docTitle);
	
	return false;
}

// ---------------------------------------------------------------------------
//		SaveRequested
// ---------------------------------------------------------------------------
//	The save panel requested that we save so we need to create the file
//	object and specify it.

void
MTextWindow::SaveRequested(
	entry_ref* 		directory, 
	const char *	name)
{
	if (fSavingACopyAs)
		SaveACopyAsRequested(directory, name);
	else
	{
		BDirectory		dir(directory);	// Specify directory
		BFile			file;

		status_t		err = dir.CreateFile(name, &file);

		if (err == B_NO_ERROR)
		{
			entry_ref		oldRef;

			if (fFile)
			{
				fFile->GetRef(&oldRef);
				delete fFile;
			}
	
			fFile = new BEntry;						// Create file object

			err = dir.FindEntry(name, fFile);

			if (err == B_NO_ERROR)
			{
				BNodeInfo	mimefile(&file);
				
				err = mimefile.SetType(kIDETextMimeType);

				SetTitle(name);					// Set the window title
				
				err = WriteDataToFile();		// Really save it
				
				// Is our project open?
				if (err == B_NO_ERROR && fProject != nil)
				{
					entry_ref		newRef;
					fFile->GetRef(&newRef);
					BMessage		msg(msgSaveAsForFile);
					
					// Add both refs
					msg.AddRef(kTextFileRef, &oldRef);
					msg.AddRef(kTextFileRef, &newRef);
					
					fProject->PostMessage(&msg);
				}
				// Are we supposed to add this file as we save it?
				else if (err == B_NO_ERROR && this->SavePanelAddProjectChecked()) {
					this->AddToProject();
				}
			}
			
			fTextView->SetSuffixType(GetSuffixType(name));
		}
	}
	StartMonitor();
}

// ---------------------------------------------------------------------------
//		DoSaveACopyAs
// ---------------------------------------------------------------------------
//	Open the save panel in 'save a copy as' mode.

void
MTextWindow::DoSaveACopyAs()
{
	fSavingACopyAs = true;
	ShowSavePanel("Save a Copy As", Title());
}

// ---------------------------------------------------------------------------
//		SaveACopyAsRequested
// ---------------------------------------------------------------------------

void
MTextWindow::SaveACopyAsRequested(
	entry_ref* 		inRef, 
	const char *	name)
{
	BDirectory		dir(inRef);		// Specify directory
	status_t		err;

	{
		BFile			file;
	
		// Specify file object and create it on disk
		err = dir.CreateFile(name, &file);
	}

	if (err == B_NO_ERROR)
	{
		bool fileWriteOK = false;
		BEntry entry;
		err = dir.FindEntry(name, &entry);

		if (err == B_NO_ERROR)
		{
			{
				BFile		file(&entry, B_WRITE_ONLY);
				
				if (B_OK == file.InitCheck())
				{
					// Write out the data
					ssize_t		byteswritten = file.Write(fTextView->Text(), fTextView->TextLength());
					
					err = byteswritten == fTextView->TextLength() ? B_NO_ERROR : B_ERROR;
					
					if (err == B_OK)
					{
						fileWriteOK = true;
						BNode textFile(fFile);
						err = CopyAttrs(textFile, file);
					}
				}
			}
			
			if (err == B_NO_ERROR)
			{
				// Write out the preferences
				BEntry*	tempFile = fFile;
				fFile = &entry;
				SetPrefs();
				fFile = tempFile;
			}
			
			// err can signal an error because of the CopyAttrs 
			// (ie: from a newly created text file)
			// if the file was written ok, then see if we were instructed
			// to add the saved copy to the project also
			if (fileWriteOK && this->SavePanelAddProjectChecked()) {			
				this->AddEntryToProject(entry);
			}
		}
	}
}

// ---------------------------------------------------------------------------
//		OpenGoToLineWindow
// ---------------------------------------------------------------------------

void
MTextWindow::OpenGoToLineWindow()
{
	if (fGoToLineWindow)
		fGoToLineWindow->Activate();
	else
	{
		BPoint 		topLeft = Bounds().LeftTop();

		ConvertToScreen(&topLeft);
		topLeft.y += kTabHeight;
		topLeft.x -= 5;

		fGoToLineWindow = new MGoToLineWindow(*this, topLeft);
		fGoToLineWindow->Show();
	}
}

// ---------------------------------------------------------------------------
//		DoGoToLine
// ---------------------------------------------------------------------------
//	Respond to a message from the gotoline window or from the message window.

void
MTextWindow::DoGoToLine(
	BMessage& inMessage)
{
	bool		good = false;
	bool		isFunction = false;
	int32		selStart;
	int32		selEnd;
	int32		lineNumber;

	// Check for a Token Identifier
	if (inMessage.HasData(kTokenIdentifier, kTokenIDType))
	{
		ssize_t				length;
		TokenIdentifier*	token;
		if (B_NO_ERROR == inMessage.FindData(kTokenIdentifier, kTokenIDType, (const void**) &token, &length))
		{
			ASSERT(length == sizeof(TokenIdentifier));
	
			const char*		text = fTextView->Text();
			int32			size = fTextView->TextLength();
			
			isFunction = token->eIsFunction;
	
			// Find the sync text
			// If the text has been modified the sync text may still be somewhere in the
			// document.  Move back and forth looking for it.  This will fail if any text within
			// the sync string (usually length of 31) has been altered.
			for (int i = 0; i < 1000 && ! good; i++)
			{
				int	j = -i;
				do {
					int	k = token->eOffset - token->eSyncOffset + j;
					if (k >= 0 && k < size)
						if (0 == memcmp (&text[k], token->eSync, token->eSyncLength)) 
						{
							selStart = k + token->eSyncOffset;
							selEnd = k + token->eSyncOffset + token->eLength;
	
							fTextView->Select(selStart, selEnd);
							good = true;
							break;
						}
					j += i + i;
				} while (j <= i && i != 0);
			}
	
			// If we failed to find the sync text then just go to the specified line.
			// This will probably not be correct.
			if (! good)
			{
				fTextView->GoToLine(token->eLineNumber);
				good = true;
			}
		}
	}
	else
	// If no token identifier check for a line number
	if (B_NO_ERROR == inMessage.FindInt32(kLineNumber, &lineNumber))
	{
		fTextView->GoToLine(lineNumber);
		good = true;
	}

	if (good)
	{
		Activate();		// Bring us to the front

		if (isFunction)
			fTextView->ScrollToFunction(selStart, selEnd);
		else
			fTextView->ScrollToSelection();
	}
}

// ---------------------------------------------------------------------------
//		GetDocTitle
// ---------------------------------------------------------------------------

void
MTextWindow::GetDocTitle(
	char * outName)
{
	if (fFile)
	{
		fFile->GetName(outName);
	}
	else
	{
		strcpy(outName, Title());
	}
}

// ---------------------------------------------------------------------------
//		ReadTextFile
// ---------------------------------------------------------------------------
//	Read in the contents of a text file.

void
MTextWindow::ReadTextFile()
{
	ASSERT(fFile != NULL);
	off_t		length;
	status_t	err = fFile->GetSize(&length);;
	length = max(length, 0LL);

	if (err == B_NO_ERROR)
	{
		char* 	buffer = new char[length + 1];

		if (buffer)
		{
			buffer[length] = 0;
			
			BFile		file(fFile, B_READ_ONLY);
			
			err = file.InitCheck();

			if (err == B_NO_ERROR)
			{
				ssize_t	size = file.Read(buffer, length);
				err = size == length ? B_NO_ERROR : B_ERROR;;
			}

			if (err == B_NO_ERROR)
			{
				fNativeTextFormat = MFormatUtils::FindFileFormat(buffer);
				if (fNativeTextFormat != kNewLineFormat)
					MFormatUtils::ConvertToNewLineFormat(buffer, length, fNativeTextFormat);

				fTextView->SetText(buffer, length);
			}
			
			delete[] buffer;
		}
		else
		{
			BAlert * alert = new BAlert("Too large!", "The file you want to open is larger than available memory.", "Oops!");
			alert->Go();
		}
	}

	if (err != B_NO_ERROR)
	{
		String		text = "An error ocurred while reading the file "B_UTF8_OPEN_QUOTE;
		text += Title();
		text += B_UTF8_CLOSE_QUOTE".";
		text += err;

		MAlert	alert(text);
		alert.Go();
	}
}

// ---------------------------------------------------------------------------
//		DoCompile
// ---------------------------------------------------------------------------
//	Respond to a command K by asking the project to compile us.

void
MTextWindow::DoCompile()
{
	ASSERT(fProject);

	DoSave();		// fascist aren't we?

	BMessage		msg(msgCompileOne);
	entry_ref		ref;
	
	msg.AddString(kFileName, Title());

	if (B_NO_ERROR == fFile->GetRef(&ref))
			msg.AddRef(kTextFileRef, &ref);
	
	fProject->PostMessage(&msg);
}

// ---------------------------------------------------------------------------
//		DoSendToProject
// ---------------------------------------------------------------------------
//	A general function that handles preprocess, precompile, check syntax,
//	and disassemble.  These processes can be done if the file is saved to
//	disk and a project file is open.  This file doesn't have to be part
//	of the open project.  Pass in the appropriate inWhat message to be
//	sent to the project.

void
MTextWindow::DoSendToProject(
	uint32 inWhat)
{
	if (fProjectIsOpen && fFile)
	{
		DoSave();		// fascist aren't we?
	
		entry_ref		ref;

		if (B_NO_ERROR == fFile->GetRef(&ref))
		{
			BMessage		msg(inWhat);
			msg.AddString(kFileName, Title());
			msg.AddRef(kTextFileRef, &ref);
			
			if (fProject)
				fProject->PostMessage(&msg);
			else
				be_app_messenger.SendMessage(&msg);
		}
	}
}

// ---------------------------------------------------------------------------
//		WindowActivated
// ---------------------------------------------------------------------------

void
MTextWindow::WindowActivated(
	bool inActive)
{
	BWindow::WindowActivated(inActive);
	if (inActive)
	{
		MDynamicMenuHandler::WindowActivated(this);
		SetPulseRate(kNormalPulseRate);		// half second

		// Set the cursor to the hand if it isn't in the textview
		// The textview sets it to the IBeam if the mouse is inside the view
		BPoint 	where;
		uint32	buttons;
		fTextView->GetMouse(&where, &buttons);
		if (! fTextView->Bounds().Contains(where))
			be_app->SetCursor(B_HAND_CURSOR);
	}
	else
	{
		SetPulseRate(kSlowPulseRate);		// off
	}
}

// ---------------------------------------------------------------------------
//		DoEnterFindString
// ---------------------------------------------------------------------------
//	Post a setfindstring message to the find window.  This message is
//	also optionally used for 'Find Selection' (cmd-H).

void
MTextWindow::DoEnterFindString(
	bool	inFindIt)
{
	BMessage		msg(msgSetFindString);
	AddSelectionToMessage(msg);
	
	msg.AddBool(kFindIt, inFindIt);

	MFindWindow::GetFindWindow().PostMessage(&msg);
}

// ---------------------------------------------------------------------------
//		DoFindSelection
// ---------------------------------------------------------------------------
//	Post a find selection message to the find window. (cmd-H).

void
MTextWindow::DoFindSelection(
	bool	inForward)
{
	int32			selStart;
	int32			selEnd;
	
	fTextView->GetSelection(&selStart, &selEnd);
	
	int32			textLen = selEnd - selStart;
	const char *	text = fTextView->Text() + selStart;

	bool		found = MFindWindow::GetFindWindow().DoFindSelection(*fTextView, text, textLen, inForward);
	
	Activate();
}

// ---------------------------------------------------------------------------
//		AddSelectionToMessage
// ---------------------------------------------------------------------------

void
MTextWindow::AddSelectionToMessage(
	BMessage&	inMessage)
{
	int32			selStart;
	int32			selEnd;
	
	fTextView->GetSelection(&selStart, &selEnd);
	
	int32			textLen = selEnd - selStart;
	const char *	text = fTextView->Text();
	
	inMessage.AddData(kAsciiText, B_ASCII_TYPE, text + selStart, textLen);
}

// ---------------------------------------------------------------------------
//		DoFindNext
// ---------------------------------------------------------------------------
// Ask the find window to find the text for us

bool
MTextWindow::DoFindNext(
	bool	inForward)
{
	bool		found = MFindWindow::GetFindWindow().DoFindNext(*fTextView, inForward);
	
	Activate();

	return found;
}

// ---------------------------------------------------------------------------
//		DoReplace
// ---------------------------------------------------------------------------

void
MTextWindow::DoReplace()
{
	if (fIsWritable)
		MFindWindow::GetFindWindow().DoReplace(*fTextView);
	else
		beep();

	Activate();
}

// ---------------------------------------------------------------------------
//		DoReplaceAndFind
// ---------------------------------------------------------------------------

void
MTextWindow::DoReplaceAndFind(
	bool	inForward)
{
	if (fIsWritable)
		MFindWindow::GetFindWindow().DoReplaceAndFind(*fTextView, inForward);
	else
		beep();

	Activate();
}

// ---------------------------------------------------------------------------
//		DoReplaceAll
// ---------------------------------------------------------------------------

void
MTextWindow::DoReplaceAll()
{
	if (fIsWritable)
		MFindWindow::GetFindWindow().DoReplaceAll(*fTextView);
	else
		beep();

	Activate();
}

// ---------------------------------------------------------------------------
//		GetRef
// ---------------------------------------------------------------------------
//	Private accessor for the find window.

status_t
MTextWindow::GetRef(
	entry_ref&	inoutRef)
{
	status_t	err = B_ERROR;

	if (fFile != nil)
	{
		err = fFile->GetRef(&inoutRef);
	}
	
	return err;
}


// ---------------------------------------------------------------------------
//		AddToProject
// ---------------------------------------------------------------------------
//	Tell the app to add us to the currently open project.

void
MTextWindow::AddToProject()
{
	ASSERT(fFile != nil);
		
	BMessage		msg(msgAddToProject);
	
	msg.AddPointer(kTextWindow, this);
	
	entry_ref		ref;
	
	status_t		err = fFile->GetRef(&ref);

	if (err == B_NO_ERROR)
	{
		msg.AddRef(kTextFileRef, &ref);
		
		be_app_messenger.SendMessage(&msg);
	}
}

// ---------------------------------------------------------------------------
//		HandleProjectClosed
// ---------------------------------------------------------------------------
//	A Project window has closed.

void
MTextWindow::HandleProjectClosed(
	BMessage& 			inMessage)
{
	MProjectWindow*		project;
	if (B_NO_ERROR == inMessage.FindPointer(kProjectMID, (void**) &project))
	{
		if (project == fProject) {
			fProject = nil;
			fProjectIsOpen = false;
		}
	}
}

// ---------------------------------------------------------------------------
//		HandleProjectOpened
// ---------------------------------------------------------------------------
//	A Project window has opened.

void
MTextWindow::HandleProjectOpened(
	BMessage& 			inMessage)
{
	if (fFile)
	{
		MProjectWindow*		project;
		if (B_NO_ERROR == inMessage.FindPointer(kProjectMID, (void**) &project))
		{
			bool				isInProject = false;
	
			if (inMessage.HasBool(kIsInProject))
				isInProject = inMessage.FindBool(kIsInProject);
			if (project)
			{
				if (isInProject)
					fProject = project;
				else
				if (project->Lock())
				{
					if (project->FileIsInProjectByFile(fFile))
						fProject = project;
					project->Unlock();
				}
			}
		}
	}

	fProjectIsOpen = true;
}

// ---------------------------------------------------------------------------
//		OpenSelection
// ---------------------------------------------------------------------------
//	The app has asked us to send it an open selection message in response to 
//	cmd-d in the app menu.  The app actually coordinates the work.
//	There are two cases.  Either there is a selection in this window or not.
//	if no selection then the app needs to open the open selection dialog.
//	If there is a selection then we send this to the app as the file to open
//	and the open selection dialog isn't involved.  The same message is sent
//	but different options are included in the message.

void 
MTextWindow::OpenSelection()
{
	long 			start;
	long 			end;
	BMessage		msg(msgDoOpenSelection);

	fTextView->GetSelection(&start, &end);

	if (start != end) 
	{
		// Extend the selection if a '.' follows the current selection.
		const long 		MAXID = 256;
		char 			fileName[MAXID];
		bool	 		systemInclude =  false;
		const char *	text = 	fTextView->Text();

		if (text[end] == '.') 
		{
			end++;
			while (isalnum(text[end]))
			{
				end++;
			}
		}

		// Stop the selection at MAXID characters.
		if (end - start > MAXID - 1)
			end = start + MAXID - 1;
		memcpy(fileName, &text[start], end - start);
		fileName[end - start] = 0;

		// Update the selection.
		fTextView->Select(start, end);

		// Check to see if the file is a system include. ie <...>
		if (start > 0)	//	selection starts at start of file ...
			if (text[start - 1] == '<' && text[end] == '>')
				systemInclude = true;

		// Build the message
		entry_ref		ref;
		
		if (fFile != nil && B_NO_ERROR == fFile->GetRef(&ref))
			msg.AddRef(kTextFileRef, &ref);				// our entry_ref
		msg.AddString(kFileName, fileName);				// filename to be opened
		msg.AddBool(kSystemInclude, systemInclude);		// is the file enclosed in brackets
	}

	// In either case (selection or no), pass in the owning project of this file
	msg.AddPointer(kProjectMID, fProject);

	// Ask the app to open the file or open the selection window
	be_app_messenger.SendMessage(&msg);
}

// ---------------------------------------------------------------------------
//		DoAndyFeature
// ---------------------------------------------------------------------------
//	Respond to alt-tab in this window.  Ask the app to do the work.
//	Can't do anything for an unsaved file.

void 
MTextWindow::DoAndyFeature()
{
	entry_ref		ref;
	
	if (fFile != nil && B_NO_ERROR == fFile->GetRef(&ref))
	{
		// Build the message
		BMessage		msg(cmd_AndyFeature);
		msg.AddRef(kTextFileRef, &ref);			// our entry_ref
		msg.AddString(kFileName, Title());		// our filename
		msg.AddPointer(kProjectMID, fProject);	// our project
	
		be_app_messenger.SendMessage(&msg);
	}
}

// ---------------------------------------------------------------------------
//		HasSelection
// ---------------------------------------------------------------------------
//	Is there a selection in the textView?  Called from the find window.

bool
MTextWindow::HasSelection()
{
	MLocker<BLooper>	lock(this);
	
	return fTextView->HasSelection();
}

// ---------------------------------------------------------------------------
//		IsMatch
// ---------------------------------------------------------------------------
//	Return true if we're editing this file

bool
MTextWindow::IsMatch(
	const entry_ref& inRef)
{
	bool	result = false;

	if (fFile != nil)
	{
		entry_ref		ref;
		
		status_t	err = fFile->GetRef(&ref);
		
		result = (err == B_NO_ERROR && ref == inRef);
	}

	return result;
}


// ---------------------------------------------------------------------------
//		GetData
// ---------------------------------------------------------------------------
//	Fill the BMessage with preferences data of the kind that this window
//	knows about.

void
MTextWindow::GetData(
	BMessage&	inOutMessage)
{
	MLocker<BLooper>	lock(this);

	if (inOutMessage.HasData(kFontPrefs, kMWPrefs))
	{
		inOutMessage.ReplaceData(kFontPrefs, kMWPrefs, &fFontPrefs, sizeof(fFontPrefs));
	}
}

// ---------------------------------------------------------------------------
//		SetData
// ---------------------------------------------------------------------------
//	Extract preferences data of the kind that this window knows about 
//	from the BMessage, and set the fields in the view.
//	inPermanent indicates whether these changes should be saved 
//	with the file when the file is closed.

void
MTextWindow::SetData(
	BMessage&	inOutMessage,
	bool		inPermanent)
{
	MLocker<BLooper>	lock(this);
	ssize_t				length;
	FontPrefs*			font;
	float				tabSize = fFontPrefs.pTabSize;

	if (B_NO_ERROR == inOutMessage.FindData(kFontPrefs, kMWPrefs, (const void**) &font, &length))
	{
		ASSERT(length == sizeof(FontPrefs));

		if (inPermanent)
		{
			fFontPrefs = *font;
			tabSize = fFontPrefs.pTabSize;
		}
		else
		{
			fTempFontPrefs = *font;
			tabSize = fTempFontPrefs.pTabSize;
		}

		UpdateTextView(inPermanent);
		
		fChangeFontInfo = fChangeFontInfo || inPermanent;
		fUsingPermanentFontPrefs = inPermanent;

		if (inPermanent)
			SetPrefs();
	}

	SyntaxStylePrefs*	style;
	if (B_NO_ERROR == inOutMessage.FindData(kSyntaxStylePrefs, kMWPrefs, (const void**) &style, &length))
	{
		ASSERT(length == sizeof(SyntaxStylePrefs));

		fTextView->UpdateSyntaxStyleInfo(*style, tabSize);
	}

	AppEditorPrefs*	colors;
	if (B_NO_ERROR == inOutMessage.FindData(kAppEditorPrefs, kMWPrefs, (const void**) &colors, &length))
	{
		ASSERT(length == sizeof(AppEditorPrefs));

		fTextView->SetViewColor(colors->pBackgroundColor);
		fTextView->Invalidate();
	}
	
	this->UpdateViewMenu();
}

// ---------------------------------------------------------------------------

void
MTextWindow::ChangeFontSize(float size)
{
	BAutolock lock(this);

	// If the user is currently looking at the temporary settings,
	// use those as a basis.  But since they are explicitly setting
	// the values, we switch to permanent.
	if (fUsingPermanentFontPrefs == false) {
		fFontPrefs = fTempFontPrefs;
		fUsingPermanentFontPrefs = true;
	}
	
	fFontPrefs.pFontSize = size;
	this->UpdateFontData();
}

// ---------------------------------------------------------------------------

void
MTextWindow::ChangeFont(const font_family family, const font_style style)
{
	BAutolock lock(this);

	// If the user is currently looking at the temporary settings,
	// use those as a basis.  But since they are explicitly setting
	// the values, we switch to permanent.
	if (fUsingPermanentFontPrefs == false) {
		fFontPrefs = fTempFontPrefs;
		fUsingPermanentFontPrefs = true;
	}

	strcpy(fFontPrefs.pFontFamily, family);
	if (style == nil) {
		style = "";
	}
	strcpy(fFontPrefs.pFontStyle, style);	
	this->UpdateFontData();
}

// ---------------------------------------------------------------------------

void
MTextWindow::UpdateFontData()
{
	this->UpdateTextView();
	fChangeFontInfo = true;
	this->SetPrefs();
}

// ---------------------------------------------------------------------------
//		ToggleSyntaxColoring
// ---------------------------------------------------------------------------

void
MTextWindow::ToggleSyntaxColoring()
{
	bool	usesSyntaxColoring = ! fTextView->UsesSyntaxColoring();
	fTextView->UseSyntaxColoring(usesSyntaxColoring);
	fChangeFontInfo = true;

	if (! usesSyntaxColoring)
	{
		if (fUsingPermanentFontPrefs)
			fTextView->UpdateFontInfo(fFontPrefs.pFontFamily, fFontPrefs.pFontStyle,
				fFontPrefs.pFontSize, fFontPrefs.pTabSize, fFontPrefs.pDoAutoIndent);
		else
			fTextView->UpdateFontInfo(fTempFontPrefs.pFontFamily, fTempFontPrefs.pFontStyle, 
				fTempFontPrefs.pFontSize, fTempFontPrefs.pTabSize, fTempFontPrefs.pDoAutoIndent);
	}

	this->UpdateViewMenu();
}

// ---------------------------------------------------------------------------
//		RememberSelections
// ---------------------------------------------------------------------------
//	Set whether windows scroll to selection when opened.

void
MTextWindow::RememberSelections(
	bool inRemember)
{
	MLocker<BLocker>	lock(fSavePanelLock);

	fRememberSelections = inRemember;
}

// ---------------------------------------------------------------------------
//		SelectionsAreRemembered
// ---------------------------------------------------------------------------
//	Does the window scroll to selection when it is opened?

bool
MTextWindow::SelectionsAreRemembered()
{
	MLocker<BLocker>	lock(fSavePanelLock);
	
	return fRememberSelections;
}

// ---------------------------------------------------------------------------
//		UpdateTextView
// ---------------------------------------------------------------------------
//	Set the font settings that will be seen onscreen.  inPermanent indicates
//	whether to use the settings from the permanent font prefs struct or the 
//	temp font prefs struct.

void
MTextWindow::UpdateTextView(
	bool		inPermanent)
{
	if (inPermanent)
	{
		fTextView->UpdateFontInfo(fFontPrefs.pFontFamily, fFontPrefs.pFontStyle, 
			fFontPrefs.pFontSize, fFontPrefs.pTabSize, fFontPrefs.pDoAutoIndent);
	}
	else
	{
		fTextView->UpdateFontInfo(fTempFontPrefs.pFontFamily, fTempFontPrefs.pFontStyle, 
			fTempFontPrefs.pFontSize, fTempFontPrefs.pTabSize, fTempFontPrefs.pDoAutoIndent);
	}

	fUsingPermanentFontPrefs = inPermanent;
	fTextView->Invalidate();
	this->UpdateViewMenu();
}

// ---------------------------------------------------------------------------

void
MTextWindow::UpdateViewMenu()
{
	// enable/disable the various pieces of the view menu based on current
	// settings
	
	::MarkCurrentFontSize(fSizeMenu, 
					   fUsingPermanentFontPrefs ? fFontPrefs.pFontSize : fTempFontPrefs.pFontSize);
	::MarkCurrentFont(fFontMenu, 
				    fUsingPermanentFontPrefs ? fFontPrefs.pFontFamily : fTempFontPrefs.pFontFamily, 
				    fUsingPermanentFontPrefs ? fFontPrefs.pFontStyle : fTempFontPrefs.pFontStyle);

	BMenuItem* syntaxItem = fViewMenu->FindItem(kSyntaxItemLabel);
	BMenuItem* sizeItem = fViewMenu->FindItem(kSizeItemLabel);
	BMenuItem* fontItem = fViewMenu->FindItem(kFontItemLabel);
	
	if (this->UsesSyntaxColoring()) {
		if (syntaxItem) {
			syntaxItem->SetMarked(true);
		}
		if (sizeItem) {
			sizeItem->SetEnabled(false);
		}
		if (fontItem) {
			fontItem->SetEnabled(false);
		}
	}
	else {
		if (syntaxItem) {
			syntaxItem->SetMarked(false);
		}
		if (sizeItem) {
			sizeItem->SetEnabled(true);
		}
		if (fontItem) {
			fontItem->SetEnabled(true);
		}
	}
}

// ---------------------------------------------------------------------------
//		BuildPopupMenu
// ---------------------------------------------------------------------------
//	Build the header popup menu for this window.

void
MTextWindow::BuildPopupMenu(
	MPopupMenu& 	inPopup)
{
	if (fProject)		// Is our project open
		fProject->BuildPopupMenu(Title(), inPopup);
	else
	{
		BMessage*		msg = new BMessage(msgNull);
		BMenuItem*		item = new BMenuItem("Not a project file", msg);

		inPopup.AddItem(item);
		item->SetEnabled(false);
	}
}

// ---------------------------------------------------------------------------
//		GetPrefs
// ---------------------------------------------------------------------------
//	Get the data that contains the font settings for this window from the 
//	file's attributes.

void
MTextWindow::GetPrefs()
{
	bool		gotPrefs = false;

	ASSERT(sizeof(FontPrefsData) == 164);

	if (fFile != nil)
	{
		FontPrefsData prefs;
		memset(&prefs, 0, sizeof(prefs));
		
		BNode file(fFile);
		ssize_t size = file.ReadAttr(kFontPrefsNewName, 'pref', 0, &prefs, sizeof(prefs));
		bool newFormat = (size > 0);		

		if (! newFormat)
		{
			size = file.ReadAttr(kFontPrefsName, 'pref', 0, &prefs, sizeof(prefs));
			if (size > 0)
			{
				file.RemoveAttr(kFontPrefsName);
				prefs.pFlags = kUsesSyntaxStyles;
				file.WriteAttr(kFontPrefsNewName, 'pref', 0, &prefs, sizeof(prefs));
			}
		}

		if (size == sizeof(FontPrefsData))
		{
			// Get the values from the attribute
			prefs.SwapBigToHost();
			if (strlen(prefs.pFontFamily) == 0 || strlen(prefs.pFontStyle) == 0 || prefs.pFontSize == 0) {
				// Eddie doesn't write the font/style correctly
				// if we get garbage, clean it up here, but keep the selection preferences
				this->GetDefaultPrefs();
			}
			else {
				strcpy(fFontPrefs.pFontFamily, prefs.pFontFamily);
				strcpy(fFontPrefs.pFontStyle, prefs.pFontStyle);
				fFontPrefs.pFontSize = prefs.pFontSize;
				fFontPrefs.pTabSize = prefs.pTabSize;
				fFontPrefs.pDoAutoIndent = prefs.pAutoIndent;
			}
			fSelStart = prefs.pSelStart;
			fSelEnd = prefs.pSelEnd;

			// This actually gets blown away because we use the preference setting
			// for turning on syntax coloring  (see IDEApp::SetTextFileFont)
			fTextView->UseSyntaxColoring((prefs.pFlags & kUsesSyntaxStyles) != 0);
			
			if (SelectionsAreRemembered())
			{
				fTextView->Select(fSelStart, fSelEnd);
				fTextView->GetSelection(&fSelStart, &fSelEnd);	// validate the selection
				fTextView->ScrollToSelection();
			}
			
			MoveTo(prefs.pFrame.LeftTop());
			ResizeTo(prefs.pFrame.Width(), prefs.pFrame.Height());

			ValidateWindowFrame(*this);

			gotPrefs = true;
		}
	}

	if (! gotPrefs)
		GetDefaultPrefs();
}

// ---------------------------------------------------------------------------
//		GetDefaultPrefs
// ---------------------------------------------------------------------------

void
MTextWindow::GetDefaultPrefs()
{
	MDefaultPrefs::SetFontDefaults(fFontPrefs);
	fChangeFontInfo = true;
}

// ---------------------------------------------------------------------------
//		SelectionChanged
// ---------------------------------------------------------------------------

inline bool
MTextWindow::SelectionChanged()
{
	long	selStart;
	long	selEnd;

	fTextView->GetSelection(&selStart, &selEnd);

	return selStart != fSelStart || selEnd != fSelEnd;
}

// ---------------------------------------------------------------------------
//		SetPrefs
// ---------------------------------------------------------------------------
//	Save a field that contains the font settings for this window in the 
//	file's attrubutes.

void
MTextWindow::SetPrefs()
{
	if (fIsWritable && fSavePrefs && fFile != nil && fFile->Exists() && 
		(fChangeFontInfo || SelectionChanged()))
	{	
		// Fill in the prefs struct
		FontPrefsData		newPrefs;

		memset(&newPrefs, 0, sizeof(newPrefs));
		strcpy(newPrefs.pFontFamily, fFontPrefs.pFontFamily);
		strcpy(newPrefs.pFontStyle, fFontPrefs.pFontStyle);
		newPrefs.pFontSize = (int32) fFontPrefs.pFontSize;
		newPrefs.pTabSize = fFontPrefs.pTabSize;
		newPrefs.pAutoIndent = fFontPrefs.pDoAutoIndent;
		newPrefs.pFlags = fTextView->UsesSyntaxColoring() ? kUsesSyntaxStyles : 0;
		newPrefs.pFrame = Frame();
		fTextView->GetSelection(&newPrefs.pSelStart, &newPrefs.pSelEnd);
		newPrefs.SwapHostToBig();

		BNode	file(fFile);
		
		file.WriteAttr(kFontPrefsNewName, 'pref', 0, &newPrefs, sizeof(newPrefs));
	}
}

// ---------------------------------------------------------------------------

void
FontPrefsData::SwapBigToHost()
{
	if (B_HOST_IS_LENDIAN) {
		SwapRectToHost(pFrame);
		pSelStart = B_BENDIAN_TO_HOST_INT32(pSelStart);
		pSelEnd = B_BENDIAN_TO_HOST_INT32(pSelEnd);
		pTabSize = B_BENDIAN_TO_HOST_INT32(pTabSize);
		pFontSize = B_BENDIAN_TO_HOST_INT32(pFontSize);
	}
}

// ---------------------------------------------------------------------------

void
FontPrefsData::SwapHostToBig()
{
	if (B_HOST_IS_LENDIAN) {
		SwapRectToBig(pFrame);
		pSelStart = B_HOST_TO_BENDIAN_INT32(pSelStart);
		pSelEnd = B_HOST_TO_BENDIAN_INT32(pSelEnd);
		pTabSize = B_HOST_TO_BENDIAN_INT32(pTabSize);
		pFontSize = B_HOST_TO_BENDIAN_INT32(pFontSize);
	}
}

// ---------------------------------------------------------------------------
//		ScanForEditAddOns
// ---------------------------------------------------------------------------
//	The edit addons live in the 'Editor_add_ons' folder inside the 'plugins'
//	folder inside the same folder that the ide lives in.


long
MTextWindow::ScanForEditAddOns()
{
	app_info	info;
	be_app->GetAppInfo(&info);

	BEntry 		file(&info.ref);
	BDirectory 	appfolder;

	status_t	err = file.GetParent(&appfolder);

	if (err == B_NO_ERROR)
	{
		BDirectory 	pluginsFolder;

		err = FindDirectory(appfolder, kPluginsFolderName, &pluginsFolder);
	
		if (err == B_NO_ERROR)
		{
			BDirectory 	addOnFolder;
		
			err = FindDirectory(pluginsFolder, kEditorAddOnsName, &addOnFolder);
		
			if (err == B_NO_ERROR)
			{
				BEntry		file;

				while (B_NO_ERROR == addOnFolder.GetNextEntry(&file))
				{
					BPath		path;
					
					if (file.IsFile() && B_NO_ERROR == file.GetPath(&path))
					{
						image_id	id = load_add_on(path.Path());
						if (id >= 0) 
						{
							perform_edit_func	func = NULL;
							get_image_symbol(id, "perform_edit", B_SYMBOL_TYPE_TEXT, (void**) &func);
	
							if (func) 
							{
						        edit_add_on *e = new edit_add_on;
						        e->image = id;
						        e->perform_edit = func;
						        file.GetName(e->name);
						        fEditAddOns.AddItem(e);
								RunStartupShutdownFunction(id, "edit_addon_startup");
							} else {
								unload_add_on(id);
							}
						}
					}
				}
			}
		}
	}

	return err;
}

// ---------------------------------------------------------------------------
//		ShutDownEditAddOns
// ---------------------------------------------------------------------------
//	Call from application destructor to clean up nicely

void
MTextWindow::ShutDownEditAddOns()
{
	edit_add_on *	e;
	long			i = fEditAddOns.CountItems() - 1;

	while (fEditAddOns.GetNthItem(e, i--))
	{
		RunStartupShutdownFunction(e->image, "edit_addon_shutdown");
		fEditAddOns.RemoveItem(e);
		unload_add_on(e->image);
		delete e;
	}
}

// ---------------------------------------------------------------------------
//		RunStartup
// ---------------------------------------------------------------------------

void
MTextWindow::RunStartupShutdownFunction(image_id id, const char* name)
{
	// Run edit_addon_startup/shutdown when called (if it exists)
	// The addon my export the following:
	//	extern "C" status_t edit_addon_startup(MTextAddOnStorage* storage);
	//	extern "C" status_t edit_addon_shutdown(MTextAddOnStorage* storage);

	typedef status_t (*startup_shutdown_func)(MTextAddOnStorage *addon);

	startup_shutdown_func func = NULL;
	get_image_symbol(id, name, B_SYMBOL_TYPE_TEXT, (void**) &func);
	if (func) {
		MTextAddOnStorage storageHandler;
		(*func)(&storageHandler);
	}
}

// ---------------------------------------------------------------------------
//		BuildEditAddOnMenu
// ---------------------------------------------------------------------------
// Call in Window constructors to create add-on menu.

BMenu *
MTextWindow::BuildEditAddOnMenu(
	const char *title)
{
	if (!fEditAddOns.CountItems())
		return NULL;

	BMenu *			menu = new BMenu(title);
	BMenuItem *		item;
	edit_add_on *	e;
	long			i = 0;

	while (fEditAddOns.GetNthItem(e, i++))
	{
		BMessage *	m = new BMessage(CMD_EDIT_ADD_ON);

		m->AddInt32("image", e->image);
		m->AddString("name", e->name);

		item = new BMenuItem(e->name, m, 0);
		menu->AddItem(item);
	}

	return menu;
}

// ---------------------------------------------------------------------------
//		PerformEditAddOn
// ---------------------------------------------------------------------------
//	Call from window MessageReceived() when message->what
//	is CMD_EDIT_ADD_ON

status_t
MTextWindow::PerformEditAddOn(
	BMessage *	message)
{
	edit_add_on *	e;
	image_id		id;
	status_t		result = B_ERROR;

	if (B_NO_ERROR == message->FindInt32("image", &id))
	{
		long			i = 0;
	
		while (fEditAddOns.GetNthItem(e, i++))
		{
			if (e->image == id)
			{
				MTextAddOn	addOn(*fTextView);
				
				fTextView->StartAddon();

				result = (e->perform_edit)(&addOn);

				fTextView->StopAddon();
				break;
			}
		}
	}

	return result;
}
