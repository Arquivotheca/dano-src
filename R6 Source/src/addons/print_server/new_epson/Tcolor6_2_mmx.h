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
