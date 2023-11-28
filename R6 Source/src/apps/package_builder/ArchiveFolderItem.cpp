#include <Be.h>
// ArchiveFolderItem.cpp

#include "FArchiveItem.h"
#include "PackArc.h"

#include "GlobalIcons.h"
#include "PackMessages.h"
#include "DestinationList.h"

#include "Replace.h"
#include "ExtractUtils.h"

#include "Util.h"
#include "MyDebug.h"

ArchiveFolderItem::ArchiveFolderItem()
	: ArchiveItem()
{
	smallIcon = gGenericFolderIcon;
	numCompressibleItems = 0;
//	numEntries = 0;
	entries = new RList<ArchiveItem *>;
	compressEntries = NULL;
	replacement = R_MERGE_FOLDER;
}


ArchiveFolderItem::ArchiveFolderItem(BEntry &ref, struct stat *stBuf)
	: ArchiveItem(ref)
{
	smallIcon = gGenericFolderIcon;
	// we include the folder itself
	// since it has compressible attributes
	numCompressibleItems = 1;
//	numEntries = 0;
	entries = new RList<ArchiveItem *>;
	compressEntries = NULL;
	replacement = R_MERGE_FOLDER;
	
	name = strdup(fRef.name);
	
	creationTime = stBuf->st_crtime;
	modTime = stBuf->st_mtime;
}

ArchiveFolderItem::~ArchiveFolderItem()
{
	if (entries != NULL) {
  		for(long i = entries->CountItems()-1; i >= 0; i--)
			delete entries->ItemAt(i);
		delete entries;
	}
	if (compressEntries != NULL) {
		// don't do individual items since this
		// is just copy of the tree pointers
		delete compressEntries;
	}
}

bool ArchiveFolderItem::IsFolder()
{	
	return true;
}

/*
/	Called to gather the subentries when a folder is added
*/
void ArchiveFolderItem::GatherEntries()
{
	BDirectory			curDir(&fRef);
	if (curDir.InitCheck() < B_NO_ERROR) {
		doError("Invalid directory \"%s\".",name);
	}
	ArchiveFolderItem	*foldItem;
	ArchiveItem			*arcItem;

	entry_ref			itemRef;
	BEntry				dirItem;
	while (curDir.GetNextEntry(&dirItem) == B_NO_ERROR) {
		struct stat	statBuf;
		dirItem.GetStat(&statBuf);
		if (S_ISREG(statBuf.st_mode))
		{
			arcItem = new ArchiveFileItem(dirItem,&statBuf);
			AddEntry(arcItem);
			arcItem->SetParent(this);
			numCompressibleItems++;
//			numEntries++;
			uncompressedSize += arcItem->uncompressedSize;
		}
		else if (S_ISDIR(statBuf.st_mode))
		{
			foldItem = new ArchiveFolderItem(dirItem,&statBuf);
			AddEntry(foldItem);
			foldItem->SetParent(this);
			foldItem->GatherEntries();
			foldItem->canEnter = TRUE;
			numCompressibleItems += foldItem->numCompressibleItems;
//			numEntries++;
			uncompressedSize += foldItem->uncompressedSize;
		}
		else if (S_ISLNK(statBuf.st_mode))
		{
			arcItem = new ArchiveLinkItem(dirItem,&statBuf);
			AddEntry(arcItem);
			arcItem->SetParent(this);
			numCompressibleItems++;
			uncompressedSize += arcItem->uncompressedSize;
		}
	}
}

// builds a full copy of the current tree so that compression can
// take place even while items in the interface are added or removed
//
// this is a separate routine in case compression is separated from
// file addition?
void ArchiveFolderItem::BuildCompressTree()
{
	ArchiveItem *arcItem;

	// this may be bad, better choice might be to delete all items
	// and then add them on
	if (compressEntries != NULL) {
		delete compressEntries;
	}
	// copies the whole list
	PRINT(("doing list for folder %s  ....",name));
	compressEntries = new RList<ArchiveItem *>(*entries);
	PRINT(("done\n"));
	// build the tree for any folders
	for (long i = compressEntries->CountItems()-1; i >= 0; i--) {
		arcItem = compressEntries->ItemAt(i);
		arcItem->BuildCompressTree();
	}
}

/*
/	GetCleanFiles
/	used by the deletion routine to find the files which need
/	to have their offsets adjusted
/	the entire list of files is gathered into a flat list
*/
void ArchiveFolderItem::GetCleanFiles(RList<ArchiveItem *> *list,
									  off_t	minOffset)
{
	for (long i = entries->CountItems()-1; i >= 0; i--) {
		ArchiveItem *arcItem = entries->ItemAt(i);
		if (arcItem->dirty)
			continue;
			
		if (arcItem->IsFolder()) {
			if (arcItem->seekOffset > minOffset)
				list->AddItem(arcItem);
			((ArchiveFolderItem *)arcItem)->GetCleanFiles(list,minOffset);
		}
		else if (((ArchiveFileItem *)arcItem)->seekOffset > minOffset /** && arcItem->copyOnly **/) {
			list->AddItem(arcItem);
		}
	}
}

