/*	$Id: DSourceFileTable.cpp,v 1.1 1999/05/11 21:31:43 maarten Exp $
	
	Copyright Hekkelman Programmatuur b.v.
	Maarten L. Hekkelman
	
	Created: 05/06/99 20:29:37
*/

#include "bdb.h"
#include "DSourceFileTable.h"

#include <Path.h>
#include <Directory.h>
#include <algorithm>

DSourceFileTable *DSourceFileTable::sfInstance;

DSourceFileTable::DSourceFileTable()
{
} // DSourceFileTable::DSourceFileTable

DSourceFileTable::~DSourceFileTable()
{
} // DSourceFileTable::~DSourceFileTable

DSourceFileTable& DSourceFileTable::Instance()
{
	if (sfInstance == NULL)
		sfInstance = new DSourceFileTable;

	return *sfInstance;
} // DSourceFileTable::Instance

class DFileFinder
{
  public:
	DFileFinder(const entry_ref& ref) : _ref(ref) {}
	bool operator () (const DSourceFileTable::DFileInfo& fi)
		{ return fi.ref == _ref; }
	const entry_ref& _ref;
};

DFileNr DSourceFileTable::AddFile(const entry_ref& ref)
{
	std::vector<DFileInfo>::iterator i;
	DFileNr result;
	
	i = find_if(fFiles.begin(), fFiles.end(), DFileFinder(ref));
	if (i == fFiles.end())
	{
		DFileInfo fi;
		BEntry e(&ref, true);
		BPath p;
		
		fi.ignore = false;
		fi.exists = e.Exists();
		
		if (! fi.exists && e.GetPath(&p) == B_OK)
			fi.ref.set_name(p.Path());
		else
			fi.ref = ref;
		
		result = fFiles.size();
		fFiles.push_back(fi);
	}
	else
		result = i - fFiles.begin();
	
	return result;
} // DSourceFileTable::AddFile

DFileNr DSourceFileTable::AddFile(const string& comp_dir, const char *path)
{
	entry_ref ref;
	status_t err;

	string f;
	
	if (path[0] == '/')
	{
		err = get_ref_for_path(path, &ref);
		if (err != B_OK)
			f = path;
	}
	else
	{
		BEntry e;

		f = comp_dir + '/' + path;

		err = e.SetTo(f.c_str(), true);
		if (err == B_OK)
			err = e.GetRef(&ref);
	}

	if (err != B_OK)
		ref.set_name(f.c_str());
	
	return AddFile(ref);
} // DSourceFileTable::AddFile

void DSourceFileTable::AddIncludePath(const char *dir)
{
	entry_ref ref;

	if (get_ref_for_path(dir, &ref) == B_OK)
		fPaths.push_back(ref);
} // DSourceFileTable::AddIncludePath

void DSourceFileTable::LocatedFile(DFileNr nr, const entry_ref& ref)
{
	char *d = strdup(fFiles[nr].ref.name);

	fFiles[nr].ref = ref;

	BEntry e;
	FailOSErr(e.SetTo(&ref, true));

	if (fFiles[nr].exists = e.Exists())
	{
		try
		{
			// we've got a new path, add it for future use
			BPath p;
	
			FailOSErr(e.GetParent(&e));
			FailOSErr(e.GetPath(&p));
			AddIncludePath(p.Path());
			
			// now lets see if we have more files inside our file table that are not
			// located and start with the same path prefix..
			
			char *t = strrchr(d, '/');
			if (t)
			{
				*t = 0;
				int l = strlen(d);
				
				BDirectory dir(&e);
				
				for (std::vector<DFileInfo>::iterator i = fFiles.begin(); i != fFiles.end(); i++)
				{
					if ((*i).exists)
						continue;
					
					t = (*i).ref.name + l + 1;
					
					if (strncmp(d, (*i).ref.name, l) == 0 && dir.Contains(t))
					{
						FailOSErr(dir.FindEntry(t, &e, true));
						FailOSErr(e.GetRef(&(*i).ref));
						(*i).exists = e.Exists();
						(*i).ignore = false;
					}
				}
			}
		}
		catch (HErr& e)
		{
			// silently ignore errors here
		}
	}
	
	if (d)
		free(d);
} // DSourceFileTable::LocatedFile

string DSourceFileTable::Basename(DFileNr nr) const
{
	ASSERT_OR_THROW (nr >= 0 && nr < (int) fFiles.size());

	const char *r = strrchr(fFiles[nr].ref.name, '/');
	
	if (r == NULL)
		r = fFiles[nr].ref.name;
	
	return r;
} // DSourceFileTable::Basename

string DSourceFileTable::Path(DFileNr nr) const
{
	ASSERT_OR_THROW (nr >= 0 && nr < (int) fFiles.size());

	BPath p;
	FailOSErr(BEntry(&fFiles[nr].ref).GetPath(&p));
	return p.Path();
} // DSourceFileTable::Path

bool DSourceFileTable::Located(DFileNr nr) const
{
	if (nr >= fFiles.size())
		THROW(("Source file not available"));
	return fFiles[nr].exists;
} // DSourceFileTable::Located

void DSourceFileTable::Locate(DFileNr nr)
{
	entry_ref& ref = fFiles[nr].ref;
	
	BDirectory dir;
	
	for (std::vector<entry_ref>::iterator i = fPaths.begin(); i != fPaths.end(); i++)
	{
		if (dir.SetTo(&(*i)) != B_OK)
			continue;
		
		if (dir.Contains(ref.name, B_FILE_NODE | B_SYMLINK_NODE))
		{
			BEntry e;
			FailOSErr(dir.FindEntry(ref.name, &e, true));
			FailOSErr(e.GetRef(&ref));
			fFiles[nr].exists = e.Exists();
			
			return;
		}
	}
} // DSourceFileTable::Locate

void DSourceFileTable::IgnoreFile(DFileNr nr)
{
	fFiles[nr].ignore = true;
} // DSourceFileTable::IgnoreFile

bool DSourceFileTable::Ignore(DFileNr nr) const
{
	return fFiles[nr].ignore;
} // DSourceFileTable::Ignore
