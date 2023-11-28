// ---------------------------------------------------------------------------
/*
	ErrorParser.cpp
	
	includes classes...
		ErrorParser
		CompilerErrorParser
		LinkerErrorParser
		
	Copyright (c) 1998 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			2 October 1998

	---------------------------------------------------------------------------
	Algorithm Introduction
	---------------------------------------------------------------------------
	
	While gcc errors are wonderfully diverse.  They can be broken down into three categories...
	
	1. Error line (with potential more info lines following)
	2. Introduction line with error line following
	3. Include file introduction, with an error line following
	4. Warnings from the assembler
	5. Warnings/messages from different phases of compilation
	
	Here are examples of the different kinds of errors
		*** 1 ***
		PlugIn.cpp:51: parse error before `const'
	
		*** 2a ***
		GCCLinker.cpp: In method `void GCCLinker::SaveCurrentPreferences()':
		GCCLinker.cpp:374: warning: passing `void **' as argument 3 of `BMessage::FindData(...)' adds cv-quals without intervening `const'
	
		*** 2b (with more info lines) ***
		GCCLinker.cpp: In method `void GCCLinker::SaveCurrentPreferences()':
		GCCLinker.cpp:374: no matching function for call to `BMessage::FindData (...)'
		Message.h:189: candidates are: BMessage::FindData(const char *, type_code, const void **, ssize_t *) const
		Message.h:191:                 BMessage::FindData(const char *, type_code, int32, const void **, ssize_t *) const
	
		*** 2c (with "by the way" lines) ***
		Main.cpp: In function `int main(int, char **)':
		Main.cpp:9: `Stack' undeclared (first use this function)
		Main.cpp:9: (Each undeclared identifier is reported only once
		Main.cpp:9: for each function it appears in.)

		*** 2d (with another style of introduction) ***
		PlugInUtil.h: In function `bool ValidateSetting<WarningSettings>()':
		GCCBuilder.cpp:322:   instantiated from here
		PlugInUtil.h:49: warning: declaration of `message' shadows global declaration
	
		*** 3a ***
		In file included from /boot/home/exp/src/apps/beide/gccPlugin/source/PlugIn.cpp:20:
		PlugIn.h:16: SupportDefss.h: No such file or directory
	
		*** 3b ***
		In file included from /boot/MPlugInPrefsView.h:10,
		                 from /boot/CommandLineTextView.h:20,
		                 from /boot/PlugIn.cpp:21:
		PlugInPreferences.h:9: Rects.h: No such file or directory

		*** 4 ***
		/tmp/cc2nQ04h.s: Assembler messages:
		/tmp/cc2nQ04h.s:161: Warning: translating to `fst %st(1)'
		
		*** 5 ***
		cc1plus: warnings being treated as errors
		cc1plus: Invalid option `-Wbozo' 
		collect: recompiling /boot/home/exp/src/apps/beide/IDE.f/Main/MNewProjectWindow.cpp
		cc1plus: warning: -Wuninitialized is not supported without -O

	---------------------------------------------------------------------------
	Algorithm
	---------------------------------------------------------------------------
		
	We look at each line in the file.
	If it is an introduction (ends in ":" or ","), then the text is just appended to the error message.
	When we find an actual error line
		we get the filename, the line number
		and check if it is a warning
		(all this information is stored in the current error message)
	
	Rather than throwing away text after parsing it, I always add it to the error message.
	In this way, if we get a syntax that I don't know about, the user never loses information.
	(Correction... I throw away the file information from the actual error line since
	keeping it pushes the important error message too fart to the right.)
	
	Since error messages can have multiple lines, I keep the multiple lines that gcc gives me.
	One additional problem is with additional information or ...by the way... type messages.
	I tried skipping errors that were duplicate file + line + type.  But that causes me to
	miss multiple different errors or warnings on one line.  I don't mind the errors as much
	as the warnings since they can ignore them.  For now, I look for the exact text
		(Each undeclared identifier is reported only once
		for each function it appears in.)   

	There is another case where I look for exact text.  Any line with the following text
	will be considered a warning line.  (This is so that the number of errors should be
	correct, but you can still navigate to these other locations.)
		a:	"candidates are:"
		b:	Any line starting with 3 or more spaces (which covers c also)
		c	"   instantiated from here"
		
		
	Really the only parsing is done for the actual error line.
	The syntax I'm looking for is:
		ERRORLINE := <filename>:<line>:["warning:"]<message_text>
	
	(We do need to weed out the warning types #5 above.  These fall out because we
	don't recognize the syntax, so they become warning messages.)
	
	---------------------------------------------------------------------------
	Linker Errors...
	---------------------------------------------------------------------------

	Linker errors are much more simple.
	
	obj.i586/PlugIn.o: In function `MakePlugInView':
	obj.i586/PlugIn.o(.text+0x36): undefined reference to `__builtin_new'
	obj.i586/PlugIn.o(.text+0x81): undefined reference to `__throw'
	obj.i586/PlugIn.o: In function `MakePlugInLinker':
	obj.i586/PlugIn.o(.text+0xd62): undefined reference to `_files'
	obj.i586/PlugIn.o(.text+0xd6d): undefined reference to `fprintf'

	All we need to do is add the first introduction line to the first error,
	and let the other errors stand on their own.
	
*/
// ---------------------------------------------------------------------------

