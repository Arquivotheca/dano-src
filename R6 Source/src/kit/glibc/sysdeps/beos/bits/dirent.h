/* Directory entry structure `struct dirent'.  BeOS version.
   Copyright (C) 1996, 1997, 1998 Free Software Foundation, Inc.
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

#ifndef _DIRENT_H
# error "Never use <bits/dirent.h> directly; include <dirent.h> instead."
#endif

#define d_fileno d_ino   /* make both work on BeOS */

struct dirent
  {
    __dev_t d_dev;
    __dev_t d_pdev;
    __ino_t d_fileno;
    __ino_t d_pino;
    unsigned short int d_reclen;
    char d_name[1];		/* Variable length.  */
  };


#ifdef __USE_LARGEFILE64
struct dirent64
  {
    __dev_t d_dev;
    __dev_t d_pdev;
    __ino_t d_fileno;
    __ino_t d_pino;
    unsigned short int d_reclen;
    char d_name[1];		/* Variable length.  */
  };
#endif

#define _DIRENT_HAVE_D_RECLEN

#include <dirstream.h>
