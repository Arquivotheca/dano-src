/*
 * bone_endpoint_api.h
 * Copyright (c) 2000 Be, Inc.	All Rights Reserved 
 *
 * Networking endpoint descriptor api
 *
 */

#ifndef H_BONE_EP_API
#define H_BONE_EP_API

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	NOTIFY_SEND =		0,
	NOTIFY_RECV = 		1,
	NOTIFY_EXCEPTION = 	2
} notify_event_t;

struct bone_endpoint;
struct bone_data;

typedef void (*ep_notify_func)(struct bone_endpoint *ep, int32 value, uint32 arg1, void *arg2 );

typedef struct bone_endpoint_info
{
	struct module_info info;
	
	status_t			(*endpoint)(struct bone_endpoint **ep_ptr, int family, int type, int proto);
	status_t			(*open)(struct bone_endpoint *ep);
	status_t			(*close)(struct bone_endpoint *ep);
	status_t			(*free)(struct bone_endpoint *ep);
	
	/* device I/O api */
	status_t			(*read)(struct bone_endpoint *ep, off_t position, void *data, size_t *numBytes);
	status_t			(*write)(struct bone_endpoint *ep, off_t position, const void *data, size_t *numBytes);
	status_t			(*readv)(struct bone_endpoint *ep, off_t position, const iovec *vec, size_t count, size_t *numBytes);
	status_t			(*writev)(struct bone_endpoint *ep, off_t position, const iovec *vec, size_t count, size_t *numBytes);
	status_t			(*control)(struct bone_endpoint *ep, uint32 op, void *data, size_t len);
	
	ssize_t				(*recv_avail)(struct bone_endpoint *ep);
	ssize_t				(*send_avail)(struct bone_endpoint *ep);
	
	/* alternative kernel I/O api */
	status_t			(*send_data)(struct bone_endpoint *ep, struct bone_data *data);
	status_t			(*recv_data)(struct bone_endpoint *ep, size_t numBytes, uint32 flags, struct bone_data **data_ptr);
	
	/* sockets api */
	int					(*accept)(struct bone_endpoint *ep, struct bone_endpoint **ep_ptr, struct sockaddr *addr, int *addrlen);
	int					(*bind)(struct bone_endpoint *ep, const struct sockaddr *addr, int addrlen);
	int					(*connect)(struct bone_endpoint *ep, const struct sockaddr *addr, int addrlen);
	int					(*getpeername)(struct bone_endpoint *ep, struct sockaddr *addr, int *addrlen);
	int					(*getsockname)(struct bone_endpoint *ep, struct sockaddr *addr, int *addrlen);
	int					(*getsockopt)(struct bone_endpoint *ep, int level, int option, void *optval, int *optlen);
	int					(*listen)(struct bone_endpoint *ep, int backlog);
	ssize_t				(*recv)(struct bone_endpoint *ep, void *data, size_t datalen, int flags);
	ssize_t				(*recvfrom)(struct bone_endpoint *ep, void *data, size_t datalen, int flags, struct sockaddr *addr, int *addrlen);
	ssize_t				(*send)(struct bone_endpoint *ep, const void *data, size_t datalen, int flags);
	ssize_t				(*sendto)(struct bone_endpoint *ep, const void *data, size_t datalen, int flags, const struct sockaddr *addr, int addrlen);
	int					(*setsockopt)(struct bone_endpoint *ep, int level, int option, const void *optval, int optlen);
	int					(*shutdown)(struct bone_endpoint *ep, int direction);
	
	/* async. notification */
	status_t			(*request_notification)(struct bone_endpoint *ep, notify_event_t event, bool once, ep_notify_func function, uint32 arg1, void *arg2);
	status_t			(*cancel_notification)(struct bone_endpoint *ep, notify_event_t event, void *arg2);
	
} bone_endpoint_info_t;

#define BONE_EP_MOD_NAME "network/bone_endpoint"

#ifdef __cplusplus
}
#endif

#endif /* H_BONE_EP_API */
