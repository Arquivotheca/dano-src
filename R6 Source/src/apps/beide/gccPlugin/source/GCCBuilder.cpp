// ---------------------------------------------------------------------------
/*
	GCCBuilder.cpp
	
	Copyright (c) 1998 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			20 September 1998

*/
// ---------------------------------------------------------------------------

#include "GCCBuilder.h"
#include "PlugIn.h"
#include "PlugInUtil.h"
#include "ErrorParser.h"
#include "CommandLineTextView.h"
#include "MProject.h"
#include "PlugInPreferences.h"
#include "ErrorMessage.h"
#include "IDEConstants.h"
#include "IDEMessages.h"
#include "ELFReader.h"

#include <String.h>
#include <Path.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <AppFileInfo.h>
#include <Application.h>
#include <Autolock.h>
#include <Debug.h>

// ---------------------------------------------------------------------------
// Hardcoded flags for different steps of the make process
// ---------------------------------------------------------------------------

const char* kOutputFileIsNextFlag = "-o";
const char* kGenerateDebugSymbolsFlag = "-g";
const char* kGenerateAssemblyFlag = "-S";
const char* kVerboseAssembly = "-fverbose-asm";
const char* kPreprocessOnlyFlag = "-E";
const char* kNoLinkFlag = "-c";
const char* kProjectSystemIncludeSeparator = "-I-";
const char* kIncludePathPrefix = "-I";
const char* kGenerateDepFileFlag = "-MMD";
const char* kUsePipesFlag = "-pipe";
const char* kCheckSyntaxFlag = "-fsyntax-only";
const char* kPathSeparatorStr = "/";

const char* kDependsFileExtension = ".d";
const char kNewLine = EOL_CHAR;
const char kSpace = ' ';
const char kColon = ':';
const char kBackSlash = '\\';
const char kPathSeparator = '/';
const char kPeriod = '.';

// This isn't defined in Mime.h yet, when it is, we can use that one...
const char* kObjectFileMimeType = "application/x-vnd.Be.ELF-object";

// ---------------------------------------------------------------------------
//	Class DependencyFileInterpreter (local class used by GCCBuilder)
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
/*
	Format...
	The dependency file lists each header file included in a make style output.
	The object file comes first, and then the list of files separated by
	spaces (or space then newline).
	Paths can have spaces also (since they are valid).  Any space is escaped
	with a backslash also. 
	
	Here are two samples...
	
CommandLineTextView.o: \
 /boot/home/exp/src/apps/beide/FakeProject/source/CommandLineTextView.cpp \
 [...]
 /boot/home/SomeHeader.h /boot/home/AnotherHeader.h \
 /boot/home/exp/src/apps/beide/FakeProject/source/PlugInUtil.h

my\ main.o: my\ main.cpp main.h foo.h

*/
// ---------------------------------------------------------------------------

class DependencyFileInterpreter
{
public:
			DependencyFileInterpreter(BEntry& fileEntry);
			~DependencyFileInterpreter();

	bool	GetNextPath(char filePath[B_FILE_NAME_LENGTH]);

private:
	char*	fText;
	char*	fCursor;
	BFile	fOpenFile;
};

// ---------------------------------------------------------------------------

DependencyFileInterpreter::DependencyFileInterpreter(BEntry& fileEntry)
						  : fOpenFile(&fileEntry, B_READ_ONLY)
{
	// First off - set fText/fCursor to NULL, that way we will report
	// zero entries in any error condition
	fText = NULL;
	fCursor = NULL;
	off_t fileSize = 0;
	
	// Verify that the BFile was constucted properly and that we
	// can read it
	if (fOpenFile.InitCheck() != B_OK || fOpenFile.IsReadable() != true) {
		// trouble opening/reading the file
		return;
	}

	// Read the entire file into a buffer we create (and delete in destructor)
	if (fOpenFile.GetSize(&fileSize) == B_OK) {
		fText = new char[fileSize + 1];
		if (fOpenFile.Read(fText, fileSize) != fileSize) {
			fileSize = 0;
		}
		fText[fileSize] = NIL;
	}
	else {
		// if we can't get the size for some reason, just
		// create a small empty string for simpler error handling
		fText = new char[1];
		fText[0] = NIL;
	}

	// get past the "myObject.o:" introduction in the file (see file format above)
	fCursor = strchr(fText, kColon);
	if (fCursor) {
		fCursor += 1;
	}

}

// ---------------------------------------------------------------------------

DependencyFileInterpreter::~DependencyFileInterpreter()
{
	delete [] fText;
}

// ---------------------------------------------------------------------------

