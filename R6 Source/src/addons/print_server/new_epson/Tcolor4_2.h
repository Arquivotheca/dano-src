#ifndef TCOLOR4_2_H
#define TCOLOR4_2_H

#include <SupportDefs.h>
#include <Tcolor4.h>

class t_fcolor_cmyk_2 : public t_fcolor_cmyk
{
public:
	t_fcolor_cmyk_2() : t_fcolor_cmyk()			{ }
	t_fcolor_cmyk_2(int32 f) : t_fcolor_cmyk(f)	{ }

	void dither(	const t_fcolor_cmyk_2& prev_error,
					uint16 *pixel,
					int16 *f)
	{
		cyan		= prev_error.cyan		+ f[0];
		magenta		= prev_error.magenta	+ f[1];
		yellow		= prev_error.yellow		+ f[2];
		black		= prev_error.black		+ f[3];
		do_dither(pixel);
	}
	
	void dither(	const t_fcolor_cmyk_2& prev_error,
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
	}
};


#endif

