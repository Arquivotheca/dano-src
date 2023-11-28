//========================================================================
//	MIgnoreFileLine.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include "MIgnoreFileLine.h"
#include "MProjectView.h"
#include "IDEMessages.h"


// ---------------------------------------------------------------------------
//		MIgnoreFileLine
// ---------------------------------------------------------------------------
//	Constructor for existing file.

MIgnoreFileLine::MIgnoreFileLine(
	const BEntry&		inFile, 
	bool 				inInSystemTree, 
	MSectionLine& 		inSection,
	MProjectView& 		inProjectView,
	ProjectTargetRec*	inRec,
	const char *		inName)
	:	 MSourceFileLine(
		inFile,
		inInSystemTree,
		MSourceFile::kHeaderFileKind,
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
MIgnoreFileLine::InitLine()
{
	SetBlockType(kIgnoreFileBlockType);
	fCodeSize = -1;
	fDataSize = -1;
}

// ---------------------------------------------------------------------------
//		MIgnoreFileLine
// ---------------------------------------------------------------------------
//	Constructor for existing file.  Read all the info from the blockfile.

MIgnoreFileLine::MIgnoreFileLine(
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
//		~MIgnoreFileLine
// ---------------------------------------------------------------------------
//	Destructor

MIgnoreFileLine::~MIgnoreFileLine()
{
}

// ---------------------------------------------------------------------------
//		Draw
// ---------------------------------------------------------------------------

void
MIgnoreFileLine::Draw(
	BRect 			inFrame, 
	BRect			inIntersection,
	MProjectView& 	inView)
{
	if (fSourceFile)
	{
		// Draw the file name
		inView.SetFont(be_fixed_font);
		DrawName(inFrame, inIntersection, inView, fSourceFile->GetFileName());
		
		// Draw code and data size
		MProjectLine::Draw(inFrame, inIntersection, inView);
	}
}

// ---------------------------------------------------------------------------
//		DoClick
// ---------------------------------------------------------------------------

bool
MIgnoreFileLine::DoClick(
	BRect		/*inFrame*/,
	BPoint		inPoint,
	bool 		/*inIsSelected*/,
	uint32		inModifiers,
	uint32		inButtons)
{
	ShowPathPopup(inPoint, inModifiers, inButtons);
	
	return false;
}

// ---------------------------------------------------------------------------
//		SelectImmediately
// ---------------------------------------------------------------------------

bool
MIgnoreFileLine::SelectImmediately(
	BRect	/*inFrame*/,
	BPoint	/*inPoint*/,
	bool	/*inIsSelected*/,
	uint32	/*inModifiers*/,
	uint32	/*inButtons*/)
{
	return false;
}

// ---------------------------------------------------------------------------
//		BuildCompileObj
// ---------------------------------------------------------------------------
//	Tell the compiler to compile this source file.

MCompile*
MIgnoreFileLine::BuildCompileObj(
	MakeActionT		inKind)
{
	return MProjectLine::BuildCompileObj(inKind);
}

// ---------------------------------------------------------------------------
//		NeedsToBeCompiled
// ---------------------------------------------------------------------------
//	Does this file need to be compiled?

bool
MIgnoreFileLine::NeedsToBeCompiled()
{
	return false;
}

// ---------------------------------------------------------------------------
//		NeedsToBePreCompiled
// ---------------------------------------------------------------------------
//	Does this file need to be precompiled?

bool
MIgnoreFileLine::NeedsToBePreCompiled()
{
	return false;
}

// ---------------------------------------------------------------------------
//		DeleteObjectFile
// ---------------------------------------------------------------------------
//	Delete the object file for this source file.

void
MIgnoreFileLine::DeleteObjectFile()
{
}
