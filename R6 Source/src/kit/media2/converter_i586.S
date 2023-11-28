#include "media_asm_i586.h"


#undef L
#ifdef __ELF__
#ifdef __STDC__
#define L(body)	.L##body
#else
#define L(body)	.L/**/body
#endif
#else
#ifdef __STDC__
#define L(body) L##body
#else
#define L(body) L/**/body
#endif
#endif


        .text
// void convertBufferFloatToShort( int16 *dest, float *src, int32 count, float scale )
ENTRY (convertBufferFloatToShort)

		push %esi
		movl 16(%esp), %ecx
		movl 12(%esp), %esi
		movl 8(%esp), %edx
		flds 20(%esp)
		subl $4, %esp

L(0):
		jecxz L(1)
		flds (%esi)
		fmul %st(1), %st(0)
		fistpl (%esp)
		
		movl (%esp), %eax
		cmpl $0x00008000, %eax
		jge L(4)
		cmpl $0xffff8000, %eax
		jle L(3)
		movw %ax, (%edx)
		decl %ecx
		addl $4, %esi
		addl $2, %edx
		jmp L(0) 


L(4):
		movl $0x00007fff, %eax
		movw %ax, (%edx)
		decl %ecx
		addl $4, %esi
		addl $2, %edx
		jmp L(0) 

L(3):		
		movl $0x00008001, %eax
		movw %ax, (%edx)
		decl %ecx
		addl $4, %esi
		addl $2, %edx
		jmp L(0) 
		

L(1):
		addl $4, %esp
		pop %esi
		fstp %st(0)
		ret
