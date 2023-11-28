#include <Be.h>
#include "PackListView.h"

#include "PackWindow.h"

#include "PackMessages.h"
#include "PackArc.h"
#include "FileLooper.h"
#include "FArchiveItem.h"
#include "ArchiveScriptItem.h"

#include "GlobalIcons.h"
#include "IconMenuItem.h"
#include "NameDialog.h"
#include "Replace.h"
//#include "CondMessages.h"

#include "FolderCalc.h"

#include "IconCache.h"
#include "MyDebug.h"
#include "Util.h"
#include <ctype.h>

void PackList::RefsReceived(BMessage *msg)
{
	PackWindow *wind = (PackWindow *)Window();
	ulong type;
	long count;
	BMessage addMsg(M_ADD_ITEMS);
	ArchiveItem *arcItem;
	long folderCount = 0;
	bool	immed = wind->autoCompress; // grab the current value
	
	ArchiveFolderItem *destFolder = CurrentFolder();
	sem_id calc_sem;
	
	calc_sem = create_sem(0,"foldercalc");
	
	msg->GetInfo("refs", &type, &count);
	for (long i = 0; i < count; i++) {
		BEntry		entry;
		entry_ref	itemRef;
		status_t	err;
		
		msg->FindRef("refs",i,&itemRef);
		err = entry.SetTo(&itemRef);
		if ( err == B_NO_ERROR )
		{
			bool itemok = true;
			bool foldernode = false;
			
			struct stat		statBuf;
			entry.GetStat(&statBuf);
			
			if (S_ISREG(statBuf.st_mode))
			{
				PRINT(("view got file ref\n"));
				arcItem = new ArchiveFileItem(entry,&statBuf);
				
				// store the original path to update the file later
				BPath	path;
				entry.GetPath(&path);
				free(arcItem->srcPath);
				arcItem->srcPath = strdup(path.Path());
			}
			else if (S_ISLNK(statBuf.st_mode))
			{
				PRINT(("view got link ref\n"));
				arcItem = new ArchiveLinkItem(entry,&statBuf);
			}
			else if (S_ISDIR(statBuf.st_mode))
			{
				PRINT(("view got folder ref\n"));
				arcItem = new ArchiveFolderItem(entry,&statBuf);
				((ArchiveFolderItem *)arcItem)->canEnter = false;
				ArchiveFolderItem *cur = destFolder;
				while( cur != NULL) {
					ulong nBits = cur->groups | arcItem->groups;
					if (nBits == cur->groups)
						break;
					else
						cur->groups = nBits;
					cur = cur->GetParent();
				}
				destFolder->AddEntry(arcItem);
				arcItem->SetParent(destFolder);
				
				// sizes and number of items will be updated after the folder
				// is parsed in a different thread
				if (immed) {
					PRINT(("folder added to file looper message\n"));
					addMsg.AddPointer("archive item",arcItem);
				}
				if (destFolder == toplevel) {
					arcItem->destination = D_INSTALL_FOLDER;
				}
		
				// create a thread to calculate the folder
				// perhaps do build compress tree if auto
				new FolderCalc((ArchiveFolderItem *)arcItem,
								calc_sem,wind->calcGroupSem,
								&(wind->calcThreadCount),
								BMessenger(wind));
				folderCount++;
				foldernode = true;
			}
			else {
				itemok = false;
			}
			
			if (itemok && !foldernode) {
				arcItem->SetParent(destFolder);
				if (destFolder == toplevel) {
					arcItem->destination = D_INSTALL_FOLDER;
				}
				// update parents' counts of files and sizes
				// linear in the depth of folders
				ArchiveFolderItem *cur = destFolder;
				while( cur != NULL) {
					cur->numCompressibleItems++;
					cur->uncompressedSize += arcItem->uncompressedSize;
					cur->groups |= arcItem->groups;
					
					cur = cur->GetParent();
				}
				destFolder->AddEntry(arcItem);
				if (immed) addMsg.AddPointer("archive item",arcItem);
			}
			
			if (itemok) wind->realUpdateNeeded = true;
		}
	}
	
	// new items mean the attributes are dirty
	wind->attribDirty = TRUE;
	
	if (immed) {
		PRINT(("sending messsage to file looper\n"));
		
		// is this used? NO
		// addMsg->AddMessenger("reply",BMessenger(this));
		addMsg.AddPointer("top folder",toplevel);
		addMsg.AddPointer("tree lock",&treeLock);
		addMsg.AddPointer("attributes",&(wind->attrib));
		
		// compression will block until folder calculation is complete
		addMsg.AddInt32("calcsem",calc_sem);
		addMsg.AddInt32("foldercount",folderCount);
		
		// should this be accessed like this? ok for the moment
		atomic_add(&wind->msgCount,1);
		ASSERT(wind->arcFile);
		ASSERT(wind->arcFile->fileLooper);
		wind->arcFile->fileLooper->PostMessage(&addMsg);
	}
	else {
		// not auto compressing
		delete_sem(calc_sem);
	}
	Update();
}

