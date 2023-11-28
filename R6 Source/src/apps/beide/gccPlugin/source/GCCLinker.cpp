// ---------------------------------------------------------------------------
/*
	GCCLinker.cpp
	
	Copyright (c) 1998 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			22 September 1998

*/
// ---------------------------------------------------------------------------

#include "GCCLinker.h"
#include "PlugIn.h"
#include "ErrorParser.h"
#include "CodeGenerationOptionsView.h"
#include "CommandLineTextView.h"
#include "PlugInUtil.h"
#include "MProject.h"
#include "PlugInPreferences.h"
#include "ErrorMessage.h"

#include <String.h>
#include <List.h>
#include <Directory.h>
#include <Path.h>
#include <File.h>
#include <AppFileInfo.h>
#include <Alert.h>
#include <Debug.h>

// ---------------------------------------------------------------------------
// constant strings
// ---------------------------------------------------------------------------

const char* kLinkerEscapeFlag = "-Xlinker";
const char* kInternalNameFlagPrefix = "-soname=";
const char* k_APP_Name = "_APP_";
const char* kSharedLibraryFlag = "-nostart";
const char* kNoStandardLibFlag = "-nostdlib";

const char* kArchiveFlags = "-cru";

const char kPathSeparator = '/';

// ---------------------------------------------------------------------------
// ProjectLinkerSettings Member functions
// ---------------------------------------------------------------------------

ProjectLinkerSettings::ProjectLinkerSettings()
{
	// See comments in ProjectSettings::ProjectSettings (GCCBuilder.cpp)
	// for why default setting is needed here.
	
	fLinkerSettings.SetDefaults();
	fCommandLineText.fVersion = CommandLineText::kCurrentVersion;
	strcpy(fCommandLineText.fText, kAdditionalLinkerOptionsDefaults);
	SetProjectPrefsx86Defaults(fProjectOptions);
}

ProjectLinkerSettings::~ProjectLinkerSettings()
{
}

// ---------------------------------------------------------------------------
// GCCLinker Member functions
// ---------------------------------------------------------------------------

GCCLinker::GCCLinker()
{
}

// ---------------------------------------------------------------------------

GCCLinker::~GCCLinker()
{
}

// ---------------------------------------------------------------------------

const char*
GCCLinker::TargetName()
{
	return kTargetName;
}

// ---------------------------------------------------------------------------

const char*
LeafNameOf(const char* applicationName)
{
	// The user can specify a partial or full path name in the
	// application name text box
	// For the -soname, we just want the leaf name of that string

	const char* leafName = strrchr(applicationName, kPathSeparator);
	if (leafName) {
		// bump leafName past slash
		leafName += 1;
	}
	else {
		leafName = applicationName;
	}

	return leafName;
}

// ---------------------------------------------------------------------------

