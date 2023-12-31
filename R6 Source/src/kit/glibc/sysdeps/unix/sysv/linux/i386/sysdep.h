/* Copyright (C) 1992, 93, 95, 96, 97, 98 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper, <drepper@gnu.ai.mit.edu>, August 1995.

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

#ifndef _LINUX_I386_SYSDEP_H
#define _LINUX_I386_SYSDEP_H 1

/* There is some commonality.  */
#include <sysdeps/unix/i386/sysdep.h>

/* For Linux we can use the system call table in the header file
	/usr/include/asm/unistd.h
   of the kernel.  But these symbols do not follow the SYS_* syntax
   so we have to redefine the `SYS_ify' macro here.  */
#undef SYS_ify
#define SYS_ify(syscall_name)	__NR_##syscall_name

/* ELF-like local names start with `.L'.  */
#undef L
#define L(name)	.L##name

#ifdef __ASSEMBLER__

/* Linux uses a negative return value to indicate syscall errors,
   unlike most Unices, which use the condition codes' carry flag.

   Since version 2.1 the return value of a system call might be
   negative even if the call succeeded.  E.g., the `lseek' system call
   might return a large offset.  Therefore we must not anymore test
   for < 0, but test for a real error by making sure the value in %eax
   is a real error number.  Linus said he will make sure the no syscall
   returns a value in -1 .. -4095 as a valid result so we can savely
   test with -4095.  */

/* We don't want the label for the error handle to be global when we define
   it here.  */
#ifdef PIC
# define SYSCALL_ERROR_LABEL 0f
#else
# define SYSCALL_ERROR_LABEL syscall_error
#endif

#undef	PSEUDO
#define	PSEUDO(name, syscall_name, args)				      \
  .text;								      \
  ENTRY (name)								      \
    DO_CALL (args, syscall_name);					      \
    cmpl $-4095, %eax;							      \
    jae SYSCALL_ERROR_LABEL;						      \
  L(pseudo_end):

#undef	PSEUDO_END
#define	PSEUDO_END(name)						      \
  SYSCALL_ERROR_HANDLER							      \
  END (name)

#ifndef PIC
#define SYSCALL_ERROR_HANDLER	/* Nothing here; code in sysdep.S is used.  */
#else
/* Store (- %eax) into errno through the GOT.  */
#ifdef _LIBC_REENTRANT
#define SYSCALL_ERROR_HANDLER						      \
0:pushl %ebx;								      \
  call 1f;								      \
1:popl %ebx;								      \
  xorl %edx, %edx;							      \
  addl $_GLOBAL_OFFSET_TABLE_+[.-1b], %ebx;				      \
  subl %eax, %edx;							      \
  pushl %edx;								      \
  call __errno_location@PLT;						      \
  popl %ecx;								      \
  popl %ebx;								      \
  movl %ecx, (%eax);							      \
  movl $-1, %eax;							      \
  jmp L(pseudo_end);
/* A quick note: it is assumed that the call to `__errno_location' does
   not modify the stack!  */
#else
#define SYSCALL_ERROR_HANDLER						      \
0:call 1f;								      \
1:popl %ecx;								      \
  xorl %edx, %edx;							      \
  addl $_GLOBAL_OFFSET_TABLE_+[.-1b], %ecx;				      \
  subl %eax, %edx;							      \
  movl errno@GOT(%ecx), %ecx;						      \
  movl %edx, (%ecx);							      \
  movl $-1, %eax;							      \
  jmp L(pseudo_end);
#endif	/* _LIBC_REENTRANT */
#endif	/* PIC */

