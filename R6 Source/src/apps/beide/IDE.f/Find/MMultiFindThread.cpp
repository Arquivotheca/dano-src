//========================================================================
//	MMultiFindThread.cpp
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include "MMultiFindThread.h"
#include "MFindThing.h"

// ---------------------------------------------------------------------------
//		MMultiFindThread
// ---------------------------------------------------------------------------
//	Constructor

MMultiFindThread::MMultiFindThread(
	MFindThing&		inFinder)
	: MThread("multifind"),
	fFinder(inFinder)
{
}

// ---------------------------------------------------------------------------
//		~MMultiFindThread
// ---------------------------------------------------------------------------
//	Destructor

MMultiFindThread::~MMultiFindThread()
{
}

// ---------------------------------------------------------------------------
//		Execute
// ---------------------------------------------------------------------------
//	Override this virtual function and do whatever it is that the thread
//	object does.

status_t
MMultiFindThread::Execute()
{
	bool	found = fFinder.ExecuteMultiFind(*this);
	
	// important to call with true if there were items found
	// before the point cancelled
	if (this->Cancelled()) {
		found = true;
	}
	
	fFinder.MultiFileFindDone(found);

	return B_NO_ERROR;
}
