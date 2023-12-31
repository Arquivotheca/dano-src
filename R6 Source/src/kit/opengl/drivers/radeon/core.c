#ifdef __INTEL__

#ifndef TCOLOR6_2_MMX_H
#define TCOLOR6_2_MMX_H

#include <SupportDefs.h>
#include "Tcolor6.h"

class t_fcolor_cmyklclm_2_mmx : public t_fcolor_cmyklclm_mmx
{
public:
	t_fcolor_cmyklclm_2_mmx() : t_fcolor_cmyklclm_mmx()
	{
	}

	t_fcolor_cmyklclm_2_mmx(int32 f) : t_fcolor_cmyklclm_mmx(f)
	{
	}

	void dither(	const t_fcolor_cmyklclm_2_mmx& prev_error,
					uint16 *pixel,
					int16 *f)
	{
		asm volatile
		(
			"movq		 (%1), %%mm0 \n\t"
			"movd	    8(%1), %%mm1 \n\t"
			"paddsw		 (%2), %%mm0 \n\t"
			"paddsw		8(%2), %%mm1 \n\t"
			"movq		%%mm0,  (%0) \n\t"
			"movd		%%mm1, 8(%0) \n\t"
			:
			: "r"	((int16*)mmx_wrapper),				// %0
			  "r"	((int16*)f),						// %1
			  "r"	((int16*)prev_error.mmx_wrapper)	// %2
		);
		do_dither(pixel);
	}
	
	void dither(	const t_fcolor_cmyklclm_2_mmx& prev_error,
					uint16 *pixel)
	{
		asm volatile
		(
			"movq		 (%1), %%mm0 \n\t"
			"movq		%%mm0,  (%0) \n\t"
			"movd		8(%1), %%mm0 \n\t"
			"movd		%%mm0, 8(%0) \n\t"
			:
			: "r"	((int16*)mmx_wrapper),				// %0
			  "r"	((int16*)prev_error.mmx_wrapper)	// %1
		);
		do_dither(pixel);
	}

protected:

	inline void do_dither(uint16 *pixel)
	{ // Should be rewritten using MMX
		if (mmx_wrapper[0] >= 0x0800)
		{
			if (mmx_wrapper[0] < 0x1800)		{	pixel[0] = 1;	mmx_wrapper[0] -= 0x1000; }
			else if (mmx_wrapper[0] < 0x3000)	{	pixel[0] = 2;	mmx_wrapper[0] -= 0x2000; }
			else								{	pixel[0] = 3;	mmx_wrapper[0] -= 0x4000; }
		} else pixel[0] = 0;

		if (mmx_wrapper[1] >= 0x0800)
		{
			if (mmx_wrapper[1] < 0x1800)		{	pixel[1] = 1;	mmx_wrapper[1] -= 0x1000; }
			else if (mmx_wrapper[1] < 0x3000)	{	pixel[1] = 2;	mmx_wrapper[1] -= 0x2000; }
			else								{	pixel[1] = 3;	mmx_wrapper[1] -= 0x4000; }
		} else pixel[1] = 0;

		if (mmx_wrapper[2] >= 0x0800)
		{
			if (mmx_wrapper[2] < 0x1800)		{	pixel[2] = 1;	mmx_wrapper[2] -= 0x1000; }
			else if (mmx_wrapper[2] < 0x3000)	{	pixel[2] = 2;	mmx_wrapper[2] -= 0x2000; }
			else								{	pixel[2] = 3;	mmx_wrapper[2] -= 0x4000; }
		} else pixel[2] = 0;

		if (mmx_wrapper[3] >= 0x0800)
		{
			if (mmx_wrapper[3] < 0x1800)		{	pixel[3] = 1;	mmx_wrapper[3] -= 0x1000; }
			else if (mmx_wrapper[3] < 0x3000)	{	pixel[3] = 2;	mmx_wrapper[3] -= 0x2000; }
			else								{	pixel[3] = 3;	mmx_wrapper[3] -= 0x4000; }
		} else pixel[3] = 0;

		if (mmx_wrapper[4] >= 0x0800)
		{
			if (mmx_wrapper[4] < 0x1800)		{	pixel[4] = 1;	mmx_wrapper[4] -= 0x1000; }
			else if (mmx_wrapper[4] < 0x3000)	{	pixel[4] = 2;	mmx_wrapper[4] -= 0x2000; }
			else								{	pixel[4] = 3;	mmx_wrapper[4] -= 0x4000; }
		} else pixel[4] = 0;

		if (mmx_wrapper[5] >= 0x0800)
		{
			if (mmx_wrapper[5] < 0x1800)		{	pixel[5] = 1;	mmx_wrapper[5] -= 0x1000; }
			else if (mmx_wrapper[5] < 0x3000)	{	pixel[5] = 2;	mmx_wrapper[5] -= 0x2000; }
			else								{	pixel[5] = 3;	mmx_wrapper[5] -= 0x4000; }
		} else pixel[5] = 0;
	}
};

