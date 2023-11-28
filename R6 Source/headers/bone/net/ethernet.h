/*
	Modified for BeOS
	
	Copyright 1999, Be Incorporated, All Rights Reserved.
*/


/* 
 * Fundamental constants relating to ethernet. 
 * 
 * $Id: ethernet.h,v 1.2 1996/08/06 21:14:21 phk Exp $ 
 * 
 */ 

#ifndef _NET_ETHERNET_H_ 
#define _NET_ETHERNET_H_ 

/* 
 * The number of bytes in an ethernet (MAC) address. 
 */ 
#define        ETHER_ADDR_LEN          6 

/* 
 * The number of bytes in the type field. 
 */ 
#define        ETHER_TYPE_LEN          2 

/* 
 * The number of bytes in the trailing CRC field. 
 */ 
#define        ETHER_CRC_LEN           4 

/* 
 * The length of the combined header. 
 */ 
#define        ETHER_HDR_LEN           (ETHER_ADDR_LEN*2+ETHER_TYPE_LEN) 

/* 
 * The minimum packet length. 
 */ 
#define        ETHER_MIN_LEN           64 

/* 
 * The maximum packet length. 
 */ 
#define        ETHER_MAX_LEN           1518 

/* 
 * The maximum packet length minus the trailer
 */ 
#define        ETHER_MAX_LEN_NOTRAILER       1514


/*
 * the maximum length of the data itself
 */
#define 	   ETHER_MAX_DATA_LEN      1500

/* 
 * A macro to validate a length with 
 */ 
#define        ETHER_IS_VALID_LEN(foo) ((foo) >= ETHER_MIN_LEN && (foo) <= ETHER_MAX_LEN) 

/* 
 * Structure of a 10Mb/s Ethernet header. 
 */ 
typedef struct        ether_header { 
        uint8  ether_dhost[ETHER_ADDR_LEN]; 
        uint8  ether_shost[ETHER_ADDR_LEN]; 
        uint16 ether_type; 
} ether_header_t; 

/* 
 * Structure of a 48-bit Ethernet address. 
 */ 
typedef struct        ether_addr { 
        uint8 octet[ETHER_ADDR_LEN]; 
} ether_addr_t; 

#endif 


