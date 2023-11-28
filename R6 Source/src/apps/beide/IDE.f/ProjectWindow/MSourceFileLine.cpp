//========================================================================
//	MSourceFileLine.cpp
//	Copyright 1995 - 97 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	Encapsulates a line in the project window that represents a sourcefile
//	in the project.  All of the dependency information is stored in these
//	objects, and they are essentially written out to the project file when
//	it's saved and read back in from the project file when it's opened.
//	BDS

#include <string.h>
#include <time.h>

#include <Roster.h>

#include "MSourceFileLine.h"
#include "MSectionLine.h"
#include "MProjectView.h"
#include "MProject.h"
#include "MSourceFile.h"
#include "MCompilerObj.h"
#include "MPreCompileObj.h"
#include "MPopupMenu.h"
#include "MPathPopup.h"
#include "MFunctionParser.h"
#include "MSearchBrowse.h"
#include "MDynamicMenuHandler.h"
#include "MTextWindow.h"
#include "MFormatUtils.h"
#include "MBuildCommander.h"
#include "MainMenus.h"
#include "MAlert.h"
#include "MWEditUtils.h"
#include "IDEConstants.h"
#include "IDEMessages.h"
#include "Utils.h"
#include "MPlugInBuilder.h"

const uint32 kDR9Flag = 0x80000000;
const uint32 kTouchedFlag = 0x00000001;


// ---------------------------------------------------------------------------
//		MSourceFileLine
// ---------------------------------------------------------------------------
//	Constructor for existing file.

MSourceFileLine::MSourceFileLine(
								 const BEntry& inFile, 
								 bool inInSystemTree,
								 MSourceFile::FileKind inKind,
								 MSectionLine& inSection,
								 MProjectView& inProjectView,
								 ProjectTargetRec* inRec,
								 const char* inName)
				: MProjectLine(inSection, inProjectView),
				  fTarget(inRec)
{
	fSourceFile = new MSourceFile(inFile, inInSystemTree, inKind, &inProjectView, inName);
	InitLine();
}

// ---------------------------------------------------------------------------
//		InitLine
// ---------------------------------------------------------------------------

void
MSourceFileLine::InitLine()
{
	fBrowseData = nil;
	fTouched = true;
	fShowCompileMark = false;
	fGoodCompileTime = 0;
	SetBlockType(kCompileFileBlockType);
	UpdateSuffixType();
	fSourceFile->GetMimeType(fMimeType);
}

// ---------------------------------------------------------------------------
//		MSourceFileLine
// ---------------------------------------------------------------------------
//	Constructor for file stored in the project.  Read all the info from the blockfile.

MSourceFileLine::MSourceFileLine(
	MSectionLine& 	inSection,
	MProjectView& 	inProjectView,
	MBlockFile&		inFile,
	BlockType		inBlockType,
	uint32			inProjectVersion)
	:	 MProjectLine(
		inSection,
		inProjectView)
{
	if (inProjectVersion <= kDR8plusProjectVersion)
		BuildOldLine(inFile, inBlockType);
	else
	{
		SourceFileLineBlock		sourceLineBlock;

		inFile.GetBytes(sizeof(sourceLineBlock), &sourceLineBlock);
		sourceLineBlock.SwapBigToHost();		// swap bytes

		fCodeSize = sourceLineBlock.sCodeSize;
		fDataSize = sourceLineBlock.sDataSize;
		fGoodCompileTime = sourceLineBlock.sGoodCompileTime;
		fTouched = ((sourceLineBlock.sFlags & kTouchedFlag) != 0);

		fBrowseData = nil;
		fTarget = nil;
		fShowCompileMark = false;
		SetBlockType(kCompileFileBlockType);

		BlockType			type;
		char				buffer[1024];

		while (B_NO_ERROR == inFile.ScanBlock(type))
		{
			switch (type)
			{
				case kMySourceFileBlockType:
					fSourceFile = new MSourceFile(inFile, &fProjectView);
					break;

				case kSourceFileBlockType:
				{
					SourceFileBlock		sourceBlock;
					BlockType			type1;
			
					// get the block
					inFile.GetBytes(sizeof(sourceBlock), &sourceBlock);
					sourceBlock.SwapBigToHost();

					// get the name
					if (B_NO_ERROR == inFile.ScanBlock(type1))
					{
						switch (type1)
						{
							case kNameBlockType:
								inFile.GetString(buffer, sizeof(buffer));
								break;
							
							default:
								ASSERT(false);
								break;
						}
					
						inFile.DoneBlock(type1);
					} 	
			
					// Find out if a sourceFile object already exists for this file
					MSourceFile*		sourceFile = fProjectView.FindHeaderFile(buffer, sourceBlock);
					ASSERT(sourceFile);
			
					fDependencies.AddItem(sourceFile);
					break;
				}
				
				case kMimeBlockType:
					inFile.GetString(buffer, sizeof(buffer));
					fMimeType = buffer;
					break;

				case kBrowseBlockType:
					fBrowseDataLength = inFile.GetCurBlockSize();
					fBrowseData = new char[fBrowseDataLength];
					inFile.GetBytes(fBrowseDataLength, fBrowseData);
					break;
				
				default:
					ASSERT(false);
					break;
			}
			
			inFile.DoneBlock(type);	//	done with whatever block we got; move to next
		}

		UpdateSuffixType();
	}
}

// ---------------------------------------------------------------------------
//		BuildOldLine
// ---------------------------------------------------------------------------
//	Build a line from a pre-dr9-format project.

