/*
 * libbind_netdb.c
 * Copyright (c) 2001 Be, Inc.	All Rights Reserved 
 *
 * libnet-compatible name resolver which uses libbind to do the resolving
 *
 */

//note that we include <netdb.h> (not <oldnetdb.h>) - this is because
//ISC libbind and howard's resolver define 'h_errno' differently, and
//including <netdb.h> gives us the proper definition.  luckily, the
//net_server's 'hostent' and 'servent' structures, 'h_errno' values,
//and function prototypes are all identical to those used by libbind,
//so <netdb.h> properly defines everything we need
#include <netdb.h>
#undef inet_addr
#undef inet_nota

//libbind.so contains weak symbols which alias these functions to
//the proper names
extern struct hostent *__b_gethostbyname(const char *hostname);
extern struct hostent *__b_gethostbyaddr(const char *hostname, int len, int type);
extern struct servent *__b_getservbyname(const char *name, const char *proto);
extern int *__h_errno();
extern void __b_herror(const char *str);
extern unsigned long __inet_addr(const char *a_addr);
extern char *__inet_ntoa(struct in_addr addr);
extern int _h_errno();
extern const char *__b_hstrerror(int herr);

struct hostent *gethostbyname(const char *hostname)
{
	//call through to libbind.so
	return __b_gethostbyname(hostname);
}

struct hostent *gethostbyaddr(const char *hostname, int len, int type)
{
	//call through to libbind.so
	return __b_gethostbyaddr(hostname, len, type);
}

struct servent *getservbyname(const char *name, const char *proto)
{
	//call through to libbind.so
	return __b_getservbyname(name, proto);
}

//oldnetdb.h #defines h_errno to '*_h_errnop()'
int *_h_errnop(void)
{
	static int __herrno;

	//retrieve the current error number from the resolver (note that,
	//since we included <netdb.h>, this works properly with both
	//libbinds, even though they actually implement 'h_errno' differently)
	__herrno = h_errno;
	return &__herrno;
}

void herror(const char *str)
{
	//call through to libbind.so
	__b_herror(str);
}

const char *hstrerror(int herr)
{
	//call through to libbind.so
	return __b_hstrerror(herr);
}

unsigned long inet_addr(const char *a_addr)
{
	//call through to libbind.so
	return __inet_addr(a_addr);
}

char *inet_ntoa(struct in_addr addr)
{
	//call through to libbind.so
	return __inet_ntoa(addr);
}
