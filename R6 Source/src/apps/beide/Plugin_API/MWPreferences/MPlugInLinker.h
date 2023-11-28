//========================================================================
//	MPlugInLinker.h
//	Copyright 1996 - 97 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MPLUGINLINKER_H
#define _MPLUGINLINKER_H

#include "MPlugInBuilder.h"

class MProject;

class MPlugInLinker : public MPlugInBuilder
{
public:

	virtual const char *		TargetName() = 0;
	
	virtual long				BuildLinkArgv(MProject& inProject,
											  BList& inArgv) = 0;

	virtual	status_t			GetExecutableRef(MProject& inProject,
												 entry_ref& outSYMFileRef) = 0;

	virtual	status_t			GetBuiltAppPath(MProject& inProject,
												char* outPath,
												int32 inBufferLength) = 0;

	virtual bool				IsLaunchable(MProject& inProject) = 0;
	
	virtual status_t			Launch(MProject& inProject,
									   bool inRunWithDebugger) = 0;
};

#endif
