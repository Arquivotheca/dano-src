//========================================================================
//	MBuildCommander.cpp
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "MAlert.h"
#include "MBuildCommander.h"
#include "MBuildersKeeper.h"
#include "MCompilerObj.h"
#include "MDeferredScriptHandler.h"
#include "MDynamicMenuHandler.h"
#include "MMessageWindow.h"
#include "MLauncher.h"
#include "MLinkObj.h"
#include "MPlugInLinker.h"
#include "MPreCompileObj.h"
#include "MProject.h"
#include "MPCHFileLine.h"
#include "MSourceFileLine.h"
#include "MSubProjectFileLine.h"
#include "MStartBuildThread.h"
#include "MScriptUtils.h"
#include "MScripting.h"

#include "IDEMessages.h"
#include "ProjectCommands.h"
#include "Scripting.h"

#include <OS.h>
#include <Roster.h>

const int32 kOneConcurrentCompile = 1;
const status_t kNoSymFile = 28;			// whatever
const status_t kNoDebugger = 29;		// whatever
const status_t kPluginProblem = 30;		// whatever

// ---------------------------------------------------------------------------
//		MBuildCommander
// ---------------------------------------------------------------------------
//	Constructor

MBuildCommander::MBuildCommander(
	MProjectView&		inProjectView,
	MProjectLineList&	inFileList,
	BLocker&			inFileListLocker)
	: fProjectView(inProjectView),
	fFileList(inFileList),
	fFileListLocker(inFileListLocker),
	fLock("buildcommanderprefslock")
{
	fWindow = nil;
	fConcurrentCompiles = 1;
	fCompilingState = sNotCompiling;
	fNeedsToLink = false;
	fStopOnErrors = false;
	fAutoOpenErrorWindow = true;
	fLaunchCommand = DONT_LAUNCH;
	fCompileObj = nil;
	fScriptHandler = nil;
	fBuildWaiter = nil;
	fBuildThread = nil;
	fLinker = nil;
}

// ---------------------------------------------------------------------------
//		~MBuildCommander
// ---------------------------------------------------------------------------
//	Destructor

MBuildCommander::~MBuildCommander()
{
	Cancel();

	while (B_NAME_NOT_FOUND != find_thread("CompileServe"))
		snooze(100000);

	EmptyTargetList();
}

// ---------------------------------------------------------------------------
//		EmptyTargetList
// ---------------------------------------------------------------------------

void
MBuildCommander::EmptyTargetList()
{
	// Empty the target list
	ProjectTargetRec*		targetRec;
	int32					i = 0;

	while (fTargetList.GetNthItem(targetRec, i++))
		delete targetRec;

	fTargetList.MakeEmpty();
}

// ---------------------------------------------------------------------------
//		InitCommander
// ---------------------------------------------------------------------------
//	Build the list of targets/builders.

void
MBuildCommander::InitCommander(
	const TargetRec*	inRecArray,
	int32				inRecCount)
{
	EmptyTargetList();
	MBuildersKeeper::BuildTargetList(inRecArray, inRecCount, fTargetList);
}

// ---------------------------------------------------------------------------
//		ClearMessages
// ---------------------------------------------------------------------------

inline void					
MBuildCommander::ClearMessages()
{
	fProjectView.GetErrorMessageWindow()->ClearMessages();
}

// ---------------------------------------------------------------------------
//		GetTargetRec
// ---------------------------------------------------------------------------
//	Get the target rec that matches the specified file type and extension.
//	Either file type or extension can be empty but not both.  returns nil
//	if there is no match,
//	If the target rec has both file type and extension they must match exactly.
//	If the target rec has no file type it matches any file type.
//	If the target rec has no extension it matches any extension.