bool
DependencyFileInterpreter::GetNextPath(char filePath[B_FILE_NAME_LENGTH])
{
	// Get the next path in our list of dependencies
	// return true if we get a valid file
	// otherwise return false
	
	// make sure the file path is initialized to NIL
	long characterCount = 0;
	filePath[0] = NIL;

	// First check if we even have any text left
	if (fCursor == NULL) {
		return false;
	}

	// clean up the left over before the next file name
	// (this simplifies out space handling below)
	while (*fCursor == kSpace || *fCursor == kNewLine || *fCursor == kBackSlash) {
		fCursor += 1;
	}

	// now iterate until we get to the next break between files (a space)	
	bool isEscaped = false;
	bool keepGoing = true;
	while (keepGoing) {
		char aChar = *fCursor++;
		switch (aChar) {
			case kBackSlash:
				// set the escape state
				isEscaped = true;
				break;

			case kSpace:
				// if we are escaped, turn off escape flag and add the character to the path
				// otherwise add a NIL (and we are done)
				if (isEscaped) {
					isEscaped = false;
				}
				else {
					aChar = NIL;
					keepGoing = false;
				}
				filePath[characterCount++] = aChar;
				break;

			case kNewLine:
				// just eat the newline (but turn off escape flag)
				isEscaped = false;
				break;

			case NIL:
				fCursor = NULL;
				filePath[characterCount++] = NIL;
				keepGoing = false;
				break;

			default:
				filePath[characterCount++] = aChar;
				break;
		}
	}
	
	// check if we added any non-NIL character to the string
	if (filePath[0] == NIL) {
		return false;
	}
	
	return true;
}

// ---------------------------------------------------------------------------
// ProjectSettings Member functions
// ---------------------------------------------------------------------------

ProjectSettings::ProjectSettings() 
				: fProjectIncludes(kIncludePathPrefix),
				  fSystemIncludes(kIncludePathPrefix)
{
	// While it doesn't seem like we should do it here, this is needed
	// because of the order of calls (and the fact that I delete old
	// ProjectSettings when new ones are created.)
	// For example, in a new project, the order of calls can be:
	//	1. kProjectOpened (no prefs in message)
	// 	2. kPrefsChanged (no prefs in message)
	//	3. BuildStandardArgv - here we need the default settings

	fLanguageSettings.SetDefaults();
	fCommonWarningSettings.SetDefaults();
	fWarningSettings.SetDefaults();
	fCodeGenerationSettings.SetDefaults();
	fCommandLineText.fVersion = CommandLineText::kCurrentVersion;
	strcpy(fCommandLineText.fText, kAdditionalCompilerOptionsDefaults);
	fTreatQuotesAsBrackets = false;
}

// ---------------------------------------------------------------------------

ProjectSettings::~ProjectSettings()
{
	// delete all internal allocations
	ProjectSettings::Reset();
}

// ---------------------------------------------------------------------------

void
ProjectSettings::Reset()
{
	// clear the include caches
	fProjectIncludes.FlushCache();
	fSystemIncludes.FlushCache();
}

// ---------------------------------------------------------------------------
// GCCBuilder Member functions
// ---------------------------------------------------------------------------

GCCBuilder::GCCBuilder() 
{	
	// set up the path to the temp file directory (remains constant)
	
	fTempFileDirectoryName[0] = NIL;
	BPath tempDirectory;
	if (find_directory(B_COMMON_TEMP_DIRECTORY, &tempDirectory) == B_OK) {
		strcpy(fTempFileDirectoryName, tempDirectory.Path());
		get_ref_for_path(tempDirectory.Path(), &fTempFileDirectory);
	}
}

// ---------------------------------------------------------------------------

GCCBuilder::~GCCBuilder()
{
}

// ---------------------------------------------------------------------------

status_t
GCCBuilder::GetToolName(MProject* /* inProject */, char* outName, int32 inBufferLength,
						MakeStageT /* inStage */, MakeActionT /* inAction */)
{
	strncpy(outName, kGCCCompilerName, inBufferLength);
	return B_OK;
}

// ---------------------------------------------------------------------------

const char*
GCCBuilder::LinkerName()
{
	return kGCCLinkerName;
}

// ---------------------------------------------------------------------------

MakeStageT
GCCBuilder::MakeStages()
{
	// gcc doesn't do precompiled headers (!kPrecompileStage)
	return kCompileStage;
}

// ---------------------------------------------------------------------------

MakeActionT
GCCBuilder::Actions()
{
	// gcc doesn't do kPrecompile
	return (kCompile | kPreprocess | kCheckSyntax | kDisassemble);
}

// ---------------------------------------------------------------------------

PlugInFlagsT
GCCBuilder::Flags()
{
	return kNotIDEAware;
}

// ---------------------------------------------------------------------------

ulong
GCCBuilder::MessageDataType()
{
	return kCompilerMessageType;
}

