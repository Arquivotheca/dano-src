#ifndef __MAIL_SMTP_CLIENT_H__
#define __MAIL_SMTP_CLIENT_H__

#ifdef MAIL_CLIENT_SMTP_DEBUG
#define MAIL_CLIENT_SMTP_PRINT(f...) 			printf("MailSMTPClient : " f)
#else
#define MAIL_CLIENT_SMTP_PRINT(f...)				0
#endif

#endif
