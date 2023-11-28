// ===========================================================================
//	Bookmarks.h
//  Copyright 1998 by Be Incorporated.
// ===========================================================================

#ifndef __BOOKMARKS__
#define __BOOKMARKS__

#include "FolderWatcher.h"

#include <SupportDefs.h>
#include <String.h>
class BMenu;

class BookmarkItem;

class BookmarkFolder : public FolderWatcher {
public:
static FolderWatcher*	CreateFolderWatcher();
static BookmarkFolder*	GetBookmarkFolder();
	virtual FolderItem* CreateFolderItem() const;
	virtual void		HandleFileAdded(const char *filename, BMessage *msg, bool isInitializing);
	virtual void		HandleFileRemoved(const char *filename, BMessage *msg);
	virtual void		HandleFileChanged(const char *filename, BMessage *msg);
	virtual void		AddEntry(const BString& URL, const BString& title);
	virtual void		AddMenu(BMenu *menu);
	virtual void		RemoveMenu(BMenu *menu);
	virtual void		Show();

#ifdef UPDATEBOOKMARKS
	virtual bool		SetItemTimes(const char *url, int32 lastVisited, int32 lastModified);
	virtual void		FindUpdatedBookmarks();
#endif

	
protected:
						BookmarkFolder(const BEntry& path, const char *fileType, const char *appSig, BLooper *looper, bool recursive, bool createDir);
static int32			InitThreadEntry(void *data);
	void				MoveToFolder(const char *filename);
	BMenu*				FindLastSubmenu(BMenu *menu, const char *filename, BString& lastLeafName);
static int32			AddMenuThread(void *args);
static int32			AddEntryThread(void *args);
	BookmarkItem*		FindItemByURL(const char *url);

	TLocker				mListLocker;
	BList				mMenuList;
	BMenu*				mMasterMenu;
	bool				mIsInitializing;

#ifdef UPDATEBOOKMARKS
	void				CheckItemTime(const char *filename, bool isDirectory);
	static int32			UpdateBookmarkThread(void *args);
#endif
};


class BookmarkItem : public FolderItem {
public:
						BookmarkItem();
						BookmarkItem(const char *URL, const char *title);

	virtual void		ReadMessageAttributes(BMessage *msg);

	BString 			mURL;
	BString 			mTitle;
	unsigned		mValid : 1;
	unsigned		mAddedToMenu : 1;
	unsigned		mIsDirectory : 1;

#ifdef UPDATEBOOKMARKS
	int32				mLastModified;
	int32				mLastVisited;
	uint32				mChecksum;
#endif
};

#endif
