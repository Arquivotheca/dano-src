//========================================================================
//	MSourceFileLine.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
// BDS

#ifndef _MSOURCEFILELINE_H
#define _MSOURCEFILELINE_H

#include "MProjectLine.h"
#include "MSourceFile.h"
#include "MSourceFileList.h"
#include "MBlockFile.h"
#include "MMessageItem.h"
#include "Utils.h"

class MProjectView;
class MCompile;
class MBuildCommander;
struct ProjectTargetRec;
struct TargetRec;
struct MFileRec;

// Obsolete as of DR8 and version d4c
struct SourceFileLineBlockOld
{
	int32			sCodeSize;
	int32			sDataSize;
	int32			sDependencyFileCount;
};

// New version as of DR8 version 1.0
struct SourceFileLineBlock
{
	void		SwapHostToBig();
	void		SwapBigToHost();

	int32			sCodeSize;
	int32			sDataSize;
	int32			sDependencyFileCount;
	uint32			sFileType;
	uint32			sFlags;
	uint32			sGoodCompileTime;
};


class MSourceFileLine : public MProjectLine
{
	friend class MCompilerObj;
public:
								MSourceFileLine(
									const BEntry&		inFile,
									bool 				inInSystemTree,
									MSourceFile::FileKind inKind,
									MSectionLine& 		inSection,
									MProjectView& 		inProjectView,
									ProjectTargetRec*	inRec,
									const char *		inName);
								MSourceFileLine(
									MSectionLine& 	inSection,
									MProjectView& 	inProjectView,
									MBlockFile&		inBlockFile,
									BlockType		inBlockType,
									uint32			inProjectVersion);

	virtual						~MSourceFileLine();

	virtual	void				Draw(
									BRect 			inFrame, 
									BRect 			inIntersection, 
									MProjectView& 	inView);

	virtual bool				DoClick(
									BRect 			inFrame, 
									BPoint 			inPoint,
									bool 			inIsSelected,
									uint32			inModifiers,
									uint32			inButtons);
	virtual bool				SelectImmediately( 
									BRect 			inFrame, 
									BPoint 			inPoint,
									bool 			inIsSelected,
									uint32			inModifiers,
									uint32			inButtons);
	virtual void				Invoke();

	virtual void				WriteToFile(
									MBlockFile & 	inFile);
	void						SetTargetRec(
									MBuildCommander&	inCommander);
	virtual bool				NeedsToBeCompiled();
	virtual bool				NeedsToBePreCompiled();
	virtual bool				NeedsToBePostLinked();
	bool						NeedsToBeExecuted(
									MakeStageT	inStage,
									MakeActionT	inAction);
	virtual bool				CanBeExecuted() const;
			bool				UsesFileCache() const;

	virtual MCompile*			BuildCompileObj(
									MakeActionT		inKind);
	virtual void				CompileDone(
									status_t			errCode,
									int32				inCodeSize = 0,
									int32				inDataSize = 0,
									MSourceFileList*	inList = nil);

	virtual void				Touch();
	virtual void				UnTouch();

	virtual void				DeleteObjectFile();
	virtual void				RemoveObjects(
									bool inRemoveAll);

	virtual void				UpdateSuffixType();

	void						GetFilePath(
									char*		outFilePath) const;

	virtual void				BuildPopupMenu(
									MPopupMenu & inMenu) const;

	void 						SearchObjectDef(
									const char *		defname, 
									bool 			search_statics,
									InfoStructList&		inList) const;
	void 						SearchDefsInFile(
									const char *		defname, 
									bool	 			is_code,
									int32 				sourceoffset,
									InfoStructList&		inList) const ;
	void						SetBrowseData(
									char* 	inBrowseData,
									int32	inLength);

	bool						NeedsResourcesToBeCopied();
	void						ResourcesWereCopied();

	bool						HasResources() const;
	MakeStageT					CompileStage() const;
	void						SetTarget(
									ProjectTargetRec* inTarget);
	bool						UpdateType(
									TargetRec*	inTargetArray,
									int32		inTargetCount);
	void						UpdateLine();

	status_t					ParseMessageText(
									const char*	inText,
									BList&		outList);
	void						CodeDataSize(
									int32&	outCodeSize,
									int32&	outDataSize);
	void						GenerateDependencies();
	void						FillDependencyList(
									BList& inList) const;
	void						FillTargetFilePathList(
									BList& inList) const;
	void						FillFileRec(
									MFileRec&	outRec) const;


	bool						FileExists() const
								{
									return fSourceFile->FileExists();
								}

	MSourceFile*				GetSourceFile() const
								{
									return fSourceFile;
								}

	const char *				GetFileName() const
								{
									return fSourceFile->GetFileName();
								}
	virtual const char *		Name() const
								{
									return fSourceFile->GetFileName();
								}
	virtual void				ExternalName(char* outName) const
								{
									this->GetFilePath(outName);
								}
								
	const char *				MimeType() const
								{
									return fMimeType;
								}
	short						FileID() const
								{
									return fSourceFile->FileID();
								}
	bool						Touched() const
								{
									return fTouched;
								}
	SuffixType					GetSuffix() const
								{
									return fSuffixType;
								}
	void*						GetBrowseData() const
								{
									return fBrowseData;
								}
	void						ShowCompileMark(bool inShow = true)
								{
									fShowCompileMark = inShow;
								}
	static short				ConvertStage(
									MakeStageT	inStage);

protected:

		MSourceFile*			fSourceFile;
		ProjectTargetRec*		fTarget;
		BlockType				fBlockType;
		SuffixType				fSuffixType;
		String					fMimeType;
		time_t					fGoodCompileTime;
		time_t					fStartCompileTime;
		MakeActionT				fCompileType;
		char*					fBrowseData;
		int32					fBrowseDataLength;
		MSourceFileList			fDependencies;
		bool					fTouched;
		bool					fShowCompileMark;
	
		void					InitLine();
		bool					ObjectFileExists() const;
		bool					DependentFileChanged(
									size_t		inDate) const;

		void					ReportNoTarget();
		void					ShowPathPopup(
									BPoint 	inWhere,
									uint32	inModifiers,
									uint32	inButtons);

		static int				CompareFunction(
									const void* 	inSourceOne, 
									const void* 	inSourceTwo);
		void					SetBlockType(
									BlockType inBlockType)
								{
									fBlockType = inBlockType;
								}
private:

		void					BuildOldLine(
									MBlockFile&		inFile,
									BlockType		inBlockType);
		MCompile*				CompileSelf(
									MakeActionT		inKind);

		void					ShowResult(
									const char *	 	text,
									int32 				sourceoffset,
									entry_ref&			ref,
									BEntry&				file,
									InfoStructList&		inList) const;
};

#endif