void PackList::AddScript(BMessage *msg)
{
	PackWindow *wind = (PackWindow *)Window();
	bool itemok = false;
	ArchiveItem *arcItem;
	bool	immed = wind->autoCompress; // cache in case it changes
	
	ArchiveFolderItem *destFolder = CurrentFolder();
	
	BEntry		entry;
	entry_ref	itemRef;
	status_t	err;
		
	msg->FindRef("refs",&itemRef);
	err = entry.SetTo(&itemRef);
	if ( err == B_NO_ERROR ) {
		struct stat		statBuf;
		entry.GetStat(&statBuf);
			
		if (S_ISREG(statBuf.st_mode)) {
			PRINT(("view got file ref\n"));
			
			// check file contents
			char line[80];
			const char *pattern = "#!/bin/sh";
			char *c;
			ssize_t	res;
			BFile	sfile(&entry,O_RDONLY);
			
			res = sfile.Read(line,79);
			if (res < 0) res = 0;
			line[res] = 0;
			c = line;
			while (*c && isspace(*c)) c++;
			if (memcmp(c,pattern,strlen(pattern))) {
				doError("Invalid shell script. Scripts should begin with \n%s",pattern);
			}
			else {		
				arcItem = new ArchiveScriptItem(entry,&statBuf);
				arcItem->SetParent(destFolder);
//				destFolder->numEntries++;
				if (destFolder == toplevel) {
					arcItem->destination = D_INSTALL_FOLDER;
				}
				// update parents' counts of files and sizes
				// linear in the depth of folders
				ArchiveFolderItem *cur = destFolder;
				while( cur != NULL) {
					cur->numCompressibleItems++;
					cur->uncompressedSize += arcItem->uncompressedSize;
					cur->groups |= arcItem->groups;
					
					cur = cur->GetParent();
				}
				destFolder->AddEntry(arcItem);
				itemok = true;
				wind->realUpdateNeeded = TRUE;
			}
		}
		else {
			doError("Sorry, unsupported file system type.");
		}
	}
	
	// new items mean the attributes are dirty
	if (itemok) {
		wind->attribDirty = TRUE;
		if (immed) {
			BMessage addMsg(M_ADD_ITEMS);
			
			addMsg.AddPointer("archive item",arcItem);
		
			PRINT(("sending messsage to file looper\n"));
			
			// is this used? NO
			// addMsg->AddMessenger("reply",BMessenger(this));
			addMsg.AddPointer("top folder",toplevel);
			addMsg.AddPointer("tree lock",&treeLock);
			addMsg.AddPointer("attributes",&(wind->attrib));
			
			// compression will block until folder calculation is complete
			addMsg.AddInt32("calcsem",0);
			
			// should this be accessed like this? ok for the moment
			atomic_add(&wind->msgCount,1);
			ASSERT(wind->arcFile);
			ASSERT(wind->arcFile->fileLooper);
			wind->arcFile->fileLooper->PostMessage(&addMsg);
		}
		Update();
	}
}


void PackList::RemoveItems(BMessage *msg)
{
	// if nothing selected
	if (lowSelection == -1)
		return;
	// PRINT(("doing remove items in pack list view\n"));

	// some of this is a bit ugly because it inspects the file list directly
	// through inherited stuff from FListView

	PackWindow *wind = (PackWindow *)Window();
	bool	immed = wind->autoCompress;
		
	deletePoolLock.Lock();

	wind->attribDirty = TRUE;
	
	// if the pool is null, all deletes have been performed (or are in the process
	// of being performed so we need a new pool)
	bool newPool = FALSE;
	if (deletePool == NULL) {
		PRINT(("######   creaing new pool\n"));
		deletePool = new RList<ArchiveItem *>;
		newPool = TRUE;
	}

	// tree should be locked here
	long numAdded = 0;
	for (long i = CountItems()-1; i >= 0; i--) {
		ArchiveItem *arcItem = (ArchiveItem *)ItemAt(i);
		if (arcItem->selected) {
			arcItem->deleteThis = TRUE;

			// remove from the view list
			RemoveItem(i);
			// this is inefficient when a clump of items is deleted
			// at once. The group fixup should be done outside this loop
			// in a separate pass
			CurrentFolder()->UpdateRemovedEntry(arcItem);
			// do a no-brainer delete check-- if msgCount == 0
			// then no operations are in progress
			// so some immediate deletion can take place
			// directly add clean files to delete pool from
			bool addToPool = TRUE;
			if (wind->msgCount == 0) {
				PRINT(("message count is zero, checking for imeediate delete\n"));
				if (arcItem->IsFolder()) {
					ArchiveFolderItem *afold = (ArchiveFolderItem *)arcItem;
					long totalDirty, bytes;
					totalDirty = afold->CountDirty(bytes);
					PRINT(("total dirty: %d  numCompressible: %d\n",
							totalDirty,afold->numCompressibleItems));
					if (totalDirty == afold->numCompressibleItems) {
						PRINT(("deleting folder\n"));
						delete afold;
						addToPool = FALSE;
					}
				}
				else if (arcItem->dirty) {
					// delete it now
					PRINT(("deleting item\n"));
					delete arcItem;
					addToPool = FALSE;
				}
			}
			
			// will be deleted in another thread
			if (addToPool) {
				numAdded++;
				deletePool->AddItem(arcItem);
			}
		}
	}
	deletePoolLock.Unlock();
	
	wind->realUpdateNeeded = TRUE;
	
	if (newPool && immed && (numAdded > 0)) {
		BMessage delMsg(M_REMOVE_ITEMS);
		delMsg.AddPointer("delete pool",deletePool);
		delMsg.AddPointer("attributes",&(wind->attrib));
		delMsg.AddPointer("tree lock",&treeLock);
		delMsg.AddPointer("top folder",toplevel);
		atomic_add(&wind->msgCount,1);
		
		ASSERT(wind->arcFile);
		PRINT(("######   sending message\n"));		
		wind->arcFile->fileLooper->PostMessage(&delMsg);
	}

	highSelection = lowSelection = -1;
	SelectionSet();

	// this fixes up the scroll bar as items come and go
	// and also invalidates the view for a redraw
	Update();
}

