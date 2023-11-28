// ===========================================================================
//	Bookmarks.cpp
//  Copyright 1998 by Be Incorporated.
// ===========================================================================

#include "HistoryMenu.h"
#include "FolderWatcher.h"
#include "Bookmarks.h"
#include "NPApp.h"
#include "Strings.h"
#include "Protocols.h"
#include "MessageWindow.h"

#include <Application.h>
#include <Debug.h>
#include <Entry.h>
#include <Path.h>
#include <FindDirectory.h>
#include <MenuItem.h>
#include <malloc.h>
#include <Autolock.h>
#include <stdio.h>
#include <Bitmap.h>

static BookmarkFolder *sBookmarkFolder = NULL;


class AddBookmarkEntryParms {
public:
	BString mURL;
	BString mTitle;
	BookmarkFolder *mBookmarkFolder;
};


class AddMenuParms {
public:
	BMenu *mMenu;
	BookmarkFolder *mBookmarkFolder;
};

#ifdef UPDATEBOOKMARKS
enum {visited = 0, modified, unsure};
#endif

class _EXPORT BookmarkMenuItem;

class BookmarkMenuItem : public BMenuItem {
public:
	BookmarkMenuItem(const char *label, BMessage *message);
	BookmarkMenuItem(BMenu *menu, BMessage *message = NULL);
	BookmarkMenuItem(BMessage *data);
	virtual	status_t		Archive(BMessage *data, bool deep = true) const;
	virtual void Draw();
	static	BArchivable		*Instantiate(BMessage *data);
	
#ifdef UPDATEBOOKMARKS
	void SetVisitState(int state) {mVisitState = state;}
	int VisitState() {return mVisitState;}

protected:
	int			mVisitState;
	static BBitmap*	mNewBookmarkIcon;
	static BBitmap* mUnsureBookmarkIcon;
#endif
};

#ifdef UPDATEBOOKMARKS
BBitmap* BookmarkMenuItem::mNewBookmarkIcon = 0;
BBitmap* BookmarkMenuItem::mUnsureBookmarkIcon = 0;
#endif


BookmarkMenuItem::BookmarkMenuItem(const char *label, BMessage *message)
: BMenuItem(label, message)
{
#ifdef UPDATEBOOKMARKS
		mVisitState = visited;
#endif
}


BookmarkMenuItem::BookmarkMenuItem(BMenu *menu, BMessage *message)
 : BMenuItem(menu, message)
{
#ifdef UPDATEBOOKMARKS
	mVisitState = visited;
#endif
}

void BookmarkMenuItem::Draw()
{
#ifdef UPDATEBOOKMARKS
	if (!mNewBookmarkIcon) {
		mNewBookmarkIcon = TranslateGIF("netpositive:star.gif");
	}
	
	if (!mUnsureBookmarkIcon) {
		mUnsureBookmarkIcon = TranslateGIF("netpositive:question.gif");
	}
#endif
	
	BMenuItem::Draw();
	
#ifdef UPDATEBOOKMARKS
#ifndef R4_COMPATIBLE
	if (mVisitState == modified) {
		Menu()->SetDrawingMode(B_OP_OVER);
		Menu()->DrawBitmapAsync(mNewBookmarkIcon, BPoint(Frame().right - mNewBookmarkIcon->Bounds().Width() - 2, Frame().top + ((Frame().Height() - mNewBookmarkIcon->Bounds().Height()) / 2)));
	} else if (mVisitState == unsure) {
		Menu()->SetDrawingMode(B_OP_OVER);
		Menu()->DrawBitmapAsync(mUnsureBookmarkIcon, BPoint(Frame().right - mUnsureBookmarkIcon->Bounds().Width() - 2, Frame().top + ((Frame().Height() - mUnsureBookmarkIcon->Bounds().Height()) / 2)));
	}
#endif	
#endif
}

