/* Routines for dealing with '\0' separated arg vectors.
   Copyright (C) 1995, 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Written by Miles Bader <miles@gnu.ai.mit.edu>

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

#include <argz.h>

/* Puts pointers to each string in ARGZ, plus a terminating 0 element, into
   ARGV, which must be large enough to hold them all.  */
void
__argz_extract (char *argz, size_t len, char **argv)
{
  while (len > 0)
    {
      size_t part_len = strlen (argz);
      *argv++ = argz;
      argz += part_len + 1;
      len -= part_len + 1;
    }
  *argv = 0;
}
weak_alias (__argz_extract, argz_extract)
