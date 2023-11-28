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

#include <errno.h>
#include <stddef.h>
#include <sys/stat.h>

#include <kernel_stat.h>

extern int __syscall_fstat (int, struct kernel_stat *);

/* Get information about the file descriptor FD in BUF.  */
int
__fxstat (int vers, int fd, struct stat *buf)
{
  struct kernel_stat kbuf;
  int result;

  switch (vers)
    {
    case _STAT_VER_LINUX_OLD:
      /* Nothing to do.  The struct is in the form the kernel expects
	 it to be.  */
      result = __syscall_fstat (fd, (struct kernel_stat *) buf);
      break;

    case _STAT_VER_LINUX:
      /* Do the system call.  */
      result = __syscall_fstat (fd, &kbuf);

      /* Convert to current kernel version of `struct stat'.  */
      buf->st_dev = kbuf.st_dev;
      buf->st_pad1[0]  = 0; buf->st_pad1[1]  = 0; buf->st_pad1[2]  = 0;
      buf->st_ino = kbuf.st_ino;
      buf->st_mode = kbuf.st_mode;
      buf->st_nlink = kbuf.st_nlink;
      buf->st_uid = kbuf.st_uid;
      buf->st_gid = kbuf.st_gid;
      buf->st_rdev = kbuf.st_rdev;
      buf->st_pad2[0] = 0; buf->st_pad2[1] = 0;
      buf->st_pad3 = 0;
      buf->st_size = kbuf.st_size;
      buf->st_blksize = kbuf.st_blksize;
      buf->st_blocks = kbuf.st_blocks;

      buf->st_atime = kbuf.st_atime; buf->__reserved0 = 0;
      buf->st_mtime = kbuf.st_mtime; buf->__reserved1 = 0;
      buf->st_ctime = kbuf.st_ctime; buf->__reserved2 = 0;

      buf->st_pad4[0] = 0; buf->st_pad4[1] = 0;
      buf->st_pad4[2] = 0; buf->st_pad4[3] = 0;
      buf->st_pad4[4] = 0; buf->st_pad4[5] = 0;
      buf->st_pad4[6] = 0; buf->st_pad4[7] = 0;
      break;

    default:
      __set_errno (EINVAL);
      result = -1;
      break;
    }

  return result;
}
weak_alias (__fxstat, _fxstat)
