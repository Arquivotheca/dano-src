/* Copyright (C) 1994, 1996, 1997, 1998 Free Software Foundation, Inc.
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

#include <alloca.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/param.h>
#include <unistd.h>

/* Poll the file descriptors described by the NFDS structures starting at
   FDS.  If TIMEOUT is nonzero and not -1, allow TIMEOUT milliseconds for
   an event to occur; if TIMEOUT is -1, block until an event occurs.
   Returns the number of file descriptors with events, zero if timed out,
   or -1 for errors.  */

int
__poll (fds, nfds, timeout)
     struct pollfd *fds;
     unsigned long int nfds;
     int timeout;
{
  static int max_fd_size;
  struct timeval tv;
  fd_set *rset, *wset, *xset;
  struct pollfd *f;
  int ready;
  int maxfd = 0;
  int bytes;

  if (!max_fd_size)
    max_fd_size = __getdtablesize ();

  bytes = howmany (max_fd_size, __NFDBITS);
  rset = alloca (bytes);
  wset = alloca (bytes);
  xset = alloca (bytes);

  /* We can't call FD_ZERO, since FD_ZERO only works with sets
     of exactly __FD_SETSIZE size.  */
  __bzero (rset, bytes);
  __bzero (wset, bytes);
  __bzero (xset, bytes);

  for (f = fds; f < &fds[nfds]; ++f)
    if (f->fd >= 0)
      {
	if (f->events & POLLIN)
	  FD_SET (f->fd, rset);
	if (f->events & POLLOUT)
	  FD_SET (f->fd, wset);
	if (f->events & POLLPRI)
	  FD_SET (f->fd, xset);
	if (f->fd > maxfd && (f->events & (POLLIN|POLLOUT|POLLPRI)))
	  maxfd = f->fd;
      }

  tv.tv_sec = timeout / 1000;
  tv.tv_usec = (timeout % 1000) * 1000;

  ready = __select (maxfd + 1, rset, wset, xset, timeout == -1 ? NULL : &tv);
  if (ready > 0)
    for (f = fds; f < &fds[nfds]; ++f)
      {
	f->revents = 0;
	if (f->fd >= 0)
	  {
	    if (FD_ISSET (f->fd, rset))
	      f->revents |= POLLIN;
	    if (FD_ISSET (f->fd, wset))
	      f->revents |= POLLOUT;
	    if (FD_ISSET (f->fd, xset))
	      f->revents |= POLLPRI;
	  }
      }

  return ready;
}
#ifndef __poll
weak_alias (__poll, poll)
#endif
