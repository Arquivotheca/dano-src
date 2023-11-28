/***************************************************************************
//
//	File:			render2/IPath.h
//
//	Description:	Path definition interface
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _RENDER2_PATH_H_
#define _RENDER2_PATH_H_

#include <math.h>
#include <support2/SupportDefs.h>
#include <support2/IInterface.h>
#include <render2/Point.h>

namespace B {
namespace Render2 {

#define B_AUTO_RADIUS	(-1)

using namespace Support2;

/**************************************************************************************/

class IPath : public IInterface
{
	public:

		B_DECLARE_META_INTERFACE(Path)

		/*	MoveTo(), LineTo(), BezierTo(), ArcTo() and Close() have direct analogs in Postscript.
			Arc() explicitly defines an arc via centerpoint, radius and start and stop angle
				("connected" determines if the arc is started with a LineTo()(true) or MoveTo()(false))
			Clear() clears the current path */

		virtual	void				MoveTo(const BPoint& pt) = 0;

		virtual	void				LinesTo(const BPoint* points, int32 lineCount=1) = 0;
				void				LineTo(BPoint endpoint);
				void				LineTo(coord x, coord y);

		virtual	void				BeziersTo(const BPoint* points, int32 bezierCount=1);
				void				BezierTo(BPoint pt1, BPoint pt2, BPoint endpoint);
				void				BezierTo(coord x1, coord y1, coord x2, coord y2, coord x3, coord y3);

		virtual	void				ArcsTo(const BPoint* points, coord radius, int32 arcCount=1);
				void				ArcTo(BPoint p1, BPoint p2, coord radius = B_AUTO_RADIUS);
				void				ArcTo(coord x1, coord y1, coord x2, coord y2, coord radius = B_AUTO_RADIUS);
		
		virtual	void				Arc(const BPoint& center, coord radius, float startAngle=0, float arcLen=M_2_PI, bool connected=false);
				void				Arc(coord centerX, coord centerY, coord radius, float startAngle=0, float arcLen=M_2_PI, bool connected=false);

		virtual	void				Close() = 0;

		virtual	void				Line(const BPoint &p0, const BPoint &p1);
		virtual	void				Rect(const BRect &r);
		virtual	void				RoundRect(const BRect &r, const BPoint& radii);
};

/**************************************************************************************/

} } // namespace B::Render2

#endif