void
GCCLinker::BuildStandardArgv(MProject* inProject, BList& inArgv)
{
	// We need to deal with two tools here, gcc_link and ar
	// For Applications, SharedLibraries, Drivers then do normal options
	// For StaticLibraries - use ar
	// If we are building an archive, then we don't do anything with
	// any linker option except the name

	ProjectLinkerSettings* cache = fSettingsMap.GetSettings(*inProject);

	// Get the application/library no matter what
	BString appName;
	this->MakeTargetApplicationName(cache->fProjectOptions, appName);

	// If we are building an archive, then we don't do anything with any
	// linker options except the output name
	if (cache->fProjectOptions.pProjectKind == LibraryType) {
		// now add in the archive flags that we need
		inArgv.AddItem(strdup(kArchiveFlags));
	}
	else {

		// both the compiler and linker need to specify the following:
		// -g if debugging is on...
		// -p if profiling is on...
		// get the flags from the code generation options -- but only the ones we need
		// and set the flags if true
		if (cache->fCodeGenerationSettings.GetOption(kGenerateDebugSymbols) == true) {
			cache->fCodeGenerationSettings.AddOption(kGenerateDebugSymbols, inArgv);
		}
	
		if (cache->fCodeGenerationSettings.GetOption(kGenerateProfileCode) == true) {
			cache->fCodeGenerationSettings.AddOption(kGenerateProfileCode, inArgv);
		}
		
		// Set up the linker options
		cache->fLinkerSettings.AddAllOptions(inArgv);
	
		// Now set up the options based on what type of project we are building
	
		// Shared libraries get the following flags
		// -nostart -Xlinker -soname=<libName>
		// Applications get the following flags
		// -Xlinker -soname=_APP_
		// Kernel drivers get the following falgs
		// -nostdlib
	
		// if we are dealing with a shared library, add the shared library options
		if (cache->fProjectOptions.pProjectKind == SharedLibType) {
			inArgv.AddItem(strdup(kSharedLibraryFlag));
		}
		// if we are dealing with a kernel driver, we don't link with the standard libaries
		else if (cache->fProjectOptions.pProjectKind == DriverType) {
			inArgv.AddItem(strdup(kNoStandardLibFlag));
		}
			
		// Add the -soname option (for setting internal name) in both
		// application and shared library cases...
		// build up the full -soname=<name>
		if (cache->fProjectOptions.pProjectKind == SharedLibType ||
				cache->fProjectOptions.pProjectKind == AppType) {
			const char* internalName = NULL;
			if (cache->fProjectOptions.pProjectKind == SharedLibType) {
				internalName = LeafNameOf(appName.String());
			}
			else {
				internalName = k_APP_Name;
			}
			char* internalNameOption = new char[strlen(kInternalNameFlagPrefix) + 
												strlen(internalName) + 1];
			strcpy(internalNameOption, kInternalNameFlagPrefix);
			strcat(internalNameOption, internalName);
			inArgv.AddItem(strdup(kLinkerEscapeFlag));
			inArgv.AddItem(strdup(internalNameOption));
			delete [] internalNameOption;
		}
	
		// prepare for name of application/library
		inArgv.AddItem(strdup("-o"));
		// WARNING... the name of application/library must come next...
	}

	// the name of the application/library itself
	inArgv.AddItem(strdup(appName.String()));

}

// ---------------------------------------------------------------------------

status_t
GCCLinker::BuildLinkArgv(MProject& inProject, BList& inArgv)
{
#ifdef _GCCDEBUG_
	fprintf(stderr, "GCCLinker::BuildLinkArgv\n");
#endif

	ProjectLinkerSettings* cache = fSettingsMap.GetSettings(inProject);

	// do a little archive housekeeping if we are dealing with a
	// static library
	if (cache->fProjectOptions.pProjectKind == LibraryType) {
		// this is the best we have for a "starting to link" trigger
		// delete the current archive file before we go to build a new one
		BString appName;
		this->MakeTargetApplicationName(cache->fProjectOptions, appName);
		this->CleanArchive(inProject, appName);
	}

	// build the arguments specified in the settings panels
	this->BuildStandardArgv(&inProject, inArgv);

	// override other settins with the supplementation command line settings
	cache->fCommandLineText.FillArgvList(inArgv);

	// now add all the object files
	MFileRec fileRecord;
	BList targetObjectFiles;
	long fileCount = 0;

	// iterate through the project and get all the object files into a list
	while (inProject.GetNthFile(fileRecord, targetObjectFiles, fileCount)) {
		// (each GetNthFile is adding files to the list)
		fileCount++;
	}
	
	// now iterate the list and put each object file on our argv list
	
	int32 numTargets = targetObjectFiles.CountItems();
	for (int32 i = 0; i < numTargets; i++) {
		char* objectFilePath = (char*) targetObjectFiles.ItemAtFast(i);
		// inArgv adopts the objectFilePath
		inArgv.AddItem(objectFilePath);
	}
	targetObjectFiles.MakeEmpty();

	return B_OK;
}
// ---------------------------------------------------------------------------

void
GCCLinker::MakeTargetApplicationName(ProjectPrefsx86& options, BString& appName)
{
	if (options.pAppName[0]) {
		appName = options.pAppName;
	}
	else {
		appName = "Application";
	}
}

// ---------------------------------------------------------------------------

