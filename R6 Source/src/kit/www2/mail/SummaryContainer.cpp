/*
	SummaryContainer.cpp
*/

#include <Binder.h>
#include "BufferedFileAdapter.h"
#include <BufferIO.h>
#include <Directory.h>
#include <File.h>
#include <IMAP.h>
#include <InterfaceDefs.h>
#include "MailDebug.h"
#include <SmartArray.h>
#include <stdio.h>
#include <StringBuffer.h>
#include "SummaryContainer.h"
#include "www/util.h"

static const char * const kCacheBase = "/boot/binder/user";
static const char * const kMailCache = "mail_cache";
static const char * const kInbox = "inbox";
static const char * const kDrafts = "Drafts";
static const char * const kSentItems = "Sent Items";
static const char * const kSummaryFileName = "summary.toc";

int32 sc = 0;

SummaryContainer::SummaryContainer()
	:	BAtom(),
		fSummaryLock("summary-container-lock")
{
	MDB(MailDebug md);
	fSc = sc++;
	DB(md.Print("Created summary container with id = '%ld'\n", fSc));
	
	memset(fEntries, 0, kEntryHashSize * sizeof(SummaryEntry *));
	Clear();
}

SummaryContainer::SummaryContainer(const char *user, const char *mailbox)
	:	BAtom(),
		fSummaryLock("summary-container-lock")
{
	MDB(MailDebug md);
	fSc = sc++;
	DB(md.Print("Created summary container with id = '%ld'\n", fSc));
	memset(fEntries, 0, kEntryHashSize * sizeof(SummaryEntry *));
	SetTo(user, mailbox);
}

SummaryContainer::SummaryContainer(const SummaryContainer &inContainer)
	:	BAtom(),
		fSummaryLock("summary-container-lock")
{
	memset(fEntries, 0, kEntryHashSize * sizeof(SummaryEntry *));
	*this = inContainer;
}

SummaryContainer::~SummaryContainer()
{
	Clear();
	MDB(MailDebug md);
	DB(md.Print("Deleted summary container with id = '%ld'\n", fSc));
}

SummaryContainer &SummaryContainer::operator=(const SummaryContainer &inContainer)
{
	if (this != &inContainer) {
		memset(fEntries, 0, kEntryHashSize * sizeof(SummaryEntry *));
		fModified = inContainer.IsModified();
		fDirty = inContainer.IsDirty();
		fActualMessageCount = inContainer.ActualMessageCount();
		fTotalMessages = inContainer.TotalMessages();
		fUnseenMessages = inContainer.UnseenMessages();
		fNewMessagesSinceLastSync = inContainer.NewMessagesSinceLastSync();
		fMailboxSize = inContainer.MailboxSize();
		fMailboxQuota = inContainer.MailboxQuota();
		fMailboxUid = inContainer.MailboxUid();
		fMailboxName = inContainer.MailboxName();
		fUser = inContainer.User();
		SummaryContainer *container = const_cast<SummaryContainer *>(&inContainer);
		SummaryIterator iterator(container);
		for (iterator.First(); !iterator.IsDone(); iterator.Next())
			AddEntry(iterator.CurrentItem());
	}
	return *this;
}

void SummaryContainer::Clear()
{
	// Mailboxname and user do not get cleared here...
	fModified = false;
	fDirty = false;
	fShutdown = false;
	fActualMessageCount = 0;
	fTotalMessages = 0;
	fUnseenMessages = 0;
	fNewMessagesSinceLastSync = 0;
	fMailboxSize = 0;
	fMailboxQuota = 0;
	fMailboxUid = B_EMPTY_STRING;
	for (int32 i = 0; i < kEntryHashSize; i++) {
		for (SummaryEntry *entry = fEntries[i]; entry != 0; /* Empty loop control */) {
			SummaryEntry *temp = entry->fHashNext;
			delete entry;
			entry = temp;
		}
	}
	memset(fEntries, 0, kEntryHashSize * sizeof(SummaryEntry *));
}

status_t SummaryContainer::SetTo(const char *user, const char *mailbox)
{
	Clear();
	fUser.SetTo(user);
	fMailboxName = mailbox;
	return B_OK;
}

