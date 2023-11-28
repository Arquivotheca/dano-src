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

L(BYTE_OVER_MAX):	.float 0.00787401574803
L(WORD_OVER_MAX):	.float 0.000030518509476
L(INT_OVER_MAX):	.float 4.65661287308e-10

L(BYTE_MAX):		.float 127.0
L(BYTE_OFFSET):		.float -128.0
L(WORD_MAX):		.float 32767
L(INT_MAX):			.float 2147483500	// Remember FP only has 24-bits of precision
		
// void unsignedByteToFloatN( float *dest, unsigned char *src, float *gain, int channels, int samples )
ENTRY (unsignedByteToFloatN)
		push %ebp
		mov %esp, %ebp

		movl 20(%ebp), %edx
		shll $2, %edx
		addl $20, %edx
		subl %edx, %esp

		movl %edi, -4(%ebp)
		movl %esi, -8(%ebp)
		
		movl 8(%ebp), %edi		// dest
		movl 12(%ebp), %esi		// source
		movl 16(%ebp), %eax		// gain
		movl 20(%ebp), %edx
		movl 24(%ebp), %ecx		// samples
		
L(BYTE_PROCESS_GAIN):
		dec  %edx
		flds (%eax, %edx, 4)
		fmuls L(BYTE_OVER_MAX)
		fstps (%esp, %edx, 4)
		andl %edx, %edx
		jnz L(BYTE_PROCESS_GAIN)
		
		
L(BYTE_PROCESS_SOURCE):
		jecxz L(BYTE_DONE)
		dec %ecx
		xor %edx, %edx
		
L(BYTE_PROCESS_CHANNEL):
		movzbl (%esi), %eax
		inc %esi
		sub $127, %eax
		movl %eax, -12(%ebp)
		
//		movl %eax, (%edi)
		
		fildl -12(%ebp)
		fmuls (%esp, %edx, 4)
		inc %edx
		fstps (%edi)	
		addl $4, %edi
		cmp %edx, 20(%ebp)
		jnz L(BYTE_PROCESS_CHANNEL)
		
		jmp L(BYTE_PROCESS_SOURCE)

L(BYTE_DONE):
		movl -4(%ebp), %edi
		movl -8(%ebp), %esi
		
		
		mov %ebp, %esp
		pop %ebp
		ret


// void unsignedByteToFloat1( float *dest, unsigned char *src, float *gain, int samples )
ENTRY (unsignedByteToFloat1)
		subl $24, %esp
		movl %esi, 16(%esp)
		movl %edi, 20(%esp)
		movl 36(%esp), %eax
		movl 28(%esp), %edi
		movl 32(%esp), %esi
		movl 40(%esp), %ecx
		
		flds L(BYTE_OVER_MAX)
		fmuls (%eax)			// st0 = gain * max
		
L(BYTE_PROCESS_SOURCE1):
		jecxz L(BYTE_DONE1)
		sub $4, %ecx

		movzbl (%esi), %eax
		sub $127, %eax
		movl %eax, (%esp)
		movzbl 1(%esi), %eax
		sub $127, %eax
		movl %eax, 4(%esp)
		fildl (%esp)
		fmul %st(1), %st(0)
		
		movzbl 2(%esi), %eax
		sub $127, %eax
		movl %eax, 8(%esp)
		fildl 4(%esp)
		fmul %st(2), %st(0)
		
		movzbl 3(%esi), %eax
		addl $4, %esi		
		sub $127, %eax
		movl %eax, 12(%esp)
		fildl 8(%esp)
		fmul %st(3), %st(0)
		fildl 12(%esp)
		fmul %st(4), %st(0)

		fxch %st(3)					// st0=0  st1=2  st2=1  st3=3
		fstps (%edi)				// st0=2  st1=1  st2=3
		fxch %st(1)
		fstps 4(%edi)
		fstps 8(%edi)
		fstps 12(%edi)
		addl $16, %edi
		jmp L(BYTE_PROCESS_SOURCE1)

L(BYTE_DONE1):

		movl 20(%esp), %edi
		movl 16(%esp), %esi
		addl $24, %esp
		fstp %st(0)
		ret



// void unsignedByteToFloat2( float *dest, unsigned char *src, float *gain, int samples )
ENTRY (unsignedByteToFloat2)
		subl $24, %esp
		movl %esi, 16(%esp)
		movl %edi, 20(%esp)
		movl 36(%esp), %eax
		movl 28(%esp), %edi
		movl 32(%esp), %esi
		movl 40(%esp), %ecx
		
		flds L(BYTE_OVER_MAX)
		fmuls (%eax)			// st0 = gain * max
		flds L(BYTE_OVER_MAX)
		fmuls 4(%eax)			// st0 = gain * max
		
L(BYTE_PROCESS_SOURCE2):
		jecxz L(BYTE_DONE2)
		sub $2, %ecx

		movzbl (%esi), %eax
		sub $127, %eax
		movl %eax, (%esp)
		movzbl 1(%esi), %eax
		sub $127, %eax
		movl %eax, 4(%esp)
		fildl (%esp)
		fmul %st(2), %st(0)
		
		movzbl 2(%esi), %eax
		sub $127, %eax
		movl %eax, 8(%esp)
		fildl 4(%esp)
		fmul %st(2), %st(0)
		
		movzbl 3(%esi), %eax
		addl $4, %esi		
		sub $127, %eax
		movl %eax, 12(%esp)
		fildl 8(%esp)
		fmul %st(4), %st(0)
		fildl 12(%esp)
		fmul %st(4), %st(0)

		fxch %st(3)					// st0=0  st1=2  st2=1  st3=3
		fstps (%edi)				// st0=2  st1=1  st2=3
		fxch %st(1)
		fstps 4(%edi)
		fstps 8(%edi)
		fstps 12(%edi)
		addl $16, %edi
		jmp L(BYTE_PROCESS_SOURCE2)

L(BYTE_DONE2):

		movl 20(%esp), %edi
		movl 16(%esp), %esi
		addl $24, %esp
		fstp %st(0)
		fstp %st(0)
		ret



// void unsignedByteToFloat1to2( float *dest, unsigned char *src, float *gain, int samples )
ENTRY (unsignedByteToFloat1to2)
		subl $24, %esp
		movl %esi, 16(%esp)
		movl %edi, 20(%esp)
		movl 36(%esp), %eax
		movl 28(%esp), %edi
		movl 32(%esp), %esi
		movl 40(%esp), %ecx
		
		flds L(BYTE_OVER_MAX)
		fmuls (%eax)			// st0 = gain * max
		flds L(BYTE_OVER_MAX)
		fmuls 4(%eax)			// st0 = gain * max
		
L(BYTE_PROCESS_SOURCE12):
		jecxz L(BYTE_DONE12)
		sub $2, %ecx

		movzbl (%esi), %eax
		sub $127, %eax
		movl %eax, (%esp)
		movzbl 1(%esi), %eax
		sub $127, %eax
		movl %eax, 4(%esp)
		fildl (%esp)
		fmul %st(2), %st(0)
		fildl (%esp)
		fmul %st(2), %st(0)
		fildl 4(%esp)
		fmul %st(4), %st(0)
		fildl 4(%esp)
		fmul %st(4), %st(0)
		addl $2, %esi
		
		fxch %st(3)					// st0=0  st1=2  st2=1  st3=3
		fstps (%edi)				// st0=2  st1=1  st2=3
		fxch %st(1)
		fstps 4(%edi)
		fstps 8(%edi)
		fstps 12(%edi)
		addl $16, %edi
		jmp L(BYTE_PROCESS_SOURCE12)

L(BYTE_DONE12):

		movl 20(%esp), %edi
		movl 16(%esp), %esi
		addl $24, %esp
		fstp %st(0)
		fstp %st(0)
		ret



// void wordToFloatN( float *dest, unsigned char *src, float *gain, int channels, int samples )
ENTRY (wordToFloatN)
		push %ebp
		mov %esp, %ebp

		movl 20(%ebp), %edx
		shll $2, %edx
		addl $20, %edx
		subl %edx, %esp

		movl %edi, -4(%ebp)
		movl %esi, -8(%ebp)
		
		movl 8(%ebp), %edi		// dest
		movl 12(%ebp), %esi		// source
		movl 16(%ebp), %eax		// gain
		movl 20(%ebp), %edx
		movl 24(%ebp), %ecx		// samples
		