status_t
GCCLinker::GetExecutableRef(MProject& inProject, entry_ref& outExecutableRef)
{
	// rather misnamed since the ref can be to an archive
	// really should be called GetTargetFileRef

	ProjectLinkerSettings* cache = fSettingsMap.GetSettings(inProject);

#ifdef _GCCDEBUG_
	fprintf(stderr, "GCCLinker::GetExecutableRef\n");
#endif

	BDirectory projectDirectory;
	PlugInUtil::GetProjectDirectory(&inProject, projectDirectory);
	
	// using the project's directory, find the executable
	BEntry targetApplication;
	status_t result = projectDirectory.FindEntry(cache->fProjectOptions.pAppName, &targetApplication);
	if (result == B_OK) {
		result = targetApplication.GetRef(&outExecutableRef);
	}
	
	return result;
}

// ---------------------------------------------------------------------------

status_t
GCCLinker::GetBuiltAppPath(MProject& inProject, char * outPath, int32 inBufferLength)
{
#ifdef _GCCDEBUG_
	fprintf(stderr, "GCCLinker::GetBuiltAppPath\n");
#endif

	ProjectLinkerSettings* cache = fSettingsMap.GetSettings(inProject);

	if (cache->fProjectOptions.pAppName[0]) {
		strncpy(outPath, cache->fProjectOptions.pAppName, inBufferLength);
	}
	else {
		strncpy(outPath, "Application", inBufferLength);
	}
	return B_OK;
}

// ---------------------------------------------------------------------------

bool
GCCLinker::IsLaunchable(MProject& inProject)
{
	ProjectLinkerSettings* cache = fSettingsMap.GetSettings(inProject);
	return cache->fProjectOptions.pProjectKind == AppType;
}

// ---------------------------------------------------------------------------

status_t
GCCLinker::Launch(MProject& inProject, bool /* inRunWithDebugger */)
{
	entry_ref appRef;
	status_t result = this->GetExecutableRef(inProject, appRef);
	if (result == B_OK) {
		result = inProject.Launch(appRef);
	}
	
	return result;
}

// ---------------------------------------------------------------------------

status_t
GCCLinker::GetToolName(MProject* inProject, char* outName, int32 inBufferLength,
					   MakeStageT inStage, MakeActionT inAction)
{	
	// Normally, our linker tool is kGCCLinkerName, however, if we
	// are building a static library - and we are actually doing the link
	// phase, then we use kGCCArchiveBuilderName

	ProjectLinkerSettings* cache = nil;
	if (inProject) {
		cache = fSettingsMap.GetSettings(*inProject);
	}
	
	if (cache && cache->fProjectOptions.pProjectKind == LibraryType && 
			inStage == kLinkStage && inAction == kLink) {
		strncpy(outName, kGCCArchiveBuilderName, inBufferLength);
	}
	else {
		strncpy(outName, kGCCLinkerName, inBufferLength);
	}

#ifdef _GCCDEBUG_
	fprintf(stderr, "GCCLinker::GetToolName: returning %s\n", outName);
#endif

	return B_OK;
}

// ---------------------------------------------------------------------------

const char*
GCCLinker::LinkerName()
{
	return kGCCLinkerName;
}

// ---------------------------------------------------------------------------

MakeStageT
GCCLinker::MakeStages()
{
	return kLinkStage;
}

// ---------------------------------------------------------------------------

MakeActionT
GCCLinker::Actions()
{
	return kLink;
}

// ---------------------------------------------------------------------------

PlugInFlagsT
GCCLinker::Flags()
{
	return kNotIDEAware;
}

// ---------------------------------------------------------------------------

ulong
GCCLinker::MessageDataType()
{
	return kLinkerMessageType;
}

// ---------------------------------------------------------------------------

bool
GCCLinker::ValidateSettings(BMessage& inOutMessage)
{
	// (CodeGenerationSettings validated in compiler)
	
	LinkerSettings defaultLinker;
	defaultLinker.SetDefaults();

	bool changed = ValidateSetting<LinkerSettings>(inOutMessage,
												   kLinkerOptionsMessageName,
												   kLinkerMessageType,
												   defaultLinker);

	CommandLineText defaultCommandLineText;
	defaultCommandLineText.fVersion = CommandLineText::kCurrentVersion;
	strcpy(defaultCommandLineText.fText, kAdditionalCompilerOptionsDefaults);
	changed = ValidateSetting<CommandLineText>(inOutMessage, 
											   kAdditionalLinkerOptionsMessageName, 
											   kLinkerMessageType,
											   defaultCommandLineText) || changed;

	return changed;
}

