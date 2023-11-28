// InstallLooper.h
#ifndef _INSTALLLOOPER_H_
#define _INSTALLLOOPER_H_

#include <Looper.h>
#include <Message.h>
#include <Messenger.h>

#include "IStatusWindow.h"
#include "RList.h"

#define STATUS_FREQUENCY 50.0

class InstallPack;
class ArchiveFileItem;
class ArchiveFolderItem;
class ArchivePatchItem;
class DestManager;
class PackAttrib;
class ArcCatalog;

class PackageItem;
class PTreeItem;
class TreeItem;

class InstallLooper : public BLooper {
private:
	StatusWindow				*statWindow;
	InstallPack					*arcFile;
	ArcCatalog					*fCatalog;
	
	uint32						installFolderGroups;
	uint32						totalBytes;
	
	int32						currentPatchCount;
	RList<ArchivePatchItem *>	*patchList;
	
	void					DoExtractItems(BMessage *msg);
	long					InstallEntries(BDirectory *parDir,
										DestManager *dests,
										ulong inGrps, bool topDir = false, bool skip = false);
	void					FindPatches( BDirectory *parDir, DestManager *dests,
									ulong inGrps,
									bool topDir = FALSE);
	void					DoPreview(DestManager *destObject,
								uint32 chosenGroups,
								bool &cancelInst);
	void					InitInstall();
	void					FinishInstall();
#if (!SEA_INSTALLER)
	long 					BuildDisplay(PTreeItem *parItem,
									 DestManager *dests,
									 ulong inGrps,
									 TreeItem *root,
									 bool topDir );
	PackageItem				*AddToRegistry(PackAttrib *attr, dev_t device);
#endif
	
public:
							InstallLooper(InstallPack *arc);
	virtual thread_id 		Run();
	virtual bool 			QuitRequested();
	virtual void 			MessageReceived(BMessage *msg);

	BMessenger				updateMessenger;
};

/*********************
	Error messages
*********************/

/////// in InstallLooper.cpp
#define errBADSTATWIND	"Error closing status window!"
#define errNOPATCHES	"Cannot install because files to be patched were not found."
#define errBADTOPLEVEL	"Unkown installation mode encountered (the top level folder was not found)."


#endif
