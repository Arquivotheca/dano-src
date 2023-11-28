// ===========================================================================
//	IDEApp.cpp
//	
//	Copyright 1995 - 96 Metrowerks Corporation, All Rights Reserved.
//
// ===========================================================================

#include <string.h>

#include "IDEApp.h"
#include "MTextWindow.h"
#include "MProjectWindow.h"
#include "MMessageWindow.h"
#include "MMessageItem.h"
#include "MFindWindow.h"
#include "MEnvironmentPreferencesWindow.h"
#include "MDefaultPrefs.h"
#include "MAlert.h"
#include "MOpenSelectionWindow.h"
#include "MOpenSelectionTask.h"
#include "MLookupDocumentationWindow.h"
#include "MLookupDocumentationTask.h"
#include "MAndyFeatureTask.h"
#include "MTargetTypes.h"
#include "MKeywordList.h"
#include "MPreferences.h"
#include "MFunctionPopup.h"
#include "MScripting.h"
#include "MScriptUtils.h"
#include "MessengerHandler.h"
#include "MKeyBindingManager.h"
#include "MNewProjectWindow.h"
#include "ProjectCommands.h"
#include "IDEConstants.h"
#include "IDEMessages.h"
#include "AboutBox.h"
#include "CString.h"
#include "Utils.h"
#include "MLocker.h"
#include "MBuildersKeeper.h"
#include "MHiliteColor.h"

#include <Autolock.h>
#include <Roster.h>
#include <priv_syscalls.h>

// #include "libprof.h"

// Important constant - counts the number of windows that always hang around
// If we change how windows work, we need to change this
// 2 = Find Window + Preferences Window
const int kPersistentWindowCount = 2;

// static lock for cursor manipulations
BLocker CursorLock("cursorlock");

void MyNewHandler();

#include <new.h>
void MyNewHandler()
{
	struct mstats	stats = mstats();

	printf("BeIDE Out of Memory!!, heap size = %d\n", stats.bytes_total);
	throw -108;
}

const size_t MB = 1024 * 1024;

// ---------------------------------------------------------------------------
//		 main
// ---------------------------------------------------------------------------

int
main()
{
	// Each project chews up around 10 file descriptors
	// Also, during the recurse of access directories it temporary uses a lot
	// make sure we have an ample amount
	_kset_fd_limit_(512);
	
	IDEApp 		*myApplication;
	status_t	err;

//#if ! __INTEL__
	// TEMP ????
	set_new_handler(MyNewHandler);
//#endif
	try {
		myApplication = new IDEApp;
		myApplication->Run();
	} catch(...)
	{
	}
	
	delete myApplication;

	// This ought to go in the destructor but
	// there seem to be some cases of windows still 
	// existing at that point
	err = MPreferences::ClosePreferences();
	ASSERT(err == B_NO_ERROR);

	return 0;
}


// ---------------------------------------------------------------------------
//		 IDEApp
// ---------------------------------------------------------------------------
//	Constructor

IDEApp::IDEApp()
	: 	BApplication(kIDESigMimeType),
		ScriptHandler(NewScriptID(), "BeIDE", this),
		fPrefsLock("appprefslock"),
		fProjectListLock("IDEAPP_project_lock")
{
	status_t		err;
	err = MPreferences::OpenPreferences("BeIDE_Prefs");
	ASSERT(err == B_NO_ERROR);

	fCurrentProject = nil;
	fLastDirectoryGood = false;
	fUntitledCount = 0;
	fPreferencesWindow = nil;
	fQuitting = false;
	fOpenSelectionWind = nil;
	fLookupDocumentationWind = nil;
	fLookupDocumentationTask = nil;
	fFilePanelStatus = opNothing;
	fAccessPathsView = nil;
	fPrintSettings = nil;
	fFilePanel = nil;
	fNewProjectWindow = nil;
	fNewProjectPanel = nil;
	fAbout = nil;

	// If the option key is down on launch then we remove all
	// IDE preferences from the prefs file. 
	if ((modifiers() & B_OPTION_KEY) != 0) {
		MAlert alert("Remove all BeIDE environment preferences?", "Remove", "Cancel");
		if (alert.Go() == kOKButton) {
			RemoveAllPreferences();
		}
	}
	
	GetPrefs();								// needs to be done before we build any windows

	fPreferencesWindow = new MEnvironmentPreferencesWindow;
	fFindWindow = new MFindWindow;

	fBuilderKeeper = new MBuildersKeeper;	// needs to be done after message window
	
	MTextWindow::RememberSelections(fAppEditorPrefs.pRememberSelection);
	MTextWindow::ScanForEditAddOns();
	STEngine::Setup();
	
	MKeywordList::InitLists();
	MSyntaxStyler::Init();
	MIDETextView::Init();
}

// ---------------------------------------------------------------------------
//		 ~IDEApp
// ---------------------------------------------------------------------------
//	Destructor

IDEApp::~IDEApp()
{
	SetPrefs();
	MTextWindow::ShutDownEditAddOns();
	stop_watching(be_app_messenger);
	delete fBuilderKeeper;
}

// ---------------------------------------------------------------------------
//		 ReadyToRun
// ---------------------------------------------------------------------------

void
IDEApp::ReadyToRun()
{
	// Allow for any message windows that might have been opened because of errors
	if (CountWindows() == kPersistentWindowCount + MMessageWindow::MessageWindowCount()) {
		DoCmdNew();
	}
}

// ---------------------------------------------------------------------------
//		 MessageReceived
// ---------------------------------------------------------------------------
// Respond to messages from the main menu.

void
IDEApp::MessageReceived(
	BMessage * message)
{
	switch (message->what)
	{
		// Printer changed
		case B_PRINTER_CHANGED:
			PrinterChanged();
			break;
			
		// App-wide items
		case cmd_New:
			DoCmdNew();
			break;

		case cmd_NewProject:
			DoCmdNewProject();
			break;

		case cmd_OpenFile:
			DoCmdOpen();
			break;

		case cmd_OpenSelection:
			DoOpenSelection();
			break;

		case cmd_Preferences:
			this->DoPreferences();
			break;

		// Project window
		case msgOpenSourceFile:
			this->OpenSourceFile(message);
			break;

		case msgRequestAddFiles:
			// catch locked project here rather
			// than after doing the panel work
			if (fCurrentProject && fCurrentProject->OKToModifyProject()) {
				this->DoAddFiles();
			}
			break;

		case msgFindDocumentation:
			this->HandleFindDocumentation(*message);
			break;
				
		// Text window
		case msgPreProcessOne:
		case msgPreCompileOne:
		case msgCheckSyntaxOne:
		case msgDisassembleOne:
		case msgAddToProject:
		case msgAddFiles:				// Add files Panel
		case cmd_Make:					// Any of the special windows
		case cmd_BringUpToDate:			// Any of the special windows
		case cmd_Run:					// Any of the special windows
		case cmd_RunOpposite:			// Any of the special windows
			ASSERT(fCurrentProject);
			if (fCurrentProject) {
				fCurrentProject->PostMessage(message);
			}
			break;

		case cmd_Cancel:
			// If we are doing a documentation lookup,
			// assume the cancel is for that, otherwise
			// assume the cancel is for the current build
			if (fLookupDocumentationTask) {
				fLookupDocumentationTask->Cancel();
				this->FindDocumentationDone();
			}
			else if (fCurrentProject) {
				fCurrentProject->PostMessage(message);
			}
			break;
			
		case msgFileSaved:
			if (fCurrentProject) {
				this->HandleFileSaved(*message);
			}
			break;

		case msgWindowMenuClicked:
		case msgOpenRecent:
		case cmd_Stack:
		case cmd_Tile:
		case cmd_TileVertical:
			MDynamicMenuHandler::MenuClicked(*message);
			break;

		case cmd_CloseAll:
			MDynamicMenuHandler::CloseAllTextWindows();
			break;

		case cmd_SaveAll:
			MDynamicMenuHandler::SaveAllTextWindows();
			break;

		case msgDoOpenSelection:
			HandleOpenSelection(*message);
			break;

		case cmd_AndyFeature:
			HandleAndyFeature(*message);
			break;

		// A preprocess or disassemble compile object
		case msgRawTextWindow:
			OpenRawTextWindow(*message);
			break;

		// Open selection window
		case msgOpenSelectionClosed:
			if (fOpenSelectionWind->Lock())
				fOpenSelectionWind->Quit();
			fOpenSelectionWind = nil;
			break;

		case msgFindDocumentationClosed:
			if (fLookupDocumentationWind->Lock()) {
				fLookupDocumentationWind->Quit();
			}
			fLookupDocumentationWind = nil;
			break;
		
		// New Project window
		case msgNewProjectClosed:
			if (fNewProjectWindow->Lock())
				fNewProjectWindow->Quit();
			fNewProjectWindow = nil;
			this->AppActivated(false);
			break;

		case msgCreateProject:
			// If the new project panel is the only window left,
			// the user can cancel it and quit the BeIDE
			// However, if we are in the middle of project creation
			// we want to not worry about quiting
			// fNewProjectPanelDoingWork will only be true from the
			// msgSaveProject until the cancel - which is where it is
			// needed.
			fNewProjectPanelDoingWork = false;
			fNewProjectPanel = MNewProjectWindow::CreateProject(message);
			break;

		case msgSaveProject:
			fNewProjectPanelDoingWork = true;
			MNewProjectWindow::SaveProject(message);
			break;

		case msgCreateEmptyProject:
			BuildEmptyProject(message);
			break;
		
		case msgOpenProjectAndReply:
			this->OpenProjectAndReply(*message);
			break;
					
		case B_CANCEL:
			delete fNewProjectPanel;
			fNewProjectPanel = nil;
			// fNewProjectPanelDoingWork allows us to quit when 
			// the user presses cancel, but not when they press save 
			// (and we still come through here as the window closes)
			if (fNewProjectPanelDoingWork == false) {
				this->AppActivated(false);
			}
			fNewProjectPanelDoingWork = false;
			break;
		
		// Find Window
		case msgStartOther:
			AddOtherFiles();
			break;

		// Open Panel
		case msgPanelOpenMessage:		// obsolete
			RefsReceived(message);
			break;

		case msgAddOtherFile:
			MFindWindow::GetFindWindow().PostMessage(message);
			break;

		case msgFilePanelRefsReceived:
			RefsReceived(message);
			CloseFilePanel();
			break;

		// About box
		case cmd_About:
			AboutRequested();
			break;

		case BYE_BYE_BOX:
			fAbout = nil;
			this->AppActivated(false);
			break;

		// Message Window
		case cmd_PrevMessage:
		case cmd_NextMessage:
			MMessageWindow::MessageToMostRecent(*message);
			break;
			
		// Window list all gone
		// The project window closed or
		// a project-wide window hidden (find, preference, message...)
		// In the case of msgProjectClosed if we are switching projects
		// this works here because we are doing the close/post/open in
		// a single thread, so we don't get back here to handle the
		// message until the new project is open
		case msgWindowsAllGone:
		case msgProjectClosed:
		case msgFindWindowHidden:
		case msgPreferencesWindowHidden:
		case msgProjectWindowClosed:
		case msg_MessageWindowsAllGone:
			this->AppActivated(false);
			break;
	
		case msgProjectActivated:
			if (message->HasPointer(kProjectMID)) {
				MProjectWindow* project = nil;
				if (message->FindPointer(kProjectMID, (void **) &project) == B_OK) {
					this->ProjectActivated(project);
				}
			}
			break;
			
		// someone is watching for file changes
		// currently I just hardcode who might be interested
		// in the future we might want to register a handler object
		case B_NODE_MONITOR:
         	MDynamicMenuHandler::HandleNodeMonitorChange(*message);
			break;
			
		default:
			if (! TryScriptMessage(message, this, sProjectVerbs))
				BApplication::MessageReceived(message);
			break;
	}
}