status_t SummaryContainer::Load()
{
	status_t result = B_OK;
	BFile file;
	if ((result = GetSummaryFile(&file, B_READ_ONLY)) != B_OK) {
		// If we couldn't find a summary file, then we flag ourselves
		// as being modified so that a subsequent save will create the file.
		fModified = (result == B_ENTRY_NOT_FOUND);
		return result;
	}
	// Preserve this, it might get inadvertantly changed in AddEntry
	bool modified = fModified;
	// Let's be CFS friendly and use a buffered IO to read from the file
	BufferedFileAdapter stream(&file, 1024 * 16, false);
	// Load in the summary's header
	stream.Read((void *)&(fDirty), sizeof(bool));						// Dirty
	stream.Read((void *)&(fTotalMessages), sizeof(int32));	 			// Total number of messages
	stream.Read((void *)&(fMailboxSize), sizeof(int32));				// Mailbox size
	stream.Read((void *)&(fMailboxQuota), sizeof(int32));				// Mailbox quota
	int32 uidSize = 0;													// Mailbox uid	
	stream.Read((void *)&(uidSize), sizeof(int32));
	if (uidSize > 0) {
		char *uidBuffer = fMailboxUid.LockBuffer(uidSize);
		stream.Read((void *)uidBuffer, uidSize);
		fMailboxUid.UnlockBuffer(uidSize);
	}
	// Now load in each mime message entry
	for (int32 i = 0; i < fTotalMessages; i++) {
		if (fShutdown)
			break;
			
		SummaryEntry *entry = new SummaryEntry();
		stream.Read((void *)&(entry->fDirty), sizeof(bool));
		entry->fMessage->LoadFromBuffer(stream);
		// Assumes ownership
		result = AddEntry(entry);
		// If that didn't work, then get out of here. (Chances are,
		// that this container was told to shutdown...)
		if (result != B_OK)
			break;
	}
	// AddEntry in the above loop will set fModified to true.
	// However, it shouldn't get set if we are just loading
	// the summary. We need to restore the value...
	fModified = modified;
	
	return result;
}

status_t SummaryContainer::Save()
{
	MDB(MailDebug md);
	status_t result = B_OK;
	// If nothing has changed or we've been shutdown, then don't bother saving.
	if (!fModified || fShutdown)
		return result;
	// If the user no longer exists, then don't bother
	// XXX: Should have a better way of handling this.
	BinderNode::property prop = BinderNode::Root()["user"][Username()];
	if (!prop->IsValid())
		return B_ERROR;
		
	BFile file;
	if ((result = GetSummaryFile(&file, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE)) != B_OK) {
		DB(md.Print("ERROR in SummaryLoad: GetSummaryFile failed with: '%s'\n", strerror(result)));
		return result;
	}
	// Allocate a 16k CFS friendly buffer
 	BBufferIO buffer(&file, BBufferIO::DEFAULT_BUF_SIZE, false);
 	// Start of cache file is in the format:
 	// (bool:dirty, int32:totalMsg,
 	// int32:mailboxSize, int32:mailboxQuota, int32:uidLength, string:uid)
 	buffer.Write((void *)&(fDirty), sizeof(bool));
 	buffer.Write((void *)&(fActualMessageCount), sizeof(int32)); // Yes, this is correct
 	buffer.Write((void *)&(fMailboxSize), sizeof(int32));
 	buffer.Write((void *)&(fMailboxQuota), sizeof(int32));
 	int32 uidLength = fMailboxUid.Length();
 	buffer.Write((void *)&uidLength, sizeof(int32));
 	buffer.Write((void *)(fMailboxUid.String()), uidLength);
 	// Output each messages envelope into the buffer	
 	SummaryIterator iterator(this);
 	for (iterator.First(); !iterator.IsDone(); iterator.Next()) {
 		SummaryEntry *entry = iterator.CurrentItem();
 		bool dirty = entry->IsDirty();
 		buffer.Write((void *)&(dirty), sizeof(bool));
 		entry->fMessage->WriteToStream(&buffer);
 	}
	// Flush the buffered output..
	buffer.Flush();
	file.Unset();
	return B_OK;
}