status_t BookmarkMenuItem::Archive(BMessage *data, bool deep) const
{
	BMenuItem::Archive(data, deep);
#ifdef UPDATEBOOKMARKS
	data->AddInt32("BookmarkItemVisitState", mVisitState);
#endif
	return 0;
}

BookmarkMenuItem::BookmarkMenuItem(BMessage *data) : BMenuItem(data)
{


#ifdef UPDATEBOOKMARKS
	mVisitState = data->FindInt32("BookmarkItemVisitState");
#endif
}

BArchivable *BookmarkMenuItem::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BookmarkMenuItem"))
		return NULL;
	return new BookmarkMenuItem(data);
}


BookmarkFolder* BookmarkFolder::GetBookmarkFolder()
{
	return sBookmarkFolder;
}

FolderWatcher* BookmarkFolder::CreateFolderWatcher()
{
	BPath settingsPath;
	find_directory(B_USER_SETTINGS_DIRECTORY, &settingsPath);
	settingsPath.Append(kBookmarkFolderName);
	BEntry entry(settingsPath.Path());
	sBookmarkFolder = new BookmarkFolder(entry, kBookmarkMimeType, kApplicationSig, be_app, true, true);
	sBookmarkFolder->mIsInitializing = true;
	
	thread_id tid = spawn_thread(InitThreadEntry, "BookmarkMenu init", B_LOW_PRIORITY, NULL);
	resume_thread(tid);
	NetPositive::AddThread(tid);
	
	return sBookmarkFolder;
}

BookmarkFolder::BookmarkFolder(const BEntry& path, const char *fileType, const char *appSig, BLooper *looper, bool recursive, bool createDir)
	: FolderWatcher(path, fileType, appSig, looper, recursive, createDir, true, false), mListLocker("Bookmark List Lock")
{
	mMasterMenu = new BMenu("Bookmarks");
	mMasterMenu->AddSeparatorItem();
	AddMenu(mMasterMenu);
}

FolderItem* BookmarkFolder::CreateFolderItem() const
{
	return new BookmarkItem;
}

void BookmarkFolder::AddMenu(BMenu *menu)
{
	AddMenuParms *parms = new AddMenuParms;
	parms->mMenu = menu;
	parms->mBookmarkFolder = this;
	
	if (mListLocker.LockWithTimeout(0) != B_OK) {
		thread_id tid = spawn_thread(AddMenuThread, "Add Menu", B_LOW_PRIORITY, parms);
		resume_thread(tid);
		NetPositive::AddThread(tid);
	} else {
		AddMenuThread(parms);
		mListLocker.Unlock();
	}
}

void BookmarkFolder::RemoveMenu(BMenu *menu)
{
	BAutolock lock(mListLocker);
	mMenuList.RemoveItem(menu);
}


int32 BookmarkFolder::InitThreadEntry(void *)
{
	bool createDir;

	BAutolock lock(sBookmarkFolder->mListLocker);
	sBookmarkFolder->Init(&createDir);
	sBookmarkFolder->mIsInitializing = false;

	return 0;
}


