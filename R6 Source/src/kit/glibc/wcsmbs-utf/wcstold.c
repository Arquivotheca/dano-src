/* Copyright (C) 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@gnu.ai.mit.edu>, 1996.

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


/* The actual implementation for all floating point sizes is in strtod.c.
   These macros tell it to produce the `long double' version, `wcstold'.  */

#define	FLOAT		long double
#define	FLT		LDBL
#define	STRTOF		wcstold
#define	MPN2FLOAT	__mpn_construct_long_double
#define	FLOAT_HUGE_VAL	HUGE_VALL
#define	USE_WIDE_CHAR	1
#define SET_MANTISSA(flt, mant) \
  do { union ieee854_long_double u;                                           \
       u.d = (flt);                                                           \
       if ((mant & 0x7fffffffffffffffULL) == 0)                               \
         mant = 0x4000000000000000ULL;                                        \
       u.ieee.mantissa0 = (((mant) >> 32) & 0x7fffffff) | 0x80000000;         \
       u.ieee.mantissa1 = (mant) & 0xffffffff;                                \
       (flt) = u.d;                                                           \
  } while (0)

#include "../stdlib/strtod.c"
