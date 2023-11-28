//========================================================================
//	MStartBuildThread.cpp
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include "MStartBuildThread.h"
#include "MBuildCommander.h"

// ---------------------------------------------------------------------------
//		MStartBuildThread
// ---------------------------------------------------------------------------
//	Constructor

MStartBuildThread::MStartBuildThread(
	MBuildCommander&	inCommander,
	CompileState		inState)
	: MThread("builder"),
	fCommander(inCommander),
	fState(inState)
{
}

// ---------------------------------------------------------------------------
//		Execute
// ---------------------------------------------------------------------------

status_t
MStartBuildThread::Execute()
{
	fCommander.MakeStepOne(fState);
	
	return B_NO_ERROR;
}