void BookmarkFolder::HandleFileChanged(const char *filename, BMessage *msg)
{
	BAutolock lock(mListLocker);
	
	BookmarkItem *hm = (BookmarkItem *)FindItem(filename);
	if (!hm) {
		HandleFileAdded(filename, msg, false);
		return;
	}
	
	BString oldTitle = hm->mTitle;
	BString oldURL = hm->mURL;

	BMessage attrs;
	msg->FindMessage("AttrData", &attrs);
	const char *urlAttr = attrs.FindString(kBookmarkURLAttr);
	bool isDirectory = false;
	msg->FindBool("IsDirectory", &isDirectory);

	if (!isDirectory && (!urlAttr || !(*urlAttr)))
		return;

	hm->ReadMessageAttributes(&attrs);

	for (int i = 0; i < mMenuList.CountItems(); i++) {
		BMenu *menu = (BMenu *)mMenuList.ItemAt(i);
		if (strchr(filename, '/')) {
			BString lastLeafName;
			menu = FindLastSubmenu(menu, filename, lastLeafName);
		}

		if (!menu)
			break;

		for (int j = 0; j < menu->CountItems(); j++) {
			BMenuItem *item = menu->ItemAt(j);
#ifdef UPDATEBOOKMARKS
			BookmarkMenuItem *bmItem = dynamic_cast<BookmarkMenuItem*>(item);
#endif
			BMessage *itemMsg = item->Message();
			if (item && itemMsg && itemMsg->what == OPEN_HIST_BOOKMARK && oldURL == itemMsg->FindString("url")) {
				item->SetLabel(hm->mTitle.String());

#ifdef UPDATEBOOKMARKS
				if (bmItem && hm->mLastVisited != -1 && hm->mLastModified != -1 && (hm->mLastModified > hm->mLastVisited || hm->mLastModified == 0)) {
					if (hm->mLastModified == 0)
						bmItem->SetVisitState(unsure);
					else
						bmItem->SetVisitState(modified);
				} else
					bmItem->SetVisitState(visited);
#endif		
				break;
			}
		}		
	}
}


void BookmarkFolder::HandleFileAdded(const char *filename, BMessage *msg, bool isInitializing)
{

	BMessage attrs;
	msg->FindMessage("AttrData", &attrs);

	BAutolock lock(mListLocker);
	bool foundItem = true;
	const char *urlAttr = attrs.FindString(kBookmarkURLAttr);
	if (!urlAttr)
		urlAttr = attrs.FindString("url");
	// Now add the new entry to the History menu.
	bool isDirectory = false;
	msg->FindBool("IsDirectory", &isDirectory);

	BookmarkItem *hm = 0;
	if (!isDirectory && (!urlAttr || !(*urlAttr)))
		return;
	
	hm = FindItemByURL(attrs.FindString(kBookmarkURLAttr));
	if (!hm)
		hm = (BookmarkItem *)FindItem(filename);
	if (!hm || hm->mAddedToMenu) {
		hm = new BookmarkItem;
		foundItem = false;
	}
	hm->mFilename = filename;
	hm->ReadMessageAttributes(&attrs);
	if (!foundItem)
		mItemList.AddItem(hm);
	hm->mValid = true;
	hm->mIsDirectory = isDirectory;
	hm->mAddedToMenu = true;
	
	if (!isDirectory && hm->mURL.Length() == 0)
		return;
		
	for (int i = 0; i < mMenuList.CountItems(); i++) {
		BMenu *menu = (BMenu *)mMenuList.ItemAt(i);
		BString lastLeafName;
		menu = FindLastSubmenu(menu, filename, lastLeafName);
		if (!menu)
			break;
		if (isDirectory) {
			bool hasSeparator = false;
			for (int i = 0; i < menu->CountItems(); i++) {
				BMenuItem *item = menu->ItemAt(i);
				if (dynamic_cast<BSeparatorItem *>(item)) {
					hasSeparator = true;
					break;
				}
			}
			if (!hasSeparator)
				menu->AddSeparatorItem();
				
			BMessage attrMsg;
			msg->FindMessage("AttrData", &attrMsg);
			const char *menuName = attrMsg.FindString(kBookmarkTitleAttr);
			if (!menuName)
				menuName = lastLeafName.String();
			int32 index = menu->CountItems();
			BMenu *subdir = new BMenu(menuName);
			BMessage *msg = new BMessage;
			msg->AddString("filename", lastLeafName.String());
			BMenuItem *item = new BookmarkMenuItem(subdir, msg);
			menu->AddItem(item, index);
		} else {
			int32 index = menu->CountItems();
			for (int i = 0; i < menu->CountItems(); i++) {
				BMenuItem *item = menu->ItemAt(i);
				if (dynamic_cast<BSeparatorItem *>(item))
					index = i;
			}
			BMessage *msg = new BMessage(OPEN_HIST_BOOKMARK);
			msg->AddString("url", hm->mURL.String());
			const char *name = hm->mTitle.String();
			if (!name)
				name = " ";
				
			BookmarkMenuItem *item = new BookmarkMenuItem(name, msg);

#ifdef UPDATEBOOKMARKS
			if (hm->mLastVisited != -1 && hm->mLastModified != -1 && (hm->mLastModified > hm->mLastVisited || hm->mLastModified == 0)) {
				if (hm->mLastModified == 0)
					item->SetVisitState(unsure);
				else
					item->SetVisitState(modified);
			} else
				item->SetVisitState(visited);
#endif

			menu->AddItem(item, index);
		}
		if (isInitializing)
			snooze(10);
	}
}


