/*
	Modified for BeOS
	
	Copyright 1999, Be Incorporated, All Rights Reserved.
*/

/* 
 * Copyright (c) 1982, 1986, 1989, 1993 
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
 *      @(#)if.h        8.1 (Berkeley) 6/10/93 
 * $Id: if.h,v 1.36.2.3 1997/10/05 21:41:05 julian Exp $ 
 */ 

#ifndef _NET_IF_H_ 
#define        _NET_IF_H_ 

#include <SupportDefs.h>
#include <OS.h>
#include <sys/socket.h>
#include <net/if_media.h>

//forward reference
struct bone_data;

/*
 * Structure describing information about an interface 
 * which may be of interest to management entities. 
 */

typedef struct if_data { 
        /* generic interface information */
		ifmedia_t	ifi_media;			/* media, see if_media.h */
        uint8  ifi_type;               /* ethernet, tokenring, etc */  
        uint8  ifi_addrlen;            /* media address length */ 
        uint8  ifi_hdrlen;             /* media header length */ 
        uint32  ifi_mtu;                /* maximum transmission unit */
										/* this does not count framing; */
										/* e.g. ethernet would be 1500 */ 
        uint32  ifi_metric;             /* routing metric (external only) */ 
        /* volatile statistics */ 
        uint32  ifi_ipackets;           /* packets received on interface */ 
        uint32  ifi_ierrors;            /* input errors on interface */ 
        uint32  ifi_opackets;           /* packets sent on interface */ 
        uint32  ifi_oerrors;            /* output errors on interface */ 
        uint32  ifi_collisions;         /* collisions on csma interfaces */ 
        uint32  ifi_ibytes;             /* total number of octets received */ 
        uint32  ifi_obytes;             /* total number of octets sent */ 
        uint32  ifi_imcasts;            /* packets received via multicast */ 
        uint32  ifi_omcasts;            /* packets sent via multicast */ 
        uint32  ifi_iqdrops;            /* dropped on input, this interface */ 
        uint32  ifi_noproto;            /* destined for unsupported protocol */ 
        bigtime_t ifi_lastchange; /* time of last administrative change */ 
} ifdata_t; 


/* 
 * The ifaddr structure contains information about one address 
 * of an interface.  They are maintained by the different address families, 
 * are allocated and attached when an address is set, and are linked 
 * together so all addresses for an interface can be located. 
 */
typedef struct ifaddr {
		struct 			domain_info *domain;	/* address family of this address */
        struct        	sockaddr *ifa_addr;     /* address of interface */ 
        struct        	sockaddr *ifa_dstaddr;  /* other end of p-to-p link */ 
#define	ifa_broadaddr   ifa_dstaddr     		/* broadcast address interface */ 
        struct        	sockaddr *ifa_netmask;  /* used to determine subnet */ 
        struct        	lognet *ifa_ifp;        /* back-pointer to logical interface */ 
        struct        	ifaddr *ifa_next;       /* next address for domain */ 
} ifaddr_t; 

/*
 * "Base structure" for any data passed to a physical or logical interface
 * via the datalink (i.e., through an ioctl() or setsockopt(SOL_SOCKET)) -
 * the logical interface's name must appear at the top of any data you
 * send (for an example, see 'ifreq_t' below)
 */
typedef struct {
#define IFNAMSIZ 32
	char lognet_name[IFNAMSIZ];
} lognet_data_base_t;


/* 
 * Structures defining a network interface. 
 */
typedef unsigned int if_index_t;

typedef struct lognet {
		char						log_name[IFNAMSIZ];	/* not necessarily null-terminated */
		struct ifaddr				log_addr;
		if_index_t					log_index;		/* Index number, used by the interface indentifier fcns */
		uint32						log_flags;
		uint8						log_type;		/* Framing type used by this lognet, as defined in if_types.h */
		uint32						log_mtu;		/* logical MTU (hardware MTU less bytes used for framing) */
		uint32						log_metric;		/* routing metric - externally determined/used */
		struct ifnet				*log_driver;	/* back-pointer to the physical interface which sends/recvs our data */
		struct bone_dl_proto_node	*log_out_stack;
		int							log_framer;		/* system-assigned index for this stack's framing module */
		struct lognet				*log_next;		/* pointer to next logical interface attached to same physical interface */
} lognet_t;

typedef struct ifnet { 
        char        				*if_name;		/* name, e.g. ``en'' or ``lo'' */ 
		if_index_t					if_index;		/* Index number, used by the interface indentifier fcns */
        uint32						if_flags;		/* loopback, etc. (only flags in IFF_CANTCHANGE, though) */
		int32						if_up_count;	/* number of "up" logical interfaces running on this device */
		lognet_t					*if_lognets;	/* logical interfaces bound to this physical interface */
		volatile thread_id			if_thread;
		struct bone_interface_info	*if_info;		/* pointer to the specific interface instance */
        struct if_data 				if_data;
		struct bone_proto_info		*peeper;		/* If packet capture enabled, a pointer to the peeper module*/
		void						*if_deframers;	/* used by the datalink to deframe incoming packets */
		struct ifnet 				*if_next;
} ifnet_t; 

