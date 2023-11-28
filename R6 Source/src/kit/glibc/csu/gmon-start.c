/* Code to enable profiling at program startup.
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

#include <sys/types.h>
#include <sys/gmon.h>
#include <stdlib.h>
#include <unistd.h>

/* Beginning and end of our code segment.  */
extern void _start (void), etext (void);

#ifndef HAVE_INITFINI
/* This function gets called at startup by the normal constructor
   mechanism.  We link this file together with start.o to produce gcrt1.o,
   so this constructor will be first in the list.  */

void __gmon_start__ (void) __attribute__ ((constructor));
#else
/* In ELF and COFF, we cannot use the normal constructor mechanism to call
   __gmon_start__ because gcrt1.o appears before crtbegin.o in the link.
   Instead crti.o calls it specially (see initfini.c).  */
#endif

void
__gmon_start__ (void)
{
#ifdef HAVE_INITFINI
  /* Protect from being called more than once.  Since crti.o is linked
     into every shared library, each of their init functions will call us.  */
  static int called;

  if (called++)
    return;
#endif

  /* Start keeping profiling records.  */
  __monstartup ((u_long) &_start, (u_long) &etext);

  /* Call _mcleanup before exiting; it will write out gmon.out from the
     collected data.  */
  atexit (&_mcleanup);
}
