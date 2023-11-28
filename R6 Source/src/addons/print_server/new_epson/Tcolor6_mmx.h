#ifdef __INTEL__

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

