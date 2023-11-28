//========================================================================
//	MSourceFile.h
//========================================================================

#ifndef _MSOURCEFILE_H
#define _MSOURCEFILE_H

#include "IDEConstants.h"
#include "CString.h"
#include <Entry.h>
#include <Path.h>

class MBlockFile;
class MProjectView;

struct SourceFileBlockOld 
{
	uint32			sModDate;		// not used
	bool			sSystemTree;
	int32			sFileRecord1;	// not used was a record_ref
	int32			sFileRecord2;	// not used
	char			sName[B_FILE_NAME_LENGTH_DR8];
};

struct SourceFileBlockDR8 
{
	void		SwapBigToHost();

	int32			sFlag;		// not used
	bool			sSystemTree;
	char			sPad;		// padding
	ushort			sNameLength;
};

struct SourceFileBlock 
{
	void		SwapHostToBig();
	void		SwapBigToHost();

	uint32			sFlag;
	bool			sSystemTree;
	uchar			sFileKind;
	bool			sUnused1;
	bool			sUnused2;
};

enum {
	kSystemTree = 1,
	kHeaderFile = 2
};


class MSourceFile
{
public:
	enum FileKind 
				{
					kSourceFileKind, 
					kHeaderFileKind, 
					kPrecompiledHeaderKind
				};

public:

								MSourceFile(
									const BEntry&		inFile,
									bool 				inSystemTree,
									FileKind			inKind,
									MProjectView*		inProjectView,
									const char *		inName = nil);

								MSourceFile(
									const entry_ref&	inRef,
									bool 				inSystemTree,
									FileKind 			inKind,
									const char*			inName,
									MProjectView*		inProjectView);

								MSourceFile(
									const char*			inPath,
									bool 				inSystemTree,
									FileKind			inKind,
									const char*			inName,
									MProjectView*		inProjectView);
									
								MSourceFile(
									MBlockFile&			inBlockFile,
									MProjectView*		inProjectView,
									uint32				inFlag);
								MSourceFile(
									MBlockFile&			inBlockFile,
									MProjectView*		inProjectView);
								MSourceFile(
									const char * 		inName,	
									SourceFileBlock&	inBlock,
									MProjectView*		inProjectView);
								MSourceFile(
									const MSourceFile&	inSourceFile);

								~MSourceFile();

		bool					FileExists();
		bool					IsPrecompiledHeader();
		bool					IsUpToDate(
									time_t inTargetDate);
		time_t					ModificationTime();

		void					WriteToFile(
									MBlockFile & inFile);
		void					ResetFilePath();
		void					SetRef(
									const entry_ref&	inRef,
									bool				inUpdateName = false);
		void					SetRef(
									const BEntry&	inEntry,
									bool			inUpdateName = false);


		const entry_ref&		Ref()
								{
									return fRef;
								}

		status_t				GetRef(
									entry_ref&	inoutRef);
		status_t				GetLocRef(
									entry_ref& inoutRef);

		status_t				GetPath(
									char*	outPath,
									int32	inPathLength);
		size_t					GetPath(
									char*	outPath);

		off_t					Size();
		status_t				GetMimeType(
									String&		outType);

		status_t					WriteToBlock(
									char*	inBlock,
									off_t&	inSize);

		const char *			GetFileName() const
								{
									return fName;
								}
		int						NameLength() const
								{
									return fName.GetLength();
								}
		bool					IsInSystemTree()
								{
									return fSystemTree;
								}
		bool					IsHeaderFile()
								{
									return fFileKind == kHeaderFileKind || fFileKind == kPrecompiledHeaderKind;
								}
		bool					IsSourceFile()
								{
									return fFileKind == kSourceFileKind;
								}
		bool					HasProjectView()
								{
									return fProjectView != nil;
								}
		short					FileID() const
								{
									return fFileID;
								}
		void					InvalidateModTime()
								{
									fModTimeValid = false;
								}

		bool					IsSymLink()
								{
									return fIsSymLink;
								}
private:

		MProjectView*			fProjectView;
		entry_ref				fRef;
		String					fName;
		BPath					fPath;
		time_t					fModTime;
		FileKind				fFileKind;
		short					fFileID;
		bool					fSystemTree;
		bool					fModTimeValid;

		bool					fIsSymLink;
		entry_ref				fSymRef;
		BPath					fSymPath;


		void					InitFromBlock(
									SourceFileBlock&	inBlock);
		void					BuildOldSourceFile(
									MBlockFile&		inBlockFile,
									uint32			inVersion);
		void					FindFileIfNeeded(
									BEntry&	inFile);
};

#endif
