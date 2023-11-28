/* set new stack? */

#include <asm.h>

ENTRY(start)
		call	PIC
PIC:	popl	%esi
		movl	%esi, %ebp

		addl	$ EXTERN(_edata), %ebp
		subl	$ EXTERN(PIC), %ebp

		/* No need to worry about A20 yet because 0x200000 & 0x100000 = 0 */
		movl	$PIC, %edi
		movl	$ EXTERN(_edata), %ecx	/* copy 5 bytes extra */
		subl	%edi, %ecx
		rep
		movsb

		mov		$moved, %eax
		jmp		*%eax

moved:
		/* clear BSS */
		xorl	%eax, %eax
		movl	$ EXTERN(_edata), %edi
		movl	$ EXTERN(_end), %ecx
		subl	%edi, %ecx
		rep
		stosb

		mov		$0x400000, %eax
		push	%eax			/* destination */
		pushl	%ebp			/* source */
		call	EXTERN(gunzip)
		pop		%eax

		ret

pos:		.long	0xb8000

puts:
		pushl	%edi
		movl	(pos), %edi
		movl	8(%esp), %esi
		movb	$7, %ah
1:
		lodsb
		orb		%al, %al
		jz		1f
		stosw
		jmp		1b

1:		movl	%edi, (pos)
		popl	%edi
		ret

panic_prefix:	.ascii	"BAD BOOT IMAGE: \0"

ENTRY(panic)
		pushl	$panic_prefix
		call	puts
		pushl	8(%esp)
		call	puts
		.word	0xfeeb

brkval:	.long	0x300000

ENTRY(malloc)
		movl	(brkval), %eax
		subl	4(%esp), %eax
		movl	%eax, (brkval)
ENTRY(free)
		ret

ENTRY(memcpy)
		pushl	%esi
		pushl	%edi
		movl	0x0c(%esp), %edi
		movl	0x10(%esp), %esi
		movl	0x14(%esp), %ecx
		rep
		movsb
		popl	%edi
		popl	%esi
		ret