// ---------------------------------------------------------------------------
//		 GetSubHandler
// ---------------------------------------------------------------------------

ScriptHandler *
IDEApp::GetSubHandler(
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
	if (0 == strcmp(propertyName, kProjectProp) || (0 == --propCount)) 
	{
		switch (form) 
		{
			case formName:
				if (fCurrentProject != nil && 0 == strcmp(data.name, fCurrentProject->Title()))
					subHandler = fCurrentProject;
				break;

			case formDirect:
				subHandler = fCurrentProject;
				break;

			default:	// Other forms are not supported
				break;
		}
	}
	else
	if (propCount > 0) {
		SData 	newData;
		newData.index = propCount;
		return ScriptHandler::GetSubHandler(propertyName, formIndex, newData);
	}

	if (subHandler)
		return subHandler;
	else
		return ScriptHandler::GetSubHandler(propertyName, form, data);
}

// ---------------------------------------------------------------------------
//		 FilePanelIsRunning
// ---------------------------------------------------------------------------
//	Does it exist?

bool
IDEApp::FilePanelIsRunning()
{
	return fFilePanel != nil;
}

// ---------------------------------------------------------------------------
//		 CloseFilePanel
// ---------------------------------------------------------------------------
//	Close the panel.

void
IDEApp::CloseFilePanel()
{
	if (fFilePanel != nil)
	{
		fFilePanel->GetPanelDirectory(&fLastDirectory);
		fFilePanelFrame = fFilePanel->Window()->Frame();
		fLastDirectoryGood = true;

		delete fFilePanel;
		fFilePanel = nil;

		fFilePanelStatus = opNothing;
	}
}

// ---------------------------------------------------------------------------
//		 ShowFilePanel
// ---------------------------------------------------------------------------

void
IDEApp::ShowFilePanel(
	const char *	inTitle,
	uint32			inMessage,
	const char *	inCancelButtonLabel,
	const char *	inDefaultButtonLabel,
	bool			inDirectorySelection)
{
	if (FilePanelIsRunning())
	{
		fFilePanel->Window()->Show();
		fFilePanel->Window()->Activate();
	}
	else
	{
		BWindow*		panel;
		uint32			nodeFlavor = B_FILE_NODE;
		entry_ref*		ref = nil;
		
		if (inDirectorySelection)
			nodeFlavor = B_DIRECTORY_NODE;
		
		if (inMessage == 0)
			inMessage = msgFilePanelRefsReceived;	

		// Restore the frame and directory
		if (fLastDirectoryGood)
			ref = &fLastDirectory;

		BMessage		msg(inMessage);

		fFilePanel = new BFilePanel(B_OPEN_PANEL, nil, ref, nodeFlavor, true, &msg);

		panel = fFilePanel->Window();

		if (inTitle != nil)
			panel->SetTitle(inTitle);

		if (fLastDirectoryGood && fFilePanelFrame.IsValid())
		{
			panel->MoveTo(fFilePanelFrame.LeftTop());
			panel->ResizeTo(fFilePanelFrame.Width(), fFilePanelFrame.Height());
			ValidateWindowFrame(fFilePanelFrame, *panel);
		}

		if (inCancelButtonLabel != nil)
			fFilePanel->SetButtonLabel(B_CANCEL_BUTTON, inCancelButtonLabel);
		if (inDefaultButtonLabel != nil)
			fFilePanel->SetButtonLabel(B_DEFAULT_BUTTON, inDefaultButtonLabel);
			
		fFilePanel->Show();
	}
}

// ---------------------------------------------------------------------------
//		 ProjectClosed
// ---------------------------------------------------------------------------
//	A Project window has closed.

void
IDEApp::ProjectClosed(MProjectWindow* project)
{
	this->RemoveProject(project);
	
	// tell myself so I can check if I should quit
	this->PostMessage(msgProjectClosed);
}

// ---------------------------------------------------------------------------
//		 HandleFileSaved
// ---------------------------------------------------------------------------
//	When a new file is saved it needs to know if there is a project file
//	open.  It asks us about it and if there is then we send back a 
//	ProjectOpened message.

void
IDEApp::HandleFileSaved(
	BMessage& inMessage)
{
	MTextWindow*	textWindow;

	if (inMessage.FindPointer(kTextWindow, (void**) &textWindow) == B_NO_ERROR) {
		ASSERT(fCurrentProject);
		ASSERT(textWindow);
	
		if (textWindow) {
			BMessage msg(msgProjectOpened);
			
			msg.AddPointer(kProjectMID, fCurrentProject);
			textWindow->PostMessage(&msg);
		}
	}
}

// ---------------------------------------------------------------------------

void
IDEApp::DoPreferences()
{
	ASSERT(fPreferencesWindow);
	fPreferencesWindow->ShowAndActivate();
}

// ---------------------------------------------------------------------------
//		 AboutRequested
// ---------------------------------------------------------------------------
//	Show the about box.

void
IDEApp::AboutRequested()
{
	if (fAbout) 
	{
		fAbout->Activate(true);
	}
	else
	{
		try {
			fAbout = new AboutBox('vide', 1);
			fAbout->Show();
		} catch (...) {
			fAbout = NULL;
			PRINT(("Rogue exception in AboutRequested!\n"));
		}
	}
}

// ---------------------------------------------------------------------------
//		 DoCmdNew
// ---------------------------------------------------------------------------
//	Build a new text window.

void
IDEApp::DoCmdNew()
{
	String			fileName;

	GetNewFileName(fileName);

	MTextWindow* 	wind = new MTextWindow(fileName);

	SetTextFileFont(wind, true);

	if (fCurrentProject) {
		BMessage msg(msgProjectOpened);
		
		msg.AddPointer(kProjectMID, fCurrentProject);
		wind->PostMessage(&msg);
	}

	wind->Show();
}

// ---------------------------------------------------------------------------
//		 GetNewFileName
// ---------------------------------------------------------------------------
// Build a file name for a new text file.

void
IDEApp::GetNewFileName(
	String& ioFileName)
{
	ioFileName = "Untitled ";

	ioFileName += ++fUntitledCount;
}

// ---------------------------------------------------------------------------
//		 DoCmdOpen
// ---------------------------------------------------------------------------
//	Run the FilePanel so the user can choose a file to open.

void
IDEApp::DoCmdOpen()
{
	if (FilePanelIsRunning() && (fFilePanelStatus != opOpeningFiles || WindowIsHidden(fFilePanel->Window())))
	{
		CloseFilePanel();
	}

	ShowFilePanel("Open:");

	fFilePanelStatus = opOpeningFiles;

	// Should do some filtering ????
}

// ---------------------------------------------------------------------------
//		 DoAddFiles
// ---------------------------------------------------------------------------
//	Run the FilePanel so the user can choose a file to Add to the open project.

void
IDEApp::DoAddFiles()
{
	if (fFilePanelStatus != opAddingFiles && FilePanelIsRunning())
	{
		CloseFilePanel();
	}

	ShowFilePanel("Add Files", msgAddFiles, "Done", "Add");
	
	fFilePanelStatus = opAddingFiles;
}

// ---------------------------------------------------------------------------
//		 AddOtherFiles
// ---------------------------------------------------------------------------
//	The find window has asked that we open up the Add other files open panel.

void
IDEApp::AddOtherFiles()
{
	if (FilePanelIsRunning() && fFilePanelStatus != opAddingOthersToFind)
	{
		CloseFilePanel();
	}
	
	ShowFilePanel("Select a File to Search", msgAddOtherFile, "Done", "Add");

	fFilePanelStatus = opAddingOthersToFind;
}

// ---------------------------------------------------------------------------
//		 AppActivated
// ---------------------------------------------------------------------------
//	Since Be apps quit when there are no windows open we check
//	the number of windows open and quit when there are none.
//	Windows that matter:
//		Any text window, find window, or all project windows
void
IDEApp::AppActivated(
	bool 	inActive)
{
	// Check text windows, project window, find window, 
	// (and new project, so we don't dump them out while they are creating a new project)
	if (! inActive &&
		MDynamicMenuHandler::GetFrontTextWindow() == nil &&
		fProjectList.CountItems() == 0 &&
		fNewProjectWindow == nil &&
		fNewProjectPanel == nil &&
		WindowIsHidden(&MFindWindow::GetFindWindow()))
		{
			PostMessage(B_QUIT_REQUESTED);
		}
}

