/*
 * netdb.h
 * Copyright (c) 2001 Be, Inc.	All Rights Reserved 
 *
 * old netdb.h for linking against libnet.  DO NOT USE FOR NEW DEVELOPMENT
 *
 */
#ifndef _ONETDB_H
#define _ONETDB_H

#include <oldsocket.h>

#if __cplusplus
extern "C" {
#endif /* __cplusplus */

#undef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64

#define HOST_NOT_FOUND 1
#define TRY_AGAIN 2
#define NO_RECOVERY 3
#define NO_DATA 4

#ifndef h_errno
extern int *_h_errnop(void);
#define h_errno (*_h_errnop())
#endif /* h_errno */


struct hostent {
	char *h_name;
	char **h_aliases;
	int h_addrtype;
	int h_length;
	char **h_addr_list;
};
#define h_addr h_addr_list[0]

struct servent {
	char *s_name;
	char **s_aliases;
	int s_port;
	char *s_proto;
};	

extern struct hostent *gethostbyname(const char *hostname);
extern struct hostent *gethostbyaddr(const char *hostname, int len, int type);
extern struct servent *getservbyname(const char *name, const char *proto);
extern void herror(const char *);
extern const char *hstrerror(int herr);
extern unsigned long inet_addr(const char *a_addr);
extern char *inet_ntoa(struct in_addr addr);


extern int gethostname(char *hostname, size_t hostlen);

/* BE specific, because of lack of UNIX passwd functions */
extern int getusername(char *username, size_t userlen);
extern int getpassword(char *password, size_t passlen);

#if __cplusplus
}
#endif /* __cplusplus */

#endif /* _NETDB_H */
