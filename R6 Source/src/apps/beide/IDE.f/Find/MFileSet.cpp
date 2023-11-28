//========================================================================
//	MFileSet.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include <string.h>

#include "MFileSet.h"
#include "MSourceFile.h"
#include "MPreferences.h"
#include "MPrefsStruct.h"
#include "IDEMessages.h"
#include <ByteOrder.h>
#include <Debug.h>

// ---------------------------------------------------------------------------
//		MFileSetKeeper
// ---------------------------------------------------------------------------
//	Constructor

MFileSetKeeper::MFileSetKeeper()
{
	// Get the global file sets and reanimate them
	ReadGlobalSets();
}

// ---------------------------------------------------------------------------
//		~MFileSetKeeper
// ---------------------------------------------------------------------------
//	Destructor

MFileSetKeeper::~MFileSetKeeper()
{
	EmptyList(fProjectSets);
	EmptyList(fGlobalSets);
}

// ---------------------------------------------------------------------------
//		RemovePreferences
// ---------------------------------------------------------------------------

void
MFileSetKeeper::RemovePreferences()
{
	(void) MPreferences::RemovePreference(kFileSetPrefs, kPrefsType);
}

// ---------------------------------------------------------------------------
//		AddFileSet
// ---------------------------------------------------------------------------
//	Add a file set to the list of file sets.  Save it in the database
//	for global sets.  The find window will tell the project about it
//	if it's a project set.

void
MFileSetKeeper::AddFileSet(const char* inName,
						  bool isProjectList,
						  const MList<MSourceFile*>& inFileList)
{
	size_t			count = inFileList.CountItems();
	size_t			bufferSize = max(count * 50, 5 * sizeof(FileSetRec));	//estimate 50 bytes per record
	size_t			actualSize = 0;
	void*			recArray = ::operator new(bufferSize);
	FileSetRec*		rec = (FileSetRec*) recArray;	
	MSourceFile*	sourceFile;
	int32			i = 0;
	
	// Build all of the file set records
	while (inFileList.GetNthItem(sourceFile, i++))
	{
		if (actualSize + sizeof(FileSetRec) > bufferSize)
		{
			size_t			newSize = 2 * bufferSize;
			void*			temp = ::operator new(newSize);
			memcpy(temp, recArray, actualSize);
			::operator delete(recArray);
			recArray = temp;
			bufferSize = newSize;
			rec = (FileSetRec*) ((char*) recArray + actualSize);
		}
		
		size_t		pathLen = sourceFile->GetPath(rec->path);
		if (pathLen > 0)
		{
			pathLen += sizeof(short) + sizeof(char);	// include nil?
			rec->length = pathLen;
			rec->flags = 0;
			if (! sourceFile->HasProjectView())
				FSSetOther(rec->flags);
			else
			if (! sourceFile->IsHeaderFile())
				FSSetSource(rec->flags);
			else
			{
				if (sourceFile->IsInSystemTree())
					FSSetSystemHeader(rec->flags);
				else
					FSSetProjectHeader(rec->flags);
			}
			
			actualSize += pathLen;
			rec = (FileSetRec*)((char*) rec + pathLen);
		}
	}
	
	// Build the fileset object
	MFileSet*		fileSet = new MFileSet;
	
	fileSet->fName = inName;
	fileSet->fCount = count;
	fileSet->fSize = actualSize;
	fileSet->fRecs =  (FileSetRec*) recArray;
	
	// Add it to the database or the project
	if (! isProjectList)
	{
		fGlobalSets.AddItem(fileSet);
		WriteGlobalSets();
	}
	else
	{
		fProjectSets.AddItem(fileSet);
	}
}

// ---------------------------------------------------------------------------
//		WriteGlobalSets
// ---------------------------------------------------------------------------
//	Save the file set objects in a single block in the database.  The
//	block consists of a header, followed by each list.  Each of the lists
//	consists of a header, followed by records for each file in the list.

