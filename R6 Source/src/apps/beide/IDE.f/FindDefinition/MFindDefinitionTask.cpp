//========================================================================
//	MFindDefinitionTask.cpp
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include "MFindDefinitionTask.h"
#include "MProjectView.h"


// ---------------------------------------------------------------------------
//		MFindDefinitionTask
// ---------------------------------------------------------------------------
//	Constructor

MFindDefinitionTask::MFindDefinitionTask(
	const char *	inToken,
	MProjectView&	inProjectView,
	MTextWindow *	inWindow)
	: MThread("finddefinition"),
	fToken(inToken),
	fProjectView(inProjectView),
	fWindow(inWindow)
{
}

// ---------------------------------------------------------------------------
//		~MFindDefinitionTask
// ---------------------------------------------------------------------------
//	Destructor

MFindDefinitionTask::~MFindDefinitionTask()
{
}

// ---------------------------------------------------------------------------
//		Execute
// ---------------------------------------------------------------------------
//	Override this virtual function and do whatever it is that the thread
//	object does.

status_t
MFindDefinitionTask::Execute()
{
	fProjectView.ExecuteFindDefinition(fToken, fWindow, *this);

	return B_NO_ERROR;
}
