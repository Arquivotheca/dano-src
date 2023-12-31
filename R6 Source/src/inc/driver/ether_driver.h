/*
 * ether_driver.h
 * Copyright (c) 1995 Be, Inc.	All Rights Reserved 
 *
 * Ethernet driver: handles NE2000 and 3C503 cards
 */
#ifndef _ETHER_DRIVER_H
#define _ETHER_DRIVER_H

#include <Drivers.h>

/*
 * ioctls: belongs in a public header file
 * somewhere, so that the net_server and other ethernet drivers can use.
 */
enum {
	ETHER_GETADDR = B_DEVICE_OP_CODES_END,	/* get ethernet address */
	ETHER_INIT,								/* set irq and port */
	ETHER_NONBLOCK,							/* set/unset nonblocking mode */
	ETHER_ADDMULTI,							/* add multicast addr */
	ETHER_REMMULTI,							/* rem multicast addr */
	ETHER_SETPROMISC,						/* set promiscuous */
	ETHER_GETFRAMESIZE						/* get frame size */
};


/*
 * 48-bit ethernet address, passed back from ETHER_GETADDR
 */
typedef struct {
	unsigned char ebyte[6];
} ether_address_t;


/*
 *	info passed to ETHER_INIT
 */

typedef struct ether_init_params {
	short port;
	short irq;
	unsigned long mem;
} ether_init_params_t;

#endif /* _ETHER_DRIVER_H */
