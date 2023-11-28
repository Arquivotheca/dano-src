#if (defined(__INTEL__) && defined(__GNUC__) && defined(M_MMX))

#ifndef TPIXEL6_MMX_H
#define TPIXEL6_MMX_H

#include "Tpixel6.h"

class t_pixel_cmyklclm_nslices_mmx : public t_pixel_cmyklclm_nslices
{
public:
	t_pixel_cmyklclm_nslices_mmx() : t_pixel_cmyklclm_nslices() { }
	t_pixel_cmyklclm_nslices_mmx(int nbslices, uint32 bpl) : t_pixel_cmyklclm_nslices(nbslices, bpl) { }
	void put(uint32 x) const
	{
		// %eax = used for processing
		// %ebx = used for processing
		// %ecx = p
		// %edx = d
		// %esi = this (== fData)
		// %edi = i

		asm volatile
		(
			"pushl %%ebx \n\t"
			"pushl %%edi \n\t"

			"movl 24(%%esi), %%eax		\n\t"		// fShift
			"leal 5(%%eax), %%ecx		\n\t"		// fShift+5
			"movl %%edi, %%edx			\n\t"		// x
			"shrl %%cl, %%edx			\n\t"		// d

			"movl %%eax, %%ecx			\n\t"		// fShift
			"movl %%edi, %%eax			\n\t"		// x
			"shrl %%cl, %%eax			\n\t"		// x >> fShift
			"movl %%eax, %%ecx			\n\t"		// x >> fShift
			"notl %%ecx					\n\t"
			"andl $31, %%ecx			\n\t"
			"xorl $24, %%ecx			\n\t"		// p

			"andl 28(%%esi), %%edi		\n\t"		// i
			
			"movswl (%%esi), %%eax			\n\t"
			"movl 32(%%esi), %%ebx			\n\t"		// pCyan
			"andl $1,%%eax					\n\t"
			"movl (%%ebx, %%edi, 4), %%ebx	\n\t"		// pCyan[i]
			"sall %%cl,%%eax				\n\t"
			"orl  %%eax, (%%ebx, %%edx, 4)	\n\t"		// pCyan[i][d]

			"movswl 2(%%esi), %%eax			\n\t"
			"movl 36(%%esi), %%ebx			\n\t"		// pMagenta
			"andl $1,%%eax					\n\t"
			"movl (%%ebx, %%edi, 4), %%ebx	\n\t"		// pMagenta[i]
			"sall %%cl,%%eax				\n\t"
			"orl  %%eax, (%%ebx, %%edx, 4)	\n\t"		// pMagenta[i][d]

			"movswl 4(%%esi), %%eax			\n\t"
			"movl 40(%%esi), %%ebx			\n\t"		// pYellow
			"andl $1,%%eax					\n\t"
			"movl (%%ebx, %%edi, 4), %%ebx	\n\t"		// pYellow[i]
			"sall %%cl,%%eax				\n\t"
			"orl  %%eax, (%%ebx, %%edx, 4)	\n\t"		// pYellow[i][d]

			"movswl 6(%%esi), %%eax			\n\t"
			"movl 44(%%esi), %%ebx			\n\t"		// pBlack
			"andl $1,%%eax					\n\t"
			"movl (%%ebx, %%edi, 4), %%ebx	\n\t"		// pBlack[i]
			"sall %%cl,%%eax				\n\t"
			"orl  %%eax, (%%ebx, %%edx, 4)	\n\t"		// pBlack[i][d]

			"movswl 8(%%esi), %%eax			\n\t"
			"movl 48(%%esi), %%ebx			\n\t"		// pLightCyan
			"andl $1,%%eax					\n\t"
			"movl (%%ebx, %%edi, 4), %%ebx	\n\t"		// pLightCyan[i]
			"sall %%cl,%%eax				\n\t"
			"orl  %%eax, (%%ebx, %%edx, 4)	\n\t"		// pLightCyan[i][d]

			"movswl 10(%%esi), %%eax		\n\t"
			"movl 52(%%esi), %%ebx			\n\t"		// pLightMag
			"andl $1,%%eax					\n\t"
			"movl (%%ebx, %%edi, 4), %%ebx	\n\t"		// pLightMag[i]
			"sall %%cl,%%eax				\n\t"
			"orl  %%eax, (%%ebx, %%edx, 4)	\n\t"		// pLightMag[i][d]

			"popl %%edi \n\t"
			"popl %%ebx \n\t"

			:
			:	"D"	(x),	// edi = x
				"S" (this)	// esi = this == fData
			:	"eax", "ebx", "ecx", "edx", "memory"
		);
	}
};

#endif

#endif
