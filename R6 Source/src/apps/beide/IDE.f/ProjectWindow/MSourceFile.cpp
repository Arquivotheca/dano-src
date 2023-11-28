//========================================================================
//	MSourceFile.cpp
//	Copyright 1995 - 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	Encapsulates a project source file or header file or library file.
//	BDS

#include <string.h>

#include "MSourceFile.h"
#include "MProjectView.h"
#include "IDEConstants.h"
#include <StorageKit.h>


// ---------------------------------------------------------------------------
//		MSourceFile
// ---------------------------------------------------------------------------
//	Constructor for new file
//	The inName parameter can specify a relative path to this
//	file.  This is to support multiple files with the same name
//	in the project.  The relative path allows the file to be
//	identified (more-or-less) uniquely.

MSourceFile::MSourceFile(
	const BEntry&	inFile,
	bool 			inSystemTree,
	FileKind		inKind,
	MProjectView*	inProjectView,
	const char *	inName)				// can be nil
	: fSystemTree(inSystemTree),
	fProjectView(inProjectView),
	fIsSymLink(false)
{
	if (inName != nil)
		fName = inName;
	else
	{
		FileNameT		name;
	
		inFile.GetName(name);
	
		fName = name;
	}

	//SymLink support
	if (inFile.IsSymLink()) {
		inFile.GetRef(&fSymRef);
		inFile.GetPath(&fSymPath);
		
		fIsSymLink = true;
		
		BEntry entry(&fSymRef, true);
		entry.GetRef(&fRef);
		entry.GetPath(&fPath);
	}
	else {
		fIsSymLink = false;
		inFile.GetRef(&fRef);
		inFile.GetPath(&fPath);
	}
	fFileKind = inKind;
}

// ---------------------------------------------------------------------------
//		MSourceFile
// ---------------------------------------------------------------------------
//	Constructor for sourcefile specified by entry_ref.

MSourceFile::MSourceFile(
	const entry_ref&	inRef,
	bool 				inSystemTree,
	FileKind			inKind,
	const char*			inName,
	MProjectView*		inProjectView)
	: fSystemTree(inSystemTree),
	fProjectView(inProjectView),
	fName(inName),
	fRef(inRef),
	fIsSymLink(false)
{
	fFileKind = inKind;

	BEntry		entry(&inRef);

/* symlink support -hippo 3/29/98 */
	if (entry.IsSymLink()) {
		fIsSymLink = true;
		entry.GetRef(&fSymRef);
		entry.GetPath(&fSymPath);
		
		entry.SetTo(&inRef, true);
		entry.GetRef(&fRef);
	}
	
	entry.GetPath(&fPath);
}

// ---------------------------------------------------------------------------
//		MSourceFile
// ---------------------------------------------------------------------------
//	Constructor for sourcefile specified by entry_ref.

MSourceFile::MSourceFile(
	const char*		inPath,
	bool 			inSystemTree,
	FileKind		inKind,
	const char*		inName,
	MProjectView*	inProjectView)
	: fSystemTree(inSystemTree),
	fProjectView(inProjectView),
	fName(inName),
	fIsSymLink(false)
{
	get_ref_for_path(inPath, &fRef);
	fPath.SetTo(inPath);
	fFileKind = inKind;
}

// ---------------------------------------------------------------------------
//		MSourceFile
// ---------------------------------------------------------------------------
//	Constructor for sourcefile that was stored in the project file.

MSourceFile::MSourceFile(
	const char * 		inName,	
	SourceFileBlock&	inBlock,
	MProjectView*		inProjectView)
	: fProjectView(inProjectView),
	fName(inName),
	fIsSymLink(false)
{
	InitFromBlock(inBlock);
}

// ---------------------------------------------------------------------------
//		InitFromBlock
// ---------------------------------------------------------------------------

void
MSourceFile::InitFromBlock(
	SourceFileBlock&	inBlock)
{
	fSystemTree = inBlock.sSystemTree;
	fFileKind = (FileKind) inBlock.sFileKind;
}

// ---------------------------------------------------------------------------
//		MSourceFile
// ---------------------------------------------------------------------------
//	Constructor for sourcefile that was stored in the project file.
//	DR9

