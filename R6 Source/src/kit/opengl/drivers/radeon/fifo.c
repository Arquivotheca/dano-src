#if (defined(__INTEL__) && defined(__GNUC__) && defined(M_MMX))

#ifndef TPIXEL4_MMX_H
#define TPIXEL4_MMX_H

#include "Tpixel4.h"

class t_pixel_cmyk_nslices_mmx : public t_pixel_cmyk_nslices
{
public:
	t_pixel_cmyk_nslices_mmx() : t_pixel_cmyk_nslices() { }
	t_pixel_cmyk_nslices_mmx(int nbslices, uint32 bpl) : t_pixel_cmyk_nslices(nbslices, bpl) { }
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

			"andl 20(%%esi), %%edi		\n\t"		// i

			"movswl (%%esi), %%eax			\n\t"
			"movl 24(%%esi), %%ebx			\n\t"		// pCyan
			"andl $1,%%eax					\n\t"
			"movl (%%ebx, %%edi, 4), %%ebx	\n\t"		// pCyan[i]
			"sall %%cl,%%eax				\n\t"
			"orl  %%eax, (%%ebx, %%edx, 4)	\n\t"		// pCyan[i][d]

			"movswl 2(%%esi), %%eax			\n\t"
			"movl 28(%%esi), %%ebx			\n\t"		// pMagenta
			"andl $1,%%eax					\n\t"
			"movl (%%ebx, %%edi, 4), %%ebx	\n\t"		// pMagenta[i]
			"sall %%cl,%%eax				\n\t"
			"orl  %%eax, (%%ebx, %%edx, 4)	\n\t"		// pMagenta[i][d]

			"movswl 4(%%esi), %%eax			\n\t"
			"movl 32(%%esi), %%ebx			\n\t"		// pYellow
			"andl $1,%%eax					\n\t"
			"movl (%%ebx, %%edi, 4), %%ebx	\n\t"		// pYellow[i]
			"sall %%cl,%%eax				\n\t"
			"orl  %%eax, (%%ebx, %%edx, 4)	\n\t"		// pYellow[i][d]

			"movswl 6(%%esi), %%eax			\n\t"
			"movl 36(%%esi), %%ebx			\n\t"		// pBlack
			"andl $1,%%eax					\n\t"
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

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                #ifndef TPIXEL6_H
#define TPIXEL6_H

#include <BeBuild.h>
#include <ByteOrder.h>
#include <SupportDefs.h>

class t_pixel_cmyklclm_nslices
{
public:
	t_pixel_cmyklclm_nslices()
		: 	fWordsPerPlane(0),
			fNbSlices(0),
			fShift(0),
			fMask(0),
			pCyan(0),
			pMagenta(0),
			pYellow(0),
			pBlack(0),
			pLightCyan(0),
			pLightMag(0)
	{
	}
	
	t_pixel_cmyklclm_nslices(int nbslices, uint32 bpl)
		: 	fWordsPerPlane(bpl/4),
			fNbSlices(nbslices),
			fShift(0),
			fMask(0)
	{
		pCyan = new uint32*[fNbSlices];
		pMagenta = new uint32*[fNbSlices];
		pYellow = new uint32*[fNbSlices];
		pBlack = new uint32*[fNbSlices];
		pLightCyan = new uint32*[fNbSlices];
		pLightMag = new uint32*[fNbSlices];

		switch (fNbSlices)
		{
			case  2:	fShift = 1;		fMask = 0x1;	break;
			case  4:	fShift = 2;		fMask = 0x3;	break;
			case  8:	fShift = 3;		fMask = 0x7;	break;
			case 16:	fShift = 4;		fMask = 0xF;	break;
		};
	}
	
	~t_pixel_cmyklclm_nslices()
	{
		delete [] pCyan;
		delete [] pMagenta;
		delete [] pYellow;
		delete [] pBlack;
		delete [] pLightCyan;
		delete [] pLightMag;
	}

	uint16 *data(void)	{ return fData; }

	void set(uint32 **pCmyk)
	{
		for (int i=0 ; i<(const int)fNbSlices ; i++)
		{
		 	pCyan[i]		= pCmyk[i];
			pMagenta[i]		= pCmyk[i] + fWordsPerPlane;
			pYellow[i]		= pCmyk[i] + fWordsPerPlane*2;
			pBlack[i]		= pCmyk[i] + fWordsPerPlane*3;
			pLightCyan[i]	= pCmyk[i] + fWordsPerPlane*4;
			pLightMag[i]	= pCmyk[i] + fWordsPerPlane*5;
		}
	}

	void put(uint32 x) const
	{
		#if B_HOST_IS_LENDIAN
			const uint32 p = (0x1F - ((x >> fShift) & 0x1F)) ^ 0x18;
		#else
			const uint32 p = (0x1F - ((x >> fShift) & 0x1F));
		#endif
		const int i = x & fMask;
		const uint32 d = (x >> (5+fShift));
		pCyan[i][d]		|= ((fData[0] & 0x1U) << p);
		pMagenta[i][d]	|= ((fData[1] & 0x1U) << p);
		pYellow[i][d]	|= ((fData[2] & 0x1U) << p);
		pBlack[i][d]	|= ((fData[3] & 0x1U) << p);
		pLightCyan[i][d]|= ((fData[4] & 0x1U) << p);
		pLightMag[i][d]	|= ((fData[5] & 0x1U) << p);
	}

protected:
	uint16 fData[8];		//  0
	uint32 fWordsPerPlane;	// 16
	int fNbSlices;			// 20
	int fShift;				// 24
	int fMask;				// 28
	uint32 **pCyan;			// 32
	uint32 **pMagenta;		// 36
	uint32 **pYellow;		// 40
	uint32 **pBlack;		// 44
	uint32 **pLightCyan;	// 48
	uint32 **pLightMag;		// 52
};

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               #ifndef TPIXEL6_2_H
#define TPIXEL6_2_H

#include "Tpixel6.h"

class t_pixel_cmyklclm_nslices_2 : public t_pixel_cmyklclm_nslices
{
public:
	t_pixel_cmyklclm_nslices_2() : t_pixel_cmyklclm_nslices() { }
	t_pixel_cmyklclm_nslices_2(int nbslices, uint32 bpl) : t_pixel_cmyklclm_nslices(nbslices, bpl) { }
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
		pLightCyan[i][d]|= (fData[4] << p);
		pLightMag[i][d]	|= (fData[5] << p);
	}
};

#endif
                                                                                                                                                                                                                                   #if (defined(__INTEL__) && defined(__GNUC__) && defined(M_MMX))

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
                                                                                                                                                                                                                                                                                                                                                                                                     #if (defined(__INTEL__) && defined(__GNUC__) && defined(M_MMX))

#ifndef TPIXEL6_MMX_H
#define TPIXEL6_MMX_H

#include "Tpixel6.h"

class t_pixel_cmyklclm_nslices_mmx : public t_pixel_cmyklclm_nslices
{
public:
	t_pixel_cmyklclm_nslices_mmx(