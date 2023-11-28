//========================================================================
//	MProjectWindow.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
// BDS

#include <string.h>
#include <unistd.h>
#include "MProjectWindow.h"
#include "MProjectSettingsWindow.h"
#include "MTextWindow.h"
#include "MBlockFile.h"
#include "MAlert.h"
#include "MMessageInfoView.h"
#include "MProjectInfoView.h"
#include "MKeyFilter.h"
#include "IDEApp.h"
#include "IDEConstants.h"
#include "IDEMessages.h"
#include "MainMenus.h"
#include "ProjectCommands.h"
#include "Utils.h"
#include "Auto.h"

#include "MScripting.h"
#include "MEntryRefHandler.h"
#include "MScriptUtils.h"
#include "MessengerHandler.h"
#include "MProjectFileHandler.h"
#include "MLocker.h"
#include <Application.h>
#include <String.h>

// ---------------------------------------------------------------------------

const float	kFileCountWindowSize = 60.0;
const float kMinHoriz = 280.0;
const float kMinVert = 100.0;
const float kProjZoomWidth = 325.0;
const float kProjZoomTop = 25.0;
const float kDefaultLeft = 80.0;
const float kDefaultTop = 50.0;
const float kTopInfoViewHeight = 16.0;

const char* kSettingsWindowString = " Settings";

extern BLocker CursorLock;

BRect PROJ_projectFrame(kDefaultLeft, kDefaultTop, 
	kDefaultLeft + kMinHoriz, kDefaultTop + (2 * kMinVert));

// ---------------------------------------------------------------------------
//		MProjectWindow
// ---------------------------------------------------------------------------
//	Constructor for new project

MProjectWindow::MProjectWindow(
	const char * title)
	: BWindow(
		PROJ_projectFrame,
		title,
		B_DOCUMENT_WINDOW,
		0L),
		ScriptHandler(NewScriptID(), title, this)
{
	InitWindow();

	BuildWindow();
	fProjectView->AddDefaultSection();
	UpdateRunMenuItems(fProjectView->RunsWithDebugger());
}

// ---------------------------------------------------------------------------
//		MProjectWindow
// ---------------------------------------------------------------------------
//	Constructor for existing project or new empty project.  If empty project
//	the file specified by the ref must already exist but be empty.

MProjectWindow::MProjectWindow(
	const entry_ref &	inRef,
	bool				inEmptyProject)
	: BWindow(
		PROJ_projectFrame,
		inRef.name,
		B_DOCUMENT_WINDOW,
		0L),
		ScriptHandler(NewScriptID(), inRef.name, this)
{
	InitWindow();
	BuildWindow();

	fFile = new MBlockFile(inRef);
	status_t	err = fFile->GetParent(&fProjectView->GetProjectDirectory());
	ASSERT(err == B_NO_ERROR);

	SetTitle(inRef.name);
	
	fProjectView->SetProject(inRef);

	if (! inEmptyProject)
	{
		BuildProject();
		GetPrefs();
	}
	else
	{
		fProjectView->AddDefaultSection();
		fProjectView->SetWorkingDirectory();
	}
	UpdateRunMenuItems(fProjectView->RunsWithDebugger());

	MTextWindow::SetSaveDirectory(*fFile);
}

// ---------------------------------------------------------------------------
//		~MProjectWindow
// ---------------------------------------------------------------------------

MProjectWindow::~MProjectWindow()
{
	Lock();

	if (fDirty && fFile != nil)
		DoSave();

	if (SavePanelIsRunning())
		CloseSavePanel();

	SetPrefs();
	delete fFile;
	
	Unlock();
}

// ---------------------------------------------------------------------------
//		InitWindow
// ---------------------------------------------------------------------------

void
MProjectWindow::InitWindow()
{
	fFile = nil;
	fDirty = false;
	fSavingStatus = sNotSaving;
	fSavePanel = nil;
	fUserState = true;
	fSettingsWindow = nil;
}

// ---------------------------------------------------------------------------
//		BuildWindow
// ---------------------------------------------------------------------------

