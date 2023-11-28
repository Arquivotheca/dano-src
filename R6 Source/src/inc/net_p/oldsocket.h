/*
 * socket.h
 * Copyright (c) 2000-2001 Be, Inc.	All Rights Reserved 
 *
 * old socket.h for linking against libnet.  DO NOT USE FOR NEW DEVELOPMENT
 *
 */
#ifndef _OSOCKET_H
#define _OSOCKET_H

#include <BeBuild.h>
#include <sys/types.h>
#include <sys/time.h>       /* for timeval/timezone structs & gettimeofday */
#include <ByteOrder.h>
#include <sys/select.h>		/* for fd_set and select() */

#if __cplusplus
extern "C" {
#endif /* __cplusplus */

#define AF_INET 1


#define INADDR_ANY			0x00000000	
#define INADDR_BROADCAST	0xffffffff
#define INADDR_LOOPBACK		0x7f000001	/* in host order */

#define SOL_SOCKET 1

#define SO_DEBUG 1
#define SO_REUSEADDR 2
#define SO_NONBLOCK 3
#define SO_REUSEPORT 4
#define SO_FIONREAD	5

#define MSG_OOB 0x1

#define SOCK_DGRAM 1
#define SOCK_STREAM 2

#define IPPROTO_UDP 1
#define IPPROTO_TCP 2
#define IPPROTO_ICMP 3

/* 
 * Be extension
 */
#define B_UDP_MAX_SIZE (65536 - 1024) 

struct sockaddr {
	unsigned short sa_family;
	char sa_data[10];
};

struct in_addr {
	unsigned int s_addr;
};

struct sockaddr_in {
	unsigned short sin_family;
	unsigned short sin_port;
	struct in_addr sin_addr;
	char sin_zero[4];
};


int socket(int family, int type, int proto);
int bind(int fd, const struct sockaddr *addr, int size);
int getsockname(int fd, struct sockaddr *addr, int *size);
int getpeername(int fd, struct sockaddr *addr, int *size);
ssize_t recvfrom(int fd, void *buf, size_t size, int flags,
			 struct sockaddr *from, int *fromlen);
ssize_t sendto(int fd, const void *buf, size_t size, int flags,
		   const struct sockaddr *to, int tolen);

ssize_t send(int fd, const void *buf, size_t size, int flags);
ssize_t recv(int fd, void *buf, size_t size, int flags);


int connect(int fd, const struct sockaddr *addr, int size);
int accept(int fd, struct sockaddr *addr, int *size);


int listen(int fd, int backlog);
int closesocket(int fd);

int shutdown(int fd, int how);  /* doesn't work yet */

int setsockopt(int sd, int prot, int opt, const void *data, unsigned datasize);
int getsockopt(int sd, int prot, int opt, void *data, int *datasize);


#if __cplusplus
}
#endif /* __cplusplus */

#endif /* _SOCKET_H */
