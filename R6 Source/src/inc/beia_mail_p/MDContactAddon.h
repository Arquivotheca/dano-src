#ifndef __MDCONTACTADDON_H__
#define __MDCONTACTADDON_H__

#include <SupportDefs.h>
#include <OS.h>
#include <image.h>
#include <Entry.h>
#include <String.h>

#include "MDContactAddonDefs.h"

class MDAddressbooksList;
class MDContact;
class MDContactList;

class BMessage;

class MDContactAddon
{
public:
					MDContactAddon(image_id);
virtual 			~MDContactAddon();

image_id			image;

// The add-on says what it is able to do
virtual	uint32		GetCapabilities();

// Used to start the connection (can be network or not)
virtual	status_t	Connect(const char *server, uint16 port, bool secured = false);
virtual	bool		IsConnected();

// return the list of all available folders
virtual status_t	GetAddressbooksList(MDAddressbooksList *list);

// create an addressbook
virtual status_t	CreateAddressbook(const char *ref, const char *name);
// delete addressbook
virtual status_t	DeleteAddressbook(const char *name);
// rename addressbook
virtual status_t	RenameAddressbook(const char *old_name, const char *new_name);

// addressbook stats
// returns the total number of entries
virtual status_t	GetAddressbookStats(const char *name, uint32 *total);

// Login/Logout
// login process, use login/password and authentication method (can be none, login, md5)
virtual	status_t	Login(const char *login, const char *password, md_contactaddon_authentication authentication = md_contactaddon_authentication_none);
// logout process
virtual status_t	Logout();

// mailbox selection
// select an addressbook
virtual status_t	SelectAddressbook(const char *);
// deselect an addressbook
virtual status_t	DeselectAddressbook();

// Folders
// set the root of the folders (used by the local addon)
virtual	status_t	SetAddressbookRoot(const char *);

virtual status_t	AddContact(MDContact *);
virtual status_t	DeleteContact(int64);
virtual status_t	ModifyContact(int64, MDContact *);

// query functions
virtual status_t 	QueryContactStart(const BMessage *parameters);
virtual status_t	QueryContact(MDContactList *, bool *, int32);

private:

char					*fServer;			// server address
uint16					fServerPort;		// server port

char					*fLogin;			// login
char					*fPassword;			// password

void					*protocol_data;		// used to store private data
bool					connected;
};

// exports
extern "C" _EXPORT MDContactAddon* instantiate_contact_daemon_addon(image_id id);

extern "C" _EXPORT MDContactAddon* create_contact_daemon_addon_client(const char *);
extern "C" _EXPORT void delete_address_contact_daemon_client(MDContactAddon *);

#endif
