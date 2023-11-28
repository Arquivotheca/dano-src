// ===========================================================================
//	HistoryMenu.h
//  Copyright 1998 by Be Incorporated.
// ===========================================================================

#ifndef __HISTORYMENU__
#define __HISTORYMENU__

#include "Bookmarks.h"

class HistoryFolder : public BookmarkFolder {
public:
static FolderWatcher*	CreateFolderWatcher();
static HistoryFolder*	GetHistoryFolder();
	virtual void		HandleFileAdded(const char *filename, BMessage *msg, bool isInitializing);
	virtual void		AddEntry(const BString& URL, const BString& title);

protected:
						HistoryFolder(const BEntry& path, const char *fileType, const char *appSig, BLooper *looper, bool recursive, bool createDir);
static int32			InitThreadEntry(void *data);
	void				CheckItemTime(const char *filename, bool isDirectory);
	void				MoveToFolder(const char *filename);
	void				MoveToFolder(BookmarkItem *bi);
	time_t				GetModTime(const char *filename);
static int32			AddEntryThread(void *args);
};

#endif
