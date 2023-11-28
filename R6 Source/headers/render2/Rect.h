/*******************************************************************************
/
/	File:			render2/Rect.h
/
/   Description:    BRect represents a rectangular area.
/
/	Copyright 1993-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef	_RENDER2_RECT_H
#define	_RENDER2_RECT_H

#include <support2/SupportDefs.h>
#include <support2/TypeFuncs.h>
#include <support2/Value.h>
#include <render2/Point.h>

#include <math.h>

namespace B {
namespace Render2 {
using namespace Support2;

/*----------------------------------------------------------------*/
/*----- BRect class ----------------------------------------------*/

class BRect {

public:
		coord		left;
		coord		top;
		coord		right;
		coord		bottom;

		BRect();
		BRect(const BRect &);
		BRect(coord l, coord t, coord r, coord b);
		BRect(const BPoint& leftTop, const BPoint& rightBottom);
		BRect(const BValue &o, status_t *result = NULL);
		BRect(const value_ref &o, status_t *result = NULL);
		
		BRect		&operator=(const BRect &from);
		BRect		&Set(coord l, coord t, coord r, coord b);
		BRect		&SetAsSize(coord l, coord t, coord w, coord h);

		BValue		AsValue() const;
inline				operator BValue() const					{ return AsValue(); }

/* BPoint selectors */
		BPoint &	LeftTop() const;
		BPoint &	RightBottom() const;
		BPoint		LeftBottom() const;
		BPoint		RightTop() const;

/* BPoint setters */
		BRect &		SetLeftTop(const BPoint&);
		BRect &		SetRightBottom(const BPoint&);
		BRect &		SetLeftBottom(const BPoint&);
		BRect &		SetRightTop(const BPoint&);

/* transformation */
		BRect &		InsetBy(const BPoint&);
inline	BRect &		InsetBy(coord dx, coord dy)				{ return InsetBy(BPoint(dx,dy)); }
		BRect &		OffsetBy(const BPoint&);
inline	BRect &		OffsetBy(coord dx, coord dy)			{ return OffsetBy(BPoint(dx,dy)); }
		BRect &		OffsetTo(const BPoint&);
inline	BRect &		OffsetTo(coord x, coord y)				{ return OffsetTo(BPoint(x,y)); }

/* expression transformations */
		BRect		InsetByCopy(const BPoint&) const;
inline	BRect		InsetByCopy(coord dx, coord dy) const	{ return InsetByCopy(BPoint(dx,dy)); }
		BRect		OffsetByCopy(const BPoint&) const;
inline	BRect		OffsetByCopy(coord dx, coord dy) const	{ return OffsetByCopy(BPoint(dx,dy)); }
		BRect		OffsetToCopy(const BPoint&) const;
inline	BRect		OffsetToCopy(coord dx, coord dy) const	{ return OffsetToCopy(BPoint(dx,dy)); }
		BRect &		InsetBySelf(const BPoint&);
inline	BRect &		InsetBySelf(coord dx, coord dy)			{ return InsetBySelf(BPoint(dx,dy)); }
		BRect &		OffsetBySelf(const BPoint&);
inline	BRect &		OffsetBySelf(coord dx, coord dy)		{ return OffsetBySelf(BPoint(dx,dy)); }
		BRect &		OffsetToSelf(const BPoint&);
inline	BRect &		OffsetToSelf(coord dx, coord dy)		{ return OffsetToSelf(BPoint(dx,dy)); }

/* comparison */
		int32		Compare(const BRect&) const;
		bool		operator==(const BRect&) const;
		bool		operator!=(const BRect&) const;
		bool		operator<(const BRect&) const;
		bool		operator<=(const BRect&) const;
		bool		operator>=(const BRect&) const;
		bool		operator>(const BRect&) const;

/* intersection and union */
		BRect		operator&(const BRect&) const;
		BRect		operator|(const BRect&) const;

/* utilities */
		bool		Intersects(const BRect& r) const;
		bool		IsValid() const;
		BPoint		Size() const;
		coord		Width() const;
		coord		Height() const;
		bool		Contains(const BPoint&) const;
		bool		Contains(const BRect&) const;
		
