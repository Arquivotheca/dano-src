/* Copyright (C) 1991, 92, 93, 95, 96, 97, 98 Free Software Foundation, Inc.
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
#include <unistd.h>
#include <string.h>


/* Store at most BUFLEN character of the pathname of the terminal FD is
   open on in BUF.  Return 0 on success,  otherwise an error number.  */
int
__ttyname_r (fd, buf, buflen)
     int fd;
     char *buf;
     size_t buflen;
{
  const char *result = getenv ("TTY");
  size_t len = strlen (result);

  if (buflen < (int) (len + 1))
    {
      __set_errno (ERANGE);
      return ERANGE;
    }

  memcpy (buf, result, len + 1);

  return 0;
}

weak_alias (__ttyname_r, ttyname_r)