void BookmarkFolder::HandleFileRemoved(const char *filename, BMessage *)
{
	if (!filename || !(*filename))
		return;
		
	BAutolock lock(mListLocker);
	
	BookmarkItem *hm = (BookmarkItem *)FindItem(filename);
		
	bool isSubfile = strchr(filename, '/');
	if (isSubfile) {
		BString dirname(filename);
		dirname.Truncate(strchr(filename, '/') - filename);
		int32 numFiles;
		if (CountSubfiles(dirname.String(), numFiles) == B_OK && numFiles == 0)
			RemoveFile(dirname.String());
	}
	
	if (hm) {
		for (int i = 0; i < mMenuList.CountItems(); i++) {
			BMenu *menu = (BMenu *)mMenuList.ItemAt(i);
			if (isSubfile) {
				BString lastLeafName;
				menu = FindLastSubmenu(menu, filename, lastLeafName);
			}

			if (!menu)
				break;

			BMenuItem *item;
			bool removedIt = false;
			for (int j = 0; j < menu->CountItems(); j++) {
				item = menu->ItemAt(j);
				BMessage *itemMsg = item->Message();
				if (itemMsg && itemMsg->what == OPEN_HIST_BOOKMARK && hm->mURL == itemMsg->FindString("url")) {
					menu->RemoveItem(item);
					removedIt = true;
					break;
				}
			}		
			if (!removedIt && (bool)(item = menu->FindItem(filename)) && item->Submenu())
				menu->RemoveItem(item);
		}

		mItemList.RemoveItem(hm);
		delete hm;

	}
}


BMenu *BookmarkFolder::FindLastSubmenu(BMenu *menu, const char *filename, BString& lastLeafName)
{
	const char *curLeaf = filename;
	while (curLeaf) {
		lastLeafName = curLeaf;
		const char *nextLeaf = strchr(curLeaf, '/');
		if (nextLeaf) {
			if (nextLeaf >= curLeaf)
				lastLeafName.Truncate(nextLeaf - curLeaf);
			else
				lastLeafName = "";
			if (lastLeafName.Length() == 0)
				break;
			BMenuItem *menuItem = NULL;
			for (int i = 0; i < menu->CountItems(); i++) {
				BMenuItem *item = menu->ItemAt(i);
				if (item && item->Submenu() && item->Message()) {
					const char *filename = item->Message()->FindString("filename");
					if (lastLeafName == filename) {
						menuItem = item;
						break;
					}
				}
			}

			if (!menuItem)
				break;
			menu = menuItem->Submenu();
			if (!menu)
				break;
		}
		if (nextLeaf)
			curLeaf = nextLeaf + 1;
		else
			curLeaf = NULL;
	}
	return menu;
}


