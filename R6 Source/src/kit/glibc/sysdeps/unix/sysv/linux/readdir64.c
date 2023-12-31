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
#include <limits.h>
#include <stddef.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <assert.h>

#include <dirstream.h>

extern ssize_t __getdirentries64 (int, char *, size_t, off_t *);


/* Read a directory entry from DIRP.  */
struct dirent64 *
__readdir64 (DIR *dirp)
{
  struct dirent64 *dp;

  __libc_lock_lock (dirp->lock);

  do
    {
      size_t reclen;

      if (dirp->offset >= dirp->size)
	{
	  /* We've emptied out our buffer.  Refill it.  */

	  size_t maxread;
	  off_t base;
	  ssize_t bytes;

#ifndef _DIRENT_HAVE_D_RECLEN
	  /* Fixed-size struct; must read one at a time (see below).  */
	  maxread = sizeof *dp;
#else
	  maxread = dirp->allocation;
#endif

	  base = dirp->filepos;
	  bytes = __getdirentries64 (dirp->fd, dirp->data, maxread, &base);
	  if (bytes <= 0)
	    {
	      dp = NULL;
	      break;
	    }
	  dirp->size = (size_t) bytes;

	  /* Reset the offset into the buffer.  */
	  dirp->offset = 0;
	}

      dp = (struct dirent64 *) &dirp->data[dirp->offset];

#ifdef _DIRENT_HAVE_D_RECLEN
      reclen = dp->d_reclen;
#else
      /* The only version of `struct dirent64' that lacks `d_reclen'
	 is fixed-size.  */
      assert (sizeof dp->d_name > 1);
      reclen = sizeof *dp;
      /* The name is not terminated if it is the largest possible size.
	 Clobber the following byte to ensure proper null termination.  We
	 read jst one entry at a time above so we know that byte will not
	 be used later.  */
      dp->d_name[sizeof dp->d_name] = '\0';
#endif

      dirp->offset += reclen;

#ifdef _DIRENT_HAVE_D_OFF
      dirp->filepos = dp->d_off;
#else
      dirp->filepos += reclen;
#endif

      /* Skip deleted files.  */
    } while (dp->d_ino == 0);

  __libc_lock_unlock (dirp->lock);

  return dp;
}
weak_alias (__readdir64, readdir64)
