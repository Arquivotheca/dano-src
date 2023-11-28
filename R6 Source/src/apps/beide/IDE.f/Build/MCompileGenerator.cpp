//========================================================================
//	MCompileGenerator.cpp
//	Copyright 1995 - 96 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
// BDS

#include "MCompileGenerator.h"
#include "MSourceFileLine.h"
#include "MCompile.h"
#include "MFileCache.h"
#include "IDEConstants.h"
#include "IDEMessages.h"

#include <Window.h>
#include <Autolock.h>
#include <Debug.h>

const int32 kFileCacheSize = 1024 * 1024;

// ---------------------------------------------------------------------------
// MCompileGenerator member functions
// ---------------------------------------------------------------------------

MCompileGenerator::MCompileGenerator() 
				  : fLock("compilegenerator")
{
	fCompileThread = B_BAD_THREAD_ID;
	fConcurrentCompiles = -1;
	fNumCompiles = 0;
	fFileList = nil;
	fWindow = nil;
	fCount = 0;
	fNumCompiles = 0;
	fCompileType = kInvalidAction;
	fStopOnErrors = false;
	fCompileErrors = 0;
	fFileCache = nil;
}

// ---------------------------------------------------------------------------

MCompileGenerator::~MCompileGenerator()
{
}

// ---------------------------------------------------------------------------

status_t
MCompileGenerator::StartCompile(int32 inHowMany, 
								BList* inFileList, 
								BWindow* inWindow,
								bool stopOnError, 
								MakeActionT inKind,
								MFileGetter* inFileGetter)	// can  be nil to indicate don't use file cache
{
	status_t err = B_NO_ERROR;

#if DEBUG
printf("StartCompile: inHowMany = %d\n", inHowMany);
#endif

	ASSERT(inHowMany > 0);
	if (inHowMany <= 0)
		inHowMany = 1;

	fNumCompiles = min(inHowMany, inFileList->CountItems());

	fStopOnErrors = stopOnError;
	fCompileErrors = 0;

	//	Manufacture a lock for running compiles
	fConcurrentCompiles = create_sem(fNumCompiles, "concurrent");
	ASSERT(fConcurrentCompiles > B_NO_ERROR);
	if (fConcurrentCompiles < B_NO_ERROR)
		err = fConcurrentCompiles;

	// Spawn the thread that generates all the compile objects
	if (B_NO_ERROR == err)
	{
		fCompileThread = spawn_thread(Compile, "Compile Thread", B_NORMAL_PRIORITY, this);
		ASSERT(fCompileThread >= B_NO_ERROR);
		if (fCompileThread < B_NO_ERROR)
		{
			err = fCompileThread;
			delete_sem(fConcurrentCompiles);
		}

		fFileList = inFileList;
		fWindow = inWindow;
		fCompileType = inKind;
	}
	else
	{
		fNumCompiles = 0;
	}
	
	// Create the file cache
	fFileCache = nil;
	if (inFileGetter != nil)
	{
		fFileCache = new MFileCache(kFileCacheSize, *inFileGetter);
	}

	return err;
}

// ---------------------------------------------------------------------------

void
MCompileGenerator::Run()
{
	ASSERT(fCompileThread >= B_NO_ERROR);
	resume_thread(fCompileThread);
}

// ---------------------------------------------------------------------------

void
MCompileGenerator::Kill()
{
	BAutolock lock(fLock);

	if (fCompileThread > 0)
	{
		kill_thread(fCompileThread);

		for (int32 i = 0; i < fNumCompiles; i++)
		{
			if (fCompileList[i].inUse > 0)	// race condition ????
			{
				MCompile*	compiler = fCompileList[i].compiler;
				compiler->Kill();
				delete compiler;
			}
		}
	}

	if (fConcurrentCompiles > 0)
		delete_sem(fConcurrentCompiles);

	// There is probably a race condition here
	// since we delete the areas while the compiler is still
	// running
	for (int32 i = 0; i < fNumCompiles; i++)
		delete_area(fCompileList[i].id);

	delete fFileCache;

	fFileCache = nil;
	fCount = 0;
	fConcurrentCompiles = 0;
	fCompileThread = -1;
	fNumCompiles = 0;
	fCompileErrors = 0;
}

