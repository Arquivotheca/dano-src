/*
	PostOffice.cpp
*/
#include <Autolock.h>
#include <BufferIO.h>
#include <File.h>
#include <GLooper.h>
#include <Path.h>
#include <StreamIO.h>
#include <VolumeRoster.h>
#include "BufferedFileAdapter.h"
#include "MailCacheProxy.h"
#include "MailDebug.h"
#include "MailStatusCodes.h"
#include "PartContainer.h"
#include "PostOffice.h"
#include "Rfc822MessageAdapter.h"
#include "SendMailProxy.h"
#include "SendMessageContainer.h"
#include "StreamToFileAdapter.h"
#include "StringBuffer.h"
#include "Protocol.h"
#include "util.h"

using namespace Wagner;

static const char * const kCacheBase = "/boot/binder/user";
static const char * const kInbox = "inbox";
static const char * const kDrafts = "Drafts";
static const char * const kSentItems = "Sent Items";
// Static's...
PostOffice *PostOffice::fPostOfficeInstance = NULL;
BLocker PostOffice::fFactoryLocker;

PostOffice *PostOffice::MailMan()
{
	fFactoryLocker.Lock();
	if (fPostOfficeInstance == NULL)
		fPostOfficeInstance = new PostOffice();

	fFactoryLocker.Unlock();
	return fPostOfficeInstance;
}

void PostOffice::GoPostal()
{
	fFactoryLocker.Lock();
	// Unstack ourselves...
	fPostOfficeInstance->Unstack();
	fFactoryLocker.Unlock();
}
		
PostOffice::PostOffice()
	:	BinderNode(),
		fSyncInterval(600),
		fSyncing(false),
		fKeepRemindingMe(true),
		fNetworkUp(true),
		fStatus(0),
		fBodyRedirectUrl(B_EMPTY_STRING),
		fAttachmentRedirectUrl(B_EMPTY_STRING),
		fCurrentSummary(NULL),
		fCursorToken(0),
		fCursorQueueToken(0),
		fCacheLock("mail-cache-lock"),
		fSyncingLock("mail-sync-lock"),
		fStatusLock("mail-status-lock")
{
	MDB(MailDebug md);
	// Create local references to these nodes since we use them so often
	fUserRootNode = BinderNode::Root()["user"];
	// Setup a callback for when users are deleted or created.
	fUserRootNode->AddObserverCallback(this, (BinderNode::observer_callback)UserNodeCallback, B_NAMES_CHANGED, NULL);
	// Create a new mail binder status node for each user...
	BinderNode::iterator i = fUserRootNode->Properties();
	BString user;
	while ((user = i.Next()) != "") {
		if ((user != "~") && (user != "system")) {
			// Initialize each mailbox node to reflect number of unseen messages...
			SummaryContainer container;
			// Inbox
			container.SetTo(user.String(), kInbox);
			container.Load();
			UpdateMailboxNode(user.String(), &container);
			// Drafts
			container.SetTo(user.String(), kDrafts);
			container.Load();
			UpdateMailboxNode(user.String(), &container);
			// Sent Items
			container.SetTo(user.String(), kSentItems);
			container.Load();
			UpdateMailboxNode(user.String(), &container);
		}
	}
	BinderNode::property mailNode = BinderNode::Root()["service"]["mail"];
	// Grab the url redirect for a messages main body
	BinderNode::property redirect = mailNode["redirect"]["mainBodyUrl"];
	if (redirect.IsString())
		fBodyRedirectUrl = redirect.String();
	// Grab the url redirect for message attachments
	redirect = mailNode["redirect"]["attachmentUrl"];
	if (redirect.IsString())
		fAttachmentRedirectUrl = redirect.String();
	// Set our BinderContainer to the status mount point
	fStatusContainer = new BinderContainer;
	mailNode["status"] = static_cast<BinderNode *>(fStatusContainer);
	// Stack ourselves onto the service mail node.
	StackOnto(mailNode.Object());
	// Initialize our 'busy' cursor
	BCursorManager::cursor_data cursorData;
	cursorData.name = "busy_mail";
	cursorData.hotspotX = 1;
	cursorData.hotspotY = 6;
	cursorManager.GetCursorToken(&cursorData, &fCursorToken);
	// This doesn't leak, GHandlers are different...
	// Note that using the built-in functionality of GHandler is much
	// better then using either a) a BMessageRunner or b) a polling
	// thread. Use the tools!
	PostDelayedMessage(new BMessage(kSyncAllMsg), fSyncInterval * 1000000);
}

PostOffice::~PostOffice()
{
	// Empty destructor... See GoPostal()
	printf("POSTOFFICE DESTRUCTOR\n");
	// Lock the cache (effictively locking the loaded summary)
	BAutolock _lock(fCacheLock);
	// Unstack all the user nodes...
	BinderNode::property rootUser = BinderNode::Root()["user"];
	BinderNode::iterator i = rootUser->Properties();
	BString user;
	while ((user = i.Next()) != "") {
		if ((user != "~") && (user != "system"))
			rootUser[user.String()]["email"]["account"]["status"] = BinderNode::property::undefined;
	}
	// Remove the status node
	BinderNode::Root()["service"]["mail"]["status"] = BinderNode::property::undefined;
	// Save the loaded summary to disk.
	if (fCurrentSummary != NULL) {
		fCurrentSummary->Save();
//		fCurrentSummary->Release();
	}
}