/*
/	CompressItem
/	Compress a folder entry and subentries
/	IN ASSERTION: compressEntries tree is built
/
*/
long ArchiveFolderItem::CompressItem(PackArc *archiveFile)
{
	// folders only return canceled errors	
	status_t err = B_NO_ERROR;
	
	if (dirty && !deleteThis)
	{
		BNode	folderNode(&fRef);
		err = folderNode.InitCheck();
		if (err < B_OK)
		{
			err = B_OK;
			dirty = false;
		}
		else
		{	
			off_t	totalEntrySize;
			err = archiveFile->AddNode(&folderNode, &totalEntrySize, B_DIRECTORY_NODE);
			PRINT(("add node done\n"));
			if (err != B_NO_ERROR && err != B_CANCELED) {
				// option to continue or cancel
				long res = doError("There was an error compressing the folder data for \"%s\"",name,
							"Continue","Cancel");
				if (res == 0) err = B_CANCELED;
					
				return err;
			}
			if (err == B_CANCELED) {
				// compression was canceled
				PRINT(("*************Dirty is true for folder %s\n",name));
				dirty = true;
				return err;
			}
			
			dataCompressedSize = totalEntrySize;
			seekOffset = archiveFile->catalogOffset;
			archiveFile->catalogOffset += dataCompressedSize;
			dirty = false;
			
			PRINT(("item \"%s\", seekOffset %Ld, dirty %d\n",name,seekOffset,(int)dirty));
				
			ArchiveItem *cur = GetParent();
			while( cur != NULL) {
				cur->compressedSize += compressedSize;
				cur = cur->GetParent();
			}
		}
	}
	
	int32 count = compressEntries->CountItems();
	for(int32 i = 0; i < count; i++) {
		ArchiveItem *curItem = compressEntries->ItemAt(i);
		err = curItem->CompressItem(archiveFile);
		// if canceled then quit
		if (err == B_CANCELED) {
			return err;
		}
		// if there was an error just keep chugging through the list

		BMessage updtMessage(M_ITEMS_UPDATED);
		updtMessage.AddPointer("item",curItem);
		archiveFile->updateMessenger.SendMessage(&updtMessage);
		
		// try to skip out early if deletion is detected		
		// only works if deletion is marked down through parents
		// not sure why this is here?
		if (deleteThis) {
			return B_NO_ERROR;
		}
	}
	return err;
}

/*
/	ExtractItem
/	extract and entrie folder and items contained within
/	scripts, patches and the like are skipped
*/
long ArchiveFolderItem::ExtractItem(PackArc *archiveFile,BDirectory *destDir)
{
	long err = B_NO_ERROR;
	bool create = TRUE;
	
	BDirectory newDir;

	if (destDir->Contains(name)) {
		/// existing item may be a file OR folder, find out which
		BEntry	existingItem;
		destDir->FindEntry(name,&existingItem);
		
		char mbuf[B_FILE_NAME_LENGTH*2 + 45];
		BAlert *dupAlert;
		if (existingItem.IsDirectory()) {
			sprintf(mbuf,"The folder \"%s\" already exists. Items will be placed inside the existing folder",name);
			dupAlert = new BAlert(B_EMPTY_STRING,mbuf,"Continue");
			dupAlert->Go();
			newDir.SetTo(&existingItem);
			create = FALSE;
		}
		else if (existingItem.IsFile()){
			char fname[B_FILE_NAME_LENGTH];
			BEntry destDirEnt;
			destDir->GetEntry(&destDirEnt);
			destDirEnt.GetName(fname);
			sprintf(mbuf,"A file with the name \"%s\" already exists in the folder \"%s\".",
					name,fname);
			dupAlert =new BAlert(B_EMPTY_STRING,mbuf,"Skip","Rename","Replace",B_WIDTH_FROM_WIDEST);
			long result = dupAlert->Go();
			switch (result) {
				case 0:
					return err;
					break;
				case 1:
					RenameExisting(destDir,&existingItem," copy");
					break;
				case 2:
					err = existingItem.Remove();
					if (err != B_NO_ERROR) {
						doError("Could not remove existing file. Skipping.");
						return err;
					}
					break;
			}
		}
	}
	// we should create the new directory
	if (create) {
		err = destDir->CreateDirectory(name,&newDir);
		// handle error
	}
	if (!dirty && seekOffset >= 0)
	{
		BNode	outNode(&newDir,".");
		err = archiveFile->ExtractFile(&outNode,dataCompressedSize,seekOffset);	
	}

	long count = compressEntries->CountItems();
	for (long i = 0; i < count; i++) {
		ArchiveItem *arcItem = compressEntries->ItemAt(i);
		err = arcItem->ExtractItem(archiveFile,&newDir);
		if (err == B_CANCELED) {
			return err;
		}
		else if (err != B_NO_ERROR) {
			return err;
		}
	}
	return err;
}

