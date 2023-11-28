/*
	PostOffice.h
*/

#ifndef POST_OFFICE_H
#define POST_OFFICE_H
#include <Binder.h>
#include <CursorManager.h>
#include <Directory.h>
#include "IMAP.h"
#include "MailBinderSupport.h"
#include <Resource.h>
#include <SmartArray.h>
#include "SummaryContainer.h"
#include "UserContainer.h"

namespace Wagner {

const uint32 kSyncAllMsg = 'sall';
const uint32 kSyncUserMsg = 'susr';
const uint32 kExpungeMsg = 'expg';

// Forward Declarations
class SendMessageContainer;
class PartContainer;

class PostOffice : public BinderNode {
	public:
		static PostOffice *		MailMan();
		static void				GoPostal();
		virtual 				~PostOffice();
		
								// BinderNode virtuals
		virtual get_status_t	ReadProperty(const char *name, property &outProperty, const property_list &inArgs = empty_arg_list);
		
								// GHandler virtuals	
		virtual	status_t		HandleMessage(BMessage *message);
		virtual	void			Cleanup();
		
								// PostOffice calls
		status_t				SyncCache(const char *username, const char *mailbox, SummaryContainer *outSummary = NULL, IMAP *inServer = NULL);
		status_t				DownloadUnreadMessagesToCache(SummaryContainer *summary);
		status_t				ExpungeCache(const char *username);
		status_t				SendMessage(SendMessageContainer *container);
		
		status_t				SelectMailbox(const char *username, const char *mailbox, SummaryObserver *observer = NULL);
		BDataIO *				FetchSection(PartContainer &container);
		
		MimeMessage *			FindMessage(const char *msgUid);
		status_t				SetFlags(const char *msgUid, uint32 flags, bool addThisFlag);
		status_t				SaveDraft(const property_list &inArgs, const char *inUid, const char *bodyText, BString &encodedRunArray);

		status_t				OpenServerConnection(IMAP *imap, UserContainer &container, const char *mailbox = NULL, bool writable = false);
		status_t				UpdateMailboxNode(const char *username, SummaryContainer *summary, bool includeNewMessagesSinceLastSync = false);

		status_t				AddStatus(uint32 status);
		status_t				RemoveStatus(uint32 status);
		
		const char *			LoggedInUsername() const;
		SummaryContainer *		LoadedSummaryContainer();
		
		static void				HttpHeaderCallback(void *cookie, const char *tag, const char *value);
		status_t				ParseRfc822Header(const char *url, atom<BinderContainer> &node);

	private:

								// You can only have one mail cache...
								PostOffice();

		void					VerifyMailStatusNode(const char *name);
		static status_t			UserNodeCallback(PostOffice *postOffice, uint32 observed, void *data);

		bool					IsNetworkUp() const;
		inline bool				CanSync();
		inline void				EnableCheckMailButton(bool enable);
		inline void				ReplaceMacros(BString &inBuf, const BStringMap &map);
		
		
		// Member variables.
		time_t fSyncInterval;
		volatile bool fSyncing;
		volatile bool fKeepRemindingMe;
		volatile bool fNetworkUp;
		uint32 fStatus;
		BString fBodyRedirectUrl;
		BString fAttachmentRedirectUrl;
		SummaryContainer *fCurrentSummary;

		BCursorManager::cursor_token fCursorToken;
		BCursorManager::queue_token	fCursorQueueToken;			

		BinderNode::property fUserRootNode;
		atom<BinderContainer> fStatusContainer;
		
		BLocker fCacheLock;
		BLocker fSyncingLock;
		BLocker fStatusLock;
		
		static PostOffice * fPostOfficeInstance;
		static BLocker fFactoryLocker;
};

}
#endif
// End of PostOffice.h
