/* Convert `time_t' to `struct tm' in local time zone.
   Copyright (C) 1991, 92, 93, 95, 96, 97 Free Software Foundation, Inc.
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

#include <time.h>

/* The C Standard says that localtime and gmtime return the same pointer.  */
struct tm _tmbuf;

/* Prototype for the internal function to get information based on TZ.  */
extern struct tm *__tz_convert __P ((const time_t *t, int use_localtime,
				     struct tm *tp));



/* Return the `struct tm' representation of *T in local time,
   using *TP to store the result.  */
struct tm *
__localtime_r (t, tp)
     const time_t *t;
     struct tm *tp;
{
  return __tz_convert (t, 1, tp);
}
weak_alias (__localtime_r, localtime_r)


/* Return the `struct tm' representation of *T in local time.  */
struct tm *
localtime (t)
     const time_t *t;
{
  return __tz_convert (t, 1, &_tmbuf);
}
