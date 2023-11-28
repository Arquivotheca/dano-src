/* Minimum guaranteed maximum values for system limits.  BeOS version.
   Copyright (C) 1993, 94, 95, 96, 97, 98 Free Software Foundation, Inc.
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

/* Maximum length of argument list.  */
#define ARG_MAX		(128 * 1024)

/* Maximum number of simultaneous processes per real user ID.  */
#define CHILD_MAX	666

/* Maximum number of links a file may have.  */
#define LINK_MAX	1

/* Size of canonical input queue.  */
#define MAX_CANON	255

/* Size of type-ahead buffer.  */
#define MAX_INPUT	255

/* Maximum number of characters in a file name.  */
#define NAME_MAX	256

/* Maximum number of supplemental groups available.  */
#define NGROUPS_MAX	32

/* Maximum number of open files a process may have.  */
#define OPEN_MAX	128

/* Maximum number of characters in a path name.  */
#if _SUPPORTS_FEATURE_SHORT_PATH_MAX
#	define PATH_MAX				512
#else
#	define PATH_MAX				1024
#endif

/* Maximum number of bytes in atomic write to a pipe.  */
#define PIPE_MAX	512

#define SSIZE_MAX		32767
#define TZNAME_MAX		32
#define SYMLINKS_MAX	16
#define MAXPATHLEN		PATH_MAX