get_status_t PostOffice::ReadProperty(const char *name, property &outProperty, const property_list &inArgs)
{
	MDB(MailDebug md);
	
	get_status_t result = ENOENT;
	outProperty.Undefine();
	
	if (strcmp("SyncAll", name) == 0) {
		PostMessage(new BMessage(kSyncAllMsg));
		outProperty = "B_OK";
		result = B_OK;

	} else if (strcmp("SyncUser", name) == 0) {
		if (inArgs.Count() > 0) {
			BMessage *message = new BMessage(kSyncUserMsg);
			message->AddString("user", inArgs[0].String().String());
			// Check if mailbox parameter was passed in
			if (inArgs.Count() > 1)
				message->AddString("mailbox", inArgs[1].String().String());
			// PostMessage to ourselves to handle
			PostMessage(message);
			result = B_OK;
		} else {
			outProperty = "ERR: Missing args.";
			result = B_ERROR;
		}

	} else if (strcmp("Stop", name) == 0) {
		fKeepRemindingMe = false;
		// Remove any pending requests..
		BMessage *message = NULL;
		while ((message = DequeueMessage(kSyncAllMsg)) != NULL)
			delete message;
		outProperty = "B_OK";
		result = B_OK;

	} else if (strcmp("Start", name) == 0) {
		if (!fKeepRemindingMe) {
			fKeepRemindingMe = true;
			PostMessage(new BMessage(kSyncAllMsg));
		}
		outProperty = "B_OK";
		result = B_OK;

	} else if (strcmp("Expunge", name) == 0) {
		if ((inArgs.Count() == 1) && (inArgs[0].IsString())) {
			BMessage *message = new BMessage(kExpungeMsg);
			message->AddString("user", inArgs[0].String().String());
			PostMessage(message);
			result = B_OK;
		} else {
			outProperty = "ERR: Missing args.";
			result = B_ERROR;
		}
	}
	return result;
}

status_t PostOffice::HandleMessage(BMessage *message)
{
	MDB(MailDebug md);

	status_t result = B_OK;

	switch (message->what) {
		case kSyncAllMsg: {
			DB(md.Print("kSyncAllMsg\n"));
			if (CanSync()) {
				ResumeScheduling();
				MailStatusProxy proxy(kStatusSyncOperation);

				// Iterate over each account and sync it
				BinderNode::iterator i = fUserRootNode->Properties();
				BString user;
				while ((user = i.Next()) != "") {
					if ((user != "~") && (user != "system")) {
						SyncCache(user.String(), kInbox);
						SyncCache(user.String(), kSentItems);
					}
				}
				fSyncing = false;
				if (fKeepRemindingMe)
					PostDelayedMessage(new BMessage(kSyncAllMsg), fSyncInterval * 1000000);
				
			}
			break;
		}
		case kSyncUserMsg: {
			if (CanSync()) {
				GLooper::UnregisterThis();
				const char *user = NULL;
				const char *mailbox = NULL;
				message->FindString("user", &user);
				if (message->FindString("mailbox", &mailbox) == B_OK) {
					SyncCache(user, mailbox);
				} else {
					// Create an IMAP connection that we will reuse
					IMAP server;
					UserContainer container(user);
					OpenServerConnection(&server, container);
					// Sync each mailbox
					SyncCache(user, kInbox, NULL, &server);
					SyncCache(user, kSentItems, NULL, &server);
					// Close the imap connection
					server.Close();
				}
					
				fSyncing = false;
			}
			break;
		}
		case kExpungeMsg: {
			const char *user = NULL;
			message->FindString("user", &user);
			ExpungeCache(user);
			break;
		}
		default: {
			result = BinderNode::HandleMessage(message);
			break;
		}
	}
	return result;
}

void PostOffice::Cleanup()
{
	BinderNode::Cleanup();
}

status_t PostOffice::SyncCache(const char *username, const char *mailbox, SummaryContainer *outSummary, IMAP *inServer)
{
	ASSERT(username != NULL && mailbox != NULL);
	MDB(MailDebug md);

	// Initialize status
	MailStatusProxy proxy(kStatusSyncingMailbox);
	// Check for valid user container...
	UserContainer container(username);
	if (!container.IsValid()) {
		// Not a valid container? Gonna be hard to sync the accounts...
		DB(md.Print("UserContainer is not valid\n"));
		container.PrintToStream();
		return B_ERROR;
	}
	// No network? Nothing to do...
	if (!IsNetworkUp()) {
		// It's ok, we just don't do anything.
		DB(md.Print("Network is not running, did not sync cache.\n"));
		return B_OK;
	}
	// 	Ensure that this user has a mail status node...
	VerifyMailStatusNode(container.GetUserName());
			
	status_t result = B_OK;
	SummaryContainer *summary = NULL;
	bool deleteSummary = false;
	// Get the summary for this sync...
	if (outSummary != NULL) {
		// If we were passed in a summary, then use it...
		summary = outSummary;
	} else {
		// Otherwise, we can use either the loaded summary, if that's what
		// we're syncing, or we have to create a temporary summary container.	
		if ((fCurrentSummary != NULL) && (strcmp(fCurrentSummary->Username(), username) == 0) && (strcmp(fCurrentSummary->MailboxName(), mailbox) == 0)) {
			fCacheLock.Lock();
			summary = fCurrentSummary;
		} else {
			deleteSummary = true;
			summary = new SummaryContainer(username, mailbox);
			summary->Load();
		}
	}
	IMAP *server;
	bool deleteServer = false;
	if (inServer != NULL) {
		// Reuse a server connection if one was given to us..
		server = inServer;
		// Ensure that the current state is closed then select the mailbox
		// (We assume that the server is properly authenticated)
		if (strcmp(server->GetCurrentFolder(), summary->MailboxName()) != 0)
			server->Close();
		server->SelectMailbox(summary->MailboxName(), true);
	} else {
		// Open a new connection to the server...
		server = new IMAP();
		deleteServer = true;
		if ((result = OpenServerConnection(server, container, summary->MailboxName(), true)) != B_OK) {
			deleteSummary ? delete summary : fCacheLock.Unlock();
				
			if (deleteServer)
				delete server;

			return result;
		}
	}
	// Sync this summary...
	if ((result = summary->Sync(server)) != B_OK) {
		deleteSummary ? delete summary : fCacheLock.Unlock();
			
		if (deleteServer)
			delete server;

		return result;
	}
	// Save summary changes.
	summary->Save();
	// Download all 'unread' messages into the mail cache
	DownloadUnreadMessagesToCache(summary);
	// Update the mailbox node
	UpdateMailboxNode(username, summary, true);
	// If needed, delete this summary container
	deleteSummary ? delete summary : fCacheLock.Unlock();
	// If needed, delete the server connection
	if (deleteServer)
		delete server;
	return result;
}