/**
void PackList::UpdateItems(BMessage *msg)
{
	// if nothing selected
	if (lowSelection == -1)
		return;
	
	PRINT(("doing update items in pack list view\n"));

	PackWindow *wind = (PackWindow *)Window();
	bool	immed = wind->autoCompress;
		
	deletePoolLock.Lock();

	// this only becomes true until we check!!!
	wind->attribDirty = true;
	
	// if the pool is null, all deletes have been performed (or are in the process
	// of being performed)
	bool newPool = false;
	if (deletePool == NULL) {
		PRINT(("######   creaing new pool\n"));
		deletePool = new RList<ArchiveItem *>;
		newPool = true;
	}

	// tree should be locked here?????????
	long numAdded = 0;
	for (long i = CountItems()-1; i >= 0; i--) {
		ArchiveItem *arcItem = (ArchiveItem *)ItemAt(i);
		if (arcItem->selected && arcItem->) {
			
			if 
			
			arcItem->deleteThis = TRUE;

			// remove from the view list
			RemoveItem(i);
			
			// do a no-brainer delete check-- if msgCount == 0
			// then no operations are in progress
			// so some immediate deletion can take place
			// directly add clean files to delete pool from
			bool addToPool = TRUE;
			if (wind->msgCount == 0) {
				PRINT(("message count is zero, checking for imeediate delete\n"));
				if (arcItem->IsFolder()) {
					ArchiveFolderItem *afold = (ArchiveFolderItem *)arcItem;
					long totalDirty, bytes;
					totalDirty = afold->CountDirty(bytes);
					PRINT(("total dirty: %d  numCompressible: %d\n",
							totalDirty,afold->numCompressibleItems));
					if (totalDirty == afold->numCompressibleItems) {
						PRINT(("deleting folder\n"));
						delete afold;
						addToPool = FALSE;
					}
				}
				else if (arcItem->dirty) {
					// delete it now
					PRINT(("deleting item\n"));
					delete arcItem;
					addToPool = FALSE;
				}
			}
			
			// will be deleted in another thread
			if (addToPool) {
				numAdded++;
				deletePool->AddItem(arcItem);
			}
			

			// curBits represents groups in common with the groups
			// of the item being removed

			// this is inefficient when a clump of items is deleted
			// at once. The group fixup should be done outside this loop
			// in a separate pass
			CurrentFolder()->UpdateRemovedEntry(arcItem);
		}
	}
	deletePoolLock.Unlock();
	
	wind->realUpdateNeeded = TRUE;
	
	if (newPool && immed && (numAdded > 0)) {
		BMessage delMsg(M_REMOVE_ITEMS);
		delMsg.AddPointer("delete pool",deletePool);
		delMsg.AddPointer("attributes",&(wind->attrib));
		delMsg.AddPointer("tree lock",&treeLock);
		delMsg.AddPointer("top folder",toplevel);
		atomic_add(&wind->msgCount,1);
		
		ASSERT(wind->arcFile);
		PRINT(("######   sending message\n"));		
		wind->arcFile->fileLooper->PostMessage(&delMsg);
	}

	highSelection = lowSelection = -1;
	SelectionSet();

	// this fixes up the scroll bar as items come and go
	// and also invalidates the view for a redraw
	Update();
}
**/

