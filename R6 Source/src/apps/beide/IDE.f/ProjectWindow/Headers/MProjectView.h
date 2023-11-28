//========================================================================
//	MProjectView.h
//	Copyright 1995 - 96 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MPROJECTVIEW_H
#define _MPROJECTVIEW_H

#include "MListView.h"
#include "ScriptHandler.h"
#include "MCompileState.h"
#include "MBlockFile.h"
#include "MProjectLine.h"
#include "MMessageWindow.h"
#include "MPrefsStruct.h"
#include "MSourceFileList.h"
#include "MAccessPathsView.h"
#include "MFileUtils.h"
#include "MFileGetter.h"
#include "BeIDEComm.h"
#include "CString.h"
#include "MTargetTypes.h"
#include "MTextWindow.h"

#include <Directory.h>
#include <Locker.h>

const uint32 kDR8ProjectVersion 		= 0x00000001;	// version 1.0
const uint32 kDR8plusProjectVersion 	= 0x00000002;	// version 1.1 & 1.1.1, 1.2
const uint32 kCurrentProjectVersion 	= 0x00000003;	// version 1.3

const BlockType kProjectVersionBlockType = MW_FOUR_CHAR_CODE('PrVr');
const BlockType kSectionBlockType 		= MW_FOUR_CHAR_CODE('Sect');
const BlockType kBrowseBlockType 		= MW_FOUR_CHAR_CODE('Brws');
const BlockType kCompileFileBlockType 	= MW_FOUR_CHAR_CODE('Fil1');
const BlockType kPrecompileFileBlockType= MW_FOUR_CHAR_CODE('PreF');
const BlockType kLinkFileBlockType 		= MW_FOUR_CHAR_CODE('Link');
const BlockType kPostLinkFileBlockType 	= MW_FOUR_CHAR_CODE('PLnk');
const BlockType kIgnoreFileBlockType 	= MW_FOUR_CHAR_CODE('IgFl');
const BlockType kSubProjectFileBlockType= MW_FOUR_CHAR_CODE('SbPr');
const BlockType kSourceFileBlockType 	= MW_FOUR_CHAR_CODE('SrFl');	// added for 1.3
const BlockType kMySourceFileBlockType 	= MW_FOUR_CHAR_CODE('MSFl');	// added for 1.3
const BlockType kMimeBlockType 			= MW_FOUR_CHAR_CODE('Mime');	// added for 1.3
const BlockType kNameBlockType 			= MW_FOUR_CHAR_CODE('Name');	// added for 1.3
const BlockType kEndianBlockType 		= MW_FOUR_CHAR_CODE('PrEn');	// added for intel release

// These are old block types made obsolete
// with the DR8 release
const BlockType kFileBlockTypeOld 		= MW_FOUR_CHAR_CODE('File');
const BlockType kPCHFileBlockTypeOld	= MW_FOUR_CHAR_CODE('PCHF');
const BlockType kLibFileBlockTypeOld 	= MW_FOUR_CHAR_CODE('shlb');

typedef MList<MProjectLine*> MProjectLineList;
typedef MList<MSectionLine*> MSectionLineList;

class MSectionList;
class MSourceFileLine;
class MSection;
class MSourceFile;
class MCompile;
class MLinkObj;
class MPreCompileObj;
class MCompilerObj;
class MFindFilesThread;
class MFindDefinitionTask;
class MFindDefinitionWindow;
class MBuildCommander;
class MPlugInLinker;
class MProject;
class MThread;
class MProjectWindow;

struct FileSetRec;
struct TargetRec;
struct MFileRec;
struct SourceFileBlock;

enum SourceListT
{
	kSourceFiles,
	kProjectHeaderFiles,
	kSystemHeaderFiles,
	kSourceFilesRefs
};

