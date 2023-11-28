/*
	ip.h
	
	ipv4 protocol header
	
	Copyright 1999, Be Incorporated, All Rights Reserved.
*/

#ifndef H_NETINET_IP
#define H_NETINET_IP

#include <OS.h>
#include <netinet/in.h>
#include <ByteOrder.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IPVERSION       4

/*
 * basic IP header
 */
typedef struct ip
{
#if B_HOST_IS_LENDIAN == 1
    uint8	hdr_length:4;			/* header length, in 4-byte chunks */
    uint8	version:4;				/* version */
#else
    uint8	version:4;                   
    uint8	hdr_length:4;                   
#endif
    uint8	tos;					/* type of service */
    uint16	total_len;				/* total length */
    uint16	id;						/* identification */
    uint16	frag_offset;			/* fragment offset field (see flags below) */
    uint8	ttl;					/* time to live */
    uint8	proto;					/* protocol */
    uint16	checksum;				/* IP checksum */
    struct	in_addr src, dest;		/* source and dest IP address */
} iphdr_t;

/*
 *	IP flags
 */
#define IP_RF 0x8000				/* reserved fragment flag */
#define IP_DF 0x4000				/* don't fragment flag */
#define IP_MF 0x2000				/* more fragments flag */
#define IP_OFFMASK 0x1fff			/* frag_offset field mask for fragmenting bits */

/*
 * Definitions for IP type of service (ip_tos)
 */
#define	IPTOS_LOWDELAY		0x10
#define	IPTOS_THROUGHPUT	0x08
#define	IPTOS_RELIABILITY	0x04

/*
 * Definitions for IP precedence (also in ip_tos) (hopefully unused)
 */
#define	IPTOS_PREC_NETCONTROL		0xe0
#define	IPTOS_PREC_INTERNETCONTROL	0xc0
#define	IPTOS_PREC_CRITIC_ECP		0xa0
#define	IPTOS_PREC_FLASHOVERRIDE	0x80
#define	IPTOS_PREC_FLASH		0x60
#define	IPTOS_PREC_IMMEDIATE		0x40
#define	IPTOS_PREC_PRIORITY		0x20
#define	IPTOS_PREC_ROUTINE		0x00

/*
 * limits
 */
#define IP_MAXPACKET    65535           /* maximum packet size */
#define IP_DATA_MAX (65536 - sizeof(iphdr_t))

#define MAX_IPOPTLEN 40

/*
 * Definitions for options, mostly unimplemented.
 */
#define IPOPT_COPIED(o)         ((o)&0x80)
#define IPOPT_CLASS(o)          ((o)&0x60)
#define IPOPT_NUMBER(o)         ((o)&0x1f)

#define IPOPT_CONTROL           0x00
#define IPOPT_RESERVED1         0x20
#define IPOPT_DEBMEAS           0x40
#define IPOPT_RESERVED2         0x60

#define IPOPT_EOL               0               /* end of option list */
#define IPOPT_NOP               1               /* no operation */

#define IPOPT_RR                7               /* record packet route */
#define IPOPT_TS                68              /* timestamp */
#define IPOPT_SECURITY          130             /* provide s,c,h,tcc */
#define IPOPT_LSRR              131             /* loose source route */
#define IPOPT_SATID             136             /* satnet id */
#define IPOPT_SSRR              137             /* strict source route */
#define IPOPT_RA                148             /* router alert */

/*
 * Offsets to fields in options other than EOL and NOP.
 */
#define IPOPT_OPTVAL            0               /* option ID */
#define IPOPT_OLEN              1               /* option length */
#define IPOPT_OFFSET            2               /* offset within option */
#define IPOPT_MINOFF            4               /* min value of above */

/*
 * Time stamp option structure.
 */
struct  ip_timestamp {
        uint8  ipt_code;               /* IPOPT_TS */
        uint8  ipt_len;                /* size of structure (variable) */
        uint8  ipt_ptr;                /* index of current entry */
#if BYTE_ORDER == LITTLE_ENDIAN
        uint8  ipt_flg:4,              /* flags, see below */
                ipt_oflw:4;             /* overflow counter */
#endif
#if BYTE_ORDER == BIG_ENDIAN
        uint8  ipt_oflw:4,             /* overflow counter */
                ipt_flg:4;              /* flags, see below */
#endif
        union ipt_timestamp {
                int32  ipt_time[1];
                struct  ipt_ta {
                        struct in_addr ipt_addr;
                        int32 ipt_time;
                } ipt_ta[1];
        } ipt_timestamp;
};

/* flag bits for ipt_flg */
#define IPOPT_TS_TSONLY         0               /* timestamps only */
#define IPOPT_TS_TSANDADDR      1               /* timestamps and addresses */
#define IPOPT_TS_PRESPEC        3               /* specified modules only */

/* bits for security (not byte swapped) */
#define IPOPT_SECUR_UNCLASS     0x0000
#define IPOPT_SECUR_CONFID      0xf135
#define IPOPT_SECUR_EFTO        0x789a
#define IPOPT_SECUR_MMMM        0xbc4d
#define IPOPT_SECUR_RESTR       0xaf13
#define IPOPT_SECUR_SECRET      0xd788
#define IPOPT_SECUR_TOPSECRET   0x6bc5


#define IP_MSS          576             /* default maximum segment size */
        
/*
 * BSD compatibility
 */
#define ip_hl hdr_length
#define ip_v version
#define ip_tos tos
#define ip_len total_len
#define ip_id id
#define ip_off frag_offset
#define ip_ttl ttl
#define ip_p proto
#define ip_sum checksum
#define ip_src src
#define ip_dst dest 

#ifdef __cplusplus
}
#endif
 

#endif
