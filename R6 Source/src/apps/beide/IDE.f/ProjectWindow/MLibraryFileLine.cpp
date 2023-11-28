//========================================================================
//	MLibraryFileLine.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
// BDS

#include <string.h>

#include "MLibraryFileLine.h"
#include "MProjectView.h"
#include "IDEConstants.h"
#include "IDEMessages.h"
#include <Beep.h>
#include <File.h>

// ---------------------------------------------------------------------------
//		MLibraryFileLine
// ---------------------------------------------------------------------------
//	Constructor for existing file.

MLibraryFileLine::MLibraryFileLine(
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
	GetCodeDataSize();
	SetBlockType(kLinkFileBlockType);
}

// ---------------------------------------------------------------------------
//		MLibraryFileLine
// ---------------------------------------------------------------------------
//	Constructor for existing file.

MLibraryFileLine::MLibraryFileLine(
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
	GetCodeDataSize();
	SetBlockType(kLinkFileBlockType);
}

// ---------------------------------------------------------------------------
//		MLibraryFileLine
// ---------------------------------------------------------------------------
//	Destructor

MLibraryFileLine::~MLibraryFileLine()
{
}

// ---------------------------------------------------------------------------
//		Draw
// ---------------------------------------------------------------------------

void
MLibraryFileLine::Draw(
	BRect 			inFrame, 
	BRect			inIntersection,
	MProjectView& 	inView)
{
	inView.SetFont(be_bold_font);
	DrawName(inFrame, inIntersection, inView, fSourceFile->GetFileName());
	inView.SetFont(be_fixed_font);
	MProjectLine::Draw(inFrame, inIntersection, inView);
}

// ---------------------------------------------------------------------------
//		DoClick
// ---------------------------------------------------------------------------

bool
MLibraryFileLine::DoClick(
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
//		Invoke
// ---------------------------------------------------------------------------

void
MLibraryFileLine::Invoke()
{
	beep();	// Can't edit a lib file
}

// ---------------------------------------------------------------------------
//		BuildCompileObj
// ---------------------------------------------------------------------------
//	Library files don't do anything at the compile step.
//	Certain library files can be disassembled though.

MCompile*
MLibraryFileLine::BuildCompileObj(
	MakeActionT		inKind)
{
	MCompile*		compiler = nil;

	if (inKind == kDisassemble)
	{
		compiler = MSourceFileLine::BuildCompileObj(inKind);
	}

	return compiler;
}

// ---------------------------------------------------------------------------
//		NeedsToBeCompiled
// ---------------------------------------------------------------------------
//	Library files don't need to be compiled.

bool
MLibraryFileLine::NeedsToBeCompiled()
{
	if (fTouched)
	{
		GetCodeDataSize();
		fTouched = false;

		UpdateLine();
	}

	return false;
}

// ---------------------------------------------------------------------------
//		DeleteObjectFile
// ---------------------------------------------------------------------------
//	Not really.  Just make sure that we get the code and data size again
//	next time that we are asked if we need to be recompiled.

void
MLibraryFileLine::DeleteObjectFile()
{
	fCodeSize = 0;
	fDataSize = 0;
	fTouched = true;
}

// ---------------------------------------------------------------------------
//		GetCodeDataSize
// ---------------------------------------------------------------------------
//	Open the file and read the code and data size from the header.
//	This only works for CW static Lib files.

void
MLibraryFileLine::GetCodeDataSize()
{
	BFile			file(&fSourceFile->Ref(), B_READ_ONLY);
	
	if (B_OK == file.InitCheck() && file.IsReadable())
	{
		LibHeader		header;
		int32			len = file.Read(&header, sizeof(header));
		
		if (len > 0 && header.magicword == LIB_MAGIC_WORD)
		{
			fCodeSize = header.code_size;
			fDataSize = header.data_size;
		}
	}
}
