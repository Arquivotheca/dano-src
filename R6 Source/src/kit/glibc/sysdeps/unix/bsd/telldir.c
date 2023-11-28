/* Copyright (C) 1994, 1995, 1996, 1997 Free Software Foundation, Inc.
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
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include "dirstream.h"

/* Internal data structure for telldir and seekdir.  */
struct record
  {
    struct record *next; /* Link in chain.  */
    off_t cookie;		/* Value returned by `telldir'.  */
    off_t pos;
    size_t offset;
  };
#define NBUCKETS 32
static struct record *records[32];
static off_t lastpos;
__libc_lock_define_initialized(static, lock) /* Locks above data.  */


/* Return the current position of DIRP.  */
off_t
telldir (dirp)
     DIR *dirp;
{
  struct record *new;
  off_t pos;

  __libc_lock_lock (lock);

  new = malloc (sizeof *new);
  if (new == NULL)
    return (off_t) -1;

  new->pos = dirp->filepos;
  new->offset = dirp->offset;
  new->cookie = ++lastpos;
  new->next = records[new->cookie % NBUCKETS];
  records[new->cookie % NBUCKETS] = new;

  pos = new->cookie;

  __libc_lock_unlock (lock);

  return pos;
}



/* Seek to position POS in DIRP.  */
void
seekdir (dirp, pos)
     DIR *dirp;
     __off_t pos;
{
  struct record *r, **prevr;

  __libc_lock_lock (lock);

  for (prevr = &records[pos % NBUCKETS], r = *prevr;
       r != NULL;
       prevr = &r->next, r = r->next)
    if (r->cookie == pos)
      {
	__libc_lock_lock (dirp->__lock);
	if (dirp->filepos != r->pos || dirp->offset != r->offset)
	  {
	    dirp->size = 0;	/* Must read a fresh buffer.  */
	    /* Move to the saved position.  */
	    __lseek (dirp->fd, r->pos, SEEK_SET);
	    dirp->filepos = r->pos;
	    dirp->offset = 0;
	    /* Read entries until we reach the saved offset.  */
	    while (dirp->offset < r->offset)
	      {
		struct dirent *scan;
		__libc_lock_unlock (dirp->__lock);
		scan = readdir (dirp);
		__libc_lock_lock (dirp->__lock);
		if (! scan)
		  break;
	      }
	  }
	__libc_lock_unlock (dirp->__lock);

	/* To prevent leaking memory, cookies returned from telldir
	   can only be used once.  So free this one's record now.  */
	*prevr = r->next;
	free (r);
	break;
      }

  __libc_lock_unlock (lock);

  /* If we lost there is no way to indicate it.  Oh well.  */
}
