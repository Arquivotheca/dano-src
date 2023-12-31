/* Copyright (C) 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Richard Henderson <richard@gnu.ai.mit.edu>, 1997.

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

/* __brk is a special syscall under Linux since it never returns an
   error.  Instead, the error condition is indicated by returning the old
   break value (instead of the new, requested one).  */

#include <sysdep.h>
#define _ERRNO_H
#include <bits/errno.h>

#ifdef PIC
.section .bss
	.align 8
	.globl __curbrk
__curbrk: .skip 8
	.type __curbrk,@object
	.size __curbrk,8
#else
.common __curbrk, 8, 8
#endif

	.text
ENTRY(__brk)
	save	%sp, -160, %sp
#ifdef PIC
1:	call	2f
	sethi	%hi(_GLOBAL_OFFSET_TABLE_-(1b-.)), %l7
2:	or	%l7, %lo(_GLOBAL_OFFSET_TABLE_-(1b-.)), %l7
	add	%l7, %o7, %l7
#endif

	LOADSYSCALL(brk)
	mov	%i0, %o0

	ta	0x11

	/* All the ways we can fail... */
	bcs,pn	%xcc, .Lerr1
	 nop
	brz	%i0, .Lok
	 subcc	%i0, %o0, %g0
	bne,pn	%xcc, .Lerr0

	/* Update __curbrk and return cleanly.  */
.Lok:	sethi	%hi(__curbrk), %g1
	or	%g1, %lo(__curbrk), %g1
#ifdef PIC
	ldx	[%l7+%g1], %g1
	stx	%o0, [%g1]
#else
	stx	%o0, [%g4+%g1]
#endif
	mov	%g0, %i0

	/* Don't use "ret" cause the preprocessor will eat it.  */
	jmpl	%i7+8, %g0
	 restore
	
	/* What a horrible way to die.  */
.Lerr0:	set	ENOMEM, %o0
.Lerr1:
#ifndef _LIBC_REENTRANT
	sethi	%hi(errno), %g1
	or	%g1, %lo(errno), %g1
#ifdef PIC
	ldx	[%l7+%g1], %g1
	st	%o0, [%g1]
#else
	st	%o0, [%g4+%g1]
#endif
#else
	call	__errno_location
	 mov	%o0,%l1
	st	%l1, [%o0]
#endif
	sub	%g0, 1, %i0
	jmpl	%i7+8, %g0
	 restore

	.size __brk, .-__brk

weak_alias (__brk, brk)
