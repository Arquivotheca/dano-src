// ---------------------------------------------------------------------------
/*
	FlexHelper.cpp
		
	Author:	John R. Dance
			26 February 1999

	A build helper that implements the interface for flex
*/
// ---------------------------------------------------------------------------

#include "FlexHelper.h"
#include <List.h>

// ---------------------------------------------------------------------------

FlexHelper::~FlexHelper()
{
}

// ---------------------------------------------------------------------------

const char*
FlexHelper::GetToolName() const
{
	return "flex";
}

// ---------------------------------------------------------------------------

ulong 
FlexHelper::GetMessageType() const
{
	return 'flex';
}

// ---------------------------------------------------------------------------

void 
FlexHelper::MakeOutputFileName(MProject& inProject, const char *filePath, BString &outputName)
{
	// take the input filePath and strip off the last .l
	// and put on a .cpp
	
	outputName = filePath;
	outputName.ReplaceLast(".l", ".cpp");
}

// ---------------------------------------------------------------------------

bool 
FlexHelper::ValidateSettings(BMessage &inOutMessage)
{
	// we don't have user settable settings right now
	return false;
}

// ---------------------------------------------------------------------------

status_t 
FlexHelper::BuildArgv(MProject& inProject, BList &inArgv, const char *filePath)
{
	// create a duplicated string for each argument
	// flex is funny in that it needs -oOutput rather than -o Output
	// here is where we would put in any user specified options

	BString outputName;
	this->MakeOutputFileName(inProject, filePath, outputName);
	outputName.Prepend("-o");
	inArgv.AddItem(strdup(outputName.String()));
	inArgv.AddItem(strdup(filePath));
	return B_OK;
}

// ---------------------------------------------------------------------------

ErrorMessage*
FlexHelper::CreateErrorMessage(const BString& text)
{
	// Look at text and see if we recognize the format.
	// If we do, we can create a file/line message so that
	// the BeIDE can use this error for navigation.  If we don't,
	// then we have to create a text only message.
	// Format of error messages:
	//		"foo.l", line 123: Something is wrong.
	
	return this->ParseFileLineError(text, "\"", "\",", "line ", ":");
}