void
MSourceFileLine::BuildOldLine(
	MBlockFile&		inFile,
	BlockType		inBlockType)
{
	SourceFileLineBlock		sourceLineBlock;

	switch (inBlockType)
	{
		// Current formats
		case kCompileFileBlockType:
		case kPrecompileFileBlockType:
		case kLinkFileBlockType:
		case kPostLinkFileBlockType:
		case kIgnoreFileBlockType:
			inFile.GetBytes(sizeof(sourceLineBlock), &sourceLineBlock);
			sourceLineBlock.SwapBigToHost();
			break;
		
		// Obsolete formats
		case kFileBlockTypeOld:
		case kPCHFileBlockTypeOld:
		case kLibFileBlockTypeOld:
			inFile.GetBytes(sizeof(SourceFileLineBlockOld), &sourceLineBlock);
			sourceLineBlock.sFileType = 0;
			sourceLineBlock.sFlags = 0;
			sourceLineBlock.sGoodCompileTime = 0;
			sourceLineBlock.SwapBigToHost();
			break;
	}

	fCodeSize = sourceLineBlock.sCodeSize;
	fDataSize = sourceLineBlock.sDataSize;
	fGoodCompileTime = sourceLineBlock.sGoodCompileTime;
	fTouched = ((sourceLineBlock.sFlags & kTouchedFlag) != 0);

	if ((sourceLineBlock.sFlags & kDR9Flag) != 0)
	{
		// DR8plus Format
		// Build a sourceFile object for this source file using the filestream constructor
		fSourceFile = new MSourceFile(inFile, &fProjectView, kDR8plusProjectVersion);
	
		// Build sourceFile Objects for all of our dependency files
		int32		count = sourceLineBlock.sDependencyFileCount;
		
		for (int32 i = 0; i < count; i++)
		{
			SourceFileBlockDR8	sourceBlock;
			char				filename[1024];
	
			inFile.GetBytes(sizeof(sourceBlock), &sourceBlock);
			sourceBlock.SwapBigToHost();
			inFile.GetBytes(sourceBlock.sNameLength, &filename);
	
			// Find out if a sourceFile object already exists for this record ref
			MSourceFile*		sourceFile = fProjectView.FindHeaderFile(filename, sourceBlock.sSystemTree);
			ASSERT(sourceFile);
	
			fDependencies.AddItem(sourceFile);
		}
	}
	else
	{
		// Pre-DR9 format
		// Build a sourceFile object for this source file using the filestream constructor
		fSourceFile = new MSourceFile(inFile, &fProjectView, kDR8ProjectVersion);
	
		// Build sourceFile Objects for all of our dependency files
		int32		count = sourceLineBlock.sDependencyFileCount;
		
		for (int32 i = 0; i < count; i++)
		{
			SourceFileBlockOld		sourceBlock;
	
			inFile.GetBytes(sizeof(sourceBlock), &sourceBlock);
			// don't need to swap bytes
	
			// Find out if a sourceFile object already exists for this record ref
			MSourceFile*		sourceFile = fProjectView.FindHeaderFile(sourceBlock.sName, sourceBlock.sSystemTree);
			ASSERT(sourceFile);
	
			fDependencies.AddItem(sourceFile);
		}
	}
	
	fBrowseData = nil;
	fTarget = nil;
	fShowCompileMark = false;
	UpdateSuffixType();
	SetBlockType(kCompileFileBlockType);

	// Get the browse data if it's there
	if (inFile.GetBlockLeft() > 0)
	{
		BlockType			type;

		if (B_NO_ERROR == inFile.ScanBlock(type))
		{
			switch (type)
			{
				case kBrowseBlockType:
					fBrowseDataLength = inFile.GetCurBlockSize();
					fBrowseData = new char[fBrowseDataLength];
					inFile.GetBytes(fBrowseDataLength, fBrowseData);
					break;
				
				default:
					ASSERT(false);
					break;
			}
		} 
		
		inFile.DoneBlock(type);
	}
}

// ---------------------------------------------------------------------------
//		~MSourceFileLine
// ---------------------------------------------------------------------------
//	Destructor

MSourceFileLine::~MSourceFileLine()
{
	delete [] fBrowseData;
}

// ---------------------------------------------------------------------------
//		SetTargetRec
// ---------------------------------------------------------------------------

void
MSourceFileLine::SetTargetRec(
	MBuildCommander&	inCommander)
{
	const char *	name = fSourceFile->GetFileName();
	const char *	extension = strrchr(name, '.');
	
	if (extension != nil)
		extension++;
	else
		extension = name;
	
	fTarget = inCommander.GetTargetRec(fMimeType, extension);
	if (fTarget == nil)
		ReportNoTarget();
}

// ---------------------------------------------------------------------------
//		SetTarget
// ---------------------------------------------------------------------------

void
MSourceFileLine::SetTarget(
	ProjectTargetRec*	inTarget)
{
	fTarget = inTarget;
	if (fTarget == nil)
		ReportNoTarget();
}

// ---------------------------------------------------------------------------
//		ReportNoTarget
// ---------------------------------------------------------------------------

void
MSourceFileLine::ReportNoTarget()
{
	String		text = "The file "B_UTF8_OPEN_QUOTE;
	text += fSourceFile->GetFileName();
	text += B_UTF8_CLOSE_QUOTE" has no matching target record.\n"
	 			"Check the target records in the Targets preferences panel\n"
	 			"and the mime type of the file.";

	InfoStruct	 	info;

	info.iTextOnly = true;
	strncpy(info.iLineText, text, kLineTextLength);
	info.iLineText[kLineTextLength-1] = '\0';
	
	BMessage		msg(msgAddInfoToMessageWindow);
	
	msg.AddData(kInfoStruct, kInfoType, &info, sizeof(info));
	
	MMessageWindow::GetGeneralMessageWindow()->PostMessage(&msg);
	MMessageWindow::GetGeneralMessageWindow()->PostMessage(msgShowAndActivate);
}

// ---------------------------------------------------------------------------
//		CompileStage
// ---------------------------------------------------------------------------

