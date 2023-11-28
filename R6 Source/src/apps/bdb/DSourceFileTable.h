/*	$Id: DSourceFileTable.h,v 1.1 1999/05/11 21:31:43 maarten Exp $
	
	Copyright Hekkelman Programmatuur b.v.
	Maarten L. Hekkelman
	
	Created: 05/06/99 20:24:44
*/

#ifndef DSOURCEFILETABLE_H
#define DSOURCEFILETABLE_H

#include "HError.h"

#include <Entry.h>

#include <vector>
#include <string>

typedef int DFileNr;
using std::string;

class DSourceFileTable
{
	friend class DFileFinder;

	DSourceFileTable();
	virtual ~DSourceFileTable();

  public:
	
		// This class is a Singleton
	static DSourceFileTable& Instance();
	
		// access to entry_refs
	const entry_ref& operator[] (DFileNr ix);
	
	string Basename(DFileNr nr) const;
	string Path(DFileNr nr) const;
	bool Located(DFileNr) const;
	bool Ignore(DFileNr) const;

		// Call AddFile to add a new file or to find the DFileNr for an existing file
		// Two ways to do that:
	DFileNr AddFile(const entry_ref& ref);
	DFileNr AddFile(const string& comp_dir, const char *path);
	
	void AddIncludePath(const char *dir);
	
		// If a file was not found, and the user located it, call this member
		// to notify the SourceFileTable of this new info. SourceFileTable
		// will then try to identify other unfound files as well using the new
		// path information.
	void LocatedFile(DFileNr nr, const entry_ref& ref);
		
		// if user pressed cancel, then we ignore this file from now on.
	void IgnoreFile(DFileNr nr);
	
  private:
	
	void Locate(DFileNr nr);
  	
  	static DSourceFileTable *sfInstance;

	struct DFileInfo
	{
		entry_ref ref;
		bool exists;
		bool ignore;
	};
  	
	std::vector<DFileInfo> fFiles;
	std::vector<entry_ref> fPaths;		// paths indicated by user
};

inline const entry_ref& DSourceFileTable::operator[] (DFileNr ix)
{
	ASSERT_OR_THROW (ix >= 0 && (size_t) ix < fFiles.size());
	
	if (!fFiles[ix].exists)
		Locate(ix);
	
	return fFiles[ix].ref;
} // DSourceFileTable::operator[]

#endif