// ---------------------------------------------------------------------------

status_t
MCompileGenerator::Compile(void* arg)
{
	// Launch compiles for all the projectlines in the filelist
	
	MCompileGenerator* This = (MCompileGenerator*) arg;
	
	This->fCount = This->fFileList->CountItems();
	const int32		size = B_PAGE_SIZE * 10; 

	// Allocate an area for each compiler
	// This needs to allow breathing room between the areas
	// in case they need to expand ????
	for (int32 i = 0; i < This->fNumCompiles; i++)
	{
		void*		address = (void*) B_PAGE_SIZE;

		This->fCompileList[i].id = create_area("compilearea", &address, B_ANY_ADDRESS, size, B_NO_LOCK, B_READ_AREA);
		This->fCompileList[i].inUse = 0;
		This->fCompileList[i].compiler = nil;
	}

	// Start all the compiles
	for (int32 i = 0; i < This->fCount; i++)
	{
		acquire_sem(This->fConcurrentCompiles);
		
		// at this point, we are ready to fire off another compile
		// if the user wants to stop when the first errors are seen then
		// just break out of the compile loop
		// (any other concurrent compiles will finish as normal)
		if (This->fStopOnErrors && This->fCompileErrors > 0) {
			// since we aren't starting a compile
			// release the semaphore we just acquired
			release_sem(This->fConcurrentCompiles);		
			break;
		}
		
		// lock while we start the compile going...
		This->fLock.Lock();
		MSourceFileLine* line;
		CompileRec* rec = This->UnusedCompileRecord();
		status_t err = B_ERROR;

		line = (MSourceFileLine*) This->fFileList->ItemAt(i);
		rec->compiler = line->BuildCompileObj(This->fCompileType);
		if (rec->compiler != nil)
		{
			rec->compiler->SetInfo(This, rec->id, This->fFileCache);
			err = rec->compiler->Run();
			if (err != B_NO_ERROR)
			{
				//	Error!
				ASSERT(!"Error starting compiler");
			}
		}
		
		if (err != B_NO_ERROR)
		{
			line->CompileDone(err);
			delete rec->compiler;
			atomic_add(&rec->inUse, -1);
			release_sem(This->fConcurrentCompiles);		
		}

		This->fLock.Unlock();
	}
	
	// Don't quit until all the compiles are done
	for (int32 i = 0; i < This->fNumCompiles; i++) {
		acquire_sem(This->fConcurrentCompiles);
	}

	// Delete the areas
	for (int32 i = 0; i < This->fNumCompiles; i++) {
		delete_area(This->fCompileList[i].id);
	}
	
	// Clean up
	BAutolock lock(This->fLock);
	
	delete This->fFileCache;
	This->fFileCache = nil;
	delete_sem(This->fConcurrentCompiles);
	This->fConcurrentCompiles = 0;
	This->fCompileThread = -1;
	This->fNumCompiles = 0;
	This->fCompileErrors = 0;

	// Tell the window that we're done
	This->fWindow->PostMessage(msgCompileDone);
	This->fWindow = nil;
	
	return B_NO_ERROR;
}

// ---------------------------------------------------------------------------

void
MCompileGenerator::CompileDone(MCompile* inCompiler, status_t status)
{
	// A compile is complete so the area needs to be marked as free.

	for (int32 i = 0; i < fNumCompiles; i++) {
		if (fCompileList[i].compiler == inCompiler) {
			fCompileList[i].compiler = nil;
			atomic_add(&fCompileList[i].inUse, -1);
			// if the compiler reports an error - keep track of it
			if (status != B_OK) {
				atomic_add(&fCompileErrors, 1);
			}
			release_sem(fConcurrentCompiles);
			break;
		}
	}
}

// ---------------------------------------------------------------------------

CompileRec*
MCompileGenerator::UnusedCompileRecord()
{
	// Return the first free record from the list.
	
	CompileRec* rec = nil;

	for (int32 i = 0; i < fNumCompiles; i++)
	{
		if (atomic_add(&fCompileList[i].inUse, 1) == 0L)
		{
			rec = &fCompileList[i];
			break;
		}
		else
			atomic_add(&fCompileList[i].inUse, -1);
	}
	
	ASSERT(rec != nil);

	return rec;
}