#define        if_mtu          if_data.ifi_mtu 
#define        if_type         if_data.ifi_type 
#define			if_media    if_data.ifi_media 
#define        if_addrlen      if_data.ifi_addrlen 
#define        if_hdrlen       if_data.ifi_hdrlen 
#define        if_metric       if_data.ifi_metric 
#define        if_baudrate     if_data.ifi_baudrate 
#define        if_ipackets     if_data.ifi_ipackets 
#define        if_ierrors      if_data.ifi_ierrors 
#define        if_opackets     if_data.ifi_opackets 
#define        if_oerrors      if_data.ifi_oerrors 
#define        if_collisions   if_data.ifi_collisions 
#define        if_ibytes       if_data.ifi_ibytes 
#define        if_obytes       if_data.ifi_obytes 
#define        if_imcasts      if_data.ifi_imcasts 
#define        if_omcasts      if_data.ifi_omcasts 
#define        if_noproto      if_data.ifi_noproto 
#define        if_lastchange   if_data.ifi_lastchange 

#define        IFF_UP          0x1             /* interface is up */ 
#define        IFF_BROADCAST   0x2             /* broadcast address valid */ 
#define        IFF_DEBUG       0x4             /* turn on debugging */ 
#define        IFF_LOOPBACK    0x8             /* is a loopback net */ 
#define        IFF_POINTOPOINT 0x10            /* interface is point-to-point link */
#define        IFF_PTP         IFF_POINTOPOINT
#define		   IFF_NOARP       0x40				/* don't arp */
#define        IFF_AUTOUP      0x80            /* for autodial ppp - anytime the interface is downed, it will immediately be re-upped by the datalink */
#define        IFF_PROMISC     0x100           /* receive all packets */ 
#define        IFF_ALLMULTI    0x200           /* receive all multicast packets */ 
#define        IFF_SIMPLEX     0x800           /* can't hear own transmissions */ 
#define        IFF_MULTICAST   0x8000          /* supports multicast */ 

/* flags set internally only: */ 
#define	IFF_CANTCHANGE (IFF_BROADCAST|IFF_LOOPBACK|IFF_POINTOPOINT| \
		IFF_NOARP|IFF_SIMPLEX|IFF_MULTICAST|IFF_ALLMULTI)


struct rtentry;

/*
 * interface request
 */

typedef struct ifreq {
	char ifr_name[IFNAMSIZ];
	if_index_t		ifr_index;
	struct sockaddr ifr_addr;
	struct sockaddr ifr_mask;
	struct sockaddr ifr_dstaddr;
#define ifr_broadaddr ifr_dstaddr
	ifdata_t		ifr_data;
	uint32          ifr_flags;
	if_index_t		ifr_driver_index;
	uint8			ifr_hwaddr[256];
#define        ifr_mtu          ifr_data.ifi_mtu 
#define        ifr_type         ifr_data.ifi_type 
#define		   ifr_media    ifr_data.ifi_media 
#define        ifr_addrlen      ifr_data.ifi_addrlen 
#define        ifr_hdrlen       ifr_data.ifi_hdrlen 
#define        ifr_metric       ifr_data.ifi_metric 
#define        ifr_baudrate     ifr_data.ifi_baudrate 
#define        ifr_ipackets     ifr_data.ifi_ipackets 
#define        ifr_ierrors      ifr_data.ifi_ierrors 
#define        ifr_opackets     ifr_data.ifi_opackets 
#define        ifr_oerrors      ifr_data.ifi_oerrors 
#define        ifr_collisions   ifr_data.ifi_collisions 
#define        ifr_ibytes       ifr_data.ifi_ibytes 
#define        ifr_obytes       ifr_data.ifi_obytes 
#define        ifr_imcasts      ifr_data.ifi_imcasts 
#define        ifr_omcasts      ifr_data.ifi_omcasts 
#define        ifr_noproto      ifr_data.ifi_noproto 
#define        ifr_lastchange   ifr_data.ifi_lastchange 
} ifreq_t;


typedef struct ifmediareq {
	char ifm_name[IFNAMSIZ];
	uint32 ifm_current;
	uint32 ifm_mask;
	uint32 ifm_status;
	uint32 ifm_active;
	uint32 ifm_count;
	uint32 *ifm_ulist; 
} ifmediareq_t;

/*
 *	ifconf
 */

typedef struct ifconf {
	int ifc_len;
	union {
		void 		*ifcu_buf;
		ifreq_t		*ifcu_req;
		int32		ifcu_val;
	} ifc_ifcu;
#define ifc_buf ifc_ifcu.ifcu_buf
#define ifc_req ifc_ifcu.ifcu_req
#define ifc_val ifc_ifcu.ifcu_val
} ifconf_t;

//one of these will be passed as the 'data' value to
//bone_datalink_info::control(), when the 'command' value is SIOCAIFADDR
//XXX i don't belong here
typedef struct
{
	const char *name;
	const char *driver;
	int flavor;
} bone_lognet_params_t;


/* "Interface Identification", ala rfc2553 */
struct if_nameindex {
	if_index_t		if_index;
	char		   	*if_name;
};

if_index_t			 if_nametoindex(const char *ifname);
char 				*if_indextoname(if_index_t index, char *ifname);
struct if_nameindex	*if_nameindex(void);
void 				 if_freenameindex(struct if_nameindex *ptr);





#include <net/if_arp.h> 



#endif /* !_NET_IF_H_ */
