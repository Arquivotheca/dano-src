
#include <render2/Point.h>
#include <render2/Rect.h>
#include <render2/Path.h>

#warning These call-throughs may change for ARM, for register calling conventions

namespace B {
namespace Render2 {

extern void DecomposeBezier(const BPoint *p, BRect *clip, IPath &path, float maxError);

void 
IPath::LineTo(BPoint endpoint)
{
	LinesTo(&endpoint,1);
}

void 
IPath::LineTo(coord x, coord /*y*/)
{
	LinesTo((const BPoint*)&x,1);
}

void 
IPath::BeziersTo(const BPoint *points, int32 bezierCount)
{
	while (bezierCount--) {
		DecomposeBezier(points,NULL,*this,0.4);
		points += 3;
	}
}

void 
IPath::BezierTo(BPoint pt1, BPoint /*pt2*/, BPoint /*endpoint*/)
{
	BeziersTo(&pt1,1);
}

void 
IPath::BezierTo(coord x1, coord y1, coord x2, coord y2, coord x3, coord y3)
{
	BeziersTo((const BPoint*)&x1,1);	
}

void 
IPath::ArcsTo(const BPoint *points, coord radius, int32 arcCount)
{
	#warning default implementation for arcsto
}

void 
IPath::ArcTo(BPoint p1, BPoint /*p2*/, coord radius)
{
	ArcsTo(&p1,radius,1);
}

void 
IPath::ArcTo(coord x1, coord /*y1*/, coord /*x2*/, coord /*y2*/, coord radius)
{
	ArcsTo((const BPoint*)&x1,radius,1);
}

void 
IPath::Arc(const BPoint &center, coord radius, float startAngle, float arcLen, bool connected)
{
	#warning default implementation for arc
}

void 
IPath::Line(const BPoint &p0, const BPoint &p1)
{
	MoveTo(p0);
	LinesTo(&p1,1);
}

void
IPath::Rect(const BRect &r)
{
	BPoint p[3];
	MoveTo(r.LeftTop());
	p[0] = r.RightTop();
	p[1] = r.RightBottom();
	p[2] = r.LeftBottom();
	LinesTo(p,3);
	Close();
}

void
IPath::RoundRect(const BRect &r, const BPoint& radii)
{
	if (radii.x == radii.y) {
		BPoint p[5];
		p[0] = r.RightTop();
		p[1] = r.RightBottom();
		p[2] = r.LeftBottom();
		p[3] = r.LeftTop();
		p[4] = r.RightTop();
		MoveTo(p[3] + BPoint(radii.x,0));
		ArcsTo(p,radii.x,4);
		Close();
	} else {
		#warning implement RoundRect() for radx != rady
	}
}

} }
