#ifndef __MAIL_IMAP_CLIENT_H__
#define __MAIL_IMAP_CLIENT_H__

#ifdef MAIL_CLIENT_IMAP_DEBUG
#define MAIL_CLIENT_IMAP_PRINT(f...) 			printf("MailIMAPClient : " f)
#else
#define MAIL_CLIENT_IMAP_PRINT(f...)				0
#endif

#endif