MakeStageT
MSourceFileLine::CompileStage() const
{
	MakeStageT		stage;
	
	switch (fBlockType)
	{
		case kIgnoreFileBlockType:
			stage = ignoreStage;
			break;

		case kPrecompileFileBlockType:
			stage = precompileStage;
			break;

		case kCompileFileBlockType:
			stage = compileStage;
			break;

		case kLinkFileBlockType:
			stage = linkStage;
			break;

		case kPostLinkFileBlockType:
			stage = postLinkStage;
			break;
		
		case kSubProjectFileBlockType:
			stage = precompileStage;
			break;

		default:
			ASSERT(false);
			break;
	}
	
	return stage;
}

// ---------------------------------------------------------------------------
//		Draw
// ---------------------------------------------------------------------------

void
MSourceFileLine::Draw(
	BRect 			inFrame, 
	BRect			inIntersection,
	MProjectView& 	inView)
{
	if (fSourceFile)
	{
		// Draw the checkmark
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
		
		// Draw the file name
		bool		isText = 0 == strncmp("text/", fMimeType, 5);

		if (isText)
			inView.SetFont(be_fixed_font);
		else
			inView.SetFont(be_bold_font);

		DrawName(inFrame, inIntersection, inView, fSourceFile->GetFileName());
		if (! isText)
		{
			inView.SetFont(be_fixed_font);
		}

		// Draw code and data size
		MProjectLine::Draw(inFrame, inIntersection, inView);
		
		// Draw the arrow button
		fProjectView.GetPainter().DrawBitmap(MProjectLinePainter::kRightArrow, inFrame, inView);
	}
}

// ---------------------------------------------------------------------------
//		DoClick
// ---------------------------------------------------------------------------

bool
MSourceFileLine::DoClick(
	BRect		inFrame,
	BPoint		inPoint,
	bool 		/*inIsSelected*/,
	uint32		inModifiers,
	uint32		inButtons)
{
	BPoint		hit = inPoint;
	hit.y -= inFrame.top;
	
	BRect arrowRect = fProjectView.GetPainter().GetArrowRect();
	if (arrowRect.Contains(hit))
	{
		MPopupMenu headerPopup("header");
		
		BuildPopupMenu(headerPopup);
		headerPopup.SetTargetForItems(fProjectView.Window());
		
		BPoint where = arrowRect.RightTop();
		
		where.x += 3.0;
		where.y += inFrame.top;
		fProjectView.ConvertToScreen(&where);
	
		// Show the popup
		BMenuItem* item = headerPopup.Go(where, true);
	}
	else
		ShowPathPopup(inPoint, inModifiers, inButtons);

	return false;
}

// ---------------------------------------------------------------------------
//		ShowPathPopup
// ---------------------------------------------------------------------------
//	Show the path popup for this file.

void
MSourceFileLine::ShowPathPopup(
	BPoint		inWhere,
	uint32		inModifiers,
	uint32		inButtons)
{
	bool	showit = ((inModifiers & (B_OPTION_KEY | B_CONTROL_KEY)) != 0) ||
		((inButtons & B_SECONDARY_MOUSE_BUTTON) != 0);

	if (showit)
	{
		entry_ref		ref;
		
		if (B_NO_ERROR == fSourceFile->GetLocRef(ref))
		{
			BPopUpMenu		popup("");
			popup.SetFont(be_plain_font);
			MPathMenu*		path = MakeProjectContextMenu(ref, popup, fProjectView.Window());

			fProjectView.ConvertToScreen(&inWhere);
		
			// Show the popup
			BMenuItem*		item = popup.Go(inWhere, true);
			if (item)
			{
				path->OpenItem(item);
			}
		}
	}
}

// ---------------------------------------------------------------------------
//		SelectImmediately
// ---------------------------------------------------------------------------

bool
MSourceFileLine::SelectImmediately(
	BRect	inFrame,
	BPoint	inPoint,
	bool	inIsSelected,
	uint32	inModifiers,
	uint32	inButtons)
{
	inPoint.y -= inFrame.top;
	bool		rightClick = ((inButtons & B_SECONDARY_MOUSE_BUTTON) != 0) ||
			(((inButtons & B_PRIMARY_MOUSE_BUTTON) != 0) && (inModifiers & B_CONTROL_KEY) != 0);

	return fProjectView.GetPainter().PointInArrow(inPoint) || rightClick || 
			(! inIsSelected && ((inModifiers & (B_SHIFT_KEY | B_COMMAND_KEY) == 0)));
}

// ---------------------------------------------------------------------------
//		Invoke
// ---------------------------------------------------------------------------

void
MSourceFileLine::Invoke()
{
	if (FileExists())
	{
		entry_ref		ref;
		
		if (B_NO_ERROR == fSourceFile->GetRef(ref))
		{
			if (fMimeType.GetLength() == 0 || strncmp("text/", fMimeType, 5) == 0) {
				// open it if it is a text file
				BMessage msg(msgOpenSourceFile);
			
				msg.AddRef("refs", &ref);
				msg.AddBool(kIsInProject, true);
				be_app_messenger.SendMessage(&msg);
			}
			else if (strcmp(kProjectMimeType, fMimeType) == 0) {
				// it is a subproject file - open it up through RefsReceived
				BMessage msg(B_REFS_RECEIVED);
				msg.AddRef("refs", &ref);
				be_app_messenger.SendMessage(&msg);
			}
			else {
				// let the roster launch it
				status_t err = be_roster->Launch(&ref);
			}
		}
	}
	else
	{
		MAlert		alert("File not found.");
		alert.Go();
	}
}

// ---------------------------------------------------------------------------
//		UpdateSuffixType
// ---------------------------------------------------------------------------

void
MSourceFileLine::UpdateSuffixType()
{
	fSuffixType = GetSuffixType(fSourceFile->GetFileName());
}

// ---------------------------------------------------------------------------
//		BuildPopupMenu
// ---------------------------------------------------------------------------
//	Build a header popup menu for this source file line.

void
BuildSizeString(String& text, int32 size)
{
	if (size < 0) {
		text += "n/a";
	}
	else {
		text += size;
	}
}