int32 BookmarkFolder::AddEntryThread(void *args)
{
	const char *URL = ((AddBookmarkEntryParms *)args)->mURL.String();
	const char *title = ((AddBookmarkEntryParms *)args)->mTitle.String();
	BookmarkFolder *bookmarkFolder = ((AddBookmarkEntryParms *)args)->mBookmarkFolder;

	BString t;
	if (title && *title)
		t = title;
	else
		t = URL;
	t.Truncate(64);

	BMessage attrData;
	attrData.AddString(kBookmarkURLAttr, URL);
	attrData.AddString(kBookmarkTitleAttr, t.String());

	char *origFilename = (char *)malloc(B_FILE_NAME_LENGTH + 1);
	t.Truncate(32);
	strcpy(origFilename, t.String());
	for (char *n = origFilename; *n; n++)
		if (*n == '/')
			*n = '_';

	bookmarkFolder->AddFile(origFilename, attrData, NULL);

	free(origFilename);
	
	delete (AddBookmarkEntryParms *)args;
	return 0;
}

void BookmarkFolder::AddEntry(const BString& URL, const BString& title)
{

	AddBookmarkEntryParms *args = new AddBookmarkEntryParms;
	args->mURL = URL;
	args->mTitle = title;
	args->mBookmarkFolder = this;
	if (mListLocker.LockWithTimeout(0) != B_OK) {
		thread_id tid = spawn_thread(AddEntryThread, "Add Bookmark Entry", B_LOW_PRIORITY, args);
		resume_thread(tid);
		NetPositive::AddThread(tid);
	} else {
		AddEntryThread(args);
		mListLocker.Unlock();
	}
}


int32 BookmarkFolder::AddMenuThread(void *args)
{
	BMenu *menu = ((AddMenuParms *)args)->mMenu;
	BookmarkFolder *bookmarkFolder = ((AddMenuParms *)args)->mBookmarkFolder;
	
	if (!bookmarkFolder->mListLocker.Lock())
		return 0;
	bookmarkFolder->mMenuList.AddItem(menu);
	
	if (menu != bookmarkFolder->mMasterMenu)
		for (int i = 0; i < bookmarkFolder->mMasterMenu->CountItems(); i++) {
			BMenuItem *item = bookmarkFolder->mMasterMenu->ItemAt(i);
			BMessage msg;
			if (dynamic_cast<BSeparatorItem *>(item))
				menu->AddSeparatorItem();
			else if (item->Archive(&msg, true) == B_OK) {
				BookmarkMenuItem *newItem = new BookmarkMenuItem(&msg);
				menu->AddItem(newItem);
			}
			if (bookmarkFolder->mIsInitializing){
					snooze(10); 
			}
		}
	bookmarkFolder->mListLocker.Unlock();
	return 0;
}


BookmarkItem *BookmarkFolder::FindItemByURL(const char *url)
{
	BAutolock lock(mListLocker);
	// We should use a hashed version of the list.  This is going to be slow.
	for (int i = 0; i < mItemList.CountItems(); i++) {
		BookmarkItem *entry = (BookmarkItem *)mItemList.ItemAt(i);
		if (entry->mURL == url) {
			return entry;
		}
	}
	return NULL;
}


BookmarkItem::BookmarkItem() : mValid(false)
{
}

BookmarkItem::BookmarkItem(const char *URL, const char *title) : mURL(URL), mTitle(title), mValid(false)
{
}

void BookmarkItem::ReadMessageAttributes(BMessage *msg)
{
	const char *tmp = msg->FindString(kBookmarkURLAttr);
	if (!tmp)
		tmp = msg->FindString("url");
	if (!tmp)
		tmp = "";
	mURL = tmp;
	tmp = msg->FindString(kBookmarkTitleAttr);

	if (!tmp)
		tmp = mFilename.String();
	if (!tmp)
		tmp = "";
	mTitle = tmp;
	
#ifdef UPDATEBOOKMARKS
	if (msg->FindInt32(kBookmarkLastModifiedAttr, &mLastModified) != B_OK)
		mLastModified = -1;

	if (msg->FindInt32(kBookmarkLastVisitedAttr, &mLastVisited) != B_OK)
		mLastVisited = -1;
#endif
}