// ---------------------------------------------------------------------------

status_t
GCCLinker::BuildPrecompileArgv(MProject&, BList&, MFileRec&)
{
	// we don't do anything at this stage of the build
	return B_ERROR;
}

// ---------------------------------------------------------------------------

status_t
GCCLinker::BuildCompileArgv(MProject& /* inProject */, BList& /* inArgv */, MakeActionT /* inAction */, MFileRec& /* inFileRec */)
{
	// we don't do anything at this stage of the build
	return B_ERROR;
}

// ---------------------------------------------------------------------------

status_t
GCCLinker::BuildPostLinkArgv(MProject&, BList&, MFileRec&)
{
	// we don't do anything at this stage of the build
	return B_ERROR;
}

// ---------------------------------------------------------------------------

bool
GCCLinker::FileIsDirty(MProject&, MFileRec&, MakeStageT, MakeActionT, time_t)
{
	return false;
}

// ---------------------------------------------------------------------------

status_t
GCCLinker::ParseMessageText(MProject&, const char* inText, BList& outList)
{
	LinkerErrorParser errorParser(inText, outList);
	errorParser.ParseText();
	return B_OK;
}

// ---------------------------------------------------------------------------

void
GCCLinker::CodeDataSize(MProject& /* inProject */, const char* /* inFilePath */, int32& outCodeSize, int32& outDataSize)
{
	outCodeSize = -1;
	outDataSize = -1;
}

// ---------------------------------------------------------------------------
										
status_t
GCCLinker::GenerateDependencies(MProject&, const char*, BList&)
{
	return B_ERROR;
}

// ---------------------------------------------------------------------------

void
GCCLinker::GetTargetFilePaths(MProject& /* inProject */, MFileRec& inFileRec, BList& inOutTargetFileList)
{
	// No documentation for this method. (code copied from MLinkerBuilder.cpp)
	// In MLinkerBuilder, it says...
	//     In a linker builder the file itself is the target file.

	inOutTargetFileList.AddItem(strdup(inFileRec.path));
}

// ---------------------------------------------------------------------------

void
GCCLinker::ProjectChanged(MProject& inProject, ChangeT inChange)
{
#ifdef _GCCDEBUG_
	fprintf(stderr, "GCCLinker::ProjectChanged\n");
#endif

	// ProjectChanged isn't documented in the plugin API, but it is important
	// because this is how we remember the project and also how we are notified
	// when the preferences change.  We don't get any other opportunity to
	// get the preferences (other than to validate them) before being asked to
	// generate the command line
	
	switch (inChange)
	{
		case kProjectOpened:
			fSettingsMap.AddSettings(inProject);
			this->SaveProjectSetting(inProject);
			break;

		case kProjectClosed:
			fSettingsMap.RemoveSettings(inProject);
			break;

		case kPrefsChanged:
			this->SaveProjectSetting(inProject);
			break;

		case kLinkDone:
			this->LinkDone(inProject);
			break;
			
		case kRunMenuItemChanged:
			this->CheckDebuggingOptions(inProject);
			break;

		case kBuildStarted:
		case kFilesAdded:
		case kFilesRemoved:
		case kFilesRearranged:
			// Don't do anything for now
			break;
	}
}

// ---------------------------------------------------------------------------

void
GCCLinker::LinkDone(MProject& inProject)
{
#ifdef _GCCDEBUG_
	fprintf(stderr, "GCCLinker::LinkDone\n");
#endif

	// The linking is done.  Do any housekeeping for the application/archive itself
	// (resources are handled by automatically if they are marked as "has resource")
	// Set the file type and attributes based on project settings

	entry_ref appRef;
	if (this->GetExecutableRef(inProject, appRef) != B_OK) {
		return;
	}

	// If we have an executable (app, shared library, driver), set access permissions
	ProjectLinkerSettings* cache = fSettingsMap.GetSettings(inProject);
	if (cache->fProjectOptions.pProjectKind != LibraryType) {
		BEntry appEntry(&appRef);
		BPath appPath;
		if (appEntry.GetPath(&appPath) == B_OK) {
			update_mime_info(appPath.Path(), false, true, true);

			// Set the Unix access permissions
			chmod(cache->fProjectOptions.pAppName, S_IRWXU + S_IRWXG + S_IRWXO);
		}
	}

	// Set the file type
	BFile appFile(&appRef, B_READ_WRITE);
	BAppFileInfo info(&appFile);	
	info.SetType(cache->fProjectOptions.pAppType);
	
}

