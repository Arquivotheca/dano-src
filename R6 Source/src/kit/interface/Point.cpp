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

#include <stdio.h>

#ifndef _POINT_H
#include "Point.h"
#endif

#ifndef _RECT_H
#include "Rect.h"
#endif

#ifndef _STREAM_IO_H
#include <StreamIO.h>
#endif

const BPoint B_ORIGIN(0,0);	// define a global constant point for the origin

//------------------------------------------------------------------------------

void BPoint::PrintToStream() const
{
#if SUPPORTS_STREAM_IO
	BOut << *this << endl;
#endif
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

bool BPoint::operator!=(const BPoint& pt) const
{
	return(x != pt.x || y != pt.y);
}

//------------------------------------------------------------------------------

bool BPoint::operator==(const BPoint& pt) const
{
	return(x == pt.x && y == pt.y);
}

//------------------------------------------------------------------------------

void BPoint::ConstrainTo(BRect rect)
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

BDataIO& operator<<(BDataIO& io, const BPoint& point)
{
#if SUPPORTS_STREAM_IO
	io << "BPoint(" << point.x << ", " << point.y << ")";
#else
	(void)point;
#endif
	return io;
}
