// ---------------------------------------------------------------------------
/*
	FilterHandler.cpp
	
	Copyright (c) 2001 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			18 May 2001


	Support two styles of use, either a direct specification of the filter
	with SetCommand, or asking the user for a command.  Then apply the
	specified (or selected) filter on the text.
	
	QueryUser needs to make sure the window is gone away before it proceeds
	because the BeIDE doesn't like the edited text window to not be active
	when the text transformations are made.
	
*/
// ---------------------------------------------------------------------------

#include "FilterWindow.h"
#include "MTextAddOn.h"

#include <Alert.h>
#include <String.h>
#include <memory>
#include <Path.h>
#include <File.h>
#include <FindDirectory.h>
#include <Message.h>

const char* kFilterStorageName = "_Filter_List";
const char* kCommandName = "cmd";

// ---------------------------------------------------------------------------

FilterHandler::FilterHandler()
{	
	fCurrentCommand = NULL;
}

// ---------------------------------------------------------------------------

FilterHandler::~FilterHandler()
{
	// delete our local storage
	int32 count = fCommandCache.CountItems();
	for (int i = 0; i < count; i++) {
		BString* cmd = (BString*) fCommandCache.ItemAt(i);
		delete cmd;
	}
	fCommandCache.MakeEmpty();
}

// ---------------------------------------------------------------------------

status_t
FilterHandler::GetData(MTextAddOnStorage* storage)
{
	// Read the storage and see if we have a filter list already cached
	BMessage cacheList;
	if (storage->GetData(kFilterStorageName, cacheList) == B_OK) {	
		int32 index = 0;
		const char* command = NULL;
		while ((command = cacheList.FindString(kCommandName, index++)) != NULL) {
			fCommandCache.AddItem(new BString(command));
		}
	}
	else {
		fCommandCache.AddItem(new BString("fold -s"));
		fCommandCache.AddItem(new BString("sort -f"));
		fCommandCache.AddItem(new BString("tr [a-z] [A-Z]"));
		fCommandCache.AddItem(new BString("tr [A-Z] [a-z]"));
	}

	return B_OK;
}

// ---------------------------------------------------------------------------

status_t
FilterHandler::StoreData(MTextAddOnStorage* storage)
{
	// Save our current filter list to the storage
	BMessage cacheList;
	int32 count = fCommandCache.CountItems();
	for (int i = 0; i < count; i++) {
		BString* cmd = (BString*) fCommandCache.ItemAt(i);
		cacheList.AddString(kCommandName, cmd->String());
	}
	return storage->SaveData(kFilterStorageName, cacheList);
}

// ---------------------------------------------------------------------------
					
void
FilterHandler::QueryUser()
{
	// Create a FilterWindow and wait until it is done before continuing

	fCurrentCommand = NULL;
	FilterWindow* filterWindow = new FilterWindow(this, fCommandCache);
	
	// We want to get the thread of the window, but we can't until we show it
	// but what happens if the user quits fast enough that it is gone before
	// we then query for the thread id.  To get around the race condition,
	// make the first show just start it running, and the second show make it
	// visible.
	filterWindow->Hide();
	filterWindow->Show();
	thread_id thread = filterWindow->Thread();
	filterWindow->Show();

	status_t dontCare;
	wait_for_thread(thread, &dontCare);
}

// ---------------------------------------------------------------------------

void
FilterHandler::SetCommand(const BString& command)
{
	// Set the current command and cache it
	// (cache the command only if we don't already have it)
	// The current command is just an index into the cache
	
	fCurrentCommand = NULL;
	int32 count = fCommandCache.CountItems();
	for (int index = 0; index < count; index++) {
		BString* cmd = (BString*) fCommandCache.ItemAt(index);
		if (*cmd == command) {
			fCurrentCommand = cmd;
			break;
		}
	}	

	if (fCurrentCommand == NULL) {
		fCurrentCommand = new BString(command);
		fCommandCache.AddItem(fCurrentCommand);
	}	
}

// ---------------------------------------------------------------------------

void
FilterHandler::SetCommand(const char* command)
{
	this->SetCommand(BString(command));
}

// ---------------------------------------------------------------------------

void
FilterHandler::RemoveCommand(const char* command)
{
	// If we find the command in our cache, remove it
	
	fCurrentCommand = NULL;
	BString removeCommand(command);
	int32 count = fCommandCache.CountItems();
	for (int index = 0; index < count; index++) {
		BString* cmd = (BString*) fCommandCache.ItemAt(index);
		if (*cmd == removeCommand) {
			fCommandCache.RemoveItem(index);
			delete cmd;
			break;
		}
	}	
}

