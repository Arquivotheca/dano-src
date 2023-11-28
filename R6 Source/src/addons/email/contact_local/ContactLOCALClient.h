#ifndef __CONTACT_LOCAL_CLIENT_H__
#define __CONTACT_LOCAL_CLIENT_H__

#ifdef CONTACT_CLIENT_LOCAL_DEBUG
#define CONTACT_CLIENT_LOCAL_PRINT(f...) 			printf("ContactLOCALClient : " f)
#else
#define CONTACT_CLIENT_LOCAL_PRINT(f...)				0
#endif

#endif
