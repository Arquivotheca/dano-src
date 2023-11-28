/* Copyright (C) 1997 Free Software Foundation, Inc.
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

#include <errno.h>
#define __need_NULL
#include <stddef.h>

unsigned int
if_nametoindex (const char *ifname)
{
  __set_errno (ENOSYS);
  return 0;
}
stub_warning (if_nametoindex)

char *
if_indextoname (unsigned int ifindex, char *ifname)
{
  __set_errno (ENOSYS);
  return NULL;
}
stub_warning (if_indextoname)

void
if_freenameindex (struct if_nameindex *ifn)
{
}
stub_warning (if_freenameindex)

struct if_nameindex *
if_nameindex (void)
{
  __set_errno (ENOSYS);
  return NULL;
}
stub_warning (if_nameindex)
#include <stub-tag.h>
