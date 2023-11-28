#ifndef __MDMAILADDON_H__
#define __MDMAILADDON_H__

#include <SupportDefs.h>
#include <OS.h>
#include <image.h>
#include <Entry.h>
#include <String.h>

class BMallocIO;

#include "MDMailAddonDefs.h"

class MDMailboxesList;
class MDMailHeadersList;
class MDMailHeader;
class MDMail;
class MDMailList;
class MDMailContainer;

class BMessage;

class MDMailAddon
{
public:
					MDMailAddon(image_id);
virtual 			~MDMailAddon();

image_id			image;

// The add-on says what it is able to do
virtual	md_mailaddon_capability		GetCapabilities();

// The add-on says in which direction it is able to send data.
virtual	uint32		GetDirection();

// Used to start the connection (can be network or not)
virtual	status_t	Connect(const char *server, uint16 port, bool secured = false);
virtual	bool		IsConnected();

// Login/Logout
// login process, use login/password and authentication method (can be none, login, md5)
virtual	status_t	Login(const char *login, const char *password, md_mailaddon_authentication authentication = md_mailaddon_authentication_none);
// logout process
virtual status_t	Logout();

// Server functions
virtual status_t	GetServerAvailableSpace(const char *mailbox, off_t *storage_current, off_t *storage_limit, int32 *message_current, int32 *message_limit);

// Folders
// set the root of the folders (used by the local addon)
virtual	status_t	SetMailboxesRoot(const char *);

// return the list of all available folders
virtual status_t	GetMailboxesList(MDMailboxesList *list, bool get_mailbox_stats = true);

// return the list of all subscribed available folders
virtual status_t	GetSubscribedMailboxesList(MDMailboxesList *list, bool get_mailbox_stats = true);

// create a mailbox
virtual status_t	CreateMailbox(const char *ref, const char *name);
// delete a mailbox
virtual status_t	DeleteMailbox(const char *name);
// rename a mailbox
virtual status_t	RenameMailbox(const char *old_name, const char *new_name);

// subscribe a mailbox
virtual status_t	SubscribeMailbox(const char *name);
// unsubscribe a mailbox
virtual status_t	UnsubscribeMailbox(const char *name);

// mailbox stats
// returns the total number of messages, the total number of new messages in a mailbox
virtual status_t	GetMailboxStats(const char *name, uint32 *total_messages, uint32 *new_messages);

// mailbox selection
// select a mailbox (read and write)
virtual status_t	SelectMailbox(const char *);
// select a mailbox (read only)
virtual status_t	SelectMailboxRO(const char *);
// deselect a mailbox
virtual status_t	DeselectMailbox();

// mailbox cleaning
// clean a mailbox (like expunge for imap addon)
virtual	status_t	CleanupMailbox();

// send data the server
virtual	status_t	SendData(MDMail *, const char *);

// data receiving (headers only)
virtual status_t	GetMailHeaders(const char *uid,MDMailHeader *, const BMessage *needed_headers = NULL);
virtual status_t	GetMailHeadersStart(const char *start, const char *end, const BMessage *needed_headers = NULL);
virtual status_t	GetMailHeaders(MDMailHeadersList *,bool *, int32);

// data receiving (structure only)
virtual	status_t	GetMailStructure(const char *uid, MDMail *);
virtual	status_t	GetMailStructureStart(const char *start, const char *end);
virtual	status_t	GetMailStructure(MDMailList *,bool *, int32);

// data receiving (headers+structure only)
virtual	status_t	GetMailHeadersStructure(const char *uid, MDMail *, const BMessage *needed_headers = NULL);
virtual	status_t	GetMailHeadersStructureStart(const char *start, const char *end, const BMessage *needed_headers = NULL);
virtual	status_t	GetMailHeadersStructure(MDMailList *, bool *, int32);

// data receiving (body)
virtual status_t	GetMailContentStart(const char *mail_id, const char *section);							// start transfer
virtual status_t	GetMailContent(MDMailContainer *,BMallocIO *,int64,int64 *, bool *);				// loading n bytes
virtual status_t	GetMailContentEnd();																// finish the transfer

// message functions
virtual	status_t	DeleteMail(const char *uid);

private:

char					*fServer;			// server address
uint16					fServerPort;		// server port

char					*fLogin;			// login
char					*fPassword;			// password

void					*protocol_data;		// used to store private data
bool					connected;
};

// exports
extern "C" _EXPORT MDMailAddon* instantiate_mail_daemon_addon(image_id id);

extern "C" _EXPORT MDMailAddon* create_mail_daemon_addon_client(const char *);
extern "C" _EXPORT void delete_mail_daemon_addon_client(MDMailAddon *);

#endif
