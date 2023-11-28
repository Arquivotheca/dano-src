#ifndef __CONTACT_LOCAL_CLIENT_DATA_H__
#define __CONTACT_LOCAL_CLIENT_DATA_H__

#include <SupportDefs.h>

#include "MDContactList.h"

class BString;
class BDirectory;

class MDContact;
class MDAddressbooksList;

class ContactLOCALClientData
{
public:
					ContactLOCALClientData();
					~ContactLOCALClientData();

void				PrintToStream();
void 				GetAddressbooksList(BDirectory,MDAddressbooksList *list);
bool				Match(MDContact *);
void				Filter(MDContact *);

BString				root;
BString				selected_addressbook;

MDContactList		contact_list;			// used when an addressbook is selected
int32				index;

BMessage 			parameters;
};

#endif
