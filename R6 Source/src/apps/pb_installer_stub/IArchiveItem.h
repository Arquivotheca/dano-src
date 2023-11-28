#ifndef _IARCHIVEITEM_H_
#define _IARCHIVEITEM_H_

#include <Directory.h>
#include <Entry.h>
#include <SupportDefs.h>

#include "SimpleListView.h"  // for definition of listItem
#include "RList.h"

class ArchiveFolderItem;
class ArchiveFileItem;
class ArchivePatchItem;
class InstallPack;
class DestManager;
class PTreeItem;
class TreeItem;


status_t RenameExisting(BDirectory *destDir,BEntry *,
						char *postfix = ".old");
long SetCreationTime(BEntry *fi,long time);
long SetVersion(BEntry *fi,long vers);
void ParseVersion(ulong version, char *buf);
long GetVersion(BEntry *);


class ArchiveItem :  public ListItem
{
public:
					ArchiveItem();		
	virtual			~ArchiveItem();
	virtual long 	ExtractItem(InstallPack *archiveFile, 
									BDirectory *parDir, 
									DestManager *dests, 
									ulong inGrps,
									bool skip) = 0;			
	virtual bool	HandleExisting(BDirectory *destDir, BEntry *existingItem, 
								bool &skip, DestManager *dests,
								long *retErr);
	virtual bool	IsFolder() = 0;
	inline	void	SetParent(ArchiveFolderItem *p);
	inline	ArchiveFolderItem 	*GetParent();
			void	ISetType(BEntry *entry);
			
	virtual void	Unset();
			
	ulong			groups;
	long			destination;
	bool			customDest;
	long			replacement;
		
	off_t			uncompressedSize;	
	long			creationTime;
	long			modTime;
	int32			platform;
	
	off_t			seekOffset;
	mode_t			mode;

	char			*name;
	int32			versionInfo[5];
private:
	// might need this only on folders, check it out
	ArchiveFolderItem *parentFolder;
};

inline	void ArchiveItem::SetParent(ArchiveFolderItem *p)
{
	parentFolder = p;
}

inline	ArchiveFolderItem *ArchiveItem::GetParent()
{
	return parentFolder;
}

/***
inline	void ArchiveItem::SetName(const char *value)
{
	delete[] name;
	int len = strlen(value) + 1;
	name = new char[len];
	memcpy(name, value, len);
}
********/

class ArchiveFolderItem : public ArchiveItem {
	// this constructor will fill in the appropriate fields
	// as if this is an *empty* folder
	// the folder will be filled in asynchronously
	// and then compressed asynchrnously

public:
	// when creating from archive
					ArchiveFolderItem();
	virtual			~ArchiveFolderItem();
	virtual bool 	IsFolder();
	virtual long 	ExtractItem(InstallPack *archiveFile, 
									BDirectory *parDir, 
									DestManager *dests, 
									ulong inGrps,
									bool skip);
	virtual bool	HandleExisting(BDirectory *destDir, BEntry *existingItem, 
								bool &skip, DestManager *dests,
								long *retErr);
	void			GetGroupInfo(ulong inGrps, long &iCount, long &bytes,long blockSz);
	void			InitDestinations( BDirectory *parDir, 
									DestManager *dests, 
									ulong inGrps,
									RList<ArchivePatchItem *> *plist);
	void			BuildDisplay(	PTreeItem *parItem,																		
									DestManager *dests, 
									ulong inGrps,
									TreeItem *root );
	virtual void	Unset();
	// initially zero, filled in recursively
	//long			numCompressibleItems;
	// number of entries in the list (includes folders)
	//long			numEntries;
	//RList<ArchiveItem *>	 *entries;
	//inline void		AddEntry(ArchiveItem *entry);
	bool			isTopDir;
	BDirectory		*newDir;
};


class ArchiveFileItem : public ArchiveItem {
public:
					ArchiveFileItem();
	virtual			~ArchiveFileItem();
	virtual bool 	IsFolder();
	virtual long 	ExtractItem(InstallPack *archiveFile, 
									BDirectory *parDir, 
									DestManager *dests, 
									ulong inGrps,
									bool skip);
	void			ISetType(BEntry *entry);
	char			*fType;
};

class ArchiveLinkItem : public ArchiveItem
{
public:
					ArchiveLinkItem();
	virtual			~ArchiveLinkItem();
	virtual bool 	IsFolder();
	virtual long 	ExtractItem(InstallPack *archiveFile, 
									BDirectory *parDir, 
									DestManager *dests, 
									ulong inGrps,
									bool skip);
	char			*linkPath;
};


/*********************
	Error messages
*********************/

#define errNOUNIQNAME	"Could not create a unique file name!"
// bad error!
#define errBADREPLOPT	"Unrecognized replacement option encountered."
#define errNODESTDIR	"Destination directory for \"%s\" could not be found."
#define errMKFILE		"Error creating new file \"%s\"."
#define errMKLINK		"Error creating symbolic link \"%s\"."
#define errDECOMPRESS	"Encountered an error when decompressing the file \"%s\"."
#define errCHKSUM		"Warning, checksums do not not match. The file \"%s\" may be corrupt."
#define errBADSIZE		"Warning, input and output file sizes do not match. The file \"%s\" may be corrupt."

#endif