#include "ErrorParser.h"
#include "PlugIn.h"
#include "ErrorMessage.h"
#include "IDEConstants.h"

#include <stdlib.h>
#include <string.h>
#include <List.h>
#include <Debug.h>

// ---------------------------------------------------------------------------
//	Constants used when parsing error messages
// ---------------------------------------------------------------------------

const char kNewLine = EOL_CHAR;
const char kColon = ':';
const char kComma = ',';
const char kPathSeparator = '/';

const char* kIgnoreLineOne = "(Each undeclared identifier is reported only once";
const char* kIgnoreLineTwo = "for each function it appears in.)";

const char* kStandardInputFileName = "{standard input}";

// ---------------------------------------------------------------------------
//	Warning checker class
//	(optimizes warning checking so it goes a little faster)
// ---------------------------------------------------------------------------

static const char kWarningPrefix[]		= " warning:";
static const char kAltWarningPrefix[]	= " Warning:";
static const char kCandidatesAre[]		= " candidates are:";
static const char kMoreCandidatesAre[]	= "   ";
static const char kInstantiatedFrom[]	= "   instantiated from here";
static const short kNumberWarnings = 5;

static const char kErrorPrefix[]		= " error:";
static const short kNumberErrors = 1;

// ---------------------------------------------------------------------------

class Prefix
{
public:
	const char* fString;
	long 		fLength;
	bool		fSkipPrefixInMessage;	// don't repeat things like "warning:"
};

// ---------------------------------------------------------------------------

class PrefixChecker
{
public:	
	PrefixChecker(Prefix (*table));
	bool ContainsPrefix(const char* message, long& skipAmountIfFound);

private:
	Prefix (*fPrefixTable);
};

// ---------------------------------------------------------------------------

PrefixChecker::PrefixChecker(Prefix (*table))
{
	fPrefixTable = table;
}

// ---------------------------------------------------------------------------

bool
PrefixChecker::ContainsPrefix(const char* message, long& skipAmountIfFound)
{
	// Check the message for starting out with any one of the prefixes in our table
	
	bool found = false;
	for (int index = 0; fPrefixTable[index].fString != NULL; index++) {
		if (strncmp(message, fPrefixTable[index].fString, fPrefixTable[index].fLength) == 0) {
			found = true;
			skipAmountIfFound = fPrefixTable[index].fSkipPrefixInMessage ? 
									fPrefixTable[index].fLength :
									0;
			break;
		}
	}

	return found;
}

// ---------------------------------------------------------------------------

class WarningChecker : public PrefixChecker
{
public:
	WarningChecker() : PrefixChecker(fgWarningPrefixes) { }

private:
	static Prefix fgWarningPrefixes[kNumberWarnings]; 
};

Prefix WarningChecker::fgWarningPrefixes[kNumberWarnings] =
{
	{ kWarningPrefix, 		strlen(kWarningPrefix),		true },
	{ kAltWarningPrefix,	strlen(kAltWarningPrefix),	true },
	{ kInstantiatedFrom,	strlen(kInstantiatedFrom),	false },
	{ kCandidatesAre,		strlen(kCandidatesAre),		false },
	{ kMoreCandidatesAre,	strlen(kMoreCandidatesAre),	false }
};

// ---------------------------------------------------------------------------

