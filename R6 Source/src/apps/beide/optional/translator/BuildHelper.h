// ---------------------------------------------------------------------------
/*
	BuildHelper.cpp
		
	Author:	John R. Dance
			26 February 1999

	A helper for GenericBuilder.  Does a few simple actions for tools
	that take one input file and produce one output file.
	
*/
// ---------------------------------------------------------------------------

#ifndef _BUILDHELPER_H
#define _BUILDHELPER_H

#include <SupportDefs.h>
#include <String.h>

class ErrorMessage;
class MProject;

class BuildHelper
{
public:
	virtual					~BuildHelper();
	
	virtual const char*		GetToolName() const = 0;
	virtual ulong			GetMessageType() const = 0;
	virtual	bool			ValidateSettings(BMessage& inOutMessage) = 0;
	virtual status_t		BuildArgv(MProject& inProject, BList& inArgv, const char* filePath) = 0;
	virtual ErrorMessage*	CreateErrorMessage(const BString& text) = 0;
	virtual void			MakeOutputFileName(MProject& inProject, const char* filePath, BString& outputName) = 0;

protected:
	virtual ErrorMessage*	ParseFileLineError(const BString& text,
											   const char* filePrefix, 
											   const char* fileSuffix, 
											   const char* linePrefix, 
											   const char* lineSuffix);

	virtual ErrorMessage*	CreateEmptyError();
	virtual ErrorMessage*	CreateTextOnlyMessage(const BString& text);
};

#endif
