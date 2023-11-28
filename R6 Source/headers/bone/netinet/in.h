/*
	in.h
	
	internet addressing
	
	Copyright 1999, Be Incorporated, All Rights Reserved.
	
	portions of this file are:

	 * Copyright (c) 1982, 1986, 1990, 1993 
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
	 *      @(#)in.h        8.3 (Berkeley) 1/3/94 
	 * $Id: in.h,v 1.22.2.1 1996/11/11 23:40:37 phk Exp $  

*/

#ifndef H_NETINET_IN
#define H_NETINET_IN

#include <OS.h>
#include <ByteOrder.h>
#include <endian.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * internet protocols
 */

#define IPPROTO_IP		0    /* dummy for setting IP level options */
#define IPPROTO_ICMP	1    /* control message protocol */
#define IPPROTO_IGMP	2    /* multicast group management protocol */
#define IPPROTO_TCP		6    /* transmission control protocol */
#define IPPROTO_EGP		8	 /* Exterior Gateway Protocol */
#define IPPROTO_UDP		17   /* user datagram protocol */
#define IPPROTO_IPV6	41   /* tunneled ipv6 */
#define IPPROTO_ICMPV6	58   /* tunneled icmpv6 */
#define IPPROTO_DIVERT	254	 /* divert pseudo-protocol */
#define IPPROTO_RAW		255  /* raw ip support */
#define IPPROTO_MAX		256

/* 
 * Ports < IPPORT_RESERVED are reserved for 
 * privileged processes (e.g. root).         (IP_PORTRANGE_LOW) 
 * Ports > IPPORT_USERRESERVED are reserved 
 * for servers, not necessarily privileged.  (IP_PORTRANGE_DEFAULT) 
 */ 
#define        IPPORT_RESERVED         1024 
#define        IPPORT_USERRESERVED     5000 

/* 
 * Default local port range to use by setting IP_PORTRANGE_HIGH 
 */ 
#define        IPPORT_HIFIRSTAUTO      40000 
#define        IPPORT_HILASTAUTO       44999 

/* 
 * Scanning for a free reserved port return a value below IPPORT_RESERVED, 
 * but higher than IPPORT_RESERVEDSTART.  Traditionally the start value was 
 * 512, but that conflicts with some well-known-services that firewalls may 
 * have a fit if we use. 
 */ 
#define IPPORT_RESERVEDSTART   600 
                                                               
/*
 * internet addressing
 */

typedef uint16 in_port_t;
typedef uint32 in_addr_t;

typedef struct in_addr
{
	in_addr_t s_addr;
} in_addr;


/* 
 * Definitions of bits in internet address integers. 
 * On subnets, the decomposition of addresses to host and net parts 
 * is done according to subnet mask, not the masks here. 
 */ 
#define        IN_CLASSA(i)             (((in_addr_t)(i) & 0x80000000) == 0) 
#define        IN_CLASSA_NET           0xff000000 
#define        IN_CLASSA_NSHIFT        24 
#define        IN_CLASSA_HOST          0x00ffffff 
#define        IN_CLASSA_MAX           128 

#define        IN_CLASSB(i)             (((in_addr_t)(i) & 0xc0000000) == 0x80000000) 
#define        IN_CLASSB_NET           0xffff0000 
#define        IN_CLASSB_NSHIFT        16 
#define        IN_CLASSB_HOST          0x0000ffff 
#define        IN_CLASSB_MAX           65536 

#define        IN_CLASSC(i)            (((in_addr_t)(i) & 0xe0000000) == 0xc0000000) 
#define        IN_CLASSC_NET           0xffffff00 
#define        IN_CLASSC_NSHIFT        8 
#define        IN_CLASSC_HOST          0x000000ff 

#define        IN_CLASSD(i)             (((in_addr_t)(i) & 0xf0000000) == 0xe0000000) 
#define        IN_CLASSD_NET           0xf0000000      /* These ones aren't really */ 
#define        IN_CLASSD_NSHIFT        28              /* net and host fields, but */ 
#define        IN_CLASSD_HOST          0x0fffffff      /* routing needn't know.    */ 
#define        IN_MULTICAST(i)             IN_CLASSD(i) 

#define        IN_EXPERIMENTAL(i)     (((in_addr_t)(i) & 0xf0000000) == 0xf0000000) 
#define        IN_BADCLASS(i)          (((in_addr_t)(i) & 0xf0000000) == 0xf0000000) 

#define        INADDR_ANY              (in_addr_t)0x00000000 
#define        INADDR_BROADCAST        (in_addr_t)0xffffffff      /* must be masked */ 
#define        INADDR_NONE             0xffffffff              /* -1 return */ 

#define        INADDR_UNSPEC_GROUP     (in_addr_t)0xe0000000      /* 224.0.0.0 */ 
#define        INADDR_ALLHOSTS_GROUP   (in_addr_t)0xe0000001      /* 224.0.0.1 */ 
#define        INADDR_ALLRTRS_GROUP    (in_addr_t)0xe0000002      /* 224.0.0.2 */ 
#define        INADDR_MAX_LOCAL_GROUP  (in_addr_t)0xe00000ff      /* 224.0.0.255 */ 