void PackList::NewFolder(BMessage *msg)
{
	PRINT(("got new folder message\n"));		
	ArchiveFolderItem *newFolder;
	ArchiveFolderItem *destFolder;
	const char *foldName;
	PackWindow *wind = (PackWindow *)Window();
	ulong type;
	long count;

	msg->GetInfo("text",&type,&count);
	if (count > 0) {
		destFolder = CurrentFolder();
		// a slight hack to force catalog write
		wind->attribDirty = TRUE;
		while(count--) {
			foldName = msg->FindString("text",count);
			newFolder = new ArchiveFolderItem();
			newFolder->SetParent(destFolder);
			newFolder->canEnter = TRUE;
			
			newFolder->name = strdup(foldName);
			
//			destFolder->numEntries++;
			if (destFolder == toplevel) {
				newFolder->destination = D_INSTALL_FOLDER;
			}
			
			ArchiveFolderItem *cur = destFolder;
			while( cur != NULL) {
				cur->groups |= newFolder->groups;
				cur = cur->GetParent();
			}
			newFolder->dirty = false;
			destFolder->AddEntry(newFolder);
			Update();		
		}
	}
	else {
		NameDialog *nd = new NameDialog(BRect(0,0,280,80),"Folder Name:",
			"New Folder",new BMessage(M_NEW_FOLDER),this);
	}
}

void PackList::MessageReceived(BMessage *msg)
{
	// be sure to check lock
	char whatCode[5];
	whatCode[4] = '\0';
	*((long *)whatCode) = msg->what;
	PRINT(("pack list got message %s\n",whatCode));
	
	PackWindow *wind = (PackWindow *)Window();
	long i;
	bool mark;
	long count;
	bool stillSet;
	long destination;
	bool cust;
	status_t err;
	long replace;
	int32 arch;
	int conditional;
	int condition;
	ulong bitmap;
	ArchiveFolderItem *cur;
	
	switch(msg->what) {
		case 'DATA':
			// PRINT(("pack list got DATA message, find any refs\n"));
		case M_ADD_REFS:
		case B_REFS_RECEIVED:
			RefsReceived(msg);
			break;
		case M_ADD_PATCH:
			AddPatch(msg);
			break;
		case M_ADD_SCRIPT:
			AddScript(msg);
			break;
		case M_REMOVE_ITEMS:
			RemoveItems(msg);
			break;
		case M_CHANGE_FOLDER_LEVEL:
			ChangeFolderLevel(msg);
			break;
		case M_NEW_FOLDER:
			NewFolder(msg);
			break;
		case M_CHANGE_NAME:
			ChangeName(msg);
			break;
		case M_SELECT_ALL:
			i = CountItems()-1;
			if (i >= 0) {
				highSelection = i;
				lowSelection = 0;
				for(long i = CountItems()-1; i >= 0; i--) {
					Select(i);
				}
				SelectionSet();
			}
			break;
		case M_INVOKE:
			if (lowSelection != -1 && lowSelection == highSelection) {
				Invoke(lowSelection);
			}
			break;
		case M_EXTRACT_ITEMS:
			if (wind->arcFile == NULL) {
				doError("Can't extract items because there is no disk file.");
				break;
			}
			PRINT(("list view got extract items message\n"));

			Looper()->DetachCurrentMessage();
			for (long i = CountItems()-1; i >= 0; i--) {
				ArchiveItem *arcItem = (ArchiveItem *)ItemAt(i);
				if (arcItem->selected) {
					msg->AddPointer("archive item",arcItem);
				}
			}
			msg->AddPointer("tree lock",&treeLock);
			msg->AddPointer("top folder",toplevel);
			atomic_add(&wind->msgCount,1);
			wind->arcFile->fileLooper->PostMessage(msg);
			break;
		case M_GROUP_SELECTED:
			if (lowSelection == -1)
				break;
			wind->attribDirty = TRUE;
			PRINT(("#####listing got groups selected msg\n"));
			bitmap = msg->FindInt32("bitmap");
			mark = msg->FindBool("mark");
			if (!mark) {
				bitmap = ~bitmap;
			}
			for (long i = lowSelection; i <= highSelection; i++) {
				ArchiveItem *ai = (ArchiveItem *)ItemAt(i);
				if (ai->selected) {
					ai->SetGroups(bitmap,mark);
				}
			}
			ArchiveFolderItem *destFolder;
			destFolder = CurrentFolder();
			/////////// set parent groups, efficient version /////////////
			cur = destFolder;
			while( cur != NULL) {
				if (mark) {
					ulong nBits = cur->groups | bitmap;
					if (nBits == cur->groups)
						// if the parent already has this group, no need to move
						// up the tree any farther
						break;
					else 
						cur->groups = nBits;
				}
				else {
					// loop through all the items in this folder
					count = cur->entries->CountItems();
					stillSet = FALSE;
					for (long i = 0; i < count; i++) {
						ArchiveItem *ai = (ArchiveItem *)cur->entries->ItemAt(i);
						if ((ai->groups & ~bitmap) != 0) {
							stillSet = TRUE;
							break;
						}
					}
					if (stillSet)
						break;
					// if this group is still set in any of the entries
					// don't remove it
					
					// otherwise remove this group to the parent 
					cur->groups &= bitmap;
				}
				cur = cur->GetParent();
			}
			
			break;
		case M_DEST_SELECTED: {
			if (lowSelection == -1)
				break;
			wind->attribDirty = TRUE;
			
			destination = msg->FindInt32("dest");

			err = msg->FindBool("custom",&cust);
			
			PRINT(("  DESTINATION %d  CUSTOM %d\n",destination,cust));
			if (err != B_NO_ERROR) {
				doError("M_DEST_SELECTED couldn't find custom data in message");
			}
			for (long i = lowSelection; i <= highSelection; i++) {
				ArchiveItem *ai = (ArchiveItem *)ItemAt(i);
				if (ai->selected) {
					ai->destination = destination;
					ai->customDest = cust;
				}
			}
			break;
		}
		case M_REPL_SELECTED: {
			if (lowSelection == -1)
				break;
			wind->attribDirty = TRUE;
			
			// here we lookup the real value in the replaceOptions array
			// actually we have it stored in the message
			replace = msg->FindInt32("repl");
			for (long i = lowSelection; i <= highSelection; i++) {
				ArchiveItem *ai = (ArchiveItem *)ItemAt(i);
				if (ai->selected) {
					ai->replacement = replace;
				}
			}
			break;
		}
		case M_ARCH_SELECTED: {
			if (lowSelection == -1)
				break;
			wind->attribDirty = true;
			arch = msg->FindInt32("platform");
			for (long i = lowSelection; i <= highSelection; i++) {
				ArchiveItem *ai = (ArchiveItem *)ItemAt(i);
				if (ai->selected) {
					ai->platform = arch;
				}
			}
			break;
		}
#if CONDITIONAL_INSTALL
		case M_COND_SELECTED: {
#if DEBUG			
			msg->PrintToStream();
#endif
			if (lowSelection == -1)
				break;
			wind->attribDirty = true;
			
			
			conditional = msg->FindInt32("conditional");
			condition = msg->FindInt32("condition");
			
			//printf("condtional %d, condition %d\n",conditional, condition);
			for (long i = lowSelection; i <= highSelection; i++) {
				ArchiveItem *ai = (ArchiveItem *)ItemAt(i);
				if (ai->selected) {
					ai->conditional = conditional;
					ai->condition = condition;
				}
			}
			
			break;
		}
#endif

#if 0
		case M_TEST_PATCH:
			DoTestPatch(msg);
			break;
#endif
		case M_ITEMS_UPDATED:
			Update();
			break;
		default:
			break;
	}
}

