#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Debug.h>
#include <InterfaceDefs.h>
#include <PrivateScreen.h>

#include <interface_misc.h>
#include <screen_private.h>

#include "screen_utils.h"

static bool
CompareFloats(float f1, float f2)
{
	char str1[10], str2[10];
	sprintf(str1, "%.1f", f1);
	sprintf(str2, "%.1f", f2);
	return (strcmp(str1,str2) == 0);
}

void
SetRefreshRate(float rate)
{
	if (!CompareFloats(RefreshRate(),rate))
		set_screen_refresh_rate(0, rate, true);
}

float
RefreshRate()
{
	screen_desc desc;
	BScreen b(B_MAIN_SCREEN_ID);
	b.private_screen()->get_screen_desc(&desc);
  	return desc.cur_rate;
}

void
RateLimits(float *min, float *max)
{
	screen_desc desc;
	BScreen b(B_MAIN_SCREEN_ID);
	b.private_screen()->get_screen_desc(&desc);
	*min = desc.min_rate;
	*max = desc.max_rate;
}

void
SetDesktopColor(rgb_color c)
{
	BScreen b(B_MAIN_SCREEN_ID);
	
	b.SetDesktopColor(c, true);
}

rgb_color
DesktopColor(void)
{
	BScreen b(B_MAIN_SCREEN_ID);
	
	return b.DesktopColor();
}

void
SetResolution(color_space c, float w, float h)
{
	uint32 r = PartsToScreenSpace(c, w, h);
	
	set_screen_space(0, r, true);
}

resolution
ResolutionFor(float w, float h)
{
	if (w==640 && h==480)
		return k640x480Resolution;
	else if (w==800 && h==600)
		return k800x600Resolution;
	else if (w==1024 && h==768)
		return k1024x768Resolution;
	else if (w==1152 && h==900)
		return k1152x900Resolution;
	else if (w==1280 && h==1024)
		return k1280x1024Resolution;
	else if (w==1600 && h==1200)
		return k1600x1200Resolution;

	// No exact match, so find the closest one we can by area
	float area = w * h;
	float delta = area / 4;
	float newDelta;
	const float widths[]  = {640, 800, 1024, 1152, 1280, 1600, 0};
	const float heights[] = {480, 600,  786,  900, 1024, 1200, 0};
	const resolution res[] = {k640x480Resolution, k800x600Resolution, k1024x768Resolution,
		k1152x900Resolution, k1280x1024Resolution, k1600x1200Resolution, (resolution)-1};
	resolution theRes = (resolution)-1;
	int i = 0;
	while (widths[i]) {
		newDelta = fabs((widths[i] * heights[i]) - area);
		if (newDelta < delta) {
			theRes = res[i];
			delta = newDelta;
		}
		i++;
	}

	return theRes;
}

resolution
Resolution()
{
	float w = ScreenWidth();
	float h = ScreenHeight();
	return ResolutionFor(w, h);
}

uint32
MonitorColorSpace()
{
	screen_desc desc;
	BScreen screen(B_MAIN_SCREEN_ID);
	screen.private_screen()->get_screen_desc(&desc);
	return desc.space;
}

float
ScreenWidth()
{
	BScreen b(B_MAIN_SCREEN_ID);
	return b.Frame().Width()+1;
}

float
ScreenHeight()
{
	BScreen b(B_MAIN_SCREEN_ID);
	return b.Frame().Height()+1;	
}	

void
DimensionsFor(resolution r, float *w, float *h)
{
	switch (r) {
		case k640x480Resolution:
			*w = 640; *h = 480;
			break;
		case k800x600Resolution:
			*w = 800; *h = 600;
			break;
		case k1024x768Resolution:
			*w = 1024; *h = 768;
			break;
		case k1152x900Resolution:
			*w = 1152; *h = 900;
			break;
		case k1280x1024Resolution:
			*w = 1280; *h = 1024;
			break;
		case k1600x1200Resolution:
			*w = 1600; *h = 1200;
			break;
		default:
			*w = 0; *h = 0;
			break;
	}
}

