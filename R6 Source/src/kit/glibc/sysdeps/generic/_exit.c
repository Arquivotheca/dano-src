/* Copyright (C) 1991, 1994, 1995, 1996, 1997 Free Software Foundation, Inc.
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

#include <unistd.h>
#include <stdlib.h>

/* The function `_exit' should take a status argument and simply
   terminate program execution, using the low-order 8 bits of the
   given integer as status.  */
void
_exit (status)
     int status;
{
  status &= 0xff;
  abort ();
}

stub_warning (_exit)
#include <stub-tag.h>
