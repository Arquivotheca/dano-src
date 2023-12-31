/* Copyright (C) 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1996.

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
#include <sys/time.h>
#include <sys/select.h>

/* Check the first NFDS descriptors each in READFDS (if not NULL) for read
   readiness, in WRITEFDS (if not NULL) for write readiness, and in EXCEPTFDS
   (if not NULL) for exceptional conditions.  If TIMEOUT is not NULL, time out
   after waiting the interval specified therein.  Returns the number of ready
   descriptors, or -1 for errors.  */
int
__pselect (nfds, readfds, writefds, exceptfds, timeout)
     int nfds;
     fd_set *readfds;
     fd_set *writefds;
     fd_set *exceptfds;
     struct timespec *timeout;
{
  struct timeval tval;
  int retval;

  /* Change nanosecond number to microseconds.  This may loose
     precision and therefore the `pselect` should be available.  But
     for now it is hardly found.  */
  if (timeout != NULL)
    TIMESPEC_TO_TIMEVAL (&tval, timeout);

  retval = __select (nfds, readfds, writefds, exceptfds,
		     timeout != NULL ? &tval : NULL);

  /* Change the result back.  The remaining time must be made
     available to the caller.  */
  if (timeout != NULL)
    TIMEVAL_TO_TIMESPEC (&tval, timeout);

  return retval;
}
weak_alias (__pselect, pselect)
