/*
	SummaryContainer.h
*/
#ifndef SUMMARY_CONTAINER_H
#define SUMMARY_CONTAINER_H
#include <Atom.h>
#include <Locker.h>
#include "MimeMessage.h"
#include "UserContainer.h"

const uint8 kEntryHashSize = 97;
#define B_SUMMARY_SHUTDOWN 		B_ERRORS_END + 1

// Forward declarations...
class MailServer;
class SummaryEntry;
class SummaryIterator;
class SummaryObserver;
class BFile;

class SummaryContainer : public BAtom {
	public:
								SummaryContainer();
								SummaryContainer(const char *user, const char *mailbox);
								SummaryContainer(const SummaryContainer &inContainer);
								~SummaryContainer();

		SummaryContainer &		operator=(const SummaryContainer &inContainer);

		void					Clear();
		status_t				SetTo(const char *user, const char *mailbox);
		status_t				Load();
		status_t				Save();
		status_t				Sync(MailServer *server);
		status_t				SyncLocalFlagsUpstream(MailServer *server);
		status_t				SyncNewEntriesUpstream(MailServer *server);
		status_t				Expunge(MailServer *server);
		status_t				GetSummaryFile(BFile *file, uint32 openMode);
		
		void					Lock();
		bool					IsLocked() const;
		void					Unlock();
				
		status_t				AddEntry(SummaryEntry *entry);
		status_t				AddEntry(MimeMessage *message, bool dirty);
		status_t				RemoveEntry(SummaryEntry *entry);
		status_t				RemoveEntry(const char *uid);
		SummaryEntry *			FindEntry(const char *uid);
		status_t				UpdateEntryFlags(const char *uid, uint32 &flags, bool addThisFlag);
		status_t				InvalidateEntry(SummaryEntry *entry);
		void					PruneOldEntries(const BList &list);
		status_t				GetEntryStream(const char *uid, const char *part, BFile *file, uint32 openMode);
		
								// Access calls
		bool					IsModified() const;
		bool					IsDirty() const;
		bool					Shutdown() const;
		int32					ActualMessageCount() const;
		int32					TotalMessages() const;
		int32					UnseenMessages() const;
		int32					NewMessagesSinceLastSync() const;
		int32					MailboxSize() const;
		int32					MailboxQuota() const;
		const char *			MailboxUid() const;
		const char *			MailboxName() const;
		UserContainer &			User();
		const UserContainer &	User() const;
		const char *			Username() const;
		const char *			UserPassword() const;
		int32					BucketCount() const;
		
								// Set calls
		void					SetModified(bool modified);
		void					SetDirty(bool dirty);
		void					SetShutdown(bool shutdown = true);
		void					SetActualMessageCount(int32 count);
		void					SetTotalMessages(int32 count);
		void					SetUnseenMessages(int32 count);
		void					SetNewMessagesSinceLastSync(int32 count);
		void					SetMailboxSize(int32 size);
		void					SetMailboxQuota(int32 quota);
		void					SetMailboxUid(const char *uid);
	
		bool					AddObserver(SummaryObserver *observer);
		bool					RemoveObserver(SummaryObserver *observer);
		void					UpdateObservers(uint32 event);

		// Observer events that you can watch for
		enum {
			kEntryAddedEvent = 0x00000001,
			kEntryRemovedEvent = 0x00000002,
			kEntryModifiedEvent = 0x00000004,
			kUnknownEvent = 0x00000008 | kEntryAddedEvent | kEntryRemovedEvent | kEntryModifiedEvent
		};
		
		void					PrintToStream(bool messages = false);

	private:
		int32					FindClosestPrime(int32 value);
		
		volatile bool fModified;
		volatile bool fDirty;
		volatile bool fShutdown;
		int32 fActualMessageCount;
		int32 fTotalMessages;
		int32 fUnseenMessages;
		int32 fNewMessagesSinceLastSync;
		int32 fMailboxSize;
		int32 fMailboxQuota;
		BString fMailboxUid;
		BString fMailboxName;
		SummaryEntry *fEntries[kEntryHashSize];
		UserContainer fUser;
		BList fObservers;
		BLocker fSummaryLock;
		
		int32 fSc;
		friend class SummaryIterator;
};

class SummaryEntry {
	public:
								SummaryEntry();
								SummaryEntry(MimeMessage *message, bool dirty);
								SummaryEntry(const SummaryEntry &inEntry);
								~SummaryEntry();
		
		SummaryEntry &			operator=(const SummaryEntry &inEntry);
		
		void					SetDirty(bool dirty);
		bool					IsDirty() const;
		const MimeMessage *		Message() const;
		MimeMessage *			Message();
		
	private:
		bool fDirty;
		MimeMessage *fMessage;
		SummaryEntry *fHashNext;
		SummaryEntry **fHashPrevious;
		
		friend class SummaryContainer;
		friend class SummaryIterator;
};

class SummaryIterator {
	public:
								SummaryIterator(SummaryContainer *summary);
								~SummaryIterator();
								
			void				First();
			void				Next();
			bool				IsDone() const;
			SummaryEntry *		CurrentItem() const;

	private:
		SummaryContainer *fSummary;
		SummaryEntry *fEntry;
		int32 fCurrentBucket;
		int32 fLastBucket;
};

class SummaryObserver {
	public:
								SummaryObserver();
		virtual					~SummaryObserver();
	
		virtual status_t		Update(uint32 event) = 0;

		bool fDirty;
};

#endif