MSourceFile::MSourceFile(
	MBlockFile&		inFile,
	MProjectView*	inProjectView)
	: fProjectView(inProjectView),
	fIsSymLink(false)
{
	ASSERT(sizeof(SourceFileBlock) == 8);
	SourceFileBlock		sourceBlock;
	char				filename[1024] = { '\0' };
	
	// Read the struct and the filename
	inFile.GetBytes(sizeof(sourceBlock), &sourceBlock);
	sourceBlock.SwapBigToHost();
	BlockType			type;

	if (B_NO_ERROR == inFile.ScanBlock(type))
	{
		switch (type)
		{
			case kNameBlockType:
				inFile.GetString(filename, sizeof(filename));
				break;
			
			default:
				ASSERT(false);
				break;
		}
	
		inFile.DoneBlock(type);
	} 	
	
	fName = filename;
	fSystemTree = sourceBlock.sSystemTree;
	fFileKind = (FileKind) sourceBlock.sFileKind;
}

// ---------------------------------------------------------------------------
//		MSourceFile
// ---------------------------------------------------------------------------
//	Constructor for sourcefile that was stored in the project file.
//	PreDR9

MSourceFile::MSourceFile(
	MBlockFile&		inBlockFile,
	MProjectView*	inProjectView,
	uint32			inVersion)
	: fProjectView(inProjectView),
	fIsSymLink(false)
{
	switch (inVersion)
	{
		case kDR8plusProjectVersion:
		{
			SourceFileBlockDR8		sourceBlock;
			char				filename[1024];
			ASSERT(sizeof(sourceBlock) == 8);
			
			// Read the struct and the filename
			inBlockFile.GetBytes(sizeof(sourceBlock), &sourceBlock);
			sourceBlock.SwapBigToHost();		// swap bytes
			inBlockFile.GetBytes(sourceBlock.sNameLength, &filename);
			
			fName = filename;
			fSystemTree = sourceBlock.sSystemTree;
			fFileKind = kSourceFileKind;
		}
			break;

		default:
		{
			//	PreDR9
			SourceFileBlockOld		sourceBlock;
			
			// Read the struct
			inBlockFile.GetBytes(sizeof(sourceBlock), &sourceBlock);
			// don't need to swap bytes
			fName = sourceBlock.sName;
			fSystemTree = sourceBlock.sSystemTree;
			fFileKind = kSourceFileKind;
		}
			break;
	}
}

// ---------------------------------------------------------------------------
//		MSourceFile
// ---------------------------------------------------------------------------
//	Copy Constructor.

MSourceFile::MSourceFile(
	const MSourceFile&	inSourceFile)
	: fProjectView(inSourceFile.fProjectView),
	fRef(inSourceFile.fRef),
	fName(inSourceFile.fName),
	fPath(inSourceFile.fPath),
	fFileID(inSourceFile.fFileID),
	fSystemTree(inSourceFile.fSystemTree),
	fFileKind(inSourceFile.fFileKind),
	fIsSymLink(inSourceFile.fIsSymLink),
	fSymRef(inSourceFile.fSymRef),
	fSymPath(inSourceFile.fSymPath)
{
	fModTimeValid = false;
}

// ---------------------------------------------------------------------------
//		~MSourceFile
// ---------------------------------------------------------------------------
//	Destructor

MSourceFile::~MSourceFile()
{
}

// ---------------------------------------------------------------------------
//		IsUpToDate
// ---------------------------------------------------------------------------
//	return true if the passed-in date of the target is later than our mod date
//	We use the cached mod time so CacheModificationTime must have been called
//	at the start of this build.

bool
MSourceFile::IsUpToDate(
	time_t inTargetDate)
{
	status_t	err = B_NO_ERROR;

	if (!fModTimeValid)
	{
		BEntry		file(fPath.Path());
		FindFileIfNeeded(file);

		err = file.GetModificationTime(&fModTime);
		if (err == B_NO_ERROR)
			fModTimeValid = true;
	}
	
	return err == B_NO_ERROR && inTargetDate > fModTime;
}

// ---------------------------------------------------------------------------
//		GetRef
// ---------------------------------------------------------------------------

status_t
MSourceFile::GetRef(
	entry_ref&	inoutRef)
{
	status_t	err;
	BEntry		file(fPath.Path());
	FindFileIfNeeded(file);

	if (fRef.device == -1)
		err = B_ERROR;
	else
	{
		inoutRef = fRef;
		err = B_NO_ERROR;
	}

	return err;
}

