/*
	ColorUtils.cpp
	
	Copyright Hekkelman Programmatuur
	
	Created: 10/08/97 21:51:22
*/

#include "HColorUtils.h"

#include <Screen.h>

#include <algorithm>
using std::min;
using std::max;

unsigned char gSelectedMap[256];
unsigned char gDisabledMap[256];

void InitSelectedMap()
{
	BScreen screen;
	rgb_color c, d;
	int i;
	
	for (i = 0; i < 255; i++)
	{
		d = c = screen.ColorForIndex(i);

		c.red = c.red * 2 / 3;
		c.green = c.green * 2 / 3;
		c.blue = c.blue * 2 / 3;

		gSelectedMap[i] = screen.IndexForColor(c);

		d.red = (c.red + 438) / 3;
		d.green = (c.green + 438) / 3;
		d.blue = (c.blue + 438) / 3;

		gDisabledMap[i] = screen.IndexForColor(d);
	}

	gSelectedMap[255] = 255;
	gDisabledMap[255] = 255;
} /* InitSelectedMap */

//////////////////////////////////////////////////////
//
//   Color conversion routines
//

void rgb2f(rgb_color rgb, float& r, float& g, float& b, float& a)
{
	r = rgb.red / 255.0;
	g = rgb.green / 255.0;
	b = rgb.blue / 255.0;
	a = rgb.alpha / 255.0;
} /* rgb2f */

rgb_color f2rgb(float r, float g, float b, float a)
{
	rgb_color rgb;

	rgb.red = (int)(max(0.0, min(255.0, r * 255.0)));
	rgb.green = (int)(max(0.0, min(255.0, g * 255.0)));
	rgb.blue = (int)(max(0.0, min(255.0, b * 255.0)));
	rgb.alpha = (int)(max(0.0, min(255.0, a * 255.0)));

	return rgb;
} /* f2rgb */

void rgb2hsv(float r, float g, float b, float& h, float& s, float& v)
{
	float cmin, cmax, delta;
	
	cmax = max(r, max(g, b));
	cmin = min(r, min(g, b));
	delta = cmax - cmin;
	
	v = cmax;
	s = cmax ? delta / cmax : 0.0;

	if (s == 0.0)
		h = -1;
	else
	{
		if (r == cmax)
			h = (g - b) / delta;
		else if (g == cmax)
			h = 2 + (b - r) / delta;
		else if (b == cmax)
			h = 4 + (r - g) / delta;
		h /= 6.0;
	}
} /* rgb2hsv */

void hsv2rgb(float h, float s, float v, float& r, float& g, float& b)
{
	float A, B, C, F;
	int i;
	
	if (s == 0.0)
		r = g = b = v;
	else
	{
		if (h >= 1.0 || h < 0.0)
			h = 0.0;
		h *= 6.0;
		i = (int)floor(h);
		F = h - i;
		A = v * (1 - s);
		B = v * (1 - (s * F));
		C = v * (1 - (s * (1 - F)));
		switch (i)
		{
			case 0:	r = v; g = C; b = A; break;
			case 1:	r = B; g = v; b = A; break;
			case 2:	r = A; g = v; b = C; break;
			case 3:	r = A; g = B; b = v; break;
			case 4:	r = C; g = A; b = v; break;
			case 5:	r = v; g = A; b = B; break;
		}
	}
} /* hsv2rgb */

void rgb2ro(rgb_color rgb, roSColor& ro)
{
	rgb2f(rgb, ro.m_Red, ro.m_Green, ro.m_Blue, ro.m_Alpha);
	
	float sat, val;
	rgb2hsv(ro.m_Red, ro.m_Green, ro.m_Blue, ro.m_Hue, sat, val);
} /* rgb2ro */

rgb_color ro2rgb(roSColor& ro)
{
	return f2rgb(ro.m_Red, ro.m_Green, ro.m_Blue);
} /* ro2rgb */

