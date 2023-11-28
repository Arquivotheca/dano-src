/*
	CacheContainer.cpp
*/
#include <BufferIO.h>
#include "CacheContainer.h"
#include <Entry.h>
#include <File.h>
#include <List.h>
#include <String.h>
#include "www/util.h"

static const char * const kUserBase = "/boot/binder/user";
// Tag names
static const char * const kSummaryTag = "user-mailbox-container";
static const char * const kEntryTag = "cache-entry";

CacheContainer::CacheContainer()
{
	Clear();
}

CacheContainer::CacheContainer(const char *username)
{
	SetTo(username);	
}

CacheContainer::CacheContainer(const CacheContainer &inContainer)
{
	*this = inContainer;
}


CacheContainer::~CacheContainer()
{
	Clear();
}

CacheContainer &CacheContainer::operator=(const CacheContainer &inContainer)
{
	if (this != &inContainer) {
		fUsername = inContainer.Username();
		fDirty = inContainer.IsDirty();
		// XXX:	Finish implementation.
	}
	return *this;
}

status_t CacheContainer::SetTo(const char *username)
{
	Clear();
	fUsername = username;
	return Load();
}

void CacheContainer::Clear()
{
	Save();
	
	fUsername = B_EMPTY_STRING;
	fDirty = false;
	fEntryCount = 0;
	fContainerSize = 0;
	fMaxContainerSize = kDefaultMaxContainerSize;
	for(QueueEntry *entry; (entry = fLRU.Dequeue()) != NULL; )
		delete entry;
	
	memset(fEntries, 0, kCacheHashSize * sizeof(CacheEntry *));
}

status_t CacheContainer::Load()
{
	status_t result = B_OK;
	
	// If the user no longer exists, then don't bother
	// XXX: Should have a better way of handling this.
	BinderNode::property prop = BinderNode::Root()["user"][Username()];
	if (!prop->IsValid())
		return B_ERROR;

	BString path(kUserBase);
	path << "/" << Username() << "/mail_cache/" << "mailcache.xml";

	BFile summaryFile(path.String(), B_READ_ONLY | B_CREATE_FILE);
	if ((result = summaryFile.InitCheck()) != B_OK)
		return result;

	BBufferIO buffer(&summaryFile, BBufferIO::DEFAULT_BUF_SIZE, false);
	BXMLDataIOSource stream(&buffer);
	CacheContainerParseContext context(this);
	if ((result = ParseXML(&context, &stream)) != B_OK)
		SERIAL_PRINT(("\tParseXML was not B_OK: %s\n", strerror(result)));
	return result;
}