// ---------------------------------------------------------------------------
//		GetLocRef
// ---------------------------------------------------------------------------
status_t
MSourceFile::GetLocRef(
	entry_ref&	inoutRef)
{
	status_t	err;
	BEntry		file(fPath.Path());

	FindFileIfNeeded(file);

	if (fRef.device == -1)
		err = B_ERROR;
	else
	{

		if(fIsSymLink)
			inoutRef = fSymRef;
		else inoutRef = fRef;
		err = B_NO_ERROR;
	}

	return err;
}


// ---------------------------------------------------------------------------
//		GetPath
// ---------------------------------------------------------------------------
//	returns B_NO_ERROR for success.

status_t
MSourceFile::GetPath(
	char*	outPath,
	int32	/*inPathLength*/)
{
	// If we have a symbolic link, pass the link to the tool, not the
	// resolved file.  (This is so we link with _KERNEL_ and not
	// with resolved file, like kernel_mac.)
	
	BPath* pathToUse = &fPath;
	if (IsSymLink()) {
		pathToUse = &fSymPath;
	}

	BEntry			file(pathToUse->Path());
	status_t		err;

	FindFileIfNeeded(file);

	if (file.Exists())
	{
		strcpy(outPath, pathToUse->Path());
		err = B_NO_ERROR;
	}
	else
		err = B_ERROR;

	return err;
}

// ---------------------------------------------------------------------------
//		GetPath
// ---------------------------------------------------------------------------
//	returns the length of the path.  Assumes the path buffer is 1024.

size_t
MSourceFile::GetPath(
	char*	outPath)
{
	// If we have a symbolic link, pass the link to the tool, not the
	// resolved file.  (This is so we link with _KERNEL_ and not
	// with resolved file, like kernel_mac.)

	BPath* pathToUse = &fPath;
	if (IsSymLink()) {
		pathToUse = &fSymPath;
	}
	
	BEntry			file(pathToUse->Path());
	size_t			len = 0;

	FindFileIfNeeded(file);

	if (file.Exists())
	{
		len = strlen(pathToUse->Path()) + 1;
		memcpy(outPath, pathToUse->Path(), len);
	}

	return len;
}

// ---------------------------------------------------------------------------
//		ModificationTime
// ---------------------------------------------------------------------------

time_t
MSourceFile::ModificationTime()
{
	BEntry		file(fPath.Path());
	time_t		result = 0;
	FindFileIfNeeded(file);
	
	(void) file.GetModificationTime(&result);

	return result;
}

// ---------------------------------------------------------------------------
//		ResetFilePath
// ---------------------------------------------------------------------------

void
MSourceFile::ResetFilePath()
{
	entry_ref		ref;
	
	fRef = ref;
	fPath.Unset();
}

// ---------------------------------------------------------------------------
//		SetRef
// ---------------------------------------------------------------------------

void
MSourceFile::SetRef(
	const entry_ref&	inRef,
	bool				inUpdateName)
{
	BEntry			file(&inRef);
	if (file.IsSymLink()) {
		fIsSymLink = true;
		file.GetRef(&fSymRef);
		file.GetPath(&fSymPath);
		
		file.SetTo(&inRef, true);
	}

	file.GetRef(&fRef);
	file.GetPath(&fPath);

	if (inUpdateName)
	{
		char			name[B_FILE_NAME_LENGTH + 1] = {'\0' };
		
		file.GetName(name);
		fName = name;
	}
}

// ---------------------------------------------------------------------------
//		SetRef
// ---------------------------------------------------------------------------

void
MSourceFile::SetRef(
	const BEntry&	inEntry,
	bool			inUpdateName)
{
	if (inEntry.IsSymLink()) {
		fIsSymLink = true;
		inEntry.GetRef(&fSymRef);
		inEntry.GetPath(&fSymPath);

		BEntry entry(&fSymRef, true);
		entry.GetRef(&fRef);
		entry.GetPath(&fPath);
	} else {
		inEntry.GetRef(&fRef);
		inEntry.GetPath(&fPath);
	}

	if (inUpdateName)
	{
		char			name[B_FILE_NAME_LENGTH + 1] = {'\0' };
		
		inEntry.GetName(name);
		fName = name;
	}
}

// ---------------------------------------------------------------------------
//		FindFileIfNeeded
// ---------------------------------------------------------------------------

