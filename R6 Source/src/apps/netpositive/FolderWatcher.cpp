// ===========================================================================
//	FolderWatcher.cpp
//  Copyright 1998 by Be Incorporated.
// ===========================================================================
 
#include "FolderWatcher.h"

#include <fs_attr.h>
#include <malloc.h>
#include <string.h>
#include <Looper.h>
#include <NodeInfo.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <stdio.h>
#include <time.h>

BList FolderWatcher::mFolderWatcherList;
BList FolderWatcher::mMessageFilterList;
TLocker FolderWatcher::mListLocker("FW List Lock");

#define ONE_SECOND 1000000

// MessageFilterEntry
// ------------------
// The FolderWatcher maintains a list of these, which keep track
// of which BLoopers we have messsage filters inserted into.  The
// entries are reference counted, so they can be inserted only once into
// the BLooper but used multiple times; reference an entry before using it
// and dereference it when done; if Dereference() returns zero, then we're
// finished with it, so remove it from the BLooper and delete it and its
// list entry.
class MessageFilterEntry {
public:
					MessageFilterEntry(filter_hook hookFunction, BLooper *looper) :	
						mFilter(B_NODE_MONITOR, hookFunction), mLooper(looper), mRefCount(1) {}
			
	void			Reference() {mRefCount++;}
	bool			Dereference() {return (--mRefCount == 0);}

	BMessageFilter	mFilter;
	BLooper*		mLooper;

private:
	int32			mRefCount;
};


// NodeEntry
// ---------
// The FolderWatcher maintains a list of these, which allow us to match up node_ref
// values with the file name and other information.  Often, the only data we get in
// a Node Monitor notification is a node_ref, so we need to keep a table to turn this
// into something more useful.
class NodeEntry {
public:
				NodeEntry(const node_ref& ref, const char *name, bool isDirectory);
				~NodeEntry();

	node_ref	mRef;
	char*		mName;
	bool		mCorrectType;
	bool		mIsDirectory;
};


// FolderWatcher::FolderWatcher
// ----------------------------
// Initialize data and add ourselves to the global list of FolderWatchers.  Don't do
// anything dramatic yet.
FolderWatcher::FolderWatcher(const BEntry& path, 
	const char *fileType,
	const char *appSig,
	BLooper *looper,
	bool recursive,
	bool createDir,
	bool handlesMessages,
	bool requireCorrectFileType) :
		mDirectory(&path),
		mDirEntry(0),
		mLooper(looper),
		mMessenger(looper),
		mCreateDir(createDir),
		mRecursive(recursive),
		mItemListLocker("FW Item List Lock"),
		mHandlesMessages(handlesMessages),
		mRequireCorrectFileType(requireCorrectFileType)
{
	// If we have to create the directory from scratch, store a BEntry to it
	// which we will use and dispose of in Init.  We need to do this because the
	// mDirectory member we set up in the constructor will be invalid, as it doesn't
	// point to a real directory, so we can't reconstitute the information from it.
	if (createDir) {
		mDirEntry = new BEntry(path);
		if (mDirEntry->InitCheck() != B_NO_ERROR) {
			delete mDirEntry;
			mDirEntry = 0;
		}
	}
		
	strcpy(mFileType, fileType);
	strcpy(mAppSig, appSig);

	BEntry dirEntry;
	mDirectory.GetEntry(&dirEntry);
	dirEntry.GetPath(&mDirPath);

	// We won't worry about the case where this Lock() fails.
	mListLocker.Lock();
	mFolderWatcherList.AddItem(this);
	mListLocker.Unlock();
}


// FolderWatcher::~FolderWatcher
// -----------------------------
// Clean up any nodes we may still be watching, and remove ourselves from the
// global FolderWatcher list.  Clean up our message filter, too.
FolderWatcher::~FolderWatcher()
{
	node_ref nodeRef;
	// Stop watching our main directory.
	if (mDirectory.GetNodeRef(&nodeRef) == B_NO_ERROR)
		watch_node(&nodeRef, B_STOP_WATCHING, mMessenger);
	
	// Stop watching the files in our directory.
	for (int i = 0; i < mFileList.CountItems(); i++) {
		NodeEntry *nodeEntry = (NodeEntry *)mFileList.ItemAt(i);
		watch_node(&nodeEntry->mRef, B_STOP_WATCHING, mMessenger);
		delete nodeEntry;
	}
	
	for (int i = 0; i < mSubdirectories.CountItems(); i++) {
		BEntry *entry = (BEntry *)mSubdirectories.ItemAt(i);
		delete entry;
	}

	// We won't worry about the case where this Lock() fails.
	mListLocker.Lock();
	mFolderWatcherList.RemoveItem(this);
	
	// Look for our entry in the message filter list and dereference it.
	// If the last user is done with it, then remove it from its BLooper and
	// trash it.
	for (int i = 0; i < mMessageFilterList.CountItems(); i++) {
		MessageFilterEntry *filterEntry = (MessageFilterEntry *)mMessageFilterList.ItemAt(i);
		if (filterEntry->mLooper == mLooper) {
			if (filterEntry->Dereference()) {
				mMessageFilterList.RemoveItem(filterEntry);
				mLooper->Lock();
				mLooper->RemoveCommonFilter(&filterEntry->mFilter);	
				mLooper->Unlock();
				delete filterEntry;
			}
			break;
		}
	}

	mListLocker.Unlock();
}


