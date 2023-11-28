/* System-specific socket constants and types.  BeOS version.
   Copyright (C) 1991, 92, 94, 95, 96, 97, 98 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef __BITS_SOCKET_H
#define __BITS_SOCKET_H	1

#if !defined _SYS_SOCKET_H && !defined _NETINET_IN_H
# error "Never include <bits/socket.h> directly; use <sys/socket.h> instead."
#endif

#define	__need_size_t
#include <stddef.h>

/* Type for length arguments in socket calls.  */
typedef unsigned int socklen_t;


/* Types of sockets.  */
enum __socket_type
{
  SOCK_DGRAM = 1,		/* Connectionless, unreliable datagrams
				   of fixed maximum length.  */
#define SOCK_DGRAM SOCK_DGRAM
  SOCK_STREAM = 2		/* Sequenced, reliable, connection-based
				   byte streams.  */
#define SOCK_STREAM SOCK_STREAM
};

/* Protocol families.  */
#define PF_UNSPEC	0
#define	PF_INET		1	/* IP protocol family.  */
#define	PF_MAX		2

/* Address families.  */
#define AF_UNSPEC	PF_UNSPEC
#define	AF_INET		PF_INET
#define	AF_MAX		PF_MAX


/* Get the definition of the macro to define the common sockaddr members.  */
#include <bits/sockaddr.h>

/* Structure describing a generic socket address.  */
struct sockaddr
  {
    __SOCKADDR_COMMON (sa_);	/* Common data: address family and length.  */
    char sa_data[10];		/* Address data.  */
  };


/* Bits in the FLAGS argument to `send', `recv', et al.  */
enum
  {
    MSG_OOB		= 0x01,	/* Process out-of-band data.  */
#define MSG_OOB MSG_OOB
  };


/* Protocol number used to manipulate socket-level options
   with `getsockopt' and `setsockopt'.  */
#define	SOL_SOCKET	1

/* Socket-level options for `getsockopt' and `setsockopt'.  */
enum
  {
    SO_DEBUG = 1,		/* Record debugging information.  */
#define SO_DEBUG SO_DEBUG
    SO_REUSEPORT = 2,		/* Allow local address and port reuse.  */
#define SO_REUSEADDR SO_REUSEADDR
    SO_NONBLOCK = 3
#define SO_NONBLOCK SO_NONBLOCK
  };

#endif	/* bits/socket.h */
