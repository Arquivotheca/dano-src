/* Copyright (C) 1991, 92, 95, 96, 97, 98 Free Software Foundation, Inc.
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

#include <stdlib.h>
#include <wchar.h>
#include <gconv.h>
#include <wcsmbs/wcsmbsload.h>


/* Common state for all non-restartable conversion functions.  */
mbstate_t __no_r_state;

/* Convert the multibyte character at S, which is no longer
   than N characters, to its `wchar_t' representation, placing
   this n *PWC and returning its length.

   Attention: this function should NEVER be intentionally used.
   The interface is completely stupid.  The state is shared between
   all conversion functions.  You should use instead the restartable
   version `mbrtowc'.  */
int
mbtowc (wchar_t *pwc, const char *s, size_t n)
{
  int result;

  /* If S is NULL the function has to return null or not null
     depending on the encoding having a state depending encoding or
     not.  */
  if (s == NULL)
    {
      /* Make sure we use the correct value.  */
      update_conversion_ptrs ();

      result = __wcsmbs_gconv_fcts.towc->stateful;
    }
  else if (*s == '\0')
    {
      if (pwc != NULL)
	*pwc = L'\0';
      result = 0;
    }
  else
    {
      result = __mbrtowc (pwc, s, n, &__no_r_state);

      /* The `mbrtowc' functions tell us more than we need.  Fold the -1
	 and -2 result into -1.  */
      if (result < 0)
	result = -1;
    }

  return result;
}