void
MSourceFileLine::BuildPopupMenu(
	MPopupMenu & 	inMenu) const
{
	BMessage*				msg;

	// Add the Touch menu item
	msg = new BMessage(msgTouchFile);
	msg->AddString(kFileName, fSourceFile->GetFileName());
	inMenu.AddItem(new BMenuItem("Touch", msg));
	inMenu.AddSeparatorItem();

	// If we rounded in the project window, show the
	// actual code and data sizes here
	if (fCodeSize >= 99999 || fDataSize >= 99999) {
		String text("Code = ");
		BuildSizeString(text, fCodeSize);
		text += " / Data = ";
		BuildSizeString(text, fDataSize);
		BMenuItem* sizeItem = new BMenuItem(text, NULL);
		inMenu.AddItem(sizeItem);
		sizeItem->SetEnabled(false);
	
		inMenu.AddSeparatorItem();
	}

	// The Dependency list is sorted in a case sensitive way
	// and we need to display the header file list in a case
	// insensensitive way.  We build a duplicate list and 
	// sort it in a case insensitive manner that also sorts
	// the system tree files before the project tree files.  		
	if (! fDependencies.IsEmpty())
	{
		MSourceFile*			sourceFile;
		String					text;
		int32					index = 0;
		MList<MSourceFile*>		list(fDependencies);
		
		list.SortItems(CompareFunction);

		// Build all the menu items and add them to the menu
		while (list.GetNthItem(sourceFile, index++))
		{			
			bool		isInSystemTree = sourceFile->IsInSystemTree();
			
			// Build text for the menu item
			if (isInSystemTree)
				text = "<";
			else
				text = "";
			
			text += sourceFile->GetFileName();
			if (isInSystemTree)
				text += ">";
	
			// Add the menu item to the menu
			entry_ref		ref;
			
			if (B_NO_ERROR == sourceFile->GetRef(ref))
			{
				msg = new BMessage(msgOpenSourceFile);
				msg->AddRef("refs", &ref);
				inMenu.AddItem(new BMenuItem(text, msg));
			}
		}
	}
}

// ---------------------------------------------------------------------------
//		CompareFunction
// ---------------------------------------------------------------------------
//	Comparison function called by BList to compare two sourceFile objects.
//	System tree sorts before nonsystem tree and the string comparison is
//	case insensitive.

int
MSourceFileLine::CompareFunction(
	const void* 	inSourceOne, 
	const void* 	inSourceTwo)
{
	MSourceFile&	sourceOne = ** ((MSourceFile**) inSourceOne);
	MSourceFile&	sourceTwo = ** ((MSourceFile**) inSourceTwo);
	bool			systemTreeOne = sourceOne.IsInSystemTree();
	
	if (systemTreeOne == sourceTwo.IsInSystemTree())
		return CompareStringsNoCase(sourceOne.GetFileName(), sourceTwo.GetFileName());
	else
	if (systemTreeOne)
		return -1;		// sourceOne sorts first
	else
		return 1;		// sourceTwo sorts first
}

// ---------------------------------------------------------------------------
//		BuildCompileObj
// ---------------------------------------------------------------------------
//	Tell the compiler to compile this source file.

MCompile*
MSourceFileLine::BuildCompileObj(
	MakeActionT		inKind)
{
	MCompile*		compiler = nil;

	fCompileType = inKind;
	time(&fStartCompileTime);

	compiler = CompileSelf(inKind);

	ShowCompileMark();
	UpdateLine();

	return compiler;
}

// ---------------------------------------------------------------------------
//		CompileSelf
// ---------------------------------------------------------------------------
//	Tell the compiler to compile this source file.

MCompile*
MSourceFileLine::CompileSelf(
	MakeActionT		inKind)
{
	MCompile*		compiler = nil;
	PathNameT		compilerPath;
	status_t		err = B_NO_ERROR;
	StringList		argv(50);

	if (B_NO_ERROR == err && (fTarget == nil || fTarget->Builder == nil || fTarget->Builder->builder == nil))
		err = B_ERROR;

	if (B_NO_ERROR == err)
	{		
		MFileRec		rec;
		FillFileRec(rec);

		err = fProjectView.BuildCompileArgv(inKind, fTarget->Builder, argv, rec);

		if (B_NO_ERROR == err)
		{
			bool ideAware = ((fTarget->Builder->builder->Flags() & kIDEAware) != 0);
			fProjectView.BuildCommander().GetToolPath(fTarget, compilerPath, kCompileStage, inKind);

			switch (inKind)
			{
				case kCompile:
				case kPreprocess:
				case kCheckSyntax:
				case kDisassemble:
					compiler = new MCompilerObj(this, compilerPath, nil, argv, ideAware, fProjectView, inKind);
					break;
				
				case kPrecompile:
					compiler = new MPreCompileObj(compilerPath, nil, argv, ideAware, fProjectView, this);
					break;
			}
		}
	}

	return compiler;
}

// ---------------------------------------------------------------------------
//		UpdateLine
// ---------------------------------------------------------------------------
//	Tell the projectview to update this line so the compile mark shows.

void
MSourceFileLine::UpdateLine()
{
	BMessage		msg(msgUpdateLine);

	msg.AddPointer(kProjectLine, this);
	fProjectView.Window()->PostMessage(&msg, &fProjectView);
}

// ---------------------------------------------------------------------------
//		CompileDone
// ---------------------------------------------------------------------------
//	The compiler is done with this file.

void
MSourceFileLine::CompileDone(
	status_t			errCode,
	int32				inCodeSize,
	int32				inDataSize,
	MSourceFileList*	inList)
{
	if (fCompileType == kCompile && errCode == B_NO_ERROR)
	{
		fGoodCompileTime = fStartCompileTime;
		fTouched = false;
		fCodeSize = inCodeSize;
		fDataSize = inDataSize;
		fProjectView.Window()->PostMessage(cmdSetDirty);
		
		if (inList)
		{
			fDependencies = *inList;
		}
	}

	ShowCompileMark(false);

	BMessage		msg(msgOneCompileDone);

	msg.AddPointer(kProjectLine, this);
	fProjectView.Window()->PostMessage(&msg);
}

