/*******************************************************************************
/
/	File:			Point.h
/
/   Description:    BRasterPoint represents a single *discrete* x,y coordinate.
/
/	Copyright 1993-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef	_RASTER2_POINT_H
#define	_RASTER2_POINT_H

#include <support2/ITextStream.h>
#include <support2/SupportDefs.h>
#include <support2/TypeFuncs.h>
#include <support2/Value.h>
#include <raster2/RasterDefs.h>

namespace B {
namespace Raster2 {
using namespace Support2;

/*----------------------------------------------------------------*/
/*----- BRasterPoint class ---------------------------------------*/

class BRasterPoint {

public:
		int32 x;
		int32 y;

		BRasterPoint();
		BRasterPoint(int32 X, int32 Y);
		BRasterPoint(const BRasterPoint& pt);
		BRasterPoint(const BValue& o, status_t* result = NULL);
		BRasterPoint(const value_ref& o, status_t* result = NULL);
		
		BRasterPoint	&operator=(const BRasterPoint &from);
		void			Set(int32 X, int32 Y);

		BValue		AsValue() const;
inline				operator BValue() const					{ return AsValue(); }

		void			ConstrainTo(const BRasterRect& rect);

		void		PrintToStream(ITextOutput::arg io, uint32 flags=0) const;
static status_t		printer(ITextOutput::arg io, const value_ref& val, uint32 flags);
		
		BRasterPoint	operator-() const;
		BRasterPoint	operator+(const BRasterPoint&) const;
		BRasterPoint	operator-(const BRasterPoint&) const;
		BRasterPoint&	operator+=(const BRasterPoint&);
		BRasterPoint&	operator-=(const BRasterPoint&);
		BRasterPoint	operator*(const float f) const;
		BRasterPoint	operator/(const float f) const;
		BRasterPoint&	operator*=(const float f);
		BRasterPoint&	operator/=(const float f);
		BRasterPoint	operator*(const int32 i) const;
		BRasterPoint&	operator*=(const int32 i);

		int32		Compare(const BRasterPoint& p) const;
		bool		operator!=(const BRasterPoint&) const;
		bool		operator==(const BRasterPoint&) const;
		bool		operator<(const BRasterPoint&) const;
		bool		operator<=(const BRasterPoint&) const;
		bool		operator>=(const BRasterPoint&) const;
		bool		operator>(const BRasterPoint&) const;

private:
		void		Construct(const value_ref& ref, status_t* result);
};

/*----- Type and STL utilities --------------------------------------*/
B_IMPLEMENT_SIMPLE_TYPE_FUNCS(BRasterPoint);
int32			BCompare(const BRasterPoint& p1, const BRasterPoint& p2);

ITextOutput::arg	operator<<(ITextOutput::arg io, const BRasterPoint& point);

/*----------------------------------------------------------------*/
/*----- inline definitions ---------------------------------------*/

inline BRasterPoint::BRasterPoint()
{
}

inline BRasterPoint::BRasterPoint(int32 X, int32 Y) : x(X), y(Y)
{
}

inline BRasterPoint::BRasterPoint(const BRasterPoint& pt) : x(pt.x), y(pt.y)
{
}

inline BRasterPoint &BRasterPoint::operator=(const BRasterPoint& from)
{
	x = from.x;
	y = from.y;
	return *this;
}

inline void BRasterPoint::Set(int32 X, int32 Y)
{
	x = X;
	y = Y;
}

inline bool
BRasterPoint::operator<(const BRasterPoint& other) const
{
	return Compare(other) < 0;
}

inline bool
BRasterPoint::operator<=(const BRasterPoint& other) const
{
	return Compare(other) <= 0;
}

inline bool
BRasterPoint::operator>=(const BRasterPoint& other) const
{
	return Compare(other) >= 0;
}

inline bool
BRasterPoint::operator>(const BRasterPoint& other) const
{
	return Compare(other) > 0;
}

inline int32 BCompare(const BRasterPoint& p1, const BRasterPoint& p2)
{
	return p1.Compare(p2);
}

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

} } // namespace B::Raster2

#endif /* _RASTER2_POINT_H */
