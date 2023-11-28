//******************************************************************************
//
//	File:		Point.cpp
//
//	Description:	BPoint class implementation.
//	
//	Written by:	Steve Horowitz
//
//	Copyright 1993, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#include <render2/Point.h>

#include <stdio.h>
#include <render2/Rect.h>
#include <support2/ITextStream.h>

namespace B {
namespace Render2 {

const BPoint B_ORIGIN(0,0);	// define a global constant point for the origin

void BPoint::Construct(const value_ref& ref, status_t* result)
{
	if (ref.type == B_POINT_TYPE) {
		if (ref.length == sizeof(BPoint)) {
			*this = *static_cast<const BPoint*>(ref.data);
			if (result) *result = B_OK;
		} else {
			if (result) *result = B_BAD_VALUE;
			*this = BPoint();
		}
	} else {
		if (result) *result = B_BAD_TYPE;
		*this = BPoint();
	}
}

BPoint::BPoint(const BValue& o, status_t *result)
{
	Construct(value_ref(o), result);
}

BPoint::BPoint(const value_ref& o, status_t *result)
{
	Construct(o, result);
}

BValue BPoint::AsValue() const
{
	return BValue(B_POINT_TYPE, this, sizeof(BPoint));
}

//------------------------------------------------------------------------------

void BPoint::PrintToStream(ITextOutput::arg io, uint32 flags) const
{
	if (flags&B_PRINT_STREAM_HEADER) io << "BPoint(";
	else io << "(";
	io << x << ", " << y << ")";
}

status_t BPoint::printer(ITextOutput::arg io, const value_ref& val, uint32 flags)
{
	status_t result;
	BPoint obj(val, &result);
	if (result == B_OK) obj.PrintToStream(io, flags);
	return result;
}

//------------------------------------------------------------------------------

BPoint BPoint::operator-() const
{
	return BPoint(-x, -y);
}

//------------------------------------------------------------------------------

BPoint BPoint::operator+(const BPoint& pt) const
{
	return BPoint(x+pt.x, y+pt.y);
}

//------------------------------------------------------------------------------

BPoint BPoint::operator-(const BPoint& pt) const
{
	return BPoint(x-pt.x, y-pt.y);
}

//------------------------------------------------------------------------------

BPoint& BPoint::operator+=(const BPoint& pt)
{
	x += pt.x;
	y += pt.y;

	return(*this);
}

//------------------------------------------------------------------------------

BPoint& BPoint::operator-=(const BPoint& pt)
{
	x -= pt.x;
	y -= pt.y;

	return(*this);
}

//------------------------------------------------------------------------------

BPoint BPoint::operator*(const float f) const
{
	return BPoint(x*f, y*f);
}

BPoint BPoint::operator/(const float f) const
{
	return operator * (1.0f/f);
}

BPoint& BPoint::operator*=(const float f)
{
	x*=f;
	y*=f;
	return (*this);
}

BPoint& BPoint::operator/=(const float f)
{
	return operator *= (1.0f/f);
}


//------------------------------------------------------------------------------

int32 BPoint::Compare(const BPoint& pt) const
{
	if (operator == (pt)) return 0;
	return ((x*x+y*y) < (pt.x*pt.x+pt.y*pt.y)) ? -1 : 1;	
//	if (x != pt.x) return x < pt.x ? -1 : 1;
//	if (y != pt.y) return y < pt.y ? -1 : 1;
//	return 0;
}

bool BPoint::operator!=(const BPoint& pt) const
{
	return(x != pt.x || y != pt.y);
}

bool BPoint::operator==(const BPoint& pt) const
{
	return(x == pt.x && y == pt.y);
}

//------------------------------------------------------------------------------

void BPoint::ConstrainTo(const BRect& rect)
{
	if (x < rect.left)
		x = rect.left;
	else
		if (x > rect.right)
			x = rect.right;

	if (y < rect.top)
		y = rect.top;
	else
		if (y > rect.bottom)
			y = rect.bottom;
}

//------------------------------------------------------------------------------

ITextOutput::arg operator<<(ITextOutput::arg io, const BPoint& point)
{
	point.PrintToStream(io, B_PRINT_STREAM_HEADER);
	return io;
}

} }	// namespace B::Render2
