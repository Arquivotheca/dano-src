/*****  Copyright  (C) 1996-97 Intel Corporation. All rights reserved. 
***
*** The information and source code contained herein is the exclusive 
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization 
*** from the company.
**/

/*
 *  cpu_asm.S contains cpu dependent assembly routines
 *  taken mostly from the kernel cpu_asm.S, minus APIC stuff.
 */

#include	<asm.h>

.text

ENTRY(enable_interrupts)
	sti
	ret

ENTRY(disable_interrupts)
    pushf                        /* push the eflags register */
    popl     %eax                /* and pop it into eax */

    movl     $0x200, %ecx        /* load the IF (interrupt flag) mask */
    andl     %ecx, %eax          /* mask out the IF bit (as a return value) */

    cli                          /* disable interrupts */
    ret


ENTRY(restore_interrupts)
    movl     4(%esp), %eax       /* load the old IF status */
    cmpl     $0, %eax            /* if it was zero, skip restoring IF */
    je       restore_interrupts1

    pushf                        /* push the eflags register */
    popl     %eax                /* pop it into eax */
    orl      $0x200, %eax        /* OR in the IF bit */
    pushl    %eax                /* push eax on the stack */
    popf                         /* and pop it back into eflags */
restore_interrupts1:
    ret

/*
 * get_cpu_id 
 */

ENTRY(get_cpu_id)
	xorl	%eax, %eax
	ret

ENTRY(read_io_8)
	xorl	%eax, %eax
	movl	4(%esp), %edx		/* get I/O port address in edx */
	inb		%dx, %al
	ret

ENTRY(write_io_8)
	xorl	%eax, %eax
	movl	4(%esp), %edx		/* I/O port address */
	movl	8(%esp), %eax		/* value */
	outb	%al, %dx
	ret

ENTRY(read_io_16)
	xorl	%eax, %eax
	movl	4(%esp), %edx		/* get I/O port address in edx */
	inw		%dx, %ax
	ret

ENTRY(write_io_16)
	xorl	%eax, %eax
	movl	4(%esp), %edx		/* I/O port address */
	movl	8(%esp), %eax		/* value */
	outw	%ax, %dx
	ret

ENTRY(read_io_32)
	xorl	%eax, %eax
	movl	4(%esp), %edx		/* get I/O port address in edx */
	inl	%dx, %eax
	ret

ENTRY(write_io_32)
	xorl	%eax, %eax
	movl	4(%esp), %edx		/* I/O port address */
	movl	8(%esp), %eax		/* value */
	outl	%eax, %dx
	ret


ENTRY(set_stack_and_jump) 
	movl	8(%esp), %edx
	movl	4(%esp), %eax
	movl	%eax, %esp
	movl	$0, %ebp
	jmp		*%edx				/* never returns */

/* ~~~~~~~~~~
	system_time - return the number of usec since <whenever>
		-- INTEGER version
		entry:	<ret>
		exit:	[%edx,%eax] = time
~~~~~ */
ENTRY(system_time)
	/* load 64-bit factor into %eax (low), %edx (high) */
	/* hand-assemble rdtsc -- read time stamp counter */
	rdtsc		/* time in %edx,%eax */
	/* This tail code is common to this and previous routine. */
system_time_common:
	pushl	%ebx
	pushl	%ecx
	movl	EXTERN(cv_factor), %ebx
	movl	%edx, %ecx	/* save high half */
	mull	%ebx 		/* truncate %eax, but keep %edx */
	movl	%ecx, %eax
	movl	%edx, %ecx	/* save high half of low */
	mull	%ebx /*, %eax*/
	/* now compute  [%edx, %eax] + [%ecx], propagating carry */
	subl	%ebx, %ebx	/* need zero to propagate carry */
	addl	%ecx, %eax
	adc	%ebx, %edx
	popl	%ecx
	popl	%ebx
	ret

/* ~~~~~~~~~~
	system_time_setup - compute and store F = usec/ticks factor
		entry:	<ret>
			<long processor ticks per second>
		exit:	conversion factor F stored in global cv_factor
			and fp_cv_factor
	
	Compute 1,000,000/proc_clock as a 32-bit fraction.
	
	In the integer domain, this is the same as computing
	(1,000,000 * 2^32) / proc_clock and taking the integer
	part.  This requires a 64/32 divide to give an integer quotient.
	
	In the floating domain, just compute the ratio as a float.
~~~~~ */
ENTRY(system_time_setup)
	/* First divide 1M * 2^32 by proc_clock */
	movl	$0x0F4240, %ecx
	movl	%ecx, %edx
	subl	%eax, %eax
	movl	4(%esp), %ebx
	divl	%ebx, %eax		/* should be 64 / 32 */
	movl	%eax, EXTERN(cv_factor)

	ret

/*
	bigtime_t get_time_base()
	### change that to return long long when we have compiler support
 */

ENTRY(get_time_base)
	.byte	0x0f
	.byte	0x31			/* read time stamp counter */
	ret


/*
	clear_time_base()
*/

ENTRY(clear_time_base)
	xorl	%eax, %eax		/* clear edx:eax */
	xorl	%edx, %edx
	movl	$0x10, %ecx		/* TSC value for RDMSR */
	.byte	0x0f
	.byte	0x30			/* set time stamp */
	ret

/*
	execute_n_instructions (register int)
	  count must be in multiples of 16

 */

ENTRY(execute_n_instructions)
	movl	SARG0, %ecx
	shrl	$4, %ecx		/* divide count by 16 */
.again:
	xorl	%eax, %eax
	xorl	%eax, %eax
	xorl	%eax, %eax
	xorl	%eax, %eax
	xorl	%eax, %eax
	xorl	%eax, %eax
	xorl	%eax, %eax
	xorl	%eax, %eax
	xorl	%eax, %eax
	xorl	%eax, %eax
	xorl	%eax, %eax
	xorl	%eax, %eax
	xorl	%eax, %eax
	xorl	%eax, %eax
	xorl	%eax, %eax
	loop	.again
	ret

/* status_t get_cpuid_simple(cpuid_info* info, uint32 eax_register); */
ENTRY(get_cpuid_simple)
		pushl	%ebx
		pushl	%ecx
		pushl	%edx
		pushl	%esi
		movl	0x14(%esp), %esi	/* get info */
		movl	0x18(%esp), %edx	/* get eax_register */
		pushfl
		popl	%eax
		movl	%eax, %ebx 
		/* unless its xorl, C preprocessor screws it up. rwh 12/30/97 xor	$0x00200000, %eax */
		xorl	$0x00200000, %eax
		pushl	%eax
		popfl
		pushfl

		popl	%eax
		cmpl	%ebx, %eax
		jz		get_cpuid2_ex

		movl	%edx, %eax		/* put saved eax_register */
		cpuid

		movl	%eax,    (%esi)
		movl	%ebx, 0x4(%esi)	/* Vendor ID */
		movl	%edx, 0x8(%esi)
		movl	%ecx, 0xc(%esi)
		xorl	%eax, %eax		/* return B_OK */		

get_cpuid1_ex:
		popl	%esi
		popl	%edx
		popl	%ecx
		popl	%ebx
		ret

get_cpuid2_ex:
		movl	$(-1), %eax		/* B_ERROR */
		jmp		get_cpuid1_ex