bool PackList::MessageDropped(BMessage *msg, BPoint pt, BPoint offset)
{
	MakeFocus(TRUE);
	MessageReceived(msg);
	return TRUE;
}

PackList::PackList(BRect frame,
			const char *name,
			ulong resizeMask,
			ulong flags)
		: FListView(frame,name,resizeMask,flags)
{
	ArchiveFolderItem *top = new ArchiveFolderItem();
	toplevel = top;
	
	top->destination = D_INSTALL_FOLDER;
	top->SetParent(NULL);
	SetCurrentFolder(top);
	deletePool = NULL;
	
	iconCache = new IconCache(1023);
	gtBitmap = new BBitmap(BRect(0,0,B_MINI_ICON-1,B_MINI_ICON-1),B_COLOR_8_BIT);
}

// bad interface
void PackList::ReadFromDisk(PackArc *archive)
{
	PackWindow *wind = (PackWindow *)Window();
	
	archive->ReadCatalog(toplevel);	
	
	Window()->Lock();
	Update();
	Window()->Unlock();
}
	
void PackList::AttachedToWindow()
{
	FListView::AttachedToWindow();

	const char *t = Window()->Title();	
	if (toplevel->name) free(toplevel->name);
	toplevel->name = strdup(t);
	
	BMessage *upLevel = new BMessage(M_CHANGE_FOLDER_LEVEL);
	upLevel->AddInt32("index",-1);
	
	Window()->AddShortcut(B_UP_ARROW,B_COMMAND_KEY,upLevel,this);
	Window()->AddShortcut(B_DOWN_ARROW,B_COMMAND_KEY,new BMessage(M_INVOKE),this);
	
	SetFont(be_plain_font);
	AddColumn(ICON_TAG,26.0,B_EMPTY_STRING);
	AddColumn(NAME_TAG,140.0,"Name");
	AddColumn(SIZE_TAG,50.0,"Original\nSize");
	AddColumn(COMPRESSED_SIZE_TAG,50.0,"Comp\nSize");
	AddColumn(PERCENTAGE_TAG,30.0,"%");
	AddColumn(FILES_TAG,50.0,"Items");
	
	MakeFocus();
}

