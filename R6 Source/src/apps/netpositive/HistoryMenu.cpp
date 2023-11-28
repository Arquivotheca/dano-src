// ===========================================================================
//	HistoryMenu.cpp
//  Copyright 1998 by Be Incorporated.
// ===========================================================================

#include "HistoryMenu.h"
#include "FolderWatcher.h"
#include "Bookmarks.h"
#include "NPApp.h"

#include <Application.h>
#include <Entry.h>
#include <Path.h>
#include <FindDirectory.h>
#include <MenuItem.h>
#include <malloc.h>
#include <time.h>

extern const char *kHistoryFolderName;
extern const char *kApplicationSig;
extern const char *kBookmarkMimeType;
extern const char *kBookmarkURLAttr;
extern const char *kBookmarkTitleAttr;

static int sNumEntriesForToday;
static HistoryFolder *sHistoryFolder = NULL;

class AddEntryParms {
public:
	BString mURL;
	BString mTitle;
	HistoryFolder *mHistoryFolder;
};


HistoryFolder* HistoryFolder::GetHistoryFolder()
{
	return sHistoryFolder;
}

FolderWatcher* HistoryFolder::CreateFolderWatcher()
{
	BPath settingsPath;
	find_directory(B_USER_SETTINGS_DIRECTORY, &settingsPath);
	settingsPath.Append(kHistoryFolderName);
	BEntry entry(settingsPath.Path());
	sHistoryFolder = new HistoryFolder(entry, kBookmarkMimeType, kApplicationSig, be_app, true, true);
	sHistoryFolder->mIsInitializing = true;

	thread_id tid = spawn_thread(InitThreadEntry, "HistoryMenu init", B_NORMAL_PRIORITY - 2, NULL);
	resume_thread(tid);
	NetPositive::AddThread(tid);
	return sHistoryFolder;
}

HistoryFolder::HistoryFolder(const BEntry& path, const char *fileType, const char *appSig, BLooper *looper, bool recursive, bool createDir)
	: BookmarkFolder(path, fileType, appSig, looper, recursive, createDir)
{
}

int32 HistoryFolder::InitThreadEntry(void *)
{
	bool createDir;
	sHistoryFolder->mListLocker.Lock();
	sHistoryFolder->Init(&createDir);
	sHistoryFolder->mListLocker.Unlock();
	sHistoryFolder->mIsInitializing = false;
	
	NetPositive::RemoveThread(find_thread(NULL));
	return 0;
}


void HistoryFolder::HandleFileAdded(const char *filename, BMessage *msg, bool isInitializing)
{
	bool isDirectory = false;
	msg->FindBool("IsDirectory", &isDirectory);
	bool killed = false;

	mListLocker.Lock();

	if (isDirectory && isInitializing) {
		BString dirname(filename);
		dirname.Truncate(strchr(filename, '/') - filename - 1);
		int32 numFiles;
		if (CountSubfiles(dirname.String(), numFiles) == B_OK && numFiles == 0) {
			RemoveFile(dirname.String());
			killed = true;
		}
	}
	
	if (!killed) {
		BookmarkFolder::HandleFileAdded(filename, msg, isInitializing);	
		CheckItemTime(filename, isDirectory);
	}
	
	mListLocker.Unlock();				
}


void HistoryFolder::CheckItemTime(const char *filename, bool isDirectory)
{
	struct tm fileTimeData;
	struct tm currentTimeData;
	
	long fileMod = GetModTime(filename);
	long current = time(NULL);
	
	mListLocker.Lock();
	memcpy(&fileTimeData, localtime(&fileMod), sizeof(tm));
	memcpy(&currentTimeData, localtime(&current), sizeof(tm));
	
	// These aren't accurate wrt leap years, but we need only
	// to compare one to the other, so it doesn't matter.
	unsigned long fileModDay = fileTimeData.tm_year * 365 + fileTimeData.tm_yday;
	unsigned long currentDay = currentTimeData.tm_year * 365 + currentTimeData.tm_yday;

	uint32 daysToKeep = gPreferences.FindInt32("HistoryLength");
	
	if (((currentDay - fileModDay) > (daysToKeep - 1)) ||
		(daysToKeep == 0)) {
		RemoveFile(filename);
		goto done;
	} else if ((currentDay - fileModDay) > 0 && !strchr(filename, '/') && !isDirectory) {
		MoveToFolder(filename);
		goto done;
	}

//printf("NumEntriesForToday: %d  Filename %s\n", sNumEntriesForToday, filename);
	if (!strchr(filename, '/') && !isDirectory)
		if (++sNumEntriesForToday > gPreferences.FindInt32("NumTodaysLinks")) {
			for (int i = 0; i < mItemList.CountItems(); i++) {
				BookmarkItem *hm = (BookmarkItem *)mItemList.ItemAt(i);
//printf("Considering %s\n", hm->mFilename.String());
				if (hm->mFilename.Length() > 0 && !strchr(hm->mFilename.String(), '/') && hm->mValid && !hm->mIsDirectory) {
//printf("Moving %s\n", hm->mFilename.String());
					sNumEntriesForToday--;
					MoveToFolder(hm);
					break;
				}
			}
		}

done:
	mListLocker.Unlock();
}


