// Misc. utility routines
// Implementation
#include "DarkenBitmap.h"
#include <Screen.h>


void	DarkenBitmap(BBitmap* original, BBitmap* darkened, long amount)
{
	if (original->ColorSpace() != B_COLOR_8_BIT &&
		darkened->ColorSpace() != B_COLOR_8_BIT)
		return;
		
	uchar* oldBits = (uchar*)original->Bits();
	uchar* newBits = (uchar*)darkened->Bits();
	long	len = darkened->BitsLength();
	
	BScreen screen;
	for (long i = 0; i < len; i++) {
		uchar	index = *oldBits;
		if (index == B_TRANSPARENT_8_BIT)
			*newBits = index;
		else {
			rgb_color color = system_colors()->color_list[index];
			if (color.red + color.green + color.blue != (255 * 3)) {
				color.red = max_c(0, color.red - amount);
				color.green = max_c(0, color.green - amount);
				color.blue = max_c(0, color.blue - amount);
			}
					
			*newBits = screen.IndexForColor(color);
		}
		newBits++;
		oldBits++;
	}
}