// HACK //
#if 0
void PackList::Show()
{
}
#endif

PackList::~PackList()
{
	// void *item;
	
	// make sure these items aren't waiting to be compressed!
	// does this recursively
	
	delete toplevel;
	if (deletePool) {
		for(long i = deletePool->CountItems()-1; i >= 0; i--)
			delete deletePool->ItemAt(i);
		delete deletePool;
	}
	delete iconCache;
	delete gtBitmap;
}

void PackList::DrawItem(BRect updateRect, long index, BRect *itemFrame)
{	
	BRect iFrame;
	if (itemFrame == NULL)
		iFrame = ItemFrame(index);
	else
		iFrame = *itemFrame;
		
	/* 
	if (iFrame.top > updateRect.bottom || iFrame.bottom < updateRect.top)
		return; */
		
	ArchiveItem *li = (ArchiveItem *)ItemAt(index);
	
	// FillRect(iFrame, B_SOLID_LOW);
	SetPenSize(1.0);
	// Draw columns left to right!
	char	truncString[B_FILE_NAME_LENGTH+3];
	char	*truncArray[] = {truncString};
	long max = ColumnCount();
	long columnLeft = 0;
	for(long i = 0; i < max; i++)
	{
		ColumnInfo *cInfo = fColumns->ItemAt(i);
		long columnWidth = (long)cInfo->width;
		switch(cInfo->tag) {
			case ICON_TAG: {
				if (li->smallIcon) {
					BRect iconRect(li->smallIcon->Bounds());
					iconRect.OffsetTo((columnWidth - iconRect.Width())/2.0 + columnLeft,
						(ItemHeight() - iconRect.Height())/2.0 + iFrame.top - 1.0);
					if (typeid(*li) == typeid(ArchiveFileItem)) {
						BBitmap *icon;
						ArchiveFileItem *it = (ArchiveFileItem *)li;
						icon = iconCache->Icon(it->fType, it->fSignature);
						if (icon) {
							SetDrawingMode(B_OP_OVER);
							DrawBitmap(icon,iconRect);
							SetDrawingMode(B_OP_COPY);
							break;
						}
					}
					//PRINT(("small icon is valid\n"));
					SetDrawingMode(B_OP_OVER);
					DrawBitmapAsync(li->smallIcon,iconRect);
					SetDrawingMode(B_OP_COPY);
				}
				break;
			}
			case NAME_TAG: {
				MovePenTo(columnLeft+2,
					iFrame.bottom-4);
				be_plain_font->GetTruncatedStrings((const char **)&li->name,1,B_TRUNCATE_MIDDLE,columnWidth,truncArray);
				DrawString(truncString);
				if (typeid(*li) == typeid(ArchiveLinkItem)) {
					BPoint s(columnLeft+2,
						iFrame.bottom-4);
					s.y += 2;
					BPoint e = s;
					e.x += StringWidth(truncString);
					StrokeLine(	s, e, B_MIXED_COLORS);
				}
				break;
			}
			case SIZE_TAG: {
				MovePenTo(columnLeft+2,
					iFrame.bottom-4);
				char buf[40];
				if (li->uncompressedSize >= 1024)
					sprintf(buf,"%u K",(ulong)(0.5+li->uncompressedSize/1024.0));
				else
					sprintf(buf,"%ubytes",(ulong)li->uncompressedSize);
				float width = StringWidth(buf);
				MovePenTo(columnLeft+columnWidth-8.0-width,iFrame.bottom-4);
				DrawString(buf);
				
				break;
			}
			case COMPRESSED_SIZE_TAG: {
				MovePenTo(columnLeft+2,
					iFrame.bottom-4);
				if (li->compressedSize <= 0) {
					DrawString("---");
				}
				else {
					char buf[40];
					if (li->compressedSize >= 1024) {
						sprintf(buf,"%d K",(long)(0.5+((float)li->compressedSize)/1024.0));
					}
					else
						sprintf(buf,"%dbytes",(long)li->compressedSize);
					float width = StringWidth(buf);
					MovePenTo(columnLeft+columnWidth-8.0-width,iFrame.bottom-4);
					DrawString(buf);
				}
				break;
			}
			case PERCENTAGE_TAG: {
				MovePenTo(columnLeft+2,
					iFrame.bottom-4);
				if (li->compressedSize <= 0) {
					DrawString("---");
				}
				else {
					char buf[40];
					long percent = li->uncompressedSize ? 
							100*(li->uncompressedSize-li->compressedSize)/li->uncompressedSize : 
							0;
					sprintf(buf,"%d\%",max_c(0,percent));
					float width = StringWidth(buf);
					MovePenTo(columnLeft+columnWidth-8.0-width,iFrame.bottom-4);
					DrawString(buf);
				}
				break;
			}
			case FILES_TAG: {
				if (li->IsFolder() == TRUE) {
					ArchiveFolderItem *fi = (ArchiveFolderItem *)li;
					char buf[24];
					int32 count = fi->numCompressibleItems;
					if (fi->seekOffset >= 0) count--;
					sprintf(buf,"%d items",count);
					float width = StringWidth(buf);
					MovePenTo(columnLeft+columnWidth-8.0-width,iFrame.bottom-4);
					DrawString(buf);
				}
				break;
			}
		}
		columnLeft += columnWidth+1;
	}
	if (li && li->selected) {
		// darken
		SetDrawingMode(B_OP_SUBTRACT);
		SetHighColor(60,60,60);
		FillRect(iFrame,B_SOLID_HIGH);
		//InvertRect(iFrame);
		SetDrawingMode(B_OP_COPY);
		SetHighColor(0,0,0);
	}
}