// ---------------------------------------------------------------------------
//		 CloseProject
// ---------------------------------------------------------------------------

inline bool
IDEApp::CloseAllProjects()
{
	// Only called from quiting, but if the user cancels in the middle
	// we'd better have all our state set correctly
	// Since ProjectClosed() isn't called if we are quitting, we do
	// it explicitly here.
	// (remember that our fProjectList is being modified from
	// under us, so the iteration needs to be careful)
	
	// iterate from back to front so that the current project
	// isn't changed until the very end...
			
	bool result = true;
	while (result) {
		MProjectWindow* project = this->GetBackmostProject();
		if (project == nil) {
			break;
		}
		if (project->Lock()) {	
			result = project->QuitRequested();
			if (result) {
				int32 ignore;
				thread_id tid = project->Thread();
	
				project->Quit();
				wait_for_thread(tid, &ignore);
				
				this->RemoveProject(project);
			}
			else {
				project->Unlock();
			}
		}
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//		 ClosePersistentWindows
// ---------------------------------------------------------------------------

inline bool
IDEApp::ClosePersistentWindows()
{
	if (fAbout && fAbout->Lock() && fAbout->QuitRequested())
	{
		fAbout->Quit();
	}

	bool		okToQuit = true;
	thread_id	tid;
	int32		ignore;
	
	if (okToQuit && fFindWindow->Lock())
	{
		if (fFindWindow->QuitRequested())
		{
			tid = fFindWindow->Thread();
			fFindWindow->Quit();
			wait_for_thread(tid, &ignore);
		}
		else
		{
			okToQuit = false;
			fFindWindow->Unlock();
		}
	}
	if (fPreferencesWindow->Lock())
	{
		// allow preference window to cleanup, but
		// ignore result of QuitRequested
		// (we have already allowed them to cancel/save)
		fPreferencesWindow->QuitRequested();
		tid = fPreferencesWindow->Thread();
		fPreferencesWindow->Quit();
		wait_for_thread(tid, &ignore);
	}
	
	ASSERT(okToQuit);
	
	return okToQuit;
}

// ---------------------------------------------------------------------------
//		 QuitRequested
// ---------------------------------------------------------------------------
//	Our own version of this function.  Doesn't call BApplication's version.

bool
IDEApp::QuitRequested()
{
	fQuitting = true;		// The windows need to know that we're quitting

	// Ask all the windows if it's ok to quit
	// (we ask the preference window first so that if the user cancel's, they
	// aren't left with nothing but the preference window)
	fQuitting = fPreferencesWindow->OKToQuit() &&
				MDynamicMenuHandler::CloseAllTextWindows() &&
				CloseAllProjects() &&
				ClosePersistentWindows();

	// If we get past all the checks, then close down all the message windows
	if (fQuitting) {
		BMessage msg(cmd_Close);
		MMessageWindow::MessageToAllMessageWindows(msg);
	}
	
	return fQuitting;
}

// ---------------------------------------------------------------------------
//		 OpenTextFile
// ---------------------------------------------------------------------------

MTextWindow*
IDEApp::OpenTextFile(
	BEntry*	inFile,
	bool	inIsInProject)	// true if it is and false if we don't know
{
	MTextWindow* 	wind = nil;

	// Is the window already open?
	if (inFile->IsFile()) {
		wind = MDynamicMenuHandler::FindTextWindowForFile(inFile);
		if (wind) {
			// If so we don't need this file object
			delete inFile;
			wind->Activate(true);
		}
		else {
			// Build the new text window
			char title[B_FILE_NAME_LENGTH] = { '\0' };
		
			inFile->GetName(title);
			wind = new MTextWindow(inFile, title);
		
			SetTextFileFont(wind, false);
	
			wind->Show();
			
			if (fCurrentProject) {
				BMessage msg(msgProjectOpened);
				msg.AddPointer(kProjectMID, fCurrentProject);

				if (inIsInProject) {
					msg.AddBool(kIsInProject, true);
				}
				wind->PostMessage(&msg);
			}
		}
	}
	
	return wind;
}

// ---------------------------------------------------------------------------
//		 OpenRawTextWindow
// ---------------------------------------------------------------------------
//	Open a window given a buffer of raw text.  Used to implement preprocess
//	and disassemble.  We aren't responsible for cleaning up the text bufffer.

void
IDEApp::OpenRawTextWindow(
	BMessage& 	inMessage)
{
	char*		text = nil;
	const char*	name;
	int32		size = 0;

	if (B_NO_ERROR == inMessage.FindPointer(kAddress, (void**) &text) && 
		B_NO_ERROR == inMessage.FindInt32(kSize, &size) &&
		B_NO_ERROR == inMessage.FindString(kFileName, &name) &&
		text != nil)
	{
		// Build the new text window
		MTextWindow* wind = new MTextWindow(name, text, size);
		SetTextFileFont(wind, false);

		wind->Show();
		
		if (fCurrentProject) {
			BMessage msg(msgProjectOpened);
			msg.AddPointer(kProjectMID, fCurrentProject);
			wind->PostMessage(&msg);
		}
	}
}

// ---------------------------------------------------------------------------
//		 SetTextFileFont
// ---------------------------------------------------------------------------
//	Decide what font settings to use whan a text file is opened.  If the file
//	is an existing file the settings are temporary.  For a new file these
//	settings become the permanent settings of the file.

void
IDEApp::SetTextFileFont(
	MTextWindow*	inWindow,
	bool			inNewFile)
{
	LockPrefs();
	BMessage msg(msgSetData);
	
	// Decide if we use the app settings for the font
	// The default is to let the document use its own font settings
	// New windows are always given font settings here
	if (fEditorPrefs.pUseAppFont || inNewFile) {
		msg.AddData(kFontPrefs, kMWPrefs, &fFontPrefs, sizeof(fFontPrefs));
	}
	
	msg.AddData(kSyntaxStylePrefs, kMWPrefs, &fSyntaxStylePrefs, sizeof(fSyntaxStylePrefs));
	msg.AddData(kAppEditorPrefs, kMWPrefs, &fAppEditorPrefs, sizeof(fAppEditorPrefs));

	inWindow->SetData(msg, inNewFile);
	
	// Set the balance when typing info
	if (fEditorPrefs.pBalanceWhileTyping) {
		BMessage msg(msgBalanceWhileTyping);

		msg.AddInt32(kFlashDelay, fEditorPrefs.pFlashingDelay);
		inWindow->PostMessage(&msg);	
	}

	UnlockPrefs();
}

// ---------------------------------------------------------------------------
//		 ArgvReceived
// ---------------------------------------------------------------------------
//	Open files from the command line.

void
IDEApp::ArgvReceived(int32 argc, char **argv)
{
	const char *	cwd;
	
	// The cwd is stored in the message
	if (B_OK == CurrentMessage()->FindString("cwd", &cwd))
	{
		BDirectory		dir(cwd);
		entry_ref		ref;
		BEntry			entry;

		for (int i = 1; i < argc; i++) 
		{
			if (B_OK == entry.SetTo(&dir, argv[i]) &&
				entry.IsFile() &&
				B_OK == entry.GetRef(&ref))
			{
				OpenRef(ref);
			}
		}
	}
}

// ---------------------------------------------------------------------------
//		 DoCmdNewProject
// ---------------------------------------------------------------------------
//	Build a new project window.

void
IDEApp::DoCmdNewProject()
{
	if (! fNewProjectWindow)
	{
		fNewProjectWindow = new MNewProjectWindow;
	}

	ShowAndActivate(fNewProjectWindow);
}

// ---------------------------------------------------------------------------
//		 BuildEmptyProject
// ---------------------------------------------------------------------------
//	Build a new project window.

void
IDEApp::BuildEmptyProject(
	BMessage*	inMessage)
{
	entry_ref		ref;
	
	if (inMessage->FindRef(kEmptyProject, &ref) == B_OK) {
	
		MProjectWindow* project = new MProjectWindow(ref, true);
		
		// Send all the default prefs to the new project
		BMessage msg(msgSetData);
		
		LockPrefs();
		msg.AddData(kPrivatePrefs, kMWPrefs, &fPrivatePrefs, sizeof(fPrivatePrefs));
		msg.AddData(kAccessPathsPrefs, kMWPrefs, &fAccessPathsPrefs, sizeof(fAccessPathsPrefs));
		MFileUtils::AddAccessPathsToMessage(msg, fSystemPathList, fProjectPathList);
		msg.AddData(kTargetPrefs, kMWPrefs, &fTargetPrefs, sizeof(fTargetPrefs));
		msg.AddData(kTargetBlockPrefs, kMWPrefs, fTargetPrefs.pTargetArray, fTargetPrefs.pCount * sizeof(TargetRec));
		msg.AddData(kRunPrefs, kMWPrefs, &fRunPrefs, sizeof(fRunPrefs));
		UnlockPrefs();

		project->SetData(msg);
		
		// Set up the build preferences in the project
		project->SetBuildParameters(fBuildExtrasPrefs);

		project->NewProjectCreated();
		project->Show();

		MDynamicMenuHandler::ProjectOpened(project);
		
		// This is now our new current project
		this->ProjectActivated(project);
	}
}

// ---------------------------------------------------------------------------
//		 OpenProjectFile
// ---------------------------------------------------------------------------
//	Open an existing project file in response to a double click or a drag 
//	and drop.

MProjectWindow*
IDEApp::OpenProjectFile(const entry_ref& inRef)
{
	// Decide if this project is already open...
	// Iterate through our open projects and see if inRef matches any
		
	MProjectWindow* openProject = this->IsOpenProject(inRef);
	if (openProject) {
		openProject->Activate();
	}
	else {
		// Open the project file if not currently open
		openProject = new MProjectWindow(inRef);

		// Set up the build preferences in the project
		openProject->SetBuildParameters(fBuildExtrasPrefs);
		
		// Start up the window
		openProject->Show();

		MDynamicMenuHandler::ProjectOpened(openProject);
		// This is now our new current project
		this->ProjectActivated(openProject);
		
		// Save the directory of the project for the open panel
		SaveLastDirectory(inRef);
	}
	
	return openProject;
}	

// ---------------------------------------------------------------------------

void
IDEApp::OpenProjectAndReply(BMessage& message)
{
	// Open up the project given in ref
	// Reply to the message that we have completed the task

	entry_ref ref;	
	BMessage reply;
	if (message.FindRef("refs", &ref) == B_OK) {
		MProjectWindow* projectWindow = this->OpenProjectFile(ref);
		reply.AddInt32("status", B_OK);
		reply.AddPointer(kProjectMID, projectWindow);
	}
	else {
		reply.AddInt32("status", B_NAME_NOT_FOUND);
	}

	if (message.IsSourceWaiting()) {
		message.SendReply(&reply);
	}
}

// ---------------------------------------------------------------------------

void
IDEApp::SaveLastDirectory(const entry_ref& inEntry)
{
	// inEntry is assumed to refer to a file.
	// Save the directory of the project for the open panel
	BDirectory		dir;
	BEntry			entry(&inEntry);
	
	if (B_NO_ERROR == entry.GetParent(&dir) &&
		B_NO_ERROR == dir.GetEntry(&entry) &&
		B_NO_ERROR == entry.GetRef(&fLastDirectory))
	{
		fLastDirectoryGood = true;
	}
}

// ---------------------------------------------------------------------------

void
IDEApp::OpenSourceFile(BMessage* message)
{
	// We have been asked to open a file system reference from a...
	// double-click, message window, find result
	// First, check what editor we are supposed to use.
	// If normal, we just turn around and all RefsReceived - which
	// does all the work for us.  If we are to use an external editor
	// just pass off the message to that editor with the line/character
	// massaged into the "external" format.
	
	// While we trap the events listed above, any "Open...", or "Open File..."
	// or file drag/drop will be opened locally always since they don't come
	// through here but go directly to RefsReceived.

	if (fEditorPrefs.pUseExternalEditor == false) {
		this->RefsReceived(message);
		return;
	}
		
	// Iterate through the message and for each ref, look for its line number
	// or token information.  If given line/token, convert it into the following:
	// refs 					entry_ref	the file to open
	// be:line					int32		line number (first line in file = 1)
	// be:selection_offset		int32		selection offset (from beginning of file)
	// be:selection_length		int32		length of selection
	
	int32 refCount = 0;
	type_code messageType;
	int32 length;
	if (message->GetInfo("refs", &messageType, &refCount) == B_OK) {
		entry_ref aRef;
		// iterate each reference in message
		for (int i = 0; i < refCount; i++) {
			if (message->FindRef("refs", i, &aRef) == B_OK) {
				// Make a message with the proper kind for launch...
				BMessage externalMessage(B_REFS_RECEIVED);
				externalMessage.AddRef("refs", &aRef);
				// look for position token or line number
				TokenIdentifier* positionToken;
				int32 lineNumber;
				if (message->FindData(kTokenIdentifier, kTokenIDType, i,
						(const void**) &positionToken, &length) == B_OK) {
					// convert line number to one based and also send offset/length/text
					lineNumber = positionToken->eLineNumber + 1;
					externalMessage.AddInt32(kBeLineNumber, lineNumber);
					// we communicate in terms of the selection only
					externalMessage.AddInt32(kBeSelectionOffset, positionToken->eOffset);
					externalMessage.AddInt32(kBeSelectionLength, positionToken->eLength);
				}
				else if (message->FindInt32(kLineNumber, i, &lineNumber) == B_OK) {
					// convert line number to one based
					// all we have in this case is the line number, so that is all we can give
					lineNumber += 1;
					externalMessage.AddInt32(kBeLineNumber, lineNumber);
				}
				// we have our message - launch the application
				// (if we don't have an application signature, launch the ref itself
				// which will launch the preferrred application for the file type)
				status_t err;
				if (strlen(fEditorPrefs.pEditorSignature) > 0) {
				 	err = be_roster->Launch(fEditorPrefs.pEditorSignature, &externalMessage);
				}
				else {
					err = be_roster->Launch(&aRef, &externalMessage);
				}
				
				if (err != B_OK && err != B_ALREADY_RUNNING) {
					// got error launching external editor, just use internal
					// (but give a warning first)
					String errorMessage = "Could not launch ";
					if (strlen(fEditorPrefs.pEditorSignature) > 0) {
						errorMessage += "application with signature "B_UTF8_OPEN_QUOTE;
						errorMessage += fEditorPrefs.pEditorSignature;
						errorMessage += B_UTF8_CLOSE_QUOTE".";
					}
					else {
						errorMessage += "preferred application for "B_UTF8_OPEN_QUOTE;
						errorMessage += aRef.name;
						errorMessage += B_UTF8_CLOSE_QUOTE".";
					}
					errorMessage += " Using BeIDE editor.";
					MWarningAlert alert(errorMessage);
					alert.Go();

					this->RefsReceived(message);
					// the original message has all information needed for local, don't
					// loop here
					break;
				}
			}
		}
	}			
}
// ---------------------------------------------------------------------------

void
IDEApp::HandleFindDocumentation(BMessage& inMessage)
{
	// We have been asked to search for documentation.
	// If we have a string in the message, then look that up
	// otherwise open up a window to allow the user to enter 
	// a string.  If that happens, then this same function
	// will be called back with a msgFindDocumentation + string.

	if (inMessage.HasString(kText)) {
		const char* lookupString = inMessage.FindString(kText);
		// if we are already looking for something, cancel that one
		// and start on this request (FindDocumentationDone must be
		// done here, if we allow the thread to call it, then it will
		// potentially clear out our new fLookupDocumentationTask)
		if (fLookupDocumentationTask != nil) {
			fLookupDocumentationTask->Cancel();
			this->FindDocumentationDone();
		}
		fLookupDocumentationTask = new MLookupDocumentationTask(this, lookupString);
		fLookupDocumentationTask->Run();
	}
	else {
		// Open the find documentation selection window
		ShowLookupDocumentationWindow();
	}
}

// ---------------------------------------------------------------------------

void
IDEApp::FindDocumentationDone()
{
	// just record that our lookup thread is gone so that
	// cancel won't try to go find one
	fLookupDocumentationTask = nil;
}

// ---------------------------------------------------------------------------

void
IDEApp::ShowLookupDocumentationWindow()
{
	if (fLookupDocumentationWind == NULL) {
		fLookupDocumentationWind = new MLookupDocumentationWindow;
	}

	ShowAndActivate(fLookupDocumentationWind);
}

// ---------------------------------------------------------------------------
//		 RefsReceived
// ---------------------------------------------------------------------------
//	We have been asked to open a file system reference.  This should come
//	from a double-click, a drag onto our icon, from the OpenFilePanel, or
//	from a sourceFile line in the project window.

void
IDEApp::RefsReceived(
	BMessage * message)
{
	type_code		messageType;
	int32			count;
	
	if (B_NO_ERROR == message->GetInfo("refs", &messageType, &count))
	{
		entry_ref	ref;

		for (int i = 0; i < count; i++)
		{
			if (B_NO_ERROR == message->FindRef("refs", i, &ref))
				OpenRef(ref, message);
		}
	}
}

// ---------------------------------------------------------------------------
//		 OpenRef
// ---------------------------------------------------------------------------

void
IDEApp::OpenRef(const entry_ref& inRef, BMessage* inMessage)
{
	BEntry* file = new BEntry(&inRef, true);
	
	// quick check that BEntry is initialized correctly
	if (file->InitCheck() == B_BAD_VALUE) {
		String text = "Error attempting to open file (name unknown). Request is being ignored.\n";
		// don't try to access the name of file here - it is a bad value
		MWarningAlert alert(text);
		alert.Go();
		delete file;
		return;
	}
	
	// another quick check to see that the file really exists
	// (a #line directive can cause gcc to return a bogus path)
	// (normally this won't ever happen)
	if (file->Exists() == false) {
		BPath fullPath;
		status_t err = file->GetPath(&fullPath);
		String text = "The file "B_UTF8_OPEN_QUOTE;
		if (err == B_OK) {
			text += fullPath.Path();
		}
		else {
			text += inRef.name;
		}
		text += B_UTF8_CLOSE_QUOTE" does not exist. Request is being ignored.";
		MWarningAlert alert(text);
		alert.Go();
		delete file;
		return;
	}
	
	bool isInProject = false;
	if (inMessage != nil) {
		inMessage->FindBool(kIsInProject, &isInProject);
	}
	
	type_code type = ::MimeType(*file);
	if (type == kNULLType) {
		::FixFileType(file);
		type = ::MimeType(*file);
	}
	
	switch (type) {
		case kTextType:
			{
				MTextWindow* wind = OpenTextFile(file, isInProject);
				if (wind && inMessage) {
					GoToLineInWindow(wind, inMessage);
				}
				// OpenTextFile adopted file, don't delete at return
				file = NULL;
				
				if (!fLastDirectoryGood)	// save the dir if this is the first file opened
					SaveLastDirectory(inRef);
			}
			break;
		
		case kProjectType:
			OpenProjectFile(inRef);
			break;
		
		case kSharedLibType:
		case kCWLibType:
		case kPreCompiledType:
		case kAppType:
			{
				// Can't open these files for viewing
				String		text = "Can't open a file of this type.\n";
				text += inRef.name;
				
				MWarningAlert alert1(text);
				alert1.Go();
			}
			break;

		default:
			// A type we don't know about
			String		text = "The file "B_UTF8_OPEN_QUOTE;

			text += inRef.name;
			text += B_UTF8_CLOSE_QUOTE" is not a TEXT or project file.  Open it anyway?";
		
			MWarningAlert 		alert(text, "Open", "Cancel");
			if (1 == alert.Go()) {
				MTextWindow* wind = OpenTextFile(file, isInProject);
				if (wind && inMessage) {
					GoToLineInWindow(wind, inMessage);
				}
				// OpenTextFile adopted the file, don't delete it at return
				file = NULL;
			}
			break;
	}
	
	delete file;
}

// ---------------------------------------------------------------------------
//		 GoToLineInWindow
// ---------------------------------------------------------------------------
//	Look for a linenumber or tokenidentifier in the message.  If present then
//	post it to the window.  If there is a sem_id in the message then release
//	it to indicate that the window has been opened.

void
IDEApp::GoToLineInWindow(
	BWindow*	inWindow,
	BMessage*	inMessage)
{
	ASSERT(inWindow && inMessage);

	if (inWindow && inMessage)
	{
		BMessage			msg;
		ssize_t				length;
		TokenIdentifier*	token;
		int32				line;
		sem_id				sem;

		if (B_NO_ERROR == inMessage->FindData(kTokenIdentifier, kTokenIDType, (const void**) &token, &length))
		{
			ASSERT(length == sizeof(TokenIdentifier));

			msg.what = msgGoToLine;
			msg.AddData(kTokenIdentifier, kTokenIDType, token, length);
		}
		else
		if (B_NO_ERROR == inMessage->FindInt32(kLineNumber, &line))
		{
			msg.what = msgGoToLine;
			msg.AddInt32(kLineNumber, line);
		}
		else
		if (B_NO_ERROR == inMessage->FindInt32(kSemID, &sem))
		{
			release_sem(sem);
		}

		if (msg.what != 0)
			inWindow->PostMessage(&msg);
	}
}

// ---------------------------------------------------------------------------
//		 DoOpenSelection
// ---------------------------------------------------------------------------
//	Handle cmd-d.

void
IDEApp::DoOpenSelection()
{
	MTextWindow*	frontText = MDynamicMenuHandler::GetFrontTextWindow();

	// If the front window is a text window then ask it to send
	// us an open selection message based on its selection
	if (frontText && frontText->IsActive()) {
		frontText->PostMessage(msgDoOpenSelection);
	}
	else {
		ShowOpenSelectionWindow(fCurrentProject);
	}
}

// ---------------------------------------------------------------------------
//		 OpenSelection
// ---------------------------------------------------------------------------
//	Handle cmd-d.

bool
IDEApp::OpenSelection(
	const char * 		inName,
	bool				inSystemTree,
	const entry_ref&	inRef,
	MProjectWindow*		inProject,
	bool				inSyncronous,
	bool				inSearchMemory,
	bool				inSearchDisk)
{
	bool				found = false;
	entry_ref			ref;
	status_t			err;

	// If there's a project ask it to find the file
	if (inProject != nil && inProject->Lock())
	{
		found = inProject->OpenSelection(inName, inSystemTree, ref, inSearchMemory, inSearchDisk);
		inProject->Unlock();
	}
	else
	if (inSearchDisk)
	{
		// Search in the system tree for this file
		found = MFileUtils::FindFileInDirectoryList(inName, fSystemDirectories, ref);
	}

	// If the file has been saved search in the same folder as the file
	if (! found && inSearchDisk)
	{
		BEntry			entry(&inRef);
		BDirectory		dir;
		
		if (B_NO_ERROR == entry.GetParent(&dir) && 
			B_NO_ERROR == FindFile(dir, inName, &entry))
		{
			err = entry.GetRef(&ref);
			found = err == B_NO_ERROR;			// We found it
		}
	}
	
	// If found open it
	if (found)
	{
		if (inSyncronous)
			OpenRef(ref, nil);
		else
		{
			BMessage		msg(B_REFS_RECEIVED);
			
			msg.AddRef("refs", &ref);
			
			be_app_messenger.SendMessage(&msg);
		}
	}

	return found;
}

// ---------------------------------------------------------------------------
//		 OpenSelectionAsync
// ---------------------------------------------------------------------------
//	Called asyncronously from the MOpenSelectionTask.

void
IDEApp::OpenSelectionAsync(
	const char * 		inName,
	bool				inSystemTree,
	const entry_ref&	inRef,
	MProjectWindow*		inProject)
{
	bool 		found = OpenSelection(inName, inSystemTree, inRef, inProject, false);
	
	if (! found && ! inSystemTree)
		found = OpenSelection(inName, true, inRef, inProject, false);

	if (! found)
	{
		String		text = "Couldn't find the file "B_UTF8_OPEN_QUOTE;
		text += inName;
		text += B_UTF8_CLOSE_QUOTE".";
		
		MAlert		alert(text);
		alert.Go();
	}
}

// ---------------------------------------------------------------------------
//		 HandleOpenSelection
// ---------------------------------------------------------------------------
//	Handle cmd-d.

void
IDEApp::HandleOpenSelection(
	BMessage	&inMessage)
{
	// Our message from the text window (or project view)
	// should include the current project to use for the lookup
	// If it doesn't or if it is nil, use the current project

	MProjectWindow* project = nil;
	inMessage.FindPointer(kProjectMID, (void **) &project);
	if (project == nil) {
		project = fCurrentProject;
	}
	
	if (! inMessage.HasString(kFileName))
	{
		// Show the open selection window
		ShowOpenSelectionWindow(project);
	}
	else
	{
		entry_ref			ref;
		bool				inSystemTree = false;
		const char *		fileName = inMessage.FindString(kFileName);
		status_t			err = inMessage.FindRef(kTextFileRef, &ref);	// optional parameter
		
		err = inMessage.FindBool(kSystemInclude, &inSystemTree);
		MOpenSelectionTask*	thread = new MOpenSelectionTask(fileName, inSystemTree, ref, project);
		thread->Run();
	}
}

// ---------------------------------------------------------------------------
//		 HandleAndyFeature
// ---------------------------------------------------------------------------
//	Handle alt-tab in a text window.

extern const char* sHeaderSuffixes[];
extern const char* sSourceSuffixes[];

void
IDEApp::HandleAndyFeature(
	BMessage	&inMessage)
{
	if (inMessage.HasString(kFileName))
	{
		const char *		fileName = inMessage.FindString(kFileName);
		SuffixType			suffix = GetSuffixType(fileName);
		bool				isSourceFile;
		const char **		suffixArray;

		// What kind of file is this?
		switch (suffix)
		{
			case kCSuffix:
			case kCPSuffix:
				isSourceFile = true;
				suffixArray = sHeaderSuffixes;
				break;
			
			case kHSuffix:
				isSourceFile = false;
				suffixArray = sSourceSuffixes;
				break;

			default:
				// If we don't know how to do this file then don't try
				suffix = kInvalidSuffix;
				break;
		}
			
		if (suffix != kInvalidSuffix)
		{
			entry_ref		ref;
			
			inMessage.FindRef(kTextFileRef, &ref);	// optional parameter
			MProjectWindow* project = nil;
			inMessage.FindPointer(kProjectMID, (void **) &project);
			// Use the current project if the project isn't found in message (or is nil)
			if (project == nil) {
				project = fCurrentProject;
			}

			MAndyFeatureTask*	thread = new MAndyFeatureTask(fileName, suffixArray, isSourceFile, ref, project);
			thread->Run();
		}
	}
}

// ---------------------------------------------------------------------------
//		 HandleAndyFeature
// ---------------------------------------------------------------------------
//	Handle alt-tab in a text window.  Called from the AndyFeatureTask.

void
IDEApp::AndyFeatureAsync(
	const char *	fileName,
	const char **	suffixArray,
	bool			isSourceFile,
	entry_ref&		ref,
	MProjectWindow*	inProject)
{
	bool			found = false;
	String			name = fileName;
	int				offset = name.ROffsetOf('.') + 1;
	int				i = 0;

	// First search the inMemory lists in the project
	while (suffixArray[i][0] != 0 && ! found)
	{
		name.Replace(suffixArray[i], offset, name.GetLength() - offset);
		
		found = OpenSelection(name, false, ref, inProject, false, true, false);
		i++;
	}

	// Search the disk in the project tree
	i = 0;
	while (suffixArray[i][0] != 0 && ! found)
	{
		name.Replace(suffixArray[i], offset, name.GetLength() - offset);
		
		found = OpenSelection(name, false, ref, inProject, false, false, true);
		i++;
	}

	// Search the Disk in the system tree
	if (! found)
	{
		BEntry			file;
		i = 0;
		while (suffixArray[i][0] != 0 && ! found)
		{
			name.Replace(suffixArray[i], offset, name.GetLength() - offset);
			
			found = MFileUtils::FindFileInDirectory(name, MFileUtils::SystemDirectory(), file);
			
			if (found)
			{
				MLocker<BLooper>	lock(this);
				BEntry*				foundfile = new BEntry(file);
				MTextWindow*		wind = OpenTextFile(foundfile);
	
				if (wind)
					wind->Activate();
			}
			i++;
		}
	}

	// Search the Directory containing this file
	if (! found && ref.name != nil)
	{
		BEntry		file1(&ref);
		BDirectory	dir;
		
		if (B_NO_ERROR == file1.GetParent(&dir))
		{
			i = 0;
			BEntry*		file2 = new BEntry;

			while (suffixArray[i][0] != 0 && ! found)
			{
				name.Replace(suffixArray[i], offset, name.GetLength() - offset);

				if (B_NO_ERROR == FindFile(dir, name, file2))
				{
					MLocker<BLooper>	lock(this);
					MTextWindow*		wind = OpenTextFile(file2);
					if (wind)
						wind->Activate();
					found = true;
				}
				i++;
			}

			if (! found)
				delete file2;
		}
	}
		
	// Show an alert if we didn't find the file
	if (! found)
	{
		String		text;
		
		if (isSourceFile)
			text = "Couldn't find header file for "B_UTF8_OPEN_QUOTE;
		else
			text = "Couldn't find source file for "B_UTF8_OPEN_QUOTE;
		
		text += fileName;
		text += B_UTF8_CLOSE_QUOTE".";

		MAlert		alert(text);
		alert.Go();
	}
}

// ---------------------------------------------------------------------------
//		 ShowOpenSelectionWindow
// ---------------------------------------------------------------------------

void
IDEApp::ShowOpenSelectionWindow(MProjectWindow* inProject)
{
	if (! fOpenSelectionWind)
	{
		fOpenSelectionWind = new MOpenSelectionWindow(inProject);
	}
	else {
		fOpenSelectionWind->SetLookupProject(inProject);
	}
	
	ShowAndActivate(fOpenSelectionWind);
}

// ---------------------------------------------------------------------------
//		 GetPrintSettings
// ---------------------------------------------------------------------------

void
IDEApp::GetPrintSettings()
{
	size_t		length;
	status_t	err;

	if (MPreferences::PreferenceExists(kPrintPrefs, kMWDefaultPrefs, &length))
	{
		char*	data = new char[length];
		err = MPreferences::GetPreference(kPrintPrefs, kMWDefaultPrefs, data, length);
		if (err == B_NO_ERROR)
		{
			fPrintSettings = new BMessage;
			err = fPrintSettings->Unflatten(data);

			if (err != B_NO_ERROR)
			{
				delete fPrintSettings;
				fPrintSettings = nil;
			}
		}
		
		delete [] data;
	}
}

// ---------------------------------------------------------------------------
//		 SetPrintSettings
// ---------------------------------------------------------------------------

void
IDEApp::SetPrintSettings()
{
	ASSERT(fPrintSettings != nil);

	ssize_t		length = fPrintSettings->FlattenedSize();
	char*		data = new char[length];
	status_t	err = fPrintSettings->Flatten(data, length);
	
	if (err == B_NO_ERROR)
	{
		err = MPreferences::SetPreference(kPrintPrefs, kMWDefaultPrefs, data, length);
	}

	delete [] data;
}

// ---------------------------------------------------------------------------
//		 PageSetup
// ---------------------------------------------------------------------------

status_t
IDEApp::PageSetup(
	BPrintJob&	inPrintJob, bool alwaysShowDialog)
{
	// Get the current page settings
	// If alwaysShowDialog is true (called from Page Setup...) then
	// we show the dialog no matter what our current settings
	// Otherwise, we only show the dialog if we don't have any settings
	
	LockPrefs();
	int32 result = B_OK;
	
	if (fPrintSettings == nil) {
		this->GetPrintSettings();
	}
	
	if (fPrintSettings != nil) {
		inPrintJob.SetSettings(new BMessage(*fPrintSettings));
	}
	
	if (alwaysShowDialog || (fPrintSettings == nil)) {
		result = inPrintJob.ConfigPage();
		if (result == B_OK) {
			delete fPrintSettings;
			fPrintSettings = nil;
			fPrintSettings = inPrintJob.Settings();
			SetPrintSettings();
 		}
	}

	UnlockPrefs();

	return result;
}

// ---------------------------------------------------------------------------
//		 PrinterChanged
// ---------------------------------------------------------------------------

void IDEApp::PrinterChanged(void)
{
	delete fPrintSettings;
	fPrintSettings = nil;	
	MPreferences::RemovePreference(kPrintPrefs, kMWDefaultPrefs);
}

// ---------------------------------------------------------------------------
//		 PrintSetup
// ---------------------------------------------------------------------------

status_t
IDEApp::PrintSetup(
	BPrintJob&	inPrintJob)
{
	// Set everything up before we start printing
	// If the user hasn't yet called PageSetup (so we don't have any 
	// current print settings), then we do it from here...
	
	LockPrefs();
	int			result = -1;
	status_t setupResult = B_OK;
	
	if (fPrintSettings == nil) {
		setupResult = this->PageSetup(inPrintJob, false);
	}
	else {
		inPrintJob.SetSettings(new BMessage(*fPrintSettings));
	}
	
	if (setupResult == B_OK && inPrintJob.ConfigJob() == B_OK) 
	{
		delete fPrintSettings;
		fPrintSettings = nil;
		fPrintSettings = inPrintJob.Settings();
		SetPrintSettings();
		result = 0;
	}

	UnlockPrefs();

	return result;
}

// ---------------------------------------------------------------------------
//		 GetData
// ---------------------------------------------------------------------------
//	Fill the BMessage with preferences data of the kind specified in the
//	BMessage.

void
IDEApp::GetData(
	BMessage&	inOutMessage)
{
	LockPrefs();
	if (inOutMessage.HasData(kEditorPrefs, kMWPrefs))
	{
		inOutMessage.ReplaceData(kEditorPrefs, kMWPrefs, &fEditorPrefs, sizeof(fEditorPrefs));
	}
	if (inOutMessage.HasData(kFontPrefs, kMWPrefs))
	{
		inOutMessage.ReplaceData(kFontPrefs, kMWPrefs, &fFontPrefs, sizeof(fFontPrefs));
	}
	if (inOutMessage.HasData(kAppEditorPrefs, kMWPrefs))
	{
		inOutMessage.ReplaceData(kAppEditorPrefs, kMWPrefs, &fAppEditorPrefs, sizeof(fAppEditorPrefs));
	}
	if (inOutMessage.HasData(kSyntaxStylePrefs, kMWPrefs))
	{
		inOutMessage.ReplaceData(kSyntaxStylePrefs, kMWPrefs, &fSyntaxStylePrefs, sizeof(fSyntaxStylePrefs));
	}
	if (inOutMessage.HasData(kPrivatePrefs, kMWPrefs))
	{
		inOutMessage.ReplaceData(kPrivatePrefs, kMWPrefs, &fPrivatePrefs, sizeof(fPrivatePrefs));
	}
	if (inOutMessage.HasData(kBuildExtrasPrefs, kMWPrefs))
	{
		inOutMessage.ReplaceData(kBuildExtrasPrefs, kMWPrefs, &fBuildExtrasPrefs, sizeof(fBuildExtrasPrefs));
	}
	if (inOutMessage.HasData(kAccessPathsPrefs, kMWPrefs))
	{
		inOutMessage.ReplaceData(kAccessPathsPrefs, kMWPrefs, &fAccessPathsPrefs, sizeof(fAccessPathsPrefs));
		MFileUtils::AddAccessPathsToMessage(inOutMessage, fSystemPathList, fProjectPathList);
	}
	if (inOutMessage.HasData(kTargetPrefs, kMWPrefs))
	{
		inOutMessage.ReplaceData(kTargetPrefs, kMWPrefs, &fTargetPrefs, sizeof(fTargetPrefs));
		if (fTargetPrefs.pTargetArray)
			inOutMessage.AddData(kTargetBlockPrefs, kMWPrefs, fTargetPrefs.pTargetArray, fTargetPrefs.pCount * sizeof(TargetRec));
	}
	if (inOutMessage.HasData(kKeyBindingPrefs, kMWPrefs))
	{
		// The keybinding prefs only exist in the prefs file and in the global
		// KeyBindingsManager object
		
		size_t		length;

		if (MPreferences::PreferenceExists(kKeyBindingPrefs, kMWDefaultPrefs, &length))
		{
			char*		data = new char[length];
			if (B_NO_ERROR == MPreferences::GetPreference(kKeyBindingPrefs, kMWDefaultPrefs, data, length))
			{
				if (B_HOST_IS_LENDIAN)
				{
					// Swap the data to host endian
					BMemoryIO				memdata(data, length);
					MKeyBindingManager		tempManager(memdata, kBigEndian);
					BMallocIO				swapdata;
	
					tempManager.GetKeyBindingsHost(swapdata);
					inOutMessage.ReplaceData(kKeyBindingPrefs, kMWPrefs, swapdata.Buffer(), swapdata.BufferLength());
				}
				else
					inOutMessage.ReplaceData(kKeyBindingPrefs, kMWPrefs, data, length);
			}
			else
				ASSERT(false);
			
			delete [] data;
		}
		else
		{
			BMallocIO		data;
			
			MKeyBindingManager::BuildDefaultKeyBindings(data);
			inOutMessage.ReplaceData(kKeyBindingPrefs, kMWPrefs, data.Buffer(), data.BufferLength());
		}
	}

	UnlockPrefs();
}

// ---------------------------------------------------------------------------
//		 SetData
// ---------------------------------------------------------------------------
//	Get the preferences from the message and update the correct prefs struct.

void
IDEApp::SetData(
	BMessage&	inOutMessage)
{
	ssize_t		length;

	LockPrefs();

	EditorPrefs*	edit;
	if (B_NO_ERROR == inOutMessage.FindData(kEditorPrefs, kMWPrefs, (const void**) &edit, &length))
	{
		ASSERT(length == sizeof(EditorPrefs));
		fEditorPrefs = *edit;

		inOutMessage.RemoveName(kEditorPrefs);
	}
	FontPrefs*	font;
	if (B_NO_ERROR == inOutMessage.FindData(kFontPrefs, kMWPrefs, (const void**) &font, &length))
	{
		ASSERT(length == sizeof(FontPrefs));
		fFontPrefs = *font;
		inOutMessage.RemoveName(kFontPrefs);
	}
	AppEditorPrefs*	colors;
	if (B_NO_ERROR == inOutMessage.FindData(kAppEditorPrefs, kMWPrefs, (const void**) &colors, &length))
	{
		ASSERT(length == sizeof(AppEditorPrefs));
		fAppEditorPrefs = *colors;
		inOutMessage.RemoveName(kAppEditorPrefs);

		SetHiliteColor(fAppEditorPrefs.pHiliteColor);
		MFunctionPopup::SortAllPopups(fAppEditorPrefs.pSortFunctionPopup);
		MFunctionParser::SetRelaxedCParse(fAppEditorPrefs.pRelaxedPopupParsing);
		MTextWindow::RememberSelections(fAppEditorPrefs.pRememberSelection);

		// Tell all the windows that use the hilite or background colors
		BMessage msg(msgColorsChanged);
			
		msg.AddData(kAppEditorPrefs, kMWPrefs, &fAppEditorPrefs, sizeof(fAppEditorPrefs));

		MDynamicMenuHandler::MessageToAllTextWindows(msg);
		MMessageWindow::MessageToAllMessageWindows(msg);
		fPreferencesWindow->PostMessage(&msg);
		this->MessageToAllProjectWindows(msg);
		
		// Update the balance while typing info in all open windows
		BMessage		msg1(msgBalanceWhileTyping);
		
		if (fAppEditorPrefs.pBalanceWhileTyping)
			msg1.AddInt32(kFlashDelay, fAppEditorPrefs.pFlashingDelay);

		MDynamicMenuHandler::MessageToAllTextWindows(msg1);
	}
	SyntaxStylePrefs*	stylePrefs;
	if (B_NO_ERROR == inOutMessage.FindData(kSyntaxStylePrefs, kMWPrefs, (const void**) &stylePrefs, &length))
	{
		ASSERT(length == sizeof(SyntaxStylePrefs));
		fSyntaxStylePrefs = *stylePrefs;
		inOutMessage.RemoveName(kSyntaxStylePrefs);
	}
	PrivateProjectPrefs*	privatePrefs;
	if (B_NO_ERROR == inOutMessage.FindData(kPrivatePrefs, kMWPrefs, (const void**) &privatePrefs, &length))
	{
		ASSERT(length == sizeof(PrivateProjectPrefs));
		fPrivatePrefs = *privatePrefs;
		inOutMessage.RemoveName(kPrivatePrefs);
	}
	BuildExtrasPrefs*	buildExtras;
	if (B_NO_ERROR == inOutMessage.FindData(kBuildExtrasPrefs, kMWPrefs, (const void**) &buildExtras, &length))
	{
		ASSERT(length == sizeof(BuildExtrasPrefs));
		fBuildExtrasPrefs = *buildExtras;
		inOutMessage.RemoveName(kBuildExtrasPrefs);
		// Tell all the projects that the build parameters changed
		BMessage msg(msgBuildExtrasChanged);
		msg.AddData(kBuildExtrasPrefs, kMWPrefs, &fBuildExtrasPrefs, sizeof(fBuildExtrasPrefs));
		this->MessageToAllProjectWindows(msg);
	}
	AccessPathsPrefs*	accessPaths;
	if (B_NO_ERROR == inOutMessage.FindData(kAccessPathsPrefs, kMWPrefs, (const void**) &accessPaths, &length))
	{
		ASSERT(length == sizeof(AccessPathsPrefs));
		fAccessPathsPrefs = *accessPaths;
		
		MFileUtils::GetAccessPathsFromMessage(inOutMessage, fSystemPathList, fProjectPathList, 
							fAccessPathsPrefs.pSystemPaths, fAccessPathsPrefs.pProjectPaths);
		BDirectory*		dir = nil;
		
		if (fCurrentProject)
			dir = &fCurrentProject->GetProjectDirectory();

		MFileUtils::EmptyDirectoryList(fSystemDirectories, dir);
		MFileUtils::BuildDirectoriesList(fSystemPathList, fSystemDirectories, dir);
		inOutMessage.RemoveName(kAccessPathsPrefs);
		inOutMessage.RemoveName(kAccessPathsData);
	}
	TargetPrefs*	target;
	if (B_NO_ERROR == inOutMessage.FindData(kTargetPrefs, kMWPrefs, (const void**) &target, &length))
	{
		ASSERT(length == sizeof(TargetPrefs));

		delete [] fTargetPrefs.pTargetArray;
		fTargetPrefs = *target;

		TargetRec*		targetArray;
		if (fTargetPrefs.pCount > 0 && B_NO_ERROR == inOutMessage.FindData(kTargetBlockPrefs, kMWPrefs, (const void**) &targetArray, &length))
		{
			ASSERT(length == fTargetPrefs.pCount * sizeof(TargetRec));
			if (targetArray)
			{
				fTargetPrefs.pTargetArray = new TargetRec[fTargetPrefs.pCount];
				memcpy(fTargetPrefs.pTargetArray, targetArray, fTargetPrefs.pCount * sizeof(TargetRec));
			}
		}
		
		inOutMessage.RemoveName(kTargetPrefs);
		inOutMessage.RemoveName(kTargetBlockPrefs);
	}

	// Write the keyBinding prefs out to the prefs file and update the
	// global KeyBindingManager

	void*		keyData;
	if (B_NO_ERROR == inOutMessage.FindData(kKeyBindingPrefs, kMWPrefs, (const void**) &keyData, &length))
	{
		// Don't allow making any windows while the keybinding manager is being modified
		MLocker<BApplication>	lock(this);
		BMemoryIO				memData(keyData, length);

		MKeyBindingManager::Manager().SetKeyBindingsHost(memData);
		
		if (B_HOST_IS_LENDIAN)
		{
			// Swap to big endian
			BMallocIO		swapData;
			
			MKeyBindingManager::Manager().GetKeyBindingsBig(swapData);

			(void) MPreferences::SetPreference(kKeyBindingPrefs, kMWDefaultPrefs, swapData.Buffer(), length);
		}
		else
			(void) MPreferences::SetPreference(kKeyBindingPrefs, kMWDefaultPrefs, keyData, length);

		inOutMessage.RemoveName(kKeyBindingPrefs);
	
		// Tell all windows to update their menus
		BMessage		msg(msgUpdateMenus);

		for (int32 i = 0; i < CountWindows(); i++)
		{
			BWindow*	wind = WindowAt(i);
			
			wind->PostMessage(&msg);
		}
	}

	SetPrefs();
	UnlockPrefs();
}

// ---------------------------------------------------------------------------
//		 GetPrefs
// ---------------------------------------------------------------------------
//	Get all of the preferences structs from the preferences file.
//	Should verify length and version number here.  

void
IDEApp::GetPrefs()
{
	size_t		length;
	status_t	err;

	length = sizeof(fEditorPrefs);
	err = MPreferences::GetPreference(kEditorPrefs, kMWDefaultPrefs, &fEditorPrefs, length);
	if (err == B_NO_ERROR)
		fEditorPrefs.SwapBigToHost();
	else
		MDefaultPrefs::SetEditorDefaults(fEditorPrefs);
	fEditorPrefs.pVersion = kCurrentVersion;

	length = sizeof(fFontPrefs);
	err = MPreferences::GetPreference(kFontPrefs, kMWDefaultPrefs, &fFontPrefs, length);
	if (err == B_NO_ERROR)
		fFontPrefs.SwapBigToHost();
	else
		MDefaultPrefs::SetFontDefaults(fFontPrefs);
	fFontPrefs.pVersion = kCurrentVersion;

	length = sizeof(fAppEditorPrefs);
	err = MPreferences::GetPreference(kAppEditorPrefs, kMWDefaultPrefs, &fAppEditorPrefs, length);
	if (err == B_NO_ERROR)
		fAppEditorPrefs.SwapBigToHost();
	else
	{
		MDefaultPrefs::SetAppEditorDefaults(fAppEditorPrefs);
		// this update is only for mptp users that saw the early colorprefs implementation
		if (MPreferences::PreferenceExists(kColorPrefs, kMWDefaultPrefs))
		{
			length = sizeof(ColorPrefs);
			MPreferences::GetPreference(kColorPrefs, kMWDefaultPrefs, &fAppEditorPrefs, length);
//			MPreferences::RemovePreference(kColorPrefs, kMWDefaultPrefs);
		}

		fAppEditorPrefs.pFlashingDelay = fEditorPrefs.pFlashingDelay;
		fAppEditorPrefs.pBalanceWhileTyping = fEditorPrefs.pBalanceWhileTyping;
	}
	fAppEditorPrefs.pVersion = kCurrentVersion;
	SetHiliteColor(fAppEditorPrefs.pHiliteColor);
	MFunctionPopup::SortAllPopups(fAppEditorPrefs.pSortFunctionPopup);
	MFunctionParser::SetRelaxedCParse(fAppEditorPrefs.pRelaxedPopupParsing);

	length = sizeof(fSyntaxStylePrefs);
	err = MPreferences::GetPreference(kSyntaxStylePrefs, kMWDefaultPrefs, &fSyntaxStylePrefs, length);
	if (err == B_NO_ERROR)
		fSyntaxStylePrefs.SwapBigToHost();
	else
		MDefaultPrefs::SetSyntaxStylingDefaults(fSyntaxStylePrefs);
	fSyntaxStylePrefs.pVersion = kCurrentVersion;

	length = sizeof(fPrivatePrefs);
	err = MPreferences::GetPreference(kPrivatePrefs, kMWDefaultPrefs, &fPrivatePrefs, length);
	if (err == B_NO_ERROR)
		fPrivatePrefs.SwapBigToHost();
	else
		MDefaultPrefs::SetPrivateDefaults(fPrivatePrefs);

	length = sizeof(BuildExtrasPrefs);
	err = MPreferences::GetPreference(kBuildExtrasPrefs, kMWDefaultPrefs, &fBuildExtrasPrefs, length);
	if (err == B_NO_ERROR)
		fBuildExtrasPrefs.SwapBigToHost();
	else {
		MDefaultPrefs::SetBuildExtraDefaults(fBuildExtrasPrefs);
	}

	// We used to read the "New Projects" target setting here
	// Targets are now only related to project settings.
	
	MDefaultPrefs::SetTargetsDefaults(fTargetPrefs);

	// KeyBinding preferences
	if (MPreferences::PreferenceExists(kKeyBindingPrefs, kMWDefaultPrefs, &length))
	{
		char*		data = new char[length];
		if (B_NO_ERROR == MPreferences::GetPreference(kKeyBindingPrefs, kMWDefaultPrefs, data, length))
		{
			// keybindingmanager expects big endian
			BMemoryIO		memData(data, length);
	
			new MKeyBindingManager(memData, kBigEndian, true);
		}
		else
			ASSERT(false);
		
		delete [] data;
	}
	else
	{
		// Build the global KeyBindingManager
		BMallocIO		data;
		
		MKeyBindingManager::BuildDefaultKeyBindings(data);
	
		BMemoryIO		memData(data.Buffer(), data.BufferLength());

		new MKeyBindingManager(memData, kHostEndian, true);
		// Save big endian prefs
	 	BMallocIO		swapData;
		
		MKeyBindingManager::Manager().GetKeyBindingsBig(swapData);

		(void) MPreferences::SetPreference(kKeyBindingPrefs, kMWDefaultPrefs, swapData.Buffer(), swapData.BufferLength());
	}

	// We used to read the access paths preferences here
	// We no longer do.  It is only a project setting.
	// However, access paths are used for OpenSelection when there is no project
	// They are also passed to empty projects, so generate the default settings here.
	 
	MDefaultPrefs::SetAccessPathsDefaults(fAccessPathsPrefs);
	GenerateDefaultAccessPaths();

	// Get the recent document cache
	if (MPreferences::MessagePreferenceExists(kRecentDocumentCache)) {
		BMessage msg;
		if (MPreferences::GetMessagePreference(kRecentDocumentCache, msg) == B_OK) {
			MDynamicMenuHandler::PutRecentDocumentList(msg);
		}
	}
	
	if (MPreferences::PreferenceExists(kProjectPrefs, kMWDefaultPrefs))
		UpdateProjectPrefs();
}

// ---------------------------------------------------------------------------
//		 UpdateProjectPrefs
// ---------------------------------------------------------------------------

void
IDEApp::UpdateProjectPrefs()
{
	ProjectPrefs projectPrefs;
	size_t length = sizeof(projectPrefs);
	status_t err = MPreferences::GetPreference(kProjectPrefs, kMWDefaultPrefs, &projectPrefs, length);
	if (err == B_NO_ERROR) {
		fPrivatePrefs.runsWithDebugger = projectPrefs.pRunsWithDebugger;
	}
}

// ---------------------------------------------------------------------------
//		 GenerateDefaultAccessPaths
// ---------------------------------------------------------------------------

void
IDEApp::GenerateDefaultAccessPaths()
{
	AccessPathData accessPaths[7];
	int32 count = 0;
	int32 i = 0;

	// Get the data into temporary storage, and count up how much we have
	memset(accessPaths, '\0', sizeof(accessPaths));

	while (accessData[i].pathType <= kPathToSystemTree) {
		accessPaths[count].pathType = accessData[i].pathType;
		accessPaths[count].recursiveSearch = accessData[i].recursiveSearch;
		strcpy(accessPaths[count++].pathName, accessData[i++].pathName);
	}

	accessPaths[count].pathType = kPathToProjectTree;
	accessPaths[count].recursiveSearch = true;
	strcpy(accessPaths[count].pathName, kProjectPathName);

	count++;

	ASSERT(count <= 7);

	size_t length = count * sizeof(AccessPathData);
	fAccessPathsPrefs.pSystemPaths = count - 1;
	fAccessPathsPrefs.pProjectPaths = 1;
	
	// Now use the utilities we already have to getting the access paths into
	// fSystemPathList and fProjectPathList
	BMessage msg;
	msg.AddData(kAccessPathsData, kMWDefaultPrefs, &accessPaths[0], length);	
	MFileUtils::GetAccessPathsFromMessage(msg, 
										  fSystemPathList, 
										  fProjectPathList,
										  fAccessPathsPrefs.pSystemPaths, 
										  fAccessPathsPrefs.pProjectPaths);


	// Cache fSystemPathList into fSystemDirectories for use in OpenSelection lookup
	MFileUtils::BuildDirectoriesList(fSystemPathList, fSystemDirectories, nil);
}

// ---------------------------------------------------------------------------
//		 SetPrefs
// ---------------------------------------------------------------------------
//	Save all of the preferences structs in the prefs file.

void
IDEApp::SetPrefs()
{
	size_t				length;

	// Swapping twice may not be great, some timings should be done
	length = sizeof(fEditorPrefs);
	fEditorPrefs.SwapHostToBig();
	(void) MPreferences::SetPreference(kEditorPrefs, kMWDefaultPrefs, &fEditorPrefs, length);
	fEditorPrefs.SwapBigToHost();
	
	length = sizeof(fFontPrefs);
	fFontPrefs.SwapHostToBig();
	(void) MPreferences::SetPreference(kFontPrefs, kMWDefaultPrefs, &fFontPrefs, length);
	fFontPrefs.SwapBigToHost();

	length = sizeof(fAppEditorPrefs);
	fAppEditorPrefs.SwapHostToBig();
	(void) MPreferences::SetPreference(kAppEditorPrefs, kMWDefaultPrefs, &fAppEditorPrefs, length);
	fAppEditorPrefs.SwapBigToHost();

	length = sizeof(fSyntaxStylePrefs);
	fSyntaxStylePrefs.SwapHostToBig();
	(void) MPreferences::SetPreference(kSyntaxStylePrefs, kMWDefaultPrefs, &fSyntaxStylePrefs, length);
	fSyntaxStylePrefs.SwapBigToHost();

	length = sizeof(fPrivatePrefs);
	fPrivatePrefs.SwapHostToBig();
	(void) MPreferences::SetPreference(kPrivatePrefs, kMWDefaultPrefs, &fPrivatePrefs, length);
	fPrivatePrefs.SwapBigToHost();

	length = sizeof(fBuildExtrasPrefs);
	fBuildExtrasPrefs.SwapHostToBig();
	(void) MPreferences::SetPreference(kBuildExtrasPrefs, kMWDefaultPrefs, &fBuildExtrasPrefs, length);
	fBuildExtrasPrefs.SwapBigToHost();

	// Save the recent document cache
	BMessage msg;
	MDynamicMenuHandler::GetRecentDocumentList(msg);
	MPreferences::SetMessagePreference(kRecentDocumentCache, msg);
}

// ---------------------------------------------------------------------------
//		 RemovePreferences
// ---------------------------------------------------------------------------

void
IDEApp::RemovePreferences()
{
	(void) MPreferences::RemovePreference(kEditorPrefs, kMWDefaultPrefs);
	(void) MPreferences::RemovePreference(kFontPrefs, kMWDefaultPrefs);
	(void) MPreferences::RemovePreference(kAppEditorPrefs, kMWDefaultPrefs);
	(void) MPreferences::RemovePreference(kSyntaxStylePrefs, kMWDefaultPrefs);
	(void) MPreferences::RemovePreference(kPrintPrefs, kMWDefaultPrefs);
	(void) MPreferences::RemovePreference(kKeyBindingPrefs, kMWDefaultPrefs);
	(void) MPreferences::RemovePreference(kPrivatePrefs, kMWDefaultPrefs);
	(void) MPreferences::RemovePreference(kBuildExtrasPrefs, kMWDefaultPrefs);
	(void) MPreferences::RemoveMessagePreference(kRecentDocumentCache);

	// No longer saved, but could be sitting around from previous release
	(void) MPreferences::RemovePreference(kAccessPathsPrefs, kMWDefaultPrefs);
	(void) MPreferences::RemovePreference(kAccessPathsData, kMWDefaultPrefs);
	(void) MPreferences::RemovePreference(kTargetPrefs, kMWDefaultPrefs);
	(void) MPreferences::RemovePreference(kTargetBlockPrefs, kMWDefaultPrefs);
	(void) MPreferences::RemovePreference(kGenericPrefs, kMWDefaultPrefs);

}

// ---------------------------------------------------------------------------

void
IDEApp::RemoveAllPreferences()
{
	RemovePreferences();
	MMessageWindow::RemovePreferences();
	MFindWindow::RemovePreferences();
	MPreferencesWindow::RemovePreferences();
	MOpenSelectionWindow::RemovePreferences();
}

// ---------------------------------------------------------------------------

void
IDEApp::RemoveProject(MProjectWindow* project)
{
	BAutolock listLock(&fProjectListLock);
	
	// quick safety check...
	if (project == nil) {
		return;
	}
		
	// Remove the window from its position in the list if it exists
	fProjectList.RemoveItem(project);

	// If we removed the current project, we need to tell everyone
	// that we have a new current project
	if (project == fCurrentProject) {

		// The add file panel goes along with the current project
		// If it is open, shut it down...
		if (fFilePanelStatus == opAddingFiles) {
			this->CloseFilePanel();
		}

		this->NewCurrentProject(fProjectList.LastItem());
	}
}

// ---------------------------------------------------------------------------

void
IDEApp::ProjectActivated(MProjectWindow* project)
{
	// Make project be the current project
	// 1. Rearrange the list hierarchy
	// 2. Cache it in fCurrentProject
		
	BAutolock listLock(&fProjectListLock);
	
	// Remove the window from its position in the list if it exists
	// and add it to the end
	fProjectList.RemoveItem(project);
	fProjectList.AddItem(project);
	
	// Cache the new most recent project in fCurrentProject
	this->NewCurrentProject(fProjectList.LastItem());
}

// ---------------------------------------------------------------------------

void
IDEApp::NewCurrentProject(MProjectWindow* newProject)
{
	// When we get a new current project we
	// 1. Cache it in fCurrentProject
	// 2. Notify the add-ons so they know what project we are working with
	
	MProjectWindow* oldCurrent = fCurrentProject;
	fCurrentProject = newProject;	
}

// ---------------------------------------------------------------------------

MProjectWindow*
IDEApp::GetBackmostProject()
{
	BAutolock listLock(&fProjectListLock);
	return fProjectList.ItemAt(0);
}

// ---------------------------------------------------------------------------

MProjectWindow*
IDEApp::CurrentProject()
{
	BAutolock listLock(&fProjectListLock);
	return fCurrentProject;
}

// ---------------------------------------------------------------------------

MProjectWindow*
IDEApp::IsOpenProject(const entry_ref& inRef)
{
	BAutolock listLock(&fProjectListLock);

	// Iterate through our open projects and see if any match
	MProjectWindow* matchingProject = nil;
	
	int32 projectCount = fProjectList.CountItems();
	for (int32 i = 0; i < projectCount; i++) {
		MProjectWindow* project = fProjectList.ItemAt(i);
		if (project->FileIsProjectFile(inRef)) {
			matchingProject = project;
			break;
		}
	}
	
	return matchingProject;
}

// ---------------------------------------------------------------------------

void
IDEApp::MessageToAllProjectWindows(BMessage& inMessage)
{
	BAutolock listLock(&fProjectListLock);

	// Iterate through our open projects - send the message	
	int32 projectCount = fProjectList.CountItems();
	for (int32 i = 0; i < projectCount; i++) {
		fProjectList.ItemAt(i)->PostMessage(&inMessage);
	}
}
