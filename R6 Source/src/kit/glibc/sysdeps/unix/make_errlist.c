/* Copyright (C) 1991, 1992, 1995, 1997 Free Software Foundation, Inc.
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

#include <stdio.h>

/* Make a definition for sys_errlist.  */

extern int sys_nerr;
#ifndef HAVE_STRERROR
extern char *sys_errlist[];
#endif

int
main ()
{
  register int i;

  puts ("#include <stddef.h>\n");
  puts ("\n/* This is a list of all known `errno' codes.  */\n");

  printf ("\nconst int _sys_nerr = %d;\n\n", sys_nerr);
  puts ("const char *const _sys_errlist[] =\n  {");

  for (i = 0; i < sys_nerr; ++i)
    printf ("    \"%s\",\n",
#ifdef HAVE_STRERROR
	  strerror (i)
#else /* ! HAVE_STRERROR */
	  sys_errlist[i]
#endif /* HAVE_STRERROR */
	  );

  puts ("    NULL\n  };\n");

  puts ("weak_alias (_sys_errlist, sys_errlist)");
  puts ("weak_alias (_sys_nerr, sys_nerr)");

  exit (0);
}
