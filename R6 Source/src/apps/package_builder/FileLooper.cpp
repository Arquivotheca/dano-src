#include <Be.h>
// FileLooper.cpp

#include "FileLooper.h"
#include "PackMessages.h"
#include "PackArc.h"

#include "FArchiveItem.h"
#include "ArchivePatchItem.h"

#include "PackListView.h"
#include "PackWindow.h"
#include "StatusWindow.h"
#include "ExtractUtils.h"

#define MODULE_DEBUG 0
#include "MyDebug.h"
#include "Util.h"


FileLooper::FileLooper()
	: BLooper()
{
 	PRINT(("file looper created\n"));
	doCancel = FALSE;
}

FileLooper::~FileLooper()
{

}

thread_id FileLooper::Run()
{
	PRINT(("starting compression loop\n"));
	BEntry	arcEnt(packArc->ArcRef());
	
	char theName[B_FILE_NAME_LENGTH];
	arcEnt.GetName(theName);
	char windowTitle[B_FILE_NAME_LENGTH+10];
	sprintf(windowTitle, "Status \"%s\"",theName);

	statWindow = new StatusWindow(windowTitle, &doCancel);
	
	packArc->statusMessenger = statWindow->StatusMessenger();
	
	return BLooper::Run();
}

bool FileLooper::QuitRequested()
{
	// PRINT(("file looper got quit requested\n"));
	
	long err = statWindow->PostMessage(B_QUIT_REQUESTED);
	if (err != B_NO_ERROR) {
		PRINT(("invalid BLooper\n"));
	}
	return TRUE;
}

int fileOffsetCompare(ArchiveItem * const *a, ArchiveItem * const *b );

int fileOffsetCompare(ArchiveItem * const *a, ArchiveItem * const *b )
{
	if ((*a)->seekOffset < (*b)->seekOffset)
		return -1;
	else if ((*a)->seekOffset == (*b)->seekOffset)
		return 0;
	else 
		return 1;
}