status_t PostOffice::DownloadUnreadMessagesToCache(SummaryContainer *summary)
{
	MDB(MailDebug md);
	
	// Initialize the status
	MailStatusProxy proxy(kStatusDownloadingUnread);

	status_t result = B_OK;
	
	SummaryIterator iterator(summary);
	for (iterator.First(); !iterator.IsDone(); iterator.Next()) {
		SummaryEntry *entry = iterator.CurrentItem();
		ASSERT(entry != NULL);
		uint32 flags = entry->Message()->GetFlags();
		if (!(flags & kMessageSeen)) {
			MessagePart *part = entry->Message()->GetMainDoc("text/html");
			if (part != NULL) {
				PartContainer container(summary->MailboxName(), entry->Message()->GetUid(), part->id.String());
				container.SetSize(part->size);
				container.SetUser(summary->Username());
				container.SetFlagRead(false);
				
				BDataIO *stream = FetchSection(container);
				if (stream != NULL) {
					// Just read from this so that it get's pulled
					// through the mail cache adapter...
					char buffer[4096];
					while (stream->Read(buffer, 4096) > 0) {
						// Empty body
					}
					delete stream;
				}
			} 
		}
	}

	return result;
}

status_t PostOffice::ExpungeCache(const char *username)
{
	ASSERT(username != NULL);
	MDB(MailDebug md);

	// Initialize the status
	MailStatusProxy proxy(kStatusExpungingMailbox);
	
	BAutolock _lock(fCacheLock);

	// Make sure we have a valid user container
	UserContainer container(username);
	if (!container.IsValid())
		return B_ERROR;
	// Create an imap connection, we can use the same one for each mailbox.
	status_t result = B_OK;
	// Iterate through the user's three mailboxes...
	const char *mailboxes[] = { kInbox, kDrafts, kSentItems };
	for (int32 i = 0; i < 3; i++) {
		// Should we open a network connection here? This is moved down here so that
		// we can allow users to delete drafts without requiring a network connection
		// (Which wouldn't make sense since drafts are not stored on the server.)
		if ((!IsNetworkUp()) && (i != 1))
			continue;

		SummaryContainer *summary = NULL;
		bool deleteSummary = false;
		
		if (strcmp(mailboxes[i], fCurrentSummary->MailboxName()) == 0) {
			// Use loaded mailbox
			fCacheLock.Lock();
			summary = fCurrentSummary;
		} else {
			summary = new SummaryContainer(username, mailboxes[i]);
			summary->Load();
			deleteSummary = true;
		}
		// HACK: Special case the 'Drafts' folder the time being... Eeewww...
		if (strcmp(summary->MailboxName(), kDrafts) != 0) {
			// I'm not really happy about doing this. Ideally we should be re-using
			// this server connection for each container. However, I couldn't get
			// that to work properly and I'm running out of time... I'd kinda like
			// to move this into SummaryContainer::Expunge(), but then SummaryContainer
			// would have to know how to open a server connection, which I don't
			// really like either. Maybe I can improve this later...
			IMAP server;
			if ((result = OpenServerConnection(&server, container, mailboxes[i], true)) != B_OK) {
				deleteSummary ? delete summary : fCacheLock.Unlock();
				return result;
			}
			// Expunging the cache is kinda tricky. Since the local
			// summary file could be out of date with the server,
			// we first have to sync the local message flags up
			// to the server...
			if (summary->IsDirty()) {
				if ((result = summary->SyncLocalFlagsUpstream(&server)) != B_OK) {
					deleteSummary ? delete summary : fCacheLock.Unlock();
					return result;
				}
				summary->SetModified(true);
				summary->SetDirty(false);
			}
			// Expunge this mailbox by closing it...
			if ((result = server.Close()) != B_OK) {
				deleteSummary ? delete summary : fCacheLock.Unlock();
				return result;
			}
			// Resync the container...
			if ((result = SyncCache(username, mailboxes[i], summary)) != B_OK) {
				deleteSummary ? delete summary : fCacheLock.Unlock();
				return result;
			}
		} else {
			// Sync the Drafts mailbox slightly differently, since in BeIA 1.0 it does not
			// copy the drafts up to the IMAP server, all we have to do is explicitly remove
			// any messages that have the deleted flag set...
			SummaryIterator iterator(summary);
			for (iterator.First(); !iterator.IsDone(); iterator.Next()) {
				SummaryEntry *entry = iterator.CurrentItem();
				ASSERT(entry != NULL);
				uint32 flags = entry->Message()->GetFlags();
				if (flags & kMessageDeleted)
					summary->RemoveEntry(entry);
			}
			UpdateMailboxNode(summary->Username(), summary);
		}
		// Save changes
		summary->Save();
		// Delete if needed
		deleteSummary ? delete summary : fCacheLock.Unlock();
	}	
	return result;
}

