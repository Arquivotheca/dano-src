/* Test program for
   Copyright (C) 1998 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Jochen Hein <jochen.hein@delphi.central.de>.

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

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

int
main (int argc, char* argv[])
{
  setlocale (LC_ALL, argv[1]);

  if (rpmatch (argv[2]) != atol (argv[3]))
    {
      fprintf (stderr,"Failed: Locale %s, String %s, Exp: %s, got %d\n",
	       argv[1], argv[2], argv[3], rpmatch (argv[2]));
      exit (EXIT_FAILURE);
    }
  exit (EXIT_SUCCESS);
}
