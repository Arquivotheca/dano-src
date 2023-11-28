#ifndef TCOLOR6_2_H
#define TCOLOR6_2_H

#include <SupportDefs.h>
#include "Tcolor6.h"

class t_fcolor_cmyklclm_2 : public t_fcolor_cmyklclm
{
public:
	t_fcolor_cmyklclm_2() : t_fcolor_cmyklclm()
	{
	}

	t_fcolor_cmyklclm_2(int32 f) : t_fcolor_cmyklclm(f)
	{
	}

	void dither(	const t_fcolor_cmyklclm_2& prev_error,
					uint16 *pixel,
					int16 *f)
	{
		cyan		= prev_error.cyan		+ f[0];
		magenta		= prev_error.magenta	+ f[1];
		yellow		= prev_error.yellow		+ f[2];
		black		= prev_error.black		+ f[3];
		light_cyan	= prev_error.light_cyan + f[4];
		light_mag	= prev_error.light_mag	+ f[5];	
		do_dither(pixel);
	}
	
	void dither(	const t_fcolor_cmyklclm_2& prev_error,
					uint16 *pixel)
	{
		*this = prev_error;
		do_dither(pixel);
	}

protected:

	inline void do_dither(uint16 *pixel)
	{
		if (cyan >= 0x0800)
		{
			if (cyan < 0x1800)			{	pixel[0] = 1;	cyan -= 0x1000; }
			else if (cyan < 0x3000)		{	pixel[0] = 2;	cyan -= 0x2000; }
			else						{	pixel[0] = 3;	cyan -= 0x4000; }
		} else pixel[0] = 0;

		if (magenta >= 0x0800)
		{
			if (magenta < 0x1800)		{	pixel[1] = 1;	magenta -= 0x1000; }
			else if (magenta < 0x3000)	{	pixel[1] = 2;	magenta -= 0x2000; }
			else						{	pixel[1] = 3;	magenta -= 0x4000; }
		} else pixel[1] = 0;

		if (yellow >= 0x0800)
		{
			if (yellow < 0x1800)		{	pixel[2] = 1;	yellow -= 0x1000; }
			else if (yellow < 0x3000)	{	pixel[2] = 2;	yellow -= 0x2000; }
			else						{	pixel[2] = 3;	yellow -= 0x4000; }
		} else pixel[2] = 0;

		if (black >= 0x0800)
		{
			if (black < 0x1800)			{	pixel[3] = 1;	black -= 0x1000; }
			else if (black < 0x3000)	{	pixel[3] = 2;	black -= 0x2000; }
			else						{	pixel[3] = 3;	black -= 0x4000; }
		} else pixel[3] = 0;

		if (light_cyan >= 0x0800)
		{
			if (light_cyan < 0x1800)		{	pixel[4] = 1;	light_cyan -= 0x1000; }
			else if (light_cyan < 0x3000)	{	pixel[4] = 2;	light_cyan -= 0x2000; }
			else							{	pixel[4] = 3;	light_cyan -= 0x4000; }
		} else pixel[4] = 0;

		if (light_mag >= 0x0800)
		{
			if (light_mag < 0x1800)			{	pixel[5] = 1;	light_mag -= 0x1000; }
			else if (light_mag < 0x3000)	{	pixel[5] = 2;	light_mag -= 0x2000; }
			else							{	pixel[5] = 3;	light_mag -= 0x4000; }
		} else pixel[5] = 0;
	}
};

#endif