L(WORD_PROCESS_GAIN):
		dec  %edx
		flds (%eax, %edx, 4)
		fmuls L(WORD_OVER_MAX)
		fstps (%esp, %edx, 4)
		andl %edx, %edx
		jnz L(WORD_PROCESS_GAIN)
		
		
L(WORD_PROCESS_SOURCE):
		jecxz L(WORD_DONE)
		dec %ecx
		xor %edx, %edx
		
L(WORD_PROCESS_CHANNEL):
		fild (%esi)
		add $2, %esi
		fmuls (%esp, %edx, 4)
		inc %edx
		fstps (%edi)	
		addl $4, %edi
		cmp %edx, 20(%ebp)
		jnz L(WORD_PROCESS_CHANNEL)
		jmp L(WORD_PROCESS_SOURCE)

L(WORD_DONE):
		movl -4(%ebp), %edi
		movl -8(%ebp), %esi
		
		
		mov %ebp, %esp
		pop %ebp
		ret


// void wordToFloat1( float *dest, unsigned char *src, float *gain, int samples )
ENTRY (wordToFloat1)
		subl $24, %esp
		movl %esi, 16(%esp)
		movl %edi, 20(%esp)
		movl 36(%esp), %eax
		movl 28(%esp), %edi
		movl 32(%esp), %esi
		movl 40(%esp), %ecx
		
		flds L(WORD_OVER_MAX)
		fmuls (%eax)			// st0 = gain * max
		
L(WORD_PROCESS_SOURCE1):
		jecxz L(WORD_DONE1)
		sub $4, %ecx

		fild (%esi)
		fmul %st(1), %st(0)
		fild 2(%esi)
		fmul %st(2), %st(0)
		fild 4(%esi)
		fmul %st(3), %st(0)
		fild 6(%esi)
		fmul %st(4), %st(0)
		addl $8, %esi

		fxch %st(3)					// st0=0  st1=2  st2=1  st3=3
		fstps (%edi)				// st0=2  st1=1  st2=3
		fxch %st(1)
		fstps 4(%edi)
		fstps 8(%edi)
		fstps 12(%edi)
		addl $16, %edi
		jmp L(WORD_PROCESS_SOURCE1)

L(WORD_DONE1):

		movl 20(%esp), %edi
		movl 16(%esp), %esi
		addl $24, %esp
		fstp %st(0)
		ret


// void wordToFloat2( float *dest, unsigned char *src, float *gain, int samples )
ENTRY (wordToFloat2)
		subl $24, %esp
		movl %esi, 16(%esp)
		movl %edi, 20(%esp)
		movl 36(%esp), %eax
		movl 28(%esp), %edi
		movl 32(%esp), %esi
		movl 40(%esp), %ecx
		
		flds L(WORD_OVER_MAX)
		fmuls (%eax)			// st0 = gain * max
		flds L(WORD_OVER_MAX)
		fmuls 4(%eax)			// st0 = gain * max
		
L(WORD_PROCESS_SOURCE2):
		jecxz L(WORD_DONE2)
		sub $2, %ecx

		fild (%esi)
		fmul %st(2), %st(0)
		fild 2(%esi)
		fmul %st(2), %st(0)
		fild 4(%esi)
		fmul %st(4), %st(0)
		fild 6(%esi)
		fmul %st(4), %st(0)
		addl $8, %esi

		fxch %st(3)					// st0=0  st1=2  st2=1  st3=3
		fstps (%edi)				// st0=2  st1=1  st2=3
		fxch %st(1)
		fstps 4(%edi)
		fstps 8(%edi)
		fstps 12(%edi)
		addl $16, %edi
		jmp L(WORD_PROCESS_SOURCE2)

L(WORD_DONE2):

		movl 20(%esp), %edi
		movl 16(%esp), %esi
		addl $24, %esp
		fstp %st(0)
		fstp %st(0)
		ret


// void wordToFloat1to2( float *dest, unsigned char *src, float *gain, int samples )
ENTRY (wordToFloat1to2)
		subl $24, %esp
		movl %esi, 16(%esp)
		movl %edi, 20(%esp)
		movl 36(%esp), %eax
		movl 28(%esp), %edi
		movl 32(%esp), %esi
		movl 40(%esp), %ecx
		
		flds L(WORD_OVER_MAX)
		fmuls (%eax)			// st0 = gain * max
		flds L(WORD_OVER_MAX)
		fmuls 4(%eax)			// st0 = gain * max
		
L(WORD_PROCESS_SOURCE12):
		jecxz L(WORD_DONE12)
		sub $2, %ecx

		fild (%esi)
		fmul %st(2), %st(0)
		fild (%esi)
		fmul %st(2), %st(0)
		fild 2(%esi)
		fmul %st(4), %st(0)
		fild 2(%esi)
		fmul %st(4), %st(0)
		addl $4, %esi

		fxch %st(3)					// st0=0  st1=2  st2=1  st3=3
		fstps (%edi)				// st0=2  st1=1  st2=3
		fxch %st(1)
		fstps 4(%edi)
		fstps 8(%edi)
		fstps 12(%edi)
		addl $16, %edi
		jmp L(WORD_PROCESS_SOURCE12)

L(WORD_DONE12):

		movl 20(%esp), %edi
		movl 16(%esp), %esi
		addl $24, %esp
		fstp %st(0)
		fstp %st(0)
		ret



// void intToFloatN( float *dest, unsigned char *src, float *gain, int channels, int samples )
ENTRY (intToFloatN)
		push %ebp
		mov %esp, %ebp

		movl 20(%ebp), %edx
		shll $2, %edx
		addl $20, %edx
		subl %edx, %esp

		movl %edi, -4(%ebp)
		movl %esi, -8(%ebp)
		
		movl 8(%ebp), %edi		// dest
		movl 12(%ebp), %esi		// source
		movl 16(%ebp), %eax		// gain
		movl 20(%ebp), %edx
		movl 24(%ebp), %ecx		// samples
		
L(INT_PROCESS_GAIN):
		dec  %edx
		flds (%eax, %edx, 4)
		fmuls L(INT_OVER_MAX)
		fstps (%esp, %edx, 4)
		andl %edx, %edx
		jnz L(INT_PROCESS_GAIN)
		
		
L(INT_PROCESS_SOURCE):
		jecxz L(INT_DONE)
		dec %ecx
		xor %edx, %edx
		
L(INT_PROCESS_CHANNEL):
		fildl (%esi)
		add $4, %esi
		fmuls (%esp, %edx, 4)
		inc %edx
		fstps (%edi)	
		addl $4, %edi
		cmp %edx, 20(%ebp)
		jnz L(INT_PROCESS_CHANNEL)
		jmp L(INT_PROCESS_SOURCE)

L(INT_DONE):
		movl -4(%ebp), %edi
		movl -8(%ebp), %esi
		
		
		mov %ebp, %esp
		pop %ebp
		ret


// void intToFloat1( float *dest, unsigned char *src, float *gain, int samples )
ENTRY (intToFloat1)
		subl $8, %esp
		movl %esi, (%esp)
		movl %edi, 4(%esp)
		movl 20(%esp), %eax
		movl 12(%esp), %edi
		movl 16(%esp), %esi
		movl 24(%esp), %ecx
		
		flds L(INT_OVER_MAX)
		fmuls (%eax)			// st0 = gain * max
		
L(INT_PROCESS_SOURCE1):
		jecxz L(INT_DONE1)
		sub $4, %ecx

		fildl (%esi)
		fmul %st(1), %st(0)
		fildl 4(%esi)
		fmul %st(2), %st(0)
		fildl 8(%esi)
		fmul %st(3), %st(0)
		fildl 12(%esi)
		addl $16, %esi
		
		fmul %st(4), %st(0)
		fxch %st(3)					// st0=0  st1=2  st2=1  st3=3
		fstps (%edi)				// st0=2  st1=1  st2=3
		fxch %st(1)
		fstps 4(%edi)
		fstps 8(%edi)
		fstps 12(%edi)
		addl $16, %edi
		jmp L(INT_PROCESS_SOURCE1)