// ---------------------------------------------------------------------------


bool
GCCBuilder::ValidateSettings(BMessage& inOutMessage)
{
	bool changed = false;

	// Validate each setting
	// For each setting, set up a default in case we need to perform an update

	// Language
	LanguageSettings defaultLanguage;
	defaultLanguage.SetDefaults();
	changed = ValidateSetting<LanguageSettings>(inOutMessage, 
												kLanguageOptionsMessageName, 
												kCompilerMessageType, 
												defaultLanguage) || changed;

	// Common warnings
	CommonWarningSettings defaultCommonWarning;
	defaultCommonWarning.SetDefaults();
	changed = ValidateSetting<CommonWarningSettings>(inOutMessage, 
													 kCommonWarningOptionsMessageName, 
													 kCompilerMessageType, 
													 defaultCommonWarning) || changed;

	// Warnings
	WarningSettings defaultWarning;
	defaultWarning.SetDefaults();
	changed = ValidateSetting<WarningSettings>(inOutMessage, 
											   kWarningOptionsMessageName,
											   kCompilerMessageType,
											   defaultWarning) || changed;

	// Code generation
	CodeGenerationSettings defaultCodeGeneration;
	defaultCodeGeneration.SetDefaults();
	changed = ValidateSetting<CodeGenerationSettings>(inOutMessage,
													  kCodeGenerationOptionsMessageName,
													  kCompilerMessageType,
													  defaultCodeGeneration) || changed;

	// Supplemental command line
	CommandLineText defaultCommandLineText;
	defaultCommandLineText.fVersion = CommandLineText::kCurrentVersion;
	strcpy(defaultCommandLineText.fText, kAdditionalCompilerOptionsDefaults);
	changed = ValidateSetting<CommandLineText>(inOutMessage,
											   kAdditionalCompilerOptionsMessageName,
											   kCompilerMessageType,
											   defaultCommandLineText) || changed;

	return changed;
}

// ---------------------------------------------------------------------------

status_t
GCCBuilder::BuildPrecompileArgv(MProject& inProject, BList& inArgv, MFileRec& inFileRec)
{
	// gcc doesn't do precompiled headers
	return B_ERROR;
}

// ---------------------------------------------------------------------------

status_t
GCCBuilder::BuildCompileArgv(MProject& inProject, BList& inArgv, MakeActionT inAction, MFileRec& inFileRec)
{
	if (inAction == kPrecompile) {
		return B_ERROR;
	}

	ProjectSettings* cache = fSettingsMap.GetSettings(inProject);
	switch (inAction) {
		case kCompile:
		{
			// use pipes rather than temporary files
			// 5/01 - pipes can cause a hang in certain error conditions
			// and in 4 minutes of compiling, they don't speed up the compile
			// inArgv.AddItem(strdup(kUsePipesFlag));

			// generate dependency info as we compile
			inArgv.AddItem(strdup(kGenerateDepFileFlag));

			// add -o objectFileName
			BString objectFileName;
			this->BuildObjectFileName(cache->fObjectFileDirectoryPath, inFileRec.name, objectFileName);

			inArgv.AddItem(strdup(kOutputFileIsNextFlag));
			inArgv.AddItem(strdup(objectFileName.String()));

			// Top everything off with -c for no linking
			inArgv.AddItem(strdup(kNoLinkFlag));
			break;
		}
		case kCheckSyntax:
			// use pipes rather than temporary files
			// 5/01 - pipes can cause a hang in certain error conditions
			// and in 4 minutes of compiling, they don't speed up the compile
			// inArgv.AddItem(strdup(kUsePipesFlag));			

			// specify "check syntax only"
			inArgv.AddItem(strdup(kCheckSyntaxFlag));
			break;
		
		case kPreprocess:
		{
			// add -E for preprocessing
			inArgv.AddItem(strdup(kPreprocessOnlyFlag));
			
			// We want to open up the preprocessor output when we
			// are done.  Unfortunately, we don't know that the user
			// requested kPreprocess when we get to "CompileDone".
			// (which right now is CodeDataSize/GenerateDependencies).
			// The existence of this temporary _i file will trigger
			// us to open it up for the user.
			
			char outputFile[B_FILE_NAME_LENGTH];
			this->BuildIntermediateFileName(inFileRec.name, outputFile, true);
			inArgv.AddItem(strdup(kOutputFileIsNextFlag));
			inArgv.AddItem(strdup(outputFile));
			
			break;
		}
		
		case kDisassemble:
		{
			// add -S for assembly creation
			// -g for debbugger info
			// -fverbose-asm for extra goodies in asm code
			inArgv.AddItem(strdup(kGenerateAssemblyFlag));
			inArgv.AddItem(strdup(kGenerateDebugSymbolsFlag));
			inArgv.AddItem(strdup(kVerboseAssembly));
			
			// (see comments above for preprocessor output)
			char outputFile[B_FILE_NAME_LENGTH];
			this->BuildIntermediateFileName(inFileRec.name, outputFile, true);
			inArgv.AddItem(strdup(kOutputFileIsNextFlag));
			inArgv.AddItem(strdup(outputFile));
			break;
		}
	}
		
	// Now add all the options that we have from preferences
	this->BuildStandardArgv(&inProject, inArgv);
	
	// override (or add) options with our supplementation list
	cache->fCommandLineText.FillArgvList(inArgv);

	// Add all the include paths
	// add a -I- before all include paths if we are to treat
	// quotes as brackets, otherwise add the -I- inbetween
	// the project and system includes
	if (cache->fTreatQuotesAsBrackets == true) {
		inArgv.AddItem(strdup(kProjectSystemIncludeSeparator));
	}
	cache->fProjectIncludes.FillArgvList(inArgv);
	if (cache->fTreatQuotesAsBrackets == false) {
		inArgv.AddItem(strdup(kProjectSystemIncludeSeparator));
	}
	cache->fSystemIncludes.FillArgvList(inArgv);
	
	// Finally add the source file to compile
	inArgv.AddItem(strdup(inFileRec.path));

#ifdef _GCCDEBUG_
	fprintf(stderr, "GCCBuilder::BuildCompileArgv...\n");
	for (int i = 0; i < inArgv.CountItems(); i++) {
		char* anArg = (char*) inArgv.ItemAt(i);
		fprintf(stderr, "%s\n", anArg);
	}
	fprintf(stderr, "...GCCBuilder::BuildCompileArgv\n");
#endif

	return B_OK;
}

