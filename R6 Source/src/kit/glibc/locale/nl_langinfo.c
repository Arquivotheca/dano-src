/* User interface for extracting locale-dependent parameters.
   Copyright (C) 1995, 1996, 1997 Free Software Foundation, Inc.
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

#include <langinfo.h>
#include <errno.h>
#include <stddef.h>
#include "localeinfo.h"

/* This array duplicates `_nl_current' defined in setlocale.c; but since
   the references here are not weak references, this guarantees that the
   data for all the categories will be linked in.  */

static struct locale_data * *const nldata[] =
{
#define DEFINE_CATEGORY(category, category_name, items, a, b, c, d) \
  [category] = &_nl_current_##category,
#include "categories.def"
#undef	DEFINE_CATEGORY
};


/* Return a string with the data for locale-dependent parameter ITEM.  */

char *
nl_langinfo (item)
     nl_item item;
{
  int category = _NL_ITEM_CATEGORY (item);
  unsigned int index = _NL_ITEM_INDEX (item);
  const struct locale_data *data;

  if (category < 0 || category >= LC_ALL)
    {
      /* Bogus category: bogus item.  */
      __set_errno (EINVAL);
      return NULL;
    }

  data = *nldata[category];

  if (index >= data->nstrings)
    {
      /* Bogus index for this category: bogus item.  */
      __set_errno (EINVAL);
      return NULL;
    }

  /* Return the string for the specified item.  */
  return (char *) data->values[index].string;
}
