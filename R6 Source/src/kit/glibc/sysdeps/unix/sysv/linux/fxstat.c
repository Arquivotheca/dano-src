/* fxstat using old-style Unix fstat system call.
   Copyright (C) 1991, 1995, 1996, 1997 Free Software Foundation, Inc.
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

/* Ho hum, if xstat == xstat64 we must get rid of the prototype or gcc
   will complain since they don't strictly match.  */
#define __fxstat64 __fxstat64_disable

#include <errno.h>
#include <stddef.h>
#include <sys/stat.h>
#include <kernel_stat.h>

#include <xstatconv.c>

extern int __syscall_fstat (int, struct kernel_stat *);

/* Get information about the file FD in BUF.  */
int
__fxstat (int vers, int fd, struct stat *buf)
{
  struct kernel_stat kbuf;
  int result;

  if (vers == _STAT_VER_KERNEL)
    return __syscall_fstat (fd, (struct kernel_stat *) buf);

  result =  __syscall_fstat (fd, &kbuf);
  if (result == 0)
    result = xstat_conv (vers, &kbuf, buf);

  return result;
}

weak_alias (__fxstat, _fxstat);
#ifdef XSTAT_IS_XSTAT64
#undef __fxstat64
strong_alias (__fxstat, __fxstat64);
#endif