void
MFileSetKeeper::WriteGlobalSets()
{
	// Get the set block
	MSetHolder		globalSet;

	ListToSetBlock(fGlobalSets, globalSet);

	// Store it in the database
	status_t	err = MPreferences::SetPreference(kFileSetPrefs, kPrefsType, globalSet.Set(), globalSet.Size());
	ASSERT(err == B_NO_ERROR);
}

// ---------------------------------------------------------------------------
//		ReadGlobalSets
// ---------------------------------------------------------------------------
//	Read the global file set records from the database and generate fileset
//	objects.

void
MFileSetKeeper::ReadGlobalSets()
{
	size_t		size;

	if (MPreferences::PreferenceExists(kFileSetPrefs, kPrefsType, &size))
	{
		void*			globalSet = ::operator new(size);
		status_t		err = MPreferences::GetPreference(kFileSetPrefs, kPrefsType, globalSet, size);

		if (err == B_NO_ERROR)
		{
			SetBlockToList(globalSet, fGlobalSets);
		}
		
		::operator delete(globalSet);
	}
}

// ---------------------------------------------------------------------------
//		ListToSetBlock
// ---------------------------------------------------------------------------
//	Given a file list generate a memory block that contains its contents.
//	This will be saved in either the database or the project file.

void
MFileSetKeeper::ListToSetBlock(
	FileSetList&	inList,
	MSetHolder&		outSet)
{
	FileSetHeader		header;
	MFileSet*			fileSet;
	int32				records = inList.CountItems() + 1;
	int32				size = (inList.CountItems() + 1) * sizeof(FileSetHeader);
	int32				i = 0;
	
	while (inList.GetNthItem(fileSet, i++))
	{
		records += fileSet->fCount;
		size += fileSet->fSize + sizeof(FileSetHeader);
	}
	
	char *			set = (char*) ::operator new(size);

	// The list header just has the number of lists in it
	memset(&header, '\0', sizeof(FileSetHeader));
	header.RecordCount = inList.CountItems();
	header.Version = kFileSetVersion;
	header.Size = size;
	header.name[0] = 0;
	memcpy(set, &header, sizeof(FileSetHeader));

	// Add all of the file sets to the memory block
	char *			setPtr = set + sizeof(FileSetHeader);

	i = 0;
	while (inList.GetNthItem(fileSet, i++))
	{
		// write out the information in host format
		// (to remain compatible with previously written file sets)
		// (To handle endian issues, we not only have to swap the fCount
		// and fSize, but also the length in each individual file record).
		header.RecordCount = fileSet->fCount;
		header.Version = kFileSetVersion;
		header.Size = fileSet->fSize;
		strcpy(header.name, fileSet->fName);

		memcpy(setPtr, &header, sizeof(FileSetHeader));
		setPtr += sizeof(FileSetHeader);
		memcpy(setPtr, fileSet->fRecs, fileSet->fSize);
		setPtr += fileSet->fSize;
	}

	outSet.SetSet(set, size);
}

// ---------------------------------------------------------------------------
//		SetBlockToList
// ---------------------------------------------------------------------------
//	Given a block of memory previously generated by ListToSetBlock generate
//	a file list.  

void
MFileSetKeeper::SetBlockToList(
	void*			inSet,
	FileSetList&	inList)
{
	char*					setPtr = (char*) inSet;
	const FileSetHeader*	header = (FileSetHeader*) setPtr;
	const int32				setCount = header->RecordCount;
	setPtr += sizeof(FileSetHeader);

	for (int32 i = 0; i < setCount; i++)
	{
		MFileSet*		fileSet = new MFileSet;
		
		// If we find we don't know about the version
		// (most probably because it was written on a different
		// endian machine), then just stop trying to read
		// the file sets -- essentially throwing them away
		// (See notes in ListToSetBlock above)
		header = (FileSetHeader*) setPtr;
		if (header->Version == B_SWAP_INT32(kFileSetVersion)) {
			// ack! Need to convert RecordCount, Size, and each 
			// individual record
			return;
		}
		
		fileSet->fCount = header->RecordCount;
		fileSet->fSize = header->Size;
		fileSet->fName = header->name;
		fileSet->fRecs = (FileSetRec*) ::operator new(fileSet->fSize);

		setPtr +=  sizeof(FileSetHeader);
		memcpy(fileSet->fRecs, setPtr, fileSet->fSize);
		setPtr += fileSet->fSize;
		
		inList.AddItem(fileSet);
	}
}