L(INT_DONE1):

		movl 4(%esp), %edi
		movl (%esp), %esi
		addl $8, %esp
		fstp %st(0)
		ret


// void intToFloat2( float *dest, unsigned char *src, float *gain, int samples )
ENTRY (intToFloat2)
		subl $8, %esp
		movl %esi, (%esp)
		movl %edi, 4(%esp)
		movl 20(%esp), %eax
		movl 12(%esp), %edi
		movl 16(%esp), %esi
		movl 24(%esp), %ecx
		
		flds L(INT_OVER_MAX)
		fmuls (%eax)			// st0 = gain * max
		flds L(INT_OVER_MAX)
		fmuls 4(%eax)			// st0 = gain * max
		
L(INT_PROCESS_SOURCE2):
		jecxz L(INT_DONE2)
		sub $2, %ecx

		fildl (%esi)
		fmul %st(2), %st(0)
		fildl 4(%esi)
		fmul %st(2), %st(0)
		fildl 8(%esi)
		fmul %st(4), %st(0)
		fildl 12(%esi)
		addl $16, %esi
		fmul %st(4), %st(0)

		fxch %st(3)					// st0=0  st1=2  st2=1  st3=3
		fstps (%edi)				// st0=2  st1=1  st2=3
		fxch %st(1)
		fstps 4(%edi)
		fstps 8(%edi)
		fstps 12(%edi)
		addl $16, %edi
		jmp L(INT_PROCESS_SOURCE2)

L(INT_DONE2):

		movl 4(%esp), %edi
		movl (%esp), %esi
		addl $8, %esp
		fstp %st(0)
		fstp %st(0)
		ret


// void intToFloat1to2( float *dest, unsigned char *src, float *gain, int samples )
ENTRY (intToFloat1to2)
		subl $8, %esp
		movl %esi, (%esp)
		movl %edi, 4(%esp)
		movl 20(%esp), %eax
		movl 12(%esp), %edi
		movl 16(%esp), %esi
		movl 24(%esp), %ecx
		
		flds L(INT_OVER_MAX)
		fmuls (%eax)			// st0 = gain * max
		flds L(INT_OVER_MAX)
		fmuls 4(%eax)			// st0 = gain * max
		
L(INT_PROCESS_SOURCE12):
		jecxz L(INT_DONE12)
		sub $2, %ecx

		fildl (%esi)
		fmul %st(2), %st(0)
		fildl (%esi)
		fmul %st(2), %st(0)
		fildl 4(%esi)
		fmul %st(4), %st(0)
		fildl 4(%esi)
		addl $8, %esi
		fmul %st(4), %st(0)

		fxch %st(3)					// st0=0  st1=2  st2=1  st3=3
		fstps (%edi)				// st0=2  st1=1  st2=3
		fxch %st(1)
		fstps 4(%edi)
		fstps 8(%edi)
		fstps 12(%edi)
		addl $16, %edi
		jmp L(INT_PROCESS_SOURCE12)

L(INT_DONE12):

		movl 4(%esp), %edi
		movl (%esp), %esi
		addl $8, %esp
		fstp %st(0)
		fstp %st(0)
		ret



// void floatToFloatN( float *dest, float *src, float *gain, int channels, int frames )
ENTRY (floatToFloatN)
		push %ebp
		mov %esp, %ebp

		movl 20(%ebp), %edx
		shll $2, %edx
		addl $20, %edx
		subl %edx, %esp

		movl %edi, -4(%ebp)
		movl %esi, -8(%ebp)
		
		movl 8(%ebp), %edi		// dest
		movl 12(%ebp), %esi		// source
		movl 16(%ebp), %eax		// gain
		movl 24(%ebp), %ecx		// samples
		
L(FLOAT_PROCESS_SOURCE):
		jecxz L(FLOAT_DONE)
		dec %ecx
		xor %edx, %edx
		
L(FLOAT_PROCESS_CHANNEL):
		flds (%esi)
		add $4, %esi
		fmuls (%eax, %edx, 4)
		inc %edx
		fstps (%edi)	
		addl $4, %edi
		cmp %edx, 20(%ebp)
		jnz L(FLOAT_PROCESS_CHANNEL)
		jmp L(FLOAT_PROCESS_SOURCE)

L(FLOAT_DONE):
		movl -4(%ebp), %edi
		movl -8(%ebp), %esi
		
		
		mov %ebp, %esp
		pop %ebp
		ret


// void floatToFloat1( float *dest, unsigned char *src, float *gain, int samples )
ENTRY (floatToFloat1)
		subl $8, %esp
		movl %esi, (%esp)
		movl %edi, 4(%esp)
		movl 20(%esp), %eax
		movl 12(%esp), %edi
		movl 16(%esp), %esi
		movl 24(%esp), %ecx
		flds (%eax)
		
L(FLOAT_PROCESS_SOURCE1):
		jecxz L(FLOAT_DONE1)
		sub $4, %ecx

		flds (%esi)
		fmul %st(1), %st(0)
		flds 4(%esi)
		fmul %st(2), %st(0)
		flds 8(%esi)
		fmul %st(3), %st(0)
		flds 12(%esi)
		addl $16, %esi
		
		fmul %st(4), %st(0)
		fxch %st(3)					// st0=0  st1=2  st2=1  st3=3
		fstps (%edi)				// st0=2  st1=1  st2=3
		fxch %st(1)
		fstps 4(%edi)
		fstps 8(%edi)
		fstps 12(%edi)
		addl $16, %edi
		jmp L(FLOAT_PROCESS_SOURCE1)

L(FLOAT_DONE1):

		movl 4(%esp), %edi
		movl (%esp), %esi
		addl $8, %esp
		fstp %st(0)
		ret


// void floatToFloat2( float *dest, unsigned char *src, float *gain, int samples )
ENTRY (floatToFloat2)
		subl $8, %esp
		movl %esi, (%esp)
		movl %edi, 4(%esp)
		movl 20(%esp), %eax
		movl 12(%esp), %edi
		movl 16(%esp), %esi
		movl 24(%esp), %ecx
		
		flds (%eax)			// st0 = gain * max
		flds 4(%eax)			// st0 = gain * max
		
L(FLOAT_PROCESS_SOURCE2):
		jecxz L(FLOAT_DONE2)
		sub $2, %ecx

		flds (%esi)
		fmul %st(2), %st(0)
		flds 4(%esi)
		fmul %st(2), %st(0)
		flds 8(%esi)
		fmul %st(4), %st(0)
		flds 12(%esi)
		addl $16, %esi
		fmul %st(4), %st(0)

		fxch %st(3)					// st0=0  st1=2  st2=1  st3=3
		fstps (%edi)				// st0=2  st1=1  st2=3
		fxch %st(1)
		fstps 4(%edi)
		fstps 8(%edi)
		fstps 12(%edi)
		addl $16, %edi
		jmp L(FLOAT_PROCESS_SOURCE2)

L(FLOAT_DONE2):

		movl 4(%esp), %edi
		movl (%esp), %esi
		addl $8, %esp
		fstp %st(0)
		fstp %st(0)
		ret


// void floatToFloat1to2( float *dest, unsigned char *src, float *gain, int samples )
ENTRY (floatToFloat1to2)
		subl $8, %esp
		movl %esi, (%esp)
		movl %edi, 4(%esp)
		movl 20(%esp), %eax
		movl 12(%esp), %edi
		movl 16(%esp), %esi
		movl 24(%esp), %ecx
		
		flds (%eax)			// st0 = gain * max
		flds 4(%eax)			// st0 = gain * max
		