// ---------------------------------------------------------------------------
//		SetBrowseData
// ---------------------------------------------------------------------------
//	BrowseData was generated for this source file so save it.

void
MSourceFileLine::SetBrowseData(
	char*	inBrowseData,
	int32	inDataLength)
{
	if (fCompileType == kCompile)
	{
		delete [] fBrowseData;
		fBrowseData = inBrowseData;
		fBrowseDataLength = inDataLength;
	}
}

// ---------------------------------------------------------------------------
//		ConvertStage
// ---------------------------------------------------------------------------

short
MSourceFileLine::ConvertStage(
	MakeStageT	inStage)
{
	// This needs to change if the two enums change
	ASSERT(postLinkStage == 4);
	const short		result[] = { 0, precompileStage, compileStage, 0, linkStage, 0, 0, 0, postLinkStage };

	return result[inStage];
}

// ---------------------------------------------------------------------------
//		NeedsToBeExecuted
// ---------------------------------------------------------------------------
//	Does this file need to be Executed?

bool
MSourceFileLine::NeedsToBeExecuted(
	MakeStageT	inStage,
	MakeActionT	inAction)
{
	bool		needsCompiling = false;
	time_t		modDate;

	if (fTarget && fTarget->Builder && fTarget->Target.Stage == ConvertStage(inStage))
	{
		needsCompiling = fTouched;

		if (! needsCompiling)
		{
			modDate = fSourceFile->ModificationTime();
			if (modDate > fGoodCompileTime || DependentFileChanged(fGoodCompileTime))
				needsCompiling = true;
		}

		if (! needsCompiling)
		{
			MFileRec		rec;
			
			FillFileRec(rec);

			needsCompiling = fTarget->Builder->builder->
				FileIsDirty(fProjectView.BuilderProject(), rec, inStage, inAction, modDate);
		}

		fTouched = needsCompiling;	// So we don't have to check again if the file doesn't compile
		if (fTouched) {
			this->UpdateLine();
		}
	}

	return needsCompiling;
}

// ---------------------------------------------------------------------------
//		NeedsToBeCompiled
// ---------------------------------------------------------------------------
//	Does this file need to be compiled?

bool
MSourceFileLine::NeedsToBeCompiled()
{
	bool		needsCompiling = false;
	time_t		modDate;

	if (fTarget && fTarget->Builder && fTarget->Target.Stage == compileStage)
	{
		needsCompiling = fTouched;

		if (! needsCompiling)
		{
			modDate = fSourceFile->ModificationTime();
			if (modDate > fGoodCompileTime || DependentFileChanged(fGoodCompileTime))
				needsCompiling = true;
		}

		if (! needsCompiling)
		{
			MFileRec		rec;
			
			FillFileRec(rec);

			needsCompiling = fTarget->Builder->builder->
				FileIsDirty(fProjectView.BuilderProject(), rec, kCompileStage, kCompile, modDate);
		}

		fTouched = needsCompiling;	// So we don't have to check again if the file doesn't compile
		if (fTouched) {
			this->UpdateLine();
		}
	}

	return needsCompiling;
}

// ---------------------------------------------------------------------------
//		NeedsToBePostLinked
// ---------------------------------------------------------------------------
//	Does this file need to be postlinked?

bool
MSourceFileLine::NeedsToBePostLinked()
{
	return false;
}

// ---------------------------------------------------------------------------
//		NeedsToBePreCompiled
// ---------------------------------------------------------------------------
//	Does this file need to be precompiled?

bool
MSourceFileLine::NeedsToBePreCompiled()
{
	return false;
}

// ---------------------------------------------------------------------------
//		CanBeExecuted
// ---------------------------------------------------------------------------

bool
MSourceFileLine::CanBeExecuted() const
{
	return (fTarget != nil && fTarget->Builder != nil && 
		fTarget->Builder->builder != nil);
}

// ---------------------------------------------------------------------------
//		UsesFileCache
// ---------------------------------------------------------------------------

bool
MSourceFileLine::UsesFileCache() const
{
	return (fTarget != nil && fTarget->Builder != nil && 
		fTarget->Builder->builder != nil && (fTarget->Builder->builder->Flags() & kUsesFileCache) != 0);
}

// ---------------------------------------------------------------------------
//		DependentFileChanged
// ---------------------------------------------------------------------------
//	Is one of our dependent files newer than this source file?

bool
MSourceFileLine::DependentFileChanged(
	size_t inDate) const
{
	bool				result = false;
	int32				index = 0;
	MSourceFile*		sourceFile;

	while (fDependencies.GetNthItem(sourceFile, index++))
	{			
		if (! sourceFile->IsUpToDate(inDate))
		{
			result = true;
			break;
		}
	}

	return result;
}

// ---------------------------------------------------------------------------
//		ParseMessageText
// ---------------------------------------------------------------------------

status_t
MSourceFileLine::ParseMessageText(
	const char*	inText,
	BList&		outList)
{
	status_t	err = B_ERROR;
	ASSERT(fTarget);

	if (fTarget != nil && fTarget->Builder != nil && fTarget->Builder->builder != nil)
	{
		err = fTarget->Builder->builder->ParseMessageText(fProjectView.BuilderProject(), inText, outList);
	}
	
	return err;
}

// ---------------------------------------------------------------------------
//		CodeDataSize
// ---------------------------------------------------------------------------

void
MSourceFileLine::CodeDataSize(
	int32&	outCodeSize,
	int32&	outDataSize)
{
	ASSERT(fTarget);
	if (fTarget != nil && fTarget->Builder != nil && fTarget->Builder->builder != nil)
	{
		char		filePath[kPathSize] = { 0 };

		if (B_NO_ERROR <= fSourceFile->GetPath(filePath, kPathSize))
			fTarget->Builder->builder->CodeDataSize(fProjectView.BuilderProject(), filePath, outCodeSize, outDataSize);
	}
}

