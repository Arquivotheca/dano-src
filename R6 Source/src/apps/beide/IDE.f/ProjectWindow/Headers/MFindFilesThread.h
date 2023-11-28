//========================================================================
//	MFindFilesThread.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MFINDFILESTHREAD_H
#define _MFINDFILESTHREAD_H

#include "MThread.h"
#include "MFileUtils.h"

class MProjectView;
class MSourceFileList;
class ProgressStatusWindow;

class MFindFilesThread : public MThread 
{
public:
 								MFindFilesThread(
									MProjectView*	inView); 
								~MFindFilesThread();

virtual	status_t				Execute();

	void						FindFilesInDirectory(
									MSourceFileList&	inFileList,
									MSourceFileList&	inSlashFileList,
									BDirectory&			inDir,
									NodeList&			inNodeList,
									bool				inIsRecursive,
									bool				inIsSystem);

private:
	
		MProjectView*			fView;
		bool					fFoundSymLink;
		char					fEntryName[B_FILE_NAME_LENGTH];

		ProgressStatusWindow*	fProgressStatus;
		bigtime_t 				fStartTime;

virtual	void					LastCall();
};

#endif