void FileLooper::DoAddItems(BMessage *msg)
{
	// num compressible items
	long totalItems = 0;
	// number of bytes to compress
	long totalBytes = 0;
	ulong type;
	long count;
	status_t err = B_NO_ERROR;
	doCancel = FALSE;
	
	msg->GetInfo("archive item", &type, &count);
	
	if (count <= 0)
		return;
	
	err = packArc->BeginCompression();
	if (err != B_NO_ERROR) {
		doError("There was an error starting compression.");
		return;
	}
	
	// first pass, gather count and bytes for status info
	
	BLocker *tLock;
	err = msg->FindPointer("tree lock",reinterpret_cast<void **>(&tLock));
	if (err != B_NO_ERROR) {
		doError("Error, tree lock is null");
		return;
	}
	sem_id calc_sem = msg->FindInt32("calcsem");
	if (calc_sem) {
		long threadCount = msg->FindInt32("foldercount");
		PRINT(("file looper waiting for folders to finish.... "));
		acquire_sem_etc(calc_sem,threadCount,0,0);
		PRINT(("FINISHED\n"));
	
		delete_sem(calc_sem);
	}
	// items could get added in while we wait, meaning they would 
	// get compressed twice!!!
	
	tLock->Lock();
	
	// building compress tree
	for (long i = 0; i < count; i++) {
		ArchiveItem *arcItem;
		err = msg->FindPointer("archive item",i,reinterpret_cast<void **>(&arcItem));
		if (err != B_NO_ERROR) {
			doError("File thread failed to find item archive item");
			tLock->Unlock();
			return;
		}
		if (arcItem->dirty != true) {
			doError("Archive item should be tagged as dirty");
		}
		else if (doCancel) {
			PRINT(("cancelled\n"));
			break;
		}
		else if (arcItem->deleteThis != true)
		{
			//PRINT(("Item is dirty and not marked for deletion\n"));
			if (arcItem->IsFolder()) {
				//PRINT(("loop got archive folder item\n"));
				ArchiveFolderItem *farcItem = (ArchiveFolderItem *)arcItem;
				
				// this has been done already
				// farcItem->GatherEntries();
				
				// gather information for the status window
				totalBytes += arcItem->uncompressedSize;
				totalItems += ((ArchiveFolderItem *)arcItem)->numCompressibleItems;
				
				// Gather information for the parents
				//ArchiveFolderItem *cur = farcItem->GetParent();
				//while( cur != NULL) {
				//	cur->uncompressedSize += arcItem->uncompressedSize;
	 			//	cur->numCompressibleItems += farcItem->numCompressibleItems;
				//	cur = cur->GetParent();
				// }
						
				/* this is ok because the folder can't be entered yet */
				PRINT(("building compress tree\n"));
				farcItem->BuildCompressTree();
			}
			else {
				totalBytes += arcItem->uncompressedSize;
				totalItems++;
			}
		}
	}
	tLock->Unlock();
	
	// second pass do compression
	BMessage addMsg(M_ADD_ITEMS);
	
	addMsg.AddInt32("item count",totalItems);
	addMsg.AddInt32("bytes",totalBytes);
	packArc->byteUpdateSize = max_c(totalBytes/STATUS_FREQUENCY,1);
	// should this be zero or what?
	packArc->bucket = 0;
	
	packArc->statusMessenger.SendMessage(&addMsg);
	
	// doing compression
	for (long i = 0; i < count; i++) {
		ArchiveItem *arcItem;
		err = msg->FindPointer("archive item",i,reinterpret_cast<void **>(&arcItem));
		if (err != B_NO_ERROR) {
			doError("File thread: archive item is NULL");
			return;
		}
		if (doCancel) {
			goto canceled;
		}
		else if (arcItem->deleteThis != TRUE) {
			//PRINT(("##### calling compress Item\n"));
			err = arcItem->CompressItem(packArc);

			BMessage updtMessage(M_ITEMS_UPDATED);
			updtMessage.AddPointer("item",arcItem);
			packArc->updateMessenger.SendMessage(&updtMessage);
			
			if (err == B_CANCELED) {	
				goto canceled;
			}
			else if (err != B_NO_ERROR) {
				//char *errorstring = new char[B_FILE_NAME_LENGTH+60];
				//sprintf(errorstring,"There was an error compressing the file \"%s\"",arcItem->name);
				//oError(errorstring);
				// keep going!
				// continue or cancel all?
			}
		}
		else {
			// if the item is dirty (hasn't been written to disk)
			// then go ahead and delete the object
			// delete arcItem;
			// wait to do this in a separate delete message!
		}
	}

canceled:
	if (doCancel != TRUE) {
		packArc->backBytes = 0;	
	}
	else {
		doError("Compression was canceled.");
	}
	err = packArc->EndCompression();
	
	DoWriteCatalog(msg);
	
	// ugg this wont work since dirty items may have been added,
	// in which case the window isn't clean!
	//archiveFile->updateMessenger.SendMessage(M_WINDOW_CLEAN);
}

