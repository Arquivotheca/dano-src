//
// GfxUtils.h
//
//  Some useful functions dealing with graphics
//

#ifndef GFX_UTILS_H_
#define GFX_UTILS_H_

#include <GraphicsDefs.h>

// factory function for rgb_colors
rgb_color create_color(uchar red, uchar green, uchar blue, uchar alpha = 255);

// functions for shifting colors by a percent
inline uchar ShiftComponent(uchar component, float percent);
rgb_color ShiftColor(rgb_color color, float percent);

// changes a color to gray scale
rgb_color gray_color(rgb_color col);

void ReplaceTransparentColor(BBitmap *bitmap, rgb_color with);

#endif // GFX_UTILS_H_
