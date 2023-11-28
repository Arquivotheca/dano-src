/* Copyright (C) 1991, 1996, 1997 Free Software Foundation, Inc.
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

#include <stddef.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <confstr.h>

/* If BUF is not NULL, fill in at most LEN characters of BUF
   with the value corresponding to NAME.  Return the number
   of characters required to hold NAME's entire value.  */
size_t
confstr (name, buf, len)
     int name;
     char *buf;
     size_t len;
{
  const char *string;
  size_t string_len;

  switch (name)
    {
    case _CS_PATH:
      {
	static const char cs_path[] = CS_PATH;
	string = cs_path;
	string_len = sizeof (cs_path);
      }
      break;

    default:
      __set_errno (EINVAL);
      return 0;
    }

  if (buf != NULL)
    (void) strncpy (buf, string, len);
  return string_len;
}