void FileLooper::DoRemoveItems(BMessage *msg, bool doCatalog)
{
	PRINT(("FileLooper::DoRemoveItems: Enter\n"));
	int32 totalBytes = 0;
	doCancel = FALSE;
	
	// perhaps we should just mark the master list????
	
	// then perform deletion by looking at the whole list
	// the list view should simply skip over items that have been marked
	// as deleted when drawing the display!!!
	
	// for now, try the old system
	
	// the list of ITEMS (files and folders) selected for removal
	RList<ArchiveItem *> *deleteItems;

	PackList *listView = (PackList *)wind->FindView("listing");
	if (listView == NULL) {
		doError("Failed to find list view object");
		return;
	}
	
	// the pool cannot be changed
	listView->deletePoolLock.Lock();
	msg->FindPointer("delete pool",reinterpret_cast<void **>(&deleteItems));
	
	if (!deleteItems) {
		doError("Delete pool was null");
		listView->deletePoolLock.Unlock();
		return;
	}
	// new deletions will be added to another pool
	listView->deletePool = NULL;
	
	listView->treeLock.Lock();
	listView->deletePoolLock.Unlock();
	
	// the final flat list of files/Patches/scripts/folders to remove
	RList<ArchiveItem *> 		deleteList;
	
	// minor memory mngmt hacks
	// the selected folders/files to be deleted (freed)
	RList<ArchiveItem *>		freeList;
	
	// the clean items whose offsets may need fixing
	RList<ArchiveItem *>		cleanList(64);
	
	long count = deleteItems->CountItems();
	PRINT(("   FileLooper::RemoveItems: got %d items in delete pool\n",count));

	for (long i = 0; i < count; i++) {
		ArchiveItem *arcItem = deleteItems->ItemAt(i);
		if (arcItem == NULL) {
			doError("File thread failed to find item archive item");
			return;
		}
		else if (doCancel) {
			// cancelling is disabled in status window
			PRINT(("   canceled\n"));
			break;
		}
		
		if (arcItem->IsFolder() == TRUE) {
			PRINT(("   FileLooper::RemoveItems: Getting all clean files in folder \"%s\"\n",arcItem->name));
			// get all non-dirty items written in the archive
			((ArchiveFolderItem *)arcItem)->GetCleanFiles(&deleteList,-1);
		}	
		PRINT(("	FileLooper::RemoveItems: checking item \"%s\", seekOffset %Ld, dirty %d\n",arcItem->name,arcItem->seekOffset,(int)arcItem->dirty));
		if (!arcItem->dirty && arcItem->seekOffset > -1) {
			deleteList.AddItem(arcItem);
		}
				
		// add this item to the free list
		// we can't delete the item now since we may need to use its sub-items
		PRINT(("   FileLooper::RemoveItems: adding item \"%s\" to free list\n",arcItem->name));
		freeList.AddItem(arcItem);
	}
	// second pass do deletion
		
	// we need to update all of the offsets for files following and in between the deleted items!
	
	ArchiveFolderItem *topfolder;
	msg->FindPointer("top folder",reinterpret_cast<void **>(&topfolder));
	if (topfolder == NULL) {
		doError("Error, toplevel folder not found");
		return;
	}
	
	long minOffset;
	if (deleteList.CountItems() > 0) {
		BMessage delMsg(M_REMOVE_ITEMS);
		
		for (int i = deleteList.CountItems()-1; i >= 0; i--) {
			ArchiveItem *arcItem = deleteList.ItemAt(i);
			if (arcItem->IsFolder())
				totalBytes += ((ArchiveFolderItem *)arcItem)->dataCompressedSize;
			else
				totalBytes += arcItem->compressedSize;		
		}
		delMsg.AddInt32("item count",deleteList.CountItems());
		delMsg.AddInt32("bytes",(int32)totalBytes);
		packArc->statusMessenger.SendMessage(&delMsg);
		packArc->byteUpdateSize = max_c(totalBytes/STATUS_FREQUENCY,1);
		// should this be zero or what?
		packArc->bucket = 0;

		PRINT(("   FileLooper::RemoveItems: sorting clean deleted files\n"));
		deleteList.SortItems(fileOffsetCompare);
		minOffset = deleteList.ItemAt(0)->seekOffset;

		// get a list of all the non-deleted clean files
		PRINT(("   FileLooper::RemoveItems: getting all non-deleted clean files with offsets greater than %d\n",minOffset));

		topfolder->GetCleanFiles(&cleanList,minOffset);
		listView->treeLock.Unlock();
	
		cleanList.SortItems(fileOffsetCompare);
	
		packArc->RemoveItems(&deleteList,&cleanList);
	}
	else
		listView->treeLock.Unlock();

	PRINT(("   FileLooper::RemoveItems: Freeing selected folders\n"));
	for(long i = freeList.CountItems()-1; i >= 0; i--) {
		delete freeList.ItemAt(i);
	}
	
	if (doCatalog) {
		DoWriteCatalog(msg);
	}
	PRINT(("FileLooper::DoRemoveItems: Exit\n"));
}

