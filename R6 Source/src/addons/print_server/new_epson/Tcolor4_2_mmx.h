#ifdef __INTEL__

#ifndef TCOLOR4_2_MMX_H
#define TCOLOR4_2_MMX_H

#include <SupportDefs.h>
#include <Tcolor4_mmx.h>

class t_fcolor_cmyk_2_mmx : public t_fcolor_cmyk_mmx
{
public:
	t_fcolor_cmyk_2_mmx() : t_fcolor_cmyk_mmx()			{ }
	t_fcolor_cmyk_2_mmx(int32 f) : t_fcolor_cmyk_mmx(f)	{ }

	void dither(	const t_fcolor_cmyk_2_mmx& prev_error,
					uint16 *pixel,
					int16 *f)
	{
		asm volatile
		(
			"movq		 (%1), %%mm0 \n\t"
			"paddsw		 (%2), %%mm0 \n\t"
			"movq		%%mm0,  (%0) \n\t"
			:
			: "r"	((int16*)mmx_wrapper),				// %0
			  "r"	((int16*)f),						// %1
			  "r"	((int16*)prev_error.mmx_wrapper)	// %2
		);
		do_dither(pixel);
	}
	
	void dither(	const t_fcolor_cmyk_2_mmx& prev_error,
					uint16 *pixel)
	{
		asm volatile
		(
			"movq		 (%1), %%mm0 \n\t"
			"movq		%%mm0,  (%0) \n\t"
			:
			: "r"	((int16*)mmx_wrapper),				// %0
			  "r"	((int16*)prev_error.mmx_wrapper)	// %1
		);
		do_dither(pixel);
	}
	
protected:

	inline void do_dither(uint16 *pixel)
	{
		static const int16 data[6][4] = {
			{ 0x07ff, 0x07ff, 0x07ff, 0x07ff },			// 0
			{ 0x1800, 0x1800, 0x1800, 0x1800 },			// 8
			{ 0x3000, 0x3000, 0x3000, 0x3000 },			// 16
			{ 1,1,1,1 },								// 24
			{ -0x1000, -0x1000, -0x1000, -0x1000 },		// 32
			{ -0x2000, -0x2000, -0x2000, -0x2000 } };	// 40
		
		asm volatile
		(
			"movq	(%0), %%mm0 \n\t"				// Load source (Already in mm0?)
			"movq	%%mm0, %%mm6 \n\t"				// Copy to output
			"pxor	%%mm7, %%mm7 \n\t"				// Clear pixel out
			"movq	24(%2), %%mm2 \n\t"				// Preload pxiel 1s
			
			"movq	%%mm0, %%mm1 \n\t"				// dup source
			"pcmpgtw (%2), %%mm1 \n\t"				// create test mask
			"movq	%%mm2, %%mm3 \n\t"
			"movq	32(%2), %%mm4 \n\t"
			"pand	%%mm1, %%mm3 \n\t"
			"pand	%%mm1, %%mm4 \n\t"
			"paddsw %%mm3, %%mm7 \n\t"
			"paddsw %%mm4, %%mm6 \n\t"

			"movq	%%mm0, %%mm1 \n\t"				// dup source
			"pcmpgtw 8(%2), %%mm1 \n\t"				// create test mask
			"movq	%%mm2, %%mm3 \n\t"
			"movq	32(%2), %%mm4 \n\t"
			"pand	%%mm1, %%mm3 \n\t"
			"pand	%%mm1, %%mm4 \n\t"
			"paddsw %%mm3, %%mm7 \n\t"
			"paddsw %%mm4, %%mm6 \n\t"

			"movq	%%mm0, %%mm1 \n\t"				// dup source
			"pcmpgtw 16(%2), %%mm1 \n\t"			// create test mask
			"movq	%%mm2, %%mm3 \n\t"
			"movq	40(%2), %%mm4 \n\t"
			"pand	%%mm1, %%mm3 \n\t"
			"pand	%%mm1, %%mm4 \n\t"
			"paddsw %%mm3, %%mm7 \n\t"
			"paddsw %%mm4, %%mm6 \n\t"
			
			"movq	%%mm7, (%1) \n\t"
			"movq	%%mm6, (%0) \n\t"
			:
			: "r"	((int16*)mmx_wrapper),				// %0
			  "r"	((int16*)pixel),					// %1
			  "r"	((int16*)&data[0][0])				// %2
		);
	}
};


#endif

#endif

