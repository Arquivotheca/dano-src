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

    case _CS_XBS5_ILP32_OFFBIG_CFLAGS:
    case _CS_LFS_CFLAGS:
#if defined _XBS5_ILP32_OFF32 && !defined _XBS5_ILP32_OFFBIG
      /* Signal that we want the new ABI.  */
      {
	static const char file_offset[] = "-D_FILE_OFFSET_BITS=64";
	string = file_offset;
	string_len = sizeof (file_offset);
      }
      break;
#endif

    case _CS_LFS_LINTFLAGS:
    case _CS_LFS_LDFLAGS:
    case _CS_LFS_LIBS:
    case _CS_LFS64_CFLAGS:
    case _CS_LFS64_LINTFLAGS:
    case _CS_LFS64_LDFLAGS:
    case _CS_LFS64_LIBS:

    case _CS_XBS5_ILP32_OFF32_CFLAGS:
    case _CS_XBS5_ILP32_OFF32_LDFLAGS:
    case _CS_XBS5_ILP32_OFF32_LIBS:
    case _CS_XBS5_ILP32_OFF32_LINTFLAGS:
    case _CS_XBS5_ILP32_OFFBIG_LDFLAGS:
    case _CS_XBS5_ILP32_OFFBIG_LIBS:
    case _CS_XBS5_ILP32_OFFBIG_LINTFLAGS:
    case _CS_XBS5_LP64_OFF64_CFLAGS:
    case _CS_XBS5_LP64_OFF64_LDFLAGS:
    case _CS_XBS5_LP64_OFF64_LIBS:
    case _CS_XBS5_LP64_OFF64_LINTFLAGS:
    case _CS_XBS5_LPBIG_OFFBIG_CFLAGS:
    case _CS_XBS5_LPBIG_OFFBIG_LDFLAGS:
    case _CS_XBS5_LPBIG_OFFBIG_LIBS:
    case _CS_XBS5_LPBIG_OFFBIG_LINTFLAGS:
      /* GNU libc does not require special actions to use LFS functions.  */
      string = "";
      string_len = 1;
      break;

    default:
      __set_errno (EINVAL);
      return 0;
    }

  if (buf != NULL)
    (void) strncpy (buf, string, len);
  return string_len;
}