void
MSourceFile::FindFileIfNeeded(
	BEntry&	inFile)
{
	if (! inFile.Exists() && fProjectView != nil)
	{
		if (fProjectView->FindSourceFile(*this))
		{
			inFile.SetTo(&fRef);
			inFile.GetPath(&fPath);
		}
	}
}

// ---------------------------------------------------------------------------
//		FileExists
// ---------------------------------------------------------------------------

bool
MSourceFile::FileExists()
{
	BEntry		file(fPath.Path());
	FindFileIfNeeded(file);

	return file.Exists();
}

// ---------------------------------------------------------------------------
//		IsPrecompiledHeader
// ---------------------------------------------------------------------------

bool
MSourceFile::IsPrecompiledHeader()
{
	return fFileKind == kPrecompiledHeaderKind;
}

// ---------------------------------------------------------------------------
//		GetMimeType
// ---------------------------------------------------------------------------

status_t
MSourceFile::GetMimeType(
	String&		outType)
{
	mime_t		mimeType;
	BEntry		file(fPath.Path());
	FindFileIfNeeded(file);

	BNode		fileformime(&file);
	BNodeInfo	mimefile(&fileformime);
	status_t	err = mimefile.GetType(mimeType);

	if (err == B_NO_ERROR)
		outType = mimeType;

	return err;
}

// ---------------------------------------------------------------------------
//		Size
// ---------------------------------------------------------------------------

off_t
MSourceFile::Size()
{
	BEntry		file(fPath.Path());
	FindFileIfNeeded(file);

	off_t		result;
	status_t	err = file.GetSize(&result);

	return result;
}

// ---------------------------------------------------------------------------
//		WriteFileToBlock
// ---------------------------------------------------------------------------
//	Copy the contents of the file to the memory block.  inSize
//	specifies the size of the block on input and the size of the
//	contents on output.  return B_NO_ERROR if no problems
//	occur.

status_t
MSourceFile::WriteToBlock(
	char*		inBlock,
	off_t&		inSize)
{
	BEntry		file(fPath.Path());
	FindFileIfNeeded(file);

	off_t			length;
	status_t		err = file.GetSize(&length);

	if (err == B_NO_ERROR)
	{
		if (length >= inSize)
			err = B_ERROR;
		else
		{
			inBlock[length] = 0;
			inSize = length;
			
			BFile		file(&fRef, B_READ_ONLY);
			err = file.InitCheck();

			if (err == B_NO_ERROR)
			{
				off_t	bytes = file.Read(inBlock, length);
				err = bytes != length;
			}
		}
	}

	return err;
}

// ---------------------------------------------------------------------------
//		WriteToFile
// ---------------------------------------------------------------------------
//	Write our Block to the stream.

void
MSourceFile::WriteToFile(
	MBlockFile & inFile)
{
	SourceFileBlock		sourceBlock;
	
	sourceBlock.sFlag = 0;
	sourceBlock.sSystemTree = fSystemTree;
	sourceBlock.sFileKind = fFileKind;
	sourceBlock.sUnused1 = false;
	sourceBlock.sUnused2 = false;
	sourceBlock.SwapHostToBig();		// swap bytes

	// Write out the struct and the name
	inFile.PutBytes(sizeof(sourceBlock), &sourceBlock);

	inFile.StartBlock(kNameBlockType);
	inFile.PutString(fName);
	inFile.EndBlock();
}

// ---------------------------------------------------------------------------
//		• SwapBigToHost
// ---------------------------------------------------------------------------

void
SourceFileBlock::SwapBigToHost()
{
	if (B_HOST_IS_LENDIAN)
	{
		sFlag = B_BENDIAN_TO_HOST_INT32(sFlag);
	}
}

// ---------------------------------------------------------------------------
//		• SwapHostToBig
// ---------------------------------------------------------------------------

void
SourceFileBlock::SwapHostToBig()
{
	if (B_HOST_IS_LENDIAN)
	{
		sFlag = B_HOST_TO_BENDIAN_INT32(sFlag);
	}
}

// ---------------------------------------------------------------------------
//		• SwapBigToHost
// ---------------------------------------------------------------------------
//	Don't write this one out so don't need SwapHostToBig.

void
SourceFileBlockDR8::SwapBigToHost()
{
	if (B_HOST_IS_LENDIAN)
	{
		sNameLength = B_BENDIAN_TO_HOST_INT32(sNameLength);
	}
}

