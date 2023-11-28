
#include <raster2/RasterRect.h>
#include <support2/ITextStream.h>

// XXX TO DO: Move BRect conversion functions in to BRect.
#include <render2/Rect.h>

namespace B {
namespace Raster2 {

BRasterRect::BRasterRect(const BRect &initFrom)
{
	left = (int32)floor(initFrom.left);
	top = (int32)floor(initFrom.top);
	right = (int32)ceil(initFrom.right);
	bottom = (int32)ceil(initFrom.bottom);
}

BRasterRect::operator BRect() const
{
	return BRect(left,top,right,bottom);
}

void
BRasterRect::Construct(const value_ref& ref, status_t *result)
{
	if (ref.type == B_RASTER_RECT_TYPE) {
		if (ref.length == sizeof(BRasterRect)) {
			*this = *static_cast<const BRasterRect*>(ref.data);
			if (result) *result = B_OK;
		} else {
			if (result) *result = B_BAD_VALUE;
			*this = BRasterRect();
		}
	} else {
		if (result) *result = B_BAD_TYPE;
		*this = BRasterRect();
	}
}

BRasterRect::BRasterRect(const BValue& o, status_t *result)
{
	Construct(value_ref(o), result);
}

BRasterRect::BRasterRect(const value_ref& o, status_t *result)
{
	Construct(o, result);
}

BValue
BRasterRect::AsValue() const
{
	return BValue(B_RASTER_RECT_TYPE, this, sizeof(BRasterRect));
}

BRasterRect &
BRasterRect::SetAsSize(int32 l, int32 t, int32 w, int32 h)
{
	left = l;
	top = t;
	right = l+w;
	bottom = t+h;
	return (*this);
}

void
BRasterRect::PrintToStream(ITextOutput::arg io, uint32 flags) const
{
	if (flags&B_PRINT_STREAM_HEADER) io << "BRasterRect(";
	else io << "(";
	io	<< left << ","
		<< top << ","
		<< right << ","
		<< bottom << ")";
}

status_t
BRasterRect::printer(ITextOutput::arg io, const value_ref& val, uint32 flags)
{
	status_t result;
	BRect obj(val, &result);
	if (result == B_OK) obj.PrintToStream(io, flags);
	return result;
}

BRasterRect &
BRasterRect::InsetBy(const BRasterPoint& p)
{
	left += p.x;
	right -= p.x;
	top += p.y;
	bottom -= p.y;
	return *this;
}

BRasterRect &
BRasterRect::OffsetBy(const BRasterPoint& p)
{
	left += p.x;
	right += p.x;
	top += p.y;
	bottom += p.y;
	return *this;
}

BRasterRect &
BRasterRect::OffsetTo(const BRasterPoint& p)
{
	right += p.x - left;
	left = p.x;
	bottom += p.y - top;
	top = p.y;
	return *this;
}

bool
BRasterRect::Contains(const BRasterPoint& p) const
{
	return
		(p.x >= left) &&
		(p.x < right) &&
		(p.y >= top) &&
		(p.y < bottom);
}

bool
BRasterRect::Contains(const BRasterRect& r) const
{
	return(left <= r.left &&
	       r.left < r.right &&
	       r.right <= right &&
	       top <= r.top &&
	       r.top < r.bottom &&
	       r.bottom <= bottom);
}

BRasterRect 
BRasterRect::InsetByCopy(const BRasterPoint& p) const
{
	return BRasterRect(*this).InsetBy(p);
}

BRasterRect 
BRasterRect::OffsetByCopy(const BRasterPoint& p) const
{
	return BRasterRect(*this).OffsetBy(p);
}

BRasterRect 
BRasterRect::OffsetToCopy(const BRasterPoint& p) const
{
	return BRasterRect(*this).OffsetTo(p);
}

int32
BRasterRect::Compare(const BRasterRect& r) const
{
	if (left != r.left) return left < r.left ? -1 : 1;
	if (top != r.top) return top < r.top ? -1 : 1;
	if (right != r.right) return right < r.right ? -1 : 1;
	if (bottom != r.bottom) return bottom < r.bottom ? -1 : 1;
	return 0;
}

bool
BRasterRect::operator==(const BRasterRect& other) const
{
	return (
		(left==other.left) &&
		(right==other.right) &&
		(top==other.top) &&
		(bottom==other.bottom)
	);
}

bool
BRasterRect::operator!=(const BRasterRect& other) const
{
	return (
		(left!=other.left) ||
		(right!=other.right) ||
		(top!=other.top) ||
		(bottom!=other.bottom)
	);
}

BRasterRect
BRasterRect::operator&(const BRasterRect& r) const
{
	BRasterRect aRect;
	
	aRect.left = max_c(left, r.left);
	aRect.top = max_c(top, r.top);
	aRect.right = min_c(right, r.right);
	aRect.bottom = min_c(bottom, r.bottom);
	
	return(aRect);
}

BRasterRect
BRasterRect::operator|(const BRasterRect& r) const
{
	BRasterRect aRect;
	
	aRect.left = min_c(left, r.left);
	aRect.top = min_c(top, r.top);
	aRect.right = max_c(right, r.right);
	aRect.bottom = max_c(bottom, r.bottom);
	
	return(aRect);
}

bool
BRasterRect::Intersects(const BRasterRect& r) const
{
	if (top >= r.bottom)
		return(false);
	if (bottom <= r.top)
		return(false);
	if (left >= r.right)
		return(false);
	if (right <= r.left)
		return(false);

	return(true);
}

//------------------------------------------------------------------------------

BRasterRect BRasterRect::operator * (const int32 f) const
{
	return BRasterRect(left*f, top*f, right*f, bottom*f);
}

BRasterRect BRasterRect::operator / (const int32 f) const
{
	return BRasterRect(left/f, top/f, right/f, bottom/f);
}

BRasterRect BRasterRect::operator + (const BRasterPoint& p) const
{
	return BRasterRect(left+p.x, top+p.y, right+p.x, bottom+p.y);
}

BRasterRect BRasterRect::operator - (const BRasterPoint& p) const
{
	return BRasterRect(left-p.x, top-p.y, right-p.x, bottom-p.y);
}

BRasterRect& BRasterRect::operator *= (const int32 f)
{
	left *= f;
	top *= f;
	right *= f;
	bottom *= f;
	return *this;
}

BRasterRect& BRasterRect::operator /= (const int32 f)
{
	left /= f;
	top /= f;
	right /= f;
	bottom /= f;
	return *this;
}

BRasterRect& BRasterRect::operator += (const BRasterPoint& p)
{
	left += p.x;
	right += p.x;
	top += p.y;
	bottom += p.y;
	return *this;
}

BRasterRect& BRasterRect::operator -= (const BRasterPoint& p)
{
	left -= p.x;
	right -= p.x;
	top -= p.y;
	bottom -= p.y;
	return *this;
}

//------------------------------------------------------------------------------

ITextOutput::arg
operator<<(ITextOutput::arg io, const BRasterRect &rect)
{
	rect.PrintToStream(io, B_PRINT_STREAM_HEADER);
	return io;
}


} }	// namespace B::Raster2