// ---------------------------------------------------------------------------

status_t
GCCBuilder::BuildPostLinkArgv(MProject& /* inProject */, BList& /* inArgv */, MFileRec& /* inFileRec */)
{
	// we don't do anything at this stage of the build
	return B_ERROR;
}

// ---------------------------------------------------------------------------

void
GCCBuilder::BuildStandardArgv(MProject* inProject, BList& inArgv)
{
#ifdef _GCCDEBUG_
	fprintf(stderr, "GCCBuilder::CreateStandardArgv\n");
#endif

	// Add all the preference settings (which command line settings last so they can override)
	ProjectSettings* cache = fSettingsMap.GetSettings(*inProject);
	
	cache->fLanguageSettings.AddAllOptions(inArgv);
	cache->fCommonWarningSettings.AddAllOptions(inArgv);
	cache->fWarningSettings.AddAllOptions(inArgv);
	cache->fCodeGenerationSettings.AddAllOptions(inArgv);
}

// ---------------------------------------------------------------------------

void
GCCBuilder::IterateDirectoryList(BList& directoryList, IncludePathCache& theCache)
{
#ifdef _GCCDEBUG_
	fprintf(stderr, "GCCBuilder::IterateDirectoryList\n");
#endif

	// Iterate the directory list
	// For each directory marked recursive, add subdirectories to the
	// list also
	
	// The BList comes from the IDE as a list of AccessDirectoryInfo
	
	int32 numDirectories = directoryList.CountItems();
	for (int i = 0; i < numDirectories; i++) {
		AccessDirectoryInfo* info = (AccessDirectoryInfo*) directoryList.ItemAt(i);
		theCache.AddADirectory(info->fDirectory, info->fSearchRecursive);
	}
}

// ---------------------------------------------------------------------------

void
GCCBuilder::EmptyDirectoryList(BList& directoryList)
{
	AccessDirectoryInfo* info;
	int numItems = directoryList.CountItems();
	for (int32 i = 0; i < numItems; i++) {
		info = (AccessDirectoryInfo*) directoryList.ItemAtFast(i);
		delete info;
	}
	
	directoryList.MakeEmpty();
}


// ---------------------------------------------------------------------------

bool
GCCBuilder::FileIsDirty(MProject& inProject, MFileRec& inFileRec, MakeStageT /* inStage */,
						MakeActionT /* inAction */, time_t inModDate)
{
	ProjectSettings* cache = fSettingsMap.GetSettings(inProject);

	// Metrowerks takes care of most of the file dirty checking.
	// This method returns true if the object file is older than source file
	BString objectFileName;
	this->BuildObjectFileName(cache->fObjectFileDirectoryPath, inFileRec.name, objectFileName);
		
	BEntry objectFile;
	BDirectory directory(&cache->fObjectFileDirectory);
	
	// Make sure we can get to the directory... If not, go make it again
	if (directory.InitCheck() != B_OK) {
		this->CacheObjectFileDirectory(inProject);
		directory.SetTo(&cache->fObjectFileDirectory);
	}
	
	if (directory.FindEntry(objectFileName.String(), &objectFile) == B_ERROR) {
		// can't find the object file - source must be dirty
		return true;
	}
	if (objectFile.IsFile() == false) {
		// the object file better be a file - source is dirty
		return true;
	}
	time_t objectModDate;
	objectFile.GetModificationTime(&objectModDate);
	bool isDirty = true;
	if (objectModDate > inModDate) {
		// if we get here, the object file exists, it is a file, and its modification
		// date is later than the source file - it must not be dirty
		isDirty = false;
	}
	
	return isDirty;
}


