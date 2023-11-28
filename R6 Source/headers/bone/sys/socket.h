/*
 * sys/socket.h
 * Copyright (c) 1999, 2000 Be, Inc.	All Rights Reserved 
 *
 * socket interface
 *
 */

#ifndef H_SYS_SOCKET
#define	H_SYS_SOCKET

#include <BeBuild.h>
#include <sys/types.h>
#include <sys/time.h>      
#include <OS.h>
#include <ByteOrder.h>

__extern_c_start

/* The BeOS Networking Environment version number */
#define BONE_VERSION	0x010a      /* 1.0a */

/*
 * Address/protocol families.  
 */
#define AF_UNSPEC		0
#define	AF_INET			1		
#define	AF_APPLETALK	2
#define AF_ROUTE        3
#define AF_LINK			4
#define	AF_INET6		5
#define AF_DLI			6	
#define AF_IPX			7
#define AF_NOTIFY		8
#define AF_MAX			9

#define PF_UNSPEC 		AF_UNSPEC		
#define	PF_INET			AF_INET					
#define	PF_APPLETALK	AF_APPLETALK	
#define PF_ROUTE 		AF_ROUTE        
#define PF_LINK 		AF_LINK			
#define	PF_INET6		AF_INET6
#define PF_DLI			AF_DLI			
#define PF_IPX			AF_IPX
#define PF_NOTIFY  		AF_NOTIFY        
#define PF_MAX  		AF_MAX         


/*
 * Types
 */
#define	SOCK_STREAM	1
#define	SOCK_DGRAM	2
#define	SOCK_RAW	3
#define SOCK_MISC	255


/*
 * Socket Options at the SOL_SOCKET level
 */
#define	SOL_SOCKET		0xffffffff		

#define SO_ACCEPTCONN	0x00000001
#define SO_BROADCAST	0x00000002
#define	SO_DEBUG		0x00000004
#define	SO_DONTROUTE	0x00000008
#define	SO_KEEPALIVE	0x00000010
#define SO_OOBINLINE    0x00000020
#define	SO_REUSEADDR	0x00000040		
#define SO_REUSEPORT	0x00000080
#define SO_USELOOPBACK	0x00000100
#define SO_LINGER		0x00000200

#define SO_SNDBUF		0x40000001
#define SO_SNDLOWAT		0x40000002
#define SO_SNDTIMEO		0x40000003
#define SO_RCVBUF		0x40000004
#define SO_RCVLOWAT		0x40000005
#define SO_RCVTIMEO		0x40000006
#define	SO_ERROR		0x40000007
#define	SO_TYPE			0x40000008
#define SO_NONBLOCK		0x40000009
#define SO_BINDTODEVICE	0x4000000a

struct linger {
	int l_onoff;
	int l_linger;
};

/*
 * socket address structure
 */
struct sockaddr {
	uint8	sa_len;			
	uint8	sa_family;		
	uint8	sa_data[30];	/* may be longer.  This is big enough for IPv6 */	
};


#define	SOMAXCONN	32	/* Max listen queue */

/*
 * flags for recv/send
 */
#define	MSG_OOB			0x0001		/* process OOB data */
#define	MSG_PEEK		0x0002		/* do not dequeue data */
#define MSG_DONTROUTE	0x0004		/* send without using routing tables */
#define MSG_EOR			0x0008		/* data completes record */
#define MSG_TRUNC		0x0010		/* data discarded before delivery */
#define MSG_CTRUNC		0x0020		/* control data lost before delivery */
#define MSG_WAITALL		0x0040		/* wait for full request or error */
#define MSG_DONTWAIT	0x0080		/* don't block */
#define MSG_BCAST		0x0100		/* received via link-level bcast */
#define MSG_MCAST		0x0200		/* received via link-level mcast */
#define	MSG_EOF			0x0400		/* data completes connection */

/*
 * direction constants for shutdown()
 */
#define SHUTDOWN_RECV	0x0
#define SHUTDOWN_SEND	0x1
#define SHUTDOWN_BOTH	0x2

#ifndef _KERNEL_MODE

/*
 * the sockets API, alphabetized for your reading pleasure
 */

int		accept(int sock, struct sockaddr *addr, int *addrlen);
int		bind(int sock, const struct sockaddr *addr, int addrlen);
int		connect(int sock, const struct sockaddr *addr, int addrlen);
int		getpeername(int sock, struct sockaddr *addr, int *addrlen);
int		getsockname(int sock, struct sockaddr *addr, int *addrlen);
int		getsockopt(int sock, int level, int option, void *optval, int *optlen);
int		listen(int sock, int backlog);
ssize_t	recv(int sock, void *data, size_t datalen, int flags);
ssize_t	recvfrom(int sock, void *data, size_t datalen, int flags, struct sockaddr *addr, int *addrlen);
ssize_t	send(int sock, const void *data, size_t datalen, int flags);
ssize_t	sendto(int sock, const void *data, size_t datalen, int flags, const struct sockaddr *addr, int addrlen);
int		setsockopt(int sock, int level, int option, const void *optval, int optlen);
int		shutdown(int sock, int direction);
int		socket(int family, int type, int proto);

#endif	/* _KERNEL_MODE */

__extern_c_end

#endif /* H_SYS_SOCKET */