// FolderWatcher::Init
// -------------------
// Take care of our message filter needs, create the directory if we're told to
// and it doesn't exist, and set up node monitors for the directory and its
// contents.
status_t FolderWatcher::Init(bool *dirCreated)
{
	if (dirCreated)
		*dirCreated = false;
	if (!mLooper->Lock())
		return B_ERROR;
	if (!mListLocker.Lock()) {
		mLooper->Unlock();
		return B_ERROR;
	}
		
	// See if we already have a message filter installed into this BLooper.
	// If so, add a refcount to it.  If not, create and install one.
	MessageFilterEntry *filterEntry = 0;
	for (int i = 0; i < mMessageFilterList.CountItems(); i++) {
		MessageFilterEntry *tmp = (MessageFilterEntry *)mMessageFilterList.ItemAt(i);
		if (tmp->mLooper == mLooper) {
			filterEntry = tmp;
			break;
		}
	}
	if (filterEntry)
		filterEntry->Reference();
	else {
		filterEntry = new MessageFilterEntry(MessageFilter, mLooper);
		mMessageFilterList.AddItem(filterEntry);
		mLooper->AddCommonFilter(&filterEntry->mFilter);
	}

	mListLocker.Unlock();
	mLooper->Unlock();

	status_t status;
	
	// Create the directory if we're supposed to and it isn't there.
	if (mCreateDir && mDirEntry) {
		if (!mDirEntry->Exists()) {
			BPath path;
			status = mDirEntry->GetPath(&mDirPath);
			delete mDirEntry;
			if (status)
				return status;
			BDirectory temp;
			if ((bool)(status = temp.CreateDirectory(mDirPath.Path(), &mDirectory)))
				return status;
			if (dirCreated)
				*dirCreated = true;
		}
	}
	
	// Set up a Node Monitor for the directory.
	BEntry entry;
	mDirectory.GetEntry(&entry);
	return WatchDirectory(entry, true, true, true);
}


// FolderWatcher::MessageReceived
// ------------------------------
void FolderWatcher::MessageReceived(BMessage *msg)
{
	FolderWatcher *folderWatcher;
	if (msg->FindPointer("FolderWatcher", (void **)&folderWatcher) == B_OK
		&& folderWatcher == this) {
		const char *filename = NULL;
		msg->FindString("Filename", &filename);
		if (!filename)
			return;
		switch(msg->what) {
			case kFileAdded:
				HandleFileAdded(filename, msg, false);
				break;

			case kFileRemoved:
				HandleFileRemoved(filename, msg);
				break;
				
			case kFileChanged:
				HandleFileChanged(filename, msg);
				break;
		}
	}
}

void FolderWatcher::HandleFileAdded(const char *filename, BMessage *msg, bool isInitializing)
{
	BMessage attrs;
	msg->FindMessage("AttrData", &attrs);
	FolderItem *item = FindItem(attrs);
	if (!item) {
		item = CreateFolderItem();
		item->ReadMessageAttributes(&attrs);

		mItemListLocker.Lock();
		mItemList.AddItem(item);
		mItemListLocker.Unlock();
	}
	item->mFilename = filename;
	
	if (isInitializing)
		snooze(10000);
}


void FolderWatcher::HandleFileRemoved(const char *filename, BMessage *)
{
	mItemListLocker.Lock();
	FolderItem *item = FindItem(filename);
	if (item) {
		mItemList.RemoveItem(item);
		delete item;
	}
	mItemListLocker.Unlock();
}


void FolderWatcher::HandleFileChanged(const char *filename, BMessage *msg)
{
	mItemListLocker.Lock();
	FolderItem *item = FindItem(filename);
	BMessage attrs;
	if (item && msg->FindMessage("AttrData", &attrs) == B_OK)
		item->ReadMessageAttributes(&attrs);
	mItemListLocker.Unlock();
}



// FolderWatcher::FindItem
// -----------------------
FolderItem* FolderWatcher::FindItem(const char *filename) const
{
	for (int i = 0; i < mItemList.CountItems(); i++) {
		FolderItem *item = (FolderItem *)mItemList.ItemAt(i);
		if (item->mFilename == filename)
			return item;
	}
	return NULL;
}

FolderItem* FolderWatcher::FindItem(const BMessage&) const 
{
	return NULL;
}


// FolderWatcher::MatchFileType
// ----------------------------
// Check the given file to see if it matches the type of the file that we've been
// told to monitor.  Based on the result, set the corresponding flag in the file's
// entry in our list.
bool FolderWatcher::MatchFileType(const BEntry& entry, bool searchFileList) const
{
	BNode node(&entry);
	BNodeInfo nodeInfo(&node);
	char type[B_MIME_TYPE_LENGTH];
	bool retval = true;
	
	if (entry.IsDirectory() && mRecursive)
		return true;
	
	if (status_t err = nodeInfo.GetType(type) != B_NO_ERROR)
		return true;

	if (strcasecmp(type, mFileType) != 0 && mRequireCorrectFileType)
		retval = false;

	if (searchFileList) {
		char filename[B_PATH_NAME_LENGTH];
		// Don't fret if GetRelativePathname returns an error.  That just means
		// that there is no relative pathname.
		GetRelativePathname(entry, filename);
	
		NodeEntry *nodeEntry = SearchFileList(filename);
		if (nodeEntry)
			nodeEntry->mCorrectType = retval;
	}

	return retval;
}


