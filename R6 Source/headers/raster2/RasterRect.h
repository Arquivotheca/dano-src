/*******************************************************************************
/
/	File:			Rect.h
/
/   Description:    BRasterRect represents a rectangular area.
/
/	Copyright 1993-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef	_RASTER2_RECT_H
#define	_RASTER2_RECT_H

#include <support2/ITextStream.h>
#include <support2/SupportDefs.h>
#include <support2/TypeFuncs.h>
#include <support2/Value.h>
#include <render2/RenderDefs.h>
#include <raster2/RasterPoint.h>

namespace B {
// XXX TO DO: Move raster conversions in to BRect.
namespace Render2 {
class BRect;
}
namespace Raster2 {

using namespace Support2;
using namespace Render2;

/*----------------------------------------------------------------*/
/*----- BRasterRect class ----------------------------------------*/

class BRasterRect {

public:
		int32		left;
		int32		top;
		int32		right;
		int32		bottom;

		BRasterRect();
		BRasterRect(const BRasterRect &);
		BRasterRect(int32 l, int32 t, int32 r, int32 b);
		BRasterRect(const BRasterPoint& leftTop, const BRasterPoint& rightBottom);
		BRasterRect(const BValue &o, status_t *result = NULL);
		BRasterRect(const value_ref &o, status_t *result = NULL);
		BRasterRect(const BRect &);

		BValue			AsValue() const;
inline					operator BValue() const		{ return AsValue(); }

						operator BRect() const;
		BRasterRect		&operator=(const BRasterRect &from);
		BRasterRect		&Set(int32 l, int32 t, int32 r, int32 b);
		BRasterRect		&SetAsSize(int32 l, int32 t, int32 w, int32 h);

/* BRasterPoint selectors */
		BRasterPoint &	LeftTop() const;
		BRasterPoint &	RightBottom() const;
		BRasterPoint	LeftBottom() const;
		BRasterPoint	RightTop() const;

/* BRasterPoint setters */
		BRasterRect &	SetLeftTop(const BRasterPoint&);
		BRasterRect &	SetRightBottom(const BRasterPoint&);
		BRasterRect &	SetLeftBottom(const BRasterPoint&);
		BRasterRect &	SetRightTop(const BRasterPoint&);

/* transformation */
		BRasterRect &	InsetBy(const BRasterPoint&);
inline	BRasterRect &	InsetBy(int32 dx, int32 dy)				{ return InsetBy(BRasterPoint(dx,dy)); }
		BRasterRect &	OffsetBy(const BRasterPoint&);
inline	BRasterRect &	OffsetBy(int32 dx, int32 dy)			{ return OffsetBy(BRasterPoint(dx,dy)); }
		BRasterRect &	OffsetTo(const BRasterPoint&);
inline	BRasterRect &	OffsetTo(int32 x, int32 y)				{ return OffsetTo(BRasterPoint(x,y)); }

/* expression transformations */
		BRasterRect		InsetByCopy(const BRasterPoint&) const;
inline	BRasterRect		InsetByCopy(int32 dx, int32 dy) const	{ return InsetByCopy(BRasterPoint(dx,dy)); }
		BRasterRect		OffsetByCopy(const BRasterPoint&) const;
inline	BRasterRect		OffsetByCopy(int32 dx, int32 dy) const	{ return OffsetByCopy(BRasterPoint(dx,dy)); }
		BRasterRect		OffsetToCopy(const BRasterPoint&) const;
inline	BRasterRect		OffsetToCopy(int32 dx, int32 dy) const	{ return OffsetToCopy(BRasterPoint(dx,dy)); }
		BRasterRect &	InsetBySelf(const BRasterPoint&);
inline	BRasterRect &	InsetBySelf(int32 dx, int32 dy)			{ return InsetBySelf(BRasterPoint(dx,dy)); }
		BRasterRect &	OffsetBySelf(const BRasterPoint&);
inline	BRasterRect &	OffsetBySelf(int32 dx, int32 dy)		{ return OffsetBySelf(BRasterPoint(dx,dy)); }
		BRasterRect &	OffsetToSelf(const BRasterPoint&);
inline	BRasterRect &	OffsetToSelf(int32 dx, int32 dy)		{ return OffsetToSelf(BRasterPoint(dx,dy)); }

/* comparison */
		int32		Compare(const BRasterRect&) const;
		bool		operator==(const BRasterRect&) const;
		bool		operator!=(const BRasterRect&) const;
		bool		operator<(const BRasterRect&) const;
		bool		operator<=(const BRasterRect&) const;
		bool		operator>=(const BRasterRect&) const;
		bool		operator>(const BRasterRect&) const;

/* intersection and union */
		BRasterRect		operator&(const BRasterRect&) const;
		BRasterRect		operator|(const BRasterRect&) const;

/* utilities */
		bool			Intersects(const BRasterRect&) const;
		bool			IsValid() const;
		BRasterPoint	Size() const;
		int32			Width() const;
		int32			Height() const;
		bool			Contains(const BRasterPoint&) const;
		bool			Contains(const BRasterRect&) const;