void FileLooper::AddPatchItem(BMessage *msg)
{
	long err;
	
	ArchivePatchItem	*arcItem;
	
	msg->FindPointer("archive item",reinterpret_cast<void **>(&arcItem));

	BEntry tfil(&arcItem->tempFileRef);
	if (tfil.InitCheck() < B_NO_ERROR) {
		doError("Error getting patch file");
		return;
		// should remove it from the interface!!
	}
	if (arcItem->dirty != TRUE) {
		doError("Archive item should be tagged as dirty");
	}
	else if (doCancel) {
		PRINT(("cancelled\n"));
	}
	else if (!arcItem->deleteThis) {
		err = packArc->BeginCompression();
		if (err != B_NO_ERROR) {
			doError("There was an error starting compression");
		}	
		else {
			BMessage addMsg(M_ADD_PATCH);
			addMsg.AddString("message","Adding patch...");
			
			off_t	sz;
			tfil.GetSize(&sz);
			long totalBytes = sz;
		
			addMsg.AddInt32("bytes",totalBytes);
			packArc->byteUpdateSize = max_c(totalBytes/STATUS_FREQUENCY,1);
			packArc->bucket = 0;
			packArc->statusMessenger.SendMessage(&addMsg);
			
			err = arcItem->CompressItem(packArc);
			
			BMessage updtMessage(M_ITEMS_UPDATED);
			updtMessage.AddPointer("item",arcItem);
			packArc->updateMessenger.SendMessage(&updtMessage);
		
			if (!doCancel)
				packArc->backBytes = 0;
		
			err = packArc->EndCompression();

			//arcItem->tempFileRef.record = -1;
			//arcItem->tempFileRef.database = -1;
						
			DoWriteCatalog(msg);
		}
	}
}


long	CopyFile(BEntry *dst, BEntry *src, BMessenger msg);


/*
/	DoWriteCatalog 
/	Write out the entire catalog section
/
/	This routine builds its own copy of the file tree. As
/	a result it may be out of sync with the contents of the file
/	if some deletions are pending.
/
*/
void FileLooper::DoWriteCatalog(BMessage *msg)
{
	long err;
	
	packArc->statusMessenger.SendMessage(M_WRITE_CATALOG);
	
	BLocker *tLock;
	msg->FindPointer("tree lock",reinterpret_cast<void **>(&tLock));
	if (tLock == NULL) {
		doError("Error, tree lock is null");
		// recovery?
		return;
	}
	ArchiveFolderItem *topfolder;
	msg->FindPointer("top folder",reinterpret_cast<void **>(&topfolder));
	if (topfolder == NULL) {
		doError("Error, toplevel folder not found");
		return;
	}
	
	PRINT(("building catalog tree, lock owner is ??\n"));
	tLock->Lock();
	// make a copy list of the entire tree
	// dirty items not needed?
	
	PRINT(("got tree lock\n"));
	topfolder->BuildCompressTree();
	PRINT(("return from build compress tree\n"));
	
	tLock->Unlock();
	PRINT(("tree lock unlocked, opening archive file ... "));
	// include items even if they are marked for deletion because
	// they are current members of the file!
	
	packArc->WriteFinalData(topfolder,wind->isCDInstall,&wind->attrib);

	bool closing = msg->FindBool("closing");

	BEntry *realFile;
	
	if ((msg->FindPointer("realfile",reinterpret_cast<void **>(&realFile)) == B_NO_ERROR) && realFile ) {
		// we have just completed saving to a temp file
		// now we should copy back to the real file
	
		//PRINT(("SWAPPING temp file and real file\n"));
		// err = realFile->SwitchWith(archiveFile);
		
		PRINT(("COPYING data back to temp file\n"));

		// fix this!!!!
		//if (!closing) {
			BEntry	thisFile(packArc->ArcRef());
			err = CopyFile(realFile,&thisFile,packArc->statusMessenger);
			if (err != B_NO_ERROR)
				doError("Error updating the actual file. Recover from temp file.");
			SetEntryType(realFile,"application/x-scode-DPkg");
		//}
	}
	else {
		PRINT(("realfile pointer not found\n"));
	}
	packArc->statusMessenger.SendMessage(M_DONE);
	
	int32	buildMsgCode;	
	if (closing) {
		PRINT(("posting quit requested back to window\n"));
		wind->PostMessage(B_QUIT_REQUESTED);
	}
	else if (msg->FindInt32("installer",&buildMsgCode) == B_NO_ERROR) {
		wind->PostMessage(buildMsgCode);
	}
}