#define        IN_LOOPBACKNET          127                     /* official! */ 
#define        INADDR_LOOPBACK         (in_addr_t)0x7f000001      /* 127.0.0.1 */

/* 
 * Structure used to describe IP options. 
 * Used to store options internally, to pass them to a process, 
 * or to restore options retrieved earlier. 
 * The ip_dst is used for the first-hop gateway when using a source route 
 * (this gets put into the header proper). 
 */ 
typedef struct { 
        struct        in_addr ip_dst;         /* first hop, 0 w/o src rt */ 
        char        ip_opts[40];            /* actually variable in size */ 
} ip_opts_t;

/* 
 * Options for use with [gs]etsockopt at the IP level. 
 * First word of comment is data type; bool is stored in int.
 * Some of these are not implemented.
 */

#define	IP_HDRINCL				0x00000001	/* bool; header is included with data */ 
#define IP_RECVIF				0x00000002	/* bool; receive reception if w/dgram */
#define IP_RECVOPTS				0x00000004	/* bool; receive all IP opts w/dgram */ 
#define IP_RECVRETOPTS			0x00000008	/* bool; receive IP opts for response */ 
#define IP_RECVDSTADDR			0x00000010	/* bool; receive IP dst addr w/dgram */
#define IP_ONESBCAST			0x00000020	/* bool; IP broadcast address always set to 255.255.255.255 by kernel */

#define IP_OPTIONS				0x40000001	/* buf/ip_opts; set/get IP options */ 
#define IP_TOS					0x40000002	/* int; IP type of service and preced. */ 
#define IP_TTL					0x40000003	/* int; IP time to live */ 
 
#define IP_RETOPTS				0x40000004	/* ip_opts; set/get IP options */ 
#define IP_MULTICAST_IF			0x40000005	/* u_char; set/get IP multicast i/f  */ 
#define IP_MULTICAST_TTL		0x40000006	/* u_char; set/get IP multicast ttl */ 
#define IP_MULTICAST_LOOP		0x40000007	/* u_char; set/get IP multicast loopback */ 
#define IP_ADD_MEMBERSHIP		0x40000008	/* ip_mreq; add an IP group membership */ 
#define IP_DROP_MEMBERSHIP		0x40000009	/* ip_mreq; drop an IP group membership */

#define IP_MULTICAST_VIF		0x4000000a	/* set/get IP mcast virt. iface */ 
#define IP_RSVP_ON				0x4000000b	/* enable RSVP in kernel */ 
#define IP_RSVP_OFF				0x4000000c	/* disable RSVP in kernel */ 
#define IP_RSVP_VIF_ON			0x4000000d	/* set RSVP per-vif socket */ 
#define IP_RSVP_VIF_OFF			0x4000000e	/* unset RSVP per-vif socket */ 
#define IP_PORTRANGE			0x4000000f	/* int; range to choose for unspec port */ 


#define IP_FW_ADD				0x40000010	/* add a firewall rule to chain */ 
#define IP_FW_DEL				0x40000020	/* delete a firewall rule from chain */ 
#define IP_FW_FLUSH				0x40000030	/* flush firewall rule chain */ 
#define IP_FW_ZERO				0x40000040	/* clear single/all firewall counter(s) */ 
#define IP_FW_GET				0x40000050	/* get entire firewall rule chain */ 
#define IP_NAT					0x40000060	/* set/get NAT opts */ 

/* 
 * Defaults and limits for options 
 */ 
#define        IP_DEFAULT_MULTICAST_TTL  1     /* normally limit m'casts to 1 hop  */ 
#define        IP_DEFAULT_MULTICAST_LOOP 1     /* normally hear sends if a member  */ 
#define        IP_MAX_MEMBERSHIPS      20      /* per socket */ 

/* 
 * Argument structure for IP_ADD_MEMBERSHIP and IP_DROP_MEMBERSHIP. 
 */ 
struct ip_mreq { 
        struct        in_addr imr_multiaddr;  /* IP multicast address of group */ 
        struct        in_addr imr_interface;  /* local IP address of interface */ 
}; 

/* 
 * Argument for IP_PORTRANGE: 
 * - which range to search when port is unspecified at bind() or connect() 
 */ 
#define        IP_PORTRANGE_DEFAULT    0       /* default range */ 
#define        IP_PORTRANGE_HIGH       1       /* "high" - request firewall bypass */ 
#define        IP_PORTRANGE_LOW        2       /* "low" - vouchsafe security */ 


/*
 * internet socket address
 */
struct sockaddr_in
{
	uint8			sin_len;
	uint8			sin_family;
	in_port_t		sin_port;
	struct in_addr	sin_addr;
	uint8			sin_zero[24];
};

typedef struct in_proto_state
{
	int proto;
} in_proto_state_t;


#ifdef __cplusplus
}
#endif

#endif
