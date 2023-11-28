//========================================================================
//	MPreCompileObj.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include <string.h>

#include "MPreCompileObj.h"
#include "MSourceFileLine.h"


// ---------------------------------------------------------------------------
//		MPreCompileObj
// ---------------------------------------------------------------------------

MPreCompileObj::MPreCompileObj(
	const char * 	compiler,
	const char * 	file,
	BList&			 args,
	bool 			IDEAware,
	MProjectView& 	inProjectView,
	MSourceFileLine* inSourceLine)
	 : MCompilerObj(
	 	inSourceLine,
		compiler,
		file,
		args,
		IDEAware,
		inProjectView,
		kCompile)
{
}

// ---------------------------------------------------------------------------
//		DoStatus
// ---------------------------------------------------------------------------
//	Precompiling is different from compiling in that a file can be precompiled
//	if it doesn't belong to the current project.  The final notification here
//	is different depending on whether the file is associated with a sourcefileLine
//	or not.

void
MPreCompileObj::DoStatus(
	const CompilerStatusNotification& inRec)
{
	if (fSourceFileLine)
		fSourceFileLine->CompileDone(inRec.errorCode, 0, 0, &fHeaderFileList);
	
	fSourceFileCalled = true;
}

