//========================================================================
//	MOpenSelectionTask.h
//	Copyright 1998 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MOPENSELECTIONTASK_H
#define _MOPENSELECTIONTASK_H

#include "MThread.h"
#include "CString.h"
#include <StorageKit.h>

class MProjectWindow;

class MOpenSelectionTask : public MThread 
{
public:
 								MOpenSelectionTask(
									const char *	inFileName,
									bool			inIsInSystemTree,
									entry_ref&		inRef,
									MProjectWindow*	inProject);
								~MOpenSelectionTask();

virtual	status_t				Execute();


private:
	MProjectWindow*			fProject;
	String					fName;
	bool					fInSystemTree;
	entry_ref				fRef;
};

#endif