// FolderWatcher::AddFile
// -----------------------
// Create a file in our folder with the filename and given attributes.  If the file
// already exists, it will be clobbered and replaced with a new file.  When the
// directory's node monitor picks up the new file, it will send notification back
// to the BLooper.
status_t FolderWatcher::AddFile(const char *filename,
	const BMessage& attrData,
	char *returnedFilename)
{
	status_t status;
	
	
	// To save ourselves from getting a pile of node monitor messages,
	// create the file in /tmp, write the attributes, and then move it
	// to the directory of interest.
	BDirectory tmpDir("/tmp");
	if((bool)(status = tmpDir.InitCheck()))
		return status;

	// There wasn't a filename specified, so we'll make one up.
	char leafName[B_FILE_NAME_LENGTH + 1];
	GetUniqueFilename(tmpDir, filename, leafName);
	filename = leafName;
	
	BEntry entry(&tmpDir, filename);
	if ((bool)(status = entry.InitCheck()))
		return status;
		
	BFile file;
	if ((bool)(status = tmpDir.CreateFile(filename, &file)))
		return status;
	
	if ((bool)(status = WriteAttributes(file, attrData)))
		return status;
		
	// Set the file's type and preferred app.
	BNodeInfo nodeInfo(&file);
	if ((bool)(status = nodeInfo.SetType(mFileType)))
		return status;
	if ((bool)(status = nodeInfo.SetPreferredApp(mAppSig)))
		return status;
		
	char fullPathname[B_PATH_NAME_LENGTH];
	strcpy(fullPathname, "/tmp/");
	strcat(fullPathname, filename);
	
	if ((bool)(status = MoveFile(fullPathname, "", true, returnedFilename)))
		return status;
		
	return WatchFile(entry, false, false);
}


void FolderWatcher::GetUniqueFilename(const BDirectory& dir,
	const char *origFilename, char *filename) const
{
	// Our made-up filenames are simple numbers.  Sit in a loop, trying out
	// prospective filenames, until we hit one that doesn't exist.
	bool exists;
	bigtime_t number;

	if (origFilename) {
		BEntry entry(&dir, origFilename);
		exists = entry.Exists();
		number = 1;
		if (!exists) {
			strcpy(filename, origFilename);
			return;
		}
	} else {
		exists = true;
		number = real_time_clock_usecs();
	}

	while (exists) {
		if (origFilename)
			sprintf(filename, "%s %Ld", origFilename, number);
		else
			sprintf(filename, "%Ld", number);
		BEntry entry(&dir, filename);
		exists = entry.Exists();
		if (exists)
			number++;
		else
			return;
	}
}


// FolderWatcher::AddDirectory
// ---------------------------
// Add a subdirectory with the given name.
status_t FolderWatcher::AddDirectory(const char *dirname)
{
	BEntry entry(&mDirectory, dirname);
	status_t status;
	
	if ((bool)(status = entry.InitCheck()))
		return status;
		
	BDirectory dummy;
	if ((bool)(status = mDirectory.CreateDirectory(dirname, &dummy)))
		return status;

	if (mRecursive)
		return WatchDirectory(entry, false, true, false);
	else
		return B_OK;
}


// FolderWatcher::CountSubfiles
// ----------------------------
// If the given name refers to a directory, return a count of the number of
// files in that directory.
status_t FolderWatcher::CountSubfiles(const char *dirname, int32& numFiles) const
{
	BDirectory dir(&mDirectory, dirname);
	status_t status;
	
	if ((bool)(status = dir.InitCheck()))
		return status;
		
	numFiles = dir.CountEntries();
	
	return B_OK;
}


// FolderWatcher::RemoveFile
// -------------------------
// Delete the given file from our folder.  When the directory's node monitor picks
// up the deletion, it will send notification back.
status_t FolderWatcher::RemoveFile(const char *filename)
{
	status_t status;
	
	BEntry entry(&mDirectory, filename);
	if ((bool)(status = entry.InitCheck()))
		return status;
		
	return entry.Remove();
}


// FolderWatcher::TouchFile
// ------------------------
// Set the modification time of the given file to the current time.
status_t FolderWatcher::TouchFile(const char *filename) const
{
	status_t status;
	
	BEntry entry(&mDirectory, filename);
	if ((bool)(status = entry.InitCheck()))
		return status;
		
	return entry.SetModificationTime(time(NULL));
}


// FolderWatcher::MoveFile
// -----------------------
// Move the given file to the specified directory
status_t FolderWatcher::MoveFile(const char *fromFilename,
	const char *toDirname,
	bool forceUniqueName,
	char *returnedFilename)
{
	status_t status;
	BEntry fromEntry;
	
	// If the name isn't fully qualified, make it relative to our base directory.
	if (*fromFilename == '/')
		fromEntry.SetTo(fromFilename);
	else
		fromEntry.SetTo(&mDirectory, fromFilename);
		
	if ((bool)(status = fromEntry.InitCheck()))
		return status;
	BDirectory toDirectory(&mDirectory, toDirname);

	char filename[B_FILE_NAME_LENGTH + 1];
	if (forceUniqueName) {
		const char *slashPos = fromFilename;
		do {
			slashPos = strchr(fromFilename, '/');
			if (slashPos)
				fromFilename = slashPos + 1;
		} while (slashPos);
		GetUniqueFilename(toDirectory, fromFilename, filename);
		if (returnedFilename)
			strcpy(returnedFilename, filename);
		return fromEntry.MoveTo(&toDirectory, filename);
	} else {
		if (returnedFilename)
			strcpy(returnedFilename, fromFilename);
		return fromEntry.MoveTo(&toDirectory);
	}
}