/* Linux takes system call arguments in registers:

	syscall number	%eax	     call-clobbered
	arg 1		%ebx	     call-saved
	arg 2		%ecx	     call-clobbered
	arg 3		%edx	     call-clobbered
	arg 4		%esi	     call-saved
	arg 5		%edi	     call-saved

   The stack layout upon entering the function is:

	20(%esp)	Arg# 5
	16(%esp)	Arg# 4
	12(%esp)	Arg# 3
	 8(%esp)	Arg# 2
	 4(%esp)	Arg# 1
	  (%esp)	Return address

   (Of course a function with say 3 arguments does not have entries for
   arguments 4 and 5.)

   The following code tries hard to be optimal.  A general assumption
   (which is true according to the data books I have) is that

	2 * xchg	is more expensive than	pushl + movl + popl

   Beside this a neat trick is used.  The calling conventions for Linux
   tell that among the registers used for parameters %ecx and %edx need
   not be saved.  Beside this we may clobber this registers even when
   they are not used for parameter passing.

   As a result one can see below that we save the content of the %ebx
   register in the %edx register when we have less than 3 arguments
   (2 * movl is less expensive than pushl + popl).

   Second unlike for the other registers we don't save the content of
   %ecx and %edx when we have more than 1 and 2 registers resp.

   The code below might look a bit long but we have to take care for
   the pipelined processors (i586).  Here the `pushl' and `popl'
   instructions are marked as NP (not pairable) but the exception is
   two consecutive of these instruction.  This gives no penalty on
   other processors though.  */

#undef	DO_CALL
#define DO_CALL(args, syscall_name)			      		      \
    PUSHARGS_##args							      \
    DOARGS_##args							      \
    movl $SYS_ify (syscall_name), %eax;					      \
    int $0x80								      \
    POPARGS_##args

#define PUSHARGS_0	/* No arguments to push.  */
#define	DOARGS_0	/* No arguments to frob.  */
#define	POPARGS_0	/* No arguments to pop.  */
#define	_PUSHARGS_0	/* No arguments to push.  */
#define _DOARGS_0(n)	/* No arguments to frob.  */
#define	_POPARGS_0	/* No arguments to pop.  */

#define PUSHARGS_1	movl %ebx, %edx; PUSHARGS_0
#define	DOARGS_1	_DOARGS_1 (4)
#define	POPARGS_1	POPARGS_0; movl %edx, %ebx
#define	_PUSHARGS_1	pushl %ebx; _PUSHARGS_0
#define _DOARGS_1(n)	movl n(%esp), %ebx; _DOARGS_0(n-4)
#define	_POPARGS_1	_POPARGS_0; popl %ebx

#define PUSHARGS_2	PUSHARGS_1
#define	DOARGS_2	_DOARGS_2 (8)
#define	POPARGS_2	POPARGS_1
#define _PUSHARGS_2	_PUSHARGS_1
#define	_DOARGS_2(n)	movl n(%esp), %ecx; _DOARGS_1 (n-4)
#define	_POPARGS_2	_POPARGS_1

#define PUSHARGS_3	_PUSHARGS_2
#define DOARGS_3	_DOARGS_3 (16)
#define POPARGS_3	_POPARGS_3
#define _PUSHARGS_3	_PUSHARGS_2
#define _DOARGS_3(n)	movl n(%esp), %edx; _DOARGS_2 (n-4)
#define _POPARGS_3	_POPARGS_2

#define PUSHARGS_4	_PUSHARGS_4
#define DOARGS_4	_DOARGS_4 (24)
#define POPARGS_4	_POPARGS_4
#define _PUSHARGS_4	pushl %esi; _PUSHARGS_3
#define _DOARGS_4(n)	movl n(%esp), %esi; _DOARGS_3 (n-4)
#define _POPARGS_4	_POPARGS_3; popl %esi

#define PUSHARGS_5	_PUSHARGS_5
#define DOARGS_5	_DOARGS_5 (32)
#define POPARGS_5	_POPARGS_5
#define _PUSHARGS_5	pushl %edi; _PUSHARGS_4
#define _DOARGS_5(n)	movl n(%esp), %edi; _DOARGS_4 (n-4)
#define _POPARGS_5	_POPARGS_4; popl %edi

#endif	/* __ASSEMBLER__ */

#endif /* linux/i386/sysdep.h */
