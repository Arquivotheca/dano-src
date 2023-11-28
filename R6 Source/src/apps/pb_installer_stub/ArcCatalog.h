#ifndef _ARCCATALOG_H_
#define _ARCCATALOG_H_

#include <File.h>

class SPackData;

class ArchiveItem;
class ArchiveFolderItem;
class ArchiveFileItem;
class ArchivePatchItem;
class ArchiveScriptItem;
class ArchiveLinkItem;

class InstallPack;


class ArcCatalog
{
public:

	ArcCatalog();
	~ArcCatalog();
	
	status_t	Set(const InstallPack *pack);
	status_t	Unset();
	status_t	Rewind();
	

	void		GetFolderData(ArchiveItem **item);
	
	status_t	StartFolderGet();
	status_t	EndFolderGet();
	
	int32		GetFolderItemType();
	
	void	GetItemData(ArchiveItem **item, int32 type);
	
	void	GetFileData(ArchiveItem **item);
	void	GetPatchData(ArchiveItem **item);
	void	GetScriptData(ArchiveItem **item);
	void	GetLinkData(ArchiveItem **item);
private:
	ArchiveFileItem		*newFile;
	ArchiveFolderItem	*newFolder;
	ArchivePatchItem	*newPatch;
	ArchiveScriptItem	*newScript;
	ArchiveLinkItem		*newLink;

	BFile		*fArcFile;
	SPackData	*fPackData;
	
	off_t		fOffset;
};


enum {
	FILE_FLAG = 0x0001,
	FOLDER_FLAG = 0x0002,
	PATCH_FLAG = 0x0003,
	SCRIPT_FLAG = 0x0004,
	LINK_FLAG = 0x0005
};

#endif