// FolderWatcher::GetItemEntry
// ---------------------------
// Retrieve a BEntry for the given file.
status_t FolderWatcher::GetItemEntry(const char *filename, BEntry& entry) const
{
	entry.SetTo(&mDirectory, filename);
	return entry.InitCheck();
}


// FolderWatcher::CreateFolderItem
// -------------------------------
FolderItem* FolderWatcher::CreateFolderItem() const
{
	return NULL;
}


// FolderWatcher::ChangeFile
// -------------------------
// Write out the given attributes to the specified file in our folder.  The new
// attributes will overlay any previous attributes that were there, allowing you
// to change the value of just one attribute if you so wish by putting just one
// attribute spec into the BMessage.
status_t FolderWatcher::ChangeFile(const char *filename,
	const BMessage& attrData) const
{
	status_t status;
	
	BEntry entry(&mDirectory, filename);
	if ((bool)(status = entry.InitCheck()))
		return status;
	
	BFile file(&entry, B_WRITE_ONLY);
	return WriteAttributes(file, attrData);
}


// FolderWatcher::ReadAttributes
// -----------------------------
// Read the attributes from the given file and add them to the BMessage.
status_t FolderWatcher::ReadAttributes(BNode& node,
	BMessage& attrData) const
{
	status_t status;
	void *data = 0;
	
	if ((bool)(status = node.InitCheck()))
		return status;
		
	// Iterate through all of the attributes in the node.
	// For each attribute, get its name, type, data, and size, and
	// create a corresponding entry in the message;
	if ((bool)(status = node.RewindAttrs()))
		return status;
	do {
		char attrName[B_ATTR_NAME_LENGTH];
		if (node.GetNextAttrName(attrName))
			break;

		attr_info attrInfo;
		if (node.GetAttrInfo(attrName, &attrInfo))
			break;
			
		if (data)
			free(data);
		data = malloc(attrInfo.size);
		if (!data)
			break;

		if (node.ReadAttr(attrName, attrInfo.type, 0, data, attrInfo.size) != attrInfo.size)
			break;
		
		if (attrData.AddData(attrName, attrInfo.type, data, attrInfo.size, false))
			break;
	} while (true);

	if (data)
		free(data);
	return B_NO_ERROR;
}


// FolderWatcher::WriteAttributes
// ------------------------------
// Write out the attributes contained in the BMessage to the given file.
status_t FolderWatcher::WriteAttributes(BFile& file,
	const BMessage& attrData) const
{
	status_t status;

	// Iterate through all of the bits of data in the message.
	// For each component, get its name, type, data, and size, and
	// create a corresponding attribute in the file.
	for (int i = 0; i < attrData.CountNames(B_ANY_TYPE); i++) {
		const char *name;
		type_code type;
		if ((bool)(status = attrData.GetInfo(B_ANY_TYPE, i, &name, &type)))
			return status;
		const void *data;
		ssize_t size;
		if ((bool)(status = attrData.FindData(name, type, &data, &size)))
			return status;
		if (file.WriteAttr(name, type, 0, data, size) != size)
			return status;
	}
	return B_NO_ERROR;
}


// FolderWatcher::WatchFile
// ------------------------
// Set up a node monitor on the file, create an entry containing the node
// and file information we need to keep, and save the entry in our file list.
status_t FolderWatcher::WatchFile(const BEntry& entry, bool sendNotification, bool isInitializing, const char *relativePathname)
{
	if (entry.IsDirectory() && mRecursive)
		return WatchDirectory(entry, sendNotification, sendNotification, isInitializing);

	// NOTE:  If we are called upon to watch a file immediately after we get
	// a Node Monitor B_ENTRY_CREATED message on it, its type may not be set
	// yet.  This can happen when copying a file in the Tracker, for instance --
	// we get the creation notification first, but attributes get written later.
	// We don't just want to ignore this file, because it may be valid, but
	// we don't want to assume it will be valid, either.  We need to watch it
	// and send notification back to the class' user later if the type attribute
	// gets set to what we want.  We'll watch the node, but flag it in our list
	// as having the incorrect type and not send notification back just yet.
	status_t status;
	
	char pathname[B_PATH_NAME_LENGTH];
	if (relativePathname)
		strcpy(pathname, relativePathname);
	else if (GetRelativePathname(entry, pathname) != B_NO_ERROR)
		return B_ERROR;

	// If we're already watching this file, return without error.
	if (SearchFileList(pathname)) {
		return B_NO_ERROR;
	}
			
	node_ref dummyRef;
	NodeEntry *nodeEntry = new NodeEntry(dummyRef, pathname, false);
	if ((bool)(status = entry.GetNodeRef(&nodeEntry->mRef)))
		return status;

	// Watch the file and add it to our list.
	if ((bool)(status = watch_node(&nodeEntry->mRef, B_WATCH_ATTR, mMessenger))) {
		return status;
	}
	
	bool matches = MatchFileType(entry, false);
	nodeEntry->mCorrectType = matches;

	mFileList.AddItem(nodeEntry);
	
	if (sendNotification && matches) {
		BMessage msg;
		if ((bool)(status = SendEntryAddedMessage(msg, pathname, false, true, isInitializing)))
			return status;
	}
		
	return B_NO_ERROR;
}