// ---------------------------------------------------------------------------

status_t
GCCBuilder::ParseMessageText(MProject& inProject, const char* inText, BList& outList)
{
	CompilerErrorParser errorParser(inText, outList);
	errorParser.ParseText();
	return B_OK;	
}

// ---------------------------------------------------------------------------

void
GCCBuilder::CodeDataSize(MProject& inProject, const char* inFilePath, int32& outCodeSize, int32& outDataSize)
{
#ifdef _GCCDEBUG_
	fprintf(stderr, "GCCBuilder::CodeDataSize: %s\n", inFilePath);
#endif

	// We don't get a "CompileDone" message like the linker
	// This is the notification we get that the file has been compiled
	// So we do a few extra things here and in GCCBuilder::GenerateDependencies
	// that would better be done in a "CompileDone(actionJustPerformed)"
	//		1. Open up the preprocessed file if it exists (and then delete it)
	//		(done in GenerateDependencies)
	//		2. Set the mime type of the object file
	// In adidition, we do what this method was intended for...
	// Calculate how much code and data were generated in the object file
	// Take the inFilePath that comes in, and create its corresponding object file
	// Then open that up and read the elf headers to calculate sizes
		
	// inFilePath is a full path, but BuildObjectFileName wants a leaf
	// get to the last path separator
	
	ProjectSettings* cache = fSettingsMap.GetSettings(inProject);

	char* leafName = strrchr(inFilePath, kPathSeparator);
	if (leafName) {
		// bump leafName past slash
		leafName += 1;
	}
	
	BString objectFilePath;
	this->BuildObjectFileName(cache->fObjectFileDirectoryPath, leafName ? leafName : inFilePath, objectFilePath);
	
	ELFReader elfReader(objectFilePath.String());
	if (elfReader.InitCheck() != B_OK) {
		// trouble opening/reading/understanding the file
		outCodeSize = -1;
		outDataSize = -1;
		return;
	}
	
	// Iterprete the elf headers for code/data size
	if (elfReader.CodeDataSize(outCodeSize, outDataSize) != B_OK) {
		outCodeSize = -1;
		outDataSize = -1;		
	}

	// We don't get a "CompileDone" message like the linker
	// This is the notification we get that the file has been compiled
	// Set the mime type of the object file (but only if needed, 
	// otherwise we force a link when we might
	// not need to for check syntax, preprocess cases)
	BFile objectFile(objectFilePath.String(), B_READ_WRITE);
	BAppFileInfo info(&objectFile);
	char currentType[B_MIME_TYPE_LENGTH];
	currentType[0] = NIL;
	info.GetType(currentType);
	if (strcmp(currentType, kObjectFileMimeType) != 0) {
		info.SetType(kObjectFileMimeType);
	}
	
#ifdef _GCCDEBUG_
	fprintf(stderr, "GCCBuilder::CodeDataSize: %d : %d\n", outCodeSize, outDataSize);
#endif
}

// ---------------------------------------------------------------------------
										
