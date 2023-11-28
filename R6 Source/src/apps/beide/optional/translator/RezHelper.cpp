// ---------------------------------------------------------------------------
/*
	RezHelper.cpp
		
	Author:	John R. Dance
			2 March 1999

	A build helper that implements the interface for mwbres (or rez)
*/
// ---------------------------------------------------------------------------

#include "RezHelper.h"
#include <List.h>

// ---------------------------------------------------------------------------

RezHelper::~RezHelper()
{
}

// ---------------------------------------------------------------------------

const char*
RezHelper::GetToolName() const
{
	// change this line to "rez" to support the rez tool
	return "rez";
}

// ---------------------------------------------------------------------------

ulong 
RezHelper::GetMessageType() const
{
	return 'Rezt';
}

// ---------------------------------------------------------------------------

void 
RezHelper::MakeOutputFileName(MProject& inProject, const char *filePath, BString &outputName)
{

	// rez doesn't like full paths to the output (it does the directory concatenation)
	// itself.  So convert filePath into a leaf name, and then...
	// take the input filePath and strip off the last .r
	// and put on a .cpp
	
	char* leafName = strrchr(filePath, '/');
	if (leafName) {
		// bump past the slash
		leafName++;
	}
	else {
		leafName = (char*) filePath;
	}
	
	outputName = leafName;
	outputName.ReplaceLast(".r", ".rsrc");
}

// ---------------------------------------------------------------------------

bool 
RezHelper::ValidateSettings(BMessage &inOutMessage)
{
	// we don't have user settable settings right now
	return false;
}

// ---------------------------------------------------------------------------

status_t 
RezHelper::BuildArgv(MProject& inProject, BList &inArgv, const char *filePath)
{
	// create a duplicated string for each argument
	// here is where we would put in any user specified options

	BString outputName;
	this->MakeOutputFileName(inProject, filePath, outputName);
	inArgv.AddItem(strdup("-o"));
	inArgv.AddItem(strdup(outputName.String()));

	// use -t always until we set up argument handling
	inArgv.AddItem(strdup("-t"));

	// always include the directory of our file
	BString includeDirective("-I");
	
	char* directoryOfFile = strrchr(filePath, '/');
	if (directoryOfFile) {
		includeDirective.Append(filePath, directoryOfFile-filePath);
	}
	else {
		includeDirective.Append(".");
	}
	inArgv.AddItem(strdup(includeDirective.String()));
	
	// now add the file name to compile
	inArgv.AddItem(strdup(filePath));
	return B_OK;
}

// ---------------------------------------------------------------------------

char* kSkipLines[] = {"### Rez Error", "#------------", "#-------------", NULL};

ErrorMessage*
RezHelper::CreateErrorMessage(const BString& text)
{
	// Look at text and see if we recognize the format.
	// If we do, we can create a file/line message so that
	// the BeIDE can use this error for navigation.  If we don't,
	// then we have to create a text only message.
	// Format of Rez messages:
	// ### Rez Error
	// # parse error
	// #------------
	// File "./Resources/RButtons.r"; Line 183
	// #-------------     
	
	bool skipLine = false;
	for (int i = 0; kSkipLines[i] != NULL; i++) {
		if (strcmp(text.String(), kSkipLines[i]) == 0) {
			skipLine = true;
			break;
		}
	}
	
	if (skipLine) {
		return NULL;
	}
	else if (text[0] == '#') {
		return this->CreateTextOnlyMessage(text);
	}
	else {
		return this->ParseFileLineError(text, "File \"", "\"; ", "Line ", NULL);
	}
}
