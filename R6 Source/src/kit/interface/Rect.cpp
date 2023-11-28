//******************************************************************************
//
//	File:		Rect.cpp
//
//	Description:	BRect class.
//	
//	Written by:	Steve Horowitz
//
//	Copyright 1993-98, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#ifndef _RECT_H
#include "Rect.h"
#endif

#include <Insets.h>
#include <StreamIO.h>
#include <SupportDefs.h>

#include <stdio.h>

//------------------------------------------------------------------------------

bool clipping_rect::operator==(const clipping_rect& r) const
{
	return(top == r.top &&
	       left == r.left &&
	       bottom == r.bottom &&
	       right == r.right);
}

//------------------------------------------------------------------------------

bool clipping_rect::operator!=(const clipping_rect& r) const
{
	return(top != r.top ||
	       left != r.left ||
	       bottom != r.bottom ||
	       right != r.right);
}

//------------------------------------------------------------------------------

BDataIO& operator<<(BDataIO& io, const clipping_rect& rect)
{
#if SUPPORTS_STREAM_IO
	io << "clipping_rect(" << rect.left << ", " << rect.top
	   << ")-(" << rect.right << ", " << rect.bottom << ")";
#else
	(void)rect;
#endif
	return io;
}

//------------------------------------------------------------------------------

void BRect::PrintToStream() const
{
#if SUPPORTS_STREAM_IO
	BOut << *this << endl;
#endif
}

//------------------------------------------------------------------------------

void BRect::SetAsSize(float l, float t, float w, float h)
{
	left = l;
	top = t;
	right = l+w;
	bottom = t+h;
}

//------------------------------------------------------------------------------

void BRect::SetLeftTop(const BPoint pt)
{
	left = pt.x;
	top = pt.y;
}

//------------------------------------------------------------------------------

void BRect::SetRightBottom(const BPoint pt)
{
	right = pt.x;
	bottom = pt.y;
}

//------------------------------------------------------------------------------

void BRect::SetLeftBottom(const BPoint pt)
{
	left = pt.x;
	bottom = pt.y;
}
//------------------------------------------------------------------------------

void BRect::SetRightTop(const BPoint pt)
{
	right = pt.x;
	top = pt.y;
}

//------------------------------------------------------------------------------

void BRect::InsetBy(BPoint delta)
{
	left += delta.x;
	top += delta.y;
	right -= delta.x;
	bottom -= delta.y;
}

//------------------------------------------------------------------------------

void BRect::InsetBy(float dx, float dy)
{
	left += dx;
	top += dy;
	right -= dx;
	bottom -= dy;
}

//------------------------------------------------------------------------------

void BRect::InsetBy(const BInsets& d)
{
	left += d.left;
	top += d.top;
	right -= d.right;
	bottom -= d.bottom;
}

//------------------------------------------------------------------------------

void BRect::OffsetBy(BPoint delta)
{
	left += delta.x;
	top += delta.y;
	right += delta.x;
	bottom += delta.y;
}

//------------------------------------------------------------------------------

void BRect::OffsetBy(float dx, float dy)
{
	left += dx;
	top += dy;
	right += dx;
	bottom += dy;
}

//------------------------------------------------------------------------------

void BRect::OffsetTo(BPoint pt)
{
	float width = right - left;
	float height = bottom - top;

	left = pt.x;
	top = pt.y;
	right = left + width;
	bottom = top + height;
}

//------------------------------------------------------------------------------

void BRect::OffsetTo(float x, float y)
{
	float width = right - left;
	float height = bottom - top;

	left = x;
	top = y;
	right = left + width;
	bottom = top + height;
}

//------------------------------------------------------------------------------

BRect & BRect::InsetBySelf(BPoint pt)
{
	InsetBy(pt);
	return *this;
}

//------------------------------------------------------------------------------

BRect & BRect::InsetBySelf(float dx, float dy)
{
	InsetBy(dx, dy);
	return *this;
}

//------------------------------------------------------------------------------

BRect & BRect::InsetBySelf(const BInsets& d)
{
	InsetBy(d);
	return *this;
}

//------------------------------------------------------------------------------

