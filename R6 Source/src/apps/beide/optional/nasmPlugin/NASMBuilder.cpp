// ---------------------------------------------------------------------------
/*
	NASMBuilder.cpp
		
	Author:	John R. Dance
			21 June 1999

*/
// ---------------------------------------------------------------------------

#include "NASMBuilder.h"
#include "ErrorMessage.h"
#include "MProject.h"
#include "Plugin.h"
#include "PlugInUtil.h"
#include "CommandLineText.h"
#include "ELFReader.h"
#include "IDEMessages.h"

#include <Application.h>
#include <AppFileInfo.h>
#include <File.h>
#include <String.h>
#include <Entry.h>
#include <Path.h>
#include <FindDirectory.h>
#include <Debug.h>

#include <stdio.h>
#include <stdlib.h>

const char* kToolName = "nasm";

const char kSpace = ' ';
const char kColon = ':';
const char kBackSlash = '\\';
const char kPathSeparator = '/';
const char kPeriod = '.';
const char* kPathSeparatorStr = "/";

// ---------------------------------------------------------------------------
//	NasmSettings member functions
// ---------------------------------------------------------------------------

NasmSettings::NasmSettings()
{
	// set up the settings to their defaults
	fCommandLineOptions.fVersion = CommandLineText::kCurrentVersion;
	strcpy(fCommandLineOptions.fText, "");
}

// ---------------------------------------------------------------------------

NasmSettings::~NasmSettings()
{
}

// ---------------------------------------------------------------------------
//	NASMBuilder member functions
// ---------------------------------------------------------------------------

NASMBuilder::NASMBuilder()
{	
	// set up the path to the temp file directory (remains constant)
	
	fTempFileDirectoryName[0] = NIL;
	BPath tempDirectory;
	if (find_directory(B_COMMON_TEMP_DIRECTORY, &tempDirectory) == B_OK) {
		strcpy(fTempFileDirectoryName, tempDirectory.Path());
		get_ref_for_path(fTempFileDirectoryName, &fTempFileDirectory);
	}
}

// ---------------------------------------------------------------------------

NASMBuilder::~NASMBuilder()
{
}

// ---------------------------------------------------------------------------

status_t 
NASMBuilder::GetToolName(MProject* inProject, char *outName, int32 inBufferLength, MakeStageT inStage, MakeActionT inAction)
{
	strncpy(outName, kToolName, inBufferLength);
	return B_OK;
}

// ---------------------------------------------------------------------------

const char *
NASMBuilder::LinkerName()
{
	// we don't deal with linking
	return B_EMPTY_STRING;
}

// ---------------------------------------------------------------------------

MakeStageT 
NASMBuilder::MakeStages()
{
	// all our work is done during compile
	return kCompileStage;	
}

// ---------------------------------------------------------------------------

MakeActionT 
NASMBuilder::Actions()
{
	return (kCompile | kPreprocess | kCheckSyntax | kDisassemble);
}

// ---------------------------------------------------------------------------

PlugInFlagsT 
NASMBuilder::Flags()
{
	// nasm is not ide aware
	return kNotIDEAware;
}

// ---------------------------------------------------------------------------

ulong 
NASMBuilder::MessageDataType()
{
	return kNASMMessageType;
}

// ---------------------------------------------------------------------------

bool 
NASMBuilder::ValidateSettings(BMessage &inOutMessage)
{
	CommandLineText defaultCommandLineText;
	defaultCommandLineText.fVersion = CommandLineText::kCurrentVersion;
	strcpy(defaultCommandLineText.fText, "");
	return ValidateSetting<CommandLineText>(inOutMessage, 
											kNASMOptionsMessageName, 
											kNASMMessageType, 
											defaultCommandLineText);
}

// ---------------------------------------------------------------------------

status_t 
NASMBuilder::BuildPrecompileArgv(MProject& inProject, BList &inArgv, MFileRec &inFileRec)
{
	// we don't do anything in this stage
	return B_ERROR;
}

// ---------------------------------------------------------------------------

