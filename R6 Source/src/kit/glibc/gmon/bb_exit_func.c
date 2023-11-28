/* Copyright (C) 1996, 1997, 1998 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by David Mosberger (davidm@cs.arizona.edu).

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

/* __bb_exit_func() dumps all the basic-block statistics linked into
   the __bb_head chain to .d files.  */

#include <sys/gmon_out.h>
#include <sys/types.h>

#include <stdio.h>
#include <string.h>

/* structure emitted by -a */
struct bb
  {
    long int zero_word;
    const char *filename;
    long int *counts;
    long int ncounts;
    struct bb *next;
    const unsigned long	int *addresses;
  };

extern struct bb *__bb_head;	/* from gmon.c */

#define OUT_NAME	"gmon.out"


void
__bb_exit_func (void)
{
  const int version = GMON_VERSION;
  struct gmon_hdr ghdr;
  struct bb *ptr;
  FILE *fp;
  fp = fopen (OUT_NAME, "wb");
  if (!fp)
    {
      perror (OUT_NAME);
      return;
    }
  memcpy (&ghdr.cookie[0], GMON_MAGIC, 4);
  memcpy (&ghdr.version, &version, sizeof (version));
  fwrite (&ghdr, sizeof (ghdr), 1, fp);

  for (ptr = __bb_head; ptr != 0; ptr = ptr->next)
    {
      u_int ncounts = ptr->ncounts;
      u_char tag;
      u_int i;

      tag = GMON_TAG_BB_COUNT;
      fwrite (&tag, sizeof (tag), 1, fp);
      fwrite (&ncounts, sizeof (ncounts), 1, fp);

      for (i = 0; i < ncounts; ++i)
	{
	  fwrite (&ptr->addresses[i], sizeof (ptr->addresses[0]), 1, fp);
	  fwrite (&ptr->counts[i], sizeof (ptr->counts[0]), 1, fp);
	}
    }
  fclose (fp);
}