status_t
GCCBuilder::GenerateDependencies(MProject& inProject, const char* inFilePath, BList& outList)
{
	// Metrowerks provides no documentation for GenerateDependencies
	// Populate outList with entry_ref's for each file included by inFilePath
	// As we compiled inFilePath, we generated a inFilePath.d
	// (This is dropped in the project directory by gcc.)
	// Open it up
	// Read the lines
	// Create entry_ref for each file
	// Delete the file
	
	// In addition to the above, if we can't find the dependency file
	// then I assume they did a kPreprocess and look for the
	// preprocess output (the existence of which is my only trigger
	// that they requested preprocess)
	
#ifdef _GCCDEBUG_
	fprintf(stderr, "GCCBuilder::GenerateDependencies\n");
#endif
	// note... rather than nesting 100 levels deep, I return as soon as I get an error

	// inFilePath is a full path, we want just the leaf name
	char* leafNamePart = strrchr(inFilePath, kPathSeparator);
	if (leafNamePart == NULL) {
		return B_ERROR;
	}
	
	// bump leafNamePart past slash
	leafNamePart += 1;
	
	// copy leafNamePart into modifiable buffer
	char leafName[B_PATH_NAME_LENGTH];
	strcpy(leafName, leafNamePart);
	
	// now find the extension and replace with .d
	char* dotPtr = strrchr(leafName, '.');
	if (dotPtr == NULL) {
		return B_ERROR;
	}
	
	// Ok, replace the suffix with .d and look up that file
	// in the project directory
	*dotPtr = NIL;
	strcat(leafName, kDependsFileExtension);

	BDirectory projectDirectory;
	PlugInUtil::GetProjectDirectory(&inProject, projectDirectory);
	
	BEntry depFileEntry;
	if (projectDirectory.FindEntry(leafName, &depFileEntry) != B_OK) {
		// We can't find the dependeny file
		// Well, maybe the user did "preprocess" or "disassemble"...
		// (Look for the intermediate output file and open it up)
		// And then just return with B_ERROR
				
		this->OpenIntermediateFile(leafNamePart);
		return B_ERROR;
	}
	
	DependencyFileInterpreter depFileReader(depFileEntry);	
	char filePath[B_FILE_NAME_LENGTH];
	while (depFileReader.GetNextPath(filePath)) {
		entry_ref aRef;
		// one of the dependencies is the file itself, so ignore that one
		if (strcmp(filePath, inFilePath) == 0) {
			continue;
		}
		if (get_ref_for_path(filePath, &aRef) == B_OK) {
			outList.AddItem(new entry_ref(aRef));
		}
	}
	
	// we have used the dependency file, delete it from the file system
	depFileEntry.Remove();
	return B_OK;
}

// ---------------------------------------------------------------------------

void
GCCBuilder::GetTargetFilePaths(MProject& inProject, MFileRec& inFileRec, BList& inOutTargetFileList)
{
	// This is probably used to generate the link command line.  There is
	// no documentation for it.
	// The comment in MmwccBuilder.cpp says:
	//   Add the full paths of the target files for this source file to the
	//   targetlist.

#ifdef _GCCDEBUG_
	fprintf(stderr, "GCCBuilder::GetTargetFilePaths called with %s\n", inFileRec.name);
#endif

	// ignore pch files and .h files
	if (inFileRec.makeStage != kIgnoreStage)
	{
		BString objectFileName;
		ProjectSettings* cache = fSettingsMap.GetSettings(inProject);
		this->BuildObjectFileName(cache->fObjectFileDirectoryPath, inFileRec.name, objectFileName);
		inOutTargetFileList.AddItem(strdup(objectFileName.String()));
	}
}

// ---------------------------------------------------------------------------

void
GCCBuilder::ProjectChanged(MProject& inProject, ChangeT inChange)
{
#ifdef _GCCDEBUG_
	fprintf(stderr, "GCCBuilder::ProjectChanged\n");
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
			this->CacheObjectFileDirectory(inProject);
			break;

		case kProjectClosed:
			fSettingsMap.RemoveSettings(inProject);
			break;

		case kPrefsChanged:
			this->SaveProjectSetting(inProject);
			break;
			
		case kFilesAdded:
		case kFilesRemoved:
			// this could potentially change access paths
			// so make sure the preferences are all up to date
			this->SaveProjectSetting(inProject);
			break;
			
		case kBuildStarted:
		case kFilesRearranged:
		case kRunMenuItemChanged:
		case kLinkDone:
			// Don't do anything for now
			break;
	}
}

// ---------------------------------------------------------------------------

void
GCCBuilder::BuildObjectFileName(BString& objectPath, const char* fileName, BString& objectFileName)
{
#ifdef _GCCDEBUG_
	fprintf(stderr, "GCCBuilder::BuildObjectFileName \n");
#endif

	// turn the fileName into the corresponding object file name
	// ie: turn myFile.cpp into /boot/myStuff/Objects/myFile.o
	// On occasion, the fileName can have '/'.  A common example is 
	// something like x86/libbe.so.  However, a cpp can also be partially
	// specified if it is added under an access path that isn't recursive.
	
	// First, copy in the name without the extension
	BString leafNamePart;
	char* periodPtr = strrchr(fileName, '.');
	int32 copyLength = (periodPtr) ? periodPtr-fileName : strlen(fileName);
	leafNamePart.Append(fileName, copyLength);
	
	// Make sure we don't have any '/' in our name by replacing all with '.'
	leafNamePart.ReplaceAll(kPathSeparator, kPeriod);

	// Add the object file extension
	leafNamePart += ".o";
	
	// Now prepare the full path to the objects directory
	// (separated by a / of course...)
	objectFileName = objectPath;
	objectFileName += kPathSeparator;
	
	// And add in the sanitized leaf name
	objectFileName += leafNamePart;
	
#ifdef _GCCDEBUG_
	fprintf(stderr, "leaving GCCBuilder::BuildObjectFileName - %s\n", objectFileName.String());
#endif
}

