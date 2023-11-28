
#include "FListView.h"
#include "RList.h"

#ifndef _FARCHIVEITEM_H
#define _FARCHIVEITEM_H

// if current view folder == folder changed then update view item

class ArchiveFolderItem;
class ArchiveFileItem;
class PackArc;



class ArchiveItem : public ListItem {
public:
					ArchiveItem();
					ArchiveItem(BEntry &r);
virtual				~ArchiveItem();
virtual long 		CompressItem(PackArc *archiveFile) = 0;
virtual long 		ExtractItem(PackArc *archiveFile,
						BDirectory *destDir) = 0;
						
virtual void 		BuildCompressTree();
virtual ulong 		GetUnionGroups(ulong bits);
virtual void		SetGroups(ulong bits,bool mark);
virtual bool 		IsFolder();

inline	void 				SetParent(ArchiveFolderItem *p)
							{
								parentFolder = p;
							}
inline	ArchiveFolderItem	*GetParent()
							{
								return parentFolder;
							}
	
	entry_ref	fRef;
	BBitmap*	smallIcon;

	char		*name;	
	ulong		groups;
	int32		destination;
	bool		customDest;
	int32		replacement;
	int32		platform;
	//int			conditional;
	//int			condition;
			
	off_t		compressedSize;
	off_t		uncompressedSize;
	off_t		seekOffset;
	bool		dirty;
	
// marked for deletion
	bool		deleteThis;

	int32		creationTime;
	int32		modTime;
	int32		versionInfo[5];
	
	// original location
	char		*srcPath;
private:
	ArchiveFolderItem *parentFolder;
	void		Init();
};



class ArchiveFolderItem : public ArchiveItem {
	// this constructor will fill in the appropriate fields
	// as if this is an *empty* folder
	// the folder will be filled in asynchronously
	// and then compressed asynchrnously

public:
	// when creating from archive
					ArchiveFolderItem();
					ArchiveFolderItem(BEntry &f, struct stat *);
virtual				~ArchiveFolderItem();
virtual	bool 		IsFolder();
inline	void		AddEntry(ArchiveItem *entry);
	
		void		UpdateRemovedEntry(ArchiveItem *entry);
		   
virtual	long		CompressItem(PackArc *archiveFile);
virtual	long 		ExtractItem(PackArc *archiveFile,BDirectory *destDir);
		void		GatherEntries();
virtual	void		BuildCompressTree();
virtual ulong		GetUnionGroups(ulong bits);
		long		CountDirty(long &bytes);
		void		GetCleanFiles(RList<ArchiveItem *> *list, off_t minOffset);
		void		GroupDeleted(ulong lowM, ulong highM);
virtual	void		SetGroups(ulong bits,bool mark);
		void		DestDeleted(long old);
		void		DestMoved(long old,long now);
	
	off_t			dataCompressedSize;
	off_t			dataUncompressedSize;
	// initially zero, filled in recursively
	long					numCompressibleItems;
	// number of entries in the list (includes folders)
	//long					numEntries;
	bool					canEnter;
	RList<ArchiveItem *>	*entries;
	
	// a copy of the tree when executing compression
	RList<ArchiveItem *>	*compressEntries;
};

inline void ArchiveFolderItem::AddEntry(ArchiveItem *entry)
{
	entries->AddItem(entry);
}






class ArchiveFileItem : public ArchiveItem {
	// when creating from archive
public:
					ArchiveFileItem();
					ArchiveFileItem(BEntry &f, struct stat *stBuf = NULL);
virtual				~ArchiveFileItem();
virtual	long 		CompressItem(PackArc *archiveFile);
virtual	long 		ExtractItem(PackArc *archiveFile,BDirectory *destDir);
		void		CheckDepends();
		
RList<char *>		*DependList();
	
		mode_t		mode;
		char		*fType;
		char		*fSignature;	// only for apps
private:
	RList<char *>	*fDependList;
};

class ArchiveLinkItem : public ArchiveItem
{
public:
					ArchiveLinkItem();
					ArchiveLinkItem(BEntry &f, struct stat *stBuf = NULL);
virtual				~ArchiveLinkItem();
virtual long		CompressItem(PackArc *archiveFile);
virtual long		ExtractItem(PackArc *arhiveFile, BDirectory *destDir);
	char			*fLinkPath;
};
#endif
