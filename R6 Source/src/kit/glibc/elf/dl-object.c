/* Storage management for the chain of loaded shared objects.
   Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.
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
#include <string.h>
#include <stdlib.h>
#include <elf/ldsodefs.h>

#include <assert.h>

/* List of objects currently loaded is [2] of this, aka _dl_loaded.  */
struct link_map *_dl_default_scope[5];

/* Allocate a `struct link_map' for a new object being loaded,
   and enter it into the _dl_loaded list.  */

struct link_map *
internal_function
_dl_new_object (char *realname, const char *libname, int type)
{
  struct link_map *new = malloc (sizeof *new);
  struct libname_list *newname = malloc (sizeof *newname);
  if (! new || ! newname)
    return NULL;

  memset (new, 0, sizeof *new);
  new->l_name = realname;
  newname->name = libname;
  newname->next = NULL;
  new->l_libname = newname;
  new->l_type = type;

  if (_dl_loaded == NULL)
    {
      new->l_prev = new->l_next = NULL;
      _dl_loaded = new;
    }
  else
    {
      struct link_map *l = _dl_loaded;
      while (l->l_next)
	l = l->l_next;
      new->l_prev = l;
      new->l_next = NULL;
      l->l_next = new;
    }

  return new;
}
