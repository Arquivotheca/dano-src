/* Copyright (C) 1991, 1996 Free Software Foundation, Inc.
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

#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

/* Return the username of the caller.
   If S is not NULL, it points to a buffer of at least L_cuserid bytes
   into which the name is copied; otherwise, a static buffer is used.  */
char *
cuserid (s)
     char *s;
{
  static char name[L_cuserid];
  char buf[NSS_BUFLEN_PASSWD];
  struct passwd pwent;
  struct passwd *pwptr;

  if (__getpwuid_r (geteuid (), &pwent, buf, sizeof (buf), &pwptr))
    {
      if (s != NULL)
	s[0] = '\0';
      return NULL;
    }

  if (s == NULL)
    s = name;
  return strncpy (s, pwptr->pw_name, L_cuserid);
}
