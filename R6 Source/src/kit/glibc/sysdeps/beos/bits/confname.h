/* `sysconf', `pathconf', and `confstr' NAME values.  Generic version.
   Copyright (C) 1993, 1995, 1996, 1997, 1998 Free Software Foundation, Inc.
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

#ifndef _UNISTD_H
# error "Never use <bits/confname.h> directly; include <unistd.h> instead."
#endif

/* Values for the NAME argument to `pathconf' and `fpathconf'.  */
enum
  {
    _PC_CHOWN_RESTRICTED = 1,
#define	_PC_CHOWN_RESTRICTED		_PC_CHOWN_RESTRICTED
    _PC_MAX_CANON,
#define	_PC_MAX_CANON			_PC_MAX_CANON
    _PC_MAX_INPUT,
#define	_PC_MAX_INPUT			_PC_MAX_INPUT
    _PC_NAME_MAX,
#define	_PC_NAME_MAX			_PC_NAME_MAX
    _PC_NO_TRUNC,
#define	_PC_NO_TRUNC			_PC_NO_TRUNC
    _PC_PATH_MAX,
#define	_PC_PATH_MAX			_PC_PATH_MAX
    _PC_PIPE_BUF,
#define	_PC_PIPE_BUF			_PC_PIPE_BUF
    _PC_VDISABLE,
#define _PC_VDISABLE			_PC_VDISABLE
    _PC_LINK_MAX = 25,
#define	_PC_LINK_MAX			_PC_LINK_MAX
  };

/* Values for the argument to `sysconf'.  */
enum
  {
    _SC_ARG_MAX = 15,
#define	_SC_ARG_MAX			_SC_ARG_MAX
    _SC_CHILD_MAX,
#define	_SC_CHILD_MAX			_SC_CHILD_MAX
    _SC_CLK_TCK,
#define	_SC_CLK_TCK			_SC_CLK_TCK
    _SC_JOB_CONTROL,
#define	_SC_JOB_CONTROL			_SC_JOB_CONTROL
    _SC_NGROUPS_MAX,
#define	_SC_NGROUPS_MAX			_SC_NGROUPS_MAX
    _SC_OPEN_MAX,
#define	_SC_OPEN_MAX			_SC_OPEN_MAX
    _SC_SAVED_IDS,
#define _SC_SAVED_IDS			_SC_SAVED_IDS
    _SC_STREAM_MAX,
#define	_SC_STREAM_MAX			_SC_STREAM_MAX
    _SC_TZNAME_MAX,
#define	_SC_TZNAME_MAX			_SC_TZNAME_MAX
    _SC_VERSION,
#define	_SC_VERSION			_SC_VERSION

    _SC_2_LOCALEDEF = 6666,
#define _SC_2_LOCALEDEF			_SC_2_LOCALEDEF
    _SC_PAGESIZE,
#define _SC_PAGESIZE			_SC_PAGESIZE
    _SC_UINT_MAX,
#define _SC_UINT_MAX			_SC_UINT_MAX
    _SC_ULONG_MAX,
#define _SC_ULONG_MAX			_SC_ULONG_MAX
    _SC_CHARCLASS_NAME_MAX,
#define _SC_CHARCLASS_NAME_MAX		_SC_CHARCLASS_NAME_MAX
  };

#ifdef __USE_POSIX2
/* Values for the NAME argument to `confstr'.  */
enum
  {
    _CS_PATH			/* The default search path.  */
#define _CS_PATH		_CS_PATH
  };
#endif
