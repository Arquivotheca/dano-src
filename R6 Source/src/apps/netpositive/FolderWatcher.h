// ===========================================================================
//	FolderWatcher.h
//  Copyright 1998 by Be Incorporated.
// ===========================================================================
 
#ifndef __FOLDERWATCHER__
#define __FOLDERWATCHER__

#include <Directory.h>
#include <List.h>
#include <Locker.h>
#include <MessageFilter.h>
#include <Messenger.h>
#include <String.h>
#include <Path.h>

#include "Utils.h"

class NodeEntry;
class FolderItem;

// FolderWatcher
// -------------
// This class sets up a Node Monitor on a folder and all of the files within it.  It
// inserts a message filter into a BLooper that you supply and intercepts the Node
// Monitor messages sent to that BLooper, transforming them into a more useful form.
// For information on how to use this class, see the Be Newsletter, Volume II, Issue 19,
// "The Tracker is Your Friend".
class FolderWatcher {
public:
						FolderWatcher(const BEntry& path, 
							  		  const char *fileType,
							  		  const char *appSig,
							  		  BLooper *looper, 
							  		  bool recursive,
							  		  bool createDir,
									  bool handlesMessages = false,
									  bool requireCorrectFileType = true);
	virtual				~FolderWatcher();
	
	status_t			Init(bool *dirCreated = NULL);

	status_t			AddFile(const char *filename,
								const BMessage& attrData,
								char *returnedFilename = NULL);
	status_t			AddDirectory(const char *dirname);
	status_t			RemoveFile(const char *filename);
	status_t			ChangeFile(const char *filename,
						 		   const BMessage &attrData) const;
	status_t			MoveFile(const char *fromFilename,
								 const char *toDirname,
								 bool forceUniqueName,
								 char *returnedFilename = NULL);
	status_t			TouchFile(const char *filename) const;
	status_t			CountSubfiles(const char *dirname,
									  int32& numFiles) const;
								 
	status_t			GetItemEntry(const char *filename,
									 BEntry &entry) const;
	
	virtual void		MessageReceived(BMessage *msg);
	virtual FolderItem*	FindItem(const char *filename) const;
	virtual FolderItem* FindItem(const BMessage& attrs) const;
	virtual FolderItem*	CreateFolderItem() const;
	virtual void		HandleFileAdded(const char *filename, BMessage *msg, bool isInitializing);
	virtual void		HandleFileRemoved(const char *filename, BMessage *msg);
	virtual void		HandleFileChanged(const char *filename, BMessage *msg);
	
static const uint32		kFileAdded   = 'FWfa';
static const uint32		kFileRemoved = 'FWfr';
static const uint32		kFileChanged = 'FWfc';
	
	
protected:
	status_t			WatchFile(const BEntry& entry,
								  bool sendNotification,
								  bool isInitializing,
								  const char *relativePathname = NULL);
	status_t			WatchDirectory(const BEntry& entry,
									   bool sendDirNotification,
									   bool sendFileNotification,
									   bool isInitializing,
									   const char *relativePathname = NULL);
	status_t			StopWatching(const char *filename);
	
	status_t			GetRelativePathname(const BEntry& entry, char *pathname) const;

	status_t			ReadAttributes(BNode& node,
									   BMessage& attrData) const;
	status_t			WriteAttributes(BFile& file,
										const BMessage& attrData) const;

	int32				SearchFileList(const node_ref& nodeRef,
									   const char **filename = 0) const;
	NodeEntry*			SearchFileList(const char *filename) const;
	bool				MatchFileType(const BEntry& entry,
									  bool searchFileList = true) const;
	void				GetUniqueFilename(const BDirectory& dir,
										  const char *origFilename,
										  char *filename) const;

	status_t			SendEntryAddedMessage(BMessage &message,
											  const char *filename,
											  bool isDirectory,
											  bool send,
											  bool isInitializing,
											  const BEntry *entry = 0);
	status_t			SendEntryRemovedMessage(BMessage& message,
												const char *filename,
												bool isDirectory,
												bool send,
												bool isInitializing);
	status_t			SendEntryChangedMessage(BMessage& message,
												const char *filename,
												bool isDirectory,
												bool send,
												bool isInitializing);

static filter_result	MessageFilter(BMessage *message,
									  BHandler **target,
									  BMessageFilter *filter);

static FolderWatcher*	FindFolderWatcher(const BDirectory& directory);
static FolderWatcher*	FindFolderWatcher(const node_ref& nodeRef,
										  const char **filename = 0);
	
	BDirectory			mDirectory;
	BPath				mDirPath;
	BList				mSubdirectories;
	BEntry				*mDirEntry;
	BList				mFileList;
	char				mFileType[B_MIME_TYPE_LENGTH];
	char				mAppSig[B_MIME_TYPE_LENGTH];
	BLooper*			mLooper;
	BMessenger			mMessenger;
	bool				mCreateDir;
	bool				mRecursive;
	BList				mItemList;
	TLocker				mItemListLocker;
	bool				mHandlesMessages;
	bool				mRequireCorrectFileType;
	
static BList			mFolderWatcherList;
static BList			mMessageFilterList;
static TLocker			mListLocker;
};


class FolderItem {
public:
	virtual void ReadMessageAttributes(BMessage *msg) = 0;
			void SetFilename(const char *filename) {mFilename = filename;}
	virtual		 ~FolderItem() {}

	BString		mFilename;
};

#endif