BRect BRect::InsetByCopy(BPoint pt) const
{
	BRect copy(*this);
	copy.InsetBy(pt);
	return copy;
}

//------------------------------------------------------------------------------

BRect BRect::InsetByCopy(float dx, float dy) const
{
	BRect copy(*this);
	copy.InsetBy(dx, dy);
	return copy;
}

//------------------------------------------------------------------------------

BRect BRect::InsetByCopy(const BInsets& d) const
{
	BRect copy(*this);
	copy.InsetBy(d);
	return copy;
}

//------------------------------------------------------------------------------

BRect & BRect::OffsetBySelf(BPoint pt)
{
	OffsetBy(pt);
	return *this;
}

//------------------------------------------------------------------------------

BRect & BRect::OffsetBySelf(float dx, float dy)
{
	OffsetBy(dx, dy);
	return *this;
}

//------------------------------------------------------------------------------

BRect BRect::OffsetByCopy(BPoint pt) const
{
	BRect copy(*this);
	copy.OffsetBy(pt);
	return copy;
}

//------------------------------------------------------------------------------

BRect BRect::OffsetByCopy(float dx, float dy) const
{
	BRect copy(*this);
	copy.OffsetBy(dx, dy);
	return copy;
}

//------------------------------------------------------------------------------

BRect & BRect::OffsetToSelf(BPoint pt)
{
	OffsetTo(pt);
	return *this;
}

//------------------------------------------------------------------------------

BRect & BRect::OffsetToSelf(float dx, float dy)
{
	OffsetTo(dx, dy);
	return *this;
}

//------------------------------------------------------------------------------

BRect BRect::OffsetToCopy(BPoint pt) const
{
	BRect copy(*this);
	copy.OffsetTo(pt);
	return copy;
}

//------------------------------------------------------------------------------

BRect BRect::OffsetToCopy(float dx, float dy) const
{
	BRect copy(*this);
	copy.OffsetTo(dx, dy);
	return copy;
}

//------------------------------------------------------------------------------

bool BRect::operator==(BRect r) const
{
	return(top == r.top &&
	       left == r.left &&
	       bottom == r.bottom &&
	       right == r.right);
}

//------------------------------------------------------------------------------

bool BRect::operator!=(BRect r) const
{
	return(top != r.top ||
	       left != r.left ||
	       bottom != r.bottom ||
	       right != r.right);
}

//------------------------------------------------------------------------------

BRect BRect::operator&(BRect r) const
{
	BRect aRect;
	
	aRect.left = max_c(left, r.left);
	aRect.top = max_c(top, r.top);
	aRect.right = min_c(right, r.right);
	aRect.bottom = min_c(bottom, r.bottom);
	
	return(aRect);
}

//------------------------------------------------------------------------------

BRect BRect::operator|(BRect r) const
{
	BRect aRect;
	
	aRect.left = min_c(left, r.left);
	aRect.top = min_c(top, r.top);
	aRect.right = max_c(right, r.right);
	aRect.bottom = max_c(bottom, r.bottom);
	
	return(aRect);
}

//------------------------------------------------------------------------------

bool BRect::Contains(BPoint pt) const
{
	return(pt.y >= top &&
	       pt.y <= bottom &&
	       pt.x >= left &&
	       pt.x <= right);
}

//------------------------------------------------------------------------------

bool BRect::Contains(BRect r) const
{
	return(left <= r.left &&
	       r.left <= r.right &&
	       r.right <= right &&
	       top <= r.top &&
	       r.top <= r.bottom &&
	       r.bottom <= bottom);
}
//  Modified by Jean-Baptiste Queru

//------------------------------------------------------------------------------

bool BRect::Intersects(BRect r) const
{
	if (top > r.bottom)
		return(FALSE);
	if (bottom < r.top)
		return(FALSE);
	if (left > r.right)
		return(FALSE);
	if (right < r.left)
		return(FALSE);

	return(TRUE);
}

//------------------------------------------------------------------------------

BInsets BRect::InsetOf(const BRect& interior) const
{
	return BInsets(interior.left-left, interior.top-top,
				   right-interior.right, bottom-interior.bottom);
}

//------------------------------------------------------------------------------