status_t 
NASMBuilder::BuildCompileArgv(MProject& inProject, BList &inArgv, MakeActionT inAction, MFileRec &inFileRec)
{
	if (inAction == kPrecompile) {
		return B_ERROR;
	}

	// always output elf format
	inArgv.AddItem(strdup("-f"));
	inArgv.AddItem(strdup("elf"));
	
	NasmSettings* cache = fSettingsMap.GetSettings(inProject);
	switch (inAction) {
		case kCompile:
		case kCheckSyntax:
		{
			// add -o objectFileName
			BString objectFileName;
			this->BuildObjectFileName(cache->fObjectFileDirectoryPath, inFileRec.name, objectFileName);

			inArgv.AddItem(strdup("-o"));
			inArgv.AddItem(strdup(objectFileName.String()));
			break;
		}
		
		case kPreprocess:
		{
			// add -e for preprocessing
			inArgv.AddItem(strdup("-e"));
			
			// We want to open up the preprocessor output when we
			// are done.  Unfortunately, we don't know that the user
			// requested kPreprocess when we get to "CompileDone".
			// (which right now is CodeDataSize/GenerateDependencies).
			// The existence of this temporary _i file will trigger
			// us to open it up for the user.
			
			char outputFile[B_FILE_NAME_LENGTH];
			this->BuildIntermediateFileName(inFileRec.name, outputFile, true);
			inArgv.AddItem(strdup("-o"));
			inArgv.AddItem(strdup(outputFile));
			
			break;
		}
		
		case kDisassemble:
		{
			// use kDisassemble to produce a listing file			
			// (see comments above for preprocessor output)
			char outputFile[B_FILE_NAME_LENGTH];
			this->BuildIntermediateFileName(inFileRec.name, outputFile, true);
			inArgv.AddItem(strdup("-l"));
			inArgv.AddItem(strdup(outputFile));
			break;
		}
	}
			
	// add options with user specified text list
	cache->fCommandLineOptions.FillArgvList(inArgv);

	// Finally add the source file to compile
	inArgv.AddItem(strdup(inFileRec.path));

#ifdef _DEBUG_
	fprintf(stderr, "NASMBuilder::BuildCompileArgv...\n");
	for (int i = 0; i < inArgv.CountItems(); i++) {
		char* anArg = (char*) inArgv.ItemAt(i);
		fprintf(stderr, "%s\n", anArg);
	}
	fprintf(stderr, "...NASMBuilder::BuildCompileArgv\n");
#endif

	return B_OK;
}

// ---------------------------------------------------------------------------

status_t 
NASMBuilder::BuildPostLinkArgv(MProject& inProject, BList &inArgv, MFileRec &inFileRec)
{
	// we don't do anything in this stage
	return B_ERROR;
}

// ---------------------------------------------------------------------------

