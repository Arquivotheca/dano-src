/*
	udp.h
	
	UDP protocol header
	
	Copyright 1999, Be Incorporated, All Rights Reserved.
*/

#ifndef H_NETINET_UDP
#define H_NETINET_UDP

#include <OS.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct udphdr
{
	uint16 src_port;
	uint16 dest_port;
	uint16 length;
	uint16 checksum;
} udphdr_t;


/*
 * BSD compatability
 */
#define uh_sport src_port
#define uh_dport dest_port
#define uh_ulen length
#define uh_sum	checksum

/*
 * constants
 */
#define UDP_PORT_MAX 65535
#define UDP_SIZE_MAX 65515

/*
 * socket options
 */
#define UDP_CHECKSUM	0x00000001		/* calculate UDP checksum */
										/* arg is int, 1 = on, 0 = off, default on */

#ifdef __cplusplus
}
#endif

#endif