status_t SummaryContainer::Sync(IMAP *server)
{
	MDB(MailDebug md);
	ASSERT(server != NULL);
		
	status_t result = B_OK;
	fNewMessagesSinceLastSync = 0;
	// Sync mailbox size and quota
	SetMailboxSize(server->GetMailboxSize());
	SetMailboxQuota(server->GetMailboxQuota());
	// Sync Upstream ...
	if (fDirty) {
		if (SyncLocalFlagsUpstream(server) != B_OK)
			DB(md.Print("SyncLocalFlagsUpstream did not work\n"));
		// This summary has been modified by setting the message's dirty flag above.
		// However, we're no longer dirty with respect to what's on the server. 
		fModified = true;
		fDirty = false;
	}
	// Fetch the list of message uid's on the server
	BList serverUidList;
	if ((result = server->GetMessageUidList(&serverUidList) != B_OK)) {
		DB(md.Print("Error getting uid list from server.\n"));
		return result;
	}
	// If the mailbox uid has changed, then we have to assume that all our messages
	// are out of date. If the mailbox UID has changed, then none of our current
	// uid's are valid anymore, so all messages have to be pulled down. Clear the summary.
	// Small hack added here... If the 'shift' key is held down, then we will do a force sync.
	if ((strcmp(MailboxUid(), server->GetFolderUid()) != 0) || (modifiers() & B_SHIFT_KEY)) {
		DB(md.Print("MAILBOX UID's don't match! Us: '%s', Them: '%s'\n", MailboxUid(), server->GetFolderUid()));
		Clear();
		// We've no doubt changed, so flag this container as being modified
		fModified = true;
	}
	// Copy over the flags from the server into our loaded summary, if a message uid
	// is not in our loaded summary, then remember it so that we can download it...
	SmartArray<BString> newUidList;
	int32 uidCount = serverUidList.CountItems();
	for (int32 j = 0; j < uidCount; j++) {
		BString *uid = static_cast<BString *>(serverUidList.ItemAt(j));
		SummaryEntry *entry = FindEntry(uid->String());
		if (entry != NULL) {
			// XXX:	Need to add support here for copying the server flags locally.
#if 0
			uint32 flags = 0;
			entry->fMessage->SetFlags(flags);
#endif
		} else {
			// Entry not in summary container, need to download it..
			newUidList.AddItem(BString(uid->String()));
		}
	}
	// Have the summary container go through the list of server uid's
	// and remove any entries in the summary that are not on the server
	// This will delete the entries in the list!!!
	PruneOldEntries(serverUidList);
	// Update the header info for the summary.
	fTotalMessages = server->GetTotalMessageCount();
	fMailboxSize = server->GetMailboxSize();
	fMailboxQuota = server->GetMailboxQuota();
	fMailboxUid = server->GetFolderUid();
	// Now add each entry on the server that we don't have in the summary
	// file to the summary file.
	uidCount = newUidList.CountItems();
	for (int32 k = 0; k < uidCount; k++) {
		BString &uid = newUidList.ItemAt(k);

		MimeMessage *message = new MimeMessage;
		if (server->GetEnvelope(uid.String(), message) != B_OK)
			continue;
		if (server->GetStructure(message->GetUid(), message) != B_OK)
			continue;
		// Add entry into summary, 'false' says the message is not dirty.
		AddEntry(message, false);
		fNewMessagesSinceLastSync++;
	}
	return result;
}

status_t SummaryContainer::SyncLocalFlagsUpstream(IMAP *server)
{
	ASSERT(server != NULL);
	
	SummaryIterator iterator(this);
	for (iterator.First(); !iterator.IsDone(); iterator.Next()) {
		SummaryEntry *entry = iterator.CurrentItem();
		ASSERT(entry != NULL);
		// If we're dirty, then that means one of our flags was updated, sync these flags with the server
		if (entry->IsDirty()) {
			if (server->SetFlags(entry->fMessage->GetUid(), entry->fMessage->GetFlags()) != B_OK) {
				// Probably couldn't find message uid
			}
			entry->SetDirty(false);	
		}
	}
	return B_OK;
}

status_t SummaryContainer::SyncNewEntriesUpstream(IMAP *server)
{
	ASSERT(server != NULL);
	(void)server;
	
	SummaryIterator iterator(this);
	for (iterator.First(); !iterator.IsDone(); iterator.Next()) {
		SummaryEntry *entry = iterator.CurrentItem();
		ASSERT(entry != NULL);
		// If the UID of the message is not a digit, then it must be an invalid
		// UID, which for us means that the message needs to be uploaded to the
		// server...
		if (strtol(entry->fMessage->GetUid(), NULL, 10) == 0) {
			// Invalid UID, upload
			// XXX: Cannot easily upload something here since the IMAP
			// 		'store' command requires the size ahead of time.
		}
	}
	return B_OK;
}

