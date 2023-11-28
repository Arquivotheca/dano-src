/* Copyright (C) 1996, 1997 Free Software Foundation, Inc.
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

#ifndef _SYS_IOCTL_H
# error "Never use <bits/ioctls.h> directly; include <sys/ioctl.h> instead."
#endif

/* Use the definitions from the kernel header files.  */
#include <asm/ioctls.h>
#include <kernel_termios.h>

/* Oh well, this is necessary since the kernel data structure is
   different from the user-level version.  */
#undef  TCGETS
#undef  TCSETS
#undef  TCSETSW
#undef  TCSETSF
#define TCGETS	_IOR ('t', 19, struct __kernel_termios)
#define TCSETS	_IOW ('t', 20, struct __kernel_termios)
#define TCSETSW	_IOW ('t', 21, struct __kernel_termios)
#define TCSETSF	_IOW ('t', 22, struct __kernel_termios)

#include <linux/sockios.h>
