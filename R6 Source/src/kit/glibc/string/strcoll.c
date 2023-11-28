/* Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.
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

#include <endian.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef STRING_TYPE
# define STRING_TYPE char
# define USTRING_TYPE unsigned char
# define STRCOLL strcoll
# define STRCMP strcmp
#endif


/* Compare S1 and S2, returning less than, equal to or
   greater than zero if the collated form of S1 is lexicographically
   less than, equal to or greater than the collated form of S2.  */
int
STRCOLL (s1, s2)
     const STRING_TYPE *s1;
     const STRING_TYPE *s2;
{
  return STRCMP (s1, s2);
}
