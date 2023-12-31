/* Copyright (C) 1995, 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Written by Ulrich Drepper <drepper@gnu.ai.mit.edu>, 1995.

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
#include <stdlib.h>
#include <string.h>

#ifndef WIDE_VERSION
# define STRING_TYPE char
# define USTRING_TYPE unsigned char
# define L_(Ch) Ch
# define STRXFRM strxfrm
# define STRLEN strlen
# define STPNCPY strncpy
#endif


/* Transform SRC into a form such that the result of strcmp
   on two strings that have been transformed by strxfrm is
   the same as the result of strcoll on the two strings before
   their transformation.  The transformed string is put in at
   most N characters of DEST and its length is returned.  */
size_t
STRXFRM (STRING_TYPE *dest, const STRING_TYPE *src, size_t n)
{
  if (n != 0)
    STPNCPY (dest, src, n);

  return STRLEN (src);
}
