#ifndef __MAIL_LOCAL_CLIENT_DATA_H__
#define __MAIL_LOCAL_CLIENT_DATA_H__

#include <SupportDefs.h>

class BString;
class BDirectory;

class MDMail;
class MDMailList;

class MailLOCALClientData
{
public:
					MailLOCALClientData();
					~MailLOCALClientData();

void				PrintToStream();
void 				GetMailboxesList(BDirectory,MDMailboxesList *list);

BString				root;
BString				selected_mailbox;

MDMailList			mail_list;			// used when a mailbox is selected

MDMail				*read_mail;
int32				index_start;
int32				index_end;
int32				index;
};

#endif
