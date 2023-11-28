// ---------------------------------------------------------------------------
/*
	RezHelper.h
		
	Author:	John R. Dance
			2 March 1999

	A build helper that implements the interface for mwbres (or rez)
*/
// ---------------------------------------------------------------------------

#ifndef _REZHELPER_H
#define _REZSHELPER_H

#include "BuildHelper.h"

class RezHelper : public BuildHelper 
{

	virtual					~RezHelper();
	
	virtual const char*		GetToolName() const;
	virtual ulong			GetMessageType() const;
	virtual	bool			ValidateSettings(BMessage& inOutMessage);
	virtual status_t		BuildArgv(MProject& inProject, BList& inArgv, const char* filePath);
	virtual ErrorMessage*	CreateErrorMessage(const BString& text);
	virtual void			MakeOutputFileName(MProject& inProject, const char* filePath, BString& outputName);
};

#endif
