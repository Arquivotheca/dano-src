/* Copyright (C) 1997, 1998 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1997.

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
#include <unistd.h>
#include <sys/syscall.h>

#ifdef __NR_pread

extern ssize_t __syscall_pread64 (int fd, void *buf, size_t count,
				  off_t offset_hi, off_t offset_lo);

static ssize_t __emulate_pread64 (int fd, void *buf, size_t count,
				  off64_t offset) internal_function;


ssize_t
__pread64 (fd, buf, count, offset)
     int fd;
     void *buf;
     size_t count;
     off64_t offset;
{
  ssize_t result;

  /* First try the syscall.  */
  result = __syscall_pread64 (fd, buf, count, (off_t) (offset >> 32),
			      (off_t) (offset & 0xffffffff));
  if (result == -1 && errno == ENOSYS)
    /* No system call available.  Use the emulation.  */
    result = __emulate_pread64 (fd, buf, count, offset);

  return result;
}

weak_alias (__pread64, pread64)

#define __pread64(fd, buf, count, offset) \
     static internal_function __emulate_pread64 (fd, buf, count, offset)
#endif
#include <sysdeps/posix/pread64.c>
