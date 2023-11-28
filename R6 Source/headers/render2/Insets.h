/*******************************************************************************
/
/	File:			Insets.h
/
/   Description:    BInsets represents rectangular offsets.
/
/	Copyright 2000, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef	_RENDER2_INSETS_H
#define	_RENDER2_INSETS_H

#include <support2/SupportDefs.h>
#include <render2/Point.h>

namespace B {
namespace Render2 {

using namespace Support2;

/*----------------------------------------------------------------*/
/*----- BInsets class --------------------------------------------*/

class BInsets {

public:
		float		left;
		float		top;
		float		right;
		float		bottom;

		BInsets();
		BInsets(const BInsets &);
		BInsets(float l, float t, float r, float b);

		BInsets		&operator=(const BInsets &from);
		void		Set(float l, float t, float r, float b);

		void		PrintToStream() const;

/* BPoint selectors */
		BPoint		LeftTop() const;
		BPoint		RightBottom() const;
		BPoint		LeftBottom() const;
		BPoint		RightTop() const;

/* transformation */
		BInsets &	InsetBy(float dx, float dy);
		BInsets &	InsetBy(const BInsets& d);

/* expression transformations */
		BInsets &	InsetBySelf(float dx, float dy);
		BInsets &	InsetBySelf(const BInsets& d);
		BInsets		InsetByCopy(float dx, float dy) const;
		BInsets		InsetByCopy(const BInsets& d) const;

/* comparison */
		bool		operator==(const BInsets&) const;
		bool		operator!=(const BInsets&) const;

/* a few helpful operators */
		BInsets		operator - () const;
		BInsets		operator * (const float f) const;
		BInsets		operator / (const float f) const;
		BInsets		operator + (const float f) const;
		BInsets		operator + (const BInsets& v) const;
		BInsets		operator - (const float f) const;
		BInsets		operator - (const BInsets& v) const;
		BInsets &	operator *= (const float f);
		BInsets &	operator /= (const float f);
		BInsets &	operator += (const float f);
		BInsets &	operator += (const BInsets& v);
		BInsets &	operator -= (const float f);
		BInsets &	operator -= (const BInsets& v);
};

BDataIO& operator<<(BDataIO& io, const BInsets& rect);

/*----------------------------------------------------------------*/
/*----- inline definitions ---------------------------------------*/

inline BPoint BInsets::LeftTop() const
{
	return(*((const BPoint*)&left));
}

inline BPoint BInsets::RightBottom() const
{
	return(*((const BPoint*)&right));
}

inline BPoint BInsets::LeftBottom() const
{
	return(BPoint(left, bottom));
}

inline BPoint BInsets::RightTop() const
{
	return(BPoint(right, top));
}

inline BInsets::BInsets()
	: left(0), top(0), right(0), bottom(0)
{
}

inline BInsets::BInsets(float l, float t, float r, float b)
	: left(l), top(t), right(r), bottom(b)
{
}

inline BInsets::BInsets(const BInsets &r)
	: left(r.left), top(r.top), right(r.right), bottom(r.bottom)
{
}

inline BInsets &BInsets::operator=(const BInsets& from)
{
	left = from.left;
	top = from.top;
	right = from.right;
	bottom = from.bottom;
	return *this;
}

inline void BInsets::Set(float l, float t, float r, float b)
{
	left = l;
	top = t;
	right = r;
	bottom = b;
}

inline BInsets &BInsets::InsetBySelf(float dx, float dy)
{
	return InsetBy(dx, dy);
}

inline BInsets &BInsets::InsetBySelf(const BInsets& d)
{
	return InsetBy(d);
}

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

} } // namespace B::Render2

#endif /* _RENDER2_INSETS_H */
