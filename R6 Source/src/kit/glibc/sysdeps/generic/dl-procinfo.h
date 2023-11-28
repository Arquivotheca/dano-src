/* Stub version of processor capability information handling macros.
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

#ifndef _DL_PROCINFO_H
#define _DL_PROCINFO_H	1

/* We cannot provide a general printing function.  */
#define _dl_procinfo(word) -1

/* There are no hardware capabilities defined.  */
#define _dl_hwcap_string(idx) ""

/* By default there is no important hardware capability.  */
#define HWCAP_IMPORTANT (0)

#endif /* dl-procinfo.h */