L(FLOAT_PROCESS_SOURCE12):
		jecxz L(FLOAT_DONE12)
		sub $2, %ecx

		flds (%esi)
		fmul %st(2), %st(0)
		flds (%esi)
		fmul %st(2), %st(0)
		flds 4(%esi)
		fmul %st(4), %st(0)
		flds 4(%esi)
		addl $8, %esi
		fmul %st(4), %st(0)

		fxch %st(3)					// st0=0  st1=2  st2=1  st3=3
		fstps (%edi)				// st0=2  st1=1  st2=3
		fxch %st(1)
		fstps 4(%edi)
		fstps 8(%edi)
		fstps 12(%edi)
		addl $16, %edi
		jmp L(FLOAT_PROCESS_SOURCE12)

L(FLOAT_DONE12):

		movl 4(%esp), %edi
		movl (%esp), %esi
		addl $8, %esp
		fstp %st(0)
		fstp %st(0)
		ret


/***********************************************************************************************/

// void unsignedByteToFloatAccumN( float *dest, unsigned char *src, float *gain, int channels, int samples )
ENTRY (unsignedByteToFloatAccumN)
		push %ebp
		mov %esp, %ebp

		movl 20(%ebp), %edx
		shll $2, %edx
		addl $20, %edx
		subl %edx, %esp

		movl %edi, -4(%ebp)
		movl %esi, -8(%ebp)
		
		movl 8(%ebp), %edi		// dest
		movl 12(%ebp), %esi		// source
		movl 16(%ebp), %eax		// gain
		movl 20(%ebp), %edx
		movl 24(%ebp), %ecx		// samples
		
L(BYTE_PROCESS_GAINA):
		dec  %edx
		flds (%eax, %edx, 4)
		fmuls L(BYTE_OVER_MAX)
		fstps (%esp, %edx, 4)
		andl %edx, %edx
		jnz L(BYTE_PROCESS_GAINA)
		
		
L(BYTE_PROCESS_SOURCEA):
		jecxz L(BYTE_DONEA)
		dec %ecx
		xor %edx, %edx
		
L(BYTE_PROCESS_CHANNELA):
		movzbl (%esi), %eax
		inc %esi
		sub $127, %eax
		movl %eax, -12(%ebp)
		fildl -12(%ebp)
		fmuls (%esp, %edx, 4)
		fadds (%edi)
		inc %edx
		fstps (%edi)	
		addl $4, %edi
		cmp %edx, 20(%ebp)
		jnz L(BYTE_PROCESS_CHANNELA)
		
		jmp L(BYTE_PROCESS_SOURCEA)

L(BYTE_DONEA):
		movl -4(%ebp), %edi
		movl -8(%ebp), %esi
		
		
		mov %ebp, %esp
		pop %ebp
		ret


// void unsignedByteToFloatAccum1( float *dest, unsigned char *src, float *gain, int samples )
ENTRY (unsignedByteToFloatAccum1)
		subl $24, %esp
		movl %esi, 16(%esp)
		movl %edi, 20(%esp)
		movl 36(%esp), %eax
		movl 28(%esp), %edi
		movl 32(%esp), %esi
		movl 40(%esp), %ecx
		
		flds L(BYTE_OVER_MAX)
		fmuls (%eax)			// st0 = gain * max
		
L(BYTE_PROCESS_SOURCE1A):
		jecxz L(BYTE_DONE1A)
		sub $4, %ecx

		movzbl (%esi), %eax
		sub $127, %eax
		movl %eax, (%esp)
		movzbl 1(%esi), %eax
		sub $127, %eax
		movl %eax, 4(%esp)
		fildl (%esp)
		fmul %st(1), %st(0)
		
		movzbl 2(%esi), %eax
		sub $127, %eax
		movl %eax, 8(%esp)
		fildl 4(%esp)
		fmul %st(2), %st(0)
		
		movzbl 3(%esi), %eax
		addl $4, %esi		
		sub $127, %eax
		movl %eax, 12(%esp)
		fildl 8(%esp)
		fmul %st(3), %st(0)
		fildl 12(%esp)
		fmul %st(4), %st(0)

		flds (%edi)
		faddp %st(0),%st(4)
		flds 4(%edi)
		faddp %st(0),%st(3)
		flds 8(%edi)
		faddp %st(0),%st(2)
		flds 12(%edi)
		faddp %st(0),%st(1)

		fxch %st(3)					// st0=0  st1=2  st2=1  st3=3
		fstps (%edi)				// st0=2  st1=1  st2=3
		fxch %st(1)
		fstps 4(%edi)
		fstps 8(%edi)
		fstps 12(%edi)
		addl $16, %edi
		jmp L(BYTE_PROCESS_SOURCE1A)

L(BYTE_DONE1A):

		movl 20(%esp), %edi
		movl 16(%esp), %esi
		addl $24, %esp
		fstp %st(0)
		ret


// void unsignedByteToFloatAccum2( float *dest, unsigned char *src, float *gain, int samples )
ENTRY (unsignedByteToFloatAccum2)
		subl $24, %esp
		movl %esi, 16(%esp)
		movl %edi, 20(%esp)
		movl 36(%esp), %eax
		movl 28(%esp), %edi
		movl 32(%esp), %esi
		movl 40(%esp), %ecx
		
		flds L(BYTE_OVER_MAX)
		fmuls (%eax)			// st0 = gain * max
		flds L(BYTE_OVER_MAX)
		fmuls 4(%eax)			// st0 = gain * max
		
L(BYTE_PROCESS_SOURCE2A):
		jecxz L(BYTE_DONE2A)
		sub $2, %ecx

		movzbl (%esi), %eax
		sub $127, %eax
		movl %eax, (%esp)
		movzbl 1(%esi), %eax
		sub $127, %eax
		movl %eax, 4(%esp)
		fildl (%esp)
		fmul %st(2), %st(0)
		
		movzbl 2(%esi), %eax
		sub $127, %eax
		movl %eax, 8(%esp)
		fildl 4(%esp)
		fmul %st(2), %st(0)
		
		movzbl 3(%esi), %eax
		addl $4, %esi		
		sub $127, %eax
		movl %eax, 12(%esp)
		fildl 8(%esp)
		fmul %st(4), %st(0)
		fildl 12(%esp)
		fmul %st(4), %st(0)

		flds (%edi)
		faddp %st(0),%st(4)
		flds 4(%edi)
		faddp %st(0),%st(3)
		flds 8(%edi)
		faddp %st(0),%st(2)
		flds 12(%edi)
		faddp %st(0),%st(1)

		fxch %st(3)					// st0=0  st1=2  st2=1  st3=3
		fstps (%edi)				// st0=2  st1=1  st2=3
		fxch %st(1)
		fstps 4(%edi)
		fstps 8(%edi)
		fstps 12(%edi)
		addl $16, %edi
		jmp L(BYTE_PROCESS_SOURCE2A)

L(BYTE_DONE2A):

		movl 20(%esp), %edi
		movl 16(%esp), %esi
		addl $24, %esp
		fstp %st(0)
		fstp %st(0)
		ret


// void unsignedByteToFloatAccum1to2( float *dest, unsigned char *src, float *gain, int samples )
ENTRY (unsignedByteToFloatAccum1to2)
		subl $24, %esp
		movl %esi, 16(%esp)
		movl %edi, 20(%esp)
		movl 36(%esp), %eax
		movl 28(%esp), %edi
		movl 32(%esp), %esi
		movl 40(%esp), %ecx
		
		flds L(BYTE_OVER_MAX)
		fmuls (%eax)			// st0 = gain * max
		flds L(BYTE_OVER_MAX)
		fmuls 4(%eax)			// st0 = gain * max
		
L(BYTE_PROCESS_SOURCE12A):
		jecxz L(BYTE_DONE12A)
		sub $2, %ecx

		movzbl (%esi), %eax
		sub $127, %eax
		movl %eax, (%esp)
		movzbl 1(%esi), %eax
		sub $127, %eax
		movl %eax, 4(%esp)
		addl $2, %esi

		fildl (%esp)
		fmul %st(2), %st(0)
		fildl (%esp)
		fmul %st(2), %st(0)
		fildl 4(%esp)
		fmul %st(4), %st(0)
		fildl 4(%esp)
		fmul %st(4), %st(0)

		flds (%edi)
		faddp %st(0),%st(4)
		flds 4(%edi)
		faddp %st(0),%st(3)
		flds 8(%edi)
		faddp %st(0),%st(2)
		flds 12(%edi)
		faddp %st(0),%st(1)

		fxch %st(3)					// st0=0  st1=2  st2=1  st3=3
		fstps (%edi)				// st0=2  st1=1  st2=3
		fxch %st(1)
		fstps 4(%edi)
		fstps 8(%edi)
		fstps 12(%edi)
		addl $16, %edi
		jmp L(BYTE_PROCESS_SOURCE12A)

