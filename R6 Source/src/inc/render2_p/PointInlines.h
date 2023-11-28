
#ifndef POINT_INLINES_H
#define POINT_INLINES_H

#include <render2/Point.h>

/*
inline point f2ipoint(BPoint f)
{
	point p;
	p.x = (int32)(f.x + 0.5);
	p.y = (int32)(f.y + 0.5);
	return p;
};
*/

namespace B {
namespace Render2 {

inline BPoint MidPoint(BPoint p0, BPoint p1)
{
	BPoint p;
	p.x = (p0.x + p1.x)*0.5;
	p.y = (p0.y + p1.y)*0.5;
	return p;
};

inline int FindIntersection(BPoint p0, BPoint d0, BPoint p1, BPoint d1, BPoint &p)
{
	float t1,cross;
	
	cross = d1.x*d0.y - d1.y*d0.x;
		
	t1 = ((p1.y-p0.y)*d0.x - (p1.x-p0.x)*d0.y) / cross;
	if (fabs(t1) > 1e10)
		return -1;
	p.x = p1.x + d1.x*t1;
	p.y = p1.y + d1.y*t1;
	return 0;
};

inline int FindSegmentIntersection(BPoint s1, BPoint s2, BPoint t1, BPoint t2, BPoint &p)
{
	BPoint sd,td;
	float t;
	
	sd.x = s2.x - s1.x;
	sd.y = s2.y - s1.y;
	td.x = t2.x - t1.x;
	td.y = t2.y - t1.y;
	
	if (FindIntersection(s1,sd,t2,td,p) != 0)
		return 1;

	if (sd.y > sd.x)
		t = ((p.y - s1.y)/sd.y);
	else
		t = ((p.x - s1.x)/sd.x);
	if (t < 0)
		return -1;

	if (td.y > td.x)
		t = ((p.y - t1.y)/td.y);
	else
		t = ((p.x - t1.x)/td.x);
	if (t > 1)
		return -1;
		
	return 0;	
};

} }

#endif