// ---------------------------------------------------------------------------
//		GenerateDependencies
// ---------------------------------------------------------------------------
//	Generate the dependency list for non-ideaware tools.

void
MSourceFileLine::GenerateDependencies()
{
	ASSERT(fTarget);
	if (fTarget != nil && fTarget->Builder != nil && fTarget->Builder->builder != nil)
	{
		BList		list;
		char		filePath[kPathSize] = { 0 };

		if (B_NO_ERROR == fSourceFile->GetPath(filePath, kPathSize) &&
			B_NO_ERROR == fTarget->Builder->builder->GenerateDependencies(fProjectView.BuilderProject(), filePath, list))
		{
			fDependencies.MakeEmpty();
			
			entry_ref*		ref;
			MSourceFile*	sourceFile;
			FileNameT		name;

			for (int32 i = 0; i < list.CountItems(); i++)
			{
				ref = (entry_ref*) list.ItemAt(i);
				
				if (ref != nil)
				{
					BEntry	file(ref);
	
					if (B_NO_ERROR == file.InitCheck() && B_NO_ERROR == file.GetName(name))
					{
						bool dontCareResult;
						sourceFile = fProjectView.FindHeader(name, false, dontCareResult);
						// In the rare occurance of a full or partial path include file
						// that isn't in the access paths - search again asking 
						// the project to be creative in finding the file
						// (IDEAware compilers do this automatically by passing in
						// the full path to FindHeader)
						if (sourceFile == NULL) {
							BPath filePath;
							if (file.GetPath(&filePath) == B_OK) {
								sourceFile = fProjectView.FindPartialPathHeader(file);
							}
						}
						ASSERT(sourceFile);
						
						if (sourceFile != nil) {
							fDependencies.AddItem(sourceFile);
						}
					}
					
					delete ref;
				}
			}
		}
	}
}

// ---------------------------------------------------------------------------
//		GetFilePath
// ---------------------------------------------------------------------------

void
MSourceFileLine::GetFilePath(char* outFilePath) const
{
	outFilePath[0] = 0;

	fSourceFile->GetPath(outFilePath, kPathSize);
}

// ---------------------------------------------------------------------------
//		RemoveObjects
// ---------------------------------------------------------------------------
//	Delete the browse data and if removing all objects delete
//	the dependency info.  Doesn't delete the object file.

void
MSourceFileLine::RemoveObjects(
	bool	inRemoveAll)
{
	delete [] fBrowseData;
	fBrowseData = nil;
	
	if (inRemoveAll)
	{
		fDependencies.MakeEmpty();
	}
}

// ---------------------------------------------------------------------------
//		DeleteObjectFile
// ---------------------------------------------------------------------------
//	Delete the object file for this source file.

void
MSourceFileLine::DeleteObjectFile()
{
	BList		targets;

	FillTargetFilePathList(targets);

	for (int32 i = 0; i < targets.CountItems(); ++i)
	{
		entry_ref	ref;
		char *		filePath = (char*) targets.ItemAtFast(i);

		if (filePath != nil)	// other invalid values are possible of course
		{
			BEntry		file(filePath);
			file.Remove();
			
			free(filePath);
		}
	}
	
	fCodeSize = 0;
	fDataSize = 0;
	fGoodCompileTime = 0;
	// might as well touch the file, we know it needs to be recompiled
	// this way, the user can also see that it needs to be recompiled
	this->Touch();
}

// ---------------------------------------------------------------------------
//		FillDependencyList
// ---------------------------------------------------------------------------
//	Fill the BList with entry_refs for each dependency file.

void
MSourceFileLine::FillDependencyList(
	BList& inList) const
{
	MSourceFile*	sourceFile;
	int32			i = 0;
	
	while (fDependencies.GetNthItem(sourceFile, i++))
	{
		entry_ref*		ref = new entry_ref;
		if (B_NO_ERROR == sourceFile->GetRef(*ref))
			inList.AddItem(ref);
		else
			delete ref;
	}
}

// ---------------------------------------------------------------------------
//		FillTargetFilePathList
// ---------------------------------------------------------------------------
//	Ask the builder to fill the BList with fullpaths for all the
//	target files that are generated when this file is executed.
//	For C and C++ there is only a single target file for each
//	source file but for Jave it is possible for a single .java
//	file to generate multiple .class files if there are multiple
//	classes inside the .java file.

void
MSourceFileLine::FillTargetFilePathList(
	BList& inList) const
{
	if (fTarget != nil && fTarget->Builder != nil && fTarget->Builder->builder != nil)
	{
		MFileRec		rec;
		
		FillFileRec(rec);
		fTarget->Builder->builder->GetTargetFilePaths(fProjectView.BuilderProject(), rec, inList);
	}
}

// ---------------------------------------------------------------------------
//		FillFileRec
// ---------------------------------------------------------------------------

void
MSourceFileLine::FillFileRec(
	MFileRec&	outRec) const
{
	outRec.fileType = 0;
	outRec.makeStage = CompileStage();
	outRec.hasResources = HasResources();
	outRec.fileID = FileID();

	if (B_NO_ERROR <= fSourceFile->GetPath(outRec.path, sizeof(outRec.path)))
	{
		int32	len = strlen(outRec.path);

		outRec.name = &outRec.path[len - fSourceFile->NameLength()];
	}
	else
	{
		// punt
		outRec.path[0] = '\0';
		outRec.name = &outRec.path[0];
	}
}

// ---------------------------------------------------------------------------
//		HasResources
// ---------------------------------------------------------------------------

bool
MSourceFileLine::HasResources() const
{
	return (fTarget != nil && TargetHasResources(fTarget->Target.Flags));
}

// ---------------------------------------------------------------------------
//		Touch
// ---------------------------------------------------------------------------
//	Make sure that the file is recompiled.  