// ---------------------------------------------------------------------------

void
GCCBuilder::SaveProjectSetting(MProject& inProject)
{
#ifdef _GCCDEBUG_
	fprintf(stderr, "GCCBuilder::SaveProjectSetting\n");
#endif

	// Save the current state of all preference panels and access paths
	// associate them with inProject
	// (prepare the cache for new settings)
	ProjectSettings* cache = fSettingsMap.GetSettings(inProject);
	cache->Reset();
	
	// now update our current cache based on actual project settings
	// First the preference panels...
	BMessage message;
	inProject.GetPrefs(kCompilerMessageType, message);
	
	// The compiler specific preferences are:
	// 1. Language
	// 2. Common warnings
	// 3. (more) warnings
	// 4. Code generation
	// 5. Supplemental command line settings

	// (step 1)
	LanguageSettings* languageSettings;
	long settingLength;
	if (message.FindData(kLanguageOptionsMessageName, kCompilerMessageType, (const void**) &languageSettings, &settingLength) == B_OK) {
		cache->fLanguageSettings = *languageSettings;
		cache->fLanguageSettings.SwapLittleToHost();
	}

	// (step 2)
	CommonWarningSettings* commonWarningSettings;
	if (message.FindData(kCommonWarningOptionsMessageName, kCompilerMessageType, (const void**) &commonWarningSettings, &settingLength) == B_OK) {
		cache->fCommonWarningSettings = *commonWarningSettings;
		cache->fCommonWarningSettings.SwapLittleToHost();
	}

	// (step 3)
	WarningSettings* warningSettings;
	if (message.FindData(kWarningOptionsMessageName, kCompilerMessageType, (const void**) &warningSettings, &settingLength) == B_OK) {
		cache->fWarningSettings = *warningSettings;
		cache->fWarningSettings.SwapLittleToHost();
	}

	// (step 4)
	CodeGenerationSettings* codeGenSettings;
	if (message.FindData(kCodeGenerationOptionsMessageName, kCompilerMessageType, (const void**) &codeGenSettings, &settingLength) == B_OK) {
		cache->fCodeGenerationSettings = *codeGenSettings;
		cache->fCodeGenerationSettings.SwapLittleToHost();
	}

	// (step 5)
	CommandLineText* commandLine;
	if (message.FindData(kAdditionalCompilerOptionsMessageName, kCompilerMessageType,
						 (const void**) &commandLine, &settingLength) == B_OK) {
		cache->fCommandLineText = *commandLine;
		cache->fCommandLineText.SwapLittleToHost();
	}
	
	// keep track of the include paths for this project

	// Get the access paths from the project, and turn them into
	// a set of cached include paths for the compiler	
	BList systemPathList;
	BList projectPathList;
	inProject.GetAccessDirectories(projectPathList, systemPathList, cache->fTreatQuotesAsBrackets);

	// add each of the directories from both lists
	this->IterateDirectoryList(projectPathList, cache->fProjectIncludes);
	this->IterateDirectoryList(systemPathList, cache->fSystemIncludes);
	
	// clean up (and delete) all entries in the different lists
	// (we own the entries in the lists)
	this->EmptyDirectoryList(systemPathList);
	this->EmptyDirectoryList(projectPathList);
}

// ---------------------------------------------------------------------------