// FolderWatcher::WatchDirectory
// -----------------------------
// Set up a node monitor on the directory and all of the files contained
// within.  If sendDirNotification is true, a notification message will
// be sent for this directory.  If snedFileNotification is true, 
// messages will be sent for all subfiles and subfolders.
status_t FolderWatcher::WatchDirectory(const BEntry &dirEntry, bool sendDirNotification, bool sendFileNotification, bool isInitializing, const char *relativePathname)
{
	status_t status;
	node_ref nodeRef;

	// If we're already watching this file, return without error.
	char pathname[B_PATH_NAME_LENGTH];
	if (relativePathname)
		strcpy(pathname, relativePathname);
	else if (GetRelativePathname(dirEntry, pathname) != B_NO_ERROR)
		return B_ERROR;
	
	if (SearchFileList(pathname)) {
		return B_NO_ERROR;
	}
			
	BDirectory directory(&dirEntry);
	if ((bool)(status = directory.GetNodeRef(&nodeRef)))
		return status;
	if ((bool)(status = watch_node(&nodeRef, B_WATCH_DIRECTORY, mMessenger)))
		return status;
	
	NodeEntry *nodeEntry = new NodeEntry(nodeRef, pathname, true);
	nodeEntry->mCorrectType = true;
	mFileList.AddItem(nodeEntry);

	BEntry dirEntry2;
	mDirectory.GetEntry(&dirEntry2);
	if (dirEntry != dirEntry2) {
		BEntry *entryCopy = new BEntry(dirEntry);
		mSubdirectories.AddItem(entryCopy);
	}
	
	if (mRecursive && sendDirNotification && *pathname) {
		BMessage msg;
		SendEntryAddedMessage(msg, pathname, true, true, isInitializing);
	}

	// Set up Node Monitors for each matching file in
	// the directory, and send FileAdded messages for
	// each one.
	if ((bool)(status = directory.Rewind()))
		return status;
	BEntry entry;
	while (directory.GetNextEntry(&entry) == B_NO_ERROR) {
		// If you're modifying this to recurse into directories,
		// check entry.IsDirectory() and take action here.
		
		char filename[B_PATH_NAME_LENGTH];
		BString path = pathname;
		if (path.Length() > 0)
			path += "/";
		entry.GetName(filename);
		path += filename;

		if (entry.IsFile() && MatchFileType(entry, false)) {
			if ((bool)(status = WatchFile(entry, sendFileNotification, isInitializing, path.String())))
				return status;
		} else if (entry.IsDirectory() && mRecursive) {
			if ((bool)(status = WatchDirectory(entry, sendFileNotification, sendFileNotification, isInitializing, path.String()))) {
				return status;
			}
		}
	}	
	
	return B_NO_ERROR;
}


// FolderWatcher::StopWatching
// ---------------------------
// Remove the node monitor from the given file and the file's entry from our file
// list.
status_t FolderWatcher::StopWatching(const char *filename)
{
	BEntry entry(&mDirectory, filename);
	status_t result;
	
	if ((bool)(result = entry.InitCheck()))
		return result;
	
	if (entry.IsDirectory()) {
		for (int i = 0; i < mSubdirectories.CountItems(); i++) {
			BEntry *subdir = (BEntry *)mSubdirectories.ItemAt(i);
			if (entry == *subdir) {
				mSubdirectories.RemoveItem(subdir);
				delete subdir;
			}
		}
	}

	NodeEntry *nodeEntry = SearchFileList(filename);
	if (nodeEntry) {
		mFileList.RemoveItem(nodeEntry);
		result = watch_node(&nodeEntry->mRef, B_STOP_WATCHING, mMessenger);
		delete nodeEntry;
		return result;
	}
	return B_ERROR;
}


// FolderWatcher::SendEntryAddedMessage
// ------------------------------------
// Modify the supplied BMessage to turn it into a kFileAdded message suitable for
// this class' user, using the filename given to us.  If we are instructed to do
// so, we can actually send the message, or we can rely on the caller to take
// care of it (despite the name of this function).
status_t FolderWatcher::SendEntryAddedMessage(BMessage& message, const char *filename, bool isDirectory, bool send, bool isInitializing, const BEntry *entry)
{
	message.MakeEmpty();
	message.what = kFileAdded;
	message.AddString("Filename", filename);
	message.AddPointer("FolderWatcher", this);
	message.AddBool("IsDirectory", isDirectory);
	message.AddBool("IsInitializing", isInitializing);

	BMessage attrData;

	BNode node;
	if (entry)
		node.SetTo(entry);
	else
		node.SetTo(&mDirectory, filename);

	if (ReadAttributes(node, attrData) == B_NO_ERROR)
		message.AddMessage("AttrData", &attrData);

	if (send)
		if (mHandlesMessages)
			HandleFileAdded(filename, &message, isInitializing);
		else
			return mMessenger.SendMessage(&message, (BHandler *)NULL, ONE_SECOND);
	return B_OK;
}


