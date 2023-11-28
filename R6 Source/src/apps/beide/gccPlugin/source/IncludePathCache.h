// ---------------------------------------------------------------------------
/*
	IncludePathCache.h
	
	Copyright (c) 1998 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			15 October 1998

	IncludePathCache keeps track of all the include directories
	We only modify the cache when notified that the preferences have changed
	since we don't want to have to do this on every compile.
	
	Internally, the cache keeps a BList of char* witht the full
	"-I<fullpath>".  I do this so that I don't have to worry about directory
	changes inbetween cache and use.
	
	The IncludePathCache doubles as an MDirIteratorFunct internally
	so that it has the protocol for the call back for each directory
	during an IterateDirectory.
	
	From the outside, the GCCBuilder just knows that it can throw in
	directories with AddDirectory, clear everything with FlushCache
	and fill up an argv list with FillArgvList.

*/
// ---------------------------------------------------------------------------

#ifndef _INCLUDEPATHCACHE_H
#define _INCLUDEPATHCACHE_H

#include <StorageDefs.h>
#include <List.h>

class BDirectory;
class BEntry;

// ---------------------------------------------------------------------------
//	Class MDirIteratorFunc
// ---------------------------------------------------------------------------

class MDirIteratorFunc
{
public:
	
					MDirIteratorFunc() {}
	virtual			~MDirIteratorFunc() {}
	
	// return true to continue the iteration
	virtual bool	ProcessItem(BEntry& inEntry,
								node_flavor	inFlavor,
								const BDirectory& inDir) = 0;
};

// ---------------------------------------------------------------------------
//	Class IncludePathCache
// ---------------------------------------------------------------------------

class IncludePathCache : public BList, private MDirIteratorFunc
{
public:
					IncludePathCache(const char* includePathPrefix);
	virtual			~IncludePathCache();

	void			FlushCache();
	void			FillArgvList(BList& inArgv);

	void			AddADirectory(BDirectory& dir, bool recursiveDir);
		
	// return true to continue the iteration
	virtual bool	ProcessItem(BEntry& inEntry, node_flavor inFlavor, const BDirectory& inDir);

private:
	void			AddDirectory(BDirectory& dir, bool recursiveDir);

	char*			fIncludePathPrefix;

	// valid only during iteration 
	long			fLevel;			// -- used to keep the depth of search sane
	long			fIterateCount;	// -- used to keep the breadth of search sane
};

#endif
