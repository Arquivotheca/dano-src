/* ix87 specific implementation of pow function.
   Copyright (C) 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1996.

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

#include <machine/asm.h>

#ifdef __ELF__
	.section .rodata
#else
	.text
#endif

	.align ALIGNARG(4)
	ASM_TYPE_DIRECTIVE(infinity,@object)
inf_zero:
infinity:
	.byte 0, 0, 0, 0, 0, 0, 0xf0, 0x7f
	ASM_SIZE_DIRECTIVE(infinity)
	ASM_TYPE_DIRECTIVE(zero,@object)
zero:	.double 0.0
	ASM_SIZE_DIRECTIVE(zero)
	ASM_TYPE_DIRECTIVE(minf_mzero,@object)
minf_mzero:
minfinity:
	.byte 0, 0, 0, 0, 0, 0, 0xf0, 0xff
mzero:
	.byte 0, 0, 0, 0, 0, 0, 0, 0x80
	ASM_SIZE_DIRECTIVE(minf_mzero)
	ASM_TYPE_DIRECTIVE(one,@object)
one:	.double 1.0
	ASM_SIZE_DIRECTIVE(one)
	ASM_TYPE_DIRECTIVE(limit,@object)
limit:	.double 0.29
	ASM_SIZE_DIRECTIVE(limit)

#ifdef PIC
#define MO(op) op##@GOTOFF(%ecx)
#define MOX(op,x,f) op##@GOTOFF(%ecx,x,f)
#else
#define MO(op) op
#define MOX(op,x,f) op(,x,f)
#endif

	.text
ENTRY(__ieee754_powf)
	flds	8(%esp)	// y
	fxam

#ifdef	PIC
	call	1f
1:	popl	%ecx
	addl	$_GLOBAL_OFFSET_TABLE_+[.-1b], %ecx
#endif

	fnstsw
	movb	%ah, %dl
	andb	$0x45, %ah
	cmpb	$0x40, %ah	// is y == 0 ?
	je	11f

	cmpb	$0x05, %ah	// is y == �inf ?
	je	12f

	cmpb	$0x01, %ah	// is y == NaN ?
	je	30f

	flds	4(%esp)		// x : y

	subl	$4, %esp

	fxam
	fnstsw
	movb	%ah, %dh
	andb	$0x45, %ah
	cmpb	$0x40, %ah
	je	20f		// x is �0

	cmpb	$0x05, %ah
	je	15f		// x is �inf

	fxch			// y : x

	/* First see whether `y' is a natural number.  In this case we
	   can use a more precise algorithm.  */
	fld	%st		// y : y : x
	fistpl	(%esp)		// y : x
	fildl	(%esp)		// int(y) : y : x
	fucomp	%st(1)		// y : x
	fnstsw
	sahf
	jne	2f

	/* OK, we have an integer value for y.  */
	popl	%edx
	orl	$0, %edx
	fstp	%st(0)		// x
	jns	4f		// y >= 0, jump
	fdivrl	MO(one)		// 1/x		(now referred to as x)
	negl	%edx
4:	fldl	MO(one)		// 1 : x
	fxch

6:	shrl	$1, %edx
	jnc	5f
	fxch
	fmul	%st(1)		// x : ST*x
	fxch
5:	fmul	%st(0), %st	// x*x : ST*x
	testl	%edx, %edx
	jnz	6b
	fstp	%st(0)		// ST*x
30:	ret

	.align ALIGNARG(4)
2:	/* y is a real number.  */
	fxch			// x : y
	fldl	MO(one)		// 1.0 : x : y
	fld	%st(1)		// x : 1.0 : x : y
	fsub	%st(1)		// x-1 : 1.0 : x : y
	fabs			// |x-1| : 1.0 : x : y
	fcompl	MO(limit)	// 1.0 : x : y
	fnstsw
	fxch			// x : 1.0 : y
	sahf
	ja	7f
	fsub	%st(1)		// x-1 : 1.0 : y
	fyl2xp1			// log2(x) : y
	jmp	8f

7:	fyl2x			// log2(x) : y
8:	fmul	%st(1)		// y*log2(x) : y
	fst	%st(1)		// y*log2(x) : y*log2(x)
	frndint			// int(y*log2(x)) : y*log2(x)
	fsubr	%st, %st(1)	// int(y*log2(x)) : fract(y*log2(x))
	fxch			// fract(y*log2(x)) : int(y*log2(x))
	f2xm1			// 2^fract(y*log2(x))-1 : int(y*log2(x))
	faddl	MO(one)		// 2^fract(y*log2(x)) : int(y*log2(x))
	fscale			// 2^fract(y*log2(x))*2^int(y*log2(x)) : int(y*log2(x))
	addl	$4, %esp
	fstp	%st(1)		// 2^fract(y*log2(x))*2^int(y*log2(x))
	ret


	// pow(x,�0) = 1
	.align ALIGNARG(4)