		void		PrintToStream(ITextOutput::arg io, uint32 flags=0) const;
static status_t		printer(ITextOutput::arg io, const value_ref& val, uint32 flags);
		
/* a few helpful operators */
		BRasterRect		operator * (const int32 f) const;
		BRasterRect		operator + (const BRasterPoint& p) const;
		BRasterRect		operator - (const BRasterPoint& p) const;
		BRasterRect		operator / (const int32 f) const;
		BRasterRect &	operator *= (const int32 f);
		BRasterRect &	operator += (const BRasterPoint& p);
		BRasterRect &	operator -= (const BRasterPoint& p);
		BRasterRect &	operator /= (const int32 f);

private:
		void		Construct(const value_ref& ref, status_t* result);
};

/*----- Type and STL utilities --------------------------------------*/
B_IMPLEMENT_SIMPLE_TYPE_FUNCS(BRasterRect);
int32			BCompare(const BRasterRect& r1, const BRasterRect& r2);

ITextOutput::arg operator<<(ITextOutput::arg io, const BRasterRect& rect);

/*----------------------------------------------------------------*/
/*----- inline definitions ---------------------------------------*/

inline BRasterPoint & BRasterRect::LeftTop() const
{
	return *reinterpret_cast<BRasterPoint*>(const_cast<int32 *>(&left));
}

inline BRasterPoint & BRasterRect::RightBottom() const
{
	return *reinterpret_cast<BRasterPoint*>(const_cast<int32 *>(&right));
}

inline BRasterPoint BRasterRect::LeftBottom() const
{
	return(BRasterPoint(left, bottom));
}

inline BRasterPoint BRasterRect::RightTop() const
{
	return(BRasterPoint(right, top));
}

inline BRasterRect& BRasterRect::SetLeftTop(const BRasterPoint& p)
{
	left = p.x;
	top = p.y;
	return (*this);
}

inline BRasterRect& BRasterRect::SetRightBottom(const BRasterPoint& p)
{
	right = p.x;
	bottom = p.y;
	return (*this);
}

inline BRasterRect& BRasterRect::SetLeftBottom(const BRasterPoint& p)
{
	left = p.x;
	bottom = p.y;
	return (*this);
}

inline BRasterRect& BRasterRect::SetRightTop(const BRasterPoint& p)
{
	right = p.x;
	top = p.y;
	return (*this);
}

inline BRasterRect::BRasterRect() : left(0), top(0), right(-1), bottom(-1)
{
}

inline BRasterRect::BRasterRect(int32 l, int32 t, int32 r, int32 b)
	: left(l), top(t), right(r), bottom(b)
{
}

inline BRasterRect::BRasterRect(const BRasterRect &r)
	: left(r.left), top(r.top), right(r.right), bottom(r.bottom)
{
}

inline BRasterRect::BRasterRect(const BRasterPoint& leftTop, const BRasterPoint& rightBottom)
	: left(leftTop.x), top(leftTop.y), right(rightBottom.x), bottom(rightBottom.y)
{
}

inline BRasterRect &BRasterRect::operator=(const BRasterRect& from)
{
	left = from.left;
	top = from.top;
	right = from.right;
	bottom = from.bottom;
	return *this;
}

inline BRasterRect& BRasterRect::Set(int32 l, int32 t, int32 r, int32 b)
{
	return (*this = BRasterRect(l,t,r,b));
}

inline bool BRasterRect::IsValid() const
{
	return (left < right && top < bottom);
}

inline int32 BRasterRect::Width() const
{
	return(right - left);
}

inline int32 BRasterRect::Height() const
{
	return(bottom - top);
}

inline BRasterPoint BRasterRect::Size() const
{
	return BRasterPoint(Width(), Height());
}

inline bool 
BRasterRect::operator<(const BRasterRect& other) const
{
	return Compare(other) < 0;
}

inline bool 
BRasterRect::operator<=(const BRasterRect& other) const
{
	return Compare(other) <= 0;
}

inline bool 
BRasterRect::operator>=(const BRasterRect& other) const
{
	return Compare(other) >= 0;
}

inline bool 
BRasterRect::operator>(const BRasterRect& other) const
{
	return Compare(other) > 0;
}

inline int32 BCompare(const BRasterRect& r1, const BRasterRect& r2)
{
	return r1.Compare(r2);
}

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

} } // namespace B::Raster2

#endif /* _RASTER2_RECT_H */

