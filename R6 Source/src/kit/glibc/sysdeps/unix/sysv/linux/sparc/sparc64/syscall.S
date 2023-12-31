/* Copyright (C) 1997 Free Software Foundation, Inc.
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
	.text

ENTRY (syscall)

	mov	%o0,%g1
	mov	%o1,%o0
	mov	%o2,%o1
	mov	%o3,%o2
	mov	%o4,%o3
	mov	%o5,%o4

	ta	0x11

	bcc,pt %xcc,1f
	 nop
	SYSCALL_ERROR_HANDLER

1:	retl

PSEUDO_END (syscall)
