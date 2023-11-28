// ---------------------------------------------------------------------------
/*
	BuildHelper.cpp
		
	Author:	John R. Dance
			1 March 1999

*/
// ---------------------------------------------------------------------------

#include "BuildHelper.h"
#include "ErrorMessage.h"

#include <SupportDefs.h>
#include <String.h>
#include <stdlib.h>
#include <algobase.h>

// ---------------------------------------------------------------------------

BuildHelper::~BuildHelper()
{
}

// ---------------------------------------------------------------------------

ErrorMessage*
BuildHelper::CreateEmptyError()
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
BuildHelper::CreateTextOnlyMessage(const BString& text)
{
	ErrorMessage* error = this->CreateEmptyError();
	int32 amount = min(text.Length(), MAX_ERROR_TEXT_LENGTH-1);
	text.CopyInto(error->errorMessage, 0, amount);
	error->errorMessage[amount] = 0;

	return error;
}

// ---------------------------------------------------------------------------


ErrorMessage*
BuildHelper::ParseFileLineError(const BString& text,
								const char* filePrefix, 
								const char* fileSuffix, 
								const char* linePrefix, 
								const char* lineSuffix)
{
	// Look for the following in the text:
	//	filePrefix<fileName>fileSuffix
	// 	linePrefix<lineNumber>lineSuffix
	// If we don't succeed in finding everything, create
	// a text only error message with the full text
	// as the error text
	// (lineSuffix can be nil, in which case we just go to the end of the text)
	
	// rather than deeply nesting, I return at text only
	// message at the first problem encountered...
	int32 fileStart = text.FindFirst(filePrefix);
	if (fileStart == -1) {
		return this->CreateTextOnlyMessage(text);
	}
	fileStart += strlen(filePrefix);
	int32 fileEnd = text.FindFirst(fileSuffix, fileStart);
	if (fileEnd == -1) {
		return this->CreateTextOnlyMessage(text);
	}
	
	int32 lineStart = text.FindFirst(linePrefix, fileEnd);
	if (lineStart == -1) {
		return this->CreateTextOnlyMessage(text);
	}
	lineStart += strlen(linePrefix);
	int32 lineEnd = -1;
	if (lineSuffix) {
		lineEnd = text.FindFirst(lineSuffix, lineStart);
	}
	else {
		lineEnd = text.Length();
	}
	
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
	if (lineSuffix) {
		int32 messageStart = lineEnd + strlen(lineSuffix);
		length = min(text.Length()-messageStart, MAX_ERROR_TEXT_LENGTH-1);
		text.CopyInto(error->errorMessage, messageStart, length);
	}
	else {
		// no error message if we don't have a line number suffix
		length = 0;
	}
	error->errorMessage[length] = 0;
	return error;
}
