//========================================================================
//	MCompileGenerator.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MCOMPILEGENERATOR_H
#define _MCOMPILEGENERATOR_H

#include "IDEConstants.h"
#include "MProjectLine.h"

#include <Locker.h>

class MCompile;
class MFileCache;
class MFileGetter;
class BList;
class BWindow;
struct CompileRec
{
	int32		inUse;
	MCompile*	compiler;
	area_id		id;
};


class MCompileGenerator
{
public:
						MCompileGenerator();
						~MCompileGenerator();
				
	status_t			StartCompile(int32 inHowMany, 
									 BList* inFileList,
									 BWindow* inWindow,
									 bool stopOnError,
									 MakeActionT inKind = kCompile,
									 MFileGetter*inFileGetter = nil);

	void				Run();
	void				Kill();
	void				CompileDone(MCompile* inCompiler, status_t status);

private:

	thread_id			fCompileThread;
	sem_id				fConcurrentCompiles;
	int32				fNumCompiles;
	BList*				fFileList;
	BWindow*			fWindow;
	volatile int32		fCount;
	MakeActionT			fCompileType;
	BLocker				fLock;
	CompileRec			fCompileList[kMaxConcurrentCompiles];
	bool				fStopOnErrors;
	int32				fCompileErrors;
	MFileCache*			fFileCache;

	static status_t		Compile(void* arg);
	CompileRec*			UnusedCompileRecord();
};

#endif