L(BYTE_DONE12A):

		movl 20(%esp), %edi
		movl 16(%esp), %esi
		addl $24, %esp
		fstp %st(0)
		fstp %st(0)
		ret




// void wordToFloatAccumN( float *dest, unsigned char *src, float *gain, int channels, int samples )
ENTRY (wordToFloatAccumN)
		push %ebp
		mov %esp, %ebp

		movl 20(%ebp), %edx
		shll $2, %edx
		addl $20, %edx
		subl %edx, %esp

		movl %edi, -4(%ebp)
		movl %esi, -8(%ebp)
		
		movl 8(%ebp), %edi		// dest
		movl 12(%ebp), %esi		// source
		movl 16(%ebp), %eax		// gain
		movl 20(%ebp), %edx
		movl 24(%ebp), %ecx		// samples
		
L(WORD_PROCESS_GAINA):
		dec  %edx
		flds (%eax, %edx, 4)
		fmuls L(WORD_OVER_MAX)
		fstps (%esp, %edx, 4)
		andl %edx, %edx
		jnz L(WORD_PROCESS_GAINA)
		
		
L(WORD_PROCESS_SOURCEA):
		jecxz L(WORD_DONEA)
		dec %ecx
		xor %edx, %edx
		
L(WORD_PROCESS_CHANNELA):
		fild (%esi)
		add $2, %esi
		fmuls (%esp, %edx, 4)
		inc %edx
		fadds (%edi)
		fstps (%edi)	
		addl $4, %edi
		cmp %edx, 20(%ebp)
		jnz L(WORD_PROCESS_CHANNELA)
		jmp L(WORD_PROCESS_SOURCEA)

L(WORD_DONEA):
		movl -4(%ebp), %edi
		movl -8(%ebp), %esi
		
		
		mov %ebp, %esp
		pop %ebp
		ret


// void wordToFloatAccum1( float *dest, unsigned char *src, float *gain, int samples )
ENTRY (wordToFloatAccum1)
		subl $24, %esp
		movl %esi, 16(%esp)
		movl %edi, 20(%esp)
		movl 36(%esp), %eax
		movl 28(%esp), %edi
		movl 32(%esp), %esi
		movl 40(%esp), %ecx
		
		flds L(WORD_OVER_MAX)
		fmuls (%eax)			// st0 = gain * max
		
L(WORD_PROCESS_SOURCE1A):
		jecxz L(WORD_DONE1A)
		sub $4, %ecx

		fild (%esi)
		fmul %st(1), %st(0)
		fild 2(%esi)
		fmul %st(2), %st(0)
		fild 4(%esi)
		fmul %st(3), %st(0)
		fild 6(%esi)
		fmul %st(4), %st(0)
		addl $8, %esi

		flds (%edi)
		faddp %st(0),%st(4)
		flds 4(%edi)
		faddp %st(0),%st(3)
		flds 8(%edi)
		faddp %st(0),%st(2)
		flds 12(%edi)
		faddp %st(0),%st(1)

		fxch %st(3)					// st0=0  st1=2  st2=1  st3=3
		fstps (%edi)				// st0=2  st1=1  st2=3
		fxch %st(1)
		fstps 4(%edi)
		fstps 8(%edi)
		fstps 12(%edi)
		addl $16, %edi
		jmp L(WORD_PROCESS_SOURCE1A)

L(WORD_DONE1A):

		movl 20(%esp), %edi
		movl 16(%esp), %esi
		addl $24, %esp
		fstp %st(0)
		ret


// void wordToFloatAccum2( float *dest, unsigned char *src, float *gain, int samples )
ENTRY (wordToFloatAccum2)
		subl $24, %esp
		movl %esi, 16(%esp)
		movl %edi, 20(%esp)
		movl 36(%esp), %eax
		movl 28(%esp), %edi
		movl 32(%esp), %esi
		movl 40(%esp), %ecx
		
		flds L(WORD_OVER_MAX)
		fmuls (%eax)			// st0 = gain * max
		flds L(WORD_OVER_MAX)
		fmuls 4(%eax)			// st0 = gain * max
		
L(WORD_PROCESS_SOURCE2A):
		jecxz L(WORD_DONE2A)
		sub $2, %ecx

		fild (%esi)
		fmul %st(2), %st(0)
		fild 2(%esi)
		fmul %st(2), %st(0)
		fild 4(%esi)
		fmul %st(4), %st(0)
		fild 6(%esi)
		fmul %st(4), %st(0)
		addl $8, %esi

		flds (%edi)
		faddp %st(0),%st(4)
		flds 4(%edi)
		faddp %st(0),%st(3)
		flds 8(%edi)
		faddp %st(0),%st(2)
		flds 12(%edi)
		faddp %st(0),%st(1)

		fxch %st(3)					// st0=0  st1=2  st2=1  st3=3
		fstps (%edi)				// st0=2  st1=1  st2=3
		fxch %st(1)
		fstps 4(%edi)
		fstps 8(%edi)
		fstps 12(%edi)
		addl $16, %edi
		jmp L(WORD_PROCESS_SOURCE2A)

L(WORD_DONE2A):

		movl 20(%esp), %edi
		movl 16(%esp), %esi
		addl $24, %esp
		fstp %st(0)
		fstp %st(0)
		ret


// void wordToFloatAccum1to2( float *dest, unsigned char *src, float *gain, int samples )
ENTRY (wordToFloatAccum1to2)
		subl $24, %esp
		movl %esi, 16(%esp)
		movl %edi, 20(%esp)
		movl 36(%esp), %eax
		movl 28(%esp), %edi
		movl 32(%esp), %esi
		movl 40(%esp), %ecx
		
		flds L(WORD_OVER_MAX)
		fmuls (%eax)			// st0 = gain * max
		flds L(WORD_OVER_MAX)
		fmuls 4(%eax)			// st0 = gain * max
		
L(WORD_PROCESS_SOURCE12A):
		jecxz L(WORD_DONE12A)
		sub $2, %ecx

		fild (%esi)
		fmul %st(2), %st(0)
		fild (%esi)
		fmul %st(2), %st(0)
		fild 2(%esi)
		fmul %st(4), %st(0)
		fild 2(%esi)
		fmul %st(4), %st(0)
		addl $4, %esi

		flds (%edi)
		faddp %st(0),%st(4)
		flds 4(%edi)
		faddp %st(0),%st(3)
		flds 8(%edi)
		faddp %st(0),%st(2)
		flds 12(%edi)
		faddp %st(0),%st(1)

		fxch %st(3)					// st0=0  st1=2  st2=1  st3=3
		fstps (%edi)				// st0=2  st1=1  st2=3
		fxch %st(1)
		fstps 4(%edi)
		fstps 8(%edi)
		fstps 12(%edi)
		addl $16, %edi
		jmp L(WORD_PROCESS_SOURCE12A)

L(WORD_DONE12A):

		movl 20(%esp), %edi
		movl 16(%esp), %esi
		addl $24, %esp
		fstp %st(0)
		fstp %st(0)
		ret




// void intToFloatAccumN( float *dest, unsigned char *src, float *gain, int channels, int samples )
ENTRY (intToFloatAccumN)
		push %ebp
		mov %esp, %ebp

		movl 20(%ebp), %edx
		shll $2, %edx
		addl $20, %edx
		subl %edx, %esp

		movl %edi, -4(%ebp)
		movl %esi, -8(%ebp)
		
		movl 8(%ebp), %edi		// dest
		movl 12(%ebp), %esi		// source
		movl 16(%ebp), %eax		// gain
		movl 20(%ebp), %edx
		movl 24(%ebp), %ecx		// samples
		
