/* Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.
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

#include <sysdep.h>

/* We define errno here, to be consistent with Linux/i386.  */
	.bss
	.globl C_SYMBOL_NAME(errno)
	.type C_SYMBOL_NAME(errno),%object
	.size C_SYMBOL_NAME(errno),4
C_SYMBOL_NAME(errno):	.zero 4
weak_alias (C_SYMBOL_NAME(errno), C_SYMBOL_NAME(_errno))
	.text

/* The syscall stubs jump here when they detect an error.
   The code for Linux is almost identical to the canonical Unix
   code, except that the error number in R0 is negated.  */

#undef CALL_MCOUNT
#define CALL_MCOUNT /* Don't insert the profiling call, it clobbers R0.  */

ENTRY (__syscall_error)
	rsb r0, r0, $0

#define __syscall_error __syscall_error_1
#include <sysdeps/unix/arm/sysdep.S>
