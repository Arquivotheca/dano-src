#ifndef __MAIL_LOCAL_CLIENT_H__
#define __MAIL_LOCAL_CLIENT_H__

#ifdef MAIL_CLIENT_LOCAL_DEBUG
#define MAIL_CLIENT_LOCAL_PRINT(f...) 			printf("MailLOCALClient : " f)
#else
#define MAIL_CLIENT_LOCAL_PRINT(f...)				0
#endif

#endif
