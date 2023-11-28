#ifndef TCOLOR4_H
#define TCOLOR4_H

#include <SupportDefs.h>

class t_fcolor_cmyk
{
public:
	t_fcolor_cmyk() { }
	t_fcolor_cmyk(int32 f)	: cyan(f), magenta(f), yellow(f), black(f) { }

	static void InitMMX(void) { }
	static void ExitMMX(void) { }

	void error(const t_fcolor_cmyk& error, const int32 r, const int fact)
	{
		cyan	= (error.cyan + ((r*error.cyan)>>14)) >> fact;
		magenta	= (error.magenta + ((r*error.magenta)>>14)) >> fact;
		yellow	= (error.yellow + ((r*error.yellow)>>14)) >> fact;
		black	= (error.black + ((r*error.black)>>14)) >> fact;
	}

	void error(const t_fcolor_cmyk& error, const int fact)
	{
		cyan	= error.cyan >> fact;
		magenta	= error.magenta >> fact;
		yellow	= error.yellow >> fact;
		black	= error.black >> fact;
	}

	void prev(const t_fcolor_cmyk& error, const t_fcolor_cmyk& next)
	{
		cyan		= error.cyan	+ next.cyan;
		magenta		= error.magenta	+ next.magenta;
		yellow		= error.yellow	+ next.yellow;
		black		= error.black	+ next.black;
	}
	
	t_fcolor_cmyk& operator += (const t_fcolor_cmyk& c)
	{
		cyan		+= c.cyan;
		magenta		+= c.magenta;
		yellow		+= c.yellow;
		black		+= c.black;
		return *this;
	}

	t_fcolor_cmyk& operator -= (const t_fcolor_cmyk& c)
	{
		cyan		-= c.cyan;
		magenta		-= c.magenta;
		yellow		-= c.yellow;
		black		-= c.black;
		return *this;
	}

	void dither(	const t_fcolor_cmyk& prev_error,
					uint16 *pixel,
					int16 *f)
	{
		cyan		= prev_error.cyan		+ f[0];
		magenta		= prev_error.magenta	+ f[1];
		yellow		= prev_error.yellow		+ f[2];
		black		= prev_error.black		+ f[3];
		do_dither(pixel);
	}
	
	void dither(	const t_fcolor_cmyk& prev_error,
					uint16 *pixel)
	{
		*this = prev_error;
		do_dither(pixel);
	}

protected:

	inline void do_dither(uint16 * pixel)
	{
		if (cyan		>= 0x1FFF)	{ cyan		-= 0x3FFF;	pixel[0] = 1; } else { pixel[0] = 0; }
		if (magenta		>= 0x1FFF)	{ magenta	-= 0x3FFF;	pixel[1] = 1; } else { pixel[1] = 0; }
		if (yellow		>= 0x1FFF)	{ yellow	-= 0x3FFF;	pixel[2] = 1; } else { pixel[2] = 0; }
		if (black		>= 0x1FFF)	{ black		-= 0x3FFF;	pixel[3] = 1; } else { pixel[3] = 0; }
	}

	int32	cyan;
	int32	magenta;
	int32	yellow;
	int32	black;
};


#endif

