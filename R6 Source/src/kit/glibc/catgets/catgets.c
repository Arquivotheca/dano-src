/* Copyright (C) 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper, <drepper@gnu.ai.mit.edu>.

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
#include <errno.h>
#include <nl_types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#include "catgetsinfo.h"


/* Open the catalog and return a descriptor for the catalog.  */
nl_catd
catopen (const char *cat_name, int flag)
{
  __nl_catd result;
  const char *env_var;
  const char *nlspath;

  result = (__nl_catd) malloc (sizeof (*result));
  if (result == NULL)
    /* We cannot get enough memory.  */
    return (nl_catd) -1;

  result->status = closed;

  result->cat_name = __strdup (cat_name);
  if (result->cat_name == NULL)
    {
      free (result);
      __set_errno (ENOMEM);
      return (nl_catd) -1;
    }

  if (strchr (cat_name, '/') == NULL)
    {
      if (flag == NL_CAT_LOCALE)
	{
	  env_var = getenv ("LC_ALL");
	  if (env_var == NULL)
	    {
	      env_var = getenv ("LC_MESSAGES");
	      if (env_var == NULL)
		{
		  env_var = getenv ("LANG");
		  if (env_var == NULL)
		    env_var = "C";
		}
	    }
	}
      else
	{
	  env_var = getenv ("LANG");
	  if (env_var == NULL)
	    env_var = "C";
	}

      result->env_var = __strdup (env_var);
      if (result->env_var == NULL)
	{
	  free ((void *) result->cat_name);
	  free ((void *) result);
	  __set_errno (ENOMEM);
	  return (nl_catd) -1;
	}

      nlspath = __secure_getenv ("NLSPATH");
      if (nlspath != NULL && *nlspath != '\0')
	{
	  /* Append the system dependent directory.  */
	  size_t len = strlen (nlspath) + 1 + sizeof NLSPATH;
	  char *tmp = alloca (len);

	  __stpcpy (__stpcpy (__stpcpy (tmp, nlspath), ":"), NLSPATH);
	  nlspath = tmp;
	}
      else
	nlspath = NLSPATH;

      result->nlspath = __strdup (NLSPATH);
      if (result->nlspath == NULL)
	{
	  free ((void *) result->cat_name);
	  free ((void *) result->env_var);
	  free ((void *) result);
	  __set_errno (ENOMEM);
	  return (nl_catd) -1;
	}
    }
  else
    {
      result->env_var = NULL;
      result->nlspath = NULL;
    }

  __libc_lock_init (result->lock);

  return (nl_catd) result;
}


/* Return message from message catalog.  */
char *
catgets (nl_catd catalog_desc, int set, int message, const char *string)
{
  __nl_catd catalog;
  size_t idx;
  size_t cnt;

  /* Be generous if catalog which failed to be open is used.  */
  if (catalog_desc == (nl_catd) -1 || ++set <= 0 || message < 0)
    return (char *) string;

  catalog = (__nl_catd) catalog_desc;

  if (catalog->status == closed)
    __open_catalog (catalog);

  if (catalog->status == nonexisting)
    {
      __set_errno (EBADF);
      return (char *) string;
    }

  idx = ((set * message) % catalog->plane_size) * 3;
  cnt = 0;
  do
    {
      if (catalog->name_ptr[idx + 0] == (u_int32_t) set
	  && catalog->name_ptr[idx + 1] == (u_int32_t) message)
	return (char *) &catalog->strings[catalog->name_ptr[idx + 2]];

      idx += catalog->plane_size * 3;
    }
  while (++cnt < catalog->plane_depth);

  __set_errno (ENOMSG);
  return (char *) string;
}


/* Return resources used for loaded message catalog.  */
int
catclose (nl_catd catalog_desc)
{
  __nl_catd catalog;

  catalog = (__nl_catd) catalog_desc;

  if (catalog->status == mmapped)
    __munmap ((void *) catalog->file_ptr, catalog->file_size);
  else if (catalog->status == malloced)
    free ((void *) catalog->file_ptr);
  else if (catalog->status != closed && catalog->status != nonexisting)
    {
      __set_errno (EBADF);
      return -1;
    }

  if (catalog->nlspath)
    free ((void *) catalog->nlspath);
  if (catalog->env_var)
    free ((void *) catalog->env_var);
  free ((void *) catalog);

  return 0;
}