11:	fstp	%st(0)		// pop y
	fldl	MO(one)
	ret

	// y == �inf
	.align ALIGNARG(4)
12:	fstp	%st(0)		// pop y
	flds	4(%esp)		// x
	fabs
	fcompl	MO(one)		// < 1, == 1, or > 1
	fnstsw
	andb	$0x45, %ah
	cmpb	$0x45, %ah
	je	13f		// jump if x is NaN

	cmpb	$0x40, %ah
	je	14f		// jump if |x| == 1

	shlb	$1, %ah
	xorb	%ah, %dl
	andl	$2, %edx
	fldl	MOX(inf_zero, %edx, 4)
	ret

	.align ALIGNARG(4)
14:	fldl	MO(infinity)
	fmull	MO(zero)	// raise invalid exception
	ret

	.align ALIGNARG(4)
13:	flds	4(%esp)		// load x == NaN
	ret

	.align ALIGNARG(4)
	// x is �inf
15:	fstp	%st(0)		// y
	testb	$2, %dh
	jz	16f		// jump if x == +inf

	// We must find out whether y is an odd integer.
	fld	%st		// y : y
	fistpl	(%esp)		// y
	fildl	(%esp)		// int(y) : y
	fucompp			// <empty>
	fnstsw
	sahf
	jne	17f

	// OK, the value is an integer, but is the number of bits small
	// enough so that all are coming from the mantissa?
	popl	%edx
	testb	$1, %dl
	jz	18f		// jump if not odd
	movl	%edx, %eax
	orl	%edx, %edx
	jns	155f
	negl	%eax
155:	cmpl	$0x01000000, %eax
	ja	18f		// does not fit in mantissa bits
	// It's an odd integer.
	shrl	$31, %edx
	fldl	MOX(minf_mzero, %edx, 8)
	ret

	.align ALIGNARG(4)
16:	fcompl	MO(zero)
	addl	$4, %esp
	fnstsw
	shrl	$5, %eax
	andl	$8, %eax
	fldl	MOX(inf_zero, %eax, 1)
	ret

	.align ALIGNARG(4)
17:	shll	$30, %edx	// sign bit for y in right position
	addl	$4, %esp
18:	shrl	$31, %edx
	fldl	MOX(inf_zero, %edx, 8)
	ret

	.align ALIGNARG(4)
	// x is �0
20:	fstp	%st(0)		// y
	testb	$2, %dl
	jz	21f		// y > 0

	// x is �0 and y is < 0.  We must find out whether y is an odd integer.
	testb	$2, %dh
	jz	25f

	fld	%st		// y : y
	fistpl	(%esp)		// y
	fildl	(%esp)		// int(y) : y
	fucompp			// <empty>
	fnstsw
	sahf
	jne	26f

	// OK, the value is an integer, but is the number of bits small
	// enough so that all are coming from the mantissa?
	popl	%edx
	testb	$1, %dl
	jz	27f		// jump if not odd
	cmpl	$0xff000000, %edx
	jbe	27f		// does not fit in mantissa bits
	// It's an odd integer.
	// Raise divide-by-zero exception and get minus infinity value.
	fldl	MO(one)
	fdivl	MO(zero)
	fchs
	ret

25:	fstp	%st(0)
26:	popl	%eax
27:	// Raise divide-by-zero exception and get infinity value.
	fldl	MO(one)
	fdivl	MO(zero)
	ret

	.align ALIGNARG(4)
	// x is �0 and y is > 0.  We must find out whether y is an odd integer.
21:	testb	$2, %dh
	jz	22f

	fld	%st		// y : y
	fistpl	(%esp)		// y
	fildl	(%esp)		// int(y) : y
	fucompp			// <empty>
	fnstsw
	sahf
	jne	23f

	// OK, the value is an integer, but is the number of bits small
	// enough so that all are coming from the mantissa?
	popl	%edx
	testb	$1, %dl
	jz	24f		// jump if not odd
	cmpl	$0xff000000, %edx
	jae	24f		// does not fit in mantissa bits
	// It's an odd integer.
	fldl	MO(mzero)
	ret

22:	fstp	%st(0)
23:	popl	%eax
24:	fldl	MO(zero)
	ret

END(__ieee754_powf)
