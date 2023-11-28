
/*
 *  cpu dependent assembly routines
 */

#include	<asm.h>

.data
	GLOBAL(cv_factor_ptr)
	GLOBAL(system_time_base_ptr)
	GLOBAL(system_real_time_base_ptr)

.text


ENTRY_EXPORT(atomic_add)
    pushl    %ebx
    movl     8(%esp),%ebx     /* load address of variable into ebx */

atomic_add1:
    movl     12(%esp),%ecx    /* load the count to increment by */
    movl     (%ebx),%eax      /* load the value of variable into eax */
    addl     %eax,%ecx        /* do the addition, result in ecx */

    lock
    cmpxchgl  %ecx,(%ebx)

    jnz      atomic_add1	  /* if zf = 0, cmpxchng failed so redo it */

    popl     %ebx
    ret


ENTRY_EXPORT(atomic_add64)
    pushl    %ebx
	pushl	 %esi			  /* save esi */
    movl     12(%esp),%esi    /* load address of variable into ebx */

atomic_add64_1:
	movl	 20(%esp),%ecx	  /* load the high order part of count to increment by */		
    movl     16(%esp),%ebx    /* load the low order part of count to increment by */
	movl	  4(%esi),%edx	  /* load high order part of value */
	movl	  0(%esi),%eax	  /* load low order part of value */			

    addl     %eax,%ebx        /* add low order, result in ebx */
	adcl	 %edx,%ecx		  /* add high order, result in ecx */

    lock
	cmpxchg8b (%esi)											 

    jnz      atomic_add64_1	  /* if zf = 0, cmpxchng failed so redo it */

	popl	 %esi		
    popl     %ebx
    ret


ENTRY_EXPORT(atomic_and)
    pushl    %ebx
    movl     8(%esp),%ebx     /* load address of variable into ebx */

atomic_and1:
    movl     12(%esp),%ecx    /* load the count to increment by */
    movl     (%ebx),%eax      /* load the value of variable into eax */
    andl     %eax,%ecx        /* do the AND, result in ecx */

    lock
    cmpxchg  %ecx,(%ebx)

    jnz      atomic_and1      /* if zf = 0, cmpxchng failed so redo it */

    popl     %ebx
    ret



ENTRY_EXPORT(atomic_or)
    pushl    %ebx
    movl     8(%esp),%ebx     /* load address of variable into ebx */

atomic_or1:
    movl     12(%esp),%ecx    /* load the count to increment by */
    movl     (%ebx),%eax      /* load the value of variable into eax */
    orl      %eax,%ecx        /* do the OR, result in ecx */

    lock
    cmpxchg  %ecx,(%ebx)

    jnz      atomic_or1       /* if zf = 0, cmpxchng failed so redo it */

    popl     %ebx
    ret


ENTRY_EXPORT(test_and_set)
    pushl    %ebx
    movl     8(%esp),%ebx     /* load address of variable into ebx */

test_and_set1:
    movl     12(%esp),%ecx    /* load the value to set the lock to */
    movl     (%ebx),%eax      /* load the value of variable into eax */

    cmpl     $0, %eax         /* see if someone else got it */
    jnz      test_and_set2    /* if so, bail out */

    lock
    cmpxchg  %ecx,(%ebx)

    jnz      test_and_set1    /* if zf = 0, cmpxchng failed so redo it */

test_and_set2:
    popl     %ebx
    ret

#
#	int32 compare_and_swap32(int32 *location, int32 oldValue, int32 newValue);
#
ENTRY_EXPORT(compare_and_swap32)
	pushl	%ebx			/* Save these */
	pushl	%edi
	movl	12(%esp), %edi	/* Get location */
	movl	16(%esp), %eax	/* Get old value */
	movl	20(%esp), %ebx	/* Get new value */
	lock
	cmpxchgl %ebx, (%edi)
	sete	%cl				/* get success */
	xorl	%eax, %eax
	movb	%cl, %al
	popl	%edi
	popl	%ebx
	ret
									
									
#
#	int32 compare_and_swap64(int64 *location, int64 oldValue, int64 newValue);
#
ENTRY_EXPORT(compare_and_swap64)
	pushl	%ebx
	pushl	%edx
	pushl	%edi
	movl	16(%esp), %edi	/* Location pointer */
	movl	20(%esp), %eax	/* Low dword of old value */
	movl	24(%esp), %edx	/* High dword of old value */
	movl	28(%esp), %ebx	/* Low dword of new value */
	movl	32(%esp), %ecx	/* High dword of new value */
	lock
	cmpxchg8b (%edi)
	sete	%cl				/* get success */
	xorl	%eax, %eax
	movb	%cl, %al
	popl	%edi
	popl	%edx
	popl	%ebx
	ret
									

ENTRY_EXPORT(system_real_time)
	pushl	%ebx
	pushl	%esi

	COMPUTE_GOT(system_real_time)
	
	movl	EXTERN_GOT(system_real_time_base_ptr), %esi
	jmp	system_time_common

/* ~~~~~~~~~~
	system_time - return the number of usec since booting
		entry:	<ret>
		exit:	[%edx,%eax] = time
~~~~~ */

ENTRY_EXPORT(system_time)
	pushl	%ebx
	pushl	%esi

	COMPUTE_GOT(system_time)

	movl	EXTERN_GOT(system_time_base_ptr), %esi
system_time_common:

	/* load 64-bit factor into %eax (low), %edx (high) */
	rdtsc						/* time in %edx,%eax */

	movl	(%esi), %esi	/* GOT -> *_time_base_ptr */
	movl	(%esi), %ecx	/* *_time_base_ptr -> time_base */

	/* substract the base */
	addl	%ecx, %eax
	movl	4(%esi), %ecx
	adc		%ecx, %edx

	movl	EXTERN_GOT(cv_factor_ptr), %esi
	movl	(%esi), %esi
	movl	(%esi), %esi
	movl	%edx, %ecx			/* save high half */
	mull	%esi /*, %eax	-- truncate %eax, but keep %edx */
	movl	%ecx, %eax
	movl	%edx, %ecx			/* save high half of low */
	mull	%esi /*, %eax*/

	/* now compute  [%edx, %eax] + [%ecx], propagating carry */
	subl	%esi, %esi			/* need zero to propagate carry */
	addl	%ecx, %eax
	adc		%esi, %edx

	popl	%esi
	popl	%ebx
	ret


ENTRY_EXPORT(get_stack_frame)
	movl	%ebp, %eax
	ret

