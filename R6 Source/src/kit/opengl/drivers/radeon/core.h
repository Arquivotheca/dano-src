#ifndef TPIXEL4_2_H
#define TPIXEL4_2_H

#include "Tpixel4.h"

class t_pixel_cmyk_nslices_2 : public t_pixel_cmyk_nslices
{
public:
	t_pixel_cmyk_nslices_2() : t_pixel_cmyk_nslices() { }
	t_pixel_cmyk_nslices_2(int nbslices, uint32 bpl) : t_pixel_cmyk_nslices(nbslices, bpl) { }

	void put(uint32 x) const
	{
		#if B_HOST_IS_LENDIAN
			const uint32 p = ((0x0F - ((x>>fShift) & 0x0F)) << 1) ^ 0x18;
		#else
			const uint32 p = ((0x0F - ((x>>fShift) & 0x0F)) << 1);
		#endif

		const uint32 d = ((x>>fShift) >> 4);
		const int i = x & fMask;

		pCyan[i][d]		|= (fData[0] << p);
		pMagenta[i][d]	|= (fData[1] << p);
		pYellow[i][d]	|= (fData[2] << p);
		pBlack[i][d]	|= (fData[3] << p);
	}
};

#endif

                                                                                                                                                                                                                                                                                                                                     #if (defined(__INTEL__) && defined(__GNUC__) && defined(M_MMX))

#ifndef TPIXEL4_2_MMX_H
#define TPIXEL4_2_MMX_H

#include "Tpixel4_2.h"

class t_pixel_cmyk_nslices_2_mmx : public t_pixel_cmyk_nslices_2
{
public:
	t_pixel_cmyk_nslices_2_mmx() : t_pixel_cmyk_nslices_2() { }
	t_pixel_cmyk_nslices_2_mmx(int nbslices, uint32 bpl) : t_pixel_cmyk_nslices_2(nbslices, bpl) { }
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

			"movl 16(%%esi), %%eax		\n\t"		// fShift
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

			"andl 20(%%esi), %%edi		\n\t"		// i
			
			"movl 24(%%esi), %%ebx			\n\t"		// pCyan
			"movswl (%%esi), %%eax			\n\t"
			"movl (%%ebx, %%edi, 4), %%ebx	\n\t"		// pCyan[i]
			"sall %%cl,%%eax				\n\t"
			"orl  %%eax, (%%ebx, %%edx, 4)	\n\t"		// pCyan[i][d]

			"movl 28(%%esi), %%ebx			\n\t"		// pMagenta
			"movswl 2(%%esi), %%eax			\n\t"
			"movl (%%ebx, %%edi, 4), %%ebx	\n\t"		// pMagenta[i]
			"sall %%cl,%%eax				\n\t"
			"orl  %%eax, (%%ebx, %%edx, 4)	\n\t"		// pMagenta[i][d]

			"movl 32(%%esi), %%ebx			\n\t"		// pYellow
			"movswl 4(%%esi), %%eax			\n\t"
			"movl (%%ebx, %%edi, 4), %%ebx	\n\t"		// pYellow[i]
			"sall %%cl,%%eax				\n\t"
			"orl  %%eax, (%%ebx, %%edx, 4)	\n\t"		// pYellow[i][d]

			"movl 36(%%esi), %%ebx			\n\t"		// pBlack
			"movswl 6(%%esi), %%eax			\n\t"
			"movl (%%ebx, %%edi, 4), %%ebx	\n\t"		// pBlack[i]
			"sall %%cl,%%eax				\n\t"
			"orl  %%eax, (%%ebx, %%edx, 4)	\n\t"		// pBlack[i][d]

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

                   