bool
NASMBuilder::FileIsDirty(MProject& inProject, MFileRec& inFileRec, MakeStageT /* inStage */,
						MakeActionT /* inAction */, time_t inModDate)
{
	// Metrowerks takes care of most of the file dirty checking.
	// This method returns true if the object file is older than source file
	NasmSettings* cache = fSettingsMap.GetSettings(inProject);
	BString objectFileName;
	this->BuildObjectFileName(cache->fObjectFileDirectoryPath, inFileRec.name, objectFileName);
		
	// If we can't find the object file directory, go create it again
	BDirectory directory(&cache->fObjectFileDirectory);
	if (directory.InitCheck() != B_OK) {
		this->CacheObjectFileDirectory(inProject, cache);
		directory.SetTo(&cache->fObjectFileDirectory);
	}
	
	BEntry objectFile;
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

const char kNewLine = '\n';

status_t 
NASMBuilder::ParseMessageText(MProject& inProject, const char *inText, BList &outList)
{	
	// put the text into a BString and make sure it ends with a newline
	// for less edge case testing below...
	BString text(inText);
	if (text[text.Length()-1] != kNewLine) {
		text += kNewLine;
	}
	
	// iterate through the text pasing one line at a time to the 
	// build helper
	int32 lineEnd = 0;
	int32 lineStart = 0;
	while ((lineEnd = text.FindFirst(kNewLine, lineStart)) != -1) {
		// skip empty lines
		if (lineEnd > lineStart + 1) {
			BString oneLine;
			text.CopyInto(oneLine, lineStart, lineEnd-lineStart);
			outList.AddItem(this->CreateFileLineError(oneLine));
		}
		lineStart = lineEnd + 1;
	}
	
	return B_OK;
}

// ---------------------------------------------------------------------------

// This isn't defined in Mime.h yet, when it is, we can use that one...
const char* kObjectFileMimeType = "application/x-vnd.Be.ELF-object";

void
NASMBuilder::CodeDataSize(MProject& inProject, 
						  const char* inFilePath, 
						  int32& outCodeSize, 
						  int32& outDataSize)
{
#ifdef _DEBUG_
	fprintf(stderr, "NASMBuilder::CodeDataSize: %s\n", inFilePath);
#endif

	// We don't get a "CompileDone" message like the linker
	// This is the notification we get that the file has been compiled
	// So we do a few extra things here and in NASMBuilder::GenerateDependencies
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
	
	NasmSettings* cache = fSettingsMap.GetSettings(inProject);

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
}

// ---------------------------------------------------------------------------
										
status_t
NASMBuilder::GenerateDependencies(MProject& inProject, const char* inFilePath, BList& outList)
{
	// We do not handle any dependencies in Nasm.
	// However, look for the preprocess output (the existence of which is 
	// my only trigger that they requested preprocess)
	
#ifdef _DEBUG_
	fprintf(stderr, "NASMBuilder::GenerateDependencies\n");
#endif
	// note... rather than nesting 100 levels deep, I return as soon as I get an error

	// inFilePath is a full path, we want just the leaf name
	char* leafNamePart = strrchr(inFilePath, kPathSeparator);
	if (leafNamePart == NULL) {
		return B_ERROR;
	}
	
	// bump leafNamePart past slash
	leafNamePart += 1;
	
	this->OpenIntermediateFile(leafNamePart);
	return B_ERROR;
}

// ---------------------------------------------------------------------------

void
NASMBuilder::GetTargetFilePaths(MProject& inProject, MFileRec& inFileRec, BList& inOutTargetFileList)
{
	// This is probably used to generate the link command line.  There is
	// no documentation for it.
	// The comment in MmwccBuilder.cpp says:
	//   Add the full paths of the target files for this source file to the
	//   targetlist.

#ifdef _DEBUG_
	fprintf(stderr, "NASMBuilder::GetTargetFilePaths called with %s\n", inFileRec.name);
#endif

	// ignore pch files and .h files
	if (inFileRec.makeStage != kIgnoreStage)
	{
		NasmSettings* cache = fSettingsMap.GetSettings(inProject);
		BString objectFileName;
		this->BuildObjectFileName(cache->fObjectFileDirectoryPath, inFileRec.name, objectFileName);
		inOutTargetFileList.AddItem(strdup(objectFileName.String()));
	}
}

// ---------------------------------------------------------------------------

void
NASMBuilder::ProjectChanged(MProject& inProject, ChangeT inChange)
{
#ifdef _DEBUG_
	fprintf(stderr, "NASMBuilder::ProjectChanged\n");
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
			this->SaveProjectSettings(inProject);
			break;

		case kProjectClosed:
			fSettingsMap.RemoveSettings(inProject);
			break;

		case kPrefsChanged:
			this->SaveProjectSettings(inProject);
			break;

		case kBuildStarted:
		case kFilesAdded:
		case kFilesRemoved:
		case kFilesRearranged:
		case kRunMenuItemChanged:
		case kLinkDone:
			// Don't do anything for now
			break;
	}
}

// ---------------------------------------------------------------------------

void
NASMBuilder::CacheObjectFileDirectory(MProject& inProject, NasmSettings* cache)
{
	// Keep the object file directory around so that we don't have
	// to go find it for every file.
	
	// First, if the directory doesn't exist, create it
	// Second, set up our fObjectFileDirectory & fObjectFileDirectoryPath member
	
	// The project allows us to get at the entry_ref for the project
	// use this to get to (or create) a directory at the same level
	// as the project

#ifdef _DEBUG_
	fprintf(stderr, "NASMBuilder::CacheObjectFileDirectory\n");
#endif
	
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

#ifdef _DEBUG_
	fprintf(stderr, "leaving NASMBuilder::CacheObjectFileDirectory - %s\n", fObjectFileDirectoryPath.String());
#endif
}

// ---------------------------------------------------------------------------

void
NASMBuilder::BuildObjectFileName(BString& objectPath, const char* fileName, BString& objectFileName)
{
#ifdef _DEBUG_
	fprintf(stderr, "NASMBuilder::BuildObjectFileName \n");
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
	
#ifdef _DEBUG_
	fprintf(stderr, "leaving NASMBuilder::BuildObjectFileName - %s\n", objectFileName.String());
#endif
}