void HistoryFolder::MoveToFolder(const char *filename)
{
	BookmarkItem *bi = (BookmarkItem *)FindItem(filename);
	if (bi)
		MoveToFolder(bi);
}


void HistoryFolder::MoveToFolder(BookmarkItem *bi)
{
	time_t modTime = GetModTime(bi->mFilename.String());

	char dirname[64];
	struct tm timeData;
	
	mListLocker.Lock();
	memcpy(&timeData, localtime(&modTime), sizeof(tm));
	
	strftime(dirname, 63, "%a, %d %b", &timeData);
	AddDirectory(dirname);

	if (bi)
		bi->mValid = false;
//printf("Moving %s to %s\n", bi->mFilename.String(), dirname);
	MoveFile(bi->mFilename.String(), dirname, true);
	mListLocker.Unlock();
}


time_t HistoryFolder::GetModTime(const char *filename)
{
	BEntry entry;
	GetItemEntry(filename, entry);
	BNode node(&entry);
	struct stat st;
	node.GetStat(&st);
	return st.st_mtime;
}

int32 HistoryFolder::AddEntryThread(void *args)
{
	const char *URL = ((AddEntryParms *)args)->mURL.String();
	const char *title = ((AddEntryParms *)args)->mTitle.String();
	HistoryFolder *historyFolder = ((AddEntryParms *)args)->mHistoryFolder;

	BString t;
	if (title && *title)
		t = title;
	else
		t = URL;
	t.Truncate(48);

	historyFolder->mListLocker.Lock();
	BookmarkItem *hm = historyFolder->FindItemByURL(URL);
	if (hm) {
		bool needsMove = hm->mFilename.Length() > 0 && strchr(hm->mFilename.String(), '/');
		if (URL != hm->mURL || t != hm->mTitle) {
			BMessage msg;
			msg.AddString(kBookmarkURLAttr, URL);
			msg.AddString(kBookmarkTitleAttr, t.String());
			historyFolder->ChangeFile(hm->mFilename.String(), msg);
		}
		if (needsMove) {
			historyFolder->TouchFile(hm->mFilename.String());
			historyFolder->MoveFile(hm->mFilename.String(), "", true);
		}
		historyFolder->mListLocker.Unlock();
		return 0;
	}
	
	hm = new BookmarkItem(URL, title);
	historyFolder->mItemList.AddItem(hm);
	
	BMessage attrData;
	attrData.AddString(kBookmarkURLAttr, URL);
	attrData.AddString(kBookmarkTitleAttr, t.String());

	char *origFilename = (char *)malloc(B_FILE_NAME_LENGTH + 1);
	char *filename = (char *)malloc(B_FILE_NAME_LENGTH + 1);
	t.Truncate(32);
	strcpy(origFilename, t.String());
	for (char *n = origFilename; *n; n++)
		if (*n == '/')
			*n = '_';

	historyFolder->AddFile(origFilename, attrData, filename);
	hm->mFilename = filename;
	historyFolder->mListLocker.Unlock();
	
	free(origFilename);
	free(filename);
	
	delete (AddEntryParms *)args;
	return 0;
}

void HistoryFolder::AddEntry(const BString& URL, const BString& title)
{
	if (gPreferences.FindInt32("HistoryLength") == 0)
		return;
		
	AddEntryParms *args = new AddEntryParms;
	args->mURL = URL;
	args->mTitle = title;
	args->mHistoryFolder = this;
	if (mListLocker.LockWithTimeout(0) != B_OK) {
		thread_id tid = spawn_thread(AddEntryThread, "Add History Entry", B_LOW_PRIORITY + 2, args);
		resume_thread(tid);
		NetPositive::AddThread(tid);
	} else {
		AddEntryThread(args);
		mListLocker.Unlock();
	}
}
