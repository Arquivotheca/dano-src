/* Copyright (C) 1991, 92, 93, 94, 95, 96, 98 Free Software Foundation, Inc.
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
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>

#include <dirstream.h>

/* Open a directory stream on NAME.  */
DIR *
__opendir (const char *name)
{
  DIR *dirp;
  struct stat statbuf;
  int fd;
  size_t allocation;
  int save_errno;

  if (name[0] == '\0')
    {
      /* POSIX.1-1990 says an empty name gets ENOENT;
	 but `open' might like it fine.  */
      __set_errno (ENOENT);
      return NULL;
    }

  fd = __open (name, O_RDONLY|O_NDELAY);
  if (fd < 0)
    return NULL;

  if (__fcntl (fd, F_SETFD, FD_CLOEXEC) < 0)
    goto lose;

  if (fstat (fd, &statbuf) < 0)
    goto lose;
  if (! S_ISDIR (statbuf.st_mode))
    {
      save_errno = ENOTDIR;
      goto lose2;
    }

#ifdef _STATBUF_ST_BLKSIZE
  if (statbuf.st_blksize < sizeof (struct dirent))
    allocation = sizeof (struct dirent);
  else
    allocation = statbuf.st_blksize;
#else
  allocation = (BUFSIZ < sizeof (struct dirent)
		? sizeof (struct dirent) : BUFSIZ);
#endif

  dirp = (DIR *) calloc (1, sizeof (DIR) + allocation); /* Zero-fill.  */
  if (dirp == NULL)
  lose:
    {
      save_errno = errno;
    lose2:
      (void) __close (fd);
      __set_errno (save_errno);
      return NULL;
    }
  dirp->data = (char *) (dirp + 1);
  dirp->allocation = allocation;
  dirp->fd = fd;

  __libc_lock_init (dirp->lock);

  return dirp;
}
weak_alias (__opendir, opendir)
