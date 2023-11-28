/*
	Modified for BeOS
	
	Copyright 1999, Be Incorporated, All Rights Reserved.
*/
/* 
 * Copyright (c) 1980, 1986, 1993 
 *      The Regents of the University of California.  All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 * 3. All advertising materials mentioning features or use of this software 
 *    must display the following acknowledgement: 
 *      This product includes software developed by the University of 
 *      California, Berkeley and its contributors. 
 * 4. Neither the name of the University nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE. 
 * 
 *      @(#)route.h     8.3 (Berkeley) 4/19/94 
 * $Id: route.h,v 1.23 1996/10/09 18:35:10 wollman Exp $ 
 */ 

#ifndef _NET_ROUTE_H_ 
#define _NET_ROUTE_H_ 

#include <SupportDefs.h>
#include <net/if.h>
/* 
 * Kernel resident routing tables. 
 * 
 * The routing tables are initialized when interface addresses 
 * are set by making entries for all directly connected interfaces. 
 */ 

/* 
 * A route consists of the destination address, a reference to
 * a routing entry, and pointers for a double-linked list.
 */ 
typedef struct route {
	struct route *ro_next;
	struct route *ro_prev;
	struct rtentry *ro_rt;
	struct sockaddr ro_dst;	/* must be last in struct, to accommodate large sockaddr_*s */
} route_t;

/* 
 * These numbers are used by reliable protocols for determining 
 * retransmission behavior and are included in the routing structure. 
 */ 
struct rt_metrics { 
        uint32  rmx_locks;      /* Kernel must leave these values alone */ 
        uint32  rmx_mtu;        /* MTU for this path */ 
        uint32  rmx_hopcount;   /* max hops expected */ 
        uint32  rmx_expire;     /* lifetime for route, e.g. redirect */ 
        uint32  rmx_recvpipe;   /* inbound delay-bandwidth product */ 
        uint32  rmx_sendpipe;   /* outbound delay-bandwidth product */ 
        uint32  rmx_ssthresh;   /* outbound gateway buffer limit */ 
        uint32  rmx_rtt;        /* estimated round trip time */ 
        uint32  rmx_rttvar;     /* estimated rtt variance */ 
        uint32  rmx_pksent;     /* packets sent using this route */ 
}; 


/* 
 * We distinguish between routes to hosts and routes to networks, 
 * preferring the former if available.  For each route we infer 
 * the interface to use from the gateway address supplied when 
 * the route was entered.  Routes that forward packets through 
 * gateways are marked so that the output routines know to address the 
 * gateway rather than the ultimate destination. 
 */
typedef struct rtentry { 
		struct sockaddr		*rt_dst;
		struct sockaddr 	*rt_mask;
        struct sockaddr 	*rt_gateway;   			/* value */
        uint32				rt_flags;               /* up/down?, host/net */ 
        struct				lognet *rt_ifp;         /* the answer: interface to use */
        struct				rt_metrics rt_rmx;      /* metrics used by rx'ing protocols */ 
        struct				rtentry *rt_gwroute;    /* implied entry for gatewayed routes */
} rtentry_t; 

#define rt_use rt_rmx.rmx_pksent 

#define RTF_UP          0x1             /* route usable */ 
#define RTF_GATEWAY     0x2             /* destination is a gateway */ 
#define RTF_HOST        0x4             /* host entry (net otherwise) */ 
#define RTF_REJECT      0x8             /* host or net unreachable */ 
#define RTF_DYNAMIC     0x10            /* created dynamically (by redirect) */ 
#define RTF_MODIFIED    0x20            /* modified dynamically (by redirect) */ 
#define RTF_DONE        0x40            /* message confirmed */ 
#define RTF_DEFAULT     0x80            /* defines the default route */ 
#define RTF_CLONING     0x100           /* generate new routes on use */ 
#define RTF_XRESOLVE    0x200           /* external daemon resolves name */
#define RTF_LLINFO		0x400
#define RTF_STATIC      0x800           /* manually added */ 
#define RTF_BLACKHOLE   0x1000          /* just discard pkts (during updates) */ 
#define RTF_PROTO2      0x4000          /* protocol specific routing flag */ 
#define RTF_PROTO1      0x8000          /* protocol specific routing flag */ 
 
#define RTF_PRCLONING   0x10000         /* protocol requires cloning */ 
#define RTF_WASCLONED   0x20000         /* route generated through cloning */ 
#define RTF_PROTO3      0x40000         /* protocol specific routing flag */ 
/*                       0x80000            unused */ 
#define RTF_PINNED      0x100000        /* future use */ 
#define RTF_LOCAL       0x200000        /* route represents a local address */ 
#define RTF_BROADCAST   0x400000        /* route represents a bcast address */ 
#define RTF_MULTICAST   0x800000        /* route represents a mcast address */ 
                                        /* 0x1000000 and up unassigned */ 





/* 
 * Routing statistics. 
 */ 
struct        rtstat { 
        int32        rts_badredirect;        /* bogus redirect calls */ 
        int32        rts_dynamic;            /* routes created by redirects */ 
        int32        rts_newgateway;         /* routes modified by redirects */ 
        int32        rts_unreach;            /* lookups which failed */ 
}; 

/*
 *  Routing request
 */
typedef struct route_req {
	int					family;
	struct sockaddr		dst;
	struct sockaddr		mask;
	struct sockaddr		gateway;
	uint32				flags;
	uint32				refcnt;
	char				iface[IFNAMSIZ];
} route_req_t;

typedef struct route_table_req {
	route_req_t *	rrtp;
	uint32			len;
	uint32			cnt;
} route_table_req_t;

#endif 



