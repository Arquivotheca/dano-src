// ---------------------------------------------------------------------------
/*
	IncludePathCache.cpp
	
	Copyright (c) 1998 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			18 September 1998

*/
// ---------------------------------------------------------------------------

#include "PlugIn.h"
#include "IncludePathCache.h"
#include "IDEConstants.h"

#include <Directory.h>
#include <Entry.h>
#include <String.h>
#include <Alert.h>
#include <Path.h>
#include <dirent.h>
#include <string.h>
#include <Debug.h>

// ---------------------------------------------------------------------------
// Utility functions (from Metrowerks MFileUtils.cpp)
// ---------------------------------------------------------------------------

bool
IsHiddenDirectory(const BEntry& inEntry)
{
	// Check for "("name")" directories
	FileNameT name;
	
	return (inEntry.GetName(name) == B_OK &&
			name[0] == '(' && 
			name[strlen(name) - 1] == ')');
}

// ---------------------------------------------------------------------------
//	Iterate over the contents of the directory passing BEntry objects to
//	the IteratorFunc.  The nodeflavors indicates whether either files,
//	directories, or both are to be passed to the iterator func.  If
//	traverselinks is true then the object pointed to by the link is passed
//	to the iterator func.  The symlinks themselves are never passed directly
//	to the iterator func.  If traverselinks is false then links are ignored.
// ---------------------------------------------------------------------------

status_t
IterateDirectory(
	BDirectory& 		inDir,
	MDirIteratorFunc&	inFunc,
	uint32				inNodeFlavors,		// some combo of B_FILE_NODE and B_DIRECTORY_NODE
	bool				inTraverseLinks)
{
	status_t		err = B_OK;
	BEntry			entry;
	dirent			dirents[2];
	int32			count;
	bool			doFiles = (inNodeFlavors & B_FILE_NODE) != 0;
	bool			doFolders = (inNodeFlavors & B_DIRECTORY_NODE) != 0;
	bool			more = true;

	ASSERT(doFiles || doFolders);	// need to look for at least one of them
	
	while (more && (count = inDir.GetNextDirents(dirents, sizeof(dirents))) > 0)
	{
		dirent*			ent = dirents;
		
		for (int i = 0; i < count; i++)
		{
			if (0 != strcmp(ent->d_name, ".") && 0 != strcmp(ent->d_name, ".."))
			{
				struct stat		rec;
				inDir.GetStatFor(ent->d_name, &rec);

				if (S_ISREG(rec.st_mode) && doFiles)	// regular file
				{
					err = entry.SetTo(&inDir, ent->d_name);
					more = inFunc.ProcessItem(entry, B_FILE_NODE, inDir);
				}
				else
				if (S_ISDIR(rec.st_mode) && doFolders)	// directory
				{
					err = entry.SetTo(&inDir, ent->d_name);
					more = inFunc.ProcessItem(entry, B_DIRECTORY_NODE, inDir);
				}
				else
				if (inTraverseLinks && S_ISLNK(rec.st_mode))		// symlink
				{
					err = entry.SetTo(&inDir, ent->d_name, true);	// traverse
					
					if (err == B_OK)
					{
						if (entry.IsFile() && doFiles)
						{
							more = inFunc.ProcessItem(entry, B_FILE_NODE, inDir);
						}
						else
						if (entry.IsDirectory() && doFolders)
						{
							more = inFunc.ProcessItem(entry, B_DIRECTORY_NODE, inDir);
						}
					}
				}
			}
		
			ent = (dirent*)((char*)ent + ent->d_reclen);
		}
	}
	
	return err;
}

// ---------------------------------------------------------------------------
//	Class IncludePathCache
// ---------------------------------------------------------------------------

IncludePathCache::IncludePathCache(const char* includePathPrefix)
				 : BList()
{
	fLevel = 0;
	fIterateCount = 0;
	fIncludePathPrefix = new char[strlen(includePathPrefix) + 1];
	strcpy(fIncludePathPrefix, includePathPrefix);
}

// ---------------------------------------------------------------------------

IncludePathCache::~IncludePathCache()
{
#ifdef _GCCDEBUG_
	fprintf(stderr, "IncludePathCache::~IncludePathCache: fLevel = %d\n", fLevel);
	ASSERT_WITH_MESSAGE(fLevel == 0, "IncludePathCache::~IncludePathCache - level not zero!");
#endif

	// delete all the items that we have adopted
	IncludePathCache::FlushCache();
	
	delete [] fIncludePathPrefix;
}