status_t CacheContainer::Save()
{
	status_t result = B_OK;
	if (!fDirty)
		return result;
	// If the user no longer exists, then don't bother
	// XXX: Should have a better way of handling this.
	BinderNode::property prop = BinderNode::Root()["user"][Username()];
	if (!prop->IsValid())
		return B_ERROR;

	BString path(kUserBase);
	path << "/" << Username() << "/mail_cache/" << "mailcache.xml";

	BFile summaryFile(path.String(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	if ((result = summaryFile.InitCheck()) != B_OK)
		return result;
	
	BBufferIO buffer(&summaryFile, BBufferIO::DEFAULT_BUF_SIZE, false);
	BOutputStream stream(buffer, true);
	BStringMap map;

	map.Add("containerSize", fContainerSize);
	map.Add("currentCacheCount", fEntryCount);

	stream.StartTag(kSummaryTag, map, stfCanAddWS);
	CacheEntry *entry = dynamic_cast<CacheEntry *>(fLRU.Head());
	while (entry != NULL) {
		BStringMap entryMap;
		entryMap.Add("uid", entry->Uid());
		entryMap.Add("part", entry->Part());
		entryMap.Add("mailbox", entry->Mailbox());
		entryMap.Add("hashKey", entry->HashKey());
		entryMap.Add("lastAccess", entry->LastAccess());
		entryMap.Add("size", entry->Size());
		stream.StartTag(kEntryTag, entryMap, stfCanAddWS);
		stream.EndTag(kEntryTag);
		
		entry = dynamic_cast<CacheEntry *>(fLRU.GetNext(entry));
	}

	stream.EndTag(kSummaryTag);
	buffer.Flush();
	summaryFile.Unset();
	fDirty = false;
	return result;
}

status_t CacheContainer::AddEntry(const char *mailbox, const char *uid, const char *part, off_t size)
{
	// printf("Adding entry: mailbox = '%s', uid = '%s', part = '%s', size = '%Ld'\n", mailbox, uid, part, size);
	status_t result = B_OK;
	
	// If this entry is already in the list, then just touch it...
	CacheEntry *entry = FindEntry(mailbox, uid, part, size);
	if (entry != NULL) {
		printf("Found entry, just going to touch it\n");
		entry->Touch();
		return B_OK;
	}
	// Try and make room for this entry, if we can't make room, then we can't add it...
	// printf("Asking for more room\n");
	result = MakeRoom(size);
	if (result != B_OK) {
		printf("Could not make enough room, bailing from AddEntry\n");
		return result;
	}

	// printf("Enough room in the cache, adding entry\n");
	entry = new CacheEntry(mailbox, uid, part, size);
	return AddEntry(entry);
}

status_t CacheContainer::AddEntry(CacheEntry *entry, bool checkForSpace)
{
	ASSERT(entry != NULL);
	status_t result = B_OK;
	if (checkForSpace) {
		result = MakeRoom(entry->Size());
		if (result != B_OK)
			return result;
	}
	BString hashKey = entry->Mailbox();
	hashKey << "_" << entry->Uid() << "_" << entry->Part();
	CacheEntry **entryBucket = &fEntries[HashString(hashKey.String()) % kCacheHashSize];
	entry->fHashPrevious = entryBucket;
	entry->fHashNext = *entryBucket;
	*entryBucket = entry;
	if (entry->fHashNext != 0)
		entry->fHashNext->fHashPrevious = &entry->fHashNext;
	fLRU.Enqueue(entry);
	fDirty = true;
	fEntryCount++;
	return result;
}

CacheEntry *CacheContainer::FindEntry(const char *mailbox, const char *uid, const char *part, off_t size)
{
	BString hashKey = mailbox;
	hashKey << "_" << uid << "_" << part;

	CacheEntry **entryBucket = &fEntries[HashString(hashKey.String()) % kCacheHashSize];
	CacheEntry *entry = 0;
	bool found = false;
	for (entry = *entryBucket; entry != 0; entry = entry->fHashNext) {
		if ((strcmp(entry->Mailbox(), mailbox) == 0) &&
			(strcmp(entry->Uid(), uid) == 0) &&
			(strcmp(entry->Part(), part) == 0)) {
			if (size > 0) {
				// Check against size as well.
				if (entry->Size() == size) {
					found = true;
					break;
				}
			} else {
				// Otherwise don't check against size.
				found = true;
				break;
			}
		}
	}
	return (found ? entry : NULL);
}

status_t CacheContainer::ContainsEntry(const char *mailbox, const char *uid, const char *part, off_t size)
{
	// printf("ContainsEntry called with mailbox = '%s' and uid = '%s' and part = '%s'\n", mailbox, uid, part);
	CacheEntry *entry = FindEntry(mailbox, uid, part, size);
	return (entry != NULL ? B_OK : B_ENTRY_NOT_FOUND);
}

status_t CacheContainer::ResizeEntry(const char *mailbox, const char *uid, const char *part, off_t size)
{
	CacheEntry *entry = FindEntry(mailbox, uid, part, size);
	if (entry == NULL)
		return B_ENTRY_NOT_FOUND;
	
	if (entry->Size() > size)
		return B_OK;
	
	// Need to make room
	status_t result = MakeRoom(size - entry->Size());
	if (result == B_NO_MEMORY)
		return result;
	// Update size
	entry->SetSize(size);
	return B_OK;
}

status_t CacheContainer::TouchEntry(const char *mailbox, const char *uid, const char *part)
{
	// printf("Touching entry for mailbox = '%s', uid = '%s', part = '%s'\n", mailbox, uid, part);
	CacheEntry *entry = FindEntry(mailbox, uid, part);
	if (entry == NULL) {
		// printf("Could not find entry to touch, bailing\n");
		return B_ENTRY_NOT_FOUND;
	}
	// Update entries access time.
	entry->Touch();
	// Move entry to top of LRU
	fLRU.RemoveEntry(entry);
	fLRU.Enqueue(entry);
	fDirty = true;
	
	return B_OK;	
}

status_t CacheContainer::RemoveEntry(const char *mailbox, const char *uid, const char *part, bool force)
{
	// printf("Removing entry mailbox = '%s', uid = '%s', part = '%s', force = '%s'\n", mailbox, uid, part, force ? "true" : "false");
	CacheEntry *entry = FindEntry(mailbox, uid, part);
	if (entry == NULL) {
		// printf("Could not find entry to remove, bailing from RemoveEntry\n");
		return B_ENTRY_NOT_FOUND;
	}

	return RemoveEntry(entry, force);
}

status_t CacheContainer::RemoveEntry(CacheEntry *entry, bool force)
{
	// printf("RemoveEntry with a *entry\n");
	ASSERT(entry != NULL);
	// Check whether or not we should be deleting this entry...
	// We only for delete when we are explicitly told to delete a Draft.
	// Otherwise, Drafts cannot be deleted to make room for other entries.
	if (!force && !entry->CanDelete()) {
		// printf("Not forcing the remove and entry cannot deleted\n");
		return B_ERROR;
	}
		
	*entry->fHashPrevious = entry->fHashNext;
	if (entry->fHashNext)
		entry->fHashNext->fHashPrevious = entry->fHashPrevious;
	
	fLRU.RemoveEntry(entry);
	fDirty = true;
	fEntryCount--;
	fContainerSize -= entry->Size();
	// Remove the actual cache file...
	// ** Note, I'm not using a StringBuffer here because then I have to link against libwww.so ** 
	BString path;
	path << kUserBase << "/" << Username() << "/mail_cache/" << entry->Mailbox() << "/" << entry->Uid() << "." << entry->Part();
	BEntry file(path.String());
	file.Remove();
	// And finally, delete the entry itself.
	delete entry;
	
	return B_OK;
}

status_t CacheContainer::RemoveMessages(const char *mailbox, const char *uid)
{
	ASSERT((mailbox != NULL) && (uid != NULL));
	// Remove all entries in the cache that are related to this uid...
	status_t result = B_OK;
	for (uint8 i = 0; i < kCacheHashSize; i++) {
		for (CacheEntry *entry = fEntries[i]; entry != 0; entry = entry->fHashNext) {
			if ((strcmp(entry->Mailbox(), mailbox) == 0) && (strcmp(entry->Uid(), uid) == 0))
				RemoveEntry(entry, true);
		}
	}
	return result;
}

status_t CacheContainer::MakeRoom(off_t entrySize)
{
	// printf("Make room for entrySize = '%Ld' when the cache size is = '%Ld'\n", entrySize, fContainerSize);

	if (fMaxContainerSize - fContainerSize > entrySize) {
		// printf("fMaxContainerSize - fContainerSize was = '%Ld', which is larger then the entry size\n", fMaxContainerSize - fContainerSize);
		fContainerSize += entrySize;
		return B_OK;
	}
	
	// Instead of just blindly deleting entries, we'll instead
	// keep track of which entries we need to delete. In the event that
	// after emptying the cache, we still don't have enough room, then
	// we might was well just restore the old entries, since deleting
	// them doesn't really get us anywhere.
	off_t tempSize = fContainerSize;
	BList entryList;
	CacheEntry *entry = dynamic_cast<CacheEntry *>(fLRU.Head());
	while ((entry != NULL) && (fMaxContainerSize - tempSize < entrySize)) {
		// printf("fMaxContainerSize - fContainerSize = '%Ld'\n", fMaxContainerSize - tempSize);
		tempSize -= entry->Size();
		entryList.AddItem(entry);
		entry = dynamic_cast<CacheEntry *>(fLRU.GetNext(entry));
	}
	// If entry is NULL, then that means that we didn't find enough room, so
	// don't bother doing anything...
	if ((entry != NULL) && (fMaxContainerSize - tempSize > entrySize)) {
		// Then we were able to make space, so now we can actually remove the entries...
		int32 count = entryList.CountItems() - 1;
		for (int32 i = count; i >= 0; i--) {
			entry = static_cast<CacheEntry *>(entryList.RemoveItem(i));
			ASSERT(entry != NULL);
			fLRU.RemoveEntry(entry);
			RemoveEntry(entry);
		}
	}
	return (fMaxContainerSize - fContainerSize > entrySize ? B_OK : B_NO_MEMORY);
}

const char *CacheContainer::Username() const
{
	return fUsername.String();
}

bool CacheContainer::IsDirty() const
{
	return fDirty;
}

void CacheContainer::SetDirty(bool dirty)
{
	fDirty = dirty;
}

off_t CacheContainer::Size() const
{
	return fContainerSize;
}

void CacheContainer::SetMaximumSize(off_t size)
{
	// XXX: Not yet implemented
#if 0
	Clear();
	fMaxContainerSize = size;
#endif
}

void CacheContainer::PrintToStream()
{
	printf("- CacheContainer for user '%s'\n", Username());
	printf("\tDirty: %s\n", fDirty ? "true" : "false");
	printf("\tEntryCount: %ld\n", fEntryCount);
	printf("\tContainerSize: %Ld\n", fContainerSize);
	printf("\tMaxContainerSize: %Ld\n", fMaxContainerSize);
	int32 count = 1;
	CacheEntry *entry = dynamic_cast<CacheEntry *>(fLRU.Head());
	while (entry != NULL) {
		printf("\t(%ld) - ", count++);
		entry->PrintToStream();
		entry = dynamic_cast<CacheEntry *>(fLRU.GetNext(entry));	
	}
	printf("- End CacheContainer\n");
}

// ---------------------------------------------------------------------------
//									CacheEntry
// #pragma mark -
// ---------------------------------------------------------------------------
CacheEntry::CacheEntry(const char *mailbox, const char *uid, const char *part, off_t size)
	:	QueueEntry(),
		fCanDelete(true),
		fMailbox(mailbox),
		fUid(uid),
		fPart(part),
		fSize(size),
		fHashKey(B_EMPTY_STRING),
		fLastAccess(0),
		fHashNext(NULL),
		fHashPrevious(NULL)
{
	// It's a bit more efficient to keep track of this, since it get's
	// called rather frequently. We can't hash on just the filename since
	// it's possible that two file's in different mailboxes might have
	// the same name. Therefore, the key used for hashing is the mailbox
	// with the filename appended.
	fHashKey = fMailbox;
	fHashKey << "_" << uid << "." << part;
	// Files that in the "Draft" mailbox cannot be pruned from the cache to
	// make room for a new entry since these entries are only on the device,
	// and not on the server.
	fCanDelete = (fMailbox.ICompare("Drafts") != 0);
	
	Touch();
}

CacheEntry::CacheEntry(BStringMap &attributes)
	:	QueueEntry(),
		fCanDelete(true),
		fMailbox(B_EMPTY_STRING),
		fUid(B_EMPTY_STRING),
		fPart(B_EMPTY_STRING),
		fSize(0),
		fHashKey(B_EMPTY_STRING),
		fLastAccess(0),
		fHashNext(NULL),
		fHashPrevious(NULL)
{
	BString value;
	attributes.Find("uid", fUid);
	attributes.Find("part", fPart);
	attributes.Find("mailbox", fMailbox);
	attributes.Find("hashKey", fHashKey);
	attributes.Find("lastAccess", value);
	fLastAccess = atol(value.String());
	attributes.Find("size", value);
	fSize = atol(value.String());	
	fCanDelete = (fMailbox.ICompare("Drafts") != 0);
}

CacheEntry::~CacheEntry()
{
	// Empty body.
}

void CacheEntry::Touch()
{
	fLastAccess = real_time_clock();
}

bool CacheEntry::CanDelete() const
{
	return fCanDelete;
}

off_t CacheEntry::Size() const
{
	return fSize;
}

void CacheEntry::SetSize(off_t size)
{
	fSize = size;
}

const char *CacheEntry::Mailbox() const
{
	return fMailbox.String();
}

const char *CacheEntry::Uid() const
{
	return fUid.String();
}

const char *CacheEntry::Part() const
{
	return fPart.String();
}

time_t CacheEntry::LastAccess() const
{
	return fLastAccess;
}

const char *CacheEntry::HashKey() const
{
	return fHashKey.String();
}

void CacheEntry::PrintToStream()
{
	printf("Mailbox: '%s', Uid: '%s', Part: '%s', Size: '%Ld', LastAccess: '%ld', CanDelete: '%s'\n",
		Mailbox(), Uid(), Part(), Size(), LastAccess(),
		CanDelete() ? "true" : "false");
}
// ---------------------------------------------------------------------------
//									CacheContainerParseContext
// #pragma mark -
// ---------------------------------------------------------------------------
CacheContainerParseContext::CacheContainerParseContext(CacheContainer *container)
	:	BXMLParseContext(),
		fContainer(container)
{
	// empty constructor
	BString path(kUserBase);
	path << "/" << container->Username() << "/mail_cache/";
	fDirectory.SetTo(path.String());
}

status_t CacheContainerParseContext::OnStartTag(BString &name, BStringMap &attributes)
{
	if (name == kEntryTag) {
		// Verify that this entry exists
		BString path;
		BString value;
		BString filename;
		attributes.Find("mailbox", value);
		path << value << "/";
		attributes.Find("uid", filename);
		attributes.Find("part", value);
		filename << "." << value;
		path << filename;
		if (fDirectory.Contains(path.String())) 
			fContainer->AddEntry(new CacheEntry(attributes), true);
		else
			fContainer->SetDirty(true);
	}

	return BXMLParseContext::OnStartTag(name, attributes);
}

// End of CacheContainer.cpp