#endif

#endif
                                                                                                                                                                                                        #ifdef __INTEL__

#ifndef TCOLOR6_MMX_H
#define TCOLOR6_MMX_H

#include <SupportDefs.h>

class t_fcolor_cmyklclm_mmx
{
public:
	t_fcolor_cmyklclm_mmx()
	{
	}
	
	t_fcolor_cmyklclm_mmx(int32 v)
	{
		const int32 v32 = ((v<<16)|v);
		((int32 *)mmx_wrapper)[0] = v32;
		((int32 *)mmx_wrapper)[1] = v32;
		((int32 *)mmx_wrapper)[2] = v32;
	}

	static void InitMMX(void)
	{
		const uint32 mmx6[] ALLIGN(8) = {0x1FFF1FFF, 0x1FFF1FFF};
		const uint32 mmx7[] ALLIGN(8) = {0x3FFF3FFF, 0x3FFF3FFF};
		asm
		(
			"movq	 (%0), %%mm6	\n\t"
			"movq	 (%1), %%mm7	\n\t"
			:
			: "r" ((uint64 *)mmx6),	// %0
			  "r" ((uint64 *)mmx7)	// %1
		);
	}

	static void ExitMMX(void)
	{
		asm ("emms\n\t");
	}

	void error(const t_fcolor_cmyklclm_mmx& error, const int32 r, const int fact)
	{
		int16 rt[6] ALLIGN(16);
		rt[0] = (int16)((r*error.mmx_wrapper[0])>>14);
		rt[1] = (int16)((r*error.mmx_wrapper[1])>>14);
		rt[2] = (int16)((r*error.mmx_wrapper[2])>>14);
		rt[3] = (int16)((r*error.mmx_wrapper[3])>>14);
		rt[4] = (int16)((r*error.mmx_wrapper[4])>>14);
		rt[5] = (int16)((r*error.mmx_wrapper[5])>>14);

		asm volatile
		(
			"movd	 (%2), %%mm5	\n\t"
			"movq	 (%3), %%mm3	\n\t"
			"movq	 (%1), %%mm0	\n\t"
			"paddsw	%%mm3, %%mm0	\n\t"
			"psraw	%%mm5, %%mm0	\n\t"
			"movq	%%mm0,  (%0)	\n\t"
			"movd	8(%3), %%mm4	\n\t"
			"movd	8(%1), %%mm1	\n\t"
			"paddsw	%%mm4, %%mm1	\n\t"
			"psraw	%%mm5, %%mm1	\n\t"
			"movd	%%mm1, 8(%0)	\n\t"
			:
			: "r"	((int16*)mmx_wrapper),			// %0
			  "r"	((int16*)error.mmx_wrapper),	// %1
			  "r"	(&fact),						// %2
			  "r"	((uint64 *)rt)					// %3
		);
	}

