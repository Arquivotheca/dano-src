// ---------------------------------------------------------------------------
/*
	MSubProjectFileLine.cpp
	
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			12 July 1999

*/
// ---------------------------------------------------------------------------

#include "MSubProjectFileLine.h"
#include "MProjectWindow.h"
#include "MProjectView.h"
#include "MProjectCompiler.h"
#include "IDEMessages.h"
#include "ProjectCommands.h"

#include <Application.h>

// ---------------------------------------------------------------------------
// MSubProjectFileLine member functions
// ---------------------------------------------------------------------------

MSubProjectFileLine::MSubProjectFileLine(const BEntry& inFile, 
										 bool inInSystemTree,
										 MSectionLine& inSection,
										 MProjectView& inProjectView,
										 ProjectTargetRec* inRec,
										 const char* inName)
					: MSourceFileLine(inFile, inInSystemTree, MSourceFile::kSourceFileKind, inSection, inProjectView, inRec, inName)
{
	InitLine();
}

// ---------------------------------------------------------------------------

MSubProjectFileLine::MSubProjectFileLine(MSectionLine& inSection,
										 MProjectView& inProjectView,
										 MBlockFile& inBlockFile,
										 BlockType inBlockType,
										 uint32 inProjectVersion)
					: MSourceFileLine(inSection, inProjectView, inBlockFile, inBlockType, inProjectVersion)
{
	InitLine();
}

// ---------------------------------------------------------------------------

void
MSubProjectFileLine::InitLine()
{
	SetBlockType(kSubProjectFileBlockType);
	fCodeSize = -1;
	fDataSize = -1;
}

// ---------------------------------------------------------------------------

MSubProjectFileLine::~MSubProjectFileLine()
{
}

// ---------------------------------------------------------------------------

bool
MSubProjectFileLine::DoClick(BRect /*inFrame*/, 
							 BPoint inPoint,
							 bool /*inIsSelected*/,
							 uint32 inModifiers,
							 uint32 inButtons)
{
	this->ShowPathPopup(inPoint, inModifiers, inButtons);
	return false;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//		Draw
// ---------------------------------------------------------------------------

void
MSubProjectFileLine::Draw(
	BRect 			inFrame, 
	BRect			inIntersection,
	MProjectView& 	inView)
{
	// Draw any checkmark
	if (fShowCompileMark) {
		inView.SetDrawingMode(B_OP_OVER);
		fProjectView.GetPainter().DrawBitmap(MProjectLinePainter::kCheckMark, inFrame, inView);
		inView.SetDrawingMode(B_OP_COPY);
	}
	else if (fTouched) {
		inView.SetDrawingMode(B_OP_OVER);
		fProjectView.GetPainter().DrawBitmap(MProjectLinePainter::kTouchedMark, inFrame, inView);
		inView.SetDrawingMode(B_OP_COPY);
	}
	
	inView.SetFont(be_bold_font);
	DrawName(inFrame, inIntersection, inView, fSourceFile->GetFileName());
	inView.SetFont(be_fixed_font);
	MProjectLine::Draw(inFrame, inIntersection, inView);
}



// ---------------------------------------------------------------------------

bool
MSubProjectFileLine::NeedsToBeCompiled()
{
	return false;
}

// ---------------------------------------------------------------------------

bool
MSubProjectFileLine::NeedsToBePreCompiled()
{
	// We always need to go do a build of subprojects
	if (fTarget && fTarget->Target.Stage == precompileStage) {
		fTouched = true;
		this->UpdateLine();
	}
	
	return fTouched;
}

// ---------------------------------------------------------------------------

MCompile*
MSubProjectFileLine::BuildCompileObj(MakeActionT /*inKind*/)
{
	ShowCompileMark();
	UpdateLine();

	return new MProjectCompiler(fProjectView, this);
}

// ---------------------------------------------------------------------------

void
MSubProjectFileLine::DeleteObjectFile()
{
	// We don't have any object file to delete...
}

// ---------------------------------------------------------------------------

void
MSubProjectFileLine::RemoveObjects(bool inRemoveAll)
{
	// DeleteObjectFile is called from many places (like removing the line from the view)
	// Meanwhile, RemoveObjects is only called when dealing with cmd_RemoveBinaries
	
	// Pass along the command to the sub project...
	// Open up the sub project
	// Send it a cmd_RemoveBinaries too
	// (rather than indent 100 levels, I return at the first error found)

// WHAA!  I thought this would be a good idea.  That way you could do a "make clean" from the
// top project.  However, there is no way to prohibit cyclic project inclusions and this loops
// forever.  I tried doing it by calling the MProjectView::RemoveObjects directly and keeping track
// of a list of projects visited, but you need to be locked to do the Touch().  Locking created
// a deadlock.  I'm out of ideas right now.
		
//	if (this->FileExists() == false) {
//		return;
//	}
//	
//	entry_ref ref;
//	if (fSourceFile->GetRef(ref) != B_OK) {
//		return;
//	}
//	
//	// the entry_ref is good, open up the window
//	BMessage msg(msgOpenProjectAndReply);
//	BMessage reply;
//	msg.AddRef("refs", &ref);
//	be_app_messenger.SendMessage(&msg, &reply);
//	status_t errCode = B_ERROR;
//	errCode = (status_t) reply.FindInt32("status");
//	if (errCode != B_OK) {
//		return;
//	}
//	
//	// get the actuall MProjectWindow and send it the message
//	MProjectWindow* subProjectWindow = nil;
//	reply.FindPointer(kProjectMID, (void**) &subProjectWindow);
//	if (subProjectWindow == nil) {
//		return;
//	}
//	
//	subProjectWindow->PostMessage(inRemoveAll ? cmd_RemoveBinariesCompact : cmd_RemoveBinaries);
//
//	// might as well touch the file, we know it needs to be recompiled
//	// this way, the user can also see that it needs to be recompiled
//	this->Touch();
//	
//	// Opening the subproject window takes away the focus from where we did the command
//	// bring it back to the top...
//	fProjectView.Window()->Activate();
}

// ---------------------------------------------------------------------------

void
MSubProjectFileLine::CompileDone(
	status_t			errCode,
	int32				/*inCodeSize*/,
	int32				/*inDataSize*/,
	MSourceFileList*	inList)
{
	if (errCode == B_NO_ERROR) {
		fTouched = false;
		fProjectView.Window()->PostMessage(cmdSetDirty);
	}

	ShowCompileMark(false);

	BMessage msg(msgOneCompileDone);

	msg.AddPointer(kProjectLine, this);
	fProjectView.Window()->PostMessage(&msg);
}