status_t SummaryContainer::Expunge(IMAP *server)
{
	ASSERT(server != NULL);
	(void)server;
	
	// Placeholder function

	return B_OK;
}

status_t SummaryContainer::GetSummaryFile(BFile *file, uint32 openMode)
{
	MDB(MailDebug md);
	
	status_t result = B_OK;
	// Find the mail cache summary file in question
	StringBuffer path;
	path << kCacheBase << "/" << Username() << "/" << kMailCache << "/" << fMailboxName.String();
	BDirectory dir(path.String());
	if (dir.InitCheck() == B_ENTRY_NOT_FOUND) {
		if (openMode & B_CREATE_FILE) {
			create_directory(path.String(), 0777);
			// Try again
			result = dir.SetTo(path.String());
			if (result != B_OK) {
				DB(md.Print("GetSummaryFile failed with error: '%s'\n", strerror(result)));
				return result;
			}
		} else {
			return B_ENTRY_NOT_FOUND;
		}
	}
	// Should have a valid directory, now get the summary file.
	return file->SetTo(&dir, kSummaryFileName, openMode);
}

void SummaryContainer::Lock()
{
	fSummaryLock.Lock();
}

bool SummaryContainer::IsLocked() const
{
	return fSummaryLock.IsLocked();
}

void SummaryContainer::Unlock()
{
	fSummaryLock.Unlock();
}

status_t SummaryContainer::AddEntry(SummaryEntry *entry)
{
	MDB(MailDebug md);
	ASSERT(entry != NULL);

	fSummaryLock.Lock();

	if (fShutdown)
		return B_SUMMARY_SHUTDOWN;

	// Poor man's memory management. We're trying to prevent the
	// very likely scenario in which we over allocate, crash
	// Wagner, have Wagner restore itself, which in turn tells
	// us to reload the list, which will over allocate, crash
	// Wagner... etc... This should prevent the circular crash
	// of death.
	system_info info;
	get_system_info(&info);
	if (((info.max_pages * B_PAGE_SIZE) - (info.used_pages * B_PAGE_SIZE)) < (1024 * 512)) {
		// Less then one half megabyte of system memory remaining, bail, but return B_OK...
		return B_OK;
	}

	SummaryEntry **entryBucket = &fEntries[HashString(entry->fMessage->GetUid()) % kEntryHashSize];
	entry->fHashPrevious = entryBucket;
	entry->fHashNext = *entryBucket;
	*entryBucket = entry;
	if (entry->fHashNext != 0)
		entry->fHashNext->fHashPrevious = &entry->fHashNext;
	fModified = true;
	fActualMessageCount++;
	if (!(entry->Message()->GetFlags() & kMessageSeen))
		fUnseenMessages++;
	UpdateObservers(kEntryAddedEvent);

	fSummaryLock.Unlock();

	return B_OK;
}

status_t SummaryContainer::AddEntry(MimeMessage *message, bool dirty)
{
	MDB(MailDebug md);
	ASSERT(message != NULL);

	return AddEntry(new SummaryEntry(message, dirty));
}

status_t SummaryContainer::RemoveEntry(SummaryEntry *entry)
{
	ASSERT(entry != NULL);
	
	fSummaryLock.Lock();

	if (fShutdown)
		return B_SUMMARY_SHUTDOWN;

	*entry->fHashPrevious = entry->fHashNext;
	if (entry->fHashNext)
		entry->fHashNext->fHashPrevious = entry->fHashPrevious;

	BinderNode::property cacheReply;
	// Build the four arguments needed to define an entry in the cache
	BinderNode::property user(Username());
	BinderNode::property mailbox(MailboxName());
	BinderNode::property buid(entry->fMessage->GetUid());
	// Note that this order is important. If you change it, then you have to change MailCacheNode::ReadProperty()
	BinderNode::property_list args;
	args.AddItem(&user);
	args.AddItem(&mailbox);
	args.AddItem(&buid);
	BinderNode::property node = BinderNode::Root()["service"]["mail"]["cache"];
	BinderNode::property reply;
	node->GetProperty("RemoveMessages", reply, args);

	if (!(entry->Message()->GetFlags() & kMessageSeen))
		fUnseenMessages--;

	delete entry;
	fModified = true;
	fActualMessageCount--;
	ASSERT(fActualMessageCount > -1);

	UpdateObservers(kEntryRemovedEvent);

	fSummaryLock.Unlock();

	return B_OK;
}

