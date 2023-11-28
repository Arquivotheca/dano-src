/*
	sockio.h
	
	ioctl defs for sockets
	
	Copyright 1999, Be Incorporated, All Rights Reserved.
*/

 

#ifndef	H_SYS_SOCKIO
#define	H_SYS_SOCKIO

#include <bone_ioctl.h>

/*
 * Socket datalink ioctls.  Where possible I've tried to preserve some BSD compatibility.
 */ 

typedef enum {
        SIOCADDRT=BONE_SOCKIO_IOCTL_BASE,	/* add route */ 
		SIOCDELRT,				/* delete route */ 
        SIOCSIFADDR,			/* set ifnet_t address */ 
        SIOCGIFADDR,			/* get ifnet_t address */ 
        SIOCSIFDSTADDR,			/* set point-to-point address */ 
        SIOCGIFDSTADDR,			/* get point-to-point address */ 
        SIOCSIFFLAGS,			/* set ifnet_t flags */ 
        SIOCGIFFLAGS,			/* get ifnet_t flags */ 
        SIOCGIFBRDADDR,			/* get broadcast address */ 
        SIOCSIFBRDADDR,			/* set broadcast address */
		SIOCGIFCOUNT,			/* count interfaces */
        SIOCGIFCONF,			/* get ifnet_t list */ 
		SIOCGIFINDEX,			/* name -> if_index map */
		SIOCGIFNAME,			/* if_index -> name map */
        SIOCGIFNETMASK,			/* get net address mask */ 
        SIOCSIFNETMASK,			/* set net address mask */ 
        SIOCGIFMETRIC,			/* get interface metric */ 
        SIOCSIFMETRIC,			/* set interface metric */ 
        SIOCDIFADDR,			/* delete interface address */ 
        SIOCAIFADDR,			/* configure interface alias */ 
        SIOCADDMULTI,			/* add multicast address */ 
        SIOCDELMULTI,			/* del multicast address */ 
        SIOCGIFMTU,				/* get interface mtu */ 
        SIOCSIFMTU,				/* set interface mtu */ 
        SIOCSIFMEDIA,			/* set net media */ 
        SIOCGIFMEDIA,			/* get net media */
		SIOCGRTSIZE,			/* get num of routes */
		SIOCGRTTABLE,			/* get route table */
		SIOCGIFTRANSPORTID,			/* get port_id for ppp, vampire iface */
		SIOCSIFTRANSPORTID,			/* get port_id for ppp, vampire iface */
		SIOCGIFRECVPORTID,			/* get port_id for ppp, vampire iface */
		SIOCSIFRECVPORTID,			/* get port_id for ppp, vampire iface */
		SIOCADDIFACE,			/* Add an interface */
		SIOCSPACKETCAP,			/* Start capturing packets on an interface */
		SIOCCPACKETCAP,			/* Stop capturing packets on an interface */
		
		SIOCSHIWAT,				/* set high watermark */
		SIOCGHIWAT,				/* get high watermark */
		SIOCSLOWAT,				/* set low watermark */
		SIOCGLOWAT,				/* get low watermark */
		SIOCATMARK,				/* at oob mark? */
		SIOCSPGRP,				/* set process group */
		SIOCGPGRP				/* get process group */
} bone_socket_ioctls;

#endif