L(INT_PROCESS_GAINA):
		dec  %edx
		flds (%eax, %edx, 4)
		fmuls L(INT_OVER_MAX)
		fstps (%esp, %edx, 4)
		andl %edx, %edx
		jnz L(INT_PROCESS_GAINA)
		
		
L(INT_PROCESS_SOURCEA):
		jecxz L(INT_DONEA)
		dec %ecx
		xor %edx, %edx
		
L(INT_PROCESS_CHANNELA):
		fildl (%esi)
		add $4, %esi
		fmuls (%esp, %edx, 4)
		inc %edx
		fadds (%edi)
		fstps (%edi)	
		addl $4, %edi
		cmp %edx, 20(%ebp)
		jnz L(INT_PROCESS_CHANNELA)
		jmp L(INT_PROCESS_SOURCEA)

L(INT_DONEA):
		movl -4(%ebp), %edi
		movl -8(%ebp), %esi
		
		
		mov %ebp, %esp
		pop %ebp
		ret


// void intToFloatAccum1( float *dest, unsigned char *src, float *gain, int samples )
ENTRY (intToFloatAccum1)
		subl $8, %esp
		movl %esi, (%esp)
		movl %edi, 4(%esp)
		movl 20(%esp), %eax
		movl 12(%esp), %edi
		movl 16(%esp), %esi
		movl 24(%esp), %ecx
		
		flds L(INT_OVER_MAX)
		fmuls (%eax)			// st0 = gain * max
		
L(INT_PROCESS_SOURCE1A):
		jecxz L(INT_DONE1A)
		sub $4, %ecx

		fildl (%esi)
		fmul %st(1), %st(0)
		fildl 4(%esi)
		fmul %st(2), %st(0)
		fildl 8(%esi)
		fmul %st(3), %st(0)
		fildl 12(%esi)
		addl $16, %esi
		fmul %st(4), %st(0)

		flds (%edi)
		faddp %st(0),%st(4)
		flds 4(%edi)
		faddp %st(0),%st(3)
		flds 8(%edi)
		faddp %st(0),%st(2)
		flds 12(%edi)
		faddp %st(0),%st(1)

		fxch %st(3)					// st0=0  st1=2  st2=1  st3=3
		fstps (%edi)				// st0=2  st1=1  st2=3
		fxch %st(1)
		fstps 4(%edi)
		fstps 8(%edi)
		fstps 12(%edi)
		addl $16, %edi
		jmp L(INT_PROCESS_SOURCE1A)

L(INT_DONE1A):

		movl 4(%esp), %edi
		movl (%esp), %esi
		addl $8, %esp
		fstp %st(0)
		ret


// void intToFloatAccum2( float *dest, unsigned char *src, float *gain, int samples )
ENTRY (intToFloatAccum2)
		subl $8, %esp
		movl %esi, (%esp)
		movl %edi, 4(%esp)
		movl 20(%esp), %eax
		movl 12(%esp), %edi
		movl 16(%esp), %esi
		movl 24(%esp), %ecx
		
		flds L(INT_OVER_MAX)
		fmuls (%eax)			// st0 = gain * max
		flds L(INT_OVER_MAX)
		fmuls 4(%eax)			// st0 = gain * max
		
L(INT_PROCESS_SOURCE2A):
		jecxz L(INT_DONE2A)
		sub $2, %ecx

		fildl (%esi)
		fmul %st(2), %st(0)
		fildl 4(%esi)
		fmul %st(2), %st(0)
		fildl 8(%esi)
		fmul %st(4), %st(0)
		fildl 12(%esi)
		addl $16, %esi
		fmul %st(4), %st(0)

		flds (%edi)
		faddp %st(0),%st(4)
		flds 4(%edi)
		faddp %st(0),%st(3)
		flds 8(%edi)
		faddp %st(0),%st(2)
		flds 12(%edi)
		faddp %st(0),%st(1)

		fxch %st(3)					// st0=0  st1=2  st2=1  st3=3
		fstps (%edi)				// st0=2  st1=1  st2=3
		fxch %st(1)
		fstps 4(%edi)
		fstps 8(%edi)
		fstps 12(%edi)
		addl $16, %edi
		jmp L(INT_PROCESS_SOURCE2A)

L(INT_DONE2A):

		movl 4(%esp), %edi
		movl (%esp), %esi
		addl $8, %esp
		fstp %st(0)
		fstp %st(0)
		ret




// void intToFloatAccum1to2( float *dest, unsigned char *src, float *gain, int samples )
ENTRY (intToFloatAccum1to2)
		subl $8, %esp
		movl %esi, (%esp)
		movl %edi, 4(%esp)
		movl 20(%esp), %eax
		movl 12(%esp), %edi
		movl 16(%esp), %esi
		movl 24(%esp), %ecx
		
		flds L(INT_OVER_MAX)
		fmuls (%eax)			// st0 = gain * max
		flds L(INT_OVER_MAX)
		fmuls 4(%eax)			// st0 = gain * max
		
L(INT_PROCESS_SOURCE12A):
		jecxz L(INT_DONE12A)
		sub $2, %ecx

		fildl (%esi)
		fmul %st(2), %st(0)
		fildl (%esi)
		fmul %st(2), %st(0)
		fildl 4(%esi)
		fmul %st(4), %st(0)
		fildl 4(%esi)
		addl $8, %esi
		fmul %st(4), %st(0)

		flds (%edi)
		faddp %st(0),%st(4)
		flds 4(%edi)
		faddp %st(0),%st(3)
		flds 8(%edi)
		faddp %st(0),%st(2)
		flds 12(%edi)
		faddp %st(0),%st(1)

		fxch %st(3)					// st0=0  st1=2  st2=1  st3=3
		fstps (%edi)				// st0=2  st1=1  st2=3
		fxch %st(1)
		fstps 4(%edi)
		fstps 8(%edi)
		fstps 12(%edi)
		addl $16, %edi
		jmp L(INT_PROCESS_SOURCE12A)

L(INT_DONE12A):

		movl 4(%esp), %edi
		movl (%esp), %esi
		addl $8, %esp
		fstp %st(0)
		fstp %st(0)
		ret




// void floatToFloatAccumN( float *dest, unsigned char *src, float *gain, int channels, int samples )
ENTRY (floatToFloatAccumN)
		push %ebp
		mov %esp, %ebp

		movl 20(%ebp), %edx
		shll $2, %edx
		addl $20, %edx
		subl %edx, %esp

		movl %edi, -4(%ebp)
		movl %esi, -8(%ebp)
		
		movl 8(%ebp), %edi		// dest
		movl 12(%ebp), %esi		// source
		movl 16(%ebp), %eax		// gain
		movl 24(%ebp), %ecx		// samples
		
L(FLOAT_PROCESS_SOURCEA):
		jecxz L(FLOAT_DONEA)
		dec %ecx
		xor %edx, %edx
		
L(FLOAT_PROCESS_CHANNELA):
		flds (%esi)
		add $4, %esi
		fmuls (%eax, %edx, 4)
		inc %edx
		fadds (%edi)
		fstps (%edi)	
		addl $4, %edi
		cmp %edx, 20(%ebp)
		jnz L(FLOAT_PROCESS_CHANNELA)
		jmp L(FLOAT_PROCESS_SOURCEA)

L(FLOAT_DONEA):
		movl -4(%ebp), %edi
		movl -8(%ebp), %esi
		
		
		mov %ebp, %esp
		pop %ebp
		ret


// void floatToFloatAccum1( float *dest, unsigned char *src, float *gain, int samples )
ENTRY (floatToFloatAccum1)
		subl $8, %esp
		movl %esi, (%esp)
		movl %edi, 4(%esp)
		movl 20(%esp), %eax
		movl 12(%esp), %edi
		movl 16(%esp), %esi
		movl 24(%esp), %ecx
		flds (%eax)			// st0 = gain * max
		
