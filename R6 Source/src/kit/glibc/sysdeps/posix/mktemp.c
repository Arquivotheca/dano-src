/* Copyright (C) 1991, 1992, 1993, 1996, 1998 Free Software Foundation, Inc.
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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

/* Generate a unique temporary file name from TEMPLATE.
   The last six characters of TEMPLATE must be "XXXXXX";
   they are replaced with a string that makes the filename unique.  */
char *
mktemp (template)
     char *template;
{
  static const char letters[]
    = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  static uint64_t value;
  struct timeval tv;
  char *XXXXXX;
  size_t len;
  int count;

  len = strlen (template);
  if (len < 6 || strcmp (&template[len - 6], "XXXXXX"))
    {
      __set_errno (EINVAL);
      return NULL;
    }

  /* This is where the Xs start.  */
  XXXXXX = &template[len - 6];

  /* Get some more or less random data.  */
  __gettimeofday (&tv, NULL);
  value += ((uint64_t) tv.tv_usec << 16) ^ tv.tv_sec ^ getpid ();

  for (count = 0; count < TMP_MAX; ++count)
    {
      struct stat ignored;
      uint64_t v = value;

      /* Fill in the random bits.  */
      XXXXXX[0] = letters[v % 62];
      v /= 62;
      XXXXXX[1] = letters[v % 62];
      v /= 62;
      XXXXXX[2] = letters[v % 62];
      v /= 62;
      XXXXXX[3] = letters[v % 62];
      v /= 62;
      XXXXXX[4] = letters[v % 62];
      v /= 62;
      XXXXXX[5] = letters[v % 62];

      if (stat (template, &ignored) < 0 && errno == ENOENT)
	/* The file does not exist.  So return this name.  */
	return template;

      /* This is a random value.  It is only necessary that the next
	 TMP_MAX values generated by adding 7777 to VALUE are different
	 with (module 2^32).  */
      value += 7777;
    }

  /* We return the null string if we can't find a unique file name.  */
  template[0] = '\0';
  return template;
}