void
MSourceFileLine::Touch()
{
	fTouched = true;
	this->UpdateLine();
}

// ---------------------------------------------------------------------------
//		UnTouch
// ---------------------------------------------------------------------------
//	Need to untouch after a successful compile or precompile.

void
MSourceFileLine::UnTouch()
{
	fTouched = false;
	this->UpdateLine();
}

// ---------------------------------------------------------------------------
//		NeedsResourcesToBeCopied
// ---------------------------------------------------------------------------
//	Does this file need to copy resources to the executable.

bool
MSourceFileLine::NeedsResourcesToBeCopied()
{
	bool	result = false;
	
	if (HasResources())
	{
		result = fTouched;
		
		if (! result)
		{
			time_t		modTime = fSourceFile->ModificationTime();

			result = (modTime != -1) && (modTime > fGoodCompileTime);
		}
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//		ResourcesWereCopied
// ---------------------------------------------------------------------------
//	Does this file need to copy resources to the executable.

void
MSourceFileLine::ResourcesWereCopied()
{
	fTouched = false;
	time(&fGoodCompileTime);
}

// ---------------------------------------------------------------------------
//		UpdateType
// ---------------------------------------------------------------------------
//	See if the type we have stored matches what's on disk.  return true if
//	the type changed.

typedef char	path_t[1024];

bool
MSourceFileLine::UpdateType(
	TargetRec*	inTargetArray,
	int32		inTargetCount)
{
	bool		result = false;
	String		type;
	status_t	err = fSourceFile->GetMimeType(type);

	if ((B_OK == err && type.GetLength() == 0) || ENOENT == err)
	{
		entry_ref		ref;

		if (B_NO_ERROR == fSourceFile->GetRef(ref))
		{
			BEntry		entry(&ref);

			if (FixFileType(&entry, inTargetArray, inTargetCount))
			{
				err = fSourceFile->GetMimeType(type);
			}
		}
	}

	if (err == B_OK && type != fMimeType)
	{
		fMimeType = type;
		result = true;
	}

	return result;
}

// ---------------------------------------------------------------------------
//		WriteToFile
// ---------------------------------------------------------------------------
//	Write our Block to the stream.

void
MSourceFileLine::WriteToFile(
	MBlockFile & inFile)
{
	// Write the sourceFile block out
	SourceFileLineBlock		sourceBlock;

	sourceBlock.sCodeSize = fCodeSize;
	sourceBlock.sDataSize = fDataSize;
	sourceBlock.sDependencyFileCount = fDependencies.CountItems();
	sourceBlock.sFileType = 0;
	sourceBlock.sGoodCompileTime = fGoodCompileTime;

	if (fTouched)
		sourceBlock.sFlags = kDR9Flag | kTouchedFlag;
	else
		sourceBlock.sFlags = kDR9Flag;

	sourceBlock.SwapHostToBig();		// swap bytes
	inFile.StartBlock(fBlockType);

	// Write out the struct
	inFile.PutBytes(sizeof(sourceBlock), &sourceBlock);

	// Write out the Mime type
	inFile.StartBlock(kMimeBlockType);
	inFile.PutString(fMimeType);
	inFile.EndBlock();

	// Tell our sourceFile to write itself to the stream
	inFile.StartBlock(kMySourceFileBlockType);
	fSourceFile->WriteToFile(inFile);
	inFile.EndBlock();

	// Write out all of the dependency info
	int32		count = fDependencies.CountItems();
	for (int32 i = 0; i < count; i++)
	{
		MSourceFile*	sourceFile = (MSourceFile*) fDependencies.ItemAt(i);

		inFile.StartBlock(kSourceFileBlockType);
		sourceFile->WriteToFile(inFile);
		inFile.EndBlock();
	}

	// Write out the Browse data if there is any
	if (fBrowseData)
	{
		inFile.StartBlock(kBrowseBlockType);
		inFile.PutBytes(fBrowseDataLength, fBrowseData);
		inFile.EndBlock();
	}

	inFile.EndBlock();
}

/****************************************************************/
/* Purpose..: Find the definition of an identifier in object	*/
/* Input....: name of definition								*/
/* Input....: number of entry in project list					*/
/* Input....: flag if statics are also searched for 			*/
/* Returns..: ---												*/
/****************************************************************/

void 
MSourceFileLine::SearchObjectDef(
	const char *		defname, 
	bool	 			search_statics,
	InfoStructList&		inList) const
{
	if (fBrowseData)
	{
		bool	 	isCode;
		int32 		foundOffset;
		bool		found = SearchBrowseDataForIdentifier(fBrowseData, defname,
							 search_statics, &foundOffset, &isCode);
		if (found)
			SearchDefsInFile(defname, isCode, foundOffset, inList);
	}
}

/****************************************************************/
/* Purpose..: Find the definition of an identifier in object	*/
/* Input....: name of definition								*/
/* Input....: number of entry in project list					*/
/* Input....: flag if definition is code						*/
/* Input....: sourcecode offset 								*/
/* Returns..: ---												*/
/****************************************************************/

void 
MSourceFileLine::SearchDefsInFile(
	const char *		defname, 
	bool	 			is_code,
	int32 				sourceoffset,
	InfoStructList&		inList) const
{
	if (fSourceFile)
	{
		const char		*text;
		const char		*temp;
		char*			textbuffer = nil;
		InfoStruct*	 	info = nil;
		int32			size;
		int32			line;
		entry_ref		ref;
		status_t		err = fSourceFile->GetRef(ref);
		MTextWindow*	wind = MDynamicMenuHandler::FindWindow(ref);
		BEntry			entry(&ref);

		// Get a pointer and length to the text either from an open
		// window or by reading in the file

		// Is the window for this file open?
		if (wind && wind->Lock())
		{
			text = wind->Text();
		}
		else
		{
			// If not open the file and read in the text
			status_t	err;
			off_t		len;
			off_t		textLength;
			err = entry.GetSize(&textLength);
			textbuffer = new char[textLength + 1];
			BFile		file(&entry, B_READ_ONLY);

			len = file.Read(textbuffer, textLength);
			err = len >= 0 ? B_NO_ERROR : err;

			textbuffer[textLength] = 0;
			text = (const char*) textbuffer;

			// Deal with returns or cr-lfs here
			TextFormatType		format = MFormatUtils::FindFileFormat(textbuffer);
			
			if (format != kNewLineFormat)
				MFormatUtils::ConvertToNewLineFormat(textbuffer, textLength, format);
		}

		if (! is_code) 
		{  
			// If this is a variable, just jump to the offset given in the
			// browse data and hope for the best.  This will usually work even if the file's
			// been modified slightly.
			ShowResult(text, sourceoffset, ref, entry, inList);
		} 
		else 
		{
			// If this is a function, we'll use ParseDelarations().		
			MFunctionParser			parser(text, 0, kCPSuffix, false);
			int32					numFunctions = parser.FunctionCount();
			const DeclarationInfo *	funcInfo = parser.FunctionStorage();

			for (int32 i = 0; i < numFunctions; i++) 
			{
				if (0 == strcmp(funcInfo[i].name, defname))
				{
					info = new InfoStruct;
					info->iTextOnly = false;
					info->iRef = new entry_ref(ref);
					int32		offset = funcInfo[i].selectionStart;

					for (size = 0, temp = text + offset;
						 (*temp != EOL_CHAR) && (*temp != 0) && size < kLineTextLength; 
						 temp ++)
						size++;

					memcpy(info->iLineText, text + offset, size);
					info->iLineText[size] = 0;

					for (line = 1, temp = text; temp < (text + offset); temp++)
						if (*temp == EOL_CHAR) 
							line++;

					info->iLineNumber = line;		// message item lines count from 0
					err = entry.GetName(info->iFileName);

					// Build the token id struct
					info->iToken.eLineNumber = line;
					info->iToken.eOffset = offset;
					info->iToken.eLength = funcInfo[i].selectionEnd - funcInfo[i].selectionStart;
					info->iToken.eSyncLength = min(31L, size);
					info->iToken.eSyncOffset = 0;
					memcpy(info->iToken.eSync, &text[offset], info->iToken.eSyncLength);
					info->iToken.eSync[info->iToken.eSyncLength] = 0;
					info->iToken.eIsFunction = true;

					inList.AddItem(info);
				}
			}
			
			// If we didn't find it with ParseDeclarations then punt
			// Use the offsets from the browsedata.
			if (info == nil)
				ShowResult(text, sourceoffset, ref, entry, inList);
		}
		
		if (wind)
			wind->Unlock();
		
		delete textbuffer;
	}
}

// ---------------------------------------------------------------------------
//		ShowResult
// ---------------------------------------------------------------------------
//	Display the result based on the offsets in the browse data.

void 
MSourceFileLine::ShowResult(
	const char *	 	text,
	int32 				sourceoffset,
	entry_ref&			ref,
	BEntry&				file,
	InfoStructList&		inList) const
{
	const char		*temp;
	char*			textbuffer = nil;
	InfoStruct*	 	info;
	int32			size;
	int32			line;

	// If this is a variable, just jump to the offset given in the
	// browse data and hope for the best.  This will usually work even if the file's
	// been modified slightly.
	for (line = 1, temp = text; temp < (text + sourceoffset); temp++)
		if (*temp == EOL_CHAR)
			line++;

	info = new InfoStruct;
	info->iTextOnly = false;
	info->iRef = new entry_ref(ref);
	info->iLineNumber = line;		// message item lines count from 0
	file.GetName(info->iFileName);
	for (size = 0; size < kLineTextLength; size++) 
	{
		if (*(text + sourceoffset + size) == EOL_CHAR) 
			break;
		info->iLineText[size] = *(text + sourceoffset + size);
	}
	info->iLineText[size] = 0;

	// Build the token id struct
	info->iToken.eLineNumber = line;
	info->iToken.eOffset = sourceoffset;
	info->iToken.eLength = size;				// use linelength for the size
	info->iToken.eSyncLength = min(31L, size);
	info->iToken.eSyncOffset = 0;
	memcpy(info->iToken.eSync, &text[sourceoffset], info->iToken.eSyncLength);
	info->iToken.eSync[info->iToken.eSyncLength] = 0;
	info->iToken.eIsFunction = false;

	inList.AddItem(info);
}

// ---------------------------------------------------------------------------
//		• SwapBigToHost
// ---------------------------------------------------------------------------

void
SourceFileLineBlock::SwapBigToHost()
{
	if (B_HOST_IS_LENDIAN)
	{
		sCodeSize = B_BENDIAN_TO_HOST_INT32(sCodeSize);
		sDataSize = B_BENDIAN_TO_HOST_INT32(sDataSize);
		sDependencyFileCount = B_BENDIAN_TO_HOST_INT32(sDependencyFileCount);
		sFlags = B_BENDIAN_TO_HOST_INT32(sFlags);
		sGoodCompileTime = B_BENDIAN_TO_HOST_INT32(sGoodCompileTime);
	}
}

// ---------------------------------------------------------------------------
//		• SwapHostToBig
// ---------------------------------------------------------------------------

void
SourceFileLineBlock::SwapHostToBig()
{
	if (B_HOST_IS_LENDIAN)
	{
		sCodeSize = B_HOST_TO_BENDIAN_INT32(sCodeSize);
		sDataSize = B_HOST_TO_BENDIAN_INT32(sDataSize);
		sDependencyFileCount = B_HOST_TO_BENDIAN_INT32(sDependencyFileCount);
		sFlags = B_HOST_TO_BENDIAN_INT32(sFlags);
		sGoodCompileTime = B_HOST_TO_BENDIAN_INT32(sGoodCompileTime);
	}
}