L(FLOAT_PROCESS_SOURCE1A):
		jecxz L(FLOAT_DONE1A)
		sub $4, %ecx

		flds (%esi)
		fmul %st(1), %st(0)
		flds 4(%esi)
		fmul %st(2), %st(0)
		flds 8(%esi)
		fmul %st(3), %st(0)
		flds 12(%esi)
		addl $16, %esi
		fmul %st(4), %st(0)

		flds (%edi)
		faddp %st(0),%st(4)
		flds 4(%edi)
		faddp %st(0),%st(3)
		flds 8(%edi)
		faddp %st(0),%st(2)
		flds 12(%edi)
		faddp %st(0),%st(1)

		fxch %st(3)					// st0=0  st1=2  st2=1  st3=3
		fstps (%edi)				// st0=2  st1=1  st2=3
		fxch %st(1)
		fstps 4(%edi)
		fstps 8(%edi)
		fstps 12(%edi)
		addl $16, %edi
		jmp L(FLOAT_PROCESS_SOURCE1A)

L(FLOAT_DONE1A):

		movl 4(%esp), %edi
		movl (%esp), %esi
		addl $8, %esp
		fstp %st(0)
		ret


// void floatToFloatAccum2( float *dest, unsigned char *src, float *gain, int samples )
ENTRY (floatToFloatAccum2)
		subl $8, %esp
		movl %esi, (%esp)
		movl %edi, 4(%esp)
		movl 20(%esp), %eax
		movl 12(%esp), %edi
		movl 16(%esp), %esi
		movl 24(%esp), %ecx
		flds (%eax)			// st0 = gain * max
		flds 4(%eax)			// st0 = gain * max
		
L(FLOAT_PROCESS_SOURCE2A):
		jecxz L(FLOAT_DONE2A)
		sub $2, %ecx

		flds (%esi)
		fmul %st(2), %st(0)
		flds 4(%esi)
		fmul %st(2), %st(0)
		flds 8(%esi)
		fmul %st(4), %st(0)
		flds 12(%esi)
		addl $16, %esi
		fmul %st(4), %st(0)

		flds (%edi)
		faddp %st(0),%st(4)
		flds 4(%edi)
		faddp %st(0),%st(3)
		flds 8(%edi)
		faddp %st(0),%st(2)
		flds 12(%edi)
		faddp %st(0),%st(1)

		fxch %st(3)					// st0=0  st1=2  st2=1  st3=3
		fstps (%edi)				// st0=2  st1=1  st2=3
		fxch %st(1)
		fstps 4(%edi)
		fstps 8(%edi)
		fstps 12(%edi)
		addl $16, %edi
		jmp L(FLOAT_PROCESS_SOURCE2A)

L(FLOAT_DONE2A):

		movl 4(%esp), %edi
		movl (%esp), %esi
		addl $8, %esp
		fstp %st(0)
		fstp %st(0)
		ret




// void floatToFloatAccum1to2( float *dest, unsigned char *src, float *gain, int samples )
ENTRY (floatToFloatAccum1to2)
		subl $8, %esp
		movl %esi, (%esp)
		movl %edi, 4(%esp)
		movl 20(%esp), %eax
		movl 12(%esp), %edi
		movl 16(%esp), %esi
		movl 24(%esp), %ecx
		
		flds (%eax)				// st0 = gain * max
		flds 4(%eax)			// st0 = gain * max
		
L(FLOAT_PROCESS_SOURCE12A):
		jecxz L(FLOAT_DONE12A)
		sub $2, %ecx

		flds (%esi)
		fmul %st(2), %st(0)
		flds (%esi)
		fmul %st(2), %st(0)
		flds 4(%esi)
		fmul %st(4), %st(0)
		flds 4(%esi)
		addl $8, %esi
		fmul %st(4), %st(0)

		flds (%edi)
		faddp %st(0),%st(4)
		flds 4(%edi)
		faddp %st(0),%st(3)
		flds 8(%edi)
		faddp %st(0),%st(2)
		flds 12(%edi)
		faddp %st(0),%st(1)

		fxch %st(3)					// st0=0  st1=2  st2=1  st3=3
		fstps (%edi)				// st0=2  st1=1  st2=3
		fxch %st(1)
		fstps 4(%edi)
		fstps 8(%edi)
		fstps 12(%edi)
		addl $16, %edi
		jmp L(FLOAT_PROCESS_SOURCE12A)

L(FLOAT_DONE12A):

		movl 4(%esp), %edi
		movl (%esp), %esi
		addl $8, %esp
		fstp %st(0)
		fstp %st(0)
		ret


/************************************************************************************/



// extern void convertBufferFloatToInt( int32 *dest, float *src, float gain, int32 samples );
ENTRY (convertBufferFloatToInt)
		subl $28, %esp
		movl %ebx, 16(%esp)
		movl %esi, 20(%esp)
		movl %edi, 24(%esp)
		flds 40(%esp)
		fmuls L(INT_MAX)
		movl 32(%esp), %edi
		movl 36(%esp), %esi
		movl 44(%esp), %ecx
		
L(FLOAT_TO_INT_LOOP):
		test %ecx, %ecx
		jnz L(FLOAT_TO_INT_CONT)
		movl 16(%esp), %ebx
		movl 20(%esp), %esi
		movl 24(%esp), %edi
		addl $28, %esp
		fstp %st(0)
		ret
		
L(FLOAT_TO_INT_CONT):		
		sub $4, %ecx

		/* begin clamp -1.0 to 1.0 */
		movl (%esi), %eax
		movl %eax, %edx
		andl $0x7fffffff, %eax		// Get absolute value
		andl $0x80000000, %edx		// Get sign bit

		movl $1065353216, %ebx		// load ebx with float 1.0
		movl %edx, (%esp)			// Save sign bit
		
		// convoluted chunk which clamps eax to ebx
		subl %eax, %ebx
		sbbl %edx, %edx
		andl %ebx, %edx
		addl %edx, %eax
		
		orl (%esp), %eax
		movl %eax, (%esp)


		/* begin clamp -1.0 to 1.0 */
		movl 4(%esi), %eax
		mov %eax, %edx
		andl $0x7fffffff, %eax		// Get absolute value
		andl $0x80000000, %edx		// Get sign bit

		movl $1065353216, %ebx		// load ebx with float 1.0
		movl %edx, 4(%esp)			// Save sign bit
		
		// convoluted chunk which clamps eax to ebx
		subl %eax, %ebx
		sbbl %edx, %edx
		andl %ebx, %edx
		addl %edx, %eax
		
		orl 4(%esp), %eax
		movl %eax, 4(%esp)


		/* begin clamp -1.0 to 1.0 */
		movl 8(%esi), %eax
		movl %eax, %edx
		andl $0x7fffffff, %eax		// Get absolute value
		andl $0x80000000, %edx		// Get sign bit

		movl $1065353216, %ebx		// load ebx with float 1.0
		movl %edx, 8(%esp)			// Save sign bit
		
		// convoluted chunk which clamps eax to ebx
		subl %eax, %ebx
		sbbl %edx, %edx
		andl %ebx, %edx
		addl %edx, %eax
		
		orl 8(%esp), %eax
		movl %eax, 8(%esp)


		/* begin clamp -1.0 to 1.0 */
		movl 12(%esi), %eax
		mov %eax, %edx
		andl $0x7fffffff, %eax		// Get absolute value
		andl $0x80000000, %edx		// Get sign bit

		movl $1065353216, %ebx		// load ebx with float 1.0
		movl %edx, 12(%esp)			// Save sign bit
		
		// convoluted chunk which clamps eax to ebx
		subl %eax, %ebx
		sbbl %edx, %edx
		andl %ebx, %edx
		addl %edx, %eax
		
		orl 12(%esp), %eax
		movl %eax, 12(%esp)
		
		
		/* Do float to int conversion */
		add $16, %esi
	
		flds (%esp)
		fmul %st(1), %st(0)
		flds 4(%esp)
		fmul %st(2), %st(0)
		flds 8(%esp)
		fmul %st(3), %st(0)
		flds 12(%esp)
		fmul %st(4), %st(0)
		
		fxch %st(3)
		fistpl (%edi)
		fxch %st(1)
		fistpl 4(%edi)
		fistpl 8(%edi)
		fistpl 12(%edi)
		
		add $16, %edi
		
		jmp L(FLOAT_TO_INT_LOOP)





