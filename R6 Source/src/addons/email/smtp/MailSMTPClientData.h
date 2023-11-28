#ifndef __MAIL_SMTP_CLIENT_DATA_H__
#define __MAIL_SMTP_CLIENT_DATA_H__

#include <SupportDefs.h>

class MailClientSocket;
class MailSMTPAnswerList;
class BString;

typedef int enum smtp_state
{
	smtp_state_nothing = 0,
	smtp_state_connecting,
	smtp_state_helo,
	smtp_state_transation,
};

typedef int enum smtp_authentication
{
	smtp_authentication_none = 0,
	smtp_authentication_login,
	smtp_authentication_cram_md5,
};

typedef int enum smtp_capabilities
{
	smtp_capabilities_none = 0,
};

typedef int enum smtp_version
{
	smtp_version_none = 0,
	smtp_version_smtp,
	smtp_version_esmtp,
};

class MailSMTPClientData
{
public:
					MailSMTPClientData();
					~MailSMTPClientData();
			
status_t			hello(const char *,const char *);
status_t			load_server_response(MailSMTPAnswerList *,bool *server_answer);
status_t			load_server_response(BString *,bool *server_answer, int16 answer_code = 250);

void				PrintToStream();

MailClientSocket	socket;
	
smtp_authentication	authentication;
smtp_capabilities	capabilities;
smtp_version		version;
smtp_state			state;

private:
status_t			helo(const char *);
status_t			ehlo(const char *);
};

#endif
