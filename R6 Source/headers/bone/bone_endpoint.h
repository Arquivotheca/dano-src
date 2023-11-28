/*
 * bone_endpoint.h
 * Copyright (c) 1999-2000 Be, Inc.	All Rights Reserved 
 *
 * Networking endpoint descriptor
 *
 */


#ifndef H_BONE_ENDPOINT
#define H_BONE_ENDPOINT

#include <bone_util.h>
#include <bone_data.h>
#include <sys/socket.h>
#include <net/if.h>
#include <bone_proto.h>


typedef struct bone_endpoint
{
	bone_benaphore_t    lock;
	bone_proto_info_t	*pinfo;			/* the proto info for the stack head */
	bone_proto_node_t	*stack_head;	/* the protocol stack for this endpoint */
	status_t			async_error;
	
	struct sockaddr 	*addr;	/* addr & peer are set to initially point to */
	struct sockaddr		_addr;			/* _addr & _peer.  If an endpoint needs large */
	int 				addrlen;		/* sockaddr's, it should allocate them. */
	
	struct sockaddr		*peer;			
	struct sockaddr		_peer;
	int					peerlen;
	
	if_index_t			ifindex;		/* interface bound by SO_BINDTODEVICE */
	
	int32				family;
	int32				type;
	int32				proto;
	
	uint32 				options;		/* SOL_SOCKET socket options */
	int32				linger;			/* valid if SO_LINGER is set */
	
	struct {
		uint32				buf;		/* buffer size in bytes */
		uint32				lowat;		/* low water mark */
		bigtime_t			timeo;		/* io timeout */
	} snd, rcv;
	
	/* listen queue */
	struct {
		/* Our parent and the linked list pointers used by our parent */
		struct bone_endpoint	*parent;
		struct bone_endpoint	*next;
		struct bone_endpoint	**prev;
		
		/* our listen queue */
		struct bone_endpoint	*head;		/* the head of our ready queue */
		struct bone_endpoint	*tail;		/* the tail of our ready queue */
		struct bone_endpoint	*pending;	/* linked list of pending connections */
		int32					max;		/* the maximum connections allowed */
		int32					backlog;	/* the number of _remaining_ connections allowed */
	} lq;
	
	struct notification			*notify[3];
} bone_endpoint_t;

#endif /* H_BONE_ENDPOINT */
