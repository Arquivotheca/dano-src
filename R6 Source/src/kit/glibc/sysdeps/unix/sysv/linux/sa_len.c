/* Copyright (C) 1998 Free Software Foundation, Inc.
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

#include <sys/socket.h>

#include <netatalk/at.h>
#include <netax25/ax25.h>
#include <netinet/in.h>
#include <netipx/ipx.h>
#include <netrose/rose.h>

int
__libc_sa_len (sa_family_t af)
{
  switch (af)
    {
    case AF_APPLETALK:
      return sizeof (struct sockaddr_at);
    case AF_AX25:
      return sizeof (struct sockaddr_ax25);
    case AF_INET:
      return sizeof (struct sockaddr_in);
    case AF_INET6:
      return sizeof (struct sockaddr_in6);
    case AF_IPX:
      return sizeof (struct sockaddr_ipx);
    case AF_ROSE:
      return sizeof (struct sockaddr_rose);
    }
  return 0;
}