// FolderWatcher::SendEntryChangedMessage
// --------------------------------------
// Modify the supplied BMessage to turn it into a kFileChanged message suitable for
// this class' user, using the filename given to us.  If we are instructed to do
// so, we can actually send the message, or we can rely on the caller to take
// care of it (despite the name of this function).
status_t FolderWatcher::SendEntryChangedMessage(BMessage& message, const char *filename, bool isDirectory, bool send, bool isInitializing)
{
	// Since this message is so like the EntryAddedMessage, we'll use its code.
	SendEntryAddedMessage(message, filename, isDirectory, false, isInitializing);
	message.what = kFileChanged;

	if (send)
		if (mHandlesMessages)
			HandleFileChanged(filename, &message);
		else
			return mMessenger.SendMessage(&message, (BHandler *)NULL, ONE_SECOND);
	return B_OK;
}


// FolderWatcher::SendEntryRemovedMessage
// --------------------------------------
// Modify the supplied BMessage to turn it into a kFileRemoved message suitable for
// this class' user, using the filename given to us.  If we are instructed to do
// so, we can actually send the message, or we can rely on the caller to take
// care of it (despite the name of this function).
status_t FolderWatcher::SendEntryRemovedMessage(BMessage& message, const char *filename, bool isDirectory, bool send, bool isInitializing)
{
	message.MakeEmpty();
	message.what = kFileRemoved;
	message.AddString("Filename", filename);
	message.AddPointer("FolderWatcher", this);
	message.AddBool("IsDirectory", isDirectory);
	message.AddBool("IsInitializing", isInitializing);

	if (send)
		if (mHandlesMessages)
			HandleFileRemoved(filename, &message);
		else
			return mMessenger.SendMessage(&message, (BHandler *)NULL, ONE_SECOND);
	return B_OK;
}


// FolderWatcher::GetRelativePathname
// ----------------------------------
// Given a BEntry, return a pathame that is relative to the root directory
// that the FolderWatcher is watching.
status_t FolderWatcher::GetRelativePathname(const BEntry& entry, char *pathname) const
{
	*pathname = 0;
//	BEntry dirEntry;
//	mDirectory.GetEntry(&dirEntry);
//	BPath dirPath;
//	dirEntry.GetPath(&dirPath);

	unsigned int dirPathLen;
	if (!mDirPath.Path())
		dirPathLen = 0;
	else
		dirPathLen = strlen(mDirPath.Path());
	
	BPath filePath(&entry);
	
	if (!filePath.Path() || !*(filePath.Path()))
		return B_OK;
	
	if (dirPathLen && strncmp(mDirPath.Path(), filePath.Path(), dirPathLen) != 0)
		return B_ERROR;
	
	if (dirPathLen == strlen(filePath.Path()))
		return B_OK;
		
	strcpy(pathname, filePath.Path() + dirPathLen + 1);
	return B_OK;
}


// FolderWatcher::SearchFileList
// -----------------------------
// Search the list of watched files to find one that matches the supplied node_ref.
// If it is found, then set filename to point to its name and return the index
// of the node entry in the list.
int32 FolderWatcher::SearchFileList(const node_ref& nodeRef, const char **filename) const
{
	for (int i = 0; i < mFileList.CountItems(); i++) {
		NodeEntry *listEntry = (NodeEntry *)mFileList.ItemAt(i);
		if (listEntry->mRef == nodeRef) {
			if (filename)
				*filename = listEntry->mName;
			return i;
		}
	}
	return -1;
}


// FolderWatcher::SearchFileList
// -----------------------------
// Search the list of watched files to find one that matches the supplied filename.
// If it is found, return a pointer to its node entry in the file list.
NodeEntry* FolderWatcher::SearchFileList(const char *filename) const
{
	for (int i = 0; i < mFileList.CountItems(); i++) {
		NodeEntry *listEntry = (NodeEntry *)mFileList.ItemAt(i);
		if (strcmp(listEntry->mName, filename) == 0) {
			return listEntry;
		}
	}
	return 0;
}


// FolderWatcher::FindFolderWatcher
// --------------------------------
// Search through the global list of FolderWatchers to see if one of them is watching the file
// with the given node ref.  If so, then fill out the filename parameter with the filename of the
// file and return the FolderWatcher that is responsible for it.
FolderWatcher* FolderWatcher::FindFolderWatcher(const node_ref& nodeRef, const char **filename)
{
	if (!mListLocker.Lock())
		return 0;
	for (int i = 0; i < mFolderWatcherList.CountItems(); i++) {
		FolderWatcher *watcher = (FolderWatcher *)mFolderWatcherList.ItemAt(i);
		if (watcher->SearchFileList(nodeRef, filename) >= 0) {
			mListLocker.Unlock();
			return watcher;
		}
	}
	mListLocker.Unlock();
	return 0;
}


