/* Copyright (C) 1998 Free Software Foundation, Inc.
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
#define _ERRNO_H        1
#include <bits/errno.h>

/* The problem here is that initially we made struct timeval compatible with
   OSF/1, using int32.  But we defined time_t with uint64, and later found
   that POSIX requires tv_sec to be time_t.

   So now we have to do compatibility stuff.  */

/* The variable is shared between all wrappers around signal handling
   functions which have RT equivalents.  */
.comm __libc_missing_axp_tv64, 4

.text

#if defined HAVE_ELF && defined PIC && defined DO_VERSIONING
#define GETITIMER	__getitimer_tv64
#else
#define GETITIMER	getitimer
#endif

LEAF(GETITIMER, 16)
	ldgp	gp, 0(pv)
	subq	sp, 16, sp
#ifdef PROF
	.set noat
	lda	AT, _mcount
	jsr	AT, (AT), _mcount
	.set at
#endif
	.prologue 1

	ldl	t0, __libc_missing_axp_tv64

	/* Save arguments in case we do need to fall back.  */
	stq	a0, 0(sp)
	stq	a1, 8(sp)

	bne	t0, $do32

	ldi	v0, SYS_ify(getitimer)
	callsys
	bne	a3, $err64

	/* Everything ok.  */
	addq	sp, 16, sp
	ret

	/* If we didn't get ENOSYS, it is a real error.  */
	.align 3
$err64:	cmpeq	v0, ENOSYS, t0
	beq	t0, $error
	stl	t0, __libc_missing_axp_tv64

	/* Recover the saved arguments.  */
	ldq	a1, 8(sp)
	ldq	a0, 0(sp)

	.align 3
$do32:	ldi	v0, SYS_ify(osf_getitimer)
	callsys
	bne	a3, $error

	/* Copy back to proper format.  */
	ldq	a1, 8(sp)
	ldl	t0, 0(a1)
	ldl	t1, 4(a1)
	ldl	t2, 8(a1)
	ldl	t3, 12(a1)
	stq	t0, 0(a1)
	stq	t1, 8(a1)
	stq	t2, 16(a1)
	stq	t3, 24(a1)

	addq	sp, 16, sp
	ret

	.align 3
$error:
	lda	pv, __syscall_error
	addq	sp, 16, sp
	jmp	zero, (pv), __syscall_error

END(GETITIMER)

#if defined HAVE_ELF && defined PIC && defined DO_VERSIONING
default_symbol_version (__getitimer_tv64, getitimer, GLIBC_2.1)
#endif