class ErrorChecker : public PrefixChecker
{
public:
	ErrorChecker() : PrefixChecker(fgErrorPrefixes) { }

private:
	static Prefix fgErrorPrefixes[kNumberErrors]; 
};

Prefix ErrorChecker::fgErrorPrefixes[kNumberErrors] =
{
	{ kErrorPrefix, 		strlen(kErrorPrefix),		true }
};


// ---------------------------------------------------------------------------
//	ErrorParser member functions
// ---------------------------------------------------------------------------

ErrorParser::ErrorParser(const char* inText, BList& outList)
			: fErrorList(outList), fErrorTextBuffer()
{
#ifdef _GCCDEBUG_
	fprintf(stderr, "ErrorParser::ErrorParser\n");
#endif

	fBufferPosition = inText;
	fCurrentLine = NULL;
	fLineLength = 0;
	fErrorMessage = NULL;
}

// ---------------------------------------------------------------------------

ErrorParser::~ErrorParser()
{
	delete [] fCurrentLine;
}

// ---------------------------------------------------------------------------

void
ErrorParser::ParseText()
{
	// Make sure we have some error text to parse
	if (fBufferPosition == NULL || *fBufferPosition == NIL) {
		return;
	}
	this->DoParse();
}

// ---------------------------------------------------------------------------

bool
ErrorParser::GetNextLine()
{
#ifdef _GCCDEBUG_
	fprintf(stderr, "ErrorParser::GetNextLine\n");
#endif

	// delete the old line if we have one
	delete [] fCurrentLine;
	fCurrentLine = NULL;
	fLineLength = 0;

	// skip past the old newline (if any)
	while (*fBufferPosition == kNewLine) {
		fBufferPosition++;
	}
	
	// peek ahead until the next newline or nil	
	const char* ptr = fBufferPosition;
	while (*ptr != NIL && *ptr != kNewLine) {
		ptr++;
		fLineLength++;
	}

	// if we actually have some text, create a current line
	// and move our buffer pointer
	
	if (fLineLength) {
		fCurrentLine = new char[fLineLength+1];
		strncpy(fCurrentLine, fBufferPosition, fLineLength);
		fCurrentLine[fLineLength] = NIL;

		// now move fBufferPosition past this line
		// leaving it sitting on the ending newline
		fBufferPosition += fLineLength;
	}
	
	return fLineLength != 0;
}

// ---------------------------------------------------------------------------

char
ErrorParser::GetLastChar()
{
	return fCurrentLine ? fCurrentLine[fLineLength-1] : 0;
}

// ---------------------------------------------------------------------------

void
ErrorParser::CreateNewErrorMessage()
{	
	fErrorMessage = new ErrorMessage;
	// -1 is a sentinal to trigger the creation of dynamic offsets
	// in MMessageItem.  (With an offset and length, the error message
	// will try to track edits made to the file.)
	fErrorMessage->offset = -1;
	fErrorMessage->length = 0;
	fErrorMessage->synclen = 0;
	fErrorMessage->syncoffset = 0;
	fErrorMessage->errorlength = 0;
	fErrorMessage->erroroffset = 0;
	
	fErrorMessage->textonly = false;
	fErrorMessage->filename[0] = NIL;
	fErrorMessage->errorMessage[0] = NIL;
	fErrorMessage->errorMessage[MAX_ERROR_TEXT_LENGTH-1] = NIL;
	
	// the default (until we correctly parse the filename/linenumber) is text only
	fErrorMessage->linenumber = 0;
	fErrorMessage->textonly = true;
	fErrorMessage->isWarning = false;

	// make sure we are also starting with a fresh error message buffer
	fErrorTextBuffer.Truncate(0, false);
}

// ---------------------------------------------------------------------------

void
ErrorParser::AppendToErrorText(const char* newText)
{
#ifdef _GCCDEBUG_
	fprintf(stderr, "ErrorParser::AppendToErrorText: %s\n", newText);
#endif

	// Just put all our text into our error message BString
	// we move the BString into the actual message when we are ready
	// to post the message
	// (this way we can easily deal with sending all the text and
	// now worry about overflow and edge cases)
	
	// We have something to put into the error message,
	// make sure we have one
	if (fErrorMessage == NULL) {
		this->CreateNewErrorMessage();
	}
	
	// If we are appending to something already there, stick in a newline
	if (fErrorTextBuffer.Length() > 0) {
		fErrorTextBuffer += "\n";
	}
	
	// buffer up the new text until we are ready to post the message
	fErrorTextBuffer += newText;
}