// FolderWatcher::FindFolderWatcher
// --------------------------------
// Search through the global list of FolderWatchers to see if one of them is watching the given
// directory.  If so, return a pointer to it.
FolderWatcher* FolderWatcher::FindFolderWatcher(const BDirectory& directory)
{
	if (!mListLocker.Lock())
		return 0;
		
	BEntry entry1,entry2;
	directory.GetEntry(&entry1);
	
	for (int i = 0; i < mFolderWatcherList.CountItems(); i++) {
		FolderWatcher *folderWatcher = (FolderWatcher *)mFolderWatcherList.ItemAt(i);
		folderWatcher->mDirectory.GetEntry(&entry2);
		
		if (entry1 == entry2)
			return folderWatcher;
			
		for (int j = 0; j < folderWatcher->mSubdirectories.CountItems(); j++) {
			BEntry *entry3 = (BEntry *)folderWatcher->mSubdirectories.ItemAt(j);
			if (entry1 == *entry3)
				return folderWatcher;
		}
	}
		
	mListLocker.Unlock();
	return 0;
}


// FolderWatcher::MessageFilter
// ----------------------------
// This is the hook function of the message filter that we install in the BLooper of our
// client.  It intercepts Node Monitor messages, and if they apply to one of the nodes we are
// monitoring, we munge the message and convert it into a format that our client expects.  As
// necessary, we may also send additional messages if the situation warrants.
filter_result FolderWatcher::MessageFilter(BMessage *message,
	BHandler **,
	BMessageFilter *)
{
	int32 opcode;
	if (message->FindInt32("opcode", &opcode) != B_NO_ERROR)
		return B_DISPATCH_MESSAGE;
	switch(opcode) {
		case B_ATTR_CHANGED: {
			ino_t node;
			dev_t device;
			if (message->FindInt32("device", &device) != B_NO_ERROR ||
				message->FindInt64("node", &node) != B_NO_ERROR)
				break;
				
			const char *filename;
			node_ref nodeRef;
			nodeRef.device = device;
			nodeRef.node = node;
			FolderWatcher *folderWatcher;
			
			if (!((bool)(folderWatcher = FindFolderWatcher(nodeRef, &filename))))
				break;

			// Take special care here.  If this node is being monitored but isn't
			// marked as having the correct type (see FolderWatcher::WatchFile), 
			// then see if its type has changed to something favorable, and send
			// a kFileAdded message instead of a kFileChanged message.  If it is
			// marked as the correct type, but the type changes to something
			// unfavorable (an unlikely case), then send a kFileRemoved message.
			message->MakeEmpty();
			
			NodeEntry *nodeEntry = folderWatcher->SearchFileList(filename);
			if (!nodeEntry)
				break;
			BEntry entry(&folderWatcher->mDirectory, filename);
			if (entry.InitCheck() != B_NO_ERROR)
				break;

			if (!nodeEntry->mCorrectType && folderWatcher->MatchFileType(entry)) {
				nodeEntry->mCorrectType = true;
				folderWatcher->SendEntryAddedMessage(*message, filename, false, false, false);
			} else if (nodeEntry->mCorrectType && !(folderWatcher->MatchFileType(entry))) {
				folderWatcher->SendEntryRemovedMessage(*message, filename, false, false, false);
				nodeEntry->mCorrectType = false;
			} else if (nodeEntry->mCorrectType) {
				folderWatcher->SendEntryChangedMessage(*message, filename, false, false, false);
			}
			break;
		}

		case B_ENTRY_MOVED: {
			ino_t fromDirectoryNode;
			ino_t toDirectoryNode;
			dev_t device;
			ino_t node;
			const char *tmpFrom;
			const char *tmpTo;
			if (message->FindInt32("device", &device) != B_NO_ERROR ||
				message->FindInt64("from directory", &fromDirectoryNode) != B_NO_ERROR ||
				message->FindInt64("to directory", &toDirectoryNode) != B_NO_ERROR ||
				message->FindInt64("node", &node) != B_NO_ERROR ||
				message->FindString("name", &tmpTo) != B_NO_ERROR)
				break;
				
			char toName[B_PATH_NAME_LENGTH];
			strcpy(toName, tmpTo);
			
			node_ref nodeRef;
			nodeRef.node = node;
			nodeRef.device = device;

			entry_ref toEntryRef(device, toDirectoryNode, toName);
			BEntry toEntry(&toEntryRef);
			BDirectory toDirectory("/");
			if (toEntry.InitCheck() != B_NO_ERROR ||
				toEntry.GetParent(&toDirectory) != B_NO_ERROR)
				break;

			// Get the filename stored for the node when we started watching it.
			// We can't report the filename reported to us in the message, because
			// the filename has changed if the item was moved into the trash and an
			// identically named item already existed there.
			
			// Handle the case where a file has been moved from a folder we're watching.
			// It may or may not being moved *to* a folder we're watching.
			if (FindFolderWatcher(nodeRef, &tmpFrom)) {
				char fromName[B_PATH_NAME_LENGTH];
				strcpy(fromName, tmpFrom);
				while ((bool)(tmpFrom = strchr(fromName, '/')))
					strcpy(fromName, tmpFrom + 1);
				
				// Create BEntry and BDirectory objects for the nodes and their parents,
				// respectively.
				entry_ref fromEntryRef(device, fromDirectoryNode, fromName);
				BEntry fromEntry(&fromEntryRef);
				BDirectory fromDirectory("/");
				
				if (fromEntry.InitCheck() != B_NO_ERROR ||
					fromEntry.GetParent(&fromDirectory) != B_NO_ERROR)
					break;
					
				bool messageChanged = false;							
				
				// See if there's a FolderWatcher for the From directory.  If so,
				// send a notification message back.
				FolderWatcher *fromFolderWatcher = FindFolderWatcher(fromDirectory);
				if (fromFolderWatcher && fromFolderWatcher->SearchFileList(nodeRef) != -1) {
					fromFolderWatcher->GetRelativePathname(fromEntry, fromName);						
					fromFolderWatcher->StopWatching(fromName);
					if (fromFolderWatcher->MatchFileType(toEntry)) {
						fromFolderWatcher->SendEntryRemovedMessage(*message, fromName, fromEntry.IsDirectory(), false, false);
						messageChanged = true;
					}
				}

				// See if there's a FolderWatcher for the To directory.  If so,
				// send a notification message back.  If both from and to are
				// the same folder watcher, then drop the added message on the
				// floor for the moment.  We'll get another B_ENTRY_MOVED message
				// right after this one, and we'll pick it up then.  Since the nodeRef
				// has been removed from the list, the code above won't find it and won't
				// send an entry removed message.
				FolderWatcher *toFolderWatcher = FindFolderWatcher(toDirectory);
				if (toFolderWatcher && (toFolderWatcher != fromFolderWatcher || toDirectory == fromDirectory) && toFolderWatcher->SearchFileList(nodeRef) == -1) {
					toFolderWatcher->GetRelativePathname(toEntry, toName);						
					toFolderWatcher->WatchFile(toEntry, false, false);
					if (toFolderWatcher->MatchFileType(toEntry))
						if (!messageChanged)
							toFolderWatcher->SendEntryAddedMessage(*message, toName, toEntry.IsDirectory(), false, false);
						else {
							// We've already used up the message we were sent, but
							// we need to send another message.  Build one and fire
							// it off.  Don't overwrite the old message, because it
							// will be passed on to the BLooper when this filter is
							// done with it; thus, the message we are creating here
							// will be received after the kEntryRemoved message.
							BMessage msg;
							toFolderWatcher->SendEntryAddedMessage(msg, toName, toEntry.IsDirectory(), true, false);
						}								
				}
				break;
			} else if (FindFolderWatcher(toDirectory)) {
				// The file has been moved into a folder we're watching, but we're ignorant
				// of the folder it's coming from.  Convert this message into a
				// B_ENTRY_CREATED message and fall through to the code that handles it.
				message->what = B_ENTRY_CREATED;
				message->AddInt64("directory", toDirectoryNode);
			}
		}
		
		case B_ENTRY_CREATED: {	
			ino_t dir;
			dev_t device;
			const char *name;
			if (message->FindInt32("device", &device) != B_NO_ERROR ||
				message->FindInt64("directory", &dir) != B_NO_ERROR ||
				message->FindString("name", &name) != B_NO_ERROR)
				break;
				
			char filename[B_PATH_NAME_LENGTH];
			strcpy(filename, name);
			entry_ref entryRef(device, dir, filename);
			BEntry entry(&entryRef);
			BDirectory directory("/");
			
			// Find the parent directory of the node that was created.  Find a FolderWatcher
			// for it and send the notification.
			if (entry.InitCheck() != B_NO_ERROR || entry.GetParent(&directory) != B_NO_ERROR)
				break;

			FolderWatcher *folderWatcher = FindFolderWatcher(directory);
			if (!folderWatcher)
				break;

			folderWatcher->WatchFile(entry, false, false);
			if (!folderWatcher->MatchFileType(entry))
				break;
				
			folderWatcher->GetRelativePathname(entry, filename);
			folderWatcher->SendEntryAddedMessage(*message, filename, entry.IsDirectory(), false, false);
			break;
		}

		case B_ENTRY_REMOVED: {		
			ino_t directory;
			dev_t device;
			ino_t node;
			if (message->FindInt32("device", &device) != B_NO_ERROR ||
				message->FindInt64("directory", &directory) != B_NO_ERROR ||
				message->FindInt64("node", &node) != B_NO_ERROR)
				break;
				
			const char *tmpName;
			node_ref nodeRef;
			nodeRef.device = device;
			nodeRef.node = node;
			FolderWatcher *folderWatcher;

			// Find the parent directory of the node that was deleted.  Find a FolderWatcher
			// for it and send the notification.
			if (!((bool)(folderWatcher = FindFolderWatcher(nodeRef, &tmpName))))
				break;
				
			char filename[B_PATH_NAME_LENGTH];
			strcpy(filename, tmpName);
			NodeEntry *nodeEntry = folderWatcher->SearchFileList(filename);
			if (nodeEntry && nodeEntry->mCorrectType)
				folderWatcher->SendEntryRemovedMessage(*message, filename, nodeEntry->mIsDirectory, false, false);

			folderWatcher->StopWatching(filename);
				
			break;
		}

	}
	return B_DISPATCH_MESSAGE;
}


// NodeEntry::NodeEntry
// --------------------
NodeEntry::NodeEntry(const node_ref& ref, const char *name, bool isDirectory) : mRef(ref), mIsDirectory(isDirectory)
{
	mName = (char *)malloc(strlen(name) + 1);
	strcpy(mName, name);
	mCorrectType = false;
}


// NodeEntry::~NodeEntry
// ---------------------
NodeEntry::~NodeEntry()
{
	free(mName);
}

