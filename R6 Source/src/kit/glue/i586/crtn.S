#include <asm.h>

.section .init
	pushl	8(%ebp)
	call	EXTERN(_init_two)
	addl	$4, %esp
	popl	%ebx
	movl	%ebp, %esp
	popl	%ebp
	ret

.section .fini
	pushl	8(%ebp)
	call	EXTERN(_fini_two)
	addl	$4, %esp
	popl	%ebx
	movl	%ebp, %esp
	popl	%ebp
	ret