void
BookmarkFolder::Show()
{
	static BPath *settingsPath = NULL;
	if (!settingsPath) {
		settingsPath = new BPath;
		find_directory(B_USER_SETTINGS_DIRECTORY, settingsPath);
		settingsPath->Append(kBookmarkFolderName);
	}
	BEntry entry(settingsPath->Path());

	entry_ref ref;
	entry.GetRef(&ref);

	BMessage msg(B_REFS_RECEIVED);
	msg.AddRef("refs", &ref);	

	BMessenger messenger(kTrackerSig);
	messenger.SendMessage(&msg);
}

#ifdef UPDATEBOOKMARKS
bool BookmarkFolder::SetItemTimes(const char *url, int32 lastVisited, int32 lastModified)
{
//printf("SetItemTimes %s visited %ld modified %ld\n", url, lastVisited, lastModified);
	bool retval = false;
	BAutolock lock(mListLocker);
	for (int i = 0; i < mItemList.CountItems(); i++) {
		BookmarkItem *item = (BookmarkItem *)mItemList.ItemAt(i);
		if (item->mURL == url) {
//printf("Found it\n");
			if (lastVisited != -1)
				item->mLastVisited = lastVisited;
			if (lastModified != -1)
				item->mLastModified = lastModified;
			BMessage msg;
			msg.AddString(kBookmarkURLAttr, item->mURL.String());
			msg.AddString(kBookmarkTitleAttr, item->mTitle.String());
			msg.AddInt32(kBookmarkLastModifiedAttr, item->mLastModified);
			msg.AddInt32(kBookmarkLastVisitedAttr, item->mLastVisited);
			ChangeFile(item->mFilename.String(), msg);
			retval = true;
		}
	}
	return retval;
}

void BookmarkFolder::FindUpdatedBookmarks()
{
	thread_id tid = spawn_thread(UpdateBookmarkThread, "Bookmark Update Searcher", B_LOW_PRIORITY, NULL);
	resume_thread(tid);
	NetPositive::AddThread(tid);
}

extern ConnectionManager *gConnectionMgr;

int32 BookmarkFolder::UpdateBookmarkThread(void *args)
{
	BList listCopy;
	{
		BAutolock lock(sBookmarkFolder->mListLocker);
		listCopy = sBookmarkFolder->mItemList;
	}
	
	for (int i = 0; i < listCopy.CountItems(); i++) {
		BookmarkItem *item = (BookmarkItem *)listCopy.ItemAt(i);
		int32 lastVisited = item->mLastVisited;
		if (item->mLastVisited == -1 || item->mLastModified == -1 || (item->mLastModified > 0 && item->mLastModified <= item->mLastVisited)) {
		 	URLParser parser;
		 	parser.SetURL(item->mURL.String());
			if (parser.GuessScheme() == kHTTP) {
				UResourceImp *imp = new UResourceImp(item->mURL.String(), NULL, NULL, NULL);
				imp->RefCount(1);
				
				pprint("Checking Bookmark %s", item->mURL.String());
				gConnectionMgr->GetHTTP(imp, item->mURL, NULL, NULL, NULL, false, NULL, 0, true);
				
				int waitTime = 0;
				StoreStatus status = kError;
				do {
					snooze(250000);
					status = imp->GetStatus();
					waitTime++;
				} while (status != kComplete && status != kAbort && status != kError && waitTime < 60);
				
//				if (imp->GetLastModified() != 0) {
					if (lastVisited == -1)
						lastVisited = 0;
					pprint("Bookmark %s last visited %ld, last modified %ld", item->mURL.String(), lastVisited, imp->GetLastModified());
					sBookmarkFolder->SetItemTimes(item->mURL.String(), lastVisited, imp->GetLastModified());
//				}
				imp->MarkForDeath();
				imp->RefCount(-1);
			}
		}
	}
	
	NetPositive::RemoveThread(find_thread(NULL));
	return 0;
}
#endif