status_t PostOffice::SendMessage(SendMessageContainer *container)
{
	status_t result = SendMailProxy::Newman().SendMessage(container);
	return result;
}

status_t PostOffice::SelectMailbox(const char *username, const char *mailbox, SummaryObserver *observer)
{
	MDB(MailDebug md);

	// No user or mailbox? Then no point in continuing...
	if ((username == NULL) || (mailbox == NULL)) {
		TRESPASS();
		return B_ERROR;
	}
	// Init the status
	MailStatusProxy mailStatus;
	// Get the cache lock so that fCurrentSummary doesn't change underneath up
	BAutolock _lock(fCacheLock);
	// Check if this user/mailbox is already selected
	if (fCurrentSummary != NULL) {
		if ((strcmp(fCurrentSummary->Username(), username) == 0) && (strcmp(fCurrentSummary->MailboxName(), mailbox) == 0)) {
			// Add any observers here though...
			if (observer != NULL)
				fCurrentSummary->AddObserver(observer);
			return B_OK;
		}
		// Save the currently loaded summary back to disk
		mailStatus.SetStatus(kStatusSavingSummaryFile);
		fCurrentSummary->Lock();
		fCurrentSummary->Save();
		// Shutdown this summary and release our reference on it
		fCurrentSummary->Shutdown();
		fCurrentSummary->Unlock();
		fCurrentSummary->Release();
	}
	// Create a new summary for this user and mailbox
	fCurrentSummary = new SummaryContainer(username, mailbox);
	fCurrentSummary->Acquire();
	// Make sure this user has a valid status node...
	VerifyMailStatusNode(username);
	// Add possible SummaryObserver to this SummaryContainer
	if (observer != NULL)
		fCurrentSummary->AddObserver(observer);
	// Load the mailbox into this summary
	mailStatus.SetStatus(kStatusLoadingSummaryFile);
	status_t result = fCurrentSummary->Load();
	// If the summary file did not exist, then we need to sync with the server to create it
	if ((result == B_ENTRY_NOT_FOUND) && (strcmp(mailbox, kDrafts) != 0)) {
		BMessage *message = new BMessage(kSyncUserMsg);
		message->AddString("user", username);
		message->AddString("mailbox", mailbox);
		PostMessage(message);
	}
	// That's it folks...
	return result;
}