// ---------------------------------------------------------------------------

void
IncludePathCache::FlushCache()
{
	// iterate through our list and delete all the items
	
	char* aString;
	int numItems = this->CountItems();
	for (int32 i = 0; i < numItems; i++) {
		aString = (char*) this->ItemAtFast(i);
		delete [] aString;
	}
	
	this->MakeEmpty();
}

// ---------------------------------------------------------------------------

// The number of directories that can be added from one recursive directory
// This keeps the time sane while iterating a massive base directory.  The user
// really wants to go change this anyway, so this allows them to do it quicker
const int32 kMaxDirectoriesAllowed = 256;

void
IncludePathCache::AddADirectory(BDirectory& dir, bool recursiveDir)
{
	// Externally visible covering to AddDirectory
	// keep track of number of directories added because of this one directory
	// once we reach a limit, we quit...
	
	fIterateCount = 0;
	try {
		this->AddDirectory(dir, recursiveDir);
	}
	catch (...) {
		// In the process of adding our current directory, we have iterated
		// over too many directories.  Tell the user
		BPath aPath(&dir, NULL);
		BString message = aPath.Path();
		message += " contains too many include directories.  Project may not build correctly.  Use Project Settings/Access Paths to narrow the scope of the access paths.";
		BAlert* tooManyIncludes = new BAlert("toomanyincludes", 
											 message.String(), 
											 "OK", 
											 NULL, 
											 NULL, 
											 B_WIDTH_AS_USUAL, 
											 B_STOP_ALERT);
		tooManyIncludes->Go();
	}
}

// ---------------------------------------------------------------------------

void
IncludePathCache::AddDirectory(BDirectory& dir, bool recursiveDir)
{
#ifdef _GCCDEBUG_
	fprintf(stderr, "IncludePathCache::AddDirectory\n");
#endif

	// Combined -I + path to create the argument string to cache
	BPath aPath(&dir, NULL);
	if (aPath.InitCheck() != B_OK) {
		throw 0;						// maybe out of FDs, pull out
	}
	char* includePathString = new char[strlen(aPath.Path()) + strlen(fIncludePathPrefix) + 1];
	strcpy(includePathString, fIncludePathPrefix);
	strcat(includePathString, aPath.Path());

#ifdef _GCCDEBUG_
	fprintf(stderr, "IncludePathCache::AddDirectory: adding %s\n", includePathString);
#endif

	// we adopt each string - deleted when flushed or destructed
	this->AddItem(includePathString);

	if (++fIterateCount > kMaxDirectoriesAllowed) {
		throw fIterateCount;
	}
	
	// Now continue adding sub-directories if we have a recursive directory 
	if (recursiveDir && fLevel < 16) {
		fLevel++;
#ifdef _GCCDEBUG_
	fprintf(stderr, "IncludePathCache::AddDirectory: recursing with level = %d\n", fLevel);
#endif
		IterateDirectory(dir, *this, B_DIRECTORY_NODE, false);
		fLevel--;
	}
}


// ---------------------------------------------------------------------------

bool
IncludePathCache::ProcessItem(BEntry& inEntry, node_flavor inFlavor, const BDirectory& inDir)
{
	// This method is called from IterateDirectory
	// Just turn around and call our AddDirectory method...

	// Add myself to the cache and recurse inside myself
	// (but don't do it for any "(Objects)" directories
	if (::IsHiddenDirectory(inEntry) == false) {
		BDirectory dir(&inEntry);
		if (dir.InitCheck() != B_OK) {
			throw 0;					// wow, maybe out of FDs - pull out
		}
		this->AddDirectory(dir, true);
	}
	
	// always continue iteration
	return true;
}

// ---------------------------------------------------------------------------

void
IncludePathCache::FillArgvList(BList& inArgv)
{
#ifdef _GCCDEBUG_
	fprintf(stderr, "IncludePathCache::FillArgvList\n");
#endif

	// Iterate over our cache of include paths and build up one
	// that will be owned by the IDE
	
	long listCount = this->CountItems();
	for (int i = 0; i < listCount; i++) {
		char* anArg = (char*) this->ItemAt(i);
		inArgv.AddItem(strdup(anArg));
	}
}