float PackList::ItemHeight()
{
	return 19.0;
	//return FListView::ItemHeight()+BaselineOffset();
}

void PackList::InvalidateItem(long index)
{
	BRect fr = ItemFrame(index);
	// hack to chop out icon and name from updating
	fr.left = 26 + 140;
	Invalidate(fr);
}

void PackList::Invoke(long index)
{
	ArchiveItem *ai = CurrentFolder()->entries->ItemAt(index);
	if (ai->IsFolder() && ((ArchiveFolderItem *)ai)->canEnter == TRUE) {
		DeSelect(index);
		lowSelection = highSelection = -1;
		SetCurrentFolder((ArchiveFolderItem *)ai);
		// SelectionSet();
		
		// add item to menu
		BMessage *folderChangeMsg = new BMessage(M_CHANGE_FOLDER_LEVEL);
		folderChangeMsg->AddPointer("folder",ai);
		
		IconMenuItem *menuitem = new IconMenuItem(ai->name,
							gGenericFolderIcon,folderChangeMsg);
		menuitem->SetTarget(this);
		foldersMenu->AddItem(menuitem);
		
		long count = foldersMenu->CountItems();
		if (count == 1) {
			superItem->SetIcon(gTopLevelIcon);
		}
		else {
			superItem->SetIcon(gGenericFolderIcon);
		}
		BMessage *chnFld = new BMessage(M_CHANGE_FOLDER_LEVEL);
		chnFld->AddInt32("index",count-1);
		Looper()->PostMessage(chnFld,Window()->FindView("Settings"));

		menuitem->SetMarked(TRUE);
		Update();
	}
}

void PackList::ChangeName(BMessage *msg)
{
	ArchiveItem *arcItem;
	if (msg->HasString("text")) {
		msg->FindPointer("item",reinterpret_cast<void **>(&arcItem));
		const char *newn = msg->FindString("text");
		
		if (arcItem->name) free(arcItem->name);
		arcItem->name = strdup(newn);

		/*********
		BMessage *upMsg = new BMessage(M_ITEMS_UPDATED);
		upMsg->AddPointer("item",arcItem);
		PackWindow *wind = (PackWindow *)Window();
		wind->PostMessage(upMsg,wind);
		*********/

		FListView::InvalidateItem(IndexOf(arcItem));
		
		PackWindow *wind = (PackWindow *)Window();
		wind->attribDirty = TRUE;
	}
	else if (lowSelection != -1 && lowSelection == highSelection) {
		ArchiveItem *arcItem = CurrentFolder()->entries->ItemAt(lowSelection);
		BMessage *modm = new BMessage(M_CHANGE_NAME);
		modm->AddPointer("item",arcItem);
		NameDialog *nd = new NameDialog(BRect(0,0,280,80),"Item Name:",
			arcItem->name,modm,this);
	}
}

void PackList::ChangeFolderLevel(BMessage *msg)
{
	long level = msg->FindInt32("index");
	ArchiveFolderItem *newFold;
	
	// level -1 is for Command-UpArrow navigation
	if (level == -1) {
		level = (foldersMenu->CountItems())-2;
		if (level >= 0) newFold = CurrentFolder()->GetParent();
	}
	else {
		msg->FindPointer("folder",reinterpret_cast<void **>(&newFold));
	}
	if (level >= 0) {
		// deselect current items
		if (lowSelection != -1)
			for (long i = lowSelection; i <= highSelection; i++)
				DeSelect(i);
		
		SetCurrentFolder(newFold);
		
		BMessage chnFld(M_CHANGE_FOLDER_LEVEL);
		chnFld.AddInt32("index",level);
				
		Looper()->PostMessage(&chnFld,Window()->FindView("Settings"));
		
		if (level == 0) {
			superItem->SetIcon(gTopLevelIcon);
		}
		else {
			superItem->SetIcon(gGenericFolderIcon);
		}
		for (long i = foldersMenu->CountItems()-1; i > level; i--) {
			BMenuItem *folderitem = foldersMenu->RemoveItem(i);
			delete folderitem;
		}
		BMenuItem *item = foldersMenu->ItemAt(level);
		item->SetMarked(TRUE);
		lowSelection = highSelection = -1;
		// scroll to zero
		ScrollTo(0,0);
		SelectionSet();
		Update();
	}
}