// ---------------------------------------------------------------------------

void
ErrorParser::SetErrorFileName(const char* fileName)
{
	if (fErrorMessage == NULL) {
		this->CreateNewErrorMessage();
	}
	
	// as we set the file name, make sure it isn't {standard input}
	// if so, just ignore it.
	if (strcmp(fileName, kStandardInputFileName) != 0) {
		strncpy(fErrorMessage->filename, fileName, MAX_ERROR_PATH_LENGTH);

		// with a file name, we have more than text...
		fErrorMessage->textonly = false;
	}
}

// ---------------------------------------------------------------------------

void
ErrorParser::SetErrorLineNumber(long lineNumber)
{
	if (fErrorMessage == NULL) {
		this->CreateNewErrorMessage();
	}

	fErrorMessage->linenumber = lineNumber;
}

// ---------------------------------------------------------------------------

void
ErrorParser::SetErrorWarning(bool isWarning)
{
	if (fErrorMessage == NULL) {
		this->CreateNewErrorMessage();
	}

	fErrorMessage->isWarning = isWarning;
}

// ---------------------------------------------------------------------------

void
ErrorParser::PostErrorMessage()
{
	ASSERT(fErrorMessage);
	// Move all the text we have buffered up into the actual
	// error message.  If we overflow, clone the error message
	// and move the remainder into the new one	
	while (true) {
		int32 fullTextLength = fErrorTextBuffer.Length();
		int32 copyAmount = min(MAX_ERROR_TEXT_LENGTH-1, fullTextLength);
		
		// if we can't copy the entire buffer - move back to the last space
		// so the text is readable when broken up
		if (copyAmount < fullTextLength) {
			int32 lastSpace = fErrorTextBuffer.FindLast(' ', copyAmount);
			if (lastSpace > 0) {
				copyAmount = lastSpace;
			}
		}
		
		// now move the most we can into the error message
		fErrorTextBuffer.MoveInto(fErrorMessage->errorMessage, 0, copyAmount);
		fErrorMessage->errorMessage[copyAmount] = NIL;
		
		// finally add our constructed error message to the list
		fErrorList.AddItem(fErrorMessage);
		
		// if after moving text we have some left, create a new error message
		if (fErrorTextBuffer.Length() > 0) {
			// clone the current error message
			ErrorMessage* newMessage = new ErrorMessage;
			*newMessage = *fErrorMessage;
			fErrorMessage = newMessage;
			// give the user a little hint that we are starting in the middle
			fErrorTextBuffer.Prepend("...");
		}
		else {
			// we are done, get out of the loop
			break;
		}
	}		
		
	// force the next insertion into the error message
	// to create a new one
	fErrorMessage = NULL;
}

// ---------------------------------------------------------------------------

const char*
ErrorParser::GetCurrentErrorText()
{
	return fErrorTextBuffer.String();
}

// ---------------------------------------------------------------------------
//	CompilerErrorParser member functions
// ---------------------------------------------------------------------------

CompilerErrorParser::CompilerErrorParser(const char* inText, BList& outList)
					: ErrorParser(inText, outList)
{
}

// ---------------------------------------------------------------------------

void
CompilerErrorParser::DoParse()
{
#ifdef _GCCDEBUG_
	fprintf(stderr, "CompilerErrorParser::DoParse\n");
#endif

	// Loop through each error line
	while (this->GetNextLine()) {
		
		// See if the current line is an "introduction line"
		// if so, just add the text to our error message
		// if not, parse the error message to get file/line
		// and add that error message to the list
		
		char lastChar = this->GetLastChar();
		if (lastChar == kColon || lastChar == kComma) {
			this->AppendToErrorText(fCurrentLine);
		}
		else {
			this->ParseErrorLine();
			if (this->IsThrowAwayLine() == false) {
				this->PostErrorMessage();
			}
			else {
				// if we have a "by the way" message - throw it away
				delete fErrorMessage;
				fErrorMessage = NULL;
			}
		}
	}
}

// ---------------------------------------------------------------------------

