//	MCompile.h
//	Implementation for handling compiles in the IDE
//	Copyright 1995 Metrowerks Corporation. All rights reserved.
//	Jon Watte

#ifndef _MCOMPILE_H
#define _MCOMPILE_H

#include "BeIDEComm.h"
#include <Locker.h>

class MFileCache;
class MCompileGenerator;
struct ErrorNotificationMessageShort;

class MCompile
{
public:
								MCompile();
	virtual						~MCompile();

	// Actually start the compile. If it returns an error, the compile
	//	is NOT running, but the IsDone() function will think it is.
	virtual	status_t			Run() = 0;

	// This will return FALSE as long as the compile is running
	virtual	bool				IsDone(status_t & status);

	// This will kill the compile, and set the status to SYS_ERROR
	// (This can be overridden, but be sure to call inherited!)
	virtual	status_t			Kill();
	
		
	void						SetInfo(MCompileGenerator* handler, 
										area_id inAreaID, 
										MFileCache* inFileCache)
								{
									fMyHandler = handler;
									fAreaID = inAreaID;
									fFileCache = inFileCache;
								}


protected:
	// This can be overridden, but be sure to call inherited!
	virtual	void				DoStatus(bool objProduced,
										 status_t errorCode);
	virtual	void				DoStatus(const CompilerStatusNotification& inRec);


	void						DoneWithLastFile();

	bool						Lock();
	void						Unlock();
	
	area_id						fAreaID;
	MFileCache*					fFileCache;
	FileHandle					fLastFile;
	MCompileGenerator*			fMyHandler;
	status_t					fCompletionStatus;
	bool						fCompilerDone;

private:
	BLocker						fLock;

};

#endif