void PackList::ReorderItem(long prevItem, long curItem)
{
	FListView::ReorderItem(prevItem,curItem);
	
	((PackWindow *)Window())->attribDirty = TRUE;
}

void PackList::SelectionSet()
{
	BMessage itemSel(M_ITEMS_SELECTED);
	if (lowSelection != -1) {
		// calculate intersection of groups
		ulong bitmap = 0;
		int32 destination;
		int32 repl = -1;
		bool custom;
		bool commonDest = true;
		bool hasfold = false;
		bool hasfil = false;
		bool hasscr = false;
		int32	archs;
		//int conditional;
		//int condition;
		
		for(long i = lowSelection; i <= highSelection; i++) {
			ArchiveItem *ai = (ArchiveItem *)ItemAt(i);
			if (ai->selected) {
				if (bitmap == 0)
					bitmap = 0xFFFFFFFF;
					bitmap &= ai->groups;
				if (ai->IsFolder()) {
					//bitmap = ai->GetUnionGroups(bitmap);
					hasfold = true;
				}
				else {
					hasfil = true;
					if (cast_as(ai,ArchiveScriptItem))
						hasscr = true;
				}
				
				// first item
				if (i == lowSelection) {
					destination = ai->destination;
					custom = ai->customDest;
					commonDest = true;
					repl = ai->replacement;
					archs = ai->platform;
					//conditional = ai->conditional;
					//condition = ai->condition;
				}
				else {
					// successive items
					if (commonDest) {
						if (ai->destination != destination ||
								ai->customDest != custom)
							commonDest = false;
					}
					if (repl != -1 && (hasscr || repl != ai->replacement))
						repl = -1;
						
					if (archs != 0xFFFFFFFF)
						if (ai->platform != archs) archs = 0xFFFFFFFF;	
					//if (conditional >= 0) {
					//	if (conditional != ai->conditional ||
					//		condition != ai->condition)
					//	{
					//		conditional = -1;
					//		condition = 0;	
					//	}
					//}
				}
			}
		}
		itemSel.AddInt32("bitmap",bitmap);
		itemSel.AddInt32("platform",archs);
		
		// do destination selected
		if (commonDest == true) {
			itemSel.AddInt32("dest",destination);
			itemSel.AddBool("custom",custom);
		}
		else {
			itemSel.AddInt32("dest",D_NO_DEST);
		}			
		
		// M_REPL_SELECTED, here lookup the menu index!!
		if (repl >= 0) {
			for (int i = 0; i < R_END_REPLACE ; i++) {
				if (kReplaceOptions[i].code == repl) {
					repl = i;
					break;
				}
			}
		}
		
		itemSel.AddInt32("replindex",repl);
		itemSel.AddBool("folder",hasfold && !hasfil);
		itemSel.AddBool("script",hasscr);
		//itemSel.AddInt32("conditional",conditional);
		//itemSel.AddInt32("condition",condition);
		
		// not big enough!
		char dependNames[512];
		char *depstr = dependNames;
		dependNames[0] = 0;
		
		if (lowSelection == highSelection) {
			ListItem *it = ItemAt(lowSelection);
			ArchiveFileItem *af;
			
			if (!hasscr && (af = cast_as(it,ArchiveFileItem))) {
				// this is broken, should have a virtual function
				// for DependList
				
				// af = (ArchiveFileItem *)it;
				RList<char *> *dl = af->DependList();
				if (dl) {
					long count = dl->CountItems();
					for (long i = 0; i < count; i++) {
						char *dep = dl->ItemAt(i);
						long len = strlen(dep);
						memcpy(depstr,dep,len);
						depstr += len;
						*depstr++ = ' ';
						*depstr = '\0';
					}
				}
			}
		}
		PRINT(("depends: %s\n",dependNames));
		itemSel.AddString("depends",dependNames);
	}
				
	//////////////// Send the message ///////////////////
	// to the settings view
	BHandler *dest = Window()->FindView("Settings");
	dest->Looper()->PostMessage(&itemSel,dest);
}

void	PackList::SetCurrentFolder(ArchiveFolderItem *fi)
{
	// reset  selections?
	currentlyViewing = fi;
	fList = (RList<ListItem *> *)currentlyViewing->entries;
};

void	PackList::AddEntry(ArchiveItem *addItem)
{
	treeLock.Lock();
	currentlyViewing->entries->AddItem(addItem);
	treeLock.Unlock();
};

void	PackList::RemoveEntry(long index)
{
	treeLock.Lock();
	currentlyViewing->entries->RemoveIndex(index);
	treeLock.Unlock();
}
