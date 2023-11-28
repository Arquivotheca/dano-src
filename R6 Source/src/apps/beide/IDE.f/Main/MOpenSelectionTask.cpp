//========================================================================
//	MOpenSelectionTask.cpp
//	Copyright 1998 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include "MOpenSelectionTask.h"
#include "IDEApp.h"
#include "MProjectWindow.h"

// ---------------------------------------------------------------------------
//		 MOpenSelectionTask
// ---------------------------------------------------------------------------
//	Constructor

MOpenSelectionTask::MOpenSelectionTask(
	const char *	inFileName,
	bool			inIsInSystemTree,
	entry_ref&		inRef,
	MProjectWindow*	inProject)
	: MThread("openselection"),
	fName(inFileName), 
	fInSystemTree(inIsInSystemTree), 
	fRef(inRef),
	fProject(inProject)
{
}

// ---------------------------------------------------------------------------
//		 ~MOpenSelectionTask
// ---------------------------------------------------------------------------
//	Destructor

MOpenSelectionTask::~MOpenSelectionTask()
{
}

// ---------------------------------------------------------------------------
//		 Execute
// ---------------------------------------------------------------------------

status_t
MOpenSelectionTask::Execute()
{
	IDEApp::BeAPP().OpenSelectionAsync(fName, fInSystemTree, fRef, fProject);

	return B_NO_ERROR;
}

