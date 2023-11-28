//========================================================================
//	MFindDefinitionTask.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MFINDDEFINITIONTASK_H
#define _MFINDDEFINITIONTASK_H

#include "MThread.h"
#include "CString.h"

class MProjectView;
class MTextWindow;

class MFindDefinitionTask : public MThread 
{
public:
 								MFindDefinitionTask(
									const char *	inToken,
									MProjectView&	inProjectView,
									MTextWindow *	inWindow); 
		virtual					~MFindDefinitionTask();


protected:

		MProjectView&			fProjectView;
		MTextWindow *			fWindow;
		String					fToken;

virtual	status_t				Execute();
};

#endif