status_t SummaryContainer::RemoveEntry(const char *uid)
{
	ASSERT(uid != NULL);

	status_t result = B_OK;
	
	SummaryEntry **entryBucket = &fEntries[HashString(uid) % kEntryHashSize];
	SummaryEntry *entry = 0;
	for (entry = *entryBucket; entry != 0; entry = entry->fHashNext) {
		if (strcmp(entry->fMessage->GetUid(), uid) == 0) {
			result = RemoveEntry(entry);
			if (result != B_OK)
				return result;
		}
	}
}

SummaryEntry *SummaryContainer::FindEntry(const char *uid)
{
	ASSERT(uid != NULL);

	SummaryEntry **entryBucket = &fEntries[HashString(uid) % kEntryHashSize];
	SummaryEntry *entry = 0;
	for (entry = *entryBucket; entry != 0; entry = entry->fHashNext) {
		if (strcmp(entry->fMessage->GetUid(), uid) == 0) {
			break;
		}
	}
	return entry;
}

status_t SummaryContainer::UpdateEntryFlags(const char *uid, uint32 &newFlag, bool addThisFlag)
{
	SummaryEntry *entry = FindEntry(uid);
	if (entry == NULL)
		return B_ENTRY_NOT_FOUND;
	
	uint32 messageFlags = entry->fMessage->GetFlags();
	
	if ((addThisFlag && !(messageFlags & newFlag)) || (!addThisFlag && (messageFlags & newFlag))){
		fSummaryLock.Lock();
		if (fShutdown)
			return B_SUMMARY_SHUTDOWN;
			
		fModified = true;
		fDirty = true;
		entry->SetDirty(true);
		addThisFlag ? messageFlags |= newFlag : messageFlags &= ~newFlag;
		entry->fMessage->SetFlags(messageFlags);
		
		if (newFlag == kMessageSeen)
			fUnseenMessages = (addThisFlag ? fUnseenMessages - 1 : fUnseenMessages + 1);

		UpdateObservers(kEntryModifiedEvent);
		fSummaryLock.Unlock();
	}
	return B_OK;
}

status_t SummaryContainer::InvalidateEntry(SummaryEntry *entry)
{
	fSummaryLock.Lock();
	fModified = true;
	fDirty = true;
	entry->SetDirty(true);
	UpdateObservers(kEntryModifiedEvent);
	fSummaryLock.Unlock();
	return B_OK;
}

void SummaryContainer::PruneOldEntries(const BList &list)
{
	// ** Can't use the iterator class since we could
	// potentially be modifying the list, which would
	// cause problems for the iterator...
	int32 listSize = list.CountItems();
	for (uint8 i = 0; i < kEntryHashSize; i++) {
		SummaryEntry *entry;
		for (entry = fEntries[i]; entry != 0; /* empty loop control */) {
			SummaryEntry *temp = entry->fHashNext;
			// If this entry is *not* in the list, then it's not
			// on the server, so remove it.
			bool found = false;
			for (int32 i = 0; i < listSize; i++) {
				BString *uid = static_cast<BString *>(list.ItemAt(i));
				if (uid->Compare(entry->fMessage->GetUid()) == 0) {
					found = true;
					break;
				}
			}
			// Not found, then not in list, remove it from the container...
			if (!found) {
				// Hold on though, if it's a draft, then don't delete it.
				// Since draft's are not presently uploaded to the server,
				// they will always appear in this list, for now just ignore them...
				if (strncmp(entry->fMessage->GetUid(), "DRAFT-", 6) != 0) {
					// Ok, it's not a draft. Remove the entry from our
					// list and tell the cache to do the same...
#if 0
					fCacheNode->GetProperty("RemoveEntry", ...);
#endif
					RemoveEntry(entry);
				}
			}
			// Since we have just deleted this entry, we need this temp one
			// for getting the "next" entry.
			entry = temp;
		}
	}
}

