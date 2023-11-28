// ---------------------------------------------------------------------------
/*
	MLookupDocumentationTask.h
	
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			14 January 1999

*/
// ---------------------------------------------------------------------------

#ifndef _MLOOKUPDOCUMENTATIONTASK_H
#define _MLOOKUPDOCUMENTATIONTASK_H

#include "MThread.h"
#include <String.h>

class IDEApp;

class MLookupDocumentationTask : public MThread 
{
public:
						MLookupDocumentationTask(IDEApp* theApp, const char* apiName);
						~MLookupDocumentationTask();

	virtual status_t	Execute();

private:
	bool 				CheckBookmarkDirectory();
	
private:
	IDEApp*				fIDEApp;
	BString				fAPIName;
};

#endif