// ---------------------------------------------------------------------------

void
GCCLinker::SaveProjectSetting(MProject& inProject)
{
	// We need to worry about both the linker preferences and the
	// project preferences here...
	
#ifdef _GCCDEBUG_
	fprintf(stderr, "GCCLinker::SaveCurrentPreferences\n");
#endif

	ProjectLinkerSettings* cache = fSettingsMap.GetSettings(inProject);

	// First get the linker preferences
	// LinkerSettings + CommandLineText
	BMessage message;
	inProject.GetPrefs(kLinkerMessageType, message);

	// LinkerSettings
	LinkerSettings* linkerSettings;
	long settingLength;
	if (message.FindData(kLinkerOptionsMessageName, kLinkerMessageType, (const void**) &linkerSettings, &settingLength) == B_OK) {
		cache->fLinkerSettings = *linkerSettings;
		cache->fLinkerSettings.SwapLittleToHost();
	}
	
	// CommandLineText
	CommandLineText* commandLine;
	if (message.FindData(kAdditionalLinkerOptionsMessageName, kLinkerMessageType,
						 (const void**) &commandLine, &settingLength) == B_OK) {
		cache->fCommandLineText = *commandLine;
		cache->fCommandLineText.SwapLittleToHost();
	}	
	
	// Now get the code generation preferences
	message.MakeEmpty();
	inProject.GetPrefs(kCompilerMessageType, message);

	CodeGenerationSettings* codeGenerationSettings;
	if (message.FindData(kCodeGenerationOptionsMessageName, kCompilerMessageType, 
						 (const void**) &codeGenerationSettings, &settingLength) == B_OK) {
		cache->fCodeGenerationSettings = *codeGenerationSettings;
		cache->fCodeGenerationSettings.SwapLittleToHost();
	}

	// Finally get the project preferences
	message.MakeEmpty();
	inProject.GetPrefs(kMWCCx86Type, message);

	ProjectPrefsx86* projectPrefs;
	long projectPreferenceLength;
	if (message.FindData(kProjectPrefsx86, kMWCCx86Type,
						 (const void**) &projectPrefs, &projectPreferenceLength) == B_OK) {
		cache->fProjectOptions = *projectPrefs;
		cache->fProjectOptions.SwapLittleToHost();
	}
	
#ifdef _GCCDEBUG_
	fprintf(stderr, "GCCLinker::SaveCurrentPreferences (prefs saved)\n");
	fprintf(stderr, "app name = %s, type = %s\n", 
			cache->fProjectOptions.pAppName, cache->fProjectOptions.pAppType);
#endif
}

// ---------------------------------------------------------------------------

void
GCCLinker::CleanArchive(MProject& inProject, const BString& archiveName)
{
	// Archives act differently than the normal compile/link tools in that
	// the output file doesn't get completely regenerated each iteration.
	// When we run ar, we just append (or replace) objects in the archive.
	// If we have renamed any files, the old objects can remain in the archive
	// and unfortunately, the old objects are first - leading to errors when
	// we use the archive.
	// As we begin to execute the link phase, delete the current archive.
	// Notice that this means existing archives from other sources cannot be merged
	// here in the BeIDE.  If a merging is needed, the shell plugin would have to
	// be used after the link step.

#ifdef _GCCDEBUG_
	fprintf(stderr, "GCCLinker::DeleteArchive\n");
#endif

	BDirectory projectDirectory;
	PlugInUtil::GetProjectDirectory(&inProject, projectDirectory);
	
	// using the project's directory, find the archive
	// (it is ok if we don't find it)
	BEntry targetArchive;
	status_t result = projectDirectory.FindEntry(archiveName.String(), &targetArchive);
	if (result == B_OK) {
		// delete the current archive file (if it exists)
		targetArchive.Remove();
	}
}


