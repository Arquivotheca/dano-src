// ---------------------------------------------------------------------------
/*
	FlexHelper.h
		
	Author:	John R. Dance
			1 March 1999

	A build helper that implements the interface for flex
*/
// ---------------------------------------------------------------------------

#ifndef _FLEXHELPER_H
#define _FLEXHELPER_H

#include "BuildHelper.h"

class FlexHelper : public BuildHelper 
{

	virtual					~FlexHelper();
	
	virtual const char*		GetToolName() const;
	virtual ulong			GetMessageType() const;
	virtual	bool			ValidateSettings(BMessage& inOutMessage);
	virtual status_t		BuildArgv(MProject& inProject, BList& inArgv, const char* filePath);
	virtual ErrorMessage*	CreateErrorMessage(const BString& text);
	virtual void			MakeOutputFileName(MProject& inProject, const char* filePath, BString& outputName);
};

#endif
