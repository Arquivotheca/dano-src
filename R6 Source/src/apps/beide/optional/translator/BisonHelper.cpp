// ---------------------------------------------------------------------------
/*
	BisonHelper.cpp
		
	Author:	John R. Dance
			2 March 1999

	A build helper that implements the interface for bison
*/
// ---------------------------------------------------------------------------

#include "BisonHelper.h"
#include <List.h>

// ---------------------------------------------------------------------------

BisonHelper::~BisonHelper()
{
}

// ---------------------------------------------------------------------------

const char*
BisonHelper::GetToolName() const
{
	return "bison";
}

// ---------------------------------------------------------------------------

ulong 
BisonHelper::GetMessageType() const
{
	return 'biso';
}

// ---------------------------------------------------------------------------

void 
BisonHelper::MakeOutputFileName(MProject& inProject, const char *filePath, BString &outputName)
{
	// take the input filePath and strip off the last .l
	// and put on a .cpp
	
	outputName = filePath;
	outputName.ReplaceLast(".y", ".cpp");
}

// ---------------------------------------------------------------------------

bool 
BisonHelper::ValidateSettings(BMessage &inOutMessage)
{
	// we don't have user settable settings right now
	return false;
}

// ---------------------------------------------------------------------------

status_t 
BisonHelper::BuildArgv(MProject& inProject, BList &inArgv, const char *filePath)
{
	// create a duplicated string for each argument
	// here is where we would put in any user specified options

	BString outputName;
	this->MakeOutputFileName(inProject, filePath, outputName);
	inArgv.AddItem(strdup("-dv"));
	inArgv.AddItem(strdup("-o"));
	inArgv.AddItem(strdup(outputName.String()));
	inArgv.AddItem(strdup(filePath));
	return B_OK;
}

// ---------------------------------------------------------------------------

ErrorMessage*
BisonHelper::CreateErrorMessage(const BString& text)
{
	// Look at text and see if we recognize the format.
	// If we do, we can create a file/line message so that
	// the BeIDE can use this error for navigation.  If we don't,
	// then we have to create a text only message.
	// Format of error messages:
	// 	("foo.y", line 123) error: something is wrong   	
	return this->ParseFileLineError(text, "(\"", "\",", "line ", ") error:");
}