// ---------------------------------------------------------------------------

const char* kSwitchCompilerSettingsText = "Current selections in Project Settings/Code Generation \
do not specify the generation of debugging information. \
Would you like to automatically modify Code Generation settings for debugging? \
(This will trigger all sources to be recompiled.)";

const char* kSwitchBothSettingsText = "Current selections in Project Settings \
do not generate (or discard) debugging information. \
Would you like to automatically modify Project Settings for debugging? \
(This will trigger all sources to be recompiled/relinked.)";

const char* kSwitchLinkerSettingsText = "Current selections in Project Settings/x86 Linker \
discard needed debugging information. \
Would you like to automatically modify x86 Linker settings for debugging? \
(This will trigger all sources to be relinked.)";

void
GCCLinker::CheckDebuggingOptions(MProject& inProject)
{
	// This is called from ProjectChanged because the user has switched the debug/run menu
	// If the user enabled the debugger, make sure -g is specified and symbol stripping is off.
	// If not, then ask the user if they really want to switch the settings and recompile/relink
	// the entire project
	
	if (inProject.RunsWithDebugger()) {

		ProjectLinkerSettings* cache = fSettingsMap.GetSettings(inProject);

		bool promptChange = false;
		bool compile = false;
		bool link = false;
		const char* messageString = nil;
				
		if (cache->fCodeGenerationSettings.GetOption(kGenerateDebugSymbols) == false && 
				(cache->fLinkerSettings.GetOption(kStripSymbols) == true || cache->fLinkerSettings.GetOption(kStripLocals) == true)) {
			promptChange = true;
			messageString = kSwitchBothSettingsText;
			compile = true;
			link = true;
		}
		else if (cache->fCodeGenerationSettings.GetOption(kGenerateDebugSymbols) == false) {
			promptChange = true;
			messageString = kSwitchCompilerSettingsText;
			compile = true;
			link = false;
		}
		else if (cache->fLinkerSettings.GetOption(kStripSymbols) == true || cache->fLinkerSettings.GetOption(kStripLocals) == true) {
			promptChange = true;
			messageString = kSwitchLinkerSettingsText;
			compile = false;
			link = true;
		}
		
		// prompt the user depending on what needs to be fixed	
		if (promptChange) {
			BAlert* switchSettingsAlert = new BAlert("enabledebug", messageString, "No", "Yes", NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT); 
			switchSettingsAlert->SetShortcut(0, B_ESCAPE); 
			if (switchSettingsAlert->Go() == 1) {
			
				BMessage msg;
				// modify the appropriate settings (perhaps both)
				
				if (compile == true) {		
					// turn on debugging symbols
					cache->fCodeGenerationSettings.SetOption(kGenerateDebugSymbols, true);
				
					// make sure all optimizations are off	
					for (int i = kFirstOptLevel; i <= kLastOptLevel; i++) {
						cache->fCodeGenerationSettings.SetOption((ECodeGenerationOption)i, false);
					}
					cache->fCodeGenerationSettings.SetOption(kOptSpace, false);
				
					// well, except the lowest level optimization
					cache->fCodeGenerationSettings.SetOption(kOptLevel0, true);
				
					cache->fCodeGenerationSettings.SwapHostToLittle();
					msg.AddData(kCodeGenerationOptionsMessageName, kCompilerMessageType, &cache->fCodeGenerationSettings, sizeof(CodeGenerationSettings));
					cache->fCodeGenerationSettings.SwapLittleToHost();
				}
				
				if (link == true) {
					cache->fLinkerSettings.SetOption(kStripSymbols, false);
					cache->fLinkerSettings.SetOption(kStripLocals, false);
					cache->fLinkerSettings.SwapHostToLittle();
					msg.AddData(kLinkerOptionsMessageName, kLinkerMessageType, &cache->fLinkerSettings, sizeof(LinkerSettings));
					cache->fLinkerSettings.SwapLittleToHost();
				}
				
				
				// now tell the project so next make will force a recompile/relink
				inProject.SetPrefs(msg, compile ? kCompileUpdate : kLinkUpdate);
			}
		}
	}
}
