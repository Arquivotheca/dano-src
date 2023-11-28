#ifndef TCOLOR6_H
#define TCOLOR6_H

#include <SupportDefs.h>

class t_fcolor_cmyklclm
{
public:
	t_fcolor_cmyklclm()
	{
	}

	t_fcolor_cmyklclm(int32 f)
			: cyan(f), magenta(f), yellow(f), black(f), light_cyan(f), light_mag(f)
	{
	}

	static void InitMMX(void) { }
	static void ExitMMX(void) { }
	
	void error(const t_fcolor_cmyklclm& error, const int32 r, const int fact)
	{
		cyan	= (error.cyan + ((r*error.cyan)>>14)) >> fact;
		magenta	= (error.magenta + ((r*error.magenta)>>14)) >> fact;
		yellow	= (error.yellow + ((r*error.yellow)>>14)) >> fact;
		black	= (error.black + ((r*error.black)>>14)) >> fact;
		light_cyan	= (error.light_cyan + ((r*error.light_cyan)>>14)) >> fact;
		light_mag	= (error.light_mag + ((r*error.light_mag)>>14)) >> fact;
	}

	void error(const t_fcolor_cmyklclm& error, const int fact)
	{
		cyan	= error.cyan >> fact;
		magenta	= error.magenta >> fact;
		yellow	= error.yellow >> fact;
		black	= error.black >> fact;
		light_cyan	= error.light_cyan >> fact;
		light_mag	= error.light_mag >> fact;
	}

	void prev(const t_fcolor_cmyklclm& error, const t_fcolor_cmyklclm& next)
	{
		cyan		= error.cyan		+ next.cyan;
		magenta		= error.magenta		+ next.magenta;
		yellow		= error.yellow		+ next.yellow;
		black		= error.black		+ next.black;
		light_cyan	= error.light_cyan	+ next.light_cyan;
		light_mag	= error.light_mag	+ next.light_mag;
	}
	
	t_fcolor_cmyklclm& operator += (const t_fcolor_cmyklclm& c)
	{
		cyan		+= c.cyan;
		magenta		+= c.magenta;
		yellow		+= c.yellow;
		black		+= c.black;
		light_cyan	+= c.light_cyan;
		light_mag	+= c.light_mag;
		return *this;
	}

	t_fcolor_cmyklclm& operator -= (const t_fcolor_cmyklclm& c)
	{
		cyan		-= c.cyan;
		magenta		-= c.magenta;
		yellow		-= c.yellow;
		black		-= c.black;
		light_cyan	-= c.light_cyan;
		light_mag	-= c.light_mag;
		return *this;
	}

	void dither(	const t_fcolor_cmyklclm& prev_error,
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
	
	void dither(	const t_fcolor_cmyklclm& prev_error,
					uint16 *pixel)
	{
		*this = prev_error;
		do_dither(pixel);
	}

protected:
	inline void do_dither(uint16 *pixel)
	{
		if (cyan		>= 0x1FFF)	{ cyan		-= 0x3FFF;	pixel[0] = 1; } else { pixel[0] = 0; }
		if (magenta		>= 0x1FFF)	{ magenta	-= 0x3FFF;	pixel[1] = 1; } else { pixel[1] = 0; }
		if (yellow		>= 0x1FFF)	{ yellow	-= 0x3FFF;	pixel[2] = 1; } else { pixel[2] = 0; }
		if (black		>= 0x1FFF)	{ black		-= 0x3FFF;	pixel[3] = 1; } else { pixel[3] = 0; }
		if (light_cyan	>= 0x1FFF)	{ light_cyan-= 0x3FFF;	pixel[4] = 1; } else { pixel[4] = 0; }
		if (light_mag	>= 0x1FFF)	{ light_mag	-= 0x3FFF;	pixel[5] = 1; } else { pixel[5] = 0; }
	}

	int32	cyan;
	int32	magenta;
	int32	yellow;
	int32	black;
	int32	light_cyan;
	int32	light_mag;
};

#endif