	void error(const t_fcolor_cmyklclm_mmx& error, const int fact)
	{
		asm volatile
		(
			"movd	 (%2), %%mm5	\n\t"
			"movq	 (%1), %%mm0	\n\t"
			"movd	8(%1), %%mm1	\n\t"
			"psraw	%%mm5, %%mm0	\n\t"
			"psraw	%%mm5, %%mm1	\n\t"
			"movq	%%mm0,  (%0)	\n\t"
			"movd	%%mm1, 8(%0)	\n\t"
			:
			: "r"	((int16*)mmx_wrapper),			// %0
			  "r"	((int16*)error.mmx_wrapper),	// %1
			  "r"	(&fact)							// %2
		);
	}

	void prev(const t_fcolor_cmyklclm_mmx& error, const t_fcolor_cmyklclm_mmx& next)
	{
		// *this = error + next
		asm volatile
		(
			"movq		 (%2), %%mm0 \n\t"
			"paddsw		 (%1), %%mm0 \n\t"			
			"movd		8(%2), %%mm1 \n\t"
			"movd		8(%1), %%mm2 \n\t"
			"paddsw		%%mm2, %%mm1 \n\t"
			"movq		%%mm0,  (%0) \n\t"
			"movd		%%mm1, 8(%0) \n\t"
			:
			: "r"	((int16*)mmx_wrapper),			// %0
			  "r"	((int16*)error.mmx_wrapper),	// %1
			  "r"	((int16*)next.mmx_wrapper)		// %2
		);
	}
	
	t_fcolor_cmyklclm_mmx& operator += (const t_fcolor_cmyklclm_mmx& c)
	{
		// *this += c
		asm volatile
		(
			"movq		 (%0), %%mm0 \n\t"
			"paddsw		 (%1), %%mm0 \n\t"
			"movd		8(%0), %%mm1 \n\t"
			"movd		8(%1), %%mm2 \n\t"
			"paddsw		%%mm2, %%mm1 \n\t"
			"movq		%%mm0,  (%0) \n\t"
			"movd		%%mm1, 8(%0) \n\t"
			:
			: "r"	((int16*)mmx_wrapper),			// %0
			  "r"	((int16*)c.mmx_wrapper)			// %1
		);
		return *this;
	}

	t_fcolor_cmyklclm_mmx& operator -= (const t_fcolor_cmyklclm_mmx& c)
	{
		// *this -= c
		asm volatile
		(
			"movq		 (%0), %%mm0 \n\t"
			"psubsw		 (%1), %%mm0 \n\t"
			"movd		8(%0), %%mm1 \n\t"
			"movd		8(%1), %%mm2 \n\t"
			"psubsw		%%mm2, %%mm1 \n\t"
			"movq		%%mm0,  (%0) \n\t"
			"movd		%%mm1, 8(%0) \n\t"
			:
			: "r"	((int16*)mmx_wrapper),			// %0
			  "r"	((int16*)c.mmx_wrapper)			// %1
		);
		return *this;
	}

	void dither(	const t_fcolor_cmyklclm_mmx& prev_error,
					uint16 *pixel_data,
					int16 *f)
	{
		// codage d'une couleur : s001 1111 1111 1111
		// maximum d'erreur     : s111 1111 1111 1111
		asm volatile
		(
			"movq		 (%1), %%mm0 \n\t"
			"paddsw		 (%2), %%mm0 \n\t"
			"movq		%%mm0, %%mm1 \n\t"
			"pcmpgtw	%%mm6, %%mm1 \n\t"
			"pand		%%mm7, %%mm1 \n\t"
			"psubsw		%%mm1, %%mm0 \n\t"
			"movq		%%mm1,  (%3) \n\t"
			"movq		%%mm0,  (%0) \n\t"
			"movq	    8(%1), %%mm0 \n\t"
			"paddsw		8(%2), %%mm0 \n\t"
			"movq		%%mm0, %%mm1 \n\t"
			"pcmpgtw	%%mm6, %%mm1 \n\t"
			"pand		%%mm7, %%mm1 \n\t"
			"psubsw		%%mm1, %%mm0 \n\t"
			"movd		%%mm1, 8(%3) \n\t"
			"movd		%%mm0, 8(%0) \n\t"
			:
			: "r"	((int16*)mmx_wrapper),				// %0
			  "r"	((int16*)f),						// %1
			  "r"	((int16*)prev_error.mmx_wrapper),	// %2
			  "r"	((int16*)pixel_data)				// %3
		);
	}
	
