/*
	CacheContainer.h
*/
#ifndef CACHE_CONTAINER_H
#define CACHE_CONTAINER_H
#include <Directory.h>
#include <String.h>
#include <Queue.h>
#include <xml/BXMLBinderNode.h>

using namespace B::XML;

// This should really be a more dynamic number..
const uint32 kCacheHashSize = 97;
// Default max size of one meg per container.
const off_t kDefaultMaxContainerSize = 1024 * 1024;

class CacheEntry;
class CacheIterator;

class CacheContainer {
	public:
								CacheContainer();
								CacheContainer(const char *username);
								CacheContainer(const CacheContainer &inContainer);
								~CacheContainer();
		
		CacheContainer &		operator=(const CacheContainer &inContainer);
	
		status_t				SetTo(const char *username);
		void					Clear();

		status_t				Load();
		status_t				Save();
		
		status_t				AddEntry(const char *mailbox, const char *uid, const char *part, off_t size);
		status_t				AddEntry(CacheEntry *entry, bool checkForSpace = false);
		CacheEntry *			FindEntry(const char *mailbox, const char *uid, const char *part, off_t size = -1);
		status_t				ContainsEntry(const char *mailbox, const char *uid, const char *part, off_t size);
		status_t				ResizeEntry(const char *mailbox, const char *uid, const char *part, off_t size);
		status_t				TouchEntry(const char *mailbox, const char *uid, const char *part);
		status_t				RemoveEntry(const char *mailbox, const char *uid, const char *part, bool force = false);
		status_t				RemoveEntry(CacheEntry *entry, bool force = false);
		status_t				RemoveMessages(const char *mailbox, const char *uid);
		status_t				MakeRoom(off_t entrySize);
		
		
		const char *			Username() const;
		bool					IsDirty() const;
		void					SetDirty(bool dirty);
		off_t					Size() const;
		void					SetMaximumSize(off_t size);
		void					PrintToStream();
		
	private:
		BString fUsername;
		bool fDirty;
		int32 fEntryCount;
		off_t fContainerSize;
		off_t fMaxContainerSize;
		CacheEntry *fEntries[kCacheHashSize];
		Queue fLRU;
};

class CacheEntry : public QueueEntry {
	public:
								CacheEntry(const char *mailbox, const char *uid, const char *part, off_t size);
								CacheEntry(BStringMap &attributes);
								~CacheEntry();
		
		void					Touch();
		bool					CanDelete() const;
		off_t					Size() const;
		void					SetSize(off_t size);
		const char *			Mailbox() const;
		const char *			Uid() const;
		const char *			Part() const;
		time_t					LastAccess() const;
		const char *			HashKey() const;
	
		void					PrintToStream();
		
	private:
								// Can't create an empty cach entry, don't define
								CacheEntry();
								// No copying of CacheEntries, don't define them either.
								CacheEntry(const CacheEntry &inEntry);
		CacheEntry &			operator=(const CacheEntry &inEntry);

		bool fCanDelete;
		BString fMailbox;
		BString fUid;
		BString fPart;
		off_t fSize;
		BString fHashKey;
		time_t fLastAccess;

		CacheEntry *fHashNext;
		CacheEntry **fHashPrevious;
		
		friend class CacheContainer;
};

class CacheContainerParseContext : public BXMLParseContext
{
	public:
								CacheContainerParseContext(CacheContainer *container);

		virtual status_t		OnStartTag(BString &name, BStringMap &attributes);

	private:
		CacheContainer *fContainer;
		BDirectory fDirectory;
};
#endif

// End of CacheContainer.h