/*
/	CountDirty
/	count the number of dirty entries in this folder
/	deep recursive
/	must be modifed for copyOnly items
/	copyOnly and folders?
*/
long ArchiveFolderItem::CountDirty(long &bytes)
{
	ArchiveItem *arcItem;
	
	long count = 0;
	long max = entries->CountItems();
	for (long i = 0; i < max; i++) {
		arcItem = entries->ItemAt(i);
		if (arcItem->IsFolder())
		{
			count += ((ArchiveFolderItem *)arcItem)->CountDirty(bytes);
		}
		if (arcItem->dirty)
		{
			count++;
			bytes += arcItem->uncompressedSize;
		}
	}
	return count;
}

ulong ArchiveFolderItem::GetUnionGroups(ulong bits)
{
	ArchiveItem *arcItem;
	
	// the setting for the folder itself
	bits = bits | groups;
	
	long count = 0;
	long max = entries->CountItems();
	for (long i = 0; i < max; i++) {
		arcItem = entries->ItemAt(i);
		bits = arcItem->GetUnionGroups(bits);
	}
	return bits;
}


void ArchiveFolderItem::GroupDeleted(ulong lowM, ulong highM)
{
	ArchiveItem *arcItem;
	
	groups = (groups & lowM) | ((groups & highM) >> 1);
	
	long max = entries->CountItems();
	for (long i = 0; i < max; i++) {
		arcItem = entries->ItemAt(i);
		arcItem->groups = (arcItem->groups & lowM) | 
							((arcItem->groups & highM) >> 1);
		if (arcItem->IsFolder() == TRUE) {
			((ArchiveFolderItem *)arcItem)->GroupDeleted(lowM,highM);			
		}
	}
}

void ArchiveFolderItem::DestDeleted(long old)
{
	ArchiveItem *arcItem;
	
	long max = entries->CountItems();
	for (long i = 0; i < max; i++) {
		arcItem = entries->ItemAt(i);
		if (arcItem->customDest == TRUE) {
			if (arcItem->destination > old) {
				arcItem->destination -= 1;
			}
			else if (arcItem->destination == old) {
				if (arcItem->GetParent()->GetParent() == NULL)
					arcItem->destination = D_INSTALL_FOLDER;
				else
					arcItem->destination = D_PARENT_FOLDER;
					
				arcItem->customDest = FALSE;
			}
		}
		
		if (arcItem->IsFolder() == TRUE) {
			((ArchiveFolderItem *)arcItem)->DestDeleted(old);			
		}
	}
}

void ArchiveFolderItem::DestMoved(long old, long now)
{
	ArchiveItem *arcItem;
	
	long max = entries->CountItems();
	for (long i = 0; i < max; i++) {
		arcItem = entries->ItemAt(i);
		if (arcItem->customDest == TRUE) {
			long dest = arcItem->destination;
			if (old < now) {
				if (dest == old)
					arcItem->destination = now;
				else if (dest > old &&
						 dest <= now)
					arcItem->destination -= 1;
			}
			else {
				if (dest == old)
					arcItem->destination = now;
				else if (dest >= now &&
						 dest < old)
					arcItem->destination += 1;
			
			}
		}
		if (arcItem->IsFolder() == TRUE) {
			((ArchiveFolderItem *)arcItem)->DestMoved(old,now);			
		}
	}
}

void ArchiveFolderItem::SetGroups(ulong bits, bool mark)
{
	ArchiveItem *arcItem;
	
	if (!mark)
		groups &= bits;
	else
		groups |= bits; 
	long count = 0;
	long max = entries->CountItems();
	for (long i = 0; i < max; i++) {
		arcItem = entries->ItemAt(i);
		arcItem->SetGroups(bits,mark);
	}
}

/*
/	Update stats for this folder and parents when an item
/	contained within that folder has been removed
*/
void ArchiveFolderItem::UpdateRemovedEntry(ArchiveItem *arcItem)
{
	long count;
	
	ArchiveFolderItem *cur = this;
	ulong delGroups = arcItem->groups;
	
	int32 numCompressible = 1;
	if (arcItem->IsFolder())
		numCompressible = ((ArchiveFolderItem *)arcItem)->numCompressibleItems;
	
	while( cur != NULL) {
		cur->numCompressibleItems -= numCompressible;		
		cur->uncompressedSize -= arcItem->uncompressedSize;
		cur->compressedSize -= arcItem->compressedSize;
		count = cur->entries->CountItems();
		ulong curBits = 0;
		for (long i = 0; i < count; i++) {
			ArchiveItem *ai = (ArchiveItem *)cur->entries->ItemAt(i);
			// get groups common with the item being deleted
			curBits |= ai->groups & delGroups;
			if (curBits == arcItem->groups)
				// in this case all groups are common
				// and none can be removed
				break;	
		}
		if (curBits != arcItem->groups) {
			// the groups which can be deleted are those
			// groups in arcItem->groups not set in curBits
			delGroups = ~curBits & delGroups;	
			cur->groups &= ~delGroups;
		}
		cur = cur->GetParent();
	}
}
