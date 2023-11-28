/*
	MailDebug.cpp
*/
#include <Autolock.h>
#include <ctype.h>
#include "MailDebug.h"

int32 MailDebug::fIndent = 0;
BLocker MailDebug::fPrintLock;
BDataIO *MailDebug::fDebugOutput = &BOut; // Default output is serial port

MailDebug::MailDebug(const char *className, const char *functionName)
	:	fOutput(MailDebug::fDebugOutput)
{
	BAutolock _lock(fPrintLock);
	// Small hack to get something as close as possible
	// to the class name. Does something like __CLASS__
	// exist?
	
	// This might seem super convoluted, but it get's the job done.
	// __PRETTY_FUNCTION__ returns *way* more then you want, so it's
	// not as easy as just reading from the front...
	
	// Skip to start of function parameters...
	const char *ptr = strstr(className, "(");
	if (ptr != NULL) {
		// Backtrack over the function name...
		while ((ptr != className) && (*ptr != ':'))
			ptr--;
		
		// Backtrack over the scope operator
		while ((ptr != className) && (*ptr == ':'))
			ptr--;
		
		// Backtrack until a whitespace, colon or EOL
		while ((ptr != className) && (*ptr != ':') && !isspace(*ptr))
			ptr--;
		
		// Skip forward over whatever stopped us...
		if ((*ptr == ':') || (isspace(*ptr)))
			ptr++;
		
		// Should be on start of class name...
		while (*ptr != ':')
			fClass << *ptr++;
			
		fName << fClass.String() << "::";
	}
	fName << functionName;
	
	fThread = find_thread(NULL);
	
	// Set the debug output stream based on the class name...
	fOutput = MailDebug::fDebugOutput;
		
	MailDebug::fIndent += 1;
	Indent();
	*fOutput << "Enter " << fName.String() << "\n";
}

MailDebug::~MailDebug()
{
	BAutolock _lock(fPrintLock);
	Indent();
	*fOutput << "Exit  " << fName.String() << "\n";
	MailDebug::fIndent -= 1;
}

void MailDebug::Indent()
{
	BAutolock _lock(fPrintLock);
	for (int32 i = 0; i < MailDebug::fIndent; i++)
		*fOutput << "-";
	*fOutput << "(" << MailDebug::fIndent << "." << fThread << ") ";
}

void MailDebug::Print(const char *format, ...)
{
	va_list list;
	va_start(list, format);
	
	// Ouch... I hope the string is smaller then 16k...
	char *temp = new char[16384];
	
	vsprintf(temp, format, list);
	
	BAutolock _lock(fPrintLock);
	
	Indent();
	*fOutput << temp;
	
	delete [] temp;
}

void MailDebug::SPrint(const char *format, ...)
{
	va_list list;
	va_start(list, format);
	
	// Ouch... I hope the string is smaller then 16k...
	char *temp = new char[16384];
	
	vsprintf(temp, format, list);
	
	BAutolock _lock(fPrintLock);
	
	*fDebugOutput << temp;
	
	delete [] temp;
}

void MailDebug::SetDebugOutput(const char *str)
{
	BAutolock _lock(fPrintLock);

	if (strcasecmp(str, "serial") == 0) {
		fDebugOutput = &BSer;
		
	} else if (strcasecmp(str, "stdout") == 0) {
		fDebugOutput = &BOut;
		
	} else if (strcasecmp(str, "stderr") == 0) {
		fDebugOutput = &BErr;
		
	} else if (strncasecmp(str, "file:", 5) == 0) {
	
	} else {
	
	}
}
