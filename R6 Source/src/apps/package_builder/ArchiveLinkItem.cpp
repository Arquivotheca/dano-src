#include <Be.h>
// ArchiveLinkItem.cpp

#include "FArchiveItem.h"
#include "PackArc.h"

#include "GlobalIcons.h"
#include "PackMessages.h"
#include "DestinationList.h"

#include "Replace.h"
#include "ExtractUtils.h"
#include <Path.h>

#include "Util.h"
#define MODULE_DEBUG 0
#include "MyDebug.h"

ArchiveLinkItem::ArchiveLinkItem()
	: ArchiveItem()
{
	fLinkPath = NULL;
	smallIcon = gGenericLinkIcon;
}

ArchiveLinkItem::ArchiveLinkItem(BEntry &entry, struct stat *stBuf)
	: ArchiveItem(entry)
{
	// get the info
	name = strdup(fRef.name);

	struct stat		statBuf;
	if (!stBuf) {
		stBuf = &statBuf;
		entry.GetStat(stBuf);
	}	
	uncompressedSize = stBuf->st_size;
	//mode = stBuf->st_mode;
	
	creationTime = stBuf->st_crtime;
	modTime = stBuf->st_mtime;
	
	smallIcon = gGenericLinkIcon;
		
	/// get symlink value
	BSymLink		link(&entry);
	char			linkPath[PATH_MAX];
	
	ssize_t	sz = link.ReadLink(linkPath,PATH_MAX);
	if (sz > 0)
	{
		fLinkPath = strdup(linkPath);
		memcpy(fLinkPath,linkPath,sz);
	}
	else
		fLinkPath = strdup(B_EMPTY_STRING);
}

ArchiveLinkItem::~ArchiveLinkItem()
{
	free(fLinkPath);
}

long	ArchiveLinkItem::CompressItem(PackArc *archiveFile)
{
	if (!dirty || deleteThis)
		return B_NO_ERROR;
		
	status_t	err;
	long		res;
	BMessage upMsg(M_CURRENT_FILENAME);
	
	upMsg.AddString("filename",name);
	archiveFile->statusMessenger.SendMessage(&upMsg);
	
	off_t	totalEntrySize;
	
	PRINT(("calling add link for %s\n",name));
	BNode	theFile(&fRef);
	err = theFile.InitCheck();
	if (err < B_OK) {
		res = doError("There was an error opening the symbolic link \"%s\" (%s).",name,
					"Continue","Cancel",NULL,err);
		if (res == 0) err = B_OK;
		else err = B_CANCELED;
		return err;
	}
	err = archiveFile->AddNode(&theFile, &totalEntrySize, B_SYMLINK_NODE);
	PRINT(("add link done\n"));
	if (err != B_NO_ERROR && err != B_CANCELED) {
		// option to continue or cancel
		res = doError("There was an error compressing the item \"%s\"",name,
					"Continue","Cancel");
		if (res == 0) err = B_OK;
		else err = B_CANCELED;
			
		// this might allow us to continue
		//archiveFile->ResetStream();
		return err;
	}
	if (err == B_CANCELED) {
		// compression was canceled
		PRINT(("*************Dirty is true for link %s\n",name));
		dirty = true;
		return err;
	}
	compressedSize = totalEntrySize;
	seekOffset = archiveFile->catalogOffset;
	PRINT(("compressed size is %d, by position %d\n",compressedSize));
	
	archiveFile->catalogOffset += compressedSize;
	dirty = false;
		
	ArchiveItem *cur = GetParent();
	while( cur != NULL) {
		cur->compressedSize += compressedSize;
		cur = cur->GetParent();
	}	
	return err;
}

long	ArchiveLinkItem::ExtractItem(PackArc *archiveFile,BDirectory *destDir)
{
	long err = B_NO_ERROR;
	BMessage upMsg(M_CURRENT_FILENAME);
	
	upMsg.AddString("filename",name);
	archiveFile->statusMessenger.SendMessage(&upMsg);

	if (dirty == TRUE || !fLinkPath)
		return B_ERROR;

	if (destDir->Contains(name) == TRUE) {
		/// existing item may be a file OR folder, find out which
		BEntry	existingItem;
		destDir->FindEntry(name,&existingItem);
		
		char fname[B_FILE_NAME_LENGTH];
		BEntry destDirEnt;
		destDir->GetEntry(&destDirEnt);
		destDirEnt.GetName(fname);
		
		char mbuf[B_FILE_NAME_LENGTH*2 + 45];
		sprintf(mbuf,"An item with the name \"%s\" already exists in the folder \"%s\".",
						name,fname);
		BAlert *dupAlert = new BAlert(B_EMPTY_STRING,mbuf,"Skip Item",
			"Rename Existing","Replace Existing",B_WIDTH_FROM_WIDEST);
		long result = dupAlert->Go();
		switch (result) {
			case 0:
				return err;
				break;
			case 1:
				RenameExisting(destDir,&existingItem," copy");
				break;
			case 2:
				// if this is a folder blow it away!!
				// if the is a file, it will be blown away as well
				err = RecursiveDelete(&existingItem);
				if (err != B_NO_ERROR) {
					doError("Could not remove existing item. Skipping.");
					return err;
				}
				break;
		}
	}
	BSymLink	outLink;
	BEntry		outEntry;
	PRINT(("creating new file\n"));
	err = destDir->CreateSymLink(name, fLinkPath, &outLink);
	if (err != B_NO_ERROR) {
		doError("Error creating new symbolic link.");
		return err;	
	}
	destDir->FindEntry(name,&outEntry);

	err = archiveFile->ExtractFile(&outLink,compressedSize,seekOffset);
	if (err == B_CANCELED) {
		outEntry.Remove();
		return err;
	}
	if (err != B_NO_ERROR) {
		doError("Encountered an error when decompressing symbolic link.");
		return err;
	}

	err = outEntry.SetModificationTime(modTime);
	err = outEntry.SetCreationTime(creationTime);

	return err;
}
