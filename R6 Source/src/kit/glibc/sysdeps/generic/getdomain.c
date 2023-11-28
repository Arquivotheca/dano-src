/* Copyright (C) 1994, 1995, 1997 Free Software Foundation, Inc.
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

/* Put the name of the current YP domain in no more than LEN bytes of NAME.
   The result is null-terminated if LEN is large enough for the full
   name and the terminator.  */

#include <errno.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <string.h>

#if _UTSNAME_DOMAIN_LENGTH
/* The `uname' information includes the domain name.  */

int
getdomainname (name, len)
    char *name;
    size_t len;
{
  struct utsname u;

  if (uname (&u) < 0)
    return -1;

  strncpy (name, u.domainname, len);
  return 0;
}

#else

int
getdomainname (name, len)
     char *name;
     size_t len;
{
  __set_errno (ENOSYS);
  return -1;
}

stub_warning (getdomainname)
#include <stub-tag.h>

#endif