void
MProjectWindow::BuildWindow()
{
	BView*				infoView;
	BStringView*		caption;
	const BRect			bounds = Bounds();
	float				menuBarHeight = 16.0;
	float				stringWidth;

	// Build the menu bar
	BRect	r = bounds;
	r.bottom = r.top + menuBarHeight;
	BMenuBar * bar = MakeProjectMenus(r);
	AddChild(bar);
	fMenuBar = bar;
	menuBarHeight = fMenuBar->Bounds().Height();

	// Save window menu
	BMenuItem*		item = bar->FindItem(cmd_ErrorMessageWindow);
	ASSERT(item);
	fWindowMenu = item->Menu();
	ASSERT(fWindowMenu);

	// Build the top info view
	r = bounds;
	r.top = menuBarHeight + 1.0;
	r.bottom = menuBarHeight + 1.0 + kTopInfoViewHeight;
	menuBarHeight = r.bottom;
	infoView = new MMessageInfoView(r, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	AddChild(infoView);
	SetGrey(infoView, kLtGray);
	infoView->SetHighColor(kMenuDarkHilite);
	infoView->SetFont(be_bold_font);
	infoView->SetFontSize(10.0);

	// File, Code and Data Captions
	BRect	infoBounds = infoView->Bounds();
	r = infoBounds;
	r.bottom--;
	r.left = kSourceMargin;
	r.right = r.left + infoView->StringWidth("File") + 5.0;
	caption = new BStringView(r, "file", "File", B_FOLLOW_LEFT | B_FOLLOW_TOP);
	infoView->AddChild(caption);
	caption->SetFont(be_bold_font);

	stringWidth = infoView->StringWidth("Code");
	r.left = infoBounds.right - B_V_SCROLL_BAR_WIDTH - (kArrowWidth + kDataWidth) - stringWidth;
	r.right = r.left + stringWidth + 5.0;
	caption = new BStringView(r, "code", "Code", B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	infoView->AddChild(caption);
	caption->SetFont(be_bold_font);

	stringWidth = infoView->StringWidth("Data");
	r.left = infoBounds.right - kArrowWidth - B_V_SCROLL_BAR_WIDTH - stringWidth;
	r.right = r.left + stringWidth + 5.0;
	caption = new BStringView(r, "data", "Data", B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	infoView->AddChild(caption);
	caption->SetFont(be_bold_font);

	// Build the project view
	r = bounds;
	r.top = menuBarHeight;
	r.right -= B_V_SCROLL_BAR_WIDTH;
	r.bottom -= B_H_SCROLL_BAR_HEIGHT;
	r.left--;
	fProjectView = new MProjectView(r, "Project List", this);
	PulseOn(fProjectView);		// kluge

	// Build the scrolling view and add the project view to it
	BScrollView 	* scrollView = new BScrollView("scroller", 
							fProjectView, B_FOLLOW_ALL_SIDES, B_WILL_DRAW, false, true, B_NO_BORDER);
	AddChild(scrollView);
	fProjectView->MoveBy(1.0, 1.0);
	fProjectView->ResizeBy(-1.0, -1.0);

	PulseOff(fProjectView);		// kluge

	// file count text area
	r = bounds;
	r.top = r.bottom - B_H_SCROLL_BAR_HEIGHT + 1.0;
	r.right = kFileCountWindowSize;
		
	// file count popup window
	fFileCountCaption = new MProjectInfoView(*this, r, "", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	AddChild(fFileCountCaption);
	SetGrey(fFileCountCaption, kMenuBodyGray);
	// status bar at bottom will no longer be bold
	// fFileCountCaption->SetFont(be_bold_font);

	// build status text area
	r = bounds;
	r.top = r.bottom - B_H_SCROLL_BAR_HEIGHT + 1.0;
	r.left = kFileCountWindowSize+1;
	r.right -= B_V_SCROLL_BAR_WIDTH;

	// building status window
	fStatusCaption = new MStatusInfoView(r, "", "", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	AddChild(fStatusCaption);
	SetGrey(fStatusCaption, kLtGray);
	
	EnableItem(bar, cmd_SaveCopyAs, true);

	// Disable all these edit menu items
	EnableItem(fMenuBar, cmd_Undo, false);
	EnableItem(fMenuBar, cmd_Cut, false);
	EnableItem(fMenuBar, cmd_Paste, false);
	EnableItem(fMenuBar, cmd_Clear, false);

	AddCommonFilter(new MProjectKeyFilter(this, kBindingGlobal));
	AddCommonFilter(new MWindowMenuFilter);

	// These should really be among the settable key bindings
	BMessage*		msg;
	msg = new BMessage(msgCommandRightArrow);
	AddShortcut(B_RIGHT_ARROW, 0L, msg, fProjectView);	// cmd-right arrow
	msg = new BMessage(msgCommandLeftArrow);
	AddShortcut(B_LEFT_ARROW, 0L, msg, fProjectView);	// cmd-left arrow

	fProjectView->MakeFocus();
	
	SetSizeLimits(kMinHoriz, 500.0, kMinVert, 2000.0);
	SetZoomLimits(kProjZoomWidth, 2000.0);

	// build the project settings window
	// Set the real title based on the project name + "Settings"
	BString title(this->Title());
	title += kSettingsWindowString;
	fSettingsWindow = new MProjectSettingsWindow(title.String(), *this);
}

// ---------------------------------------------------------------------------
//		BuildProject
// ---------------------------------------------------------------------------

void
MProjectWindow::BuildProject()
{
	status_t	err = fFile->Open();
	err = fFile->ResetRead();
	ASSERT(B_NO_ERROR == err);
	fProjectView->ReadFromFile(*fFile);
}

// ---------------------------------------------------------------------------
//		QuitRequested
// ---------------------------------------------------------------------------

bool
MProjectWindow::QuitRequested()
{
	bool	okToClose = false;

	if (! fProjectView->IsIdle())
	{
		// While I'd rather not have "Force Close" be the default button, I don't want escape to
		// be "Force Close" either, so "Force Close" is the "OK", and "Don't Force" is cancel.
		MWarningAlert alert("The project is compiling, force it to close?", "Force Close", "Don't Force");
		if (alert.Go() == kOKButton)
		{
			fProjectView->Cancel();
			okToClose = true;
		}
	}
	else {
		if (fDirty) {
			this->DoSave();
		}
		okToClose = fSettingsWindow->OKToQuit();
	}
		
	if (okToClose)
	{
		// close down the settings window
		// We can get to this point either because 
		// the setting window replied OKToQuit, or because we
		// are forcing closed the project.  Just grab the lock
		// and quit the window.
					
		if (fSettingsWindow->Lock()) {
			// ignore result of QuitRequested (but allow it to clean up)
			fSettingsWindow->QuitRequested();
			fSettingsWindow->Quit();
			fSettingsWindow = nil;
		}
		
		// close down the error message window
		MMessageWindow* projectErrorMessageWindow = fProjectView->GetErrorMessageWindow();
		if (projectErrorMessageWindow->Lock()) {
			projectErrorMessageWindow->QuitRequested();
			projectErrorMessageWindow->Quit();
		}
	
		// let the dynamic menu handler know we are closing down
		// and take care of the save file panel

		MDynamicMenuHandler::ProjectClosed(this);
		if (this->SavePanelIsRunning()) {
			this->CloseSavePanel();
		}
	}

	return okToClose;
}

// ---------------------------------------------------------------------------
//		WindowActivated
// ---------------------------------------------------------------------------
//	Workaround for the problems with cursors.

void
MProjectWindow::WindowActivated(
	bool inActive)
{
	BWindow::WindowActivated(inActive);
	if (inActive) {
		// Tell the IDEApp that this project is now on top...
		BMessage msg(msgProjectActivated);
		msg.AddPointer(kProjectMID, this);
		be_app_messenger.SendMessage(&msg);
			
		MLocker<BLocker> lock(CursorLock);
		if (lock.IsLocked()) {
			if (be_app->IsCursorHidden())		// just paranoid
				be_app->ShowCursor();
			be_app->SetCursor(B_HAND_CURSOR);
		}
	}
}

// ---------------------------------------------------------------------------
//		MessageReceived
// ---------------------------------------------------------------------------

void
MProjectWindow::MessageReceived(
	BMessage * inMessage)
{
	switch (inMessage->what)
	{
		case cmd_SaveCopyAs:
			{
				String		name(Title());
				name += ".copy";
				ShowSavePanel(name);
				fSavingStatus = sSavingAs;
			}
			break;

		case cmd_Close:
			PostMessage(B_QUIT_REQUESTED);
			break;

		case cmd_CreateGroup:
		case cmd_SortGroup:
			if (this->OKToModifyProject() == false) {
				// don't allow group creation/sorting on locked projects
				break;
			}
			// Fall through...
		case cmd_RevealInFinder:
		case cmd_OpenCurrent:
			fProjectView->MessageReceived(inMessage);
			break;

		case cmd_Save:
			if (fDirty)
				DoSave();
			break;

		case CMD_LINK:
			Activate();
			fProjectView->GetErrorMessageWindow()->ClearMessages();	
			// Fall through
		case cmd_Compile:
		case cmd_BringUpToDate:
		case cmd_Make:
		case cmd_Precompile:
		case cmd_Preprocess:
		case cmd_CheckSyntax:
		case cmd_Disassemble:
			// make sure we can write to the project before proceeding
			if (this->OKToModifyProject()) {
				fProjectView->BuildAction(inMessage->what);
			}
			break;

		case msgMakeAndReply:
			// make sure we can write to the project before proceeding
			if (this->OKToModifyProject()) {
				this->BuildProjectAndReply(*inMessage);
			}
			break;

		case cmd_Cancel:
			fProjectView->Cancel();
			break;
		
		case cmd_SelectAll:
			fProjectView->SelectAllLines();
			break;

		case cmd_ClearSelection:
			fProjectView->ClearSelection();
			break;
		
		case cmd_Copy:
			fProjectView->DoCopy();
			break;

		case cmd_ProjectSettings:
			this->DoProjectSettings();
			break;
				
		case cmd_Run:
		case cmd_RunOpposite:
			// you wouldn't think that running would cause a project write, but
			// it does a build first, so don't allow it if the project is locked
			if (this->OKToModifyProject()) {
				// If the project doesn't run with the debugger just pass along 
				// the command, else pass along the opposite command
				if (! fProjectView->RunsWithDebugger())
					fProjectView->BuildAction(inMessage->what);
				else
					fProjectView->BuildAction((cmd_Run + cmd_RunOpposite) - inMessage->what);			
			}
			break;

		case cmd_RemoveBinaries:
		case cmd_RemoveBinariesCompact:
			if (this->OKToModifyProject()) {
				fProjectView->RemoveObjects(inMessage->what);
			}
			break;

		case cmd_ResetFilePaths:
			if (fFile)
				fProjectView->ResetFilePaths();
			break;

		case cmdSetDirty:
			fDirty = true;
			break;

		// Window menu
		case msgWindowMenuClicked:
		case msgOpenRecent:
		case cmd_Stack:
		case cmd_Tile:
		case cmd_TileVertical:
			be_app_messenger.SendMessage(inMessage);
			break;
		
		// Hey, this is me
		case cmd_ShowProjectWindow:
			this->ShowProjectWindow(inMessage);
			break;
		
		// We know how to do this here
		case cmd_ErrorMessageWindow:
			fProjectView->GetErrorMessageWindow()->PostMessage(inMessage);
			break;

		// Short cuts for next/previous error
		case cmd_PrevMessage:
		case cmd_NextMessage:
			// IDEApp knows how to handle these, this just reduces depedencies
			be_app_messenger.SendMessage(inMessage);
			break;
			
		// Edit menu
		case cmd_FindDocumentation:
			be_app_messenger.SendMessage(msgFindDocumentation);
			break;
			
		// From a text window
		case msgCompileOne:
		case msgPreCompileOne:
		case msgPreProcessOne:
		case msgCheckSyntaxOne:
		case msgDisassembleOne:
			if (this->OKToModifyProject()) {
				fProjectView->BuildAction(*inMessage);
			}
			break;

		case msgAddToProject:
			if (this->OKToModifyProject()) {
				fProjectView->AddToProject(*inMessage);
			}
			break;

		case msgSaveAsForFile:
			if (this->OKToModifyProject()) {
				fProjectView->SaveAsForSourceFile(*inMessage);
			}
			break;

		case msgFindDefinition:
			fProjectView->FindDefinition(*inMessage);
			break;

		// From the set section name dialog
		case msgSetName:
			fProjectView->SetSectionName(*inMessage);
			break;

		// From the compiler generator object
		case msgCompileDone:
			fProjectView->CompileDone();
			break;

		case msgOneCompileDone:
			fProjectView->OneCompileDone(*inMessage);
			fDirty = true;
			break;

		case cmd_RemoveFiles:
			if (this->OKToModifyProject()) {
				fProjectView->RemoveSelection();
			}
			break;

		// From the project view
		case msgSetCaptionText:
			SetCaptionText(*inMessage);
			break;

		case msgSetStatusText:
			SetStatusText(*inMessage);
			break;

		case msgAllFilesFound:
		{
			BMessage		msg(msgProjectOpened);
			msg.AddPointer(kProjectMID, (MProjectWindow*) this);
			MDynamicMenuHandler::MessageToAllTextWindows(msg);
		}
			break;

		// From the app
		case msgAddFiles:
			if (this->OKToModifyProject()) {
				fProjectView->AddFilesMessage(*inMessage);
			}
			break;

		case cmd_AddFiles:
			be_app_messenger.SendMessage(msgRequestAddFiles);
			break;

		// Header popup menu
		case msgOpenSourceFile:
			be_app_messenger.SendMessage(inMessage);		// The app will open any windows
			break;

		case msgTouchFile:
			fProjectView->TouchFile(*inMessage);
			break;

		case msgFindDefinitionClosed:
			fProjectView->MessageReceived(inMessage);
			break;

		case cmd_EnableDebugging:
			if (this->OKToModifyProject()) {
				UpdateRunMenuItems(!fProjectView->RunsWithDebugger());	// toggle values
				fProjectView->RunMenuItemChanged();
				fDirty = true;
			}
			break;
		
		// Special
		case cmd_AndyFeature:
			DoAndyFeature();
			break;

		case msgUpdateMenus:
			UpdateProjectMenus(fMenuBar);
			break;

		// Hilite color changed
		case msgColorsChanged:
 			fProjectView->HiliteColorChanged();
			break;

		case msgBuildExtrasChanged:
			this->SetBuildParameters(*inMessage);
			break;
			
		// The save panel
		case B_SAVE_REQUESTED:
		{
			entry_ref		dir;
			inMessage->FindRef("directory", &dir);

			const char *	name = NULL;
			inMessage->FindString("name", &name);

			SaveRequested(&dir, name);
			CloseSavePanel();
			fProjectView->NewProjectSaved();
			break;
		}

		default:
			if (! SpecialMessageReceived(*inMessage, this) &&
				! TryScriptMessage(inMessage, this, sProjectVerbs, -1))
				BWindow::MessageReceived(inMessage);
			break;
	}
}

// ---------------------------------------------------------------------------

void
MProjectWindow::BuildProjectAndReply(BMessage& message)
{
	// Make sure we aren't building when we get here...
	if 	(fProjectView->IsIdle()) {
		fProjectView->BuildAction(message);
	}
	else {
		// tell the client that I'm busy right now
		BMessage reply;
		reply.AddInt32("status", B_BUSY);
		message.SendReply(&reply);		
	}
	
}

// ---------------------------------------------------------------------------
//		Zoom
// ---------------------------------------------------------------------------
//	Override the BWindow version so we can move the window flushright.

void
MProjectWindow::Zoom(
	BPoint			inPosition,
	float 			inWidth,
	float 			inHeight)
{
	// Calculate the system state rect
	BRect		windFrame = Frame();
	BRect		frame = fProjectView->Frame();
	BPoint		topLeft(0.0, 0.0);
	fProjectView->ConvertToScreen(&topLeft);
	fProjectView->ConvertToScreen(&frame);
	
	const float		contentHeight = fProjectView->GetMaxYPixel() - (topLeft.y - frame.top) +
			fMenuBar->Bounds().Height() + B_H_SCROLL_BAR_HEIGHT + kTopInfoViewHeight;
	
	BScreen			screen(this);
	BRect			screenFrame = screen.Frame();

	inPosition.x = screenFrame.right - kProjZoomWidth - kBorderWidth;
	inPosition.y = kProjZoomTop;
	inWidth = kProjZoomWidth;
	if (contentHeight < screenFrame.Height())
		inHeight = contentHeight;
	else
		inHeight = screenFrame.Height() - kBorderWidth - kProjZoomTop;

	BRect		sysFrame(inPosition.x, inPosition.y, inPosition.x + inWidth, inPosition.y + inHeight);

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
//		GetSubHandler
// ---------------------------------------------------------------------------

ScriptHandler *
MProjectWindow::GetSubHandler(
	const char *			propertyName,
	EForm					form,
	const SData &			data)
{
	ScriptHandler *		subHandler = nil;
	int 				propCount = -1;

	if (0 == strcmp(propertyName, "property") && (form == formIndex)) {
		propCount = data.index;
	}

	if (0 == strcmp(propertyName, "messenger") || (0 == --propCount)) 
	{
		subHandler = new MessengerHandler("BeIDE", this);
	}
	else
	if (0 == strcmp(propertyName, kFilesProp) || (0 == --propCount)) 
	{
		switch (form) 
		{
			case formDirect:
			{
				BList*		list = new BList(50);
				fProjectView->FillFileList(list, kSourceFilesRefs);
				subHandler = new MEntryRefHandler(Title(), list);
				break;
			}
			default:	// Other forms are not supported
				break;
		}
	}
	else
	if (0 == strcmp(propertyName, kFileProp) || (0 == --propCount)) 
	{
		MSourceFileLine*	line = nil;
		bool				good = true;
		int32				index = -1;

		switch (form) 
		{
			case formIndex:
				if (data.index >= 1 && data.index <= fProjectView->FileCount())
				{
					index = data.index - 1;
					line = fProjectView->GetLineByIndex(index);
				}
				else
					good = false;
				break;
			case formFirst:
				index = 0;
				line = fProjectView->GetLineByIndex(0);
				break;
			case formLast:
				index = fProjectView->CountRows();
				line = fProjectView->GetLineByIndex(index);
				break;

			case formDirect:
//				subHandler = fProjectView;
				break;

			case formName:
				good = fProjectView->GetSourceFileLineByName(data.name, line);
				if (good)
					index = fProjectView->fFileList.IndexOf((MProjectLine*) line);
				else
				{
					// Is it a path?
					const char *	ptr = strrchr(data.name, '/');
					if (ptr != nil)
					{
						entry_ref		ref;
						if (B_NO_ERROR == get_ref_for_path(data.name, &ref))
						{
							good = fProjectView->GetSourceFileLineByRef(ref, line);
							if (good)
								index = fProjectView->fFileList.IndexOf((MProjectLine*) line);					
						}
					}
				}
				break;

			// These forms are not supported			
			case formReverseIndex:
			case formID:
			default:
				good = false;
				break;
		}
		
		if (good)
		{
			subHandler = new MProjectFileHandler(line, *fProjectView, index);
		}
	}
	else
	if (0 == strcmp(propertyName, kRefProp) || (0 == --propCount)) 
	{
//		subHandler = new MEntryRefHandler(Title(), GetRecordRef());
	}
	else
	if (propCount > 0) {
		SData 	newData;
		newData.index = propCount;
		subHandler = ScriptHandler::GetSubHandler(propertyName, formIndex, newData);
	}

	if (subHandler)
		return subHandler;
	else
		return ScriptHandler::GetSubHandler(propertyName, form, data);
}


// ---------------------------------------------------------------------------
//		PerformScriptAction
// ---------------------------------------------------------------------------

status_t
MProjectWindow::PerformScriptAction(
	BMessage *			message,
	BMessage * &		reply,
	bool&				wasDeferred)
{
	int32		result;

	switch (message->what) 
	{
		case kMakeProject:
		case kCreateVerb:
			result = fProjectView->PerformScriptAction(message, reply, wasDeferred);
			break;

		case kCloseVerb:
//			return DoClose(message, reply);
			break;

		default:
			result = SCRIPT_BAD_VERB;
			break;
	}

	return result;
}

// ---------------------------------------------------------------------------
//		MenusBeginning
// ---------------------------------------------------------------------------
//	Adjust the active/inactive state of the menu items.  The default is 
//	activated so only need to modify those items that might be inactive.

void
MProjectWindow::MenusBeginning()
{
	BMenuBar*		bar = fMenuBar;
	
	// Set up the menu items to match the current modifier keys, if any
	UpdateMenuItemsForModifierKeys(bar);

	//	Can't compile if we already are
	bool			isIdle = fProjectView->IsIdle();
	bool			isApplication = fProjectView->IsRunnable();
	bool			hasFiles = fProjectView->FileCount() > 0;
		
	EnableItem(bar, cmd_BringUpToDate, isIdle && hasFiles);
	EnableItem(bar, cmd_Make, isIdle && hasFiles);
	EnableItem(bar, CMD_LINK, isIdle && hasFiles);
	EnableItem(bar, cmd_RemoveBinaries, isIdle);
	EnableItem(bar, cmd_ResetFilePaths, isIdle);
	EnableItem(bar, cmd_AddFiles, isIdle);

	// Can't run if we're compiling or aren't an application

	EnableItem(bar, cmd_RunOpposite, isApplication && isIdle);
	EnableItem(bar, cmd_Run, isApplication && isIdle);
	EnableItem(bar, cmd_EnableDebugging, isApplication && isIdle);	

	BMenu*		menu = bar->SubmenuAt(kProjectMenuIndex);
	BMenuItem*	runitem = menu->FindItem(cmd_Run);
	BMenuItem*	runoppositeitem = menu->FindItem(cmd_RunOpposite);
		
	if (fProjectView->RunsWithDebugger())
	{
		if (runitem != nil)
			SetItemMessageAndName(menu, kRunID, cmd_Run, kDebug);
		else
		if (runoppositeitem)
			SetItemMessageAndName(menu, kRunID, cmd_RunOpposite, kRun);			
	}
	else
	{
		if (runitem != nil)
			SetItemMessageAndName(menu, kRunID, cmd_Run, kRun);
		else
		if (runoppositeitem)
			SetItemMessageAndName(menu, kRunID, cmd_RunOpposite, kDebug);	
	}

	// Can only copy (or clear selection) if there is a selection
	EnableItem(bar, cmd_Copy, fProjectView->CurrentSelection() >= 0);
	EnableItem(bar, cmd_ClearSelection, fProjectView->CurrentSelection() >= 0);
	
	// Can only modify sections if there is a selection - and we're not compiling
	bool			hasSelection = isIdle && (fProjectView->CurrentSelection() >= 0);

	EnableItem(bar, cmd_CreateGroup, isIdle && (fProjectView->CurrentSelection() >= 2));
	EnableItem(bar, cmd_SortGroup, hasSelection);
	EnableItem(bar, cmd_RemoveFiles, hasSelection);

	EnableItem(bar, cmd_Compile, hasSelection);			// Can only compile if there's a selection
	EnableItem(bar, cmd_Precompile, hasSelection);
	EnableItem(bar, cmd_Preprocess, hasSelection);
	EnableItem(bar, cmd_CheckSyntax, hasSelection);
	EnableItem(bar, cmd_Disassemble, hasSelection);

	// Upate the window/project list menus
	MDynamicMenuHandler::UpdateDynamicMenus(*bar, this);
}

// ---------------------------------------------------------------------------
//		UpdateRunMenuItems
// ---------------------------------------------------------------------------

void
MProjectWindow::UpdateRunMenuItems(
	bool	inRunWithDebugger)
{
	BMenu*		menu = fMenuBar->SubmenuAt(kProjectMenuIndex);

	if (inRunWithDebugger)
	{
		SetItemMessageAndName(menu, kEnableDebuggerID, cmd_EnableDebugging, kDisableDebugger);
		SetItemMessageAndName(menu, kRunID, cmd_RunOpposite, kDebug);
		fProjectView->RunWithDebugger(true);
	}
	else
	{
		SetItemMessageAndName(menu, kEnableDebuggerID, cmd_EnableDebugging, kEnableDebugger);
		SetItemMessageAndName(menu, kRunID, cmd_Run, kRun);
		fProjectView->RunWithDebugger(false);	
	}
}

// ---------------------------------------------------------------------------
//		CurrentRunMenuCommand
// ---------------------------------------------------------------------------

uint32
MProjectWindow::CurrentRunMenuCommand()
{
	BMenu*			menu = fMenuBar->SubmenuAt(kProjectMenuIndex);
	BMenuItem*		item = menu->ItemAt(kRunID);

	return item->Command();
}

// ---------------------------------------------------------------------------
//		RunsWithDebugger
// ---------------------------------------------------------------------------
//	Accessor for MProject.

bool
MProjectWindow::RunsWithDebugger()
{
	return fProjectView->RunsWithDebugger();
}

// ---------------------------------------------------------------------------
//		NewProjectCreated
// ---------------------------------------------------------------------------
//	Called from the application object just after creating a new project 
//	window so that the user can immediately save the project file.

void
MProjectWindow::NewProjectCreated()
{
	MLocker<BWindow>	lock(this);

	// Validate (or setup) all the default preferences for empty project
	fProjectView->ValidateGenericData();
	DoSave();
}

// ---------------------------------------------------------------------------

void
MProjectWindow::SetBuildParameters(const BuildExtrasPrefs& buildparams)
{
	// Called from the application object when the build parameters
	// change (or project opened).  This just sets transient state 
	// in the project unlike the other preferences that are saved in the project
	
	fProjectView->SetBuildParameters(buildparams);
}

// ---------------------------------------------------------------------------

void
MProjectWindow::SetBuildParameters(const BMessage& inMessage)
{
	// Alternative for when messenging needs to be used
	
	BuildExtrasPrefs* buildExtras;
	int32 length;
	if (inMessage.FindData(kBuildExtrasPrefs, kMWPrefs, (const void**) &buildExtras, &length) == B_OK) {
		fProjectView->SetBuildParameters(*buildExtras);
	}
}

// ---------------------------------------------------------------------------
//		SavePanelIsRunning
// ---------------------------------------------------------------------------
//	Is it visible?

bool
MProjectWindow::SavePanelIsRunning()
{
	return fSavePanel != nil;
}

// ---------------------------------------------------------------------------
//		CloseSavePanel
// ---------------------------------------------------------------------------
//	Close it and get all its info.

void
MProjectWindow::CloseSavePanel()
{
	if (fSavePanel != nil)
	{
		delete fSavePanel;
		fSavePanel = nil;
	}
}

// ---------------------------------------------------------------------------
//		ShowSavePanel
// ---------------------------------------------------------------------------

void
MProjectWindow::ShowSavePanel(
	const char *	inSaveText)
{
	if (SavePanelIsRunning())
	{
		CloseSavePanel();
	}

	BMessenger		me(this);
	BWindow*		panel;

	fSavePanel = new BFilePanel(B_SAVE_PANEL, &me);

	panel = fSavePanel->Window();

	if (inSaveText != nil)
		fSavePanel->SetSaveText(inSaveText);

	fSavePanel->Show();
}

// ---------------------------------------------------------------------------
//		SaveRequested
// ---------------------------------------------------------------------------
//	The save panel requested that we save so we need to create the file
//	object and specify it.

void
MProjectWindow::SaveRequested(
	entry_ref* 		directory,
	const char *	name)
{
	BDirectory		dir(directory);	// Specify directory
	status_t		err;

	switch (fSavingStatus)
	{
		case sSaving:
		{
			BFile			file;
			fFile = new MBlockFile;			// Create file object
			err = dir.CreateFile(name, &file);		// Specify file object and create it on disk

			err = dir.FindEntry(name, fFile);

			SetTitle(name);
			ScriptHandler::SetName(name);	// Set the scripthandler's name
			
			BNodeInfo	mimefile(&file);
			
			err = mimefile.SetType(kProjectMimeType);
			
			err = fFile->Open();
			if (B_NO_ERROR == err) 
			{
				// Initialize the project Directory
				status_t	err = fFile->GetParent(&fProjectView->GetProjectDirectory());
				ASSERT(err == B_NO_ERROR);

				fProjectView->SetWorkingDirectory();
				DoSave();		// Really save it
				entry_ref		projectref;
				
				err = fFile->GetRef(&projectref);
				fProjectView->SetProject(projectref);
			}
		}
			break;

		case sSavingAs:
		{
			BFile			file;
			MBlockFile		blockfile;			// Local file object
	
			err = dir.CreateFile(name, &file);	// Specify file object and create it on disk
			err = dir.FindEntry(name, &blockfile);
			BNodeInfo	mimefile(&file);
			
			err = mimefile.SetType(kProjectMimeType);

			err = blockfile.Open();
			if (B_NO_ERROR == err) 
			{
				MBlockFile*		temp = fFile;
				fFile = &blockfile;

				DoSave(name);				//Save it

				blockfile.Close();
				fFile = temp;
			}
		}
			break;

		default:
			ASSERT(false);
			break;
	}
	
	fSavingStatus = sNotSaving;
}

// ---------------------------------------------------------------------------
//		DoSave
// ---------------------------------------------------------------------------
//	Save the project file.

void
MProjectWindow::DoSave(
	const char *	inName)
{
	if (fFile)
	{
		// If we are saving the current project, 
		// and the project is locked...
		// it is a bug that we got here, but allow the user to back out
		if (inName == NULL && this->OKToModifyProject() == false) {
			return;
		}
		
		status_t	err;

		// Allocate a temp file in the ~/tmp directory
		MBlockFile		temp;
		auto_Entry		autoE(&temp);
		MFileUtils::GetTempFile(temp, fFile);

		err = temp.Open();
		if (err == B_NO_ERROR)
			err = temp.ResetWrite();
		if (err == B_NO_ERROR)
		{
			try {

				fFile->Open();

				fProjectView->WriteToFile(temp);

				err = CopyAttrs(*fFile->File(), *temp.File());


			} catch (int32 inErr) {
				err = inErr;			// Don't propagate the exception
			}

			err = temp.Close();
			fFile->Close();

			if (err == B_NO_ERROR)
			{
				time_t			creationTime;
				BDirectory		parentDir;
		
				if (B_NO_ERROR == fFile->GetCreationTime(&creationTime))
					temp.SetCreationTime(creationTime);
		
				err = fFile->GetParent(&parentDir);

				sync();		// make sure the file is saved to disk

				if (inName == nil)
					inName = Title();
				if (err == B_NO_ERROR)
					err = temp.MoveTo(&parentDir, inName, true);

				if (err == B_NO_ERROR)
				{
					fDirty = false;
					autoE.release();	// Don't remove the file
				}
			}
		}

		if (err != B_NO_ERROR)
		{
			String		text = "An error occurred while saving the project file.\n";

			text += err;

			MAlert		alert(text);
			alert.Go();
		}
	}
}

// ---------------------------------------------------------------------------
//		DoAndyFeature
// ---------------------------------------------------------------------------
//	Respond to alt-tab in this window.  Ask the app to do the work.
//	Can't do anything for an unsaved file.

void 
MProjectWindow::DoAndyFeature()
{
	fProjectView->DoAndyFeature();
}

// ---------------------------------------------------------------------------
//		FileIsInProjectByFile
// ---------------------------------------------------------------------------
//	Check if a specified file is already in the project.
//	Lock the window before calling this function.

bool
MProjectWindow::FileIsInProjectByFile(
	BEntry* 	inFile) const
{
	return fProjectView->FileIsInProjectByFile(inFile);
}

// ---------------------------------------------------------------------------
//		FileIsProjectFile
// ---------------------------------------------------------------------------
//	Check if a specified file is the project file.  Called from the app to
//	determine if a project file to be opened is already open.

bool
MProjectWindow::FileIsProjectFile(
	const BEntry &	inRef) const
{
	bool			result = false;
	
	if (fFile != nil)
	{		
		if (*fFile == inRef)
			result = true;
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//		FileIsProjectFile
// ---------------------------------------------------------------------------
//	Check if a specified file is the project file.  Called from the app to
//	determine if a project file to be opened is already open.

bool
MProjectWindow::FileIsProjectFile(
	const entry_ref &	inRef) const
{
	bool			result = false;
	
	if (fFile != nil)
	{
		entry_ref		projref;
		
		if (B_NO_ERROR == fFile->GetRef(&projref) && projref == inRef)
			result = true;
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//		OpenSelection
// ---------------------------------------------------------------------------

bool
MProjectWindow::OpenSelection(
	const char * 	inFileName,
	bool			inSystemTree,
	entry_ref&		outRef,
	bool			inSearchInMemory,
	bool			inSearchOnDisk)
{
	return fProjectView->OpenSelection(inFileName, inSystemTree, outRef, inSearchInMemory, inSearchOnDisk);
}

// ---------------------------------------------------------------------------
//		BuildPopupMenu
// ---------------------------------------------------------------------------
//	Build the header popup menu for the specified window.

void
MProjectWindow::BuildPopupMenu(
	const char *	inName,
	MPopupMenu& 	inPopup)
{
	MLocker<BWindow>	lock(this);
	fProjectView->BuildPopupMenu(inName, inPopup);
}

// ---------------------------------------------------------------------------
//		FillFileList
// ---------------------------------------------------------------------------

void
MProjectWindow::FillFileList(
	MSourceFileList&	inList, 
	SourceListT			inKind)
{
	MLocker<BWindow>	lock(this);
	fProjectView->FillFileList(inList, inKind);
}

// ---------------------------------------------------------------------------

void
MProjectWindow::SetCaptionText(BMessage& inMessage)
{
	// Set the text of the file count window
	this->SetInfoText(inMessage, fFileCountCaption);
}

// ---------------------------------------------------------------------------

void
MProjectWindow::SetStatusText(BMessage& inMessage)
{
	// Set the text of the status window
	this->SetInfoText(inMessage, fStatusCaption);
}

// ---------------------------------------------------------------------------

void
MProjectWindow::SetInfoText(BMessage& inMessage, BStringView* captionWindow)
{
	//	Set the text of the caption window
	const char* text = inMessage.FindString(kText);
	if (text) {
		captionWindow->SetText(text);
		captionWindow->Invalidate();
		UpdateIfNeeded();
	}
}

// ---------------------------------------------------------------------------
//		GetData
// ---------------------------------------------------------------------------
//	Fill the BMessage with preferences data of the kind specified in the
//	BMessage.

void
MProjectWindow::GetData(
	BMessage&	inOutMessage)
{
	MLocker<BWindow>	lock(this);
	fProjectView->GetData(inOutMessage);
}

// ---------------------------------------------------------------------------
//		SetData
// ---------------------------------------------------------------------------
//	Get the preferences from the message and update the correct prefs struct.

void
MProjectWindow::SetData(
	BMessage&	inOutMessage)
{
	MLocker<BWindow>	lock(this);
	fProjectView->SetData(inOutMessage);
}

// ---------------------------------------------------------------------------

void
MProjectWindow::GetPrefs()
{
	// Get the frame
	// Historically, the BeIDE would write out big-endian (kFramePrefsName) for
	// PPC and little-endian (kFramePrefsNameLE) for x86.  However, zip/unzip
	// handle the swapping of bytes for known B_TYPES (like B_RECT_TYPE).
	// So we don't have do to any swapping here.  In fact if we do, then we 
	// double swap and end up with bad frames
	
	if (fFile != nil && fFile->Exists()) {
		BFile file(fFile, B_READ_ONLY);
		BRect frame;

		ssize_t size = file.ReadAttr(kFramePrefsName, B_RECT_TYPE, 0, &frame, sizeof(frame));
		// unfortunately we need to support the existance of kFramePrefsNameLE
		// so we don't lose all x86 frame data
		if (size <= 0) {
			size = file.ReadAttr(kFramePrefsNameLE, B_RECT_TYPE, 0, &frame, sizeof(frame));
			file.RemoveAttr(kFramePrefsNameLE);
		}
			
		if (size == sizeof(frame) && frame.IsValid()) {
			MoveTo(frame.LeftTop());
			ResizeTo(frame.Width(), frame.Height());
			ValidateWindowFrame(frame, *this);
		}
	}
}

// ---------------------------------------------------------------------------

void
MProjectWindow::SetPrefs()
{
	// Historically, the BeIDE would write big-endian on ppc and little endian on intel
	// (using different attribute names)
	// I just write it all out to "frame" because the swapping is handled by zip/unzip.
	
	if (fFile != nil && fFile->Exists()) {
		BFile file(fFile, B_WRITE_ONLY);
		BRect frame = Frame();
		
		file.WriteAttr(kFramePrefsName, B_RECT_TYPE, 0, &frame, sizeof(frame));
	}
}

// ---------------------------------------------------------------------------
//		GetRef
// ---------------------------------------------------------------------------

status_t
MProjectWindow::GetRef(
	entry_ref&	inoutRef)
{
	MLocker<BWindow>	lock(this);
	status_t			err = B_ERROR;

	if (fFile)
		err = fFile->GetRef(&inoutRef);
	
	return err;
}

// ---------------------------------------------------------------------------

void
MProjectWindow::DoProjectSettings()
{
	ASSERT(fSettingsWindow);
	fSettingsWindow->ShowAndActivate();
}

// ---------------------------------------------------------------------------

void
MProjectWindow::ShowProjectWindow(BMessage* inMessage)
{	
	bool moveToCurrentWorkspace = false;
	status_t err = B_OK;
	
	if (inMessage != nil) {
		err = inMessage->FindBool(kUseOptionKey, &moveToCurrentWorkspace);
	}
	
	if (err != B_OK || inMessage == nil) {
		moveToCurrentWorkspace = (modifiers() & (B_OPTION_KEY | B_CONTROL_KEY)) != 0;
	}
	
	if (moveToCurrentWorkspace) {
		this->SetWorkspaces(B_CURRENT_WORKSPACE);
		ValidateWindowFrame(*this);					// make sure it's onscreen
	}
	this->Activate();
}