void FileLooper::DoExtractItems(BMessage *msg)
{
	long	err;
	long	count;
	ulong	type;
	doCancel = FALSE;
		
	// could probably use canEnter flag on folders
	// as long as it is set in the other thread!
	BLocker *tLock;
	msg->FindPointer("tree lock",reinterpret_cast<void **>(&tLock));
	if (tLock == NULL) {
		doError("Error, tree lock is null");
		// recovery?
		return;
	}
	entry_ref	dirRef;
	msg->FindRef("refs",&dirRef);
	BDirectory *destDir = new BDirectory(&dirRef);
	
	// don't worry about the fact that we are receiveing pointers since
	// the objects won't be deleted
	// until this thread runs again
	// however, items might get skipped if there are deleted from the list
	// in the interim, locks at least maintain data integrity
	msg->GetInfo("archive item",&type,&count);
	
	long totalBytes = 0;
	long totalItems = 0;
	
	// first pass gather sizes for status
	for(long i = 0; i < count; i++) {
		ArchiveItem *arcItem;
		msg->FindPointer("archive item",i,reinterpret_cast<void **>(&arcItem));
		if (arcItem->IsFolder() == TRUE) {
			tLock->Lock();
			PRINT(("building compress tree for item %s\n",arcItem->name));
			// only clean items are needed
			arcItem->BuildCompressTree();
			tLock->Unlock();
			// gather information for the status window
			totalItems += ((ArchiveFolderItem *)arcItem)->numCompressibleItems;
		}
		else {
			totalItems++;
		}
		totalBytes += arcItem->compressedSize;
	}
	
	// second pass does decompression
	BMessage exMsg(M_EXTRACT_ITEMS);
	
	exMsg.AddInt32("item count",totalItems);
	exMsg.AddInt32("bytes",totalBytes);
	packArc->statusMessenger.SendMessage(&exMsg);
		
	packArc->byteUpdateSize = max_c(totalBytes/STATUS_FREQUENCY,1);
	packArc->bucket = 0;
	packArc->BeginDecompression();
	
//	archiveFile->readTime = 0;
//	archiveFile->writeTime = 0;
//	archiveFile->startTime = system_time();
	
	for(long i = 0; i < count; i++) {
		if (doCancel) {
			break;
		}
		ArchiveItem *arcItem;
		msg->FindPointer("archive item",i,reinterpret_cast<void **>(&arcItem));
		PRINT(("extracting top item %s\n",arcItem->name));
		err = arcItem->ExtractItem(packArc,destDir);
		if (err == B_CANCELED)
			break;
		else {
			// press on
		}
	}
//	printf("Total read time is %f \ntotal write time %f\ntotal time %f",
//		(float)(archiveFile->readTime/1000.0),
//		(float)(archiveFile->writeTime/1000.0),
//		(float)((system_time()-archiveFile->startTime)/1000.0));
//	printf("Total written %d\n",archiveFile->grandTotal);

	packArc->EndDecompression();
	delete destDir;
	
	packArc->statusMessenger.SendMessage(M_DONE);
}

