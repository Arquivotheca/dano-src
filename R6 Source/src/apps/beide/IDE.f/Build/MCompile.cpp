// ---------------------------------------------------------------------------
/*
	MCompile.cpp
	
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			1 January 1999

	Abstracted from MCompile (which became MCompileTool)
	Copyright 1995 - 96  Metrowerks Corporation. All rights reserved.
	Jon Watte, BS
	
*/
// ---------------------------------------------------------------------------

#include <OS.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <stdlib.h>

#include "MCompile.h"
#include "MCompileGenerator.h"
#include "MFileUtils.h"
#include "MFileCache.h"
#include "MAlert.h"
#include "BeIDEComm.h"
#include "CString.h"
#include "Utils.h"
#include "MMessageWindow.h"

#include <File.h>
#include <Debug.h>

MCompile::MCompile()
		 : fLock("compile")
{
	fCompletionStatus = B_NO_ERROR;
	fCompilerDone = false;
	fAreaID = -1;
	fLastFile.id = -1;
	fFileCache = nil;
	fMyHandler = nil;
}

// ---------------------------------------------------------------------------
//		~MCompile
// ---------------------------------------------------------------------------

MCompile::~MCompile()
{
	// MCompile::Kill can be called multiple times here (since a Derived::Kill
	// will also call MCompile::Kill.)  This is OK since we check fCompilerDone
	// each time...
	
	if (Lock()) {
		DoneWithLastFile();
		
		if (fCompilerDone == false) {
			Kill();
		}	
		Unlock();
	}
}

// ---------------------------------------------------------------------------
//		IsDone
// ---------------------------------------------------------------------------

bool
MCompile::IsDone(
	status_t & status)
{
	status = fCompletionStatus;
	return fCompilerDone;
}

// ---------------------------------------------------------------------------
//		Kill
// ---------------------------------------------------------------------------

status_t
MCompile::Kill()
{
	if (Lock()) {	
		fCompletionStatus = B_ERROR;
		fCompilerDone = TRUE;
	
		DoStatus(false, B_ERROR);
	
		Unlock();
	}

	return B_NO_ERROR;
}

// ---------------------------------------------------------------------------

void
MCompile::DoStatus(
	bool objProduced,
	status_t errorCode)
{
	DoneWithLastFile();
	
	fCompilerDone = TRUE;
	if (objProduced) {
		fCompletionStatus = B_NO_ERROR;
	}
	else if (errorCode) {
		fCompletionStatus = errorCode;
	}
	else {
		fCompletionStatus = B_ERROR;
	}

	// Tell handler (if I have one)
	if (fMyHandler) {
		fMyHandler->CompileDone(this, errorCode);
	}
}

// ---------------------------------------------------------------------------
//		DoStatus
// ---------------------------------------------------------------------------
// Called after each source file line.

void
MCompile::DoStatus(
	const CompilerStatusNotification& /*inRec*/)
{
}

// ---------------------------------------------------------------------------
//		DoneWithLastFile
// ---------------------------------------------------------------------------

void
MCompile::DoneWithLastFile()
{
	if (fLastFile.id != -1 && fFileCache != nil)
	{
		fFileCache->DoneWithFile(fLastFile);
		fLastFile.id = -1;
	}
}

// ---------------------------------------------------------------------------

bool
MCompile::Lock()
{
	return fLock.Lock();
}

// ---------------------------------------------------------------------------

void
MCompile::Unlock()
{
	fLock.Unlock();
}

