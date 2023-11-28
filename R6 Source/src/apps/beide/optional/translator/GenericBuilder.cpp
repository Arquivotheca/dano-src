// ---------------------------------------------------------------------------
/*
	GenericBuilder.cpp
		
	Author:	John R. Dance
			26 February 1999

*/
// ---------------------------------------------------------------------------

#include "GenericBuilder.h"
#include "BuildHelper.h"
#include "ErrorMessage.h"
#include "MProject.h"

#include <String.h>
#include <Entry.h>

#include <stdio.h>

// ---------------------------------------------------------------------------

GenericBuilder::GenericBuilder(BuildHelper* adoptHelper)
{
	fBuildHelper = adoptHelper;
}

// ---------------------------------------------------------------------------

GenericBuilder::~GenericBuilder()
{
	delete fBuildHelper;
}

// ---------------------------------------------------------------------------

status_t 
GenericBuilder::GetToolName(MProject* inProject, char *outName, int32 inBufferLength, MakeStageT inStage, MakeActionT inAction)
{
	strncpy(outName, fBuildHelper->GetToolName(), inBufferLength);
	return B_OK;
}

// ---------------------------------------------------------------------------

const char *
GenericBuilder::LinkerName()
{
	// we don't deal with linking
	return B_EMPTY_STRING;
}

// ---------------------------------------------------------------------------

MakeStageT 
GenericBuilder::MakeStages()
{
	// all our work is done during pre-compile
	return kPrecompileStage;	
}

// ---------------------------------------------------------------------------

MakeActionT 
GenericBuilder::Actions()
{
	return kPrecompile;
}

// ---------------------------------------------------------------------------

PlugInFlagsT 
GenericBuilder::Flags()
{
	// generic tools are not ide aware
	return kNotIDEAware;
}

// ---------------------------------------------------------------------------

ulong 
GenericBuilder::MessageDataType()
{
	return fBuildHelper->GetMessageType();
}

// ---------------------------------------------------------------------------

bool 
GenericBuilder::ValidateSettings(BMessage &inOutMessage)
{
	return fBuildHelper->ValidateSettings(inOutMessage);
}

// ---------------------------------------------------------------------------

status_t 
GenericBuilder::BuildPrecompileArgv(MProject& inProject, BList &inArgv, MFileRec &inFileRec)
{
	if (inFileRec.makeStage != kPrecompileStage) {
		return B_ERROR;
	}

	return fBuildHelper->BuildArgv(inProject, inArgv, inFileRec.path);
}

// ---------------------------------------------------------------------------

status_t 
GenericBuilder::BuildCompileArgv(MProject& inProject, BList &inArgv, MakeActionT inAction, MFileRec &inFileRec)
{
	// we don't do anything in this stage
	return B_ERROR;
}

// ---------------------------------------------------------------------------

status_t 
GenericBuilder::BuildPostLinkArgv(MProject& inProject, BList &inArgv, MFileRec &inFileRec)
{
	// we don't do anything in this stage
	return B_ERROR;
}

// ---------------------------------------------------------------------------

bool 
GenericBuilder::FileIsDirty(MProject& inProject, MFileRec &inFileRec, MakeStageT inStage, MakeActionT inAction, time_t inModDate)
{
	// See if inFile is newer than its output file
	
	BString outputFileName;
	fBuildHelper->MakeOutputFileName(inProject, inFileRec.path, outputFileName);
	
	// assume the best...
	bool isDirty = false;
		
	BEntry outputFile(outputFileName.String());
	if (outputFile.Exists() == false || outputFile.IsFile() == false) {
		// if we can't find the output file or it isn't a file
		// we have to assume the source is dirty
		isDirty = true;
	}
	else {
		time_t outputModDate;
		outputFile.GetModificationTime(&outputModDate);
		if (outputModDate < inModDate) {
			// if the output file is older than our input file
			// it is dirty
			isDirty = true;
		}
	}
	
	return isDirty;	
}

// ---------------------------------------------------------------------------

const char kNewLine = '\n';

status_t 
GenericBuilder::ParseMessageText(MProject& inProject, const char *inText, BList &outList)
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
			ErrorMessage* message = fBuildHelper->CreateErrorMessage(oneLine);
			if (message) {
				outList.AddItem(message);
			}
		}
		lineStart = lineEnd + 1;
	}
	
	return B_OK;
}

// ---------------------------------------------------------------------------

void 
GenericBuilder::CodeDataSize(MProject& inProject, const char *inFilePath, int32 &outCodeSize, int32 &outDataSize)
{
	// we don't create object files in these generic tools,
	// so ignore the code/data sizes
	outCodeSize = -1;
	outDataSize = -1;
}

// ---------------------------------------------------------------------------

status_t 
GenericBuilder::GenerateDependencies(MProject& inProject, const char *inFilePath, BList &outList)
{
	// we are not dependent on anything, our output file is
	// but we aren't called for that one, the compiler is (or whatever tool
	// is used to include it in the build)
	return B_ERROR;
}

// ---------------------------------------------------------------------------

void 
GenericBuilder::GetTargetFilePaths(MProject& inProject, MFileRec &inFileRec, BList &inOutTargetFileList)
{
	// this is used by the linker to generate the list of object files to link
	// not applicable for tools
}

// ---------------------------------------------------------------------------

void 
GenericBuilder::ProjectChanged(MProject& inProject, ChangeT inChange)
{
	switch (inChange)
	{
		case kProjectOpened:
		case kProjectClosed:
		case kPrefsChanged:
		case kFilesAdded:
		case kFilesRemoved:
		case kBuildStarted:
		case kFilesRearranged:
		case kRunMenuItemChanged:
		case kLinkDone:
			// Don't do anything for now
			break;
	}
}
