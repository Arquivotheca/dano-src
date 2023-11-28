
#include "ImageUtils.h"

BRect 
transformed_bounds(BRect rect, const BTransform2d &tm)
{
	BPoint r[4];
	r[0] = rect.LeftTop();
	r[1] = rect.RightTop();
	r[2] = rect.RightBottom();
	r[3] = rect.LeftBottom();
	// transformed
	tm.Transform(r, 4);
	// determine the boundaries
	BPoint tl(FLT_MAX, FLT_MAX), br(-FLT_MAX, -FLT_MAX);
	for (int i = 0; i < 4; i++)
	{
		//printf("[%d] %f,%f\n", i, r[i].x, r[i].y);
		if (r[i].x < tl.x) tl.x = r[i].x;
		if (r[i].y < tl.y) tl.y = r[i].y;
		if (r[i].x > br.x) br.x = r[i].x;
		if (r[i].y > br.y) br.y = r[i].y;
	}
	return BRect(tl, br);
}

