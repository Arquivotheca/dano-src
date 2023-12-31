/* Copyright (C) 1993, 1995, 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by David Mosberger <davidm@azstarnet.com>, 1995.

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

#define SSI_IEEE_FP_CONTROL	14

LEAF(__ieee_set_fp_control, 16)
#ifdef PROF
	ldgp	gp, 0(pv)
	lda	sp, -16(sp)
	.set noat
	lda	AT, _mcount
	jsr	AT, (AT), _mcount
	.set at
	.prologue 1
#else
	lda	sp, -16(sp)
	.prologue 0
#endif

	stq	a0, 0(sp)
	mov	sp, a1
	ldi	a0, SSI_IEEE_FP_CONTROL
	ldi	v0, __NR_osf_setsysinfo
	call_pal PAL_callsys

	bne	a3, $error

	lda	sp, 16(sp)
	ret

$error:
#ifndef PROF
	br	gp, 1f
1:	ldgp	gp, 0(gp)
#endif
	lda	pv, __syscall_error
	lda	sp, 16(sp)
	jmp	zero, (pv), __syscall_error

	END(__ieee_set_fp_control)

weak_alias (__ieee_set_fp_control, ieee_set_fp_control)