BDataIO *PostOffice::FetchSection(PartContainer &container)
{
	MDB(MailDebug md);
	DB(md.Print("#0: FetchSection Started for uid = '%s' and part = '%s'.\n", container.Uid(), container.Part()));
	ASSERT(container.IsValid());
	
	// We need this lock to protect fCurrentSummary
	BAutolock _lock(fCacheLock);
	// Check to make sure that we have a valid fCurrentSummary. When wouldn't we?
	// When restoring from state directly to a displayed message, it's possible
	// the mailbox has yet to be loaded...
	if (fCurrentSummary == NULL)
		SelectMailbox(container.User(), container.Mailbox());
	// This better not be NULL
	ASSERT(fCurrentSummary != NULL);
	
	status_t result = B_OK;
	MailCacheProxy cacheProxy;
	// We get called from MerlinProtocol and it takes over ownership of the DataIO that we return.
	BDataIO *source = NULL;
	// Check if the message is in the cache...
	bool inCache = cacheProxy.ContainsEntry(container);

	if (inCache) {
		DB(md.Print("#1: Entry is in the cache.\n"));
		// Means that the cache contains this part, and that we can read it on disk...
		BFile *cacheFile = new BFile();
		result = fCurrentSummary->GetEntryStream(container.Uid(), container.Part(), cacheFile, B_READ_ONLY);
		if (result == B_OK) {
			DB(md.Print("#2: Got entry stream for entry from loaded summary.\n"));
			// Set the outgoing source to this BFile
			source = cacheFile;
			// Touch this entry so that it moves to the front of the cache queue...
			cacheProxy.TouchEntry(container);
		} else {
			DB(md.Print("#3: ERROR: Did not get expected entry stream from loaded summary.\n"));
			// Ok, this is a problem. For some reason the mail cache
			// thinks this entry exists, but the summary container could
			// not find it. In this case, we'll delete the BFile and
			// hopefully pick up the stream lower down...
			delete cacheFile;
			// Tell the mail cache to remove this entry, something is wrong with it.
			cacheProxy.RemoveEntry(container);
		}
	} 
	// If the source is NULL here, then the entry was not in the cache, so we'll have to fetch it.
	if (source == NULL) {
		DB(md.Print("#4: Source is NULL.\n"));
		// Should we open a network connection here?
		if (IsNetworkUp()) {
			DB(md.Print("#5: Network is up.\n"));
			BDataIO *contentStream = NULL;
			BString redirectString = (container.IsMainPart() ? fBodyRedirectUrl : fAttachmentRedirectUrl);
			
			if ((redirectString.Length() > 0) && (!container.BypassRedirectServer())) {
				DB(md.Print("#6: Redirect length is larger then zero.\n"));
				BStringMap map;
				map.Add("USERNAME", fCurrentSummary->User().GetLogin());
				map.Add("PASSWORD", fCurrentSummary->User().GetPassword());
				map.Add("MAILBOX", fCurrentSummary->MailboxName());
				map.Add("MESSAGEID", container.Uid());
				map.Add("BODYPARTID", container.Part());
				map.Add("SERVER", fCurrentSummary->User().GetServer());
				map.Add("PORT", fCurrentSummary->User().GetPort());
				map.Add("MARKREAD", container.FlagRead() ? "true" : "false");
				ReplaceMacros(redirectString, map);

				URL serverUrl;
				serverUrl.SetTo(redirectString.String());
				DB(md.Print("** UX: Using redirect URL of '%s'\n", redirectString.String()));
				
				// If a non-null URL was passed in for the redirectURL, then that implies
				// that whoever called us, which is most likely the MerlinProtocol, is
				// going to handle the redirect on their end. If the URL is null, then
				// we are still going to redirect this request through the conversion
				// server, only the PostOffice will instantiate the protocol, and not
				// the calling function. We also have to check whether or not we want
				// to cache this part. If we do, then we cannot do the redirect, since
				// that would bypass the cache. 
				if ((container.RedirectUrlPtr() != NULL) && (!container.Cache())) {
					DB(md.Print("#7: The redirect pointer was not null, and we're not caching.\n"));
					container.SetRedirectUrl(serverUrl);
					// We'll return NULL, since the caller is going to instantiate
					// the protcol itself. We don't have to do anything with the
					// cache proxy since we're not cacheing this part.
					if (container.CachePolicyPtr() != NULL) {
						// As far as we're concerned, don't cache this part.
						// The redirect URL has the power to override this.
						container.SetCachePolicy(CC_NO_CACHE);
					}	
					return NULL;
				} else {
					DB(md.Print("#8: Instantiating protocol.\n"));
					Protocol *instance = Protocol::InstantiateProtocol(serverUrl.GetScheme());
					if (!instance) {
						DB(md.Print("#9: ERROR: Could not instantiate protocol.\n"));
						return NULL;
					}
					// Create a callback so that we can parse the HTTP header, which might
					// contain error information that we're interested in.
					instance->SetMetaCallback((void *)&container, HttpHeaderCallback);

					// We'll open the protocol here, so that when we return it's all ready to
					// be read from. 	
					DB(md.Print("#10: Opening URL.\n"));
					BMessage errorMessage;
					if(instance->Open(serverUrl, URL(), &errorMessage, 0) < 0) {
						DB(md.Print("#11: Could not open URL.\n"));
						delete instance;
						return NULL;
					}

					// If the caller is interested in knowing the content type, then update it. 
					if (container.ContentTypePtr() != NULL) {
						DB(md.Print("#12: Content type pointer was not null, setting content type.\n"));
						char ctype[256];
						instance->GetContentType(ctype, 256);
						container.SetContentType(ctype);
					}
					contentStream = instance;
				}
			} else {
				DB(md.Print("#13: Redirect length is smaller then zero, not redirecting through server.\n"));
				// The appropriate redirect parameters are not set, so we'll just connect directly to
				// the IMAP server and fetch the part outselves.
				IMAP *imap = new IMAP();
				DB(md.Print("#14: Opening IMAP server connection.\n"));
				if ((result = OpenServerConnection(imap, fCurrentSummary->User(), fCurrentSummary->MailboxName(), false)) != B_OK) {
					DB(md.Print("#15: ERROR: Could not open IMAP server connection.\n"));
					delete imap;
					return NULL;
				}
				DB(md.Print("#16: Fetching section from IMAP server.\n"));
				if ((result = imap->FetchSection(container.Uid(), container.Part())) != B_OK) {
					DB(md.Print("#17: ERROR: Could not fetch section from IMAP server.\n"));
					delete imap;
					return NULL;
				}
				contentStream = imap;
			}
			DB(md.Print("#18: Starting to add entry to cache.\n"));
			// Try and register this entry with the mail cache if we're trying to cache it...
			bool entryAdded = false;
			
			if (container.Cache()) {
				DB(md.Print("#19: Entry should be cached.\n"));
				entryAdded = cacheProxy.AddEntry(container);
			}

			if (entryAdded) {
				DB(md.Print("#20: Entry was added to cache.\n"));
				BFile file;
				result = fCurrentSummary->GetEntryStream(container.Uid(), container.Part(), &file, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
				if (result == B_OK) {
					DB(md.Print("#21: GetEntryStream returned B_OK.\n"));
					// Create a new adapter that will write this part to disk as it streams in...
					// This adapter will assume ownsership of 'contentStream'.
					source = new StreamToFileAdapter(contentStream, file, true);
				} else {
					DB(md.Print("#22: ERROR: GetEntryStream did not return B_OK: %s\n", strerror(result)));
					// If for some reason we could not get a file to stream to, then just return the raw stream and
					// remove this entry from the cache.
					source = contentStream;
					cacheProxy.RemoveEntry(container);
				}
			} else {
				DB(md.Print("#23: Entry was not added to cache.\n"));
				// Either not caching or no room to cache, just return the streamm
				source = contentStream;
			}
		} else {
			DB(md.Print("#24: Network is not up.\n"));
			// We'll do a top-level redirect here so that this page appears to
			// have been loaded with a "file://" URL, instead of a "merlin://"
			// URL. This is needed so that we can included some custom_content
			// in the offline.html page. 			
			URL offlineRedirect("file://$RESOURCES/Mail/mail_display_offline.html");
			container.SetRedirectUrl(offlineRedirect);
			// Technically these following four lines are bogus, but we'll be
			// thorough and enter the appropriate values anyways.
			ASSERT(container.ContentTypePtr() != NULL);
			container.SetContentType("text/html");
			// Make sure that Wagner does not cache this page!
			ASSERT(container.CachePolicyPtr() != NULL);
			container.SetCachePolicy(CC_NO_CACHE);
			// Return NULL so that MerlinProtocol does a top-level redirect.
			return NULL;
		}
	}	
	DB(md.Print("#25: Checking whether to flag message read.\n"));
	// Mark the message as being read
	if (container.FlagRead()) {
		DB(md.Print("#26: Flagging message as read.\n"));
		if (SetFlags(container.Uid(), kMessageSeen, true) != B_OK) {
			DB(md.Print("#27: ERROR: Setting flags on message did not succeed.\n"));
		}
	}
	DB(md.Print("#28: FetchSection EXIT.\n"));
	return source;
}

MimeMessage *PostOffice::FindMessage(const char *msguid)
{
	// MerlinProtocol calls this function.
	SummaryEntry *entry = fCurrentSummary->FindEntry(msguid);
	if (entry != NULL)
		return entry->Message();

	return NULL;
}

status_t PostOffice::SetFlags(const char *msgUid, uint32 flag, bool addThisFlag)
{
	MDB(MailDebug md);

	BAutolock _lock(fCacheLock);
	status_t result = fCurrentSummary->UpdateEntryFlags(msgUid, flag, addThisFlag);
	if (result == B_OK)
		UpdateMailboxNode(fCurrentSummary->Username(), fCurrentSummary);
	return result;
}

status_t PostOffice::SaveDraft(const property_list &inArgs, const char *inUid, const char *bodyText, BString &encodedRunArray)
{
	ASSERT(inUid != NULL && bodyText != NULL);

	MDB(MailDebug md);
	DB(md.Print("Save draft for uid '%s'\n", inUid));

	status_t result = B_OK;
	bool deleteSummary = false;
	bool newEntry = false;
	BString uid(inUid);
	BFile file;
	
	BString dirPath(kCacheBase);
	dirPath << "/" << fCurrentSummary->Username() << "/mail_cache/" << kDrafts;
	create_directory(dirPath.String(), 0777);
	
	SummaryContainer *summary = fCurrentSummary;
	// If the 'Draft' summary is not the currently loaded summary, then we need to temporarily load it ...
	if ((summary == NULL) || (strcmp(summary->MailboxName(), kDrafts) != 0)) {
		summary = new SummaryContainer(summary->Username(), kDrafts);
		summary->Load();
		deleteSummary = true;
	}
	ASSERT(summary != NULL);
	// Create a PartContainer to define this part.
	PartContainer container(summary->MailboxName(), uid.String(), "0");
	container.SetUser(summary->Username());
	// Use a MailCacheProxy to communicate with the mail cache binder node
	MailCacheProxy cacheProxy;
	// Find this entry
	bool inCache = cacheProxy.ContainsEntry(container);

	if (inCache) {
		// Cache thinks this entry exists, ask the summary for it...
		result = summary->GetEntryStream(uid.String(), "0", &file, B_READ_WRITE | B_ERASE_FILE);
		if (result == B_ENTRY_NOT_FOUND) {
			// Hmm, this is a problem, the entry is in the cache but does not have
			// a corresponding file on disk, remove it from the cache and create
			// an entry on disk below.
			cacheProxy.RemoveEntry(container);
		} else {
			// Ask the cache to resize this entry, it may have changed since the last save...
			result = cacheProxy.ResizeEntry(container);
			if (result != B_OK) {
				// Could't do it, not enough memory left to save this draft
				return result;
			}
		}
	} else {
		uid << real_time_clock();
		container.SetUid(uid.String());
		// Try and make room for this entry...
		container.SetSize((int)(strlen(bodyText)));
		if (!cacheProxy.AddEntry(container))
			return B_NO_MEMORY;
		// Create an entry in the summary
		result = summary->GetEntryStream(uid.String(), "0", &file, B_READ_WRITE | B_CREATE_FILE);
		if (result != B_OK)
			return result;
		newEntry = true;
	}
	
	if (file.InitCheck() == B_OK) {
		// Dump body text to file.
		if (bodyText != NULL)
			file.Write(bodyText, strlen(bodyText));
		// Dump the attributes
		file.WriteAttr("beia_mail:subject", B_STRING_TYPE, 0, inArgs[0].String().String(), inArgs[0].String().Length() + 1);
		file.WriteAttr("beia_mail:recipients", B_STRING_TYPE, 0, inArgs[1].String().String(), inArgs[1].String().Length() + 1);
		file.WriteAttr("beia_mail:encodedRunArray", B_STRING_TYPE, 0, encodedRunArray.String(), encodedRunArray.Length() + 1);
		
		int32 size = strlen(bodyText);
		time_t time = real_time_clock();
		struct tm *lcltm = localtime(&time);
		char date[512];
		strftime(date,512,"%a, %d %b %Y %H:%M:%S",lcltm);
		sprintf(date, "%s %0+5d", date, (lcltm->tm_gmtoff / 3600) * 100);
		
		file.WriteAttr("beia_mail:date", B_STRING_TYPE, 0, date, strlen(date) + 1);
		file.WriteAttr("beia_mail:size", B_INT32_TYPE, 0, &size, sizeof(int32));
	
		if (newEntry) {
			file.WriteAttr("beia_mail:draft-uid", B_STRING_TYPE, 0, uid.String(), uid.Length() + 1);
			// Need to create a mime message so that we can add this to the summary...
			MimeMessage *message = new MimeMessage;
			message->SetSubject(inArgs[0].String().String());
			message->SetRecipient(inArgs[1].String().String());
			message->SetUid(uid.String());
			message->SetDate(date);
			message->SetSize(size);
			message->SetFlags(kMessageSeen | kMessageDraft);
			summary->AddEntry(message, false);
		} else {
			SummaryEntry *sentry = summary->FindEntry(uid.String());
			ASSERT(sentry != NULL);
			sentry->Message()->SetSubject(inArgs[0].String().String());
			sentry->Message()->SetRecipient(inArgs[1].String().String());
			sentry->Message()->SetSize(size);
			summary->InvalidateEntry(sentry);
		}
			
	}
	// Save the summary if needed.
	summary->Save();
	// Update the mailbox node
	UpdateMailboxNode(summary->Username(), summary);
	// Delete any temporary summary file
	if (deleteSummary)
		delete summary;

	return result;
}

bool PostOffice::IsNetworkUp() const
{
	MDB(MailDebug md);
	BinderNode::property prop = BinderNode::Root()["service"]["network"]["control"]["status"]["route"]["interface"];
	// Return true if it's undefined, probably means that the net_node is not running
	if ((prop.IsUndefined()) || (prop.String() == ""))
		return true;

	prop = BinderNode::Root()["service"]["network"]["control"]["status"]["interfaces"][prop.String().String()]["status"];
	if (!prop.IsUndefined() && prop.String() == "up")
		return true;

	DB(md.Print("Hey, your network is down, I can't do anything\n"));
	return false;
}

status_t PostOffice::OpenServerConnection(IMAP *imap, UserContainer &container, const char *mailbox, bool writable)
{
	MDB(MailDebug md);
	MailStatusProxy proxy;
	EnableCheckMailButton(false);
#if 0
	BAutolock _lock(fCacheLock);
#endif
	status_t result = B_OK;
	
	proxy.SetStatus(kStatusConnectingToServer);
	if ((result = imap->Connect(container.GetServer(), container.GetPort())) != B_OK) {
		DB(md.Print("Connection to server '%s' at port '%d' failed.\n", container.GetServer(), container.GetPort()));
		EnableCheckMailButton(true);
		return result;
	}
	proxy.SetStatus(kStatusAuthenticating);
	// XXX: Temp hack till I add proper observer call back stuff
	if (!container.IsValid())
		container.SetTo(container.GetUserName()); // Force reload, user info might have changed.
	if ((result = imap->Login(container.GetLogin(), container.GetPassword())) != B_OK) {
		DB(md.Print("Login of user '%s' with password '%s' failed.\n", container.GetLogin(), container.GetPassword()));
		EnableCheckMailButton(true);
		return result;
	}

	// If a mailbox was passed in, try and select it...
	if (mailbox != NULL) {
		proxy.SetStatus(kStatusSelectingMailbox);
		if ((result = imap->SelectMailbox(mailbox, writable)) != B_OK) {
			DB(md.Print("Selecting folder '%s' failed.\n", mailbox));
			EnableCheckMailButton(true);
			return result;
		}
	}
	EnableCheckMailButton(true);
	return result;
}

status_t PostOffice::UpdateMailboxNode(const char *username, SummaryContainer *summary, bool includeNewMessagesSinceLastSync)
{
	MDB(MailDebug md);
	// Make sure this user has a valid status node
	VerifyMailStatusNode(username);
	// Note that these arguments are added in a certain order. If you change
	// their order, then you also need to change MailboxNodeContainer::ReadProperty()
	BinderNode::property_list args;
	BinderNode::property total((int)(summary->ActualMessageCount()));
	args.AddItem(&total);
	BinderNode::property unseen((int)(summary->UnseenMessages()));
	args.AddItem(&unseen);
	BinderNode::property size((int)(summary->MailboxSize()));
	args.AddItem(&size);
	BinderNode::property quota((int)(summary->MailboxQuota()));
	args.AddItem(&quota);
	BinderNode::property lastSync((int)real_time_clock());
	args.AddItem(&lastSync);
	// You might not want to have this field updated since it will trigger
	// an observation callback in the Javascript code. (Even if the property
	// has not changed.) We'll only include this when we are called from
	// PostOffice::SyncCache(), since we definitely want to have the observation
	// callback triggered for that operation. Kenny
	BinderNode::property sinceSync((int)(summary->NewMessagesSinceLastSync()));
	if (includeNewMessagesSinceLastSync)
		args.AddItem(&sinceSync);

	BinderNode::property out;
	BinderNode::property node = fUserRootNode[username]["email"]["account"]["status"][summary->MailboxName()];
	return node->GetProperty("SetProperties", out, args);
}

status_t PostOffice::AddStatus(uint32 status)
{
	BAutolock _lock(fStatusLock);
	status_t result = B_OK;
	// Start a cursor if our status is zero
	if ((fStatus == 0) && (fCursorQueueToken == 0))
		cursorManager.SetCursor(fCursorToken, 2, &fCursorQueueToken);
	// If we don't already have this status, then add it
	if (!(fStatus & status)) {
		fStatus |= status;
		BinderNode::property prop("start");
		result = fStatusContainer->AddProperty(StatusCodeToString(status), prop);
	}
	return result;
}

status_t PostOffice::RemoveStatus(uint32 status)
{
	BAutolock _lock(fStatusLock);
	status_t result = B_OK;
	// If this item is in our status, then remove it
	if (fStatus & status) {
		fStatus &= ~status;
		result = fStatusContainer->RemoveProperty(StatusCodeToString(status));
	}
	// Reset the cursor.
	if ((fStatus == 0) && (fCursorQueueToken != 0)) {
		cursorManager.RemoveCursor(fCursorQueueToken);
		fCursorQueueToken = 0;
	}
	return result;
}

SummaryContainer *PostOffice::LoadedSummaryContainer()
{
	return fCurrentSummary;
}

void PostOffice::HttpHeaderCallback(void *castToPartContainer, const char *tag, const char *value)
{
	MDB(MailDebug md);
	PartContainer *container = static_cast<PartContainer *>(castToPartContainer);
	DB(md.Print("Callback called with name = '%s' and value = '%s'\n", tag, value));
	
	if (strcasecmp(tag, "cache-control") == 0 && container->CachePolicyPtr() != NULL) {
		if (strcasecmp(value, "no-cache") == 0 || strcasecmp(value, "private") == 0)
			container->SetCachePolicy(CC_NO_CACHE);
		else if (strcasecmp(value, "no-store") == 0)
			container->SetCachePolicy(CC_NO_STORE);
	}
}

status_t PostOffice::ParseRfc822Header(const char *url, atom<BinderContainer> &node)
{
	Rfc822MessageAdapter::ParseHeader(url, node);
	// Ensure that the following exist
	if (!node->HasProperty("from"))
		node->AddProperty("from", B_EMPTY_STRING);
	if (!node->HasProperty("subject"))
		node->AddProperty("subject", B_EMPTY_STRING);
	if (!node->HasProperty("date"))
		node->AddProperty("date", B_EMPTY_STRING);
	// Add these special headers...
	node->AddProperty("uid", "*be:eml");
	node->AddProperty("mailbox", "*be:eml");
	// Now provide an escaped version of the name for display purposes
	BinderNode::property prop;
	node->ReadProperty("from", prop);
	BString escaped;
	escape_for_html(&escaped, prop.String().String());
	node->AddProperty("from_escaped", escaped.String());

	return B_OK;
}

void PostOffice::VerifyMailStatusNode(const char *username)
{
	MDB(MailDebug md);
	if (fUserRootNode[username]["email"]["account"]["status"] == BinderNode::property::undefined)
		fUserRootNode[username]["email"]["account"]["status"] = new AccountStatusNode();
}

status_t PostOffice::UserNodeCallback(PostOffice *postOffice, uint32 observed, void *data)
{
	MDB(MailDebug md);
	const char *username = reinterpret_cast<const char *>(data);
	switch (observed) {
		case B_PROPERTY_ADDED:
			postOffice->VerifyMailStatusNode(username);
			break;
		case B_PROPERTY_REMOVED: {
			BAutolock _lock(postOffice->fCacheLock);
			SummaryContainer *container = postOffice->LoadedSummaryContainer();
			if (strcmp(container->Username(), username) == 0) {
				// Currently logged in user has been deleted, do some cleanup
				container->SetTo(B_EMPTY_STRING, B_EMPTY_STRING);
				BinderNode::Root()["user"][username]["email"]["account"]["status"] = BinderNode::property::undefined;
				
			}
			break;
		}
		default:
			break;
	}
	return B_OK;
}

inline bool PostOffice::CanSync()
{
	BAutolock _lock(fSyncingLock);
	// If we're already syncing, then we can't start a new sync
	if (fSyncing)
		return false;
	// Otherwise, we're not syncing, so we can start a new one.
	fSyncing = true;
	return true;
}

inline void PostOffice::EnableCheckMailButton(bool enable)
{
	BinderNode::property arg = (enable ? 1 : 0);
	BinderNode::Root()["service"]["web"]["topContent"]["window"]["enable_check_button"](&arg, NULL);
	BinderNode::Root()["service"]["web"]["topContent"]["window"]["enable_trash_button"](&arg, NULL);
}

void PostOffice::ReplaceMacros(BString &inBuf, const BStringMap &map)
{
	BString out;
	char variableName[128],outBuf[1024];
	const char *in = inBuf.String();
	while (*in) {
		if (*in == '$') {
			in++;
			int i = 0;
			int len = 0;
			while (*in && (((*in >= 'A') && (*in <= 'Z')) || (*in == '_'))) {
				if (len++ < 128)
					variableName[i++] = *in;
				in++;
			}
			variableName[i] = '\0';
			BString *found = map.Find(variableName);
			if (found) {
				URL::EscapePathString(false,outBuf,found->String(),found->Length());
				out << outBuf;
			}
		} else
			out << *in++;
	}
	inBuf = out;
}

// End of PostOffice.cpp
