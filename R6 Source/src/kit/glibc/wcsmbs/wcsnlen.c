/* Copyright (C) 1998 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1998.

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

#include <wchar.h>


/* Copy SRC to DEST.  */
size_t
__wcsnlen (s, maxlen)
     const wchar_t *s;
     size_t maxlen;
{
  size_t len = 0;

  while (s[len] != L'\0' && maxlen > 0)
    {
      if (s[++len] == L'\0' || --maxlen == 0)
	return len;
      if (s[++len] == L'\0' || --maxlen == 0)
	return len;
      if (s[++len] == L'\0' || --maxlen == 0)
	return len;
      ++len;
    }

  return len;
}
weak_alias (__wcsnlen, wcsnlen)
