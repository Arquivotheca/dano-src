#include <raster2/RasterPoint.h>

#include <stdio.h>
#include <math.h>
#include <raster2/RasterRect.h>
#include <support2/ITextStream.h>


namespace B {
namespace Raster2 {

void BRasterPoint::Construct(const value_ref& ref, status_t* result)
{
	if (ref.type == B_RASTER_POINT_TYPE) {
		if (ref.length == sizeof(BRasterPoint)) {
			*this = *static_cast<const BRasterPoint*>(ref.data);
			if (result) *result = B_OK;
		} else {
			if (result) *result = B_BAD_VALUE;
			*this = BRasterPoint();
		}
	} else {
		if (result) *result = B_BAD_TYPE;
		*this = BRasterPoint();
	}
}

BRasterPoint::BRasterPoint(const BValue& o, status_t *result)
{
	Construct(value_ref(o), result);
}

BRasterPoint::BRasterPoint(const value_ref& o, status_t *result)
{
	Construct(o, result);
}

BValue BRasterPoint::AsValue() const
{
	return BValue(B_RASTER_POINT_TYPE, this, sizeof(BRasterPoint));
}

//------------------------------------------------------------------------------

void BRasterPoint::PrintToStream(ITextOutput::arg io, uint32 flags) const
{
	if (flags&B_PRINT_STREAM_HEADER) io << "BRasterPoint(";
	else io << "(";
	io	<< x << "," << y << ")";
}

status_t BRasterPoint::printer(ITextOutput::arg io, const value_ref& val, uint32 flags)
{
	status_t result;
	BRasterPoint obj(val, &result);
	if (result == B_OK) obj.PrintToStream(io, flags);
	return result;
}

//------------------------------------------------------------------------------

BRasterPoint BRasterPoint::operator-() const
{
	return BRasterPoint(-x, -y);
}

//------------------------------------------------------------------------------

BRasterPoint BRasterPoint::operator+(const BRasterPoint& pt) const
{
	return BRasterPoint(x+pt.x, y+pt.y);
}

//------------------------------------------------------------------------------

BRasterPoint BRasterPoint::operator-(const BRasterPoint& pt) const
{
	return BRasterPoint(x-pt.x, y-pt.y);
}

//------------------------------------------------------------------------------

BRasterPoint& BRasterPoint::operator+=(const BRasterPoint& pt)
{
	x += pt.x;
	y += pt.y;

	return(*this);
}

//------------------------------------------------------------------------------

BRasterPoint& BRasterPoint::operator-=(const BRasterPoint& pt)
{
	x -= pt.x;
	y -= pt.y;

	return(*this);
}

//------------------------------------------------------------------------------

BRasterPoint BRasterPoint::operator*(const float f) const
{
	return BRasterPoint((int32)floor(x*f+0.5f), (int32)floor(y*f+0.5f));
}

BRasterPoint BRasterPoint::operator/(const float f) const
{
	return operator * (1.0f/f);
}

BRasterPoint& BRasterPoint::operator*=(const float f)
{
	return (*this = operator*(f));
}

BRasterPoint& BRasterPoint::operator/=(const float f)
{
	return operator *= (1.0f/f);
}


BRasterPoint BRasterPoint::operator*(const int32 i) const
{
	return BRasterPoint(x*i, y*i);
}

BRasterPoint& BRasterPoint::operator*=(const int32 i)
{
	return (*this = operator*(i));
}


//------------------------------------------------------------------------------

int32 BRasterPoint::Compare(const BRasterPoint& pt) const
{
	if (operator == (pt)) return 0;
	return ((x*x+y*y) < (pt.x*pt.x+pt.y*pt.y)) ? -1 : 1;	
//	if (x != pt.x) return x < pt.x ? -1 : 1;
//	if (y != pt.y) return y < pt.y ? -1 : 1;
//	return 0;
}

bool BRasterPoint::operator!=(const BRasterPoint& pt) const
{
	return(x != pt.x || y != pt.y);
}

bool BRasterPoint::operator==(const BRasterPoint& pt) const
{
	return(x == pt.x && y == pt.y);
}

//------------------------------------------------------------------------------

void BRasterPoint::ConstrainTo(const BRasterRect& rect)
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

ITextOutput::arg
operator<<(ITextOutput::arg io, const BRasterPoint &point)
{
	point.PrintToStream(io, B_PRINT_STREAM_HEADER);
	return io;
}

} }	// namespace B::Raster2
