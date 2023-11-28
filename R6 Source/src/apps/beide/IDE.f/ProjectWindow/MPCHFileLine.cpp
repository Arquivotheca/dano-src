//========================================================================
//	MPCHFileLine.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	Encapsulates a line in the project window that represents a sourcefile
//	in the project.  All of the dependency information is stored in these
//	objects, and they are essentially written out to the project file when
//	it's saved and read back in from the project file when it's opened.
//	BDS

#include <string.h>
#include <time.h>

#include "MPCHFileLine.h"
#include "MSectionLine.h"
#include "MProjectView.h"
#include "MSourceFile.h"
#include "MSourceFileList.h"
#include "MPreCompileObj.h"
#include "MBuildCommander.h"
#include "MProject.h"
#include "IDEConstants.h"
#include "IDEMessages.h"
#include "Utils.h"
#include "MPlugInBuilder.h"

// ---------------------------------------------------------------------------
//		MPCHFileLine
// ---------------------------------------------------------------------------
//	Constructor for existing file.

MPCHFileLine::MPCHFileLine(
	const BEntry&		inFile, 
	bool 				inInSystemTree, 
	MSectionLine& 		inSection,
	MProjectView& 		inProjectView,
	ProjectTargetRec*	inRec,
	const char *		inName)
	:	 MSourceFileLine(
		inFile,
		inInSystemTree,
		MSourceFile::kPrecompiledHeaderKind,
		inSection,
		inProjectView,
		inRec,
		inName)
{
	InitLine();
}

// ---------------------------------------------------------------------------
//		MPCHFileLine
// ---------------------------------------------------------------------------
//	Constructor for existing file.  Read all the info from the blockfile.

MPCHFileLine::MPCHFileLine(
	MSectionLine& 	inSection,
	MProjectView& 	inProjectView,
	MBlockFile&		inFile,
	BlockType		inType,
	uint32			inProjectVersion)
	:	 MSourceFileLine(
		inSection, 
		inProjectView,
		inFile,
		inType,
		inProjectVersion)
{
	InitLine();
}

// ---------------------------------------------------------------------------
//		MPCHFileLine
// ---------------------------------------------------------------------------
//	Destructor

MPCHFileLine::~MPCHFileLine()
{
}

// ---------------------------------------------------------------------------
//		InitLine
// ---------------------------------------------------------------------------

void
MPCHFileLine::InitLine()
{
	fTargetFile = nil;
	SetBlockType(kPrecompileFileBlockType);
	fCodeSize = -1;
	fDataSize = -1;
}

// ---------------------------------------------------------------------------
//		BuildCompileObj
// ---------------------------------------------------------------------------
//	Tell the compiler to compile this source file.

MCompile*
MPCHFileLine::BuildCompileObj(
	MakeActionT		/*inKind*/)
{
	MCompile*		compiler = nil;
	time(&fStartCompileTime);

	compiler = PrecompileSelf();
	ShowCompileMark();
	UpdateLine();

	return compiler;
}

// ---------------------------------------------------------------------------
//		PrecompileSelf
// ---------------------------------------------------------------------------
//	Tell the compiler to Precompile this source file.

MCompile*
MPCHFileLine::PrecompileSelf()
{
	ASSERT(fTarget);
	MCompile*		compiler = nil;

	if (fTarget)
	{
		StringList	argv(50);
		MFileRec	rec;
		FillFileRec(rec);

		status_t		err = fProjectView.BuildPrecompileArgv(fTarget->Builder, argv, rec);

		if (err == B_NO_ERROR)
		{
			bool ideAware = ((fTarget->Builder->builder->Flags() & kIDEAware) != 0);
			PathNameT compilerPath;
			fProjectView.BuildCommander().GetToolPath(fTarget, compilerPath, kPrecompileStage, kPrecompile);

			// Build the precompile object
			compiler = new MPreCompileObj(compilerPath, nil, argv, ideAware, fProjectView, this);
		}
	}

	return compiler;
}

// ---------------------------------------------------------------------------
//		CompileDone
// ---------------------------------------------------------------------------
//	The compiler is done with this file.

void
MPCHFileLine::CompileDone(
	status_t			errCode,
	int32				/*inCodeSize*/,
	int32				/*inDataSize*/,
	MSourceFileList*	inList)
{
	if (errCode == B_NO_ERROR)
	{
		fGoodCompileTime = fStartCompileTime;
		fTouched = false;

		// The compiler deletes the existing precompiled header file and writes a
		// new file so we have to reset the file path so the new copy is refound
		fProjectView.ResetPrecompiledHeaderFile(fSourceFile->GetFileName());

		if (inList)
		{
			fDependencies = *inList;
		}
		
		fProjectView.Window()->PostMessage(cmdSetDirty);
	}

	ShowCompileMark(false);

	BMessage		msg(msgOneCompileDone);

	msg.AddPointer(kProjectLine, this);
	fProjectView.Window()->PostMessage(&msg);
}

// ---------------------------------------------------------------------------
//		NeedsToBeCompiled
// ---------------------------------------------------------------------------
//	Does this file need to be compiled?

bool
MPCHFileLine::NeedsToBeCompiled()
{
	return false;
}

// ---------------------------------------------------------------------------
//		NeedsToBePreCompiled
// ---------------------------------------------------------------------------
//	Does this file need to be precompiled?

bool
MPCHFileLine::NeedsToBePreCompiled()
{
	bool		needsPreCompiling = false;
	time_t		modDate;
	
	if (fTarget && fTarget->Builder && fTarget->Target.Stage == precompileStage)
	{
		needsPreCompiling = fTouched;

		if (! needsPreCompiling)
		{
			modDate = fSourceFile->ModificationTime();
			if (modDate > fGoodCompileTime || DependentFileChanged(fGoodCompileTime))
				needsPreCompiling = true;
		}
		if (! needsPreCompiling)
		{
			MFileRec		rec;
			
			FillFileRec(rec);

			needsPreCompiling = fTarget->Builder->builder->
				FileIsDirty(fProjectView.BuilderProject(), rec, kPrecompileStage, kPrecompile, modDate);
		}

		fTouched = needsPreCompiling;	// So we don't have to check again if the file doesn't compile
		if (fTouched) {
			this->UpdateLine();
		}
	}

	return needsPreCompiling;
}

// ---------------------------------------------------------------------------
//		TargetFileModTime
// ---------------------------------------------------------------------------
//	What is the mod time for the target precompiled header file that matches 
//	this pch file?

time_t 
MPCHFileLine::TargetFileModTime()
{
	time_t		result = 0;

	if (fTargetFile == nil)
	{
		String		name = fSourceFile->GetFileName();
		int32		offset = name.ROffsetOf('.');
		if (offset >= 0)
			name.Replace("", offset, name.GetLength() - offset);
	
		fTargetFile = fProjectView.FindHeaderFile(name, true, false);
		
		if (fTargetFile == nil)
			fTargetFile = fProjectView.FindHeaderFile(name, false, false);
	}
	
	if (fTargetFile)
	{
		result = fTargetFile->ModificationTime();
		if (result == B_ERROR)		// file doesn't exist
			result = 0;
	}

	return result;
}

// ---------------------------------------------------------------------------
//		DeleteObjectFile
// ---------------------------------------------------------------------------
//	Delete the object file for this source file.

void
MPCHFileLine::DeleteObjectFile()
{
}