void
CompilerErrorParser::ParseErrorLine()
{
#ifdef _GCCDEBUG_
	fprintf(stderr, "CompilerErrorParser::ParseErrorLine\n");
#endif

	// The syntax we are looking at is this:
	// ERRORLINE := <filename>:<line>:[" warning:"]<message_text>
	// Add only the message text to the error message
	// If we don't understand the syntax, we will just print a warning
	// and hope the tool returns a non-zero status so the IDE won't continue
	// through subsequent steps in the make process.

	// find the first colon after the filename
	char* colonPtr = strchr(fCurrentLine, kColon);
	if (colonPtr == NULL) {
		// hey, what happened to our known syntax?
		// Go ahead and add the entire line to the error message
		this->AppendToErrorText(fCurrentLine);
		this->SetErrorWarning(true);
		return;
	}

	// create a string out of the first of the line (which is the file name)
	*colonPtr = NIL;
	char* pathName = fCurrentLine;
	
	// At this point, pathName is our NULL terminated file name
	// colonPtr is pointing to NULL right before line number
	// ...bump it past the colon location
	char* lineNumberString = colonPtr+1;
	
	// Now look for the next colon after the line number
	colonPtr = strchr(lineNumberString, kColon);
	if (colonPtr == NULL) {
		// once again, we are getting syntax we don't recognize
		// append the rest of the line (which we have already chopped up)
		this->AppendToErrorText(fCurrentLine);
		this->AppendToErrorText(lineNumberString);
		this->SetErrorWarning(true);
		return;
	}
	
	*colonPtr = NIL;
	// ...again bump past the colon location
	char* messageString = colonPtr+1;

	long lineNumber = atoi(lineNumberString);
	// quick sanity check on our line number
	// if the line number is <= 0 we have a line format we don't know about
	// ...make it a textonly warning (include all the chopped up pieces)
	if (lineNumber <= 0) {
		this->AppendToErrorText(fCurrentLine);
		// if we have "cc1plus: warning: <text>" then don't output warning again
		if (strncmp(lineNumberString, kWarningPrefix, strlen(kWarningPrefix)-1) != 0) {
			this->AppendToErrorText(lineNumberString);
		}
		this->AppendToErrorText(messageString);
		this->SetErrorWarning(true);
		return;
	}

	// We successfully have a file name and line number, set them both
	this->SetErrorFileName(pathName);
	this->SetErrorLineNumber(lineNumber);

	// Now we need to check for the warning message
	WarningChecker warningCheck;
	ErrorChecker errorCheck;
	long skipAmount = 0;
	if (warningCheck.ContainsPrefix(messageString, skipAmount)) {
		this->SetErrorWarning(true);
		messageString += skipAmount;
	}
	// newer gcc versions put in "error:"
	// strip that out since it doesn't add any new information
	else if (errorCheck.ContainsPrefix(messageString, skipAmount)) {
		messageString += skipAmount;
	}

	// get past space after : for both error and warning cases
	messageString += 1;
	
	// Finally add just the message string to the error message
	this->AppendToErrorText(messageString);
}

// ---------------------------------------------------------------------------

bool
CompilerErrorParser::IsThrowAwayLine()
{
	// Currently to be a throw away line, I have to match kIgnoreLineOne or kIgnoreLineTwo
	// We need to think of something better here
	
	const char* errorText = this->GetCurrentErrorText();
	if (fErrorMessage &&
			strcmp(errorText, kIgnoreLineOne) == 0 ||
			strcmp(errorText, kIgnoreLineTwo) == 0) {
		return true;
	}
	return false;
}

// ---------------------------------------------------------------------------
//	LinkerErrorParser member functions
// ---------------------------------------------------------------------------

LinkerErrorParser::LinkerErrorParser(const char* inText, BList& outList)
				  : ErrorParser(inText, outList)
{
}

// ---------------------------------------------------------------------------

void
LinkerErrorParser::DoParse()
{
#ifdef _GCCDEBUG_
	fprintf(stderr, "LinkerErrorParser::DoParse\n");
#endif

	// Loop through each error line
	while (this->GetNextLine()) {
		
		// See if the current line is an "introduction line"
		// if so, don't post the error message, wait
		// until we see a normal line, and then post the message

		char lastChar = this->GetLastChar();
		if (lastChar == kColon || lastChar == kComma) {
			this->AppendToErrorText(fCurrentLine);
		}
		else {
			this->AppendToErrorText(fCurrentLine);
			this->PostErrorMessage();
		}
	}
}