void
GCCBuilder::CacheObjectFileDirectory(MProject& inProject)
{
	// Keep the object file directory around so that we don't have
	// to go find it for every file.
	
	// First, if the directory doesn't exist, create it
	// Second, set up our fObjectFileDirectory & fObjectFileDirectoryPath member
	
	// The project allows us to get at the entry_ref for the project
	// use this to get to (or create) a directory at the same level
	// as the project

#ifdef _GCCDEBUG_
	fprintf(stderr, "GCCBuilder::CacheObjectFileDirectory\n");
#endif
	
	ProjectSettings* cache = fSettingsMap.GetSettings(inProject);

	BDirectory projectDirectory;
	PlugInUtil::GetProjectDirectory(&inProject, projectDirectory);
	
	// create the full name of the objects directory
	// something like "(Objects.myProject)"

	BString dirName(kObjectDirectoryName);
	entry_ref ref;
	char projectName[B_PATH_NAME_LENGTH];
	inProject.GetProjectRef(ref);
	strcpy(projectName, ref.name);
	char* extension = strrchr(projectName, kPeriod);
	if (extension) {
		*extension = NIL;
	}	
	dirName.Insert(".", dirName.Length()-1);
	dirName.Insert(projectName, dirName.Length()-1);
    
	// search for the object file directory in the project directory
	// create it if we can't find it

	BEntry objectDirectoryEntry;
	BDirectory objectDirectory;
	if (projectDirectory.FindEntry(dirName.String(), &objectDirectoryEntry) == B_OK) {
		objectDirectoryEntry.GetRef(&cache->fObjectFileDirectory);
	}
	else {
		// not found - need to create the directory
		// first - see if the old version exists "(Objects)"
		// if so, rename that to what we currently want

		if (projectDirectory.FindEntry(kObjectDirectoryName, &objectDirectoryEntry) == B_OK) {
			objectDirectoryEntry.Rename(dirName.String());
			objectDirectoryEntry.GetRef(&cache->fObjectFileDirectory);
		}
		else if (projectDirectory.CreateDirectory(dirName.String(), &objectDirectory) == B_OK) {
			objectDirectory.GetEntry(&objectDirectoryEntry);
			objectDirectoryEntry.GetRef(&cache->fObjectFileDirectory);
		}
		else {
			ASSERT_WITH_MESSAGE(false, "CacheObjectFileDiretory - can not create object directory");
			return;
		}
	}

	// keep around a full path also, so we can quickly generate the name of each .o file
	BPath objectDirectoryPath;
	if (objectDirectoryEntry.GetPath(&objectDirectoryPath) == B_OK) {
		cache->fObjectFileDirectoryPath = objectDirectoryPath.Path();
	}
	else {
		ASSERT_WITH_MESSAGE(false, "CacheObjectFileDiretory - Can't get path from object directory path");
	}

#ifdef _GCCDEBUG_
	fprintf(stderr, "leaving GCCBuilder::CacheObjectFileDirectory - %s\n", fObjectFileDirectoryPath.String());
#endif
}

// ---------------------------------------------------------------------------

void
GCCBuilder::BuildIntermediateFileName(const char* sourceName, char outputFile[B_FILE_NAME_LENGTH], bool fullPath)
{
	// little utility function to turn a source name (like mySource.cpp)
	// into a temporary file (like mySource.cpp_i) in the shared temp directory

	if (fullPath) {	
		strcpy(outputFile, fTempFileDirectoryName);
		strcat(outputFile, kPathSeparatorStr);
	}
	else {
		outputFile[0] = NIL;
	}
	
	strcat(outputFile, sourceName);
	strcat(outputFile, "_i");
}

// ---------------------------------------------------------------------------

void
GCCBuilder::OpenIntermediateFile(const char* sourceFile)
{
	// See if the intermediate (preprocess or disassembly) file output exists
	// If so we open up the file in a window (and delete the file)
	// Notice that the existence of this temporary file is our only
	// clue that the user asked for preprocess/disassmbly at compile time
	
	char preprocessOutput[B_FILE_NAME_LENGTH];
	this->BuildIntermediateFileName(sourceFile, preprocessOutput, false);

	BEntry fileEntry;
	BDirectory directory(&fTempFileDirectory);
	if (directory.FindEntry(preprocessOutput, &fileEntry) != B_OK) {
		// can't find file - no intermediate output specified
		// just return
		return;
	}
	
	BFile openFile(&fileEntry, B_READ_ONLY);
	off_t fileSize = 0;
	
	// Verify that the BFile was constucted properly and that we
	// can read it
	if (openFile.InitCheck() != B_OK || openFile.IsReadable() != true) {
		// trouble opening/reading the file
		// delete the entry and return
		fileEntry.Remove();
		return;
	}

	char* theContents = NULL;
	// Read the entire file into a buffer we create (and delete as we exit)
	if (openFile.GetSize(&fileSize) == B_OK) {
		theContents = new char[fileSize + 1];
		if (openFile.Read(theContents, fileSize) != fileSize) {
			fileSize = 0;
		}
	}
	
	// Finally open up the file in a temporary window
	if (fileSize != 0) {
		// Send a syncronous message to the IDEApp so we don't return 
		// here and delete theContents (or tempFileName) before we actually use it
		BMessenger app(be_app);
		BMessage reply;
		BMessage msg(msgRawTextWindow);
		
		char tempFileName[B_FILE_NAME_LENGTH];
		strcpy(tempFileName, "#");
		strcat(tempFileName, sourceFile);
		
		// see IDEMessages for the format of this message to IDEApp
		msg.AddPointer(kAddress, theContents);
		msg.AddInt32(kSize, fileSize);
		msg.AddString(kFileName, tempFileName);

		app.SendMessage(&msg, &reply);
	}

	delete [] theContents;
	
	// Don't leave the temp file hanging around, the existence of this
	// file is our only clue that the user wants to look at it	
	fileEntry.Remove();
}
