/* Copyright (C) 1997, 1998 Free Software Foundation, Inc.
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

#include <errno.h>
#include <signal.h>
#include <unistd.h>

extern int __syscall_sigpending (sigset_t *);
extern int __syscall_rt_sigpending (sigset_t *, size_t);


/* The variable is shared between all wrappers around signal handling
   functions which have RT equivalents.  */
int __libc_missing_rt_sigs;


/* Change the set of blocked signals to SET,
   wait until a signal arrives, and restore the set of blocked signals.  */
int
sigpending (set)
     sigset_t *set;
{
  /* First try the RT signals.  */
  if (!__libc_missing_rt_sigs)
    {
      /* XXX The size argument hopefully will have to be changed to the
	 real size of the user-level sigset_t.  */
      int saved_errno = errno;
      int result = __syscall_rt_sigpending (set, _NSIG / 8);

      if (result >= 0 || errno != ENOSYS)
	return result;

      __set_errno (saved_errno);
      __libc_missing_rt_sigs = 1;
    }

  return __syscall_sigpending (set);
}
