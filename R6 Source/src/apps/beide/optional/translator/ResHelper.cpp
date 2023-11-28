// ---------------------------------------------------------------------------
/*
	ResHelper.cpp
		
	Author:	John R. Dance
			2 March 1999

	A build helper that implements the interface for mwbres (or rez)
*/
// ---------------------------------------------------------------------------

#include "ResHelper.h"
#include <List.h>

// ---------------------------------------------------------------------------

ResHelper::~ResHelper()
{
}

// ---------------------------------------------------------------------------

const char*
ResHelper::GetToolName() const
{
	// change this line to "rez" to support the rez tool
	return "mwbres";
}

// ---------------------------------------------------------------------------

ulong 
ResHelper::GetMessageType() const
{
	return 'Rest';
}

// ---------------------------------------------------------------------------

void 
ResHelper::MakeOutputFileName(MProject& inProject, const char *filePath, BString &outputName)
{
	// take the input filePath and strip off the last .l
	// and put on a .cpp
	
	outputName = filePath;
	outputName.ReplaceLast(".r", ".rsrc");
}

// ---------------------------------------------------------------------------

bool 
ResHelper::ValidateSettings(BMessage &inOutMessage)
{
	// we don't have user settable settings right now
	return false;
}

// ---------------------------------------------------------------------------

status_t 
ResHelper::BuildArgv(MProject& inProject, BList &inArgv, const char *filePath)
{
	// create a duplicated string for each argument
	// here is where we would put in any user specified options

	BString outputName;
	this->MakeOutputFileName(inProject, filePath, outputName);
	inArgv.AddItem(strdup("-o"));
	inArgv.AddItem(strdup(outputName.String()));
	inArgv.AddItem(strdup(filePath));
	return B_OK;
}

// ---------------------------------------------------------------------------

ErrorMessage*
ResHelper::CreateErrorMessage(const BString& text)
{
	// Look at text and see if we recognize the format.
	// If we do, we can create a file/line message so that
	// the BeIDE can use this error for navigation.  If we don't,
	// then we have to create a text only message.
	// Format of error messages:
	//	File about.r; Line 3 # Bad data_item to make_data_items 
	return this->ParseFileLineError(text, "File ", "; ", "Line ", " #");
}