BRect BRect::operator * (const float f) const
{
	return BRect(left*f, top*f, right*f, bottom*f);
}

BRect BRect::operator / (const float f) const
{
	return operator * (1.0f/f);
}

BRect BRect::operator + (const BPoint& p) const
{
	return BRect(left+p.x, top+p.y, right+p.x, bottom+p.y);
}

BRect BRect::operator + (const BInsets& i) const
{
	return BRect(left+i.left, top+i.top, right-i.right, bottom-i.bottom);
}

BRect BRect::operator - (const BPoint& p) const
{
	return BRect(left-p.x, top-p.y, right-p.x, bottom-p.y);
}

BRect BRect::operator - (const BInsets& i) const
{
	return BRect(left-i.left, top-i.top, right+i.right, bottom+i.bottom);
}

BRect& BRect::operator *= (const float f)
{
	left *= f;
	top *= f;
	right *= f;
	bottom *= f;
	return *this;
}

BRect& BRect::operator /= (const float f)
{
	return operator *= (1.0f/f);
}

BRect& BRect::operator += (const BPoint& p)
{
	left += p.x;
	right += p.x;
	top += p.y;
	bottom += p.y;
	return *this;
}

BRect& BRect::operator += (const BInsets& i)
{
	left += i.left;
	right -= i.right;
	top += i.top;
	bottom -= i.bottom;
	return *this;
}

BRect& BRect::operator -= (const BPoint& p)
{
	left -= p.x;
	right -= p.x;
	top -= p.y;
	bottom -= p.y;
	return *this;
}

BRect& BRect::operator -= (const BInsets& i)
{
	left -= i.left;
	right += i.right;
	top -= i.top;
	bottom += i.bottom;
	return *this;
}

//------------------------------------------------------------------------------

BDataIO& operator<<(BDataIO& io, const BRect& rect)
{
#if SUPPORTS_STREAM_IO
	io << "BRect(" << rect.left << ", " << rect.top
	   << ")-(" << rect.right << ", " << rect.bottom << ")";
#else
	(void)rect;
#endif
	return io;
}

// --------- Const incorrect methods removed 5/2000 ---------

#if _R5_COMPATIBLE_
extern "C" {

	_EXPORT BRect
	#if __GNUC__
	InsetByCopy__5BRectG6BPoint
	#elif __MWERKS__
	InsetByCopy__5BRectF6BPoint
	#endif
	(BRect* This, BPoint pt)
	{
		BRect copy(*This);
		copy.InsetBy(pt);
		return copy;
	}
	
	_EXPORT BRect
	#if __GNUC__
	InsetByCopy__5BRectff
	#elif __MWERKS__
	InsetByCopy__5BRectFff
	#endif
	(BRect* This, float dx, float dy)
	{
		BRect copy(*This);
		copy.InsetBy(dx, dy);
		return copy;
	}
	
	_EXPORT BRect
	#if __GNUC__
	OffsetByCopy__5BRectG6BPoint
	#elif __MWERKS__
	OffsetByCopy__5BRectF6BPoint
	#endif
	(BRect* This, BPoint pt)
	{
		BRect copy(*This);
		copy.OffsetBy(pt);
		return copy;
	}
	
	_EXPORT BRect
	#if __GNUC__
	OffsetByCopy__5BRectff
	#elif __MWERKS__
	OffsetByCopy__5BRectFff
	#endif
	(BRect* This, float dx, float dy)
	{
		BRect copy(*This);
		copy.OffsetBy(dx, dy);
		return copy;
	}
	
	_EXPORT BRect
	#if __GNUC__
	OffsetToCopy__5BRectG6BPoint
	#elif __MWERKS__
	OffsetToCopy__5BRectF6BPoint
	#endif
	(BRect* This, BPoint pt)
	{
		BRect copy(*This);
		copy.OffsetTo(pt);
		return copy;
	}
	
	_EXPORT BRect
	#if __GNUC__
	OffsetToCopy__5BRectff
	#elif __MWERKS__
	OffsetToCopy__5BRectFff
	#endif
	(BRect* This, float dx, float dy)
	{
		BRect copy(*This);
		copy.OffsetTo(dx, dy);
		return copy;
	}
	
}
#endif

