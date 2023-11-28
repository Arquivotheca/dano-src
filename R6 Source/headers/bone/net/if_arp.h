/*
	Modified for BeOS
	
	Copyright 1999, Be Incorporated, All Rights Reserved.
*/

/* 
 * Copyright (c) 1986, 1993 
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
 *      @(#)if_arp.h    8.1 (Berkeley) 6/10/93 
 * $Id: if_arp.h,v 1.6 1996/10/12 19:49:23 bde Exp $ 
 */ 

#ifndef _NET_IF_ARP_H_ 
#define        _NET_IF_ARP_H_ 

/* 
 * Address Resolution Protocol. 
 * 
 * See RFC 826 for protocol description.  ARP packets are variable 
 * in size; the arphdr structure defines the fixed-length portion. 
 * Protocol type values are the same as those for 10 Mb/s Ethernet. 
 * It is followed by the variable-sized fields ar_sha, arp_spa, 
 * arp_tha and arp_tpa in that order, according to the lengths 
 * specified.  Field names used correspond to RFC 826. 
 */ 
typedef struct        arphdr { 
        uint16 ar_hrd;         /* format of hardware address */ 
#define ARPHRD_ETHER   (uint16) 1       /* ethernet hardware format */ 
#define ARPHRD_FRELAY  (uint16) 15      /* frame relay hardware format */ 
        uint16 ar_pro;         /* format of protocol address */ 
        uint8  ar_hln;         /* length of hardware address */ 
        uint8  ar_pln;         /* length of protocol address */ 
        uint16 ar_op;          /* one of: */ 
#define        ARPOP_REQUEST    (uint16) 1       /* request to resolve address */ 
#define        ARPOP_REPLY      (uint16) 2       /* response to previous request */ 
#define        ARPOP_REVREQUEST (uint16) 3      /* request protocol address given hardware */ 
#define        ARPOP_REVREPLY   (uint16) 4       /* response giving protocol address */ 
#define ARPOP_INVREQUEST        (uint16) 8     /* request to identify peer */ 
#define ARPOP_INVREPLY          (uint16) 9       /* response identifying peer */ 
/* 
 * The remaining fields are variable in size, 
 * according to the sizes above. 
 */ 
#ifdef COMMENT_ONLY 
        uint8  ar_sha[];       /* sender hardware address */ 
        uint8  ar_spa[];       /* sender protocol address */ 
        uint8  ar_tha[];       /* target hardware address */ 
        uint8  ar_tpa[];       /* target protocol address */ 
#endif 
} arphdr_t; 

/* 
 * ARP ioctl request 
 */ 
struct arpreq { 
        struct        sockaddr arp_pa;                /* protocol address */ 
        struct        sockaddr arp_ha;                /* hardware address */ 
        uint32        arp_flags;                      /* flags */ 
}; 
/*  arp_flags and at_flags field values */ 
#define        ATF_INUSE       0x01    /* entry in use */ 
#define ATF_COM                0x02    /* completed entry (enaddr valid) */ 
#define        ATF_PERM        0x04    /* permanent entry */ 
#define        ATF_PUBL        0x08    /* publish entry (respond for other host) */ 
#define        ATF_USETRAILERS 0x10    /* has requested trailers */ 

#endif /* !_NET_IF_ARP_H_ */ 
