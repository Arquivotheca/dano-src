/* i386-specific implemetation of profiling support.
   Copyright (C) 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1997.

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

/* We need a special version of the `mcount' function since for ix86 it
   must not clobber any register.  This has several reasons:
     - there is a bug in gcc as of version 2.7.2.2 which prohibits the
       use of profiling together with nested functions
     - the ELF `fixup' function uses GCC's regparm feature
     - some (future) systems might want to pass parameters in registers.  */

		.globl _mcount
		.type _mcount, @function; 
		.align 1<<4
_mcount:		
	/* Save the caller-clobbered registers.  */
	pushl %eax
	pushl %ecx
	pushl %edx

	movl 12(%esp), %eax
	movl 4(%ebp), %ecx
	pushl %eax
	pushl %ecx

	call 1f
1:	popl %ecx
	addl $_GLOBAL_OFFSET_TABLE_+[.-1b], %ecx
	movl __mcount_internal@GOT(%ecx), %eax
	call *%eax

	popl %ecx
	popl %ecx	/* Pop the parameters to _mcount_internal.  */

	cmpl $0, %eax
	jne   2f	

	movl $_mcount_post, 4(%ebp)

	/* Pop the saved registers.  Please note that `mcount' has no
	   return value.  */
2:	popl %edx
	popl %ecx
	popl %eax
	ret
	.size   _mcount   ,.-  _mcount   ; 
.weak    mcount   ;	   mcount    =   _mcount    

	.globl   _mcount_post  
	.type   _mcount_post, @function; 
	.align 1<<4  
_mcount_post: 
	pushl %eax
	pushl %ecx
	pushl %edx

	call 1f
1:	popl %ecx
	addl $_GLOBAL_OFFSET_TABLE_+[.-1b], %ecx
	movl __mcount_post_internal@GOT(%ecx), %eax
	call *%eax

	/* Pop the saved registers.  Please note that `mcount_post' has no
	   return value.  */
	popl %edx
	popl %ecx
	xchg %eax, 0(%esp)			/* eax contains the return value from mcount_post_internal */
	ret
	.size   _mcount_post   ,.-  _mcount_post   ; 
.weak    mcount_post   ;	   mcount_post    =   _mcount_post    