	void dither(	const t_fcolor_cmyklclm_mmx& prev_error,
					uint16 *pixel_data)
	{
		asm volatile
		(
			"movq		 (%1), %%mm0 \n\t"
			"movq		%%mm0, %%mm1 \n\t"
			"pcmpgtw	%%mm6, %%mm1 \n\t"
			"pand		%%mm7, %%mm1 \n\t"
			"psubsw		%%mm1, %%mm0 \n\t"
			"movq		%%mm1,  (%2) \n\t"
			"movq		%%mm0,  (%0) \n\t"
			"movd		8(%1), %%mm0 \n\t"
			"movq		%%mm0, %%mm1 \n\t"
			"pcmpgtw	%%mm6, %%mm1 \n\t"
			"pand		%%mm7, %%mm1 \n\t"
			"psubsw		%%mm1, %%mm0 \n\t"
			"movd		%%mm1, 8(%2) \n\t"
			"movd		%%mm0, 8(%0) \n\t"
			:
			: "r"	((int16*)mmx_wrapper),				// %0
			  "r"	((int16*)prev_error.mmx_wrapper),	// %1
			  "r"	((int16*)pixel_data)				// %2
		);
	}

protected:
	int16	mmx_wrapper[6];
};

#endif

#endif

                                                                                                                                                                                                                    #ifndef TPIXEL4_H
#define TPIXEL4_H

#include <BeBuild.h>
#include <ByteOrder.h>
#include <SupportDefs.h>

class t_pixel_cmyk_nslices
{
public:
	t_pixel_cmyk_nslices()
		: 	fWordsPerPlane(0),
			fNbSlices(0),
			fShift(0),
			fMask(0),
			pCyan(0),
			pMagenta(0),
			pYellow(0),
			pBlack(0)
	{
	}
	
	t_pixel_cmyk_nslices(int nbslices, uint32 bpl)
		: 	fWordsPerPlane(bpl/4),
			fNbSlices(nbslices),
			fShift(0),
			fMask(0)
	{
		pCyan = new uint32*[fNbSlices];
		pMagenta = new uint32*[fNbSlices];
		pYellow = new uint32*[fNbSlices];
		pBlack = new uint32*[fNbSlices];
		
		switch (fNbSlices)
		{
			case  2:	fShift = 1;		fMask = 0x1;	break;
			case  4:	fShift = 2;		fMask = 0x3;	break;
			case  8:	fShift = 3;		fMask = 0x7;	break;
			case 16:	fShift = 4;		fMask = 0xF;	break;
		};
	}
	
	~t_pixel_cmyk_nslices()
	{
		delete [] pCyan;
		delete [] pMagenta;
		delete [] pYellow;
		delete [] pBlack;
	}

	uint16 *data(void)	{ return fData; }

	void set(uint32 **pCmyk)
	{
		for (int i=0 ; i<(const int)fNbSlices ; i++)
		{
		 	pCyan[i]	= pCmyk[i];
			pMagenta[i]	= pCmyk[i] + fWordsPerPlane;
			pYellow[i]	= pCmyk[i] + fWordsPerPlane*2;
			pBlack[i]	= pCmyk[i] + fWordsPerPlane*3;
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
	}

protected:
	uint16 fData[4];			//  0
	uint32 fWordsPerPlane;		//  8
	int fNbSlices;				// 12
	int fShift;					// 16
	int fMask;					// 20
	uint32 **pCyan;				// 24
	uint32 **pMagenta;			// 28
	uint32 **pYellow;			// 32
	uint32 **pBlack;			// 36
};


#endif

                                                                                                                                                    