status_t SummaryContainer::GetEntryStream(const char *uid, const char *part, BFile *file, uint32 openMode)
{
	// Either of these cases would not be very good...
	ASSERT((uid != NULL) && (file != NULL));
	
	StringBuffer path;
	path << kCacheBase << "/" << Username() << "/" << kMailCache << "/";
	path << MailboxName() << "/" << uid;
	if (part != NULL)
		path << "." << part;
	
	return file->SetTo(path.String(), openMode);
}

bool SummaryContainer::IsModified() const
{
	return fModified;
}

bool SummaryContainer::IsDirty() const
{
	return fDirty;
}

bool SummaryContainer::Shutdown() const
{
	return fShutdown;
}

int32 SummaryContainer::ActualMessageCount() const
{
	return fActualMessageCount;
}

int32 SummaryContainer::TotalMessages() const
{
	return fTotalMessages;
}

int32 SummaryContainer::UnseenMessages() const
{
	return fUnseenMessages;
}

int32 SummaryContainer::NewMessagesSinceLastSync() const
{
	return fNewMessagesSinceLastSync;
}

int32 SummaryContainer::MailboxSize() const
{
	return fMailboxSize;
}

int32 SummaryContainer::MailboxQuota() const
{	
	return fMailboxQuota;
}

const char *SummaryContainer::MailboxUid() const
{
	return fMailboxUid.String();
}

const char *SummaryContainer::MailboxName() const
{
	return fMailboxName.String();
}

UserContainer &SummaryContainer::User()
{
	return fUser;
}

const UserContainer &SummaryContainer::User() const
{
	return fUser;
}

const char *SummaryContainer::Username() const
{
	return fUser.GetUserName();
}

const char *SummaryContainer::UserPassword() const
{
	return fUser.GetPassword();
}

int32 SummaryContainer::BucketCount() const
{
	return kEntryHashSize;
}

void SummaryContainer::SetModified(bool modified)
{
	fModified = modified;
}

void SummaryContainer::SetDirty(bool dirty)
{
	fDirty = dirty;
}

void SummaryContainer::SetShutdown(bool shutdown)
{
	fShutdown = shutdown;
}

void SummaryContainer::SetActualMessageCount(int32 count)
{
	fModified = fModified ? fModified : (fActualMessageCount != count);
	fActualMessageCount = count;
}

void SummaryContainer::SetTotalMessages(int32 count)
{
	fModified = fModified ? fModified : (fTotalMessages != count);
	fTotalMessages = count;
}

void SummaryContainer::SetUnseenMessages(int32 count)
{
	fModified = fModified ? fModified : (fUnseenMessages != count);
	fUnseenMessages = count;
}

void SummaryContainer::SetNewMessagesSinceLastSync(int32 count)
{
	fModified = fModified ? fModified : (fNewMessagesSinceLastSync != count);
	fNewMessagesSinceLastSync = count;
}

void SummaryContainer::SetMailboxSize(int32 size)
{
	fModified = fModified ? fModified : (fMailboxSize != size);
	fMailboxSize = size;
}

void SummaryContainer::SetMailboxQuota(int32 quota)
{
	fModified = fModified ? fModified : (fMailboxQuota != quota);
	fMailboxQuota = quota;
}

void SummaryContainer::SetMailboxUid(const char *uid)
{
	fModified = fModified ? fModified : (fMailboxUid != uid);
	fMailboxUid = uid;
}

bool SummaryContainer::AddObserver(SummaryObserver *observer)
{
	if (!fObservers.AddItem(observer))
		return false;
	
	//let our new observer know to populate itself
	observer->Update(kUnknownEvent);

	return true;
}

bool SummaryContainer::RemoveObserver(SummaryObserver *observer)
{
	return fObservers.RemoveItem(observer);
}

void SummaryContainer::UpdateObservers(uint32 event)
{
	for (int32 i = 0; i < fObservers.CountItems(); i++) {
		SummaryObserver *observer = static_cast<SummaryObserver *>(fObservers.ItemAt(i));
		if (observer != NULL)
			observer->Update(event);
	}
}

