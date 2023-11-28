//========================================================================
//	MAndyFeatureTask.h
//	Copyright 1998 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MANDYFEATURETASK_H
#define _MANDYFEATURETASK_H

#include "IDEConstants.h"
#include "MThread.h"
#include "CString.h"
#include <StorageKit.h>

class MProjectWindow;

class MAndyFeatureTask : public MThread 
{
public:
 								MAndyFeatureTask(
									const char *	inFileName,
									const char **	inSuffixArray,
									bool			isSourceFile,
									entry_ref&		inRef,
									MProjectWindow*	inProject);
								~MAndyFeatureTask();

virtual	status_t				Execute();


private:

	String					fName;
	const char **			fSuffixArray;
	MProjectWindow*			fProject;
	entry_ref				fRef;
	bool					fIsSourceFile;

};

#endif