// ---------------------------------------------------------------------------

status_t
FilterHandler::ApplyFilter(MTextAddOn* textProxy)
{
	// Setting up the filter can set it to the empty-string
	// which tells us just to quietly quit and don't worry about doing anything
	
	status_t status = B_OK;
	if (fCurrentCommand != NULL && fCurrentCommand->Length() > 0) {
		try {
			this->DoFilter(textProxy);
			status = B_OK;
		}
		catch (const BString& errorMessage) {
			textProxy->AddError("Problem(s) running external tool from Filter addon...");
			textProxy->AddError(errorMessage.String());
			textProxy->ShowErrors();
			status = B_ERROR;
		}
	}
	
	return status;
}

// ---------------------------------------------------------------------------

const char* kStdinRedirector  = " < ";
const char* kStdoutRedirector = " > ";
const char* kStderrRedirector = " 2> ";

void
FilterHandler::DoFilter(MTextAddOn* textProxy)
{
	// To apply the filter we perform the following steps
	//	1. If there is no selection, use the entire file
	//	2. Copy the file off to a temporary location
	//	3. Apply the filter to the temporary file
	//	3b Check for errors
	//	4. Get the result and paste it back into the selection
	
	
	// step 1 (get current selection)
	long selStart;
	long selEnd;
	textProxy->GetSelection(&selStart, &selEnd);
	if (selStart == selEnd) {
		selStart = 0;
		selEnd = textProxy->TextLength();
	}
	long selectionLength = selEnd-selStart;
	
	// step 2 (copy the file)
	auto_ptr<char> textBuffer(new char[selectionLength]);
	const char* text = textProxy->Text();
	memcpy(textBuffer.get(), text+selStart, selectionLength);
	

	// Set up the input/output/error files to use
	BPath tempDir;
	status_t err = find_directory(B_COMMON_TEMP_DIRECTORY, &tempDir, true);
	if (err != B_OK) {
		throw BString("Can't find temporary directory");
	}	
	char leafName[256];
	int uniqueTime = (int)(system_time()%1000);
	sprintf(leafName, "input_%d", uniqueTime);
	BPath inputPath(tempDir.Path(), leafName);
	sprintf(leafName, "output_%d", uniqueTime);
	BPath outputPath(tempDir.Path(), leafName);
	sprintf(leafName, "error_%d", uniqueTime);
	BPath errorPath(tempDir.Path(), leafName);
	
	BFile inputFile(inputPath.Path(), B_READ_WRITE+B_CREATE_FILE+B_ERASE_FILE);
	if (inputFile.InitCheck() != B_OK) {
		throw BString("Can't create temporary input file for filter.");
	}	
		
	ssize_t written = inputFile.Write(textBuffer.get(), selectionLength);
	if (written != selectionLength) {
		throw BString("Could not write filter input");
	}

	// step 3 (apply the filter)
	BString commandLine(*fCurrentCommand);
	commandLine += kStdinRedirector;
	commandLine += inputPath.Path();
	commandLine += kStdoutRedirector;
	commandLine += outputPath.Path();
	commandLine += kStderrRedirector;
	commandLine += errorPath.Path();
	system(commandLine.String());
	
	// step 3b (check for errors)
	BFile errorFile(errorPath.Path(), B_READ_ONLY);
	off_t errorLength;
	errorFile.GetSize(&errorLength);
	if (errorLength > 0) {
		auto_ptr<char> errorText(new char[errorLength+1]);
		char* errorBuffer = errorText.get();
		errorFile.Read(errorBuffer, errorLength);
		errorBuffer[errorLength] = 0;
		throw BString(errorBuffer);
	}
	
	// step 4 (paste text back)
	BFile outputFile(outputPath.Path(), B_READ_ONLY);
	if (outputFile.InitCheck() != B_OK) {
		throw BString("Can't read filter output file.");
	}
	off_t newLength;
	outputFile.GetSize(&newLength);
	auto_ptr<char> newText(new char[newLength]);
	ssize_t bytesRead = outputFile.Read(newText.get(), newLength);
	if (bytesRead != newLength) {
		throw BString("Can't read the output file.");
	}
	textProxy->Select(selStart, selEnd);
	textProxy->Delete();
	textProxy->Insert(newText.get(), newLength);
	textProxy->Select(selStart, selStart+newLength);
}