// ---------------------------------------------------------------------------

void
NASMBuilder::SaveProjectSettings(MProject& inProject)
{
#ifdef _DEBUG_
	fprintf(stderr, "NASMBuilder::SaveCurrentPreferences\n");
#endif

	NasmSettings* cache = fSettingsMap.GetSettings(inProject);

	// Get the current state of all preference panels and access paths
	BMessage message;
	int32 settingLength;
	inProject.GetPrefs(kNASMMessageType, message);
	CommandLineText* commandLine;
	if (message.FindData(kNASMOptionsMessageName, kNASMMessageType,
						 (const void**) &commandLine, &settingLength) == B_OK) {
		cache->fCommandLineOptions = *commandLine;
		cache->fCommandLineOptions.SwapLittleToHost();
	}	

	// keep track of the object file directory for this project
	this->CacheObjectFileDirectory(inProject, cache);
}

// ---------------------------------------------------------------------------

void
NASMBuilder::BuildIntermediateFileName(const char* sourceName, char outputFile[B_FILE_NAME_LENGTH], bool fullPath)
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
NASMBuilder::OpenIntermediateFile(const char* sourceFile)
{
	// See if the intermediate (preprocess or listing) file output exists
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


// ---------------------------------------------------------------------------

ErrorMessage*
NASMBuilder::CreateEmptyError()
{	
	ErrorMessage* error = new ErrorMessage;
	// -1 is a sentinal to trigger the creation of dynamic offsets
	// in MMessageItem.  (With an offset and length, the error message
	// will try to track edits made to the file.)
	error->offset = -1;
	error->length = 0;
	error->synclen = 0;
	error->syncoffset = 0;
	error->errorlength = 0;
	error->erroroffset = 0;
	
	error->filename[0] = 0;
	error->errorMessage[0] = 0;
	error->errorMessage[MAX_ERROR_TEXT_LENGTH-1] = 0;
	
	// the default (until we correctly parse the filename/linenumber) is text only
	error->linenumber = 0;
	error->textonly = true;
	error->isWarning = false;
	
	return error;
}

// ---------------------------------------------------------------------------

ErrorMessage*
NASMBuilder::CreateTextOnlyMessage(const BString& text)
{
	ErrorMessage* error = this->CreateEmptyError();
	int32 amount = min(text.Length(), MAX_ERROR_TEXT_LENGTH-1);
	text.CopyInto(error->errorMessage, 0, amount);
	error->errorMessage[amount] = 0;

	return error;
}

// ---------------------------------------------------------------------------

ErrorMessage*
NASMBuilder::CreateFileLineError(const BString& text)
{
	// Look for the following in the text:
	//	file name : line number : error message
	// If we don't succeed in finding everything, create
	// a text only error message with the full text
	// as the error text
	
	// rather than deeply nesting, I return at text only
	// message at the first problem encountered...
	int32 fileStart = 0;
	int32 fileEnd = text.FindFirst(':', fileStart);
	if (fileEnd == -1) {
		return this->CreateTextOnlyMessage(text);
	}
	
	int32 lineStart = fileEnd + 1;
	int32 lineEnd = text.FindFirst(':', lineStart);
	if (lineEnd == -1) {
		return this->CreateTextOnlyMessage(text);
	}
	
	// we found the file and line
	// set up the file/line in our error message and
	// copy only the remaining text as the message
	ErrorMessage* error = this->CreateEmptyError();

	// handle the file name
	int32 length = min(fileEnd-fileStart, MAX_ERROR_PATH_LENGTH-1);
	text.CopyInto(error->filename, fileStart, length);
	error->filename[length] = 0;
	error->textonly = false;

	// ...the line number
	char lineNumberBuffer[32];
	length = min(lineEnd-lineStart, (int32) 32);
	text.CopyInto(lineNumberBuffer, lineStart, length);
	lineNumberBuffer[length] = 0;
	error->linenumber = atoi(lineNumberBuffer);

	// ...and the error message text
	int32 messageStart = lineEnd+1;
	length = min(text.Length()-messageStart, MAX_ERROR_TEXT_LENGTH-1);
	text.CopyInto(error->errorMessage, messageStart, length);
	error->errorMessage[length] = 0;
	return error;
}
