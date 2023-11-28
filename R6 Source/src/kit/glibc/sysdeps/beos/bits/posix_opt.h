/* Define POSIX options for BeOS.
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

/*
 * Never include this file directly; use <unistd.h> instead.
 */

#ifndef	_BITS_POSIX_OPT_H
#define	_BITS_POSIX_OPT_H	1

/* Job control is supported.  */
#define	_POSIX_JOB_CONTROL	1

/* Processes have a saved set-user-ID and a saved set-group-ID.  */
#define	_POSIX_SAVED_IDS	1

/* The fsync function is present.  */
#define	_POSIX_FSYNC	1

/* Only root can change owner of file.  */
#define	_POSIX_CHOWN_RESTRICTED	1

/* `c_cc' member of 'struct termios' structure can be disabled by
   using the value _POSIX_VDISABLE.  */
#define	_POSIX_VDISABLE	((unsigned char) -2)

/* Filenames are not silently truncated.  */
#define	_POSIX_NO_TRUNC	1

#endif /* bits/posix_opt.h */
