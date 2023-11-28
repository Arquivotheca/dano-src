//========================================================================
//	MMultiFindThread.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MMULTIFINDTHREAD_H
#define _MMULTIFINDTHREAD_H

#include "MThread.h"

class MFindThing;

class MMultiFindThread : public MThread 
{
public:
 								MMultiFindThread(
 									MFindThing&		inFinder); 
		virtual					~MMultiFindThread();

virtual	status_t				Execute();

private:

	MFindThing&			fFinder;
};

#endif