// extern void convertBufferFloatToWord( int32 *dest, float *src, float gain, int32 samples );
ENTRY (convertBufferFloatToWord)
		subl $28, %esp
		movl %ebx, 16(%esp)
		movl %esi, 20(%esp)
		movl %edi, 24(%esp)
		flds 40(%esp)
		fmuls L(WORD_MAX)
		movl 32(%esp), %edi
		movl 36(%esp), %esi
		movl 44(%esp), %ecx
		
L(FLOAT_TO_WORD_LOOP):
		test %ecx, %ecx
		jnz L(FLOAT_TO_WORD_CONT)
		movl 16(%esp), %ebx
		movl 20(%esp), %esi
		movl 24(%esp), %edi
		addl $28, %esp
		fstp %st(0)
		ret
		
L(FLOAT_TO_WORD_CONT):		
		sub $4, %ecx

		/* begin clamp -1.0 to 1.0 */
		movl (%esi), %eax
		movl %eax, %edx
		andl $0x7fffffff, %eax		// Get absolute value
		andl $0x80000000, %edx		// Get sign bit

		movl $1065353216, %ebx		// load ebx with float 1.0
		movl %edx, (%esp)			// Save sign bit
		
		// convoluted chunk which clamps eax to ebx
		subl %eax, %ebx
		sbbl %edx, %edx
		andl %ebx, %edx
		addl %edx, %eax
		
		orl (%esp), %eax
		movl %eax, (%esp)


		/* begin clamp -1.0 to 1.0 */
		movl 4(%esi), %eax
		mov %eax, %edx
		andl $0x7fffffff, %eax		// Get absolute value
		andl $0x80000000, %edx		// Get sign bit

		movl $1065353216, %ebx		// load ebx with float 1.0
		movl %edx, 4(%esp)			// Save sign bit
		
		// convoluted chunk which clamps eax to ebx
		subl %eax, %ebx
		sbbl %edx, %edx
		andl %ebx, %edx
		addl %edx, %eax
		
		orl 4(%esp), %eax
		movl %eax, 4(%esp)


		/* begin clamp -1.0 to 1.0 */
		movl 8(%esi), %eax
		movl %eax, %edx
		andl $0x7fffffff, %eax		// Get absolute value
		andl $0x80000000, %edx		// Get sign bit

		movl $1065353216, %ebx		// load ebx with float 1.0
		movl %edx, 8(%esp)			// Save sign bit
		
		// convoluted chunk which clamps eax to ebx
		subl %eax, %ebx
		sbbl %edx, %edx
		andl %ebx, %edx
		addl %edx, %eax
		
		orl 8(%esp), %eax
		movl %eax, 8(%esp)


		/* begin clamp -1.0 to 1.0 */
		movl 12(%esi), %eax
		mov %eax, %edx
		andl $0x7fffffff, %eax		// Get absolute value
		andl $0x80000000, %edx		// Get sign bit

		movl $1065353216, %ebx		// load ebx with float 1.0
		movl %edx, 12(%esp)			// Save sign bit
		
		// convoluted chunk which clamps eax to ebx
		subl %eax, %ebx
		sbbl %edx, %edx
		andl %ebx, %edx
		addl %edx, %eax
		
		orl 12(%esp), %eax
		movl %eax, 12(%esp)
		
		
		/* Do float to int conversion */
		add $16, %esi
	
		flds (%esp)
		fmul %st(1), %st(0)
		flds 4(%esp)
		fmul %st(2), %st(0)
		flds 8(%esp)
		fmul %st(3), %st(0)
		flds 12(%esp)
		fmul %st(4), %st(0)
		
		fxch %st(3)
		fistp (%edi)
		fxch %st(1)
		fistp 2(%edi)
		fistp 4(%edi)
		fistp 6(%edi)
		
		add $8, %edi
		
		jmp L(FLOAT_TO_WORD_LOOP)




// extern void convertBufferFloatToUByte( int32 *dest, float *src, float gain, int32 samples );
ENTRY (convertBufferFloatToUByte)
		subl $28, %esp
		movl %ebx, 16(%esp)
		movl %esi, 20(%esp)
		movl %edi, 24(%esp)
		flds 40(%esp)
		fmuls L(BYTE_MAX)
		movl 32(%esp), %edi
		movl 36(%esp), %esi
		movl 44(%esp), %ecx
		
L(FLOAT_TO_UBYTE_LOOP):
		test %ecx, %ecx
		jnz L(FLOAT_TO_UBYTE_CONT)
		movl 16(%esp), %ebx
		movl 20(%esp), %esi
		movl 24(%esp), %edi
		addl $28, %esp
		fstp %st(0)
		ret
		
L(FLOAT_TO_UBYTE_CONT):		
		sub $4, %ecx

		/* begin clamp -1.0 to 1.0 */
		movl (%esi), %eax
		movl %eax, %edx
		andl $0x7fffffff, %eax		// Get absolute value
		andl $0x80000000, %edx		// Get sign bit

		movl $1065353216, %ebx		// load ebx with float 1.0
		movl %edx, (%esp)			// Save sign bit
		
		// convoluted chunk which clamps eax to ebx
		subl %eax, %ebx
		sbbl %edx, %edx
		andl %ebx, %edx
		addl %edx, %eax
		
		orl (%esp), %eax
		movl %eax, (%esp)


		/* begin clamp -1.0 to 1.0 */
		movl 4(%esi), %eax
		mov %eax, %edx
		andl $0x7fffffff, %eax		// Get absolute value
		andl $0x80000000, %edx		// Get sign bit

		movl $1065353216, %ebx		// load ebx with float 1.0
		movl %edx, 4(%esp)			// Save sign bit
		
		// convoluted chunk which clamps eax to ebx
		subl %eax, %ebx
		sbbl %edx, %edx
		andl %ebx, %edx
		addl %edx, %eax
		
		orl 4(%esp), %eax
		movl %eax, 4(%esp)


		/* begin clamp -1.0 to 1.0 */
		movl 8(%esi), %eax
		movl %eax, %edx
		andl $0x7fffffff, %eax		// Get absolute value
		andl $0x80000000, %edx		// Get sign bit

		movl $1065353216, %ebx		// load ebx with float 1.0
		movl %edx, 8(%esp)			// Save sign bit
		
		// convoluted chunk which clamps eax to ebx
		subl %eax, %ebx
		sbbl %edx, %edx
		andl %ebx, %edx
		addl %edx, %eax
		
		orl 8(%esp), %eax
		movl %eax, 8(%esp)


		/* begin clamp -1.0 to 1.0 */
		movl 12(%esi), %eax
		mov %eax, %edx
		andl $0x7fffffff, %eax		// Get absolute value
		andl $0x80000000, %edx		// Get sign bit

		movl $1065353216, %ebx		// load ebx with float 1.0
		movl %edx, 12(%esp)			// Save sign bit
		
		// convoluted chunk which clamps eax to ebx
		subl %eax, %ebx
		sbbl %edx, %edx
		andl %ebx, %edx
		addl %edx, %eax
		
		orl 12(%esp), %eax
		movl %eax, 12(%esp)
		
		
		/* Do float to int conversion */
		add $16, %esi
	
		flds (%esp)
		fmul %st(1), %st(0)
		flds 4(%esp)
		fmul %st(2), %st(0)
		flds 8(%esp)
		fmul %st(3), %st(0)
		flds 12(%esp)
		fmul %st(4), %st(0)

		flds L(BYTE_OFFSET)
		fadd %st(0), %st(4)
		fadd %st(0), %st(3)
		fadd %st(0), %st(2)
		faddp %st(0), %st(1)
		
		fxch %st(3)
		fistpl (%esp)
		fxch %st(1)
		fistpl 4(%esp)
		fistpl 8(%esp)
		fistpl 12(%esp)
		
		movl (%esp), %eax
		movb %al, (%edi)
		movl 4(%esp), %eax
		movb %al, 1(%edi)
		movl 8(%esp), %eax
		movb %al, 2(%edi)
		movl 12(%esp), %eax
		movb %al, 3(%edi)
		add $4, %edi
		
		jmp L(FLOAT_TO_UBYTE_LOOP)