// ---------------------------------------------------------------------------
//		GetFileSet
// ---------------------------------------------------------------------------
//	Get a file set object that has the specified name.

const MFileSet*
MFileSetKeeper::GetFileSet(
	const char *	inName) const
{
	MFileSet*		fileSet = nil;
	bool			found = false;
	int32			i = 0;

	// Search in the global list
	while (fGlobalSets.GetNthItem(fileSet, i++))
	{
		if (0 == strcmp(inName, fileSet->fName))
		{
			found = true;
			break;
		}
	}
	
	// Search in the project list
	if (! found)
	{
		i = 0;
		while (fProjectSets.GetNthItem(fileSet, i++))
		{
			if (0 == strcmp(inName, fileSet->fName))
			{
				found = true;
				break;
			}
		}
	}

	if (! found)
		fileSet = nil;
	
	return fileSet;
}

// ---------------------------------------------------------------------------

const MFileSet*	
MFileSetKeeper::GetNthFileSet(int32 index, bool isProjectList)
{
	MFileSet* fileSet = nil;
	FileSetList& whichList = isProjectList ? fProjectSets : fGlobalSets;
	return whichList.GetNthItem(fileSet, index) ? fileSet : nil;
}

// ---------------------------------------------------------------------------
//		GetProjectSets
// ---------------------------------------------------------------------------
//	Return an anonymous block to be saved by the project which contains
//	the project file sets.  Dispose of this block with 'delete [] theBlock'.

void
MFileSetKeeper::GetProjectSets(
	MSetHolder&		outHolder)
{
	ListToSetBlock(fProjectSets, outHolder);
}

// ---------------------------------------------------------------------------
//		ReplaceProjectSets
// ---------------------------------------------------------------------------
//	Change the current project file set.  Does not take ownership of the block
//	passed in.  Pass nil to remove the project file sets.

void
MFileSetKeeper::ReplaceProjectSets(
	FileSetRec*		inSets)
{
	EmptyList(fProjectSets);

	if (inSets)
		SetBlockToList(inSets, fProjectSets);
}

// ---------------------------------------------------------------------------
//		RemoveFileSet
// ---------------------------------------------------------------------------
//	Remove the file set with the specified name.  return true if it's a
//	project file set.

bool
MFileSetKeeper::RemoveFileSet(
	const char*		inName)
{
	MFileSet*		fileSet = (MFileSet*) GetFileSet(inName);	// cast away const
	bool			isProjectSet = fProjectSets.IndexOf(fileSet) >= 0;

	if (fileSet)
	{
		if (isProjectSet)
			fProjectSets.RemoveItem(fileSet);
		else
		{
			fGlobalSets.RemoveItem(fileSet);
			WriteGlobalSets();
		}

		operator delete(fileSet->fRecs);
		delete fileSet;		
	}
	
	return isProjectSet;
}

// ---------------------------------------------------------------------------
//		EmptyList
// ---------------------------------------------------------------------------

void
MFileSetKeeper::EmptyList(
	FileSetList&	inList) const
{
	MFileSet*		fileSet;
	int32			i = 0;

	while (inList.GetNthItem(fileSet, i++))
	{
		operator delete(fileSet->fRecs);
		delete fileSet;
	}
	
	inList.MakeEmpty();
}

// ---------------------------------------------------------------------------
//		MSetHolder
// ---------------------------------------------------------------------------

MSetHolder::MSetHolder(
	void*	inSet,
	int32	inSize)
	: fSet(inSet), fSize(inSize)
{
}

// ---------------------------------------------------------------------------
//		MSetHolder
// ---------------------------------------------------------------------------

MSetHolder::~MSetHolder()
{
	::operator delete(fSet);
}

// ---------------------------------------------------------------------------
//		MSetHolder
// ---------------------------------------------------------------------------

void
MSetHolder::SetSet(
	void*	inSet,
	int32	inSize)
{
	fSet = inSet;
	fSize = inSize;
}


