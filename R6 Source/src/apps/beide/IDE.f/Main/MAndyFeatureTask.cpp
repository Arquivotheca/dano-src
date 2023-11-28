//========================================================================
//	MAndyFeatureTask.cpp
//	Copyright 1998 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include "MAndyFeatureTask.h"
#include "IDEApp.h"
#include "MProjectWindow.h"

// ---------------------------------------------------------------------------
//		 MAndyFeatureTask
// ---------------------------------------------------------------------------
//	Constructor

MAndyFeatureTask::MAndyFeatureTask(const char* inFileName,
								   const char** inSuffixArray,
								   bool isSourceFile, 
								   entry_ref& inRef,
								   MProjectWindow* inProject)
				: MThread("andyfeature"),
				  fName(inFileName), 
				  fSuffixArray(inSuffixArray),
				  fRef(inRef),
				  fIsSourceFile(isSourceFile),
				  fProject(inProject)
{
}

// ---------------------------------------------------------------------------
//		 ~MAndyFeatureTask
// ---------------------------------------------------------------------------
//	Destructor

MAndyFeatureTask::~MAndyFeatureTask()
{
}

// ---------------------------------------------------------------------------
//		 Execute
// ---------------------------------------------------------------------------

status_t
MAndyFeatureTask::Execute()
{
	IDEApp::BeAPP().AndyFeatureAsync(fName, fSuffixArray, fIsSourceFile, fRef, fProject);

	return B_NO_ERROR;
}

