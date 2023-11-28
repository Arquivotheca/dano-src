#include <Bitmap.h>
#include <Debug.h>
#include <Screen.h>
#include "GfxUtils.h"

rgb_color create_color(uchar red, uchar green, uchar blue, uchar alpha)
{
	rgb_color c;
	c.red = red;
	c.green = green;
	c.blue = blue;
	c.alpha = alpha;
	return c;
}

inline uchar
ShiftComponent(uchar component, float percent)
{
	// change the color by <percent>, make sure we aren't rounding
	// off significant bits
	if (percent >= 1)
		return (uchar)(component * (2 - percent));
	else
		return (uchar)(255 - percent * (255 - component));
}

rgb_color
ShiftColor(rgb_color color, float percent)
{
	rgb_color result = {
		ShiftComponent(color.red, percent),
		ShiftComponent(color.green, percent),
		ShiftComponent(color.blue, percent),
		0
	};
	
	return result;
}

rgb_color gray_color(rgb_color col)
{
	rgb_color grayCol = col;
	uchar gray = (col.red + col.green + col.blue) / 3;
	grayCol.red = grayCol.green = grayCol.blue = gray;
	return grayCol;
}

void ReplaceTransparentColor(BBitmap *bitmap, rgb_color with)
{
	ASSERT(bitmap->ColorSpace() == B_COLOR_8_BIT); // other color spaces not implemented yet
	
	BScreen screen(B_MAIN_SCREEN_ID);
	uint32 withIndex = screen.IndexForColor(with); 
	
	uchar *bits = (uchar *)bitmap->Bits();
	int32 bitsLength = bitmap->BitsLength();	
	for (int32 index = 0; index < bitsLength; index++) 
		if (bits[index] == B_TRANSPARENT_8_BIT)
			bits[index] = withIndex;
}

