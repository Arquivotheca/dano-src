/*
 	BONE net API driver includes

	Copyright (C) 1999 Be Incorporated.  All Rights Reserved.
 */

#ifndef H_BONE_API
#define H_BONE_API

#include <bone_ioctl.h>
#include <iovec.h>

typedef enum {
	BONE_API_IOCTL_MIN = BONE_API_IOCTL_BASE,
	BONE_API_ACCEPT,
	BONE_API_BIND,
	BONE_API_CONNECT,
	BONE_API_GETPEER,
	BONE_API_GETSOCK,
	BONE_API_GETOPT,
	BONE_API_LISTEN,
	BONE_API_RECV,
	BONE_API_RECVFROM,
	BONE_API_SEND,
	BONE_API_SENDTO,
	BONE_API_SETOPT,
	BONE_API_SHUTDOWN,
	BONE_API_SOCKET,
	BONE_API_GET_COOKIE, 
	BONE_API_IOCTL_MAX	
} bone_api_ioctls;

typedef struct int_ioctl
{
	int			rc;
	int 		value;
} int_ioctl_t;

typedef struct sockaddr_ioctl
{
	int				rc;
	int 			len;
	struct sockaddr *addr;
} sockaddr_ioctl_t;

typedef struct sockopt_ioctl
{
	int		rc;
	int 	level;
	int 	option;
	void 	*optval;
	int 	optlen;
} sockopt_ioctl_t;

typedef struct data_xfer_ioctl
{
	int					rc;
	void				*data;
	int					datalen;
	int 				flags;
	struct sockaddr 	*addr;
	int 				addrlen;
} data_xfer_ioctl_t;

typedef struct socket_ioctl
{
	int	rc;
	int family;
	int type;
	int proto;
} socket_ioctl_t;

typedef struct accept_ioctl {
	int				rc;
	void 			*cookie;
	int 			len;
	struct sockaddr *addr;	
} accept_ioctl_t;

typedef union api_ioctl {
	int_ioctl_t			arg_int;
	socket_ioctl_t		socket;	
	data_xfer_ioctl_t	xfer;
	sockopt_ioctl_t		opt;
	sockaddr_ioctl_t	addr;
	accept_ioctl_t		accept;
} api_ioctl_t;

#define BONE_API_DRIVER "net/api"

#endif
