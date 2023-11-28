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

#include <alloca.h>
#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "localeinfo.h"



char *
setlocale (int category, const char *locale)
{
  /* Sanity check for CATEGORY argument.  */
  if (category < 0 || category > LC_ALL)
    return NULL;

  /* Does user want name of current locale?  */
  if (locale == NULL)
    return (char *) "C";

  if (locale[0] == '\0')
    {
      locale = getenv ("LC_ALL");
      if (locale == NULL || locale[0] == '\0')
	{
	  locale = getenv ("LANG");
	  if (locale == NULL || locale[0] == '\0')
	    locale = (char *) "C";
	}     
    }

  /* Change to a locale we don't know?  */
  if (strcmp (locale, "C") != 0 && strcmp (locale, "POSIX") != 0)
    return NULL;

  return (char *) "C";
}