ProjectTargetRec*
MBuildCommander::GetTargetRec(
	const char*		inMimeType,
	const char*		inExtension)
{
	ASSERT(inMimeType[0] != 0 || inExtension[0] != 0);
	
	ProjectTargetRec*	rec;
	ProjectTargetRec*	result = nil;
	int32				i = 0;

	while (fTargetList.GetNthItem(rec, i++))
	{
		bool	hasFileType = rec->Target.MimeType[0] != '\0';
		bool	hasExtension = rec->Target.Extension[0] != '\0';
		bool	FileTypesMatch;
		bool	ExtensionsMatch;

		if (hasExtension)
			ExtensionsMatch = (0 == strcmp(inExtension, rec->Target.Extension));
		if (hasFileType)
		{
			int32	matchLen = strlen(rec->Target.MimeType) - 1;
			bool	hasWildCard = rec->Target.MimeType[matchLen] == '*';

			if (hasWildCard)
				FileTypesMatch = (0 == strncmp(inMimeType, rec->Target.MimeType, matchLen));
			else
				FileTypesMatch = (0 == strcmp(inMimeType, rec->Target.MimeType));
		}

		if (hasFileType && hasExtension)
		{
			if (ExtensionsMatch && FileTypesMatch)
			{
				result = rec;
				break;
			}
		}
		else
		if (hasFileType)
		{
			if (FileTypesMatch)
			{
				result = rec;
				break;				
			}
		}
		else
		if (hasExtension)
		{
			if (ExtensionsMatch)
			{
				result = rec;
				break;							
			}
		}
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//		GetTargetRec
// ---------------------------------------------------------------------------
//	Get the target rec for this file.

ProjectTargetRec*
MBuildCommander::GetTargetRec(
	const BEntry&			inFile)
{
	ProjectTargetRec*	rec = nil;
	FileNameT			name;
	mime_t				mimeType = { '\0' };
	
	if (B_NO_ERROR == inFile.GetName(name))
	{
		BFile			fileformime(&inFile, B_READ_ONLY);
		BNodeInfo		mimefile(&fileformime);
		status_t		err = mimefile.GetType(mimeType);

		if (B_NO_ERROR == err || ENOENT == err)		// ENOENT indicates no file type
		{
			const char *		extension = strrchr(name, '.');
			if (extension != nil)
				extension++;
			else
				extension = "";

			rec = GetTargetRec(mimeType, extension);
		}
	}
	
	return rec;
}

// ---------------------------------------------------------------------------
//		GetToolPath
// ---------------------------------------------------------------------------
//	Get the path to this tool.

void
MBuildCommander::GetToolPath(
	ProjectTargetRec*	inRec,
	char*				outPath,
	MakeStageT			inStage,
	MakeActionT			inAction)
{
	ASSERT(inRec);

	char		path[1000] = { 0 };
	status_t	err;
	
	err = inRec->Builder->builder->GetToolName(&fProjectView.BuilderProject(), path, 1000, inStage, inAction);

	if (err == B_NO_ERROR && path[0] != 0)
	{
		const char *	p = strrchr(path, '/');

		if (path[0] == '/')
		{
			// full path
			strcpy(outPath, path);
		}
		else
		{
			// partial path or just the toolname
			BEntry		tool;

			if ((B_OK == MFileUtils::ToolsDirectory().FindEntry(path, &tool, true) && tool.IsFile()) ||
				(B_OK == MFileUtils::BinDirectory().FindEntry(path, &tool, true)) && tool.IsFile())
			{
				BPath		path1;

				tool.GetPath(&path1);
				strcpy(outPath, path1.Path());
			}
			else
			{
				// this will cause an error message about not being
				// able to find the tool
				strcpy(outPath, "/boot/develop/BeIDE/tools/");
				strcat(outPath, path);	
			}
		}
	}
}

// ---------------------------------------------------------------------------
//		GetToolPath
// ---------------------------------------------------------------------------
//	Get the path to this tool.

void
MBuildCommander::GetToolPath(
	MPlugInBuilder*		inBuilder,
	char*				outPath,
	MakeStageT			inStage,
	MakeActionT			inAction)
{
	ProjectTargetRec	rec;
	BuilderRec			builderRec;
	
	builderRec.builder = inBuilder;
	builderRec.next = nil;
	rec.Builder = &builderRec;
	
	GetToolPath(&rec, outPath, inStage, inAction);
}

// ---------------------------------------------------------------------------
//		BuildAction
// ---------------------------------------------------------------------------
//	Start a bring up to date, make, link.

void
MBuildCommander::BuildAction(
	uint32 inCommand)
{
	fCompileObj = nil;
	
	if (inCommand == cmd_Run || inCommand == cmd_RunOpposite)
	{
		fLaunchCommand = inCommand;
		inCommand = cmd_Make;
	}
	else
		fLaunchCommand = DONT_LAUNCH;

	switch (inCommand)
	{
		case cmd_Compile:
			CompileSelection();
			break;

		case cmd_BringUpToDate:
			StartBuild(sUpdatePrecompile);
			break;

		case cmd_Make:
			StartBuild(sMakePrecompile);
			break;

		case CMD_LINK:
			Window()->Activate();
			ClearMessages();
			fNeedsToLink = true;	// Force it to link
			DoLink();
			break;

		case cmd_Precompile:
			PrecompileSelection();
			break;

		case cmd_Preprocess:
			PreprocessSelection();
			break;

		case cmd_CheckSyntax:
			CheckSyntaxSelection();
			break;

		case cmd_Disassemble:
			DisassembleSelection();
			break;
	}
}

// ---------------------------------------------------------------------------
//		BuildAction
// ---------------------------------------------------------------------------
//	Start a compile one, precompile one, preprocess one, checksyntax one.

void
MBuildCommander::BuildAction(
	BMessage& inMessage)
{
	fCompileObj = nil;
	
	switch (inMessage.what)
	{
		case msgCompileOne:
			DoCompileOne(inMessage);
			break;

		case msgPreCompileOne:
			DoPreCompileOne(inMessage);
			break;

		case msgPreProcessOne:
			DoPreprocessOne(inMessage);
			break;

		case msgCheckSyntaxOne:
			DoCheckSyntaxOne(inMessage);
			break;

		case msgDisassembleOne:
			DoDisassembleOnePartOne(inMessage);
			break;

		case msgMakeAndReply:
			DoBuildAndReply();
			break;
	}
}

// ---------------------------------------------------------------------------

void
MBuildCommander::DoBuildAndReply()
{
	// Grab control of the message so that we can reply to it when done
	// (DoBuildAndReply duplicates some of the scripting actions but allows me
	// to not break scripting but get back the information I need in this case)
	
	LockData();
	fBuildWaiter = Window()->DetachCurrentMessage();
	this->StartBuild(sMakePrecompile);
	UnlockData();
}

// ---------------------------------------------------------------------------
//		PerformScriptAction
// ---------------------------------------------------------------------------

status_t
MBuildCommander::PerformScriptAction(
	BMessage *				message,
	BMessage * &			reply,
	bool&					wasDeferred)
{
	fCompileObj = nil;
	status_t		result;
	
	switch (message->what)
	{
		case kMakeProject:
			// If the project is currently building, the script driver
			// adopts the current build as its own... (see MBuildCommander::LastState)
			LockData();
			fScriptHandler = new MDeferredScriptHandler(NewScriptID(), "builder", *fWindow);
			result = fScriptHandler->PerformScriptAction(message, reply, wasDeferred);
			// If the project isn't currently building start it up
			if (CompilingState() == sNotCompiling) {
				StartBuild(sMakePrecompile);
			}
			UnlockData();
			break;

		default:
			result = SCRIPT_BAD_VERB;
			break;
	}

	return result;
}

// ---------------------------------------------------------------------------
//		CompileDone
// ---------------------------------------------------------------------------
//	Called from the compiler generator object when it's done compiling.

void
MBuildCommander::CompileDone()
{
	bool	errorFree = fProjectView.GetErrorMessageWindow()->Errors() == 0;

	if (! errorFree)
	{
		LastState();
	}
	else
	switch (fCompilingState)
	{
		case sCompilingOne:
		case sCompilingSelection:
		case sPreCompilingOne:
		case sPreCompilingSelection:
		case sUpdateCompile:
		case sPreProcessingOne:
		case sPreProcessingSelection:
		case sCheckingSyntaxOne:
		case sCheckingSyntaxSelection:
		case sDisassemblingOnePartTwo:
		case sPostLinkFinal:
			LastState();
			break;

		case sMakePrecompile:
			MakeStepOneExtra(sMakeSubproject);
			break;
	
		case sMakeSubproject:
			MakeStepTwo(sMakeCompile);
			break;
			
		case sUpdatePrecompile:
			MakeStepTwo(sUpdateCompile);
			break;

		case sMakeCompile:
			DoLink();
			break;

		case sLinking:
			MakeStepFour();
			break;

		case sCopyingResources:
			MakeStepFourFinal();
			break;

		case sPostLinkExecute:
			if (fLaunchCommand == DONT_LAUNCH)
				LastState();
			else
				Launch();
			break;

		case sCancelling:
			if (B_NAME_NOT_FOUND == find_thread("CompileServe"))
				LastState();
			break;

		case sDisassemblingOnePartOne:
		case sDisassemblingSelection:
			DisassemblePartTwo();
			break;

		default:
			ASSERT(false);
	}
}

// ---------------------------------------------------------------------------
//		LastState
// ---------------------------------------------------------------------------
//	Reset the compiling state to idle.

void
MBuildCommander::LastState()
{
	bool buildCancelled = fCompilingState == sCancelling;
	
	int32		warnings = fProjectView.GetErrorMessageWindow()->Warnings();
	int32		errors = fProjectView.GetErrorMessageWindow()->Errors();

	if ((warnings != 0 || errors != 0) && fAutoOpenErrorWindow == true) {
		fProjectView.GetErrorMessageWindow()->PostMessage(msgShowAndActivate);
	}

	LockData();
	fCompilingState = sNotCompiling;
	fNeedsToLink = false;
	Window()->PostMessage(cmd_Save);
	Window()->PostMessage(msgShowIdleStatus, &fProjectView);
	
	// If this build was started by a script (or adopted by a script) then send the reply
	// (Since scripting is already up and running, I don't want to break it.  Otherwise,
	// I could share the implementation between the waiter and the script handler)
	if (fScriptHandler) {
		status_t err = B_NO_ERROR;
		if (0 != errors) {
			err = M_MAKE_ERRORS;
		}
		else if (0 != warnings) {
			err = M_MAKE_WARNINGS;
		}
		
		fScriptHandler->SendReply(err, nil);
		delete fScriptHandler;
		fScriptHandler = nil;
	}
	
	// If someone is waiting for this build to finish, send them a reply
	
	if (fBuildWaiter) {
		BMessage msg;
		msg.AddInt32("errors", errors);
		msg.AddInt32("warnings", warnings);
		status_t reportStatus = B_OK;
		if (buildCancelled) {
			reportStatus = B_CANCELED;
		}
		else if (errors > 0) {
			reportStatus = B_ERROR;
		}
		msg.AddInt32("status", reportStatus);
		fBuildWaiter->SendReply(&msg);
		delete fBuildWaiter;
		fBuildWaiter = nil;
	}
	
	UnlockData();
}

// ---------------------------------------------------------------------------
//		Cancel
// ---------------------------------------------------------------------------
//	Go into cancel mode.

void
MBuildCommander::Cancel()
{
	if (fCompilingState != sCancelling && fCompilingState != sNotCompiling)
	{
		fCompilingState = sCancelling;

		Kill();
	}
}

// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------

bool
MBuildCommander::IsIdle()
{
	// Why do we also check fBuildThread?  Because fCompilingState can be set
	// back to sNotCompiling before fBuildThread is cleared.  This can
	// happen when there is nothing to do.  The MakeStepOne ... MakeStepFourFinal
	// will call each other running the fBuildThread and then finally return
	// and clear fBuildThread.  This means that for a moment after sNotCompiling
	// is set, we are still busy with fBuildThread.
	// Allowing StartBuild to create assign a new fBuildThread leads to bad things!

	return fCompilingState == sNotCompiling && fBuildThread == nil;
}

// ---------------------------------------------------------------------------

void
MBuildCommander::Kill()
{
	LockData();
	if (fBuildThread)
	{
		thread_id	thread = fBuildThread->Thread();
		int32		status;
		fBuildThread->Cancel();
		UnlockData();
		wait_for_thread(thread, &status);
	}
	else
		UnlockData();

	if (fCompileObj)
		fCompileObj->Kill();
	else
		fCompileGenerator.Kill();
}

// ---------------------------------------------------------------------------
//		ShowCompileStatus
// ---------------------------------------------------------------------------
//	Show the compile status in the status caption.

void
MBuildCommander::ShowCompileStatus()
{
	if (fCompilingState != sNotCompiling)
	{
		String			text;
		BMessage		msg(msgSetStatusText);

		switch(fCompilingState)
		{
			case sCompilingOne:
			case sCompilingSelection:
			case sMakeCompile:
			case sUpdateCompile:
				text += "Compiling:  ";
				break;

			case sPreCompilingOne:
			case sPreCompilingSelection:
			case sMakePrecompile:
			case sUpdatePrecompile:
				text += "Precompiling:  ";
				break;

			case sMakeSubproject:
				text += "Building sub-project: ";
				break;

			case sPreProcessingOne:
			case sPreProcessingSelection:
				text += "Preprocessing:  ";
				break;

			case sCheckingSyntaxOne:
			case sCheckingSyntaxSelection:
				text += "Checking Syntax:  ";
				break;

			case sDisassemblingOnePartOne:
			case sDisassemblingOnePartTwo:
			case sDisassemblingSelection:
				text += "Disassembling:  ";
				break;

			case sPostLinkExecute:
			case sPostLinkFinal:
				text += "Post Link:  ";
				break;

			case sLinking:
				text += "Linking";
				break;

			case sCancelling:
				text += "Cancelling";
				break;

			case sBeginMake:
				text += "Starting Build";
				break;
		}
		
		switch(fCompilingState)
		{
			case sLinking:
			case sCancelling:
			case sBeginMake:
				// Do nothing
				break;
			
			default:
				// Append the file count
				text += fCompleted;
				if (fCompleted == 1)
					text += " file completed of ";			
				else
					text += " files completed of ";
				text += fToBeCompleted;
				break;
		}

		msg.AddString(kText, text);

		Window()->PostMessage(&msg);
	}
}

// ---------------------------------------------------------------------------
//		ProjectChanged
// ---------------------------------------------------------------------------
//	Called from the project view when files are removed from
//	the project or files are rearranged in the project.  While
//	this will usually not really require a relink it could.

void
MBuildCommander::ProjectChanged()
{
	fNeedsToLink = true;
}

// ---------------------------------------------------------------------------
//		RunMenuItemChanged
// ---------------------------------------------------------------------------

void
MBuildCommander::RunMenuItemChanged(
	MProject&	inProject)
{
	if (fLinker != nil)
		fLinker->ProjectChanged(inProject, kRunMenuItemChanged);
}

// ---------------------------------------------------------------------------
//		OneCompileDone
// ---------------------------------------------------------------------------
//	Called from each file compiler object when it's done compiling.

void
MBuildCommander::OneCompileDone()
{
	fCompleted++;

	ShowCompileStatus();
}

// ---------------------------------------------------------------------------
//		StartBuild
// ---------------------------------------------------------------------------

void
MBuildCommander::StartBuild(
	CompileState inState)
{
	// Make sure that we are not busy before we start up another build.
	// The compiling state used to be changed in MakeStepOne, but that 
	// would allow a second thread to get in here and start up before 
	// we got to the transition sNotCompiling -> sBeginMake.
		
	LockData();
	if (this->IsIdle()) {
		fCompilingState = sBeginMake;
		fBuildThread = new MStartBuildThread(*this, inState);
		fBuildThread->Run();
	}
	UnlockData();
}

// ---------------------------------------------------------------------------
//		MakeStepOne
// ---------------------------------------------------------------------------

void
MBuildCommander::MakeStepOne(
	CompileState	inState)
{
	// The precompile step in a Make or a Bring Up to Date.
	// Actually two actions happen in MakeStepOne
	// Subprojects are built
	// Precompiled headers are compiled
	
	if (fCompilingState == sBeginMake)
	{
		ClearMessages();
		Window()->Activate();
		ShowCompileStatus();
	
		// Save all open text files before starting a build
		MDynamicMenuHandler::SaveAllTextWindows();
		
		fProjectView.InvalidateModificationDates();

		// Fill the compile list with sourceFileLine objects that need to be
		// precompiled
				
		fCompileList.MakeEmpty();
	
		MProjectLine*	projectLine;
		int32			i = 0;
		bool			cancelled = false;
		bool			more = true;
		bool			usesFileCache = false;
		
		while (more)
		{
			fFileListLocker.Lock();

			more = (fFileList.GetNthItem(projectLine, i++) && ! cancelled);
			if (more)
			{
				// cast to PCHFileLine to separate out MSubProjectFileLine objects
				
				MPCHFileLine* line = dynamic_cast<MPCHFileLine*>(projectLine);
				if (line) {
					if (! line->FileExists())
						PostNoFileMessage(line);
					else
					if (line->NeedsToBePreCompiled())
					{
						fCompileList.AddItem(line);
						if (!usesFileCache)
							usesFileCache = line->UsesFileCache();
					}
				}
				cancelled = fBuildThread->Cancelled();
			}
		
			fFileListLocker.Unlock();
		}
		
		if (cancelled)
			fCompilingState = sCancelling;
		else
			fCompilingState = inState;

		fCompleted = 0;
		fToBeCompleted = fCompileList.CountItems();

		// Tell the compile generator to start the compile
		if (fCompileList.CountItems() > 0 && ! cancelled)
		{
			MFileGetter*	fileGetter = nil;
			if (usesFileCache)
				fileGetter = &fProjectView;

			LockData();
			fCompileGenerator.StartCompile(kOneConcurrentCompile, 
						&fCompileList, Window(), true, kPrecompile, fileGetter);
			UnlockData();

			fCompileGenerator.Run();
	
			ShowCompileStatus();
			
			fNeedsToLink = true;
		}
		else {
			CompileDone();
		}
	}	

	// Now that we have started the build running - we set fBuildThread to nil
	// because it will delete itself as we return from this method
	LockData();
	fBuildThread = nil;
	UnlockData();
}

// ---------------------------------------------------------------------------

void
MBuildCommander::MakeStepOneExtra(CompileState inState)
{
	// The build sub projects step (only in a Make)
	
	ASSERT(fCompilingState == sMakePrecompile);

	fCompilingState = inState;
	
	fProjectView.InvalidateModificationDates();

	// Fill the compile list with MSubProjectFileLine objects that need to be
	// built - In this case we have to dynamic_cast to MSubProjectFileLine
	// Also - we do not set the fNeedsToLink flag.  That will be set
	// by any side effects of the building subprojects

	fCompileList.MakeEmpty();

	bool				usesFileCache = false;
	MProjectLine*		projectLine;
	int32				i = 0;

	while (fFileList.GetNthItem(projectLine, i++))
	{
		MSubProjectFileLine* line = dynamic_cast<MSubProjectFileLine*>(projectLine);
		if (line) {
			if (line->FileExists() == false) {
				PostNoFileMessage(line);
			}
			else if (line->NeedsToBePreCompiled()) {
				fCompileList.AddItem(line);
			}
		}
	}

	fCompleted = 0;
	fToBeCompleted = fCompileList.CountItems();

	// Tell the compile generator to start the compile
	if (fCompileList.CountItems() > 0)
	{
		LockData();
		fCompileGenerator.StartCompile(kOneConcurrentCompile,
			 &fCompileList, Window(), true, kCompile, nil);
		UnlockData();
	
		fCompileGenerator.Run();

		ShowCompileStatus();
	}
	else
		CompileDone();
}

// ---------------------------------------------------------------------------
//		MakeStepTwo
// ---------------------------------------------------------------------------
//	The compile step in a Make or Bring Up to Date.

void
MBuildCommander::MakeStepTwo(
	CompileState	inState)
{
	ASSERT(fCompilingState == sMakeSubproject || fCompilingState == sUpdatePrecompile);

	fCompilingState = inState;
	
	fProjectView.InvalidateModificationDates();

	// Fill the compile list with sourceFileLine objects that need to be
	// compiled
	fCompileList.MakeEmpty();

	bool				usesFileCache = false;
	MProjectLine*		projectLine;
	int32				i = 0;

	while (fFileList.GetNthItem(projectLine, i++))
	{
		MSourceFileLine* line = dynamic_cast<MSourceFileLine*>(projectLine);
		ASSERT(line);
		if (! line->FileExists())
			PostNoFileMessage(line);
		else
		if (line->NeedsToBeCompiled())
		{
			fCompileList.AddItem(line);
			if (!usesFileCache)
				usesFileCache = line->UsesFileCache();
		}
	}

	fCompleted = 0;
	fToBeCompleted = fCompileList.CountItems();

	// Tell the compile generator to start the compile
	if (fCompileList.CountItems() > 0)
	{
		MFileGetter*	fileGetter = nil;
		if (usesFileCache)
			fileGetter = &fProjectView;

		LockData();
		fCompileGenerator.StartCompile(fConcurrentCompiles,
			 &fCompileList, Window(), fStopOnErrors, kCompile, fileGetter);
		UnlockData();
	
		fCompileGenerator.Run();

		ShowCompileStatus();
		fNeedsToLink = true;
	}
	else
		CompileDone();
}

// ---------------------------------------------------------------------------
//		NeedsToLink
// ---------------------------------------------------------------------------
//	Do we need to link when part of a make or run?

bool
MBuildCommander::NeedsToLink()
{
	// fNeedsToLink will be true if any files were precompiled
	// or compiled during this build
	if (! fNeedsToLink)
	{
		GetExecutableRef();
		
		// If nothing was precompiled or compiled we need
		// to link if the executable doesn't exist
		BEntry		executable(&fExecutableRef);
		fNeedsToLink = ! executable.Exists();

		// Also need to link if a .o file is more recent than the executable
		// This can happen if a file is changed, then cmd-K, then cmd-R.
		if (! fNeedsToLink)
		{
			time_t		executableModTime;
 			
			if (B_NO_ERROR != executable.GetModificationTime(&executableModTime))
			{
				fNeedsToLink = true;
			}
			else
			{
				int32		count = fFileList.CountItems();
				BEntry		ofile;
				BList		targets;
				entry_ref	ref;
				
				for (int i = 0; i < count && !fNeedsToLink ; i++)
				{
					MProjectLine* projectLine = fFileList.ItemAtFast(i);
					MSourceFileLine* line = dynamic_cast<MSourceFileLine*>(projectLine);
					ASSERT(line != nil);
					
					targets.MakeEmpty();

					line->FillTargetFilePathList(targets);
				
					for (int32 j = 0; j < targets.CountItems(); ++j)
					{
						char *		filePath = (char*) targets.ItemAtFast(j);
						ASSERT(filePath != nil);
				
						if (filePath != nil) 
						{
							if (B_NO_ERROR == get_ref_for_path(filePath, &ref))
							{
								ofile.SetTo(&ref);
								time_t		oModTime;
								
								if (B_NO_ERROR != ofile.GetModificationTime(&oModTime) ||
									oModTime > executableModTime)
								{
									fNeedsToLink = true;
								}
							}
						
							free(filePath);
						}
					}
				}
			}
		}
	}
	
	return fNeedsToLink;
}

// ---------------------------------------------------------------------------
//		DoLink
// ---------------------------------------------------------------------------
//	Tell the compiler to link this project.
//	This is make step 3.

void
MBuildCommander::DoLink()
{
	fCompilingState = sLinking;

	ASSERT(fLinker != nil);

	if (! NeedsToLink())
		CompileDone();
	else
	if (fLinker != nil)
	{
		char			compilerPath[kPathSize] = "";
//		char			compilerPath[kPathSize] = "/boot/develop/tools/mwld";
		int32			count = fFileList.CountItems() + 40;	// This is a maximum
		BList			argvList(count);
		bool			ideAware = ((fLinker->Flags() & kIDEAware) != 0);

		fLinker->BuildLinkArgv(fProjectView.BuilderProject(), argvList);

		GetToolPath(fLinker, compilerPath, kLinkStage, kLink);
	
		fCompileObj = new MLinkObj(compilerPath, argvList, ideAware, fProjectView);
	
		int32	err = fCompileObj->Run();
		if (B_NO_ERROR == err)
			ShowCompileStatus();
		else
		{
			//	Error!
			DEBUGGER("Error starting compiler\n");
			delete fCompileObj;
			fCompileObj = nil;
			CompileDone();
		}
	}
	else {
		// Linker doesn't exist - we must be done
		CompileDone();
	}
}

// ---------------------------------------------------------------------------
//		ParseLinkerMessageText
// ---------------------------------------------------------------------------
//	For non IDE aware tools, this provides a call back from the MLinkObj so
//	that errors and warnings can be generated.
//	(added by John Dance)


status_t
MBuildCommander::ParseLinkerMessageText(
	const char*	inText,
	BList& outList)
{
	ASSERT(fLinker != nil);

	return fLinker->ParseMessageText(fProjectView.BuilderProject(), inText, outList);
}


// ---------------------------------------------------------------------------
//		MakeStepFour
// ---------------------------------------------------------------------------
//	The post link step.

void
MBuildCommander::MakeStepFour()
{
	fCompilingState = sCopyingResources;

	// If we didn't link then an executable already exists that has had
	// the resources copied to it.  So we don't do the resource copy step
	// if we didn't link on this build.

	// Copy resources to the executable
	SetExecutableRef();
	{
		BFile				app(&fExecutableRef, B_READ_WRITE);

		if (B_NO_ERROR == app.InitCheck())
		{
			BResources			appFile(&app);
			String				resFileName;
			bool				gotData = false;
		
			MProjectLine*		projectLine;
			int32				i = 0;
		
			while (fFileList.GetNthItem(projectLine, i++))
			{
				MSourceFileLine* line = dynamic_cast<MSourceFileLine*>(projectLine);
				ASSERT(line);
				bool replace = line->NeedsResourcesToBeCopied();	// Was file touched or modified?

				if (line->HasResources() && (fNeedsToLink || replace))
				{
					line->ShowCompileMark();
					line->UpdateLine();
		
					entry_ref		fromRef;
					if (B_NO_ERROR == line->GetSourceFile()->GetRef(fromRef))
					{		
						BFile				resFileFile(&fromRef, B_READ_ONLY);
						BResources			resFile(&resFileFile);

						if (B_NO_ERROR == resFileFile.InitCheck())
						{
							CopyAllResources(resFile, appFile, fromRef, replace);
						}
							
						line->ResourcesWereCopied();
						line->ShowCompileMark(false);
						line->UpdateLine();
					}
				}
			}
		}
	}

	CompileDone();
}

// ---------------------------------------------------------------------------
//		MakeStepFourFinal
// ---------------------------------------------------------------------------
//	The second part of the post link step.

void
MBuildCommander::MakeStepFourFinal()
{
	fCompilingState = sPostLinkFinal;
	
	// Let the projectview tell the linkerbuilder that linking is
	// done so that it can do things to the executable
	// (but only if linking really happened)
	if (fLinker != nil && fNeedsToLink == true)
		fProjectView.LinkDone(fLinker);

	ExecuteStageFourLines();	// This has to go at the very end
}

// ---------------------------------------------------------------------------
//		ExecuteStageFourLines
// ---------------------------------------------------------------------------

void
MBuildCommander::ExecuteStageFourLines()
{
	fCompilingState = sPostLinkExecute;

	// Fill the compile list with sourceFileLine objects that need to be
	// compiled
	fProjectView.InvalidateModificationDates();
	fCompileList.MakeEmpty();

	MProjectLine*	projectLine;
	int32			i = 0;

	while (fFileList.GetNthItem(projectLine, i++))
	{	
		MSourceFileLine* line = dynamic_cast<MSourceFileLine*>(projectLine);
		ASSERT(line);
		if (! line->FileExists())
			PostNoFileMessage(line);
		else
		if (line->NeedsToBeExecuted(kPostLinkStage, kCompile))
			fCompileList.AddItem(line);
	}

	fCompleted = 0;
	fToBeCompleted = fCompileList.CountItems();

	// Tell the compile generator to start the compile
	if (fCompileList.CountItems() > 0)
	{
		// Launch the compiles
		LockData();
		fCompileGenerator.StartCompile(kOneConcurrentCompile, &fCompileList, Window(), false, kPostLinkExecute);	// no header file caching
		UnlockData();

		fCompileGenerator.Run();

		ShowCompileStatus();
	}
	else
		CompileDone();
}

// ---------------------------------------------------------------------------
//		Launch
// ---------------------------------------------------------------------------
//	Tell the linker builder to launch the project.  Tell it whether it's
//	a launch with debugger or without debugger.

void
MBuildCommander::Launch()
{
	if (fLinker != nil)
		fLinker->Launch(fProjectView.BuilderProject(), fLaunchCommand == cmd_RunOpposite);

	fLaunchCommand = DONT_LAUNCH;
	CompileDone();
}

status_t
MBuildCommander::Launch(
	entry_ref&	inRef)
{
	status_t	err = B_NO_ERROR;

	switch (fLaunchCommand)
	{
		case cmd_Run:
		{
			err = MLauncher(inRef).Launch();
		}
			break;

		case cmd_RunOpposite:
		{
#ifdef __GNUC__
			// bdb takes an entry_ref to start up
			// it can be launched multiple times, so don't worry
			// about checking if it already is running
			BMessage msg(B_REFS_RECEIVED);
			msg.AddRef("refs", &inRef);
			err = be_roster->Launch(kDebuggerSigMimeType, &msg);			
#else
			if (inRef.device == -1)
			{
				err = kNoSymFile;
			}
			else
			{
				team_id			team = be_roster->TeamFor(kDebuggerSigMimeType);
				// Launch the debugger if it's not already running
				if (team == -1)
				{
					err = be_roster->Launch(&inRef);	// should work, fails if file type is bad
					if (err != B_OK)
					{
						// one final attempt
						BMessage		msg(B_REFS_RECEIVED);
		
						msg.AddRef("refs", &inRef);
						err = be_roster->Launch(kDebuggerSigMimeType, &msg);
					}
				}
				else
				{
					// Tell the debugger to open the xSYM file
					BMessage		msg(B_REFS_RECEIVED);
	
					msg.AddRef("refs", &inRef);
					BMessenger      debugger(nil, team);
					err = debugger.SendMessage(&msg);
				}
			}
#endif
		}
			break;
		
		default:
			break;
	}
	
	if (err != B_NO_ERROR)
		ReportLaunchError(err);

	return err;
}

// ---------------------------------------------------------------------------
//		ReportLaunchError
// ---------------------------------------------------------------------------
//	Report errors that occur during launch by showing an alert.

void
MBuildCommander::ReportLaunchError(
	status_t	inErr)
{
	String		text = "Error Launching Application:\n";

	switch (inErr)
	{
		case B_BAD_VALUE:
			text += "B_BAD_VALUE";
			break;
		case B_ERROR:
			text += "Executable can't be found.";
			break;
		case B_ALREADY_RUNNING:
			text += "This app is already running and can't be launched again.";
			break;
		case B_LAUNCH_FAILED:
			text += "B_LAUNCH_FAILED, generic launch failure.";
			break;
		case kNoSymFile:
			text += "There's no xSYM file.  Turn on Generate SYM file in the linker prefs panel.";
			break;
		case kNoDebugger:
			text += "Couldn't find the Metrowerks debugger.";
			break;
		case kPluginProblem:
			text += "Attempt to launch not in response to menu action.";
			break;
	}

	MAlert		alert(text);
	alert.Go();
}

// ---------------------------------------------------------------------------
//		SetExecutableRef
// ---------------------------------------------------------------------------

void
MBuildCommander::SetExecutableRef()
{
	LockData();

	// Get the value for the executable's entry_ref
	GetExecutableRef();

	UnlockData();
}

// ---------------------------------------------------------------------------
//		GetExecutableRef
// ---------------------------------------------------------------------------

void
MBuildCommander::GetExecutableRef()
{
	if (fLinker != nil)
		fLinker->GetExecutableRef(fProjectView.BuilderProject(), fExecutableRef);
}

// ---------------------------------------------------------------------------
//		GetExecutableRef
// ---------------------------------------------------------------------------

status_t
MBuildCommander::GetExecutableRef(
	entry_ref&	outRef)
{
	status_t		result = B_NO_ERROR;

	GetExecutableRef();

	BEntry			executable(&fExecutableRef);

	if (executable.Exists())
		outRef = fExecutableRef;
	else
		result = B_FILE_NOT_FOUND;

	return result;
}

// ---------------------------------------------------------------------------
//		AppendToList
// ---------------------------------------------------------------------------

void
MBuildCommander::AppendToList(
	BList&			inFromList,
	BList&			inToList)
{
	int32		count = inFromList.CountItems();

	for (int32 i = 0; i < count; i++)
	{
		void*	item = inFromList.ItemAt(i);
		inToList.AddItem(item);
	}
}

// ---------------------------------------------------------------------------
//		ArgvToList
// ---------------------------------------------------------------------------

void
MBuildCommander::ArgvToList(
	char**&			inArgv,
	MList<char*>&	inList)
{
	char**		vp = inArgv;

	while (*vp != nil)
		inList.AddItem(*vp++);
	
	delete [] inArgv;
}

// ---------------------------------------------------------------------------
//		ListToArgv
// ---------------------------------------------------------------------------

void
MBuildCommander::ListToArgv(
	char**&			inArgv,
	MList<char*>&	inList)
{
	inArgv = new char*[inList.CountItems() + 1];
	char**		vp = inArgv;
	int32		i = 0;

	while (inList.GetNthItem(*vp, i++))
	{
		vp++;
	}

	*vp = nil;
}

// ---------------------------------------------------------------------------
//		BuildPrecompileArgv
// ---------------------------------------------------------------------------

status_t
MBuildCommander::BuildPrecompileArgv(
	BuilderRec*		inBuilderRec,
	StringList&		inArgv,
	MFileRec&		inFileRec)
{
	BMessage		msg;
	uint32			type = inBuilderRec->builder->MessageDataType();
	status_t		err = B_NO_ERROR;
	StringList		argvList;

	FillMessage(msg, type);		// could cache this message based on the type

	while (inBuilderRec)
	{
		argvList.MakeEmpty();
		
		err = inBuilderRec->builder->BuildPrecompileArgv(fProjectView.BuilderProject(), argvList, inFileRec);

		if (err != B_NO_ERROR)
			break;

		AppendToList(argvList, inArgv);

		inBuilderRec = inBuilderRec->next;
	}
	
	return err;
}

// ---------------------------------------------------------------------------
//		BuildCompileArgv
// ---------------------------------------------------------------------------

status_t
MBuildCommander::BuildCompileArgv(
	MakeActionT 	inAction,
	BuilderRec*		inBuilderRec,
	StringList&		inArgv,
	MFileRec&		inFileRec)
{
	BMessage		msg;
	uint32			type = inBuilderRec->builder->MessageDataType();
	status_t		err = B_NO_ERROR;
	StringList		argvList;

	FillMessage(msg, type);		// could cache this message based on the type
	if (type == kMWLDType)		// Special case for the linker when disassembling
		FillMessage(msg, kMWCCType);

	while (inBuilderRec)
	{
		argvList.MakeEmpty();
		
		err = inBuilderRec->builder->BuildCompileArgv(fProjectView.BuilderProject(), argvList, inAction, inFileRec);

		if (err != B_NO_ERROR)
			break;
		
		AppendToList(argvList, inArgv);
		inBuilderRec = inBuilderRec->next;
	}

	return err;
}

// ---------------------------------------------------------------------------
//		BuildPostLinkArgv
// ---------------------------------------------------------------------------

status_t
MBuildCommander::BuildPostLinkArgv(
	BuilderRec*		inBuilderRec,
	StringList&		inArgv,
	MFileRec&		inFileRec)
{
	status_t		err = B_NO_ERROR;
	StringList		argvList;

	while (inBuilderRec)
	{
		argvList.MakeEmpty();
		
		err = inBuilderRec->builder->BuildPostLinkArgv(fProjectView.BuilderProject(), argvList, inFileRec);

		if (err != B_NO_ERROR)
			break;
		
		AppendToList(argvList, inArgv);
		inBuilderRec = inBuilderRec->next;
	}

	return err;
}

// ---------------------------------------------------------------------------
//		CompileSelection
// ---------------------------------------------------------------------------
//	Compile the selected lines.

void
MBuildCommander::CompileSelection()
{
	if (fCompilingState == sNotCompiling)
	{
		fCompilingState = sCompilingSelection;
		ClearMessages();
		MDynamicMenuHandler::SaveAllTextWindows();
		
		bool			usesFileCache;
		MFileGetter*	fileGetter = nil;
		fProjectView.FillCompileList(fCompileList, usesFileCache);
		if (usesFileCache)
			fileGetter = &fProjectView;
		fCompleted = 0;
		fToBeCompleted = fCompileList.CountItems();

		LockData();
		fCompileGenerator.StartCompile(fConcurrentCompiles, &fCompileList, Window(), fStopOnErrors, 
				kCompile, fileGetter);
		UnlockData();
		fCompileGenerator.Run();

		ShowCompileStatus();
	}
}

// ---------------------------------------------------------------------------
//		PreCompileSelection
// ---------------------------------------------------------------------------
//	PreCompile the selected lines.

void
MBuildCommander::PrecompileSelection()
{
	if (fCompilingState == sNotCompiling)
	{
		fCompilingState = sPreCompilingSelection;
		ClearMessages();
		MDynamicMenuHandler::SaveAllTextWindows();

		bool			usesFileCache;
		MFileGetter*	fileGetter = nil;
		fProjectView.FillCompileList(fCompileList, usesFileCache);
		if (usesFileCache)
			fileGetter = &fProjectView;
		fCompleted = 0;
		fToBeCompleted = 1;

		LockData();
		fCompileGenerator.StartCompile(kOneConcurrentCompile, 
						&fCompileList, Window(), fStopOnErrors, kPrecompile, fileGetter);
		UnlockData();
	
		fCompileGenerator.Run();

		ShowCompileStatus();
	}
}

// ---------------------------------------------------------------------------
//		PreprocessSelection
// ---------------------------------------------------------------------------

void
MBuildCommander::PreprocessSelection()
{
	if (fCompilingState == sNotCompiling)
	{
		fCompilingState = sPreProcessingSelection;
		ClearMessages();
		MDynamicMenuHandler::SaveAllTextWindows();
		bool			usesFileCache;
		MFileGetter*	fileGetter = nil;
		fProjectView.FillCompileList(fCompileList, usesFileCache);
		if (usesFileCache)
			fileGetter = &fProjectView;
		fCompleted = 0;
		fToBeCompleted = fCompileList.CountItems();

		LockData();
		fCompileGenerator.StartCompile(fConcurrentCompiles, 
						&fCompileList, Window(), false, kPreprocess, fileGetter);
		UnlockData();
		fCompileGenerator.Run();

		ShowCompileStatus();
	}
}

// ---------------------------------------------------------------------------
//		DoPreprocessOne
// ---------------------------------------------------------------------------

void
MBuildCommander::DoPreprocessOne(
	BMessage& 	inMessage)
{
	if (fCompilingState == sNotCompiling)
	{
		entry_ref			ref;

		if (B_OK == inMessage.FindRef(kTextFileRef, &ref))
		{
			fCompilingState = sPreProcessingOne;
			ClearMessages();
			fCompleted = 0;
			fToBeCompleted = 1;
			MDynamicMenuHandler::SaveAllTextWindows();

			MSourceFileLine*	line = nil;
			
			// Is the file in this project?
			if (fProjectView.GetSourceFileLineByRef(ref, line))
			{
				MFileGetter*	fileGetter = nil;
				if (line->UsesFileCache())
					fileGetter = &fProjectView;

				fCompileList.MakeEmpty();
				fCompileList.AddItem(line);

				LockData();
				fCompileGenerator.StartCompile(fConcurrentCompiles, 
								&fCompileList, Window(), false, kPreprocess, fileGetter);
				UnlockData();

				fCompileGenerator.Run();
				ShowCompileStatus();
			}
			else
			// Not in this project
			if (B_NO_ERROR == ExecuteOneFile(inMessage, kPreprocess))
			{
				ShowCompileStatus();
			}		
		}
	}
}

// ---------------------------------------------------------------------------
//		ExecuteOneFile
// ---------------------------------------------------------------------------
//	Execute a file that isn't in the project.  This is only for compile stage.

status_t
MBuildCommander::ExecuteOneFile(
	BMessage& 		inMessage,
	MakeActionT		inAction)
{
	status_t			err = B_ERROR;
	entry_ref			ref;

	if (B_NO_ERROR == inMessage.FindRef(kTextFileRef, &ref))
	{
		BEntry				file(&ref);
		ProjectTargetRec*	rec = GetTargetRec(file);
		
		if (rec != nil && rec->Builder != nil && rec->Builder->builder != nil)
		{
			StringList		argvList;
			char			filePath[kPathSize];

			MFileRec		filerec;
			FillFileRec(file, filerec);
			filerec.makeStage = kCompile;
			err = B_NO_ERROR;

			if (B_NO_ERROR == err)
			{	
				BuildCompileArgv(inAction, rec->Builder, argvList, filerec);
				bool		ideAware = ((rec->Builder->builder->Flags() & kIDEAware) != 0);
				PathNameT	compilerPath;
				
				GetToolPath(rec, compilerPath, kCompile, inAction);

				// Build the compile object
				fCompileObj = new MCompilerObj(nil, compilerPath, filePath, argvList, ideAware, fProjectView, inAction);
				
				err = fCompileObj->Run();
				if (B_NO_ERROR != err)
				{
					//	Error!
					ASSERT(!"Error starting compiler");
					delete fCompileObj;
					fCompileObj = nil;
					// may need to call compiledone here
				}
			}
		}
	}
	
	return err;
}

// ---------------------------------------------------------------------------
//		DoPreCompileOne
// ---------------------------------------------------------------------------

void
MBuildCommander::DoPreCompileOne(
	BMessage& inMessage)
{
	if (fCompilingState == sNotCompiling)
	{
		const char*			name;
		entry_ref			ref;

		if (B_OK == inMessage.FindString(kFileName, &name) &&
			B_OK == inMessage.FindRef(kTextFileRef, &ref))
		{
			fCompilingState = sPreCompilingOne;
		
			ClearMessages();
			fCompleted = 0;
			fToBeCompleted = 1;

			ASSERT(name);
			MSourceFileLine*	line = nil;
			(void) fProjectView.GetSourceFileLineByRef(ref, line);

			// Reset the sourcefile object for the 
			// precompiled header that will be produced
			fProjectView.ResetPrecompiledHeaderFile(name);

			if (line)
			{
				MFileGetter*	fileGetter = nil;
				if (line->UsesFileCache())
					fileGetter = &fProjectView;
				fCompileList.MakeEmpty();
				fCompileList.AddItem(line);

				LockData();
				fCompileGenerator.StartCompile(kOneConcurrentCompile, 
								&fCompileList, Window(), fStopOnErrors, kPrecompile, fileGetter);
				UnlockData();
			
				fCompileGenerator.Run();

				ShowCompileStatus();
			}
			else
			if (B_NO_ERROR == DoPrecompile(inMessage))
			{
				ShowCompileStatus();
			}
			else
			CompileDone();		// an error occurred
		}
	}
}

// ---------------------------------------------------------------------------
//		DoPrecompile
// ---------------------------------------------------------------------------

status_t
MBuildCommander::DoPrecompile(
	BMessage&	inMessage)
{
	status_t		err = B_ERROR;
	entry_ref		ref;

	if (B_NO_ERROR == inMessage.FindRef(kTextFileRef, &ref))
	{
		BEntry				file(&ref);
		ProjectTargetRec*	rec = GetTargetRec(file);
		
		if (rec != nil)
		{
			StringList		argvList;
			char			filePath[kPathSize];
			MFileRec		filerec;

			FillFileRec(file, filerec);
			BuildPrecompileArgv(rec->Builder, argvList, filerec);
			bool		ideAware = ((rec->Builder->builder->Flags() & kIDEAware) != 0);
			PathNameT	compilerPath;
			GetToolPath(rec, compilerPath, kPrecompileStage, kPrecompile);

			// Build the precompile object
			fCompileObj = new MPreCompileObj(compilerPath, filePath, argvList, ideAware, fProjectView, nil);
			
			err = fCompileObj->Run();
			if (B_NO_ERROR != err)
			{
				//	Error!
				ASSERT(!"Error starting compiler");
				delete fCompileObj;
				fCompileObj = nil;
				// may need to call compiledone here
			}
		}
	}
	
	return err;
}

// ---------------------------------------------------------------------------
//		FillFileRec
// ---------------------------------------------------------------------------

void
MBuildCommander::FillFileRec(
	BEntry&		inFile,
	MFileRec&	outRec) const
{
	outRec.fileType = 'TEXT';	// for now ????
	outRec.fileID = 0;
	outRec.hasResources = false;

	status_t	err = inFile.GetName(outRec.path);

	if (B_NO_ERROR == err)
	{
		int32	namelen = strlen(outRec.path);
		BPath	path;

		inFile.GetPath(&path);
		strncpy(outRec.path, path.Path(), sizeof(outRec.path));
		outRec.path[sizeof(outRec.path) - 1] = '\0';
		outRec.name = &outRec.path[namelen - strlen(outRec.path)];
	}
	
	if (B_NO_ERROR != err)
	{
		// punt
		outRec.path[0] = '\0';
		outRec.name = &outRec.path[0];
	}
}

// ---------------------------------------------------------------------------
//		DoCheckSyntaxSelection
// ---------------------------------------------------------------------------

void
MBuildCommander::CheckSyntaxSelection()
{
	if (fCompilingState == sNotCompiling)
	{
		fCompilingState = sCheckingSyntaxSelection;
		ClearMessages();
		MDynamicMenuHandler::SaveAllTextWindows();
		bool			usesFileCache;
		MFileGetter*	fileGetter = nil;
		fProjectView.FillCompileList(fCompileList, usesFileCache);
		if (usesFileCache)
			fileGetter = &fProjectView;
		fCompleted = 0;
		fToBeCompleted = fCompileList.CountItems();

		LockData();
		fCompileGenerator.StartCompile(fConcurrentCompiles, 
						&fCompileList, Window(), false, kCheckSyntax, fileGetter);
		UnlockData();
		fCompileGenerator.Run();

		ShowCompileStatus();
	}
}

// ---------------------------------------------------------------------------
//		DoCheckSyntaxOne
// ---------------------------------------------------------------------------

void
MBuildCommander::DoCheckSyntaxOne(
	BMessage& inMessage)
{
	if (fCompilingState == sNotCompiling)
	{
		entry_ref			ref;

		if (B_OK == inMessage.FindRef(kTextFileRef, &ref))
		{
			fCompilingState = sCheckingSyntaxOne;
		
			ClearMessages();
			fCompleted = 0;
			fToBeCompleted = 1;

			MSourceFileLine*	line = nil;
			
			// Is the file in this project?
			if (fProjectView.GetSourceFileLineByRef(ref, line))
			{
				MFileGetter*	fileGetter = nil;
				if (line->UsesFileCache())
					fileGetter = &fProjectView;
				fCompileList.MakeEmpty();
				fCompileList.AddItem(line);

				LockData();
				fCompileGenerator.StartCompile(kOneConcurrentCompile, 
								&fCompileList, Window(), false, kCheckSyntax, fileGetter);
				UnlockData();
				fCompileGenerator.Run();

				ShowCompileStatus();
			}
			else	// The file's not in this project
			if (B_NO_ERROR == ExecuteOneFile(inMessage, kCheckSyntax))
			{
				ShowCompileStatus();
			}
			else
				CompileDone();
		}
	}
}

// ---------------------------------------------------------------------------
//		DisassembleSelection
// ---------------------------------------------------------------------------

void
MBuildCommander::DisassembleSelection()
{
	if (fCompilingState == sNotCompiling)
	{
		fCompilingState = sDisassemblingSelection;
		ClearMessages();
		MDynamicMenuHandler::SaveAllTextWindows();
		fCompileList.MakeEmpty();

		bool		usesFileCache = false;
		fProjectView.FillCompileList(fDisassembleList, usesFileCache);
		usesFileCache = false;

		int32		count = fDisassembleList.CountItems();
		
		for (int32 i = 0; i < count; i++)
		{
			MSourceFileLine*	line = (MSourceFileLine*) fDisassembleList.ItemAt(i);
			
			if (line->NeedsToBeCompiled())
			{
				fCompileList.AddItem(line);
				if (!usesFileCache)
					usesFileCache = line->UsesFileCache();
			}
		}

		fCompleted = 0;
		fToBeCompleted = fCompileList.CountItems();

		if (fToBeCompleted > 0)
		{
			MFileGetter*	fileGetter = nil;
			if (usesFileCache)
				fileGetter = &fProjectView;

			LockData();
			fCompileGenerator.StartCompile(fConcurrentCompiles, 
							&fCompileList, Window(), fStopOnErrors, kCompile, fileGetter);
			UnlockData();
			fCompileGenerator.Run();

			ShowCompileStatus();
		}
		else
			CompileDone();
	}
}

// ---------------------------------------------------------------------------
//		DoDisassembleOnePartOne
// ---------------------------------------------------------------------------
//	Disassembling is fundamentally different than other actions in that 
//	it consists of two parts.  Our disassembler works by disassembling
//	a .o file so a disassemble must first compile a file if it's dirty 
//	and then pass the .o file to the disassemble tool.

void
MBuildCommander::DoDisassembleOnePartOne(
	BMessage& inMessage)
{
	if (fCompilingState == sNotCompiling)
	{
		entry_ref			ref;

		if (B_OK == inMessage.FindRef(kTextFileRef, &ref))
		{
			MSourceFileLine*	line = nil;
			
			// Is the file in this project?
			if (fProjectView.GetSourceFileLineByRef(ref, line))
			{
				fCompilingState = sDisassemblingOnePartOne;
			
				ClearMessages();
		
				// Save all open text files before starting a compile
				// It's possible that a header file that's included
				// by this source file is open and dirty
				MDynamicMenuHandler::SaveAllTextWindows();

				fDisassembleList.MakeEmpty();
				fDisassembleList.AddItem(line);
		
				if (line->NeedsToBeCompiled())
				{
					MFileGetter*	fileGetter = nil;
					if (line->UsesFileCache())
						fileGetter = &fProjectView;

					fCompileList.MakeEmpty();
					fCompileList.AddItem(line);
					fCompleted = 0;
					fToBeCompleted = 1;
	
					LockData();
					fCompileGenerator.StartCompile(kOneConcurrentCompile, 
							&fCompileList, Window(), fStopOnErrors, kCompile, fileGetter);
					UnlockData();
					fCompileGenerator.Run();
			
					ShowCompileStatus();
				}
				else 
					CompileDone();
			}
		}
	}
}

// ---------------------------------------------------------------------------
//		DisassemblePartTwo
// ---------------------------------------------------------------------------
//	The disassemble step of the two part disassemble procedure.

void
MBuildCommander::DisassemblePartTwo()
{
	if (fCompilingState == sDisassemblingOnePartOne || 
		fCompilingState == sDisassemblingSelection)
	{
		fCompilingState = sDisassemblingOnePartTwo;
	
		fCompleted = 0;
		fToBeCompleted = fDisassembleList.CountItems();

		LockData();
		fCompileGenerator.StartCompile(fConcurrentCompiles, 
						&fDisassembleList, Window(), false, kDisassemble);	// no header file caching
		UnlockData();
		fCompileGenerator.Run();

		ShowCompileStatus();
	}
}

// ---------------------------------------------------------------------------
//		DoCompileOne
// ---------------------------------------------------------------------------
//	Compile a file in response to a cmd-K in a text window.

void
MBuildCommander::DoCompileOne(
	BMessage& inMessage)
{
	if (fCompilingState == sNotCompiling)
	{
		entry_ref			ref;

		if (B_OK == inMessage.FindRef(kTextFileRef, &ref))
		{
			MSourceFileLine*	line = nil;
			
			// Is the file in this project?
			if (fProjectView.GetSourceFileLineByRef(ref, line))
			{
				fCompilingState = sCompilingOne;
			
				ClearMessages();
		
				// Save all open text files before starting a compile
				// It's possible that a header file that's included
				// by this source file is open and dirty
				MDynamicMenuHandler::SaveAllTextWindows();
				fCompileList.MakeEmpty();
				fCompileList.AddItem(line);
		
				fCompleted = 0;
				fToBeCompleted = 1;

				MFileGetter*	fileGetter = nil;
				if (line->UsesFileCache())
					fileGetter = &fProjectView;

				LockData();
				fCompileGenerator.StartCompile(kOneConcurrentCompile, 
							&fCompileList, Window(), fStopOnErrors, kCompile, fileGetter);
				UnlockData();
				fCompileGenerator.Run();
		
				ShowCompileStatus();
			}
		}
	}
}

// ---------------------------------------------------------------------------
//		GetBuiltAppPath
// ---------------------------------------------------------------------------
//	Place the application in the same folder as the project and give it
//	the name 'Application'.

void
MBuildCommander::GetBuiltAppPath(
	char* 	outPath, 
	int32	inBufferLength)
{
	if (fLinker != nil)
		fLinker->GetBuiltAppPath(fProjectView.BuilderProject(), outPath, inBufferLength);
}

// ---------------------------------------------------------------------------
//		LinkerName
// ---------------------------------------------------------------------------
//	return the name of the linker for the MProject object.

const char *
MBuildCommander::LinkerName()
{
	if (fLinker != nil)
		return fLinker->LinkerName();
	else
		return B_EMPTY_STRING;
}

// ---------------------------------------------------------------------------

void
MBuildCommander::SetConcurrentCompiles(uint32 inHowMany)
{
	// 0 is a special number - use the number of CPUs
	fConcurrentCompiles = inHowMany;
	if (fConcurrentCompiles == 0) {
		system_info sysInfo;
		sysInfo.cpu_count = 1;
		get_system_info(&sysInfo);
		fConcurrentCompiles = sysInfo.cpu_count;
	}
}

// ---------------------------------------------------------------------------

void
MBuildCommander::SetStopOnErrors(bool stopOnErrors)
{
	fStopOnErrors = stopOnErrors;
}

// ---------------------------------------------------------------------------

void
MBuildCommander::SetAutoOpenErrorWindow(bool openErrorWindow)
{
	fAutoOpenErrorWindow = openErrorWindow;
}


// ---------------------------------------------------------------------------

void
MBuildCommander::SetBuildPriority(int32 priority)
{
	fBuildThreadPriority = priority;
}

// ---------------------------------------------------------------------------
//		IsRunnable
// ---------------------------------------------------------------------------
//	Can the file generated by this project be run?  Called to enable/disable
//	the run/debug menu items.  Shared libs can't be run, for example.

bool
MBuildCommander::IsRunnable()
{
	if (fLinker != nil)
		return fLinker->IsLaunchable(fProjectView.BuilderProject());
	else
		return false;
}

// ---------------------------------------------------------------------------
//		CopyAllResources
// ---------------------------------------------------------------------------
//	Copy resources into the executable.  inReplace specifies
//	if duplicates should be replaced or ignored.
//	Ignoring generates a warning.  Replacing is silent.

void
MBuildCommander::CopyAllResources(
	BResources&			inFrom,
	BResources&			inTo,
	entry_ref&			inFromRef,
	bool				inReplace)
{
	type_code	type;
	int32		id;
	const char* name;
	size_t		size;
	status_t	err = B_NO_ERROR;
	int			result;

	for (int32 i = 0; err == B_NO_ERROR && ((result = inFrom.GetResourceInfo(i, &type, &id, &name, &size))!= false); i++)
	{
		void *		data = inFrom.FindResource(type, id, &size);
		bool		hasResource = inTo.HasResource(type, id);

		if (hasResource && inReplace)
		{
			err = inTo.RemoveResource(type, id);
			hasResource = false;
		}

		if (hasResource)
		{
			// Ignore a duplicate resource, 
			// Post a warning to the message window
			ErrorNotificationMessageShort 	message;
			FileNameT	fileName = { 0 };
			BEntry		fromEntry(&inFromRef);
			fromEntry.GetName(fileName);

			String		text = "Ignoring duplicate resource in "B_UTF8_OPEN_QUOTE;
			text += fileName;
			text += B_UTF8_CLOSE_QUOTE".\n";
			text += String((const char*) &type, 4);
			text += "/";
			text += id;
			text += "/"B_UTF8_OPEN_QUOTE;
			text += name;
			text += B_UTF8_CLOSE_QUOTE;

			strncpy(message.errorMessage, text, 256);
			message.hasErrorRef = false;
			message.isWarning = true;

			fProjectView.GetErrorMessageWindow()->PostErrorMessage(message);
		}
		else
		{
			err = inTo.AddResource(type, id, data, size, name);
		}

		free(data);		// Free the resource here
	}
}

// ---------------------------------------------------------------------------
//		PostNoFileMessage
// ---------------------------------------------------------------------------
//	Post an error message to the message window.  This won't interrupt a
//	compile but it will prevent the link stage of a Make.

void
MBuildCommander::PostNoFileMessage(
	MSourceFileLine*		line)
{
	ErrorNotificationMessageShort 	message;
	String		text = "File not found: "B_UTF8_OPEN_QUOTE;
	
	text += line->GetFileName();
	text += B_UTF8_CLOSE_QUOTE".";
	
	strcpy(message.errorMessage, text);
	message.hasErrorRef = false;
	message.isWarning = false;

	fProjectView.GetErrorMessageWindow()->PostErrorMessage(message);
}

// ---------------------------------------------------------------------------
//		InitializeData
// ---------------------------------------------------------------------------
//	Get the preferences from the message.

void
MBuildCommander::InitializeData(
	BMessage&	inMessage)
{
	LockData();

	SetData(inMessage);
	fPrefsContainer.ValidateGenericData();

	UnlockData();
}

// ---------------------------------------------------------------------------
//		ValidateGenericData
// ---------------------------------------------------------------------------

void
MBuildCommander::ValidateGenericData()
{
	fPrefsContainer.ValidateGenericData();
}

// ---------------------------------------------------------------------------
//		WriteToFile
// ---------------------------------------------------------------------------

void
MBuildCommander::WriteToFile(
	MBlockFile& inFile)
{
	fPrefsContainer.WriteToFile(inFile);
}

// ---------------------------------------------------------------------------
//		ReadFromFile
// ---------------------------------------------------------------------------

void
MBuildCommander::ReadFromFile(
	MBlockFile& inFile)
{
	fPrefsContainer.ReadFromFile(inFile);
	fPrefsContainer.RemoveData(kEditorPrefs);	
	fPrefsContainer.RemoveData(kTargetPrefs);	
	fPrefsContainer.RemoveData(kFontPrefs);	
	fPrefsContainer.RemoveData(kSyntaxStylePrefs);	
	fPrefsContainer.RemoveData(kProjectPrefs);	

	fWindow = fProjectView.Window();	// Remember the window
}

// ---------------------------------------------------------------------------
//		SetData
// ---------------------------------------------------------------------------
//	Get the preferences from the message.

void
MBuildCommander::SetData(
	BMessage&	inMessage)
{
	LockData();

	fPrefsContainer.SetData(inMessage);

	fWindow = fProjectView.Window();	// Remember the window

	UnlockData();
}

// ---------------------------------------------------------------------------
//		GetData
// ---------------------------------------------------------------------------
//	Fill the BMessage with preferences data of the kind specified in the
//	BMessage.

void
MBuildCommander::GetData(
	BMessage&	inMessage)
{
	LockData();
	fPrefsContainer.GetData(inMessage);
	UnlockData();
}

// ---------------------------------------------------------------------------
//		FillMessage
// ---------------------------------------------------------------------------

void
MBuildCommander::FillMessage(
	BMessage&		inMessage,
	uint32			inType)
{
	LockData();
	fPrefsContainer.FillMessage(inMessage, inType);
	UnlockData();
}

