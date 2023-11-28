#ifndef _IMAGE_UTILS_H_
#define _IMAGE_UTILS_H_

#include <Rect.h>
#include <Transform2d.h>

BRect transformed_bounds(BRect r, const BTransform2d &tm);

#endif
