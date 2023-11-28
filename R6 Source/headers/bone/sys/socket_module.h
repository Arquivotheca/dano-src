/*
 * sys/socket_module.h
 * Copyright (c) 1999 Be, Inc.	All Rights Reserved 
 *
 * kernel module socket interface
 *
 */

#ifndef H_SYS_SOCKET_MODULE
#define	H_SYS_SOCKET_MODULE

#include <module.h>
#include <sys/socket.h>
#include <netinet/in.h>

#if __cplusplus
extern "C" {
#endif /* __cplusplus */


/*
 * the sockets API, alphabetized for your reading pleasure
 */
typedef struct bone_socket_info
{
	struct module_info info;                                             

	int		(*accept)(int sock, struct sockaddr *addr, int *addrlen);
	int		(*bind)(int sock, const struct sockaddr *addr, int addrlen);
	int		(*connect)(int sock, const struct sockaddr *addr, int addrlen);
	int		(*getpeername)(int sock, struct sockaddr *addr, int *addrlen);
	int		(*getsockname)(int sock, struct sockaddr *addr, int *addrlen);
	int		(*getsockopt)(int sock, int level, int option, void *optval, int *optlen);
	int		(*listen)(int sock, int backlog);
	ssize_t	(*recv)(int sock, void *data, size_t datalen, int flags);
	ssize_t	(*recvfrom)(int sock, void *data, size_t datalen, int flags, struct sockaddr *addr, int *addrlen);
	ssize_t	(*send)(int sock, const void *data, size_t datalen, int flags);
	ssize_t	(*sendto)(int sock, const void *data, size_t datalen, int flags, const struct sockaddr *addr, int addrlen);
	int		(*setsockopt)(int sock, int level, int option, const void *optval, int optlen);
	int		(*shutdown)(int sock, int direction);
	int		(*select)(int fd, fd_set *rfds, fd_set *wfds, fd_set *efds, struct timeval *timeout);
	int		(*socket)(int family, int type, int proto);
	unsigned long 	(*inet_addr)(const char *addr);
	char 			*(*inet_ntoa)(struct in_addr addr);
} bone_socket_info_t;

#define BONE_SOCKET_MODULE "network/bone_socket"

#if __cplusplus
}
#endif /* __cplusplus */

#endif /* H_SYS_SOCKETMODULE */