// called when count dirty is non zero
// tree must be locked at this point
// include bool if closing
void FileLooper::DoSave(BMessage *msg)
{
	long totalItems; // get from message
	long totalBytes; // from message
	
	// first handle deletion
	void *deletePool;
	msg->FindPointer("delete pool",reinterpret_cast<void **>(&deletePool));
	
	if (deletePool) {
		PRINT(("delete pool is non-null, performing deletion\n"));
		DoRemoveItems(msg, FALSE);
	}

	/// ok to call compress
	/// items marked as cd-ref are not actually file compressed
	if (!wind->isCDInstall) {
		// these could be inconsistent if items are added
		totalItems = msg->FindInt32("item count");
		totalBytes = msg->FindInt32("byte count");
		
		ArchiveFolderItem *top;
		msg->FindPointer("archive item",reinterpret_cast<void **>(&top));
		if (top == NULL) {
			doError("Failed to get toplevel folder object");
			return;
		}
		BLocker *tLock;
		msg->FindPointer("tree lock",reinterpret_cast<void **>(&tLock));
		if (tLock == NULL) {
			doError("Error, tree lock is null");
			return;
		}
		tLock->Lock();
		top->BuildCompressTree();
		tLock->Unlock();
			
		long err = packArc->BeginCompression();
		if (err != B_NO_ERROR) {
			doError("There was an error starting compression");
			return;
		}
		
		doCancel = FALSE;
		BMessage addMsg(M_ADD_ITEMS);
		
		addMsg.AddInt32("item count",totalItems);
		addMsg.AddInt32("bytes",totalBytes);
		packArc->statusMessenger.SendMessage(&addMsg);
		
		packArc->byteUpdateSize = max_c(totalBytes/STATUS_FREQUENCY,1);
		packArc->bucket = 0;
		
		err = top->CompressItem(packArc);
		
		if (err != B_NO_ERROR) {
			if (err != B_CANCELED)
				doError("There was an error while compressing the files.");
			else
				wind->realUpdateNeeded = TRUE;
		}
		else if (doCancel != TRUE)
			packArc->backBytes = 0;		
		
		packArc->EndCompression();
	}
	// write the catalog
	DoWriteCatalog(msg);
}

void FileLooper::MessageReceived(BMessage *msg)
{
	PackWindow *pw = (PackWindow *)wind;
	PRINT(("file looper got message\n"));
	switch(msg->what) {
		case M_ADD_PATCH:
			AddPatchItem(msg);
			atomic_add(&wind->msgCount,-1);
			break;
		case M_ADD_ITEMS:
			DoAddItems(msg);
			atomic_add(&wind->msgCount,-1);
			break;
		case M_REMOVE_ITEMS:
			DoRemoveItems(msg);
			// decrement msgCount since catalog write is called directly
			atomic_add(&wind->msgCount,-1);
			break;
		case M_WRITE_CATALOG:
			/****/
			DoWriteCatalog(msg);
			atomic_add(&wind->msgCount,-1);
			break;
		case M_EXTRACT_ITEMS:
			DoExtractItems(msg);
			atomic_add(&wind->msgCount,-1);
			break;
		case M_BUILD_PACKAGE:
		case M_BUILD_INSTALLER: {
			// post status window
			BMessage addMsg(M_BUILD_INSTALLER);
	
			BEntry	ent(packArc->ArcRef());
			int32 totalBytes;
			off_t bigSize;
			ent.GetSize(&bigSize);
			totalBytes = bigSize;
			addMsg.AddInt32("bytes",totalBytes);
			
			packArc->statusMessenger.SendMessage(&addMsg);
			packArc->byteUpdateSize = max_c(totalBytes/STATUS_FREQUENCY,1);
			packArc->bucket = 0;

			entry_ref	ref;
			msg->FindRef("directory",&ref);
			packArc->BuildInstaller(ref, msg->FindString("name"),
					msg->what == M_BUILD_INSTALLER);
										
			packArc->statusMessenger.SendMessage(M_DONE);
			
			atomic_add(&wind->msgCount,-1);
			break;
		}
		case M_SAVE:
			DoSave(msg);
			atomic_add(&wind->msgCount,-1);
			break;
		case M_TEST_PATCH: {
			
			atomic_add(&wind->msgCount,-1);
			break;
		}
	}
}
