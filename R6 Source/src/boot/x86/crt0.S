#include <asm.h>

ENTRY(start)
#if 0
		call	PIC
PIC:	popl	%esi

		/* No need to worry about A20 yet because 0x400000 & 0x100000 = 0 */
		movl	$PIC, %edi
		movl	$ EXTERN(_edata), %ecx	/* copy 5 bytes extra */
		subl	%edi, %ecx
		rep
		movsb

		mov		$moved, %eax
		jmp		*%eax

moved:
#endif

		/* clear BSS */
		xorl	%eax, %eax
		movl	$ EXTERN(_edata), %edi
		movl	$ EXTERN(_end), %ecx
		subl	%edi, %ecx
		rep
		stosb

#if 0
		/* relocate GOT */
		movl	$ EXTERN(_got_), %esi
		movl	$ EXTERN(_edata), %edi
		addl	$0x0c, %esi
relocate_GOT:
		addl	%ebx, (%esi,%ebx,1)
		lodsl
		cmpl	%edi, %esi
		jnz		relocate_GOT
#endif

		jmp		EXTERN(main1)

/*
 * struct regs {
 *   uint32 eax, ecx, edx, ebx, ebp, esi, edi;
 * };
 *
 * int int86(unsigned char intnum, struct regs *in, struct regs *out);
 *
 * returns 0 if CF clear, 1 if CF set
 *
 */
ENTRY(int86)
		pushal

		call	pic
pic:	popl	%ebx
		subl	$ pic, %ebx

		movb	0x24(%esp), %al
		movb	%al, intnum(%ebx)

		movl	0x28(%esp), %edi
		movl	0x00(%edi), %eax
		movl	0x04(%edi), %ecx
		movl	0x08(%edi), %edx
		movl	0x0c(%edi), %ebx
		movl	0x10(%edi), %ebp
		movl	0x14(%edi), %esi
		movl	0x18(%edi), %edi
		
		.byte	0xcd
intnum:	.byte	0

		pushfl
		pushl	%edi
		movl	0x34(%esp), %edi
		movl	%eax, 0x00(%edi)
		movl	%ecx, 0x04(%edi)
		movl	%edx, 0x08(%edi)
		movl	%ebx, 0x0c(%edi)
		movl	%ebp, 0x10(%edi)
		movl	%esi, 0x14(%edi)
		popl	%eax
		movl	%eax, 0x18(%edi)
		popl	%eax
		movl	%eax, 0x1c(%esp)

		popal
		ret
