// DestManager.h
#ifndef _DESTMANAGER_H_
#define _DESTMANAGER_H_

#include "RList.h"
#include "DestinationList.h"
#include "PackData.h"
#include <Directory.h>
#include <File.h>


class ArchiveFolderItem;
class ArchivePatchItem;
class PackAttrib;
class PTreeItem;
class TreeItem;
class REntryList;
class PackageItem;

typedef struct {
	// is it a folder or not
	bool		fold;
	// original location
	entry_ref	dirRef;
	// record ref of item
	entry_ref	ref;
} DeleteItem;

class DestManager
{
public:
	DestManager(PackAttrib *_attr);
	~DestManager();
	
	void					SetInstallDir(BEntry *entry);		
	void					ClearDestinationLists();
	bool					SetupInstall( entry_ref *instDirRef, char *instDirName,
								long installVolID, ArchiveFolderItem *top,
								bool _lowMem);
								
	void					FinishInstall(bool cancel, PackageItem *it = NULL);
	BDirectory				*DirectoryFor(long index, BDirectory *parent,
								bool custom = FALSE,
								bool create = TRUE,
								ArchivePatchItem *patch = NULL);
								
	PTreeItem				*TreeItemFor(int32 destIndex,
									PTreeItem *parentItem,
									TreeItem *root,
									bool custom);
	bool					InstallPlatform(int32 platform);
	status_t				BeginRememberList();						
	status_t				RememberFile(BEntry *file,
										BEntry *oldFile);
	
	status_t				FetchQuery(FindItem *criteria,
								REntryList *result);

	BFile					*LogFile();
	void					SafeDelete(BEntry *s,BDirectory *);
	ArchiveFolderItem *		TopFolder();
	
	PTreeItem				*instFolderTreeItem;
	
	inline BDirectory		*TempDir() {
		return &tempDir;
	};
	long					AllocSize(long sz);
	long					AllocExtra(long sz);
	
	
	bool				deleteOriginals;
	bool				doInstallFolder;
	
	bool				logging;
	BDirectory			logDir;
	
	bool				lowMemMode;
	bool				canceled;
	
	short				replCurrent;
	bool				replApplyToAll;
	
	PackAttrib			*attr;
	
	char				*fileTag;
	int32				tagLen;
	
	dev_t				installDev;
private:
	RList<DestItem *>	*fDefList;
	RList<DestItem *>	*fCustList;
	
	char				*dirName;
	BDirectory			*fInstall;
	
	BDirectory			rootDir;
	BDirectory			tempDir;
	BDirectory			foundDir;
	RList<DeleteItem *>	*deletia;
	ArchiveFolderItem 	*fTop;
	BFile				*fLogFile;
	BFile				fNewFiles;
	PackData			fPackData;
	
	long				encloseFolderCount;
	long				allocBlockSize;
};

inline ArchiveFolderItem *DestManager::TopFolder() {
	return fTop;
};


long RecursiveDelete(BEntry *dir);

long CreatePathAt(const BDirectory *dir,const char *path,BEntry *final);
void WritePath(BDirectory *dir,BFile *output,char *buf);

enum {
	askUserREPLACE = 2, askUserRENAME = 1, askUserSKIP = 0
};

#define LOG_FILETYPE "text/plain"

/*******************
	Error messages
*******************/

///////// in DestManager.cpp
#define errMKTMPDIR		"Error creating temporary directory on volume \"%s\"."
#define errCANTPUTAWAY	"Error restoring item \"%s\" to original location after cancelled install."
#define errRMTMPDIR		"Some items could not be removed from the temporary directory on volume \"%s\"."
#define errMKLOGFILE	"File system error creating log file. Logging has been disabled."
#define errBADDESTLIST	"Destination list index out of range. This package may be corrupt."
// this is a bad bug
#define errMKCUSTPATH	"Error creating custom pathname \"%s\"."
// mark so not-retried
#define errMOVETOTEMP	"Error moving item \"%s\" to temporary directory."
// file or folder?
#define errINSTDIR		"The selected installation directory \"%s\" could not be created."


#endif
