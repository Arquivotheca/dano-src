/* ++++++++++
	FILE:		walker.h
	Written By:	Peter Potrebic
	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.
+++++ */

#ifndef WALKER_H
#define WALKER_H

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif
#include <VolumeRoster.h>
#include <Volume.h>
#include <List.h>
#include <EntryList.h>
#include <Directory.h>
#include <Entry.h>
#include <Query.h>

/* ------------------------------------------------------------------- */

class TWalker : public BEntryList {
	// adds a virtual destructor that is severely missing in BEntryList
	// BEntryList should never be used polymorfically because of that

public:
virtual					~TWalker();

virtual	status_t		GetNextEntry(BEntry *, bool traverse = false) = 0;
virtual	status_t		GetNextRef(entry_ref *) = 0;
virtual	int32			GetNextDirents(struct dirent *, size_t,
							int32 count = INT_MAX) = 0;
virtual	status_t		Rewind() = 0;
virtual	int32			CountEntries() = 0;

};

/* ------------------------------------------------------------------- */

class TNodeWalker : public TWalker {
// TNodeWalker supports iterating a single volume, starting from a specified
// entry; if passed a non-directory entry it returns just that one entry
public:
						TNodeWalker(bool include_top_dir);
						TNodeWalker(const char *path, bool include_top_dir);
						TNodeWalker(const entry_ref *ref, bool include_top_dir);
						TNodeWalker(const BDirectory *dir, bool include_top_dir);
virtual					~TNodeWalker();

// Backwards compatibility with Tracker compiled for R5 (remove when this
// gets integrated into the official release).
						TNodeWalker();
						TNodeWalker(const char *path);
						TNodeWalker(const entry_ref *ref);
						TNodeWalker(const BDirectory *dir);
						
virtual	status_t		GetNextEntry(BEntry *, bool traverse = false);
virtual	status_t		GetNextRef(entry_ref *);
virtual	int32			GetNextDirents(struct dirent *, size_t,
							int32 count = INT_MAX);
virtual	status_t		Rewind();

protected:
		status_t		PopDirCommon();
		void			PushDirCommon(const entry_ref *);

private:
virtual	int32			CountEntries();
					// don't know how to do that, have just a fake stub here

protected:
		BList			fDirs;
		int32			fTopIndex;
		BDirectory		*fTopDir;
		bool			fIncludeTopDir;
		bool			fOriginalIncludeTopDir;
		
private:		
		BEntry			*fJustFile;
		BDirectory		fOriginalDirCopy;
		BEntry			*fOriginalJustFile;
						// keep around to support Rewind
};

/* ------------------------------------------------------------------- */

class TVolWalker : public TNodeWalker {
// TNodeWalker supports iterating over all the mounted volumes;
// non-attribute and read-only volumes may optionaly be filtered out
public:
						TVolWalker(bool knows_attr = true,
									bool writable = true,
									bool include_top_dir = true);
virtual					~TVolWalker();

virtual	status_t		GetNextEntry(BEntry *, bool traverse = false);
virtual	status_t		GetNextRef(entry_ref *);
virtual	int32			GetNextDirents(struct dirent *, size_t,
							int32 count = INT_MAX);
virtual	status_t		Rewind();

virtual	status_t		NextVolume();
						// skips to the next volume
						// Note: it would be cool to return const BVolume *
						// that way a subclass could implement a volume filter -
						// it would just override, call inherited for as long as there
						// are volumes and it does not like them
						// we would have to give up the status_t then, which might be
						// ok

private:


		BVolumeRoster	fVolRoster;
		BVolume			fVol;
		bool			fKnowsAttr;
		bool			fWritable;
		
typedef	TNodeWalker		_inherited;
};

/* ------------------------------------------------------------------- */

class TQueryWalker : public TWalker {
public:
						TQueryWalker(const char *predicate);
virtual					~TQueryWalker();

						// Does an in-fix walk of all entries
virtual	status_t		GetNextEntry(BEntry *, bool traverse = false);
virtual	status_t		GetNextRef(entry_ref *);
virtual	int32			GetNextDirents(struct dirent *, size_t,
							int32 count = INT_MAX);

virtual	status_t		NextVolume();
						// skips to the next volume
virtual	status_t		Rewind();

private:
virtual	int32			CountEntries();
						// can't count

		BQuery			fQuery;
		BVolumeRoster	fVolRoster;
		BVolume			fVol;
		bigtime_t		fTime;
		const char		*fPredicate;

typedef	TQueryWalker	_inherited;
};

#endif
