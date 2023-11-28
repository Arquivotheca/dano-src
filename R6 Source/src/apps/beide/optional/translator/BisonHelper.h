// ---------------------------------------------------------------------------
/*
	BisonHelper.h
		
	Author:	John R. Dance
			2 March 1999

	A build helper that implements the interface for bison
*/
// ---------------------------------------------------------------------------

#ifndef _BISONHELPER_H
#define _BISONHELPER_H

#include "BuildHelper.h"

class BisonHelper : public BuildHelper 
{

	virtual					~BisonHelper();
	
	virtual const char*		GetToolName() const;
	virtual ulong			GetMessageType() const;
	virtual	bool			ValidateSettings(BMessage& inOutMessage);
	virtual status_t		BuildArgv(MProject& inProject, BList& inArgv, const char* filePath);
	virtual ErrorMessage*	CreateErrorMessage(const BString& text);
	virtual void			MakeOutputFileName(MProject& inProject, const char* filePath, BString& outputName);
};

#endif
