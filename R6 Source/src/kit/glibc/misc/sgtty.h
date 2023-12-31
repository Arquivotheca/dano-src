/* Copyright (C) 1991, 1992, 1996 Free Software Foundation, Inc.
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

#ifndef	_SGTTY_H
#define	_SGTYY_H	1

#include <features.h>

#include <sys/ioctl.h>

/* On some systems this type is not defined by <bits/ioctl-types.h>;
   in that case, the functions are just stubs that return ENOSYS.  */
struct sgttyb;

__BEGIN_DECLS

/* Fill in *PARAMS with terminal parameters associated with FD.  */
extern int gtty __P ((int __fd, struct sgttyb *__params));

/* Set the terminal parameters associated with FD to *PARAMS.  */
extern int stty __P ((int __fd, __const struct sgttyb *__params));


__END_DECLS

#endif /* sgtty.h  */
