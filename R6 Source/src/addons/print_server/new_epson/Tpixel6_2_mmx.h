#if (defined(__INTEL__) && defined(__GNUC__) && defined(M_MMX))

#ifndef TPIXEL6_2_MMX_H
#define TPIXEL6_2_MMX_H

#include "Tpixel6_2.h"

class t_pixel_cmyklclm_nslices_2_mmx : public t_pixel_cmyklclm_nslices_2
{
public:
	t_pixel_cmyklclm_nslices_2_mmx() : t_pixel_cmyklclm_nslices_2() { }
	t_pixel_cmyklclm_nslices_2_mmx(int nbslices, uint32 bpl) : t_pixel_cmyklclm_nslices_2(nbslices, bpl) { }
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
			"leal 4(%%eax), %%ecx		\n\t"		// fShift+4
			"movl %%edi, %%edx			\n\t"		// x
			"shrl %%cl, %%edx			\n\t"		// d

			"movl %%eax, %%ecx			\n\t"		// fShift
			"movl %%edi, %%eax			\n\t"		// x
			"shrl %%cl, %%eax			\n\t"		// x >> fShift
			"movl %%eax, %%ecx			\n\t"		// x >> fShift
			"notl %%ecx					\n\t"
			"andl $15, %%ecx			\n\t"
			"addl %%ecx, %%ecx			\n\t"		// (0xF - ((x>>fShift) & 0xF)) << 1
			"xorl $24, %%ecx			\n\t"		// p

			"andl 28(%%esi), %%edi		\n\t"		// i
			
			"movl 32(%%esi), %%ebx			\n\t"		// pCyan
			"movswl (%%esi), %%eax			\n\t"
			"movl (%%ebx, %%edi, 4), %%ebx	\n\t"		// pCyan[i]
			"sall %%cl,%%eax				\n\t"
			"orl  %%eax, (%%ebx, %%edx, 4)	\n\t"		// pCyan[i][d]

			"movl 36(%%esi), %%ebx			\n\t"		// pMagenta
			"movswl 2(%%esi), %%eax			\n\t"
			"movl (%%ebx, %%edi, 4), %%ebx	\n\t"		// pMagenta[i]
			"sall %%cl,%%eax				\n\t"
			"orl  %%eax, (%%ebx, %%edx, 4)	\n\t"		// pMagenta[i][d]

			"movl 40(%%esi), %%ebx			\n\t"		// pYellow
			"movswl 4(%%esi), %%eax			\n\t"
			"movl (%%ebx, %%edi, 4), %%ebx	\n\t"		// pYellow[i]
			"sall %%cl,%%eax				\n\t"
			"orl  %%eax, (%%ebx, %%edx, 4)	\n\t"		// pYellow[i][d]

			"movl 44(%%esi), %%ebx			\n\t"		// pBlack
			"movswl 6(%%esi), %%eax			\n\t"
			"movl (%%ebx, %%edi, 4), %%ebx	\n\t"		// pBlack[i]
			"sall %%cl,%%eax				\n\t"
			"orl  %%eax, (%%ebx, %%edx, 4)	\n\t"		// pBlack[i][d]

			"movl 48(%%esi), %%ebx			\n\t"		// pLightCyan
			"movswl 8(%%esi), %%eax			\n\t"
			"movl (%%ebx, %%edi, 4), %%ebx	\n\t"		// pLightCyan[i]
			"sall %%cl,%%eax				\n\t"
			"orl  %%eax, (%%ebx, %%edx, 4)	\n\t"		// pLightCyan[i][d]

			"movl 52(%%esi), %%ebx			\n\t"		// pLightMag
			"movswl 10(%%esi), %%eax		\n\t"
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