void
SetBitsPerPixel(color_space bpp, float w, float h)
{
//	float w = ScreenWidth();	// keep width and height
//	float h = ScreenHeight();
								// set the new bit depth (colors)
	uint32 resolution = PartsToScreenSpace(bpp, w, h);
	
	set_screen_space(0, resolution, true);
}

color_space
BitsPerPixel()
{
	BScreen b(B_MAIN_SCREEN_ID);
	color_space c = b.ColorSpace();
	return c;
}

uint32
ColorCount(color_space bpp)
{
	uint32 retval;
	switch(bpp) {
		case B_COLOR_8_BIT:
			retval = 8;
			break;
		
		case B_RGB15:
		case B_RGBA15:
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
			retval = 15;
			break;
			
		case B_RGB16:
		case B_RGB16_BIG:
//		case B_RGB_16_BIT:
//		case B_BIG_RGB_16_BIT:
			retval = 16;
			break;
		
		case B_RGB32:
		case B_RGBA32:
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
			retval = 32;
			break;
			
		default:
			retval = 8;
			break;
	}
	
	return retval;
}

//

uint32
PartsToScreenSpace(color_space bpp,float w,float h)
{
	uint32 retval = B_8_BIT_640x400;
	switch(bpp) {
		case B_COLOR_8_BIT:
			if ((w==640) && (h==480)) 			retval = 	B_8_BIT_640x480;
			else if ((w==800) && (h==600)) 		retval = 	B_8_BIT_800x600;
			else if ((w==1024) && (h==768)) 	retval = 	B_8_BIT_1024x768;
			else if (w==1152 && h<=900 && h>=864)			retval =  	B_8_BIT_1152x900;
			else if ((w==1280) && (h==1024)) 	retval = 	B_8_BIT_1280x1024;
			else if ((w==1600) && (h==1200)) 	retval = 	B_8_BIT_1600x1200;
			break;
		
		case B_RGB15:
		case B_RGBA15:
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
			if ((w==640) && (h==480)) 			retval = 	B_15_BIT_640x480;
			else if ((w==800) && (h==600)) 		retval = 	B_15_BIT_800x600;
			else if ((w==1024) && (h==768)) 	retval = 	B_15_BIT_1024x768;
			else if (w==1152 && h<=900 && h>=864)			retval =  	B_15_BIT_1152x900;
			else if ((w==1280) && (h==1024)) 	retval = 	B_15_BIT_1280x1024;
			else if ((w==1600) && (h==1200)) 	retval = 	B_15_BIT_1600x1200;
			break;
		
		case B_RGB16:
		case B_RGB16_BIG:
//		case B_RGB_16_BIT:
//		case B_BIG_RGB_16_BIT:
			if ((w==640) && (h==480)) 			retval = 	B_16_BIT_640x480;
			else if ((w==800) && (h==600)) 		retval = 	B_16_BIT_800x600;
			else if ((w==1024) && (h==768)) 	retval = 	B_16_BIT_1024x768;
			else if (w==1152 && h<=900 && h>=864)			retval =  	B_16_BIT_1152x900;
			else if ((w==1280) && (h==1024)) 	retval = 	B_16_BIT_1280x1024;
			else if ((w==1600) && (h==1200)) 	retval = 	B_16_BIT_1600x1200;
			break;
		
		case B_RGB32:
		case B_RGBA32:
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
			if ((w==640) && (h==480)) 			retval = 	B_32_BIT_640x480;
			else if ((w==800) && (h==600)) 		retval = 	B_32_BIT_800x600;
			else if ((w==1024) && (h==768)) 	retval = 	B_32_BIT_1024x768;
			else if (w==1152 && h<=900 && h>=864)			retval =  	B_32_BIT_1152x900;
			else if ((w==1280) && (h==1024)) 	retval = 	B_32_BIT_1280x1024;
			else if ((w==1600) && (h==1200)) 	retval = 	B_32_BIT_1600x1200;
			break;
		default:
			TRESPASS();
	}
	
	return retval;
}

void
SetCRTPosition(uchar h_pos, uchar v_pos,
	uchar h_size, uchar v_size)
{
	adjust_crt(0, h_pos, v_pos, h_size, v_size, TRUE);
}
