/* Copyright (C) 1992, 1996 Free Software Foundation, Inc.
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

#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

/* Sleep USECONDS microseconds, or until a previously set timer goes off.  */
void
usleep (useconds)
     unsigned int useconds;
{
  struct timeval delay;

  delay.tv_sec = 0;
  delay.tv_usec = useconds;

  (void) __select (0, (fd_set *) NULL, (fd_set *) NULL, (fd_set *) NULL,
		   &delay);
}
