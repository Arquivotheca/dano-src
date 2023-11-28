#ifndef __MAIL_IMAP_CLIENT_DATA_H__
#define __MAIL_IMAP_CLIENT_DATA_H__

#include <SupportDefs.h>

#define IMAP_TAG "beia"

class MailClientSocket;
class MailIMAPAnswerList;
class MDMailHeaderEntry;

typedef enum imap_state
{
	imap_state_nothing = 0,
	imap_state_connecting,
	imap_state_capability,
	imap_state_auth,
	imap_state_transaction,
};

typedef enum imap_authentication
{
	imap_authentication_none = 0,
	imap_authentication_plain = 1,
	imap_authentication_login = 2,
	imap_authentication_cram_md5 = 4,
	imap_authentication_digest_md5 = 8,
};

typedef enum imap_capabilities
{
	imap_capabilities_none = 0,
	imap_capabilities_quota = 1,
};

typedef enum imap_version
{
	imap_version_none = 0,
	imap_version_v2,
	imap_version_v4,
	imap_version_v4rev1,
};

class MailIMAPClientData
{
public:
					MailIMAPClientData();
					~MailIMAPClientData();
			
status_t			capability();
status_t			load_server_response(MailIMAPAnswerList *,bool *server_answer);
status_t			load_server_response(MailIMAPAnswerList *,bool *server_answer, int32 nb, char sep = ')', char sep2 = ')');

void				PrintToStream();

void				analyse_select(MailIMAPAnswerList *);
const char *		generate_tag(const char *command);

void				GetMailHeaders(BString *,const char *,const BMessage *needed_headers = NULL);
void				GetMailHeaders(BString *,int64, int64,const BMessage *needed_headers = NULL);

void				GetMailHeadersStructure(BString *,const char *,const BMessage *needed_headers = NULL);
void				GetMailHeadersStructure(BString *,int64, int64,const BMessage *needed_headers = NULL);

void				parse_header(const char *,MDMailHeader *);

MailClientSocket	socket;
	
imap_authentication	authentication;
imap_capabilities	capabilities;
imap_version		version;
imap_state			state;

BString				selected_mailbox;
BString				predicted_uid;

MDMailHeaderEntry	*parsed_entry;

BString				imap_tag;
BString 			returned_imap_tag;

BString				already_loaded_string;

private:
void				AddHeaders(BString *,const BMessage *);
};

#endif
