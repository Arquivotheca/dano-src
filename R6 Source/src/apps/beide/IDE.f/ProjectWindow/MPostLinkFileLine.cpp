//========================================================================
//	MPostLinkFileLine.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	Encapsulates a line in the project window that represents a sourcefile
//	in the project.  All of the dependency information is stored in these
//	objects, and they are essentially written out to the project file when
//	it's saved and read back in from the project file when it's opened.
//	BDS

#include <string.h>

#include "MPostLinkFileLine.h"
#include "MSectionLine.h"
#include "MProjectView.h"
#include "MSourceFile.h"
#include "MPopupMenu.h"
#include "MBuildCommander.h"
#include "MCompilerObj.h"
#include "MProject.h"
#include "IDEConstants.h"
#include "IDEMessages.h"
#include "Utils.h"
#include "MPlugInBuilder.h"

// ---------------------------------------------------------------------------
//		MPostLinkFileLine
// ---------------------------------------------------------------------------
//	Constructor for existing file.

MPostLinkFileLine::MPostLinkFileLine(
	const BEntry&		inFile, 
	bool 				inInSystemTree, 
	MSectionLine& 		inSection,
	MProjectView& 		inProjectView,
	ProjectTargetRec*	inRec,
	const char *		inName)
	:	 MSourceFileLine(
		inFile,
		inInSystemTree,
		MSourceFile::kSourceFileKind,
		inSection,
		inProjectView,
		inRec,
		inName)
{
	InitLine();
}

// ---------------------------------------------------------------------------
//		InitLine
// ---------------------------------------------------------------------------

void
MPostLinkFileLine::InitLine()
{
	SetBlockType(kPostLinkFileBlockType);
	fCodeSize = -1;
	fDataSize = -1;
}

// ---------------------------------------------------------------------------
//		MPostLinkFileLine
// ---------------------------------------------------------------------------
//	Constructor for existing file.  Read all the info from the blockfile.

MPostLinkFileLine::MPostLinkFileLine(
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
	SetBlockType(kPostLinkFileBlockType);
}

// ---------------------------------------------------------------------------
//		MPostLinkFileLine
// ---------------------------------------------------------------------------
//	Destructor

MPostLinkFileLine::~MPostLinkFileLine()
{
}

// ---------------------------------------------------------------------------
//		BuildCompileObj
// ---------------------------------------------------------------------------
//	Tell the compiler to compile this source file.

MCompile*
MPostLinkFileLine::BuildCompileObj(
	MakeActionT		inKind)
{
	MCompile*		compiler = nil;

	ASSERT(inKind == kPostLinkExecute || inKind == kCompile);
	if (inKind == kPostLinkExecute || inKind == kCompile)
	{
		fCompileType = inKind;
		compiler = ExecuteSelf();

		ShowCompileMark();
		UpdateLine();
	}

	return compiler;
}

// ---------------------------------------------------------------------------
//		ExecuteSelf
// ---------------------------------------------------------------------------
//	Tell the compiler to compile this source file.

MCompile*
MPostLinkFileLine::ExecuteSelf()
{
	MCompile*		compiler = nil;
	status_t		err = B_NO_ERROR;
	StringList		argv(50);
	PathNameT		compilerPath;

	ASSERT(fTarget != nil);
	if (fTarget != nil)
	{
		MFileRec		rec;
		FillFileRec(rec);

		if (B_NO_ERROR == err)
		{
			err = fProjectView.BuildPostLinkArgv(fTarget->Builder, argv, rec);

			if (B_NO_ERROR == err)
			{
				bool ideAware = ((fTarget->Builder->builder->Flags() & kIDEAware) != 0);
				fProjectView.BuildCommander().GetToolPath(fTarget, compilerPath, kCompileStage, kPostLinkExecute);

				compiler = 
				new MCompilerObj(this, compilerPath, nil, argv, ideAware, fProjectView, kPostLinkExecute);
			}
		}
	}
	
	return compiler;
}

// ---------------------------------------------------------------------------
//		NeedsToBeCompiled
// ---------------------------------------------------------------------------
//	Does this file need to be compiled?

bool
MPostLinkFileLine::NeedsToBeCompiled()
{
	return false;
}

// ---------------------------------------------------------------------------
//		NeedsToBePreCompiled
// ---------------------------------------------------------------------------
//	Does this file need to be precompiled?

bool
MPostLinkFileLine::NeedsToBePreCompiled()
{
	return false;
}

// ---------------------------------------------------------------------------
//		DeleteObjectFile
// ---------------------------------------------------------------------------
//	Delete the object file for this source file.

void
MPostLinkFileLine::DeleteObjectFile()
{
	if (fCodeSize > 0)
		fCodeSize = 0;
	if (fDataSize > 0)
		fDataSize = 0;
	fGoodCompileTime = 0;
}
