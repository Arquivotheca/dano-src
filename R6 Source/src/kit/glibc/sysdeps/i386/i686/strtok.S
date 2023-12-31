/* strtok (str, delim) -- Return next DELIM separated token from STR.
   For Intel 80686.
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

#include <sysdep.h>
#include "asm-syntax.h"

/* This file can be used for three variants of the strtok function:

   strtok:
	INPUT PARAMETER:
	str		(sp + 4)
	delim		(sp + 8)

   strtok_r:
	INPUT PARAMETER:
	str		(sp + 4)
	delim		(sp + 8)
	save_ptr	(sp + 12)

   We do a common implementation here.  */

#ifndef USE_AS_STRTOK_R
	.bss
	.local save_ptr
	ASM_TYPE_DIRECTIVE (save_ptr, @object)
	.size save_ptr, 4
save_ptr:
	.space 4

#define FUNCTION strtok
#endif

	.text

#if !defined USE_AS_STRTOK_R && defined PIC
0:	movl (%esp), %ebx
	ret
#endif

ENTRY (FUNCTION)

#if !defined USE_AS_STRTOK_R && defined PIC
	pushl %ebx			/* Save PIC register.  */
	call 0b
	addl $_GLOBAL_OFFSET_TABLE_, %ebx
#endif

	/* First we create a table with flags for all possible characters.
	   For the ASCII (7bit/8bit) or ISO-8859-X character sets which are
	   supported by the C string functions we have 256 characters.
	   Before inserting marks for the stop characters we clear the whole
	   table.  */
	movl %edi, %edx
	subl $256, %esp
	movl $64, %ecx
	movl %esp, %edi
	xorl %eax, %eax
	rep
	stosl

	/* Note: %ecx = 0 !!! */
	movl %edx, %edi

#if !defined USE_AS_STRTOK_R && defined PIC
	movl 264(%esp), %edx		/* Get start of string.  */
#else
	movl 260(%esp), %edx		/* Get start of string.  */
#endif

#ifdef USE_AS_STRTOK_R
	/* The value is stored in the third argument.  */
	movl 268(%esp), %eax
	movl (%eax), %eax
#else
	/* The value is in the local variable defined above.  But
	   we have to take care for PIC code.  */
# ifndef PIC
	movl save_ptr, %eax
# else
	movl save_ptr@GOTOFF(%ebx), %eax
# endif
#endif

	/* If the pointer is NULL we have to use the stored value of
	   the last run.  */
	cmpl $0, %edx
	cmove %eax, %edx

#if !defined USE_AS_STRTOK_R && defined PIC
	movl 268(%esp), %eax		/* Get start of delimiter set.  */
#else
	movl 264(%esp), %eax		/* Get start of delimiter set.  */
#endif

/* For understanding the following code remember that %ecx == 0 now.
   Although all the following instruction only modify %cl we always
   have a correct zero-extended 32-bit value in %ecx.  */

L(2):	movb (%eax), %cl	/* get byte from stopset */
	testb %cl, %cl		/* is NUL char? */
	jz L(1)			/* yes => start compare loop */
	movb %cl, (%esp,%ecx)	/* set corresponding byte in stopset table */

	movb 1(%eax), %cl	/* get byte from stopset */
	testb $0xff, %cl	/* is NUL char? */
	jz L(1)			/* yes => start compare loop */
	movb %cl, (%esp,%ecx)	/* set corresponding byte in stopset table */

	movb 2(%eax), %cl	/* get byte from stopset */
	testb $0xff, %cl	/* is NUL char? */
	jz L(1)			/* yes => start compare loop */
	movb %cl, (%esp,%ecx)	/* set corresponding byte in stopset table */

	movb 3(%eax), %cl	/* get byte from stopset */
	addl $4, %eax		/* increment stopset pointer */
	movb %cl, (%esp,%ecx)	/* set corresponding byte in stopset table */
	testb $0xff, %cl	/* is NUL char? */
	jnz L(2)		/* no => process next dword from stopset */

L(1):	leal -4(%edx), %eax	/* prepare loop */

	/* We use a neat trick for the following loop.  Normally we would
	   have to test for two termination conditions
	   1. a character in the stopset was found
	   and
	   2. the end of the string was found
	   As a sign that the character is in the stopset we store its
	   value in the table.  The value of NUL is NUL so the loop
	   terminates for NUL in every case.  */

L(3):	addl $4, %eax		/* adjust pointer for full loop round */

	movb (%eax), %cl	/* get byte from string */
	testb %cl, (%esp,%ecx)	/* is it contained in stopset? */
	jz L(4)			/* no => start of token */

	movb 1(%eax), %cl	/* get byte from string */
	testb %cl, (%esp,%ecx)	/* is it contained in stopset? */
	jz L(5)			/* no => start of token */

	movb 2(%eax), %cl	/* get byte from string */
	testb %cl, (%esp,%ecx)	/* is it contained in stopset? */
	jz L(6)			/* no => start of token */

	movb 3(%eax), %cl	/* get byte from string */
	testb %cl, (%esp,%ecx)	/* is it contained in stopset? */
	jnz L(3)		/* yes => start of loop */

	incl %eax		/* adjust pointer */
L(6):	incl %eax
L(5):	incl %eax

	/* Now we have to terminate the string.  */

L(4):	leal -4(%eax), %edx	/* We use %EDX for the next run.  */

L(7):	addl $4, %edx		/* adjust pointer for full loop round */

	movb (%edx), %cl	/* get byte from string */
	cmpb %cl, (%esp,%ecx)	/* is it contained in skipset? */
	je L(8)			/* yes => return */

	movb 1(%edx), %cl	/* get byte from string */
	cmpb %cl, (%esp,%ecx)	/* is it contained in skipset? */
	je L(9)			/* yes => return */

	movb 2(%edx), %cl	/* get byte from string */
	cmpb %cl, (%esp,%ecx)	/* is it contained in skipset? */
	je L(10)		/* yes => return */

	movb 3(%edx), %cl	/* get byte from string */
	cmpb %cl, (%esp,%ecx)	/* is it contained in skipset? */
	jne L(7)		/* no => start loop again */

	incl %edx		/* adjust pointer */
L(10):	incl %edx
L(9):	incl %edx

L(8):	/* Remove the stopset table.  */
	addl $256, %esp

	cmpl %eax, %edx
	je L(returnNULL)	/* There was no token anymore.  */

	movb $0, (%edx)		/* Terminate string.  */

	/* Are we at end of string?  */
	cmpb $0, %cl
	leal 1(%edx), %ecx
	cmovne %ecx, %edx

L(return):
	/* Store the pointer to the next character.  */
#ifdef USE_AS_STRTOK_R
	movl 12(%esp), %ecx
	movl %edx, (%ecx)
#else
# ifndef PIC
	movl %edx, save_ptr
# else
	movl %edx, save_ptr@GOTOFF(%ebx)
	popl %ebx
# endif
#endif
	ret

L(returnNULL):
	xorl %eax, %eax
	jmp L(return)
END (FUNCTION)
