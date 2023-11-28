#include <asm.h>

.section	.init
ENTRY(_init)
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	COMPUTE_GOT(_init)
	pushl	8(%ebp)
	call	EXTERN(_init_one)
	addl	$4, %esp

.section	.fini
ENTRY(_fini)
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	COMPUTE_GOT(_fini)
	pushl	8(%ebp)
	call	EXTERN(_fini_one)
	addl	$4, %esp
