/* Optional code to distinguish library flavours.
   Copyright (C) 1998 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1998.

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

#ifndef _DL_LIBRECON_H
#define _DL_LIBRECON_H	1

#define DISTINGUISH_LIB_VERSIONS \
  do									      \
    {									      \
      /* We have to find out whether the binary is linked against	      \
	 libc 5 or glibc.  We do this by looking at all the DT_NEEDED	      \
	 entries.  If one is libc.so.5 this is a libc 5 linked binary.  */    \
      if (main_map->l_info[DT_NEEDED])					      \
	{								      \
	  /* We have dependencies.  */					      \
	  const char *strtab = ((void *) main_map->l_addr		      \
				+ main_map->l_info[DT_STRTAB]->d_un.d_ptr);   \
	  const ElfW(Dyn) *d;						      \
									      \
	  for (d = main_map->l_ld; d->d_tag != DT_NULL; ++d)		      \
	    if (d->d_tag == DT_NEEDED					      \
		&& strcmp (strtab + d->d_un.d_val, "libc.so.5") == 0)	      \
	      break;							      \
									      \
	  /* We print a `5' or `6' depending on the outcome.  */	      \
	  _dl_sysdep_message (d->d_tag != DT_NULL ? "5\n" : "6\n", NULL);     \
	}								      \
    }									      \
  while (0)

/* Recognizing extra environment variables.  */
#define EXTRA_LD_ENVVARS \
  case 15:								      \
    if (memcmp (&envline[3], "LIBRARY_VERSION", 15) == 0)		      \
      {									      \
	_dl_correct_cache_id = envline[19] == '5' ? 2 : 3;		      \
	break;								      \
      }

/* Extra unsecure variables.  */
#define EXTRA_UNSECURE_ENVVARS "LD_AOUT_LIBRARY_PATH", "LD_AOUT_PRELOAD"

#endif /* dl-librecon.h */