		void		PrintToStream(ITextOutput::arg io, uint32 flags=0) const;
static status_t		printer(ITextOutput::arg io, const value_ref& val, uint32 flags);
		
/* a few helpful operators */
		BRect		operator * (const coord f) const;
		BRect		operator + (const BPoint& p) const;
		BRect		operator - (const BPoint& p) const;
		BRect		operator / (const coord f) const;
		BRect &		operator *= (const coord f);
		BRect &		operator += (const BPoint& p);
		BRect &		operator -= (const BPoint& p);
		BRect &		operator /= (const coord f);

private:
		void		Construct(const value_ref& ref, status_t* result);
};

/*----- Type and STL utilities --------------------------------------*/
B_IMPLEMENT_SIMPLE_TYPE_FUNCS(BRect);
int32			BCompare(const BRect& r1, const BRect& r2);

ITextOutput::arg	operator<<(ITextOutput::arg io, const BRect& rect);

/*----------------------------------------------------------------*/
/*----- inline definitions ---------------------------------------*/

inline BPoint & BRect::LeftTop() const
{
	return *reinterpret_cast<BPoint*>(const_cast<coord *>(&left));
}

inline BPoint & BRect::RightBottom() const
{
	return *reinterpret_cast<BPoint*>(const_cast<coord *>(&right));
}

inline BPoint BRect::LeftBottom() const
{
	return(BPoint(left, bottom));
}

inline BPoint BRect::RightTop() const
{
	return(BPoint(right, top));
}

inline BRect& BRect::SetLeftTop(const BPoint& p)
{
	left = p.x;
	top = p.y;
	return (*this);
}

inline BRect& BRect::SetRightBottom(const BPoint& p)
{
	right = p.x;
	bottom = p.y;
	return (*this);
}

inline BRect& BRect::SetLeftBottom(const BPoint& p)
{
	left = p.x;
	bottom = p.y;
	return (*this);
}

inline BRect& BRect::SetRightTop(const BPoint& p)
{
	right = p.x;
	top = p.y;
	return (*this);
}

inline BRect::BRect() : left(0), top(0), right(-1), bottom(-1)
{
}

inline BRect::BRect(coord l, coord t, coord r, coord b)
	: left(l), top(t), right(r), bottom(b)
{
}

inline BRect::BRect(const BRect &r)
	: left(r.left), top(r.top), right(r.right), bottom(r.bottom)
{
}

inline BRect::BRect(const BPoint& leftTop, const BPoint& rightBottom)
	: left(leftTop.x), top(leftTop.y), right(rightBottom.x), bottom(rightBottom.y)
{
}

inline BRect &BRect::operator=(const BRect& from)
{
	left = from.left;
	top = from.top;
	right = from.right;
	bottom = from.bottom;
	return *this;
}

inline BRect & BRect::Set(coord l, coord t, coord r, coord b)
{
	return (*this = BRect(l,t,r,b));
}

inline bool BRect::IsValid() const
{
	return (left < right && top < bottom);
}

inline coord BRect::Width() const
{
	return(right - left);
}

inline coord BRect::Height() const
{
	return(bottom - top);
}

inline BPoint BRect::Size() const
{
	return BPoint(Width(), Height());
}

inline bool 
BRect::operator<(const BRect& other) const
{
	return Compare(other) < 0;
}

inline bool 
BRect::operator<=(const BRect& other) const
{
	return Compare(other) <= 0;
}

inline bool 
BRect::operator>=(const BRect& other) const
{
	return Compare(other) >= 0;
}

inline bool 
BRect::operator>(const BRect& other) const
{
	return Compare(other) > 0;
}

inline int32 BCompare(const BRect& r1, const BRect& r2)
{
	return r1.Compare(r2);
}

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

} } // namespace B::Render2

#endif /* _RENDER2_RECT_H */

