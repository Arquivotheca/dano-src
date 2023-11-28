//========================================================================
//	MStartBuildThread.h
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MSTARTBUILDTHREAD_H
#define _MSTARTBUILDTHREAD_H

#include "MThread.h"
#include "MCompileState.h"

class MBuildCommander;

class MStartBuildThread : public MThread 
{
public:
 								MStartBuildThread(
 									MBuildCommander&	inCommander,
									CompileState		inState); 

virtual	status_t				Execute();

private:
	
		MBuildCommander&		fCommander;
		CompileState			fState;
};

#endif
