/* Copyright (C) 1996, 1997, 1998 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@gnu.ai.mit.edu>, 1996.

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

#include <gconv.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <wcsmbsload.h>


int
wctob (c)
     wint_t c;
{
  char buf[MB_LEN_MAX];
  struct gconv_step_data data;
  wchar_t inbuf[1];
  wchar_t *inptr = inbuf;
  size_t dummy;
  int status;

  /* Tell where we want the result.  */
  data.outbuf = buf;
  data.outbufend = buf + MB_LEN_MAX;
  data.invocation_counter = 0;
  data.internal_use = 1;
  data.is_last = 1;
  data.statep = &data.__state;

  /* Make sure we start in the initial state.  */
  memset (&data.__state, '\0', sizeof (mbstate_t));

  /* Make sure we use the correct function.  */
  update_conversion_ptrs ();

  /* Create the input string.  */
  inbuf[0] = c;

  status = (*__wcsmbs_gconv_fcts.tomb->fct) (__wcsmbs_gconv_fcts.tomb, &data,
					     (const char **) &inptr,
					     (const char *) &inbuf[1],
					     &dummy, 0);
  /* The conversion failed or the output is too long.  */
  if ((status != GCONV_OK && status != GCONV_FULL_OUTPUT
       && status != GCONV_EMPTY_INPUT)
      || data.outbuf != buf + 1)
    return EOF;

  return buf[0];
}