class MProjectView : 	public MListView,
						public ScriptHandler,
						public MFileGetter
{
	friend class MProjectWindow;
	friend class MSectionLine;
	friend class MFindFilesThread;
	friend class MBuildCommander;
	friend class MProjectFileHandler;

public:
 								MProjectView(BRect inFrame, const char* inName, MProjectWindow* ownerProject); 
								~MProjectView();

virtual	void					AttachedToWindow();
virtual	void					Pulse();
virtual	void					FrameResized(
									float new_width, 
									float new_height);

		void					MessageReceived(
									BMessage * message);
virtual	void					KeyDown(
									const char *bytes, 
									int32 		numBytes);

virtual	void					MouseMoved(	
									BPoint where,
									uint32 code,
									const BMessage *a_message);
virtual	void					Draw(
									BRect updateRect);
		void					HiliteColorChanged();

		void					Cancel();

		status_t				FindHeader(	
									const HeaderQuery& 	inQuery,
									HeaderReply& 		outReply,
									MSourceFile*&		outSourceFile,
									bool&				outFoundInSystemTree);

		MSourceFile*			FindHeader(
									const char*	fileName, 
									bool		inSystemTree, 
									bool&		foundInSystemTree);

		MSourceFile*			FindPartialPathHeader(BEntry& inFile);

		bool					AddHeaderFile(	
									MSourceFile*		inSourceFile,
									MSourceFile*&		outSourceFile);
		MSourceFile*			FindHeaderFile(
									const char * 	inName,	
									bool			inSystemTree,
									bool			inAddit = true);
		MSourceFile*			FindHeaderFile(
									const char * 		inName,	
									SourceFileBlock&	inBlock);

		void					WriteToFile(
									MBlockFile & inFile);
		void					ReadFromFile(
									MBlockFile & inFile);

		void					AddDefaultSection();
		void					AddLibs();
		void					NewProjectSaved();
		void					RunMenuItemChanged();
		void					SetWorkingDirectory();

		void					BuildAction(
									uint32 inCommand);
		void					BuildAction(
									BMessage& inMessage);
virtual	status_t				PerformScriptAction(
									BMessage *			message,
									BMessage * &		reply,
									bool&				wasDeferred);

		status_t				BuildPrecompileArgv(
									BuilderRec*		inBuilderRec,
									StringList&		inArgv,
									MFileRec& 		inFileRec);
		status_t				BuildCompileArgv(
									MakeActionT 	inAction,
									BuilderRec*		inBuilderRec,
									StringList&		inArgv,
									MFileRec& 		inFileRec);
		status_t				BuildPostLinkArgv(
									BuilderRec*		inBuilderRec,
									StringList&		inArgv,
									MFileRec& 		inFileRec);

		void					SetBuildParameters(const BuildExtrasPrefs& buildparams);
		
		void					CompileDone();
		void					LinkDone();
		void					LinkDone(
									MPlugInLinker*	inLinker);
		status_t				ParseLinkerMessageText(
									const char*	inText,
									BList&		outList);
		void					OneCompileDone(
									BMessage& inMessage);
		void					PreCompileDone();
		void					RemoveObjects(
									uint32	inRemoveCommand);
		void					ResetFilePaths();

		void					ResetPrecompiledHeaderFile(
									const char*		inFileName);
	
		void					FindDefinition(
									BMessage&	inMessage);
		void					ExecuteFindDefinition(
									const char *			inToken,
									MTextWindow *			inWindow,
									MFindDefinitionTask&	inTask);

		void					SetDirty();
		void					SaveChanges();
		void					SetPrefs(
									BMessage&	inMessage,
									uint		inUpdateType);
		void					SetSectionName(
									BMessage& 			inMessage);
		bool					SectionIsSelected();
		bool					FileIsSelected();

		bool					FileIsInProjectByFile(
									const BFile* 		inFile);
		bool					FileIsInProjectByFile(
									const BEntry* 		inFile);
		bool					FindSourceFile(	
									MSourceFile&		inSourceFile);
		bool					IsInSystemTree(
									const BEntry& 		inFile);

		void					DoAndyFeature();
		void					BuildPopupMenu(
									const char *	inName,
									MPopupMenu& 	inPopup);
		void					TouchFile(
										BMessage&	inMessage);
		void					TouchAllSourceFiles(
									bool inDoAll = false);

		bool					GetNthFile(
									MFileRec&	outRec,
									int32		inIndex);
		bool					GetNthFile(
									MFileRec&	outRec,
									BList&		outTargetList,
									int32		inIndex);
		void					GetAccessDirectories(
									BList& inOutProjectList,
									BList& inOutSystemList,
									bool& outSearchProjectTreeFirst);

		bool					IsIdle();
		int32					FileCount()
								{
									return fFileList.CountItems();
								}

		MBuildCommander&		BuildCommander()
								{
									return *fBuildCommander;
								}

		MProject&				BuilderProject()
								{
									return *fProject;
								}

	virtual	off_t				FileSize(
									const char*	inName,
									bool		inSystemTree);
	virtual	status_t			WriteFileToBlock(
									void*		inBlock,
									off_t&		ioSize,
									const char*	inName,
									bool		inSystemTree);

		MMessageWindow*			GetErrorMessageWindow()
								{
									return fErrorMessageWindow;
								}

		MProjectLinePainter&	GetPainter()
								{
									return fProjectLinePainter;
								}
								
protected:

virtual	void					DrawRow(
									int32 index,
									void * data,
									BRect rowArea,
									BRect intersectionRect);
virtual	void					HiliteRow(
									int32 		inRow,
									BRect 		inArea);
virtual	void					UnHiliteRow(
									int32 		inRow,
									BRect 		inArea);
virtual	bool					MessageDropped(
									BMessage *aMessage);

virtual	bool					ClickHook(
									BPoint	inWhere,
									int32	inRow,
									uint32	modifiers,
									uint32	buttons);
		void					InvokeRow(
									int32 inIndex);

		bool					DoRefsReceived(
									BMessage &inMessage,
									const BPoint inLoc);
		void					AddToProject(
									BMessage& inMessage);
		bool					AddFile(
									const BEntry& 	inFile, 
									BPoint 			inLoc);
		void					AddFilesMessage(
									BMessage& inMessage);
		bool					AddFolder(
									BDirectory& 	inFolder, 
									BPoint 			inLoc);
		bool					AddSymLink(
								const BEntry& 	inFile, 
									BPoint 			inLoc);

		bool					AddFileAtIndex(
									const BEntry&		inFile,
									int32				inIndex,
									ProjectTargetRec*	rec = nil);
		status_t				AddFileAtIndexAbsolute(
									const BEntry&		inFile,
									int32				inIndex);
		status_t				GetNewSourceFileLine(
									const BEntry&		inFile,
									MSectionLine*		inSection,
									MSourceFileLine*&	outLine,
									String&				inoutErrrorText,
									ProjectTargetRec*	inRec = nil);
		MSourceFileLine*		GetLine(
									const BEntry&		inFile,
									MSectionLine&		inSection,
									ProjectTargetRec*	inRec,
									const char *		inName);
		void					ValidatePath(
									const BEntry&		inFile,
									DirectoryInfo*&		outInfo);
		void					ValidatePath(
									BFile& inFile);
		void					ReportAddFileError(
									const BEntry&		inFile,
									status_t			inError,
									String&				inText);
		status_t				DuplicateNameError(
									const BEntry&		inFile,
									MSourceFileLine*	inLine,
									String&				inoutErrorText);
		status_t				DuplicateFileError(
									const BEntry&		inFile,
									MSourceFileLine*	inLine,
									String&				inoutErrorText);
		status_t				NoTargetError(
									const BEntry&		inFile,
									String&				inoutErrorText);

		void					RemoveSelection();
		void					RemoveSection(
									MSectionLine* 	inSection,
									bool			inUpdate = true);
		void					RemoveFile(
									MSourceFileLine* 	inSourceFileLine,
									bool				inUpdate = true);

		void					SelectAllLines();
		void					ClearSelection();
		
		void					DoCopy();
		
		bool					AddSectionAtIndex(
									int32 inIndex);
		bool					AddSectionAtSelection();

		void					SortSelectionGroup();
		
		void					SaveAsForSourceFile(
									BMessage& 		inMessage);

		bool					FileIsInProjectByName(
									BEntry* 			inFile) const;
		bool					FileIsInProjectByName(
									BFile* 				inFile) const;
		bool					FileIsInProjectByName(
									const char* 		inFileName,
									MSourceFileLine**	outLine = nil) const;
										
		void					FileWasAddedToProject(
									const BEntry&	inFile);

		bool					OpenSelection(
									const char * 	inFileName,
									bool			inSystemTree,
									entry_ref&		outRef,
									bool			inSearchInMemory,
									bool			inSearchOnDisk);

		void					SetProject(
									const entry_ref&		inProject);

		bool					IsRunnable();
		
		BDirectory&				GetProjectDirectory()
								{
									return fProjectDirectory;
								}

		BList&					GetFileList()
								{
									return fFileList;
								}
		void					RunWithDebugger(
									bool	inRunWithDebugger);
		bool					RunsWithDebugger()
								{
									return fPrivatePrefs.runsWithDebugger;
								}
		bool					OKToModifyProject();

private:

		MProjectLineList		fFileList;				// All the sourcefileLines in the project window
		MSectionLineList		fSectionList;			// All the sections in the project window
		MSourceFileList			fAllFileList;			// All the files in the project
		AccessPathList			fProjectPathList;		// project path records
		AccessPathList			fSystemPathList;		// system path records
		DirectoryList			fProjectDirectories;	// project directories for this project
		DirectoryList			fSystemDirectories;		// system directories for this project
		BDirectory				fProjectDirectory;
		int32					fCompleted;				// Number of compiles completed
		int32					fToBeCompleted;			// Number of compiles to be completed
		MCompile*				fCompileObj;
		MFindFilesThread*		fFinder;
		MFindDefinitionTask*	fFindDefinitionTask;
		MFindDefinitionWindow*	fFindDefinitionWindow;
		MBuildCommander*		fBuildCommander;
		MProject*				fProject;
		MProjectWindow*			fProjectWindow;
		bool					fOKToWriteProject;
		
		int32					fFilesFound;
		int32					fOldWidth;
		int32					fDragIndex;
		bigtime_t				fKeyDownTime;
		bool					fTypingAhead;
		String					fSearchString;
		BLocker					fFileListLocker;

		RunPreferences			fRunPrefs;
		AccessPathsPrefs		fAccessPathsPrefs;
		TargetPrefs				fTargetPrefs;
		PrivateProjectPrefs		fPrivatePrefs;

		MProjectLinePainter		fProjectLinePainter;
		
// These are obsolete
		ProjectPrefs			fProjectPrefs;
		LanguagePrefs			fLanguagePrefs;
		ProcessorPrefs			fProcessorPrefs;
		WarningsPrefs			fWarningsPrefs;
		DisassemblePrefs		fDisassemblePrefs;
		LinkerPrefs				fLinkerPrefs;
		PEFPrefs				fPEFPrefs;
// end obsolete

		BBitmap*				fResizeMap;
		MProjectView*			fChildView;
		void*					fFileSets;
		int32					fFileSetsSize;

		MMessageWindow*			fErrorMessageWindow;
		
		// private constructor for childview
								MProjectView(
									BRect inFrame, 
									char *inName, 
									bool inTask);

		void					InitProjectView();
		void					InitProject();

		int32					GetIndex(BPoint inPoint);
		MSectionLine*			GetSection(BPoint inLoc);
		MSectionLine*			GetSection(int32 inIndex);
		void					KillSection(
									MSectionLine* inSection);

		void					SortSection(MSectionLine* section);

		int32					GetIndexInFileList(
									const MSectionLine& inSection,
									int32 				inIndex) const;
		int32					GetIndexInView(
									int32 				inIndex) const;

		bool					GetSourceFileLineByName(
									const char* 		inName, 
									MSourceFileLine* &	outLine);
		bool					GetSourceFileLineByRef(
									entry_ref&	 		inRef, 
									MSourceFileLine* &	outLine);
									
		void					GetBuiltAppPath(char* outPath);
		void					SetUpObjectFileDirectory();

		void					LastState();
		CompileState			CompilingState();

		void					ShowFileCount();
		void					ShowIdleStatus();

		void					PostNoFileMessage(
									MSourceFileLine*	line);
	
		void					GetData(
									BMessage&	inOutMessage);
		void					SetData(
									BMessage&	inOutMessage);
		uint32					ReadPreferences(
									MBlockFile&		inFile);

		void					FillCompileList(
									BList&	inList,
									bool&	outUsesFileCache);
		void					OpenCurrentSelection();
		void					RevealSelectionInTracker();
		void					PrefsToBuildCommander(
									bool	inInitialize = false);
		void					ValidateGenericData();

		void					FillFileList(
									MSourceFileList&	inList, 
									SourceListT			inKind);
		void					FillFileList(
									BList*				inList, 
									SourceListT			inKind);
		void					InvalidateModificationDates();

		void					UpdateCodeDataRect(
									int32 inIndex,
									bool inCheckMarkColumn = false);

		void					HandleDragSelf(
									BMessage&	inMessage, 
									int32 		inIndex);
		void					DrawDragLine(
									int32 	inIndex);
		void					EraseDragLine();
		int32					GetSectionDropIndex(
									int32 		inIndex);
virtual	void					InitiateDrag(
									BPoint 	inPoint,
									int32 	inRow);
		void					DragFilesOrSections(
									BMessage&	inMessage, 
									int32 		inIndex,
									int32		inCount);
		void					DragFiles(
									BMessage&	inMessage, 
									int32 		inIndex,
									int32		inCount);
		void					DragSections(
									BMessage&	inMessage, 
									int32 		inIndex,
									int32		inCount);
		void					DragFilesAndSections(
									BMessage&	inMessage, 
									int32 		inIndex,
									int32		inCount);

		bool					AddRefToMessage(BMessage& msg, MProjectLine* projectLine);

		bool					FindOneFile(
									int32	&ioCount);
		bool					UpdateOneFile(
									int32	&ioCount);

		void					GenerateBitmap(
									float	inHeight,
									int32	inRow,
									float	inRowTop);

		void					UpdateBitmap(
									int32	inFromRow = 0);
		void					UpdateBitmap(
										int32	inFromRow,
										int32	inHowMany,
										bool	inAdding);
		void					UpdateRow(
									int32	inRow);
		void					DrawRow(
									int32	inRow);
		void					HiliteRowInChildView(
									int32 	inRow,
									bool	inDrawSelected);
		void					SelectRow(
									int32 	row,
									bool 	keepOld = false,
									bool 	toSelect = true);
		void					SelectRows(
									int32 	fromRow,
									int32 	toRow,
									bool 	keepOld = false,
									bool 	toSelect = true);
		void					InvalidateRow(
									int32 	inRow);
		void					UpdateLine(
									BMessage&	inMessage);
		void					StartFinder();

		int32					FindRowByName(
									const char * inName);
		void					DoArrowKey(
									uint32	inArrowMessage);
	
		MSectionLine*			SectionAt(
									int32	inRow);
		void					LockFileList();
		void					UnlockFileList();
		void					UpdateAllTargets();
		void					UpdateOneTarget(
									MSourceFileLine*	inLine);

		void					UpdateForDR8();

 		void					FindAllFiles(
 									MFindFilesThread&	inThread,
 									NodeList&			inNodeList);
 		void					FindFilesInDirectoryList(
 									MSourceFileList&	inFileList,
 									MSourceFileList&	inSlashFileList,
 									DirectoryList&		inDirList,
 									NodeList&			inNodeList,
 									bool				inIsSystem,
									MFindFilesThread&	inThread);
		status_t				AddFilesFromScript(
									BMessage* 	inMessage,
									BMessage*	inReply,
									int32		inIndex);
		status_t				RemoveFileFromScript(
									BMessage* 			inMessage,
									BMessage* 			inReply,
									MSourceFileLine*	inLine);
		status_t				SourceFileLineFromScript(
									BMessage* 			inMessage,
									MSourceFileLine*&	outLine);

		MSourceFile*			GetFile(
									const char*	inName,
									bool		inSystemTree);
		MSourceFileLine *		GetLineByIndex(
									int32	inIndex);
	
public:

virtual	void					RemoveRows(
									int32 fromNo,
									int32 numRows = 1);
		bool					RemoveItem(
									void * item);
		void *					RemoveItem(
									int32 index);
		void *					ItemAt(
									int32 index) const;
		int32					IndexOf(
									void * item) const;
		int32					CurrentSelection() const;
};

inline void 
MProjectView::LockFileList()
{
	fFileListLocker.Lock();
}
inline void 
MProjectView::UnlockFileList()
{
	fFileListLocker.Unlock();
}

inline bool
MProjectView::RemoveItem(
	void * data)
{
	ASSERT(GetList() != nil);
	BList * list = GetList();
	int32 index = list->IndexOf(data);
	if (index < 0)
		return false;
	RemoveRows(index);
	return true;
}

inline void *
MProjectView::RemoveItem(
	int32 index)
{
	ASSERT(GetList() != nil);
	void * ret = GetList()->ItemAt(index);
	RemoveRows(index);
	return ret;
}

inline void *
MProjectView::ItemAt(
	int32 index) const
{
	ASSERT(GetList() != nil);
	return GetList()->ItemAt(index);
}

inline int32
MProjectView::IndexOf(
	void * data) const
{
	ASSERT(GetList() != nil);
	return GetList()->IndexOf(data);
}

inline int32
MProjectView::CurrentSelection() const
{
	int32 ret = -1;
	FirstSelected(ret);
	return ret;
}

#endif