void SummaryContainer::PrintToStream(bool messages)
{
	printf("- SummaryContainer Info:\n");
	printf("\tmailbox = '%s'\n", fMailboxName.String());
	printf("\tuid = '%s'\n", fMailboxUid.String());
	printf("\tuser = '%s'\n", fUser.GetUserName());
	printf("\tactual = '%ld'\n", fActualMessageCount);
	printf("\ttotal = '%ld'\n", fTotalMessages);
	printf("\tunseen = '%ld'\n", fUnseenMessages);
	printf("\tnew from sync = '%ld'\n", fNewMessagesSinceLastSync);
	printf("\tsize = '%ld'\n", fMailboxSize);
	printf("\tquota = '%ld'\n", fMailboxQuota);
	printf("\tdirty = '%s'\n", fDirty ? "true" : "false");
	printf("\tmodified = '%s'\n", fModified ? "true" : "false");
	if (messages) {
		for (uint8 i = 0; i < kEntryHashSize; i++) {
			int32 offset = 0;
			for (SummaryEntry *entry = fEntries[i]; entry != 0; entry = entry->fHashNext) {
				printf("Bucket: %d, Offset: %ld, Uid: %s, Dirty: %s\n",
					i, offset, entry->fMessage->GetUid(), entry->fDirty ? "true" : "false");
//				entry->fMessage->Print();
				offset++;
			}
		}
	}
	printf("--------------------------\n");
}

int32 SummaryContainer::FindClosestPrime(int32 value)
{
	(void) value;
	return 97;
}


// -----------------------------------------------------------------------
//							SummaryEntry
// #pragma mark -
// -----------------------------------------------------------------------
SummaryEntry::SummaryEntry()
	:	fDirty(false),
		fMessage(NULL)
{
	fMessage = new MimeMessage;
	fMessage->AcquireReference();
}


SummaryEntry::SummaryEntry(MimeMessage *message, bool dirty)
	:	fDirty(dirty),
		fMessage(message)
{
	fMessage->AcquireReference();
}

SummaryEntry::SummaryEntry(const SummaryEntry &inEntry)
{
	*this = inEntry;
}

SummaryEntry::~SummaryEntry()
{
	fMessage->ReleaseReference();
}

SummaryEntry &SummaryEntry::operator=(const SummaryEntry &inEntry)
{
	if (this != &inEntry) {
		fDirty = inEntry.IsDirty();
		fMessage = const_cast<MimeMessage *>(inEntry.Message());
		fMessage->AcquireReference();
	}
	return *this;
}

void SummaryEntry::SetDirty(bool dirty)
{
	fDirty = dirty;
}

bool SummaryEntry::IsDirty() const
{
	return fDirty;
}

const MimeMessage *SummaryEntry::Message() const
{
	return fMessage;
}

MimeMessage *SummaryEntry::Message()
{
	return fMessage;
}


// -----------------------------------------------------------------------
//							SummaryEntry
// #pragma mark -
// -----------------------------------------------------------------------
SummaryIterator::SummaryIterator(SummaryContainer *summary)
	:	fSummary(summary),
		fEntry(NULL),
		fCurrentBucket(0),
		fLastBucket(0)
{
	fLastBucket = fSummary->BucketCount();
}

SummaryIterator::~SummaryIterator()
{

}

void SummaryIterator::First()
{
	// Find the first valid entry...
	for (fCurrentBucket = 0; fCurrentBucket < fLastBucket; fCurrentBucket++) {
		fEntry = fSummary->fEntries[fCurrentBucket];
		if (fEntry != NULL)
			break;
	}
}

void SummaryIterator::Next()
{
	ASSERT(fEntry != NULL);
	
	// Skip to next one...
	fEntry = fEntry->fHashNext;
	// If it's null, then we're at the end of the chain, fetch a new bucket...
	if (fEntry == 0) {
		for (fCurrentBucket += 1; fCurrentBucket < fLastBucket; fCurrentBucket++) {
			fEntry = fSummary->fEntries[fCurrentBucket];
			if (fEntry != NULL)
				break;
		}
	}
}

bool SummaryIterator::IsDone() const
{
	return (fCurrentBucket >= fLastBucket);
}

SummaryEntry *SummaryIterator::CurrentItem() const
{
	return fEntry;
}

// -----------------------------------------------------------------------
//							SummaryObserver
// #pragma mark -
// -----------------------------------------------------------------------
SummaryObserver::SummaryObserver()
	:	fDirty(true)
{
	// Empty body
}


SummaryObserver::~SummaryObserver()
{
	// Empty body
}

// End of SummaryContainer.cpp
