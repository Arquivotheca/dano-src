//******************************************************************************
//
//	File:			Insets.cpp
//
//	Description:	BInsets class.
//	
//	Written by:		Dianne Hackborn
//
//	Copyright 2000, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#include <stdio.h>
#include <SupportDefs.h>

#ifndef _INSETS_H
#include "Insets.h"
#endif

#ifndef _STREAM_IO_H
#include <StreamIO.h>
#endif


//------------------------------------------------------------------------------

void BInsets::PrintToStream() const
{
#if SUPPORTS_STREAM_IO
	BOut << *this << endl;
#endif
}

//------------------------------------------------------------------------------

BInsets& BInsets::InsetBy(float dx, float dy)
{
	left += dx;
	top += dy;
	right += dx;
	bottom += dy;
	return *this;
}

//------------------------------------------------------------------------------

BInsets& BInsets::InsetBy(const BInsets& d)
{
	left += d.left;
	top += d.top;
	right += d.right;
	bottom += d.bottom;
	return *this;
}

//------------------------------------------------------------------------------

BInsets BInsets::InsetByCopy(float dx, float dy) const
{
	BInsets copy(*this);
	copy.InsetBy(dx, dy);
	return copy;
}

//------------------------------------------------------------------------------

BInsets BInsets::InsetByCopy(const BInsets& d) const
{
	BInsets copy(*this);
	copy.InsetBy(d);
	return copy;
}

//------------------------------------------------------------------------------

bool BInsets::operator==(const BInsets& r) const
{
	return(top == r.top &&
	       left == r.left &&
	       bottom == r.bottom &&
	       right == r.right);
}

//------------------------------------------------------------------------------

bool BInsets::operator!=(const BInsets& r) const
{
	return(top != r.top ||
	       left != r.left ||
	       bottom != r.bottom ||
	       right != r.right);
}

//------------------------------------------------------------------------------

BInsets BInsets::operator - () const
{
	return BInsets(-left, -top, -right, -bottom);
}

BInsets BInsets::operator * (const float f) const
{
	return BInsets(left*f, top*f, right*f, bottom*f);
}

BInsets BInsets::operator / (const float f) const
{
	return operator * (1.0f/f);
}

BInsets BInsets::operator + (const float f) const
{
	return BInsets(left+f, top+f, right+f, bottom+f);
}

BInsets BInsets::operator + (const BInsets& v) const
{
	return BInsets(left+v.left, top+v.top, right+v.right, bottom+v.bottom);
}

BInsets BInsets::operator - (const float f) const
{
	return BInsets(left-f, top-f, right-f, bottom-f);
}

BInsets BInsets::operator - (const BInsets& v) const
{
	return BInsets(left-v.left, top-v.top, right-v.right, bottom-v.bottom);
}

BInsets& BInsets::operator *= (const float f)
{
	left *= f;
	top *= f;
	right *= f;
	bottom *= f;
	return *this;
}

BInsets& BInsets::operator /= (const float f)
{
	return operator *= (1.0f/f);
}

BInsets& BInsets::operator += (const float f)
{
	left += f;
	right += f;
	top += f;
	bottom += f;
	return *this;
}

BInsets& BInsets::operator += (const BInsets& v)
{
	left += v.left;
	right += v.right;
	top += v.top;
	bottom += v.bottom;
	return *this;
}

BInsets& BInsets::operator -= (const float f)
{
	left -= f;
	right -= f;
	top -= f;
	bottom -= f;
	return *this;
}

BInsets& BInsets::operator -= (const BInsets& v)
{
	left -= v.left;
	right -= v.right;
	top -= v.top;
	bottom -= v.bottom;
	return *this;
}

//------------------------------------------------------------------------------

BDataIO& operator<<(BDataIO& io, const BInsets& rect)
{
#if SUPPORTS_STREAM_IO
	io << "BInsets(" << rect.left << ", " << rect.top
	   << ")/(" << rect.right << ", " << rect.bottom << ")";
#else
	(void)rect;
#endif
	return io;
}
